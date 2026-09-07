#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-UNT-003 — the Kharuun Assemblies Cairnback.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-FAC-025.KA.CAIRNBACK.ASSET`: a broad,
low assault warform whose domed shell of layered strata courses carries nearly all of its mass, on four
short thick legs, with a low head tucked under the shell's front edge.

Silhouette first (owner ruling 2026-09-07). This asset is built to CONTRAST with the Riftstalker: leg
share of height 0.24 against its 0.69, and body width over length 0.70 against its 0.26. Both are
asserted by test, and a monochrome silhouette comparison is produced as evidence.

The mineral cover it creates is a SEPARATE asset with its own budget and is not built here.

Usage:
  python3 build_cairnback.py --evidence-dir "<root>/EBS-KHA-UNT-003" [--skinned]
  python3 build_cairnback.py --evidence-dir "<root>/EBS-KHA-UNT-003" --check

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
PACKAGE_ID = "EBS-PKG-KA-CAIRNBACK"
PRODUCTION_ID = "EBS-KHA-UNT-003"
ASSET = "SK_EBS_KHA_UNT_003"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_003/"
REVISION = "ebs-kha-unt-003-concept-v1"
CARD = "REL-FAC-025.KA.CAIRNBACK.ASSET"

STRATA = "MI_EBS_KHA_Strata"      # layered shell courses, limbs, head
AMBER = "MI_EBS_KHA_Amber"        # seams between shell courses — the only emissive

TICKS_PER_SECOND = 20.0

H = 228.0                              # standing height at the shell crown
BODY_LEN = 330.0                       # 1.45 of the height, as traced
SHELL_W = 236.0                        # body width: 0.72 of the length — broad, unlike the Riftstalker
LEG_CLEAR = 54.0                       # ground to the shell's lower edge: 0.24 of the height
SHELL_COURSES = 6                      # stacked strata rings forming the dome
SHELL_BACK_X, SHELL_FRONT_X = -168.0, 150.0
HEAD_X, HEAD_Z = 92.0, 74.0            # low and TUCKED: the snout must finish BEHIND the shell's
                                       # front edge at 150, which a first pass at 118 did not
LEG_X, LEG_Y = 96.0, 82.0              # short thick legs, set inside the shell's overhang
LEG_R = 30.0
STATES = ("intact", "chipped")

LEGS = (("fl", LEG_X, -LEG_Y), ("fr", LEG_X, LEG_Y), ("rl", -LEG_X, -LEG_Y), ("rr", -LEG_X, LEG_Y))
LEG_TAGS = tuple(t[0] for t in LEGS)

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "ground-contact centre between the feet; the runtime owns translation"),
    ("body", "root", (0.0, 0.0, LEG_CLEAR + 18.0), "the low body the shell and limbs hang from"),
    ("slab_lower", "body", (-20.0, 0.0, LEG_CLEAR + 62.0), "back-slab chain 1: the shell flexes here on the heave"),
    ("slab_upper", "slab_lower", (-40.0, 0.0, LEG_CLEAR + 118.0), "back-slab chain 2: the crown of the shell"),
    ("neck", "body", (HEAD_X - 46.0, 0.0, HEAD_Z + 10.0), "a SHORT protected neck; the head never leads the shell"),
    ("head", "neck", (HEAD_X, 0.0, HEAD_Z), "the low head, tucked under the shell's front edge"),
]
for _tag, _lx, _ly in LEGS:
    BONES.append((f"{_tag}_hip", "body", (_lx, _ly, LEG_CLEAR + 10.0), f"{_tag} limb: attachment at the body"))
    BONES.append((f"{_tag}_upper", f"{_tag}_hip", (_lx, _ly, LEG_CLEAR + 10.0),
                  f"{_tag} limb: thick upper segment, pivoting at the hip"))
    BONES.append((f"{_tag}_lower", f"{_tag}_upper", (_lx, _ly * 1.06, LEG_CLEAR * 0.5),
                  f"{_tag} limb: short lower segment, pivoting at the knee"))
    BONES.append((f"{_tag}_foot", f"{_tag}_lower", (_lx, _ly * 1.06, 16.0), f"{_tag} limb: the broad foot"))


