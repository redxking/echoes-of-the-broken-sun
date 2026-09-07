#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-BLD-003 — the Kharuun Assemblies Growth Basin.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-BLD-016.KA.BASIN.ASSET`: a shallow
circular bowl of coursed strata with an amber matrix pool three quarters of the bowl across, ringed by
six OPEN curved molt niches. The niches do not close; occupancy reads as a lit niche floor, and each
niche bone carries canon's crack-and-settle at molt completion.

Usage:
  python3 build_basin.py --evidence-dir "<root>/EBS-KHA-BLD-003" [--skinned]
  python3 build_basin.py --evidence-dir "<root>/EBS-KHA-BLD-003" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the bowl's ground-contact centre. Nanite off.
Nothing below z = 0; nothing outside the 4x4 tile footprint.
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
PACKAGE_ID = "EBS-PKG-KA-GROWTH-BASIN"
PRODUCTION_ID = "EBS-KHA-BLD-003"
ASSET = "SK_EBS_KHA_BLD_003"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_003/"
REVISION = "ebs-kha-bld-003-concept-v1"
CARD = "REL-BLD-016.KA.BASIN.ASSET"

STRATA = "MI_EBS_KHA_Strata"      # faceted coursed strata: bowl, rim, niche walls
AMBER = "MI_EBS_KHA_Amber"        # the matrix pool and the occupied niche floors — the only emissive

TICKS_PER_SECOND = 20.0
MOLT_TICKS = 80.0                 # buildings.json ka_growth_basin.adaptation.molt_ticks

TILE_CM = 200.0
FOOTPRINT_TILES = 4
F = TILE_CM * FOOTPRINT_TILES          # 800 cm square
HALF = F / 2.0

# The traced 0.75 is the pool against the WHOLE basin, niche bulges included, so the rim ring is thin
# and the niches project outward from it. BOWL_R is the rim, OUTER_R is the asset.
OUTER_R = 380.0                        # the asset's radius: 760 cm across, inside the 800 cm square
POOL_R = OUTER_R * 0.75                # 0.75 of the basin, as traced
RIM_IN, RIM_OUT = POOL_R, 330.0        # the rim ring meets the pool exactly at its inner edge
BOWL_R = RIM_OUT
RIM_COURSES = 3
RIM_Z = 52.0                           # the inner rim: a shallow bowl, per canon
POOL_Z = 14.0                          # the pool surface, slightly below the rim lip
NICHE_COUNT = 6                        # owner ruling 2026-09-07: six physical alcoves, following the concept.
                                       # This is the VISUAL COMPONENT COUNT, not a concurrent-molt capacity.
NICHE_YAWS = tuple(360.0 * i / NICHE_COUNT for i in range(NICHE_COUNT))
NICHE_HALF_DEG = 15.0                  # angular half-width of a niche alcove
NICHE_WALL_Z = 112.0                   # the outer curved wall: taller than the rim, as drawn
NICHE_IN, NICHE_OUT = 346.0, OUTER_R   # the niche bulges outward from the rim to the asset's edge
NICHE_FLOOR_R = (NICHE_IN + NICHE_OUT) / 2.0
NICHE_PLINTH_Z = 16.0                  # the niche sits on a plinth bound to root, so the settle at molt
                                       # completion never drives the alcove below ground
STATES = ("idle", "growing", "molting", "damaged", "destroyed")
MOLTING_NICHES = (1, 4)                # ILLUSTRATIVE only: the two alcoves lit in the review's molting assembly,
                                       # chosen to show the per-alcove tell. At runtime the lit set follows
                                       # authoritative adaptation activity and implies no gameplay slots.

BONES = [("root", None, (0.0, 0.0, 0.0), "bowl ground-contact centre; the basin never moves")]
for _i, _yaw in enumerate(NICHE_YAWS, start=1):
    _a = math.radians(_yaw)
    BONES.append((f"niche_{_i:02d}", "root",
                  (math.cos(_a) * NICHE_FLOOR_R, math.sin(_a) * NICHE_FLOOR_R, 0.0),
                  f"molt niche {_i}: carries canon's crack-and-settle at molt completion; the niche does not close"))
NICHE_BONES = tuple(b[0] for b in BONES[1:])


def _yaw_point(radius: float, yaw_deg: float):
    a = math.radians(yaw_deg)
    return (radius * math.cos(a), radius * math.sin(a))


def _arc(radius: float, yaw0: float, yaw1: float, steps: int):
    return [_yaw_point(radius, yaw0 + (yaw1 - yaw0) * k / steps) for k in range(steps + 1)]



