#!/usr/bin/env python3
"""EBS-MER-UNT-002 Lancer: deterministic source geometry, 18-bone rig and clips (concept-v2 blockout).

Author: Angelis Pseftis.

Design authority (owner ruling 2026-09-06): the selected concept images define what the asset
looks like; gameplay rules bound the concept but never replace it. Target record:
  concept-fidelity.md (this folder). Sources looked at, not described:
  * lancer-candidate.png (SELECTED CANDIDATE, the production direction): main braced view,
    braced side view, tactical silhouette. A narrow two-legged line-fire frame in a wide
    fore-and-aft brace; ONE long rail-lance carried level at hip height on a hip mount at the
    body's front; a low ceramic sensor cowl with a horizontal cyan visor set between two boxy
    shoulder pods (antenna rods above the pods); an open charcoal frame between the pods and the
    hips (exposed flanks, no skirt); a thin rear-leg recoil strut with a slide section running
    from the lower rear of the body down and back to the trailing ankle.
  * lancer-derived-turnaround.png: front / left side / rear / top plus the hip-mount and
    leg-attachment inset. Its FRONT and REAR panels are quarter views (the lance swings off the
    centreline in both), so only the LEFT SIDE panel is used for measurement and the quarter
    panels for the across-Y read (README section 8).
  * EBS-CON-MER-UNT-002 (meridian-units.png top-right) is SUPERSEDED (decision REPLACE) and is
    NOT a modelling target; it is retained history only.

Measurements were taken on the LEFT SIDE panel pixels with the panel scaled so H = 200 cm at the
pod top (0.545 cm per source pixel; ground at the sole line). Three lines of concept-fidelity.md
are corrected against those pixels and marked in that file (README section 8):
  * fore-and-aft brace: measured 0.63 H between the foot centres and 0.87 H heel-to-toe, not 1.0-1.2 H;
  * shoulder pods: measured ~0.44 H total across the pods (front and rear quarter panels), so the
    pod centres are 0.30 H apart, not 0.55 H;
  * leg split: measured thigh 0.31 H / shin 0.24 H (the target says 0.28 / 0.26); built 0.30 / 0.25 H.
Also measured and not in the target: two antenna rods above the pods, tips at 1.065 H.

concept-v2 (2026-09-07) corrects five confirmed defects against the same pixels; README section 8.7 has the
measurements, and each carries a regression test:
  * the upper assembly (deck, both pods, cowl) was carried 0.18 H too far FORWARD. Both side panels put the
    0.80-1.00 H silhouette band's fore-aft centroid at -30.4 / -30.2 cm; concept-v1 built +7.7. The spine
    origin moves 38 cm aft and the body's core column, flank members and shoulder yoke move with it.
  * the recoil strut is BOLTED to the trailing ankle, but concept-v1 keyed it in place while the leg swung
    away (132.9 cm of daylight in the move clip). It is now solved from the trailing leg every frame, and
    the travel states stow the piston instead, because a forward-swinging leg cannot be tracked without
    driving the strut through the body.
  * the lance's collars and muzzle head were 0.131-0.143 H deep against the concept's thickest section of
    0.110 H and a 0.095 H clear shaft; rebuilt to those numbers.
  * the collars crossed the cyan band and broke it into four dashes; they are now blocks above and below
    it, so the channel reads as one stroke, as the concept's collars do.
  * ceramic faced aft (deck plate, pod rear faces, full-box leg plates) where the concept's rear is
    charcoal frame with a pale cap only on the pod tops.

Package contract: Docs/VisualAssetPipeline/reference-packages.json, package EBS-PKG-MC-LANCER
  (canon_production_brief, gameplay_contract mc_lancer, reference_items_required);
  review decision EBS-CON-MER-UNT-002 = REPLACE (Docs/VisualAssetPipeline/review-selections.json).
Canon row: Docs/Archive/DevelopmentBible.md line 510 (SPEC-UNIT-002).
Requirement card: REL-ART-005.MC.LANCER: LOD0 <= 8,000 tris, LOD1 ceiling printed "3,3500" (a typo;
  the owner ruling of 2026-09-07 CONFIRMS 3,500 citing REL-ART-028, which states the roster rule
  verbatim), 2048^2 PBR stack, team colour by a VERTEX mask that this package cannot author
  (README section 8.3, OWNER-QUESTION 3), emissive <= 15% of the surface, 18-bone kinematic rig,
  sub-object separation for Turret_Y and Barrel_X, card sockets Muzzle_Flash_01 /
  Target_Anchor_Center / Left_Tread_Vector.

concept-v4 (2026-09-07) implements the owner ruling on the socket set (README section 8.8):
  Rear_Recoil_Strut_Anchor is added at the recoil strut's UPPER anchor - the end that transmits recoil
  into the frame - and Left_Tread_Vector becomes a TEMPORARY COMPATIBILITY ALIAS at exactly that
  transform, to be dropped once the adapter migration is tested. Left-foot ground contact is a
  different function and no longer rides on the alias: Foot_Contact_L and Foot_Contact_R carry it at
  the two sole contact centres. The frame stays two-legged; no geometry changed.

Rig (18 bones, identity rest orientation, heads in the braced rest stance):
  root, body, spine, cowl, r_pod, l_pod, lance_yaw (card Turret_Y), lance_barrel (card Barrel_X),
  r_thigh, r_shin, r_foot, r_toe, l_thigh, l_shin, l_foot, l_toe, strut_upper, strut_slide.
  The right (+Y) leg leads, planted ahead; the left (-Y) leg trails and carries the recoil strut.
  The braced rest stance is the concept's mirror-image brace: the lead knee bows forward and the
  trailing knee bows back (measured), so the trailing leg reads as a kickstand under recoil.

Conventions: cm, +X forward (lance axis), +Y right, +Z up, root at the ground-contact centre
between the feet, no root motion; Soldiers draw at PresentationScale 1.60 at runtime
(EchoesEntityView.cpp:1809-1812; not part of the asset).

CLI: --evidence-dir <dir> writes the exports, the review OBJs and the base review scenes;
     --check rebuilds into a temporary directory and compares hashes without rewriting anything.
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
PACKAGE_ID = "EBS-PKG-MC-LANCER"
PRODUCTION_ID = "EBS-MER-UNT-002"
ASSET = "SK_EBS_MER_UNT_002"
YOKE_ASSET = "SM_EBS_MER_UNT_002_LanceYoke"
BARREL_ASSET = "SM_EBS_MER_UNT_002_LanceBarrel"
REVISION = "ebs-mer-unt-002-concept-v4"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_002/"
LOD0_CAP, LOD1_CAP = 8000, 3500           # LOD1 = 3,500 CONFIRMED by the owner ruling 2026-09-07 citing
                                          # REL-ART-028; the card's printed "3,3500" is a typo (README 8.5)
EMISSIVE_CAP = 0.15
PRESENTATION_SCALE = 1.60                 # EntityType::Soldier readability scale (runtime, not authored)

CERAMIC = "MI_EBS_MER_UnitCeramic"        # pale ceramic plate; brass trim and the team mask are texture channels
FRAME = "MI_EBS_MER_UnitFrame"            # charcoal underframe, hubs, rails, feet
STATUS_REVIEW = "MI_EBS_MER_StatusCyan"   # review-only pseudo slot: folded into CERAMIC for the export

# --- concept measurements (concept-fidelity.md as corrected on the pixels; H = 200 cm at the pod top) ---
H = 200.0

# stance: LEFT SIDE panel gives foot centres 0.63 H apart along X, 0.87 H heel-to-toe, ankles 0.135 H up
HIP = (-6.0, 35.0, 112.0)                 # hip joint, right side; the body is carried low between the hips
ANKLE_Z = 27.0
LEAD_ANKLE_X = 58.0                       # right (leading) leg planted ahead
TRAIL_ANKLE_X = -70.0                     # left (trailing) leg planted back, mirrored about the hip line
THIGH_LEN = 60.0                          # 0.30 H
SHIN_LEN = 50.0                           # 0.25 H
FOOT_BACK, FOOT_FWD = 18.0, 26.0          # block foot 44 cm long (0.22 H), ankle forward of centre
TOE_PIVOT = (8.0, 0.0, -19.0)             # toe hinge, local to the ankle (world z 8)
KNEE_R = 8.0                              # prominent knee disc, 0.08 H diameter
LEG_W = 16.0
BODY_PITCH = -8.0                         # concept item 1: the body is pitched ~8 deg forward

# upper body: authored in the spine frame (unpitched), then pitched forward about the spine head.
# concept-v2: the whole upper assembly is carried BACK over the hips. Measured on both side panels
# (segmented, scaled to H = 200 cm, anchored on the trailing heel and the leading toe), the 0.80-1.00 H
# silhouette band has its fore-aft centroid at -30.4 cm (turnaround LEFT SIDE) / -30.2 cm (candidate
# BRACED SIDE) with its front edge at +16.6 / +13.4 cm. concept-v1 put that centroid at +7.7 cm with the
# cowl nosing out over the lance yoke, so the spine origin moves 38 cm aft (README section 8.7).
UPPER_PIVOT = (-42.0, 0.0, 148.0)
DECK = (54.0, 62.0, 6.0)                  # ceramic deck plate carrying the pods and the cowl
DECK_L = (-4.0, 0.0, 21.0)                # inset forward of the charcoal deck frame: the frame is the rearmost surface
DECK_FRAME = (56.0, 62.0, 14.0)           # charcoal deck frame; closes the rear of the deck so no ceramic faces aft
DECK_FRAME_L = (-7.0, 0.0, 15.0)
POD = (44.0, 28.0, 26.0)                  # 0.22 H long, 0.13 H tall (concept item 4)
POD_L = (6.0, 30.0, 39.0)                # pod centres 0.30 H apart (corrected on the pixels); tops at H
ANTENNA_L = (-22.0, 22.0, 65.0)           # antenna rods above the pods; tips measured at 1.065 H
COWL = (34.0, 30.0, 26.0)
COWL_L = (30.0, 0.0, 40.0)                # low wedge head between the pods; cyan visor at ~0.89 H, top just under the pod tops
CORE_W = 34.0                             # narrow charcoal core between the exposed flanks

# rail-lance (concept item 2)
LANCE_Z = 132.0                           # axis height (0.66 H), level, on the centreline
LANCE_TIP_X = 194.0                       # muzzle face
LANCE_LEN = 210.0                         # 1.05 H total
YAW_PIVOT = (30.0, 0.0, LANCE_Z)          # hip-mount yaw axis at the body's front (card Turret_Y)
TRUNNION = (44.0, 0.0, LANCE_Z)           # barrel pitch / recoil pivot (card Barrel_X)
# concept-v2 lance section, measured column by column on the candidate BRACED SIDE panel at 0.514 cm/px:
# clear shaft 19.0 cm = 0.095 H, thickest collar 22.1 cm = 0.110 H, muzzle end 17.5-19.0 cm. concept-v1
# built a 21 cm shaft with 26 cm collars and a 28.7 cm muzzle strip (0.105 / 0.131 / 0.143 H).
RAIL_DZ, RAIL_HY, RAIL_HZ = 6.5, 6.0, 3.0    # two parallel rails above and below the channel: envelope 19 cm (0.095 H)
CHANNEL_HY, CHANNEL_HZ = 10.0, 3.5           # continuous cyan channel between them (reads from the side and the top)
CLAMP_X = (72.0, 114.0, 156.0)               # three collar clamps
CLAMP_HZ = 11.0                              # collar half-depth: 22 cm across (0.110 H), the concept's thickest section
MUZZLE_X0 = 176.0                            # blocky muzzle head, 18 cm long (concept-v1 built 22 cm)
MUZZLE_CAP = 4.5                             # solid muzzle face plug; the cyan channel runs up to it
RECOIL_CM = 26.0                             # recoil stroke along the lance's own axis

# rear-leg recoil strut (concept item 6): thin charcoal member with a slide section at its middle
STRUT_TOP = (-46.0, -20.0, 112.0)         # on the rear tail beam, clear behind the trailing shin
STRUT_END = (-80.0, -34.0, 30.0)          # lands over the trailing foot heel, beside the ankle
STRUT_SPLIT = 0.52
STRUT_R = 3.4


# --- vector / angle helpers -----------------------------------------------------------------------
def _len(d):
    return math.sqrt(d[0] ** 2 + d[1] ** 2 + d[2] ** 2)


def _dir_angles(d):
    """(pitch, yaw) carrying local +X onto delta d with ebs_meshkit.rot_yp. A limb running backward
    (d[0] < 0) takes the over-vertical pitch with yaw 0 so its local +Y stays world +Y."""
    n = _len(d)
    pitch = math.degrees(math.asin(d[2] / n))
    if d[0] < 0:
        return 180.0 - pitch - 360.0, math.degrees(math.atan2(-d[1], -d[0]))
    return pitch, math.degrees(math.atan2(d[1], d[0]))


def solve_leg(ankle_x: float, ankle_z: float = ANKLE_Z, knee_sign: float | None = None):
    """Two-bone solve in the XZ plane: hip -> knee -> ankle. ``knee_sign`` +1 bows the knee ahead of
    the hip-ankle line (the walking / digitigrade convention); None uses the concept's mirror-image
    brace (the lead knee ahead, the trailing knee behind), which is what the pixels show at rest."""
    dx, dz = ankle_x - HIP[0], ankle_z - HIP[2]
    reach = math.sqrt(dx * dx + dz * dz)
    if reach > THIGH_LEN + SHIN_LEN:
        raise ValueError(f"leg cannot reach ({ankle_x}, {ankle_z}): {reach:.2f} > {THIGH_LEN + SHIN_LEN}")
    base = math.atan2(dz, dx)
    alpha = math.acos(max(-1.0, min(1.0, (THIGH_LEN ** 2 + reach ** 2 - SHIN_LEN ** 2) / (2.0 * THIGH_LEN * reach))))
    sign = (1.0 if dx >= 0.0 else -1.0) if knee_sign is None else knee_sign
    theta = base + sign * alpha
    thigh = (THIGH_LEN * math.cos(theta), 0.0, THIGH_LEN * math.sin(theta))
    shin = (dx - thigh[0], 0.0, dz - thigh[2])
    return thigh, shin


def _limb_theta(d):
    """Continuous direction angle in the XZ plane (degrees), safe across the backward branch."""
    return math.degrees(math.atan2(d[2], d[0]))


LEAD_THIGH_D, LEAD_SHIN_D = solve_leg(LEAD_ANKLE_X)
TRAIL_THIGH_D, TRAIL_SHIN_D = solve_leg(TRAIL_ANKLE_X)
REST_LEG = {"r": (LEAD_ANKLE_X, LEAD_THIGH_D, LEAD_SHIN_D), "l": (TRAIL_ANKLE_X, TRAIL_THIGH_D, TRAIL_SHIN_D)}
REST_THETA = {side: (_limb_theta(v[1]), _limb_theta(v[2])) for side, v in REST_LEG.items()}
STRUT_LEN = _len(tuple(STRUT_END[i] - STRUT_TOP[i] for i in range(3)))
STRUT_DIR = tuple((STRUT_END[i] - STRUT_TOP[i]) / STRUT_LEN for i in range(3))
STRUT_JOINT = tuple(STRUT_TOP[i] + STRUT_DIR[i] * STRUT_LEN * STRUT_SPLIT for i in range(3))
STRUT_PITCH, STRUT_YAW = _dir_angles(tuple(STRUT_JOINT[i] - STRUT_TOP[i] for i in range(3)))


def rest_positions() -> dict:
    """Joint heads in the braced rest stance. The right (+Y) leg leads, the left (-Y) leg trails."""
    out = {}
    for side, sign in (("r", 1.0), ("l", -1.0)):
        _ax, thigh_d, shin_d = REST_LEG[side]
        hip = (HIP[0], sign * HIP[1], HIP[2])
        knee = (hip[0] + thigh_d[0], hip[1], hip[2] + thigh_d[2])
        ankle = (knee[0] + shin_d[0], knee[1], knee[2] + shin_d[2])
        toe = (ankle[0] + TOE_PIVOT[0], ankle[1], ankle[2] + TOE_PIVOT[2])
        out[f"{side}_hip"], out[f"{side}_knee"], out[f"{side}_ankle"], out[f"{side}_toe"] = hip, knee, ankle, toe
        out[f"{side}_thigh_dir"] = _dir_angles(thigh_d)
        out[f"{side}_shin_dir"] = _dir_angles(shin_d)
    out["spine"] = UPPER_ORIGIN
    out["cowl"] = upper_place((COWL_L[0] - COWL[0] / 2.0, 0.0, COWL_L[2]))
    for side, sign in (("r", 1.0), ("l", -1.0)):
        out[f"{side}_pod"] = upper_place((POD_L[0], sign * POD_L[1], POD_L[2]))
    out["muzzle"] = (LANCE_TIP_X, 0.0, LANCE_Z)
    out["strut_top"], out["strut_joint"], out["strut_end"] = STRUT_TOP, STRUT_JOINT, STRUT_END
    return out


def build_skeleton() -> skel.Skeleton:
    p = rest_positions()
    s = skel.Skeleton(root="root")
    s.add("root", None, (0.0, 0.0, 0.0), "ground-contact centre between the braced feet; never carries simulation translation")
    s.add("body", "root", (HIP[0], 0.0, HIP[2]), "hip line: open frame, hip hubs and the lance-mount arm")
    s.add("spine", "body", p["spine"], "upper open frame and the deck carrying the pods and the cowl (pitched 8 deg forward)")
    s.add("cowl", "spine", p["cowl"], "low sensor cowl; visor pitch")
    for side in ("r", "l"):
        s.add(f"{side}_pod", "spine", p[f"{side}_pod"], "shoulder pod and its antenna rod")
    s.add("lance_yaw", "body", YAW_PIVOT, "hip-mount yaw ring; card sub-object Turret_Y")
    s.add("lance_barrel", "lance_yaw", TRUNNION, "rail-lance trunnion: pitch and the recoil slide along its own axis; card sub-object Barrel_X")
    for side in ("r", "l"):
        s.add(f"{side}_thigh", "body", p[f"{side}_hip"], "hip hinge (pitch) and stance yaw")
        s.add(f"{side}_shin", f"{side}_thigh", p[f"{side}_knee"], "knee hinge (pitch); prominent knee hub")
        s.add(f"{side}_foot", f"{side}_shin", p[f"{side}_ankle"], "ankle; planted block foot")
        s.add(f"{side}_toe", f"{side}_foot", p[f"{side}_toe"], "toe plate hinge; roll-off on the move cycle")
    s.add("strut_upper", "body", STRUT_TOP, "recoil-strut anchor at the lower rear of the body")
    s.add("strut_slide", "strut_upper", STRUT_JOINT, "strut slide/piston section: extends on plant, works on the shot")
    return s


# --- limb parts authored in their local frames (pivot at the joint, limb along local +X) -----------
def part_thigh(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_002_Thigh")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    L, sides = THIGH_LEN, (8 if lod == 0 else 6)
    m.box((L / 2.0, 0.0, 0.0), (L, LEG_W - 3.0, LEG_W - 2.0), frame, "thigh_strut")
    # concept item 7 is "pale plate on the thigh and shin OUTER faces": two plates on the +-Y flanks of
    # the charcoal strut, not a box wrapping it, so nothing pale faces aft (concept REAR panel, README 8.7)
    for sy in (-1.0, 1.0):
        m.box((L * 0.54, sy * (LEG_W / 2.0 + 0.5), 0.0), (L * 0.64, 4.0, LEG_W - 5.0), ceramic, "thigh_plate")
    m.tube((0.0, -LEG_W / 2.0 - 3.0, 0.0), (0.0, LEG_W / 2.0 + 3.0, 0.0), 10.0, sides, frame, "hip_hub", caps=True)
    if lod == 0:
        m.box((L * 0.80, 0.0, 0.0), (7.0, LEG_W + 7.0, LEG_W - 4.0), frame, "thigh_collar")
        for sy in (-1.0, 1.0):
            m.box((L * 0.40, sy * (LEG_W / 2.0 + 2.9), 0.0), (10.0, 1.2, 3.0), status, "thigh_status")
    return m


def part_shin(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_002_Shin")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    L, sides = SHIN_LEN, (8 if lod == 0 else 6)
    m.box((L / 2.0, 0.0, 0.0), (L, LEG_W - 5.0, LEG_W - 4.0), frame, "shin_strut")
    for sy in (-1.0, 1.0):                       # outer faces only (concept item 7); see part_thigh
        m.box((L * 0.52, sy * (LEG_W / 2.0 - 1.25), 0.0), (L * 0.58, 3.5, LEG_W - 7.0), ceramic, "shin_plate")
    m.tube((0.0, -LEG_W / 2.0 - 4.0, 0.0), (0.0, LEG_W / 2.0 + 4.0, 0.0), KNEE_R, sides, frame, "knee_hub", caps=True)
    if lod == 0:
        for sy in (-1.0, 1.0):
            m.tube((0.0, sy * (LEG_W / 2.0 + 4.0), 0.0), (0.0, sy * (LEG_W / 2.0 + 5.6), 0.0), KNEE_R - 3.0, sides, ceramic, "knee_ring", caps=True)
        m.box((L * 0.88, 0.0, 0.0), (6.0, LEG_W + 1.0, LEG_W - 5.0), frame, "shin_collar")
        m.box((L * 0.30, 0.0, LEG_W / 2.0 - 1.6), (9.0, 3.0, 1.2), status, "shin_status")
    return m


def part_foot(lod: int) -> kit.Mesh:
    """Ankle at the origin; the sole rests on local z = -ANKLE_Z so the foot stands on z = 0."""
    m = kit.Mesh("EBS_MER_UNT_002_Foot")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    sides = 8 if lod == 0 else 6
    m.tube((0.0, -LEG_W / 2.0 - 2.0, 0.0), (0.0, LEG_W / 2.0 + 2.0, 0.0), 7.5, sides, frame, "ankle_hub", caps=True)
    m.box((-2.0, 0.0, -ANKLE_Z / 2.0 - 2.0), (14.0, LEG_W - 2.0, ANKLE_Z - 8.0), frame, "pastern")
    m.box(((TOE_PIVOT[0] - FOOT_BACK) / 2.0, 0.0, -ANKLE_Z + 5.0), (FOOT_BACK + TOE_PIVOT[0], LEG_W + 10.0, 10.0), frame, "foot_sole")
    m.box((-FOOT_BACK + 5.0, 0.0, -ANKLE_Z + 14.0), (12.0, LEG_W + 4.0, 10.0), frame, "heel_spur")
    m.box((-2.0, 0.0, -ANKLE_Z + 15.0), (18.0, LEG_W + 6.0, 6.0), ceramic, "foot_plate")
    if lod == 0:
        for sy in (-1.0, 1.0):
            m.tube((0.0, sy * (LEG_W / 2.0 + 2.0), 0.0), (0.0, sy * (LEG_W / 2.0 + 3.4), 0.0), 5.0, sides, ceramic, "ankle_ring", caps=True)
    return m


def part_toe(lod: int) -> kit.Mesh:
    """Toe hinge at the origin (world z 8); the toe block reaches FOOT_FWD - TOE_PIVOT[0] ahead."""
    m = kit.Mesh("EBS_MER_UNT_002_Toe")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    reach = FOOT_FWD - TOE_PIVOT[0]
    m.box((reach / 2.0, 0.0, -3.0), (reach, LEG_W + 8.0, 10.0), frame, "toe_block")
    m.box((reach / 2.0 + 1.0, 0.0, 3.0), (reach - 4.0, LEG_W + 2.0, 4.0), ceramic, "toe_plate")     # raised toe plate (item 7)
    if lod == 0:
        for sy in (-1.0, 1.0):
            m.box((reach - 2.0, sy * (LEG_W / 2.0 + 3.0), -4.0), (8.0, 5.0, 8.0), frame, "toe_claw")
    return m


def part_strut_upper(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_002_StrutUpper")
    frame = m.slot(FRAME)
    L, sides = STRUT_LEN * STRUT_SPLIT, (8 if lod == 0 else 6)
    m.tube((2.0, 0.0, 0.0), (L, 0.0, 0.0), STRUT_R, sides, frame, "strut_rod", caps=True)
    m.box((0.0, 0.0, 0.0), (8.0, 9.0, 9.0), frame, "strut_anchor")
    if lod == 0:
        m.box((L - 5.0, 0.0, 0.0), (10.0, 10.0, 10.0), frame, "strut_gland")    # the visible slide gland at the middle
    return m


def part_strut_slide(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_002_StrutSlide")
    frame = m.slot(FRAME)
    L, sides = STRUT_LEN * (1.0 - STRUT_SPLIT), (8 if lod == 0 else 6)
    m.tube((0.0, 0.0, 0.0), (L - 4.0, 0.0, 0.0), STRUT_R - 1.2, sides, frame, "strut_piston", caps=True)
    m.box((L - 2.0, 0.0, 0.0), (8.0, 9.0, 9.0), frame, "strut_foot_joint")
    return m


# --- lance parts (own local frames; card sub-object separation Turret_Y / Barrel_X) ----------------
def part_lance_yoke(lod: int) -> kit.Mesh:
    """Hip-mount yaw ring and yoke; origin at the yaw axis (YAW_PIVOT), +X forward."""
    m = kit.Mesh(YOKE_ASSET)
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    sides = 10 if lod == 0 else 6
    m.tube((0.0, 0.0, -11.0), (0.0, 0.0, 11.0), 11.0, sides, frame, "yaw_ring", caps=True)
    for sy in (-1.0, 1.0):
        m.box((9.0, sy * 13.0, 0.0), (22.0, 6.0, 20.0), frame, "yoke_cheek")
    m.box((16.0, 0.0, -9.0), (14.0, 26.0, 6.0), frame, "yoke_cradle")
    if lod == 0:
        m.box((4.0, 0.0, 12.0), (16.0, 18.0, 4.0), ceramic, "yoke_cap")
        m.box((18.5, 0.0, 11.0), (5.0, 3.0, 1.4), status, "yoke_status")
    return m


def part_lance_barrel(lod: int) -> kit.Mesh:
    """Rail-lance; origin at the trunnion (TRUNNION), axis along local +X, recoils along -X."""
    m = kit.Mesh(BARREL_ASSET)
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    sides = 8 if lod == 0 else 6
    x0 = LANCE_TIP_X - LANCE_LEN - TRUNNION[0]
    x1 = LANCE_TIP_X - TRUNNION[0]
    span = x1 - x0
    m.box(((x0 + x1) / 2.0, 0.0, 0.0), (span, 10.0, 2.0 * CHANNEL_HZ + 6.0), frame, "lance_body")
    for sz in (-1.0, 1.0):
        m.box(((x0 + x1) / 2.0, 0.0, sz * RAIL_DZ), (span, 2.0 * RAIL_HY, 2.0 * RAIL_HZ), frame, "lance_rail")
    # the cyan channel runs unbroken from the breech to the muzzle plug: every collar and the muzzle head
    # are split into blocks ABOVE and BELOW the channel band, exactly as the concept's collars are, so the
    # side read is one continuous cyan stroke instead of concept-v1's three or four dashes (README 8.7)
    chan_x1 = x1 - MUZZLE_CAP
    m.box(((x0 + chan_x1) / 2.0 + 1.0, 0.0, 0.0), (chan_x1 - x0 - 2.0, 2.0 * CHANNEL_HY, 2.0 * CHANNEL_HZ), status, "lance_channel")
    m.box((x0 + 16.0, 0.0, 0.0), (32.0, 22.0, 26.0), frame, "lance_breech")
    collar_h = CLAMP_HZ - CHANNEL_HZ
    for k, cx in enumerate(CLAMP_X):
        for sz in (-1.0, 1.0):
            m.box((cx - TRUNNION[0], 0.0, sz * (CHANNEL_HZ + collar_h / 2.0)), (8.0, 16.0, collar_h), frame, f"lance_clamp_{k + 1:02d}")
        if lod == 0:
            m.box((cx - TRUNNION[0], 0.0, CLAMP_HZ - 1.0), (6.0, 12.0, 2.0), ceramic, f"lance_clamp_{k + 1:02d}_cap")
    head_h = RAIL_DZ + RAIL_HZ - CHANNEL_HZ
    for sz in (-1.0, 1.0):
        m.box(((MUZZLE_X0 + LANCE_TIP_X) / 2.0 - TRUNNION[0], 0.0, sz * (CHANNEL_HZ + head_h / 2.0)),
              (LANCE_TIP_X - MUZZLE_X0, 18.0, head_h), frame, "lance_muzzle_head")
    m.box((x1 - MUZZLE_CAP / 2.0, 0.0, 0.0), (MUZZLE_CAP, 20.0, 2.0 * CHANNEL_HZ), frame, "lance_muzzle_plug")
    if lod == 0:
        m.tube((x1 - 1.5, 0.0, 0.0), (x1 + 0.5, 0.0, 0.0), CHANNEL_HZ + 1.0, sides, frame, "lance_muzzle_bore", caps=True)
        m.box((x0 + 6.0, 0.0, 15.0), (16.0, 12.0, 4.0), ceramic, "lance_breech_cap")
    m.sockets.append(kit.Socket("Muzzle_Flash_01", (x1, 0.0, 0.0), 0.0,
                                "rail-lance muzzle face; the canon clean cyan-white line effect origin (SPEC-UNIT-002)"))
    return m


# --- body, upper frame, deck, pods, cowl ------------------------------------------------------------
def build_body(lod: int) -> kit.Mesh:
    """Hips, the open charcoal frame with exposed flanks, the lance-mount arm (bone: body)."""
    m = kit.Mesh("EBS_MER_UNT_002_Body")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    hi = lod == 0
    sides = 8 if hi else 6
    m.box((HIP[0] + 1.0, 0.0, HIP[2]), (26.0, 2.0 * HIP[1] - 6.0, 16.0), frame, "hip_beam")
    for sy in (-1.0, 1.0):
        m.tube((HIP[0], sy * (HIP[1] - 8.0), HIP[2]), (HIP[0], sy * (HIP[1] + 2.0), HIP[2]), 11.0, sides, frame, "hip_hub")
    # concept-v2: the open frame and the deck it carries sit BACK over the hips (the spine origin moved
    # 38 cm aft), so the core column, the flank struts and the shoulder yoke move with them; the lance
    # mount arm, the hip beam and the tail beam stay where the hip line puts them.
    m.box((-20.0, 0.0, 134.0), (34.0, CORE_W, 38.0), frame, "core_block")
    m.box((-36.0, 0.0, 110.0), (42.0, 44.0, 12.0), frame, "tail_beam")   # rear spur carrying the recoil-strut anchor
    for sy in (-1.0, 1.0):
        m.box((-52.0, sy * 13.0, 141.0), (7.0, 5.0, 44.0), frame, "flank_strut_rear")
        m.box((-22.0, sy * 12.0, 148.0), (6.0, 5.0, 30.0), frame, "flank_strut_front")
        if hi:
            m.box((-38.0, sy * 13.5, 156.0), (38.0, 3.0, 4.0), frame, "flank_brace")
            m.tube((-52.0, sy * 9.0, 150.0), (-52.0, sy * 17.0, 150.0), 9.0, sides, frame, "flank_hub")
    m.box((12.0, 0.0, LANCE_Z + 1.0), (34.0, 20.0, 16.0), frame, "mount_arm")
    m.box((-8.0, 0.0, 156.0), (26.0, 24.0, 20.0), frame, "shoulder_yoke")   # links the deck front forward over the breech
    for sy in (-1.0, 1.0):
        m.box((0.0, sy * 14.0, 148.0), (6.0, 5.0, 26.0), frame, "shoulder_stay")   # down each side of the breech
    m.box((6.0, 0.0, 120.0), (14.0, 24.0, 12.0), frame, "mount_brace")
    if hi:
        m.box((-6.0, 0.0, 98.0), (22.0, 26.0, 10.0), frame, "belly_pan")
        m.box((-13.0, 0.0, 141.0), (8.0, 4.0, 1.4), status, "core_status")
    m.sockets.append(kit.Socket("Target_Anchor_Center", (-4.0, 0.0, 138.0), 0.0,
                                "body centre; damage acknowledgement, selection and health reference"))
    del ceramic
    return m


def build_spine(lod: int) -> kit.Mesh:
    """Upper open frame and the ceramic deck plate, spine-local (unpitched)."""
    m = kit.Mesh("EBS_MER_UNT_002_Spine")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    m.box(DECK_FRAME_L, DECK_FRAME, frame, "deck_frame")     # charcoal, full deck width: the rearmost deck surface
    m.box(DECK_L, DECK, ceramic, "deck_plate")               # ceramic inset 5.4 cm forward of it (concept REAR panel)
    if lod == 0:
        for sy in (-1.0, 1.0):
            m.box((DECK_L[0] - 6.0, sy * (DECK[1] / 2.0 - 3.0), DECK_L[2] - 3.0), (30.0, 5.0, 6.0), frame, "deck_rail")
        m.box((DECK_L[0] - DECK[0] / 2.0 + 3.0, 0.0, DECK_L[2] + 6.0), (5.0, 30.0, 10.0), frame, "deck_stop")
    return m


def build_pod(lod: int, sign: float) -> kit.Mesh:
    """One shoulder pod with its antenna rod, spine-local (unpitched)."""
    m = kit.Mesh("EBS_MER_UNT_002_Pod")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    sides = 8 if lod == 0 else 6
    cx, cy, cz = POD_L[0], sign * POD_L[1], POD_L[2]
    back = 4.0
    m.box((cx + back / 2.0, cy, cz), (POD[0] - back, POD[1], POD[2]), ceramic, "pod_shell")
    # concept REAR panel at 2.2x: each pod closes with a CHARCOAL back and only a pale cap on its top
    # quarter; concept-v1 presented the whole 28 x 26 ceramic rear face aft (README 8.7)
    m.box((cx - (POD[0] - back) / 2.0, cy, cz - POD[2] * 0.15), (back, POD[1], POD[2] * 0.7), frame, "pod_back")
    m.box((cx - (POD[0] - back) / 2.0, cy, cz + POD[2] * 0.35), (back, POD[1], POD[2] * 0.3), ceramic, "pod_back_cap")
    m.box((cx - 4.0, cy, cz - POD[2] / 2.0 - 3.0), (POD[0] - 10.0, POD[1] - 6.0, 8.0), frame, "pod_cradle")
    m.box((cx + 6.0, cy + sign * (POD[1] / 2.0 - 0.4), cz + 2.0), (14.0, 2.0, 4.0), status, "pod_strip")
    ax, ay = cx + ANTENNA_L[0], sign * ANTENNA_L[1]
    m.tube((ax, ay, cz + POD[2] / 2.0), (ax, ay, ANTENNA_L[2]), 1.6, sides, frame, "pod_antenna", caps=True)
    if lod == 0:
        m.box((cx - 2.0, cy, cz + POD[2] / 2.0 - 1.0), (POD[0] - 12.0, POD[1] - 8.0, 3.0), frame, "pod_vent")
        m.tube((cx - 9.0, cy + sign * (POD[1] / 2.0), cz - 5.0), (cx - 9.0, cy + sign * (POD[1] / 2.0 + 2.0), cz - 5.0), 6.0, sides, frame, "pod_hub", caps=True)
    return m


POD_TOP_COMPONENTS = ("pod_shell", "pod_back_cap")   # the pale pod box; H is defined at its top


def _pod_top_unshifted() -> float:
    """Top z of the pod's pale box after the forward pitch, before the corrective lift. Built from the
    real pod part (not a stand-in box) so the H = 200 cm landing survives changes to the pod geometry."""
    probe = kit.Mesh("probe")
    probe.merge(build_pod(0, 1.0), translate=UPPER_PIVOT, pitch_deg=BODY_PITCH, component_prefix="p_")
    return max(probe.component_bounds(f"p_{c}")[1][2] for c in POD_TOP_COMPONENTS)


UPPER_DZ = H - _pod_top_unshifted()        # corrective lift so the pitched pod tops sit exactly at H
UPPER_ORIGIN = (UPPER_PIVOT[0], UPPER_PIVOT[1], UPPER_PIVOT[2] + UPPER_DZ)


def upper_place(local):
    """Spine-local (unpitched) point -> world."""
    return kit.v_add(kit.rot_yp(local, 0.0, BODY_PITCH), UPPER_ORIGIN)

def build_cowl(lod: int) -> kit.Mesh:
    """Low sensor cowl between the pods with a horizontal cyan visor slot, spine-local (unpitched)."""
    m = kit.Mesh("EBS_MER_UNT_002_Cowl")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    cx, cy, cz = COWL_L
    lower_h = 14.0
    m.box((cx, cy, cz - COWL[2] / 2.0 + lower_h / 2.0), (COWL[0], COWL[1], lower_h), ceramic, "cowl_lower")
    m.box((cx - 4.0, cy, cz + COWL[2] / 2.0 - 6.0), (COWL[0] - 8.0, COWL[1] - 4.0, 12.0), ceramic, "cowl_upper")
    m.box((cx + COWL[0] / 2.0 + 0.4, cy, cz - 1.0), (1.6, COWL[1] - 6.0, 5.0), status, "cowl_visor")
    m.box((cx + COWL[0] / 2.0 - 1.0, cy, cz - 1.0), (2.0, COWL[1] - 2.0, 9.0), frame, "cowl_bezel")
    m.box((cx + COWL[0] / 2.0 + 4.0, cy, cz - COWL[2] / 2.0 + 5.0), (10.0, COWL[1] - 8.0, 8.0), ceramic, "cowl_nose")
    # charcoal rear closure over the lower three quarters of the cowl, pale only on the top strip, as the
    # concept REAR panel shows the central block (concept-v2; concept-v1 faced ceramic aft here too)
    m.box((cx - COWL[0] / 2.0 + 3.0, cy, cz - 3.5), (6.0, COWL[1] - 2.0, 19.0), frame, "cowl_mount")
    return m


# --- assembly ---------------------------------------------------------------------------------------
BINDING = {
    "body_": "body", "spine_": "spine", "cowl_": "cowl", "r_pod_": "r_pod", "l_pod_": "l_pod",
    "yaw_": "lance_yaw", "lance_": "lance_barrel",
    "r_thigh_": "r_thigh", "r_shin_": "r_shin", "r_foot_": "r_foot", "r_toe_": "r_toe",
    "l_thigh_": "l_thigh", "l_shin_": "l_shin", "l_foot_": "l_foot", "l_toe_": "l_toe",
    "strut_upper_": "strut_upper", "strut_slide_": "strut_slide",
}
# Owner ruling 2026-09-07: "Our recorded Lancer decision specifies Rear_Recoil_Strut_Anchor, with
# Left_Tread_Vector retained temporarily as a compatibility alias. Left-foot ground contact is a
# different function; it should have a separately named foot-contact socket if needed."
# Rear_Recoil_Strut_Anchor and its alias sit on strut_upper, whose head IS the strut's body anchor, so
# both are at the joint's own rotation centre and hold that point in every clip.
SOCKETS_ON_BONES = {"Muzzle_Flash_01": "lance_barrel", "Target_Anchor_Center": "body",
                    "Rear_Recoil_Strut_Anchor": "strut_upper", "Left_Tread_Vector": "strut_upper",
                    "Foot_Contact_L": "l_foot", "Foot_Contact_R": "r_foot"}
STRUT_ANCHOR_PURPOSE = ("rear-leg recoil strut anchor: the UPPER end, where the strut bolts to the tail beam and "
                        "transmits recoil into the frame (the lower end delivers it to the ground through the "
                        "trailing foot). At strut_upper's head, i.e. the joint's rotation centre, so it holds the "
                        "anchor point through the per-clip strut solve")
ALIAS_PURPOSE = ("TEMPORARY COMPATIBILITY ALIAS of Rear_Recoil_Strut_Anchor (owner ruling 2026-09-07): identical "
                 "bone, position and rotation; to be dropped once the adapter migration is tested. This frame is "
                 "two-legged and has no tread. It is NOT a ground-contact socket - left-foot ground contact is a "
                 "different function and is carried by Foot_Contact_L")


def assemble(lod: int):
    """Rest-stance mesh with every polygon bound to its bone, plus the skeleton."""
    p = rest_positions()
    s = build_skeleton()
    m = kit.Mesh(ASSET)
    for name in (FRAME, CERAMIC, STATUS_REVIEW):
        m.slot(name)
    m.merge(build_body(lod), component_prefix="body_", include_sockets=True)
    m.merge(build_spine(lod), translate=UPPER_ORIGIN, pitch_deg=BODY_PITCH, component_prefix="spine_")
    m.merge(build_cowl(lod), translate=UPPER_ORIGIN, pitch_deg=BODY_PITCH, component_prefix="cowl_")
    for side, sign in (("r", 1.0), ("l", -1.0)):
        m.merge(build_pod(lod, sign), translate=UPPER_ORIGIN, pitch_deg=BODY_PITCH, component_prefix=f"{side}_pod_")
    m.merge(part_lance_yoke(lod), translate=YAW_PIVOT, component_prefix="yaw_", include_sockets=False)
    m.merge(part_lance_barrel(lod), translate=TRUNNION, component_prefix="lance_", include_sockets=True)
    for side in ("r", "l"):
        tp, ty = p[f"{side}_thigh_dir"]
        sp, sy = p[f"{side}_shin_dir"]
        m.merge(part_thigh(lod), translate=p[f"{side}_hip"], pitch_deg=tp, yaw_deg=ty, component_prefix=f"{side}_thigh_")
        m.merge(part_shin(lod), translate=p[f"{side}_knee"], pitch_deg=sp, yaw_deg=sy, component_prefix=f"{side}_shin_")
        m.merge(part_foot(lod), translate=p[f"{side}_ankle"], component_prefix=f"{side}_foot_")
        m.merge(part_toe(lod), translate=p[f"{side}_toe"], component_prefix=f"{side}_toe_")
    m.merge(part_strut_upper(lod), translate=STRUT_TOP, pitch_deg=STRUT_PITCH, yaw_deg=STRUT_YAW, component_prefix="strut_upper_")
    m.merge(part_strut_slide(lod), translate=STRUT_JOINT, pitch_deg=STRUT_PITCH, yaw_deg=STRUT_YAW, component_prefix="strut_slide_")
    # owner ruling 2026-09-07: the strut anchor socket, its temporary alias at the SAME transform, and
    # separately named foot-contact sockets for the different (ground-contact) function.
    m.sockets.append(kit.Socket("Rear_Recoil_Strut_Anchor", STRUT_TOP, 0.0, STRUT_ANCHOR_PURPOSE))
    m.sockets.append(kit.Socket("Left_Tread_Vector", STRUT_TOP, 0.0, ALIAS_PURPOSE))
    for side, sign in (("l", -1.0), ("r", 1.0)):
        sole = (p[f"{side}_ankle"][0] + (FOOT_FWD - FOOT_BACK) / 2.0, p[f"{side}_ankle"][1], 0.0)
        m.sockets.append(kit.Socket(f"Foot_Contact_{side.upper()}", sole, 0.0,
                                    f"{'LEFT (trailing)' if sign < 0 else 'RIGHT (leading)'} sole ground-contact centre "
                                    "(heel-to-toe midpoint of the block foot, on z = 0): footfall effects, dust and "
                                    "planted-sole reference. Separate from the recoil-strut anchor by owner ruling 2026-09-07"))
    counts = skel.bind_polygons(m, "root", BINDING)
    return m, s, counts, dict(SOCKETS_ON_BONES)


def export_mesh(m: kit.Mesh) -> kit.Mesh:
    """Two-slot export copy: the review-only cyan pseudo slot folds into the ceramic slot."""
    out = kit.Mesh(m.name)
    out.slot(FRAME)
    out.slot(CERAMIC)
    for poly in m.polygons:
        name = m.slots[poly.slot]
        slot = out.slot(CERAMIC if name == STATUS_REVIEW else name)
        q = kit.Polygon(list(poly.points), poly.normal, slot, poly.component, poly.uv_axis, poly.uv_override, poly.atlas_cells, poly.chart_id)
        q.bone = getattr(poly, "bone", None)
        out.polygons.append(q)
    out.sockets = list(m.sockets)
    out.collision = list(m.collision)
    return out


def polygon_area(poly) -> float:
    pts = poly.points
    total = (0.0, 0.0, 0.0)
    for k in range(1, len(pts) - 1):
        total = kit.v_add(total, kit.v_cross(kit.v_sub(pts[k], pts[0]), kit.v_sub(pts[k + 1], pts[0])))
    return 0.5 * kit.v_len(total)


def emissive_area_fraction(m: kit.Mesh) -> float:
    status = m.slot(STATUS_REVIEW)
    total = sum(polygon_area(p) for p in m.polygons)
    cyan = sum(polygon_area(p) for p in m.polygons if p.slot == status)
    return cyan / total if total else 0.0


# --- forward kinematics for clip authoring (mirrors ebs_skelkit.pose_mesh exactly) -----------------
def _entry(value):
    vals = tuple(float(v) for v in value)
    if len(vals) == 3:
        return vals, (0.0, 0.0, 0.0)
    if len(vals) == 6:
        return vals[:3], vals[3:]
    raise ValueError("pose entries are (pitch, yaw, roll) or (pitch, yaw, roll, tx, ty, tz)")


def fk_point(skeleton: skel.Skeleton, pose: dict, bone: str, point, stop_before: str | None = None):
    """World position of a rest-stance point bound to ``bone`` under ``pose`` (same composition as
    ebs_skelkit.pose_mesh: rotate at each bone head, bone first, root last). ``stop_before`` ends the
    walk before that ancestor, giving the point in that ancestor's own (pre-transform) frame."""
    p = tuple(point)
    for name in skeleton.chain_to_root(bone):
        if name == stop_before:
            break
        rot, tr = _entry(pose.get(name, (0.0, 0.0, 0.0)))
        head = skeleton.get(name).head
        p = kit.v_add(kit.v_add(skel.rot_rotator(kit.v_sub(p, head), rot[0], rot[1], rot[2]), head), tr)
    return p


