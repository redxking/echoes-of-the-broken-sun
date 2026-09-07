#!/usr/bin/env python3
"""Deterministic source generator for EBS-HOL-BLD-002 — the Hollow Choir Interval Loom.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-BLD-017.HC.INTERVAL.ASSET`, authored
only after the candidate was traced: two flat ribbon arches crossing diagonally over a bare 2x2
footprint, four foot plates, a ground drop-off pad, and magenta fracture edge lines along both borders
of each arch. The three power states are the asset's whole job — supplied, ticking, insolvent.

Usage:
  python3 build_interval_loom.py --evidence-dir "<root>/EBS-HOL-BLD-002" [--skinned]
  python3 build_interval_loom.py --evidence-dir "<root>/EBS-HOL-BLD-002" --check

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
PACKAGE_ID = "EBS-PKG-HC-INTERVAL-LOOM"
PRODUCTION_ID = "EBS-HOL-BLD-002"
ASSET = "SK_EBS_HOL_BLD_002"
PLANNED_FOLDER = "/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_002/"
REVISION = "ebs-hol-bld-002-concept-v1"
CARD = "REL-BLD-017.HC.INTERVAL.ASSET"

VITRIFIED = "MI_EBS_HOL_Vitrified"
GROUND = "MI_EBS_HOL_ApronStone"
MAGENTA = "MI_EBS_HOL_Magenta"

TICKS_PER_SECOND = 20.0
UPKEEP_TICKS = 600.0                   # canon: charges 5 Dawn every 600 ticks

TILE_CM = 200.0
FOOTPRINT_TILES = 2
F = TILE_CM * FOOTPRINT_TILES          # 400 cm square
HALF = F / 2.0

FOOT_R = 130.0                         # the arch feet sit near the corners of the square
ARCH_APEX_Z = 164.0                    # rise about 0.6 of the 260 cm span: the candidate's arches are
                                       # broad and low, not pointed
ARCH_SEGMENTS = 9
RIBBON_W, RIBBON_T = 30.0, 12.0
EDGE_W, EDGE_T = 1.6, 1.2              # fine lines. At 3 x 13 they were 27% of the surface area,
                                       # against the 12% Magenta Fracture ceiling
FOOT_PLATE = (58.0, 46.0, 8.0)
PAD_CENTRE = (-96.0, 0.0)
PAD = (150.0, 150.0, 7.0)
STATES = ("supplied", "upkeep_tick", "insolvent", "destroyed")

# the two arches cross diagonally: A runs corner to corner, B on the other diagonal
ARCHES = (("a", 45.0), ("b", 135.0))

BONES = [("root", None, (0.0, 0.0, 0.0), "ground-contact centre between the four feet; the Loom never moves")]
for _tag, _yaw in ARCHES:
    BONES.append((f"arch_{_tag}", "root", (0.0, 0.0, ARCH_APEX_Z * 0.5),
                  f"arch {_tag}: lets the two arches drift independently. Faction reality-bleed device, "
                  f"NOT a canon motion clause for this building"))
ARCH_BONES = tuple(b[0] for b in BONES[1:])


def _yaw_point(radius: float, yaw_deg: float):
    a = math.radians(yaw_deg)
    return (radius * math.cos(a), radius * math.sin(a))


def arch_point(tag_yaw: float, t: float):
    """A point along one arch, t from 0 at one foot to 1 at the other.

    The span runs along the diagonal at ``tag_yaw``; the rise is a half sine, which gives the
    near-semicircular profile the candidate draws without a curve primitive.
    """
    along = (t - 0.5) * 2.0 * FOOT_R
    x, y = _yaw_point(along, tag_yaw)
    # the arch springs from the top of its foot plate, not from z = 0: a ribbon has thickness,
    # and starting at the ground put its underside 2.4 cm below the floor
    z = FOOT_PLATE[2] + (ARCH_APEX_Z - FOOT_PLATE[2]) * math.sin(math.pi * t)
    return x, y, z



def _ribbon(m: kit.Mesh, p0, p1, side, width: float, thickness: float, slot: int, component: str):
    """A flat ribbon segment between two points, oriented by ``side``.

    kit.Mesh.box has no pitch, so an arch built from boxes would lie flat. This lays the eight corners
    out directly from the segment's own axis, side and up vectors.
    """
    axis = kit.v_norm(kit.v_sub(p1, p0))
    right = kit.v_norm((side[0], side[1], 0.0))
    up = kit.v_norm(kit.v_cross(axis, right))
    hw, ht = width / 2.0, thickness / 2.0

    def corner(p, s, u):
        return kit.v_add(p, kit.v_add(kit.v_mul(right, s * hw), kit.v_mul(up, u * ht)))

    a = [corner(p0, -1, -1), corner(p0, 1, -1), corner(p0, 1, 1), corner(p0, -1, 1)]
    b = [corner(p1, -1, -1), corner(p1, 1, -1), corner(p1, 1, 1), corner(p1, -1, 1)]
    faces = [[a[0], a[1], a[2], a[3]], [b[3], b[2], b[1], b[0]]]
    for k in range(4):
        j = (k + 1) % 4
        faces.append([a[k], b[k], b[j], a[j]])
    m.add_convex_solid(faces, slot, component)


def build_body(lod: int, state: str = "supplied") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    vitrified = m.slot(VITRIFIED)
    ground = m.slot(GROUND)
    magenta = m.slot(MAGENTA)
    fine = lod == 0
    segments = ARCH_SEGMENTS if fine else 5
    ruined = state == "destroyed"
    lit = state in ("supplied", "upkeep_tick")

    # 1. the drop-off pad and its delivered matter
    m.box((PAD_CENTRE[0], PAD_CENTRE[1], PAD[2] / 2.0), PAD, ground, "dropoff_pad")
    if fine and not ruined:
        for k, (dx, dy, size) in enumerate(((-38.0, -30.0, 16.0), (-10.0, 22.0, 12.0),
                                            (26.0, -14.0, 22.0), (44.0, 30.0, 14.0)), start=1):
            m.box((PAD_CENTRE[0] + dx, PAD_CENTRE[1] + dy, PAD[2] + size / 2.0),
                  (size, size, size), vitrified, f"delivered_matter_{k:02d}", yaw_deg=12.0 * k)

    for tag, yaw in ARCHES:
        # 2. the foot plates at both ends of every arch
        for end, t in (("0", 0.0), ("1", 1.0)):
            fx, fy, _ = arch_point(yaw, t)
            m.box((fx, fy, FOOT_PLATE[2] / 2.0), FOOT_PLATE, vitrified, f"arch_{tag}_foot_{end}", yaw_deg=yaw)
        if ruined:
            # the arches come down; the feet and the pad stay
            for k in range(3):
                fx, fy, _ = arch_point(yaw, 0.2 + 0.3 * k)
                m.box((fx, fy, 11.0), (96.0, RIBBON_W, 14.0), vitrified,
                      f"fallen_{tag}_{k + 1:02d}", yaw_deg=yaw + 18.0 * (k - 1))
            continue

        # 3. the ribbon arch itself, and its two magenta edge lines
        a = math.radians(yaw)
        side = (-math.sin(a), math.cos(a))
        for i in range(segments):
            p0 = arch_point(yaw, i / segments)
            p1 = arch_point(yaw, (i + 1) / segments)
            _ribbon(m, p0, p1, side, RIBBON_W, RIBBON_T, vitrified, f"arch_{tag}_span_{i + 1:02d}")
            for sign, edge in ((-1.0, "l"), (1.0, "r")):
                lateral = sign * (RIBBON_W / 2.0 - EDGE_W / 2.0)
                q0 = (p0[0] + side[0] * lateral, p0[1] + side[1] * lateral, p0[2])
                q1 = (p1[0] + side[0] * lateral, p1[1] + side[1] * lateral, p1[2])
                _ribbon(m, q0, q1, side, EDGE_W, EDGE_T,
                        magenta if lit else vitrified, f"arch_{tag}_edge_{edge}{i + 1:02d}")
        if state == "upkeep_tick" and fine:
            # the charge falling due: a surge band across the crown, in the magenta slot. Brightness is
            # a material parameter, so the STATE carries a band the other states do not have.
            cx, cy, cz = arch_point(yaw, 0.5)
            p0 = arch_point(yaw, 0.44)
            p1 = arch_point(yaw, 0.56)
            _ribbon(m, (p0[0], p0[1], p0[2] + RIBBON_T / 2.0 + 3.0),
                    (p1[0], p1[1], p1[2] + RIBBON_T / 2.0 + 3.0), side,
                    RIBBON_W * 0.5, 2.0, magenta, f"arch_{tag}_surge")
            del cx, cy, cz

    m.collision.append(kit.CollisionBox("feet", (0.0, 0.0, FOOT_PLATE[2] / 2.0),
                                        (2 * FOOT_R, 2 * FOOT_R, FOOT_PLATE[2])))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith("arch_") and "_foot_" not in c:
            by_component[c] = f"arch_{c.split('_')[1]}"
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Target_Anchor_Center": ("root", (0.0, 0.0, ARCH_APEX_Z * 0.55), 0.0, "targeting and selection anchor under the crossing"),
    "Matter_Dropoff": ("root", (PAD_CENTRE[0], PAD_CENTRE[1], PAD[2]), 180.0, "the drop-off pad: where workers deliver Matter"),
    "Arch_Crossing_Center": ("root", (0.0, 0.0, ARCH_APEX_Z), 0.0, "where the two arches cross"),
    "Upkeep_Signal": ("root", (0.0, 0.0, ARCH_APEX_Z + 24.0), 0.0,
                      "where the 600-tick coherence charge is signalled; the surge itself is a material effect"),
}


def assemble(lod: int, state: str = "supplied"):
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

    idle = new("supplied_idle", 120, True,
               "supplied: the two arches drift a little out of step with each other. Faction "
               "reality-bleed device, NOT a canon motion clause for this building")
    for index, bone in enumerate(ARCH_BONES):
        phase = index / len(ARCH_BONES)
        for step in range(5):
            t = step / 4.0 * idle.duration_s
            drift = math.sin(2.0 * math.pi * (step / 4.0 + phase))
            idle.key(bone, t, (0.0, 0.0, 0.7 * drift), translation_cm=(0.0, 0.0, 0.9 * drift))

    tick = new("upkeep", 30, False,
               "the 600-tick coherence charge falling due: both arches pull taut together, which is the "
               "one moment they agree")
    for bone in ARCH_BONES:
        for t, roll in ((0.0, 0.0), (0.4, -1.6), (1.5, 0.0)):
            tick.key(bone, t, (0.0, 0.0, roll), translation_cm=(0.0, 0.0, roll * 1.4))

    restore = new("restore", 0, False, "single-frame rest pose for reconstruction from saved state")
    for bone in ARCH_BONES:
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


def posed(lod: int, pose: dict, state: str = "supplied") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("supplied_idle", 0.25, "supplied"), ("upkeep", 0.27, "upkeep_tick")]


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


def lit_edges(m: kit.Mesh) -> int:
    return len({p.component for p in m.polygons
                if "_edge_" in p.component and m.slots[p.slot] == MAGENTA})


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "arches": {"contract": len(ARCHES),
                   "built": len({c.split("_")[1] for c in comps if c.startswith("arch_") and "_span_" in c}),
                   "spans_each": sum(1 for c in comps if "_span_" in c) // max(1, len(ARCHES))},
        "feet": {"contract": 2 * len(ARCHES), "built": sum(1 for c in comps if "_foot_" in c)},
        "dropoff_pad": {"contract": 1, "built": "dropoff_pad" in comps,
                        "delivered_matter": sum(1 for c in comps if c.startswith("delivered_matter_"))},
        "edge_lines": {"contract": "two per arch, running its full length",
                       "built": sum(1 for c in comps if "_edge_" in c),
                       "lit": lit_edges(m)},
        "surge_band": {"contract": "present only in upkeep_tick",
                       "built": sum(1 for c in comps if c.endswith("_surge"))},
        "bones": {"contract": f"root plus one bone per arch ({len(ARCHES)})", "built": len(s.bones),
                  "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["supplied_idle", "upkeep", "restore"], "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "arch_span_cm": round(2 * FOOT_R, 2),
        "height_over_span": round(z1 / (2 * FOOT_R), 4),
        "arch_count": len(ARCHES),
        "everything_inside_the_footprint": max(abs(x0), abs(x1), abs(y0), abs(y1)) <= HALF + 1e-6,
        "max_extent_cm": round(max(abs(x0), abs(x1), abs(y0), abs(y1)), 2),
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
        m, s, _c, socks = assemble(lod, "supplied")
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
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (supplied)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count(), "lit_edges": lit_edges(m)})
    m1 = assemble(1, "supplied")[0]
    path = os.path.join(review, f"{ASSET}_supplied_LOD1.obj")
    review_rows.append({"name": "supplied_lod1", "path": os.path.relpath(path, evidence_dir),
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
    m0, s, counts, _socks = assemble(0, "supplied")
    m1 = assemble(1, "supplied")[0]
    tick = assemble(0, "upkeep_tick")[0]
    insolvent = assemble(0, "insolvent")[0]
    clips = exported["clips"]
    magenta = slot_area_fraction(m0, MAGENTA)
    worst = max(m0.triangle_count(), tick.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "ground-contact centre between the four feet", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-017.HC.INTERVAL: a supply node carrying a recurring coherence obligation",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "upkeep": f"{UPKEEP_TICKS:.0f} ticks = {UPKEEP_TICKS / TICKS_PER_SECOND:.0f} s between charges",
                        "source": "Content/Data/Source/buildings.json hc_interval_loom"},
        "material_slots": [VITRIFIED, GROUND, MAGENTA],
        "material_slot_policy": ("Three slots: vitrified stone for the arches and feet, a worn stone for the drop-off pad, and "
                                 "Magenta Fracture for the arch edge lines, which are the only emissive."),
        "traced_before_the_card": {
            "directive": "Owner ruling 2026-09-07: trace each selected concept BEFORE authoring its provisional card.",
            "trace": "the candidate's SUPPLIED view, dark-pixel row spans; 849 x 550 px, height over width 0.648, two crossed arches on four feet with a separate ground pad",
            "card_written_after": True},
        "faction_language_inference": {
            "what": "the 12% Magenta Fracture ceiling",
            "source": "the four REL-FAC-027.HC.* UNIT cards",
            "status": "INFERENCE, not a requirement. No authoritative card places it on a Hollow Choir BUILDING."},
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/hollow-choir-asset-cards.json (rendered to hollow-choir-asset-cards.md)",
            "status": ("PROVISIONAL. Authored in this worktree after the trace; NOT in Docs/Requirements.md. The four "
                       "REL-FAC-027.HC.* cards cover UNITS, and REL-FAC-027.HC.INTERVALIST is the Intervalist unit, NOT this "
                       "building, despite the shared word."),
            "bounds": {"lod0_triangles": 3000, "lod1_triangles": 1200}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "upkeep_state_triangles": tick.triangle_count(), "worst_state_triangles": worst,
                    "lod0_cap": 3000, "lod1_cap": 1200, "cap_source": f"{CARD} (PROVISIONAL)",
                    "lod0_within_cap": worst <= 3000, "lod1_within_cap": m1.triangle_count() <= 1200,
                    "headroom_note": "Under the ceiling is HEADROOM, not sufficiency (owner ruling 2026-09-07).",
                    "magenta_area_fraction_supplied": round(magenta, 5), "magenta_cap": 0.12,
                    "magenta_within_cap": magenta <= 0.12},
        "solvency_read": {"supplied": lit_edges(m0), "upkeep_tick": lit_edges(tick),
                          "insolvent": lit_edges(insolvent),
                          "mechanism": ("the edge lines move between the magenta and vitrified slots, and the tick adds a "
                                        "surge band the other states do not carry. Brightness alone is a material parameter, "
                                        "so the states differ in GEOMETRY as well as in material.")},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("Root plus one bone per arch so the two drift independently. That drift is the faction's "
                           "reality-bleed device, NOT a canon motion clause for this building. No root motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {"supplied": "edge lines lit and steady",
                   "upkeep_tick": "a surge band across both crowns as the 600-tick charge falls due",
                   "insolvent": "every edge line dark; the frame reads as unpowered",
                   "destroyed": "the arches down; the feet and the pad remain"},
        "pending_requirements": [
            {"requirement": "Articulated components for a structure of this role", "status": "PENDING, not waived",
             "detail": "The arch drift is a presentation device, not a role-required articulated assembly."}],
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "animation": "NOT_EVALUATED",
                       "technical": "PENDING — built against a provisional card that is not in the authoritative requirements",
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/interval-loom-review/interval-loom-candidate.png",
                            "canon": "Requirements SPEC-BLD-017.HC.INTERVAL",
                            "gameplay": "Content/Data/Source/buildings.json hc_interval_loom"},
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
            "reference_figure": {"height_cm": 180, "position": [-320, 300, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "-X", "edges": True, "margin": 1.1, "target": [0, 0, 110]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.18, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -20, "yaw_deg": 34, "arm_cm": 760, "fov_deg": 50, "target": [0, 0, 100]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 1900, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_supplied_LOD1.obj"}], ortho + views)
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [views[0]])
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
                      "height_cm": mm["height_cm"], "span_cm": mm["arch_span_cm"],
                      "height_over_span": mm["height_over_span"],
                      "magenta_area": data["budgets"]["magenta_area_fraction_supplied"],
                      "lit_edges": data["solvency_read"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
