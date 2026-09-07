#!/usr/bin/env python3
"""Deterministic source generator for EBS-HOL-BLD-004 — the Hollow Choir Phase Anchor.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-BLD-017.HC.ANCHOR.ASSET`, authored only
after the candidate was traced: a tapering hexagonal spire on a low stepped hexagonal plinth, with
magenta arris lines up its edges and a diamond register on its faces.

Usage:
  python3 build_phase_anchor.py --evidence-dir "<root>/EBS-HOL-BLD-004" [--skinned]
  python3 build_phase_anchor.py --evidence-dir "<root>/EBS-HOL-BLD-004" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the plinth's ground-contact centre.
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
PACKAGE_ID = "EBS-PKG-HC-PHASE-ANCHOR"
PRODUCTION_ID = "EBS-HOL-BLD-004"
ASSET = "SK_EBS_HOL_BLD_004"
PLANNED_FOLDER = "/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_004/"
REVISION = "ebs-hol-bld-004-concept-v1"
CARD = "REL-BLD-017.HC.ANCHOR.ASSET"

VITRIFIED = "MI_EBS_HOL_Vitrified"
GROUND = "MI_EBS_HOL_ApronStone"
MAGENTA = "MI_EBS_HOL_Magenta"

TICKS_PER_SECOND = 20.0
UPKEEP_TICKS = 600.0
AURA_CM = 700.0                        # gameplay data, projected as VFX; NOT geometry

TILE_CM = 200.0
FOOTPRINT_TILES = 2
F = TILE_CM * FOOTPRINT_TILES          # 400 cm square
HALF = F / 2.0

SIDES = 6
HEIGHT = 640.0                         # apex above ground
PLINTH_H = 112.0                       # traced plinth share of height: 0.176
PLINTH_STEPS = ((150.0, 0.0, 44.0), (124.0, 44.0, 82.0), (104.0, 82.0, PLINTH_H))
SHAFT_BASE_R = 56.0                    # traced slenderness: 2 * 56 / 640 = 0.175
SHAFT_TOP_R = 23.0                     # traced: the shaft narrows to 50/121 = 0.41 of its base
APEX_Z = HEIGHT
CAP_Z = 488.0                          # traced: the apex cap is 0.238 of the height
SEGMENTS = 4                           # frusta between the plinth top and the cap
ARRIS_W, ARRIS_T = 3.0, 2.2
REGISTER_LOW, REGISTER_HIGH = 168.0, 452.0
REGISTER_NODES = 5                     # diamonds up the register
REGISTER_R = 2.0
STATES = ("field_active", "field_lost", "destroyed")

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "plinth ground-contact centre; the Phase Anchor never moves"),
    ("spire", "root", (0.0, 0.0, PLINTH_H), "the tapering shaft: drifts on its own phase (faction reality-bleed device)"),
    ("apex", "spire", (0.0, 0.0, CAP_Z), "the apex cap: hangs out of phase with the shaft beneath it"),
]


HEX_PHASE = 0.0                        # vertices on +/-X, so a FACE faces -Y: the register lies on the face
                                       # the front orthographic camera looks at, not across an edge


def hexagon(radius: float, phase_deg: float = HEX_PHASE):
    return [(radius * math.cos(math.radians(phase_deg + 60.0 * i)),
             radius * math.sin(math.radians(phase_deg + 60.0 * i))) for i in range(SIDES)]


def apothem(radius: float) -> float:
    """Face-centre distance. The register sits on a face, at the apothem, not at the vertex radius."""
    return radius * math.cos(math.radians(30.0))


def shaft_radius(z: float) -> float:
    """Linear taper from the plinth top to the apex cap's base."""
    if z <= PLINTH_H:
        return SHAFT_BASE_R
    if z >= CAP_Z:
        return SHAFT_TOP_R
    f = (z - PLINTH_H) / (CAP_Z - PLINTH_H)
    return SHAFT_BASE_R + (SHAFT_TOP_R - SHAFT_BASE_R) * f