def sole_probes() -> list:
    """(bone, rest-stance point) pairs on the two soles: the heel, the toe hinge and the toe tip."""
    p = rest_positions()
    out = []
    for side in ("r", "l"):
        ax, ay, _az = p[f"{side}_ankle"]
        tx, ty, _tz = p[f"{side}_toe"]
        out.append((f"{side}_foot", (ax - FOOT_BACK, ay, 0.0)))
        out.append((f"{side}_foot", (ax + TOE_PIVOT[0], ay, 0.0)))
        out.append((f"{side}_toe", (tx, ty, 0.0)))
        out.append((f"{side}_toe", (ax + FOOT_FWD, ay, 0.0)))
    return out


SOLE_PROBES = sole_probes()


def ground_drop(skeleton: skel.Skeleton, pose: dict) -> float:
    """Body translation z that puts the lowest sole point exactly on z = 0 for this pose."""
    return -min(fk_point(skeleton, pose, bone, pt)[2] for bone, pt in SOLE_PROBES)


def leg_delta(side: str, ankle_x: float, ankle_z: float = ANKLE_Z, knee_sign: float | None = None):
    """(thigh, shin) bone pitches that move one leg's ankle to (ankle_x, ankle_z) from the rest stance."""
    thigh, shin = solve_leg(ankle_x, ankle_z, knee_sign)
    th1, sh1 = _limb_theta(thigh), _limb_theta(shin)
    th0, sh0 = REST_THETA[side]
    return th1 - th0, (sh1 - th1) - (sh0 - th0)


