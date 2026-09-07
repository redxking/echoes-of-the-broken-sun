#!/usr/bin/env python3
"""EBS-MER-UNT-003 Bulwark Team: deterministic source geometry, 18-bone transformation rig and clips.

Author: Angelis Pseftis.

Design authority (owner ruling 2026-09-06): the selected concept images define what the asset
looks like; gameplay rules bound the concept but never replace it. Target record:
  concept-fidelity.md (this folder). Sources looked at, not described:
  * bulwark-centered-wings-reference.png (OWNER-CORRECTED derived reference, the production
    target): DEPLOYED panel - one flat frontal wall of six framed cells numbered 01..06, the
    03/04 seam on the chassis centreline between the two operator stations, the central emitter
    lens at the base of that seam, chassis and four block legs behind and below the wall, the
    wall clear of the ground with the legs fully visible under it. PACKED TRAVEL and PACKED REAR
    panels - the wings folded back as OPEN frames flanking the chassis (no field, three per
    side), a compact crawler wider than tall, the rear left open.
  * EBS-CON-MER-UNT-003.png (concept, KEEP as design identity): the broad two-operator shield
    chassis, articulated barrier wings, pale ceramic over charcoal frame, cyan only on the
    visors, the emitter lens and the panel edges.
  * bulwark-six-panel-reference.png (earlier derived step, wall off-centre) and
    bulwark-reference.png (superseded, five visible panes) are recorded defects, not targets.

Every proportion below is a pixel measurement of the owner-corrected reference, expressed as a
ratio of W, the deployed wall width. W = 500 cm is fixed by the deployment record
(Content/Data/Source/units.json mc_bulwark_team: cover_half_width_cm 250), so the authored face
matches the cover it grants. The reference measures 825 px across the wall, so 1 px = 0.60606 cm;
each measured ratio is carried in MEASURED_PX below and re-derived here rather than eyeballed.

Package contract: Docs/VisualAssetPipeline/reference-packages.json EBS-PKG-MC-BULWARK-TEAM.
Canon row: Docs/Archive/DevelopmentBible.md line 511 (SPEC-UNIT-003).
Requirement card REL-FAC-025.MC.BULWARK.ASSET: LOD0 <= 8,200 / LOD1 <= 3,600 tris, 2048^2 PBR,
  matte roughness floor >= 0.85, 120 deg directional gradient with impact ripples (material work,
  not geometry), 18-bone mechanical transformation rig, 20-tick deploy / 15-tick pack, sub-object
  separation for Left_Shield_Panel and Right_Shield_Panel.

Rig (18 bones, identity rest orientation): root, chassis, operator_l/r, leg_fl/fr/rl/rr, emitter,
shield_anchor, wing_root_l/r, cell_01..cell_06. The card requires 18; the fidelity file's prose
list enumerates 16, so the emitter and the shield anchor - both of which drive a named socket and
both of which move independently of the chassis shell - carry the remaining two (README section 8).

States: the REST pose is DEPLOYED (one flat frontal face). PACKED is a posed state produced by the
wing-root bones (yaw + translate, PACK_* below), the same transform the deploy/pack clips key.

Conventions: cm, +X forward (the shield faces +X), +Y right, +Z up, root at the ground-contact
centre, no root motion; Nanite off; the runtime owns facing, the anchor and the cover volume.
--check is read-only (rebuilds into a temporary directory and compares hashes).
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
import ebs_skelkit as skel  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-MC-BULWARK-TEAM"
PRODUCTION_ID = "EBS-MER-UNT-003"
ASSET = "SK_EBS_MER_UNT_003"
LEFT_PART = "SM_EBS_MER_UNT_003_Left_Shield_Panel"
RIGHT_PART = "SM_EBS_MER_UNT_003_Right_Shield_Panel"
REVISION = "ebs-mer-unt-003-concept-v3"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_003/"

FRAME = "MI_EBS_MER_UnitFrame"        # charcoal machined frame, hinges, hubs, treads
CERAMIC = "MI_EBS_MER_UnitCeramic"    # pale ceramic plates (brass edge trim is a texture channel)
FIELD = "MI_EBS_MER_ShieldField"      # translucent blue barrier field: the six cell fields only
STATUS = "MI_EBS_MER_StatusCyan"      # cyan emissive: operator visors, emitter lens, cell edge lines
SLOTS = (FRAME, CERAMIC, FIELD, STATUS)

# --- pixel measurements of bulwark-centered-wings-reference.png (deployed panel) ---------------
# Measured with a stdlib PNG scan: wall frame bbox, blue-field column runs, foreground row/column
# profiles at sum(RGB) < 430 (paper ground sits at ~588). PX_PER_CM below turns them into cm.
MEASURED_PX = {
    "wall_width": 825.0,          # wall outer frame x 151..975
    "wall_height": 462.0,         # wall outer frame y 233..695
    "wall_bottom_above_ground": 187.0,   # ground row 882 - wall bottom 695
    "cell_pitch": 135.4,          # cell 01 field centre 227 -> cell 06 field centre 904 over 5 pitches
    "cell_field_width": 105.0,    # blue run 175..279
    "cell_field_height": 427.0,   # blue field y 248..675
    "operator_centre_y": 87.0,    # operator cowl centres +-87 px from the wall centre 563
    "operator_top_below_wall_top": 97.0,  # operator cowl top 330 - wall top 233
    "chassis_top_below_wall_top": 179.0,  # chassis top edge 412 - wall top 233
    "leg_centre_y": 204.0,        # leg hub centres +-204 px from the wall centre 563
    "foot_centre_y": 217.0,       # foot pad centres +-217 px from the wall centre (rows 277..415 / 715..851)
    "foot_width": 137.0,          # foot pad span at row 870
    "foot_mass_height": 122.0,    # lower-leg + pad mass, rows 760..882
    "leg_width": 88.0,            # leg span at row 760
    "emitter_z": 207.0,           # ground 882 - emitter lens centre 675
    "packed_travel_aspect": 435.0 / 370.0,   # packed travel silhouette w/h (3/4 view)
    "packed_rear_aspect": 462.0 / 320.0,     # packed rear silhouette w/h (3/4 view)
    "packed_cage_over_body_height": 308.0 / 370.0,   # folded cell height / packed unit height
    "packed_cage_top_over_body": 343.0 / 370.0,      # folded cell top / packed unit height
}
W = 500.0                                   # deployed wall width = 2 * cover_half_width_cm (250)
PX_PER_CM = W / MEASURED_PX["wall_width"]   # 0.60606 cm per reference pixel


def _px(key: float) -> float:
    return round(key * PX_PER_CM, 1)


# --- concept proportions (named constants; every value traced to MEASURED_PX or to a cited rule) -
WALL_W = W                                       # 1.000 W
CELL_PITCH = W / 6.0                             # 83.333; measured 135.4 px = 82.1 cm
CELL_W = 82.0                                    # 0.164 W (measured pitch), 1.33 cm reveal between cells
CELL_H = _px(MEASURED_PX["wall_height"])         # 280.0 = 0.560 W
CELL_T = 28.0                                    # panel thickness (card .MESH_PROP: thick segmented plates)
CELL_FRAME = 10.0                                # frame border: field 62 x 260 vs measured 63.6 x 258.8
CELL_CHAMFER = 15.0                              # chamfered corners, as the reference frames read
CELL_EDGE = 2.0                                  # cyan pane rim width (reference: the lit rim reads 2-4 px of a 105 px pane; 2.0 cm = 3.3 px)
BACKPLATE_Z0 = 4.0                               # opaque charcoal pane back plate, local z (world x 171 -> 165)
BACKPLATE_Z1 = 10.0                              # it sits behind the field slab (local z -3..3) and inside the frame
HINGE_INSET = 6.0                                # hinge barrel centre inset from the cell's inboard edge (no neighbour crossing)
WALL_BOTTOM_Z = _px(MEASURED_PX["wall_bottom_above_ground"])   # 113.3 = 0.227 W
WALL_TOP_Z = WALL_BOTTOM_Z + CELL_H              # 393.3
WALL_FACE_X = 175.0                              # deployed panel mid-plane, ahead of the chassis front
CELL_MID_Z = WALL_BOTTOM_Z + CELL_H / 2.0        # 253.3

CHASSIS_W = 0.44 * W                             # 220.0, fidelity file item 1
CHASSIS_X0, CHASSIS_X1 = -150.0, 130.0           # depth 280; the rear plane is the open-rear datum
CHASSIS_Z0 = 118.0                               # underside, just under the deployed wall's bottom rail
CHASSIS_Z1 = WALL_TOP_Z - _px(MEASURED_PX["chassis_top_below_wall_top"])   # 284.8 measured chassis top
OPERATOR_Y = _px(MEASURED_PX["operator_centre_y"])                          # 52.7
OPERATOR_TOP_Z = WALL_TOP_Z - _px(MEASURED_PX["operator_top_below_wall_top"])  # 334.5
OPERATOR_BASE_Z = 268.0                          # station seats into the chassis deck plate (deck top = CHASSIS_Z1)
OPERATOR_X = 18.0                                # stations sit forward on the deck, as the reference shows them
OPERATOR_W, OPERATOR_D = 58.0, 64.0              # cowl 58 wide: the two stations read as two figures at tactical range

LEG_Y = _px(MEASURED_PX["leg_centre_y"])         # 123.6 hub centres
FOOT_Y_CM = _px(MEASURED_PX["foot_centre_y"])    # 131.5: the pads splay just outboard of the hubs
LEG_FRONT_X, LEG_REAR_X = 76.0, -98.0
HUB_R = _px(MEASURED_PX["leg_width"]) / 2.0      # 26.7 hub disc radius (front-view leg span 88 px)
HUB_Z = 92.0                                     # hub disc centres: reference row 730, ground 882 -> 92 cm
FOOT_L, FOOT_W, FOOT_H = 96.0, 76.0, 42.0        # 0.19 W long pad; front-view width measured 83 cm (137 px)

EMIT_X, EMIT_R, LENS_R = 184.0, 19.0, 8.0        # nose stalk off the prow, at the base of the 03/04 seam; reads in both states
EMIT_Z = 114.0                                   # straddles the wall bottom rail (measured lens row 675 -> 125 cm)

WING_PIVOT = (128.0, 118.0, 250.0)               # chassis front shoulder (mirrored in Y)
# Packed pose of a wing root: yaw about +Z at the pivot, then translate (parent frame).
# Tuned so the folded cells clear the leg hubs (inner face >= 156 cm), stay inside the chassis rear
# plane, and give a packed silhouette wider than tall at the reference's 1.18-1.44 aspect.
PACK_YAW = 80.0
PACK_T = (-119.0, 25.0, -80.0)

TICKS_PER_SECOND = 20.0                          # SPEC-UNIT-003: 140-tick production = 7.0 s
DEPLOY_TICKS, PACK_TICKS = 20.0, 15.0
DEPLOY_S, PACK_S = DEPLOY_TICKS / TICKS_PER_SECOND, PACK_TICKS / TICKS_PER_SECOND

LOD0_CAP, LOD1_CAP = 8200, 3600
ROUGHNESS_FLOOR = 0.85
PRESENTATION_SCALE_HEAVY = 1.75   # EchoesEntityView.cpp:1817 (EntityType::HeavyUnit); not part of the asset

CELL_NAMES = ("cell_01", "cell_02", "cell_03", "cell_04", "cell_05", "cell_06")
# cells 01..03 on the anatomical LEFT wing (-Y), 04..06 on the RIGHT (+Y); the 03/04 seam is y = 0.
_CELL_RANK = (2, 1, 0, 0, 1, 2)   # distance-from-centre rank of cells 01..06
CELL_SIDE = (-1.0, -1.0, -1.0, 1.0, 1.0, 1.0)   # 01-03 on the anatomical left wing, 04-06 on the right
CELL_Y = tuple(CELL_SIDE[i] * CELL_PITCH * (_CELL_RANK[i] + 0.5) for i in range(6))
CELL_HINGE_Y = tuple(CELL_Y[i] - CELL_SIDE[i] * CELL_W / 2.0 for i in range(6))


def chamfer_rect(half_u: float, half_v: float, cut: float) -> list:
    """Convex 8-point outline in the cell's local (u = height, v = width) plane."""
    cut = min(cut, half_u * 0.9, half_v * 0.9)
    return [(half_u - cut, -half_v), (half_u, -half_v + cut), (half_u, half_v - cut), (half_u - cut, half_v),
            (-half_u + cut, half_v), (-half_u, half_v - cut), (-half_u, -half_v + cut), (-half_u + cut, -half_v)]