def shell_course(index: int):
    """(x centre, z bottom, z top, half length, half width) for one stacked strata course.

    The dome is built from rings that step inward as they rise, each overhanging the one below at its
    widest point. This is the asset's whole silhouette, so it is defined before anything else is.
    """
    t = index / (SHELL_COURSES - 1)
    z0 = LEG_CLEAR + (H - LEG_CLEAR) * index / SHELL_COURSES
    z1 = LEG_CLEAR + (H - LEG_CLEAR) * (index + 1) / SHELL_COURSES
    shrink = math.sqrt(max(0.0, 1.0 - (t * 0.94) ** 2))
    half_len = (SHELL_FRONT_X - SHELL_BACK_X) / 2.0 * (0.62 + 0.38 * shrink)
    half_w = SHELL_W / 2.0 * (0.60 + 0.40 * shrink)
    x = (SHELL_FRONT_X + SHELL_BACK_X) / 2.0 - 14.0 * t
    return x, z0, z1, half_len, half_w


def _oval(half_len: float, half_w: float, sides: int, phase_deg: float = 0.0):
    pts = []
    for k in range(sides):
        a = math.radians(phase_deg) + 2.0 * math.pi * k / sides
        pts.append((half_len * math.cos(a), half_w * math.sin(a)))
    return pts


def build_body(lod: int, state: str = "intact") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    amber = m.slot(AMBER)
    fine = lod == 0
    sides = 12 if fine else 8

    # 1. the shell: the silhouette, built first and deliberately dominant
    for i in range(SHELL_COURSES):
        x, z0, z1, half_len, half_w = shell_course(i)
        m.prism(_oval(half_len, half_w, sides, phase_deg=7.0 * i), z0, z1, strata,
                f"shell_course_{i + 1:02d}", center=(x, 0.0),
                cap_bottom=(i == 0), cap_top=(i == SHELL_COURSES - 1))
        if fine and i < SHELL_COURSES - 1:
            m.ring(_oval(half_len + 2.0, half_w + 2.0, sides, phase_deg=7.0 * i),
                   _oval(half_len - 2.0, half_w - 2.0, sides, phase_deg=7.0 * i),
                   z1 - 3.0, z1, amber, f"seam_{i + 1:02d}", center=(x, 0.0))

    # 2. the low body under the shell, and four short thick limbs set inside its overhang
    m.box((0.0, 0.0, LEG_CLEAR + 16.0), (BODY_LEN * 0.62, SHELL_W * 0.58, 44.0), strata, "underbody")
    for tag, lx, ly in LEGS:
        m.tube((lx, ly, LEG_CLEAR + 10.0), (lx, ly * 1.06, LEG_CLEAR * 0.5), LEG_R, sides // 2, strata,
               f"{tag}_upper")
        m.tube((lx, ly * 1.06, LEG_CLEAR * 0.5), (lx, ly * 1.06, 16.0), LEG_R * 0.86, sides // 2, strata,
               f"{tag}_lower")
        m.box((lx + 6.0, ly * 1.06, 8.0), (76.0, 62.0, 16.0), strata, f"{tag}_foot")

    # 3. the head: LOW and TUCKED, behind the shell's front edge. The Riftstalker's prow leads its
    #    body; this one hides under its own roof, which is most of the two units' head-on separation.
    m.tube((HEAD_X - 62.0, 0.0, HEAD_Z + 12.0), (HEAD_X - 18.0, 0.0, HEAD_Z + 4.0), 30.0, sides // 2,
           strata, "neck")
    m.box((HEAD_X, 0.0, HEAD_Z), (74.0, 62.0, 44.0), strata, "head")
    if fine:
        m.box((HEAD_X + 30.0, 0.0, HEAD_Z - 4.0), (22.0, 44.0, 26.0), strata, "snout")

    if state == "chipped":
        # canon: damage chips strata
        for k, (idx, yaw) in enumerate(((4, 34.0), (3, 168.0), (5, 250.0)), start=1):
            x, z0, z1, half_len, half_w = shell_course(idx)
            a = math.radians(yaw)
            m.box((x + half_len * math.cos(a) * 0.92, half_w * math.sin(a) * 0.92, (z0 + z1) / 2.0),
                  (52.0, 44.0, (z1 - z0) + 10.0), strata, f"chip_{k:02d}", yaw_deg=yaw)

    m.collision.append(kit.CollisionBox("shell", (0.0, 0.0, (H + LEG_CLEAR) / 2.0),
                                        (BODY_LEN * 0.94, SHELL_W, H - LEG_CLEAR)))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith(("shell_course_", "seam_", "chip_")):
            index = int(c.split("_")[-1]) if c.split("_")[-1].isdigit() else 1
            by_component[c] = "slab_upper" if index >= SHELL_COURSES // 2 else "slab_lower"
        elif c == "underbody":
            by_component[c] = "body"
        elif c == "neck":
            by_component[c] = "neck"
        elif c in ("head", "snout"):
            by_component[c] = "head"
        else:
            for tag in LEG_TAGS:
                if c.startswith(tag + "_"):
                    by_component[c] = f"{tag}_{c.split('_')[-1]}"
                    break
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Cover_Cast_Origin": ("head", (HEAD_X + 40.0, 0.0, HEAD_Z), 0.0,
                          "where the heave throws its barrier from: ahead of the head, on the ground line"),
    "Back_Slab_Center": ("slab_upper", (shell_course(SHELL_COURSES - 1)[0], 0.0, H - 12.0), 0.0,
                         "the shell's crown: the 'absorbs fire, becomes cover' anchor"),
    "Target_Anchor_Center": ("body", (0.0, 0.0, LEG_CLEAR + 60.0), 0.0, "targeting and selection anchor"),
    "Foot_Contact_FL": ("fl_foot", (LEG_X + 6.0, -LEG_Y * 1.06, 0.0), 0.0, "front-left ground contact"),
    "Foot_Contact_FR": ("fr_foot", (LEG_X + 6.0, LEG_Y * 1.06, 0.0), 0.0, "front-right ground contact"),
}