def leg_keys(side: str, ankle_x: float, ankle_z: float = ANKLE_Z, knee_sign: float | None = None,
             foot_tilt: float = 0.0, toe: float = 0.0):
    """(thigh, shin, foot, toe) keys placing the ankle and keeping the sole level (plus tilts)."""
    d_thigh, d_shin = leg_delta(side, ankle_x, ankle_z, knee_sign)
    return d_thigh, d_shin, -(d_thigh + d_shin) + foot_tilt, toe


REST_LEG_KEYS = (0.0, 0.0, 0.0, 0.0)

# --- recoil strut: it is BOLTED to the trailing ankle, so it is solved from the trailing leg ---------
STOW_RETRACT = 38.0                       # travel stow: the piston runs home into the gland
STRUT_ANKLE_OFFSET = tuple(STRUT_END[i] - (TRAIL_ANKLE_X, -HIP[1], ANKLE_Z)[i] for i in range(3))
_SKELETON_CACHE = []


def _skeleton():
    if not _SKELETON_CACHE:
        _SKELETON_CACHE.append(build_skeleton())
    return _SKELETON_CACHE[0]


def _aim_rotator(d0, d1):
    """(pitch, yaw) with rot_rotator(d0, pitch, yaw, 0) == d1 for unit d0, d1. rot_y keeps y and rot_z
    keeps z, so the pitch follows from d1's z alone and the yaw from the remaining xy turn. The pitch
    branch is the one that gives (0, 0) when d1 == d0, i.e. the branch matching sign(d0.x)."""
    r0 = math.hypot(d0[0], d0[2])
    alpha = math.atan2(d0[2], d0[0])
    k = max(-1.0, min(1.0, d1[2] / r0)) if r0 > 1e-9 else 0.0
    x = math.asin(k) if d0[0] >= 0.0 else math.pi - math.asin(k)
    ux = r0 * math.cos(x)
    pitch = math.degrees(x - alpha)
    yaw = math.degrees(math.atan2(d1[1], d1[0]) - math.atan2(d0[1], ux))
    return (pitch + 180.0) % 360.0 - 180.0, (yaw + 180.0) % 360.0 - 180.0


