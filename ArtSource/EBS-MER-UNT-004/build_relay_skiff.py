#!/usr/bin/env python3
"""EBS-MER-UNT-004 Relay Skiff: deterministic source geometry, 8-bone hover rig and clips (concept-v2 blockout).

Author: Angelis Pseftis.

Design authority (owner ruling 2026-09-06): the selected concept images define what the asset
looks like; gameplay rules bound the concept but never replace it. Target record:
  concept-fidelity.md (this folder). Sources looked at, not described:
  * relay-skiff-candidate.png (SELECTED CANDIDATE, three panels: main three-quarter, SIDE VIEW,
    TACTICAL SILHOUETTE). Read on the pixels: a long, low, flat-decked hull floating clear of the
    ground; a plan silhouette that is a stretched hexagon with a blunt chamfered nose (a recessed
    cyan-lit emitter bay under the nose lip) and a squared tail with two rearward pipe stubs; six
    charcoal lift pods with cyan intake strips slung under the hull edges; ONE segmented charcoal
    mast on the deck at the REAR quarter carrying a small dish, two whip antennas and a slack cable
    loop back to the deck; a sealed pale ceramic archive cradle strapped to the deck centre; an
    orthogonal pipe/rail framework running the deck edges and wrapping the tail.
  * lancer-candidate.png (faction/style reference the candidate was made from): pale ceramic plates
    over a charcoal frame, brass edge trim, small cyan strips, matte, nothing polished.
  * EBS-CON-MER-UNT-004 (meridian-units.png bottom-right, decision REPLACE): retained history only;
    its pointed canopy and ornamental ring assembly are deliberately absent.
  Four prose lines of concept-fidelity.md are contradicted by the candidate's own pixels and the
  pixels win (owner ruling); all four are recorded in README section 8.2 with the measurement:
  the mast stands at the REAR quarter (0.26 L ahead of the tail), not the "front-left quarter"
  (item 4, and the same stale wording survives in item 2's fidelity-check line);
  the hull plan is 0.333 L wide (176 px of a 531 px length in the TACTICAL SILHOUETTE), not 0.42 L;
  and there are TWO lift pods per side, not three (item 3) - the SIDE VIEW underside bottoms out at
  y 490 only over x 1027-1140 and x 1298-1395 of a hull spanning x 951-1484, and between them
  (x 1141-1296) it sits at y 470-478, 12-20 px higher, with rails, a keel pipe and small fittings
  but no pod body, no end caps and no cyan intake; the TACTICAL SILHOUETTE shows the same two
  bumps per hull edge.

concept-v2 (2026-09-07) closes the verified defects of concept-v1; every change below is a pixel
measurement on the candidate, listed with the panel it came from:
  * lift pods 6 -> 4 (two per side) at the measured deep-run centres 0.738 L and 0.238 L from the bow.
  * dish rest aim 35 deg starboard / 16 deg up -> 10 deg / 28 deg, and the dish hub raised from
    0.49 L to 0.554 L, so the disc reads as a disc in the TACTICAL SILHOUETTE and sits level with the
    mast head as the SIDE VIEW draws it (concept dish centre 295 px above the ground line of a 533 px hull).
  * squared tail face 0.234 L -> 0.278 L: the stern structure holds 146-150 px of the 533 px plan
    length from x 968 to x 1008. The 124 px cited by concept-v1 is only reached at x 964, nine pixels
    from the stern tip, and is not the tail face.
  * the pale ceramic flank is split from the hull body: the SIDE VIEW draws the pale plate over
    y 415-451 inside a hull band running y 392 (deck rail top) to y 478 (underframe bottom), i.e. 42%
    of the band, so hull_shell now stops at 0.165 L and a charcoal hull_deck_band carries the deck.
  * the secondary emitter barrel is the full 0.11 L of item 6 (concept-v1 built 0.095 L) with the tip
    exactly on the nose face.

Package contract: Docs/VisualAssetPipeline/motion/gap-decisions.json
  production_policy[package_id == "EBS-PKG-MC-RELAY-SKIFF"]: component inventory hull/relay_mast/
  dish/archive_cradle/secondary_emitter, 8-bone source count, sockets Scouting_Sensor_Pod and
  Logistics_Relay_Beam, LOD0 <= 5,000 / LOD1 <= 2,100 tris, 2 material slots, emissive <= 8% of the
  visible area, Nanite OFF, pivot "ground-contact center; flying hull uses ground-projected center
  and separate hover offset", and the 14-track required inventory reproduced in TRACK_CONTRACT.
Canon row: Docs/Archive/DevelopmentBible.md line 512 (SPEC-UNIT-004).
Requirement card: REL-FAC-025.MC.SKIFF.ASSET (.MESH_PROP/.TEX_MAPS/.MAT_RULE/.ANIM_RIG/.VFX_POLY).

Rig (8 bones, identity rest orientation): root, hull, mast, dish, pod_bank_l, pod_bank_r, emitter,
cradle. root is the GROUND point under the hull centre and is never keyed, so the runtime's hover
offset is additive on top of the authored 0.09 L hover gap (test_pivot_is_ground_point_hover_additive).
The archive cradle is a separate component group (deck_archive_*) and a separate export
(SM_EBS_MER_UNT_004_ArchiveCradle) so loaded and unloaded both read.

Conventions: cm, +X forward, +Y right, +Z up, root at the ground-contact centre under the hull
centre, no root motion, Nanite off.
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
PACKAGE_ID = "EBS-PKG-MC-RELAY-SKIFF"
PRODUCTION_ID = "EBS-MER-UNT-004"
ASSET = "SK_EBS_MER_UNT_004"
CRADLE_ASSET = "SM_EBS_MER_UNT_004_ArchiveCradle"
REVISION = "ebs-mer-unt-004-concept-v3"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_004/"

CERAMIC = "MI_EBS_MER_UnitCeramic"        # pale ceramic plates; brass trim and team mask are texture channels
FRAME = "MI_EBS_MER_UnitFrame"            # charcoal structure, pods, mast, rails
STATUS_REVIEW = "MI_EBS_MER_StatusCyan"   # review-only pseudo slot: folds into CERAMIC for the 2-slot export

# --- concept measurements ------------------------------------------------------------------------
# L is the hull length. Every proportion below is a ratio of L taken from concept-fidelity.md
# ("measured on the candidate"); ratios marked PIXELS were re-measured on relay-skiff-candidate.png
# because the prose disagrees with the image (README section 8). Pixel basis: in both the SIDE VIEW
# and TACTICAL SILHOUETTE panels the craft is 531 px long (side view x 953..1484, top view x 958..1489)
# and the ground shadow centre sits at y 503, so 1 px = L / 531.
L = 360.0                       # concept-fidelity.md scale basis (PROVISIONAL): 500 cm/s skimmer carrying a cassette rack

HULL_LEN = L                                  # nose x = +L/2, tail x = -L/2
HULL_HALF_W = 0.1665 * L                      # PIXELS: plan width 0.333 L (176 px); prose says 0.42 L
HULL_Z0 = 0.09 * L                            # item 2: underside of the hull above the ground
HULL_THICK = 0.10 * L                         # item 1: hull BODY thickness (underside to deck top)
HULL_Z1 = HULL_Z0 + HULL_THICK                # item 2: deck top at 0.19 L
HULL_PALE_THICK = 0.075 * L                   # PIXELS: the pale ceramic flank is only part of the body.
HULL_PALE_Z1 = HULL_Z0 + HULL_PALE_THICK      #   SIDE VIEW: pale plate y 415-451 inside a band running
                                              #   y 392 (deck rail top) to y 478 (underframe bottom) =
                                              #   42% of the band; concept-v1 gave the pale plate 61%.
                                              #   Pale 0.090-0.165 L, charcoal deck band 0.165-0.19 L.
NOSE_HALF_W = 0.067 * L                       # PIXELS: blunt nose face 0.134 L across (71 px)
NOSE_CHAMFER_X = 0.35 * L                     # PIXELS: plan chamfer starts 0.85 L from the tail
TAIL_HALF_W = 0.139 * L                       # PIXELS: the squared stern structure holds 146-150 px of
                                              #   the 533 px TACTICAL SILHOUETTE length from x 968 to
                                              #   x 1008 (0.274-0.281 L); half-width 0.139 L. concept-v1
                                              #   cited "124 px", which is only reached at x 964, nine
                                              #   pixels from the stern tip, and is not the tail face.
TAIL_CHAMFER_X = -0.40 * L                    # PIXELS: the plan steps out to full width at x 1010,
                                              #   0.895 L from the bow = x -0.395 L (drawn as a step,
                                              #   built as a short chamfer)
PROW_Z0 = HULL_Z0 + 0.021 * L                 # the prow chamfers up toward the nose
PROW_Z1 = HULL_Z1 - 0.012 * L

DECK_PLATE_W = 0.058 * L                      # PIXELS: pale ceramic strip along each deck edge in the plan
DECK_DARK_HALF_W = HULL_HALF_W - DECK_PLATE_W  # the charcoal deck centre band between them

KEEL_HALF_W = 0.083 * L                       # card .MESH_PROP: "main hull floats cleanly above an engineered anti-gravity radiator core"
KEEL_Z1 = HULL_Z0 + 0.005 * L
KEEL_Z0 = HULL_Z0 - 0.035 * L
KEEL_X = (-0.42 * L, 0.39 * L)

POD_LEN = 0.14 * L                            # item 3: serviceable lift pods
POD_W = 0.050 * L
POD_H = 0.062 * L
POD_Y = 0.133 * L                             # slung under the hull edges
POD_BOTTOM_Z = 0.028 * L                      # PIXELS: pods hang to 0.028 L above the ground (y 490 of a 503 ground line)
POD_STATIONS_X = (-0.238 * L, 0.262 * L)      # PIXELS: TWO pods per side, not three (item 3 says three).
                                              #   SIDE VIEW underside, hull x 951-1484: the outline drops
                                              #   to y 490 only over x 1027-1140 (flat run x 1072-1109)
                                              #   and x 1298-1395 (flat run x 1339-1375); between them,
                                              #   x 1141-1296, it holds y 470-478 with rails, a keel pipe
                                              #   and fittings but no pod body, end caps or cyan intake.
                                              #   Flat-run centres are 0.738 L and 0.238 L from the bow,
                                              #   i.e. x -0.238 L and +0.262 L. The TACTICAL SILHOUETTE
                                              #   and the three-quarter panel show the same two per side.

RAIL_Y = HULL_HALF_W + 0.017 * L              # PIXELS: the pipe framework runs OUTBOARD of the pale hull plates (card .MESH_PROP "orthogonal rail framework")
RAIL_Z = HULL_Z1 + 0.016 * L
RAIL_R = 0.013 * L
RAIL_X = (-0.44 * L, 0.36 * L)
CROSS_RAIL_X = (-0.335 * L, -0.075 * L, 0.145 * L, 0.30 * L)
TAIL_STUB_BACK = 0.045 * L                    # PIXELS: two pipe stubs projecting behind the squared tail
TAIL_STUB_Y = 0.055 * L
TAIL_STUB_Z = HULL_Z0 + 0.035 * L

MAST_X = -0.24 * L                            # PIXELS: mast column centre 0.26 L ahead of the tail (REAR quarter)
MAST_R = 0.026 * L                            # PIXELS: a chunky segmented column, not a pole
MAST_TOP_Z = 0.55 * L                         # item 4: segmented charcoal column
MAST_SEGMENTS = 3
WHIP_TOP_Z = (0.71 * L, 0.65 * L)             # PIXELS: two thin whip antennas above the mast head
DISH_D = 0.13 * L                             # item 4: small parabolic dish. The two panels disagree on the
                                              #   size: the TACTICAL SILHOUETTE draws the disc 79 px across
                                              #   (0.149 L) and the SIDE VIEW draws it 59 px (0.111 L).
                                              #   0.13 L is the prose value and the mid-point of the two.
DISH_Y = 0.078 * L                            # mounted on the mast's side (starboard, clear of the column)
# Rest aim. Measured on both panels as the axis-aligned footprint of the disc, which is what a render can
# be measured against: TACTICAL SILHOUETTE 48.8 x 79.1 px = 33.1 x 53.6 cm (along hull x across hull);
# SIDE VIEW 29 x 59 px = 19.6 x 39.8 cm (along hull x vertical), major axis leaning 66 deg from horizontal
# with the top aft. A disc of diameter d and unit normal n projects an axis-aligned extent of
# d*sqrt(1 - (n.u)^2) along each image axis u, so the pair (yaw, pitch) below is the best joint fit at the
# built 0.13 L diameter: plan 23.1 x 46.2, side 23.1 x 41.3 cm, side major axis 62 deg.
# concept-v1's 35 deg / 16 deg gave plan 28.8 x 39.0 and side 28.8 x 45.0 and a projected plan area of
# 474 cm2 against the concept's 1,393; this gives 808 cm2. The concept's 53.6 cm across-hull footprint
# cannot be reached at all with a 46.8 cm dish - see README section 8.3.
DISH_REST_YAW = 10.0
DISH_REST_PITCH = 28.0
DISH_Z = 0.554 * L                            # PIXELS: SIDE VIEW dish feed centroid at y 207.7 with the
                                              #   ground line at y 503 = 295 px of a 533 px hull above the
                                              #   ground (199.5 cm); the disc centre sits just under the
                                              #   mast head top (y 199 = 205 cm). concept-v1 put it at
                                              #   0.49 L, 33 cm below the head block.
DISH_X = MAST_X + 0.017 * L

CRADLE_LEN = 0.42 * L                         # item 5: sealed pale ceramic cassette rack
CRADLE_W = 0.20 * L
CRADLE_H = 0.13 * L
CRADLE_X = 0.0                                # lashed to the deck centre
CRADLE_STRAP_X = (-0.105 * L, 0.105 * L)      # two dark straps

EMITTER_LEN = 0.11 * L                        # item 6: short charcoal barrel, clearly secondary. The BUILT
                                              #   barrel is the full 0.11 L: concept-v1 drew the tube from
                                              #   0.006 L to 0.92 * EMITTER_LEN, i.e. 0.095 L of geometry
                                              #   against a 0.11 L constant, and the README quoted the constant.
EMITTER_R = 0.014 * L
EMITTER_X = 0.39 * L                          # mount under the nose lip; the barrel tip lands exactly on the
                                              #   nose face at x = +L/2 (EMITTER_X + EMITTER_LEN = 180 cm)
EMITTER_Z = HULL_Z0 - 0.008 * L               # slung below the prow underside so it reads from the front and below

REFERENCE_FIGURE_CM = 180.0                   # review scenes only
HOVER_BOB_MAX_CM = 4.0                        # concept-fidelity.md rig line: idle hover bob <= 4 cm
TRACK_CONTRACT = ("idle", "move", "turn", "stop", "damage", "death", "cancel", "restore",
                  "attack_anticipation", "attack_execution", "attack_recovery",
                  "relay_activation", "relay_hold", "relay_expiry")
# The frozen package contract names the relay action "relay_activation"; concept-fidelity.md (and the
# production handoff) names the same track "relay_extend". One clip is authored, under the fidelity
# target's name, and the contract name is carried as a documented alias (README section 8, OWNER-QUESTION 1).
TRACK_ALIAS = {"relay_activation": "relay_extend"}
SIM_FOOTPRINT_HALF_EXTENT_CM = 12.5           # EchoesContentSubsystem.cpp:363 kFixedScale/8 on a 100 cm tile


def hull_outline(inset: float = 0.0):
    """Plan silhouette: a stretched hexagon, parallel sides, blunt chamfered nose, squared tail."""
    return [
        (NOSE_CHAMFER_X - inset, HULL_HALF_W - inset),
        (TAIL_CHAMFER_X + inset, HULL_HALF_W - inset),
        (-HULL_LEN / 2.0 + inset, TAIL_HALF_W - inset),
        (-HULL_LEN / 2.0 + inset, -(TAIL_HALF_W - inset)),
        (TAIL_CHAMFER_X + inset, -(HULL_HALF_W - inset)),
        (NOSE_CHAMFER_X - inset, -(HULL_HALF_W - inset)),
    ]


# --- parts -----------------------------------------------------------------------------------------
def build_hull(lod: int) -> kit.Mesh:
    """Thin flat-decked hull, radiator keel, prow with the emitter recess, deck rail framework."""
    m = kit.Mesh("hull")
    ceramic, frame, status = m.slot(CERAMIC), m.slot(FRAME), m.slot(STATUS_REVIEW)
    sides = 8 if lod == 0 else 6
    # main hull body in two bands, as the candidate's SIDE VIEW draws it: a pale ceramic flank plate
    # riding on the deep charcoal frame, with a charcoal deck band above it under the rails. The body
    # (0.09-0.19 L) is unchanged; only the pale/dark break moved (0.19 L -> 0.165 L).
    m.prism(hull_outline(), HULL_Z0, HULL_PALE_Z1, ceramic, "hull_shell", cap_top=True, cap_bottom=True)
    m.prism(hull_outline(), HULL_PALE_Z1, HULL_Z1, frame, "hull_deck_band", cap_top=True, cap_bottom=True)
    # blunt chamfered prow: tapers in plan and in profile toward the nose (item 1)
    x0, x1 = NOSE_CHAMFER_X, HULL_LEN / 2.0
    back = [(x0, HULL_HALF_W, HULL_Z0), (x0, -HULL_HALF_W, HULL_Z0), (x0, -HULL_HALF_W, HULL_Z1), (x0, HULL_HALF_W, HULL_Z1)]
    front = [(x1, NOSE_HALF_W, PROW_Z0), (x1, -NOSE_HALF_W, PROW_Z0), (x1, -NOSE_HALF_W, PROW_Z1), (x1, NOSE_HALF_W, PROW_Z1)]
    m.add_convex_solid([back, front,
                        [back[0], back[3], front[3], front[0]],
                        [back[1], front[1], front[2], back[2]],
                        [back[3], back[2], front[2], front[3]],
                        [back[0], front[0], front[1], back[1]]], ceramic, "hull_prow")
    # dark deck between pale outer deck plates, with the pale cradle standing on it: the candidate's
    # TACTICAL SILHOUETTE reads as a charcoal centre band with a pale ceramic strip along each deck edge
    # and a pale prow. The deck band above carries the charcoal, so the pale edge strips are their own
    # plates (in concept-v1 the whole deck top was the pale hull prism's cap).
    m.box((0.215 * L, 0.0, HULL_Z1 + 0.4), (0.245 * L, 2.0 * DECK_DARK_HALF_W, 1.2), frame, "hull_deck_panel")
    m.box((-0.155 * L, 0.0, HULL_Z1 + 0.4), (0.42 * L, 2.0 * DECK_DARK_HALF_W, 1.2), frame, "hull_deck_panel_aft")
    for sign in (1.0, -1.0):
        side = "r" if sign > 0 else "l"
        m.box((-0.025 * L, sign * (DECK_DARK_HALF_W + DECK_PLATE_W / 2.0), HULL_Z1 + 0.4),
              (0.75 * L, DECK_PLATE_W, 1.2), ceramic, f"hull_deck_plate_{side}")
    # radiator keel under the hull (card .MESH_PROP), with cyan intake strips on both flanks
    m.prism([(KEEL_X[1], KEEL_HALF_W), (KEEL_X[0], KEEL_HALF_W), (KEEL_X[0] - 0.02 * L, 0.0), (KEEL_X[0], -KEEL_HALF_W),
             (KEEL_X[1], -KEEL_HALF_W), (KEEL_X[1] + 0.03 * L, 0.0)], KEEL_Z0, KEEL_Z1, frame, "hull_keel")
    for sign in (1.0, -1.0):
        side = "r" if sign > 0 else "l"
        m.box((-0.02 * L, sign * (KEEL_HALF_W + 0.6), (KEEL_Z0 + KEEL_Z1) / 2.0), (0.44 * L, 1.2, 0.014 * L), status, f"hull_keel_vent_{side}")
        # pale flank strake over the charcoal frame (lancer-candidate plate language)
        m.box((-0.03 * L, sign * (HULL_HALF_W + 1.0), HULL_Z0 + 0.040 * L), (0.60 * L, 2.0, 0.030 * L), ceramic, f"hull_flank_strake_{side}")
        m.box((0.155 * L, sign * (HULL_HALF_W + 1.0), HULL_Z0 + 0.060 * L), (0.09 * L, 1.6, 0.012 * L), status, f"hull_flank_strip_{side}")
        # long charcoal spine pipe along the flank under the deck line (the candidate's side view)
        m.tube((-0.44 * L, sign * (HULL_HALF_W + 0.014 * L), HULL_Z0 + 0.016 * L), (0.33 * L, sign * (HULL_HALF_W + 0.014 * L), HULL_Z0 + 0.016 * L),
               0.012 * L, sides, frame, f"hull_flank_pipe_{side}")
        # deck rail framework: one long rail per deck edge (card .MESH_PROP "orthogonal rail framework")
        m.tube((RAIL_X[0], sign * RAIL_Y, RAIL_Z), (RAIL_X[1], sign * RAIL_Y, RAIL_Z), RAIL_R, sides, frame, f"hull_deck_rail_{side}")
        m.box((RAIL_X[0], sign * RAIL_Y, (RAIL_Z + HULL_Z1) / 2.0), (0.02 * L, 0.02 * L, RAIL_Z - HULL_Z1), frame, f"hull_rail_post_{side}_aft")
        m.box((RAIL_X[1], sign * RAIL_Y, (RAIL_Z + HULL_Z1) / 2.0), (0.02 * L, 0.02 * L, RAIL_Z - HULL_Z1), frame, f"hull_rail_post_{side}_fwd")
        # one pipe stub per side projecting behind the squared tail
        m.tube((-HULL_LEN / 2.0, sign * TAIL_STUB_Y, TAIL_STUB_Z), (-HULL_LEN / 2.0 - TAIL_STUB_BACK, sign * TAIL_STUB_Y, TAIL_STUB_Z),
               0.014 * L, sides, frame, f"hull_tail_stub_{side}")
    for i, cx in enumerate(CROSS_RAIL_X if lod == 0 else CROSS_RAIL_X[::2]):
        m.tube((cx, -RAIL_Y, RAIL_Z), (cx, RAIL_Y, RAIL_Z), RAIL_R * 0.9, sides, frame, f"hull_cross_rail_{i + 1:02d}")
    # transverse gantry beams at the mast station: the candidate's plan reads a heavy dark cluster there
    for i, dx in enumerate((-0.055 * L, 0.045 * L)):
        m.box((MAST_X + dx, 0.0, HULL_Z1 + 0.014 * L), (0.030 * L, 2.0 * (HULL_HALF_W - 0.004 * L), 0.028 * L), frame, f"hull_mast_gantry_{i + 1:02d}")
    # deck-side tie-down hardware for the archive cradle: part of the hull, so the unloaded deck still
    # reads as a strapped deck waiting for a load (fidelity check 4)
    for sx in CRADLE_STRAP_X:
        for sy in (-1.0, 1.0):
            m.box((sx, sy * (CRADLE_W / 2.0 + 0.6), HULL_Z1 + 0.013 * L), (0.038 * L, 0.020 * L, 0.026 * L), frame, "hull_cradle_tiedown")
            m.box((sx, sy * (CRADLE_W / 2.0 + 0.6), HULL_Z1 + 0.001 * L), (0.052 * L, 0.030 * L, 0.006 * L), frame, "hull_cradle_tiedown_plate")
    # recessed bay in the nose face with a cyan slot (the candidate's nose crop), above the emitter
    m.box((HULL_LEN / 2.0 - 0.008 * L, 0.0, (PROW_Z0 + PROW_Z1) / 2.0), (0.020 * L, 0.086 * L, 0.036 * L), frame, "hull_nose_bay")
    m.box((HULL_LEN / 2.0 - 0.001 * L, 0.0, (PROW_Z0 + PROW_Z1) / 2.0 + 0.004 * L), (0.006 * L, 0.062 * L, 0.008 * L), status, "hull_nose_slot")
    # tail framework hoop wrapping the squared tail (top view read)
    m.tube((-HULL_LEN / 2.0 - 0.012 * L, -TAIL_HALF_W, HULL_Z1 - 0.012 * L), (-HULL_LEN / 2.0 - 0.012 * L, TAIL_HALF_W, HULL_Z1 - 0.012 * L),
           RAIL_R * 0.8, sides, frame, "hull_tail_hoop")
    if lod == 0:
        for sign in (1.0, -1.0):
            side = "r" if sign > 0 else "l"
            m.box((-0.155 * L, sign * (HULL_HALF_W - 0.012 * L), HULL_Z1 + 0.014 * L), (0.05 * L, 0.03 * L, 0.028 * L), frame, f"hull_deck_box_{side}")
            m.box((0.31 * L, sign * (NOSE_HALF_W + 0.030 * L), HULL_Z0 + 0.055 * L), (0.05 * L, 0.02 * L, 0.020 * L), ceramic, f"hull_nose_fence_{side}")
    return m


def build_pod(lod: int) -> kit.Mesh:
    """One serviceable lift pod hung under a hull edge; local frame: pivot at the hull underside."""
    m = kit.Mesh("pod")
    frame, status, ceramic = m.slot(FRAME), m.slot(STATUS_REVIEW), m.slot(CERAMIC)
    sides = 8 if lod == 0 else 6
    top = 0.0
    bottom = POD_BOTTOM_Z - HULL_Z0                     # negative: the pod hangs below the hull line
    cz = (top + bottom) / 2.0
    # charcoal cylinder slung along the hull line (item 3), on a strut, with a cyan intake strip
    m.tube((-POD_LEN / 2.0, 0.0, cz), (POD_LEN / 2.0, 0.0, cz), POD_H / 2.0, sides, frame, "pod_body")
    m.box((0.0, 0.0, bottom + 0.14 * POD_H), (POD_LEN * 0.76, POD_W * 1.10, POD_H * 0.26), frame, "pod_shoe")
    m.box((0.0, 0.0, top - 0.02 * L), (POD_LEN * 0.45, POD_W * 0.7, 0.05 * L), frame, "pod_strut")
    m.box((POD_LEN * 0.06, 0.0, cz), (POD_LEN * 0.42, POD_W + 1.4, POD_H * 0.16), status, "pod_intake")
    if lod == 0:
        m.box((-POD_LEN * 0.42, 0.0, cz + 0.2 * POD_H), (POD_LEN * 0.12, POD_W * 0.8, POD_H * 0.34), ceramic, "pod_cap")
        m.box((POD_LEN * 0.44, 0.0, cz + 0.2 * POD_H), (POD_LEN * 0.10, POD_W * 0.8, POD_H * 0.34), ceramic, "pod_cap_fwd")
    return m


def build_mast(lod: int) -> kit.Mesh:
    """Segmented charcoal relay mast; local frame: pivot at the mast base on the deck (0,0,0)."""
    m = kit.Mesh("mast")
    frame, status, ceramic = m.slot(FRAME), m.slot(STATUS_REVIEW), m.slot(CERAMIC)
    sides = 8 if lod == 0 else 5
    head_z = MAST_TOP_Z - HULL_Z1
    m.box((0.0, 0.0, 0.028 * L), (0.095 * L, 0.088 * L, 0.056 * L), frame, "mast_base")
    for sign in (1.0, -1.0):
        m.box((-0.030 * L, sign * 0.056 * L, 0.030 * L), (0.036 * L, 0.028 * L, 0.052 * L), frame, "mast_base_block")
    seg = head_z / MAST_SEGMENTS
    for i in range(MAST_SEGMENTS):
        z0, z1 = 0.045 * L + i * (head_z - 0.045 * L) / MAST_SEGMENTS, 0.045 * L + (i + 1) * (head_z - 0.045 * L) / MAST_SEGMENTS
        r = MAST_R * (1.0 - 0.12 * i)
        m.tube((0.0, 0.0, z0), (0.0, 0.0, z1), r, sides, frame, f"mast_column_{i + 1:02d}")
        m.tube((0.0, 0.0, z1 - 0.008 * L), (0.0, 0.0, z1 + 0.004 * L), r * 1.22, sides, frame, f"mast_collar_{i + 1:02d}")
        m.box((r * 0.98, 0.0, (z0 + z1) / 2.0), (0.9, 0.020 * L, seg * 0.42), status, f"mast_strip_{i + 1:02d}")
    m.box((0.0, 0.0, head_z + 0.012 * L), (0.048 * L, 0.048 * L, 0.040 * L), frame, "mast_head")
    m.box((-0.030 * L, 0.0, head_z - 0.020 * L), (0.024 * L, 0.036 * L, 0.048 * L), ceramic, "mast_avionics")
    for i, tip in enumerate(WHIP_TOP_Z):
        m.tube((-0.010 * L + i * 0.020 * L, 0.010 * L - i * 0.020 * L, head_z + 0.030 * L),
               (-0.010 * L + i * 0.020 * L, 0.010 * L - i * 0.020 * L, tip - HULL_Z1), 0.0055 * L, 5 if lod == 0 else 4, frame, f"mast_whip_{i + 1:02d}")
    if lod == 0:
        # slack cable loop from the dish yoke back down to the deck (item 4); the top end follows DISH_Z
        arc = [(0.020 * L, DISH_Y * 0.65, DISH_Z - HULL_Z1 - 0.022 * L), (0.055 * L, DISH_Y * 0.62, head_z * 0.72),
               (0.062 * L, DISH_Y * 0.55, head_z * 0.42), (0.040 * L, DISH_Y * 0.42, head_z * 0.16), (0.012 * L, DISH_Y * 0.30, 0.020 * L)]
        for i in range(len(arc) - 1):
            m.tube(arc[i], arc[i + 1], 0.0045 * L, 4, frame, "mast_cable", caps=False)
        m.box((0.0, 0.0, 0.062 * L), (0.056 * L, 0.056 * L, 0.012 * L), frame, "mast_base_collar")
    return m


def build_dish(lod: int) -> kit.Mesh:
    """Small parabolic dish, face along +X; local frame: pivot at the dish hub (0,0,0)."""
    m = kit.Mesh("dish")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    sides = 16 if lod == 0 else 8
    r = DISH_D / 2.0
    # stepped paraboloid: three short cylinders of falling radius stacked along +X
    steps = ((-0.006 * L, 0.004 * L, r), (0.004 * L, 0.013 * L, r * 0.72), (0.013 * L, 0.021 * L, r * 0.38))
    for i, (x0, x1, rad) in enumerate(steps):
        m.tube((x0, 0.0, 0.0), (x1, 0.0, 0.0), rad, sides, ceramic, f"dish_face_{i + 1:02d}")
    m.tube((-0.020 * L, 0.0, 0.0), (-0.006 * L, 0.0, 0.0), r * 0.22, sides if lod == 0 else 5, frame, "dish_hub")
    m.box((0.022 * L, 0.0, 0.0), (0.010 * L, 0.016 * L, 0.016 * L), status, "dish_feed")
    return m


def build_dish_yoke(lod: int) -> kit.Mesh:
    """Yoke arm that carries the dish off the mast head (rides with the mast, not the dish)."""
    m = kit.Mesh("dish_yoke")
    frame = m.slot(FRAME)
    m.box((0.0, DISH_Y / 2.0, 0.0), (0.020 * L, DISH_Y, 0.020 * L), frame, "mast_dish_arm")
    m.box((0.0, DISH_Y * 0.95, 0.0), (0.026 * L, 0.014 * L, 0.030 * L), frame, "mast_dish_gimbal")
    return m


def build_emitter(lod: int) -> kit.Mesh:
    """Secondary defence emitter under the nose; local frame: pivot at the mount (0,0,0), barrel along +X."""
    m = kit.Mesh("emitter")
    frame, status = m.slot(FRAME), m.slot(STATUS_REVIEW)
    sides = 8 if lod == 0 else 5
    m.box((-0.014 * L, 0.0, 0.006 * L), (0.046 * L, 0.044 * L, 0.038 * L), frame, "emitter_mount")
    # the barrel is the full item 6 length: 0.11 L of geometry, tip on the nose face
    m.tube((0.0, 0.0, 0.0), (EMITTER_LEN, 0.0, 0.0), EMITTER_R, sides, frame, "emitter_barrel")
    m.tube((EMITTER_LEN * 0.80, 0.0, 0.0), (EMITTER_LEN, 0.0, 0.0), EMITTER_R * 1.35, sides, frame, "emitter_muzzle")
    m.box((EMITTER_LEN * 0.40, 0.0, EMITTER_R * 1.0), (EMITTER_LEN * 0.40, 0.012 * L, 0.9), status, "emitter_strip")
    return m


def build_cradle(lod: int) -> kit.Mesh:
    """Sealed pale ceramic archive cradle; local frame: pivot at the deck contact centre (0,0,0).

    Exported separately as SM_EBS_MER_UNT_004_ArchiveCradle so the loaded/unloaded states read; in the
    assembly every component is prefixed deck_archive_ and bound to the cradle bone."""
    m = kit.Mesh(CRADLE_ASSET)
    ceramic, frame, status = m.slot(CERAMIC), m.slot(FRAME), m.slot(STATUS_REVIEW)
    hz = CRADLE_H / 2.0
    m.box((0.0, 0.0, hz), (CRADLE_LEN, CRADLE_W, CRADLE_H), ceramic, "deck_archive_case")
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            m.box((sx * (CRADLE_LEN / 2.0 - 0.012 * L), sy * (CRADLE_W / 2.0 - 0.008 * L), hz),
                  (0.030 * L, 0.024 * L, CRADLE_H * 1.04), ceramic, "deck_archive_corner")
    for sx in CRADLE_STRAP_X:
        m.box((sx, 0.0, CRADLE_H + 0.4), (0.030 * L, CRADLE_W * 1.04, 1.2), frame, "deck_archive_strap")
        for sy in (-1.0, 1.0):
            m.box((sx, sy * (CRADLE_W / 2.0 + 0.6), hz), (0.030 * L, 1.4, CRADLE_H), frame, "deck_archive_strap_side")
    m.box((0.115 * L, CRADLE_W / 2.0 + 0.4, hz + 0.020 * L), (0.055 * L, 0.8, 0.014 * L), status, "deck_archive_status")
    if lod == 0:
        m.box((-0.055 * L, 0.0, CRADLE_H + 1.2), (0.10 * L, CRADLE_W * 0.55, 1.6), ceramic, "deck_archive_lid_rib")
        m.box((0.075 * L, 0.0, CRADLE_H + 1.2), (0.06 * L, CRADLE_W * 0.55, 1.6), ceramic, "deck_archive_lid_rib_fwd")
    return m


# --- rig -------------------------------------------------------------------------------------------
def dish_hub() -> tuple:
    return (DISH_X, DISH_Y, DISH_Z)


def build_skeleton() -> skel.Skeleton:
    """8 bones (contract bone_count_source 8). root is the ground point under the hull centre."""
    s = skel.Skeleton(root="root")
    s.add("root", None, (0.0, 0.0, 0.0), "ground point under the hull centre; never keyed, so the runtime hover offset is additive")
    s.add("hull", "root", (0.0, 0.0, HULL_Z0 + HULL_THICK / 2.0), "hull centre; carries the harmonic hover bob, the nose-down lean and the turn bank")
    s.add("mast", "hull", (MAST_X, 0.0, HULL_Z1), "relay mast base on the deck; sway and the lit relay state")
    s.add("dish", "mast", dish_hub(), "dish gimbal; carries Scouting_Sensor_Pod. The clips key a fixed placeholder aim; the node-facing rotation is a runtime look-at layered on this bone")
    s.add("pod_bank_l", "hull", (0.0, -POD_Y, HULL_Z0), "port lift-pod bank (two pods): tilt on move, droop on death")
    s.add("pod_bank_r", "hull", (0.0, POD_Y, HULL_Z0), "starboard lift-pod bank (two pods)")
    s.add("emitter", "hull", (EMITTER_X, 0.0, EMITTER_Z), "secondary emitter mount: small aim, recoil and recovery")
    s.add("cradle", "hull", (0.0, 0.0, HULL_Z1), "archive cradle deck contact; separate component group, hidden when unloaded")
    return s


SOCKETS_ON_BONES = {"Scouting_Sensor_Pod": "dish", "Logistics_Relay_Beam": "mast"}


def assemble(lod: int, loaded: bool = True):
    """Assembled rest-pose mesh, skeleton, per-bone triangle counts and the socket->bone map."""
    m = kit.Mesh(ASSET)
    m.slot(CERAMIC)
    m.slot(FRAME)
    m.slot(STATUS_REVIEW)
    m.merge(build_hull(lod))
    for sign, side in ((-1.0, "l"), (1.0, "r")):
        for i, px in enumerate(POD_STATIONS_X):
            m.merge(build_pod(lod), translate=(px, sign * POD_Y, HULL_Z0), component_prefix=f"pod_{side}_{i + 1:02d}_")
    m.merge(build_mast(lod), translate=(MAST_X, 0.0, HULL_Z1))
    m.merge(build_dish_yoke(lod), translate=(DISH_X, 0.0, DISH_Z))
    m.merge(build_dish(lod), translate=dish_hub(), yaw_deg=DISH_REST_YAW, pitch_deg=DISH_REST_PITCH)
    m.merge(build_emitter(lod), translate=(EMITTER_X, 0.0, EMITTER_Z))
    if loaded:
        m.merge(build_cradle(lod), translate=(0.0, 0.0, HULL_Z1))
    hub = dish_hub()
    face = kit.rot_yp((0.024 * L, 0.0, 0.0), DISH_REST_YAW, DISH_REST_PITCH)
    m.sockets.append(kit.Socket("Scouting_Sensor_Pod", (hub[0] + face[0], hub[1] + face[1], hub[2] + face[2]), DISH_REST_YAW,
                                "dish face: sensor pod origin (sight 1,500 cm), turns with the dish bone"))
    m.sockets.append(kit.Socket("Logistics_Relay_Beam", (MAST_X, 0.0, MAST_TOP_Z), 0.0,
                                "mast head: origin of the Extend Relay cone toward a grid node within 700 cm"))
    binding = {"mast_": "mast", "dish_": "dish", "emitter_": "emitter", "deck_archive_": "cradle"}
    for i in range(len(POD_STATIONS_X)):
        binding[f"pod_l_{i + 1:02d}_"] = "pod_bank_l"
        binding[f"pod_r_{i + 1:02d}_"] = "pod_bank_r"
    s = build_skeleton()
    counts = skel.bind_polygons(m, "hull", binding)
    return m, s, counts, dict(SOCKETS_ON_BONES)


def export_mesh(m: kit.Mesh) -> kit.Mesh:
    """Two-slot export copy: the review-only cyan pseudo slot folds into the ceramic slot."""
    out = kit.Mesh(m.name)
    out.slot(CERAMIC)
    out.slot(FRAME)
    for poly in m.polygons:
        name = m.slots[poly.slot]
        slot = out.slot(CERAMIC if name == STATUS_REVIEW else name)
        q = kit.Polygon(list(poly.points), poly.normal, slot, poly.component, poly.uv_axis, poly.uv_override, poly.atlas_cells, poly.chart_id)
        q.bone = getattr(poly, "bone", None)
        out.polygons.append(q)
    out.sockets = list(m.sockets)
    out.collision = list(m.collision)
    return out


def unloaded_mesh(m: kit.Mesh) -> kit.Mesh:
    """Review copy with the archive cradle hidden, as the runtime shows it when nothing is bound."""
    out = kit.Mesh(m.name + "_unloaded", slots=list(m.slots))
    for poly in m.polygons:
        if poly.component.startswith("deck_archive_"):
            continue
        q = kit.Polygon(list(poly.points), poly.normal, poly.slot, poly.component, poly.uv_axis, poly.uv_override, poly.atlas_cells, poly.chart_id)
        q.bone = getattr(poly, "bone", None)
        out.polygons.append(q)
    out.sockets = list(m.sockets)
    return out


# --- measurement -----------------------------------------------------------------------------------
def polygon_area(poly) -> float:
    pts = poly.points
    total = (0.0, 0.0, 0.0)
    for k in range(1, len(pts) - 1):
        total = kit.v_add(total, kit.v_cross(kit.v_sub(pts[k], pts[0]), kit.v_sub(pts[k + 1], pts[0])))
    return 0.5 * kit.v_len(total)


def emissive_area_fraction(m: kit.Mesh) -> float:
    """Share of the surface carried by the cyan pseudo slot (card rule: <= 8% of the visible area)."""
    status = m.slot(STATUS_REVIEW)
    total = sum(polygon_area(p) for p in m.polygons)
    cyan = sum(polygon_area(p) for p in m.polygons if p.slot == status)
    return cyan / total if total else 0.0


def dish_diameter(m: kit.Mesh) -> float:
    """Diameter of the dish face measured on the assembled mesh (largest chord; independent of the
    rest yaw/pitch the dish is mounted at)."""
    pts = [p for poly in m.polygons if poly.component == "dish_face_01" for p in poly.points]
    return max(kit.v_len(kit.v_sub(a, b)) for a in pts for b in pts)


def dish_normal() -> tuple:
    """Unit normal of the dish face in the rest pose (yaw then pitch, as assemble() mounts it)."""
    y, p = math.radians(DISH_REST_YAW), math.radians(DISH_REST_PITCH)
    return (math.cos(p) * math.cos(y), math.cos(p) * math.sin(y), math.sin(p))


def dish_projection(m: kit.Mesh) -> dict:
    """Axis-aligned footprint of the dish face in the top and side views, in cm.

    A circle of diameter d and unit normal n projects an extent d*sqrt(1 - (n.u)^2) along any image axis
    u perpendicular to the view direction, so these are the numbers a top or side render can be measured
    against directly. The concept values beside them are the candidate's own panels (README section 8.3)."""
    d = dish_diameter(m)
    n = dish_normal()
    ext = lambda u: d * math.sqrt(max(0.0, 1.0 - sum(a * b for a, b in zip(u, n)) ** 2))  # noqa: E731
    return {
        "dish_diameter_cm": d,
        "dish_normal": list(n),
        "dish_elevation_deg": math.degrees(math.asin(n[2])),
        "top_footprint_cm": [ext((1.0, 0.0, 0.0)), ext((0.0, 1.0, 0.0))],
        "top_projected_area_cm2": math.pi * (d / 2.0) ** 2 * abs(n[2]),
        "side_footprint_cm": [ext((1.0, 0.0, 0.0)), ext((0.0, 0.0, 1.0))],
        "concept_top_footprint_cm": [33.1, 53.6],
        "concept_side_footprint_cm": [19.6, 39.8],
        "concept_top_projected_area_cm2": 1393.0,
        "concept_panel_basis": "TACTICAL SILHOUETTE disc 48.8 x 79.1 px and SIDE VIEW disc 29 x 59 px, both on a 533 px hull length",
    }


