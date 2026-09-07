"""Echoes ArtSource skeletal kit: rigid-bound skeletons and keyframed clips for glTF export.

Author: Angelis Pseftis.

Pure Python 3, standard library only. Extends ebs_meshkit.Mesh with a bone hierarchy,
rigid per-polygon bone binding, and keyframed animation clips, and writes a skinned
glTF 2.0 binary (skin + JOINTS_0/WEIGHTS_0 + animations) for the installed Unreal
5.8.2 Interchange importer. Conventions:

  * Authoring frame is the Unreal frame (+X forward, +Y right, +Z up, centimetres).
  * The rest pose is the bind pose: every bone's rest orientation is identity (bone
    axes parallel to the world axes) and only its head position differs, so a clip's
    per-bone rotation is an Unreal rotator (pitch about +Y, yaw about +Z, roll about +X)
    applied at the bone head, composed down the hierarchy.
  * Keyframe rotations are Unreal rotators in degrees; translations are centimetre
    offsets from the rest head, in the parent's frame.
  * The glTF frame mapping and the quaternion/axis encodings for joints and animation
    samplers are established empirically against the installed importer by
    ArtSource/tools/skeletal_probe.py (see SKELETAL_ENCODING below) — do not assume
    the socket encoding of ebs_meshkit carries over.

The API below is the contract the asset builders code against; write_skinned_glb is the
single entry point.

Encoding summary (measured, see SKELETAL_ENCODING at the bottom of this file):

  * Positions, joint translations, socket translations and animation translations all
    use the ebs_meshkit mapping glTF = (X_ue, Z_ue, Y_ue) / 100. The GLTFCore reader
    converts them back with ConvertVec3 = (X, Z, Y) and multiplies by 100.
  * Every rotation (joint rest, animation key, socket) is the Unreal quaternion of the
    rotator (FRotator::Quaternion formula, reproduced in ue_quat_from_rotator) written as
    glTF (-x, -z, -y, w); the reader's ConvertQuat maps it back to (x, y, z, w). Skeletal
    sockets do NOT get the static-mesh reflection treatment: the skeletal mesh factory
    reads the socket node's local transform verbatim (InterchangeSkeletalMeshFactory.cpp
    EndImportAssetObject_GameThread) and no AxisConversionInverseTransform is applied.
  * inverseBindMatrices are translation-only (the joint's global rest head, negated);
    the reader derives the local bind pose as GlobalBind * ParentGlobalInverseBind, which
    equals the joint node's local translation, so the rest pose and bind pose coincide.
"""
from __future__ import annotations

import json
import math
import struct
from dataclasses import dataclass, field

import ebs_meshkit as kit

AUTHOR = "Angelis Pseftis"
SKEL_REVISION = "ebs-skelkit-v2"  # v2: clip durations must land on a whole 30 fps frame (probe-verified; see clip_duration below)

ANIMATION_FPS = 30.0
"""Frame rate the Interchange skeletal import samples glTF animation at.

VERIFIED 2026-09-07 against UE 5.8.2 by direct probe (evidence:
BuildArtifacts/Evidence/asset-production-20260906T221157Z/skeletal-clip-duration-probe/): a clip whose
duration is NOT an integer number of frames at this rate is SILENTLY DROPPED — no AnimSequence is
created, no warning is logged, and the import still reports success. Twelve probe clips: 1, 6, 8, 9,
12, 23, 24 and 30 frames all imported with exact length; 1.5, 7.5, 10.5 and 22.5 frames all vanished.
Three production packages had each lost a clip this way before the rule was found."""


def frames_for(duration_s: float, fps: float = ANIMATION_FPS) -> float:
    """Frame count of a duration at the import sampling rate (may be fractional)."""
    return duration_s * fps


def is_frame_aligned(duration_s: float, fps: float = ANIMATION_FPS, tolerance: float = 1e-6) -> bool:
    frames = frames_for(duration_s, fps)
    return abs(frames - round(frames)) <= tolerance


def frame_aligned_duration(duration_s: float, fps: float = ANIMATION_FPS) -> float:
    """The next whole frame at or after ``duration_s`` (never zero-length unless the input is 0)."""
    if duration_s <= 0.0:
        return 0.0
    return max(1.0, math.ceil(frames_for(duration_s, fps) - 1e-9)) / fps


def retime_clip(clip: "AnimationClip", duration_s: float) -> "AnimationClip":
    """Scale every key time so the clip runs to ``duration_s`` with its shape unchanged.

    Used to snap an authored duration onto a frame boundary: the pose at any normalized time is
    identical afterwards, so posed review stills and their hashes do not move."""
    if clip.duration_s <= 0.0 or duration_s <= 0.0:
        clip.duration_s = duration_s
        return clip
    scale = duration_s / clip.duration_s
    for keys in clip.tracks.values():
        for k in keys:
            k.time_s = k.time_s * scale
    clip.duration_s = duration_s
    return clip