def _frustum(m, z0, z1, r0, r1, slot, component):
    """A hexagonal frustum. kit.prism is a constant-outline extrusion and cannot taper."""
    low = [(x, y, z0) for x, y in hexagon(r0)]
    high = [(x, y, z1) for x, y in hexagon(r1)]
    faces = []
    for i in range(SIDES):
        j = (i + 1) % SIDES
        faces.append([low[i], low[j], high[j], high[i]])
    faces.append(list(reversed(low)))
    faces.append(list(high))
    m.add_convex_solid(faces, slot, component)


def _arris_points(z: float):
    """The six shaft edges at height z, pushed just clear of the face."""
    r = shaft_radius(z) + ARRIS_T / 2.0
    return [(r * math.cos(math.radians(HEX_PHASE + 60.0 * i)),
             r * math.sin(math.radians(HEX_PHASE + 60.0 * i))) for i in range(SIDES)]


def register_z(index: int) -> float:
    return REGISTER_LOW + (REGISTER_HIGH - REGISTER_LOW) * index / (REGISTER_NODES - 1)


def build_body(lod: int, state: str = "field_active") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    vitrified = m.slot(VITRIFIED)
    ground = m.slot(GROUND)
    magenta = m.slot(MAGENTA)
    fine = lod == 0
    lit = state == "field_active"

    # 1. the stepped hexagonal plinth
    for k, (radius, z0, z1) in enumerate(PLINTH_STEPS, start=1):
        m.prism(hexagon(radius), z0, z1, ground, f"plinth_step_{k:02d}", cap_bottom=(k == 1))

    if state == "destroyed":
        # The spire SNAPS and falls in two sections. A 528 cm shaft toppled whole reaches 300 cm past
        # the 400 cm footprint; it cannot lie down inside its own placement envelope unbroken.
        stump_h = 96.0
        _frustum(m, PLINTH_H, PLINTH_H + stump_h, SHAFT_BASE_R,
                 shaft_radius(PLINTH_H + stump_h), vitrified, "stump")
        for k, (length, r0, r1, yaw, offset) in enumerate((
                (168.0, shaft_radius(PLINTH_H + stump_h), 40.0, 24.0, (-46.0, 30.0)),
                (146.0, 40.0, SHAFT_TOP_R, -58.0, (54.0, -44.0))), start=1):
            piece = kit.Mesh(f"piece_{k}")
            _frustum(piece, -length / 2.0, length / 2.0, r0, r1, piece.slot(VITRIFIED), f"fallen_section_{k:02d}")
            m.merge(piece, translate=(offset[0], offset[1], PLINTH_H + r0 * 0.9), pitch_deg=90.0, yaw_deg=yaw)
        m.collision.append(kit.CollisionBox("plinth", (0.0, 0.0, PLINTH_H / 2.0),
                                            (2 * PLINTH_STEPS[0][0], 2 * PLINTH_STEPS[0][0], PLINTH_H)))
        return m

    # 2. the tapering shaft, segment by segment
    for k in range(SEGMENTS):
        z0 = PLINTH_H + (CAP_Z - PLINTH_H) * k / SEGMENTS
        z1 = PLINTH_H + (CAP_Z - PLINTH_H) * (k + 1) / SEGMENTS
        _frustum(m, z0, z1, shaft_radius(z0), shaft_radius(z1), vitrified, f"shaft_{k + 1:02d}")

    # 3. the apex cap: the taper closing to a point
    apex = [(0.0, 0.0, APEX_Z)]
    ring = [(x, y, CAP_Z) for x, y in hexagon(SHAFT_TOP_R)]
    faces = [[ring[i], ring[(i + 1) % SIDES], apex[0]] for i in range(SIDES)]
    faces.append(list(reversed(ring)))
    m.add_convex_solid(faces, vitrified, "apex_cap")

    # 4. magenta arris lines up the six shaft edges
    if fine:
        for i in range(SIDES):
            low = _arris_points(PLINTH_H + 6.0)[i]
            high = _arris_points(CAP_Z)[i]
            m.tube((low[0], low[1], PLINTH_H + 6.0), (high[0], high[1], CAP_Z),
                   ARRIS_W / 2.0, 3, magenta if lit else vitrified, f"arris_{i + 1:02d}")
    else:
        # LOD1 keeps the arris read on the two silhouette edges only
        for i in (0, 3):
            low = _arris_points(PLINTH_H + 6.0)[i]
            high = _arris_points(CAP_Z)[i]
            m.tube((low[0], low[1], PLINTH_H + 6.0), (high[0], high[1], CAP_Z),
                   ARRIS_W / 2.0, 3, magenta if lit else vitrified, f"arris_{i + 1:02d}")

    # 5. the diamond register up the front face: a zig-zag between the face's two edges
    if fine:
        for n in range(REGISTER_NODES - 1):
            z0, z1 = register_z(n), register_z(n + 1)
            for lean in (1.0, -1.0):
                # the register lies on the -Y face, proud of it by REGISTER_R, and zig-zags across it
                y0 = -(apothem(shaft_radius(z0)) + REGISTER_R)
                y1 = -(apothem(shaft_radius(z1)) + REGISTER_R)
                x0 = lean * shaft_radius(z0) * 0.40
                x1 = -lean * shaft_radius(z1) * 0.40
                m.tube((x0, y0, z0), (x1, y1, z1), REGISTER_R, 3,
                       magenta if lit else vitrified, f"register_{n + 1:02d}_{'a' if lean > 0 else 'b'}")

    m.collision.append(kit.CollisionBox("plinth", (0.0, 0.0, PLINTH_H / 2.0),
                                        (2 * PLINTH_STEPS[0][0], 2 * PLINTH_STEPS[0][0], PLINTH_H)))
    m.collision.append(kit.CollisionBox("shaft", (0.0, 0.0, (PLINTH_H + CAP_Z) / 2.0),
                                        (2 * SHAFT_BASE_R, 2 * SHAFT_BASE_R, CAP_Z - PLINTH_H)))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith("apex_"):
            by_component[c] = "apex"
        elif c.startswith("shaft_") or c.startswith("arris_") or c.startswith("register_"):
            by_component[c] = "spire"
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Target_Anchor_Center": ("root", (0.0, 0.0, PLINTH_H + (CAP_Z - PLINTH_H) * 0.4), 0.0,
                             "targeting and selection anchor, low on the shaft where the mass is"),
    "Field_Ring_Origin": ("root", (0.0, 0.0, 2.0), 0.0,
                          "origin of the 700 cm aura ring. It is a LIGHT EFFECT, not geometry; the radius "
                          "belongs to gameplay data and the mesh implies none"),
    "Apex_Beacon": ("apex", (0.0, 0.0, APEX_Z - 12.0), 0.0, "the tip: the long-range read that the field is up"),
    "Register_Center": ("spire", (0.0, -(apothem(shaft_radius((REGISTER_LOW + REGISTER_HIGH) / 2.0)) + 6.0),
                                  (REGISTER_LOW + REGISTER_HIGH) / 2.0), -90.0,
                        "centre of the diamond register on the front face"),
}


