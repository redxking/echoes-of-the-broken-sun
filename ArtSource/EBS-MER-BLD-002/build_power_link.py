#!/usr/bin/env python3
"""EBS-MER-BLD-002 Power Link: deterministic editable source geometry (blockout, concept-v3).

Author: Angelis Pseftis.

Authoritative look: concept-fidelity.md in this folder (owner ruling 2026-09-06: the
selected concept images define what the asset looks like; gameplay rules bound the
concept but never replace it). Every dimension below is a measurement taken from the
candidate concept (power-link-candidate.png, cross-checked with concepts A and B) and
scaled to the 2x2 footprint; where a rule forced a change it is recorded in README §8.

Package contract: Docs/VisualAssetPipeline/motion/gap-decisions.json
  production_policy[package_id == "EBS-PKG-MC-POWER-LINK"], gaps GAP-01..03.
Canon row: Docs/Archive/DevelopmentBible.md line 514 (SPEC-BLD-015.MC.LINK).
Gameplay source: Content/Data/Source/buildings.json id=mc_power_link (2x2 cells).
Presentation tile: Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h
  TileWorldSize = 200 cm  ->  footprint 400 x 400 cm.

Concept proportion (fidelity doc §Proportion, re-measured on the candidate pixels for
concept-v3): plinth 360 cm = 1.8 W -> shaft width W = 200 cm; cap top H = 3.9 W = 780 cm
(measured band 3.5-4.2 W, target 3.8-4.0 W); octagonal section (four wide faces, four
narrow chamfers); four numbered panels 01 above / 02-04 below one broad three-course load
collar at 0.72 H; low octagonal cap with a square fastener plate; two-step charcoal plinth
with two coupling blocks at the front corners and paired conduit stubs (Ø 0.16 W) that end
at the footprint edge.

The component inventory is the contract's: 4 numbered panels, 1 collar assembly,
2 base couplings, 4 conduits.

Outputs (run from anywhere):
  export/SM_EBS_MER_BLD_002_LOD{0,1}.{glb,obj}          main static mesh
  export/SM_EBS_MER_BLD_002_Panel_LOD{0,1}.{glb,obj}    removable numbered panel (x4 instances)
  export/SM_EBS_MER_BLD_002_ConduitStub_LOD{0,1}.{glb,obj}  short physical conduit stub (x4 instances)
  build-manifest.json                                   counts, budgets, sockets, hashes
  bake-manifest.json                                    unique UV0 atlas for the texture baker
  <evidence>/review/*.obj                               assembled review geometry (not tracked)

Regeneration under an unchanged REVISION must be byte-identical; the manifest
records every output hash so that can be checked (--check rebuilds into a temporary
directory and compares, it writes nothing into the package or the evidence directory).
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
import ebs_meshkit as kit  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-MC-POWER-LINK"
PRODUCTION_ID = "EBS-MER-BLD-002"
ASSET = "SM_EBS_MER_BLD_002"
# concept-v3: proportion re-derived from the candidate pixels (README §6.4 / §8.1 D10): W 200 / H 780
# (3.9 W) instead of 180 / 850 (4.72 W); collar 1.35 W; panels 124 x 135 (near-square as painted);
# conduit Ø 32 = 0.16 W measured against the across-flats width; couplings 0.9 x 0.37 x 0.5 W on the
# lower step, hanging 26 cm past the plinth-top chamfer at the front; pale ceramic insulator rings and
# end nuts on the stubs and a pale cross-bar / clamp rings in the open bay so they separate from the
# charcoal; the maintenance hanging conduit clears the step edge and reaches the ground through a
# second, chained instance of the same stub part (README §8.1 D3); --check made read-only.
# concept-v2: pixel pass against the comparison sheets: stubs thickened, clamp rings, larger ports,
# couplings to the fidelity size, numeral cell centred, chamfer rails narrowed, collar strips widened.
# concept-v1: rebuilt to the concept images (concept-fidelity.md) instead of the 1,238 cm / 150 cm
# construction-reference blockout.  v3 and earlier: construction-reference proportions (README §4).
REVISION = "ebs-mer-bld-002-concept-v3"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_002/"

# Material slots (provisional max 3 per contract): index order is the slot order.
CERAMIC = "MI_EBS_MER_CeramicCivic"   # pale civic ceramic skin (existing T_EchoesCeramicCivic family)
FRAME = "MI_EBS_MER_CompactFrame"     # charcoal structural frame / machined metal
STATUS = "MI_EBS_MER_StatusCyan"      # cyan status collars, indicator strips, conduit pulse (state-masked emissive)
# Review-only material: the numeral label cell is written under this name in the assembled REVIEW OBJs
# only, so the untextured review renders show the pale label the bake paints (the export keeps the cell
# on the FRAME slot, which is the slot family the baker paints the numerals under).
REVIEW_LABEL = "REVIEW_LabelCell"

# --- concept measurements scaled to the footprint (cm) ------------------------------
TILE_CM = 200.0
FOOTPRINT = (2 * TILE_CM, 2 * TILE_CM)
SHAFT_W = 200.0                       # W: shaft width across the wide faces (plinth 360 = 1.8 W as measured)
HEIGHT = 780.0                        # H: cap top = 3.90 W (fidelity target 3.8-4.0 W, measured band 3.5-4.2 W)
SHAFT_HALF, SHAFT_CHAMFER = SHAFT_W / 2.0, 35.0   # wide faces 130, chamfer faces 49.5 (0.25 W): octagon read

# Base: two-step charcoal plinth (fidelity §6). Lower step 360 across with a sloped skirt,
# upper step 260 across (1.3 W); total 92 = 0.118 H (candidate profile: ~0.08-0.12 H of vertical face).
PLINTH_HALF, PLINTH_CHAMFER = 180.0, 40.0
SKIRT_TOP_HALF, SKIRT_TOP_CHAMFER, SKIRT_TOP_Z = 172.0, 38.0, 25.0
PLINTH_TOP = 50.0
STEP_HALF, STEP_CHAMFER, STEP_TOP = 130.0, 37.0, 92.0
# Charcoal frame at the shaft foot: four corner brackets on the chamfer faces.
BRACKET = (14.0, 30.0, 80.0)          # radial depth, width along the chamfer, height
BRACKET_Z = (STEP_TOP, STEP_TOP + BRACKET[2])
# Coupling blocks (fidelity: 0.9 W x 0.5 W x 0.5 W) at the front-left / front-right plinth corners.
# Depth is 0.37 W, not 0.5 W: the outer face is pinned at |y| 140 by the stub bound below and the inner
# face must clear panel 04 (README §8.1 D8).
COUPLING = (180.0, 74.0, 100.0)       # X (across the port pair), Y (depth), Z = 0.9 / 0.37 / 0.5 W
COUPLING_X = 102.0                    # centre; spans x 12..192: hangs past the plinth-top edge / chamfer as painted
STUB_LENGTH = 56.0                    # jacket length; the end nut adds 2 cm
STUB_R = 16.0                         # conduit radius: diameter 32 = 0.16 W (measured on the candidate, README §8)
STUB_RINGS = ((8.0, 16.0), (34.0, 42.0))   # clamp rings: the candidate's segmented jacket
RING_R = STUB_R + 3.0
NUT_R = STUB_R + 5.0
PORT_R = 20.0                         # port collar radius on the coupling face (diameter 40 > conduit 32)
COUPLING_Y_OUT = FOOTPRINT[1] / 2.0 - STUB_LENGTH - 4.0   # outer face at 140: port (+2) + stub + nut (+2) = footprint edge
COUPLING_Y = COUPLING_Y_OUT - COUPLING[1] / 2.0            # centre 103; spans y 66..140
COUPLING_Z = (PLINTH_TOP, PLINTH_TOP + COUPLING[2])        # sits on the lower step, 50..150
PORT_DX, PORT_Z = 34.0, 100.0                              # ports at x 68 / 136, z 100 (face centre)
PORT_Y = COUPLING_Y_OUT + 2.0                              # port cap 2 cm proud of the coupling face (socket)
LAMP = (52.0, 4.0, 5.0)                                    # port status lamp on the outer face
LAMP_Z = COUPLING_Z[1] - 12.0                              # 138: clear of the port collars (80..120)

# Shaft: pale ceramic octagon from the upper step to the cap, split at the collar.
SHAFT_Z0 = STEP_TOP
COLLAR_Z = (532.0, 598.0)             # 0.682-0.767 H; centre 565 = 0.724 H (fidelity 0.70-0.74 H)
COLLAR_HALF, COLLAR_CHAMFER = 135.0, 45.0   # 270 across = 1.35 W (fidelity ~1.35 W)
COLLAR_COURSES = [(532.0, 550.0), (554.0, 576.0), (580.0, 598.0)]   # concept A's three rings as courses
COLLAR_GROOVES = [(550.0, 554.0), (576.0, 580.0)]
GROOVE_HALF, GROOVE_CHAMFER = 130.0, 43.0
STRIP = (8.0, 106.0, 20.0)            # cyan strip: radial depth, width along the face (0.59 of the 180 face), height
STRIP_Z = 565.0
TEAM_BAND_Z = (521.0, 529.0)          # narrow ceramic band under the collar (team mask carrier)
TEAM_BAND_HALF, TEAM_BAND_CHAMFER = 102.0, 35.0
SHAFT_TOP = 752.0
CAP_HALF, CAP_CHAMFER, CAP_TOP = 109.0, 36.0, 774.0   # low octagonal cap plate (218 across = 1.09 W)
CAP_PLATE = (60.0, 60.0, HEIGHT - CAP_TOP)           # small square fastener plate; top = H
# Chamfer rails (the "octagonal reinforced hull" ribs): proud charcoal strips on the four chamfers.
RAIL = (4.0, 14.0)                    # radial depth, width along the 49.5 cm chamfer (pale ceramic shows either side)
RAIL_Z_LOWER = (BRACKET_Z[1] + 5.0, TEAM_BAND_Z[0] - 5.0)
RAIL_Z_UPPER = (COLLAR_Z[1] + 10.0, SHAFT_TOP - 6.0)

# Removable numbered panels on the +X service face: 01 above the collar, 02-04 below.
PANEL_W, PANEL_H, PANEL_T = 124.0, 135.0, 6.0        # 135 = 0.173 H; 124 x 135 = 0.92 aspect (candidate ~1.0); fits the 130 flat face
LABEL = (56.0, 44.0)                                  # numeral cell, centred on the plate (candidate: large centred numerals)
LABEL_Z = 8.0                                         # cell centre sits a little above the plate centre as painted
PANEL_Z = {"01": 678.0, "02": 447.0, "03": 306.0, "04": 165.0}   # centres; 6 cm seams between plates
BAY_Z = (PANEL_Z["03"] - PANEL_H / 2.0, PANEL_Z["02"] + PANEL_H / 2.0)   # opening behind panels 02 and 03 (238.5..514.5)
BAY_HALF_W, BAY_DEPTH = 50.0, 30.0
BAY_TUBE_R = 7.0
BAY_RING_R = BAY_TUBE_R + 3.0


def outline_octagon(half, chamfer):
    return kit.octagon(half, chamfer)


def _chamfer_dirs():
    """(sx, sy) of the four chamfer faces; the chamfer plane normal is (sx, sy)/sqrt2."""
    return ((1, 1), (-1, 1), (-1, -1), (1, -1))


def chamfer_distance(half, chamfer):
    """Distance from the axis to a chamfer face of kit.octagon(half, chamfer)."""
    return (2.0 * half - chamfer) / math.sqrt(2.0)


def octagon_overhang(x, y, half, chamfer):
    """How far (cm) the point (x, y) lies outside kit.octagon(half, chamfer); <= 0 when inside.
    The outline is the intersection of |x| <= half, |y| <= half and |x| + |y| <= 2 half - chamfer."""
    ax, ay = abs(x), abs(y)
    return max(ax - half, ay - half, (ax + ay - (2.0 * half - chamfer)) / math.sqrt(2.0))


def add_frustum(m, outline0, z0, outline1, z1, slot, component):
    """Sloped side walls between two outlines with equal vertex counts (no caps)."""
    n = len(outline0)
    cx = sum(p[0] for p in outline0) / n
    cy = sum(p[1] for p in outline0) / n
    for i in range(n):
        j = (i + 1) % n
        face = [(outline0[i][0], outline0[i][1], z0), (outline0[j][0], outline0[j][1], z0),
                (outline1[j][0], outline1[j][1], z1), (outline1[i][0], outline1[i][1], z1)]
        mid = ((outline0[i][0] + outline0[j][0]) / 2.0 - cx, (outline0[i][1] + outline0[j][1]) / 2.0 - cy, 0.0)
        m.add_polygon(face, slot, component, normal=mid)


def add_face_quad(m, half, chamfer, z0, z1, slot, component):
    """The +X wide face of kit.octagon(half, chamfer) between z0 and z1 (normal +X)."""
    y = half - chamfer
    m.add_polygon([(half, -y, z0), (half, y, z0), (half, y, z1), (half, -y, z1)], slot, component, normal=(1.0, 0.0, 0.0))


def add_recess(m, x_back, x_open, half_w, z0, z1, slot, component):
    """Open-fronted bay on the +X face: walls face INTO the cavity so a one-sided material shows them."""
    m.add_polygon([(x_back, -half_w, z0), (x_back, half_w, z0), (x_back, half_w, z1), (x_back, -half_w, z1)], slot, component, normal=(1.0, 0.0, 0.0))
    m.add_polygon([(x_back, -half_w, z0), (x_open, -half_w, z0), (x_open, half_w, z0), (x_back, half_w, z0)], slot, component, normal=(0.0, 0.0, 1.0))
    m.add_polygon([(x_back, -half_w, z1), (x_open, -half_w, z1), (x_open, half_w, z1), (x_back, half_w, z1)], slot, component, normal=(0.0, 0.0, -1.0))
    m.add_polygon([(x_back, -half_w, z0), (x_open, -half_w, z0), (x_open, -half_w, z1), (x_back, -half_w, z1)], slot, component, normal=(0.0, 1.0, 0.0))
    m.add_polygon([(x_back, half_w, z0), (x_open, half_w, z0), (x_open, half_w, z1), (x_back, half_w, z1)], slot, component, normal=(0.0, -1.0, 0.0))


def add_stud(m, base, radius, height, sides, slot, component):
    """Hex/oct fastener boss standing on a +X face: side walls and the outer cap only."""
    bx, by, bz = base
    ring0, ring1 = [], []
    for k in range(sides):
        a = 2.0 * math.pi * k / sides + math.pi / sides
        ring0.append((bx, by + radius * math.cos(a), bz + radius * math.sin(a)))
        ring1.append((bx + height, by + radius * math.cos(a), bz + radius * math.sin(a)))
    for k in range(sides):
        j = (k + 1) % sides
        mid = ((ring0[k][1] + ring0[j][1]) / 2.0 - by, (ring0[k][2] + ring0[j][2]) / 2.0 - bz)
        m.add_polygon([ring0[k], ring0[j], ring1[j], ring1[k]], slot, component, normal=(0.0, mid[0], mid[1]))
    m.add_polygon(list(ring1), slot, component, normal=(1.0, 0.0, 0.0))


def add_chamfer_box(m, sx, sy, radial_centre, size, z0, z1, slot, component, skip=()):
    """Box pressed against a chamfer face: local +X points outward along (sx, sy)/sqrt2."""
    yaw = math.degrees(math.atan2(sy, sx))
    c = radial_centre / math.sqrt(2.0)
    m.box((sx * c, sy * c, (z0 + z1) / 2.0), (size[0], size[1], z1 - z0), slot, component, yaw_deg=yaw, skip=skip)


# --- main static mesh -----------------------------------------------------------
def build_main(lod: int) -> kit.Mesh:
    m = kit.Mesh(ASSET)
    ceramic, frame, status = m.slot(CERAMIC), m.slot(FRAME), m.slot(STATUS)
    hi = lod == 0
    sides = 8 if hi else 6

    # Base: lower step with a sloped skirt, upper step (both charcoal; fidelity §6 "charcoal frame at the base").
    add_frustum(m, outline_octagon(PLINTH_HALF, PLINTH_CHAMFER), 0.0, outline_octagon(SKIRT_TOP_HALF, SKIRT_TOP_CHAMFER), SKIRT_TOP_Z, frame, "plinth")
    m.prism(outline_octagon(SKIRT_TOP_HALF, SKIRT_TOP_CHAMFER), SKIRT_TOP_Z, PLINTH_TOP, frame, "plinth", cap_bottom=False)
    m.prism(outline_octagon(STEP_HALF, STEP_CHAMFER), PLINTH_TOP, STEP_TOP, frame, "plinth_step", cap_bottom=False)
    # Corner brackets on the four shaft chamfers (the charcoal frame the ceramic shaft stands in).
    for k, (sx, sy) in enumerate(_chamfer_dirs()):
        add_chamfer_box(m, sx, sy, chamfer_distance(SHAFT_HALF, SHAFT_CHAMFER) + BRACKET[0] / 2.0, BRACKET, BRACKET_Z[0], BRACKET_Z[1],
                        frame, f"frame_bracket_{k + 1:02d}", skip=("-X", "-Z"))

    # Shaft below the collar: octagonal ceramic prism; the +X wide face is a charcoal backing quad so
    # the seams between the pale panels read dark, and it is open where the service bay sits.
    octagon = outline_octagon(SHAFT_HALF, SHAFT_CHAMFER)
    m.prism(octagon, SHAFT_Z0, COLLAR_Z[0], ceramic, "shaft_lower", cap_bottom=False, cap_top=False, skip_edges=(2,))
    add_face_quad(m, SHAFT_HALF, SHAFT_CHAMFER, SHAFT_Z0, BAY_Z[0], frame, "shaft_lower")
    add_face_quad(m, SHAFT_HALF, SHAFT_CHAMFER, BAY_Z[1], COLLAR_Z[0], frame, "shaft_lower")
    # Service bay behind panels 02 and 03 (GAP-02; both concept maintenance views show a two-panel opening).
    bay_x0 = SHAFT_HALF - BAY_DEPTH
    add_recess(m, bay_x0, SHAFT_HALF, BAY_HALF_W, BAY_Z[0], BAY_Z[1], frame, "service_bay")
    bay_x = bay_x0 + 12.0
    bay_mid = (BAY_Z[0] + BAY_Z[1]) / 2.0
    if hi:
        # Internal conduit bundle: three vertical charcoal tubes with pale ceramic clamp rings and a pale
        # cross-bar (the candidate's fitted bundle reads against the dark recess), visible only with the panels off.
        for y in (-28.0, 0.0, 28.0):
            m.tube((bay_x, y, BAY_Z[0] + 8.0), (bay_x, y, BAY_Z[1] - 8.0), BAY_TUBE_R, sides, frame, "service_bay_conduits", caps=True)
            for zc in (BAY_Z[0] + 8.0 + 0.25 * (BAY_Z[1] - BAY_Z[0] - 16.0), BAY_Z[1] - 8.0 - 0.25 * (BAY_Z[1] - BAY_Z[0] - 16.0)):
                m.tube((bay_x, y, zc - 5.0), (bay_x, y, zc + 5.0), BAY_RING_R, sides, ceramic, "service_bay_fittings", caps=True)
        m.box((bay_x, 0.0, bay_mid), (14.0, 84.0, 14.0), ceramic, "service_bay_fittings")
    else:
        m.box((bay_x, 0.0, bay_mid), (14.0, 84.0, BAY_Z[1] - BAY_Z[0] - 16.0), frame, "service_bay_conduits")

    # Chamfer rails below and above the collar (dark ribs of the reinforced hull).
    for k, (sx, sy) in enumerate(_chamfer_dirs()):
        add_chamfer_box(m, sx, sy, chamfer_distance(SHAFT_HALF, SHAFT_CHAMFER) + RAIL[0] / 2.0, RAIL, RAIL_Z_LOWER[0], RAIL_Z_LOWER[1],
                        frame, f"frame_rail_{k + 1:02d}", skip=("-X",))
        if hi:
            add_chamfer_box(m, sx, sy, chamfer_distance(SHAFT_HALF, SHAFT_CHAMFER) + RAIL[0] / 2.0, RAIL, RAIL_Z_UPPER[0], RAIL_Z_UPPER[1],
                            frame, f"frame_rail_{k + 1:02d}", skip=("-X",))

    # Team band: narrow ceramic band just under the collar; ownership colour arrives through the
    # texture's team mask (REL-ART-028), no fourth material slot.
    m.prism(outline_octagon(TEAM_BAND_HALF, TEAM_BAND_CHAMFER), TEAM_BAND_Z[0], TEAM_BAND_Z[1], ceramic, "team_band")

    # Collar assembly: one broad octagonal collar in three courses with two grooves (concept A's three
    # rings read as courses), four cyan conductor strips on the wide faces (contract: 1 collar assembly).
    courses = COLLAR_COURSES if hi else [(COLLAR_Z[0], COLLAR_Z[1])]
    for z0, z1 in courses:
        m.prism(outline_octagon(COLLAR_HALF, COLLAR_CHAMFER), z0, z1, frame, "collar_assembly")
    if hi:
        for z0, z1 in COLLAR_GROOVES:
            m.prism(outline_octagon(GROOVE_HALF, GROOVE_CHAMFER), z0, z1, frame, "collar_assembly", cap_bottom=False, cap_top=False)
    for k, yaw in enumerate((0.0, 90.0, 180.0, 270.0)):
        r = COLLAR_HALF + STRIP[0] / 2.0 - 2.0
        p = kit.rot_z((r, 0.0, 0.0), yaw)
        m.box((p[0], p[1], STRIP_Z), (STRIP[0], STRIP[1], STRIP[2]), status, f"collar_segment_{k + 1:02d}", yaw_deg=yaw, skip=("-X",))

    # Shaft above the collar (panel 01) and the cap: low octagonal plate with a square fastener plate,
    # no crystal, no antenna (fidelity §5).
    m.prism(octagon, COLLAR_Z[1], SHAFT_TOP, ceramic, "shaft_upper", cap_bottom=False, cap_top=False, skip_edges=(2,))
    add_face_quad(m, SHAFT_HALF, SHAFT_CHAMFER, COLLAR_Z[1], SHAFT_TOP, frame, "shaft_upper")
    m.prism(outline_octagon(CAP_HALF, CAP_CHAMFER), SHAFT_TOP, CAP_TOP, frame, "cap")
    m.box((0.0, 0.0, CAP_TOP + CAP_PLATE[2] / 2.0), CAP_PLATE, ceramic, "cap", skip=("-Z",))

    # Base couplings (contract: 2) at the front-left (-Y) and front-right (+Y) plinth corners, two ports each.
    # Closed boxes (the underside faces the skirt slope where the block hangs past the plinth-top edge).
    for side, sign in (("left", -1.0), ("right", 1.0)):
        cy = sign * COUPLING_Y
        m.box((COUPLING_X, cy, (COUPLING_Z[0] + COUPLING_Z[1]) / 2.0), COUPLING, frame, f"coupling_{side}")
        if hi:
            # Port status lamp on the outer face above the ports (candidate: small cyan tell-tale on the coupling).
            m.box((COUPLING_X, sign * (COUPLING_Y_OUT + LAMP[1] / 2.0), LAMP_Z), LAMP, status, f"coupling_{side}",
                  skip=("+Y",) if sign < 0 else ("-Y",))
        for k, px in enumerate((COUPLING_X - PORT_DX, COUPLING_X + PORT_DX)):
            y_in = sign * (COUPLING_Y_OUT - 6.0)
            y_out = sign * PORT_Y
            m.tube((px, y_in, PORT_Z), (px, y_out, PORT_Z), PORT_R, sides, frame, f"coupling_{side}_port_{k + 1:02d}", caps=True)
            m.sockets.append(kit.Socket(f"Conduit_{side.capitalize()}_{k + 1:02d}", (px, y_out, PORT_Z), 90.0 * sign,
                                        "physical conduit stub attachment; the stub ends at the footprint edge and the cosmetic span continues from Span_End"))

    # Sockets for the removable panels (parts) and presentation anchors (names are the contract's).
    for number, z in PANEL_Z.items():
        m.sockets.append(kit.Socket(f"Panel_{number}", (SHAFT_HALF, 0.0, z), 0.0, f"removable numbered ceramic access panel {number}; plate extends +X"))
    m.sockets.append(kit.Socket("Collar_Center", (0.0, 0.0, (COLLAR_Z[0] + COLLAR_Z[1]) / 2.0), 0.0, "connection state effects and the thin electrical sustain audio source"))
    m.sockets.append(kit.Socket("Cap_Top", (0.0, 0.0, HEIGHT), 0.0, "damage smoke / critical degradation effects"))
    m.sockets.append(kit.Socket("Bay_Center", (SHAFT_HALF, 0.0, (BAY_Z[0] + BAY_Z[1]) / 2.0), 0.0, "maintenance / damage exposure effects"))

    # Simple collision for asset inspection only (runtime presentation disables collision).
    plinth_x1 = COUPLING_X + COUPLING[0] / 2.0 + 6.0
    m.collision.append(kit.CollisionBox("plinth", ((plinth_x1 - PLINTH_HALF - 6.0) / 2.0, 0.0, COUPLING_Z[1] / 2.0),
                                        (plinth_x1 + PLINTH_HALF + 6.0, 2 * PLINTH_HALF + 12.0, COUPLING_Z[1])))
    m.collision.append(kit.CollisionBox("mast", (0.0, 0.0, (STEP_TOP + HEIGHT) / 2.0), (2 * SHAFT_HALF + 12.0, 2 * SHAFT_HALF + 12.0, HEIGHT - STEP_TOP)))
    return m


# --- removable numbered panel (one mesh, four instances) --------------------------
def build_panel(lod: int) -> kit.Mesh:
    m = kit.Mesh(f"{ASSET}_Panel")
    ceramic, frame = m.slot(CERAMIC), m.slot(FRAME)
    hi = lod == 0
    # Plate: pivot at the inner-face centre; plate extends +X (outward from the shaft).
    m.box((PANEL_T / 2.0, 0.0, 0.0), (PANEL_T, PANEL_W, PANEL_H), ceramic, "panel_plate")
    if hi:
        # Fastener bosses at the four corners and the numeral cell centred on the plate.
        for sy in (-1.0, 1.0):
            for sz in (-1.0, 1.0):
                add_stud(m, (PANEL_T, sy * (PANEL_W / 2.0 - 10.0), sz * (PANEL_H / 2.0 - 10.0)), 4.0, 1.5, 6, frame, "panel_fasteners")
        # The cell stays on the FRAME slot in the export: ebs_texbake paints the pale label and the dark
        # numerals under the compact_metal family (rule panel_label_numerals). Review OBJs remap it (assemble()).
        m.box((PANEL_T + 0.5, 0.0, LABEL_Z), (1.0, LABEL[0], LABEL[1]), frame, "panel_label", skip=("-X",))
        for poly in m.polygons:
            if poly.component == "panel_label" and poly.normal[0] > 0.99:
                poly.atlas_cells = 4  # numeral strip 01..04; the material instance selects the cell
    m.sockets.append(kit.Socket("Label", (PANEL_T, 0.0, LABEL_Z), 0.0, "numeral decal / non-color grid marking anchor"))
    return m


# --- short physical conduit stub (one mesh, four instances) -----------------------
def build_stub(lod: int) -> kit.Mesh:
    m = kit.Mesh(f"{ASSET}_ConduitStub")
    frame, status, ceramic = m.slot(FRAME), m.slot(STATUS), m.slot(CERAMIC)
    hi = lod == 0
    sides = 8 if hi else 6
    r = STUB_R
    # Pivot at the coupling port; the stub runs along local +X (the socket yaw turns it outward)
    # and its end nut lands exactly on the footprint edge. Clamp rings and the end nut are pale ceramic
    # insulators (concept B "pale ceramic insulators") so the segmented jacket and an unplugged stub
    # separate from the charcoal plinth at the game framing.
    m.tube((0.0, 0.0, 0.0), (STUB_LENGTH, 0.0, 0.0), r, sides, frame, "conduit_jacket", caps=False)
    if hi:
        for x0, x1 in STUB_RINGS:   # segmented jacket: two clamp rings as painted
            m.tube((x0, 0.0, 0.0), (x1, 0.0, 0.0), RING_R, sides, ceramic, "conduit_clamp_ring", caps=True)
    m.tube((STUB_LENGTH - 10.0, 0.0, 0.0), (STUB_LENGTH + 2.0, 0.0, 0.0), NUT_R, sides, ceramic, "conduit_end_nut", caps=True)
    # Pulse strip along the jacket top between the clamp rings: the "conduits pulsing" connected cue.
    m.box((25.0, 0.0, r + 1.0), (12.0, 5.0, 2.0), status, "conduit_pulse_strip", skip=("-Z",))
    m.sockets.append(kit.Socket("Span_End", (STUB_LENGTH + 2.0, 0.0, 0.0), 0.0, "cosmetic span component start; carries no grid radius, collision or network authority"))
    return m


# --- assembled review geometry ----------------------------------------------------
MAINTENANCE_REMOVED = ("02", "03")   # both concept maintenance views open the two-panel bay
MAINTENANCE_HANGING = "Conduit_Right_02"
# The unplugged stub droops from its port at -41 deg: steep enough to read as hanging, shallow enough
# that its jacket clears the lower-step top (z 50) before passing the plinth-top edge at |y| 172
# (a steeper single stub ends INSIDE the plinth, which is what hid the concept-v2 hang at -70 deg).
MAINTENANCE_HANG_PITCH = -41.0
# A second instance of the same stub part is chained from the hanging stub's Span_End (the span segment
# the runtime otherwise draws from that socket) so the cable reaches the ground beside the plinth
# corner; the world yaw turns it back toward +X / -Y so the end nut stays inside the 2x2 footprint and
# clear of the skirt. The angles are the tightest of a deterministic search (README §8.1 D3): the nut's
# lowest rim ends ~6.6 cm above the ground, 0.8 cm clear of the skirt slope, max |coordinate| 199.9 cm.
MAINTENANCE_SPAN_YAW = -28.0
MAINTENANCE_SPAN_PITCH = -44.0
STUB_END = STUB_LENGTH + 2.0


def assemble(lod: int, state: str) -> kit.Mesh:
    main = build_main(lod)
    panel = build_panel(lod)
    stub = build_stub(lod)
    scene = kit.Mesh(f"{ASSET}_assembly_{state}_LOD{lod}")
    scene.merge(main, include_sockets=True)
    panel_sockets = {s.name: s for s in main.sockets if s.name.startswith("Panel_")}
    stub_sockets = {s.name: s for s in main.sockets if s.name.startswith("Conduit_")}
    removed = set(MAINTENANCE_REMOVED) if state == "maintenance" else set()
    for number in ("01", "02", "03", "04"):
        s = panel_sockets[f"Panel_{number}"]
        if number in removed:
            continue
        scene.merge(panel, translate=s.position, yaw_deg=s.yaw_deg, component_prefix=f"panel_{number}_", include_sockets=False)
    if removed:
        # Panel 02 leans against the front face of the right coupling (candidate sheet); panel 03 lies
        # flat on the ground beside it (maintenance-complete sheet). Both are cosmetic debris outside the
        # 2x2 footprint (README §8.1 D11): the plinth leaves only 20 cm of footprint margin.
        lean = 35.0
        top = PANEL_H / 2.0
        pivot_x = COUPLING_X + COUPLING[0] / 2.0 + top * math.sin(math.radians(lean))
        scene.merge(panel, translate=(pivot_x, COUPLING_Y, top * math.cos(math.radians(lean)) + 1.0), yaw_deg=0.0, pitch_deg=lean,
                    component_prefix="panel_02_", include_sockets=False)
        scene.merge(panel, translate=(270.0, -60.0, 1.0), yaw_deg=-20.0, pitch_deg=90.0, component_prefix="panel_03_", include_sockets=False)
    for name, s in stub_sockets.items():
        if state == "maintenance" and name == MAINTENANCE_HANGING:
            # Unplugged conduit still hangs from its coupling port (GAP-01: the connector remains
            # physically traceable to its coupling) and reaches the ground through the chained span segment.
            prefix = name.lower() + "_hanging_"
            scene.merge(stub, translate=s.position, yaw_deg=s.yaw_deg, pitch_deg=MAINTENANCE_HANG_PITCH, component_prefix=prefix, include_sockets=False)
            span_start = kit.v_add(kit.rot_yp((STUB_END, 0.0, 0.0), s.yaw_deg, MAINTENANCE_HANG_PITCH), s.position)
            scene.merge(stub, translate=span_start, yaw_deg=MAINTENANCE_SPAN_YAW, pitch_deg=MAINTENANCE_SPAN_PITCH,
                        component_prefix=name.lower() + "_hanging_span_", include_sockets=False)
            continue
        scene.merge(stub, translate=s.position, yaw_deg=s.yaw_deg, component_prefix=name.lower() + "_", include_sockets=False)
    # Review-only material for the numeral cells (see REVIEW_LABEL): the export keeps them on FRAME.
    review_label = scene.slot(REVIEW_LABEL)
    for poly in scene.polygons:
        if poly.component.endswith("panel_label"):
            poly.slot = review_label
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
        "frame_rails": {"built": sum(1 for c in comps if c.startswith("frame_rail_"))},
        "frame_brackets": {"built": sum(1 for c in comps if c.startswith("frame_bracket_"))},
    }


def coupling_overhang() -> dict:
    """Overhang of the coupling blocks past the plinth-top outline (cm), measured against the octagon."""
    x1 = COUPLING_X + COUPLING[0] / 2.0
    return {
        "x_axis_past_plinth_top_edge": round(x1 - SKIRT_TOP_HALF, 1),
        "x_axis_past_plinth_ground_edge": round(x1 - PLINTH_HALF, 1),
        "outer_corner_past_plinth_top_chamfer": round(octagon_overhang(x1, COUPLING_Y_OUT, SKIRT_TOP_HALF, SKIRT_TOP_CHAMFER), 1),
        "outer_corner_past_plinth_ground_chamfer": round(octagon_overhang(x1, COUPLING_Y_OUT, PLINTH_HALF, PLINTH_CHAMFER), 1),
        "underside": "closed (-Z face at z %.0f)" % COUPLING_Z[0],
    }


def concept_proportions() -> dict:
    """Measured ratios of the built geometry against the fidelity targets (README §3)."""
    return {
        "shaft_width_W_cm": SHAFT_W,
        "height_H_cm": HEIGHT,
        "H_over_W": round(HEIGHT / SHAFT_W, 3),
        "H_over_W_target": [3.8, 4.0],
        "H_over_W_measured_band": [3.5, 4.2],
        "H_over_W_note": "candidate main view: cap top y 120 -> plinth ground y 745 = 625 px against W = 176 px (service face 114 + chamfers 2 x 44/sqrt2 at ~15 deg yaw) = 3.55 raw; disconnected view 366 / 99-107 px = 3.4-3.7 raw; +6-10 % pitch correction (15-25 deg) -> 3.5-4.2 W; the fidelity doc's 4.6-5.0 W was not reproducible from the pixels (concept-fidelity.md §Proportion 1, README §8.1 D10)",
        "plinth_over_W": round(2 * PLINTH_HALF / SHAFT_W, 3),
        "plinth_over_W_target": 1.8,
        "collar_centre_over_H": round((COLLAR_Z[0] + COLLAR_Z[1]) / 2.0 / HEIGHT, 3),
        "collar_centre_target": [0.70, 0.74],
        "collar_height_over_H": round((COLLAR_Z[1] - COLLAR_Z[0]) / HEIGHT, 3),
        "collar_height_target": 0.07,
        "collar_width_over_W": round(2 * COLLAR_HALF / SHAFT_W, 3),
        "collar_width_target": 1.35,
        "panel_height_over_H": round(PANEL_H / HEIGHT, 3),
        "panel_height_target": 0.19,
        "panel_aspect_w_over_h": round(PANEL_W / PANEL_H, 3),
        "panel_aspect_note": "candidate plates measure ~0.62 W x ~0.55 W (w/h 1.05-1.15); 124 x 135 keeps the four plates covering the shaft as painted at 3.9 W (README §8.1 D12)",
        "plinth_height_over_H": round(STEP_TOP / HEIGHT, 3),
        "plinth_height_target": 0.10,
        "plinth_lower_cm": 2 * PLINTH_HALF,
        "plinth_upper_cm": 2 * STEP_HALF,
        "coupling_over_W": [round(COUPLING[0] / SHAFT_W, 3), round(COUPLING[1] / SHAFT_W, 3), round(COUPLING[2] / SHAFT_W, 3)],
        "coupling_target": [0.9, 0.5, 0.5],
        "coupling_note": "depth 0.37 W: outer face pinned at |y| 140 by the stub bound (footprint edge = port + 56 cm stub + nut), inner face clears panel 04; README §8.1 D8",
        "coupling_overhang_cm": coupling_overhang(),
        "conduit_diameter_over_W": round(2 * STUB_R / SHAFT_W, 3),
        "conduit_diameter_target": 0.16,
        "conduit_diameter_note": "measured on the candidate main view: jackets 28-31 px (columns x 60-180, rows 673-763) against the across-flats width W = 176 px = 0.16-0.18 W, perspective band 0.15-0.18 W; concept-fidelity.md §Proportion 6 amended from the unmeasured ~0.09 W (README §8.1 D2)",
        "label_cell_over_panel": [round(LABEL[0] / PANEL_W, 3), round(LABEL[1] / PANEL_H, 3)],
        "section": "octagon: wide faces %.0f cm, chamfer faces %.1f cm" % (2 * (SHAFT_HALF - SHAFT_CHAMFER), SHAFT_CHAMFER * math.sqrt(2.0)),
    }


def hanging_conduit_record(maintenance: kit.Mesh) -> dict:
    prefix = MAINTENANCE_HANGING.lower() + "_hanging_"
    (_, _, jz0), (_, _, _) = maintenance.component_bounds(prefix + "conduit_jacket")
    (nx0, ny0, nz0), (nx1, ny1, nz1) = maintenance.component_bounds(prefix + "span_conduit_end_nut")
    return {
        "socket": MAINTENANCE_HANGING, "stub_pitch_deg": MAINTENANCE_HANG_PITCH, "span_yaw_deg": MAINTENANCE_SPAN_YAW, "span_pitch_deg": MAINTENANCE_SPAN_PITCH,
        "stub_jacket_lowest_z_cm": round(jz0, 1),
        "span_nut_bounds_cm": [[round(nx0, 1), round(ny0, 1), round(nz0, 1)], [round(nx1, 1), round(ny1, 1), round(nz1, 1)]],
        "parts": "the unplugged stub part plus a second instance of the SAME stub part chained from its Span_End socket (stands in for the runtime span segment); the exported part inventory is unchanged",
        "owner_question": "a dedicated sagging _ConduitHang part would read closer to the candidate's coiled cable; it changes the contract part inventory (GAP-01) and is raised through the coordinator (README §8.2)",
    }


def generate(export_dir: str, review_dir: str, bake_manifest_path: str) -> dict:
    """Build every mesh, write the exports, bake manifest and review assemblies, return the manifest."""
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)
    outputs = []
    meshes = {}
    for lod in (0, 1):
        for builder in (build_main, build_panel, build_stub):
            mesh = builder(lod)
            meshes[(mesh.name, lod)] = mesh
    # One unique UV0 atlas for the whole asset (main + parts, both LODs); UV1 stays the lightmap cells.
    atlas = kit.pack_atlas([(mesh, lod) for (name, lod), mesh in sorted(meshes.items(), key=lambda kv: (kv[0][1], kv[0][0]))], size=1024)
    bake_manifest_sha = kit.write_bake_manifest(bake_manifest_path, atlas, extras={
        "production_asset_id": PRODUCTION_ID, "revision": REVISION,
        "slot_families": {CERAMIC: "ceramic_civic", FRAME: "compact_metal", STATUS: "status_emissive"},
        "decal_rules": {
            "panel_label": "+X face chart is a 4-cell numeral strip: cells read 01, 02, 03, 04 left to right",
            "panel_plate": "+X face: high-contrast non-colour grid markings (REL-BLD-015.MC.LINK.ASSET) and edge wear",
            "collar_segment_*": "status mask R = strip index/8 encoded as band (four strips, one per wide face); emissive cyan",
            "coupling_left|coupling_right (status slot)": "status mask G: port status lamps",
            "conduit_pulse_strip": "status mask G with along-length gradient for the pulse",
            "team_band": "status mask B: team-colour carrier band under the collar",
            "conduit_clamp_ring|conduit_end_nut|service_bay_fittings": "ceramic_civic family: pale insulator rings / fittings, no decal",
        }})
    for (name, lod), mesh in sorted(meshes.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        base = os.path.join(export_dir, f"{mesh.name}_LOD{lod}")
        # Collision nodes travel only in the LOD0 file: Interchange's custom-LOD import path would
        # merge UBX meshes into LOD1 geometry instead of treating them as collision.
        glb = mesh.write_glb(base + ".glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod,
                                                    "units": "meters in file; authored centimeters", "planned_unreal_folder": PLANNED_FOLDER},
                             include_collision=(lod == 0))
        obj = mesh.write_obj(base + ".obj", header_lines=[f"Production ID {PRODUCTION_ID}", f"Revision {REVISION}", f"LOD{lod}"])
        rel = "export/" + os.path.basename(base)   # package-relative name, stable across a temporary --check build
        outputs.append({"path": rel + ".glb", "sha256": glb, "lod": lod, "mesh": mesh.name, "triangles": mesh.triangle_count(),
                        "section_slot_names": mesh.slot_names_in_primitive_order(), "collision_in_file": lod == 0,
                        "by_slot": mesh.triangle_count_by("slot"), "by_component": mesh.triangle_count_by("component"),
                        "bounds_cm": mesh.bounds(), "sockets": [{"name": s.name, "position_cm": s.position, "yaw_deg": s.yaw_deg, "purpose": s.purpose} for s in mesh.sockets],
                        "collision_boxes": [{"name": c.name, "center_cm": c.center, "size_cm": c.size} for c in mesh.collision]})
        outputs.append({"path": rel + ".obj", "sha256": obj, "lod": lod, "mesh": mesh.name})

    assemblies = []
    maintenance_scene = None
    for lod, state in ((0, "connected"), (0, "maintenance"), (1, "connected")):
        scene = assemble(lod, state)
        if state == "maintenance":
            maintenance_scene = scene
        path = os.path.join(review_dir, f"{scene.name}.obj")
        digest = scene.write_obj(path, header_lines=[f"Assembled review geometry {state} LOD{lod}; not an export",
                                                     f"Material {REVIEW_LABEL} is review-only (numeral cells; FRAME slot in the export)"])
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
    stub_end = {s.name: s for s in stub0.sockets}["Span_End"].position[0]
    footprint_check = {
        "footprint_cm": FOOTPRINT,
        "main_mesh_xy_extent_cm": [bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1]],
        "main_mesh_within_footprint": bounds[0][0] >= -FOOTPRINT[0] / 2 and bounds[1][0] <= FOOTPRINT[0] / 2 and bounds[0][1] >= -FOOTPRINT[1] / 2 and bounds[1][1] <= FOOTPRINT[1] / 2,
        "height_cm": bounds[1][2],
        "ground_contact_z_cm": bounds[0][2],
        "conduit_span_end_abs_y_cm": PORT_Y + stub_end,
        "conduit_stub_reach_beyond_footprint_cm": PORT_Y + stub_end - FOOTPRINT[1] / 2.0,
        "note": "The conduit stubs end exactly on the 2x2 footprint edge (Span_End); the cosmetic span component beyond it carries no collision, navigation or network authority (GAP-03). The plinth announces the footprint at 360 cm on Y; on +X the coupling blocks reach %.0f cm." % bounds[1][0],
    }

    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    with open(os.path.abspath(kit.__file__), "rb") as handle:
        kit_sha = sha256_bytes(handle.read())

    maintenance_bounds = maintenance_scene.bounds()
    manifest = {
        "author": AUTHOR, "creator": AUTHOR,
        "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "asset_name": ASSET,
        "revision": REVISION, "kit_revision": kit.KIT_REVISION,
        "stage": "BLOCKOUT",
        "stage_boundary": "Editable source geometry rebuilt to the concept images (concept-v1), corrected against the comparison sheets (concept-v2) and re-proportioned from the candidate pixels (concept-v3), with review views, comparison sheets and a baked proxy texture set; no Unreal import of this revision, no gate acceptance, no owner acceptance.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters", "glb": "meters (glTF spec); Interchange imports at 1 m = 100 cm (verified on the v3 import)", "axes": "authored +X forward, +Y right, +Z up; glTF export applies glTF=(X,Z,Y)"},
        "pivot": "ground-contact centre of the 2x2 footprint (contract import_policy.pivot)",
        "front": "+X face carries the numbered panels and service bay; couplings at the front-left (-Y) and front-right (+Y) plinth corners with ports facing -Y / +Y",
        "scale_basis": {
            "tile_world_size_cm": TILE_CM,
            "tile_source": "Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h TileWorldSize = 200.0f; Public/EchoesGlassScarCompiledMapPack.h kPresentationTileWorldUnits = 200",
            "footprint_cells": [2, 2],
            "footprint_source": "Content/Data/Source/buildings.json id=mc_power_link footprint_cells [2,2]; SPEC-STR-002 / SPEC-BLD-015.MC.LINK 2x2 tiles",
            "height_status": "CONCEPT MEASUREMENT (concept-fidelity.md, re-measured for concept-v3): plinth 360 cm = 1.8 W -> W = 200 cm, H = 3.9 W = 780 cm; replaces the concept-v2 850 cm / 180 cm (4.72 W, from the doc's unreproducible 4.6-5.0 band) and the v3 construction-reference 1,238 cm",
            "noted_discrepancy": "Requirements SPEC-SKM-011 says '64x64 tiles at 100 cm simulation scale'; presentation places tiles at 200 cm. The mesh follows the presentation tile constant the runtime uses to place structures.",
        },
        "concept_proportions": concept_proportions(),
        "dimensions_cm": {
            "plinth_lower": [2 * PLINTH_HALF, 2 * PLINTH_HALF, PLINTH_TOP], "plinth_skirt_top_z": SKIRT_TOP_Z, "plinth_upper": [2 * STEP_HALF, 2 * STEP_HALF, STEP_TOP - PLINTH_TOP], "plinth_top_z": STEP_TOP,
            "shaft": [2 * SHAFT_HALF, 2 * SHAFT_HALF, SHAFT_TOP - SHAFT_Z0], "shaft_chamfer": SHAFT_CHAMFER, "collar_z": COLLAR_Z, "collar_outer_width": 2 * COLLAR_HALF,
            "collar_courses_z": COLLAR_COURSES, "cap_plate_z": [SHAFT_TOP, CAP_TOP], "cap_top_z": HEIGHT, "panel": [PANEL_T, PANEL_W, PANEL_H], "panel_centres_z": PANEL_Z, "service_bay_z": BAY_Z,
            "coupling": COUPLING, "coupling_centre": [COUPLING_X, COUPLING_Y, (COUPLING_Z[0] + COUPLING_Z[1]) / 2.0], "coupling_z": COUPLING_Z, "port_z": PORT_Z, "port_abs_y": PORT_Y, "port_radius": PORT_R,
            "lamp_z": LAMP_Z, "conduit_radius": STUB_R, "conduit_stub_length": STUB_LENGTH, "conduit_clamp_rings_x": STUB_RINGS, "conduit_ring_radius": RING_R, "conduit_nut_radius": NUT_R,
            "panel_label": LABEL, "panel_label_z": LABEL_Z, "rail": RAIL, "collar_strip": STRIP,
            "team_band_z": TEAM_BAND_Z, "bracket_z": BRACKET_Z,
        },
        "uv_atlas": {"path": "bake-manifest.json", "sha256": bake_manifest_sha, "size": atlas["size"], "density_px_per_cm": atlas["density_px_per_cm"],
                     "charts": len(atlas["charts"]), "used_fraction": atlas["used_fraction"], "uv1": "per-polygon lightmap cells (unchanged)"},
        "material_slots": [CERAMIC, FRAME, STATUS],
        "material_slot_policy": "3 provisional slots (contract material_slots_provisional_max 3); status slot is a state-masked emissive under the 15% visible-area ceiling; the stub part now carries all three (pale ceramic insulator rings and nut)",
        "review_only_materials": {REVIEW_LABEL: "numeral label cells in the assembled review OBJs only (rendered pale as the bake paints them); the export keeps the cells on the FRAME slot so ebs_texbake paints the numerals"},
        "component_inventory": contract_inventory(main0),
        "budgets": assembled,
        "footprint_check": footprint_check,
        "outputs": outputs,
        "review_assemblies": assemblies,
        "maintenance_assembly": {
            "panels_removed": list(MAINTENANCE_REMOVED), "panel_02": "leaning against the right coupling's +X face at 35 degrees", "panel_03": "flat on the ground at (270, -60)",
            "debris_outside_footprint": "panels 02 and 03 lie past the 200 cm footprint edge (bounds x %.1f); cosmetic debris without collision, navigation or network authority (GAP-03, REL-BLD-014), README §8.1 D11" % maintenance_bounds[1][0],
            "bounds_cm": maintenance_bounds,
            "hanging_conduit": hanging_conduit_record(maintenance_scene),
        },
        "tools": {"builder": os.path.relpath(os.path.abspath(__file__), HERE), "builder_sha256": builder_sha, "meshkit": "../tools/ebs_meshkit.py", "meshkit_sha256": kit_sha,
                  "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "concept_fidelity": {"path": "concept-fidelity.md", "status": "AUTHORITATIVE production target (owner ruling 2026-09-06); §Proportion 1 and 6 amended 2026-09-07 from the candidate pixels"},
            "concept_a": {"path": "Project/site/assets/concepts/meridian-structures.png", "region": [0.5, 0.0, 1.0, 0.5], "sha256_prefix": "5b64820bbd2bbc28", "crop": "<evidence root>/concept-crops/powerlink_concept_2x.png"},
            "concept_b": {"path": "Project/site/assets/concepts/echoes-meridian-structures.jpg", "region": [0.5, 0.0, 1.0, 0.5], "sha256_prefix": "b6396728d4fd2a49", "crop": "<evidence root>/concept-crops/powerlink_concept_b_2x.png"},
            "candidate_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/power-link-review/power-link-candidate.png", "sha256_prefix": "bb8ffbe50e6683a2", "role": "primary proportion source (connected / disconnected / maintenance)"},
            "maintenance_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/power-link-review/power-link-maintenance-complete.png", "sha256": "b3816b9e6ec6f56629fc73c29f461187df4274b8f3514d637f9c9d48a883bf20"},
            "construction_reference": {"path": "Docs/VisualAssetPipeline/motion/construction/power-link.svg", "sha256": "c8abc04a65a0dee8d97f40c18ebd10ae8c2b203af7b2898afc9273796c1bbc7c", "status": "superseded for proportion by the concept measurements; topology (component order) still honoured"},
            "component_geometry": {"path": "Docs/VisualAssetPipeline/motion/construction/component-geometry.json", "sha256": "fcec87e723886479c08cec0c653513be8960267110d7c8bc7e07a1cf3875554c"},
            "canon_row": {"path": "Docs/Archive/DevelopmentBible.md", "line": 514, "file_sha256": "e237a4e16fa5d6da082395ca94ed1479ed709d1d8d5937a21a5858733c7cfe8c"},
            "book": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/book-source.docx", "sha256": "994e7df5d37a6ef1532c65a7b742d71e81b4ea7752311f5e5acc2e04c817ef83", "paragraphs": [135, 169]},
            "gameplay_record": {"path": "Content/Data/Source/buildings.json", "sha256": "aba1b64b20bb7c533aa7f7487839b12fe38133b96636d3b2e67e83c5ae6ed26e", "id": "mc_power_link"},
            "contract": {"path": "Docs/VisualAssetPipeline/motion/gap-decisions.json", "package_id": PACKAGE_ID},
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }
    return manifest


def compare_manifests(manifest: dict, previous: dict) -> list:
    """Every difference between a fresh build and the stored manifest that means the shipped files
    no longer match the generator: output list length and every output hash, review assembly count
    and hashes (by file name; the evidence path differs in a temporary build), and the atlas hash."""
    drift = []
    outs, prev_outs = manifest["outputs"], previous.get("outputs", [])
    if len(outs) != len(prev_outs):
        drift.append("outputs: %d built vs %d recorded" % (len(outs), len(prev_outs)))
    prev_by_path = {o.get("path"): o.get("sha256") for o in prev_outs}
    for o in outs:
        if prev_by_path.get(o["path"]) != o["sha256"]:
            drift.append(o["path"])
    asm, prev_asm = manifest["review_assemblies"], previous.get("review_assemblies", [])
    if len(asm) != len(prev_asm):
        drift.append("review_assemblies: %d built vs %d recorded" % (len(asm), len(prev_asm)))
    prev_by_name = {os.path.basename(a.get("path", "")): a.get("sha256") for a in prev_asm}
    for a in asm:
        if prev_by_name.get(os.path.basename(a["path"])) != a["sha256"]:
            drift.append("review/" + os.path.basename(a["path"]))
    if manifest["uv_atlas"]["sha256"] != previous.get("uv_atlas", {}).get("sha256"):
        drift.append("bake-manifest.json")
    if manifest["revision"] != previous.get("revision"):
        drift.append("revision: %s built vs %s recorded" % (manifest["revision"], previous.get("revision")))
    return drift


def main_cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True, help="directory for assembled review OBJs and render scenes (outside Git)")
    parser.add_argument("--check", action="store_true",
                        help="rebuild into a temporary directory and compare every output, review assembly and atlas hash against "
                             "build-manifest.json; writes nothing into the package or the evidence directory (exit 3 on drift)")
    args = parser.parse_args()

    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        if not os.path.exists(manifest_path):
            print(json.dumps({"check": "missing", "manifest": manifest_path}))
            return 2
        with open(manifest_path, "r", encoding="utf-8") as handle:
            previous = json.load(handle)
        with tempfile.TemporaryDirectory(prefix="ebs-mer-bld-002-check-") as tmp:
            manifest = generate(os.path.join(tmp, "export"), os.path.join(tmp, "review"), os.path.join(tmp, "bake-manifest.json"))
        drift = compare_manifests(manifest, previous)
        print(json.dumps({"check": "ok" if not drift else "drift", "drift": drift, "revision": manifest["revision"]}))
        return 0 if not drift else 3

    manifest = generate(os.path.join(HERE, "export"), os.path.join(args.evidence_dir, "review"), os.path.join(HERE, "bake-manifest.json"))
    text = json.dumps(manifest, indent=1, sort_keys=True) + "\n"
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"], "footprint": manifest["footprint_check"]["main_mesh_within_footprint"],
                      "outputs": len(manifest["outputs"]), "manifest": manifest_path}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
