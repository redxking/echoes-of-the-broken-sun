#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-UNT-002 — the Kharuun Assemblies Riftstalker.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-FAC-025.KA.RIFTSTALKER.ASSET`: a lean
QUADRUPED warform in a low forward posture, four long three-segment legs, a layered faceted carapace
with amber seams, a forward-tapering prow, and a shoulder-mounted shard-caster that fires past it.

The candidate shows four legs and no neck. My card's first draft described a biped; the card was
corrected, not the model.

Usage:
  python3 build_riftstalker.py --evidence-dir "<root>/EBS-KHA-UNT-002" [--skinned]
  python3 build_riftstalker.py --evidence-dir "<root>/EBS-KHA-UNT-002" --check

Units: centimetres. +X forward, +Y right (anatomical right), +Z up. Pivot at the ground-contact centre
between the feet. Nanite off. Nothing below z = 0 in any pose.
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
PACKAGE_ID = "EBS-PKG-KA-RIFTSTALKER"
PRODUCTION_ID = "EBS-KHA-UNT-002"
ASSET = "SK_EBS_KHA_UNT_002"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_002/"
REVISION = "ebs-kha-unt-002-concept-v1"
CARD = "REL-ART-005.KA.RIFTSTALKER"          # AUTHORITATIVE, in Docs/Requirements.md
PROVISIONAL_CARD = "REL-FAC-025.KA.RIFTSTALKER.ASSET"   # mine; SUPERSEDED for this asset

STRATA = "MI_EBS_KHA_Strata"      # faceted charcoal carapace plate, legs, prow
AMBER = "MI_EBS_KHA_Amber"        # carapace seams and the caster slot — the only emissive

TICKS_PER_SECOND = 20.0

H = 200.0                              # standing height at the carapace top
BODY_LEN = 292.0                       # 1.46 of the standing height, as traced
PROW_TIP_X = 146.0                     # the prow reaches half the body length ahead of centre
BODY_BACK_X = -146.0
CARAPACE_SHELLS = 5                    # overlapping plate shells down the back
BODY_W = 78.0                          # the body is slight: the legs carry the silhouette
SHOULDER_X, HIP_X = 34.0, -70.0
LEG_Y = 46.0
KNEE_OUT, KNEE_Z = 96.0, 152.0         # knees high and outboard
FOOT_Y = 74.0
FRONT_FOOT_X, REAR_FOOT_X = 96.0, -140.0
CASTER_X, CASTER_Z = 46.0, 176.0       # the shoulder slot the shard leaves from
STATES = ("baseline", "carapace_molt", "striker_molt")

LEGS = (("fl", SHOULDER_X, -LEG_Y, FRONT_FOOT_X, -FOOT_Y),
        ("fr", SHOULDER_X, LEG_Y, FRONT_FOOT_X, FOOT_Y),
        ("rl", HIP_X, -LEG_Y, REAR_FOOT_X, -FOOT_Y),
        ("rr", HIP_X, LEG_Y, REAR_FOOT_X, FOOT_Y))

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "ground-contact centre between the feet; the runtime owns translation"),
    ("body", "root", (0.0, 0.0, H - 44.0), "the carapace body: the low forward posture lives in its rest pose"),
    ("prow_base", "body", (58.0, 0.0, H - 52.0), "where the carapace begins tapering forward"),
    ("prow_tip", "prow_base", (PROW_TIP_X - 34.0, 0.0, H - 78.0), "the prow's forward point"),
    ("caster_yaw", "body", (CASTER_X, 0.0, CASTER_Z), "shard-caster mount 1: aims independently of the gait"),
    ("caster_pitch", "caster_yaw", (CASTER_X + 20.0, 0.0, CASTER_Z), "shard-caster mount 2"),
]
for _tag, _ax, _ay, _fx, _fy in LEGS:
    _kx = (_ax + _fx) / 2.0
    _ky = _ay + (KNEE_OUT - abs(_ay)) * (1.0 if _ay > 0 else -1.0)
    # Each bone's head is the PROXIMAL end of the segment it drives. A first pass put the heads at the
    # distal ends, so rotating the thigh pivoted it about the knee and tore the leg off the body in the
    # death pose. Heads: hip at the body, upper at the hip, lower at the knee, foot at the ankle.
    BONES.append((f"{_tag}_hip", "body", (_ax, _ay, H - 56.0), f"{_tag} leg: attachment at the body"))
    BONES.append((f"{_tag}_upper", f"{_tag}_hip", (_ax, _ay, H - 56.0),
                  f"{_tag} leg: upper segment, pivoting at the hip toward the high outboard knee"))
    BONES.append((f"{_tag}_lower", f"{_tag}_upper", (_kx, _ky, KNEE_Z),
                  f"{_tag} leg: lower segment, pivoting at the knee"))
    BONES.append((f"{_tag}_foot", f"{_tag}_lower", (_fx, _fy, 26.0),
                  f"{_tag} leg: the pointed foot, pivoting at the ankle"))

LEG_TAGS = tuple(t[0] for t in LEGS)


def shell_profile(index: int):
    """(x centre, z centre, length, width, height) for one overlapping carapace shell.

    The shells step down and forward, which is where the low forward posture comes from: it is in the
    rest geometry, not only in the animation.
    """
    t = index / max(1, CARAPACE_SHELLS - 1)
    x = BODY_BACK_X + 52.0 + (170.0 * t)
    z = (H - 26.0) - 30.0 * t * t
    length = 92.0 - 16.0 * t
    width = BODY_W * (1.0 - 0.34 * t)
    height = 54.0 - 14.0 * t
    return x, z, length, width, height