@dataclass
class Bone:
    name: str
    parent: str | None
    head: tuple  # rest position in centimetres, Unreal frame
    purpose: str = ""


@dataclass
class Skeleton:
    root: str
    bones: list = field(default_factory=list)

    def add(self, name: str, parent: str | None, head, purpose: str = "") -> "Bone":
        if any(b.name == name for b in self.bones):
            raise ValueError(f"duplicate bone {name}")
        if parent is not None and not any(b.name == parent for b in self.bones):
            raise ValueError(f"parent {parent} must be added before {name}")
        bone = Bone(name, parent, tuple(float(c) for c in head), purpose)
        self.bones.append(bone)
        return bone

    def get(self, name: str) -> Bone:
        for b in self.bones:
            if b.name == name:
                return b
        raise KeyError(name)

    def index(self, name: str) -> int:
        for i, b in enumerate(self.bones):
            if b.name == name:
                return i
        raise KeyError(name)

    def children(self, name: str) -> list:
        return [b.name for b in self.bones if b.parent == name]

    def chain_to_root(self, name: str) -> list:
        out = []
        cur = self.get(name)
        while cur is not None:
            out.append(cur.name)
            cur = self.get(cur.parent) if cur.parent else None
        return out


@dataclass
class Keyframe:
    time_s: float
    rotation_deg: tuple = (0.0, 0.0, 0.0)   # Unreal rotator (pitch, yaw, roll)
    translation_cm: tuple = (0.0, 0.0, 0.0)  # offset from the rest head, parent frame


@dataclass
class AnimationClip:
    name: str
    duration_s: float
    tracks: dict = field(default_factory=dict)  # bone name -> list[Keyframe] sorted by time
    loop: bool = False
    purpose: str = ""

    def key(self, bone: str, time_s: float, rotation_deg=(0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, 0.0)):
        self.tracks.setdefault(bone, []).append(Keyframe(float(time_s), tuple(rotation_deg), tuple(translation_cm)))
        self.tracks[bone].sort(key=lambda k: k.time_s)


def bind_polygons(mesh: kit.Mesh, default_bone: str, by_component: dict) -> dict:
    """Rigidly bind every polygon to one bone: ``by_component`` maps a component-name
    prefix to a bone name; unmatched polygons use ``default_bone``. Sets
    ``polygon.bone`` and returns {bone: triangle_count}."""
    counts = {}
    for poly in mesh.polygons:
        bone = default_bone
        for prefix, name in by_component.items():
            if poly.component == prefix or poly.component.startswith(prefix):
                bone = name
                break
        poly.bone = bone
        counts[bone] = counts.get(bone, 0) + len(poly.points) - 2
    return counts


# --- Unreal rotator maths -----------------------------------------------------
def rot_x(p, roll_deg):
    """Unreal roll: positive roll about +X takes +Z toward +Y and +Y toward -Z
    (FRotationMatrix rows for roll 90: Y -> (0,0,-1), Z -> (0,1,0))."""
    c, s = math.cos(math.radians(roll_deg)), math.sin(math.radians(roll_deg))
    return (p[0], p[1] * c + p[2] * s, -p[1] * s + p[2] * c)


def rot_rotator(p, pitch_deg=0.0, yaw_deg=0.0, roll_deg=0.0):
    """Apply an Unreal rotator: roll about +X, then pitch about +Y, then yaw about +Z
    (ebs_meshkit.rot_yp extended with roll)."""
    q = rot_x(p, roll_deg) if roll_deg else p
    return kit.rot_yp(q, yaw_deg, pitch_deg)


def ue_quat_from_rotator(pitch_deg: float, yaw_deg: float, roll_deg: float):
    """Unreal FRotator::Quaternion (UnrealMath.cpp scalar path), (x, y, z, w)."""
    sp, cp = math.sin(math.radians(math.fmod(pitch_deg, 360.0)) / 2.0), math.cos(math.radians(math.fmod(pitch_deg, 360.0)) / 2.0)
    sy, cy = math.sin(math.radians(math.fmod(yaw_deg, 360.0)) / 2.0), math.cos(math.radians(math.fmod(yaw_deg, 360.0)) / 2.0)
    sr, cr = math.sin(math.radians(math.fmod(roll_deg, 360.0)) / 2.0), math.cos(math.radians(math.fmod(roll_deg, 360.0)) / 2.0)
    return (
        cr * sp * sy - sr * cp * cy,
        -cr * sp * cy - sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy,
    )


def ue_quat_to_gltf(q):
    """Inverse of GLTFCore ConvertQuat (UE = (-x, -z, -y, w) of glTF): glTF = (-x, -z, -y, w) of UE."""
    return (-q[0], -q[2], -q[1], q[3])


