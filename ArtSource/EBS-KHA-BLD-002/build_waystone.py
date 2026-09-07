#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-BLD-002 — the Kharuun Assemblies Waystone.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-BLD-016.KA.WAYSTONE.ASSET`: a tall
faceted monolith of stacked strata courses with amber seams, a stepped root plinth, roots that splay
flat when rooted and braid into vertical bundles when mobile, and a carriage plate on four stubby legs
that appears only while migrating.

The roots' splay-versus-braid is a STATE, not a pose: a braid is not a rotation of a splay. The bones
carry the ring sink and the carriage lift; the state carries the root form.

Usage:
  python3 build_waystone.py --evidence-dir "<root>/EBS-KHA-BLD-002" [--skinned]
  python3 build_waystone.py --evidence-dir "<root>/EBS-KHA-BLD-002" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the ground-contact centre under the monolith.
Nanite off. Nothing below z = 0; nothing outside the 2x2 tile footprint in any state.
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
PACKAGE_ID = "EBS-PKG-KA-WAYSTONE"
PRODUCTION_ID = "EBS-KHA-BLD-002"
ASSET = "SK_EBS_KHA_BLD_002"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_002/"
REVISION = "ebs-kha-bld-002-concept-v1"
CARD = "REL-BLD-016.KA.WAYSTONE.ASSET"

STRATA = "MI_EBS_KHA_Strata"      # faceted dark strata: shaft courses, plinth, roots, carriage
AMBER = "MI_EBS_KHA_Amber"        # seam bands and nodules — the only emissive

TICKS_PER_SECOND = 20.0           # from the Waystone's own record: 100 construction ticks = 5.0 s

TILE_CM = 200.0
FOOTPRINT_TILES = 2
F = TILE_CM * FOOTPRINT_TILES          # 400 cm square
HALF = F / 2.0

ROOT_SPREAD = 380.0                    # rooted root disc across, inside the 400 cm square
TOTAL_Z = 414.0                        # 1.09 of the root spread, as traced
SHAFT_W = 95.0                         # 0.25 of the root spread, as traced
SHAFT_BASE_Z = 182.0                   # the root mass flares below this: the shaft is the upper 0.56
COURSES = 7                            # visible stone courses up the shaft
PLINTH_STEPS = ((76.0, 0.0, 44.0), (66.0, 44.0, 92.0), (58.0, 92.0, 132.0))  # (half-width, z0, z1)
ROOT_COUNT = 12
ROOT_LEN = 80.0
ROOT_FLAT_R = 8.0                      # a flat root's radius; its top must clear neither 0 nor 20 cm
ROOT_BRAID_R = 13.0                    # thicker where the roots braid upright and are read up close
ROOT_FLAT_Z = 12.0                     # inner end height; with ROOT_FLAT_R the root spans z 0..20,
                                       # inside REL-ART-030's 20 cm ground-clutter ceiling and never below ground
CARRIAGE_PLATE = 350.0 / 2.0           # 0.92 of the rooted spread, as traced
CARRIAGE_Z = 74.0                      # the plate's top when migrating
LEG_R, LEG_W, LEG_Z = 128.0, 46.0, 74.0
MOBILE_LIFT = 46.0                     # how far the body rides above its rooted height while migrating
STATES = ("rooted", "uprooted_mobile", "damaged", "destroyed")

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "ground-contact centre under the monolith; the runtime owns translation"),
    ("ring_sink", "root", (0.0, 0.0, 60.0), "the root plinth and every root ride this bone: it sinks on root and rises on uproot"),
    ("carriage_lift", "root", (0.0, 0.0, 120.0), "the monolith shaft rides this bone: it lifts the body onto the carriage"),
    ("foot_01", "carriage_lift", (LEG_R, LEG_R, LEG_Z), "carriage foot, forward right"),
    ("foot_02", "carriage_lift", (LEG_R, -LEG_R, LEG_Z), "carriage foot, forward left"),
    ("foot_03", "carriage_lift", (-LEG_R, -LEG_R, LEG_Z), "carriage foot, rear left"),
    ("foot_04", "carriage_lift", (-LEG_R, LEG_R, LEG_Z), "carriage foot, rear right"),
]
FEET = ("foot_01", "foot_02", "foot_03", "foot_04")