def build_body(lod: int, state: str = "baseline") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    amber = m.slot(AMBER)
    fine = lod == 0
    sides = 6 if fine else 4
    heavy = state == "carapace_molt"

    # 1. the carapace: overlapping plate shells stepping down and forward
    for i in range(CARAPACE_SHELLS):
        x, z, length, width, height = shell_profile(i)
        if heavy:
            width *= 1.16
            height *= 1.18
        m.box((x, 0.0, z), (length, width, height), strata, f"shell_{i + 1:02d}")
        if fine:
            for sign, tag in ((-1.0, "l"), (1.0, "r")):
                # thin lines between the plates. A first pass used 8 cm slabs, which read as bright
                # bars stuck to the flanks instead of seams.
                m.box((x, sign * (width / 2.0 + 1.5), z - height / 2.0 + 4.0), (length - 26.0, 3.0, 4.0),
                      amber, f"seam_{i + 1:02d}_{tag}")
        if heavy and fine:
            m.box((x, 0.0, z + height / 2.0 + 7.0), (length - 20.0, width * 0.7, 14.0), strata,
                  f"molt_plate_{i + 1:02d}")

    # 2. the underbody the legs hang from
    m.box((-16.0, 0.0, H - 62.0), (196.0, BODY_W - 10.0, 40.0), strata, "underbody")

    # 3. the forward-tapering prow: no neck, no head on a stalk
    # a long, near-horizontal forward taper carrying the body's own downward slope. A first pass made
    # it short and steeply drooping, which read as a chin rather than as the front of the animal.
    x0, z0 = 44.0, H - 46.0
    tip = (PROW_TIP_X, 0.0, H - 72.0)
    ring = [(x0, p[1], z0 + p[2]) for p in
            [(0.0, -36.0, 0.0), (0.0, -22.0, 28.0), (0.0, 22.0, 28.0), (0.0, 36.0, 0.0),
             (0.0, 22.0, -26.0), (0.0, -22.0, -26.0)]]
    for k in range(len(ring)):
        j = (k + 1) % len(ring)
        m.add_polygon([ring[k], ring[j], tip], strata, "prow",
                      normal=(0.4, (ring[k][1] + ring[j][1]) / 2.0, (ring[k][2] + ring[j][2]) / 2.0 - z0))
    m.add_polygon(list(reversed(ring)), strata, "prow", normal=(-1.0, 0.0, 0.0))
    if fine:
        m.box((x0 + 34.0, 0.0, z0 + 12.0), (52.0, 5.0, 4.0), amber, "prow_seam")

    # 4. the shoulder-mounted shard-caster: an amber slot in the carapace, aimed past the prow
    striker = state == "striker_molt"
    m.box((CASTER_X, 0.0, CASTER_Z), (74.0 + (16.0 if striker else 0.0), 42.0, 30.0), strata, "caster_housing")
    m.box((CASTER_X + 34.0 + (8.0 if striker else 0.0), 0.0, CASTER_Z), (8.0, 16.0, 9.0), amber, "caster_slot")
    if striker and fine:
        for sign, tag in ((-1.0, "l"), (1.0, "r")):
            m.box((CASTER_X + 26.0, sign * 26.0, CASTER_Z + 4.0), (52.0, 10.0, 18.0), strata,
                  f"molt_striker_vane_{tag}")

    # 5. four long three-segment legs: knees high and outboard, feet small and pointed
    for tag, ax, ay, fx, fy in LEGS:
        ky = ay + (KNEE_OUT - abs(ay)) * (1.0 if ay > 0 else -1.0)
        kx = (ax + fx) / 2.0
        m.box((ax, ay, H - 56.0), (34.0, 26.0, 34.0), strata, f"{tag}_hip")
        m.tube((ax, ay, H - 56.0), (kx, ky, KNEE_Z), 15.0, sides, strata, f"{tag}_upper")
        m.box((kx, ky, KNEE_Z), (30.0, 26.0, 28.0), strata, f"{tag}_knee")
        m.tube((kx, ky, KNEE_Z), (fx, fy, 26.0), 11.0, sides, strata, f"{tag}_lower")
        # the foot tube's end ring is perpendicular to its slanted axis, so ending it at z = 0 put
        # the ring 5.2 cm underground. It ends at 8.2 instead, which lands the toe on the ground with margin for the stride and for the
        # interpolated frames of the collapse clips.
        m.tube((fx, fy, 26.0), (fx + 22.0, fy, 8.2), 8.0, 4, strata, f"{tag}_foot")

    m.collision.append(kit.CollisionBox("body", (-16.0, 0.0, H - 46.0), (BODY_LEN * 0.72, BODY_W + 16.0, 78.0)))
    return m


# --- vertex ID channels (card .MESH_PROP: "3 vertex ID coloring channels for public molting phase
# transitions"; canon REL-FAC-009: an 80-tick PUBLIC molt window between baseline, Carapace and
# Striker) -----------------------------------------------------------------------------------------
#
#   R  molt sweep order. How early a region takes on the molt-window treatment - the 512^2
#      translucent core blending skin mask the card's .TEX_MAPS clause calls for. 1.0 sweeps first,
#      0.0 never changes. This is the channel that carries the PUBLIC 80-tick transition; the other
#      two only say which end state a piece belongs to.
#   G  Carapace adaptation membership. 1.0 on geometry that exists only after a Carapace molt.
#   B  Striker adaptation membership.  1.0 on geometry that exists only after a Striker molt.
#   A  RESERVED, always 1.0.
#
# TEAM OWNERSHIP IS IN NONE OF THEM (owner ruling 2026-09-07). Team colour stays in its own material
# parameter and its own mask texture, so a reader never has to disambiguate who owns a unit from what
# it has adapted into. A test asserts no channel is used for team.
MOLT_SWEEP = {"seam": 1.0, "growth": 1.0, "prow": 0.85, "shell": (0.60, 0.30), "underbody": 0.20,
              "caster": 0.15, "leg": 0.0}