def concept_measurements(m: kit.Mesh) -> dict:
    """Proportions measured on the built rest pose, as ratios of L (concept-fidelity.md items 1-7)."""
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    hull = m.component_bounds("hull_shell")
    band = m.component_bounds("hull_deck_band")
    rails = m.component_bounds("hull_deck_rail_r")
    keel = m.component_bounds("hull_keel")
    pod = m.component_bounds("pod_r_01_pod_body")
    cradle = m.component_bounds("deck_archive_case")
    dish = m.component_bounds("dish_face_01")
    band_top, band_bottom = rails[1][2], keel[0][2]
    return {
        "L_cm": L,
        "hull_length_cm": m.component_bounds("hull_prow")[1][0] - hull[0][0],
        "hull_length_over_L": (m.component_bounds("hull_prow")[1][0] - hull[0][0]) / L,
        "hull_width_over_L": (hull[1][1] - hull[0][1]) / L,
        "hull_thickness_over_L": (band[1][2] - hull[0][2]) / L,
        "hull_pale_flank_over_L": (hull[1][2] - hull[0][2]) / L,
        "hull_pale_share_of_band": (hull[1][2] - hull[0][2]) / (band_top - band_bottom),
        "hull_band_depth_over_L": (band_top - band_bottom) / L,
        "hull_underside_over_L": hull[0][2] / L,
        "hull_deck_top_over_L": band[1][2] / L,
        "nose_face_width_over_L": 2.0 * NOSE_HALF_W / L,
        "tail_face_width_over_L": 2.0 * TAIL_HALF_W / L,
        "pod_length_over_L": (pod[1][0] - pod[0][0]) / L,
        "pod_count": sum(1 for c in m.components() if c.endswith("_pod_body")),
        "pod_bottom_over_L": min(m.component_bounds(c)[0][2] for c in m.components() if c.startswith("pod_")) / L,
        "mast_top_over_L": max(m.component_bounds(c)[1][2] for c in m.components() if c.startswith("mast_column")) / L,
        "mast_x_over_L": MAST_X / L,
        "mast_ahead_of_tail_over_L": (MAST_X + HULL_LEN / 2.0) / L,
        "mast_above_deck_over_hull_thickness": (MAST_TOP_Z - HULL_Z1) / HULL_THICK,
        "whip_top_over_L": z1 / L,
        "dish_diameter_over_L": dish_diameter(m) / L,
        "dish_centre_cm": list(dish_hub()),
        "dish_centre_over_L": DISH_Z / L,
        "dish_rest_aim_deg": {"yaw": DISH_REST_YAW, "pitch": DISH_REST_PITCH},
        "dish_projection": dish_projection(m),
        "cradle_length_over_L": (cradle[1][0] - cradle[0][0]) / L if cradle else None,
        "cradle_width_over_L": (cradle[1][1] - cradle[0][1]) / L if cradle else None,
        "cradle_height_over_L": (cradle[1][2] - cradle[0][2]) / L if cradle else None,
        "emitter_length_over_L": (m.component_bounds("emitter_barrel")[1][0] - m.component_bounds("emitter_barrel")[0][0]) / L,
        "emitter_tip_cm": [m.component_bounds("emitter_barrel")[1][0], 0.0, EMITTER_Z],
        "bounds_cm": [[x0, y0, z0], [x1, y1, z1]],
        "height_cm": z1,
        "width_cm": y1 - y0,
        "length_cm": x1 - x0,
        "lowest_vertex_cm": z0,
        "emissive_area_fraction": emissive_area_fraction(m),
        "measured_on_candidate": {
            "panel_scale": "SIDE VIEW x 951..1484 and TACTICAL SILHOUETTE x 955..1487 are both 533 px long; ground shadow centre y 503 (side panel)",
            "hull_width_over_L": 0.333, "hull_deck_top_over_L": 0.205, "hull_underside_over_L": 0.077,
            "pod_bottom_over_L": 0.024, "mast_ahead_of_tail_over_L": 0.258, "dish_diameter_over_L": 0.111,
            "whip_top_over_L": 0.710, "cradle_length_over_L": 0.377,
            "pod_stations_from_bow_over_L": [0.738, 0.238],
            "pod_count_per_side": 2,
            "tail_face_width_over_L": 0.278,
            "hull_plan_max_width_over_L": 0.373,
            "pale_flank_share_of_band": 0.42,
            "dish_centre_over_L": 0.554,
            "dish_diameter_over_L_plan_panel": 0.149,
            "note": "TACTICAL SILHOUETTE per-column trace y 620..900, threshold luminance 172; SIDE VIEW underside trace y 110..495, same threshold; the disc bounds are the bright rim of the ribbed face",
        },
    }


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "hull": {"contract": 1, "built": sum(1 for c in comps if c == "hull_shell")},
        "relay_mast": {"contract": 1, "built": sum(1 for c in comps if c == "mast_base")},
        "dish": {"contract": 1, "built": sum(1 for c in comps if c == "dish_face_01")},
        "archive_cradle": {"contract": 1, "built": sum(1 for c in comps if c == "deck_archive_case")},
        "secondary_emitter": {"contract": 1, "built": sum(1 for c in comps if c == "emitter_barrel")},
        "lift_pods": {"concept": 4, "prose": 6, "built": sum(1 for c in comps if c.endswith("_pod_body")),
                      "basis": "PIXELS: two per side in all three candidate panels; concept-fidelity.md item 3 says six (three per side). README section 8.2 and OWNER-QUESTION 5"},
        "whip_antennas": {"concept": 2, "built": sum(1 for c in comps if c.startswith("mast_whip"))},
        "cable_loop": {"concept": 1, "built": sum(1 for c in comps if c == "mast_cable")},
        "deck_rails": {"concept": 2, "built": sum(1 for c in comps if c.startswith("hull_deck_rail_"))},
        "tail_stubs": {"concept": 2, "built": sum(1 for c in comps if c.startswith("hull_tail_stub_"))},
        "bones": {"contract": 8, "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": ["Scouting_Sensor_Pod", "Logistics_Relay_Beam"], "built": [sk.name for sk in m.sockets]},
        "tracks": {"contract": list(TRACK_CONTRACT), "alias": dict(TRACK_ALIAS), "built": [c.name for c in clips]},
    }