def plain_rect(half_u: float, half_v: float) -> list:
    return [(half_u, -half_v), (half_u, half_v), (-half_u, half_v), (-half_u, -half_v)]


# --- skeleton -----------------------------------------------------------------------------------
def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton(root="root")
    s.add("root", None, (0.0, 0.0, 0.0), "ground-contact centre; no root motion")
    s.add("chassis", "root", (0.0, 0.0, 150.0), "wide low two-operator chassis; carries every other bone")
    s.add("operator_l", "chassis", (OPERATOR_X, -OPERATOR_Y, OPERATOR_BASE_Z), "left operator station (cowl and visor)")
    s.add("operator_r", "chassis", (OPERATOR_X, OPERATOR_Y, OPERATOR_BASE_Z), "right operator station (cowl and visor)")
    s.add("leg_fl", "chassis", (LEG_FRONT_X, -LEG_Y, HUB_Z), "front-left block leg (hub pivot)")
    s.add("leg_fr", "chassis", (LEG_FRONT_X, LEG_Y, HUB_Z), "front-right block leg (hub pivot)")
    s.add("leg_rl", "chassis", (LEG_REAR_X, -LEG_Y, HUB_Z), "rear-left block leg (hub pivot)")
    s.add("leg_rr", "chassis", (LEG_REAR_X, LEG_Y, HUB_Z), "rear-right block leg (hub pivot)")
    s.add("emitter", "chassis", (EMIT_X, 0.0, EMIT_Z), "central emitter housing; drives Emitter_Muzzle")
    s.add("shield_anchor", "chassis", (WALL_FACE_X, 0.0, CELL_MID_Z),
          "deployed face centre; drives Shield_Face_Center, the impact ripple origin and the cover volume")
    s.add("wing_root_l", "chassis", (WING_PIVOT[0], -WING_PIVOT[1], WING_PIVOT[2]), "left barrier wing hinge (cells 01-03)")
    s.add("wing_root_r", "chassis", (WING_PIVOT[0], WING_PIVOT[1], WING_PIVOT[2]), "right barrier wing hinge (cells 04-06)")
    for index, name in enumerate(CELL_NAMES):
        parent = "wing_root_l" if index < 3 else "wing_root_r"
        s.add(name, parent, (WALL_FACE_X, CELL_HINGE_Y[index], CELL_MID_Z),
              f"barrier cell {name[-2:]} hinge (inboard vertical edge)")
    return s


def packed_pose() -> dict:
    """Bone pose that folds both wings into the travel profile (rotation + translation per bone)."""
    return {
        "wing_root_l": (0.0, -PACK_YAW, 0.0, PACK_T[0], -PACK_T[1], PACK_T[2]),
        "wing_root_r": (0.0, PACK_YAW, 0.0, PACK_T[0], PACK_T[1], PACK_T[2]),
    }


# --- parts --------------------------------------------------------------------------------------
def part_cell(lod: int, index: int) -> kit.Mesh:
    """One barrier cell authored flat in its local (u = height along +X, v = width along +Y) plane,
    thickness along local Z; merged with pitch 90 so u becomes world +Z and the thickness world X.

    Local Z runs OPPOSITE to world X after the pitch-90 merge (world x = WALL_FACE_X - local z), so
    NEGATIVE local z is the panel FRONT (+X, the direction the shield faces) and positive local z is
    the panel BACK. Front to back the stack is: cyan edge line (front, the reference's lit pane rim),
    the translucent field slab, then the opaque charcoal back plate that stops the field and the rim
    ever reading from behind the unit (review decision on EBS-CON-MER-UNT-003: the luminous panels
    cannot imply all-round invulnerability).
    """
    m = kit.Mesh(f"cell_{index + 1:02d}")
    for name in SLOTS:
        m.slot(name)
    hu, hv = CELL_H / 2.0, CELL_W / 2.0
    inboard = -CELL_SIDE[index]      # local +Y is world +Y; the hinge sits on the inboard vertical edge
    fu, fv = hu - CELL_FRAME, hv - CELL_FRAME
    t = CELL_T / 2.0
    inner = chamfer_rect(fu, fv, CELL_CHAMFER * 0.6)
    outer = chamfer_rect(hu, hv, CELL_CHAMFER)
    m.ring(outer, inner, -t, t, m.slot(FRAME), "frame")
    # cyan edge line on the FRONT face of the pane (world x 187.0..189.9), where the reference draws it
    edge_i = chamfer_rect(fu - CELL_EDGE, fv - CELL_EDGE, CELL_CHAMFER * 0.6)
    m.ring(inner, edge_i, -t, -(t - 2.9), m.slot(STATUS), "edge")
    # translucent field slab, then the opaque charcoal back plate immediately behind it
    m.prism(inner, -3.0, 3.0, m.slot(FIELD), "field")
    m.prism(inner, BACKPLATE_Z0, BACKPLATE_Z1, m.slot(FRAME), "backplate")
    if lod == 0:
        # hinge hardware on the inboard vertical edge, standing off the panel's BACK face so it stays
        # inside the cell body: the inter-cell reveal is only 1.333 cm and the castings must not shear
        # through their neighbours during deploy/pack.
        for k, z in enumerate((fu * 0.62, -fu * 0.62)):
            m.box((z, inboard * (hv - HINGE_INSET), t + 5.0), (26.0, 12.0, 16.0), m.slot(FRAME), f"hinge_{k + 1:02d}")
        for k, (su, sv) in enumerate(((1, 1), (1, -1), (-1, 1), (-1, -1))):
            m.box((su * (hu - CELL_CHAMFER * 0.55), sv * (hv - CELL_CHAMFER * 0.75), 0.0),
                  (CELL_CHAMFER * 1.5, CELL_CHAMFER * 1.5, CELL_T + 3.0), m.slot(FRAME), f"corner_{k + 1:02d}")
    else:
        m.box((0.0, inboard * (hv - HINGE_INSET), t + 5.0), (40.0, 12.0, 16.0), m.slot(FRAME), "hinge_01")
    return m


def part_operator(lod: int) -> kit.Mesh:
    """Operator station in a local frame with the origin at the station base on the chassis top."""
    m = kit.Mesh("operator")
    for name in SLOTS:
        m.slot(name)
    total = OPERATOR_TOP_Z - OPERATOR_BASE_Z
    torso_h = total - 30.0
    m.box((0.0, 0.0, torso_h / 2.0 + 4.0), (OPERATOR_D, OPERATOR_W, torso_h), m.slot(CERAMIC), "torso")
    cowl_z = torso_h + 18.0
    if lod == 0:
        m.box((0.0, 0.0, 4.0), (OPERATOR_D + 10.0, OPERATOR_W + 12.0, 16.0), m.slot(FRAME), "collar")
        m.box((-OPERATOR_D / 2.0 - 6.0, 0.0, torso_h * 0.62), (14.0, OPERATOR_W - 14.0, torso_h * 0.52), m.slot(FRAME), "backpack")
        for sign in (-1.0, 1.0):
            m.box((-4.0, sign * (OPERATOR_W / 2.0 + 5.0), torso_h * 0.74), (OPERATOR_D - 14.0, 12.0, 26.0),
                  m.slot(FRAME), "shoulder")
        m.prism(kit.octagon(OPERATOR_W / 2.0 - 3.0, OPERATOR_W / 5.0), torso_h + 2.0, torso_h + 30.0,
                m.slot(CERAMIC), "cowl")
        for sign in (-1.0, 1.0):
            m.box((-6.0, sign * (OPERATOR_W / 2.0 + 3.0), torso_h + 16.0), (OPERATOR_D - 22.0, 14.0, 44.0),
                  m.slot(FRAME), "arch_leg")
        m.box((-6.0, 0.0, torso_h + 36.0), (OPERATOR_D - 22.0, OPERATOR_W + 20.0, 14.0), m.slot(FRAME), "arch")
        m.box((OPERATOR_D / 2.0 - 3.0, 0.0, cowl_z), (9.0, OPERATOR_W - 22.0, 9.0), m.slot(STATUS), "visor")
        m.box((OPERATOR_D / 2.0 - 1.0, 0.0, torso_h * 0.5), (7.0, OPERATOR_W - 26.0, torso_h * 0.34), m.slot(FRAME), "chest_plate")
    else:
        m.box((0.0, 0.0, torso_h + 16.0), (OPERATOR_D - 8.0, OPERATOR_W - 6.0, 30.0), m.slot(CERAMIC), "cowl")
        m.box((OPERATOR_D / 2.0 - 4.0, 0.0, cowl_z), (7.0, OPERATOR_W - 22.0, 9.0), m.slot(STATUS), "visor")
        # the frame arch is kept at LOD1: it is the top of the station, so dropping it would pop the
        # operator silhouette 12 cm at the LOD transition (REL-ART-005.AUTH)
        for sign in (-1.0, 1.0):
            m.box((-6.0, sign * (OPERATOR_W / 2.0 + 3.0), torso_h + 16.0), (OPERATOR_D - 22.0, 14.0, 44.0),
                  m.slot(FRAME), "arch_leg")
        m.box((-6.0, 0.0, torso_h + 36.0), (OPERATOR_D - 22.0, OPERATOR_W + 20.0, 14.0), m.slot(FRAME), "arch")
    return m