def vertex_colors_for(components) -> dict:
    """The RGBA every component carries. Derived from names so a new part cannot silently
    default to 'never changes' without appearing in this mapping."""
    colors = {}
    for c in components:
        g = 1.0 if c.startswith("molt_plate_") else 0.0
        b = 1.0 if c.startswith("molt_striker_vane_") else 0.0
        if c.startswith("molt_"):
            r = MOLT_SWEEP["growth"]
        elif "seam" in c:
            r = MOLT_SWEEP["seam"]
        elif c == "prow":
            r = MOLT_SWEEP["prow"]
        elif c.startswith("shell_"):
            # the sweep runs nose to tail, so the transition reads as directional rather than uniform
            head, tail = MOLT_SWEEP["shell"]
            t = (int(c.split("_")[1]) - 1) / max(1, CARAPACE_SHELLS - 1)
            r = head + (tail - head) * t
        elif c == "underbody":
            r = MOLT_SWEEP["underbody"]
        elif c.startswith("caster_"):
            r = MOLT_SWEEP["caster"]
        elif c.split("_")[0] in ("fl", "fr", "rl", "rr"):
            r = MOLT_SWEEP["leg"]
        else:
            raise ValueError(f"component {c!r} has no vertex-ID channel mapping; add one deliberately")
        colors[c] = (round(r, 4), g, b, 1.0)
    return colors


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith(("shell_", "seam_", "underbody", "molt_plate_")):
            by_component[c] = "body"
        elif c.startswith("prow"):
            by_component[c] = "prow_base"
        elif c.startswith(("caster_", "molt_striker_")):
            by_component[c] = "caster_pitch"
        else:
            for tag in LEG_TAGS:
                if c.startswith(tag + "_"):
                    part = c[len(tag) + 1:]
                    by_component[c] = {"hip": f"{tag}_hip", "upper": f"{tag}_upper",
                                       "knee": f"{tag}_upper", "lower": f"{tag}_lower",
                                       "foot": f"{tag}_foot"}[part]
                    break
    return skel.bind_polygons(m, "root", by_component)


# The authoritative card fixes three socket names. The build used its own until 2026-09-07, when the
# card was found; the card's names are now primary and the previous names are kept as aliases, which is
# the pattern the owner set for the Lancer's renamed socket on 2026-09-07.
SOCKETS = {
    "VFX_Muzzle_Shard_01": ("caster_pitch", (CASTER_X + 48.0, 0.0, CASTER_Z), 0.0,
                            "card .ANIM_RIG: where the shard leaves, forward and clear of the prow"),
    "VFX_Molt_Origin_Base": ("body", (shell_profile(2)[0], 0.0, shell_profile(2)[1] + 30.0), 0.0,
                             "card .ANIM_RIG: the molt effect origin"),
    "Target_Hitbox_Center": ("body", (-16.0, 0.0, H - 46.0), 0.0,
                             "card .ANIM_RIG: targeting and selection anchor"),
    "Caster_Mount": ("caster_yaw", (CASTER_X, 0.0, CASTER_Z), 0.0,
                     "not named by the card: the caster's own pivot, needed by the two-bone mount"),
    "Molt_Striker_Anchor": ("caster_pitch", (CASTER_X + 26.0, 0.0, CASTER_Z + 4.0), 0.0,
                            "not named by the card: where a striker molt's vanes attach"),
}

SOCKET_ALIASES = {
    "Shard_Caster_Muzzle": "VFX_Muzzle_Shard_01",
    "Molt_Carapace_Anchor": "VFX_Molt_Origin_Base",
    "Target_Anchor_Center": "Target_Hitbox_Center",
}


def assemble(lod: int, state: str = "baseline"):
    m = build_body(lod, state)
    s = build_skeleton()
    counts = bind(m)
    m.vertex_colors = vertex_colors_for(m.components())
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    return m, s, counts, {n: v[0] for n, v in SOCKETS.items()}




_SAFE_DROP_CACHE: dict = {}