def rotator_gltf_quaternion(pitch_deg: float, yaw_deg: float, roll_deg: float):
    """glTF node/sampler quaternion that imports as the given Unreal rotator (joints,
    animation keys and skeletal sockets all use this one mapping; see SKELETAL_ENCODING)."""
    return ue_quat_to_gltf(ue_quat_from_rotator(pitch_deg, yaw_deg, roll_deg))


def ue_quat_rotate(q, v):
    """Rotate vector ``v`` by the Unreal quaternion ``q`` (x, y, z, w): the FQuatRotationMatrix
    rows, i.e. the same result as rot_rotator for the quaternion's rotator."""
    x, y, z, w = q
    return (
        v[0] * (1 - 2 * (y * y + z * z)) + v[1] * (2 * (x * y - w * z)) + v[2] * (2 * (x * z + w * y)),
        v[0] * (2 * (x * y + w * z)) + v[1] * (1 - 2 * (x * x + z * z)) + v[2] * (2 * (y * z - w * x)),
        v[0] * (2 * (x * z - w * y)) + v[1] * (2 * (y * z + w * x)) + v[2] * (1 - 2 * (x * x + y * y)),
    )


def ue_quat_compose(outer, inner):
    """Unreal FQuat product ``outer * inner``: applies ``inner`` first, then ``outer``
    (Hamilton product, ebs_meshkit.quat_mul)."""
    return kit.quat_mul(outer, inner)


def _normalize_axis(deg: float) -> float:
    deg = math.fmod(deg, 360.0)
    if deg > 180.0:
        deg -= 360.0
    elif deg < -180.0:
        deg += 360.0
    return deg


def ue_rotator_from_quat(q):
    """Unreal FQuat::Rotator (UnrealMath.cpp double path), returning (pitch, yaw, roll) degrees.
    At the pitch singularity (|pitch| = 90) Unreal folds the remaining rotation into yaw and
    reports roll 0, exactly like the engine."""
    x, y, z, w = q
    singularity = z * x - w * y
    yaw_y = 2.0 * (w * z + x * y)
    yaw_x = 1.0 - 2.0 * (y * y + z * z)
    threshold = 0.4999995
    if singularity < -threshold:
        pitch = -90.0
        yaw = _normalize_axis(math.degrees(-2.0 * math.atan2(x, w)))
        roll = 0.0
    elif singularity > threshold:
        pitch = 90.0
        yaw = _normalize_axis(math.degrees(2.0 * math.atan2(x, w)))
        roll = 0.0
    else:
        pitch = math.degrees(math.asin(max(-1.0, min(1.0, 2.0 * singularity))))
        yaw = math.degrees(math.atan2(yaw_y, yaw_x))
        roll = math.degrees(math.atan2(-2.0 * (w * x + y * z), 1.0 - 2.0 * (x * x + y * y)))
    return (pitch, yaw, roll)


def _to_gltf_pos(p):
    return (p[0] / 100.0, p[2] / 100.0, p[1] / 100.0)


def _to_gltf_dir(n):
    return (n[0], n[2], n[1])


def _f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def _pose_entry(value):
    """Normalise a pose value to ((pitch, yaw, roll), (tx, ty, tz))."""
    vals = tuple(float(c) for c in value)
    if len(vals) == 3:
        return vals, (0.0, 0.0, 0.0)
    if len(vals) == 6:
        return vals[:3], vals[3:]
    raise ValueError("pose entries are (pitch, yaw, roll) or (pitch, yaw, roll, tx, ty, tz)")


