#!/usr/bin/env python3
"""EBS-MER-BLD-002 Power Link: deterministic editable source geometry (blockout).

Author: Angelis Pseftis.

Package contract: Docs/VisualAssetPipeline/motion/gap-decisions.json
  production_policy[package_id == "EBS-PKG-MC-POWER-LINK"], gaps GAP-01..03.
Canon row: Docs/Archive/DevelopmentBible.md line 514 (SPEC-BLD-015.MC.LINK).
Gameplay source: Content/Data/Source/buildings.json id=mc_power_link (2x2 cells).
Presentation tile: Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h
  TileWorldSize = 200 cm  ->  footprint 400 x 400 cm.

Every dimension below is a PROVISIONAL BLOCKOUT ESTIMATE bound to that footprint;
none is an approved production dimension. The component inventory is the
contract's: 4 numbered panels, 1 collar assembly, 2 base couplings, 4 conduits.

Outputs (run from anywhere):
  export/SM_EBS_MER_BLD_002_LOD{0,1}.{glb,obj}          main static mesh
  export/SM_EBS_MER_BLD_002_Panel_LOD{0,1}.{glb,obj}    removable numbered panel (x4 instances)
  export/SM_EBS_MER_BLD_002_ConduitStub_LOD{0,1}.{glb,obj}  short physical conduit stub (x4 instances)
  build-manifest.json                                   counts, budgets, sockets, hashes
  <evidence>/review/*.obj + scene JSONs                 assembled review geometry (not tracked)

Regeneration under an unchanged REVISION must be byte-identical; the manifest
records every output hash so that can be checked.
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
PACKAGE_ID = "EBS-PKG-MC-POWER-LINK"
PRODUCTION_ID = "EBS-MER-BLD-002"
ASSET = "SM_EBS_MER_BLD_002"
REVISION = "ebs-mer-bld-002-blockout-v1"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_002/"

# Material slots (provisional max 3 per contract): index order is the slot order.
CERAMIC = "MI_EBS_MER_CeramicCivic"   # pale civic ceramic skin (existing T_EchoesCeramicCivic family)
FRAME = "MI_EBS_MER_CompactFrame"     # charcoal structural frame / machined metal
STATUS = "MI_EBS_MER_StatusCyan"      # cyan status collars, indicator strips, conduit pulse (state-masked emissive)

# --- provisional blockout dimensions (cm) ------------------------------------
TILE_CM = 200.0
FOOTPRINT = (2 * TILE_CM, 2 * TILE_CM)
PLINTH_HALF, PLINTH_CHAMFER, PLINTH_TOP = 160.0, 20.0, 48.0
STEP_HALF, STEP_CHAMFER, STEP_TOP = 140.0, 16.0, 60.0
RING_OUT_HALF, RING_OUT_CH, RING_IN_HALF, RING_IN_CH, RING_TOP = 118.0, 14.0, 84.0, 10.0, 112.0
SHAFT_HALF, SHAFT_CHAMFER, SHAFT_TOP = 75.0, 22.0, 1200.0
# Ceramic shaft rows (bottom, top) and the charcoal grooves between them.
ROWS = [(RING_TOP, 365.0), (380.0, 615.0), (630.0, 865.0), (950.0, SHAFT_TOP)]
GROOVES = [(365.0, 380.0), (615.0, 630.0)]
GROOVE_HALF, GROOVE_CH = 70.0, 20.0
COLLAR_Z = (880.0, 950.0)
COLLAR_OUT_HALF, COLLAR_OUT_CH, COLLAR_IN_HALF, COLLAR_IN_CH = 118.0, 30.0, 77.0, 23.0
SEGMENT_Z = (902.0, 928.0)
CAP_HALF, CAP_CH, CAP_TOP = 82.0, 24.0, 1216.0
SERVICE_CAP = (64.0, 64.0, 22.0)
PANEL_W, PANEL_H, PANEL_T = 116.0, 210.0, 6.0
# Panel centres from the top (01) to the bottom (04): decision GAP-01.
PANEL_Z = {"01": 1070.0, "02": 745.0, "03": 495.0, "04": 235.0}
BAY_Z = (388.0, 852.0)            # maintenance opening spans panels 02 and 03 (GAP-02)
BAY_HALF_W, BAY_DEPTH = 58.0, 30.0
COUPLING = (104.0, 50.0, 86.0)    # X, Y (depth), Z
COUPLING_Y = 175.0                # centre; spans 150..200 on each side, inside the footprint
PORT_X, PORT_Z, PORT_R = 30.0, 46.0, 14.0
STUB_LENGTH = 90.0
RAIL_OFFSET = (SHAFT_HALF - SHAFT_CHAMFER / 2.0) + 4.0 / 1.41421356


def outline_octagon(half, chamfer):
    return kit.octagon(half, chamfer)


# --- main static mesh -----------------------------------------------------------
def build_main(lod: int) -> kit.Mesh:
    m = kit.Mesh(ASSET)
    ceramic, frame, status = m.slot(CERAMIC), m.slot(FRAME), m.slot(STATUS)
    hi = lod == 0
    sides = 8 if hi else 6

    # Base: charcoal plinth, step, and the orthogonal foundation ring that locks the mast.
    m.prism(outline_octagon(PLINTH_HALF, PLINTH_CHAMFER), 0.0, PLINTH_TOP, frame, "plinth", cap_bottom=False)
    m.prism(outline_octagon(STEP_HALF, STEP_CHAMFER), PLINTH_TOP, STEP_TOP, frame, "plinth_step", cap_bottom=False)
    m.ring(outline_octagon(RING_OUT_HALF, RING_OUT_CH), outline_octagon(RING_IN_HALF, RING_IN_CH), STEP_TOP, RING_TOP, frame, "foundation_ring")
    if hi:
        for name, (x, y, yaw) in {"lug_front": (RING_OUT_HALF + 6, 0.0, 0.0), "lug_rear": (-RING_OUT_HALF - 6, 0.0, 0.0),
                                  "lug_left": (0.0, -RING_OUT_HALF - 6, 90.0), "lug_right": (0.0, RING_OUT_HALF + 6, 90.0)}.items():
            m.box((x, y, 86.0), (12.0, 36.0, 24.0), frame, "foundation_ring", yaw_deg=yaw)

    # Pale ceramic cladding on the plinth flanks and coupling tops: the Compact's civic ceramic over
    # the charcoal frame, so the base separates from charcoal ground at gameplay distance.
    for sx in (-1.0, 1.0):
        m.box((sx * (PLINTH_HALF + 3.0), 0.0, 26.0), (6.0, 216.0, 30.0), ceramic, "plinth_cladding")
    for sy in (-1.0, 1.0):
        for x in (-92.0, 92.0):
            m.box((x, sy * (PLINTH_HALF + 3.0), 26.0), (60.0, 6.0, 30.0), ceramic, "plinth_cladding")
    for sx in (-1.0, 1.0):
        m.box((sx * (STEP_HALF + 2.0), 0.0, 54.0), (4.0, 120.0, 8.0), ceramic, "plinth_cladding")

    # Ceramic shaft in four panel rows; the two middle rows open toward +X for the service bay.
    octagon = outline_octagon(SHAFT_HALF, SHAFT_CHAMFER)
    for index, (z0, z1) in enumerate(ROWS):
        row = 4 - index  # rows are numbered like the panels: 04 at the bottom, 01 at the top
        skip = (2,) if row in (2, 3) else ()   # edge 2 of the octagon is the +X face
        m.prism(octagon, z0, z1, ceramic, f"shaft_row_{row:02d}", cap_bottom=(index == 0), cap_top=(index == len(ROWS) - 1), skip_edges=skip)
    for index, (z0, z1) in enumerate(GROOVES):
        skip = (2,) if index == 1 else ()      # the 615-630 groove lies inside the bay opening
        m.prism(outline_octagon(GROOVE_HALF, GROOVE_CH), z0, z1, frame, f"shaft_groove_{index + 1:02d}", cap_bottom=False, cap_top=False, skip_edges=skip)
    # Groove under the collar and the row-01 seat are covered by the collar assembly itself.

    # Service bay: open box behind panels 02/03 with the exposed internal conduit bundle (GAP-02).
    bay_x0 = SHAFT_HALF - BAY_DEPTH
    m.box((bay_x0 + BAY_DEPTH / 2.0, 0.0, (BAY_Z[0] + BAY_Z[1]) / 2.0), (BAY_DEPTH, 2 * BAY_HALF_W, BAY_Z[1] - BAY_Z[0]), frame, "service_bay", skip=("+X",))
    # Frame strips around the opening on the +X face (the ceramic reveal the panels seat against).
    reveal_t = 8.0
    for y in (-BAY_HALF_W - reveal_t / 2.0, BAY_HALF_W + reveal_t / 2.0):
        m.box((SHAFT_HALF - 1.0, y, (BAY_Z[0] + BAY_Z[1]) / 2.0), (2.0, reveal_t, BAY_Z[1] - BAY_Z[0] + 2 * reveal_t), frame, "service_bay")
    for z in (BAY_Z[0] - reveal_t / 2.0, BAY_Z[1] + reveal_t / 2.0):
        m.box((SHAFT_HALF - 1.0, 0.0, z), (2.0, 2 * BAY_HALF_W, reveal_t), frame, "service_bay")
    # Internal conduits: three vertical tubes and a cross-bar, visible only with panels 02/03 removed.
    for y in (-30.0, 0.0, 30.0):
        m.tube((bay_x0 + 12.0, y, BAY_Z[0] + 8.0), (bay_x0 + 12.0, y, BAY_Z[1] - 8.0), 8.0, sides, frame, "service_bay_conduits", caps=hi)
    m.box((bay_x0 + 12.0, 0.0, (BAY_Z[0] + BAY_Z[1]) / 2.0), (14.0, 96.0, 16.0), frame, "service_bay_conduits")
    if hi:
        m.box((bay_x0 + 10.0, -34.0, BAY_Z[0] + 40.0), (12.0, 20.0, 20.0), status, "service_bay_conduits")  # bay indicator

    # Exposed charcoal load frame: four corner rails on the chamfers.
    for k, (sx, sy) in enumerate(((1, 1), (-1, 1), (-1, -1), (1, -1))):
        m.box((sx * RAIL_OFFSET, sy * RAIL_OFFSET, (STEP_TOP + CAP_TOP) / 2.0), (14.0, 26.0, CAP_TOP - STEP_TOP), frame, f"frame_rail_{k + 1:02d}", yaw_deg=45.0 * sx * sy)

    # Collar assembly: one ring, eight cyan conductor segments, four gussets (contract: 1 collar assembly).
    outer = outline_octagon(COLLAR_OUT_HALF, COLLAR_OUT_CH)
    inner = outline_octagon(COLLAR_IN_HALF, COLLAR_IN_CH)
    m.ring(outer, inner, COLLAR_Z[0], COLLAR_Z[1], frame, "collar_assembly")
    for k in range(8):
        a = outer[k]
        b = outer[(k + 1) % 8]
        mid = ((a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0)
        length = ((b[0] - a[0]) ** 2 + (b[1] - a[1]) ** 2) ** 0.5
        nx, ny = kit.v_norm((mid[0], mid[1], 0.0))[:2]
        width = min(64.0, length - 8.0)
        # Segment sits proud of the octagon face; orient its long axis along the face.
        yaw = math.degrees(math.atan2(ny, nx))
        m.box((mid[0] + nx * 4.0, mid[1] + ny * 4.0, (SEGMENT_Z[0] + SEGMENT_Z[1]) / 2.0), (8.0, width, SEGMENT_Z[1] - SEGMENT_Z[0]), status, f"collar_segment_{k + 1:02d}", yaw_deg=yaw)
    for k, (sx, sy) in enumerate(((1, 1), (-1, 1), (-1, -1), (1, -1))):
        m.box((sx * 84.9, sy * 84.9, COLLAR_Z[0] - 15.0), (60.0, 24.0, 30.0), frame, "collar_assembly", yaw_deg=45.0 * sx * sy)

    # Cap: flat practical top, small service cap (no pointed crystal: package direction).
    m.prism(outline_octagon(CAP_HALF, CAP_CH), SHAFT_TOP, CAP_TOP, frame, "cap", cap_bottom=False)
    m.box((0.0, 0.0, CAP_TOP + SERVICE_CAP[2] / 2.0), SERVICE_CAP, ceramic, "cap")
    if hi:
        m.box((0.0, 0.0, CAP_TOP + SERVICE_CAP[2] + 3.0), (40.0, 8.0, 6.0), status, "cap")  # top status tell-tale

    # Base couplings (contract: 2) on the left (-Y) and right (+Y) plinth flanks with two ports each.
    for side, sign in (("left", -1.0), ("right", 1.0)):
        cy = sign * COUPLING_Y
        m.box((0.0, cy, COUPLING[2] / 2.0), COUPLING, frame, f"coupling_{side}", skip=("-Z",))
        m.box((0.0, cy, COUPLING[2] + 2.0), (92.0, 40.0, 4.0), ceramic, f"coupling_{side}")     # ceramic top plate
        m.box((0.0, cy, COUPLING[2] + 5.5), (44.0, 10.0, 3.0), status, f"coupling_{side}")     # indicator strip
        for k, px in enumerate((-PORT_X, PORT_X)):
            y_in = sign * (COUPLING_Y + COUPLING[1] / 2.0 - 2.0)
            y_out = sign * (COUPLING_Y + COUPLING[1] / 2.0)
            m.tube((px, y_in, PORT_Z), (px, y_out, PORT_Z), PORT_R + 4.0, sides, frame, f"coupling_{side}_port_{k + 1:02d}", caps=True)
            m.sockets.append(kit.Socket(f"Conduit_{side.capitalize()}_{k + 1:02d}", (px, y_out, PORT_Z), 90.0 * sign,
                                        "physical conduit stub attachment; cosmetic span continues from the stub end"))

    # Redundant paired external conduits on the rear (-X) face, plinth to collar (Bible: pairs on the outside of walls).
    rear_x = -SHAFT_HALF - 11.0
    m.box((-RING_OUT_HALF + 4.0, 0.0, RING_TOP + 20.0), (28.0, 72.0, 40.0), frame, "rear_junction")
    for k, y in enumerate((-22.0, 22.0)):
        m.tube((rear_x, y, RING_TOP + 40.0), (rear_x, y, COLLAR_Z[0]), 8.0, sides, frame, f"rear_conduit_{k + 1:02d}", caps=False)
        if hi:
            for z in (300.0, 550.0, 800.0):
                m.box((-SHAFT_HALF - 8.0, y, z), (16.0, 18.0, 14.0), frame, f"rear_conduit_{k + 1:02d}")

    # Sockets for the removable panels (parts) and presentation anchors.
    for number, z in PANEL_Z.items():
        m.sockets.append(kit.Socket(f"Panel_{number}", (SHAFT_HALF, 0.0, z), 0.0, f"removable numbered ceramic access panel {number}; plate extends +X"))
    m.sockets.append(kit.Socket("Collar_Center", (0.0, 0.0, (COLLAR_Z[0] + COLLAR_Z[1]) / 2.0), 0.0, "connection state effects and the thin electrical sustain audio source"))
    m.sockets.append(kit.Socket("Cap_Top", (0.0, 0.0, CAP_TOP + SERVICE_CAP[2]), 0.0, "damage smoke / critical degradation effects"))
    m.sockets.append(kit.Socket("Bay_Center", (SHAFT_HALF, 0.0, (BAY_Z[0] + BAY_Z[1]) / 2.0), 0.0, "maintenance / damage exposure effects"))

    # Simple collision for asset inspection only (runtime presentation disables collision).
    m.collision.append(kit.CollisionBox("plinth", (0.0, 0.0, STEP_TOP / 2.0), (2 * PLINTH_HALF, 2 * PLINTH_HALF, STEP_TOP)))
    m.collision.append(kit.CollisionBox("mast", (0.0, 0.0, (STEP_TOP + CAP_TOP + SERVICE_CAP[2]) / 2.0), (2 * SHAFT_HALF + 12.0, 2 * SHAFT_HALF + 12.0, CAP_TOP + SERVICE_CAP[2] - STEP_TOP)))
    return m


# --- removable numbered panel (one mesh, four instances) --------------------------
def build_panel(lod: int) -> kit.Mesh:
    m = kit.Mesh(f"{ASSET}_Panel")
    ceramic, frame, status = m.slot(CERAMIC), m.slot(FRAME), m.slot(STATUS)
    hi = lod == 0
    # Plate: pivot at the inner-face centre; plate extends +X (outward from the shaft).
    m.box((PANEL_T / 2.0, 0.0, 0.0), (PANEL_T, PANEL_W, PANEL_H), ceramic, "panel_plate")
    if hi:
        # Recessed fastener bosses at the four corners and the numeral label plate.
        for sy in (-1.0, 1.0):
            for sz in (-1.0, 1.0):
                m.tube((PANEL_T, sy * (PANEL_W / 2.0 - 12.0), sz * (PANEL_H / 2.0 - 12.0)),
                       (PANEL_T + 1.5, sy * (PANEL_W / 2.0 - 12.0), sz * (PANEL_H / 2.0 - 12.0)), 4.5, 8, frame, "panel_fasteners", caps=True)
        m.box((PANEL_T + 0.5, -PANEL_W / 2.0 + 30.0, PANEL_H / 2.0 - 26.0), (1.0, 36.0, 20.0), frame, "panel_label")
    m.sockets.append(kit.Socket("Label", (PANEL_T, -PANEL_W / 2.0 + 30.0, PANEL_H / 2.0 - 26.0), 0.0, "numeral decal / non-color grid marking anchor"))
    del status
    return m


# --- short physical conduit stub (one mesh, four instances) -----------------------
def build_stub(lod: int) -> kit.Mesh:
    m = kit.Mesh(f"{ASSET}_ConduitStub")
    frame, status = m.slot(FRAME), m.slot(STATUS)
    hi = lod == 0
    sides = 8 if hi else 6
    r = 12.0
    # Pivot at the coupling port; the stub runs along local +X (the socket yaw turns it outward).
    m.tube((0.0, 0.0, 0.0), (STUB_LENGTH, 0.0, 0.0), r, sides, frame, "conduit_jacket", caps=False)
    m.tube((24.0, 0.0, 0.0), (38.0, 0.0, 0.0), r + 3.0, sides, frame, "conduit_clamp_ring", caps=True)
    m.tube((STUB_LENGTH - 12.0, 0.0, 0.0), (STUB_LENGTH + 2.0, 0.0, 0.0), r + 5.0, 8 if hi else 6, frame, "conduit_end_nut", caps=True)
    # Pulse strip along the jacket top: the "faintly pulsing along their length" state cue.
    m.box((52.0, 0.0, r + 1.5), (52.0, 4.0, 3.0), status, "conduit_pulse_strip")
    m.sockets.append(kit.Socket("Span_End", (STUB_LENGTH + 2.0, 0.0, 0.0), 0.0, "cosmetic span component start; carries no grid radius, collision or network authority"))
    return m


# --- assembled review geometry ----------------------------------------------------
def assemble(lod: int, state: str) -> kit.Mesh:
    main = build_main(lod)
    panel = build_panel(lod)
    stub = build_stub(lod)
    scene = kit.Mesh(f"{ASSET}_assembly_{state}_LOD{lod}")
    scene.merge(main, include_sockets=True)
    panel_sockets = {s.name: s for s in main.sockets if s.name.startswith("Panel_")}
    stub_sockets = {s.name: s for s in main.sockets if s.name.startswith("Conduit_")}
    removed = {"02", "03"} if state == "maintenance" else set()
    for number in ("01", "02", "03", "04"):
        s = panel_sockets[f"Panel_{number}"]
        if number in removed:
            continue
        scene.merge(panel, translate=s.position, yaw_deg=s.yaw_deg, component_prefix=f"panel_{number}_", include_sockets=False)
    if removed:
        # Removed panels lie flat on the ground beside the service face (GAP-02 correction).
        for k, number in enumerate(sorted(removed)):
            scene.merge(panel, translate=(300.0 + 140.0 * k, 150.0 - 260.0 * k, 0.0), yaw_deg=25.0 * (1 - k), pitch_deg=90.0,
                        component_prefix=f"panel_{number}_", include_sockets=False)
    for name, s in stub_sockets.items():
        if state == "maintenance" and name == "Conduit_Right_02":
            # Unplugged stub resting on the ground in front of the coupling.
            scene.merge(stub, translate=(70.0, 235.0, 12.0), yaw_deg=60.0, component_prefix="conduit_right_02_unplugged_", include_sockets=False)
            continue
        scene.merge(stub, translate=s.position, yaw_deg=s.yaw_deg, component_prefix=name.lower() + "_", include_sockets=False)
    return scene


# --- manifest ------------------------------------------------------------------------
def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def contract_inventory(main: kit.Mesh) -> dict:
    comps = main.components()
    return {
        "numbered_panels": {"contract": 4, "built": 4, "evidence": "four Panel_01..Panel_04 sockets on the main mesh, one panel part mesh instanced four times"},
        "collar_assembly": {"contract": 1, "built": sum(1 for c in comps if c == "collar_assembly"), "segments": sum(1 for c in comps if c.startswith("collar_segment_"))},
        "base_couplings": {"contract": 2, "built": sum(1 for c in comps if c in ("coupling_left", "coupling_right"))},
        "conduits": {"contract": 4, "built": sum(1 for s in main.sockets if s.name.startswith("Conduit_")), "evidence": "four Conduit_* sockets, one stub part mesh instanced four times; two ports per coupling"},
        "rear_paired_conduits": {"canon": "conduits run in redundant pairs on the outside of walls", "built": sum(1 for c in comps if c.startswith("rear_conduit_"))},
        "frame_rails": {"built": sum(1 for c in comps if c.startswith("frame_rail_"))},
    }


def main_cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True, help="directory for assembled review OBJs and render scenes (outside Git)")
    parser.add_argument("--check", action="store_true", help="rebuild and compare hashes against build-manifest.json without writing")
    args = parser.parse_args()

    export_dir = os.path.join(HERE, "export")
    review_dir = os.path.join(args.evidence_dir, "review")
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)

    outputs = []
    meshes = {}
    for lod in (0, 1):
        for builder in (build_main, build_panel, build_stub):
            mesh = builder(lod)
            meshes[(mesh.name, lod)] = mesh
            base = os.path.join(export_dir, f"{mesh.name}_LOD{lod}")
            glb = mesh.write_glb(base + ".glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod,
                                                        "units": "meters in file; authored centimeters", "planned_unreal_folder": PLANNED_FOLDER})
            obj = mesh.write_obj(base + ".obj", header_lines=[f"Production ID {PRODUCTION_ID}", f"Revision {REVISION}", f"LOD{lod}"])
            outputs.append({"path": os.path.relpath(base + ".glb", HERE), "sha256": glb, "lod": lod, "mesh": mesh.name, "triangles": mesh.triangle_count(),
                            "by_slot": mesh.triangle_count_by("slot"), "by_component": mesh.triangle_count_by("component"),
                            "bounds_cm": mesh.bounds(), "sockets": [{"name": s.name, "position_cm": s.position, "yaw_deg": s.yaw_deg, "purpose": s.purpose} for s in mesh.sockets],
                            "collision_boxes": [{"name": c.name, "center_cm": c.center, "size_cm": c.size} for c in mesh.collision]})
            outputs.append({"path": os.path.relpath(base + ".obj", HERE), "sha256": obj, "lod": lod, "mesh": mesh.name})

    assemblies = []
    for lod, state in ((0, "connected"), (0, "maintenance"), (1, "connected")):
        scene = assemble(lod, state)
        path = os.path.join(review_dir, f"{scene.name}.obj")
        digest = scene.write_obj(path, header_lines=[f"Assembled review geometry {state} LOD{lod}; not an export"])
        assemblies.append({"path": path, "sha256": digest, "lod": lod, "state": state, "triangles": scene.triangle_count(),
                           "by_slot": scene.triangle_count_by("slot"), "bounds_cm": scene.bounds()})

    main0, main1 = meshes[(ASSET, 0)], meshes[(ASSET, 1)]
    panel0, panel1 = meshes[(f"{ASSET}_Panel", 0)], meshes[(f"{ASSET}_Panel", 1)]
    stub0, stub1 = meshes[(f"{ASSET}_ConduitStub", 0)], meshes[(f"{ASSET}_ConduitStub", 1)]
    assembled = {
        "lod0_assembled_triangles": main0.triangle_count() + 4 * panel0.triangle_count() + 4 * stub0.triangle_count(),
        "lod1_assembled_triangles": main1.triangle_count() + 4 * panel1.triangle_count() + 4 * stub1.triangle_count(),
        "lod0_cap": 3500, "lod1_cap": 1200,
    }
    assembled["lod0_within_cap"] = assembled["lod0_assembled_triangles"] <= 3500
    assembled["lod1_within_cap"] = assembled["lod1_assembled_triangles"] <= 1200

    bounds = main0.bounds()
    footprint_check = {
        "footprint_cm": FOOTPRINT,
        "main_mesh_xy_extent_cm": [bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1]],
        "main_mesh_within_footprint": bounds[0][0] >= -FOOTPRINT[0] / 2 and bounds[1][0] <= FOOTPRINT[0] / 2 and bounds[0][1] >= -FOOTPRINT[1] / 2 and bounds[1][1] <= FOOTPRINT[1] / 2,
        "height_cm": bounds[1][2],
        "ground_contact_z_cm": bounds[0][2],
        "conduit_stub_reach_beyond_footprint_cm": STUB_LENGTH + 2.0,
        "note": "Only the cosmetic conduit stubs (and any later span component) leave the 2x2 footprint; they carry no collision, navigation or network authority (GAP-03).",
    }

    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    with open(os.path.abspath(kit.__file__), "rb") as handle:
        kit_sha = sha256_bytes(handle.read())

    manifest = {
        "author": AUTHOR, "creator": AUTHOR,
        "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "asset_name": ASSET,
        "revision": REVISION, "kit_revision": kit.KIT_REVISION,
        "stage": "BLOCKOUT",
        "stage_boundary": "Editable source geometry with review views; no textures, no Unreal import, no gate acceptance, no owner acceptance.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters", "glb": "meters (glTF spec); Interchange expected to import at 1 m = 100 cm; verify at import", "axes": "authored +X forward, +Y right, +Z up; glTF export applies glTF=(X,Z,Y)"},
        "pivot": "ground-contact centre of the 2x2 footprint (contract import_policy.pivot)",
        "front": "+X face carries the numbered panels and service bay; couplings on -Y (Left) and +Y (Right)",
        "scale_basis": {
            "tile_world_size_cm": TILE_CM,
            "tile_source": "Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h TileWorldSize = 200.0f; Public/EchoesGlassScarCompiledMapPack.h kPresentationTileWorldUnits = 200",
            "footprint_cells": [2, 2],
            "footprint_source": "Content/Data/Source/buildings.json id=mc_power_link footprint_cells [2,2]; SPEC-STR-002 / SPEC-BLD-015.MC.LINK 2x2 tiles",
            "height_status": "PROVISIONAL BLOCKOUT ESTIMATE: no authoritative height exists; ComponentDesignCatalog's 18 m and the concept sheet proportions are proposals, not approved dimensions",
            "noted_discrepancy": "Requirements SPEC-SKM-011 says '64x64 tiles at 100 cm simulation scale'; presentation places tiles at 200 cm. The mesh follows the presentation tile constant the runtime uses to place structures.",
        },
        "dimensions_cm_provisional": {
            "plinth": [2 * PLINTH_HALF, 2 * PLINTH_HALF, PLINTH_TOP], "plinth_step_top_z": STEP_TOP, "foundation_ring": [2 * RING_OUT_HALF, 2 * RING_OUT_HALF, RING_TOP - STEP_TOP],
            "shaft": [2 * SHAFT_HALF, 2 * SHAFT_HALF, SHAFT_TOP - RING_TOP], "shaft_chamfer": SHAFT_CHAMFER, "collar_z": COLLAR_Z, "collar_outer_width": 2 * COLLAR_OUT_HALF,
            "cap_top_z": CAP_TOP + SERVICE_CAP[2], "panel": [PANEL_T, PANEL_W, PANEL_H], "panel_centres_z": PANEL_Z, "service_bay_z": BAY_Z,
            "coupling": COUPLING, "coupling_centre_y": COUPLING_Y, "port_radius": PORT_R, "conduit_stub_length": STUB_LENGTH,
        },
        "material_slots": [CERAMIC, FRAME, STATUS],
        "material_slot_policy": "3 provisional slots (contract material_slots_provisional_max 3); status slot is a state-masked emissive under the 15% visible-area ceiling",
        "component_inventory": contract_inventory(main0),
        "budgets": assembled,
        "footprint_check": footprint_check,
        "outputs": outputs,
        "review_assemblies": assemblies,
        "tools": {"builder": os.path.relpath(os.path.abspath(__file__), HERE), "builder_sha256": builder_sha, "meshkit": "../tools/ebs_meshkit.py", "meshkit_sha256": kit_sha,
                  "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "candidate_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/power-link-review/power-link-maintenance-complete.png", "sha256": "b3816b9e6ec6f56629fc73c29f461187df4274b8f3514d637f9c9d48a883bf20"},
            "construction_reference": {"path": "Docs/VisualAssetPipeline/motion/construction/power-link.svg", "sha256": "c8abc04a65a0dee8d97f40c18ebd10ae8c2b203af7b2898afc9273796c1bbc7c"},
            "component_geometry": {"path": "Docs/VisualAssetPipeline/motion/construction/component-geometry.json", "sha256": "fcec87e723886479c08cec0c653513be8960267110d7c8bc7e07a1cf3875554c"},
            "canon_row": {"path": "Docs/Archive/DevelopmentBible.md", "line": 514, "file_sha256": "e237a4e16fa5d6da082395ca94ed1479ed709d1d8d5937a21a5858733c7cfe8c"},
            "book": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/book-source.docx", "sha256": "994e7df5d37a6ef1532c65a7b742d71e81b4ea7752311f5e5acc2e04c817ef83", "paragraphs": [135, 169]},
            "gameplay_record": {"path": "Content/Data/Source/buildings.json", "sha256": "aba1b64b20bb7c533aa7f7487839b12fe38133b96636d3b2e67e83c5ae6ed26e", "id": "mc_power_link"},
            "contract": {"path": "Docs/VisualAssetPipeline/motion/gap-decisions.json", "package_id": PACKAGE_ID},
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }
    manifest_path = os.path.join(HERE, "build-manifest.json")
    text = json.dumps(manifest, indent=1, sort_keys=True) + "\n"
    if args.check and os.path.exists(manifest_path):
        with open(manifest_path, "r", encoding="utf-8") as handle:
            previous = json.load(handle)
        drift = [o["path"] for o, p in zip(manifest["outputs"], previous.get("outputs", [])) if o["sha256"] != p.get("sha256")]
        print(json.dumps({"check": "ok" if not drift else "drift", "drift": drift}))
        return 0 if not drift else 3
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    print(json.dumps({"revision": REVISION, "budgets": assembled, "footprint": footprint_check["main_mesh_within_footprint"],
                      "outputs": len(outputs), "manifest": manifest_path}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