def _safe_drop(fold: float, shin: float, margin: float = 0.54) -> float:
    """Greatest body descent that keeps the WHOLE posed mesh on or above the ground, times a margin.

    The margin covers linear interpolation BETWEEN keys, which does not follow the safe curve: at
    0.85 the interpolated frames dipped about 2.6 cm and at 0.70 about 2.3 cm.

    Measured on the real posed mesh, not on a model of it. Two earlier attempts solved this
    analytically — once with the pitch sign inverted, once with a bisection that reported its own
    search bound because the ankle's height is not monotonic in the fold — and both produced folds
    that drove the feet underground. The rig itself is the only authority worth trusting here, so this
    poses it and measures.
    """
    key = (round(fold, 3), round(shin, 3))
    if key in _SAFE_DROP_CACHE:
        return _SAFE_DROP_CACHE[key] * margin
    mesh, skeleton, _counts, _socks = assemble(0, "baseline")

    def lowest(drop):
        pose = {"body": (0.0, 0.0, 0.0, 0.0, 0.0, drop)}
        for tag in LEG_TAGS:
            pose[f"{tag}_upper"] = (fold, 0.0, 0.0, 0.0, 0.0, 0.0)
            pose[f"{tag}_lower"] = (shin, 0.0, 0.0, 0.0, 0.0, 0.0)
        return skel.pose_mesh(mesh, skeleton, pose).bounds()[0][2]

    lo, hi = 0.0, 200.0
    for _ in range(30):
        mid = (lo + hi) / 2.0
        if lowest(-mid) >= 0.0:
            lo = mid
        else:
            hi = mid
    _SAFE_DROP_CACHE[key] = lo
    return lo * margin


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def new(name, ticks, loop, purpose):
        c = skel.AnimationClip(name, ticks / TICKS_PER_SECOND, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    def gait(clip, phase_of, amount, lift):
        """Four legs stepping in a diagonal pattern: fl with rr, fr with rl."""
        for tag in LEG_TAGS:
            phase = phase_of[tag]
            for step in range(5):
                # the key TIME walks the cycle; the swing is read at that time PLUS this limb's phase.
                # Deriving both from (phase + step) gave every limb an identical curve, so the four
                # legs moved in unison and the diagonal gait was a hop. Found on 2026-09-07 while
                # testing the Cairnback's gait separation.
                t = step / 4.0 * clip.duration_s
                swing = math.sin(2.0 * math.pi * (step / 4.0 + phase))
                # (pitch, yaw, roll): a leg's stride is PITCH. Writing it into the yaw slot swung the
                # legs sideways instead of stepping, which is how this was wrong in the first pass.
                clip.key(f"{tag}_upper", t, (-amount * swing, 0.0, 0.0))
                clip.key(f"{tag}_lower", t, (amount * 0.8 * max(0.0, swing), 0.0, 0.0))
                # the swing foot lifts; the stance foot is held up by the same offset so the stride's
                # downswing cannot push it through the ground between keys
                clip.key(f"{tag}_foot", t, (0.0, 0.0, 0.0),
                         translation_cm=(0.0, 0.0, lift * (0.35 + 0.65 * max(0.0, swing))))

    diagonal = {"fl": 0.0, "rr": 0.0, "fr": 0.5, "rl": 0.5}

    idle = new("idle", 80, True, "watching: the body breathes and the legs hold their splay")
    for t, dz in ((0.0, 0.0), (2.0, 2.5), (4.0, 0.0)):
        idle.key("body", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, dz))
    for tag in LEG_TAGS:
        for t in (0.0, idle.duration_s):
            idle.key(f"{tag}_upper", t, (0.0, 0.0, 0.0))

    move = new("move", 24, True, "410 cm/s: a diagonal gait under a body that stays level")
    gait(move, diagonal, 9.0, 18.0)
    for t in (0.0, move.duration_s):
        move.key("body", t, (0.0, 0.0, 0.0))

    fire = new("fire_on_the_move", 24, True,
               "canon: fires WHILE it moves. The gait is unchanged and the caster works above it, so the "
               "two never fight each other")
    gait(fire, diagonal, 9.0, 18.0)
    for t, pitch in ((0.0, 0.0), (0.25, -5.0), (0.5, 0.0), (1.2, 0.0)):
        fire.key("caster_pitch", t, (pitch, 0.0, 0.0))
    for t, yaw in ((0.0, 0.0), (0.6, 3.0), (1.2, 0.0)):
        fire.key("caster_yaw", t, (0.0, yaw, 0.0))

    side = new("sidestep", 16, False, "canon's short sidestep after each shot: the legs carry it, the body stays level")
    for tag in LEG_TAGS:
        sign = 1.0 if tag.endswith("r") else -1.0
        for t, roll in ((0.0, 0.0), (0.35, 2.0 * sign), (0.8, 0.0)):
            side.key(f"{tag}_upper", t, (0.0, 0.0, roll))
    for t, dy in ((0.0, 0.0), (0.35, 9.0), (0.8, 0.0)):
        side.key("body", t, (0.0, 0.0, 0.0), translation_cm=(0.0, dy, 0.0))

    molt = new("molt", 80, False, "a molt at a Growth Basin: the body lowers, the carapace works, and it rises changed")
    # Each key's descent is measured against its own leg fold on the real rig (see _safe_drop), so no
    # frame can pass through the ground and no depth is invented that the legs could not support.
    # Keys are placed every 0.35 s through the descent and recovery. Sparser keys let the linear
    # interpolation cut inside the safe curve and the feet dipped a couple of centimetres.
    molt_shape = []
    for i in range(5):
        u = i / 4.0
        molt_shape.append((0.35 * i, 0.6 * u, 12.0 * u, 60.0 * u))
    molt_shape.append((2.6, 0.6, 12.0, 60.0))
    for i in range(1, 5):
        u = 1.0 - i / 4.0
        molt_shape.append((2.6 + 0.35 * i, 0.6 * u, 12.0 * u, 60.0 * u))
    for t, share, fold, shin in molt_shape:
        molt.key("body", t, (0.0, 0.0, 0.0),
                 translation_cm=(0.0, 0.0, -share * _safe_drop(fold, shin)))
        for tag in LEG_TAGS:
            molt.key(f"{tag}_upper", t, (fold, 0.0, 0.0))
            molt.key(f"{tag}_lower", t, (shin, 0.0, 0.0))

    death = new("death", 40, False, "the light frame folds: the legs give and the body settles onto the ground")
    # The same measured solve, run all the way down to the deepest settle the legs can hold.
    # A thigh 45 / shin 25 fold reads better — the legs collapse under the body instead of one
    # swinging up — but its interpolated frames sink 3.6 cm through the ground. Ground contact wins
    # over silhouette at blockout, so the safe 20 / 90 fold stays and the raised-leg read is recorded
    # as an open item in the README rather than traded for a pose that clips the floor.
    death_shape = [(2.0 * i / 8.0, i / 8.0, 20.0 * i / 8.0, 90.0 * i / 8.0) for i in range(9)]
    for t, share, fold, shin in death_shape:
        death.key("body", t, (0.0, 0.0, 0.0),
                  translation_cm=(0.0, 0.0, -share * _safe_drop(fold, shin)))
        for tag in LEG_TAGS:
            death.key(f"{tag}_upper", t, (fold, 0.0, 0.0))
            death.key(f"{tag}_lower", t, (shin, 0.0, 0.0))

    for c in clips:
        if not skel.is_frame_aligned(c.duration_s):
            skel.retime_clip(c, skel.frame_aligned_duration(c.duration_s))
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


def posed(lod: int, pose: dict, state: str = "baseline") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("move", 0.25, "baseline"), ("move", 0.5, "baseline"),
                ("fire_on_the_move", 0.25, "baseline"), ("sidestep", 0.44, "baseline"),
                ("molt", 0.5, "baseline"), ("death", 1.0, "baseline")]


