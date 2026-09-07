#!/usr/bin/env python3
"""Deterministic source generator for EBS-MER-BLD-004 — the Meridian Compact Aegis Post.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md`: a three-legged ceramic mount with a rotating twin-emitter head, a heavy
power coupling with cables on the rear of the hub, and a cyan band that circles the head only while
supplied. Powered the head is level; offline it droops and the band goes dark. The card fixes a 2-bone
pitch-and-yaw aiming rig, so this ships as a skeletal asset (root + the two aiming bones).

Usage:
  python3 build_aegis.py --evidence-dir "<root>/EBS-MER-BLD-004" [--skinned]
  python3 build_aegis.py --evidence-dir "<root>/EBS-MER-BLD-004" --check

Units: centimetres. +X forward (the head's rest facing), +Y right, +Z up. Pivot at the ground-contact
centre between the pads. Nanite off. Nothing below z = 0; nothing outside the 2x2 tile footprint.
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
PACKAGE_ID = "EBS-PKG-MC-AEGIS-POST"
PRODUCTION_ID = "EBS-MER-BLD-004"
ASSET = "SK_EBS_MER_BLD_004"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_004/"
REVISION = "ebs-mer-bld-004-concept-v1"

CERAMIC = "MI_EBS_MER_CeramicCivic"
FRAME = "MI_EBS_MER_CompactFrame"
STATUS = "MI_EBS_MER_StatusCyan"

TILE_CM = 200.0
FOOTPRINT_TILES = 2
F = TILE_CM * FOOTPRINT_TILES          # 400 cm square
HALF = F / 2.0

PAD_R = 133.0                          # pad centres 266 apart; outer edges 340 across = the leg spread
PAD_SIZE = (74.0, 60.0, 18.0)
KNEE_R, KNEE_Z = 115.0, 178.0          # the knee: outboard and high, so the shins splay
HUB_R, HUB_Z = 60.0, 229.0             # the narrow turntable neck the head turns on
HUB_H = 52.0                           # a visible turntable drum between the legs and the head
HEAD_Z = 276.0                         # head centre when powered; the head sits on the hub top at 255
HEAD_SIZE = (110.0, 204.0, 42.0)       # a wide flat drum: 0.60 of the leg spread across, 0.14 of the height tall
BARREL_LEN, BARREL_R, BARREL_DY = 60.0, 26.0, 52.0
BAND_H = 12.0
LEG_YAWS = (60.0, -60.0, 180.0)        # two legs forward at +/-60 deg and one to the rear, as measured
OFFLINE_PITCH = -34.0                  # the head droops nose-down when unsupplied
STATES = ("powered", "offline")

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "ground contact centre between the pads; the post never moves"),
    ("turret_yaw", "root", (0.0, 0.0, HUB_Z), "card .ANIM_RIG aiming bone 1: yaw on the hub, 360 deg/s sweep"),
    ("turret_pitch", "turret_yaw", (0.0, 0.0, HEAD_Z), "card .ANIM_RIG aiming bone 2: pitch at the head trunnion"),
]

SOCKETS = {
    "Muzzle_Left": ("turret_pitch", (HEAD_SIZE[0] / 2.0 + BARREL_LEN, -BARREL_DY, HEAD_Z), 0.0,
                    "left emitter mouth: tracer origin"),
    "Muzzle_Right": ("turret_pitch", (HEAD_SIZE[0] / 2.0 + BARREL_LEN, BARREL_DY, HEAD_Z), 0.0,
                     "right emitter mouth: tracer origin"),
    "Target_Anchor_Center": ("turret_yaw", (0.0, 0.0, HUB_Z + HUB_H / 2.0), 0.0, "targeting and selection anchor at the hub"),
    "Power_Coupling": ("root", (-HUB_R - 34.0, 0.0, HUB_Z - 26.0), 180.0,
                       "heavy supply coupling: the network cable lands here and the offline state follows it"),
}


def _yaw_point(radius: float, yaw_deg: float):
    a = math.radians(yaw_deg)
    return (radius * math.cos(a), radius * math.sin(a))


def build_body(lod: int, state: str = "powered") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    ceramic = m.slot(CERAMIC)
    frame = m.slot(FRAME)
    status = m.slot(STATUS)
    fine = lod == 0

    # 1. three legs: a pale ceramic thigh from the hub out to a high knee, then a charcoal shin
    #    splaying down to a flat ground pad. Tubes, so the splay reads from every angle.
    sides = 6 if fine else 4
    for index, yaw in enumerate(LEG_YAWS, start=1):
        hx, hy = _yaw_point(HUB_R - 8.0, yaw)
        kx, ky = _yaw_point(KNEE_R, yaw)
        px, py = _yaw_point(PAD_R, yaw)
        m.tube((hx, hy, HUB_Z - HUB_H / 2.0), (kx, ky, KNEE_Z), 21.0, sides, ceramic, f"leg_{index:02d}_thigh")
        m.box((kx, ky, KNEE_Z), (42.0, 42.0, 42.0), frame, f"leg_{index:02d}_knee", yaw_deg=yaw)
        m.tube((kx, ky, KNEE_Z), (px, py, PAD_SIZE[2]), 16.0, sides, frame, f"leg_{index:02d}_shin")
        m.box((px, py, PAD_SIZE[2] / 2.0), PAD_SIZE, ceramic, f"leg_{index:02d}_pad", yaw_deg=yaw)
        if fine:
            m.box((px, py, PAD_SIZE[2] + 6.0), (PAD_SIZE[0] - 24.0, PAD_SIZE[1] - 22.0, 12.0), frame,
                  f"leg_{index:02d}_pad_boss", yaw_deg=yaw)

    # 2. the hub the head turns on
    m.prism(kit.regular_polygon(HUB_R, 8 if fine else 6), HUB_Z - HUB_H / 2.0, HUB_Z + HUB_H / 2.0, frame, "hub")
    m.prism(kit.regular_polygon(HUB_R - 8.0, 8 if fine else 6), HUB_Z + HUB_H / 2.0, HUB_Z + HUB_H / 2.0 + 14.0,
            ceramic, "hub_collar")

    # 3. heavy power coupling on the rear of the hub, with two cables running down the rear leg
    m.box((-HUB_R - 12.0, 0.0, HUB_Z - 26.0), (40.0, 58.0, 40.0), frame, "power_coupling")
    for i, dy in enumerate((-18.0, 18.0)):
        rear_x, rear_y = _yaw_point(PAD_R - 20.0, 180.0)
        m.tube((-HUB_R - 26.0, dy, HUB_Z - 36.0), (rear_x, rear_y + dy, 26.0), 9.0, 6 if fine else 4,
               frame, f"power_cable_{i + 1:02d}")

    # 4. the rotating head, built about the trunnion so the pitch bone can droop it
    m.box((0.0, 0.0, HEAD_Z), HEAD_SIZE, ceramic, "head_shell")
    m.box((-HEAD_SIZE[0] / 2.0 + 10.0, 0.0, HEAD_Z - 6.0), (26.0, HEAD_SIZE[1] - 44.0, 30.0), frame, "head_yoke")
    band_slot = status if state == "powered" else frame
    band_z = HEAD_Z - HEAD_SIZE[2] / 2.0 + BAND_H / 2.0 + 2.0
    for tag, centre, size in (
            ("l", (6.0, -HEAD_SIZE[1] / 2.0 - 2.0, band_z), (HEAD_SIZE[0] - 22.0, 5.0, BAND_H)),
            ("r", (6.0, HEAD_SIZE[1] / 2.0 + 2.0, band_z), (HEAD_SIZE[0] - 22.0, 5.0, BAND_H)),
            ("f", (HEAD_SIZE[0] / 2.0 + 2.0, 0.0, band_z), (5.0, HEAD_SIZE[1] - 12.0, BAND_H))):
        m.box(centre, size, band_slot, f"supply_band_{tag}")
    for i, dy in enumerate((-BARREL_DY, BARREL_DY)):
        m.tube((HEAD_SIZE[0] / 2.0 - 8.0, dy, HEAD_Z), (HEAD_SIZE[0] / 2.0 + BARREL_LEN, dy, HEAD_Z),
               BARREL_R, 6 if fine else 4, ceramic, f"barrel_{'r' if dy > 0 else 'l'}")
        tag = "r" if dy > 0 else "l"
        # a dark charcoal bezel with a small lit core inside it: the muzzle is a recess, not a lamp
        m.box((HEAD_SIZE[0] / 2.0 + BARREL_LEN - 3.0, dy, HEAD_Z), (8.0, 30.0, 30.0), frame, f"muzzle_{tag}")
        m.box((HEAD_SIZE[0] / 2.0 + BARREL_LEN - 1.0, dy, HEAD_Z), (4.0, 13.0, 13.0), band_slot, f"muzzle_core_{tag}")
    if fine:
        m.box((-HEAD_SIZE[0] / 2.0 - 7.0, 0.0, HEAD_Z + 4.0), (14.0, 70.0, 22.0), frame, "head_rear_sight")
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith(("head_", "barrel_", "muzzle_", "supply_band")):
            by_component[c] = "turret_pitch"
        elif c.startswith(("hub", "hub_collar")):
            by_component[c] = "turret_yaw"
    return skel.bind_polygons(m, "root", by_component)


def assemble(lod: int, state: str = "powered"):
    m = build_body(lod, state)
    s = build_skeleton()
    counts = bind(m)
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    sockets_on_bones = {n: v[0] for n, v in SOCKETS.items()}
    m.collision.append(kit.CollisionBox("mount", (0.0, 0.0, (HUB_Z + HUB_H / 2.0) / 2.0),
                                        (2 * KNEE_R, 2 * KNEE_R, HUB_Z + HUB_H / 2.0)))
    return m, s, counts, sockets_on_bones


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def new(name, duration, loop, purpose):
        c = skel.AnimationClip(name, duration, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    idle = new("powered_idle", 4.0, True, "supplied and watching: a slow yaw scan with the head level")
    for t, yaw in ((0.0, -18.0), (2.0, 18.0), (4.0, -18.0)):
        idle.key("turret_yaw", t, (0.0, yaw, 0.0))
        idle.key("turret_pitch", t, (0.0, 0.0, 0.0))

    aim = new("aim", 0.6, False, "swing onto a target: yaw and pitch lead, the runtime supplies the angles (360 deg/s)")
    aim.key("turret_yaw", 0.0, (0.0, -30.0, 0.0)); aim.key("turret_yaw", 0.6, (0.0, 24.0, 0.0))
    aim.key("turret_pitch", 0.0, (0.0, 0.0, 0.0)); aim.key("turret_pitch", 0.6, (6.0, 0.0, 0.0))

    fire = new("fire", 0.4, False, "discharge: a short recoil back through the head, returning to the aim line")
    for t, pitch in ((0.0, 0.0), (0.1, 7.0), (0.4, 0.0)):
        fire.key("turret_pitch", t, (pitch, 0.0, 0.0))

    offline = new("offline", 0.6, False, "power severed: the head droops nose-down and holds (card: within 1 tick)")
    offline.key("turret_pitch", 0.0, (0.0, 0.0, 0.0)); offline.key("turret_pitch", 0.6, (OFFLINE_PITCH, 0.0, 0.0))
    offline.key("turret_yaw", 0.0, (0.0, 0.0, 0.0)); offline.key("turret_yaw", 0.6, (0.0, 0.0, 0.0))

    restore = new("restore", 0.0, False, "single-frame powered rest pose for reconstruction from saved state")
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


def posed(lod: int, pose: dict, state: str = "powered") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "three_legged_mount": {"contract": 3, "built": sum(1 for c in comps if c.endswith("_pad")),
                               "knees": sum(1 for c in comps if c.endswith("_knee")),
                               "note": "three legs, not four and not a pedestal (canon and both review decisions)"},
        "twin_emitters": {"contract": 2, "built": sum(1 for c in comps if c.startswith("barrel_")),
                          "muzzles": sum(1 for c in comps if c.startswith("muzzle_") and not c.startswith("muzzle_core_")),
                          "lit_muzzle_cores": sum(1 for c in comps if c.startswith("muzzle_core_"))},
        "rotating_head": {"contract": 1, "built": 1 if "head_shell" in comps else 0},
        "supply_band": {"contract": "one band circling the head, lit only while supplied",
                        "built": sum(1 for c in comps if c.startswith("supply_band_"))},
        "power_coupling": {"contract": 1, "built": 1 if "power_coupling" in comps else 0,
                           "cables": sum(1 for c in comps if c.startswith("power_cable_"))},
        "bones": {"contract": "2 aiming bones (card .ANIM_RIG) plus the root", "built": len(s.bones),
                  "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": [sk.name for sk in m.sockets]},
        "tracks": {"contract": ["powered_idle", "aim", "fire", "offline", "restore"], "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    spread = max(abs(x0), abs(x1), abs(y0), abs(y1)) * 2.0
    head = m.component_bounds("head_shell")
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2), "leg_spread_cm": round(2 * PAD_R + PAD_SIZE[0], 2),
        "height_over_leg_spread": round(z1 / (2 * PAD_R + PAD_SIZE[0]), 4),
        "head_width_over_leg_spread": round((head[1][1] - head[0][1]) / (2 * PAD_R + PAD_SIZE[0]), 4),
        "hub_top_over_height": round((HUB_Z + HUB_H / 2.0) / z1, 4),
        "everything_inside_the_footprint": max(abs(x0), abs(x1), abs(y0), abs(y1)) <= HALF + 1e-6,
        "max_extent_cm": round(max(abs(x0), abs(x1), abs(y0), abs(y1)), 2),
        "_overall_span_cm": round(spread, 2),
    }


def slot_area_fraction(m: kit.Mesh, slot_name: str) -> float:
    """Share of the mesh's SURFACE AREA carried by one slot; the cards cap emissive by area."""
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