def part_leg(lod: int, sign: float) -> kit.Mesh:
    """Block leg in a local frame with the origin at the hub pivot; ``sign`` is the outboard Y sign."""
    m = kit.Mesh("leg")
    for name in SLOTS:
        m.slot(name)
    out = sign * (FOOT_Y_CM - LEG_Y)
    ankle_z = -(HUB_Z - FOOT_H - 12.0)
    if lod == 0:
        m.tube((0.0, -16.0, 0.0), (0.0, 16.0, 0.0), HUB_R, 10, m.slot(FRAME), "hub")
        m.tube((0.0, sign * 16.0, 0.0), (0.0, sign * 23.0, 0.0), HUB_R * 0.52, 8, m.slot(CERAMIC), "hub_cap")
        m.box((0.0, sign * 6.0, CHASSIS_Z0 - HUB_Z + 4.0), (52.0, 40.0, (CHASSIS_Z0 - HUB_Z) * 2.0 + 8.0),
              m.slot(FRAME), "mount")
    else:
        m.box((0.0, 0.0, 0.0), (HUB_R * 1.8, 34.0, HUB_R * 1.8), m.slot(FRAME), "hub")
        m.box((0.0, 0.0, (CHASSIS_Z0 - HUB_Z) / 2.0), (44.0, 34.0, CHASSIS_Z0 - HUB_Z), m.slot(FRAME), "mount")
    m.box((0.0, out * 0.55, ankle_z * 0.5), (58.0, 46.0, abs(ankle_z) + 12.0), m.slot(CERAMIC), "thigh")
    if lod == 0:
        m.box((0.0, out * 0.55, ankle_z * 0.5), (66.0, 30.0, abs(ankle_z) * 0.5), m.slot(FRAME), "thigh_rail")
        m.box((0.0, out * 0.55 + sign * 24.0, ankle_z * 0.5), (40.0, 10.0, abs(ankle_z) * 0.8), m.slot(FRAME), "thigh_conduit")
    m.box((0.0, out, ankle_z - 6.0), (72.0, 58.0, 24.0), m.slot(FRAME), "ankle")
    m.box((0.0, out, -(HUB_Z - FOOT_H - 14.0)), (84.0, 66.0, 30.0), m.slot(CERAMIC), "boot")
    m.box((0.0, out, -(HUB_Z - FOOT_H / 2.0)), (FOOT_L, FOOT_W, FOOT_H), m.slot(CERAMIC), "foot")
    if lod == 0:
        for k, dx in enumerate((-0.33, 0.0, 0.33)):
            m.box((FOOT_L * dx, out, -HUB_Z + FOOT_H * 0.8 / 2.0), (FOOT_L * 0.24, FOOT_W + 6.0, FOOT_H * 0.8),
                  m.slot(FRAME), f"tread_{k + 1:02d}")
        m.box((FOOT_L * 0.5 - 4.0, out, -(HUB_Z - FOOT_H)), (12.0, FOOT_W - 16.0, 12.0), m.slot(FRAME), "toe_plate")
    return m


def build_chassis(lod: int) -> kit.Mesh:
    """Chassis shell, ceramic plates, structural rails, conduit lines, the open rear bay and the
    operator deck, authored in world space (REL-ART-006: engineered load paths, no organic curves)."""
    m = kit.Mesh("chassis")
    for name in SLOTS:
        m.slot(name)
    cx = (CHASSIS_X0 + CHASSIS_X1) / 2.0
    depth = CHASSIS_X1 - CHASSIS_X0
    cz = (CHASSIS_Z0 + CHASSIS_Z1) / 2.0
    height = CHASSIS_Z1 - CHASSIS_Z0
    hw = CHASSIS_W / 2.0
    # charcoal frame box with the rear face omitted: exposed rear access (canon: the rear says "flank me")
    m.box((cx, 0.0, cz), (depth, CHASSIS_W, height), m.slot(FRAME), "frame", skip=("-X",))
    # deck plate: its top face is the measured chassis top, so the operators read 49 cm proud of it
    m.box((cx, 0.0, CHASSIS_Z1 - 5.0), (depth - 14.0, CHASSIS_W + 10.0, 12.0), m.slot(CERAMIC), "deck")
    for sign, side in ((-1.0, "l"), (1.0, "r")):
        m.box((cx + 26.0, sign * (hw + 5.0), cz + 26.0), (depth - 76.0, 12.0, height * 0.46), m.slot(CERAMIC), f"flank_{side}")
        # the flank conduit line survives to LOD1: it is one of the two cyan cues that stay readable
        # from behind, and fidelity check 4 is certified at both LODs (README section 8)
        if lod == 1:
            m.box((cx + 46.0, sign * (hw + 8.0), CHASSIS_Z0 + 10.0), (depth - 96.0, 7.0, 8.0), m.slot(STATUS), f"conduit_{side}")
        if lod == 0:
            m.box((cx - 66.0, sign * (hw + 5.0), cz + 16.0), (depth * 0.30, 12.0, height * 0.40), m.slot(CERAMIC), f"flank_rear_{side}")
            m.box((cx + 18.0, sign * (hw + 4.0), CHASSIS_Z0 + 24.0), (depth - 54.0, 10.0, 30.0), m.slot(CERAMIC), f"flank_low_{side}")
            m.box((cx + 20.0, sign * (hw + 9.0), cz + 58.0), (depth - 42.0, 8.0, 9.0), m.slot(FRAME), f"rail_top_{side}")
            m.box((cx + 20.0, sign * (hw + 9.0), cz - 12.0), (depth - 46.0, 8.0, 7.0), m.slot(FRAME), f"rail_mid_{side}")
            m.box((cx + 4.0, sign * (hw + 8.0), cz + 26.0), (16.0, 7.0, height * 0.52), m.slot(FRAME), f"rib_{side}")
            m.box((cx - 40.0, sign * (hw + 8.0), cz + 26.0), (16.0, 7.0, height * 0.52), m.slot(FRAME), f"rib_rear_{side}")
            m.box((cx + 46.0, sign * (hw + 8.0), CHASSIS_Z0 + 10.0), (depth - 96.0, 7.0, 8.0), m.slot(STATUS), f"conduit_{side}")
            for k, dx in enumerate((-88.0, -24.0, 40.0)):
                m.box((cx + dx, sign * (hw + 7.0), cz + 4.0), (14.0, 6.0, height * 0.66), m.slot(FRAME), f"strake_{side}_{k + 1:02d}")
            m.box((cx + 62.0, sign * (hw + 6.0), cz - 30.0), (52.0, 8.0, 26.0), m.slot(CERAMIC), f"flank_hatch_{side}")
    # prow: the front face the wall stands off; segmented plates over the frame
    m.box((CHASSIS_X1 + 6.0, 0.0, cz + 34.0), (14.0, CHASSIS_W - 20.0, height * 0.30), m.slot(CERAMIC), "prow_upper")
    m.box((CHASSIS_X1 + 4.0, 0.0, cz - 18.0), (12.0, CHASSIS_W - 44.0, height * 0.26), m.slot(CERAMIC), "prow_lower")
    if lod == 1:
        m.box((cx + 10.0, 0.0, CHASSIS_Z0 - 12.0), (depth - 40.0, CHASSIS_W - 26.0, 26.0), m.slot(FRAME), "belly")
        for lx in (LEG_FRONT_X, LEG_REAR_X):
            m.box((lx, 0.0, CHASSIS_Z0 - 14.0), (46.0, 2.0 * LEG_Y + 24.0, 24.0), m.slot(FRAME), "axle")
        for k, (bx, by) in enumerate(((-104.0, -60.0), (-104.0, 60.0))):
            m.box((bx, by, cz - 4.0), (56.0, 62.0, height * 0.58), m.slot(FRAME), f"bay_{k + 1:02d}")
        # the bay core is the other cyan cue the rear read depends on; it stays at LOD1
        m.box((-96.0, 0.0, cz + 6.0), (14.0, 26.0, height * 0.44), m.slot(STATUS), "bay_core")
        m.box((OPERATOR_X + 4.0, 0.0, CHASSIS_Z1 + 6.0), (58.0, 34.0, 22.0), m.slot(FRAME), "deck_spine")
    if lod == 0:
        m.box((CHASSIS_X1 + 12.0, 0.0, CHASSIS_Z1 - 24.0), (12.0, CHASSIS_W - 64.0, 14.0), m.slot(FRAME), "prow_rail")
        m.box((CHASSIS_X1 + 11.0, 0.0, cz + 12.0), (14.0, CHASSIS_W - 108.0, 30.0), m.slot(FRAME), "prow_block")
        for sign in (-1.0, 1.0):
            m.box((CHASSIS_X1 + 8.0, sign * (hw - 16.0), cz + 30.0), (16.0, 26.0, height * 0.40), m.slot(FRAME), "prow_buttress")
            m.box((CHASSIS_X1 + 6.0, sign * (hw - 46.0), cz - 40.0), (12.0, 34.0, 30.0), m.slot(FRAME), "prow_vent")
            m.box((CHASSIS_X1 + 3.0, sign * (hw - 8.0), CHASSIS_Z0 + 30.0), (10.0, 18.0, 42.0), m.slot(CERAMIC), "prow_corner")
        # rear bay: interior machinery seen through the open rear plane
        for k, (bx, by) in enumerate(((-104.0, -60.0), (-104.0, 60.0), (-58.0, 0.0))):
            m.box((bx, by, cz - 4.0), (56.0, 62.0, height * 0.58), m.slot(FRAME), f"bay_{k + 1:02d}")
        m.box((-120.0, 0.0, CHASSIS_Z0 + 22.0), (32.0, CHASSIS_W - 36.0, 22.0), m.slot(CERAMIC), "bay_sill")
        m.box((-118.0, 0.0, CHASSIS_Z1 - 26.0), (36.0, CHASSIS_W - 52.0, 18.0), m.slot(FRAME), "bay_header")
        m.box((-96.0, 0.0, cz + 6.0), (14.0, 26.0, height * 0.44), m.slot(STATUS), "bay_core")
        for sign in (-1.0, 1.0):
            m.box((-124.0, sign * 78.0, cz + 10.0), (24.0, 30.0, height * 0.50), m.slot(CERAMIC), "bay_rack")
            m.box((-112.0, sign * 34.0, CHASSIS_Z0 + 46.0), (30.0, 26.0, 30.0), m.slot(CERAMIC), "bay_drum")
            m.box((-140.0, sign * (hw - 16.0), cz + 16.0), (16.0, 24.0, height * 0.62), m.slot(FRAME), "bay_post")
        m.box((-132.0, 0.0, CHASSIS_Z0 + 74.0), (18.0, 54.0, 16.0), m.slot(FRAME), "bay_duct")
        m.box((-126.0, 0.0, CHASSIS_Z0 + 8.0), (44.0, CHASSIS_W - 48.0, 10.0), m.slot(CERAMIC), "bay_floor")
        m.box((-146.0, 0.0, cz + 30.0), (8.0, CHASSIS_W - 30.0, 12.0), m.slot(FRAME), "bay_lintel")
        # operator deck spine between the two stations
        m.box((OPERATOR_X + 4.0, 0.0, CHASSIS_Z1 + 6.0), (58.0, 34.0, 22.0), m.slot(FRAME), "deck_spine")
        m.box((OPERATOR_X - 62.0, 0.0, CHASSIS_Z1 + 5.0), (40.0, CHASSIS_W - 70.0, 16.0), m.slot(CERAMIC), "deck_rear")
        for sign in (-1.0, 1.0):
            m.box((OPERATOR_X - 30.0, sign * (OPERATOR_Y + OPERATOR_W / 2.0 + 12.0), CHASSIS_Z1 + 3.0), (58.0, 20.0, 14.0),
                  m.slot(FRAME), "deck_kerb")
        # underslung machinery band: what the reference shows under the deployed wall between the legs
        m.box((cx + 10.0, 0.0, CHASSIS_Z0 - 16.0), (depth - 40.0, CHASSIS_W - 20.0, 34.0), m.slot(FRAME), "belly")
        for lx in (LEG_FRONT_X, LEG_REAR_X):
            m.box((lx, 0.0, CHASSIS_Z0 - 18.0), (52.0, 2.0 * LEG_Y + 30.0, 30.0), m.slot(FRAME), "axle")
            m.box((lx, 0.0, CHASSIS_Z0 - 18.0), (34.0, 2.0 * LEG_Y - 24.0, 36.0), m.slot(CERAMIC), "axle_cover")
        m.box((cx + 34.0, 0.0, CHASSIS_Z0 - 22.0), (depth * 0.34, 44.0, 14.0), m.slot(FRAME), "belly_keel")
    return m