def pose_mesh(mesh: kit.Mesh, skeleton: Skeleton, pose: dict) -> kit.Mesh:
    """Return a new Mesh with the pose (bone -> (pitch, yaw, roll) degrees, optional
    translation via a 6-tuple) baked into the vertices, for review renders of key poses.
    Rotation is applied at each bone head and composed from the root down.

    Each bone's local transform is: translate to its head, rotate by the Unreal rotator
    (roll about +X, then pitch about +Y, then yaw about +Z), translate back plus the
    key translation. A point bound to bone B is transformed by T_B first, then by the
    parent's transform, up to the root (standard forward kinematics, parent-first
    composition). The optional pose key ``"_sockets"`` maps socket name -> bone; those
    sockets are moved the same way. Their orientation is composed exactly as a chain
    quaternion (socket yaw first, then the bone rotators up to the root): the posed
    Socket's ``yaw_deg`` is the yaw of the composed Unreal rotator, and because
    ebs_meshkit.Socket cannot carry pitch/roll, the full composed rotator and quaternion
    are attached to the posed Socket instance as ``socket.rotation_deg`` (pitch, yaw, roll)
    and ``socket.quaternion`` (x, y, z, w) - read those for review renders whenever any
    bone in the chain has pitch or roll; ``yaw_deg`` alone is only exact when the chain
    has none. Sockets not listed in ``"_sockets"`` are copied unchanged."""
    socket_bones = pose.get("_sockets", {}) or {}
    transforms = {}
    for bone in skeleton.bones:
        rot, tr = _pose_entry(pose.get(bone.name, (0.0, 0.0, 0.0)))
        transforms[bone.name] = (rot, tr, bone.head)

    def apply_point(p, bone_name):
        for name in skeleton.chain_to_root(bone_name):
            (pitch, yaw, roll), tr, head = transforms[name]
            local = kit.v_sub(p, head)
            local = rot_rotator(local, pitch, yaw, roll)
            p = kit.v_add(kit.v_add(local, head), tr)
        return p

    def apply_dir(n, bone_name):
        for name in skeleton.chain_to_root(bone_name):
            (pitch, yaw, roll), _tr, _head = transforms[name]
            n = rot_rotator(n, pitch, yaw, roll)
        return n

    def chain_quaternion(bone_name, inner_quaternion):
        """Compose ``inner`` (applied first) with the chain's rotators, bone first, root last."""
        q = inner_quaternion
        for name in skeleton.chain_to_root(bone_name):
            (pitch, yaw, roll), _tr, _head = transforms[name]
            q = ue_quat_compose(ue_quat_from_rotator(pitch, yaw, roll), q)
        return q

    out = kit.Mesh(mesh.name, slots=list(mesh.slots))
    for poly in mesh.polygons:
        bone_name = getattr(poly, "bone", None) or skeleton.root
        pts = [apply_point(p, bone_name) for p in poly.points]
        normal = kit.v_norm(apply_dir(poly.normal, bone_name))
        uv_axis = None
        if poly.uv_axis is not None:
            uv_axis = (apply_dir(poly.uv_axis[0], bone_name), apply_dir(poly.uv_axis[1], bone_name))
        new_poly = kit.Polygon(pts, normal, poly.slot, poly.component, uv_axis,
                               list(poly.uv_override) if poly.uv_override is not None else None,
                               poly.atlas_cells, poly.chart_id)
        new_poly.bone = bone_name
        out.polygons.append(new_poly)
    for socket in mesh.sockets:
        bone_name = socket_bones.get(socket.name)
        if bone_name is None:
            out.sockets.append(kit.Socket(socket.name, tuple(socket.position), socket.yaw_deg, socket.purpose,
                                          socket.raw_gltf_rotation, socket.raw_gltf_scale))
            continue
        quaternion = chain_quaternion(bone_name, ue_quat_from_rotator(0.0, socket.yaw_deg, 0.0))
        rotation = ue_rotator_from_quat(quaternion)
        posed = kit.Socket(socket.name, apply_point(socket.position, bone_name), rotation[1], socket.purpose,
                           socket.raw_gltf_rotation, socket.raw_gltf_scale)
        posed.rotation_deg = rotation
        posed.quaternion = quaternion
        out.sockets.append(posed)
    out.collision = [kit.CollisionBox(b.name, tuple(b.center), tuple(b.size)) for b in mesh.collision]
    return out