# --- clips (Unreal rotators in degrees; seconds) -----------------------------------------------------
IDLE_BOB_CM = 1.7                 # +/- on the hull bone: 3.4 cm peak to peak, inside the <= 4 cm rule
IDLE_PITCH_DEG = 0.7
MOVE_PITCH_DEG = -3.0             # canon (Bible line 512): "glides with a slight nose-down lean"
MOVE_BOB_CM = 1.0
TURN_ROLL_DEG = 5.5               # banks into the sweep; the runtime owns the heading (no root yaw)
TURN_LIFT_CM = 1.5
STOP_PITCH_DEG = 3.2
DAMAGE_PITCH_DEG = 5.0
DAMAGE_DROP_CM = 3.0
DEATH_DROP_CM = 7.8               # settles onto the ground; the pod shoes take the weight (lowest vertex stays >= 0)
DEATH_PITCH_DEG = -1.2
DEATH_ROLL_DEG = 2.2
DEATH_MAST_ROLL_DEG = 26.0        # the mast folds over: the death read at the tactical camera
RELAY_DISH_YAW_DEG = -60.0        # dish turns toward the grid node (port bow); mast lights cyan (material state)
RELAY_DISH_PITCH_DEG = 14.0
ATTACK_EMITTER_YAW_DEG = 6.0
ATTACK_RECOIL_CM = 3.5