SOCKETS = {
    "Target_Anchor_Center": ("carriage_lift", (0.0, 0.0, SHAFT_BASE_Z + 90.0), 0.0,
                             "targeting and selection anchor on the shaft"),
    "Matter_Dropoff": ("root", (-ROOT_SPREAD / 2.0 - 8.0, 0.0, 12.0), 180.0,
                       "where workers deliver matter while the node is rooted"),
    "Root_Ring_Center": ("ring_sink", (0.0, 0.0, 0.0), 0.0,
                         "the root ring's own centre: the rooted/uprooted tell hangs off this"),
    "Carriage_Front": ("carriage_lift", (LEG_R + 20.0, 0.0, CARRIAGE_Z), 0.0,
                       "front of the grown carriage, for migration facing"),
}


def _course(index: int):
    """(half-width, z0, z1) for one shaft course; the shaft tapers slightly as it rises."""
    span = TOTAL_Z - SHAFT_BASE_Z
    z0 = SHAFT_BASE_Z + span * index / COURSES
    z1 = SHAFT_BASE_Z + span * (index + 1) / COURSES
    taper = 1.0 - 0.16 * (index / max(1, COURSES - 1))
    return SHAFT_W / 2.0 * taper, z0, z1


def build_body(lod: int, state: str = "rooted") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    amber = m.slot(AMBER)
    fine = lod == 0
    sides = 8 if fine else 6
    mobile = state == "uprooted_mobile"
    lift = MOBILE_LIFT if mobile else 0.0

    if state == "destroyed":
        # the monolith falls; canon leaves the ring in the ground
        for step, (hw, z0, z1) in enumerate(PLINTH_STEPS[:2], start=1):
            m.prism(kit.regular_polygon(hw, sides), z0 * 0.4, z1 * 0.4, strata, f"plinth_{step:02d}")
        for k in range(ROOT_COUNT if fine else ROOT_COUNT // 2):
            yaw = 360.0 * k / (ROOT_COUNT if fine else ROOT_COUNT // 2)
            a = math.radians(yaw)
            m.tube((math.cos(a) * 66.0, math.sin(a) * 66.0, ROOT_FLAT_R),
                   (math.cos(a) * (66.0 + ROOT_LEN), math.sin(a) * (66.0 + ROOT_LEN), ROOT_FLAT_R),
                   ROOT_FLAT_R * 0.8, 4, strata, f"root_{k + 1:02d}")
        for k in range(4 if fine else 3):
            # the fallen courses have to land inside the placement envelope like everything else
            m.box((-30.0 - 36.0 * k, 22.0 * (k % 2) - 11.0, 26.0), (86.0, 70.0, 52.0), strata,
                  f"fallen_course_{k + 1:02d}", yaw_deg=12.0 * k)
        m.collision.append(kit.CollisionBox("wreck", (0.0, 0.0, 30.0), (ROOT_SPREAD, ROOT_SPREAD, 60.0)))
        return m

    # 1. the stepped root plinth
    for step, (hw, z0, z1) in enumerate(PLINTH_STEPS, start=1):
        m.prism(kit.regular_polygon(hw, sides), z0 + lift, z1 + lift, strata, f"plinth_{step:02d}",
                cap_bottom=(step == 1))

    # 1b. the flared root mass that grips the shaft between the plinth top and the first course.
    #     Without it the shaft floated above the plinth with a 50 cm gap.
    for tier, (hw, z0, z1) in enumerate(((70.0, PLINTH_STEPS[-1][2], 158.0),
                                         (58.0, 158.0, SHAFT_BASE_Z)), start=1):
        m.prism(kit.regular_polygon(hw, sides, phase_deg=7.0 * tier), z0 + lift, z1 + lift, strata,
                f"flare_{tier:02d}")

    # 2. the roots. ROOTED: splayed flat across the ground in a wide disc, under the 20 cm ground-clutter
    #    ceiling. MOBILE: retracted and braided into vertical bundles hugging the plinth.
    count = ROOT_COUNT if fine else ROOT_COUNT // 2
    for k in range(count):
        yaw = 360.0 * k / count
        a = math.radians(yaw)
        inner = PLINTH_STEPS[0][0] - 10.0
        if mobile:
            bx, by = math.cos(a) * (inner + 16.0), math.sin(a) * (inner + 16.0)
            m.tube((bx, by, lift + 6.0), (bx * 0.55, by * 0.55, lift + PLINTH_STEPS[-1][2] + 46.0),
                   ROOT_BRAID_R, 4, strata, f"root_{k + 1:02d}")
        else:
            ox, oy = math.cos(a) * (inner + ROOT_LEN), math.sin(a) * (inner + ROOT_LEN)
            m.tube((math.cos(a) * inner, math.sin(a) * inner, ROOT_FLAT_Z),
                   (ox, oy, ROOT_FLAT_R), ROOT_FLAT_R, 4, strata, f"root_{k + 1:02d}")
            if fine:
                # the tip continues outward on a slightly turned bearing, so the disc reads as roots
                # rather than as spokes; computed from the bearing, never by dividing by cos(a)
                b = a + 0.17
                tx = math.cos(b) * (inner + ROOT_LEN + 44.0)
                ty = math.sin(b) * (inner + ROOT_LEN + 44.0)
                m.tube((ox, oy, ROOT_FLAT_R), (tx, ty, ROOT_FLAT_R * 0.7),
                       ROOT_FLAT_R * 0.7, 4, strata, f"root_{k + 1:02d}_tip")

    # 3. the carriage: a grown plate on four stubby legs of stacked courses, mobile only
    if mobile:
        m.prism(kit.regular_polygon(CARRIAGE_PLATE, sides), CARRIAGE_Z - 26.0, CARRIAGE_Z, strata, "carriage_plate")
        for index, (sx, sy) in enumerate(((1, 1), (1, -1), (-1, -1), (-1, 1)), start=1):
            px, py = LEG_R * sx, LEG_R * sy
            for tier, (hw, z0, z1) in enumerate(((LEG_W / 2.0, 0.0, 26.0), (LEG_W / 2.0 - 5.0, 26.0, 52.0),
                                                 (LEG_W / 2.0 - 9.0, 52.0, LEG_Z)), start=1):
                m.prism(kit.regular_polygon(hw, 6 if fine else 4), z0, z1, strata,
                        f"leg_{index:02d}_course_{tier:02d}", center=(px, py))

    # 4. the monolith: stacked faceted courses with amber seam bands between them
    for i in range(COURSES):
        hw, z0, z1 = _course(i)
        m.prism(kit.regular_polygon(hw, sides, phase_deg=11.0 * i), z0 + lift, z1 + lift, strata,
                f"course_{i + 1:02d}", cap_bottom=(i == 0), cap_top=(i == COURSES - 1))
        if i < COURSES - 1:
            m.ring(kit.regular_polygon(hw + 2.0, sides, phase_deg=11.0 * i),
                   kit.regular_polygon(hw - 2.0, sides, phase_deg=11.0 * i),
                   z1 - 4.0 + lift, z1 + lift, amber, f"seam_{i + 1:02d}")
    if fine:
        for k, z in ((1, SHAFT_BASE_Z + 118.0), (2, SHAFT_BASE_Z + 156.0)):
            for sx in (-1.0, 1.0):
                m.box((sx * (SHAFT_W / 2.0 + 4.0), 0.0, z + lift), (12.0, 16.0, 16.0), amber, f"nodule_{k:02d}_{'r' if sx > 0 else 'l'}")

    if state == "damaged":
        for k, (z, yaw) in enumerate(((SHAFT_BASE_Z + 62.0, 40.0), (SHAFT_BASE_Z + 148.0, 210.0)), start=1):
            a = math.radians(yaw)
            hw = _course(min(COURSES - 1, int((z - SHAFT_BASE_Z) / (TOTAL_Z - SHAFT_BASE_Z) * COURSES)))[0]
            m.box((math.cos(a) * (hw + 3.0), math.sin(a) * (hw + 3.0), z), (22.0, 26.0, 54.0), strata,
                  f"break_{k:02d}", yaw_deg=yaw)

    m.collision.append(kit.CollisionBox("shaft", (0.0, 0.0, (TOTAL_Z + lift) / 2.0),
                                        (SHAFT_W + 24.0, SHAFT_W + 24.0, TOTAL_Z + lift)))
    m.collision.append(kit.CollisionBox("plinth", (0.0, 0.0, (PLINTH_STEPS[-1][2] + lift) / 2.0),
                                        (2 * PLINTH_STEPS[0][0], 2 * PLINTH_STEPS[0][0], PLINTH_STEPS[-1][2] + lift)))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith(("plinth_", "root_")):
            by_component[c] = "ring_sink"
        elif c.startswith(("course_", "seam_", "nodule_", "break_", "carriage_plate", "flare_")):
            by_component[c] = "carriage_lift"
        elif c.startswith("leg_"):
            by_component[c] = FEET[int(c.split("_")[1]) - 1]
    return skel.bind_polygons(m, "root", by_component)


def assemble(lod: int, state: str = "rooted"):
    m = build_body(lod, state)
    s = build_skeleton()
    counts = bind(m)
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    return m, s, counts, {n: v[0] for n, v in SOCKETS.items()}


def build_clips(skeleton: skel.Skeleton) -> list:
    """Canon's rooting has four beats: preparation, contact, settling, release."""
    clips = []

    def new(name, ticks, loop, purpose):
        c = skel.AnimationClip(name, ticks / TICKS_PER_SECOND, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    idle = new("rooted_idle", 80, True, "rooted and supplying: the body is still; the seams carry the life")
    for t in (0.0, idle.duration_s):
        idle.key("ring_sink", t, (0.0, 0.0, 0.0))
        idle.key("carriage_lift", t, (0.0, 0.0, 0.0))

    up = new("uproot", 40, False, "40 ticks: the ring pulls out of the ground and the body rises onto the carriage")
    for t, ring, body in ((0.0, 0.0, 0.0), (0.6, 6.0, 4.0), (2.0, MOBILE_LIFT, MOBILE_LIFT)):
        up.key("ring_sink", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, ring))
        up.key("carriage_lift", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, body))
    for index, foot in enumerate(FEET):
        up.key(foot, 0.0, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, -MOBILE_LIFT))
        up.key(foot, 1.2 + 0.2 * index, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, 0.0))
        up.key(foot, 2.0, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, 0.0))

    move = new("mobile_move", 30, True, "migrating at 120 cm/s: the feet take the load in turn under a steady body")
    for index, foot in enumerate(FEET):
        phase = index / len(FEET) * move.duration_s
        for step in range(5):
            t = (phase + step * move.duration_s / 4.0) % move.duration_s
            move.key(foot, t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, 9.0 if step % 2 else 0.0))
    for t in (0.0, move.duration_s):
        move.key("carriage_lift", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, MOBILE_LIFT))
        move.key("ring_sink", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, MOBILE_LIFT))

    down = new("root", 60, False, "60 ticks in canon's four beats: preparation, contact, settling, release")
    beats = ((0.0, MOBILE_LIFT, MOBILE_LIFT), (0.9, MOBILE_LIFT - 4.0, MOBILE_LIFT),   # preparation
             (1.7, 8.0, 14.0),                                                          # contact
             (2.6, -6.0, 0.0),                                                          # settling (an overshoot)
             (3.0, 0.0, 0.0))                                                           # release
    for t, ring, body in beats:
        down.key("ring_sink", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, ring))
        down.key("carriage_lift", t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, body))
    for index, foot in enumerate(FEET):
        # the feet are children of carriage_lift, so as the body descends they must translate UP by the
        # same amount to stay planted. Keying them down instead drove the legs through the ground.
        down.key(foot, 0.0, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, 0.0))
        down.key(foot, 1.7 + 0.12 * index, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, MOBILE_LIFT * 0.7))
        down.key(foot, 3.0, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, MOBILE_LIFT))

    restore = new("restore", 0, False, "single-frame rooted rest pose for reconstruction from saved state")
    for b in skeleton.bones:
        if b.name != "root":
            restore.key(b.name, 0.0, (0.0, 0.0, 0.0))

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