def _arc_solid(m, r_in, r_out, yaw0, yaw1, z0, z1, steps, slot, component):
    """A solid annular arc between two radii and two bearings: the outer ring is continuous stone
    except where an alcove is cut into it, which is how the candidate draws it."""
    outer = _arc(r_out, yaw0, yaw1, steps)
    inner = _arc(r_in, yaw0, yaw1, steps)
    for k in range(steps):
        mid = ((outer[k][0] + outer[k + 1][0]) / 2.0, (outer[k][1] + outer[k + 1][1]) / 2.0, 0.0)
        m.add_polygon([(*outer[k], z0), (*outer[k + 1], z0), (*outer[k + 1], z1), (*outer[k], z1)],
                      slot, component, normal=mid)
        m.add_polygon([(*inner[k + 1], z0), (*inner[k], z0), (*inner[k], z1), (*inner[k + 1], z1)],
                      slot, component, normal=(-mid[0], -mid[1], 0.0))
        m.add_polygon([(*outer[k], z1), (*outer[k + 1], z1), (*inner[k + 1], z1), (*inner[k], z1)],
                      slot, component, normal=(0.0, 0.0, 1.0))
        m.add_polygon([(*inner[k], z0), (*inner[k + 1], z0), (*outer[k + 1], z0), (*outer[k], z0)],
                      slot, component, normal=(0.0, 0.0, -1.0))
    for end, sign in ((0, -1.0), (steps, 1.0)):
        a = math.radians(yaw0 if end == 0 else yaw1)
        n = (-math.sin(a) * sign, math.cos(a) * sign, 0.0)
        m.add_polygon([(*inner[end], z0), (*outer[end], z0), (*outer[end], z1), (*inner[end], z1)],
                      slot, component, normal=n)