def write_skinned_glb(mesh: kit.Mesh, skeleton: Skeleton, path: str, animations=None, extras=None,
                      include_collision: bool = True, sockets_on_bones: dict | None = None) -> str:
    """Write a skinned glTF 2.0 binary.

    ``sockets_on_bones`` maps a socket name (from mesh.sockets) to the bone it attaches
    to; the socket node is written as a child of that joint (Interchange skeletal-mesh
    sockets are leaf nodes named SOCKET_<name> under the skeleton root).
    Returns the file's SHA-256.

    Layout: node 0 is the skinned mesh node (mesh 0, skin 0, a scene root); one joint
    node per bone in ``skeleton.bones`` order, parented by the bone hierarchy with the
    root bone as a scene root; socket nodes are leaf children of their bone's joint;
    UBX_ collision box nodes (``include_collision``) are extra scene roots exactly as in
    ebs_meshkit.Mesh.write_glb. Vertices bind rigidly (weight 1.0) to ``polygon.bone``
    (default: the skeleton root). Animations: one glTF animation per AnimationClip, a
    rotation sampler per keyed bone plus a translation sampler when any key translates,
    LINEAR interpolation, times in seconds."""
    animations = list(animations or [])
    unaligned = [(c.name, c.duration_s, round(frames_for(c.duration_s), 3)) for c in animations if not is_frame_aligned(c.duration_s)]
    if unaligned:
        detail = "; ".join(f"{n} {d:.4f} s = {f} frames" for n, d, f in unaligned)
        raise ValueError(
            f"{path}: clip duration is not a whole frame at {ANIMATION_FPS:g} fps and the Interchange skeletal "
            f"import would drop it without a warning ({detail}). Snap the duration with "
            f"frame_aligned_duration() and rescale the keys with retime_clip(), or author a whole-frame duration.")
    sockets_on_bones = dict(sockets_on_bones or {})
    if not skeleton.bones:
        raise ValueError("skeleton has no bones")
    if skeleton.get(skeleton.root).parent is not None:
        raise ValueError("skeleton root must have no parent")
    for name in sockets_on_bones.values():
        skeleton.get(name)  # KeyError for unknown bones
    for clip in animations:
        for bone_name in clip.tracks:
            skeleton.get(bone_name)

    buffer = bytearray()
    buffer_views = []
    accessors = []

    def push(data: bytes, target: int | None):
        while len(buffer) % 4:
            buffer.append(0)
        offset = len(buffer)
        buffer.extend(data)
        view = {"buffer": 0, "byteOffset": offset, "byteLength": len(data)}
        if target is not None:
            view["target"] = target
        buffer_views.append(view)
        return len(buffer_views) - 1

    def accessor(view, count, ctype, atype, min_max=None):
        acc = {"bufferView": view, "componentType": ctype, "count": count, "type": atype}
        if min_max:
            acc["min"], acc["max"] = min_max
        accessors.append(acc)
        return len(accessors) - 1

    bone_index = {b.name: i for i, b in enumerate(skeleton.bones)}
    joint_ctype = 5121 if len(skeleton.bones) <= 256 else 5123
    joint_fmt = "<4B" if joint_ctype == 5121 else "<4H"

    # --- mesh primitives (ebs_meshkit encoding + JOINTS_0/WEIGHTS_0) ---------------
    tri_bones = []
    for poly in mesh.polygons:
        bone_name = getattr(poly, "bone", None) or skeleton.root
        if bone_name not in bone_index:
            raise KeyError(f"polygon {poly.component} bound to unknown bone {bone_name}")
        tri_bones.extend([bone_name] * (len(poly.points) - 2))
    per_slot = {i: [] for i in range(len(mesh.slots))}
    for tri, bone_name in zip(mesh.triangles(), tri_bones):
        per_slot[tri[4]].append((tri, bone_name))
    used_slots = [i for i in range(len(mesh.slots)) if per_slot[i]]
    materials = [{"name": mesh.slots[i], "pbrMetallicRoughness": {"baseColorFactor": [0.8, 0.8, 0.8, 1.0], "metallicFactor": 0.0, "roughnessFactor": 0.7}, "doubleSided": False} for i in used_slots]
    material_index = {slot: k for k, slot in enumerate(used_slots)}
    primitives = []
    bone_vertex_counts = {b.name: 0 for b in skeleton.bones}
    for slot_index in used_slots:
        pos_bytes, nrm_bytes, uv0_bytes, uv1_bytes, idx_bytes = bytearray(), bytearray(), bytearray(), bytearray(), bytearray()
        jnt_bytes, wgt_bytes = bytearray(), bytearray()
        mins = [float("inf")] * 3
        maxs = [float("-inf")] * 3
        vertex = 0
        for ((p0, p1, p2, n, _slot, _component, uv0, uv1), bone_name) in per_slot[slot_index]:
            g = [_to_gltf_pos(p0), _to_gltf_pos(p1), _to_gltf_pos(p2)]
            gn = _to_gltf_dir(n)
            order = (0, 1, 2)
            if kit.v_dot(kit.v_cross(kit.v_sub(g[1], g[0]), kit.v_sub(g[2], g[0])), gn) < 0:
                order = (0, 2, 1)
            j = bone_index[bone_name]
            for k in order:
                p = g[k]
                pos_bytes += struct.pack("<3f", *p)
                nrm_bytes += struct.pack("<3f", *gn)
                uv0_bytes += struct.pack("<2f", *uv0[k])
                uv1_bytes += struct.pack("<2f", *uv1[k])
                jnt_bytes += struct.pack(joint_fmt, j, 0, 0, 0)
                wgt_bytes += struct.pack("<4f", 1.0, 0.0, 0.0, 0.0)
                for axis in range(3):
                    mins[axis] = min(mins[axis], p[axis])
                    maxs[axis] = max(maxs[axis], p[axis])
                idx_bytes += struct.pack("<I", vertex)
                vertex += 1
                bone_vertex_counts[bone_name] += 1
        pos_view = push(bytes(pos_bytes), 34962)
        nrm_view = push(bytes(nrm_bytes), 34962)
        uv0_view = push(bytes(uv0_bytes), 34962)
        uv1_view = push(bytes(uv1_bytes), 34962)
        jnt_view = push(bytes(jnt_bytes), 34962)
        wgt_view = push(bytes(wgt_bytes), 34962)
        idx_view = push(bytes(idx_bytes), 34963)
        fmin = [_f32(m) for m in mins]
        fmax = [_f32(m) for m in maxs]
        primitives.append({
            "attributes": {
                "POSITION": accessor(pos_view, vertex, 5126, "VEC3", (fmin, fmax)),
                "NORMAL": accessor(nrm_view, vertex, 5126, "VEC3"),
                "TEXCOORD_0": accessor(uv0_view, vertex, 5126, "VEC2"),
                "TEXCOORD_1": accessor(uv1_view, vertex, 5126, "VEC2"),
                "JOINTS_0": accessor(jnt_view, vertex, joint_ctype, "VEC4"),
                "WEIGHTS_0": accessor(wgt_view, vertex, 5126, "VEC4"),
            },
            "indices": accessor(idx_view, vertex, 5125, "SCALAR"),
            "material": material_index[slot_index],
            "mode": 4,
        })
    meshes = [{"name": mesh.name, "primitives": primitives}]
    nodes = [{"name": mesh.name, "mesh": 0, "skin": 0}]
    root_children = [0]

    # --- joints -----------------------------------------------------------------
    joint_node_index = {}
    for bone in skeleton.bones:
        parent_head = skeleton.get(bone.parent).head if bone.parent else (0.0, 0.0, 0.0)
        local = kit.v_sub(bone.head, parent_head)
        node = {"name": bone.name,
                "translation": [kit._r(c) for c in _to_gltf_pos(local)],
                "extras": {"purpose": bone.purpose, "unreal_head_cm": [kit._r(c) for c in bone.head],
                           "bound_vertices": bone_vertex_counts[bone.name]}}
        nodes.append(node)
        joint_node_index[bone.name] = len(nodes) - 1
    for bone in skeleton.bones:
        children = [joint_node_index[c] for c in skeleton.children(bone.name)]
        if children:
            nodes[joint_node_index[bone.name]]["children"] = children
    root_children.append(joint_node_index[skeleton.root])

    # --- sockets (leaf children of their bone's joint) --------------------------
    socket_nodes = {}
    for socket in mesh.sockets:
        bone_name = sockets_on_bones.get(socket.name)
        if bone_name is None:
            continue
        head = skeleton.get(bone_name).head
        node = {
            "name": f"SOCKET_{socket.name}",
            "translation": [kit._r(c) for c in _to_gltf_pos(kit.v_sub(socket.position, head))],
            "rotation": [kit._r(c) for c in (socket.raw_gltf_rotation or rotator_gltf_quaternion(0.0, socket.yaw_deg, 0.0))],
            "extras": {"purpose": socket.purpose, "unreal_yaw_deg": socket.yaw_deg, "bone": bone_name},
        }
        if socket.raw_gltf_scale and tuple(socket.raw_gltf_scale) != (1.0, 1.0, 1.0):
            node["scale"] = [kit._r(c) for c in socket.raw_gltf_scale]
        nodes.append(node)
        socket_nodes[socket.name] = len(nodes) - 1
        nodes[joint_node_index[bone_name]].setdefault("children", []).append(len(nodes) - 1)

    # --- skin ---------------------------------------------------------------------
    ibm_bytes = bytearray()
    for bone in skeleton.bones:
        gx, gy, gz = _to_gltf_pos(bone.head)
        # column-major identity with the negated global rest head in the last column
        ibm_bytes += struct.pack("<16f", 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -gx, -gy, -gz, 1)
    ibm_view = push(bytes(ibm_bytes), None)
    skins = [{"name": f"{mesh.name}_skin",
              "joints": [joint_node_index[b.name] for b in skeleton.bones],
              "skeleton": joint_node_index[skeleton.root],
              "inverseBindMatrices": accessor(ibm_view, len(skeleton.bones), 5126, "MAT4")}]

    # --- animations ---------------------------------------------------------------
    gltf_animations = []
    for clip in animations:
        samplers, channels = [], []
        for bone_name in sorted(clip.tracks):
            keys = sorted(clip.tracks[bone_name], key=lambda k: k.time_s)
            if not keys:
                continue
            bone = skeleton.get(bone_name)
            parent_head = skeleton.get(bone.parent).head if bone.parent else (0.0, 0.0, 0.0)
            local_rest = kit.v_sub(bone.head, parent_head)
            times = [float(k.time_s) for k in keys]
            time_view = push(b"".join(struct.pack("<f", t) for t in times), None)
            time_acc = accessor(time_view, len(times), 5126, "SCALAR", ([_f32(min(times))], [_f32(max(times))]))
            rot_bytes = b"".join(struct.pack("<4f", *rotator_gltf_quaternion(*k.rotation_deg)) for k in keys)
            rot_acc = accessor(push(rot_bytes, None), len(keys), 5126, "VEC4")
            samplers.append({"input": time_acc, "interpolation": "LINEAR", "output": rot_acc})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": joint_node_index[bone_name], "path": "rotation"}})
            if any(any(abs(c) > 1e-9 for c in k.translation_cm) for k in keys):
                tr_bytes = b"".join(struct.pack("<3f", *_to_gltf_pos(kit.v_add(local_rest, k.translation_cm))) for k in keys)
                tr_acc = accessor(push(tr_bytes, None), len(keys), 5126, "VEC3")
                samplers.append({"input": time_acc, "interpolation": "LINEAR", "output": tr_acc})
                channels.append({"sampler": len(samplers) - 1, "target": {"node": joint_node_index[bone_name], "path": "translation"}})
        gltf_animations.append({"name": clip.name, "samplers": samplers, "channels": channels,
                                "extras": {"duration_s": clip.duration_s, "loop": clip.loop, "purpose": clip.purpose}})

    # --- collision boxes (UBX_, exactly as ebs_meshkit) ---------------------------
    for index, box in enumerate(mesh.collision if include_collision else []):
        box_mesh = kit.Mesh(f"UBX_{mesh.name}_{index + 1:02d}")
        box_mesh.slot("Collision")
        box_mesh.box(box.center, box.size, 0, "collision")
        pos_bytes, idx_bytes = bytearray(), bytearray()
        mins = [float("inf")] * 3
        maxs = [float("-inf")] * 3
        vertex = 0
        for (p0, p1, p2, n, _s, _c, _u0, _u1) in box_mesh.triangles():
            g = [_to_gltf_pos(p0), _to_gltf_pos(p1), _to_gltf_pos(p2)]
            gn = _to_gltf_dir(n)
            order = (0, 1, 2)
            if kit.v_dot(kit.v_cross(kit.v_sub(g[1], g[0]), kit.v_sub(g[2], g[0])), gn) < 0:
                order = (0, 2, 1)
            for k in order:
                pos_bytes += struct.pack("<3f", *g[k])
                for axis in range(3):
                    mins[axis] = min(mins[axis], g[k][axis])
                    maxs[axis] = max(maxs[axis], g[k][axis])
                idx_bytes += struct.pack("<I", vertex)
                vertex += 1
        pos_view = push(bytes(pos_bytes), 34962)
        idx_view = push(bytes(idx_bytes), 34963)
        fmin = [_f32(m) for m in mins]
        fmax = [_f32(m) for m in maxs]
        meshes.append({"name": box_mesh.name, "primitives": [{
            "attributes": {"POSITION": accessor(pos_view, vertex, 5126, "VEC3", (fmin, fmax))},
            "indices": accessor(idx_view, vertex, 5125, "SCALAR"), "material": 0, "mode": 4}]})
        nodes.append({"name": box_mesh.name, "mesh": len(meshes) - 1, "extras": {"collision": "box", "name": box.name}})
        root_children.append(len(nodes) - 1)

    gltf = {
        "asset": {"version": "2.0", "generator": f"ebs_skelkit {SKEL_REVISION} / ebs_meshkit {kit.KIT_REVISION}", "copyright": AUTHOR,
                  "extras": {"author": AUTHOR,
                             "frame": "authored in Unreal +X fwd/+Y right/+Z up cm; exported as glTF Y-up meters via glTF=(X,Z,Y)/100",
                             # Interchange copies asset extras into string attributes: keep every value a string.
                             "skeleton_root": skeleton.root, "bones": ",".join(b.name for b in skeleton.bones),
                             "clips": ",".join(c.name for c in animations),
                             "rotation_encoding": "glTF quaternion = (-x, -z, -y, w) of the Unreal FRotator::Quaternion"}},
        "scene": 0,
        "scenes": [{"name": f"{mesh.name}_scene", "nodes": root_children}],
        "nodes": nodes,
        "meshes": meshes,
        "skins": skins,
        "materials": materials,
        "accessors": accessors,
        "bufferViews": buffer_views,
        "buffers": [{"byteLength": len(buffer)}],
    }
    if gltf_animations:
        gltf["animations"] = gltf_animations
    if extras:
        gltf["asset"]["extras"].update(extras)
    json_bytes = json.dumps(gltf, separators=(",", ":"), sort_keys=True).encode("utf-8")
    while len(json_bytes) % 4:
        json_bytes += b" "
    bin_bytes = bytes(buffer)
    while len(bin_bytes) % 4:
        bin_bytes += b"\x00"
    total = 12 + 8 + len(json_bytes) + 8 + len(bin_bytes)
    with open(path, "wb") as handle:
        handle.write(struct.pack("<4sII", b"glTF", 2, total))
        handle.write(struct.pack("<I4s", len(json_bytes), b"JSON"))
        handle.write(json_bytes)
        handle.write(struct.pack("<I4s", len(bin_bytes), b"BIN\x00"))
        handle.write(bin_bytes)
    return kit.sha256_file(path)


