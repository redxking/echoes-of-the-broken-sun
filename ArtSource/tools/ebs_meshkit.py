"""Echoes ArtSource mesh kit: deterministic hard-surface geometry in the Unreal frame.

Author: Angelis Pseftis.

Pure Python 3, standard library only. Geometry is authored directly in the Unreal
world frame (+X forward, +Y right, +Z up, centimeters). Every polygon carries an
explicit outward normal, a material slot, and a named component so that component
inventories and triangle budgets can be measured from the same source that is
exported.

Exports:
  * Wavefront OBJ in the Unreal frame (container for the review renderer; no axis
    conversion; V flipped to OBJ's bottom-up convention).
  * glTF 2.0 binary (.glb) in the glTF frame (right-handed, +Y up, meters) using
    the inverse of the installed Unreal 5.8.2 Interchange GLTFCore conversion
    ``UE = (X_g, Z_g, Y_g)``, so ``glTF = (X_ue, Z_ue, Y_ue)`` and, for
    rotations, ``q_g = (-x, -z, -y, w)`` for a desired Unreal quaternion
    ``(x, y, z, w)`` (Engine/Plugins/Interchange/Runtime/Source/Parsers/GLTFCore/
    Private/GLTF/ConversionUtilities.h). Triangle winding is re-derived per
    triangle in the glTF frame from the stored outward normal so the importer
    always sees counter-clockwise front faces. Socket nodes use Interchange's
    ``SOCKET_`` prefix and box collision nodes use the ``UBX_`` prefix.

Nothing here talks to Unreal; the import step is a separate documented action.
"""
from __future__ import annotations

import hashlib
import json
import math
import struct
from dataclasses import dataclass, field

AUTHOR = "Angelis Pseftis"
KIT_REVISION = "ebs-meshkit-v3"  # v3: unique UV0 atlas packer and bake manifest; v2: Interchange socket encoding
UV_WORLD_CM = 256.0  # one UV0 tile spans 256 cm (1024 texels -> 4 texels per cm)


# --- vector helpers ---------------------------------------------------------
def v_add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def v_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def v_mul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def v_dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def v_cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def v_len(a):
    return math.sqrt(v_dot(a, a))


def v_norm(a):
    length = v_len(a)
    if length < 1e-12:
        return (0.0, 0.0, 1.0)
    return (a[0] / length, a[1] / length, a[2] / length)


def rot_z(p, yaw_deg):
    c, s = math.cos(math.radians(yaw_deg)), math.sin(math.radians(yaw_deg))
    return (p[0] * c - p[1] * s, p[0] * s + p[1] * c, p[2])


def rot_y(p, pitch_deg):
    """Unreal pitch: positive pitch lifts +X toward +Z, i.e. R(1,0,0) = (cos, 0, sin)."""
    c, s = math.cos(math.radians(pitch_deg)), math.sin(math.radians(pitch_deg))
    return (p[0] * c - p[2] * s, p[1], p[0] * s + p[2] * c)


def rot_yp(p, yaw_deg=0.0, pitch_deg=0.0):
    """Pitch about Y first, then yaw about Z (Unreal rotator composition order)."""
    q = rot_y(p, pitch_deg) if pitch_deg else p
    return rot_z(q, yaw_deg) if yaw_deg else q


def _r(value: float) -> float:
    """Round to 1e-4 cm so exports are byte-stable across platforms."""
    return round(value + 0.0, 4) + 0.0


# --- polygon / mesh ---------------------------------------------------------
@dataclass
class Polygon:
    points: list  # list of (x, y, z) in cm, ordered so that (p1-p0)x(p2-p0) is outward
    normal: tuple
    slot: int
    component: str
    uv_axis: tuple | None = None  # optional (u_dir, v_dir) override for planar mapping
    uv_override: list | None = None  # atlas UV0 per point, set by pack_atlas
    atlas_cells: int = 1  # chart width multiplier (numeral strips: material selects a cell per instance)
    chart_id: int | None = None


@dataclass
class Socket:
    name: str
    position: tuple
    yaw_deg: float = 0.0  # rotation about +Z; socket +X points along yaw
    purpose: str = ""
    raw_gltf_rotation: tuple | None = None  # probe override: quaternion written verbatim
    raw_gltf_scale: tuple | None = None     # probe override: scale written verbatim


@dataclass
class CollisionBox:
    name: str
    center: tuple
    size: tuple


# Alpha is RESERVED and always 1.0. It is not team ownership and not a spare channel: a reader must
# never have to disambiguate ownership from adaptation state (owner ruling 2026-09-07).
VERTEX_COLOR_DEFAULT = (0.0, 0.0, 0.0, 1.0)