def build_body(lod: int, state: str = "idle") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    amber = m.slot(AMBER)
    fine = lod == 0
    sides = 16 if fine else 10
    ruined = state == "destroyed"

    # 1. the bowl: a thin rim ring of stacked courses around a wide pool, shallow and faceted
    for course in range(RIM_COURSES):
        r_out = RIM_OUT - 8.0 * course
        z0 = RIM_Z * course / RIM_COURSES
        z1 = RIM_Z * (course + 1) / RIM_COURSES
        if ruined:
            z0, z1 = z0 * 0.45, z1 * 0.45
        m.ring(kit.regular_polygon(r_out, sides, phase_deg=6.0 * course),
               kit.regular_polygon(RIM_IN, sides, phase_deg=6.0 * course), z0, z1, strata,
               f"rim_course_{course + 1:02d}")
    m.prism(kit.regular_polygon(RIM_IN, sides), 0.0, POOL_Z * 0.6, strata, "bowl_floor")

    # 2. the matrix pool: the asset's identity, and allowed to dominate the amber budget
    if not ruined:
        m.prism(kit.regular_polygon(POOL_R, sides), POOL_Z * 0.6, POOL_Z,
                amber if state != "damaged" else strata, "matrix_pool", cap_bottom=False)
    else:
        m.prism(kit.regular_polygon(POOL_R * 0.8, sides), 0.0, 8.0, strata, "matrix_pool_dark", cap_bottom=False)

    # 3. the molt niches: OPEN curved alcoves. The candidate shows a warform standing in an open niche,
    #    so nothing closes over it; occupancy reads as the lit niche floor instead.
    if not ruined:
        steps = 5 if fine else 3
        # the outer ring is continuous stone between the alcoves, not a set of floating slabs
        for index, yaw in enumerate(NICHE_YAWS, start=1):
            nxt = NICHE_YAWS[index % NICHE_COUNT] + (360.0 if index == NICHE_COUNT else 0.0)
            _arc_solid(m, RIM_OUT - 6.0, NICHE_OUT, yaw + NICHE_HALF_DEG, nxt - NICHE_HALF_DEG,
                       0.0, NICHE_PLINTH_Z + NICHE_WALL_Z, steps, strata, f"ring_spur_{index:02d}")
        for index, yaw in enumerate(NICHE_YAWS, start=1):
            y0, y1 = yaw - NICHE_HALF_DEG, yaw + NICHE_HALF_DEG
            # the alcove: a plinth bound to root, a curved back wall, and an open mouth facing the pool
            px_, py_ = _yaw_point(NICHE_FLOOR_R, yaw)
            m.box((px_, py_, NICHE_PLINTH_Z / 2.0), (NICHE_OUT - RIM_OUT + 12.0, 158.0, NICHE_PLINTH_Z),
                  strata, f"plinth_{index:02d}", yaw_deg=yaw)
            # the alcove's back wall is LOWER than the ring spurs, so each niche reads as a pocket
            # cut into the ring rather than as more unbroken wall
            _arc_solid(m, NICHE_OUT - 30.0, NICHE_OUT, y0, y1, NICHE_PLINTH_Z,
                       NICHE_PLINTH_Z + NICHE_WALL_Z * 0.55, steps, strata, f"niche_{index:02d}_wall")
            fx, fy = _yaw_point((RIM_OUT + NICHE_OUT - 30.0) / 2.0, yaw)
            occupied = state == "molting" and index in MOLTING_NICHES
            m.box((fx, fy, NICHE_PLINTH_Z + 6.0), (NICHE_OUT - 30.0 - RIM_OUT, 128.0, 12.0),
                  amber if occupied else strata, f"niche_{index:02d}_floor", yaw_deg=yaw)
            if occupied:
                # the floor alone sits down inside the pocket and is invisible past the ring spurs at
                # gameplay distance, so an occupied niche also lights the lip of its low back wall,
                # which reads over the ring from the tactical camera
                lip = NICHE_PLINTH_Z + NICHE_WALL_Z * 0.55
                _arc_solid(m, NICHE_OUT - 32.0, NICHE_OUT + 2.0, y0, y1, lip, lip + 14.0, steps,
                           amber, f"niche_{index:02d}_lip")

    if state == "damaged":
        for k, yaw in enumerate((28.0, 152.0, 268.0), start=1):
            cx, cy = _yaw_point(RIM_OUT - 24.0, yaw)
            m.box((cx, cy, (RIM_Z + 22.0) / 2.0), (46.0, 36.0, RIM_Z + 22.0), strata, f"crack_{k:02d}", yaw_deg=yaw)

    if ruined:
        for k in range(6 if fine else 4):
            yaw = 360.0 * k / (6 if fine else 4) + 22.0
            rx, ry = _yaw_point(RIM_OUT * 0.72, yaw)
            m.box((rx, ry, 26.0), (120.0, 90.0, 52.0), strata, f"rubble_{k + 1:02d}", yaw_deg=yaw)

    m.collision.append(kit.CollisionBox("bowl", (0.0, 0.0, RIM_Z / 2.0),
                                        (2 * RIM_OUT, 2 * RIM_OUT, RIM_Z)))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith("niche_"):
            by_component[c] = f"niche_{c.split('_')[1]}"
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Target_Anchor_Center": ("root", (0.0, 0.0, RIM_Z + 40.0), 0.0, "targeting and selection anchor over the pool"),
    "Rally_Default": ("root", (-OUTER_R - 150.0, 0.0, 0.0), 180.0, "default rally point, clear of the niches"),
    "Pool_Center": ("root", (0.0, 0.0, POOL_Z), 0.0, "the matrix pool surface: the growing effect anchor"),
}
for _i, _yaw in enumerate(NICHE_YAWS, start=1):
    _p = _yaw_point(NICHE_FLOOR_R, _yaw)
    SOCKETS[f"Molt_Niche_{_i:02d}"] = (f"niche_{_i:02d}", (_p[0], _p[1], NICHE_PLINTH_Z + 12.0), _yaw,
                                       f"where a warform stands to molt in niche {_i}")


def assemble(lod: int, state: str = "idle"):
    m = build_body(lod, state)
    s = build_skeleton()
    counts = bind(m)
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    return m, s, counts, {n: v[0] for n, v in SOCKETS.items()}


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def new(name, ticks, loop, purpose):
        c = skel.AnimationClip(name, ticks / TICKS_PER_SECOND, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    idle = new("idle", 80, True, "resting: the niches are still and the pool holds its low glow")
    for b in NICHE_BONES:
        for t in (0.0, idle.duration_s):
            idle.key(b, t, (0.0, 0.0, 0.0))

    grow = new("growing", 80, True,
               "growing: the niche stone is still. What changes in this state is the POOL'S EMISSIVE, "
               "which is a material parameter and not geometry; the clip exists so the runtime has a "
               "named state to play")
    for b in NICHE_BONES:
        for t in (0.0, grow.duration_s):
            grow.key(b, t, (0.0, 0.0, 0.0))

    start = new("molt_start", 20, False, "a warform steps into a niche: that niche's stone takes the weight and settles a little")
    for index, b in enumerate(NICHE_BONES):
        start.key(b, 0.0, (0.0, 0.0, 0.0))
        start.key(b, 1.0, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, -3.0 if index in [n - 1 for n in MOLTING_NICHES] else 0.0))

    done = new("molt_complete", MOLT_TICKS / 4.0, False,
               "canon's crack-and-settle at completion: the niche stone drops sharply, then recovers")
    for index, b in enumerate(NICHE_BONES):
        active = index in [n - 1 for n in MOLTING_NICHES]
        for t, dz in ((0.0, -3.0 if active else 0.0), (0.2, -11.0 if active else 0.0),
                      (0.7, 2.0 if active else 0.0), (done.duration_s, 0.0)):
            done.key(b, t, (0.0, 0.0, 0.0), translation_cm=(0.0, 0.0, dz))

    restore = new("restore", 0, False, "single-frame rest pose for reconstruction from saved state")
    for b in NICHE_BONES:
        restore.key(b, 0.0, (0.0, 0.0, 0.0))

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