# Encodings established by ArtSource/tools/skeletal_probe.py against the installed 5.8.2
# Interchange importer (UnrealEditor-Cmd 5.8.2, -nullrhi, sandbox project EBSPreview).
# Evidence: BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-001/import/
# skeletal-probe/{probe-report.json, probe-checks.json, UnrealEditor-Cmd-run*.log,
# heavy-run-receipt.json}. Consumers must not guess them; re-run the probe when the engine
# build changes.
SKELETAL_ENCODING = {
    "status": "VERIFIED 2026-09-06 against UE 5.8.2 Interchange: run 2 (22/22 encoding checks, replace-import into a cleared folder) and, "
              "after the harness was made idempotent (fresh /Game/Echoes/SkeletalProbe/Run_<stamp> folder per run, replace_existing False, "
              "verified clear, imported-object and AnimSequence counts asserted, 'Using stack [OverridePipeline]' required in the log), "
              "two independent clean re-runs run5 and run6 (28/28 checks each, 6 imported objects, 4 AnimSequences of 30 frames / 1.0 s). "
              "run 3 and run 4 are recorded failures of the old clear (stale package files turned run 3 into a silent re-import with no clips)",
    "evidence": "BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-001/import/skeletal-probe/{probe-checks.json (run 2), "
                "probe-checks-run5.json, probe-checks-run6.json, probe-report-run5.json, probe-report-run6.json, UnrealEditor-Cmd-run5.log, "
                "UnrealEditor-Cmd-run6.log, heavy-run-receipt.json}",
    "joint_node_translation": "glTF = (X, Z, Y)/100 of the Unreal local head offset (head - parent head), same as mesh positions; "
                              "imported reference pose heads read back (0,0,0)/(0,0,100)/(0,0,50)/(0,0,0) in component space",
    "joint_rotation_quaternion": "rest joints carry no rotation (identity); the bind pose is the translation-only inverseBindMatrices "
                                 "(column-major identity with -global head in column 3, glTF frame), which imports as identity rotators "
                                 "for every bone (ref_component_rotation_deg (0,0,0))",
    "animation_rotation_quaternion": "glTF sampler VEC4 = (-x, -z, -y, w) of the Unreal FRotator::Quaternion (rotator_gltf_quaternion); "
                                     "imports verbatim as the bone's local rotator: knee pitch +90 -> (90,0,0) and the foot moves "
                                     "(0,0,0)->(50,0,50) (shin +X toward +Z); hip yaw +90 -> (0,90,0); hip roll +90 -> (0,0,90) "
                                     "(knee moves (0,0,50)->(0,-50,100))",
    "animation_translation": "glTF sampler VEC3 = (X, Z, Y)/100 of (local rest offset + key translation_cm), absolute local translation; "
                             "hip (0,0,0)->(0,0,-20) reads back local translation (0,0,100)->(0,0,80), delta (0,0,-20) cm",
    "socket_under_joint": "leaf child node SOCKET_<name> of the bone's joint node, translation = (socket position - bone head) as (X,Z,Y)/100, "
                          "rotation = rotator_gltf_quaternion(0, yaw, 0), no scale: imports as a mesh socket on that bone with the exact "
                          "relative location, rotator (0,yaw,0) and unit scale. The static-mesh reflection treatment "
                          "(ebs_meshkit SOCKET_GLTF_SCALE / SOCKET_BASIS_COMPENSATION) must NOT be applied to skeletal sockets",
    "anim_sequence_naming": "with AssetImportTask.destination_name=<name> and use_source_name_for_asset False, Interchange names each "
                            "AnimSequence <name><clip name> (no separator) and the skeleton <name>_Skeleton",
    "keyframe_timing": "sampler input in seconds, LINEAR; a 0..1 s clip imports as 30 frames / 31 keys, length 1.0 s",
    "collision": "UBX_ nodes are written only when include_collision is True; skeletal imports do not consume them (pass False for skinned exports)",
    "clip_duration": "VERIFIED 2026-09-07 (UE 5.8.2, skeletal-clip-duration-probe): a clip duration must be an integer "
                     "number of frames at 30 fps or the import silently creates NO AnimSequence for it and still reports "
                     "success. Probe: 1/6/8/9/12/23/24/30-frame clips imported with exact length; 1.5/7.5/10.5/22.5-frame "
                     "clips vanished. write_skinned_glb now refuses to write an unaligned clip; use frame_aligned_duration() "
                     "and retime_clip() to snap an authored duration without changing the pose at any normalized time.",
}