def assemble(lod: int, state: str = "intact"):
    m = build_body(lod, state)
    s = build_skeleton()
    counts = bind(m)
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    return m, s, counts, {n: v[0] for n, v in SOCKETS.items()}


_SAFE_DROP_CACHE: dict = {}


def _safe_drop(fold: float, shin: float, margin: float = 0.6) -> float:
    """Greatest body descent keeping the whole posed mesh above ground, times a margin.

    Measured on the real posed mesh. The Riftstalker package learned the hard way that analytic
    models of this get the rotation sign or the monotonicity wrong; the rig is the authority.
    """
    key = (round(fold, 3), round(shin, 3))
    if key not in _SAFE_DROP_CACHE:
        mesh, skeleton, _c, _s = assemble(0, "intact")

        def lowest(drop):
            pose = {"body": (0.0, 0.0, 0.0, 0.0, 0.0, drop)}
            for tag in LEG_TAGS:
                pose[f"{tag}_upper"] = (fold, 0.0, 0.0, 0.0, 0.0, 0.0)
                pose[f"{tag}_lower"] = (shin, 0.0, 0.0, 0.0, 0.0, 0.0)
            return skel.pose_mesh(mesh, skeleton, pose).bounds()[0][2]

        lo, hi = 0.0, 120.0
        for _ in range(28):
            mid = (lo + hi) / 2.0
            if lowest(-mid) >= 0.0:
                lo = mid
            else:
                hi = mid
        _SAFE_DROP_CACHE[key] = lo
    return _SAFE_DROP_CACHE[key] * margin



FOOT_LIFT: dict = {}   # clip name -> centimetres the body was raised so the feet stayed planted