@dataclass
class Mesh:
    name: str
    slots: list = field(default_factory=list)
    polygons: list = field(default_factory=list)
    sockets: list = field(default_factory=list)
    collision: list = field(default_factory=list)
    # Optional per-component vertex colours: {component_name: (r, g, b, a)} with each channel in
    # [0, 1]. Empty means NO COLOR_0 attribute is written at all, so every existing export is
    # unchanged byte for byte. Components absent from the mapping take VERTEX_COLOR_DEFAULT.
    vertex_colors: dict = field(default_factory=dict)

    # -- authoring -----------------------------------------------------------
    def slot(self, name: str) -> int:
        if name not in self.slots:
            self.slots.append(name)
        return self.slots.index(name)

    def vertex_color(self, component: str) -> tuple:
        """The RGBA a component's vertices carry. Unmapped components take the default."""
        value = self.vertex_colors.get(component, VERTEX_COLOR_DEFAULT)
        if len(value) != 4:
            raise ValueError(f"vertex colour for {component!r} must be RGBA, got {value!r}")
        for channel in value:
            if not 0.0 <= float(channel) <= 1.0:
                raise ValueError(f"vertex colour channel out of range for {component!r}: {value!r}")
        return tuple(float(c) for c in value)

    def add_polygon(self, points, slot, component, normal=None, uv_axis=None):
        pts = [tuple(float(c) for c in p) for p in points]
        if len(pts) < 3:
            raise ValueError("polygon needs at least three points")
        n = v_norm(v_cross(v_sub(pts[1], pts[0]), v_sub(pts[2], pts[0])))
        if normal is not None:
            normal = v_norm(normal)
            if v_dot(n, normal) < 0:
                pts.reverse()
                n = v_norm(v_cross(v_sub(pts[1], pts[0]), v_sub(pts[2], pts[0])))
        self.polygons.append(Polygon(pts, n, slot, component, uv_axis))

    def add_convex_solid(self, faces, slot, component):
        """faces: list of point lists. Each face is flipped so its normal points away
        from the solid's centroid, so authoring order never matters."""
        all_points = [p for face in faces for p in face]
        centroid = v_mul(
            (sum(p[0] for p in all_points), sum(p[1] for p in all_points), sum(p[2] for p in all_points)),
            1.0 / len(all_points),
        )
        for face in faces:
            face_centroid = v_mul(
                (sum(p[0] for p in face), sum(p[1] for p in face), sum(p[2] for p in face)),
                1.0 / len(face),
            )
            outward = v_sub(face_centroid, centroid)
            self.add_polygon(face, slot, component, normal=outward)

    def box(self, center, size, slot, component, yaw_deg=0.0, skip=()):
        """Axis-aligned (optionally yawed) box. ``skip`` names faces to omit from
        ('+X','-X','+Y','-Y','+Z','-Z') so an open bay or recess can be modelled
        without CSG. Normals stay outward from the full box."""
        cx, cy, cz = center
        hx, hy, hz = size[0] / 2.0, size[1] / 2.0, size[2] / 2.0
        corners = {}
        for sx in (-1, 1):
            for sy in (-1, 1):
                for sz in (-1, 1):
                    p = (sx * hx, sy * hy, sz * hz)
                    if yaw_deg:
                        p = rot_z(p, yaw_deg)
                    corners[(sx, sy, sz)] = (p[0] + cx, p[1] + cy, p[2] + cz)
        c = corners
        faces = {
            "+X": [c[(1, -1, -1)], c[(1, 1, -1)], c[(1, 1, 1)], c[(1, -1, 1)]],
            "-X": [c[(-1, -1, -1)], c[(-1, -1, 1)], c[(-1, 1, 1)], c[(-1, 1, -1)]],
            "+Y": [c[(-1, 1, -1)], c[(-1, 1, 1)], c[(1, 1, 1)], c[(1, 1, -1)]],
            "-Y": [c[(-1, -1, -1)], c[(1, -1, -1)], c[(1, -1, 1)], c[(-1, -1, 1)]],
            "+Z": [c[(-1, -1, 1)], c[(1, -1, 1)], c[(1, 1, 1)], c[(-1, 1, 1)]],
            "-Z": [c[(-1, -1, -1)], c[(-1, 1, -1)], c[(1, 1, -1)], c[(1, -1, -1)]],
        }
        centroid = (cx, cy, cz)
        for key, face in faces.items():
            if key in skip:
                continue
            face_centroid = v_mul((sum(p[0] for p in face), sum(p[1] for p in face), sum(p[2] for p in face)), 0.25)
            self.add_polygon(face, slot, component, normal=v_sub(face_centroid, centroid))

    def prism(self, outline, z0, z1, slot, component, cap_bottom=True, cap_top=True,
              center=(0.0, 0.0), yaw_deg=0.0, skip_edges=()):
        """Vertical prism from a 2D outline (list of (x, y)), convex, any winding.
        ``skip_edges`` lists outline edge indices (edge i joins point i to i+1)
        whose side wall is omitted, leaving an opening."""
        pts2 = []
        for x, y in outline:
            p = (x, y, 0.0)
            if yaw_deg:
                p = rot_z(p, yaw_deg)
            pts2.append((p[0] + center[0], p[1] + center[1]))
        bottom = [(x, y, z0) for x, y in pts2]
        top = [(x, y, z1) for x, y in pts2]
        n = len(pts2)
        cx = sum(p[0] for p in pts2) / n
        cy = sum(p[1] for p in pts2) / n
        centroid = (cx, cy, (z0 + z1) / 2.0)
        for i in range(n):
            if i in skip_edges:
                continue
            j = (i + 1) % n
            face = [bottom[i], bottom[j], top[j], top[i]]
            mid = ((pts2[i][0] + pts2[j][0]) / 2 - cx, (pts2[i][1] + pts2[j][1]) / 2 - cy, 0.0)
            self.add_polygon(face, slot, component, normal=mid)
        if cap_bottom:
            self.add_polygon(list(bottom), slot, component, normal=(0.0, 0.0, -1.0))
        if cap_top:
            self.add_polygon(list(top), slot, component, normal=(0.0, 0.0, 1.0))
        del centroid

    def ring(self, outer, inner, z0, z1, slot, component, center=(0.0, 0.0)):
        """Annular band between two outlines with equal vertex counts (e.g. two
        octagons). Side walls, inner walls and top/bottom annulus quads."""
        if len(outer) != len(inner):
            raise ValueError("ring outlines need equal vertex counts")
        n = len(outer)
        o = [(x + center[0], y + center[1]) for x, y in outer]
        i_ = [(x + center[0], y + center[1]) for x, y in inner]
        cx = sum(p[0] for p in o) / n
        cy = sum(p[1] for p in o) / n
        for k in range(n):
            m = (k + 1) % n
            # outer wall: normal points away from the ring centre
            quad = [(o[k][0], o[k][1], z0), (o[m][0], o[m][1], z0), (o[m][0], o[m][1], z1), (o[k][0], o[k][1], z1)]
            mid = ((o[k][0] + o[m][0]) / 2 - cx, (o[k][1] + o[m][1]) / 2 - cy, 0.0)
            self.add_polygon(quad, slot, component, normal=mid)
            # inner wall: normal points toward the ring centre
            quad = [(i_[k][0], i_[k][1], z0), (i_[m][0], i_[m][1], z0), (i_[m][0], i_[m][1], z1), (i_[k][0], i_[k][1], z1)]
            mid = (-((i_[k][0] + i_[m][0]) / 2 - cx), -((i_[k][1] + i_[m][1]) / 2 - cy), 0.0)
            self.add_polygon(quad, slot, component, normal=mid)
            # top and bottom annulus quads
            top = [(o[k][0], o[k][1], z1), (o[m][0], o[m][1], z1), (i_[m][0], i_[m][1], z1), (i_[k][0], i_[k][1], z1)]
            self.add_polygon(top, slot, component, normal=(0, 0, 1))
            bot = [(o[k][0], o[k][1], z0), (o[m][0], o[m][1], z0), (i_[m][0], i_[m][1], z0), (i_[k][0], i_[k][1], z0)]
            self.add_polygon(bot, slot, component, normal=(0, 0, -1))

    def tube(self, p0, p1, radius, sides, slot, component, caps=True, phase_deg=0.0):
        """Cylinder between two points (any axis)."""
        axis = v_norm(v_sub(p1, p0))
        helper = (0.0, 0.0, 1.0) if abs(axis[2]) < 0.9 else (1.0, 0.0, 0.0)
        u = v_norm(v_cross(helper, axis))
        w = v_norm(v_cross(axis, u))
        ring0, ring1 = [], []
        for k in range(sides):
            a = math.radians(phase_deg) + 2 * math.pi * k / sides
            offset = v_add(v_mul(u, radius * math.cos(a)), v_mul(w, radius * math.sin(a)))
            ring0.append(v_add(p0, offset))
            ring1.append(v_add(p1, offset))
        faces = [[ring0[k], ring0[(k + 1) % sides], ring1[(k + 1) % sides], ring1[k]] for k in range(sides)]
        if caps:
            faces.append(list(ring0))
            faces.append(list(ring1))
        self.add_convex_solid(faces, slot, component)

    def merge(self, other: "Mesh", translate=(0.0, 0.0, 0.0), yaw_deg=0.0, pitch_deg=0.0,
              component_prefix="", include_sockets=True, socket_prefix=""):
        """Append another mesh transformed by pitch (about Y) then yaw (about Z),
        then translation; material slots are matched by name."""
        slot_map = {i: self.slot(name) for i, name in enumerate(other.slots)}
        for poly in other.polygons:
            pts = [v_add(rot_yp(p, yaw_deg, pitch_deg), translate) for p in poly.points]
            n = rot_yp(poly.normal, yaw_deg, pitch_deg)
            uv_axis = None
            if poly.uv_axis is not None:
                uv_axis = (rot_yp(poly.uv_axis[0], yaw_deg, pitch_deg), rot_yp(poly.uv_axis[1], yaw_deg, pitch_deg))
            self.polygons.append(Polygon(pts, n, slot_map[poly.slot], component_prefix + poly.component, uv_axis))
        if include_sockets:
            for s in other.sockets:
                q = rot_yp(s.position, yaw_deg, pitch_deg)
                self.sockets.append(Socket(socket_prefix + s.name, v_add(q, translate), s.yaw_deg + yaw_deg, s.purpose))

    # -- measurement ---------------------------------------------------------
    def triangles(self):
        """Yield (p0, p1, p2, normal, slot, component, uv0s, uv1s) fan triangles."""
        uv1_cells = self._uv1_cells()
        for index, poly in enumerate(self.polygons):
            uv0 = self._planar_uv(poly)
            uv1 = uv1_cells[index]
            pts = poly.points
            for k in range(1, len(pts) - 1):
                yield (
                    pts[0], pts[k], pts[k + 1], poly.normal, poly.slot, poly.component,
                    (uv0[0], uv0[k], uv0[k + 1]), (uv1[0], uv1[k], uv1[k + 1]),
                )

    def triangle_count(self) -> int:
        return sum(len(p.points) - 2 for p in self.polygons)

    def triangle_count_by(self, key: str) -> dict:
        counts = {}
        for p in self.polygons:
            k = self.slots[p.slot] if key == "slot" else p.component
            counts[k] = counts.get(k, 0) + len(p.points) - 2
        return dict(sorted(counts.items()))

    def slot_names_in_primitive_order(self) -> list:
        """Material slot names in the order the GLB writer emits primitives (empty slots skipped)."""
        counts = self.triangle_count_by("slot")
        return [name for name in self.slots if counts.get(name, 0) > 0]

    def components(self) -> list:
        seen = []
        for p in self.polygons:
            if p.component not in seen:
                seen.append(p.component)
        return seen

    def bounds(self):
        xs = [p[0] for poly in self.polygons for p in poly.points]
        ys = [p[1] for poly in self.polygons for p in poly.points]
        zs = [p[2] for poly in self.polygons for p in poly.points]
        return ((min(xs), min(ys), min(zs)), (max(xs), max(ys), max(zs)))

    def component_bounds(self, component: str):
        pts = [p for poly in self.polygons if poly.component == component for p in poly.points]
        if not pts:
            return None
        return (
            (min(p[0] for p in pts), min(p[1] for p in pts), min(p[2] for p in pts)),
            (max(p[0] for p in pts), max(p[1] for p in pts), max(p[2] for p in pts)),
        )

    # -- UV generation -------------------------------------------------------
    @staticmethod
    def _planar_uv(poly: Polygon):
        if poly.uv_override is not None:
            return list(poly.uv_override)
        n = poly.normal
        if poly.uv_axis is not None:
            u_dir, v_dir = poly.uv_axis
        else:
            ax, ay, az = abs(n[0]), abs(n[1]), abs(n[2])
            if az >= ax and az >= ay:
                u_dir, v_dir = (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)  # top/bottom: X across, Y down the image
            elif ax >= ay:
                u_dir, v_dir = (0.0, 1.0, 0.0), (0.0, 0.0, -1.0)  # ±X walls: Y across, Z up -> V down
            else:
                u_dir, v_dir = (1.0, 0.0, 0.0), (0.0, 0.0, -1.0)  # ±Y walls: X across
        return [(_r(v_dot(p, u_dir) / UV_WORLD_CM), _r(v_dot(p, v_dir) / UV_WORLD_CM)) for p in poly.points]

    def _uv1_cells(self):
        """Non-overlapping per-polygon cells in UV1 (lightmap channel). Each polygon
        is planar-projected into its own grid cell with a 12% gutter. Coverage is
        modest but the chart set is guaranteed non-overlapping and deterministic."""
        count = len(self.polygons)
        grid = max(1, math.ceil(math.sqrt(count)))
        cell = 1.0 / grid
        gutter = cell * 0.12
        result = []
        for index, poly in enumerate(self.polygons):
            uv = self._planar_uv(poly)
            us = [p[0] for p in uv]
            vs = [p[1] for p in uv]
            span = max(max(us) - min(us), max(vs) - min(vs), 1e-6)
            col, row = index % grid, index // grid
            u0, v0 = col * cell + gutter, row * cell + gutter
            inner = cell - 2 * gutter
            result.append([(_r(u0 + (u - min(us)) / span * inner), _r(v0 + (v - min(vs)) / span * inner)) for u, v in uv])
        return result

    # -- export --------------------------------------------------------------
    def write_obj(self, path: str, header_lines=()):
        lines = [f"# {self.name}", f"# Author: {AUTHOR}", "# Frame: Unreal +X forward, +Y right, +Z up; centimeters; no axis conversion",
                 f"# Kit: {KIT_REVISION}"]
        lines += [f"# {line}" for line in header_lines]
        lines.append(f"o {self.name}")
        v_index = 0
        current_component = None
        current_slot = None
        positions, normals, uvs, faces = [], [], [], []
        for (p0, p1, p2, n, slot, component, uv0, _uv1) in self.triangles():
            if component != current_component:
                faces.append(f"g {component}")
                current_component = component
            if slot != current_slot:
                faces.append(f"usemtl {self.slots[slot]}")
                current_slot = slot
            for p, uv in zip((p0, p1, p2), uv0):
                positions.append(f"v {_r(p[0])} {_r(p[1])} {_r(p[2])}")
                uvs.append(f"vt {uv[0]} {_r(1.0 - uv[1])}")
                normals.append(f"vn {_r(n[0])} {_r(n[1])} {_r(n[2])}")
            faces.append(f"f {v_index + 1}/{v_index + 1}/{v_index + 1} {v_index + 2}/{v_index + 2}/{v_index + 2} {v_index + 3}/{v_index + 3}/{v_index + 3}")
            v_index += 3
        text = "\n".join(lines + positions + uvs + normals + faces) + "\n"
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        return sha256_file(path)

    def write_glb(self, path: str, extras: dict | None = None, include_collision: bool = True):
        """glTF 2.0 binary in the glTF frame (meters). One mesh with one primitive
        per material slot; SOCKET_ and UBX_ child nodes for Interchange."""
        def to_gltf_pos(p):
            return (p[0] / 100.0, p[2] / 100.0, p[1] / 100.0)

        def to_gltf_dir(n):
            return (n[0], n[2], n[1])

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

        per_slot = {i: [] for i in range(len(self.slots))}
        for tri in self.triangles():
            per_slot[tri[4]].append(tri)
        used_slots = [i for i in range(len(self.slots)) if per_slot[i]]
        # Only slots with geometry become glTF materials (an unused material would create a stray slot at import).
        materials = [{"name": self.slots[i], "pbrMetallicRoughness": {"baseColorFactor": [0.8, 0.8, 0.8, 1.0], "metallicFactor": 0.0, "roughnessFactor": 0.7}, "doubleSided": False} for i in used_slots]
        material_index = {slot: k for k, slot in enumerate(used_slots)}
        primitives = []
        for slot_index in used_slots:
            tris = per_slot[slot_index]
            pos_bytes, nrm_bytes, uv0_bytes, uv1_bytes, idx_bytes = bytearray(), bytearray(), bytearray(), bytearray(), bytearray()
            col_bytes = bytearray()
            mins = [float("inf")] * 3
            maxs = [float("-inf")] * 3
            vertex = 0
            for (p0, p1, p2, n, _slot, _component, uv0, uv1) in tris:
                rgba = self.vertex_color(_component)
                g = [to_gltf_pos(p0), to_gltf_pos(p1), to_gltf_pos(p2)]
                gn = to_gltf_dir(n)
                order = (0, 1, 2)
                right_handed = v_cross(v_sub(g[1], g[0]), v_sub(g[2], g[0]))
                if v_dot(right_handed, gn) < 0:
                    order = (0, 2, 1)
                for k in order:
                    p = g[k]
                    pos_bytes += struct.pack("<3f", *p)
                    nrm_bytes += struct.pack("<3f", *gn)
                    uv0_bytes += struct.pack("<2f", *uv0[k])
                    uv1_bytes += struct.pack("<2f", *uv1[k])
                    col_bytes += struct.pack("<4f", *rgba)
                    for axis in range(3):
                        mins[axis] = min(mins[axis], p[axis])
                        maxs[axis] = max(maxs[axis], p[axis])
                    idx_bytes += struct.pack("<I", vertex)
                    vertex += 1
            pos_view = push(bytes(pos_bytes), 34962)
            nrm_view = push(bytes(nrm_bytes), 34962)
            uv0_view = push(bytes(uv0_bytes), 34962)
            uv1_view = push(bytes(uv1_bytes), 34962)
            col_view = push(bytes(col_bytes), 34962) if self.vertex_colors else None
            idx_view = push(bytes(idx_bytes), 34963)
            # float32 min/max must be the stored values: round-trip through struct
            fmin = [struct.unpack("<f", struct.pack("<f", m))[0] for m in mins]
            fmax = [struct.unpack("<f", struct.pack("<f", m))[0] for m in maxs]
            attributes = {
                "POSITION": accessor(pos_view, vertex, 5126, "VEC3", (fmin, fmax)),
                "NORMAL": accessor(nrm_view, vertex, 5126, "VEC3"),
                "TEXCOORD_0": accessor(uv0_view, vertex, 5126, "VEC2"),
                "TEXCOORD_1": accessor(uv1_view, vertex, 5126, "VEC2"),
            }
            if col_view is not None:
                attributes["COLOR_0"] = accessor(col_view, vertex, 5126, "VEC4")
            primitives.append({
                "attributes": attributes,
                "indices": accessor(idx_view, vertex, 5125, "SCALAR"),
                "material": material_index[slot_index],
                "mode": 4,
            })
        meshes = [{"name": self.name, "primitives": primitives}]
        nodes = [{"name": self.name, "mesh": 0}]
        root_children = [0]
        mesh_children = []
        for socket in self.sockets:
            # Socket nodes must be descendants of the mesh node: with more than one mesh in the
            # file (collision boxes are meshes too) Interchange attaches sockets by parent chain
            # (InterchangePipelineMeshesUtilities.cpp, "Import of Local Sockets"). The socket
            # name is the node name after the SOCKET_ prefix.
            node = {
                "name": f"SOCKET_{socket.name}",
                "translation": [_r(c) for c in to_gltf_pos(socket.position)],
                "rotation": [_r(c) for c in (socket.raw_gltf_rotation or socket_rotation_gltf(socket.yaw_deg))],
                "extras": {"purpose": socket.purpose, "unreal_yaw_deg": socket.yaw_deg},
            }
            scale = socket.raw_gltf_scale or SOCKET_GLTF_SCALE
            if tuple(scale) != (1.0, 1.0, 1.0):
                node["scale"] = [_r(c) for c in scale]
            nodes.append(node)
            mesh_children.append(len(nodes) - 1)
        if mesh_children:
            nodes[0]["children"] = mesh_children
        for index, box in enumerate(self.collision if include_collision else []):
            box_mesh = Mesh(f"UBX_{self.name}_{index + 1:02d}")
            box_mesh.slot("Collision")
            box_mesh.box(box.center, box.size, 0, "collision")
            pos_bytes, idx_bytes = bytearray(), bytearray()
            mins = [float("inf")] * 3
            maxs = [float("-inf")] * 3
            vertex = 0
            for (p0, p1, p2, n, _s, _c, _u0, _u1) in box_mesh.triangles():
                g = [to_gltf_pos(p0), to_gltf_pos(p1), to_gltf_pos(p2)]
                gn = to_gltf_dir(n)
                order = (0, 1, 2)
                if v_dot(v_cross(v_sub(g[1], g[0]), v_sub(g[2], g[0])), gn) < 0:
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
            fmin = [struct.unpack("<f", struct.pack("<f", m))[0] for m in mins]
            fmax = [struct.unpack("<f", struct.pack("<f", m))[0] for m in maxs]
            meshes.append({"name": box_mesh.name, "primitives": [{
                "attributes": {"POSITION": accessor(pos_view, vertex, 5126, "VEC3", (fmin, fmax))},
                "indices": accessor(idx_view, vertex, 5125, "SCALAR"), "material": 0, "mode": 4}]})  # material silences the importer warning; UBX meshes never render
            nodes.append({"name": box_mesh.name, "mesh": len(meshes) - 1, "extras": {"collision": "box", "name": box.name}})
            root_children.append(len(nodes) - 1)
        gltf = {
            "asset": {"version": "2.0", "generator": f"ebs_meshkit {KIT_REVISION}", "copyright": AUTHOR,
                      "extras": {"author": AUTHOR, "frame": "authored in Unreal +X fwd/+Y right/+Z up cm; exported as glTF Y-up meters via glTF=(X,Z,Y)/100"}},
            "scene": 0,
            "scenes": [{"name": f"{self.name}_scene", "nodes": root_children}],
            "nodes": nodes,
            "meshes": meshes,
            "materials": materials,
            "accessors": accessors,
            "bufferViews": buffer_views,
            "buffers": [{"byteLength": len(buffer)}],
        }
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
        return sha256_file(path)