def build_emitter(lod: int) -> kit.Mesh:
    m = kit.Mesh("emitter")
    for name in SLOTS:
        m.slot(name)
    if lod == 0:
        m.tube((EMIT_X - 12.0, 0.0, EMIT_Z), (EMIT_X + 12.0, 0.0, EMIT_Z), EMIT_R, 8, m.slot(FRAME), "housing")
        m.tube((EMIT_X + 12.0, 0.0, EMIT_Z), (EMIT_X + 16.0, 0.0, EMIT_Z), LENS_R, 8, m.slot(STATUS), "lens")
        m.tube((EMIT_X - 12.0, 0.0, EMIT_Z), (EMIT_X - 6.0, 0.0, EMIT_Z), EMIT_R * 0.72, 8, m.slot(CERAMIC), "collar")
        m.box(((EMIT_X - 12.0 + CHASSIS_X1) / 2.0, 0.0, EMIT_Z - 2.0), (EMIT_X - 12.0 - CHASSIS_X1, 30.0, 26.0),
              m.slot(FRAME), "stalk")
    else:
        m.box((EMIT_X, 0.0, EMIT_Z), (26.0, EMIT_R * 2.0, EMIT_R * 2.0), m.slot(FRAME), "housing")
        m.box((EMIT_X + 14.0, 0.0, EMIT_Z), (6.0, LENS_R * 2.0, LENS_R * 2.0), m.slot(STATUS), "lens")
        m.box(((EMIT_X - 12.0 + CHASSIS_X1) / 2.0, 0.0, EMIT_Z), (EMIT_X - 12.0 - CHASSIS_X1, 26.0, 24.0),
              m.slot(FRAME), "stalk")
    return m


def build_wing_arm(lod: int, sign: float) -> kit.Mesh:
    """Hinge arm from the chassis shoulder out to the wing's inboard cell (bound to the wing root)."""
    m = kit.Mesh("wing")
    for name in SLOTS:
        m.slot(name)
    px, py, pz = WING_PIVOT[0], sign * WING_PIVOT[1], WING_PIVOT[2]
    tip_x, tip_y = WALL_FACE_X - CELL_T / 2.0 - 2.0, sign * 30.0
    length = math.hypot(tip_x - px, tip_y - py)
    yaw = math.degrees(math.atan2(tip_y - py, tip_x - px))
    if lod == 0:
        m.tube((px, py - sign * 18.0, pz), (px, py + sign * 18.0, pz), 22.0, 8, m.slot(FRAME), "hinge_barrel")
        for dz in (52.0, -52.0):
            m.box(((px + tip_x) / 2.0, (py + tip_y) / 2.0, pz + dz), (length, 22.0, 20.0), m.slot(FRAME), "arm", yaw_deg=yaw)
        m.box(((px + tip_x) / 2.0, (py + tip_y) / 2.0, pz), (length * 0.9, 16.0, 30.0), m.slot(CERAMIC), "arm_web", yaw_deg=yaw)
        m.box((px - 8.0, py, pz - 74.0), (48.0, 34.0, 76.0), m.slot(CERAMIC), "strut")
        m.box((px - 8.0, py, pz + 66.0), (44.0, 30.0, 30.0), m.slot(FRAME), "strut_cap")
    else:
        m.box(((px + tip_x) / 2.0, (py + tip_y) / 2.0, pz), (length, 26.0, 120.0), m.slot(FRAME), "arm", yaw_deg=yaw)
    return m


# --- assembly -----------------------------------------------------------------------------------
def assemble(lod: int):
    """Deployed rest mesh with every polygon bound to its bone, plus the skeleton."""
    s = build_skeleton()
    m = kit.Mesh(ASSET)
    for name in SLOTS:
        m.slot(name)
    m.merge(build_chassis(lod), component_prefix="chassis_")
    m.merge(build_emitter(lod), component_prefix="emitter_")
    for sign, side in ((-1.0, "l"), (1.0, "r")):
        m.merge(part_operator(lod), translate=(OPERATOR_X, sign * OPERATOR_Y, OPERATOR_BASE_Z), component_prefix=f"op_{side}_")
        m.merge(build_wing_arm(lod, sign), component_prefix=f"wing_{side}_")
    for tag, lx in (("f", LEG_FRONT_X), ("r", LEG_REAR_X)):
        for sign, side in ((-1.0, "l"), (1.0, "r")):
            m.merge(part_leg(lod, sign), translate=(lx, sign * LEG_Y, HUB_Z), component_prefix=f"leg_{tag}{side}_")
    for index, name in enumerate(CELL_NAMES):
        m.merge(part_cell(lod, index), translate=(WALL_FACE_X, CELL_Y[index], CELL_MID_Z), pitch_deg=90.0,
                component_prefix=f"{name}_")
    m.sockets.append(kit.Socket("Target_Anchor_Center", (0.0, 0.0, 200.0), 0.0, "selection/targeting anchor at the chassis centre"))
    m.sockets.append(kit.Socket("Emitter_Muzzle", (EMIT_X + 14.0, 0.0, EMIT_Z), 0.0, "central emitter lens face (Niagara concussive flash)"))
    m.sockets.append(kit.Socket("Shield_Face_Center", (WALL_FACE_X + CELL_T / 2.0, 0.0, CELL_MID_Z), 0.0,
                                "deployed face centre: impact ripple origin and the 120 deg cover volume"))
    for index, name in enumerate(CELL_NAMES):
        m.sockets.append(kit.Socket(f"Cell_{index + 1:02d}", (WALL_FACE_X, CELL_HINGE_Y[index], CELL_MID_Z), 0.0,
                                    f"barrier cell {index + 1:02d} hinge"))
    binding = {"chassis_": "chassis", "emitter_": "emitter",
               "op_l_": "operator_l", "op_r_": "operator_r",
               "wing_l_": "wing_root_l", "wing_r_": "wing_root_r",
               "leg_fl_": "leg_fl", "leg_fr_": "leg_fr", "leg_rl_": "leg_rl", "leg_rr_": "leg_rr"}
    for name in CELL_NAMES:
        binding[f"{name}_"] = name
    counts = skel.bind_polygons(m, "chassis", binding)
    sockets_on_bones = {"Target_Anchor_Center": "chassis", "Emitter_Muzzle": "emitter",
                        "Shield_Face_Center": "shield_anchor"}
    for index, name in enumerate(CELL_NAMES):
        sockets_on_bones[f"Cell_{index + 1:02d}"] = name
    m.collision.append(kit.CollisionBox("UBX_chassis", ((CHASSIS_X0 + CHASSIS_X1) / 2.0, 0.0, (CHASSIS_Z0 + CHASSIS_Z1) / 2.0),
                                        (CHASSIS_X1 - CHASSIS_X0, CHASSIS_W, CHASSIS_Z1 - CHASSIS_Z0)))
    m.collision.append(kit.CollisionBox("UBX_stance", (LEG_FRONT_X / 2.0 + LEG_REAR_X / 2.0, 0.0, HUB_Z / 2.0),
                                        (LEG_FRONT_X - LEG_REAR_X + FOOT_L, 2.0 * FOOT_Y_CM + FOOT_W, HUB_Z)))
    return m, s, counts, sockets_on_bones


def wing_part(lod: int, side: str) -> kit.Mesh:
    """Sub-object export required by the card: one wing (arm + its three cells), pivot at the wing root."""
    whole, _s, _c, _sk = assemble(lod)
    pivot = (WING_PIVOT[0], (-1.0 if side == "l" else 1.0) * WING_PIVOT[1], WING_PIVOT[2])
    keep = [f"wing_{side}_"] + [f"{CELL_NAMES[k]}_" for k in ((0, 1, 2) if side == "l" else (3, 4, 5))]
    out = kit.Mesh(LEFT_PART if side == "l" else RIGHT_PART, slots=list(whole.slots))
    for poly in whole.polygons:
        if not any(poly.component.startswith(p) for p in keep):
            continue
        pts = [kit.v_sub(p, pivot) for p in poly.points]
        q = kit.Polygon(pts, poly.normal, poly.slot, poly.component, poly.uv_axis)
        out.polygons.append(q)
    for index in ((0, 1, 2) if side == "l" else (3, 4, 5)):
        out.sockets.append(kit.Socket(f"Cell_{index + 1:02d}",
                                      kit.v_sub((WALL_FACE_X, CELL_HINGE_Y[index], CELL_MID_Z), pivot), 0.0,
                                      f"barrier cell {index + 1:02d} hinge"))
    return out


def posed(lod: int, pose: dict) -> kit.Mesh:
    m, s, _c, sockets_on_bones = assemble(lod)
    full = dict(pose)
    full["_sockets"] = sockets_on_bones
    return skel.pose_mesh(m, s, full)


def state_mesh(m: kit.Mesh, field: str = "on", edges: str = "on", suffix: str = "") -> kit.Mesh:
    """Review copy of the SAME geometry in one of the runtime material states of the barrier field.
    The runtime drives these with the field material (a 120 deg frontal gradient, card .MAT_RULE),
    never by deleting geometry; the review renderer has no translucency, so the states are baked:

      field "on"    the shipped read: the translucent blue field slab
      field "dark"  the field slot re-slotted to the charcoal frame material - the runtime's OFF
                    state (packed, or a dropped barrier). NO geometry is removed; only the material
                    assignment differs from the shipped mesh
      field "cut"   the field polygons REMOVED. This is not a runtime state: it is a stand-in for
                    translucency, so the chassis and the two operators can be seen through the pane
                    exactly as the reference draws them through the glass. Any sheet that shows it
                    must say so, and it is never used to certify a fidelity check on its own
      edges "on" / "dark"   the cyan cell rim lit, or re-slotted to charcoal with the field
    """
    if field not in ("on", "cut", "dark"):
        raise ValueError("field is on, cut or dark")
    if edges not in ("on", "dark"):
        raise ValueError("edges is on or dark")
    out = kit.Mesh(m.name + suffix, slots=list(m.slots))
    field_slot = m.slots.index(FIELD) if FIELD in m.slots else -1
    frame_slot = m.slots.index(FRAME)
    for poly in m.polygons:
        is_edge = poly.component.startswith("cell_") and poly.component.endswith("_edge")
        if poly.slot == field_slot and field == "cut":
            continue
        slot = poly.slot
        if poly.slot == field_slot and field == "dark":
            slot = frame_slot
        if is_edge and edges == "dark":
            slot = frame_slot
        q = kit.Polygon(list(poly.points), poly.normal, slot, poly.component, poly.uv_axis)
        q.bone = getattr(poly, "bone", None)
        out.polygons.append(q)
    out.sockets = list(m.sockets)
    out.collision = list(m.collision)
    return out


def packed_mesh(lod: int) -> kit.Mesh:
    out = posed(lod, packed_pose())
    out.name = f"{ASSET}_packed"
    return out


# --- measurement --------------------------------------------------------------------------------
def polygon_area(poly) -> float:
    pts = poly.points
    total = (0.0, 0.0, 0.0)
    for k in range(1, len(pts) - 1):
        total = kit.v_add(total, kit.v_cross(kit.v_sub(pts[k], pts[0]), kit.v_sub(pts[k + 1], pts[0])))
    return 0.5 * kit.v_len(total)


def slot_area_fraction(m: kit.Mesh, slot_name: str) -> float:
    if slot_name not in m.slots:
        return 0.0
    index = m.slots.index(slot_name)
    total = sum(polygon_area(p) for p in m.polygons)
    return (sum(polygon_area(p) for p in m.polygons if p.slot == index) / total) if total else 0.0


def cell_bounds(m: kit.Mesh) -> list:
    return [m.component_bounds(f"{name}_frame") for name in CELL_NAMES]