def slot_area_fraction(m: kit.Mesh, slot_name: str) -> float:
    """Share of the mesh's SURFACE AREA carried by one slot; REL-ART-029 caps amber by area."""
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


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "quadruped": {"contract": 4,
                      "legs": len({c.split("_")[0] for c in comps
                                   if c.split("_")[0] in LEG_TAGS}),
                      "segments_per_leg": len({p for p in ("upper", "lower", "foot")}),
                      "note": "four legs and no neck: the candidate's SKIRMISHER and MOVING FIRE views both show this"},
        "layered_carapace": {"contract": CARAPACE_SHELLS,
                             "built": sum(1 for c in comps if c.startswith("shell_")),
                             "seams": sum(1 for c in comps if c.startswith("seam_"))},
        "prow": {"contract": "a forward taper, not a head on a neck", "built": "prow" in comps,
                 "neck_bones": [b.name for b in s.bones if "neck" in b.name or "head" in b.name]},
        "shard_caster": {"contract": "a shoulder slot aimed past the prow",
                         "housing": "caster_housing" in comps, "slot": "caster_slot" in comps},
        "molt_variants": {"contract": "carapace and striker changes, separately controllable",
                          "carapace_plates": sum(1 for c in comps if c.startswith("molt_plate_")),
                          "striker_vanes": sum(1 for c in comps if c.startswith("molt_striker_"))},
        "bones": {"contract": "22: root, body, two-bone prow, two-bone caster, four three-segment legs",
                  "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets),
                    "aliases": dict(SOCKET_ALIASES),
                    "card_required": ["VFX_Muzzle_Shard_01", "VFX_Molt_Origin_Base", "Target_Hitbox_Center"]},
        "tracks": {"contract": ["idle", "move", "fire_on_the_move", "sidestep", "molt", "death"],
                   "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "standing_height_cm": round(z1, 2),
        "body_length_cm": round(x1 - x0, 2),
        "body_length_over_height": round((x1 - x0) / z1, 4),
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "stance_width_cm": round(y1 - y0, 2),
        "carapace_slopes_forward": shell_profile(0)[1] > shell_profile(CARAPACE_SHELLS - 1)[1],
        "prow_ahead_of_front_feet": PROW_TIP_X > FRONT_FOOT_X,
        "feet_on_the_ground": abs(z0) < 1e-6,
    }