def q_axis(axis: int, deg: float):
    """Quaternion (x, y, z, w) for a rotation of ``deg`` about glTF axis 0=X, 1=Y, 2=Z."""
    half = math.radians(deg) / 2.0
    s = math.sin(half)
    return ((s if axis == 0 else 0.0), (s if axis == 1 else 0.0), (s if axis == 2 else 0.0), math.cos(half))


def quat_mul(a, b):
    """Hamilton product a*b for (x, y, z, w) quaternions."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    )


# Socket node encoding for the installed Unreal 5.8.2 Interchange glTF importer.
# InterchangeMeshHelper.cpp ImportSockets() multiplies every socket transform by the
# scene's AxisConversionInverseTransform, which for glTF is a Y/Z swap (a reflection):
# an identity node imports as rotator (0, 180, -90) with scale (-1, 1, 1). The
# encoding below was established by probe imports (evidence:
# BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-BLD-002/import/probe-sweep
# and probe-verify): node scale (-1, 1, 1) cancels the reflection, and the rotation
# q = q_y(-yaw) * (q_y(180) * q_x(-90)) imports as a pure Unreal yaw with unit scale
# for 0, 45, 90, -90 and 180 degrees. Re-verify with ArtSource/tools/socket_probe.py
# whenever the engine build changes.
SOCKET_BASIS_COMPENSATION = quat_mul(q_axis(1, 180.0), q_axis(0, -90.0))
SOCKET_GLTF_SCALE = (-1.0, 1.0, 1.0)


def socket_rotation_gltf(yaw_deg: float):
    """glTF node quaternion for a socket whose Unreal rotation is a pure yaw (see above)."""
    return quat_mul(q_axis(1, -yaw_deg), SOCKET_BASIS_COMPENSATION)


# --- unique UV atlas ----------------------------------------------------------
def planar_frame(poly: Polygon):
    """Upright tangent frame for a polygon: v runs down the wall (world -Z projected onto
    the plane) for walls, along +Y for floors/ceilings; u completes a frame with the normal.
    Returns (origin, u_dir, v_dir, width_cm, height_cm, projected 2D points)."""
    n = poly.normal
    if abs(n[2]) < 0.9:
        down = (0.0, 0.0, -1.0)
        v_dir = v_norm(v_sub(down, v_mul(n, v_dot(down, n))))
        u_dir = v_norm(v_cross(v_dir, n))
    else:
        u_dir = (1.0, 0.0, 0.0) if n[2] > 0 else (-1.0, 0.0, 0.0)
        v_dir = v_norm(v_cross(n, u_dir))
    proj = [(v_dot(p, u_dir), v_dot(p, v_dir)) for p in poly.points]
    min_u, min_v = min(q[0] for q in proj), min(q[1] for q in proj)
    max_u, max_v = max(q[0] for q in proj), max(q[1] for q in proj)
    origin = v_add(v_mul(u_dir, min_u), v_mul(v_dir, min_v))
    return origin, u_dir, v_dir, max_u - min_u, max_v - min_v, [(q[0] - min_u, q[1] - min_v) for q in proj]


def _poly_key(poly: Polygon):
    return (poly.component, tuple(sorted(tuple(round(c, 2) for c in p) for p in poly.points)))


def pack_atlas(meshes, size: int = 1024, gutter_px: int = 2, min_px: int = 4, fill_target: float = 0.80):
    """Pack every polygon of every mesh into one unique, non-overlapping UV0 atlas.

    Identical polygons (same component and point set, e.g. LOD0/LOD1 twins) share a chart.
    Texel density is uniform (px per cm) and chosen so the shelf packing fits ``size``;
    ``atlas_cells`` widens a chart into a strip of identical cells. Deterministic.
    Returns the atlas description (charts with pixel rects and world frames)."""
    entries = {}
    order = []
    for mesh, lod in meshes:
        for index, poly in enumerate(mesh.polygons):
            key = _poly_key(poly)
            if key not in entries:
                origin, u_dir, v_dir, w, h, proj = planar_frame(poly)
                entries[key] = {"polys": [], "origin": origin, "u_dir": u_dir, "v_dir": v_dir, "w_cm": w, "h_cm": h,
                                "proj": proj, "component": poly.component, "slot": mesh.slots[poly.slot], "cells": max(1, poly.atlas_cells),
                                "normal": poly.normal, "meshes": [],
                                # Authored COLOR_0 travels with the chart so the baker's StateMask can
                                # mirror it. Only present when the mesh carries vertex colours.
                                "vertex_color": mesh.vertex_color(poly.component) if mesh.vertex_colors else None}
                order.append(key)
            entries[key]["polys"].append((mesh, index, poly))
            entries[key]["meshes"].append(f"{mesh.name}:LOD{lod}")
            entries[key]["cells"] = max(entries[key]["cells"], poly.atlas_cells)
    total_area = sum(max(e["w_cm"], 1.0) * max(e["h_cm"], 1.0) * e["cells"] for e in entries.values())
    density = math.sqrt(size * size * fill_target / max(total_area, 1.0))
    for _attempt in range(60):
        rects = []
        for key in order:
            e = entries[key]
            cell_w = max(min_px, math.ceil(e["w_cm"] * density))
            h = max(min_px, math.ceil(e["h_cm"] * density))
            rects.append((key, cell_w * e["cells"] + 2 * gutter_px, h + 2 * gutter_px, cell_w, h))
        rects.sort(key=lambda r: (-r[2], -r[1], r[0]))
        x = y = shelf_h = 0
        placed = {}
        ok = True
        for key, rw, rh, cell_w, h in rects:
            if x + rw > size:
                x = 0
                y += shelf_h
                shelf_h = 0
            if y + rh > size or rw > size:
                ok = False
                break
            placed[key] = (x + gutter_px, y + gutter_px, cell_w, h)
            x += rw
            shelf_h = max(shelf_h, rh)
        if ok:
            break
        density *= 0.96
    else:
        raise RuntimeError("atlas packing failed")
    charts = []
    for chart_id, key in enumerate(order):
        e = entries[key]
        px, py, cell_w, h = placed[key]
        uv = []
        for (u, v) in e["proj"]:
            fu = (px + (u / e["w_cm"] * cell_w if e["w_cm"] > 1e-9 else 0.0)) / size
            fv = (py + (v / e["h_cm"] * h if e["h_cm"] > 1e-9 else 0.0)) / size
            uv.append((_r(fu), _r(fv)))
        for mesh, index, poly in e["polys"]:
            poly.uv_override = list(uv)
            poly.chart_id = chart_id
        chart = {"id": chart_id, "component": e["component"], "slot": e["slot"], "meshes": sorted(set(e["meshes"])),
                       "rect_px": [px, py, cell_w * e["cells"], h], "cell_px": [cell_w, h], "cells": e["cells"],
                       "origin_cm": [_r(c) for c in e["origin"]], "u_dir": [_r(c) for c in e["u_dir"]], "v_dir": [_r(c) for c in e["v_dir"]],
                       "normal": [_r(c) for c in e["normal"]], "size_cm": [_r(e["w_cm"]), _r(e["h_cm"])],
                       "polygon_uv": uv, "polygon_world": [[_r(c) for c in p] for p in e["polys"][0][2].points]}
        if e["vertex_color"] is not None:
            chart["vertex_color"] = [_r(c) for c in e["vertex_color"]]
        charts.append(chart)
    return {"size": size, "density_px_per_cm": _r(density), "gutter_px": gutter_px, "charts": charts,
            "used_fraction": _r(sum(c["rect_px"][2] * c["rect_px"][3] for c in charts) / float(size * size))}


def write_bake_manifest(path: str, atlas: dict, extras: dict | None = None):
    doc = {"author": AUTHOR, "creator": AUTHOR, "kit_revision": KIT_REVISION,
           "frame": "Unreal +X forward, +Y right, +Z up, centimeters; atlas pixel origin top-left, v down",
           "atlas": atlas}
    if extras:
        doc.update(extras)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(doc, handle, indent=1, sort_keys=True)
        handle.write("\n")
    return sha256_file(path)


# --- outlines ---------------------------------------------------------------
def octagon(half: float, chamfer: float):
    """Square of half-width ``half`` with 45-degree corner chamfers of ``chamfer``."""
    h, c = half, chamfer
    return [(-h + c, -h), (h - c, -h), (h, -h + c), (h, h - c), (h - c, h), (-h + c, h), (-h, h - c), (-h, -h + c)]


def square(half: float):
    return [(-half, -half), (half, -half), (half, half), (-half, half)]


def regular_polygon(radius: float, sides: int, phase_deg: float = 0.0):
    return [(radius * math.cos(math.radians(phase_deg) + 2 * math.pi * k / sides),
             radius * math.sin(math.radians(phase_deg) + 2 * math.pi * k / sides)) for k in range(sides)]


# --- files ------------------------------------------------------------------
def sha256_file(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_glb(path: str) -> dict:
    """Parse a .glb back to its JSON chunk (validation helper)."""
    with open(path, "rb") as handle:
        magic, version, length = struct.unpack("<4sII", handle.read(12))
        if magic != b"glTF" or version != 2:
            raise ValueError("not a glTF 2.0 binary")
        chunk_length, chunk_type = struct.unpack("<I4s", handle.read(8))
        if chunk_type != b"JSON":
            raise ValueError("first chunk is not JSON")
        doc = json.loads(handle.read(chunk_length).decode("utf-8"))
        chunk_length, chunk_type = struct.unpack("<I4s", handle.read(8))
        if chunk_type != b"BIN\x00":
            raise ValueError("second chunk is not BIN")
        binary = handle.read(chunk_length)
        if doc["buffers"][0]["byteLength"] > len(binary):
            raise ValueError("BIN chunk shorter than declared buffer")
        doc["_bin_length"] = len(binary)
        doc["_total_length"] = length
        return doc