def deployed_face_measure(m: kit.Mesh) -> dict:
    """Flatness, span and cell equality of the deployed face (fidelity check 1)."""
    boxes = cell_bounds(m)
    xs = [b[0][0] for b in boxes] + [b[1][0] for b in boxes]
    ys = sorted((b[0][1] + b[1][1]) / 2.0 for b in boxes)
    widths = [b[1][1] - b[0][1] for b in boxes]
    heights = [b[1][2] - b[0][2] for b in boxes]
    pitches = [round(b - a, 3) for a, b in zip(ys, ys[1:])]
    span = max(b[1][1] for b in boxes) - min(b[0][1] for b in boxes)
    return {
        "cells": len(boxes),
        "slab_x_min": round(min(xs), 3), "slab_x_max": round(max(xs), 3), "slab_thickness_cm": round(max(xs) - min(xs), 3),
        "span_y_cm": round(span, 3), "span_over_W": round(span / W, 4),
        "cell_width_cm": [round(v, 3) for v in widths], "cell_height_cm": [round(v, 3) for v in heights],
        "cell_pitch_cm": pitches,
        "cell_width_spread_cm": round(max(widths) - min(widths), 4),
        "cell_height_spread_cm": round(max(heights) - min(heights), 4),
        "seam_y_cm": round((ys[2] + ys[3]) / 2.0, 3),
        "wall_bottom_z_cm": round(min(b[0][2] for b in boxes), 3),
        "wall_top_z_cm": round(max(b[1][2] for b in boxes), 3),
        "left_wing_cells": sum(1 for b in boxes if (b[0][1] + b[1][1]) / 2.0 < 0.0),
        "right_wing_cells": sum(1 for b in boxes if (b[0][1] + b[1][1]) / 2.0 > 0.0),
    }


_VIEW_AXIS = {"+X": 0, "-X": 0, "+Y": 1, "-Y": 1, "+Z": 2, "-Z": 2}


def _newell_normal(points: list) -> tuple:
    nx = ny = nz = 0.0
    n = len(points)
    for i in range(n):
        a, b = points[i], points[(i + 1) % n]
        nx += (a[1] - b[1]) * (a[2] + b[2])
        ny += (a[2] - b[2]) * (a[0] + b[0])
        nz += (a[0] - b[0]) * (a[1] + b[1])
    return (nx, ny, nz)


def _point_in_polygon(u: float, v: float, pu: list, pv: list) -> bool:
    inside = False
    n = len(pu)
    j = n - 1
    for i in range(n):
        if (pv[i] > v) != (pv[j] > v):
            x = pu[i] + (v - pv[i]) * (pu[j] - pu[i]) / (pv[j] - pv[i])
            if u < x:
                inside = not inside
        j = i
    return inside


def visibility_census(m: kit.Mesh, view_from: str = "-X", grid: int = 200) -> dict:
    """What an opaque camera on ``view_from`` actually SEES: a depth-sorted raster census of the
    silhouette, reporting the material slot and the component that is frontmost at every sample.

    This is the measurement fidelity check 4 needs. Asserting that no cyan or field POLYGON lies
    behind some plane is vacuous for this geometry (every one of them sits ahead of the chassis);
    what matters is whether a field or status polygon is the nearest surface along the view ray.
    ``view_from`` names the side the camera is on, so "-X" is the rear view and "-Y"/"+Y" are the
    two flank views of the packed state.
    """
    depth = _VIEW_AXIS[view_from]
    near_is_min = view_from.startswith("-")
    su, sv = [i for i in (0, 1, 2) if i != depth]
    (b0, b1) = m.bounds()
    span_u, span_v = b1[su] - b0[su], b1[sv] - b0[sv]
    if span_u <= 0.0 or span_v <= 0.0:
        return {"view_from": view_from, "samples": 0, "by_slot": {}, "frontmost_components": []}
    nu = max(1, int(grid))
    nv = max(1, int(round(grid * span_v / span_u)))
    du, dv = span_u / nu, span_v / nv
    best_depth = [None] * (nu * nv)
    best_slot = [-1] * (nu * nv)
    best_comp = [None] * (nu * nv)
    for poly in m.polygons:
        n = _newell_normal(poly.points)
        nd = n[depth]
        if abs(nd) < 1e-9:
            continue                      # edge-on to the camera: no coverage
        pu = [p[su] for p in poly.points]
        pv = [p[sv] for p in poly.points]
        iu0 = max(0, int((min(pu) - b0[su]) / du) - 1)
        iu1 = min(nu - 1, int((max(pu) - b0[su]) / du) + 1)
        iv0 = max(0, int((min(pv) - b0[sv]) / dv) - 1)
        iv1 = min(nv - 1, int((max(pv) - b0[sv]) / dv) + 1)
        if iu1 < iu0 or iv1 < iv0:
            continue
        p0 = poly.points[0]
        d0 = n[0] * p0[0] + n[1] * p0[1] + n[2] * p0[2]
        for iu in range(iu0, iu1 + 1):
            cu = b0[su] + (iu + 0.5) * du
            base = iu * nv
            for iv in range(iv0, iv1 + 1):
                cv = b0[sv] + (iv + 0.5) * dv
                if not _point_in_polygon(cu, cv, pu, pv):
                    continue
                d = (d0 - n[su] * cu - n[sv] * cv) / nd
                k = base + iv
                cur = best_depth[k]
                if cur is None or (d < cur if near_is_min else d > cur):
                    best_depth[k], best_slot[k], best_comp[k] = d, poly.slot, poly.component
    hit = [k for k in range(nu * nv) if best_depth[k] is not None]
    by_slot = {name: 0 for name in m.slots}
    comps = set()
    for k in hit:
        by_slot[m.slots[best_slot[k]]] += 1
        comps.add(best_comp[k])
    total = len(hit) or 1
    return {
        "view_from": view_from, "grid": [nu, nv], "samples": len(hit),
        "by_slot": by_slot,
        "slot_fraction": {name: round(count / total, 5) for name, count in by_slot.items()},
        "field_fraction": round(by_slot.get(FIELD, 0) / total, 5),
        "status_fraction": round(by_slot.get(STATUS, 0) / total, 5),
        "frontmost_components": sorted(comps),
    }


def visible_cell_pane_components(m: kit.Mesh, view_from: str, grid: int = 200) -> list:
    """The cell pane components (field slab or cyan rim) that are the frontmost surface from
    ``view_from``. Empty from the rear is the property fidelity check 4 asserts."""
    census = visibility_census(m, view_from, grid)
    return [c for c in census["frontmost_components"]
            if c.startswith("cell_") and (c.endswith("_field") or c.endswith("_edge"))]


def rear_open_measure(m: kit.Mesh) -> dict:
    """No geometry behind the chassis rear plane above the legs (fidelity check 4)."""
    leg_top = HUB_Z + HUB_R
    behind = [(p.component, round(min(q[0] for q in p.points), 2))
              for p in m.polygons
              if min(q[0] for q in p.points) < CHASSIS_X0 - 0.001 and max(q[2] for q in p.points) > leg_top]
    rear_face = [p for p in m.polygons if p.component == "chassis_frame" and all(abs(q[0] - CHASSIS_X0) < 0.001 for q in p.points)]
    return {"rear_plane_x_cm": CHASSIS_X0, "leg_top_z_cm": round(leg_top, 2),
            "polygons_behind_rear_plane_above_legs": len(behind), "offenders": behind[:6],
            "chassis_rear_face_polygons": len(rear_face), "rear_face_open": len(rear_face) == 0}


def packed_measure(lod: int = 0) -> dict:
    """Packed silhouette and the loss of the frontal face (fidelity check 3)."""
    p = packed_mesh(lod)
    d, _s, _c, _k = assemble(lod)
    (px0, py0, pz0), (px1, py1, pz1) = p.bounds()
    (dx0, dy0, dz0), (dx1, dy1, dz1) = d.bounds()
    boxes = [p.component_bounds(f"{name}_frame") for name in CELL_NAMES]
    cell_x = [round(b[1][0], 2) for b in boxes]
    cell_y_inner = min(min(abs(b[0][1]), abs(b[1][1])) for b in boxes)
    frontal_span = max(b[1][1] for b in boxes) - min(b[0][1] for b in boxes)
    slab = max(b[1][0] for b in boxes) - min(b[0][0] for b in boxes)
    return {
        "packed_bounds_cm": [[round(px0, 2), round(py0, 2), round(pz0, 2)], [round(px1, 2), round(py1, 2), round(pz1, 2)]],
        "packed_length_x_cm": round(px1 - px0, 2), "packed_width_y_cm": round(py1 - py0, 2), "packed_height_z_cm": round(pz1 - pz0, 2),
        "wider_than_tall": (py1 - py0) > (pz1 - pz0),
        "packed_aspect_w_over_h": round((py1 - py0) / (pz1 - pz0), 3),
        "reference_aspect_range": [round(MEASURED_PX["packed_travel_aspect"], 3), round(MEASURED_PX["packed_rear_aspect"], 3)],
        "deployed_bounds_cm": [[round(dx0, 2), round(dy0, 2), round(dz0, 2)], [round(dx1, 2), round(dy1, 2), round(dz1, 2)]],
        "deployed_height_z_cm": round(dz1 - dz0, 2), "deployed_width_y_cm": round(dy1 - dy0, 2),
        "packed_narrower_than_deployed_wall": (py1 - py0) < WALL_W,
        "packed_cell_front_x_cm": cell_x, "packed_cell_slab_thickness_cm": round(slab, 2),
        "packed_cell_inner_face_y_cm": round(cell_y_inner, 2),
        "leg_hub_outer_y_cm": round(LEG_Y + HUB_R, 2),
        "cells_clear_of_legs": cell_y_inner > LEG_Y + HUB_R,
        "packed_cell_frontal_span_y_cm": round(frontal_span, 2),
        "frontal_face_lost": slab > 100.0,
        "packed_cell_bottom_z_cm": round(min(b[0][2] for b in boxes), 2),
        "packed_cell_top_z_cm": round(max(b[1][2] for b in boxes), 2),
        "packed_rear_visibility": visibility_census(p, "-X"),
        "packed_flank_visibility": {side: visibility_census(p, side) for side in ("-Y", "+Y")},
        "packed_flank_field_note": (
            "The barrier field slab cannot be removed from the packed pose by geometry: the rig is "
            "rotation and translation only (ebs_skelkit.Keyframe has no scale track), so a rigid "
            "62 x 260 cm pane is present in every pose, and the packed fold turns each pane's FRONT "
            "face outboard, i.e. the flank camera looks straight down the pane normal. The packed "
            "read therefore depends on the field material being OFF while packed - the state baked "
            "into the packed_field_dark review assembly. README section 8 records the deviation."),
    }


def operator_station_measure(m: kit.Mesh) -> dict:
    """The operator station's real extent. The reference's measured cue is the COWL top (97 px below
    the wall top); the built station also carries the frame arch over the cowl, which stands higher,
    so both are reported - the earlier build read op_r_cowl alone and understated the station."""
    comps = [c for c in m.components() if c.startswith("op_r_") or c.startswith("op_l_")]
    tops = {c: m.component_bounds(c)[1][2] for c in comps}
    cowl = max(v for c, v in tops.items() if c.endswith("_cowl"))
    station = max(tops.values())
    tallest = max(tops, key=lambda c: tops[c])
    return {"cowl_top_z_cm": round(cowl, 2), "station_top_z_cm": round(station, 2),
            "tallest_component": tallest, "components": len(comps)}