def posed(lod: int, pose: dict, state: str = "rooted") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("uproot", 0.5, "rooted"), ("uproot", 1.0, "uprooted_mobile"),
                ("mobile_move", 0.25, "uprooted_mobile"), ("root", 0.6, "uprooted_mobile")]


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


def footprint_span(m: kit.Mesh) -> float:
    (x0, y0, _), (x1, y1, _) = m.bounds()
    return max(x1 - x0, y1 - y0)


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "monolith_courses": {"contract": COURSES, "built": sum(1 for c in comps if c.startswith("course_"))},
        "amber_seams": {"contract": COURSES - 1, "built": sum(1 for c in comps if c.startswith("seam_"))},
        "root_plinth": {"contract": len(PLINTH_STEPS), "built": sum(1 for c in comps if c.startswith("plinth_")),
                        "flare_tiers": sum(1 for c in comps if c.startswith("flare_"))},
        "roots": {"contract": "splayed flat when rooted, braided upright when mobile",
                  "built": sum(1 for c in comps if c.startswith("root_") and not c.endswith("_tip"))},
        "carriage": {"contract": "a plate on FOUR stubby legs, present only while migrating",
                     "plate": "carriage_plate" in comps,
                     "legs": len({c.split("_course_")[0] for c in comps if c.startswith("leg_")})},
        "bones": {"contract": "root, ring_sink, carriage_lift and four feet (card REL-BLD-016.KA.WAYSTONE.ASSET)",
                  "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["rooted_idle", "uproot", "mobile_move", "root", "restore"],
                   "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh, mobile: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    spread = footprint_span(m)
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "rooted_root_spread_cm": round(spread, 2),
        "mobile_span_cm": round(footprint_span(mobile), 2),
        "mobile_span_over_rooted": round(footprint_span(mobile) / spread, 4),
        "total_height_over_root_spread": round(z1 / spread, 4),
        "shaft_width_over_root_spread": round(SHAFT_W / spread, 4),
        "shaft_share_of_height": round((TOTAL_Z - SHAFT_BASE_Z) / TOTAL_Z, 4),
        "flat_root_top_cm": ROOT_FLAT_Z + ROOT_FLAT_R,
        "flat_roots_under_ground_clutter_ceiling": ROOT_FLAT_Z + ROOT_FLAT_R <= 20.0,
        "everything_inside_the_footprint": max(abs(x0), abs(x1), abs(y0), abs(y1)) <= HALF + 1e-6,
        "max_extent_cm": round(max(abs(x0), abs(x1), abs(y0), abs(y1)), 2),
    }


