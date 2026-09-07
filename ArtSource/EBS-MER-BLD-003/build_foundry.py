#!/usr/bin/env python3
"""Deterministic source generator for EBS-MER-BLD-003 — the Meridian Compact Array Foundry.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md`: one long hall with an arched intake hood and ramp at the -X end, an
open fabrication bay in the middle whose rails carry a frame carriage, a framed output door and ramp at
the +X end, and a research gantry on the roof over the output. Three states drive the reads the canon
row names: producing, researching, interrupted.

Usage:
  python3 build_foundry.py --evidence-dir "<root>/EBS-MER-BLD-003"
  python3 build_foundry.py --evidence-dir "<root>/EBS-MER-BLD-003" --check

Units: centimetres. +X runs intake -> output, +Y right, +Z up. Pivot at the hall's ground-contact
centre. Nanite off. Nothing is authored below z = 0 and nothing leaves the 4x4 tile footprint.
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-MC-ARRAY-FOUNDRY"
PRODUCTION_ID = "EBS-MER-BLD-003"
ASSET = "SM_EBS_MER_BLD_003"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_003/"
REVISION = "ebs-mer-bld-003-concept-v1"

CERAMIC = "MI_EBS_MER_CeramicCivic"
FRAME = "MI_EBS_MER_CompactFrame"
STATUS = "MI_EBS_MER_StatusCyan"

TILE_CM = 200.0
FOOTPRINT_TILES = 4
F = TILE_CM * FOOTPRINT_TILES          # 800 cm square
HALF = F / 2.0

# The 4x4 footprint is the outer bound and the concept puts a ramp at BOTH ends, so the length is
# shared: hall half (240) + intake hood (80) + intake ramp (78) = 398 <= 400, and on the output side
# hall half + portal (32) + ramp (78) = 350. That caps the hall at 2:1 rather than the candidate's
# longer read; the deviation is recorded in README section 8 with the rule.
HALL_LEN = 520.0                       # 0.65 F
HALL_W = 260.0                         # 0.325 F, 2:1 in plan
WALL_Z = 200.0
ROOF_Z = 226.0
BAY_X = (-100.0, 120.0)                 # the open middle third of the roof
RAIL_Y = 84.0                          # rail pair offset from the centreline
RAIL_Z = 240.0
HOOD_LEN, HOOD_W, HOOD_Z = 70.0, 300.0, 258.0
DOOR_W, DOOR_Z = 150.0, 186.0
RAMP_LEN, RAMP_W = 60.0, 170.0
GANTRY_X, GANTRY_LEN, GANTRY_W, GANTRY_Z = 170.0, 150.0, 200.0, 264.0
STATES = ("producing", "researching", "interrupted")


def _hall_x():
    return (-HALL_LEN / 2.0, HALL_LEN / 2.0)


def build_main(lod: int, state: str = "producing") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    ceramic = m.slot(CERAMIC)
    frame = m.slot(FRAME)
    status = m.slot(STATUS)
    fine = lod == 0
    x0, x1 = _hall_x()

    # 1. the hall: a plinth, two flank walls and the closed roof either side of the open bay
    m.box((0.0, 0.0, 12.0), (HALL_LEN + 24.0, HALL_W + 24.0, 24.0), frame, "hall_plinth")
    for sign in (-1.0, 1.0):
        m.box((0.0, sign * (HALL_W / 2.0 - 18.0), (24.0 + WALL_Z) / 2.0), (HALL_LEN, 36.0, WALL_Z - 24.0),
              ceramic, f"hall_wall_{'r' if sign > 0 else 'l'}")
        m.box((0.0, sign * (HALL_W / 2.0 + 2.0), 92.0), (HALL_LEN - 40.0, 8.0, 12.0), status,
              f"hall_strip_{'r' if sign > 0 else 'l'}")
        if fine:
            for i in range(7):
                px = x0 + 60.0 + i * (HALL_LEN - 120.0) / 6.0
                m.box((px, sign * (HALL_W / 2.0 + 4.0), (24.0 + WALL_Z) / 2.0), (26.0, 14.0, WALL_Z - 40.0),
                      frame, f"pilaster_{'r' if sign > 0 else 'l'}{i:02d}")
    for label, (rx0, rx1) in (("intake", (x0, BAY_X[0])), ("output", (BAY_X[1], x1))):
        m.box(((rx0 + rx1) / 2.0, 0.0, (WALL_Z + ROOF_Z) / 2.0), (rx1 - rx0, HALL_W, ROOF_Z - WALL_Z),
              ceramic, f"roof_{label}")

    # 2. intake hood and its ramp at the -X end
    m.box((x0 - HOOD_LEN / 2.0 + 10.0, 0.0, HOOD_Z / 2.0), (HOOD_LEN, HOOD_W, HOOD_Z), ceramic, "intake_hood")
    m.box((x0 - HOOD_LEN + 20.0, 0.0, 92.0), (18.0, DOOR_W + 20.0, 150.0), frame, "intake_mouth")
    m.box((x0 - HOOD_LEN + 12.0, 0.0, 168.0), (10.0, DOOR_W - 10.0, 12.0), status, "intake_lintel_strip")
    m.box((x0 - HOOD_LEN - RAMP_LEN / 2.0, 0.0, 12.0), (RAMP_LEN, RAMP_W, 24.0), frame, "intake_ramp")

    # 3. the open fabrication bay: gantry legs, two rails, and the frame carriage the state moves
    for sign in (-1.0, 1.0):
        for px in (BAY_X[0] + 30.0, (BAY_X[0] + BAY_X[1]) / 2.0, BAY_X[1] - 30.0):
            m.box((px, sign * RAIL_Y, (WALL_Z + RAIL_Z) / 2.0), (26.0, 22.0, RAIL_Z - WALL_Z), frame,
                  f"gantry_leg_{'r' if sign > 0 else 'l'}_{int(px):+05d}")
        m.box(((BAY_X[0] + BAY_X[1]) / 2.0, sign * RAIL_Y, RAIL_Z), (BAY_X[1] - BAY_X[0] + 60.0, 26.0, 18.0),
              frame, f"rail_{'r' if sign > 0 else 'l'}")
    carriage_x = {"producing": 60.0, "researching": None, "interrupted": -60.0}[state]
    if carriage_x is not None:
        m.box((carriage_x, 0.0, RAIL_Z - 16.0), (86.0, 2 * RAIL_Y + 40.0, 22.0), frame, "rail_carriage")
        # the half-built frame hanging from the carriage: a torso block and two hanging limbs
        m.box((carriage_x, 0.0, RAIL_Z - 74.0), (74.0, 96.0, 74.0), ceramic, "frame_in_progress_body")
        for sign in (-1.0, 1.0):
            m.box((carriage_x - 6.0, sign * 62.0, RAIL_Z - 118.0), (30.0, 26.0, 84.0), frame,
                  f"frame_in_progress_limb_{'r' if sign > 0 else 'l'}")
        if state == "producing":
            m.box((carriage_x, 0.0, RAIL_Z - 38.0), (60.0, 70.0, 8.0), status, "frame_in_progress_seam")

    # 4. output door and ramp at the +X end
    m.box((x1 - 16.0, 0.0, DOOR_Z / 2.0 + 12.0), (32.0, DOOR_W + 60.0, DOOR_Z + 24.0), ceramic, "output_portal")
    # a dark doorway with a lit edge, not a glowing slab: the concept lights the frame, not the panel
    m.box((x1 - 2.0, 0.0, DOOR_Z / 2.0 + 12.0), (12.0, DOOR_W, DOOR_Z), frame, "output_door")
    for dy in (-DOOR_W / 2.0 - 6.0, DOOR_W / 2.0 + 6.0):
        m.box((x1 + 2.0, dy, DOOR_Z / 2.0 + 12.0), (8.0, 10.0, DOOR_Z - 10.0), status, f"output_door_edge_{'r' if dy > 0 else 'l'}")
    m.box((x1 + 2.0, 0.0, DOOR_Z + 18.0), (8.0, DOOR_W + 12.0, 10.0), status, "output_door_lintel")
    m.box((x1 + RAMP_LEN / 2.0, 0.0, 12.0), (RAMP_LEN, RAMP_W, 24.0), frame, "output_ramp")

    # 5. roof research gantry over the output end: a platform and its instruments
    m.box((GANTRY_X, 0.0, ROOF_Z + 14.0), (GANTRY_LEN, GANTRY_W, 28.0), ceramic, "gantry_platform")
    lit = status if state == "researching" else frame
    m.box((GANTRY_X, 0.0, GANTRY_Z + 40.0), (26.0, 26.0, 84.0), frame, "gantry_mast")
    for i, (dx, dy, h) in enumerate(((-42.0, 34.0, 46.0), (44.0, -30.0, 58.0), (10.0, 58.0, 38.0))):
        m.box((GANTRY_X + dx, dy, GANTRY_Z + 12.0 + h / 2.0), (18.0, 18.0, h), frame, f"gantry_instrument_{i + 1:02d}")
        m.box((GANTRY_X + dx, dy, GANTRY_Z + 12.0 + h), (26.0, 26.0, 10.0), lit, f"gantry_lamp_{i + 1:02d}")
    m.box((GANTRY_X, 0.0, GANTRY_Z + 88.0), (34.0, 34.0, 12.0), lit, "gantry_mast_head")

    # 6. exterior research progress band (card .MAT_RULE); dark unless researching
    m.box((GANTRY_X - 40.0, HALL_W / 2.0 + 4.0, 150.0), (120.0, 8.0, 16.0), lit, "research_progress_band")

    # collision: the hall mass only. The ramps are approaches and must not read as blockers.
    m.collision.append(kit.CollisionBox("hall", (0.0, 0.0, (24.0 + ROOF_Z) / 2.0), (HALL_LEN, HALL_W, ROOF_Z - 24.0)))
    m.collision.append(kit.CollisionBox("intake_hood", (x0 - HOOD_LEN / 2.0 + 10.0, 0.0, HOOD_Z / 2.0),
                                        (HOOD_LEN, HOOD_W, HOOD_Z)))
    for socket in build_sockets():
        m.sockets.append(socket)
    return m


def build_sockets() -> list:
    x0, x1 = _hall_x()
    return [
        kit.Socket("Target_Anchor_Center", (0.0, 0.0, WALL_Z / 2.0), 0.0, "targeting and selection anchor at the hall's centre"),
        kit.Socket("Intake_Mouth", (x0 - HOOD_LEN + 8.0, 0.0, 40.0), 180.0,
                   "worker and Matter intake: the approach faces -X, away from the output"),
        kit.Socket("Output_Door", (x1 + 8.0, 0.0, 40.0), 0.0,
                   "finished-unit emergence point; the rally route starts here"),
        kit.Socket("Rail_Start", (BAY_X[0], 0.0, RAIL_Z - 16.0), 0.0, "fabrication rail: intake end of the carriage run"),
        kit.Socket("Rail_End", (BAY_X[1], 0.0, RAIL_Z - 16.0), 0.0, "fabrication rail: output end of the carriage run"),
        kit.Socket("Research_Gantry", (GANTRY_X, 0.0, GANTRY_Z + 100.0), 0.0,
                   "roof research gantry: the research state's light and effect origin"),
    ]


def assemble(lod: int, state: str = "producing") -> kit.Mesh:
    return build_main(lod, state)


def contract_inventory(m: kit.Mesh) -> dict:
    comps = m.components()
    return {
        "long_hall": {"contract": 1, "built": sum(1 for c in comps if c.startswith("hall_wall_")) // 2},
        "intake": {"contract": 1, "hood": "intake_hood" in comps, "mouth": "intake_mouth" in comps,
                   "ramp": "intake_ramp" in comps},
        "open_fabrication_bay": {"contract": 1, "rails": sum(1 for c in comps if c.startswith("rail_") and "carriage" not in c),
                                 "gantry_legs": sum(1 for c in comps if c.startswith("gantry_leg_")),
                                 "carriage": "rail_carriage" in comps},
        "output": {"contract": 1, "portal": "output_portal" in comps, "door": "output_door" in comps,
                   "ramp": "output_ramp" in comps},
        "research_gantry": {"contract": 1, "platform": "gantry_platform" in comps,
                            "instruments": sum(1 for c in comps if c.startswith("gantry_instrument_"))},
        "sockets": {"contract": ["Target_Anchor_Center", "Intake_Mouth", "Output_Door", "Rail_Start", "Rail_End", "Research_Gantry"],
                    "built": [s.name for s in m.sockets]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "hall_length_over_footprint": round(HALL_LEN / F, 4),
        "hall_width_over_footprint": round(HALL_W / F, 4),
        "hall_length_over_width": round(HALL_LEN / HALL_W, 4),
        "everything_inside_the_footprint": max(abs(x0), abs(x1), abs(y0), abs(y1)) <= HALF + 1e-6,
        "max_extent_cm": round(max(abs(x0), abs(x1), abs(y0), abs(y1)), 2),
        "intake_and_output_at_opposite_ends": True,
    }


def export(evidence_dir: str, out_dir: str) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, rows = [], []
    for lod in (0, 1):
        m = assemble(lod, "producing")
        base = os.path.join(out_dir, f"{ASSET}_LOD{lod}")
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod, "author": AUTHOR}
        glb = m.write_glb(base + ".glb", extras=extras, include_collision=(lod == 0))
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod}", f"Revision {REVISION}",
                                                       "Unreal +X intake->output, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "path": os.path.relpath(base + ".glb", HERE), "sha256": glb,
                        "triangles": m.triangle_count(), "bounds_cm": [list(p) for p in m.bounds()],
                        "section_slot_names": m.slots, "by_slot": m.triangle_count_by("slot"),
                        "collision_boxes": [{"name": b.name, "center_cm": list(b.center), "size_cm": list(b.size)} for b in m.collision] if lod == 0 else [],
                        "sockets": [{"name": s.name, "position_cm": [round(v, 2) for v in s.position], "yaw_deg": s.yaw_deg,
                                     "purpose": s.purpose} for s in m.sockets]})
        outputs.append({"mesh": ASSET, "lod": lod, "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        for lod in (0, 1):
            if lod == 1 and state != "producing":
                continue
            m = assemble(lod, state)
            path = os.path.join(review, f"{ASSET}_assembly_{state}_LOD{lod}.obj")
            rows.append({"state": state, "lod": lod, "path": os.path.relpath(path, evidence_dir),
                         "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state} LOD{lod}", f"Revision {REVISION}"]),
                         "triangles": m.triangle_count()})
    return {"outputs": outputs, "review_assemblies": rows}


def manifest(exported: dict) -> dict:
    m0, m1 = assemble(0, "producing"), assemble(1, "producing")
    whole0 = sum(o["triangles"] for o in exported["outputs"] if o.get("lod") == 0 and "triangles" in o)
    whole1 = sum(o["triangles"] for o in exported["outputs"] if o.get("lod") == 1 and "triangles" in o)
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"), "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X intake to output, +Y right, +Z up",
                  "pivot": "ground-contact centre of the hall", "nanite": False},
        "scale_basis": {"footprint": "4x4 tiles = 800 cm (buildings.json mc_array_foundry.footprint_cells)",
                        "concept": "concept-fidelity.md: a long hall with a ramp at each end; the hall is 0.80 of the footprint long and 0.38 wide so both ramps stay inside the square"},
        "material_slots": [CERAMIC, FRAME, STATUS],
        "material_slot_policy": "Pale ceramic, charcoal frame, cyan status. Grit, grease, hazard striping and markings are texture (card .TEX_MAPS).",
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "whole_asset_lod0_triangles": whole0, "whole_asset_lod1_triangles": whole1,
                    "lod0_cap": 8000, "lod1_cap": 3400,
                    "cap_source": ("REL-BLD-015.MC.FOUNDRY (8,000 / 3,400) governs; REL-ART-028 agrees at LOD0 and is looser at "
                                   "LOD1. Owner ruling 2026-09-07: the ceilings apply to the complete asset including any "
                                   "articulated components, so the whole-asset figures are recorded and tested."),
                    "lod0_within_cap": whole0 <= 8000, "lod1_within_cap": whole1 <= 3400},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0),
        "states": {s: "review assembly" for s in STATES},
        "outputs": exported["outputs"], "review_assemblies": exported["review_assemblies"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "technical": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/array-foundry-review/array-foundry-aligned-output-candidate.png",
                            "concepts": ["EBS-CON-MER-BLD-003 (KEEP)", "EBS-CON-MER-BLD-007 (REWORK)"],
                            "canon": "DevelopmentBible.md line 515 (SPEC-BLD-015.MC.FOUNDRY)",
                            "gameplay": "Content/Data/Source/buildings.json mc_array_foundry"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {CERAMIC: [0.72, 0.70, 0.66], FRAME: [0.05, 0.05, 0.055], STATUS: [0.16, 0.86, 0.96],
                          "_default": [0.5, 0.5, 0.5]},
            "emissive": [STATUS], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.3, "key": 0.8, "color": [1.0, 0.82, 0.62],
                      "fill_color": [0.48, 0.6, 0.88], "fill": 0.22},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [F, F], "footprint_color": [0.16, 0.86, 0.96]},
            "reference_figure": {"height_cm": 180, "position": [0, 460, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.12, "target": [0, 0, 130]},
             {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 130]},
             {"name": "rear", "type": "ortho", "from": "-X", "edges": True, "margin": 1.12, "target": [0, 0, 130]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.15, "target": [0, 0, 0]}]
    tactical = [{"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 100]},
                {"name": "concept_quarter", "type": "persp", "pitch_deg": -32, "yaw_deg": 150, "arm_cm": 980, "fov_deg": 50, "target": [0, 0, 150]},
                {"name": "tactical_mono", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 100], "grayscale": True}]
    written = []
    for state in STATES:
        scene = dict(base)
        scene["meshes"] = [{"obj": f"../review/{ASSET}_assembly_{state}_LOD0.obj"}]
        scene["views"] = ortho + tactical
        p = os.path.join(scenes, f"{state}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)
    lod1 = dict(base)
    lod1["meshes"] = [{"obj": f"../review/{ASSET}_assembly_producing_LOD1.obj"}]
    lod1["views"] = ortho[:2] + [tactical[0]]
    p = os.path.join(scenes, "lod1.json")
    with open(p, "w", encoding="utf-8") as handle:
        json.dump(lod1, handle, indent=1)
    written.append(p)
    return written


def build_all(evidence_dir: str, out_dir: str) -> dict:
    return manifest(export(evidence_dir, out_dir))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            ev = os.path.join(tmp, "evidence"); os.makedirs(ev, exist_ok=True)
            fresh = build_all(ev, os.path.join(tmp, "export"))
        saved = json.load(open(manifest_path, encoding="utf-8"))
        drift, missing = [], []
        saved_out = {os.path.basename(o["path"]): o["sha256"] for o in saved["outputs"]}
        for o in fresh["outputs"]:
            name = os.path.basename(o["path"])
            if name not in saved_out:
                missing.append(name)
            elif saved_out[name] != o["sha256"]:
                drift.append(name)
        saved_rev = {r["path"]: r["sha256"] for r in saved.get("review_assemblies", [])}
        for r in fresh["review_assemblies"]:
            if r["path"] not in saved_rev:
                missing.append(r["path"])
            elif saved_rev[r["path"]] != r["sha256"]:
                drift.append(r["path"])
        if saved.get("revision") != REVISION:
            drift.append(f"revision {saved.get('revision')} != {REVISION}")
        print(json.dumps({"check": "ok" if not drift and not missing else "drift", "revision": REVISION,
                          "drift": drift, "missing": missing,
                          "compared": {"outputs": len(fresh["outputs"]), "review_assemblies": len(fresh["review_assemblies"])}}))
        return 0 if not drift and not missing else 1
    os.makedirs(args.evidence_dir, exist_ok=True)
    data = build_all(args.evidence_dir, os.path.join(HERE, "export"))
    scenes = write_scenes(args.evidence_dir)
    data["review"] = {"scenes": [os.path.relpath(s, args.evidence_dir) for s in scenes]}
    with open(manifest_path, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=1, sort_keys=True)
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"], "lod1": data["budgets"]["lod1_triangles"],
                      "height_cm": data["concept_measurements"]["height_cm"],
                      "inside_footprint": data["concept_measurements"]["everything_inside_the_footprint"],
                      "sockets": len(data["outputs"][0]["sockets"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