def _plant_feet(clip) -> float:
    """Keep the feet flat and on the ground for a clip that rotates limbs or the body.

    This animal's legs are stubby vertical tubes ending in flat foot plates, so ANY rotation in the
    chain swings a plate corner underground — a 10 degree fold costs about 6 cm. Two things fix it:
    each foot is counter-rotated by the negation of everything above it in the chain, which keeps the
    plate flat, and whatever penetration survives that is removed by raising the body. The lift is
    measured on the posed mesh and reported in the manifest rather than dialled in by eye.
    """
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

    def set_key(track, t, rotation, translation=(0.0, 0.0, 0.0)):
        """Replace any key already at this time. AnimationClip.key APPENDS, and a duplicate at the
        same time is invisible to the sampler, which reads the first match — so an appended
        correction silently did nothing."""
        clip.key(track, t, rotation, translation_cm=translation)
        keys = clip.tracks[track]
        latest = keys[-1]
        clip.tracks[track] = sorted([k for k in keys[:-1] if abs(k.time_s - t) > 1e-9] + [latest],
                                    key=lambda k: k.time_s)

    times = sorted({k.time_s for keys in clip.tracks.values() for k in keys})
    for tag in LEG_TAGS:
        for t in times:
            upstream = pitch_at("body", t) + pitch_at(f"{tag}_upper", t) + pitch_at(f"{tag}_lower", t)
            existing = {k.time_s: k for k in clip.tracks.get(f"{tag}_foot", [])}
            translation = existing[t].translation_cm if t in existing else (0.0, 0.0, 0.0)
            set_key(f"{tag}_foot", t, (-upstream, 0.0, 0.0), translation)

    mesh, skeleton, _c, _s = assemble(0, "intact")
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

    idle = new("idle", 100, True, "settled: the shell rides steady and the limbs hold")
    for t, dz in ((0.0, 0.0), (2.5, 1.5), (5.0, 0.0)):
        idle.key("body", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, dz))

    # 270 cm/s against the Riftstalker's 410: a slow lateral-sequence walk, not a fast diagonal one.
    # The gait is part of the separation the owner asked for, so the phases differ deliberately.
    move = new("move", 40, True, "270 cm/s: heavy stone footfalls, one limb at a time")
    lateral = {"fl": 0.0, "rl": 0.25, "fr": 0.5, "rr": 0.75}
    for tag in LEG_TAGS:
        phase = lateral[tag]
        for step in range(5):
            # the key TIME walks the cycle; the swing is read at that time PLUS the limb's phase.
            # Deriving both from the same value gave every limb an identical curve and the phase
            # offset did nothing at all.
            t = step / 4.0 * move.duration_s
            swing = math.sin(2.0 * math.pi * (step / 4.0 + phase))
            # a small stride: rotating this stubby leg by 6 degrees dropped the body 11 cm, and a
            # constant lift that large would pop against the idle pose on transition
            move.key(f"{tag}_upper", t, (-2.5 * swing, 0.0, 0.0))
            move.key(f"{tag}_foot", t, (0.0, 0.0, 0.0),
                     # no base offset: a stance foot stays planted and only the swing foot lifts
                     translation_cm=(0.0, 0.0, 7.0 * max(0.0, swing)))
    for t, dz in ((0.0, 0.0), (1.0, -2.0), (2.0, 0.0)):
        move.key("body", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, dz))

    attack = new("attack", 28, False, "a short heavy strike at 200 cm: the body lunges, the shell stays level")
    # the body lunges forward but does NOT pitch: pitching this animal drives its flat foot plates
    # straight into the ground, so the strike is carried by the neck instead
    for t, pitch, dx in ((0.0, 0.0, 0.0), (0.4, 8.0, 14.0), (1.4, 0.0, 0.0)):
        attack.key("body", t, (0.0, 0.0, 0.0), translation_cm=(dx, 0.0, 0.0))
        attack.key("neck", t, (pitch, 0.0, 0.0))

    heave = new("heave", 60, False,
                "canon's heave: the shell rears, the forelimbs drive down, and a grown barrier is left "
                "behind it. The barrier is a SEPARATE asset; this clip only produces the motion and the "
                "cast origin the runtime places it from")
    for t, slab, dz in ((0.0, 0.0, 0.0), (0.9, -14.0, 0.0), (1.8, 10.0, 0.0), (3.0, 0.0, 0.0)):
        heave.key("slab_lower", t, (slab, 0.0, 0.0))
        heave.key("slab_upper", t, (slab * 1.4, 0.0, 0.0))
        heave.key("body", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, dz))
    for tag in ("fl", "fr"):
        for t, fold in ((0.0, 0.0), (0.9, -16.0), (1.8, 18.0), (3.0, 0.0)):
            heave.key(f"{tag}_upper", t, (fold, 0.0, 0.0))

    chip = new("damage_chip", 16, False, "a hit lands: the shell shrugs and settles")
    for t, roll in ((0.0, 0.0), (0.2, 4.0), (0.8, 0.0)):
        chip.key("slab_upper", t, (0.0, 0.0, roll))

    death = new("death", 50, False, "canon's ceramic slump: the limbs give and the shell settles onto the ground")
    shape = [(2.5 * i / 8.0, i / 8.0, 16.0 * i / 8.0, 40.0 * i / 8.0) for i in range(9)]
    for t, share, fold, shin in shape:
        death.key("body", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, -share * _safe_drop(fold, shin)))
        for tag in LEG_TAGS:
            death.key(f"{tag}_upper", t, (fold, 0.0, 0.0))
            death.key(f"{tag}_lower", t, (shin, 0.0, 0.0))

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


def posed(lod: int, pose: dict, state: str = "intact") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("move", 0.25, "intact"), ("attack", 0.3, "intact"),
                ("heave", 0.3, "intact"), ("heave", 0.6, "intact"), ("death", 1.0, "chipped")]


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