def concept_measurements(m: kit.Mesh) -> dict:
    face = deployed_face_measure(m)
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    station = operator_station_measure(m)
    op = ((0.0, 0.0, 0.0), (0.0, 0.0, station["station_top_z_cm"]))
    return {
        "W_cm": W, "px_per_cm_on_reference": round(PX_PER_CM, 5),
        "deployed_face": face,
        "wall_width_cm": face["span_y_cm"], "wall_width_over_W": face["span_over_W"],
        "wall_height_cm": round(face["wall_top_z_cm"] - face["wall_bottom_z_cm"], 3),
        "wall_height_over_W": round((face["wall_top_z_cm"] - face["wall_bottom_z_cm"]) / W, 4),
        "wall_bottom_over_W": round(face["wall_bottom_z_cm"] / W, 4),
        "cell_width_over_W": round(face["cell_width_cm"][0] / W, 4),
        "cell_height_over_W": round(face["cell_height_cm"][0] / W, 4),
        "chassis_width_cm": CHASSIS_W, "chassis_width_over_W": round(CHASSIS_W / W, 4),
        "chassis_depth_cm": CHASSIS_X1 - CHASSIS_X0, "chassis_top_z_cm": CHASSIS_Z1,
        "operator_station": station,
        "operator_top_z_cm": round(station["station_top_z_cm"], 2),
        "operator_top_over_W": round(station["station_top_z_cm"] / W, 4),
        "operator_cowl_top_z_cm": round(station["cowl_top_z_cm"], 2),
        "operator_cowl_top_over_W": round(station["cowl_top_z_cm"] / W, 4),
        "operator_centre_y_cm": OPERATOR_Y, "operator_gap_cm": round(2.0 * OPERATOR_Y, 2),
        "wall_top_above_operator_cm": round(face["wall_top_z_cm"] - station["station_top_z_cm"], 2),
        "wall_top_above_operator_cowl_cm": round(face["wall_top_z_cm"] - station["cowl_top_z_cm"], 2),
        "wall_taller_than_operators": face["wall_top_z_cm"] > op[1][2],
        "leg_centre_y_cm": LEG_Y, "foot_centre_y_cm": FOOT_Y_CM, "foot_length_cm": FOOT_L,
        "foot_length_over_W": round(FOOT_L / W, 4), "stance_width_cm": round(2.0 * FOOT_Y_CM + FOOT_W, 2),
        "emitter_z_cm": EMIT_Z, "emitter_y_cm": 0.0, "emitter_on_centreline": True,
        "emitter_at_face_base": abs(EMIT_Z - face["wall_bottom_z_cm"]) < 30.0,
        "deployed_height_cm": round(z1 - z0, 2), "deployed_height_over_W": round((z1 - z0) / W, 4),
        "deployed_bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "rear_open": rear_open_measure(m),
        "rear_visibility": visibility_census(m, "-X"),
        "front_visibility": visibility_census(m, "+X"),
        "status_cyan_area_fraction": round(slot_area_fraction(m, STATUS), 5),
        "shield_field_area_fraction": round(slot_area_fraction(m, FIELD), 5),
        "ground_contact_min_z_cm": round(z0, 3),
    }


# --- clips --------------------------------------------------------------------------------------
def _wing_keys(clip: skel.AnimationClip, time_s: float, fraction: float, extra_yaw: float = 0.0,
               extra_t=(0.0, 0.0, 0.0)) -> None:
    """Key both wing roots at ``fraction`` of the way from deployed (0) to packed (1)."""
    for side, sign in (("l", -1.0), ("r", 1.0)):
        yaw = sign * (PACK_YAW * fraction + extra_yaw)
        tx = PACK_T[0] * fraction + extra_t[0]
        ty = sign * (PACK_T[1] * fraction + extra_t[1])
        tz = PACK_T[2] * fraction + extra_t[2]
        clip.key(f"wing_root_{side}", time_s, (0.0, yaw, 0.0), (tx, ty, tz))


def _leg_shuffle(clip: skel.AnimationClip, duration: float, amp: float, steps: int = 4) -> None:
    """Crawler shuffle: each leg pitches about its hub; the 118 cm arm keeps the pads on the ground."""
    phase = {"leg_fl": 0.0, "leg_rr": 0.0, "leg_fr": 0.5, "leg_rl": 0.5}
    for k in range(steps + 1):
        t = duration * k / steps
        for bone, ph in phase.items():
            clip.key(bone, t, (amp * math.sin(2.0 * math.pi * (k / steps + ph)), 0.0, 0.0))


# --- ground solve (the four pads stay on the ground in every keyframe; no root motion) -----------
def _sole_points() -> list:
    """(point, bone) samples on the four pads' lowest faces, in the deployed rest frame."""
    pts = []
    for tag, lx in (("f", LEG_FRONT_X), ("r", LEG_REAR_X)):
        for sign, side in ((-1.0, "l"), (1.0, "r")):
            cy = sign * LEG_Y + sign * (FOOT_Y_CM - LEG_Y)
            for dx in (-FOOT_L / 2.0, -FOOT_L * 0.19, FOOT_L * 0.19, FOOT_L / 2.0):
                for dy in (-(FOOT_W + 6.0) / 2.0, (FOOT_W + 6.0) / 2.0):
                    pts.append(((lx + dx, cy + dy, 0.0), f"leg_{tag}{side}"))
    return pts


_SOLES = None


def _fk_point(p, bone_name: str, pose: dict, skeleton: skel.Skeleton):
    """Forward kinematics identical to ebs_skelkit.pose_mesh.apply_point."""
    for name in skeleton.chain_to_root(bone_name):
        entry = pose.get(name, (0.0, 0.0, 0.0))
        vals = tuple(float(c) for c in entry)
        rot, tr = (vals[:3], vals[3:]) if len(vals) == 6 else (vals, (0.0, 0.0, 0.0))
        local = skel.rot_rotator(kit.v_sub(p, skeleton.get(name).head), rot[0], rot[1], rot[2])
        p = kit.v_add(kit.v_add(local, skeleton.get(name).head), tr)
    return p


def lowest_sole_z(pose: dict, skeleton: skel.Skeleton) -> float:
    global _SOLES
    if _SOLES is None:
        _SOLES = _sole_points()
    return min(_fk_point(p, bone, pose, skeleton)[2] for p, bone in _SOLES)


def _interp(keys, t):
    if t <= keys[0].time_s:
        k = keys[0]
        return k.rotation_deg, k.translation_cm
    if t >= keys[-1].time_s:
        k = keys[-1]
        return k.rotation_deg, k.translation_cm
    for a, b in zip(keys, keys[1:]):
        if a.time_s <= t <= b.time_s:
            f = 0.0 if b.time_s == a.time_s else (t - a.time_s) / (b.time_s - a.time_s)
            return (tuple(a.rotation_deg[i] + (b.rotation_deg[i] - a.rotation_deg[i]) * f for i in range(3)),
                    tuple(a.translation_cm[i] + (b.translation_cm[i] - a.translation_cm[i]) * f for i in range(3)))
    k = keys[-1]
    return k.rotation_deg, k.translation_cm