POSE_SAMPLES = [("powered_idle", 0.5, "powered"), ("aim", 1.0, "powered"),
                ("fire", 0.25, "powered"), ("offline", 1.0, "offline")]


def export(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, _c, socks = assemble(lod, "powered")
        base = os.path.join(out_dir, f"{ASSET}_LOD{lod}")
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod, "author": AUTHOR}
        if skinned:
            digest = skel.write_skinned_glb(m, s, base + ".glb", animations=clips, extras=extras,
                                            include_collision=False, sockets_on_bones=socks)
            outputs.append({"mesh": ASSET, "lod": lod, "kind": "skinned + clips", "path": os.path.relpath(base + ".glb", HERE),
                            "sha256": digest, "triangles": m.triangle_count(), "bounds_cm": [list(p) for p in m.bounds()],
                            "section_slot_names": m.slots, "by_slot": m.triangle_count_by("slot"),
                            "clips": [c.name for c in clips],
                            "sockets": [{"name": sk.name, "bone": socks[sk.name], "position_cm": [round(v, 2) for v in sk.position],
                                         "yaw_deg": sk.yaw_deg, "purpose": sk.purpose} for sk in m.sockets]})
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (powered)", f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ", "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count()})
    m1 = assemble(1, "powered")[0]
    path = os.path.join(review, f"{ASSET}_powered_LOD1.obj")
    review_rows.append({"name": "powered_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]), "triangles": m1.triangle_count()})
    by_name = {c.name: c for c in clips}
    for name, fraction, state in POSE_SAMPLES:
        mesh = posed(0, sample_pose(by_name[name], fraction), state)
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        p = os.path.join(review, stem + ".obj")
        review_rows.append({"name": stem, "path": os.path.relpath(p, evidence_dir),
                            "sha256": mesh.write_obj(p, header_lines=[f"{ASSET} posed: {name} at {fraction:.2f} ({state})"]),
                            "lowest_z_cm": round(mesh.bounds()[0][2], 2),
                            "max_extent_cm": round(max(abs(v) for p2 in mesh.bounds() for v in p2[:2]), 2)})
    return {"outputs": outputs, "review": review_rows, "clips": clips}


def manifest(exported: dict) -> dict:
    m0, s, counts, _socks = assemble(0, "powered")
    m1 = assemble(1, "powered")[0]
    clips = exported["clips"]
    emissive = slot_area_fraction(m0, STATUS)
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"), "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward (the head's rest facing), +Y right, +Z up",
                  "pivot": "ground-contact centre between the pads", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-015.MC.AEGIS (Bible line 516): a three-legged ceramic mount with a rotating twin-emitter head",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "source": "Content/Data/Source/buildings.json mc_aegis_post.footprint_cells"},
        "material_slots": [CERAMIC, FRAME, STATUS],
        "material_slot_policy": ("Pale ceramic plate, charcoal compact frame, and the cyan status slot that carries the supply band "
                                 "and muzzle interiors ONLY while powered. Offline moves the band and muzzles into the frame slot, "
                                 "so loss of power is visible in the asset itself, not only in a material parameter."),
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "lod0_cap": 4000, "lod1_cap": 1600,
                    "cap_source": "REL-BLD-015.MC.AEGIS.ASSET (tighter than REL-ART-028, so the card governs)",
                    "cap_scope": "Owner ruling 2026-09-07: ceilings apply to the complete asset including articulated components",
                    "lod0_within_cap": m0.triangle_count() <= 4000, "lod1_within_cap": m1.triangle_count() <= 1600,
                    "status_area_fraction_lod0": round(emissive, 5),
                    "status_measure": "surface area; the cyan band and the muzzle interiors are the only status geometry"},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("The card fixes a 2-bone pitch-and-yaw aiming rig; those two bones plus the root are the whole skeleton. "
                           "No root motion — the post never moves. Yaw sweeps at 360 deg/s and snaps under Reduced Motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "loop": c.loop, "purpose": c.purpose,
                   "bones": sorted(c.tracks)} for c in clips],
        "states": {name: purpose for name, purpose in (
            ("powered", "head level and scanning, the cyan band and muzzle interiors lit"),
            ("offline", f"the head droops {abs(OFFLINE_PITCH):.0f} deg nose-down and every status surface goes dark, within 1 tick of losing supply"))},
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "technical": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
        "recorded_card_conflict": ("REL-BLD-015.MC.AEGIS.ASSET .MESH_PROP describes 'a vertical heavy turret barrel mounted onto an "
                                   "elevated orthogonal protective concrete pillbox carriage'. That contradicts the canon row, both "
                                   "review decisions and the selected candidate, all of which call for a three-legged ceramic mount "
                                   "with twin emitters; decision B explicitly replaces the bunker. This build follows canon and the "
                                   "concept. Raised as OWNER-QUESTION A in the README; unresolved."),
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/aegis-post-review/aegis-post-candidate.png",
                            "concepts": ["EBS-CON-MER-BLD-004 (REWORK)", "EBS-CON-MER-BLD-008 (REWORK)"],
                            "canon": "DevelopmentBible.md line 516 (SPEC-BLD-015.MC.AEGIS)",
                            "gameplay": "Content/Data/Source/buildings.json mc_aegis_post"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {CERAMIC: [0.80, 0.78, 0.73], FRAME: [0.17, 0.175, 0.19], STATUS: [0.24, 0.86, 0.94],
                          "_default": [0.5, 0.5, 0.5]},
            "emissive": [STATUS], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.32, "key": 0.85, "color": [1.0, 0.86, 0.7],
                      "fill_color": [0.5, 0.55, 0.7], "fill": 0.24},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [F, F], "footprint_color": [0.24, 0.86, 0.94]},
            "reference_figure": {"height_cm": 180, "position": [-460, 460, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15, "target": [0, 0, 150]},
             {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.15, "target": [0, 0, 150]},
             {"name": "rear", "type": "ortho", "from": "-X", "edges": True, "margin": 1.15, "target": [0, 0, 150]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.3, "target": [0, 0, 0]}]
    tactical = [{"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
                {"name": "three_quarter", "type": "persp", "pitch_deg": -20, "yaw_deg": 138, "arm_cm": 820, "fov_deg": 50, "target": [0, 0, 185]},
                {"name": "rear_supply", "type": "persp", "pitch_deg": -18, "yaw_deg": -32, "arm_cm": 780, "fov_deg": 50, "target": [-60, 0, 150]}]
    written = []

    def dump(name, meshes, views):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = views
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + tactical)
    dump("lod1", [{"obj": f"../review/{ASSET}_powered_LOD1.obj"}], ortho[:2] + [tactical[0]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [ortho[1], ortho[0], tactical[1]])
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
        saved = json.load(open(manifest_path, encoding="utf-8"))
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
    lows = [r["lowest_z_cm"] for r in data["review_assemblies"] if "lowest_z_cm" in r]
    ext = [r["max_extent_cm"] for r in data["review_assemblies"] if "max_extent_cm" in r]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"], "lod1": data["budgets"]["lod1_triangles"],
                      "height_cm": data["concept_measurements"]["height_cm"],
                      "h_over_leg_spread": data["concept_measurements"]["height_over_leg_spread"],
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]),
                      "status_area": data["budgets"]["status_area_fraction_lod0"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_posed_extent_cm": max(ext) if ext else None, "footprint_half_cm": HALF,
                      "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