# The Riftstalker's built figures, for the silhouette contrast the owner required on 2026-09-07.
RIFTSTALKER = {"leg_share_of_height": 0.69, "width_over_length": 0.26, "height_cm": 201.0,
               "length_cm": 295.19, "source": "ArtSource/EBS-KHA-UNT-002/build-manifest.json"}


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "domed_shell": {"contract": SHELL_COURSES,
                        "courses": sum(1 for c in comps if c.startswith("shell_course_")),
                        "seams": sum(1 for c in comps if c.startswith("seam_"))},
        "short_thick_limbs": {"contract": 4,
                              "legs": len({c.split("_")[0] for c in comps if c.split("_")[0] in LEG_TAGS}),
                              "clearance_cm": LEG_CLEAR},
        "low_protected_head": {"contract": "tucked behind the shell's front edge",
                               "built": "head" in comps,
                               "head_front_x": round(m.component_bounds("head")[1][0], 2),
                               "shell_front_x": round(max(m.component_bounds(c)[1][0]
                                                          for c in comps if c.startswith("shell_course_")), 2)},
        "mineral_cover": {"contract": "a SEPARATE asset with its own budget",
                          "built_here": False,
                          "cast_origin_socket": "Cover_Cast_Origin"},
        "bones": {"contract": "22: root, body, a two-bone back slab, a short neck and head, four limb chains",
                  "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["idle", "move", "attack", "heave", "damage_chip", "death"],
                   "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    length = x1 - x0
    width = y1 - y0
    shell_bottom = min(m.component_bounds(c)[0][2] for c in m.components() if c.startswith("shell_course_"))
    return {
        "standing_height_cm": round(z1, 2),
        "body_length_cm": round(length, 2),
        "body_width_cm": round(width, 2),
        "length_over_height": round(length / z1, 4),
        "leg_share_of_height": round(shell_bottom / z1, 4),
        "width_over_length": round(width / length, 4),
        "head_is_tucked_behind_the_shell": m.component_bounds("head")[1][0] < max(
            m.component_bounds(c)[1][0] for c in m.components() if c.startswith("shell_course_")),
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "feet_on_the_ground": abs(z0) < 1.0,
        "contrast_with_riftstalker": {
            "leg_share": {"cairnback": round(shell_bottom / z1, 4), "riftstalker": RIFTSTALKER["leg_share_of_height"],
                          "separation": round(RIFTSTALKER["leg_share_of_height"] - shell_bottom / z1, 4)},
            "width_over_length": {"cairnback": round(width / length, 4), "riftstalker": RIFTSTALKER["width_over_length"],
                                  "separation": round(width / length - RIFTSTALKER["width_over_length"], 4)},
            "requirement": "distinguishable in MONOCHROME at tactical distance (owner ruling 2026-09-07)"},
    }


def export(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, _c, socks = assemble(lod, "intact")
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
    m1 = assemble(1, "intact")[0]
    path = os.path.join(review, f"{ASSET}_intact_LOD1.obj")
    review_rows.append({"name": "intact_lod1", "path": os.path.relpath(path, evidence_dir),
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
    m0, s, counts, _socks = assemble(0, "intact")
    m1 = assemble(1, "intact")[0]
    chipped = assemble(0, "chipped")[0]
    clips = exported["clips"]
    amber = slot_area_fraction(m0, AMBER)
    worst = max(m0.triangle_count(), chipped.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "ground-contact centre between the feet", "nanite": False},
        "scale_basis": {"canon": "SPEC-UNIT-007: a broad, low assault warform whose back is a slab of layered heat-holding strata; thick forelimbs; head low and protected",
                        "standing_height_cm": H, "body_length_cm": BODY_LEN,
                        "basis": "larger than the Riftstalker's 201 x 295, as a population-3 assault unit against a population-2 skirmisher",
                        "gameplay": "Content/Data/Source/units.json ka_cairnback"},
        "material_slots": [STRATA, AMBER],
        "material_slot_policy": ("Two slots: layered strata for the shell, limbs and head, and amber restricted to the seams "
                                 "between shell courses. The back is armour, not a lamp."),
        "silhouette_first": {
            "directive": "Owner ruling 2026-09-07: establish a contrasting silhouette BEFORE detailing.",
            "how": ("The shell profile was defined first and everything else was fitted under it. The two contrast measures "
                    "below are asserted by test, and a monochrome tactical comparison against the Riftstalker's rendered "
                    "baseline is produced as evidence rather than asserted."),
            "measures": measurements(m0)["contrast_with_riftstalker"]},
        "excluded_from_this_asset": {
            "mineral_cover": ("The barrier the heave creates is a SEPARATE asset with its own budget (card .MESH_PROP). It is "
                              "not built here and not counted in these triangles. This asset provides only the motion and the "
                              "Cover_Cast_Origin socket the runtime places it from.")},
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/kharuun-asset-cards.json (rendered to kharuun-asset-cards.md)",
            "status": ("PROVISIONAL. Authored in this worktree under the owner ruling of 2026-09-07; NOT incorporated into "
                       "Docs/Requirements.md and not an existing authoritative per-asset requirement."),
            "bounds": {"lod0_triangles": 8000, "lod1_triangles": 3500, "basis": "the faction default"}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "chipped_state_triangles": chipped.triangle_count(), "worst_state_triangles": worst,
                    "lod0_cap": 8000, "lod1_cap": 3500, "cap_source": f"{CARD} (PROVISIONAL)",
                    "cap_scope": "the complete assembly; the mineral cover is a separate asset",
                    "lod0_within_cap": worst <= 8000, "lod1_within_cap": m1.triangle_count() <= 3500,
                    "headroom_note": ("Owner ruling 2026-09-07: sitting below the ceiling is HEADROOM, not sufficiency. "
                                      "Silhouette, joint and carapace detail are judged against the concept at gameplay "
                                      "distance and have not been judged here."),
                    "amber_area_fraction_lod0": round(amber, 5), "amber_cap": 0.15,
                    "amber_within_cap": amber <= 0.15},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("The card's heavy low-slung plan: four thick limb chains, a short protected neck, and a back-slab "
                           "chain that flexes on the heave. Weighted toward the limbs and the back, the inverse of the "
                           "Resonant's plan and unlike the Riftstalker's long-limbed one. No root motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "foot_planting": {"body_lift_cm": dict(FOOT_LIFT),
                          "why": ("Stubby vertical legs on flat foot plates: any rotation in the chain swings a plate corner "
                                  "underground. Each foot is counter-rotated by the negation of everything above it, and the "
                                  "residual is removed by raising the body. These lifts are MEASURED on the posed mesh, not "
                                  "dialled in by eye.")},
        "gait_separation": {"cairnback": "lateral sequence, one limb at a time, 270 cm/s",
                            "riftstalker": "diagonal pairs, 410 cm/s",
                            "why": "the owner asked for a different stance AND gait, so the phase pattern differs deliberately"},
        "states": {"intact": "the shell whole", "chipped": "strata visibly broken, per canon's 'damage chips strata'"},
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py", "silhouette": "ArtSource/tools/ebs_silhouette.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "animation": "NOT_EVALUATED",
                       "technical": ("PENDING — built against the provisional card; final technical acceptance waits on that card "
                                     "being incorporated into the authoritative requirements and its checks passing"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/cairnback-review/cairnback-candidate.png",
                            "concepts": ["EBS-CON-KHA-UNT-003"],
                            "canon": "DevelopmentBible.md SPEC-UNIT-007",
                            "gameplay": "Content/Data/Source/units.json ka_cairnback"},
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
             {"name": "head_detail", "type": "persp", "pitch_deg": -12, "yaw_deg": 158, "arm_cm": 340, "fov_deg": 48, "target": [110, 0, 90]},
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
    dump("lod1", [{"obj": f"../review/{ASSET}_intact_LOD1.obj"}], ortho[:2] + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [ortho[0], views[0]])
    # A dedicated silhouette scene: NO ground and NO reference figure, so a mask sees the animal and
    # nothing else. Measuring the tactical pass with the ground in frame compared the ground.
    sil = dict(base)
    sil.pop("ground", None)
    sil.pop("reference_figure", None)
    sil["background"] = [1.0, 1.0, 1.0]
    sil["meshes"] = [{"obj": "../review/SK_EBS_KHA_UNT_003_intact_LOD0.obj"}]
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
                      "width_cm": mm["body_width_cm"], "length_over_height": mm["length_over_height"],
                      "leg_share": mm["leg_share_of_height"], "width_over_length": mm["width_over_length"],
                      "head_tucked": mm["head_is_tucked_behind_the_shell"],
                      "amber_area": data["budgets"]["amber_area_fraction_lod0"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