def strut_keys(pose: dict, plant: float, work: float = 0.0) -> dict:
    """`strut_upper` rotation and `strut_slide` translation for a pose, solved so the strut's foot joint
    keeps its rest offset from the TRAILING ankle whenever the strut is planted.

    ``plant`` 1.0 tracks the trailing ankle (the braced, planting and firing states); 0.0 stows the strut
    — the piston runs home and the rod keeps its rest angle, because a walking leg swings forward past
    the anchor and no strut geometry can both stay bolted to it and stay clear of the body (README 8.7).
    Values between the two blend the target point, so `stop` and the fire halt-to-plant read as the strut
    reaching down onto the foot. ``work`` adds a slide stroke on top (the shot).
    """
    sk = _skeleton()
    ankle = fk_point(sk, pose, "l_foot", (TRAIL_ANKLE_X, -HIP[1], ANKLE_Z), stop_before="body")
    tracked = tuple(ankle[i] + STRUT_ANKLE_OFFSET[i] for i in range(3))
    stowed = tuple(STRUT_TOP[i] + STRUT_DIR[i] * (STRUT_LEN - STOW_RETRACT) for i in range(3))
    target = tuple(stowed[i] + (tracked[i] - stowed[i]) * plant for i in range(3))
    d = tuple(target[i] - STRUT_TOP[i] for i in range(3))
    n = _len(d)
    pitch, yaw = _aim_rotator(STRUT_DIR, tuple(c / n for c in d))
    return {"strut_upper": (pitch, yaw, 0.0), "strut_slide": strut_shift(n - STRUT_LEN + work)}


def make_pose(body=(0.0, 0.0, 0.0), r=REST_LEG_KEYS, l=REST_LEG_KEYS, extra=None,
              plant: float = 1.0, strut_work: float = 0.0) -> dict:
    """Pose dict: a body rotation with both thighs countering it (planted feet), per-leg keys, the
    solved recoil-strut keys (see ``strut_keys``) and extras."""
    pose = {"body": tuple(body)}
    for side, seg in (("r", r), ("l", l)):
        pose[f"{side}_thigh"] = (seg[0] - body[0], -body[1], -body[2])
        pose[f"{side}_shin"] = (seg[1], 0.0, 0.0)
        pose[f"{side}_foot"] = (seg[2], 0.0, 0.0)
        pose[f"{side}_toe"] = (seg[3], 0.0, 0.0)
    pose.update(strut_keys(pose, plant, strut_work))
    if extra:
        pose.update(extra)
    return pose


def write_pose(clip: skel.AnimationClip, skeleton: skel.Skeleton, t: float, pose: dict) -> None:
    """Solve the body height so the lowest sole rests on the ground, then key every posed bone."""
    dz = ground_drop(skeleton, pose)
    br = _entry(pose["body"])[0]
    for bone, value in pose.items():
        rot, tr = _entry(value)
        if bone == "body":
            rot, tr = br, (0.0, 0.0, dz)
        clip.key(bone, t, rot, tr)


def _norm_pose(pose: dict) -> dict:
    out = {}
    for bone, value in pose.items():
        rot, tr = _entry(value)
        out[bone] = (*rot, *tr)
    return out


def emit_keys(clip: skel.AnimationClip, skeleton: skel.Skeleton, controls, step: float) -> None:
    """Emit dense keys along a list of (time, pose) control points: the numeric channels are
    linearly interpolated and every emitted key re-solves the body height, so the linear playback
    between keys stays inside the ground rule (the concept's fold and plant are non-linear)."""
    pts = [(float(t), _norm_pose(pose)) for t, pose in controls]
    bones = sorted({b for _, pose in pts for b in pose})
    zero = (0.0,) * 6
    for index in range(len(pts) - 1):
        t0, p0 = pts[index]
        t1, p1 = pts[index + 1]
        steps = max(1, int(round((t1 - t0) / step)))
        last = steps + 1 if index == len(pts) - 2 else steps
        for k in range(last):
            f = k / steps
            pose = {}
            for bone in bones:
                a, c = p0.get(bone, zero), p1.get(bone, zero)
                pose[bone] = tuple(a[j] + (c[j] - a[j]) * f for j in range(6))
            write_pose(clip, skeleton, t0 + (t1 - t0) * f, pose)


def strut_shift(amount: float):
    """Translation of the strut slide along the strut's own axis (positive = extended)."""
    return (0.0, 0.0, 0.0, STRUT_DIR[0] * amount, STRUT_DIR[1] * amount, STRUT_DIR[2] * amount)