def solve_ground(clip: skel.AnimationClip, skeleton: skel.Skeleton, grid: int = 41) -> None:
    """Rewrite the chassis track so the lowest pad corner sits on z = 0 at every keyframe, then
    densely verify the linear interpolation between keys and raise the bracketing keys until the
    whole clip stays on the ground. Deterministic: a pure function of the authored keys."""
    times = sorted({round(k.time_s, 6) for keys in clip.tracks.values() for k in keys})
    chassis = clip.tracks.get("chassis") or [skel.Keyframe(times[0], (0.0, 0.0, 0.0), (0.0, 0.0, 0.0))]
    base = {t: _interp(chassis, t) for t in times}

    def pose_at(t, lift_by_time):
        pose = {}
        for bone, keys in clip.tracks.items():
            if bone == "chassis":
                continue
            rot, tr = _interp(keys, t)
            pose[bone] = (*rot, *tr)
        rot, tr = _interp(chassis, t)
        lift = _interp([skel.Keyframe(u, (0.0, 0.0, 0.0), (0.0, 0.0, lift_by_time[u])) for u in times], t)[1][2]
        pose["chassis"] = (*rot, tr[0], tr[1], lift)
        return pose

    lift = {t: 0.0 for t in times}
    for t in times:
        pose = pose_at(t, lift)
        pose["chassis"] = (*base[t][0], base[t][1][0], base[t][1][1], 0.0)
        lift[t] = round(-lowest_sole_z(pose, skeleton), 4)
    for _ in range(6):
        worst = 0.0
        for step in range(grid):
            t = times[0] + (times[-1] - times[0]) * step / (grid - 1)
            z = lowest_sole_z(pose_at(t, lift), skeleton)
            if z < -0.05:
                worst = max(worst, -z)
                lo = max([u for u in times if u <= t] or [times[0]])
                hi = min([u for u in times if u >= t] or [times[-1]])
                lift[lo] = round(lift[lo] - z, 4)
                lift[hi] = round(lift[hi] - z, 4)
        if worst <= 0.05:
            break
    clip.tracks["chassis"] = [skel.Keyframe(t, base[t][0], (base[t][1][0], base[t][1][1], lift[t])) for t in times]


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def new(name, duration, loop, purpose):
        c = skel.AnimationClip(name, duration, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    # 1. packed idle: the travel profile held, with a slow hydraulic settle
    c = new("idle_packed", 2.0, True, "packed travel profile held; slow hydraulic settle, no root motion")
    for t, extra in ((0.0, 0.0), (1.0, 0.8), (2.0, 0.0)):
        _wing_keys(c, t, 1.0, extra_yaw=extra)
        c.key("chassis", t, (0.35 * extra, 0.0, 0.0))
    # 2. move packed: the heavy crawler shuffle
    c = new("move_packed", 1.2, True, "packed movement at 230 cm/s: four-leg crawler shuffle")
    for t in (0.0, 0.3, 0.6, 0.9, 1.2):
        _wing_keys(c, t, 1.0)
    _leg_shuffle(c, 1.2, 7.0)
    for k, t in enumerate((0.0, 0.3, 0.6, 0.9, 1.2)):
        c.key("chassis", t, (0.6 * math.sin(math.pi * k / 2.0), 0.0, 0.0))
    # 3. deploy: 20 ticks, anticipation -> wing swing -> contact -> settle
    c = new("deploy", DEPLOY_S, False, f"Deploy Barrier setup, {DEPLOY_TICKS:g} ticks (SPEC-UNIT-003): anticipation, wing swing, contact, settle")
    _wing_keys(c, 0.0, 1.0)
    _wing_keys(c, 0.12 * DEPLOY_S, 1.0, extra_yaw=4.5, extra_t=(6.0, 4.0, 4.0))     # anticipation: wings load outward
    _wing_keys(c, 0.55 * DEPLOY_S, 0.42)                                            # swing
    _wing_keys(c, 0.82 * DEPLOY_S, -0.035)                                          # contact: slight overshoot past flat
    _wing_keys(c, DEPLOY_S, 0.0)                                                    # settle: flat frontal wall
    for t, pitch in ((0.0, 0.0), (0.12 * DEPLOY_S, 1.1), (0.55 * DEPLOY_S, 0.2), (0.82 * DEPLOY_S, -1.4), (DEPLOY_S, 0.0)):
        c.key("chassis", t, (pitch, 0.0, 0.0))
    # 4. deployed idle: the field hum; the cells breathe on their hinges
    c = new("idle_deployed", 2.4, True, "deployed idle: low field hum, cells breathing on their hinges")
    for t, y in ((0.0, 0.0), (1.2, 0.45), (2.4, 0.0)):
        _wing_keys(c, t, 0.0)
        for index, name in enumerate(CELL_NAMES):
            c.key(name, t, (0.0, y * (1.0 if index >= 3 else -1.0), 0.0))
    # 5. deployed drag: movement at 35% speed with the wall up
    c = new("drag_deployed", 2.0, True, "deployed movement at 35% speed: slow drag, the wall stays flat and frontal")
    for t in (0.0, 0.5, 1.0, 1.5, 2.0):
        _wing_keys(c, t, 0.0)
    _leg_shuffle(c, 2.0, 4.0)
    for k, t in enumerate((0.0, 0.5, 1.0, 1.5, 2.0)):
        c.key("chassis", t, (0.0, 0.0, 0.5 * math.sin(math.pi * k / 2.0)))
    # 6. pack: 15 ticks, the reverse with the clank at the end
    c = new("pack", PACK_S, False, f"Pack, {PACK_TICKS:g} ticks (SPEC-UNIT-003): wings fold to the travel profile, clank on seat")
    _wing_keys(c, 0.0, 0.0)
    _wing_keys(c, 0.10 * PACK_S, -0.03)                                             # anticipation: press forward
    _wing_keys(c, 0.55 * PACK_S, 0.62)
    _wing_keys(c, 0.86 * PACK_S, 1.04, extra_yaw=1.5)                               # overshoot into the cradle
    _wing_keys(c, PACK_S, 1.0)
    for t, pitch in ((0.0, 0.0), (0.10 * PACK_S, -0.5), (0.55 * PACK_S, 0.4), (0.86 * PACK_S, 1.2), (PACK_S, 0.0)):
        c.key("chassis", t, (pitch, 0.0, 0.0))
    # 7. damage: a jolt through the deployed wall (the cyan ripple itself is material work)
    c = new("damage", 0.5, False, "projectile impact on the deployed face: chassis jolt and cell shudder; the cyan ripple is material, not geometry")
    _wing_keys(c, 0.0, 0.0)
    _wing_keys(c, 0.10, 0.0, extra_yaw=-1.6, extra_t=(-5.0, 0.0, 0.0))
    _wing_keys(c, 0.5, 0.0)
    for t, pitch, dx in ((0.0, 0.0, 0.0), (0.10, -1.8, -4.0), (0.26, 0.9, 2.0), (0.5, 0.0, 0.0)):
        c.key("chassis", t, (pitch, 0.0, 0.0), (dx, 0.0, 0.0))
        for name in ("operator_l", "operator_r"):
            c.key(name, t, (pitch * 1.6, 0.0, 0.0))
    # 8. death: the chassis tips forward onto the front pads, the wings splay and drop
    c = new("death", 1.6, False, "destruction: the frame tips forward onto the front pads, the wings splay open and drop, the field dies")
    _wing_keys(c, 0.0, 0.0)
    _wing_keys(c, 0.35, 0.10, extra_yaw=6.0, extra_t=(0.0, 6.0, -8.0))
    _wing_keys(c, 1.0, 0.26, extra_yaw=16.0, extra_t=(-10.0, 16.0, -34.0))
    _wing_keys(c, 1.6, 0.30, extra_yaw=18.0, extra_t=(-12.0, 18.0, -38.0))
    for t, pitch, roll in ((0.0, 0.0, 0.0), (0.35, -4.0, 1.0), (1.0, -11.0, 2.6), (1.6, -12.5, 3.0)):
        c.key("chassis", t, (pitch, 0.0, roll))
        for name in ("operator_l", "operator_r"):
            c.key(name, t, (pitch * 1.5, 0.0, roll))
    for t, amp in ((0.0, 0.0), (1.0, 9.0), (1.6, 11.0)):
        for bone, sign in (("leg_fl", 1.0), ("leg_fr", 1.0), ("leg_rl", -1.0), ("leg_rr", -1.0)):
            c.key(bone, t, (sign * amp, 0.0, 0.0))
    # 9. cancel: a deploy interrupted mid-swing returns to the travel profile
    c = new("cancel", 0.35, False, "Deploy interrupted: the wings return to the travel profile without seating")
    _wing_keys(c, 0.0, 0.42)
    _wing_keys(c, 0.22, 0.98)
    _wing_keys(c, 0.35, 1.0)
    c.key("chassis", 0.0, (0.0, 0.0, 0.0))
    c.key("chassis", 0.35, (0.0, 0.0, 0.0))
    # 10. restore: back to the deployed idle after damage
    c = new("restore", 0.6, False, "return to the deployed idle after damage: the wall re-seats flat and frontal")
    _wing_keys(c, 0.0, 0.0, extra_yaw=-1.6, extra_t=(-5.0, 0.0, 0.0))
    _wing_keys(c, 0.34, 0.0, extra_yaw=0.5)
    _wing_keys(c, 0.6, 0.0)
    for t, pitch in ((0.0, -1.8), (0.34, 0.5), (0.6, 0.0)):
        c.key("chassis", t, (pitch, 0.0, 0.0))
    for clip in clips:
        solve_ground(clip, skeleton)
    # Snap every authored duration onto a whole 30 fps frame. VERIFIED 2026-09-07 against UE 5.8.2
    # (evidence: BuildArtifacts/.../skeletal-clip-duration-probe/): the Interchange skeletal import
    # silently creates NO AnimSequence for a clip whose duration is a half frame, and still reports
    # success. retime_clip scales the key times, so the pose at any normalized time is unchanged.
    for _clip in clips:
        if not skel.is_frame_aligned(_clip.duration_s):
            skel.retime_clip(_clip, skel.frame_aligned_duration(_clip.duration_s))
    return clips


def sample_pose(clip: skel.AnimationClip, fraction: float) -> dict:
    """Linear sample of a clip, the same interpolation the glTF samplers use."""
    t = clip.duration_s * fraction
    pose = {}
    for bone, keys in clip.tracks.items():
        if not keys:
            continue
        if t <= keys[0].time_s:
            k = keys[0]
            pose[bone] = (*k.rotation_deg, *k.translation_cm)
            continue
        if t >= keys[-1].time_s:
            k = keys[-1]
            pose[bone] = (*k.rotation_deg, *k.translation_cm)
            continue
        for a, b in zip(keys, keys[1:]):
            if a.time_s <= t <= b.time_s:
                f = 0.0 if b.time_s == a.time_s else (t - a.time_s) / (b.time_s - a.time_s)
                rot = tuple(a.rotation_deg[i] + (b.rotation_deg[i] - a.rotation_deg[i]) * f for i in range(3))
                tr = tuple(a.translation_cm[i] + (b.translation_cm[i] - a.translation_cm[i]) * f for i in range(3))
                pose[bone] = (*rot, *tr)
                break
    return pose


POSE_SAMPLES = [("idle_packed", 0.0), ("move_packed", 0.25), ("deploy", 0.15), ("deploy", 0.55), ("deploy", 0.82),
                ("idle_deployed", 0.5), ("drag_deployed", 0.25), ("pack", 0.55), ("damage", 0.2),
                ("death", 1.0), ("cancel", 0.0), ("restore", 0.0)]


# --- manifest and CLI ---------------------------------------------------------------------------
def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "barrier_cells": {"contract": 6, "built": sum(1 for c in comps if c.endswith("_frame") and c.startswith("cell_")),
                          "per_wing": [3, 3], "fields": sum(1 for c in comps if c.startswith("cell_") and c.endswith("_field")),
                          "cyan_edge_lines": sum(1 for c in comps if c.startswith("cell_") and c.endswith("_edge")),
                          "opaque_back_plates": sum(1 for c in comps if c.startswith("cell_") and c.endswith("_backplate"))},
        "barrier_wings": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_arm") and c.startswith("wing_"))},
        "operator_stations": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_cowl") and c.startswith("op_")),
                              "visors": sum(1 for c in comps if c.startswith("op_") and c.endswith("_visor"))},
        "legs": {"contract": 4, "built": sum(1 for c in comps if c.startswith("leg_") and c.endswith("_foot"))},
        "central_emitter": {"contract": 1, "built": sum(1 for c in comps if c == "emitter_housing"),
                            "lens": sum(1 for c in comps if c == "emitter_lens")},
        "chassis": {"contract": 1, "built": 1 if "chassis_frame" in comps else 0,
                    "open_rear": rear_open_measure(m)["rear_face_open"]},
        "bones": {"contract": 18, "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": ["Target_Anchor_Center", "Emitter_Muzzle", "Shield_Face_Center",
                                 "Cell_01", "Cell_02", "Cell_03", "Cell_04", "Cell_05", "Cell_06"],
                    "built": [sk.name for sk in m.sockets]},
        "tracks": {"contract": ["idle_packed", "move_packed", "deploy", "idle_deployed", "drag_deployed", "pack",
                                "damage", "death", "cancel", "restore"],
                   "built": [c.name for c in clips]},
        "sub_objects": {"contract": ["Left_Shield_Panel", "Right_Shield_Panel"], "built": [LEFT_PART, RIGHT_PART]},
    }


def build_outputs(export_dir: str, review_dir: str) -> tuple:
    """Write every export into export_dir and every review OBJ into review_dir. Output paths are
    recorded relative to the package so --check can compare them wherever they were written."""
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)
    outputs, review, meshes = [], [], {}
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, counts, sockets_on_bones = assemble(lod)
        meshes[("whole", lod)] = m
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod}
        stem = f"{ASSET}_LOD{lod}"
        glb = skel.write_skinned_glb(m, s, os.path.join(export_dir, stem + ".glb"), animations=clips,
                                     include_collision=False, sockets_on_bones=sockets_on_bones,
                                     extras=dict(extras, kind="skinned + clips"))
        outputs.append({"path": f"export/{stem}.glb", "sha256": glb, "lod": lod, "mesh": ASSET, "kind": "skinned + clips",
                        "triangles": m.triangle_count(), "by_slot": m.triangle_count_by("slot"), "by_bone": counts,
                        "bounds_cm": m.bounds(), "clips": [c.name for c in clips],
                        "sockets": [{"name": sk.name, "position_cm": sk.position, "bone": sockets_on_bones.get(sk.name)} for sk in m.sockets]})
        static = m.write_glb(os.path.join(export_dir, stem + "_static.glb"),
                             extras=dict(extras, kind="deployed rest pose, static"), include_collision=(lod == 0))
        outputs.append({"path": f"export/{stem}_static.glb", "sha256": static, "lod": lod, "mesh": ASSET,
                        "kind": "static deployed rest", "triangles": m.triangle_count(),
                        "collision_boxes": [{"name": c.name, "center_cm": c.center, "size_cm": c.size} for c in m.collision] if lod == 0 else []})
        obj = m.write_obj(os.path.join(export_dir, stem + ".obj"),
                          header_lines=[f"Production ID {PRODUCTION_ID}", f"Revision {REVISION}", f"LOD{lod} deployed rest"])
        outputs.append({"path": f"export/{stem}.obj", "sha256": obj, "lod": lod, "mesh": ASSET, "kind": "deployed rest OBJ"})
        for side, part_name in (("l", LEFT_PART), ("r", RIGHT_PART)):
            part = wing_part(lod, side)
            meshes[(side, lod)] = part
            pstem = f"{part_name}_LOD{lod}"
            pglb = part.write_glb(os.path.join(export_dir, pstem + ".glb"),
                                  extras=dict(extras, kind="sub-object", pivot="wing root hinge"), include_collision=False)
            pobj = part.write_obj(os.path.join(export_dir, pstem + ".obj"),
                                  header_lines=[f"{part_name} LOD{lod}; pivot at the wing root hinge"])
            outputs.append({"path": f"export/{pstem}.glb", "sha256": pglb, "lod": lod, "mesh": part_name,
                            "kind": "sub-object (card: physical transformation)", "triangles": part.triangle_count(),
                            "bounds_cm": part.bounds(), "pivot_world_cm": [WING_PIVOT[0], (-1.0 if side == "l" else 1.0) * WING_PIVOT[1], WING_PIVOT[2]]})
            outputs.append({"path": f"export/{pstem}.obj", "sha256": pobj, "lod": lod, "mesh": part_name, "kind": "sub-object OBJ"})
        # review assemblies: both states, both LODs
        states = [("deployed", m), ("packed", packed_mesh(lod))]
        if lod == 0:
            states += [("deployed_glass_cutaway", state_mesh(m, field="cut")),
                       ("packed_field_dark", state_mesh(packed_mesh(lod), field="dark", edges="dark"))]
        for state, mesh in states:
            rpath = os.path.join(review_dir, f"{ASSET}_{state}_LOD{lod}.obj")
            review.append({"path": rpath, "sha256": mesh.write_obj(rpath, header_lines=[f"{state} state LOD{lod}; review only"]),
                           "lod": lod, "state": state, "triangles": mesh.triangle_count(), "bounds_cm": mesh.bounds()})
    for name, fraction in POSE_SAMPLES:
        clip = next(c for c in clips if c.name == name)
        mesh = posed(0, sample_pose(clip, fraction))
        stem = f"pose_{name}_{int(round(fraction * 100)):03d}"
        rpath = os.path.join(review_dir, stem + ".obj")
        review.append({"path": rpath, "sha256": mesh.write_obj(rpath, header_lines=[f"Posed still: clip {name} at {fraction:.2f}"]),
                       "lod": 0, "clip": name, "fraction": fraction, "triangles": mesh.triangle_count(),
                       "bounds_cm": mesh.bounds(),
                       "sockets": [{"name": s.name, "position_cm": [round(c, 2) for c in s.position]} for s in mesh.sockets]})
    return outputs, review, meshes, clips


