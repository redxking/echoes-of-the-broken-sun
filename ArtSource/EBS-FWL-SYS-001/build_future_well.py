#!/usr/bin/env python3
"""EBS-FWL-SYS-001 Future Well: one bowl/spire family with four public states (blockout).

Author: Angelis Pseftis.

Package contract: Docs/VisualAssetPipeline/motion/gap-decisions.json
  production_policy[package_id == "EBS-PKG-EBS-FAM-FWL-001"], gap GAP-01 (amber Harvest
  telegraph and inward spire collapse; charcoal spent bowl is permanent; Preserve is cyan
  custody; Reshape is magenta temporary terrain; Dormant keeps one bowl/spire with doubled
  shadows). Prepared amendment FWL-HARVEST (REL-ART-014) is the selected direction; it is
  not applied to the master.
Canon: Docs/Archive/DevelopmentBible.md "Future Wells" (Harvest 180-tick public telegraph,
  permanent collapse; Preserve intact, 15 Dawn / 300 ticks, 1,400 cm intelligence; Reshape
  120 Dawn, 180-tick telegraph, 1,800-tick authored possibility with warned expiry).
Book: paragraphs 243-247 ("Amber light began to climb the core spire" ... "The spire folded
  into the bowl" ... the rising harmonic ended abruptly).
Simulation envelope (authoritative): EchoesSimCore Simulation.cpp — the Well's blocking
  footprint is one tile (footprintHalfExtentRaw = kFixedScale/2), capture radius 4.2 tiles
  (kFutureWellCaptureRadiusRaw), scar radius 6 tiles; presentation tile = 200 cm. The Well is
  impassable and indestructible (SPEC-WEL-004); it never takes combat destruction.
Presentation rule: decorative art on passable tiles must not read as impassable or exceed
  20 cm of relief (REL-ART-016, REL-ART-030). Therefore the raised bowl mass stays inside the
  one-tile footprint and the surrounding apron that carries the state rings and traces is a
  flat, walkable disc.

Every dimension is a PROVISIONAL BLOCKOUT ESTIMATE bound to that envelope.

Parts: the four spire petals are one part mesh instanced at Petal_01..04; Harvest commit tilts them
inward and sinks them into the bowl (a part pose, not a skeletal rig). All state colour lives in
the state slot's texture masks and material parameters, driven only by authoritative state.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
import ebs_meshkit as kit  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-EBS-FAM-FWL-001"
PRODUCTION_ID = "EBS-FWL-SYS-001"
ASSET = "SM_EBS_FWL_SYS_001"
REVISION = "ebs-fwl-sys-001-blockout-v1"
PLANNED_FOLDER = "/Game/Echoes/Production/FWL/SYS/EBS_FWL_SYS_001/"

BASALT = "MI_EBS_FWL_Basalt"        # charcoal vitrified masonry (bowl, apron plates)
VITRIFIED = "MI_EBS_FWL_Vitrified"  # charcoal glass with magenta micro-fracture (spire petals, core)
STATE = "MI_EBS_FWL_State"          # state-masked emissive: veins, preserve ring, reshape trace, petal seams

TILE_CM = 200.0
FOOTPRINT = (TILE_CM, TILE_CM)                    # one tile: the impassable bowl mass
BOWL_R_OUT, BOWL_R_IN, BOWL_FLOOR_Z = 98.0, 74.0, 8.0
RIM_HEIGHTS = (58.0, 44.0, 66.0, 40.0, 52.0, 72.0, 46.0, 60.0)   # eight masonry courses, ruined rhythm
RIM_BLOCKS = 16
APRON_R, APRON_Z = 300.0, 5.0                    # walkable apron: relief <= 20 cm (REL-ART-030)
APRON_RIDGE_H, APRON_RIDGE_COUNT = 11.0, 8
PRESERVE_RING_R, PRESERVE_RING_W = 215.0, 12.0
RESHAPE_TRACE_LEN = APRON_R + 30.0               # trace leaves the apron toward the authored feature
PETAL_BASE_HALF, PETAL_HEIGHT, PETAL_COUNT = 34.0, 172.0, 4
CORE_R, CORE_TOP = 22.0, 52.0
FOLD_PITCH_DEG = 28.0                             # harvest commit: petals tilt inward toward the axis ...
FOLD_SINK_CM = 150.0                              # ... and sink into the bowl (the spire folds into the bowl, book 245)


def polygon_outline(radius: float, sides: int, phase_deg: float = 0.0):
    return kit.regular_polygon(radius, sides, phase_deg)


# --- bowl (impassable one-tile mass) ----------------------------------------------------------
def build_main(lod: int) -> kit.Mesh:
    m = kit.Mesh(ASSET)
    basalt, vitrified, state = m.slot(BASALT), m.slot(VITRIFIED), m.slot(STATE)
    hi = lod == 0
    sides = 16 if hi else 12

    # Bowl floor and inner wall: a sunken cracked floor inside a masonry ring.
    m.prism(polygon_outline(BOWL_R_IN, sides), 0.0, BOWL_FLOOR_Z, basalt, "bowl_floor", cap_bottom=False)
    # Rim courses: sixteen blocks per course, heights vary around the ring so the rim reads ruined.
    for k in range(RIM_BLOCKS):
        a0 = 2 * math.pi * k / RIM_BLOCKS
        a1 = 2 * math.pi * (k + 1) / RIM_BLOCKS
        h = RIM_HEIGHTS[k % len(RIM_HEIGHTS)] * (1.0 if hi else 0.9)
        gap = 0.06
        outer = [(BOWL_R_OUT * math.cos(a0 + gap), BOWL_R_OUT * math.sin(a0 + gap)), (BOWL_R_OUT * math.cos(a1 - gap), BOWL_R_OUT * math.sin(a1 - gap))]
        inner = [(BOWL_R_IN * math.cos(a1 - gap), BOWL_R_IN * math.sin(a1 - gap)), (BOWL_R_IN * math.cos(a0 + gap), BOWL_R_IN * math.sin(a0 + gap))]
        m.prism(outer + inner, 0.0, h, basalt, f"rim_block_{k + 1:02d}", cap_bottom=False)
        if hi and k % 2 == 0:
            # a displaced coping stone on every second block
            cx, cy = (BOWL_R_OUT + BOWL_R_IN) / 2.0 * math.cos((a0 + a1) / 2.0), (BOWL_R_OUT + BOWL_R_IN) / 2.0 * math.sin((a0 + a1) / 2.0)
            m.box((cx, cy, h + 5.0), (20.0, 14.0, 10.0), basalt, f"rim_block_{k + 1:02d}", yaw_deg=math.degrees((a0 + a1) / 2.0) + 6.0)
    # Dormant fracture veins on the bowl floor: three shallow channels in the state slot.
    for k in range(3):
        yaw = 15.0 + 120.0 * k
        m.box((0.0, 0.0, BOWL_FLOOR_Z + 0.6), (2 * BOWL_R_IN - 12.0, 4.0, 1.2), state, f"bowl_vein_{k + 1:02d}", yaw_deg=yaw)
    # Core stub the petals close over.
    m.prism(polygon_outline(CORE_R, 8), BOWL_FLOOR_Z, CORE_TOP, vitrified, "core_stub", cap_bottom=False)

    # Walkable apron: a flat fractured disc with eight low ridges, the Preserve ring and the Reshape trace.
    m.prism(polygon_outline(APRON_R, 24 if hi else 16), 0.0, APRON_Z, basalt, "apron_disc", cap_bottom=False)
    for k in range(APRON_RIDGE_COUNT):
        yaw = 360.0 * k / APRON_RIDGE_COUNT + 11.0
        length = APRON_R - BOWL_R_OUT - 20.0
        cx = (BOWL_R_OUT + 12.0 + length / 2.0) * math.cos(math.radians(yaw))
        cy = (BOWL_R_OUT + 12.0 + length / 2.0) * math.sin(math.radians(yaw))
        m.box((cx, cy, APRON_Z + APRON_RIDGE_H / 2.0), (length, 9.0 + (k % 3) * 3.0, APRON_RIDGE_H), basalt, f"apron_ridge_{k + 1:02d}", yaw_deg=yaw)
    m.ring(polygon_outline(PRESERVE_RING_R + PRESERVE_RING_W / 2.0, 32 if hi else 20), polygon_outline(PRESERVE_RING_R - PRESERVE_RING_W / 2.0, 32 if hi else 20),
           APRON_Z, APRON_Z + 2.5, state, "preserve_ring")
    m.box((BOWL_R_OUT + (RESHAPE_TRACE_LEN - BOWL_R_OUT) / 2.0, 0.0, APRON_Z + 1.5), (RESHAPE_TRACE_LEN - BOWL_R_OUT, 8.0, 3.0), state, "reshape_trace")
    if hi:
        for k in range(6):
            r = 120.0 + 28.0 * k
            m.box((r, 0.0, APRON_Z + 3.5), (10.0, 18.0, 4.0), state, "reshape_trace")  # rung marks along the trace

    # Sockets: contract anchors, petal hinges (yaw k*90 so the part's local +X points outward), trace end.
    m.sockets.append(kit.Socket("Target_Anchor_Center", (0.0, 0.0, BOWL_FLOOR_Z), 0.0, "capture / protocol targeting anchor at the bowl centre"))
    m.sockets.append(kit.Socket("State_VFX_Origin", (0.0, 0.0, CORE_TOP), 0.0, "Harvest amber climb, Preserve pulse and Reshape trace effects originate here"))
    for k in range(PETAL_COUNT):
        yaw = 90.0 * k
        m.sockets.append(kit.Socket(f"Petal_{k + 1:02d}", (PETAL_BASE_HALF * 0.55 * math.cos(math.radians(yaw)), PETAL_BASE_HALF * 0.55 * math.sin(math.radians(yaw)), BOWL_FLOOR_Z), yaw,
                                    "spire petal hinge; folds outward-down on Harvest commit (part pose)"))
    m.sockets.append(kit.Socket("Reshape_Trace_End", (RESHAPE_TRACE_LEN, 0.0, APRON_Z), 0.0, "where the map-authored temporary feature's own effect continues"))
    m.collision.append(kit.CollisionBox("bowl", (0.0, 0.0, 36.0), (2 * BOWL_R_OUT, 2 * BOWL_R_OUT, 72.0)))
    return m


# --- spire petal part (one mesh, four instances) -----------------------------------------------
def build_petal(lod: int) -> kit.Mesh:
    """Hinge at the origin on the bowl floor; the petal leans inward toward -X so four instances
    at yaw 0/90/180/270 (outward +X) close into one four-sided spire."""
    m = kit.Mesh(f"{ASSET}_Petal")
    vitrified, state = m.slot(VITRIFIED), m.slot(STATE)
    hi = lod == 0
    lean = PETAL_BASE_HALF * 0.55  # apex sits over the spire axis, which is lean cm toward -X of the hinge
    apex = (-lean, 0.0, PETAL_HEIGHT)
    base = [(0.0, -PETAL_BASE_HALF, 0.0), (0.0, PETAL_BASE_HALF, 0.0), (-lean * 0.9, PETAL_BASE_HALF * 0.35, 0.0), (-lean * 0.9, -PETAL_BASE_HALF * 0.35, 0.0)]
    faces = [
        [base[0], base[1], apex],                       # outer facet
        [base[1], base[2], apex],                       # side facet
        [base[2], base[3], apex],                       # inner facet (toward the axis)
        [base[3], base[0], apex],                       # side facet
        [base[3], base[2], base[1], base[0]],           # underside
    ]
    m.add_convex_solid(faces, vitrified, "petal_body")
    # Amber seam: the Harvest telegraph light climbs this seam (state mask ramps with progress).
    seam_top = (-lean * 0.35, 0.0, PETAL_HEIGHT * 0.62)
    m.box(((seam_top[0] + 2.0) / 2.0, 0.0, seam_top[2] / 2.0), (2.5, 6.0, PETAL_HEIGHT * 0.6), state, "petal_seam", yaw_deg=0.0)
    if hi:
        m.box((-6.0, 0.0, PETAL_HEIGHT * 0.86), (6.0, 10.0, 3.0), state, "petal_tip_mark")
    return m


# --- assemblies -----------------------------------------------------------------------------------
def assemble(lod: int, state: str) -> kit.Mesh:
    main = build_main(lod)
    petal = build_petal(lod)
    scene = kit.Mesh(f"{ASSET}_assembly_{state}_LOD{lod}")
    scene.merge(main)
    amount = 1.0 if state in ("harvest_commit", "spent") else (0.5 if state == "harvest_fold_mid" else 0.0)
    for s in main.sockets:
        if s.name.startswith("Petal_"):
            sunk = (s.position[0], s.position[1], s.position[2] - FOLD_SINK_CM * amount)
            scene.merge(petal, translate=sunk, yaw_deg=s.yaw_deg, pitch_deg=FOLD_PITCH_DEG * amount, component_prefix=f"{s.name.lower()}_", include_sockets=False)
    return scene


STATE_PRESENTATIONS = [
    {"track": "dormant", "petals": "up", "state_mask": {"veins": 0.25, "preserve_ring": 0.0, "reshape_trace": 0.0, "petal_seam": 0.15}, "colour": "quiet magenta/amber fracture", "extra": "doubled-shadow decal (Crownfall dual key/fill), reduced motion: static", "authority": "wellChoice == Dormant"},
    {"track": "harvest_telegraph", "petals": "up", "state_mask": {"petal_seam": "ramps 0.15 -> 1.0 over the authoritative 180-tick telegraph progress", "veins": 0.4}, "colour": "broken-sun amber", "extra": "amber climb VFX at State_VFX_Origin; public siren/ping are simulation-owned", "authority": "harvest telegraph progress"},
    {"track": "harvest_cancel", "petals": "up", "state_mask": {"petal_seam": "returns to dormant without a spent pose or success cue"}, "colour": "dormant", "extra": "no payout cue", "authority": "control broken before tick 180"},
    {"track": "harvest_commit", "petals": "tilt inward 0 -> 28 deg and sink 150 cm over ~20 ticks", "state_mask": {"petal_seam": "1.0 then hard cut to 0"}, "colour": "amber cut to dark", "extra": "light and harmonic cut hard on commit (book 246)", "authority": "harvest committed"},
    {"track": "spent", "petals": "sunk into the bowl; only the tips break the floor", "state_mask": {"all": 0.0}, "colour": "dark cracked charcoal", "extra": "permanent; scar radius 6 tiles is terrain-owned; Well non-interactive", "authority": "permanent_state == collapsed"},
    {"track": "preserve_hold", "petals": "up", "state_mask": {"preserve_ring": "1.0 with a slow custody pulse", "petal_seam": 0.3}, "colour": "cyan custody", "extra": "1,400 cm intelligence radius is simulation-owned; ring carries no gameplay radius", "authority": "wellChoice == Preserve and controlled"},
    {"track": "preserve_loss", "petals": "up", "state_mask": {"preserve_ring": "0.0"}, "colour": "ring dark", "extra": "ownership transfer routes benefits authoritatively", "authority": "control lost"},
    {"track": "reshape_telegraph", "petals": "up", "state_mask": {"reshape_trace": "ramps 0 -> 1.0 over the 180-tick telegraph"}, "colour": "magenta fracture", "extra": "trace rung marks fill toward Reshape_Trace_End", "authority": "reshape telegraph progress"},
    {"track": "reshape_cancel", "petals": "up", "state_mask": {"reshape_trace": "returns to 0"}, "colour": "dormant", "extra": "", "authority": "telegraph interrupted"},
    {"track": "reshape_manifest", "petals": "up", "state_mask": {"reshape_trace": 1.0}, "colour": "magenta", "extra": "the temporary feature itself is a separate map-authored asset", "authority": "reshape manifest active (1,800 ticks)"},
    {"track": "reshape_warning", "petals": "up", "state_mask": {"reshape_trace": "slow warning cadence (no rapid flash)"}, "colour": "magenta", "extra": "expiry warning is simulation-owned; reduced flashing holds steady", "authority": "expiry warning window"},
    {"track": "reshape_expiry", "petals": "up", "state_mask": {"reshape_trace": 0.0}, "colour": "dormant", "extra": "authored fallback displacement is simulation-owned", "authority": "manifest expired"},
    {"track": "restore", "petals": "per current state", "state_mask": {"all": "per current state"}, "colour": "per state", "extra": "reconstruct from saved state; never replay payout, collapse or terrain creation", "authority": "load / replay"},
]


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def contract_inventory(main: kit.Mesh) -> dict:
    comps = main.components()
    return {
        "bowl": {"contract": 1, "built": 1 if "bowl_floor" in comps else 0, "rim_blocks": sum(1 for c in comps if c.startswith("rim_block_"))},
        "core_spire": {"contract": 1, "built": 1, "evidence": "one core stub plus one petal part instanced at Petal_01..04 (4 sockets)", "petal_sockets": sum(1 for s in main.sockets if s.name.startswith("Petal_"))},
        "state_family": {"contract": 4, "built": 4, "states": ["Dormant", "Harvest", "Preserve", "Reshape"], "presentations": len(STATE_PRESENTATIONS)},
        "sockets": {"contract": ["Target_Anchor_Center", "State_VFX_Origin"], "built": [s.name for s in main.sockets]},
    }


def main_cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    export_dir = os.path.join(HERE, "export")
    review_dir = os.path.join(args.evidence_dir, "review")
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)
    outputs, review = [], []
    meshes = {}
    for lod in (0, 1):
        for builder in (build_main, build_petal):
            mesh = builder(lod)
            meshes[(mesh.name, lod)] = mesh
            base = os.path.join(export_dir, f"{mesh.name}_LOD{lod}")
            glb = mesh.write_glb(base + ".glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod}, include_collision=(lod == 0))
            obj = mesh.write_obj(base + ".obj", header_lines=[f"Production ID {PRODUCTION_ID}", f"Revision {REVISION}", f"LOD{lod}"])
            outputs.append({"path": os.path.relpath(base + ".glb", HERE), "sha256": glb, "lod": lod, "mesh": mesh.name, "triangles": mesh.triangle_count(),
                            "by_slot": mesh.triangle_count_by("slot"), "bounds_cm": mesh.bounds(), "section_slot_names": mesh.slot_names_in_primitive_order(),
                            "sockets": [{"name": s.name, "position_cm": s.position, "yaw_deg": s.yaw_deg, "purpose": s.purpose} for s in mesh.sockets],
                            "collision_boxes": [{"name": c.name, "center_cm": c.center, "size_cm": c.size} for c in mesh.collision] if lod == 0 else []})
            outputs.append({"path": os.path.relpath(base + ".obj", HERE), "sha256": obj, "lod": lod, "mesh": mesh.name})
    for lod, state in ((0, "dormant"), (0, "harvest_fold_mid"), (0, "spent"), (1, "dormant")):
        scene = assemble(lod, state)
        path = os.path.join(review_dir, f"{scene.name}.obj")
        review.append({"path": path, "sha256": scene.write_obj(path, header_lines=[f"Assembled review geometry {state} LOD{lod}"]), "lod": lod, "state": state, "triangles": scene.triangle_count(), "bounds_cm": scene.bounds()})
    main0, main1 = meshes[(ASSET, 0)], meshes[(ASSET, 1)]
    petal0, petal1 = meshes[(f"{ASSET}_Petal", 0)], meshes[(f"{ASSET}_Petal", 1)]
    bowl_bounds = main0.component_bounds("rim_block_01")
    rim_max = max(main0.component_bounds(f"rim_block_{k + 1:02d}")[1][2] for k in range(RIM_BLOCKS))
    apron_max_z = max(main0.component_bounds(f"apron_ridge_{k + 1:02d}")[1][2] for k in range(APRON_RIDGE_COUNT))
    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    manifest = {
        "author": AUTHOR, "creator": AUTHOR, "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "asset_name": ASSET,
        "revision": REVISION, "kit_revision": kit.KIT_REVISION, "stage": "BLOCKOUT",
        "stage_boundary": "Editable source geometry, petal part and state-presentation plan with review views; no textures, no import, no gate acceptance, no owner acceptance.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters", "axes": "+X forward (Reshape trace direction), +Y right, +Z up", "pivot": "ground-contact centre of the one-tile footprint"},
        "scale_basis": {
            "footprint_tiles": [1, 1], "footprint_cm": FOOTPRINT, "capture_radius_cm_presentation": 4.2 * TILE_CM, "scar_radius_cm_presentation": 6 * TILE_CM,
            "source": "Source/EchoesSimCore/Private/Simulation.cpp: FutureWell footprintHalfExtentRaw = kFixedScale/2; kFutureWellCaptureRadiusRaw = 21*kFixedScale/5; kFutureWellScarRadiusRaw = 6*kFixedScale; presentation TileWorldSize 200",
            "impassable_mass_max_radius_cm": BOWL_R_OUT, "apron_radius_cm": APRON_R, "apron_relief_cm": apron_max_z, "apron_rule": "REL-ART-016 / REL-ART-030: passable dressing <= 20 cm and not reading impassable",
            "status": "PROVISIONAL BLOCKOUT ESTIMATE; the current runtime shrinks its 280 cm-radius placeholder dais by 0.35 to fit the same one-tile footprint",
        },
        "dimensions_cm_provisional": {"bowl_outer_radius": BOWL_R_OUT, "bowl_inner_radius": BOWL_R_IN, "bowl_floor_z": BOWL_FLOOR_Z, "rim_height_max": rim_max, "apron_radius": APRON_R,
                                      "apron_z": APRON_Z, "preserve_ring_radius": PRESERVE_RING_R, "reshape_trace_length": RESHAPE_TRACE_LEN, "petal_height": PETAL_HEIGHT, "spire_top_z": BOWL_FLOOR_Z + PETAL_HEIGHT, "fold_pitch_deg": FOLD_PITCH_DEG, "fold_sink_cm": FOLD_SINK_CM},
        "material_slots": [BASALT, VITRIFIED, STATE],
        "component_inventory": contract_inventory(main0),
        "state_presentations": STATE_PRESENTATIONS,
        "budgets": {"lod0_assembled_triangles": main0.triangle_count() + 4 * petal0.triangle_count(), "lod1_assembled_triangles": main1.triangle_count() + 4 * petal1.triangle_count(),
                    "lod0_cap": 8000, "lod1_cap": 3500},
        "outputs": outputs, "review_assemblies": review,
        "tools": {"builder_sha256": builder_sha, "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "candidate_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/future-well-review/future-well-candidate.png", "sha256": "b16e84cb4465c3c3cee46a71fb4b661256c0bdcbb9a030994fd7d7262ed60ba7"},
            "state_board": {"path": "Docs/VisualAssetPipeline/motion/storyboards/EBS-PKG-EBS-FAM-FWL-001-GAP-01.svg"},
            "motion_package": {"path": "Docs/VisualAssetPipeline/motion/motion-packages.json", "gap_id": "EBS-PKG-EBS-FAM-FWL-001-GAP-01"},
            "amendment": "gap-decisions.json#/master_amendments FWL-HARVEST (PREPARED_NOT_APPLIED)",
            "book": {"paragraphs": [243, 244, 245, 246, 247], "sha256": "994e7df5d37a6ef1532c65a7b742d71e81b4ea7752311f5e5acc2e04c817ef83"},
            "gameplay_record": {"path": "Content/Data/Source/future_wells.json", "sha256": "a80187648a96551dcd121b03bca2c6e31ad16e75570ef7f5b33204d18e5554e9"},
            "requirements": ["SPEC-WEL-001..004", "SPEC-WELLP-001..003", "REL-WEL-005/006/010/015/016", "REL-ART-014 (amendment pending)", "REL-ART-016", "REL-ART-030"],
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }
    manifest["budgets"]["lod0_within_cap"] = manifest["budgets"]["lod0_assembled_triangles"] <= 8000
    manifest["budgets"]["lod1_within_cap"] = manifest["budgets"]["lod1_assembled_triangles"] <= 3500
    manifest_path = os.path.join(HERE, "build-manifest.json")
    text = json.dumps(manifest, indent=1, sort_keys=True) + "\n"
    if args.check and os.path.exists(manifest_path):
        with open(manifest_path, "r", encoding="utf-8") as handle:
            previous = json.load(handle)
        prev = {o["path"]: o["sha256"] for o in previous.get("outputs", [])}
        drift = [o["path"] for o in outputs if prev.get(o["path"]) != o["sha256"]]
        print(json.dumps({"check": "ok" if not drift else "drift", "drift": drift}))
        return 0 if not drift else 3
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"], "rim_max_cm": rim_max, "apron_relief_cm": apron_max_z, "inventory": {k: v.get("built") for k, v in manifest["component_inventory"].items()}}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