# --- clips (Unreal rotators in degrees; seconds) ----------------------------------------------------
WALK_STRIDE = 96.0                       # 0.6 s cycle x two strides = 320 cm/s (SPEC-UNIT-002 move speed)
WALK_LIFT = 14.0
NEUTRAL_ANKLE = 34.0                     # travel stance the walk and the fire plant work from
FIRE_RECOIL_T = 1.00


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []
    half = WALK_STRIDE / 2.0

    def walk_leg(side, phase):
        """(keys, ) for one leg at cycle phase in [0,1): stance sweeps back, swing lifts and returns."""
        bob = 1.6 * abs(math.sin(2.0 * math.pi * phase))
        if phase < 0.5:
            u = phase / 0.5
            ax = HIP[0] + half - WALK_STRIDE * u
            toe = -8.0 * max(0.0, (u - 0.75) / 0.25)          # roll onto the toe plate before toe-off
            return leg_keys(side, ax, ANKLE_Z - bob, 1.0, toe=toe), bob
        u = (phase - 0.5) / 0.5
        ax = HIP[0] - half + WALK_STRIDE * u
        az = ANKLE_Z + WALK_LIFT * math.sin(math.pi * u) - bob
        return leg_keys(side, ax, az, 1.0, foot_tilt=6.0 * math.sin(math.pi * u), toe=0.0), bob

    # idle: the braced firing stance breathing; the lance stays level and on the centreline
    idle = skel.AnimationClip("idle", 2.4, loop=True, purpose="braced line-fire stance at rest: a 1 deg settle on the hips, the cowl scanning, the lance level at hip height on the centreline")
    for i, t in enumerate((0.0, 0.6, 1.2, 1.8, 2.4)):
        s = math.sin(2.0 * math.pi * t / 2.4)
        write_pose(idle, skeleton, t, make_pose(body=(-0.8 * s, 0.0, 0.0), extra={
            "cowl": (0.0, 2.5 * s, 0.0), "lance_yaw": (0.0, 1.2 * s, 0.0), "lance_barrel": (0.8 * s, 0.0, 0.0)}))
        del i
    clips.append(idle)

    # move: the frame leaves the brace and walks; the lance is carried level (it never fires while moving)
    move = skel.AnimationClip("move", 0.6, loop=True, purpose="walk cycle authored for 320 cm/s (two 96 cm strides per 0.6 s); leaves the braced stance for a travel stance, lance carried level and locked; play rate follows the authoritative velocity (REL-ART-009)")
    for i in range(25):
        phase = i / 24.0
        t = phase * 0.6
        rk, _ = walk_leg("r", phase)
        lk, _ = walk_leg("l", (phase + 0.5) % 1.0)
        swing = math.sin(2.0 * math.pi * phase)
        write_pose(move, skeleton, t, make_pose(body=(-4.0, 1.5 * swing, 1.2 * swing), r=rk, l=lk, plant=0.0, extra={
            "spine": (1.5, -1.5 * swing, 0.0), "lance_yaw": (0.0, -2.0 * swing, 0.0), "lance_barrel": (2.0, 0.0, 0.0)}))
    clips.append(move)

    # turn: stationary shuffle inside the brace while the runtime sweeps heading (SPEC-MOV-010)
    turn = skel.AnimationClip("turn", 0.6, loop=True, purpose="stationary shuffle inside the brace while the runtime sweeps heading (SPEC-MOV-010); no root yaw in the clip")
    for i, t in enumerate((0.0, 0.15, 0.3, 0.45, 0.6)):
        lift_r = 5.0 if i == 1 else 0.0
        lift_l = 5.0 if i == 3 else 0.0
        rk = leg_keys("r", LEAD_ANKLE_X, ANKLE_Z + lift_r, None) if lift_r else REST_LEG_KEYS
        lk = leg_keys("l", TRAIL_ANKLE_X, ANKLE_Z + lift_l, None) if lift_l else REST_LEG_KEYS
        yaw = 4.0 if i == 1 else (-4.0 if i == 3 else 0.0)
        write_pose(turn, skeleton, t, make_pose(body=(0.0, yaw, 0.0), r=rk, l=lk, extra={"lance_yaw": (0.0, -yaw, 0.0)}))
    clips.append(turn)

    # stop: the travel stance settles back into the brace (the trailing knee rolls into the kickstand)
    stop = skel.AnimationClip("stop", 0.4, purpose="settle out of travel into the braced stance: the feet spread fore-and-aft, the trailing knee rolls into the kickstand brace and the strut retracts (Bible: halts, plants, aims)")
    travel_r = leg_keys("r", NEUTRAL_ANKLE, ANKLE_Z, 1.0)
    travel_l = leg_keys("l", -NEUTRAL_ANKLE, ANKLE_Z, 1.0)
    emit_keys(stop, skeleton, [
        (0.0, make_pose(body=(-5.0, 0.0, 0.0), r=travel_r, l=travel_l, plant=0.0, extra={"lance_barrel": (2.0, 0.0, 0.0)})),
        (0.2, make_pose(body=(-6.5, 0.0, 0.0), r=tuple(v * 0.5 for v in travel_r), l=tuple(v * 0.5 for v in travel_l), plant=0.6, extra={"lance_barrel": (1.0, 0.0, 0.0)})),
        (0.4, make_pose(body=(0.0, 0.0, 0.0), plant=1.0)),
    ], 0.04)
    clips.append(stop)

    # fire: halt - plant - aim - fire - recover (canon SPEC-UNIT-002; never fires while moving)
    fire = skel.AnimationClip("fire", 2.2, purpose="halt, plant the strut, aim, fire with the lance recoiling along its own axis and the strut slide working, recover; canon SPEC-UNIT-002 line: never fires while moving")

    def fire_pose(legs, pitch, yaw, elevation, recoil, plant, work, cowl):
        """``elevation`` is the lance's ABSOLUTE elevation in degrees; the barrel key removes the
        body pitch so the aim holds the lance level on the target line (concept item 2). ``plant``
        drives the strut down onto the trailing ankle over the halt-to-plant phase and ``work`` is the
        extra slide stroke the shot drives through it."""
        bpitch = elevation - pitch
        r, l = (travel_r, travel_l) if legs == "travel" else (REST_LEG_KEYS, REST_LEG_KEYS)
        if isinstance(legs, float):
            r = tuple(v * legs for v in travel_r)
            l = tuple(v * legs for v in travel_l)
        return make_pose(body=(pitch, 0.0, 0.0), r=r, l=l, plant=plant, strut_work=work, extra={
            "lance_yaw": (0.0, yaw, 0.0), "lance_barrel": (bpitch, 0.0, 0.0, recoil, 0.0, 0.0),
            "cowl": (cowl, 0.0, 0.0), "spine": (-pitch * 0.25, 0.0, 0.0)})

    emit_keys(fire, skeleton, [
        (0.00, fire_pose("travel", -5.0, 7.0, 3.0, 0.0, 0.00, 0.0, 2.0)),
        (0.20, fire_pose(0.5, -7.0, 5.5, 2.0, 0.0, 0.55, 0.0, 1.5)),
        (0.35, fire_pose("brace", -6.0, 4.0, 1.0, 0.0, 1.00, 0.0, 1.0)),
        (0.75, fire_pose("brace", -3.0, 0.0, 0.0, 0.0, 1.00, 0.0, -1.0)),
        (0.95, fire_pose("brace", -3.0, 0.0, 0.0, 0.0, 1.00, 0.0, -1.0)),
        (FIRE_RECOIL_T, fire_pose("brace", 2.5, 0.0, 1.8, -RECOIL_CM, 1.00, 8.5, -2.0)),
        (1.15, fire_pose("brace", 1.0, 0.0, 0.9, -RECOIL_CM * 0.45, 1.00, 4.0, -1.5)),
        (1.45, fire_pose("brace", -3.0, 0.0, 0.0, 0.0, 1.00, 0.0, -1.0)),
        (1.80, fire_pose("brace", -4.0, 0.0, 0.0, 0.0, 1.00, 0.0, 0.0)),
        (2.20, fire_pose("brace", 0.0, 0.0, 0.0, 0.0, 1.00, 0.0, 0.0)),
    ], 0.05)
    clips.append(fire)

    # damage: flinch on authoritative damage; no displacement, the brace holds
    damage = skel.AnimationClip("damage", 0.35, purpose="flinch on authoritative damage: the frame rocks back on the brace and the lance lifts; no displacement, the planted feet hold")
    emit_keys(damage, skeleton, [
        (0.0, make_pose(body=(0.0, 0.0, 0.0), extra={"lance_barrel": (0.0, 0.0, 0.0), "cowl": (0.0, 0.0, 0.0)})),
        (0.1, make_pose(body=(6.0, 0.0, 0.0), extra={"lance_barrel": (4.0, 0.0, 0.0), "lance_yaw": (0.0, -3.0, 0.0), "cowl": (2.0, 3.0, 0.0)})),
        (0.22, make_pose(body=(-2.0, 0.0, 0.0), extra={"lance_barrel": (-1.0, 0.0, 0.0), "lance_yaw": (0.0, 1.0, 0.0), "cowl": (-0.5, -1.0, 0.0)})),
        (0.35, make_pose(body=(0.0, 0.0, 0.0))),
    ], 0.03)
    clips.append(damage)

    # death: the brace folds, the frame drops forward and the lance goes nose-down into the ground
    death = skel.AnimationClip("death", 1.4, purpose="engineered collapse: the braced legs fold, the frame drops forward and the rail-lance drops nose-down toward the ground; the final pose is held (cosmetic debris <= 200 ticks)")

    def death_pose(f, pitch, bpitch, yaw, roll):
        # concept-v2: the legs fold DEEPER and the frame pitches less than concept-v1's 45/-92/47 at -26
        # deg. With the upper assembly carried back over the hips, a rigid 26 deg nose-down pitch levers
        # the pods and antennas UP to 211.6 cm - above the standing height - which does not read as a
        # collapse. 70/-135/65 at -20 deg drops the hips instead: the muzzle still lands 10.6 cm off the
        # ground and the whole frame ends below H.
        fold = (70.0 * f, -135.0 * f, 65.0 * f, 0.0)
        return make_pose(body=(pitch, 0.0, roll), r=fold, l=fold, extra={
            "lance_barrel": (bpitch, 0.0, 0.0), "lance_yaw": (0.0, yaw, 0.0),
            "spine": (-8.0 * f, 0.0, 0.0), "cowl": (-6.0 * f, 0.0, 0.0)})

    emit_keys(death, skeleton, [
        (0.00, death_pose(0.0, 0.0, 0.0, 0.0, 0.0)),
        (0.20, death_pose(0.15, -4.0, -1.5, 0.0, 0.0)),
        (0.50, death_pose(0.45, -10.0, -4.5, 2.0, 0.5)),
        (0.80, death_pose(0.80, -16.0, -8.0, 5.0, 1.5)),
        (1.10, death_pose(1.00, -20.0, -10.6, 7.0, 2.5)),
        (1.40, death_pose(1.00, -20.0, -10.6, 7.0, 2.5)),
    ], 0.05)
    clips.append(death)

    # cancel: the aim is abandoned - the lance returns to the carry angle and the strut retracts
    cancel = skel.AnimationClip("cancel", 0.4, purpose="order cancelled mid-aim: the lance returns to the carry angle, the strut slide retracts and the frame relaxes back onto the brace")
    emit_keys(cancel, skeleton, [
        (0.0, make_pose(body=(-4.0, 0.0, 0.0), strut_work=4.0, extra={"lance_yaw": (0.0, 8.0, 0.0), "lance_barrel": (3.0, 0.0, 0.0), "cowl": (2.0, 0.0, 0.0)})),
        (0.4, make_pose(body=(0.0, 0.0, 0.0), extra={"lance_yaw": (0.0, 0.0, 0.0), "lance_barrel": (0.0, 0.0, 0.0), "cowl": (0.0, 0.0, 0.0)})),
    ], 0.05)
    clips.append(cancel)

    # restore: single-frame rest pose for reconstruction from saved state
    restore = skel.AnimationClip("restore", 0.0, purpose="single-frame rest pose used when reconstructing presentation from saved state; never replays a one-shot")
    for bone in [b.name for b in skeleton.bones if b.name != "root"]:
        restore.key(bone, 0.0, (0.0, 0.0, 0.0))
    clips.append(restore)
    # Snap every authored duration onto a whole 30 fps frame. VERIFIED 2026-09-07 against UE 5.8.2
    # (evidence: BuildArtifacts/.../skeletal-clip-duration-probe/): the Interchange skeletal import
    # silently creates NO AnimSequence for a clip whose duration is a half frame, and still reports
    # success. retime_clip scales the key times, so the pose at any normalized time is unchanged.
    for _clip in clips:
        if not skel.is_frame_aligned(_clip.duration_s):
            skel.retime_clip(_clip, skel.frame_aligned_duration(_clip.duration_s))
    return clips


# --- measurement --------------------------------------------------------------------------------
def _boxes(m: kit.Mesh, component: str) -> int:
    """Number of six-sided boxes carrying one component name (boxes only; used for inventory counts)."""
    return sum(1 for p in m.polygons if p.component == component) // 6


def silhouette_band(m: kit.Mesh, z0: float, z1: float, step: float = 1.0) -> dict:
    """Fore-aft centroid and extent of the XZ silhouette between two heights, rasterised at 1 cm — the
    same quantity the concept panels are measured with (segment, scale to H, take the band's centroid),
    so the build can be held to the concept number instead of to a component's bounding box."""
    cells = set()
    for poly in m.polygons:
        pts = [(p[0], p[2]) for p in poly.points]
        zs = [q[1] for q in pts]
        lo, hi = int(math.floor(max(min(zs), z0))), int(math.ceil(min(max(zs), z1)))
        for zi in range(lo, hi + 1):
            zc = zi + 0.5
            if not (z0 <= zc <= z1):
                continue
            hits = []
            for i in range(len(pts)):
                a, b = pts[i], pts[(i + 1) % len(pts)]
                if (a[1] <= zc < b[1]) or (b[1] <= zc < a[1]):
                    hits.append(a[0] + (zc - a[1]) / (b[1] - a[1]) * (b[0] - a[0]))
            hits.sort()
            for i in range(0, len(hits) - 1, 2):
                xa, xb = hits[i], hits[i + 1]
                for xi in range(int(math.floor(xa)), int(math.ceil(xb)) + 1):
                    if xa <= xi + 0.5 <= xb:
                        cells.add((xi, zi))
    if not cells:
        return {"centroid_x_cm": 0.0, "x_min_cm": 0.0, "x_max_cm": 0.0, "cells": 0}
    return {"centroid_x_cm": sum(x + 0.5 for x, _z in cells) / len(cells),
            "x_min_cm": float(min(x for x, _z in cells)), "x_max_cm": float(max(x for x, _z in cells) + 1),
            "cells": len(cells)}