def posed(lod: int, pose: dict, state: str = "idle") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("molt_start", 1.0, "molting"), ("molt_complete", 0.15, "molting"),
                ("molt_complete", 0.5, "molting")]


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


def lit_niches(m: kit.Mesh) -> list:
    """Which niche floors are in the amber slot — the per-niche occupancy tell."""
    out = []
    for p in m.polygons:
        if p.component.startswith("niche_") and p.component.endswith(("_floor", "_lip")) \
                and m.slots[p.slot] == AMBER:
            out.append(int(p.component.split("_")[1]))
    return sorted(set(out))


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "shallow_bowl": {"contract": f"{RIM_COURSES} rim courses plus a floor",
                         "courses": sum(1 for c in comps if c.startswith("rim_course_")),
                         "floor": "bowl_floor" in comps},
        "matrix_pool": {"contract": "one amber pool, 0.75 of the bowl across",
                        "built": "matrix_pool" in comps or "matrix_pool_dark" in comps,
                        "diameter_over_basin": round(POOL_R / OUTER_R, 4)},
        "molt_niches": {"contract": NICHE_COUNT,
                        "walls": len({c for c in comps if c.endswith("_wall")}),
                        "ring_spurs": len({c for c in comps if c.startswith("ring_spur_")}),
                        "floors": len({c for c in comps if c.startswith("niche_") and c.endswith("_floor")}),
                        "lit_lips": len({c for c in comps if c.endswith("_lip")}),
                        "closes": False,
                        "note": "OPEN alcoves: the candidate shows a warform standing in an open niche"},
        "bones": {"contract": f"root plus one bone per niche ({NICHE_COUNT})", "built": len(s.bones),
                  "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["idle", "growing", "molt_start", "molt_complete", "restore"],
                   "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "basin_diameter_cm": 2 * OUTER_R,
        "rim_diameter_cm": 2 * RIM_OUT,
        "pool_diameter_cm": round(2 * POOL_R, 2),
        "pool_over_basin": round(POOL_R / OUTER_R, 4),
        "height_over_basin_diameter": round(z1 / (2 * OUTER_R), 4),
        "niche_count": NICHE_COUNT,
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
        m, s, _c, socks = assemble(lod, "idle")
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
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (idle)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count(), "lit_niches": lit_niches(m),
                            "height_cm": round(m.bounds()[1][2], 2)})
    m1 = assemble(1, "idle")[0]
    path = os.path.join(review, f"{ASSET}_idle_LOD1.obj")
    review_rows.append({"name": "idle_lod1", "path": os.path.relpath(path, evidence_dir),
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
    m0, s, counts, _socks = assemble(0, "idle")
    m1 = assemble(1, "idle")[0]
    molting = assemble(0, "molting")[0]
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
                  "pivot": "bowl ground-contact centre", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-016 Growth Basin row: a shallow bowl of grown strata with an amber-lit matrix pool at its centre, ringed by visible molt niches",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "source": "Content/Data/Source/buildings.json ka_growth_basin.footprint_cells",
                        "tick_rate": f"{TICKS_PER_SECOND:.0f} ticks per second; molt_ticks {MOLT_TICKS:.0f} = {MOLT_TICKS / TICKS_PER_SECOND:.1f} s"},
        "material_slots": [STRATA, AMBER],
        "material_slot_policy": ("Two slots: faceted coursed strata for the bowl, rim and niche walls, and amber for the matrix "
                                 "pool and the occupied niche floors, which are the only emissive. The pool is allowed to "
                                 "dominate the amber budget because canon makes it the asset's identity."),
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/kharuun-asset-cards.json (rendered to kharuun-asset-cards.md)",
            "status": ("PROVISIONAL. Authored in this worktree under the owner ruling of 2026-09-07; NOT incorporated into "
                       "Docs/Requirements.md and not an existing authoritative per-asset requirement."),
            "bounds": {"lod0_triangles": 8000, "lod1_triangles": 3500,
                       "basis": "the faction default, no tighter contract applies"}},
        "alcove_count": {
            "value": NICHE_COUNT,
            "status": "CONFIRMED by the owner on 2026-09-07, following the concept",
            "meaning": ("Six is this asset's VISUAL COMPONENT COUNT. It is NOT a concurrent-molt capacity and does not imply six "
                        "independently available gameplay slots. The card's earlier requirement that the count match a gameplay "
                        "capacity was unsupported and has been removed. No six-unit limit, reservation, queue or other adaptation "
                        "rule follows from this asset, and no gameplay data was changed."),
            "history": "Raised as OWNER-QUESTION A because ka_growth_basin.adaptation carries no concurrent-molt capacity; resolved 2026-09-07"},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "molting_state_triangles": molting.triangle_count(),
                    "lod0_cap": 8000, "lod1_cap": 3500, "cap_source": f"{CARD} (PROVISIONAL)",
                    "cap_scope": "the complete assembly including every niche",
                    "lod0_within_cap": max(m0.triangle_count(), molting.triangle_count()) <= 8000,
                    "lod1_within_cap": m1.triangle_count() <= 3500,
                    "amber_area_fraction_lod0": round(amber, 5), "amber_cap": 0.15,
                    "amber_measure": "surface area, per REL-ART-029; the cap is a ceiling, not a target",
                    "amber_within_cap": amber <= 0.15},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("Root plus one bone per niche. The niches DO NOT close — the candidate shows a warform standing in an "
                           "open alcove — so each niche bone carries canon's crack-and-settle at molt completion instead. No root "
                           "motion; the basin never moves.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {name: purpose for name, purpose in (
            ("idle", "the pool holds a low steady glow and every niche floor is dark"),
            ("growing", "the pool brightens; that change is the pool's EMISSIVE, a material parameter, not geometry"),
            ("molting", f"the occupied niches' floors are lit and the rest stay dark (review assembly lights niches {list(MOLTING_NICHES)})"),
            ("damaged", "the bowl cracked and the pool moved out of the amber slot"),
            ("destroyed", "the bowl broken, the pool dark, and a rubble ring where the rim came down"))},
        "occupancy_tell": {"mechanism": "the per-alcove floor and back-wall lip move into the amber slot",
                           "authority": ("The lit set shall reflect AUTHORITATIVE ADAPTATION ACTIVITY. It is a presentation of what "
                                         "the runtime reports and never a promise about capacity; lighting two alcoves in the "
                                         "review assembly is illustrative, not a claim that two may molt at once."),
                           "open": ("Maximum-zoom readability is NOT yet verified. The tell is legible in the tactical camera and "
                                    "subtle at full zoom-out (owner ruling 2026-09-07: keep it open until verified)."),
                           "idle_lit": lit_niches(m0), "molting_lit": lit_niches(molting)},
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED",
                       "technical": ("PENDING — built against the provisional card; final technical acceptance waits on that card "
                                     "being incorporated into the authoritative requirements and its checks passing, and on "
                                     "maximum-zoom occupancy readability being verified"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/growth-basin-review/growth-basin-candidate.png",
                            "canon": "DevelopmentBible.md SPEC-BLD-016 Growth Basin row",
                            "gameplay": "Content/Data/Source/buildings.json ka_growth_basin"},
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
                       "footprint_cm": [F, F], "footprint_color": [0.98, 0.66, 0.22]},
            "reference_figure": {"height_cm": 180, "position": [-520, 480, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "-X", "edges": True, "margin": 1.12, "target": [0, 0, 60]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -30, "yaw_deg": 32, "arm_cm": 1500, "fov_deg": 50, "target": [0, 0, 40]},
             {"name": "niche_detail", "type": "persp", "pitch_deg": -24, "yaw_deg": 186, "arm_cm": 560, "fov_deg": 48, "target": [NICHE_FLOOR_R, 0, 60]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3600, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_idle_LOD1.obj"}], ortho + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [views[1], views[0]])
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
    ext = [r["max_extent_cm"] for r in data["review_assemblies"] if "max_extent_cm" in r]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"],
                      "lod1": data["budgets"]["lod1_triangles"],
                      "molting": data["budgets"]["molting_state_triangles"],
                      "height_cm": mm["height_cm"], "pool_over_basin": mm["pool_over_basin"],
                      "niches": mm["niche_count"], "amber_area": data["budgets"]["amber_area_fraction_lod0"],
                      "molting_lit": data["occupancy_tell"]["molting_lit"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_posed_extent_cm": max(ext) if ext else None,
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