def build_manifest(outputs: list, review: list, meshes: dict, clips: list) -> dict:
    m0, m1 = meshes[("whole", 0)], meshes[("whole", 1)]
    skeleton = build_skeleton()
    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    measurements = concept_measurements(m0)
    packed = packed_measure(0)
    ground = {}
    for name, fraction in POSE_SAMPLES:
        clip = next(c for c in clips if c.name == name)
        ground[f"{name}@{fraction:.2f}"] = round(posed(0, sample_pose(clip, fraction)).bounds()[0][2], 3)
    manifest = {
        "author": AUTHOR, "creator": AUTHOR, "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID,
        "asset_name": ASSET, "parts": [ASSET, LEFT_PART, RIGHT_PART],
        "revision": REVISION, "kit_revision": kit.KIT_REVISION, "skel_revision": skel.SKEL_REVISION,
        "stage": "BLOCKOUT (concept-v2)",
        "stage_boundary": "Concept-matched deployed rest geometry, 18-bone transformation rig, keyframed clip data and the packed state as a posed review assembly; no textures, no import, no gate acceptance, no owner acceptance. The deployed rear read is geometry (opaque pane back plates, measured 0.0 percent barrier field frontmost from -X); the packed flank read is a runtime material requirement, measured and recorded as README section 8 deviation 16.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters at unit scale 1.0", "axes": "+X forward (the shield faces +X), +Y right, +Z up",
                  "pivot": "ground-contact centre (root)", "nanite": False,
                  "runtime_presentation_scale": PRESENTATION_SCALE_HEAVY,
                  "runtime_presentation_note": "EchoesEntityView.cpp:1817 draws EntityType::HeavyUnit at 1.75; not part of the asset. At 1.75 the 500 cm authored face draws 875 cm wide against a 500 cm authoritative cover width - README section 8 OWNER-QUESTION."},
        "scale_basis": {
            "W_cm": W,
            "source": "concept-fidelity.md: the deployed face is authored at W = 500 cm to match cover_half_width_cm 250 (Content/Data/Source/units.json mc_bulwark_team).",
            "reference_measurement": "bulwark-centered-wings-reference.png deployed panel, wall outer frame 825 x 462 px, ground row 882; 1 px = 0.60606 cm.",
            "measured_px": MEASURED_PX,
            "status": "CONCEPT-MEASURED BLOCKOUT (concept-v2)",
        },
        "dimensions_cm": {
            "wall_width": WALL_W, "wall_height": CELL_H, "wall_bottom_z": WALL_BOTTOM_Z, "wall_top_z": WALL_TOP_Z,
            "wall_face_x": WALL_FACE_X, "cell_pitch": round(CELL_PITCH, 4), "cell_width": CELL_W, "cell_thickness": CELL_T,
            "cell_frame_border": CELL_FRAME, "cell_chamfer": CELL_CHAMFER,
            "chassis": [CHASSIS_X0, CHASSIS_X1, CHASSIS_W, CHASSIS_Z0, round(CHASSIS_Z1, 2)],
            "operator_centre_y": OPERATOR_Y, "operator_top_z": round(OPERATOR_TOP_Z, 2),
            "leg_centre_y": LEG_Y, "foot_centre_y": FOOT_Y_CM, "foot": [FOOT_L, FOOT_W, FOOT_H],
            "emitter": [EMIT_X, 0.0, EMIT_Z, EMIT_R, LENS_R],
            "wing_pivot": list(WING_PIVOT), "pack_yaw_deg": PACK_YAW, "pack_translation_cm": list(PACK_T),
        },
        "concept_measurements": measurements,
        "packed_state": packed,
        "ground_contact_min_z_cm": ground,
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": b.head, "purpose": b.purpose} for b in skeleton.bones],
                "policy": "identity rest orientation; the rest pose is DEPLOYED; the packed state is the wing-root yaw+translate pose (PACK_YAW/PACK_T); no root motion; the impact ripple and the 120 deg gradient are material work on the field slot, not geometry",
                "bone_count_note": "the card requires 18; the fidelity file's prose list enumerates 16. The emitter and shield_anchor bones carry the remaining two and each drives a named socket (README section 8)."},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND, 1),
                   "loop": c.loop, "bones": sorted(c.tracks), "purpose": c.purpose} for c in clips],
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "lod0_cap": LOD0_CAP, "lod1_cap": LOD1_CAP,
                    "lod0_within_cap": m0.triangle_count() <= LOD0_CAP, "lod1_within_cap": m1.triangle_count() <= LOD1_CAP,
                    "lod1_over_lod0": round(m1.triangle_count() / m0.triangle_count(), 3),
                    "per_part": {name: {"lod0": meshes[(key, 0)].triangle_count(), "lod1": meshes[(key, 1)].triangle_count()}
                                 for key, name in (("whole", ASSET), ("l", LEFT_PART), ("r", RIGHT_PART))},
                    "status_cyan_area_fraction_lod0": measurements["status_cyan_area_fraction"],
                    "shield_field_area_fraction_lod0": measurements["shield_field_area_fraction"]},
        "material_slots": list(SLOTS),
        "material_slot_policy": "4 slots. FRAME: charcoal machined frame, hubs, hinges, treads AND the six opaque cell back plates that stop the field and the pane rims ever reading from behind the unit. CERAMIC: pale plates. SHIELD FIELD: the six cell field slabs only. STATUS CYAN (full list, no omissions): two operator visors, the emitter lens, the six cell pane rims, the chassis bay core and the two flank conduit lines - the bay core and the conduits are the reference's own machined-conduit glow and are 26.7 percent of the cyan area. The six pane rims must light and darken WITH the field material, not with the status channel (README section 8 deviation 15). The card sets no slot ceiling; brass edge trim, scuff noise and the team mask are texture channels of the ceramic slot.",
        "material_rules": {"roughness_floor": ROUGHNESS_FLOOR, "texture_stack": "2048x2048 PBR",
                           "directional_gradient_deg": 120,
                           "note": "the 120 deg directional gradient and the chromatic impact ripple are material work on MI_EBS_MER_ShieldField; no geometry implements them"},
        "component_inventory": contract_inventory(m0, skeleton, clips),
        "outputs": outputs, "review_assemblies": review,
        "tools": {"builder_sha256": builder_sha, "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "fidelity_target": {"path": os.path.join(HERE, "concept-fidelity.md")},
            "owner_corrected_reference": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/bulwark-review/bulwark-centered-wings-reference.png",
                                          "sha256_prefix": "6a8c5cef6697f84f", "role": "PRODUCTION TARGET (owner correction: centre the six panes)"},
            "concept_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/site/assets/concepts/meridian-units.png",
                              "region": [0, 0.5, 0.5, 1], "sha256_prefix": "427e60cd27bd78e9", "status": "KEEP as DESIGN_IDENTITY (EBS-CON-MER-UNT-003)"},
            "superseded_references": [{"path": ".../bulwark-six-panel-reference.png", "sha256_prefix": "7574643acfde08f0", "defect": "wall off-centre"},
                                      {"path": ".../bulwark-reference.png", "sha256_prefix": "55e15667ecf2c6a9", "defect": "five visible panes"}],
            "canon_row": {"path": "Docs/Archive/DevelopmentBible.md", "line": 511},
            "gameplay_record": {"path": "Content/Data/Source/units.json", "id": "mc_bulwark_team",
                                "deployment": {"cover_depth_cm": 350, "cover_half_width_cm": 250, "damage_reduction_percent": 40, "move_speed_percent": 35}},
            "contract": {"path": "Docs/VisualAssetPipeline/reference-packages.json", "package_id": PACKAGE_ID},
            "review_decision": {"path": "Docs/VisualAssetPipeline/review-selections.json", "concept_id": "EBS-CON-MER-UNT-003", "choice": "KEEP"},
            "requirements": ["SPEC-UNIT-003", "REL-FAC-025.MC.BULWARK", "REL-FAC-025.MC.BULWARK.ASSET", "REL-ART-005", "REL-ART-006"],
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }
    return manifest


def main_cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--check", action="store_true",
                        help="read-only: rebuild into a temporary directory and compare every export and review hash with build-manifest.json")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        if not os.path.exists(manifest_path):
            print(json.dumps({"check": "no-manifest"}))
            return 2
        with open(manifest_path, "r", encoding="utf-8") as handle:
            previous = json.load(handle)
        with tempfile.TemporaryDirectory(prefix="ebs-bulwark-check-") as tmp:
            outputs, review, _meshes, _clips = build_outputs(os.path.join(tmp, "export"), os.path.join(tmp, "review"))
        prev_out = {o["path"]: o["sha256"] for o in previous.get("outputs", [])}
        prev_review = {os.path.basename(r["path"]): r["sha256"] for r in previous.get("review_assemblies", [])}
        drift = [o["path"] for o in outputs if prev_out.get(o["path"]) != o["sha256"]]
        drift += [os.path.basename(r["path"]) for r in review if prev_review.get(os.path.basename(r["path"])) != r["sha256"]]
        missing = [p for p in prev_out if not os.path.exists(os.path.join(HERE, p))]
        missing += [p for p in prev_review if not os.path.exists(os.path.join(args.evidence_dir, "review", p))]
        ok = not drift and not missing and previous.get("revision") == REVISION
        print(json.dumps({"check": "ok" if ok else "drift", "revision": REVISION, "manifest_revision": previous.get("revision"),
                          "drift": drift, "missing": missing,
                          "compared": {"outputs": len(outputs), "review_assemblies": len(review)}}))
        return 0 if ok else 3
    outputs, review, meshes, clips = build_outputs(os.path.join(HERE, "export"), os.path.join(args.evidence_dir, "review"))
    manifest = build_manifest(outputs, review, meshes, clips)
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(manifest, indent=1, sort_keys=True) + "\n")
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"],
                      "deployed_face": manifest["concept_measurements"]["deployed_face"],
                      "packed": manifest["packed_state"], "rear_open": manifest["concept_measurements"]["rear_open"],
                      "ground_min_z": manifest["ground_contact_min_z_cm"],
                      "inventory": {k: v.get("built") for k, v in manifest["component_inventory"].items()}}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