def slot_raster(m: kit.Mesh, axis: int, sign: float, step: float = 2.0) -> dict:
    """Nearest-surface material raster of one orthographic view: {(u, v) cell -> slot name}, where the
    view looks along ``axis`` from ``sign`` infinity. A painter pass keeps, per cell, the slot of the
    polygon closest to the camera, so this answers what a view actually SHOWS (occlusion included) —
    the same question the concept panels are read for, unlike a raw facing-area sum, which is identical
    front and rear for any closed mesh."""
    u_ax, v_ax = [i for i in (0, 1, 2) if i != axis]
    best = {}
    for poly in m.polygons:
        n = poly.normal
        if abs(n[axis]) < 1e-9:
            continue
        pts = [(p[u_ax], p[v_ax]) for p in poly.points]
        d = kit.v_dot(n, poly.points[0])
        vs = [q[1] for q in pts]
        for vi in range(int(math.floor(min(vs) / step)), int(math.ceil(max(vs) / step)) + 1):
            vc = (vi + 0.5) * step
            hits = []
            for i in range(len(pts)):
                a, b = pts[i], pts[(i + 1) % len(pts)]
                if (a[1] <= vc < b[1]) or (b[1] <= vc < a[1]):
                    hits.append(a[0] + (vc - a[1]) / (b[1] - a[1]) * (b[0] - a[0]))
            hits.sort()
            for i in range(0, len(hits) - 1, 2):
                ua, ub = hits[i], hits[i + 1]
                for ui in range(int(math.floor(ua / step)), int(math.ceil(ub / step)) + 1):
                    uc = (ui + 0.5) * step
                    if not (ua <= uc <= ub):
                        continue
                    p = [0.0, 0.0, 0.0]
                    p[u_ax], p[v_ax] = uc, vc
                    depth = (d - n[u_ax] * uc - n[v_ax] * vc) / n[axis]
                    key = (ui, vi)
                    prev = best.get(key)
                    if prev is None or depth * sign > prev[0]:
                        best[key] = (depth * sign, m.slots[poly.slot])
    return {k: v[1] for k, v in best.items()}


def ceramic_visible_fraction(m: kit.Mesh, axis: int, sign: float) -> float:
    """Share of the silhouette of one orthographic view whose nearest surface is the pale ceramic slot."""
    cells = slot_raster(m, axis, sign)
    return (sum(1 for s in cells.values() if s == CERAMIC) / len(cells)) if cells else 0.0


def cyan_side_profile_runs(m: kit.Mesh, step: float = 2.0) -> dict:
    """How the cyan lance channel reads from the side: the longest unbroken run of cyan cells along +X
    within the lance's own height band, and how many separate runs there are. concept-v1's 26 cm collars
    cut the 8 cm channel into three or four dashes; the concept keeps one stroke."""
    cells = slot_raster(m, 1, 1.0, step)
    band = m.component_bounds("lance_lance_channel")
    lo, hi = band[0][2] / step - 0.5, band[1][2] / step - 0.5
    columns = {}
    for (ui, vi), slot in cells.items():
        if lo - 0.5 <= vi <= hi + 0.5:
            columns[ui] = columns.get(ui, False) or (slot == STATUS_REVIEW)
    if not columns:
        return {"runs": 0, "longest_run_cm": 0.0, "channel_span_cm": 0.0}
    runs, run, longest = 0, 0, 0
    for ui in range(min(columns), max(columns) + 1):
        if columns.get(ui):
            run += 1
            longest = max(longest, run)
            if run == 1:
                runs += 1
        else:
            run = 0
    return {"runs": runs, "longest_run_cm": longest * step,
            "channel_span_cm": (max(columns) - min(columns) + 1) * step}


def concept_measurements(m: kit.Mesh) -> dict:
    """Proportions measured on the built rest stance, as ratios of H (concept-fidelity.md items 1-8)."""
    p = rest_positions()
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    pod_top = max(m.component_bounds(f"{s}_pod_{c}")[1][2] for s in ("r", "l") for c in POD_TOP_COMPONENTS)
    pod_span = (m.component_bounds("r_pod_pod_shell")[1][1] - m.component_bounds("l_pod_pod_shell")[0][1])
    lead_sole = m.component_bounds("r_foot_foot_sole")
    trail_sole = m.component_bounds("l_foot_foot_sole")
    lead_toe = m.component_bounds("r_toe_toe_block")
    trail_toe = m.component_bounds("l_toe_toe_block")
    muzzle = next(s for s in m.sockets if s.name == "Muzzle_Flash_01").position
    visor = m.component_bounds("cowl_cowl_visor")
    channel = m.component_bounds("lance_lance_channel")
    rail = m.component_bounds("lance_lance_rail")
    rail_dia_z = rail[1][2] - rail[0][2]
    rail_dia_y = max(rail[1][1] - rail[0][1], channel[1][1] - channel[0][1])
    lance_all = [m.component_bounds(c) for c in m.components() if c.startswith("lance_")]
    lance_x = (min(b[0][0] for b in lance_all), max(b[1][0] for b in lance_all))
    lance_z = (min(b[0][2] for b in lance_all), max(b[1][2] for b in lance_all))
    lance_y = (min(b[0][1] for b in lance_all), max(b[1][1] for b in lance_all))
    strut = m.component_bounds("strut_upper_strut_rod")
    core = m.component_bounds("body_core_block")
    clamp = m.component_bounds("lance_lance_clamp_01")
    head = m.component_bounds("lance_lance_muzzle_head")
    upper = silhouette_band(m, 0.80 * pod_top, 1.00 * pod_top)
    rear_ceramic, front_ceramic = ceramic_visible_fraction(m, 0, -1.0), ceramic_visible_fraction(m, 0, 1.0)
    cyan_runs = cyan_side_profile_runs(m)
    lead_foot_c = (lead_sole[0][0] + lead_toe[1][0]) / 2.0
    trail_foot_c = (trail_sole[0][0] + trail_toe[1][0]) / 2.0
    return {
        "H_pod_top_cm": pod_top, "overall_height_cm": z1, "antenna_tip_over_H": z1 / pod_top,
        "bounds_cm": [[x0, y0, z0], [x1, y1, z1]],
        "foot_centres_apart_cm": lead_foot_c - trail_foot_c, "foot_centres_apart_over_H": (lead_foot_c - trail_foot_c) / pod_top,
        "heel_to_toe_span_cm": lead_toe[1][0] - trail_sole[0][0], "heel_to_toe_span_over_H": (lead_toe[1][0] - trail_sole[0][0]) / pod_top,
        "feet_apart_across_Y_cm": 2.0 * HIP[1], "feet_apart_across_Y_over_H": 2.0 * HIP[1] / pod_top,
        "body_pitch_deg": BODY_PITCH,
        "lance_axis_z_cm": LANCE_Z, "lance_axis_over_H": LANCE_Z / pod_top,
        "lance_length_cm": lance_x[1] - lance_x[0], "lance_length_over_H": (lance_x[1] - lance_x[0]) / pod_top,
        "lance_rail_section_diameter_cm": max(rail_dia_z, rail_dia_y),
        "lance_rail_section_diameter_over_H": max(rail_dia_z, rail_dia_y) / pod_top,
        "lance_clamp_envelope_cm": [lance_z[1] - lance_z[0], lance_y[1] - lance_y[0]],
        # concept-v2: per-section depth against the candidate BRACED SIDE panel measured column by column
        # (clear shaft 0.095 H, thickest collar 0.110 H, muzzle end 0.0875-0.095 H)
        "lance_shaft_depth_over_H": (rail[1][2] - rail[0][2]) / pod_top,
        "lance_collar_depth_over_H": (clamp[1][2] - clamp[0][2]) / pod_top,
        "lance_muzzle_head_depth_over_H": (head[1][2] - head[0][2]) / pod_top,
        "lance_muzzle_head_length_cm": head[1][0] - head[0][0],
        "lance_max_depth_over_H": (lance_z[1] - lance_z[0]) / pod_top,
        "cyan_side_profile_runs": cyan_runs["runs"],
        "cyan_side_profile_longest_run_cm": cyan_runs["longest_run_cm"],
        "cyan_side_profile_span_cm": cyan_runs["channel_span_cm"],
        "muzzle_ahead_of_lead_toe_cm": muzzle[0] - lead_toe[1][0], "muzzle_ahead_of_lead_toe_over_H": (muzzle[0] - lead_toe[1][0]) / pod_top,
        "muzzle_ahead_of_lead_foot_centre_over_H": (muzzle[0] - lead_foot_c) / pod_top,
        "lance_on_centreline": abs((lance_y[0] + lance_y[1]) / 2.0) < 1e-6,
        "cyan_channel_full_length_fraction": (channel[1][0] - channel[0][0]) / (lance_x[1] - lance_x[0]),
        "collar_clamps": len([c for c in m.components() if c.startswith("lance_lance_clamp_") and c.endswith(("_01", "_02", "_03"))]),
        "lance_rails": _boxes(m, "lance_lance_rail"),
        "cowl_visor_z_cm": (visor[0][2] + visor[1][2]) / 2.0, "cowl_visor_over_H": ((visor[0][2] + visor[1][2]) / 2.0) / pod_top,
        "cowl_top_cm": m.component_bounds("cowl_cowl_upper")[1][2], "cowl_below_pod_top_cm": pod_top - m.component_bounds("cowl_cowl_upper")[1][2],
        "pod_centres_apart_cm": 2.0 * POD_L[1], "pod_centres_apart_over_H": 2.0 * POD_L[1] / pod_top,
        "pod_span_cm": pod_span, "pod_span_over_H": pod_span / pod_top,
        "pod_length_over_H": POD[0] / pod_top, "pod_height_over_H": POD[2] / pod_top,
        "core_width_cm": core[1][1] - core[0][1], "pods_are_widest_upper": pod_span > (core[1][1] - core[0][1]),
        "thigh_over_H": THIGH_LEN / pod_top, "shin_over_H": SHIN_LEN / pod_top,
        "knee_disc_diameter_over_H": 2.0 * KNEE_R / pod_top,
        "foot_length_cm": lead_toe[1][0] - lead_sole[0][0], "foot_length_over_H": (lead_toe[1][0] - lead_sole[0][0]) / pod_top,
        "lead_knee": list(p["r_knee"]), "trail_knee": list(p["l_knee"]),
        "lead_ankle": list(p["r_ankle"]), "trail_ankle": list(p["l_ankle"]),
        "mirror_brace": (p["r_knee"][0] - HIP[0]) > 0.0 > (p["l_knee"][0] - HIP[0]),
        "strut_top_cm": list(STRUT_TOP), "strut_end_cm": list(STRUT_END), "strut_length_cm": STRUT_LEN,
        "strut_radius_cm": STRUT_R, "strut_is_slim": 2.0 * STRUT_R < 0.5 * LEG_W,
        "strut_slide_fraction": STRUT_SPLIT, "strut_bounds_cm": [list(strut[0]), list(strut[1])],
        "strut_reaches_trailing_ankle_cm": _len(tuple(STRUT_END[i] - p["l_ankle"][i] for i in range(3))),
        # concept-v2: where the upper assembly sits fore-and-aft. Concept 0.80-1.00 H band centroid
        # -30.4 cm (turnaround LEFT SIDE) / -30.2 cm (candidate BRACED SIDE), front edge +16.6 / +13.4,
        # rear edge -75.4 / -72.6; both panels segmented, scaled to H = 200 cm and anchored on the
        # trailing heel and the leading toe (an anchoring the legs validate to within 5%).
        "upper_assembly_band_centroid_x_cm": upper["centroid_x_cm"],
        "upper_assembly_band_x_cm": [upper["x_min_cm"], upper["x_max_cm"]],
        "upper_assembly_band_over_H": upper["centroid_x_cm"] / pod_top,
        "spine_origin_x_cm": UPPER_ORIGIN[0],
        # concept-v2: the concept's rear is charcoal frame only (REAR panel at 2.2x: hub stacks, struts
        # and brass edges, no pale plate). Ceramic share of the aft-facing and fore-facing projected area.
        "ceramic_rear_facing_fraction": rear_ceramic, "ceramic_front_facing_fraction": front_ceramic,
        "ceramic_rear_over_front": (rear_ceramic / front_ceramic) if front_ceramic else 0.0,
        "emissive_area_fraction": emissive_area_fraction(m),
        "presentation_scale": PRESENTATION_SCALE,
        # walk authoring vs draw scale: the stride is matched to the authoritative 320 cm/s at AUTHORED
        # scale; the unit draws at x1.60, so at play rate 1.0 the soles would travel 512 cm/s in world
        # while the actor advances 320. The integration task must set the play rate, not discover it.
        "move_stride_cm": WALK_STRIDE, "move_cycle_s": 0.6,
        "move_authored_speed_cm_s": 2.0 * WALK_STRIDE / 0.6,
        "move_world_speed_at_draw_scale_cm_s": round(2.0 * WALK_STRIDE * PRESENTATION_SCALE / 0.6, 6),
        "integration_play_rate": 1.0 / PRESENTATION_SCALE,
        "strut_stow_retraction_cm": STOW_RETRACT,
        "lead_side": "right (+Y) leads; left (-Y) trails and carries the recoil strut",
        "no_ceramic_skirt": not any(c.startswith("body_skirt") for c in m.components()),
    }