def build_clips(skeleton: skel.Skeleton) -> list:
    """Fourteen tracks: the frozen package inventory with relay_activation authored under the fidelity
    target's name relay_extend (TRACK_ALIAS). No clip keys root; no clip moves the frame along the ground."""
    clips = []
    bones = [b.name for b in skeleton.bones if b.name != "root"]

    def hull_key(clip, t, pitch=0.0, roll=0.0, dz=0.0, yaw=0.0):
        clip.key("hull", t, (pitch, yaw, roll), (0.0, 0.0, dz))

    def pods(clip, t, pitch=0.0, roll_out=0.0):
        clip.key("pod_bank_l", t, (pitch, 0.0, -roll_out))
        clip.key("pod_bank_r", t, (pitch, 0.0, roll_out))

    def rest(clip, t, names=None):
        for b in names or bones:
            clip.key(b, t, (0.0, 0.0, 0.0))

    # idle: continuous harmonic hover bob (card .ANIM_RIG), pods breathing, mast barely swaying
    idle = skel.AnimationClip("idle", 2.0, loop=True,
                              purpose="continuous harmonic hover bob (<= 4 cm peak to peak) with the pods breathing and the mast swaying; station keeping, no ground contact")
    for i in range(0, 9):
        t = i / 8.0
        phase = 2.0 * math.pi * t
        hull_key(idle, t * 2.0, pitch=IDLE_PITCH_DEG * math.sin(phase), roll=0.35 * math.sin(phase + 1.1), dz=IDLE_BOB_CM * math.sin(phase))
        pods(idle, t * 2.0, pitch=1.2 * math.sin(phase + 0.6), roll_out=0.8 * math.sin(phase))
        idle.key("mast", t * 2.0, (0.5 * math.sin(phase + 2.0), 0.0, 0.6 * math.sin(phase + 0.4)))
    clips.append(idle)

    # move: nose-down lean at speed, tighter bob, pods raked back
    move = skel.AnimationClip("move", 0.6, loop=True,
                              purpose="glide at 500 cm/s: slight nose-down lean (canon), reduced bob, pods raked back; play rate follows authoritative velocity (REL-ART-009), no root motion")
    for i in range(0, 9):
        t = i / 8.0
        phase = 2.0 * math.pi * t
        hull_key(move, t * 0.6, pitch=MOVE_PITCH_DEG + 0.8 * math.sin(phase), roll=0.5 * math.sin(phase + 0.8), dz=MOVE_BOB_CM * math.sin(phase))
        # the pods counter the hull lean: the cushion stays level with the ground while the hull noses down
        pods(move, t * 0.6, pitch=-MOVE_PITCH_DEG + 1.0 * math.sin(phase), roll_out=0.6 * math.sin(phase + 0.5))
        move.key("mast", t * 0.6, (2.2 + 0.6 * math.sin(phase + 1.4), 0.0, 0.0))
    clips.append(move)

    # turn: banks into the runtime's heading sweep; no yaw on any bone (SPEC-MOV-010 keeps facing runtime-owned)
    turn = skel.AnimationClip("turn", 0.6, loop=True,
                              purpose="banks into the heading sweep the runtime owns (SPEC-MOV-010): hull rolls, pods differential; no root yaw, facing snaps under Reduced Motion")
    for i, t in enumerate((0.0, 0.15, 0.3, 0.45, 0.6)):
        f = math.sin(2.0 * math.pi * t / 0.6)
        hull_key(turn, t, pitch=-1.2, roll=TURN_ROLL_DEG * f, dz=TURN_LIFT_CM * abs(f))
        pods(turn, t, pitch=1.2, roll_out=1.5 * f)
        turn.key("mast", t, (0.0, 0.0, -2.4 * f))
    clips.append(turn)

    # stop: the nose lifts as the skimmer sheds speed, then settles back to the idle attitude
    stop = skel.AnimationClip("stop", 0.4,
                              purpose="deceleration flare: the nose lifts and the frame settles back to the hover attitude (Bible motion line)")
    hull_key(stop, 0.0, pitch=MOVE_PITCH_DEG, dz=MOVE_BOB_CM)
    pods(stop, 0.0, pitch=-MOVE_PITCH_DEG)
    hull_key(stop, 0.16, pitch=STOP_PITCH_DEG, dz=2.2)
    pods(stop, 0.16, pitch=-STOP_PITCH_DEG)
    hull_key(stop, 0.4, pitch=0.0, dz=0.0)
    pods(stop, 0.4, pitch=0.0)
    clips.append(stop)

    # relay_extend (contract name relay_activation): the dish turns toward the node, the mast lights
    relay = skel.AnimationClip("relay_extend", 0.7,
                               purpose="Extend Relay activation (contract track relay_activation): the dish turns toward the connected grid node within 700 cm and the mast segments light cyan (material state); the hull holds station")
    for t, f in ((0.0, 0.0), (0.2, 0.45), (0.45, 1.0), (0.7, 1.0)):
        relay.key("dish", t, (RELAY_DISH_PITCH_DEG * f, RELAY_DISH_YAW_DEG * f, 0.0))
        relay.key("mast", t, (-1.2 * f, 0.0, 0.0))
        hull_key(relay, t, pitch=0.6 * f, dz=0.8 * f)
    clips.append(relay)

    relay_hold = skel.AnimationClip("relay_hold", 1.6, loop=True,
                                    purpose="relay sustained (400 ticks): the dish holds the node with a slow scan, the hull keeps station with a reduced bob; UI owns duration and expiry")
    for i in range(0, 5):
        t = i / 4.0
        phase = 2.0 * math.pi * t
        relay_hold.key("dish", t * 1.6, (RELAY_DISH_PITCH_DEG + 1.5 * math.sin(phase), RELAY_DISH_YAW_DEG + 3.0 * math.sin(phase), 0.0))
        hull_key(relay_hold, t * 1.6, pitch=0.4 * math.sin(phase), dz=0.9 * math.sin(phase))
        relay_hold.key("mast", t * 1.6, (-1.2, 0.0, 0.4 * math.sin(phase)))
    clips.append(relay_hold)

    relay_expiry = skel.AnimationClip("relay_expiry", 0.5,
                                      purpose="relay expiry or cooldown: the dish returns to the forward rest aim and the mast lights fade (material state); no simulation effect from the clip")
    for t, f in ((0.0, 1.0), (0.35, 0.25), (0.5, 0.0)):
        relay_expiry.key("dish", t, (RELAY_DISH_PITCH_DEG * f, RELAY_DISH_YAW_DEG * f, 0.0))
        relay_expiry.key("mast", t, (-1.2 * f, 0.0, 0.0))
        hull_key(relay_expiry, t, pitch=0.6 * f, dz=0.8 * f)
    clips.append(relay_expiry)

    # weapon triple: the emitter is clearly secondary, so the anticipation and recoil stay small
    anticipation = skel.AnimationClip("attack_anticipation", 0.25,
                                      purpose="secondary emitter anticipation: the mount aims off the nose and the hull noses up slightly; no muzzle drama (canon: clearly secondary)")
    for t, f in ((0.0, 0.0), (0.25, 1.0)):
        anticipation.key("emitter", t, (2.5 * f, ATTACK_EMITTER_YAW_DEG * f, 0.0))
        hull_key(anticipation, t, pitch=1.0 * f, dz=0.4 * f)
    clips.append(anticipation)

    execution = skel.AnimationClip("attack_execution", 0.2,
                                   purpose="emitter discharge: barrel recoils along its axis and the hull kicks back 2 deg; damage is authoritative, never driven from the clip")
    execution.key("emitter", 0.0, (2.5, ATTACK_EMITTER_YAW_DEG, 0.0))
    hull_key(execution, 0.0, pitch=1.0)
    execution.key("emitter", 0.06, (2.5, ATTACK_EMITTER_YAW_DEG, 0.0), (-ATTACK_RECOIL_CM, 0.0, 0.0))
    hull_key(execution, 0.06, pitch=2.0, dz=0.6)
    execution.key("emitter", 0.2, (2.5, ATTACK_EMITTER_YAW_DEG, 0.0))
    hull_key(execution, 0.2, pitch=1.0)
    clips.append(execution)

    recovery = skel.AnimationClip("attack_recovery", 0.3,
                                  purpose="emitter returns to the stowed forward rest and the hull returns to the hover attitude")
    for t, f in ((0.0, 1.0), (0.3, 0.0)):
        recovery.key("emitter", t, (2.5 * f, ATTACK_EMITTER_YAW_DEG * f, 0.0))
        hull_key(recovery, t, pitch=1.0 * f, dz=0.4 * f)
    clips.append(recovery)

    damage = skel.AnimationClip("damage", 0.3,
                                purpose="flinch on authoritative damage: the hull pitches up, rolls and drops on its cushion, then recovers; no displacement")
    hull_key(damage, 0.0, dz=0.0)
    pods(damage, 0.0)
    hull_key(damage, 0.1, pitch=DAMAGE_PITCH_DEG, roll=2.5, dz=-DAMAGE_DROP_CM)
    pods(damage, 0.1, pitch=-6.0, roll_out=3.0)
    hull_key(damage, 0.3, dz=0.0)
    pods(damage, 0.3)
    clips.append(damage)

    # death: the cushion fails, the hull settles onto its pods and the mast folds over
    death = skel.AnimationClip("death", 1.2,
                               purpose="cushion failure: the hull sinks onto its pods and the mast folds over; final pose held (cosmetic debris only, 200 ticks), no navigation change")
    rest(death, 0.0)
    for k in range(1, 13):
        t, f = 0.1 * k, k / 12.0
        ease = f * f * (3.0 - 2.0 * f)
        hull_key(death, t, pitch=DEATH_PITCH_DEG * ease, roll=DEATH_ROLL_DEG * ease, dz=-DEATH_DROP_CM * ease)
        # the pods counter the hull attitude as the cushion collapses, so the wreck rests on its shoes
        pods(death, t, pitch=-DEATH_PITCH_DEG * ease, roll_out=0.0)
        death.key("mast", t, (-6.0 * ease, 0.0, DEATH_MAST_ROLL_DEG * ease))
        death.key("dish", t, (-10.0 * ease, 12.0 * ease, 0.0))
        death.key("emitter", t, (-6.0 * ease, 0.0, 0.0))
    clips.append(death)

    cancel = skel.AnimationClip("cancel", 0.3,
                                purpose="order cancelled: the dish and emitter return to rest and the hull resumes the idle hover attitude; never replays a one-shot")
    for b, start in (("dish", (RELAY_DISH_PITCH_DEG * 0.5, RELAY_DISH_YAW_DEG * 0.5, 0.0)), ("emitter", (1.5, 3.0, 0.0)), ("mast", (-1.0, 0.0, 0.0))):
        cancel.key(b, 0.0, start)
        cancel.key(b, 0.3, (0.0, 0.0, 0.0))
    hull_key(cancel, 0.0, pitch=-1.5, dz=0.6)
    hull_key(cancel, 0.3, pitch=0.0, dz=0.0)
    pods(cancel, 0.0, pitch=-2.0)
    pods(cancel, 0.3)
    clips.append(cancel)

    restore = skel.AnimationClip("restore", 0.0,
                                 purpose="single-frame rest pose for reconstructing presentation from saved state; never replays a one-shot (event_rule)")
    rest(restore, 0.0)
    clips.append(restore)
    # Snap every authored duration onto a whole 30 fps frame. VERIFIED 2026-09-07 against UE 5.8.2
    # (evidence: BuildArtifacts/.../skeletal-clip-duration-probe/): the Interchange skeletal import
    # silently creates NO AnimSequence for a clip whose duration is a half frame, and still reports
    # success. retime_clip scales the key times, so the pose at any normalized time is unchanged.
    for _clip in clips:
        if not skel.is_frame_aligned(_clip.duration_s):
            skel.retime_clip(_clip, skel.frame_aligned_duration(_clip.duration_s))
    return clips