def assemble(lod: int, state: str = "field_active"):
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

    idle = new("field_active_idle", 140, True,
               "field up: the shaft holds and its apex drifts out of phase with it. Faction reality-bleed "
               "device, NOT a canon motion clause for this building")
    for index, bone in enumerate(("spire", "apex")):
        for step in range(5):
            t = step / 4.0 * idle.duration_s
            phase = index / 2.0
            drift = math.sin(2.0 * math.pi * (step / 4.0 + phase))
            idle.key(bone, t, (0.0, 0.28 * drift, 0.0), translation_cm=(0.0, 0.0, 0.0))

    lost = new("field_lost", 140, True, "field down: the shaft settles and the drift dies out of the apex")
    for bone in ("spire", "apex"):
        for t in (0.0, lost.duration_s):
            lost.key(bone, t, (0.0, 0.0, 0.0))

    restore = new("restore", 0, False, "single-frame rest pose for reconstruction from saved state")
    for bone in ("spire", "apex"):
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


def posed(lod: int, pose: dict, state: str = "field_active") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("field_active_idle", 0.25, "field_active"), ("field_lost", 0.5, "field_lost")]


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


def lit_lines(m: kit.Mesh) -> int:
    return len({p.component for p in m.polygons
                if (p.component.startswith("arris_") or p.component.startswith("register_"))
                and m.slots[p.slot] == MAGENTA})


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "plinth": {"contract": len(PLINTH_STEPS), "built": sum(1 for c in comps if c.startswith("plinth_step_"))},
        "shaft": {"contract": "a tapering hexagonal spire", "built": sum(1 for c in comps if c.startswith("shaft_")),
                  "base_radius_cm": SHAFT_BASE_R, "top_radius_cm": SHAFT_TOP_R},
        "apex_cap": {"contract": 1, "built": "apex_cap" in comps},
        "arris_lines": {"contract": SIDES, "built": sum(1 for c in comps if c.startswith("arris_"))},
        "register": {"contract": (REGISTER_NODES - 1) * 2, "built": sum(1 for c in comps if c.startswith("register_"))},
        "lit_lines": lit_lines(m),
        "aura_field": {"contract": "a LIGHT EFFECT, not geometry", "radius_cm": AURA_CM,
                       "built_as_geometry": any("ring" in c or "aura" in c or "field" in c for c in comps),
                       "socket": "Field_Ring_Origin"},
        "bones": {"contract": "root, the spire and its apex", "built": len(s.bones),
                  "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["field_active_idle", "field_lost", "restore"], "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "plinth_height_cm": PLINTH_H,
        "plinth_fraction_of_height": round(PLINTH_H / HEIGHT, 4),
        "traced_plinth_fraction": 0.176,
        "slenderness": round(2 * SHAFT_BASE_R / HEIGHT, 4),
        "traced_slenderness": 0.177,
        "plinth_across_corners_cm": round(2 * PLINTH_STEPS[0][0], 2),
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
        m, s, _c, socks = assemble(lod, "field_active")
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
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (field_active)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count(), "lit_lines": lit_lines(m)})
    m1 = assemble(1, "field_active")[0]
    path = os.path.join(review, f"{ASSET}_field_active_LOD1.obj")
    review_rows.append({"name": "field_active_lod1", "path": os.path.relpath(path, evidence_dir),
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
    m0, s, counts, _socks = assemble(0, "field_active")
    m1 = assemble(1, "field_active")[0]
    lost = assemble(0, "field_lost")[0]
    clips = exported["clips"]
    magenta = slot_area_fraction(m0, MAGENTA)
    worst = max(m0.triangle_count(), lost.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "plinth ground-contact centre", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-017.HC.ANCHOR: coherence optimizer, 480 HP, 800 cm sight, 130 ticks, 2x2 footprint",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "aura_cm": AURA_CM,
                        "aura_is_not_geometry": "The 700 cm field is gameplay data projected as VFX from Field_Ring_Origin.",
                        "upkeep": f"{UPKEEP_TICKS:.0f} ticks = {UPKEEP_TICKS / TICKS_PER_SECOND:.0f} s between charges",
                        "source": "Content/Data/Source/buildings.json hc_phase_anchor"},
        "material_slots": [VITRIFIED, GROUND, MAGENTA],
        "material_slot_policy": ("Three slots: vitrified stone for the spire and apex, a worn stone for the plinth steps, "
                                 "and Magenta Fracture for the arris lines and the diamond register."),
        "traced_before_the_card": {
            "directive": "Owner ruling 2026-09-07: trace each selected concept BEFORE authoring its provisional card.",
            "trace": ("the candidate's FIELD ACTIVE view, dark-pixel spans below luminance 120: apex to plinth base 682 px, "
                      "plinth 120 px (0.176 of height), shaft base 121 px (slenderness 0.177)"),
            "measurements_rejected_as_perspective": ["the plinth's ~480 px apparent width", "the field ring's 878 x 276 px ellipse"],
            "card_written_after": True},
        "footprint_adaptation": {
            "what": "The plinth is held to 300 cm across corners.",
            "why": ("The traced plinth-to-shaft proportion would put it near 500 cm across, outside the 400 cm footprint."),
            "authority": ("Owner ruling 2026-09-07 (Array Foundry): adapt the proportion to the footprint; do not change "
                          "navigation or placement rules to match a drawing's apparent proportion."),
            "recorded_as": "a footprint-driven adaptation, not a concept deviation to be silently absorbed"},
        "faction_language_inference": {
            "what": "the 12% Magenta Fracture ceiling",
            "source": "the four REL-FAC-027.HC.* UNIT cards",
            "status": "INFERENCE, not a requirement. No authoritative card places it on a Hollow Choir BUILDING."},
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/hollow-choir-asset-cards.json (rendered to hollow-choir-asset-cards.md)",
            "status": "PROVISIONAL. Authored in this worktree after the trace; NOT in Docs/Requirements.md.",
            "bounds": {"lod0_triangles": 4000, "lod1_triangles": 1600}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "field_lost_state_triangles": lost.triangle_count(), "worst_state_triangles": worst,
                    "lod0_cap": 4000, "lod1_cap": 1600, "cap_source": f"{CARD} (PROVISIONAL)",
                    "lod0_within_cap": worst <= 4000, "lod1_within_cap": m1.triangle_count() <= 1600,
                    "headroom_note": "Under the ceiling is HEADROOM, not sufficiency (owner ruling 2026-09-07).",
                    "magenta_area_fraction_field_active": round(magenta, 5), "magenta_cap": 0.12,
                    "magenta_within_cap": magenta <= 0.12},
        "state_read": {"field_active_lit_lines": lit_lines(m0), "field_lost_lit_lines": lit_lines(lost),
                       "mechanism": ("field_lost moves every arris and register line into the vitrified slot, so the lit "
                                     "read is a slot change and not a brightness parameter. The register is the geometry "
                                     "that carries it; LOD1 keeps the two silhouette arrises so the read survives.")},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("Root, the spire and its apex. The drift is the faction's reality-bleed device, NOT a canon "
                           "motion clause for this building. No root motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {"field_active": "arrises and register lit; the aura ring is projected from Field_Ring_Origin",
                   "field_lost": "arrises and register dark; the spire reads as inert stone",
                   "destroyed": "the spire toppled across its own plinth"},
        "pending_requirements": [
            {"requirement": "Articulated components for a structure of this role", "status": "PENDING, not waived",
             "detail": "The spire and apex drift is a presentation device, not a role-required articulated assembly."}],
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "animation": "NOT_EVALUATED",
                       "technical": "PENDING — built against a provisional card that is not in the authoritative requirements",
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/phase-anchor-review/phase-anchor-candidate.png",
                            "canon": "Requirements SPEC-BLD-017.HC.ANCHOR",
                            "gameplay": "Content/Data/Source/buildings.json hc_phase_anchor"},
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
            "reference_figure": {"height_cm": 180, "position": [-300, 260, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "-Y", "edges": True, "margin": 1.1, "target": [0, 0, 320]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.18, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -14, "yaw_deg": 62, "arm_cm": 1500, "fov_deg": 50, "target": [0, 0, 300]},
             {"name": "register_detail", "type": "persp", "pitch_deg": -4, "yaw_deg": 90, "arm_cm": 620, "fov_deg": 42, "target": [0, 0, 310]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3400, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_field_active_LOD1.obj"}], ortho + [views[0], views[2]])
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
                      "height_cm": mm["height_cm"], "slenderness": mm["slenderness"],
                      "traced_slenderness": mm["traced_slenderness"],
                      "plinth_fraction": mm["plinth_fraction_of_height"],
                      "magenta_area": data["budgets"]["magenta_area_fraction_field_active"],
                      "state_read": {k: v for k, v in data["state_read"].items() if k != "mechanism"},
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