def whole_unit_triangles(outputs) -> dict:
    """Triangles for the WHOLE unit, not per asset.

    The two standalone sub-object parts are alternates, not additions: the lance yoke and barrel are
    already skinned into the skeletal mesh on the ``lance_yaw`` / ``lance_barrel`` bones, so an
    integration that instantiates SK plus both SMs draws that geometry twice. Both figures are recorded
    so a reviewer can check either route against REL-ART-028 without re-deriving the sum.
    """
    tri = {}
    for o in outputs:
        if not isinstance(o, dict) or "triangles" not in o:
            continue
        if o.get("kind", "").startswith("static rest pose"):
            continue                                     # the rest-pose twin of the skinned mesh
        tri[(o["mesh"], int(o["lod"]))] = int(o["triangles"])
    out = {}
    for lod in (0, 1):
        sk_only = tri[(ASSET, lod)]
        subs = sum(v for (mesh, l), v in tri.items() if l == lod and mesh != ASSET)
        out[f"lod{lod}_skeletal_only"] = sk_only
        out[f"lod{lod}_with_both_sub_objects"] = sk_only + subs
    out["note"] = ("The standalone SM_..._LanceYoke and SM_..._LanceBarrel parts duplicate the lance "
                   "geometry already skinned into SK_EBS_MER_UNT_002 (bones lance_yaw and lance_barrel, "
                   "96 + 244 tris at LOD0, 56 + 168 at LOD1). They are ALTERNATES for a static sub-object "
                   "integration route, not additions to the skeletal mesh; instantiating all three "
                   "double-counts the lance. Both routes are inside REL-ART-028's 8,000 / 3,500.")
    return out


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = set(m.components())
    return {
        "legs": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_thigh_thigh_strut"))},
        "feet": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_foot_foot_sole"))},
        "toe_plates": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_toe_toe_plate"))},
        "knee_hubs": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_shin_knee_hub"))},
        "rail_lance": {"contract": 1, "built": 1 if "lance_lance_body" in comps else 0,
                       "rails": _boxes(m, "lance_lance_rail"), "channel": 1 if "lance_lance_channel" in comps else 0,
                       "clamps": len([c for c in comps if c.startswith("lance_lance_clamp_") and c.endswith(("_01", "_02", "_03"))]),
                       "muzzle_head": 1 if "lance_lance_muzzle_head" in comps else 0, "breech": 1 if "lance_lance_breech" in comps else 0},
        "hip_mount": {"contract": 1, "built": 1 if "yaw_yaw_ring" in comps else 0},
        "shoulder_pods": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_pod_pod_shell"))},
        "antennas": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_pod_pod_antenna"))},
        "sensor_cowl": {"contract": 1, "built": 1 if "cowl_cowl_lower" in comps else 0, "visor": 1 if "cowl_cowl_visor" in comps else 0},
        "exposed_flank_struts": {"contract": 4, "built": _boxes(m, "body_flank_strut_rear") + _boxes(m, "body_flank_strut_front")},
        "recoil_strut": {"contract": 1, "built": 1 if "strut_upper_strut_rod" in comps else 0,
                         "slide_section": 1 if "strut_slide_strut_piston" in comps else 0,
                         "source_record_name": "recoil_brace",
                         "name_note": "Docs/VisualAssetPipeline/motion/gap-decisions.json names this component 'recoil_brace' (count 1); the canon row SPEC-UNIT-002, the requirement card and this package call it the recoil strut, and the owner ruling of 2026-09-07 names its socket Rear_Recoil_Strut_Anchor. Same single component, two names for it in the records; the ruling's spelling wins here and the difference is recorded rather than silently reconciled (README section 8.3)."},
        "ceramic_skirt": {"contract": 0, "built": 0, "note": "concept item 5: the flanks stay open charcoal frame"},
        "bones": {"contract": 18, "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"card_contract": ["Muzzle_Flash_01", "Target_Anchor_Center", "Left_Tread_Vector"],
                    "contract": sorted(SOCKETS_ON_BONES),
                    "built": sorted(sk.name for sk in m.sockets),
                    "alias": {"Left_Tread_Vector": "Rear_Recoil_Strut_Anchor",
                              "status": "TEMPORARY COMPATIBILITY ALIAS, to be dropped once the adapter migration is tested"},
                    "ruling": "owner ruling 2026-09-07: Rear_Recoil_Strut_Anchor is the named socket; Left_Tread_Vector is retained temporarily as a compatibility alias at the same transform; left-foot ground contact is a different function and is carried by Foot_Contact_L / Foot_Contact_R"},
        "sub_objects": {"contract": ["Turret_Y", "Barrel_X"], "built": {"Turret_Y": "lance_yaw", "Barrel_X": "lance_barrel"}},
        "tracks": {"contract": ["idle", "move", "turn", "stop", "fire", "damage", "death", "cancel", "restore"],
                   "built": [c.name for c in clips],
                   "source_contract": {
                       "record": "Docs/VisualAssetPipeline/motion/gap-decisions.json -> production_policy[EBS-PKG-MC-LANCER].required_track_inventory",
                       "tracks": ["idle", "move", "turn", "stop", "damage", "death", "cancel", "restore",
                                  "attack_anticipation", "attack_execution", "attack_recovery"],
                       "deviation": "The frozen record asks for eleven tracks, splitting the attack into attack_anticipation / attack_execution / attack_recovery. This package builds ONE 2.2 s clip named 'fire' that contains all three phases as keyed sections (halt 0.00-0.35 s, plant 0.35-0.75 s, aim 0.75-0.95 s = anticipation; shot at 1.00 s = execution; recover 1.00-1.45 s and settle to 2.20 s = recovery), because the canon row SPEC-UNIT-002 describes one continuous halt-plant-aim-fire-recover action and splitting it into three AnimSequences would put the plant and the recovery blend under runtime control this blockout cannot specify. The three separately named clips are therefore STILL OWED, not delivered: an integration that expects attack_anticipation / attack_execution / attack_recovery by name will not find them. Recorded as a deviation in README section 8.3; the split is an ART_ALPHA / integration decision.",
                       "status": "DEVIATION_RECORDED - three named attack clips still owed"}},
    }