# --- outputs ---------------------------------------------------------------------------------------
def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def build_outputs(export_dir: str, review_dir: str) -> tuple:
    """Write every export (both LODs) into export_dir and every review OBJ into review_dir; return
    (outputs, review, meshes). Output paths are recorded relative to the package so --check can rebuild
    into a temporary directory and compare hashes without touching the committed files."""
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)
    outputs, review, meshes = [], [], {}
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, counts, sockets_on_bones = assemble(lod)
        meshes[("assembly", lod)] = m
        ex = export_mesh(m)
        stem = f"{ASSET}_LOD{lod}"
        base = os.path.join(export_dir, stem)
        skinned = skel.write_skinned_glb(ex, s, base + ".glb", animations=clips, include_collision=False, sockets_on_bones=sockets_on_bones,
                                         extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod,
                                                 "note": "skinned skeletal export: 8 bones, 14 clips, sockets on their bones; Nanite OFF"})
        outputs.append({"path": f"export/{stem}.glb", "sha256": skinned, "lod": lod, "mesh": ASSET, "kind": "skinned + clips",
                        "triangles": ex.triangle_count(), "by_slot": ex.triangle_count_by("slot"), "by_bone": counts,
                        "bounds_cm": ex.bounds(), "section_slot_names": ex.slot_names_in_primitive_order(),
                        "clips": [c.name for c in clips],
                        "sockets": [{"name": sk.name, "position_cm": sk.position, "yaw_deg": sk.yaw_deg, "bone": sockets_on_bones.get(sk.name), "purpose": sk.purpose} for sk in ex.sockets]})
        obj = ex.write_obj(base + ".obj", header_lines=[f"Production ID {PRODUCTION_ID}", f"Revision {REVISION}", f"LOD{lod}", "Rest pose, two material slots"])
        outputs.append({"path": f"export/{stem}.obj", "sha256": obj, "lod": lod, "mesh": ASSET, "kind": "rest-pose OBJ"})
        static = ex.write_glb(base + "_static.glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod,
                                                            "note": "rest-pose static export; the skinned export carries the rig"}, include_collision=False)
        outputs.append({"path": f"export/{stem}_static.glb", "sha256": static, "lod": lod, "mesh": ASSET, "kind": "static rest pose", "triangles": ex.triangle_count()})
        # the archive cradle as its own asset so the runtime can show or hide the load
        cradle = export_mesh(build_cradle(lod))
        meshes[("cradle", lod)] = cradle
        cstem = f"{CRADLE_ASSET}_LOD{lod}"
        cbase = os.path.join(export_dir, cstem)
        cglb = cradle.write_glb(cbase + ".glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod,
                                                        "note": "archive cradle as a separate part; pivot at the deck contact centre, attach at the cradle bone"}, include_collision=False)
        cobj = cradle.write_obj(cbase + ".obj", header_lines=[f"Archive cradle part LOD{lod}; pivot at the deck contact centre"])
        outputs.append({"path": f"export/{cstem}.glb", "sha256": cglb, "lod": lod, "mesh": CRADLE_ASSET, "kind": "separate part (loaded state)",
                        "triangles": cradle.triangle_count(), "bounds_cm": cradle.bounds()})
        outputs.append({"path": f"export/{cstem}.obj", "sha256": cobj, "lod": lod, "mesh": CRADLE_ASSET, "kind": "separate part OBJ"})
        # review assemblies (three slots so the renders show the cyan strips)
        rpath = os.path.join(review_dir, f"{ASSET}_rest_LOD{lod}.obj")
        review.append({"path": rpath, "sha256": m.write_obj(rpath, header_lines=[f"Rest pose LOD{lod}, archive cradle loaded; review only"]),
                       "lod": lod, "state": "loaded", "triangles": m.triangle_count(), "bounds_cm": m.bounds()})
        un = unloaded_mesh(m)
        upath = os.path.join(review_dir, f"{ASSET}_rest_unloaded_LOD{lod}.obj")
        review.append({"path": upath, "sha256": un.write_obj(upath, header_lines=[f"Rest pose LOD{lod} with the archive cradle hidden (deck_archive_* removed); review only"]),
                       "lod": lod, "state": "unloaded", "triangles": un.triangle_count(), "bounds_cm": un.bounds()})
    cpath = os.path.join(review_dir, f"{CRADLE_ASSET}_LOD0.obj")
    review.append({"path": cpath, "sha256": build_cradle(0).write_obj(cpath, header_lines=["Archive cradle part in its own frame (pivot at the deck contact centre); review only"]),
                   "lod": 0, "state": "cradle part", "triangles": build_cradle(0).triangle_count()})
    return outputs, review, meshes