def export(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, _c, socks = assemble(lod, "baseline")
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
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (baseline)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    if skinned:
        # One skinned GLB per molt state, so the vertex ID channels can be verified through import
        # and material for EVERY required state and not only the baseline rest pose.
        for state in STATES:
            m, s, _c, socks = assemble(0, state)
            path = os.path.join(review, f"{ASSET}_{state}_vertexid.glb")
            digest = skel.write_skinned_glb(m, s, path, animations=[], extras={
                "production_id": PRODUCTION_ID, "revision": REVISION, "molt_state": state,
                "purpose": "vertex ID channel verification", "author": AUTHOR},
                include_collision=False, sockets_on_bones=socks)
            review_rows.append({"name": f"{state}_vertexid_glb", "path": os.path.relpath(path, evidence_dir),
                                "sha256": digest, "molt_state": state,
                                "authored_vertex_colors": {c: list(v) for c, v in sorted(m.vertex_colors.items())}})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count()})
    m1 = assemble(1, "baseline")[0]
    path = os.path.join(review, f"{ASSET}_baseline_LOD1.obj")
    review_rows.append({"name": "baseline_lod1", "path": os.path.relpath(path, evidence_dir),
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
    m0, s, counts, _socks = assemble(0, "baseline")
    m1 = assemble(1, "baseline")[0]
    carapace = assemble(0, "carapace_molt")[0]
    striker = assemble(0, "striker_molt")[0]
    clips = exported["clips"]
    amber = slot_area_fraction(m0, AMBER)
    worst = max(m0.triangle_count(), carapace.triangle_count(), striker.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right (anatomical right), +Z up",
                  "pivot": "ground-contact centre between the feet", "nanite": False},
        "scale_basis": {"canon": "SPEC-UNIT-006: a lean, long-limbed warform with a low forward posture, a faceted carapace in charcoal with amber seams, and a shoulder-mounted shard-caster that fires while it moves",
                        "standing_height_cm": H,
                        "basis": "between the Tender's 190 cm crown and the Lancer's 213 cm, so the roster reads consistently",
                        "gameplay": "Content/Data/Source/units.json ka_riftstalker"},
        "material_slots": [STRATA, AMBER],
        "material_slot_policy": ("Two slots: faceted charcoal carapace plate for shells, prow, legs and caster housing, and amber "
                                 "for the carapace seams, the prow seam and the caster slot, which are the only emissive."),
        "anatomy_correction": {
            "what": "An earlier draft of the provisional card described a biped with a long neck and 24 bones.",
            "why_wrong": "Written before the candidate was traced. Its SKIRMISHER and MOVING FIRE views both show FOUR legs and no neck.",
            "status": ("SUPERSEDED AND NOT GUIDANCE. Retained as history so the change is traceable; the quadruped plan is the "
                       "sole anatomy for this asset (owner ruling 2026-09-07)."),
            "source_references": [
                "BuildArtifacts/Evidence/concept-discovery-20260906/riftstalker-review/riftstalker-candidate.png (SKIRMISHER and MOVING FIRE views)",
                "BuildArtifacts/Evidence/concept-discovery-20260906/riftstalker-review/generation-record.json",
                "BuildArtifacts/Evidence/asset-production-20260906T221157Z/concept-crops/all/EBS-PKG-KA-RIFTSTALKER/EBS-CON-KHA-UNT-002.png"],
            "resolution": ("The CARD was corrected to a 22-bone quadruped plan, not the model bent to fit it. A test asserts the "
                           "skeleton contains no bone named neck or head and that four legs exist.")},
        "open_defects": [
            {"item": "death pose leaves one leg raised",
             "status": "OPEN BLOCKOUT DEFECT, not final animation acceptance (owner ruling 2026-09-07)",
             "detail": ("Ground contact was prioritised over the preferred thigh 45 / shin 25 fold, whose interpolated frames "
                        "sink 3.6 cm through the floor. Both are to be resolved later through a supported collapse pose rather "
                        "than by choosing between them.")}],
        "verification_pending": [
            {"item": "simultaneous locomotion and firing",
             "detail": ("The identical leg tracks between move and fire_on_the_move are a REGRESSION CHECK ONLY and do not "
                        "prove runtime behaviour. Verification through turns, stops, targeting changes and animation "
                        "transitions is required and has not been done (owner ruling 2026-09-07).")},
            {"item": "silhouette, joint and carapace detail",
             "detail": ("Sitting far below the triangle ceiling is HEADROOM, not sufficiency. Detail must be judged against the "
                        "concept at gameplay distance (owner ruling 2026-09-07).")},
            {"item": "monochrome separation from the Cairnback",
             "detail": ("The two share a quadruped anatomy and must be distinguishable in monochrome at tactical distance. A "
                        "monochrome tactical render is produced here as the baseline for that comparison; the comparison "
                        "itself waits on the Cairnback package.")}],
        "authoritative_card": {
            "card": CARD,
            "found": ("2026-09-07, during a full audit of every asset-card heading in Docs/Requirements.md. I had reported "
                      "that no Kharuun asset card existed; that was wrong for this asset, and the owner's ruling to author a "
                      "provisional Kharuun set was given on that false premise."),
            "bounds": {"lod0_triangles": 7500, "lod1_triangles": 3200, "rig": "14-bone kinematic layout",
                       "sockets": ["VFX_Muzzle_Shard_01", "VFX_Molt_Origin_Base", "Target_Hitbox_Center"],
                       "vertex_id_channels": 3, "emissive": "Broken-Sun Amber <= 15% surface area"},
            "compliance": {
                "triangles": "WITHIN: 602 LOD0 and 346 LOD1 against 7,500 / 3,200",
                "emissive": "WITHIN: 3.1% against 15%",
                "faceted_no_smoothed_topology": "WITHIN",
                "sockets": "NOW COMPLIANT: the card's three names are emitted; the build's previous names are aliases",
                "rig": ("NON-COMPLIANT with the clause as written: the card says 14 bones, this quadruped has 22. The owner "
                        "selected the 22-bone rig as the production direction on 2026-09-07 and directed that no limb segment "
                        "or independent caster aim be removed to reach 14. A targeted amendment is proposed in "
                        "card-amendment-REL-ART-005.KA.RIFTSTALKER.md. Docs/Requirements.md is NOT amended, so this asset "
                        "remains non-compliant with the clause until it is."),
                "vertex_id_channels": ("NOT IMPLEMENTED and NOT WAIVED. Required work; neither the rig decision nor a passing "
                                       "import affects it. The mapping is defined below; the export path cannot yet carry it."),
                "molt_texture": "PENDING: a 512^2 translucent core blend mask is required at the texture stage."}},
        "vertex_id_channels": {
            "requirement": "card .MESH_PROP: 3 vertex ID coloring channels for public molting phase transitions",
            "canon": ("REL-FAC-009: an eligible unit spends 25 Dawn for an 80-tick PUBLIC molt window, choosing Carapace "
                      "(135% HP, 80% speed) or Striker (125% damage, 85% cooldown), replacing any prior adaptation. The "
                      "window itself is public and is what the channels have to carry, not only the end states."),
            "status": "VERIFIED THROUGH EXPORT, IMPORT AND IN-ENGINE RENDER; simulation integration outstanding",
            "definition_changed_2026_09_07": (
                "The earlier mapping used R as a redundant 'baseline chassis' membership flag, which is already implied by "
                "G == 0 and B == 0, and carried nothing for the 80-tick transition. R is now the molt SWEEP ORDER, so the "
                "public window has a channel of its own."),
            "mapping": {
                "channel_r": {"meaning": ("molt sweep order: how early a region takes on the molt-window treatment, the "
                                          "512^2 translucent core blending skin mask required by .TEX_MAPS. 1.0 sweeps "
                                          "first, 0.0 never changes. This is the channel that carries the PUBLIC 80-tick "
                                          "transition."),
                              "regions": {"seams and new growth": 1.0, "prow": 0.85, "shells": "0.60 nose to 0.30 tail",
                                          "underbody": 0.20, "caster housing and slot": 0.15, "legs": 0.0},
                              "why_graded": "the sweep runs nose to tail, so the transition reads as directional rather than uniform"},
                "channel_g": {"meaning": "Carapace adaptation membership", "value": "1.0 on molt_plate_*, 0.0 elsewhere"},
                "channel_b": {"meaning": "Striker adaptation membership", "value": "1.0 on molt_striker_vane_*, 0.0 elsewhere"},
                "channel_a": {"meaning": "RESERVED", "value": "always 1.0"}},
            "team_ownership": ("IN NO CHANNEL. Owner ruling 2026-09-07: team ownership must stay distinguishable from "
                               "adaptation state. Team colour lives in its own material parameter and mask texture, so a "
                               "reader never disambiguates ownership from adaptation. Alpha is reserved rather than left "
                               "spare so it cannot later be quietly taken for team."),
            "phase_read": {
                "baseline": "G = 0 and B = 0 everywhere. Seams still carry R, so a baseline unit shows where growth would emerge.",
                "carapace_molt": "molt_plate_* carry G = 1. During the 80-tick window the material sweeps a threshold down R.",
                "striker_molt": "molt_striker_vane_* carry B = 1, swept the same way.",
                "transition": "R alone drives it; G and B only say which end state a piece belongs to."},
            "export": ("ebs_meshkit.Mesh.vertex_colors is optional and per component; when it is empty NO COLOR_0 attribute "
                       "is written, so all 21 packages export byte for byte as before. Verified across every package with "
                       "--check on 2026-09-07."),
            "sweep_convention": {
                "rule": "HIGHER R MEANS EARLIER treatment.",
                "threshold": "threshold = 1.02 - progress * 1.04; swept where R >= threshold",
                "why_outside_the_unit_range": ("progress 0 must sweep NOTHING (the maximum authored R is 1.0) and "
                                               "progress 1 must sweep EVERYTHING including the R = 0 legs. A threshold "
                                               "confined to [0, 1] cannot do both. The 0.02 margin exceeds the 1/255 "
                                               "quantisation step by a factor of five."),
                "defect_found_and_fixed": ("The first material used a strict '>' with the A == B branch wired to zero, so "
                                           "the R = 0 legs NEVER completed at progress 1. It compiled and was assigned, "
                                           "which is why 'compiles and is assigned' was not evidence of correct response. "
                                           "Now '>=' via both the A > B and A == B branches."),
                "endpoints_verified": {"progress 0": "0 of 10 quantised levels swept",
                                       "progress 1": "10 of 10 swept, the R = 0 legs included"},
                "intermediate_boundaries": ("Checked AFTER 8-bit quantisation. The ten quantised levels cross at ten "
                                            "distinct progress values; the smallest margin between neighbours is 0.049 "
                                            "progress = 3.92 ticks of the 80-tick window, against a quantisation step "
                                            "worth about 0.3 ticks. Recorded in sweep-boundaries.json."),
                "timing": ("The material exposes MoltProgress, a scalar the simulation writes, and contains no Time or "
                           "Panner node. It consumes authoritative molt progress and keeps no timing of its own. Still to "
                           "be confirmed against a running simulation, including interruption and restoration.")},
            "verification": {
                "export": {"result": "PASS",
                           "detail": "COLOR_0 present on every primitive of the LOD0 and LOD1 skinned GLBs, plus one GLB per molt state."},
                "import": {"result": "PASS",
                           "method": ("Round-trip export was the method used successfully: the asset was exported back out "
                                      "with GLTFSkeletalMeshExporter and COLOR_0 read as VALUES. It is not the only "
                                      "possible way to inspect Unreal's vertex colours - a C++ or engine-side reader of "
                                      "the render data would serve, and was not attempted. What made a round-trip "
                                      "attractive here is that no Python accessor for the values exists in 5.8."),
                           "membership": ("baseline 10 authored / 10 imported distinct colours, carapace_molt 11 / 11, "
                                          "striker_molt 11 / 11; no missing and no unexpected values. G is non-zero only "
                                          "in carapace_molt, B only in striker_molt, alpha 1.0 everywhere."),
                           "spatial_placement": ("Set equality does not establish placement, so every imported vertex was "
                                                 "matched to its authored position within 0.06 cm and its colour compared "
                                                 "to the colours authored there: LOD0 1056 of 1056 vertices placed and "
                                                 "coloured correctly, LOD1 696 of 696. Zero mismatches."),
                           "evidence": ["vertex-id-route/channel-verification.json",
                                        "vertex-id-route/lods/lod-spatial-verification.json"]},
                "material": {"result": "RENDERED IN ENGINE; integration with the simulation still outstanding",
                             "graph": ("M_EBS_KHA_UNT_002_MoltPhase reads VertexColor R, G and B, exposes the scalar "
                                       "parameter MoltProgress, and contains no Time or Panner node."),
                             "rendered": ("Unreal rendered the response offscreen: ticks 0/20/40/60/80 for all three "
                                          "variants at two framings (tactical, 1400 cm arm at -55 deg; close, 520 cm at "
                                          "-18 deg), 30 frames, plus a six-step interruption and restoration sequence."),
                             "endpoints_seen": ("tick 0 sweeps nothing - the carapace plates read as unswept new growth "
                                                "and every other surface is dark; tick 80 sweeps everything, the R = 0 "
                                                "legs included."),
                             "interruption_and_restoration": ("open -> mid -> interrupted -> restored open -> restored mid "
                                                              "-> restored complete. The interrupted frame returns to "
                                                              "baseline with no partial sweep retained and no adaptation "
                                                              "geometry, and the restored sequence RESTARTS from zero "
                                                              "rather than resuming. Canon REL-FAC-009 cancels the "
                                                              "adaptation without refund, which is what that shows."),
                             "timing": ("Every frame is a pure function of MoltProgress supplied from outside the "
                                        "material by a MaterialInstanceConstant per value. The material holds no state "
                                        "and no clock, which is why interruption reads instantly and restoration "
                                        "restarts. It therefore CONSUMES authoritative progress and creates no gameplay "
                                        "timing of its own."),
                             "outstanding": ("This is a debug material whose base colour IS the channels, rendered "
                                             "through an editor SceneCapture. Not tested: the shipped art material, and "
                                             "integration with a RUNNING simulation driving MoltProgress from the real "
                                             "80-tick window. The behaviour above is the material's, not the game's."),
                             "evidence": ["vertex-id-route/inengine/capture-report.json",
                                          "vertex-id-route/inengine/sheet_carapace_molt_tactical.png",
                                          "vertex-id-route/inengine/sheet_striker_molt_tactical.png",
                                          "vertex-id-route/inengine/sheet_interruption.png"]},
                "quantisation": ("Interchange stores skeletal vertex colours as 8-bit unsigned normalised, so authored "
                                 "floats round-trip within 1/255 (0.85 returns as 0.851). Any channel needing more than "
                                 "256 levels would not survive; the sweep grade uses ten."),
                "lods_tested": {
                    "what_was_tested": ("The two SHIPPED LODs, each authored and imported as its own asset: "
                                        "SK_EBS_KHA_UNT_002 (1056 vertices) and SK_EBS_KHA_UNT_002_LOD1Source (696). "
                                        "Channel placement and membership verified spatially on each."),
                    "what_was_NOT_tested": ("ENGINE LOD REDUCTION. Both imported meshes report a single LOD, so no "
                                            "generated LOD chain exists and none was exercised. The earlier claim that "
                                            "'LOD generation altered nothing' was unsupported and is withdrawn: nothing "
                                            "generated LODs. If a reduction chain is added later, channel placement must "
                                            "be re-verified on every generated level."),
                    "splitting": ("The exporter writes unindexed triangles, so nothing is welded on the way out and each "
                                  "corner takes its component's colour. The import returned every authored position with "
                                  "its authored colour, so no split or weld moved a value.")}},
            "team_identification": {
                "requirement": "Owner ruling 2026-09-07: record the separate team-identification mechanism BEFORE material acceptance.",
                "mechanism": ("Team ownership is delivered by a dedicated vector parameter TeamColor, modulating a team "
                              "mask carried in the packed utility channel of the 2048^2 texture stack. It is a texture "
                              "and parameter path, entirely disjoint from COLOR_0."),
                "why_disjoint": ("Adaptation state changes during play and is opponent-facing; ownership is fixed for the "
                                 "unit's life. Sharing a channel would make a molting unit's owner ambiguous exactly when "
                                 "the 80-tick window makes it most worth reading."),
                "status": "DEFINED, NOT BUILT. It belongs to the texture pass and gates material acceptance, not this export work."},
            "verified": True,
            "still_outstanding": [
                "Integration with a RUNNING simulation driving MoltProgress from the real 80-tick window; the captures show the material's behaviour, not the game's.",
                "The shipped art material: what was rendered is a debug material whose base colour is the channels.",
                "Engine LOD reduction: untested, because no generated LOD chain exists yet.",
                "The TeamColor mechanism, which is defined but not built, and gates material acceptance."]},
        "provisional_contract": {
            "card": f"{PROVISIONAL_CARD} in ArtSource/kharuun-asset-cards.json — SUPERSEDED for this asset by {CARD}",
            "status": ("PROVISIONAL. Authored in this worktree under the owner ruling of 2026-09-07; NOT incorporated into "
                       "Docs/Requirements.md and not an existing authoritative per-asset requirement."),
            "bounds": {"lod0_triangles": 6000, "lod1_triangles": 2600,
                       "basis": "below the faction default because a light frame has to read as light"}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "carapace_molt_triangles": carapace.triangle_count(),
                    "striker_molt_triangles": striker.triangle_count(),
                    "worst_state_triangles": worst,
                    "lod0_cap": 7500, "lod1_cap": 3200, "cap_source": f"{CARD} (AUTHORITATIVE); my provisional {PROVISIONAL_CARD} was tighter at 6,000/2,600",
                    "cap_scope": "the complete assembly including the caster and every molt-variant part carried on the same asset",
                    "lod0_within_cap": worst <= 7500, "lod1_within_cap": m1.triangle_count() <= 3200,
                    "amber_area_fraction_lod0": round(amber, 5), "amber_cap": 0.15,
                    "amber_measure": "surface area, per REL-ART-029; the cap is a ceiling, not a target",
                    "amber_within_cap": amber <= 0.15},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("The corrected card's own rig: root, body, a two-bone prow, a two-bone caster that aims independently "
                           "of the gait, and four three-segment legs. NOT the Tender's humanoid rig and NOT the Cairnback's heavy "
                           "plan. No root motion; the runtime owns translation.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {name: purpose for name, purpose in (
            ("baseline", "the unmolted form"),
            ("carapace_molt", "thicker shells with added plates over them: visible on the unit, not only in the interface"),
            ("striker_molt", "a longer caster housing with added vanes"))},
        "ground_solve": {"method": "_leg_fold_for_drop: forward kinematics on the leg's rest geometry, bisected so the ankle lands at the toe height for each key's body drop",
                         "checked": "every clip is sampled across its length in the tests; no frame may pass through the ground"},
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED",
                       "animation": ("NOT_ACCEPTED — the collapse pose is an open blockout defect (see open_defects), and "
                                     "simultaneous locomotion and firing is unverified (see verification_pending)"),
                       "technical": ("PENDING — built against the provisional card; final technical acceptance waits on that card "
                                     "being incorporated into the authoritative requirements, its checks passing, and the open "
                                     "defects and pending verifications above being closed"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/riftstalker-review/riftstalker-candidate.png",
                            "concepts": ["EBS-CON-KHA-UNT-002"],
                            "canon": "DevelopmentBible.md SPEC-UNIT-006",
                            "gameplay": "Content/Data/Source/units.json ka_riftstalker"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {STRATA: [0.20, 0.195, 0.19], AMBER: [0.98, 0.66, 0.22], "_default": [0.5, 0.5, 0.5]},
            "emissive": [AMBER], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.32, "key": 0.85, "color": [1.0, 0.86, 0.7],
                      "fill_color": [0.5, 0.55, 0.7], "fill": 0.24},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [200, 200], "footprint_color": [0.98, 0.66, 0.22]},
            "reference_figure": {"height_cm": 180, "position": [-300, -290, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "side", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 100]},
             {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.12, "target": [0, 0, 100]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -14, "yaw_deg": 140, "arm_cm": 620, "fov_deg": 52, "target": [0, 0, 110]},
             {"name": "shoulder_detail", "type": "persp", "pitch_deg": -18, "yaw_deg": 155, "arm_cm": 300, "fov_deg": 48, "target": [40, 0, 165]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 0]},
             # the monochrome pass is the baseline for the Cairnback silhouette comparison: colour and
             # emissive must not be what separates two quadrupeds at tactical distance
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
    dump("lod1", [{"obj": f"../review/{ASSET}_baseline_LOD1.obj"}], ortho[:2] + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [ortho[0], views[0]])
    # A dedicated silhouette scene: NO ground and NO reference figure, so a mask sees the animal and
    # nothing else. Measuring the tactical pass with the ground in frame compared the ground.
    sil = dict(base)
    sil.pop("ground", None)
    sil.pop("reference_figure", None)
    sil["background"] = [1.0, 1.0, 1.0]
    sil["meshes"] = [{"obj": "../review/SK_EBS_KHA_UNT_002_baseline_LOD0.obj"}]
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
                      "worst_state": data["budgets"]["worst_state_triangles"],
                      "height_cm": mm["standing_height_cm"], "length_cm": mm["body_length_cm"],
                      "length_over_height": mm["body_length_over_height"],
                      "stance_cm": mm["stance_width_cm"],
                      "amber_area": data["budgets"]["amber_area_fraction_lod0"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