# --- review scenes --------------------------------------------------------------------------------
SCENE_BASE = {
    "author": AUTHOR, "srgb": True,
    "materials": {CERAMIC: [0.72, 0.70, 0.66], FRAME: [0.05, 0.05, 0.055], STATUS_REVIEW: [0.16, 0.86, 0.96],
                  "MI_EBS_MER_CeramicCivic": [0.72, 0.70, 0.66], "MI_EBS_MER_CompactFrame": [0.045, 0.045, 0.05],
                  "_default": [0.5, 0.5, 0.5]},
    "emissive": [STATUS_REVIEW],
    "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
    "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.3, "key": 0.8, "color": [1.0, 0.82, 0.62],
              "fill_color": [0.48, 0.6, 0.88], "fill": 0.22},
    "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
               "footprint_cm": [200, 200], "footprint_color": [0.16, 0.86, 0.96]},
    "reference_figure": {"height_cm": 180, "position": [-200, 200, 0], "color": [0.92, 0.55, 0.2]},
}
ORTHO_TARGET = [50, 0, 105]
ORTHO_VIEWS = [
    {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15, "target": ORTHO_TARGET},
    {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": ORTHO_TARGET},
    {"name": "rear", "type": "ortho", "from": "-X", "edges": True, "margin": 1.15, "target": ORTHO_TARGET},
    {"name": "left", "type": "ortho", "from": "-Y", "edges": True, "margin": 1.12, "target": ORTHO_TARGET},
    {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [50, 0, 0]},
]
# tactical view blocks copied from EBS-MER-UNT-001/scenes (the game's orthographic RTS framing)
TACTICAL_VIEWS = [
    {"name": "tactical_default", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
    {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
    {"name": "tactical_mono", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0], "grayscale": True},
    {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
    {"name": "tactical_front_quarter", "type": "persp", "pitch_deg": -48, "yaw_deg": 135, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
    {"name": "tactical_mono_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60], "grayscale": True},
]


def scene(meshes, views, **overrides) -> dict:
    doc = json.loads(json.dumps(SCENE_BASE))
    doc.update(overrides)
    doc["meshes"] = meshes
    doc["views"] = views
    return doc


def write_scenes(scene_dir: str) -> list:
    """Base review scenes: rest orthographic, rest at the tactical framing, LOD1, and a context scene."""
    os.makedirs(scene_dir, exist_ok=True)
    rest_obj = f"../review/{ASSET}_rest_LOD0.obj"
    surveyor = "../../EBS-MER-UNT-001/review/SK_EBS_MER_UNT_001_rest_LOD0.obj"
    link = "../../EBS-MER-BLD-002/review/SM_EBS_MER_BLD_002_assembly_connected_LOD0.obj"
    docs = {
        "rest": scene([{"obj": rest_obj}], ORTHO_VIEWS),
        "rest_tactical": scene([{"obj": rest_obj, "scale": PRESENTATION_SCALE}], TACTICAL_VIEWS),
        "lod1": scene([{"obj": f"../review/{ASSET}_rest_LOD1.obj"}], ORTHO_VIEWS[:2] + [ORTHO_VIEWS[4]]),
        "scale_vs_surveyor": scene(
            [{"obj": rest_obj, "translate": [0, 0, 0]}, {"obj": surveyor, "translate": [-300, 300, 0]}],
            [{"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.08, "target": [-150, 150, 105]},
             {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.08, "target": [-150, 150, 105]}]),
        "context": scene(
            [{"obj": link, "translate": [500, 700, 0], "yaw_deg": 180},
             {"obj": surveyor, "translate": [420, 240, 0], "yaw_deg": 150, "scale": 1.5},
             {"obj": rest_obj, "translate": [0, 0, 0], "yaw_deg": 0, "scale": PRESENTATION_SCALE},
             {"obj": rest_obj, "translate": [-160, -300, 0], "yaw_deg": 8, "scale": PRESENTATION_SCALE},
             {"obj": rest_obj, "translate": [-320, 300, 0], "yaw_deg": -10, "scale": PRESENTATION_SCALE},
             {"obj": rest_obj, "translate": [-520, -60, 0], "yaw_deg": 4, "scale": PRESENTATION_SCALE}],
            TACTICAL_VIEWS[:4], ground={"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032],
                                        "grid_color": [0.07, 0.07, 0.08], "footprint_cm": [400, 400],
                                        "footprint_color": [0.16, 0.86, 0.96]}),
    }
    written = []
    for name, doc in docs.items():
        path = os.path.join(scene_dir, f"{name}.json")
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            json.dump(doc, handle, indent=1)
        written.append(path)
    return written


# --- outputs, manifest and CLI ----------------------------------------------------------------------
def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def build_outputs(export_dir: str, review_dir: str) -> tuple:
    """Write every export into export_dir and every review OBJ into review_dir; return
    (outputs, review, meshes, skeleton, clips). Output paths are recorded relative to the package
    so --check can compare them from a temporary directory."""
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)
    outputs, review, meshes = [], [], {}
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, counts, sockets_on_bones = assemble(lod)
        meshes[lod] = (m, s, counts, sockets_on_bones)
        ex = export_mesh(m)
        stem = f"{ASSET}_LOD{lod}"
        base = os.path.join(export_dir, stem)
        skinned = skel.write_skinned_glb(ex, s, base + ".glb", animations=clips, include_collision=False,
                                         sockets_on_bones=sockets_on_bones,
                                         extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod})
        outputs.append({"path": f"export/{stem}.glb", "sha256": skinned, "lod": lod, "mesh": ASSET, "kind": "skinned + clips",
                        "triangles": ex.triangle_count(), "by_slot": ex.triangle_count_by("slot"), "by_bone": counts,
                        "bounds_cm": ex.bounds(), "clips": [c.name for c in clips],
                        "sockets": [{"name": sk.name, "position_cm": sk.position, "yaw_deg": sk.yaw_deg, "bone": sockets_on_bones.get(sk.name)} for sk in ex.sockets]})
        static = ex.write_glb(base + "_static.glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION,
                                                            "lod": lod, "note": "rest-pose static export; the skinned export carries the rig"}, include_collision=False)
        outputs.append({"path": f"export/{stem}_static.glb", "sha256": static, "lod": lod, "mesh": ASSET, "kind": "static rest pose", "triangles": ex.triangle_count()})
        obj = ex.write_obj(base + ".obj", header_lines=[f"Production ID {PRODUCTION_ID}", f"Revision {REVISION}", f"LOD{lod} rest stance"])
        outputs.append({"path": f"export/{stem}.obj", "sha256": obj, "lod": lod, "mesh": ASSET, "kind": "rest-pose OBJ"})
        # card sub-object separation: Turret_Y (lance_yaw) and Barrel_X (lance_barrel) as their own parts
        for builder, name, sub in ((part_lance_yoke, YOKE_ASSET, "Turret_Y"), (part_lance_barrel, BARREL_ASSET, "Barrel_X")):
            part = export_mesh(builder(lod))
            pstem = f"{name}_LOD{lod}"
            ppath = os.path.join(export_dir, pstem)
            outputs.append({"path": f"export/{pstem}.glb", "lod": lod, "mesh": name, "kind": f"sub-object part ({sub})",
                            "sha256": part.write_glb(ppath + ".glb", extras={"production_id": PRODUCTION_ID, "revision": REVISION, "lod": lod, "sub_object": sub}, include_collision=False),
                            "triangles": part.triangle_count(), "bounds_cm": part.bounds(),
                            "pivot_cm": list(YAW_PIVOT if sub == "Turret_Y" else TRUNNION)})
            outputs.append({"path": f"export/{pstem}.obj", "lod": lod, "mesh": name, "kind": f"sub-object part OBJ ({sub})",
                            "sha256": part.write_obj(ppath + ".obj", header_lines=[f"{name} LOD{lod}; pivot at the {sub} axis"])})
        rev = os.path.join(review_dir, f"{ASSET}_rest_LOD{lod}.obj")
        review.append({"path": rev, "sha256": m.write_obj(rev, header_lines=[f"Rest stance LOD{lod} (braced firing stance); review only"]),
                       "lod": lod, "triangles": m.triangle_count(), "bounds_cm": m.bounds()})
    return outputs, review, meshes, skeleton, clips


def build_manifest(outputs, review, meshes, skeleton, clips) -> dict:
    m0, _s0, counts0, sockets_on_bones = meshes[0]
    m1 = meshes[1][0]
    measurements = concept_measurements(m0)
    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    lod0, lod1 = m0.triangle_count(), m1.triangle_count()
    return {
        "author": AUTHOR, "creator": AUTHOR, "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "asset_name": ASSET,
        "parts": [ASSET, YOKE_ASSET, BARREL_ASSET],
        "revision": REVISION, "kit_revision": kit.KIT_REVISION, "skel_revision": skel.SKEL_REVISION, "stage": f"BLOCKOUT (concept-{REVISION.rsplit('-', 1)[-1]})",
        "stage_boundary": "Concept-matched braced rest geometry, an 18-bone rig and keyframed clip data; skinned export on the verified skeletal kit; no textures, no Unreal import, no gate or owner acceptance.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters at unit scale 1.0", "axes": "+X forward (lance axis), +Y right, +Z up",
                  "pivot": "rig origin (root) on the ground plane at z = 0 on the centreline. The brace is asymmetric fore-and-aft, so the origin is not exactly on the sole-contact midpoint: it sits 2.0 cm ahead of the midpoint of Foot_Contact_L (x -66) and Foot_Contact_R (x +62), and 6.0 cm ahead of the ankle midpoint (x -70 / +58). Asserted by test_rest_stance_is_grounded_and_forward_facing",
                  "pivot_offset_cm": {"ahead_of_sole_contact_midpoint_x": 2.0, "ahead_of_ankle_midpoint_x": 6.0},
                  "nanite": False,
                  "runtime_presentation_scale": PRESENTATION_SCALE,
                  "presentation_scale_source": "EchoesEntityView.cpp:1809-1812 EntityType::Soldier (mc_lancer maps to Soldier in EchoesContentSubsystem.cpp:309-310); not part of the asset"},
        "scale_basis": {
            "concept": "concept-fidelity.md; H = 200 cm at the pod top measured on lancer-derived-turnaround.png LEFT SIDE panel (0.545 cm per source pixel, ground at the sole line)",
            "canon": "SPEC-UNIT-002 (Bible line 510): a two-legged line-fire frame, slightly taller than the Surveyor and narrow",
            "surveyor_height_cm": 176.0, "taller_than_surveyor": measurements["H_pod_top_cm"] > 176.0,
            "height_cm": measurements["H_pod_top_cm"], "overall_height_with_antennas_cm": measurements["overall_height_cm"],
            "status": f"CONCEPT-MEASURED BLOCKOUT (concept-{REVISION.rsplit('-', 1)[-1]})",
            "footprint_note": "SPEC-UNIT-002 Logistics Footprint 2 is the population cost (units.json population_cost 2). The simulation collision footprint of a unit is a 25 cm square (kFixedScale/8 half extent on a 100 cm tile, EchoesContentSubsystem.cpp:363). The visual envelope overhangs it heavily because the lance is 1.05 H long; recorded as a deviation in README section 8.",
        },
        "concept_measurements": measurements,
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": b.head, "purpose": b.purpose} for b in skeleton.bones],
                "rest_stance": rest_positions(),
                "card_sub_objects": {"Turret_Y": "lance_yaw", "Barrel_X": "lance_barrel"},
                "policy": "identity rest orientation; hinge pitch about +Y at each joint; the recoil runs as a translation of lance_barrel along its own +X axis and the strut slide translates along the strut axis; no decorative bones; no root motion"},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "loop": c.loop, "keys": sum(len(v) for v in c.tracks.values()),
                   "bones": sorted(c.tracks), "purpose": c.purpose} for c in clips],
        "budgets": {"lod0_triangles": lod0, "lod1_triangles": lod1, "lod0_cap": LOD0_CAP, "lod1_cap": LOD1_CAP,
                    "lod0_within_cap": lod0 <= LOD0_CAP, "lod1_within_cap": lod1 <= LOD1_CAP,
                    "lod1_cap_note": "CONFIRMED 3,500. Owner ruling 2026-09-07: \"Confirm 3,500 triangles for Lancer LOD1. '3,3500' is malformed. Both REL-ART-028 and our recorded production decision specify 8,000 LOD0 / 3,500 LOD1.\" REL-ART-028 (Docs/Requirements.md:2377 in this worktree; :2379 in Project/Docs/Requirements.md) states the roster rule verbatim: \"a maximum LOD0 cap of <=8,000 triangles, transitioning smoothly down to <=3,500 triangles\". REL-ART-005.MC.LANCER prints the ceiling as \"3,3500\", a Requirements.md typo recorded for correction (README section 8.5). The open question is ANSWERED; no geometry changed for it - the built LOD1 is far under the bound",
                    "whole_unit_triangles": whole_unit_triangles(outputs),
                    "emissive_area_fraction_lod0": measurements["emissive_area_fraction"], "emissive_cap": EMISSIVE_CAP,
                    "emissive_within_cap": measurements["emissive_area_fraction"] <= EMISSIVE_CAP},
        "material_slots": [FRAME, CERAMIC],
        "material_slot_policy": "2 export slots; the cyan lance channel, visor, pod strips and status strips plus the brass trim are texture channels of the ceramic slot. The review OBJ keeps a third pseudo slot so the renders show the cyan and the emissive share can be measured as a geometry proxy.",
        "team_colour_mask": {
            "card_rule": "REL-ART-005.MC.LANCER .MAT_RULE (Docs/Requirements.md:2385): \"Albedo channel masked by TeamColor vertex data.\"",
            "roster_rule": "REL-ART-028 (Docs/Requirements.md:2377): \"Team color accent mapping uses exclusive vertex ID masks.\"",
            "built": "NOT AUTHORED. The exported primitives carry POSITION, NORMAL, TEXCOORD_0, TEXCOORD_1, JOINTS_0 and WEIGHTS_0 only; TEXCOORD_1 is the mesh kit's per-polygon lightmap grid (ebs_meshkit.py:359-377), not a mask. There is no COLOR_0 and no vertex-ID channel.",
            "why": "The verified skeletal kit (ArtSource/tools/ebs_skelkit.py, whose SKELETAL_ENCODING is verified against UE 5.8.2) writes no vertex-colour accessor and the shared tools are read-only for this package, so the mask cannot be authored here without changing a verified tool. Recorded as a deviation (README section 8.3) and raised as OWNER-QUESTION 3; it is roster-wide, not Lancer-only (EBS-MER-UNT-001 exports the same attribute set).",
            "status": "DEVIATION_RECORDED"},
        "component_inventory": contract_inventory(m0, skeleton, clips),
        "triangles_by_bone_lod0": counts0,
        "sockets": [{"name": s.name, "bone": sockets_on_bones[s.name], "position_cm": s.position, "yaw_deg": s.yaw_deg, "purpose": s.purpose} for s in m0.sockets],
        "socket_policy": {
            "ruling": "Owner ruling 2026-09-07: \"Correct the socket interpretation. Our recorded Lancer decision specifies Rear_Recoil_Strut_Anchor, with Left_Tread_Vector retained temporarily as a compatibility alias. Left-foot ground contact is a different function; it should have a separately named foot-contact socket if needed. Preserve the two-legged concept.\"",
            "card_names": ["Muzzle_Flash_01", "Target_Anchor_Center", "Left_Tread_Vector"],
            "anchor_end": "UPPER. The strut runs from the tail beam at (-46, -20, 112) down and back to (-80, -34, 30) over the trailing heel. Recoil enters the frame at the upper end (the strut_anchor clevis on the tail beam) and is delivered to the ground at the lower end through the trailing foot, so the socket is at the upper end.",
            "alias": {"name": "Left_Tread_Vector", "aliases": "Rear_Recoil_Strut_Anchor",
                      "status": "TEMPORARY COMPATIBILITY ALIAS - drop once the adapter migration is tested",
                      "same_transform": True},
            "foot_contact": {"names": ["Foot_Contact_L", "Foot_Contact_R"],
                             "function": "sole ground contact (footfall effects, dust, planted-sole reference) - a DIFFERENT function from the recoil anchor, and no longer carried by the alias",
                             "placement": "heel-to-toe midpoint of each block foot on z = 0"},
            "two_legged": "preserved: no tread geometry, no change to the braced stance (component inventory unchanged from concept-v3)"},
        "outputs": outputs, "review": review,
        "tools": {"builder_sha256": builder_sha, "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "fidelity_target": {"path": "ArtSource/EBS-MER-UNT-002/concept-fidelity.md",
                                "amended": "concept-v1: brace span, pod separation and the thigh/shin split corrected on the LEFT SIDE panel pixels; antennas added. concept-v2: fore-aft placement of the upper assembly, per-section lance depth and cyan continuity, ceramic on outer faces only, the strut as a per-clip constraint. concept-v4: the socket list rewritten to the owner ruling of 2026-09-07 (Rear_Recoil_Strut_Anchor, Left_Tread_Vector as a temporary alias, Foot_Contact_L/R) and the LOD1 ceiling recorded as CONFIRMED. concept-v4 record pass 2026-09-07: the REAR-panel cross-check for the strut anchor withdrawn (it was scaled from the central cowl block, not the pod caps; re-scaled it reads 119.4 cm and disagrees with the LEFT SIDE panel by 5.5 cm), and the eleven-track gap-decisions.json inventory recorded against the built nine"},
            "selected_candidate": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/lancer-review/lancer-candidate.png",
                                   "status": "SELECTED CANDIDATE - the production direction"},
            "derived_turnaround": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/lancer-review/lancer-derived-turnaround.png",
                                   "status": "DERIVED REFERENCE; FRONT and REAR panels are quarter views (the lance leaves the centreline), so only the LEFT SIDE panel was measured"},
            "superseded_concept": {"concept_id": "EBS-CON-MER-UNT-002", "decision": "REPLACE", "use": "RETAINED_HISTORY_ONLY",
                                   "sha256_prefix": "427e60cd27bd78e9", "path": "Project/site/assets/concepts/meridian-units.png", "region": [0.5, 0, 1, 0.5]},
            "canon_row": {"path": "Docs/Archive/DevelopmentBible.md", "line": 510, "spec": "SPEC-UNIT-002"},
            "book": {"paragraphs": [178, 181, 1966, 3078]},
            "gameplay_record": {"path": "Content/Data/Source/units.json", "id": "mc_lancer",
                                "values": "145 HP, 320 cm/s, 1,100 cm sight, population cost 2, 18 damage at 650 cm, 30-tick cooldown"},
            "contract": {"path": "Docs/VisualAssetPipeline/reference-packages.json", "package_id": PACKAGE_ID},
            "review_decision": {"path": "Docs/VisualAssetPipeline/review-selections.json", "concept_id": "EBS-CON-MER-UNT-002", "choice": "REPLACE"},
            "requirement_card": {"path": "Docs/Requirements.md", "card": "REL-ART-005.MC.LANCER", "line": 2382},
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }


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
        import pose_review as pr                     # lazy: pose_review imports this module
        with tempfile.TemporaryDirectory(prefix="ebs-lancer-check-") as tmp:
            outputs, review, _meshes, _sk, _clips = build_outputs(os.path.join(tmp, "export"), os.path.join(tmp, "review"))
            posed = pr.bake(tmp)
            scenes = {os.path.basename(p): open(p, "rb").read()
                      for p in write_scenes(os.path.join(tmp, "scenes"))
                      + [s for e in posed for s in (e["scene"], e.get("scene_tactical")) if s]}
        prev_out = {o["path"]: o["sha256"] for o in previous.get("outputs", [])}
        prev_rev = {os.path.basename(r["path"]): r["sha256"] for r in previous.get("review", [])}
        drift = [o["path"] for o in outputs if prev_out.get(o["path"]) != o["sha256"]]
        drift += [os.path.basename(r["path"]) for r in review if prev_rev.get(os.path.basename(r["path"])) != r["sha256"]]
        missing = [p for p in prev_out if not os.path.exists(os.path.join(HERE, p))]
        missing += [p for p in prev_rev if not os.path.exists(os.path.join(args.evidence_dir, "review", p))]
        # the posed bake and every render scene: compare the rebuilt bytes with what is on disk, so the
        # 17 posed OBJs and the 27 scenes are inside the gate too (pose-manifest.json is excluded: it
        # stores absolute paths, which are a property of the run, not of the geometry)
        compared_posed, compared_scenes = 0, 0
        for entry in posed:
            live = os.path.join(args.evidence_dir, "review", os.path.basename(entry["path"]))
            compared_posed += 1
            if not os.path.exists(live):
                missing.append(os.path.relpath(live, args.evidence_dir))
            elif sha256_bytes(open(live, "rb").read()) != entry["sha256"]:
                drift.append(os.path.relpath(live, args.evidence_dir))
        for name, data in sorted(scenes.items()):
            live = os.path.join(args.evidence_dir, "scenes", name)
            compared_scenes += 1
            if not os.path.exists(live):
                missing.append(f"scenes/{name}")
            elif sha256_bytes(open(live, "rb").read()) != sha256_bytes(data):
                drift.append(f"scenes/{name}")
        ok = not drift and not missing and previous.get("revision") == REVISION
        print(json.dumps({"check": "ok" if ok else "drift", "revision": REVISION, "manifest_revision": previous.get("revision"),
                          "drift": drift, "missing": missing,
                          "compared": {"outputs": len(outputs), "review": len(review),
                                       "posed_review": compared_posed, "scenes": compared_scenes}}))
        return 0 if ok else 3
    outputs, review, meshes, skeleton, clips = build_outputs(os.path.join(HERE, "export"), os.path.join(args.evidence_dir, "review"))
    scenes = write_scenes(os.path.join(args.evidence_dir, "scenes"))
    manifest = build_manifest(outputs, review, meshes, skeleton, clips)
    manifest["review_scenes"] = [os.path.basename(p) for p in scenes]
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(manifest, indent=1, sort_keys=True) + "\n")
    measurements = manifest["concept_measurements"]
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"],
                      "inventory": {k: v.get("built") for k, v in manifest["component_inventory"].items() if isinstance(v, dict)},
                      "outputs": len(outputs), "scenes": len(scenes),
                      "measurements": {k: (round(v, 3) if isinstance(v, float) else v) for k, v in measurements.items()
                                       if not isinstance(v, (list, dict))}}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