# --- review scenes ------------------------------------------------------------------------------
SCENE_BASE = {
    "author": AUTHOR, "srgb": True,
    "materials": {CERAMIC: [0.72, 0.7, 0.66], FRAME: [0.05, 0.05, 0.055], STATUS_REVIEW: [0.16, 0.86, 0.96],
                  "MI_EBS_MER_CeramicCivic": [0.72, 0.7, 0.66], "MI_EBS_MER_CompactFrame": [0.045, 0.045, 0.05],
                  "_default": [0.5, 0.5, 0.5]},
    "emissive": [STATUS_REVIEW], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
    "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.3, "key": 0.8, "color": [1.0, 0.82, 0.62],
              "fill_color": [0.48, 0.6, 0.88], "fill": 0.22},
    "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
               "footprint_cm": [200, 200], "footprint_color": [0.16, 0.86, 0.96]},
    "reference_figure": {"height_cm": REFERENCE_FIGURE_CM, "position": [-250, 110, 0], "color": [0.92, 0.55, 0.2]},
}
# Tactical views copied verbatim from EBS-MER-UNT-001's rest_tactical scene (the game's orthographic
# framing: AEchoesRTSCameraPawn 3800 uu arm, 55 deg FOV, -48 default / -60 gameplay tilt).
TACTICAL_VIEWS = [
    {"name": "tactical_default", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
    {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
    {"name": "tactical_mono", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0], "grayscale": True},
    {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
    {"name": "tactical_front_quarter", "type": "persp", "pitch_deg": -48, "yaw_deg": 135, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
    # review aid only: the same camera basis pulled in to 800 cm so state differences are legible on a sheet
    {"name": "tactical_close", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 800, "fov_deg": 55, "target": [0, 0, 60]},
]
ORTHO_VIEWS = [
    {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15, "target": [0, 0, 110]},
    {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 110]},
    {"name": "rear", "type": "ortho", "from": "-X", "edges": True, "margin": 1.15, "target": [0, 0, 110]},
    {"name": "left", "type": "ortho", "from": "-Y", "edges": True, "margin": 1.12, "target": [0, 0, 110]},
    {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [0, 0, 0]},
    # same plan, laid out the way the candidate's TACTICAL SILHOUETTE panel is drawn (hull horizontal,
    # bow to the image right), so the comparison sheet can put the two side by side at one scale
    {"name": "top_plan", "type": "ortho", "from": "+Z", "image_up": "-Y", "edges": True, "margin": 1.06, "target": [0, 0, 0]},
]


# Concept-comparison views. The candidate's SIDE VIEW and TACTICAL SILHOUETTE panels are landscape and
# drawn with the hull horizontal and the bow to the image right, so these two views reproduce that layout
# at a tight margin in a 1280 x 900 frame (aspect 1.422). make_sheets.py cuts the concept panels to the
# same aspect and the same object-to-frame share, so panel and render land in a sheet cell at one scale
# (concept-v1's sheets paired a landscape panel with a portrait render and could not be read side by side).
COMPARE_FRAME = (1280, 900)
COMPARE_VIEWS = [
    {"name": "side_plate", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.04, "target": [0, 0, 128]},
    {"name": "top_plate", "type": "ortho", "from": "+Z", "image_up": "-Y", "edges": True, "margin": 1.04, "target": [0, 0, 0]},
]


def scene(meshes: list, views: list, reference: bool = True, footprint=(200, 200)) -> dict:
    out = {k: (dict(v) if isinstance(v, dict) else (list(v) if isinstance(v, list) else v)) for k, v in SCENE_BASE.items()}
    out["ground"]["footprint_cm"] = list(footprint)
    if not reference:
        out.pop("reference_figure")
    out["meshes"] = meshes
    out["views"] = views
    return out


def compare_scene(meshes: list) -> dict:
    """Concept-comparison scene: the two panel-matched views in the 1280 x 900 frame, no ground clutter
    and no reference figure, so the sheet cell carries the silhouette and nothing else."""
    out = scene(meshes, COMPARE_VIEWS, reference=False)
    out["width"], out["height"] = COMPARE_FRAME
    out.pop("ground", None)
    out["background"] = [0.84, 0.82, 0.78]
    return out


def write_scenes(scene_dir: str) -> list:
    """Review scenes: authored-scale orthographic turnaround with a 180 cm reference figure, the game's
    orthographic tactical framing, both cradle states, LOD1 and a context scene beside the Power Link."""
    os.makedirs(scene_dir, exist_ok=True)
    loaded = [{"obj": f"../review/{ASSET}_rest_LOD0.obj"}]
    unloaded = [{"obj": f"../review/{ASSET}_rest_unloaded_LOD0.obj"}]
    scenes = {
        "rest": scene(loaded, ORTHO_VIEWS),
        "rest_unloaded": scene(unloaded, ORTHO_VIEWS),
        "rest_tactical": scene(loaded, TACTICAL_VIEWS),
        "rest_unloaded_tactical": scene(unloaded, TACTICAL_VIEWS),
        "lod1": scene([{"obj": f"../review/{ASSET}_rest_LOD1.obj"}], ORTHO_VIEWS),
        "lod1_tactical": scene([{"obj": f"../review/{ASSET}_rest_LOD1.obj"}], [TACTICAL_VIEWS[1], TACTICAL_VIEWS[3]]),
        "cradle_part": scene([{"obj": f"../review/{CRADLE_ASSET}_LOD0.obj"}],
                             [ORTHO_VIEWS[0], ORTHO_VIEWS[1], ORTHO_VIEWS[4], TACTICAL_VIEWS[3]], reference=True, footprint=(100, 100)),
        "compare": compare_scene(loaded),
        "compare_unloaded": compare_scene(unloaded),
        "compare_lod1": compare_scene([{"obj": f"../review/{ASSET}_rest_LOD1.obj"}]),
        "context": scene([
            {"obj": "../../EBS-MER-BLD-002/review/SM_EBS_MER_BLD_002_assembly_connected_LOD0.obj", "translate": [0, 700, 0], "yaw_deg": 180},
            {"obj": f"../review/{ASSET}_rest_LOD0.obj", "translate": [0, 0, 0], "yaw_deg": -30},
            {"obj": f"../review/{ASSET}_rest_unloaded_LOD0.obj", "translate": [-520, -260, 0], "yaw_deg": 25},
            {"obj": "../../EBS-MER-UNT-001/review/SK_EBS_MER_UNT_001_rest_LOD0.obj", "translate": [330, -300, 0], "yaw_deg": 150, "scale": 1.5},
        ], [TACTICAL_VIEWS[1], TACTICAL_VIEWS[0], TACTICAL_VIEWS[3], TACTICAL_VIEWS[2]], footprint=(400, 400)),
    }
    written = []
    for name, data in sorted(scenes.items()):
        path = os.path.join(scene_dir, f"{name}.json")
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(json.dumps(data, indent=1) + "\n")
        written.append(path)
    return written


def build_manifest(outputs: list, review: list, meshes: dict) -> dict:
    m0, m1 = meshes[("assembly", 0)], meshes[("assembly", 1)]
    c0, c1 = meshes[("cradle", 0)], meshes[("cradle", 1)]
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    measurements = concept_measurements(m0)
    return {
        "author": AUTHOR, "creator": AUTHOR, "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID,
        "asset_name": ASSET, "parts": [ASSET, CRADLE_ASSET],
        "revision": REVISION, "kit_revision": kit.KIT_REVISION, "skel_revision": skel.SKEL_REVISION,
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Concept-matched rest geometry, the 8-bone hover rig and keyframed clip data, exported skinned on the verified skeletal kit; no textures, no Unreal import, no gate or owner acceptance.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters at unit scale 1.0", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "ground point under the hull centre (root at 0,0,0); the runtime hover offset is additive on the authored 0.09 L gap"},
        "scale_basis": {"concept": "concept-fidelity.md: L = 360 cm hull length (PROVISIONAL), every proportion a ratio of L measured on relay-skiff-candidate.png",
                        "canon": "SPEC-UNIT-004 / Bible line 512: light, fast, low-slung skimmer with a tall relay mast, a dish and a small forward weapon",
                        "length_cm": measurements["length_cm"], "width_cm": measurements["width_cm"], "height_cm": measurements["height_cm"],
                        "hover_gap_cm": measurements["lowest_vertex_cm"],
                        "simulation_footprint_half_extent_cm": SIM_FOOTPRINT_HALF_EXTENT_CM,
                        "status": "CONCEPT-MEASURED BLOCKOUT (concept-v1)"},
        "concept_measurements": measurements,
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": b.head, "purpose": b.purpose} for b in skeleton.bones],
                "policy": "8 bones (contract bone_count_source 8), identity rest orientation, no decorative bones; the whip antennas, cable loop, rails and pods' cyan strips are rigid with their bone; the relay light and the emitter muzzle are material/VFX states, not bones",
                "hover": "root is never keyed; the hull bone carries the harmonic bob so the runtime hover offset stays additive"},
        "sockets": [{"name": n, "bone": b} for n, b in sorted(SOCKETS_ON_BONES.items())],
        "socket_semantics": {
            "Logistics_Relay_Beam": "FIXED ORIGIN on the mast head, identity rotation. It is a position, not an aim transform: "
                                    "the card's cyan cone toward the nearest grid node within 700 cm must be aimed at runtime, "
                                    "not bound to this socket's rotation.",
            "Scouting_Sensor_Pod": "rides the dish bone, so it swings whenever the dish moves - including during relay_extend, "
                                   "which is a logistics action. Any scouting VFX bound to it inherits that motion.",
            "dish_aim": "the relay clips key a FIXED placeholder aim (-60 deg yaw, +14 deg pitch, unit-relative); the node-facing "
                        "rotation is a runtime look-at layered on the dish bone. No clip knows where a grid node is.",
        },
        "clips": [{"name": c.name, "duration_s": c.duration_s, "loop": c.loop, "bones": sorted(c.tracks), "purpose": c.purpose} for c in clips],
        "track_contract": {"required": list(TRACK_CONTRACT), "alias": dict(TRACK_ALIAS),
                           "note": "relay_activation (frozen contract) is authored as relay_extend (concept-fidelity.md and the production handoff); README section 8 / OWNER-QUESTION 1"},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "lod0_cap": 5000, "lod1_cap": 2100,
                    "lod0_within_cap": m0.triangle_count() <= 5000, "lod1_within_cap": m1.triangle_count() <= 2100,
                    "cradle_part_triangles": [c0.triangle_count(), c1.triangle_count()],
                    "emissive_area_fraction_lod0": measurements["emissive_area_fraction"], "emissive_cap": 0.08,
                    "nanite": "OFF", "texture_stack_resolution": 2048},
        "material_slots": [CERAMIC, FRAME],
        "material_slot_policy": "2 slots (contract material_slots_provisional_max 2); cyan link conduits, brass trim, plate seams, the cradle status strip and the team mask are texture channels of the ceramic slot; the review OBJs keep a third pseudo slot (MI_EBS_MER_StatusCyan) so the renders show the cyan",
        "component_inventory": contract_inventory(m0, skeleton, clips),
        "states": {"loaded": "deck_archive_* present (M01 archive carrier)", "unloaded": "deck_archive_* hidden; nothing else changes",
                   "rule": "the archive rack never changes unless an authoritative event binds it (canon line 512); visibility only, never animation"},
        "collision": {"authored_primitives": 0,
                      "rule": "no collision primitive is authored: hovering uses ground raycasts to float the view actor and never alters simulation passability (card .MAT_RULE); the authoritative footprint stays the simulation's 25 cm square"},
        "outputs": outputs, "review": review,
        "tools": {"builder_sha256": builder_sha, "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "selected_candidate": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/relay-skiff-review/relay-skiff-candidate.png",
                                   "status": "SELECTED CANDIDATE (production direction): main three-quarter, SIDE VIEW, TACTICAL SILHOUETTE"},
            "style_reference": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/lancer-review/lancer-candidate.png",
                                "status": "FACTION/STYLE REFERENCE the candidate was made from"},
            "superseded_concept": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/site/assets/concepts/meridian-units.png", "region": [0.5, 0.5, 1.0, 1.0],
                                   "sha256_prefix": "427e60cd27bd78e9", "status": "EBS-CON-MER-UNT-004 REPLACE, retained history only"},
            "fidelity_target": {"path": os.path.join(HERE, "concept-fidelity.md")},
            "canon_row": {"path": "Docs/Archive/DevelopmentBible.md", "line": 512},
            "gameplay_record": {"path": "Content/Data/Source/units.json", "id": "mc_relay_skiff (SPEC-UNIT-004: 500 cm/s, 75 HP, 1,500 cm sight, 6 damage at 400 cm, +4 Logistics within 700 cm)"},
            "contract": {"path": "Docs/VisualAssetPipeline/motion/gap-decisions.json", "package_id": PACKAGE_ID},
            "requirement_card": {"path": "Docs/Requirements.md", "card": "REL-FAC-025.MC.SKIFF.ASSET"},
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }


def main_cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", default=None,
                        help="evidence directory for this package; required to build, optional with --check "
                             "(without it --check still compares every hash, it just cannot look for the review OBJs on disk)")
    parser.add_argument("--check", action="store_true",
                        help="read-only: rebuild into a temporary directory and compare every export and review hash with build-manifest.json")
    args = parser.parse_args()
    if not args.check and not args.evidence_dir:
        parser.error("--evidence-dir is required when building")
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        if not os.path.exists(manifest_path):
            print(json.dumps({"check": "no-manifest"}))
            return 2
        with open(manifest_path, "r", encoding="utf-8") as handle:
            previous = json.load(handle)
        with tempfile.TemporaryDirectory(prefix="ebs-skiff-check-") as tmp:
            outputs, review, _meshes = build_outputs(os.path.join(tmp, "export"), os.path.join(tmp, "review"))
        prev_out = {o["path"]: o["sha256"] for o in previous.get("outputs", [])}
        prev_review = {os.path.basename(r["path"]): r["sha256"] for r in previous.get("review", [])}
        drift = [o["path"] for o in outputs if prev_out.get(o["path"]) != o["sha256"]]
        drift += [os.path.basename(r["path"]) for r in review if prev_review.get(os.path.basename(r["path"])) != r["sha256"]]
        missing = [p for p in prev_out if not os.path.exists(os.path.join(HERE, p))]
        if args.evidence_dir:
            missing += [p for p in prev_review if not os.path.exists(os.path.join(args.evidence_dir, "review", p))]
        ok = not drift and not missing and previous.get("revision") == REVISION
        print(json.dumps({"check": "ok" if ok else "drift", "revision": REVISION, "manifest_revision": previous.get("revision"),
                          "drift": drift, "missing": missing, "compared": {"outputs": len(outputs), "review": len(review)},
                          "review_files_on_disk_checked": bool(args.evidence_dir)}))
        return 0 if ok else 3
    outputs, review, meshes = build_outputs(os.path.join(HERE, "export"), os.path.join(args.evidence_dir, "review"))
    scenes = write_scenes(os.path.join(args.evidence_dir, "scenes"))
    manifest = build_manifest(outputs, review, meshes)
    manifest["review_scenes"] = [os.path.basename(p) for p in scenes]
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(manifest, indent=1, sort_keys=True) + "\n")
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"],
                      "inventory": {k: v.get("built") for k, v in manifest["component_inventory"].items() if isinstance(v, dict)},
                      "measurements": {k: (round(v, 4) if isinstance(v, float) else v) for k, v in manifest["concept_measurements"].items() if not isinstance(v, (dict, list))},
                      "outputs": len(outputs), "review": len(review)}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
