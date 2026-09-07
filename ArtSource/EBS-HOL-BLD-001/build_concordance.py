#!/usr/bin/env python3
"""Deterministic source generator for EBS-HOL-BLD-001 — the Hollow Choir Concordance.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-BLD-017.HC.CONCORDANCE.ASSET`, which
was authored only after the candidate was traced: a low circular disc carrying fourteen slab PAIRS,
each a vitrified slab with a second slab offset 3 cm behind it, with a ground intake apron at the front
and magenta fracture edging on the slab borders.

Usage:
  python3 build_concordance.py --evidence-dir "<root>/EBS-HOL-BLD-001" [--skinned]
  python3 build_concordance.py --evidence-dir "<root>/EBS-HOL-BLD-001" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the disc's ground-contact centre.
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
PACKAGE_ID = "EBS-PKG-HC-CONCORDANCE"
PRODUCTION_ID = "EBS-HOL-BLD-001"
ASSET = "SK_EBS_HOL_BLD_001"
PLANNED_FOLDER = "/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_001/"
REVISION = "ebs-hol-bld-001-concept-v1"
CARD = "REL-BLD-017.HC.CONCORDANCE.ASSET"

VITRIFIED = "MI_EBS_HOL_Vitrified"   # the slabs and the disc: flat vitrified stone
GROUND = "MI_EBS_HOL_ApronStone"     # the intake apron, worn by delivery
MAGENTA = "MI_EBS_HOL_Magenta"       # fracture edging — the only emissive

TICKS_PER_SECOND = 20.0

TILE_CM = 200.0
FOOTPRINT_TILES = 5
F = TILE_CM * FOOTPRINT_TILES          # 1000 cm square
HALF = F / 2.0

DISC_R = 470.0                         # 940 cm across, inside the 1000 cm square
DISC_Z = 22.0                          # barely raised, as the candidate draws it
SLAB_RING_R = 386.0                    # where the pairs stand on the disc
PAIR_COUNT = 14                        # counted on the candidate's RING / INTAKE view
SLAB_H = 226.0                         # 0.24 of the ring diameter, as traced
SLAB_W, SLAB_T = 124.0, 9.0
OFFSET_CM = 3.0                        # the reality-bleed offset the HC unit cards specify
EDGE_T = 3.0
APRON_YAW = 180.0                      # the intake faces -X, at the front of the ring
APRON_LEN, APRON_W = 132.0, 300.0
DAMAGED_PAIR = 4                       # which pair cracks in the damaged state
STATES = ("working", "damaged", "destroyed")

PAIR_YAWS = tuple(360.0 * i / PAIR_COUNT for i in range(PAIR_COUNT))

BONES = [("root", None, (0.0, 0.0, 0.0), "disc ground-contact centre; the Concordance never moves")]
for _i, _yaw in enumerate(PAIR_YAWS, start=1):
    _a = math.radians(_yaw)
    BONES.append((f"pair_{_i:02d}", "root",
                  (math.cos(_a) * SLAB_RING_R, math.sin(_a) * SLAB_RING_R, DISC_Z),
                  f"slab pair {_i}: lets the pair drift independently. Faction reality-bleed device, "
                  f"NOT a canon motion clause for this building"))
PAIR_BONES = tuple(b[0] for b in BONES[1:])


def _yaw_point(radius: float, yaw_deg: float):
    a = math.radians(yaw_deg)
    return (radius * math.cos(a), radius * math.sin(a))


def build_body(lod: int, state: str = "working") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    vitrified = m.slot(VITRIFIED)
    ground = m.slot(GROUND)
    magenta = m.slot(MAGENTA)
    fine = lod == 0
    sides = 20 if fine else 12
    ruined = state == "destroyed"

    # 1. the low disc
    m.prism(kit.regular_polygon(DISC_R, sides), 0.0, DISC_Z * (0.6 if ruined else 1.0), vitrified, "disc")

    # 2. the intake apron at the front of the ring, where workers deliver Matter
    ax, ay = _yaw_point(SLAB_RING_R + 40.0, APRON_YAW)
    m.box((ax, ay, DISC_Z + 5.0), (APRON_LEN, APRON_W, 10.0), ground, "intake_apron", yaw_deg=APRON_YAW)
    if fine and not ruined:
        for k, off in enumerate((-92.0, -30.0, 34.0, 96.0), start=1):
            mx, my = _yaw_point(SLAB_RING_R + 62.0, APRON_YAW)
            m.box((mx - off * math.sin(math.radians(APRON_YAW)), my + off * math.cos(math.radians(APRON_YAW)),
                   DISC_Z + 18.0), (26.0, 22.0, 20.0), vitrified, f"delivered_matter_{k:02d}",
                  yaw_deg=APRON_YAW + 14.0 * k)

    # 3. the slab pairs. Each is a slab plus a second slab offset 3 cm behind it: the pair reads as one
    #    object that has not settled on where it is.
    for index, yaw in enumerate(PAIR_YAWS, start=1):
        cracked = state == "damaged" and index == DAMAGED_PAIR
        px, py = _yaw_point(SLAB_RING_R, yaw)
        if ruined:
            # the ring is broken: slabs down on the disc, no pairs standing
            fx, fy = _yaw_point(SLAB_RING_R * 0.82, yaw)
            m.box((fx, fy, DISC_Z + 12.0), (SLAB_H * 0.7, SLAB_W * 0.8, 16.0), vitrified,
                  f"fallen_slab_{index:02d}", yaw_deg=yaw + 24.0 * ((index % 3) - 1))
            continue
        for layer, (back, tag) in enumerate(((0.0, "a"), (-OFFSET_CM, "b"))):
            ox, oy = _yaw_point(SLAB_RING_R + back, yaw)
            m.box((ox, oy, DISC_Z + SLAB_H / 2.0), (SLAB_T, SLAB_W, SLAB_H), vitrified,
                  f"pair_{index:02d}_slab_{tag}", yaw_deg=yaw)
            if fine:
                # fracture edging down the two vertical borders of each slab. The borders sit along the
                # slab's WIDTH, which runs on the ring's tangent, so the offset is tangential — putting
                # both at the slab centre (an earlier slip) drew one bar instead of two edges.
                a = math.radians(yaw)
                tangent = (-math.sin(a), math.cos(a))
                for sign, side in ((-1.0, "l"), (1.0, "r")):
                    lateral = sign * (SLAB_W / 2.0 - EDGE_T / 2.0)
                    m.box((ox + tangent[0] * lateral, oy + tangent[1] * lateral, DISC_Z + SLAB_H / 2.0),
                          (SLAB_T + 1.0, EDGE_T, SLAB_H - 12.0), magenta,
                          f"pair_{index:02d}_edge_{tag}{side}", yaw_deg=yaw)
        if cracked and fine:
            m.box((px, py, DISC_Z + SLAB_H * 0.52), (SLAB_T + 2.0, 14.0, SLAB_H * 0.62), vitrified,
                  f"crack_{index:02d}", yaw_deg=yaw + 8.0)
        del px, py

    m.collision.append(kit.CollisionBox("disc", (0.0, 0.0, DISC_Z / 2.0), (2 * DISC_R, 2 * DISC_R, DISC_Z)))
    if not ruined:
        m.collision.append(kit.CollisionBox("ring", (0.0, 0.0, DISC_Z + SLAB_H / 2.0),
                                            (2 * (SLAB_RING_R + SLAB_W / 2.0), 2 * (SLAB_RING_R + SLAB_W / 2.0), SLAB_H)))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith("pair_") or c.startswith("crack_"):
            by_component[c] = f"pair_{c.split('_')[1]}"
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Target_Anchor_Center": ("root", (0.0, 0.0, DISC_Z + SLAB_H * 0.6), 0.0, "targeting and selection anchor inside the ring"),
    "Unit_Emergence": ("root", (_yaw_point(SLAB_RING_R * 0.5, APRON_YAW)[0], _yaw_point(SLAB_RING_R * 0.5, APRON_YAW)[1], DISC_Z), APRON_YAW,
                       "Threadkeepers emerge inside the ring and leave past the apron"),
    "Rally_Default": ("root", (_yaw_point(DISC_R + 150.0, APRON_YAW)[0], _yaw_point(DISC_R + 150.0, APRON_YAW)[1], 0.0), APRON_YAW,
                      "default emergence rally point, clear of the apron"),
    "Matter_Dropoff": ("root", (_yaw_point(SLAB_RING_R + 62.0, APRON_YAW)[0], _yaw_point(SLAB_RING_R + 62.0, APRON_YAW)[1], DISC_Z + 10.0), APRON_YAW,
                       "the intake apron: where workers deliver Matter"),
    "Choir_Thread_Center": ("root", (0.0, 0.0, DISC_Z), 0.0,
                            "the centre filament the candidate draws is a LIGHT EFFECT, not geometry; this socket is all the mesh provides for it"),
}


def assemble(lod: int, state: str = "working"):
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

    idle = new("working_idle", 120, True,
               "the ring at work: each pair drifts a little on its own phase, so the assembly never "
               "quite agrees with itself. Faction reality-bleed device, NOT a canon motion clause")
    damaged = new("damaged", 60, True,
                  "a cracked pair falls out of step: it drifts further than the rest")
    for clip, hurt_scale in ((idle, None), (damaged, 2.6)):
        for index, bone in enumerate(PAIR_BONES):
            phase = index / len(PAIR_BONES)
            scale = hurt_scale if (hurt_scale and index == DAMAGED_PAIR - 1) else 0.9
            for step in range(5):
                # the key TIME walks the cycle; the drift is read at that time PLUS this pair's phase
                t = step / 4.0 * clip.duration_s
                drift = math.sin(2.0 * math.pi * (step / 4.0 + phase))
                clip.key(bone, t, (0.0, scale * drift, 0.0), translation_cm=(0.0, 0.0, 0.8 * drift))

    restore = new("restore", 0, False, "single-frame rest pose for reconstruction from saved state")
    for bone in PAIR_BONES:
        restore.key(bone, 0.0, (0.0, 0.0, 0.0))

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


def posed(lod: int, pose: dict, state: str = "working") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("working_idle", 0.25, "working"), ("damaged", 0.25, "damaged")]


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


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    pairs = {c.split("_")[1] for c in comps if c.startswith("pair_")}
    return {
        "disc": {"contract": "one low circular plinth", "built": "disc" in comps},
        "slab_pairs": {"contract": PAIR_COUNT, "built": len(pairs),
                       "slabs": sum(1 for c in comps if "_slab_" in c),
                       "offset_cm": OFFSET_CM,
                       "note": "each pair is a slab plus a duplicate offset 3 cm behind it"},
        "fracture_edging": {"contract": "two vertical edges per slab",
                            "built": sum(1 for c in comps if "_edge_" in c)},
        "intake_apron": {"contract": "one apron at the front of the ring",
                         "built": "intake_apron" in comps,
                         "delivered_matter": sum(1 for c in comps if c.startswith("delivered_matter_"))},
        "centre_thread": {"contract": "a LIGHT EFFECT, not geometry",
                          "built_as_geometry": any("thread" in c for c in comps),
                          "socket": "Choir_Thread_Center"},
        "bones": {"contract": f"root plus one bone per pair ({PAIR_COUNT})", "built": len(s.bones),
                  "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["working_idle", "damaged", "restore"], "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "ring_diameter_cm": 2 * DISC_R,
        "slab_height_cm": SLAB_H,
        "slab_height_over_ring_diameter": round(SLAB_H / (2 * DISC_R), 4),
        "pair_count": PAIR_COUNT,
        "reality_bleed_offset_cm": OFFSET_CM,
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
        m, s, _c, socks = assemble(lod, "working")
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
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (working)",
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
    m1 = assemble(1, "working")[0]
    path = os.path.join(review, f"{ASSET}_working_LOD1.obj")
    review_rows.append({"name": "working_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]),
                        "triangles": m1.triangle_count()})
    by_name = {c.name: c for c in clips}
    for name, fraction, state in POSE_SAMPLES:
        mesh = posed(0, sample_pose(by_name[name], fraction), state)
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        p = os.path.join(review, stem + ".obj")
        review_rows.append({"name": stem, "path": os.path.relpath(p, evidence_dir),
                            "sha256": mesh.write_obj(p, header_lines=[f"{ASSET} posed: {name} at {fraction:.2f}"]),
                            "lowest_z_cm": round(mesh.bounds()[0][2], 2),
                            "max_extent_cm": round(max(abs(v) for b in mesh.bounds() for v in b[:2]), 2)})
    return {"outputs": outputs, "review": review_rows, "clips": clips}


def manifest(exported: dict) -> dict:
    m0, s, counts, _socks = assemble(0, "working")
    m1 = assemble(1, "working")[0]
    damaged = assemble(0, "damaged")[0]
    clips = exported["clips"]
    magenta = slot_area_fraction(m0, MAGENTA)
    worst = max(m0.triangle_count(), damaged.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "disc ground-contact centre", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-017.HC.CONCORDANCE: the Choir Command Core and Matter drop-off",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "source": "Content/Data/Source/buildings.json hc_concordance.footprint_cells"},
        "material_slots": [VITRIFIED, GROUND, MAGENTA],
        "material_slot_policy": ("Three slots: vitrified stone for the disc and every slab, a worn apron stone for the intake, "
                                 "and Magenta Fracture for the slab edging, which is the only emissive. No Kharuun strata "
                                 "banding, no Meridian plate seams or conduits, no amber and no cyan."),
        "traced_before_the_card": {
            "directive": "Owner ruling 2026-09-07: trace each selected concept BEFORE authoring its provisional card.",
            "trace": "the candidate's WORKING view, dark-pixel row spans; ring 952 px across, slab height 0.24 of it, 14 pairs counted on the RING / INTAKE view",
            "card_written_after": True},
        "faction_language_inference": {
            "what": "the 3 cm offset duplicate and the 12% Magenta Fracture ceiling",
            "source": "the four REL-FAC-027.HC.* UNIT cards, which specify both for Hollow Choir units",
            "status": ("INFERENCE, not a requirement. No authoritative card places either on a Hollow Choir BUILDING. "
                       "Recorded so the provenance is visible rather than implied.")},
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/hollow-choir-asset-cards.json (rendered to hollow-choir-asset-cards.md)",
            "status": ("PROVISIONAL. Authored in this worktree after the trace; NOT incorporated into Docs/Requirements.md and "
                       "not an existing authoritative per-asset requirement. The four REL-FAC-027.HC.* cards cover UNITS and "
                       "none of them governs this building."),
            "bounds": {"lod0_triangles": 8000, "lod1_triangles": 3500}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "damaged_state_triangles": damaged.triangle_count(), "worst_state_triangles": worst,
                    "lod0_cap": 8000, "lod1_cap": 3500, "cap_source": f"{CARD} (PROVISIONAL)",
                    "lod0_within_cap": worst <= 8000, "lod1_within_cap": m1.triangle_count() <= 3500,
                    "headroom_note": "Under the ceiling is HEADROOM, not sufficiency (owner ruling 2026-09-07).",
                    "magenta_area_fraction_lod0": round(magenta, 5), "magenta_cap": 0.12,
                    "magenta_within_cap": magenta <= 0.12},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("Root plus one bone per slab pair so pairs drift independently. That drift is the faction's "
                           "reality-bleed device, NOT a canon motion clause for this building. No root motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {"working": "every pair whole, edging lit, apron clear",
                   "damaged": f"pair {DAMAGED_PAIR} cracked and drifting further than the rest",
                   "destroyed": "the ring broken: slabs down on a scarred disc"},
        "pending_requirements": [
            {"requirement": "Articulated components for a structure of this role", "status": "PENDING, not waived",
             "detail": ("The pair drift is a presentation device, not a role-required articulated assembly. Do not mark this "
                        "asset compliant until the requirement is implemented or the card is amended.")}],
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "animation": "NOT_EVALUATED",
                       "technical": ("PENDING — built against a provisional card that is not in the authoritative requirements"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/concordance-review/concordance-candidate.png",
                            "canon": "DevelopmentBible.md / Requirements SPEC-BLD-017.HC.CONCORDANCE",
                            "gameplay": "Content/Data/Source/buildings.json hc_concordance"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {VITRIFIED: [0.40, 0.40, 0.42], GROUND: [0.30, 0.29, 0.29],
                          MAGENTA: [0.86, 0.36, 0.92], "_default": [0.5, 0.5, 0.5]},
            "emissive": [MAGENTA], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.34, "key": 0.82, "color": [0.95, 0.94, 1.0],
                      "fill_color": [0.6, 0.58, 0.72], "fill": 0.26},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [F, F], "footprint_color": [0.86, 0.36, 0.92]},
            "reference_figure": {"height_cm": 180, "position": [-560, 520, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "-X", "edges": True, "margin": 1.1, "target": [0, 0, 120]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.18, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -22, "yaw_deg": 34, "arm_cm": 1700, "fov_deg": 50, "target": [0, 0, 110]},
             {"name": "pair_detail", "type": "persp", "pitch_deg": -10, "yaw_deg": 172, "arm_cm": 380, "fov_deg": 46, "target": [-SLAB_RING_R, 0, 140]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 4000, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_working_LOD1.obj"}], ortho + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [views[0], views[1]])
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
                      "height_cm": mm["height_cm"], "ring_cm": mm["ring_diameter_cm"],
                      "slab_over_ring": mm["slab_height_over_ring_diameter"], "pairs": mm["pair_count"],
                      "magenta_area": data["budgets"]["magenta_area_fraction_lod0"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
