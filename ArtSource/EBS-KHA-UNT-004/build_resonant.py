#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-UNT-004 — the Kharuun Assemblies Resonant.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-FAC-025.KA.RESONANT.ASSET`: a tall,
extremely slender QUADRUPED on four stilt legs, with an arched spine carrying a dorsal array of
thirteen translucent amber sensor fins and a small head held high.

The candidate shows four legs. My card said biped — the third of eight to carry the wrong anatomy,
all written before their concepts were traced. The card was corrected, not the model.

Usage:
  python3 build_resonant.py --evidence-dir "<root>/EBS-KHA-UNT-004" [--skinned]
  python3 build_resonant.py --evidence-dir "<root>/EBS-KHA-UNT-004" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the ground-contact centre between the feet.
"""
from __future__ import annotations

import argparse
import json
import math
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import ebs_skelkit as skel  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-KA-RESONANT"
PRODUCTION_ID = "EBS-KHA-UNT-004"
ASSET = "SK_EBS_KHA_UNT_004"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_004/"
REVISION = "ebs-kha-unt-004-concept-v1"
CARD = "REL-FAC-025.KA.RESONANT.ASSET"

STRATA = "MI_EBS_KHA_Strata"      # dark faceted plate: body, spine, limbs, head
AMBER = "MI_EBS_KHA_Amber"        # the translucent sensor fins — the only emissive

TICKS_PER_SECOND = 20.0

H = 255.0                              # crown of the head: the tallest Kharuun unit
BODY_LEN = 132.0                       # a short body: with the fin array the horizontal extent lands near
                                       # 176 cm, so height over extent reaches the traced 1.45+
BODY_W = 52.0                          # slender: nothing here may promise a fight
BODY_Z = 150.0                         # the torso rides high on the stilts
SPINE_SEGMENTS = 4
NECK_Z, HEAD_Z = 200.0, 243.0          # the crown finishes at 255 cm, the tallest Kharuun unit
FIN_COUNT = 13                         # counted on the candidate's LISTENING view
LEG_X, LEG_Y = 52.0, 30.0
FOOT_X, FOOT_Y = 64.0, 58.0            # the stilts splay outward to small feet; the fore-aft splay is
                                       # kept tight so the horizontal extent stays under the height
KNEE_Z = 92.0
LIMB_R = 7.5                           # thin: the delicacy is the read
STATES = ("passive", "detecting")

LEGS = (("fl", LEG_X, -LEG_Y, FOOT_X, -FOOT_Y), ("fr", LEG_X, LEG_Y, FOOT_X, FOOT_Y),
        ("rl", -LEG_X, -LEG_Y, -FOOT_X, -FOOT_Y), ("rr", -LEG_X, LEG_Y, -FOOT_X, FOOT_Y))
LEG_TAGS = tuple(t[0] for t in LEGS)

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "ground-contact centre between the feet; the runtime owns translation"),
    ("body", "root", (0.0, 0.0, BODY_Z), "the slight torso the spine and limbs hang from"),
]
for _i in range(SPINE_SEGMENTS):
    _t = (_i + 1) / SPINE_SEGMENTS
    BONES.append((f"spine_{_i + 1:02d}", "body" if _i == 0 else f"spine_{_i:02d}",
                  (-BODY_LEN * 0.42 + BODY_LEN * 0.76 * _t, 0.0, BODY_Z + (NECK_Z - BODY_Z) * (_t ** 1.4)),
                  f"spine segment {_i + 1}: the arch, and it carries fins {_i * 3 + 1}-{min(FIN_COUNT, _i * 3 + 4)}"))
BONES.append(("neck", f"spine_{SPINE_SEGMENTS:02d}", (BODY_LEN * 0.42, 0.0, NECK_Z), "a long thin neck"))
BONES.append(("head", "neck", (BODY_LEN * 0.52, 0.0, HEAD_Z), "the small head carried high in listening, dropped in contact"))
for _tag, _hx, _hy, _fx, _fy in LEGS:
    BONES.append((f"{_tag}_hip", "body", (_hx, _hy, BODY_Z - 16.0), f"{_tag} stilt: attachment at the body"))
    BONES.append((f"{_tag}_upper", f"{_tag}_hip", (_hx, _hy, BODY_Z - 16.0), f"{_tag} stilt: upper segment, pivoting at the hip"))
    BONES.append((f"{_tag}_lower", f"{_tag}_upper", ((_hx + _fx) / 2.0, (_hy + _fy) / 2.0, KNEE_Z),
                  f"{_tag} stilt: lower segment, pivoting at the knee"))
    BONES.append((f"{_tag}_foot", f"{_tag}_lower", (_fx, _fy, 10.0), f"{_tag} stilt: the small foot"))


def spine_point(t: float):
    """(x, z) along the arched spine, t from the tail root (0) to the base of the neck (1).

    The arc runs the LENGTH of the back, not just its front third. A first pass spanned 40 cm, which
    packed thirteen fins into a space smaller than one fin and they merged into a single lit crest.
    """
    return -BODY_LEN * 0.42 + BODY_LEN * 0.76 * t, BODY_Z + (NECK_Z - BODY_Z) * (t ** 1.4)


def fin_at(index: int):
    """(x, z, length, height, spine segment) for one dorsal fin.

    Largest at mid-back and tapering fore and aft, as the candidate draws them. Each fin is its own
    component so the runtime can light the array in sequence; the sequence is a material animation.
    """
    t = index / (FIN_COUNT - 1)
    x, z = spine_point(t)
    taper = math.sin(math.pi * (0.18 + 0.64 * t))
    length = 14.0 + 26.0 * taper
    height = 6.0 + 10.0 * taper
    segment = min(SPINE_SEGMENTS, 1 + int(t * SPINE_SEGMENTS * 0.999))
    return x, z, length, height, segment


def build_body(lod: int, state: str = "passive") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    amber = m.slot(AMBER)
    fine = lod == 0
    sides = 6 if fine else 4

    # 1. the slight torso and the arched spine above it
    m.box((0.0, 0.0, BODY_Z - 4.0), (BODY_LEN * 0.72, BODY_W, 40.0), strata, "torso")
    steps = SPINE_SEGMENTS * 2 if fine else SPINE_SEGMENTS
    for i in range(steps):
        x0, z0 = spine_point(i / steps)
        x1, z1 = spine_point((i + 1) / steps)
        m.tube((x0, 0.0, z0), (x1, 0.0, z1), 11.0 - 2.0 * (i / steps), sides, strata, f"spine_{i + 1:02d}")

    # 2. the neck and the small head
    nx, nz = spine_point(1.0)
    m.tube((nx, 0.0, nz), (BODY_LEN * 0.48, 0.0, HEAD_Z - 10.0), 8.0, sides, strata, "neck")
    m.box((BODY_LEN * 0.52, 0.0, HEAD_Z), (44.0, 26.0, 24.0), strata, "head")
    if fine:
        m.box((BODY_LEN * 0.52 + 16.0, 0.0, HEAD_Z - 3.0), (14.0, 16.0, 12.0), strata, "muzzle")

    # 3. the dorsal fin array: the asset's information channel, one component per fin
    # Each fin is a plate blade carrying a narrow VEIN. Canon calls the fins translucent amber, but
    # REL-ART-029 caps amber EMISSIVE at 15% of surface area, and thirteen fully amber blades measured
    # 32%. The translucency belongs to the texture; the emissive is the vein, which is also what the
    # candidate's SENSOR FINS panel actually draws glowing.
    lit = state == "detecting"
    for n in range(FIN_COUNT):
        x, z, length, height, _segment = fin_at(n)
        blade_z = z + height / 2.0 + 12.0
        m.box((x - length * 0.18, 0.0, blade_z), (length * 0.5, 4.0, height + 22.0), strata,
              f"fin_{n + 1:02d}")
        m.box((x - length * 0.18, 0.0, blade_z + 2.0), (length * 0.16, 5.0, height + 12.0),
              amber if lit else strata, f"fin_{n + 1:02d}_vein")

    # 4. four stilt legs: long, thin, splaying outward to small feet
    for tag, hx, hy, fx, fy in LEGS:
        kx, ky = (hx + fx) / 2.0, (hy + fy) / 2.0
        m.tube((hx, hy, BODY_Z - 16.0), (kx, ky, KNEE_Z), LIMB_R, sides, strata, f"{tag}_upper")
        m.tube((kx, ky, KNEE_Z), (fx, fy, 10.0), LIMB_R * 0.86, sides, strata, f"{tag}_lower")
        m.box((fx + 4.0, fy, 5.0), (30.0, 18.0, 10.0), strata, f"{tag}_foot")

    m.collision.append(kit.CollisionBox("body", (0.0, 0.0, BODY_Z), (BODY_LEN * 0.8, BODY_W + 10.0, 70.0)))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c == "torso":
            by_component[c] = "body"
        elif c.startswith("spine_"):
            index = int(c.split("_")[1])
            steps = max(1, len([k for k in m.components() if k.startswith("spine_")]))
            by_component[c] = f"spine_{min(SPINE_SEGMENTS, 1 + (index - 1) * SPINE_SEGMENTS // steps):02d}"
        elif c.startswith("fin_"):
            by_component[c] = f"spine_{fin_at(int(c.split('_')[1]) - 1)[4]:02d}"
        elif c in ("neck",):
            by_component[c] = "neck"
        elif c in ("head", "muzzle"):
            by_component[c] = "head"
        else:
            for tag in LEG_TAGS:
                if c.startswith(tag + "_"):
                    by_component[c] = f"{tag}_{c.split('_')[-1]}"
                    break
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Fin_Array_Base": (f"spine_01", (spine_point(0.0)[0], 0.0, spine_point(0.0)[1] + 26.0), 0.0,
                       "the first fin in the sequence: where a detection run starts"),
    "Fin_Array_Tip": (f"spine_{SPINE_SEGMENTS:02d}", (spine_point(1.0)[0], 0.0, spine_point(1.0)[1] + 26.0), 0.0,
                      "the last fin in the sequence"),
    "Target_Anchor_Center": ("body", (0.0, 0.0, BODY_Z), 0.0, "targeting and selection anchor in the torso"),
    "Emitter_Muzzle": ("head", (BODY_LEN * 0.52 + 30.0, 0.0, HEAD_Z - 3.0), 0.0,
                       "the incidental 8-damage emitter; the silhouette must not promise a fight"),
}


def assemble(lod: int, state: str = "passive"):
    m = build_body(lod, state)
    s = build_skeleton()
    counts = bind(m)
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    return m, s, counts, {n: v[0] for n, v in SOCKETS.items()}


FOOT_LIFT: dict = {}


def _plant_feet(clip) -> float:
    """Keep the feet planted for a clip that rotates limbs or the body, and measure what it costs.

    Same approach the Cairnback settled on: counter-rotate each foot by the negation of everything
    above it in the chain, then remove any residual by raising the body. Keys are REPLACED, never
    appended — an appended key at an existing time is invisible to the sampler.
    """
    def set_key(track, t, rotation, translation=(0.0, 0.0, 0.0)):
        """Replace any key already at this time, by rebuilding the track directly.

        AnimationClip.key appends AND SORTS, so the new key is not necessarily last. An earlier
        version took keys[-1] as "the one just added" and therefore deleted the key it meant to
        insert whenever the time was not the largest in the track — which silently emptied the body
        track down to a single key and made a collapse interpolate from nothing.
        """
        keys = [k for k in clip.tracks.get(track, []) if abs(k.time_s - t) > 1e-9]
        keys.append(skel.Keyframe(float(t), tuple(rotation), tuple(translation)))
        clip.tracks[track] = sorted(keys, key=lambda k: k.time_s)

    def pitch_at(track, t):
        keys = clip.tracks.get(track)
        if not keys:
            return 0.0
        if t <= keys[0].time_s:
            return keys[0].rotation_deg[0]
        if t >= keys[-1].time_s:
            return keys[-1].rotation_deg[0]
        for a, b in zip(keys, keys[1:]):
            if a.time_s <= t <= b.time_s:
                f = 0.0 if b.time_s == a.time_s else (t - a.time_s) / (b.time_s - a.time_s)
                return a.rotation_deg[0] + (b.rotation_deg[0] - a.rotation_deg[0]) * f
        return 0.0

    times = sorted({k.time_s for keys in clip.tracks.values() for k in keys})
    for tag in LEG_TAGS:
        existing = {k.time_s: k for k in clip.tracks.get(f"{tag}_foot", [])}
        for t in times:
            upstream = pitch_at("body", t) + pitch_at(f"{tag}_upper", t) + pitch_at(f"{tag}_lower", t)
            translation = existing[t].translation_cm if t in existing else (0.0, 0.0, 0.0)
            set_key(f"{tag}_foot", t, (-upstream, 0.0, 0.0), translation)

    mesh, skeleton, _c, _s = assemble(0, "passive")

    def worst():
        low = 0.0
        for i in range(65):
            low = min(low, skel.pose_mesh(mesh, skeleton, sample_pose(clip, i / 64.0)).bounds()[0][2])
        return low

    deficit = worst()
    if deficit >= 0.0:
        FOOT_LIFT[clip.name] = 0.0
        return 0.0
    lift = round(-deficit + 0.6, 2)
    body = {k.time_s: k for k in clip.tracks.get("body", [])}
    for t in times:
        base = body[t].translation_cm if t in body else (0.0, 0.0, 0.0)
        rot = body[t].rotation_deg if t in body else (0.0, 0.0, 0.0)
        set_key("body", t, rot, (base[0], base[1], base[2] + lift))
    FOOT_LIFT[clip.name] = lift
    return lift


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def new(name, ticks, loop, purpose):
        c = skel.AnimationClip(name, ticks / TICKS_PER_SECOND, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    idle = new("idle", 100, True, "listening: the head rides high and the spine breathes")
    for t, arch in ((0.0, 0.0), (2.5, 2.0), (5.0, 0.0)):
        for i in range(1, SPINE_SEGMENTS + 1):
            idle.key(f"spine_{i:02d}", t, (arch * 0.4, 0.0, 0.0))
        idle.key("neck", t, (arch, 0.0, 0.0))

    # 470 cm/s, the fastest in the faction, and canon says almost silent. A quick diagonal gait with a
    # small stride: these stilts are long, so a large rotation would move the feet a long way.
    move = new("move", 20, True, "470 cm/s and almost silent: a quick, small-stride diagonal gait")
    diagonal = {"fl": 0.0, "rr": 0.0, "fr": 0.5, "rl": 0.5}
    for tag in LEG_TAGS:
        phase = diagonal[tag]
        for step in range(5):
            t = step / 4.0 * move.duration_s
            swing = math.sin(2.0 * math.pi * (step / 4.0 + phase))
            move.key(f"{tag}_upper", t, (-4.0 * swing, 0.0, 0.0))
            move.key(f"{tag}_foot", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, 9.0 * max(0.0, swing)))

    detect = new("detect", 40, False,
                 "a contact: the spine lowers and sweeps toward the source while the fins run their "
                 "sequence. The sequence is a per-fin MATERIAL animation the runtime drives; this clip "
                 "carries only the sweep")
    for t, drop, yaw in ((0.0, 0.0, 0.0), (0.8, -9.0, 6.0), (2.0, -7.0, -4.0)):
        for i in range(1, SPINE_SEGMENTS + 1):
            detect.key(f"spine_{i:02d}", t, (drop * 0.5, yaw * 0.3, 0.0))
        detect.key("neck", t, (drop, yaw, 0.0))
        detect.key("head", t, (drop * 0.6, yaw * 1.4, 0.0))

    attack = new("attack", 24, False, "the incidental 8-damage emitter: a short head jab, no body commitment")
    for t, pitch in ((0.0, 0.0), (0.35, -8.0), (1.2, 0.0)):
        attack.key("neck", t, (pitch, 0.0, 0.0))
        attack.key("head", t, (pitch * 1.5, 0.0, 0.0))

    death = new("death", 44, False, "the delicate frame folds: the stilts give and the body comes down")
    # Measured on the rig: at thigh 60 and shin 40 these stilts support a 102 cm settle; at 26 / -34,
    # which the first version used, they support almost none and the feet went 2.8 cm under. The fold
    # and the drop are ramped together and the descent is held to 80% of what was measured.
    for i in range(9):
        t = 2.2 * i / 8.0
        u = i / 8.0
        for tag in LEG_TAGS:
            death.key(f"{tag}_upper", t, (60.0 * u, 0.0, 0.0))
            death.key(f"{tag}_lower", t, (40.0 * u, 0.0, 0.0))
        death.key("body", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, -82.0 * u))
        for j in range(1, SPINE_SEGMENTS + 1):
            death.key(f"spine_{j:02d}", t, (-10.0 * u, 0.0, 0.0))
        death.key("neck", t, (-22.0 * u, 0.0, 0.0))

    for c in clips:
        if not skel.is_frame_aligned(c.duration_s):
            skel.retime_clip(c, skel.frame_aligned_duration(c.duration_s))
    FOOT_LIFT.clear()
    for c in clips:
        _plant_feet(c)
    return clips


def sample_pose(clip: skel.AnimationClip, fraction: float) -> dict:
    t = clip.duration_s * fraction
    pose = {}
    for bone, keys in clip.tracks.items():
        if not keys:
            continue
        if t <= keys[0].time_s:
            k = keys[0]
        elif t >= keys[-1].time_s:
            k = keys[-1]
        else:
            for a, b in zip(keys, keys[1:]):
                if a.time_s <= t <= b.time_s:
                    f = 0.0 if b.time_s == a.time_s else (t - a.time_s) / (b.time_s - a.time_s)
                    pose[bone] = tuple(a.rotation_deg[i] + (b.rotation_deg[i] - a.rotation_deg[i]) * f for i in range(3)) + \
                                 tuple(a.translation_cm[i] + (b.translation_cm[i] - a.translation_cm[i]) * f for i in range(3))
                    break
            continue
        pose[bone] = (*k.rotation_deg, *k.translation_cm)
    return pose


def posed(lod: int, pose: dict, state: str = "passive") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("move", 0.25, "passive"), ("detect", 0.4, "detecting"),
                ("detect", 1.0, "detecting"), ("death", 1.0, "passive")]


def slot_area_fraction(m: kit.Mesh, slot_name: str) -> float:
    index = m.slots.index(slot_name) if slot_name in m.slots else None
    total = per = 0.0
    for poly in m.polygons:
        pts = poly.points
        area = 0.0
        for i in range(1, len(pts) - 1):
            a = kit.v_sub(pts[i], pts[0])
            b = kit.v_sub(pts[i + 1], pts[0])
            area += 0.5 * kit.v_len(kit.v_cross(a, b))
        total += area
        if poly.slot == index:
            per += area
    return per / total if total else 0.0


def lit_fins(m: kit.Mesh) -> list:
    return sorted({int(p.component.split("_")[1]) for p in m.polygons
                   if p.component.startswith("fin_") and p.component.endswith("_vein")
                   and m.slots[p.slot] == AMBER})


# The other two Kharuun quadrupeds, for the silhouette separation the owner required.
OTHER_QUADRUPEDS = {
    "riftstalker": {"height_over_extent": 0.68, "source": "ArtSource/EBS-KHA-UNT-002/build-manifest.json"},
    "cairnback": {"height_over_extent": 0.71, "source": "ArtSource/EBS-KHA-UNT-003/build-manifest.json"},
}


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "quadruped_on_stilts": {"contract": 4,
                                "legs": len({c.split("_")[0] for c in comps if c.split("_")[0] in LEG_TAGS}),
                                "limb_radius_cm": LIMB_R},
        "arched_spine": {"contract": "an arch from the shoulders to the neck",
                         "segments": sum(1 for c in comps if c.startswith("spine_")),
                         "rises": spine_point(1.0)[1] > spine_point(0.0)[1]},
        "fin_array": {"contract": FIN_COUNT,
                      "built": sum(1 for c in comps if c.startswith("fin_") and not c.endswith("_vein")),
                      "veins": sum(1 for c in comps if c.endswith("_vein")),
                      "individually_addressable": True,
                      "note": "one component per fin so the runtime can light them in sequence"},
        "small_head": {"contract": "carried high, no fight in the silhouette", "built": "head" in comps},
        "bones": {"contract": "24: root, body, a four-segment spine, neck, head, four stilt chains",
                  "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["idle", "move", "detect", "attack", "death"],
                   "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    extent = max(x1 - x0, y1 - y0)
    return {
        "standing_height_cm": round(z1, 2),
        "body_length_cm": round(x1 - x0, 2),
        "max_horizontal_extent_cm": round(extent, 2),
        "height_over_extent": round(z1 / extent, 4),
        "limb_radius_cm": LIMB_R,
        "fin_count": FIN_COUNT,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "feet_on_the_ground": abs(z0) < 1.0,
        "taller_than_it_is_long": z1 > (x1 - x0),
        "separation_from_the_other_quadrupeds": {
            name: {"theirs": data["height_over_extent"],
                   "mine": round(z1 / extent, 4),
                   "separation": round(z1 / extent - data["height_over_extent"], 4)}
            for name, data in OTHER_QUADRUPEDS.items()},
    }
def export(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, _c, socks = assemble(lod, "passive")
        base = os.path.join(out_dir, f"{ASSET}_LOD{lod}")
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION,
                  "lod": lod, "author": AUTHOR, "card": CARD}
        if skinned:
            digest = skel.write_skinned_glb(m, s, base + ".glb", animations=clips, extras=extras,
                                            include_collision=False, sockets_on_bones=socks)
            outputs.append({"mesh": ASSET, "lod": lod, "kind": "skinned + clips",
                            "path": os.path.relpath(base + ".glb", HERE), "sha256": digest,
                            "triangles": m.triangle_count(), "bounds_cm": [list(p) for p in m.bounds()],
                            "section_slot_names": m.slots, "by_slot": m.triangle_count_by("slot"),
                            "clips": [c.name for c in clips],
                            "sockets": [{"name": sk.name, "bone": socks[sk.name],
                                         "position_cm": [round(v, 2) for v in sk.position],
                                         "yaw_deg": sk.yaw_deg, "purpose": sk.purpose} for sk in m.sockets]})
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (intact)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count()})
    m1 = assemble(1, "passive")[0]
    path = os.path.join(review, f"{ASSET}_passive_LOD1.obj")
    review_rows.append({"name": "passive_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]),
                        "triangles": m1.triangle_count()})
    by_name = {c.name: c for c in clips}
    for name, fraction, state in POSE_SAMPLES:
        mesh = posed(0, sample_pose(by_name[name], fraction), state)
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        p = os.path.join(review, stem + ".obj")
        review_rows.append({"name": stem, "path": os.path.relpath(p, evidence_dir),
                            "sha256": mesh.write_obj(p, header_lines=[f"{ASSET} posed: {name} at {fraction:.2f}"]),
                            "lowest_z_cm": round(mesh.bounds()[0][2], 2)})
    return {"outputs": outputs, "review": review_rows, "clips": clips}


def manifest(exported: dict) -> dict:
    m0, s, counts, _socks = assemble(0, "passive")
    m1 = assemble(1, "passive")[0]
    detecting = assemble(0, "detecting")[0]
    clips = exported["clips"]
    # the fins only enter the amber slot while detecting, so the cap is measured on THAT state
    amber = slot_area_fraction(detecting, AMBER)
    worst = max(m0.triangle_count(), detecting.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "ground-contact centre between the feet", "nanite": False},
        "scale_basis": {"canon": "SPEC-UNIT-008: a tall, thin scout with sensor-fins of translucent amber along the spine and head, and a delicate frame",
                        "standing_height_cm": H, "body_length_cm": BODY_LEN,
                        "basis": "the tallest Kharuun unit and the lightest: 85 health against the Cairnback's 245",
                        "gameplay": "Content/Data/Source/units.json ka_resonant"},
        "material_slots": [STRATA, AMBER],
        "material_slot_policy": ("Two slots: dark faceted plate for the torso, spine, stilts and head, and amber for the "
                                 "translucent sensor fins, which are the only emissive. The fins carry a translucency mask "
                                 "in texture."),
        "silhouette_first": {
            "directive": "Owner ruling 2026-09-07: roster members sharing an anatomy must separate in monochrome at tactical distance.",
            "this_asset": ("The third Kharuun quadruped, so it must separate from BOTH the others. Its own separation is that it "
                           "is taller than it is long, which neither of them is."),
            "measures": measurements(m0)["separation_from_the_other_quadrupeds"]},
        "anatomy_correction": {
            "what": "An earlier draft of the provisional card described a slender BIPED with 26 bones.",
            "why_wrong": "Written before the candidate was traced. Its LISTENING and DETECTING views both show FOUR stilt legs.",
            "status": "SUPERSEDED AND NOT GUIDANCE. The quadruped plan is the sole anatomy for this asset.",
            "pattern": ("The third of my eight Kharuun cards to carry the wrong anatomy, all authored in one pass before their "
                        "concepts were traced. The faction rules now require the trace first."),
            "source_references": [
                "BuildArtifacts/Evidence/concept-discovery-20260906/resonant-review/resonant-candidate.png (LISTENING, DETECTING, SENSOR FINS)",
                "BuildArtifacts/Evidence/asset-production-20260906T221157Z/concept-crops/all/EBS-PKG-KA-RESONANT/EBS-CON-KHA-UNT-004.png"]},
        "detection_readout": {
            "fin_count": FIN_COUNT,
            "addressing": "one component per fin, ordered along the spine, so the runtime can light them in sequence",
            "note": ("The state assemblies show all-dark and all-lit only. The ordered sequence toward a source is a per-fin "
                     "MATERIAL animation and is not represented in geometry.")},
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/kharuun-asset-cards.json (rendered to kharuun-asset-cards.md)",
            "status": ("PROVISIONAL. Authored in this worktree under the owner ruling of 2026-09-07; NOT incorporated into "
                       "Docs/Requirements.md and not an existing authoritative per-asset requirement."),
            "bounds": {"lod0_triangles": 4500, "lod1_triangles": 1800, "basis": "matched to the Tender, the other light non-combat frame"}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "detecting_state_triangles": detecting.triangle_count(), "worst_state_triangles": worst,
                    "lod0_cap": 4500, "lod1_cap": 1800, "cap_source": f"{CARD} (PROVISIONAL)",
                    "cap_scope": "the complete assembly; the mineral cover is a separate asset",
                    "lod0_within_cap": worst <= 4500, "lod1_within_cap": m1.triangle_count() <= 1800,
                    "headroom_note": ("Owner ruling 2026-09-07: sitting below the ceiling is HEADROOM, not sufficiency. "
                                      "Silhouette, joint and carapace detail are judged against the concept at gameplay "
                                      "distance and have not been judged here."),
                    "amber_area_fraction_detecting": round(amber, 5), "amber_cap": 0.15,
                    "amber_within_cap": amber <= 0.15},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("The corrected card's plan: a slender quadruped with a four-segment spine carrying the fin array, "
                           "weighted toward the spine and neck rather than the limbs — the inverse of the Cairnback's "
                           "weighting. No root motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "foot_planting": {"body_lift_cm": dict(FOOT_LIFT),
                          "why": ("Long stilts magnify any rotation at the hip, so each foot is counter-rotated by the negation "
                                  "of everything above it and the residual is removed by raising the body. The lifts are "
                                  "MEASURED on the posed mesh, not dialled in by eye.")},
        "gait_separation": {"resonant": "a quick small-stride diagonal gait at 470 cm/s, almost silent",
                            "riftstalker": "a diagonal gait at 410 cm/s",
                            "cairnback": "a lateral sequence at 270 cm/s",
                            "note": ("This one shares the Riftstalker's diagonal pattern; its separation is carried by "
                                     "proportion and the fin array, not by the gait.")},
        "states": {"passive": "the shell whole", "chipped": "strata visibly broken, per canon's 'damage chips strata'"},
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py", "silhouette": "ArtSource/tools/ebs_silhouette.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "animation": "NOT_EVALUATED",
                       "technical": ("PENDING — built against the provisional card; final technical acceptance waits on that card "
                                     "being incorporated into the authoritative requirements and its checks passing"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/resonant-review/resonant-candidate.png",
                            "concepts": ["EBS-CON-KHA-UNT-004"],
                            "canon": "DevelopmentBible.md SPEC-UNIT-008",
                            "gameplay": "Content/Data/Source/units.json ka_resonant"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {STRATA: [0.21, 0.20, 0.19], AMBER: [0.98, 0.66, 0.22], "_default": [0.5, 0.5, 0.5]},
            "emissive": [AMBER], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.32, "key": 0.85, "color": [1.0, 0.86, 0.7],
                      "fill_color": [0.5, 0.55, 0.7], "fill": 0.24},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [200, 200], "footprint_color": [0.98, 0.66, 0.22]},
            "reference_figure": {"height_cm": 180, "position": [-300, -300, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "side", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 110]},
             {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.12, "target": [0, 0, 110]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -14, "yaw_deg": 140, "arm_cm": 700, "fov_deg": 52, "target": [0, 0, 120]},
             {"name": "fin_detail", "type": "persp", "pitch_deg": -12, "yaw_deg": 158, "arm_cm": 340, "fov_deg": 48, "target": [40, 0, 200]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 0]},
             # matched EXACTLY to the Riftstalker's monochrome passes so the two can be compared
             {"name": "tactical_monochrome", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 1400,
              "fov_deg": 55, "target": [0, 0, 0], "grayscale": True},
             {"name": "side_monochrome", "type": "ortho", "from": "+Y", "edges": False, "margin": 1.12,
              "target": [0, 0, 100], "grayscale": True}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_passive_LOD1.obj"}], ortho[:2] + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [ortho[0], views[0]])
    # A dedicated silhouette scene: NO ground and NO reference figure, so a mask sees the animal and
    # nothing else. Measuring the tactical pass with the ground in frame compared the ground.
    sil = dict(base)
    sil.pop("ground", None)
    sil.pop("reference_figure", None)
    sil["background"] = [1.0, 1.0, 1.0]
    sil["meshes"] = [{"obj": f"../review/{ASSET}_passive_LOD0.obj"}]
    sil["views"] = [
        {"name": "sil_side", "type": "ortho", "from": "+Y", "edges": False, "margin": 1.06,
         "target": [0, 0, 110], "grayscale": True},
        {"name": "sil_tactical", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 1400,
         "fov_deg": 55, "target": [0, 0, 60], "grayscale": True}]
    sp = os.path.join(scenes, "silhouette.json")
    with open(sp, "w", encoding="utf-8") as handle:
        json.dump(sil, handle, indent=1)
    written.append(sp)
    return written


def build_all(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    return manifest(export(evidence_dir, out_dir, skinned))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--skinned", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            ev = os.path.join(tmp, "evidence"); os.makedirs(ev, exist_ok=True)
            fresh = build_all(ev, os.path.join(tmp, "export"), True)
        with open(manifest_path, encoding="utf-8") as handle:
            saved = json.load(handle)
        drift, missing = [], []
        saved_out = {os.path.basename(o["path"]): o["sha256"] for o in saved["outputs"]}
        for o in fresh["outputs"]:
            name = os.path.basename(o["path"])
            if name not in saved_out:
                missing.append(name)
            elif saved_out[name] != o["sha256"]:
                drift.append(name)
        saved_rev = {os.path.basename(r["path"]): r["sha256"] for r in saved.get("review_assemblies", [])}
        for r in fresh["review_assemblies"]:
            key = os.path.basename(r["path"])
            if key not in saved_rev:
                missing.append(key)
            elif saved_rev[key] != r["sha256"]:
                drift.append(key)
        if saved.get("revision") != REVISION:
            drift.append(f"revision {saved.get('revision')} != {REVISION}")
        print(json.dumps({"check": "ok" if not drift and not missing else "drift", "revision": REVISION,
                          "drift": drift, "missing": missing,
                          "compared": {"outputs": len(fresh["outputs"]), "review": len(fresh["review_assemblies"])}}))
        return 0 if not drift and not missing else 1
    os.makedirs(args.evidence_dir, exist_ok=True)
    data = build_all(args.evidence_dir, os.path.join(HERE, "export"), args.skinned)
    scenes = write_scenes(args.evidence_dir)
    data["review"] = {"scenes": [os.path.relpath(s, args.evidence_dir) for s in scenes]}
    with open(manifest_path, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=1, sort_keys=True)
    mm = data["concept_measurements"]
    lows = [r["lowest_z_cm"] for r in data["review_assemblies"] if "lowest_z_cm" in r]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"],
                      "lod1": data["budgets"]["lod1_triangles"],
                      "height_cm": mm["standing_height_cm"], "length_cm": mm["body_length_cm"],
                      "extent_cm": mm["max_horizontal_extent_cm"],                       "height_over_extent": mm["height_over_extent"], "taller_than_long": mm["taller_than_it_is_long"],
                      "fins": mm["fin_count"],
                      "amber_area": data["budgets"]["amber_area_fraction_detecting"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