def export(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, _c, socks = assemble(lod, "rooted")
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
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (rooted)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count(), "span_cm": round(footprint_span(m), 2),
                            "height_cm": round(m.bounds()[1][2], 2)})
    m1 = assemble(1, "rooted")[0]
    path = os.path.join(review, f"{ASSET}_rooted_LOD1.obj")
    review_rows.append({"name": "rooted_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]),
                        "triangles": m1.triangle_count()})
    by_name = {c.name: c for c in clips}
    for name, fraction, state in POSE_SAMPLES:
        mesh = posed(0, sample_pose(by_name[name], fraction), state)
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        p = os.path.join(review, stem + ".obj")
        review_rows.append({"name": stem, "path": os.path.relpath(p, evidence_dir),
                            "sha256": mesh.write_obj(p, header_lines=[f"{ASSET} posed: {name} at {fraction:.2f} ({state})"]),
                            "lowest_z_cm": round(mesh.bounds()[0][2], 2),
                            "max_extent_cm": round(max(abs(v) for b in mesh.bounds() for v in b[:2]), 2)})
    return {"outputs": outputs, "review": review_rows, "clips": clips}


def manifest(exported: dict) -> dict:
    m0, s, counts, _socks = assemble(0, "rooted")
    m1 = assemble(1, "rooted")[0]
    mobile = assemble(0, "uprooted_mobile")[0]
    clips = exported["clips"]
    amber = slot_area_fraction(m0, AMBER)
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "ground-contact centre under the monolith", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-016 Waystone row: a tall faceted monolith of dark strata with amber seams; rooted it sinks a visible ring of root-strata, uprooted it lifts on a grown carriage",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "source": "Content/Data/Source/buildings.json ka_waystone.footprint_cells",
                        "tick_rate": f"{TICKS_PER_SECOND:.0f} ticks per second, from the Waystone's own 100-tick / 5.0 s construction figure"},
        "material_slots": [STRATA, AMBER],
        "material_slot_policy": ("Two slots: faceted dark strata for the shaft courses, plinth, roots and carriage, and amber for "
                                 "the seam bands and nodules, which are the only emissive. No wheels, tracks, treads, pistons or "
                                 "machined carriage anywhere (card .MAT_RULE)."),
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/kharuun-asset-cards.json (rendered to kharuun-asset-cards.md)",
            "status": ("PROVISIONAL. Authored in this worktree under the owner ruling of 2026-09-07; NOT incorporated into "
                       "Docs/Requirements.md and not an existing authoritative per-asset requirement."),
            "bounds": {"lod0_triangles": 5000, "lod1_triangles": 2200,
                       "basis": "tighter than the faction default of 8,000/3,500 because a 2x2 monolith that must also move pays a cost the static structures do not"}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "mobile_state_triangles": mobile.triangle_count(),
                    "lod0_cap": 5000, "lod1_cap": 2200, "cap_source": f"{CARD} (PROVISIONAL)",
                    "cap_scope": "the complete assembly including the root ring and the grown carriage",
                    "lod0_within_cap": max(m0.triangle_count(), mobile.triangle_count()) <= 5000,
                    "lod1_within_cap": m1.triangle_count() <= 2200,
                    "amber_area_fraction_lod0": round(amber, 5), "amber_cap": 0.15,
                    "amber_measure": "surface area, per REL-ART-029; the cap is a ceiling, not a target",
                    "amber_within_cap": amber <= 0.15},
        "concept_measurements": measurements(m0, mobile),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("The card's own rig: root, ring_sink, carriage_lift and four feet. The roots' splay-versus-braid is a "
                           "STATE and not a bone pose, because a braid is not a rotation of a splay. No root motion; the runtime "
                           "owns translation while mobile.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {name: purpose for name, purpose in (
            ("rooted", "the ring is sunk, the roots splay flat across the ground, no carriage"),
            ("uprooted_mobile", "the roots retract into braided bundles, the carriage plate and four legs carry the body clear of the ground"),
            ("damaged", "seams broken and courses chipped"),
            ("destroyed", "the monolith falls and canon's ring is left in the ground"))},
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED",
                       "technical": ("PENDING — built against the provisional card; final technical acceptance waits on that card "
                                     "being incorporated into the authoritative requirements and its checks passing"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/waystone-review/waystone-candidate.png",
                            "concepts": ["EBS-CON-KHA-BLD-002", "EBS-CON-KHA-BLD-006"],
                            "canon": "DevelopmentBible.md SPEC-BLD-016 Waystone row",
                            "gameplay": "Content/Data/Source/buildings.json ka_waystone"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {STRATA: [0.22, 0.215, 0.21], AMBER: [0.98, 0.66, 0.22], "_default": [0.5, 0.5, 0.5]},
            "emissive": [AMBER], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.32, "key": 0.85, "color": [1.0, 0.86, 0.7],
                      "fill_color": [0.5, 0.55, 0.7], "fill": 0.24},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [F, F], "footprint_color": [0.98, 0.66, 0.22]},
            "reference_figure": {"height_cm": 180, "position": [-330, 300, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "-X", "edges": True, "margin": 1.12, "target": [0, 0, 200]},
             {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 200]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.25, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -16, "yaw_deg": 32, "arm_cm": 900, "fov_deg": 52, "target": [0, 0, 200]},
             {"name": "root_detail", "type": "persp", "pitch_deg": -30, "yaw_deg": 26, "arm_cm": 480, "fov_deg": 50, "target": [0, 0, 70]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 2600, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_rooted_LOD1.obj"}], ortho[:2] + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [ortho[0], views[0], views[1]])
    return written


def build_all(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    return manifest(export(evidence_dir, out_dir, skinned))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--skinned", action="store_true", help="also write the skinned GLBs with the clips")
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
    ext = [r["max_extent_cm"] for r in data["review_assemblies"] if "max_extent_cm" in r]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"],
                      "lod1": data["budgets"]["lod1_triangles"],
                      "mobile": data["budgets"]["mobile_state_triangles"],
                      "height_cm": mm["height_cm"], "h_over_spread": mm["total_height_over_root_spread"],
                      "shaft_over_spread": mm["shaft_width_over_root_spread"],
                      "mobile_over_rooted": mm["mobile_span_over_rooted"],
                      "amber_area": data["budgets"]["amber_area_fraction_lod0"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_posed_extent_cm": max(ext) if ext else None,
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
