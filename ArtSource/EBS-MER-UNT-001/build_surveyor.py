#!/usr/bin/env python3
"""EBS-MER-UNT-001 Surveyor: deterministic editable source geometry, 12-bone rig and clips (concept-v3 blockout).

Author: Angelis Pseftis.

Design authority (owner ruling 2026-09-06): the selected concept images define what the asset
looks like; gameplay rules bound the concept but never replace it. Target record:
  concept-fidelity.md (this folder). Sources looked at, not described:
  * EBS-CON-MER-UNT-001 meridian-units.png top-left quadrant (KEEP, design identity):
    knuckle-walker; dense torso block on top pitched forward; both arms hang forward-down from
    large shoulder-hub discs on the torso sides and reach the ground ahead of the feet; two thick
    plated legs at the rear standing as near-vertical columns (knee hub at mid height, block feet
    centred under the ankles); three tall canisters with cyan window strips in an open rack HUNG
    ON THE REAR at shoulder height, offset to the anatomical RIGHT, their caps just above the
    torso top; camera mast at the REAR-LEFT top (holographic panel simplified away per the
    review); one large cyan lens on the front face's upper-LEFT third with the status band below.
  * surveyor-reference.png (derived reference: front / rear / GATHERING / DELIVERY panels) for
    the working poses. Its rear view mirrors the cradle and mast sides relative to the concept
    and its front view puts the lens on the other side; the concept wins (README section 8).
  concept-v3 (2026-09-06 review of concept-v2 against the pixels): cradle lowered and hung on
  the rear, legs re-authored as vertical columns, forearm steeper so the drill tip lands inside
  the 60-90 cm / 5-15 cm band, feet 0.45 H apart, gather/deliver/death re-keyed to the panels.

Package contract: Docs/VisualAssetPipeline/motion/gap-decisions.json
  production_policy[package_id == "EBS-PKG-MC-SURVEYOR"], gap GAP-01 (three rear canisters in
  one transverse cradle; anatomical right arm drill, left gripper with recessed palm welder; two
  planted feet; tools stop on travel/cancel; delivery empties cargo only on an authorized
  transfer). Prepared amendment MERIDIAN-ANATOMY: two articulated legs and planted feet.
Canon row: Docs/Archive/DevelopmentBible.md line 509 (SPEC-UNIT-001).
Requirement card: REL-FAC-025.MC.SURVEYOR.ASSET: LOD0 <= 4,500 / LOD1 <= 1,800 tris, 2048^2 maps,
  team colour through a mask, emissive <= 5%, 12-bone kinematic rig, sockets
  Harvest_Tether_Muzzle / Cargo_Drop_Anchor / Center_Hitbox_Socket.

Rig (12 bones, identity rest orientation, heads in the knuckle-walker rest stance):
  root, body, r_thigh, r_shin, r_foot, l_thigh, l_shin, l_foot, r_shoulder, r_forearm,
  l_shoulder, l_forearm. The optical mast and cradle are rigid with the body; the drill spin is a
  material effect (no thirteenth bone). Canisters are separate components (body_canister_01..03)
  so the runtime can hide them on the authorized transfer.

Conventions: cm, +X forward, +Y right (anatomical right), +Z up, root at the ground-contact
centre, no root motion; workers draw at PresentationScale 1.5 at runtime (not part of the asset).
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
import ebs_skelkit as skel  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-MC-SURVEYOR"
PRODUCTION_ID = "EBS-MER-UNT-001"
ASSET = "SK_EBS_MER_UNT_001"
REVISION = "ebs-mer-unt-001-concept-v3"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_001/"

CERAMIC = "MI_EBS_MER_UnitCeramic"    # pale ceramic shell; brass trim, status band, windows and team mask are texture channels
FRAME = "MI_EBS_MER_UnitFrame"        # charcoal frame, hubs, tools
STATUS_REVIEW = "MI_EBS_MER_StatusCyan"  # review-only pseudo slot: folded into CERAMIC for the export (2-slot contract)

# --- concept measurements (concept-fidelity.md; H = 176 cm at the mast top, authored scale 1.0) -----
H = 176.0
TORSO = (60.0, 66.0, 52.0)              # depth x (0.34H), width y (0.38H), height z (0.30H)
TORSO_C = (0.0, 0.0, 116.0)             # torso centre before the pitch
TORSO_PITCH = -10.0                     # forward pitch of the torso block (Unreal pitch: negative = nose down)
HIP = (-14.0, 28.0, 96.0)               # hip joint, right side (rear half of the torso); body head at y = 0
THIGH_D = (-4.0, 12.0, -42.0)           # thigh delta hip -> knee: a near-vertical column leaning 5 deg back and splayed out (0.24H)
SHIN_D = (8.0, 0.0, -41.0)              # shin delta knee -> ankle: near-vertical, 11 deg forward-down (0.24H); ankle 4 cm ahead of the hip
ANKLE_Z = 13.0
FOOT_LEN = 35.0                         # 0.20H block foot centred on the ankle: toe plate ahead longer than the heel block behind
LEG_W = 12.0
KNEE_R = 8.0                            # knee disc radius (0.09H diameter)
SHOULDER = (14.0, 46.0, 112.0)          # shoulder joint at the outer face of the shoulder hub disc (hub 15 cm deep, 0.09H)
SHOULDER_HUB_R = 12.0                   # concept: large disc on the torso side (0.14H diameter)
UPPER_D = (10.0, 28.0, -43.0)           # upper arm delta shoulder -> elbow: down and outward (0.30H); concept-v2 splay for the 1.15-1.30H arm span
FORE_D = (22.0, 16.0, -33.0)            # forearm delta elbow -> wrist: 50 deg below horizontal (0.24H); tip 79 cm ahead of the hip line, 7.5 cm up
ARM_W = 10.0
DRILL_HOUSING, DRILL_LEN = 12.0, 25.0   # housing then the tapered bit (0.14H) beyond the wrist
GRIPPER_LEN = 22.0                      # two long fingers plus a thumb nub
MAST_BASE = (-20.0, -20.0, 144.0)       # rear-LEFT top of the torso
MAST_H, MAST_HEAD = 22.0, (12.0, 10.0, 10.0)  # post 0.125H, boxy camera head 0.057H; cap top at H
CRADLE_C = (-46.0, 7.0)                 # tray centre (x, y): hung behind the rear face, offset to the anatomical RIGHT; canisters in a row across Y
CRADLE_FLOOR_Z = 124.0                  # tray floor top surface: shoulder-hub height, torso mid-height (concept: tray level with the shoulder hub)
CANISTER_R, CANISTER_H, CANISTER_PITCH = 8.8, 28.0, 18.0   # 0.10H diameter, 0.16H tall; caps 7 cm above the torso's rear top edge
CRADLE_HOOK_Z = 148.0                   # hook bars over the torso's rear top edge (z 146-150)
LENS_R = 6.0                            # 0.07H diameter optical lens, upper-left third of the front face
# Runtime code-driven M01 rig constants the compatibility parts are measured against (README section 1/8):
# Source/EchoesOfTheBrokenSun/Private/EchoesM01SurveyorRig.cpp lines 439 (+10 cm ankle above the sole) and 446-449 (42/46 two-bone IK).
RUNTIME_M01 = {"thigh_cm": 42.0, "shin_cm": 46.0, "ankle_above_sole_cm": 10.0, "source": "EchoesM01SurveyorRig.cpp:439,446-449", "tolerance_cm": 0.5}


def _len(d):
    return math.sqrt(d[0] ** 2 + d[1] ** 2 + d[2] ** 2)


def _dir_angles(d):
    """(pitch, yaw) that carry local +X onto delta d with ebs_meshkit.rot_yp (pitch about Y, then yaw about Z).
    A limb running backward (d[0] < 0) takes the over-vertical pitch with yaw near 0 so its local +Z
    (the plated front face) still faces forward instead of flipping to the rear."""
    n = _len(d)
    pitch = math.degrees(math.asin(d[2] / n))
    if d[0] < 0:
        return 180.0 - pitch - 360.0, math.degrees(math.atan2(-d[1], -d[0]))
    return pitch, math.degrees(math.atan2(d[1], d[0]))


def _front_roll(d, pitch, yaw):
    """Roll (about the limb's local +X, applied before the pitch/yaw) that turns the part's local +Z
    (the plated front face) toward world +X. Pitch-then-yaw alone fixes the twist by the heading of
    the limb, which for a near-vertical splayed leg (small x, large y) turns the plates sideways;
    the roll restores a forward-facing plate (ebs_skelkit.rot_rotator order: roll, pitch, yaw)."""
    n = _len(d)
    axis = tuple(c / n for c in d)
    fwd = (1.0 - axis[0] * axis[0], -axis[0] * axis[1], -axis[0] * axis[2])   # world +X projected off the limb axis
    fl = _len(fwd)
    if fl < 1e-9:
        return 0.0
    fwd = tuple(c / fl for c in fwd)
    y_m = kit.rot_yp((0.0, 1.0, 0.0), yaw, pitch)
    z_m = kit.rot_yp((0.0, 0.0, 1.0), yaw, pitch)
    # rot_x maps local (0,0,1) to (0, sin r, cos r): front = sin r * y_m + cos r * z_m
    return math.degrees(math.atan2(kit.v_dot(fwd, y_m), kit.v_dot(fwd, z_m)))


def limb_frame(d):
    """(pitch, yaw, roll) placing a part authored along local +X onto delta d with its plates facing forward."""
    pitch, yaw = _dir_angles(d)
    return pitch, yaw, _front_roll(d, pitch, yaw)


def rolled(m: kit.Mesh, roll_deg: float) -> kit.Mesh:
    """Copy of a part rolled about its local +X (Unreal roll) before it is merged with pitch/yaw."""
    out = kit.Mesh(m.name, slots=list(m.slots))
    for poly in m.polygons:
        pts = [skel.rot_x(p, roll_deg) for p in poly.points]
        n = skel.rot_x(poly.normal, roll_deg)
        uv_axis = None
        if poly.uv_axis is not None:
            uv_axis = (skel.rot_x(poly.uv_axis[0], roll_deg), skel.rot_x(poly.uv_axis[1], roll_deg))
        out.polygons.append(kit.Polygon(pts, n, poly.slot, poly.component, uv_axis))
    for s in m.sockets:
        out.sockets.append(kit.Socket(s.name, skel.rot_x(s.position, roll_deg), s.yaw_deg, s.purpose))
    return out


THIGH_LEN, SHIN_LEN = _len(THIGH_D), _len(SHIN_D)
UPPER_ARM_LEN, FOREARM_LEN = _len(UPPER_D), _len(FORE_D)


def rest_positions():
    """Joint heads in the knuckle-walker rest stance (right side; left mirrors Y). The *_dir entries are
    (pitch, yaw, roll) part frames for the right side; the left side mirrors the delta and re-solves."""
    knee = (HIP[0] + THIGH_D[0], HIP[1] + THIGH_D[1], HIP[2] + THIGH_D[2])
    ankle = (knee[0] + SHIN_D[0], knee[1] + SHIN_D[1], knee[2] + SHIN_D[2])
    elbow = (SHOULDER[0] + UPPER_D[0], SHOULDER[1] + UPPER_D[1], SHOULDER[2] + UPPER_D[2])
    wrist = (elbow[0] + FORE_D[0], elbow[1] + FORE_D[1], elbow[2] + FORE_D[2])
    fore_unit = tuple(c / FOREARM_LEN for c in FORE_D)
    drill_tip = tuple(wrist[i] + fore_unit[i] * (DRILL_HOUSING + DRILL_LEN) for i in range(3))
    assert abs(ankle[2] - ANKLE_Z) < 1e-9, ankle
    return {"hip": HIP, "knee": knee, "ankle": ankle, "elbow": elbow, "wrist": wrist, "drill_tip": drill_tip,
            "thigh_dir": limb_frame(THIGH_D), "shin_dir": limb_frame(SHIN_D), "upper_dir": limb_frame(UPPER_D), "fore_dir": limb_frame(FORE_D)}


def _mirror(d, sign):
    return (d[0], sign * d[1], d[2])


def build_skeleton() -> skel.Skeleton:
    p = rest_positions()
    s = skel.Skeleton(root="root")
    s.add("root", None, (0.0, 0.0, 0.0), "ground contact centre; never carries simulation translation")
    s.add("body", "root", (HIP[0], 0.0, HIP[2]), "hip centre; torso, mast and cradle as one rigid mass")
    for side, sign in (("r", 1.0), ("l", -1.0)):
        s.add(f"{side}_thigh", "body", (HIP[0], sign * HIP[1], HIP[2]), "hip hinge (pitch) and turn (yaw)")
        s.add(f"{side}_shin", f"{side}_thigh", (p["knee"][0], sign * p["knee"][1], p["knee"][2]), "knee hinge (pitch)")
        s.add(f"{side}_foot", f"{side}_shin", (p["ankle"][0], sign * p["ankle"][1], p["ankle"][2]), "ankle; planted foot")
    for side, sign in (("r", 1.0), ("l", -1.0)):
        s.add(f"{side}_shoulder", "body", (SHOULDER[0], sign * SHOULDER[1], SHOULDER[2]), "shoulder hub (pitch/yaw)")
        s.add(f"{side}_forearm", f"{side}_shoulder", (p["elbow"][0], sign * p["elbow"][1], p["elbow"][2]), "elbow (pitch); carries the tool")
    return s


# --- parts authored in their local frames (pivot at the joint, limb along +X; local +Z faces the front) ----
def part_upper_leg(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_UpperLeg")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    L, sides = THIGH_LEN, (8 if lod == 0 else 6)
    m.box((L / 2.0, 0.0, 0.0), (L, LEG_W, LEG_W), frame, "thigh_strut")
    # thick ceramic thigh plate wrapping the front and sides (concept: plated thighs wider than the frame)
    m.box((L * 0.54, 0.0, 2.0), (L * 0.66, LEG_W + 6.0, LEG_W + 5.0), ceramic, "thigh_plate")
    m.tube((0.0, -LEG_W / 2.0 - 4.0, 0.0), (0.0, LEG_W / 2.0 + 4.0, 0.0), 8.0, sides, frame, "hip_hub", caps=True)
    if lod == 0:
        m.box((L * 0.74, 0.0, LEG_W / 2.0 + 4.6), (5.0, 4.0, 1.0), status, "thigh_status")
    return m


def part_lower_leg(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_LowerLeg")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    L, sides = SHIN_LEN, (8 if lod == 0 else 6)
    m.box((L / 2.0, 0.0, 0.0), (L, LEG_W - 1.0, LEG_W - 1.0), frame, "shin_strut")
    m.box((L * 0.52, 0.0, 1.5), (L * 0.56, LEG_W + 4.0, LEG_W + 3.0), ceramic, "shin_plate")
    # prominent knee disc (0.09H diameter) with outer cap rings at LOD0
    m.tube((0.0, -LEG_W / 2.0 - 5.0, 0.0), (0.0, LEG_W / 2.0 + 5.0, 0.0), KNEE_R, sides, frame, "knee_hub", caps=True)
    if lod == 0:
        for sy in (-1.0, 1.0):
            m.tube((0.0, sy * (LEG_W / 2.0 + 5.0), 0.0), (0.0, sy * (LEG_W / 2.0 + 6.5), 0.0), KNEE_R - 3.0, sides, ceramic, "knee_ring", caps=True)
        m.box((L * 0.86, 0.0, LEG_W / 2.0 + 2.0), (8.0, 3.0, 1.0), status, "shin_status")
    return m


def part_foot(lod: int) -> kit.Mesh:
    """Ankle at the origin, sole resting on local Z = -ANKLE_Z (so the foot stands on z=0 in the assembly)."""
    m = kit.Mesh("EBS_MER_UNT_001_Foot")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    sides = 8 if lod == 0 else 6
    # 0.20H block foot centred on the ankle (concept: foot under the leg column): the sole runs 17.5 cm
    # either way; a 15 cm raised toe plate ahead, a 9 cm heel block behind
    m.box((0.0, 0.0, -ANKLE_Z + 4.0), (FOOT_LEN, LEG_W + 8.0, 8.0), frame, "foot_sole")
    m.box((10.0, 0.0, -ANKLE_Z + 11.0), (15.0, LEG_W + 6.0, 6.0), ceramic, "foot_toe_plate")
    m.box((-13.0, 0.0, -ANKLE_Z + 10.0), (9.0, LEG_W + 2.0, 12.0), frame, "heel_block")
    m.box((0.0, 0.0, -4.0), (10.0, 8.0, 10.0), frame, "ankle_strut")
    if lod == 0:
        m.tube((0.0, -LEG_W / 2.0 - 3.0, 0.0), (0.0, LEG_W / 2.0 + 3.0, 0.0), 6.0, sides, frame, "ankle_hub", caps=True)
    return m


def part_upper_arm(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_UpperArm")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    L, sides = UPPER_ARM_LEN, (8 if lod == 0 else 6)
    m.box((L / 2.0, 0.0, 0.0), (L, ARM_W, ARM_W), frame, "upper_arm_strut")
    m.box((L * 0.52, 0.0, 1.5), (L * 0.6, ARM_W + 5.0, ARM_W + 3.0), ceramic, "upper_arm_plate")
    m.tube((0.0, -ARM_W / 2.0 - 3.0, 0.0), (0.0, ARM_W / 2.0 + 3.0, 0.0), 7.0, sides, frame, "shoulder_hub", caps=True)
    if lod == 0:
        m.box((L * 0.7, 0.0, ARM_W / 2.0 + 3.1), (4.0, 3.0, 1.0), status, "upper_arm_status")
    return m


def part_forearm(lod: int, tool: str) -> kit.Mesh:
    m = kit.Mesh(f"EBS_MER_UNT_001_Forearm_{tool}")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    L, sides = FOREARM_LEN, (8 if lod == 0 else 6)
    m.box((L / 2.0, 0.0, 0.0), (L, ARM_W - 1.0, ARM_W - 1.0), frame, "forearm_strut")
    m.box((L * 0.46, 0.0, 1.0), (L * 0.56, ARM_W + 4.0, ARM_W + 2.0), ceramic, "forearm_plate")
    m.tube((0.0, -ARM_W / 2.0 - 3.0, 0.0), (0.0, ARM_W / 2.0 + 3.0, 0.0), 7.0, sides, frame, "elbow_hub", caps=True)
    m.box((L * 0.5, 0.0, ARM_W / 2.0 + 2.1), (10.0, 3.0, 1.0), status, "forearm_status_strip")
    if lod == 0:
        m.tube((L, -ARM_W / 2.0 - 2.0, 0.0), (L, ARM_W / 2.0 + 2.0, 0.0), 6.0, sides, frame, "wrist_hub", caps=True)
    if tool == "drill":
        # Rotary drill: cylindrical housing with a cyan status strip, then a tapered bit (stepped cone).
        m.tube((L - 2.0, 0.0, 0.0), (L + DRILL_HOUSING, 0.0, 0.0), 8.0, sides, frame, "drill_housing", caps=True)
        m.box((L + DRILL_HOUSING / 2.0, 0.0, 8.3), (8.0, 4.0, 1.0), status, "drill_status_strip")
        steps = 5 if lod == 0 else 3
        for k in range(steps):
            x0 = L + DRILL_HOUSING + k * DRILL_LEN / steps
            x1 = L + DRILL_HOUSING + (k + 1) * DRILL_LEN / steps
            r = 6.5 - k * (5.0 / steps)
            m.tube((x0, 0.0, 0.0), (x1, 0.0, 0.0), r, sides, frame, "drill_bit", caps=True)
        m.sockets.append(kit.Socket("Harvest_Tether_Muzzle", (L + DRILL_HOUSING + DRILL_LEN, 0.0, 0.0), 0.0, "drill tip; harvest / Network Repair particle stream origin"))
    else:
        # Gripper: knuckle block, two long fingers, thumb nub below, recessed palm welder between the fingers.
        m.box((L + 4.0, 0.0, 0.0), (8.0, ARM_W + 4.0, ARM_W + 1.0), frame, "gripper_knuckle")
        for sy in (-1.0, 1.0):
            m.box((L + 8.0 + GRIPPER_LEN / 2.0, sy * 5.5, 0.0), (GRIPPER_LEN, 3.5, 6.0), frame, f"gripper_finger_{'r' if sy > 0 else 'l'}")
            if lod == 0:
                m.box((L + 8.0 + GRIPPER_LEN - 2.0, sy * 4.0, -0.5), (6.0, 3.0, 5.0), frame, f"gripper_tip_{'r' if sy > 0 else 'l'}")
        m.box((L + 13.0, 0.0, -5.5), (10.0, 5.0, 4.0), frame, "gripper_thumb")
        m.box((L + 9.5, 0.0, 0.0), (3.0, 5.0, 4.0), status, "palm_welder")
    return m


# --- body (torso block, hubs, hip block, mast, cradle) --------------------------------------------
def build_torso_block(lod: int) -> kit.Mesh:
    """Torso block in its own frame (centre at the origin); merged with the forward pitch."""
    m = kit.Mesh("torso")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    dx, dy, dz = TORSO
    sides = 8 if lod == 0 else 6
    m.box((0.0, 0.0, 0.0), (dx - 6.0, dy - 6.0, dz - 4.0), frame, "torso_frame")
    # pale ceramic plates over the charcoal frame: front, top, both flanks, rear
    for sy, w in ((-1.0, (dy - 8.0) / 2.0 - 2.0), (1.0, (dy - 8.0) / 2.0 - 2.0)):
        m.box((dx / 2.0 - 2.0, sy * (w / 2.0 + 2.0), 1.0), (4.0, w, dz - 10.0), ceramic, "torso_shell_front")
    m.box((1.0, 0.0, dz / 2.0 - 2.0), (dx - 12.0, dy - 12.0, 4.0), ceramic, "torso_shell_top")
    for sy in (-1.0, 1.0):
        m.box((3.0, sy * (dy / 2.0 - 2.0), 3.0), (dx - 16.0, 4.0, dz - 12.0), ceramic, "torso_shell_side")
    m.box((-dx / 2.0 + 2.0, 0.0, 0.0), (4.0, dy - 14.0, dz - 16.0), ceramic, "torso_shell_rear")
    # ONE large optical lens on the upper-LEFT third of the front face (anatomical left = -Y)
    lens_y, lens_z = -dy * 0.22, dz * 0.22
    m.tube((dx / 2.0 - 1.0, lens_y, lens_z), (dx / 2.0 + 2.5, lens_y, lens_z), LENS_R + 1.5, sides, frame, "optic_bezel", caps=True)
    m.tube((dx / 2.0 + 2.5, lens_y, lens_z), (dx / 2.0 + 3.2, lens_y, lens_z), LENS_R, sides, status, "optic_window", caps=True)
    # cyan status band below the lens, across the chest
    m.box((dx / 2.0 + 0.5, -4.0, -6.0), (1.0, 34.0, 5.0), status, "chest_status_band")
    # repaired shoulder plate (book paragraph 1606): a newer plate bolted over the upper-right front
    m.box((dx / 2.0 + 0.6, dy * 0.24, dz * 0.22), (2.0, 15.0, 13.0), ceramic, "repair_patch")
    if lod == 0:
        for by, bz in ((-5.5, 4.5), (5.5, 4.5), (-5.5, -4.5), (5.5, -4.5)):
            m.box((dx / 2.0 + 1.9, dy * 0.24 + by, dz * 0.22 + bz), (1.0, 1.6, 1.6), frame, "repair_patch_bolt")
        # team band: a raised ceramic stripe along each flank top edge (masked in the texture)
        for sy in (-1.0, 1.0):
            m.box((2.0, sy * (dy / 2.0 + 0.5), dz / 2.0 - 8.0), (26.0, 1.0, 4.0), ceramic, "team_band")
    return m


def build_body(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_Body")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    hi = lod == 0
    sides = 8 if hi else 6
    m.merge(build_torso_block(lod), translate=TORSO_C, pitch_deg=TORSO_PITCH)
    # charcoal hip block under the rear half; hip hubs on its flanks
    m.box((HIP[0] + 2.0, 0.0, HIP[2] - 4.0), (30.0, 2.0 * HIP[1] + 4.0, 14.0), frame, "hip_block")
    for sy in (-1.0, 1.0):
        m.tube((HIP[0], sy * (HIP[1] - 4.0), HIP[2]), (HIP[0], sy * (HIP[1] + 2.0), HIP[2]), 9.0, sides, frame, "hip_hub")
        # large shoulder-hub discs on the torso sides (concept: the biggest joint on the frame)
        m.tube((SHOULDER[0], sy * (TORSO[1] / 2.0 - 3.0), SHOULDER[2]), (SHOULDER[0], sy * (SHOULDER[1] - 1.0), SHOULDER[2]), SHOULDER_HUB_R, sides, frame, "shoulder_disc")
        if hi:
            m.tube((SHOULDER[0], sy * (SHOULDER[1] - 1.0), SHOULDER[2]), (SHOULDER[0], sy * (SHOULDER[1] + 0.5), SHOULDER[2]), SHOULDER_HUB_R - 4.0, sides, ceramic, "shoulder_disc_ring")
    # optical mast: short post and boxy camera head, rear-LEFT top; top face at H
    m.box((MAST_BASE[0], MAST_BASE[1], MAST_BASE[2] + MAST_H / 2.0), (6.0, 6.0, MAST_H), frame, "optical_mast")
    head_z = MAST_BASE[2] + MAST_H + MAST_HEAD[2] / 2.0
    m.box((MAST_BASE[0] + 1.0, MAST_BASE[1], head_z), MAST_HEAD, frame, "optical_mast_head")
    m.box((MAST_BASE[0] + 1.0 + MAST_HEAD[0] / 2.0 + 0.5, MAST_BASE[1], head_z), (1.0, 6.0, 5.0), status, "optical_lens")
    if hi:
        m.box((MAST_BASE[0] + 1.0, MAST_BASE[1], head_z + MAST_HEAD[2] / 2.0 - 0.5), (8.0, 7.0, 1.0), ceramic, "optical_mast_cap")  # flush cap, top at H
    # cargo cradle (concept: open rack hung on the REAR at shoulder height, caps just above the torso top;
    # GAP-01 transverse cradle): tray behind the rear face, outer rail flush with the right flank, three
    # upright canisters in a row across Y
    cx, cy = CRADLE_C
    tray_w = 3 * CANISTER_PITCH + 4.0
    tray_d = 2 * CANISTER_R + 10.0
    m.box((cx, cy, CRADLE_FLOOR_Z - 2.0), (tray_d, tray_w, 4.0), frame, "cradle_floor")
    for sx in (-1.0, 1.0):
        m.box((cx + sx * (tray_d / 2.0 - 1.5), cy, CRADLE_FLOOR_Z + 2.5), (3.0, tray_w, 5.0), frame, "cradle_rail")
    for sy in (-1.0, 1.0):
        m.box((cx, cy + sy * (tray_w / 2.0 - 1.5), CRADLE_FLOOR_Z + 2.5), (tray_d, 3.0, 5.0), frame, "cradle_rail")
    # hangers: two vertical bars up the rear face to hook bars over the torso's rear top edge, and a
    # strut closing the gap between the tray's front rail and the rear shell (open rack, rigid with the body)
    x_hang = cx + tray_d / 2.0 + 0.5           # just behind the tray's front rail, against the rear face
    for y in (cy - tray_w / 2.0 + 4.0, cy + tray_w / 2.0 - 4.0):
        m.box((x_hang, y, (CRADLE_FLOOR_Z - 4.0 + CRADLE_HOOK_Z + 2.0) / 2.0), (4.0, 4.0, CRADLE_HOOK_Z + 2.0 - (CRADLE_FLOOR_Z - 4.0)), frame, "cradle_bracket")
        m.box((x_hang + 5.0, y, CRADLE_HOOK_Z), (14.0, 4.0, 4.0), frame, "cradle_bracket")
    m.box((x_hang + 1.0, cy, CRADLE_FLOOR_Z - 2.0), (6.0, tray_w - 8.0, 4.0), frame, "cradle_bracket")
    for k, y in enumerate((cy - CANISTER_PITCH, cy, cy + CANISTER_PITCH)):
        z0, z1 = CRADLE_FLOOR_Z, CRADLE_FLOOR_Z + CANISTER_H
        name = f"canister_{k + 1:02d}"
        m.tube((cx, y, z0), (cx, y, z1), CANISTER_R, sides, ceramic, name, caps=True)
        # vertical cyan window strip on the rear face, readable from the rear and top
        m.box((cx - CANISTER_R - 0.4, y, (z0 + z1) / 2.0), (1.0, 5.0, CANISTER_H - 9.0), status, f"{name}_window")
        if hi:
            m.tube((cx, y, z1), (cx, y, z1 + 2.0), CANISTER_R - 2.5, sides, frame, f"{name}_cap", caps=True)
            m.tube((cx, y, z0 + 3.0), (cx, y, z0 + 5.0), CANISTER_R + 0.6, sides, frame, f"{name}_band", caps=False)
    m.sockets.append(kit.Socket("Cargo_Drop_Anchor", (cx, cy, CRADLE_FLOOR_Z + CANISTER_H / 2.0), 180.0, "cargo cradle centre; delivery latch effect and cargo visibility anchor"))
    m.sockets.append(kit.Socket("Center_Hitbox_Socket", TORSO_C, 0.0, "damage acknowledgement and selection/health reference"))
    return m


# --- assembly ----------------------------------------------------------------------------------
def assemble(lod: int):
    """Rest-stance mesh with every polygon bound to its bone, plus the skeleton."""
    p = rest_positions()
    s = build_skeleton()
    m = kit.Mesh(ASSET)
    for name in (FRAME, CERAMIC, STATUS_REVIEW):
        m.slot(name)
    m.merge(build_body(lod), component_prefix="body_")
    for side, sign in (("r", 1.0), ("l", -1.0)):
        hip = s.get(f"{side}_thigh").head
        knee = s.get(f"{side}_shin").head
        ankle = s.get(f"{side}_foot").head
        # legs: (pitch, yaw, roll) frames solved per side so the plates face forward on the near-vertical columns
        tp, ty, tr = limb_frame(_mirror(THIGH_D, sign))
        sp, sy, sr = limb_frame(_mirror(SHIN_D, sign))
        m.merge(rolled(part_upper_leg(lod), tr), translate=hip, pitch_deg=tp, yaw_deg=ty, component_prefix=f"{side}_thigh_")
        m.merge(rolled(part_lower_leg(lod), sr), translate=knee, pitch_deg=sp, yaw_deg=sy, component_prefix=f"{side}_shin_")
        m.merge(part_foot(lod), translate=ankle, component_prefix=f"{side}_foot_")
        shoulder = s.get(f"{side}_shoulder").head
        elbow = s.get(f"{side}_forearm").head
        # arms: pitch/yaw only (their heading is forward-outward, so the plates already face out and up)
        m.merge(part_upper_arm(lod), translate=shoulder, pitch_deg=p["upper_dir"][0], yaw_deg=sign * p["upper_dir"][1], component_prefix=f"{side}_shoulder_")
        tool = "drill" if side == "r" else "gripper"
        m.merge(part_forearm(lod, tool), translate=elbow, pitch_deg=p["fore_dir"][0], yaw_deg=sign * p["fore_dir"][1], component_prefix=f"{side}_forearm_", include_sockets=True)
    binding = {"body_": "body"}
    for side in ("r", "l"):
        binding[f"{side}_thigh_"] = f"{side}_thigh"
        binding[f"{side}_shin_"] = f"{side}_shin"
        binding[f"{side}_foot_"] = f"{side}_foot"
        binding[f"{side}_shoulder_"] = f"{side}_shoulder"
        binding[f"{side}_forearm_"] = f"{side}_forearm"
    counts = skel.bind_polygons(m, "root", binding)
    sockets_on_bones = {"Harvest_Tether_Muzzle": "r_forearm", "Cargo_Drop_Anchor": "body", "Center_Hitbox_Socket": "body"}
    return m, s, counts, sockets_on_bones


def export_mesh(m: kit.Mesh) -> kit.Mesh:
    """Two-slot export copy: the review-only status pseudo slot folds into the ceramic slot."""
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


def unloaded_mesh(m: kit.Mesh) -> kit.Mesh:
    """Review copy with the cargo hidden (canister components removed), as the runtime does after the transfer."""
    out = kit.Mesh(m.name + "_unloaded", slots=list(m.slots))
    for poly in m.polygons:
        if poly.component.startswith("body_canister_"):
            continue
        q = kit.Polygon(list(poly.points), poly.normal, poly.slot, poly.component, poly.uv_axis, poly.uv_override, poly.atlas_cells, poly.chart_id)
        q.bone = getattr(poly, "bone", None)
        out.polygons.append(q)
    out.sockets = list(m.sockets)
    return out


def polygon_area(poly) -> float:
    pts = poly.points
    total = (0.0, 0.0, 0.0)
    for k in range(1, len(pts) - 1):
        c = kit.v_cross(kit.v_sub(pts[k], pts[0]), kit.v_sub(pts[k + 1], pts[0]))
        total = kit.v_add(total, c)
    return 0.5 * kit.v_len(total)


def emissive_area_fraction(m: kit.Mesh) -> float:
    """Share of the surface area carried by the cyan status pseudo slot (emissive <= 5% rule)."""
    status = m.slot(STATUS_REVIEW)
    total = sum(polygon_area(p) for p in m.polygons)
    cyan = sum(polygon_area(p) for p in m.polygons if p.slot == status)
    return cyan / total if total else 0.0


def concept_measurements(m: kit.Mesh) -> dict:
    """Proportions measured on the built rest stance, as ratios of H (concept-fidelity.md items 1-8)."""
    p = rest_positions()
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    hip_line_x = HIP[0]
    torso_top = max(m.component_bounds("body_torso_shell_top")[1][2], m.component_bounds("body_torso_frame")[1][2])
    cap_top = max(m.component_bounds(f"body_canister_{k:02d}")[1][2] for k in (1, 2, 3))
    cap_top = max(cap_top, *(m.component_bounds(f"body_canister_{k:02d}_cap")[1][2] for k in (1, 2, 3) if f"body_canister_{k:02d}_cap" in m.components()))
    tray_y_max = max(m.component_bounds("body_cradle_rail")[1][1], m.component_bounds("body_cradle_floor")[1][1])
    tray_x = m.component_bounds("body_cradle_floor")
    thigh_deg = math.degrees(math.atan2(THIGH_D[0], -THIGH_D[2]))   # + = knee ahead of the hip
    shin_deg = math.degrees(math.atan2(SHIN_D[0], -SHIN_D[2]))      # + = ankle ahead of the knee
    return {
        "H_cm": z1,
        "width_across_arms_cm": y1 - y0, "width_across_arms_over_H": (y1 - y0) / z1,
        "wider_than_tall": (y1 - y0) > z1,
        "feet_apart_cm": 2.0 * p["ankle"][1], "feet_apart_over_H": 2.0 * p["ankle"][1] / z1,
        "torso_wdh_over_H": [TORSO[1] / H, TORSO[0] / H, TORSO[2] / H], "torso_pitch_deg": TORSO_PITCH,
        "upper_arm_over_H": UPPER_ARM_LEN / H, "forearm_over_H": FOREARM_LEN / H, "drill_tool_over_H": (DRILL_HOUSING + DRILL_LEN) / H,
        "thigh_over_H": THIGH_LEN / H, "shin_over_H": SHIN_LEN / H, "foot_len_over_H": FOOT_LEN / H, "knee_disc_diameter_over_H": 2.0 * KNEE_R / H,
        "thigh_lean_deg_from_vertical": thigh_deg, "shin_lean_deg_from_vertical": shin_deg,
        "drill_tip_height_cm": p["drill_tip"][2], "drill_tip_ahead_of_hip_line_cm": p["drill_tip"][0] - hip_line_x,
        "forearm_below_horizontal_deg": math.degrees(math.asin(-FORE_D[2] / FOREARM_LEN)),
        "knee_ahead_of_hip_cm": p["knee"][0] - HIP[0], "ankle_ahead_of_knee_cm": p["ankle"][0] - p["knee"][0], "toe_ahead_of_knee_cm": (p["ankle"][0] + FOOT_LEN / 2.0) - p["knee"][0],
        "lens_diameter_over_H": 2.0 * LENS_R / H, "canister_diameter_over_H": 2.0 * CANISTER_R / H, "canister_height_over_H": CANISTER_H / H,
        "cradle_floor_z_cm": CRADLE_FLOOR_Z, "cradle_floor_over_torso_height": (CRADLE_FLOOR_Z - (TORSO_C[2] - TORSO[2] / 2.0)) / TORSO[2],
        "canister_cap_above_torso_top_cm": cap_top - torso_top, "cradle_tray_x_cm": [tray_x[0][0], tray_x[1][0]], "cradle_tray_y_max_cm": tray_y_max,
        "torso_flank_y_cm": TORSO[1] / 2.0,
        "mast_post_over_H": MAST_H / H, "mast_head_over_H": MAST_HEAD[2] / H,
        "cradle_side": "rear-right (+Y)", "mast_side": "rear-left (-Y)", "lens_side": "front upper-left (-Y)",
        "emissive_area_fraction": emissive_area_fraction(m),
    }


def compatibility_parts_record() -> dict:
    """Authored lengths of the M01 compatibility parts against the runtime rig's hard-coded constants."""
    authored = {"thigh_cm": THIGH_LEN, "shin_cm": SHIN_LEN, "foot_len_cm": FOOT_LEN, "ankle_above_sole_cm": ANKLE_Z}
    tol = RUNTIME_M01["tolerance_cm"]
    deltas = {k: authored[k] - RUNTIME_M01[k] for k in ("thigh_cm", "shin_cm", "ankle_above_sole_cm")}
    matches = all(abs(v) <= tol for v in deltas.values())
    return {"authored": authored, "runtime": RUNTIME_M01, "delta_cm": deltas, "matches_runtime": matches, "requires_runtime_change": not matches,
            "note": "Parts are authored at the concept lengths; the static-part route needs UpdateM01SurveyorRig's 42/46/+10 constants changed (or a second export at the runtime lengths). README section 8."}


# --- clips (Unreal rotators in degrees; seconds) --------------------------------------------------
def leg_drop(thigh_deg: float, shin_deg: float) -> float:
    """Body translation (z, cm) that returns the level sole to the ground for a knee bend measured
    from the rest leg (thigh and shin pitches about their joints; the foot counters both)."""
    p = rest_positions()
    hip, knee, ankle = p["hip"], p["knee"], p["ankle"]
    a = kit.v_add(knee, skel.rot_rotator(kit.v_sub(ankle, knee), shin_deg, 0.0, 0.0))
    a = kit.v_add(hip, skel.rot_rotator(kit.v_sub(a, hip), thigh_deg, 0.0, 0.0))
    return -(a[2] - ankle[2])


# Pose constants tuned numerically with ebs_skelkit.pose_mesh against the rest stance (scratch grid search
# over the crouch and the shoulder/forearm rotators; every clip sampled at 20 ms keeps z >= -1 cm).
# concept-v3 (2026-09-06): the legs are vertical columns, so a crouch needs a real knee bend to lower the
# frame (thigh forward + shin back); gather/deliver/death re-keyed to the reference panels (README section 5/8).
GATHER_PITCH, GATHER_THIGH, GATHER_SHIN = -14.0, 48.0, -96.0         # GATHERING panel crouch: knee bend lowers the frame 18.5 cm, torso 24 deg nose-down
GATHER_R_SH, GATHER_R_FA = (71.5, -5.0), (-70.0, 0.0, 0.0)           # elbow high (z 71), drill bit 61 deg into the ground ~82 cm ahead of the toe (tip 1.4-2.6 cm up with the pump)
GATHER_L_SH, GATHER_L_FA = (-6.0, 6.0, 0.0), (45.0, 0.0, 0.0)        # gripper down beside the frame, tips ~8 cm up
DELIVER_PITCH, DELIVER_THIGH, DELIVER_SHIN = -2.0, 60.0, -120.0      # DELIVERY panel squat: knee bend drops the body 31 cm; body -2 keeps the torso at -12 deg total
DELIVER_R_SH, DELIVER_FA = (78.0, 10.0), -90.0                       # upper arms level forward, both tools folded 60 deg down: tips 10-17 cm up, 66 cm ahead of the toe
DEATH_PITCH, DEATH_ROLL, DEATH_THIGH, DEATH_SHIN = -45.0, 2.0, 60.0, -130.0  # knees fold (drop 37 cm), torso pitches 45 deg forward onto the arms
DEATH_SH, DEATH_FA = (115.0, -20.0), -40.0                           # arms swing forward and fold: elbows ~38 cm up under the torso front (front edge ~27 cm up), drill tip on the ground ahead


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def leg_cycle(clip, side, phase, amp=22.0, lift=34.0, body_pitch=None):
        # thigh swings forward (+pitch) then back; shin folds during the swing; foot stays level.
        # body_pitch(t) is the torso lean keyed on the body bone; the thigh counters it so the
        # stride is measured from the vertical, not from the leaning torso.
        T = clip.duration_s
        for i in range(0, 9):
            t = i / 8.0
            th = amp * math.cos(2 * math.pi * (t + phase))
            swing = max(0.0, math.sin(2 * math.pi * (t + phase)))  # >0 while the leg swings forward
            sh = -6.0 - lift * swing
            bp = body_pitch(t) if body_pitch else 0.0
            clip.key(f"{side}_thigh", t * T, (th - bp, 0.0, 0.0))
            clip.key(f"{side}_shin", t * T, (sh, 0.0, 0.0))
            clip.key(f"{side}_foot", t * T, (-(th + sh) * 0.85, 0.0, 0.0))

    def lean(clip, t, pitch, roll=0.0, stance_yaw=0.0):
        # Torso lean about the hips with the feet planted: the legs hang from the body bone, so the
        # thighs counter the body pitch and the soles stay on z = 0 (positive pitch = leans back).
        clip.key("body", t, (pitch, 0.0, roll))
        clip.key("r_thigh", t, (-pitch, stance_yaw, -roll))
        clip.key("l_thigh", t, (-pitch, -stance_yaw, -roll))

    def crouch(clip, t, pitch, thigh, shin, roll=0.0):
        # Knee-bend crouch with level soles: foot = -(thigh + shin); the body translation that
        # returns the soles to z = 0 is solved from the rest joints (leg_drop).
        clip.key("body", t, (pitch, 0.0, roll), (0.0, 0.0, leg_drop(thigh, shin)))
        for side in ("r", "l"):
            clip.key(f"{side}_thigh", t, (-pitch + thigh, 0.0, -roll))
            clip.key(f"{side}_shin", t, (shin, 0.0, 0.0))
            clip.key(f"{side}_foot", t, (-(thigh + shin), 0.0, 0.0))

    def arms(clip, t, r_sh=(0.0, 0.0, 0.0), r_fa=(0.0, 0.0, 0.0), l_sh=(0.0, 0.0, 0.0), l_fa=(0.0, 0.0, 0.0)):
        clip.key("r_shoulder", t, r_sh)
        clip.key("r_forearm", t, r_fa)
        clip.key("l_shoulder", t, l_sh)
        clip.key("l_forearm", t, l_fa)

    def rest(clip, t, bones=None):
        for b in bones or [b.name for b in skeleton.bones if b.name != "root"]:
            clip.key(b, t, (0.0, 0.0, 0.0))

    LEGS = ("body", "r_thigh", "l_thigh", "r_shin", "l_shin", "r_foot", "l_foot")

    idle = skel.AnimationClip("idle", 2.0, loop=True, purpose="knuckle-walker at rest: breathing lean on the hips, tools touching down ahead, feet planted")
    for i, t in enumerate((0.0, 1.0, 2.0)):
        lean(idle, t, 1.0 if i == 1 else 0.0)
        idle.key("r_shoulder", t, (2.0 if i == 1 else 0.0, 0.0, 0.0))
        idle.key("l_shoulder", t, (2.0 if i == 1 else 0.0, 0.0, 0.0))
    clips.append(idle)

    move = skel.AnimationClip("move", 0.5, loop=True, purpose="walk cycle authored for 360 cm/s (two 90 cm strides); play rate scales with authoritative velocity; <= 2 cm vertical bob; the hanging tools swing clear of the ground")
    move_pitch = lambda t: 1.5 * math.sin(4 * math.pi * t)  # noqa: E731
    leg_cycle(move, "r", 0.0, body_pitch=move_pitch)
    leg_cycle(move, "l", 0.5, body_pitch=move_pitch)
    for i in range(0, 9):
        t = i / 8.0
        bounce = 2.0 * abs(math.sin(2 * math.pi * t))
        move.key("body", t * 0.5, (move_pitch(t), 0.0, 1.5 * math.cos(2 * math.pi * t)), (0.0, 0.0, bounce))
        # arms counter-swing at the shoulder; the forearm folds a little on the back swing so the tools stay clear
        move.key("r_shoulder", t * 0.5, (6.0 + 8.0 * math.cos(2 * math.pi * t), 0.0, 0.0))
        move.key("l_shoulder", t * 0.5, (6.0 - 8.0 * math.cos(2 * math.pi * t), 0.0, 0.0))
        move.key("r_forearm", t * 0.5, (6.0, 0.0, 0.0))
        move.key("l_forearm", t * 0.5, (6.0, 0.0, 0.0))
    clips.append(move)

    carry = skel.AnimationClip("carry", 0.5, loop=True, purpose="loaded walk: same cycle, body pitched forward, arms tucked up clear of the ground; cargo visibility follows the authoritative load")
    carry_pitch = lambda t: -4.0 + 1.0 * math.sin(4 * math.pi * t)  # noqa: E731
    leg_cycle(carry, "r", 0.0, amp=20.0, lift=30.0, body_pitch=carry_pitch)
    leg_cycle(carry, "l", 0.5, amp=20.0, lift=30.0, body_pitch=carry_pitch)
    for i in range(0, 9):
        t = i / 8.0
        carry.key("body", t * 0.5, (carry_pitch(t), 0.0, 1.5 * math.cos(2 * math.pi * t)), (0.0, 0.0, 1.5 * abs(math.sin(2 * math.pi * t))))
        arms(carry, t * 0.5, (25.0, 0.0, 0.0), (35.0, 0.0, 0.0), (25.0, 0.0, 0.0), (35.0, 0.0, 0.0))
    clips.append(carry)

    turn = skel.AnimationClip("turn", 0.5, loop=True, purpose="stationary shuffle while the runtime sweeps heading (SPEC-MOV-010); no root yaw in the clip")
    for i, t in enumerate((0.0, 0.125, 0.25, 0.375, 0.5)):
        turn.key("body", t, (0.0, 3.0 if i % 2 else 0.0, 0.0))
        # shuffle step: knee forward with the shin folded so the level sole lifts ~2.4 cm (a vertical column
        # only rises when both segments leave the vertical; thigh 28 / shin -34 / foot 6)
        turn.key("r_thigh", t, (28.0 if i % 4 == 1 else 0.0, 0.0, 0.0))
        turn.key("l_thigh", t, (28.0 if i % 4 == 3 else 0.0, 0.0, 0.0))
        turn.key("r_shin", t, (-34.0 if i % 4 == 1 else 0.0, 0.0, 0.0))
        turn.key("l_shin", t, (-34.0 if i % 4 == 3 else 0.0, 0.0, 0.0))
        turn.key("r_foot", t, (6.0 if i % 4 == 1 else 0.0, 0.0, 0.0))   # level sole on the lifted shuffle step
        turn.key("l_foot", t, (6.0 if i % 4 == 3 else 0.0, 0.0, 0.0))
        turn.key("r_shoulder", t, (4.0, 0.0, 0.0))
        turn.key("l_shoulder", t, (4.0, 0.0, 0.0))
    clips.append(turn)

    stop = skel.AnimationClip("stop", 0.3, purpose="short settle on stop (Bible): forward lean and recover, feet planted")
    lean(stop, 0.0, -3.0)
    lean(stop, 0.15, 1.0)
    lean(stop, 0.3, 0.0)
    for t, v in ((0.0, 4.0), (0.15, 0.0), (0.3, 0.0)):
        stop.key("r_shoulder", t, (v, 0.0, 0.0))
        stop.key("l_shoulder", t, (v, 0.0, 0.0))
    clips.append(stop)

    # GATHERING panel: deep forward crouch, drill tip into the deposit ahead, gripper down beside the frame
    gather = skel.AnimationClip("gather", 1.0, loop=True, purpose="GATHERING panel: crouched forward, drill tip at the deposit ahead (tip 0-6 cm above ground, ~100 cm forward), gripper on the ground beside; drill spin is a material effect gated by this state")
    for i, t in enumerate((0.0, 0.5, 1.0)):
        bob = 1.0 if i == 1 else 0.0   # drill pumps ~2 cm at the tip
        crouch(gather, t, GATHER_PITCH, GATHER_THIGH, GATHER_SHIN)
        arms(gather, t, (GATHER_R_SH[0] + bob, GATHER_R_SH[1], 0.0), GATHER_R_FA, GATHER_L_SH, GATHER_L_FA)
    clips.append(gather)

    # DELIVERY panel: the frame squats low beside the drop point, both tools down; cargo is emptied by the transfer
    deliver = skel.AnimationClip("deliver", 0.8, purpose="DELIVERY panel: low squat beside the drop point with both tools down; the cradle's canisters are runtime-hidden on the authorized transfer event (no arm reach required)")
    # squat in over 0.3 s and out over 0.3 s; keys every 50 ms carry the exact body drop for the
    # interpolated knee bend so the linearly sampled soles stay on the ground (deep bend: 60/-120)
    deliver_keys = [(0.05 * k, k / 6.0) for k in range(0, 7)] + [(0.5, 1.0)] + [(0.5 + 0.05 * k, 1.0 - k / 6.0) for k in range(1, 7)]
    for t, f in deliver_keys:
        crouch(deliver, t, DELIVER_PITCH * f, DELIVER_THIGH * f, DELIVER_SHIN * f)
        arms(deliver, t, (DELIVER_R_SH[0] * f, DELIVER_R_SH[1] * f, 0.0), (DELIVER_FA * f, 0.0, 0.0), (DELIVER_R_SH[0] * f, -DELIVER_R_SH[1] * f, 0.0), (DELIVER_FA * f, 0.0, 0.0))
    clips.append(deliver)

    build = skel.AnimationClip("build", 1.0, loop=True, purpose="both arms extended forward to the footprint (Bible build pose), slight forward lean; welder effect on the left palm")
    for i, t in enumerate((0.0, 0.5, 1.0)):
        lean(build, t, -6.0)
        arms(build, t, (60.0 + (3.0 if i == 1 else 0.0), -20.0, 0.0), (10.0 - (4.0 if i == 1 else 0.0), 0.0, 0.0),
             (60.0 + (3.0 if i == 1 else 0.0), 20.0, 0.0), (10.0 - (4.0 if i == 1 else 0.0), 0.0, 0.0))
    clips.append(build)

    repair = skel.AnimationClip("repair_when_authorized", 1.0, loop=True, purpose="welder arm only (Bible); the drill stays at rest")
    for i, t in enumerate((0.0, 0.5, 1.0)):
        lean(repair, t, -3.0)
        repair.key("l_shoulder", t, (55.0 + (3.0 if i == 1 else 0.0), 25.0, 0.0))
        repair.key("l_forearm", t, (12.0 - (3.0 if i == 1 else 0.0), 0.0, 0.0))
        repair.key("r_shoulder", t, (0.0, 0.0, 0.0))
    clips.append(repair)

    damage = skel.AnimationClip("damage", 0.3, purpose="flinch on authoritative damage: torso jolts back, arms lift; no displacement")
    lean(damage, 0.0, 0.0); lean(damage, 0.1, 8.0, roll=2.0); lean(damage, 0.3, 0.0)
    for side in ("r", "l"):
        damage.key(f"{side}_shoulder", 0.0, (0.0, 0.0, 0.0)); damage.key(f"{side}_shoulder", 0.1, (12.0, 0.0, 0.0)); damage.key(f"{side}_shoulder", 0.3, (0.0, 0.0, 0.0))
    clips.append(damage)

    # death: knees fold, the body drops and pitches forward onto the long arms; tools splay on the ground ahead
    death = skel.AnimationClip("death", 1.0, purpose="engineered collapse: knees fold, the frame drops and pitches forward onto its arms (the long arms carry it), tools on the ground ahead; final pose held (cosmetic debris <= 200 ticks)")
    rest(death, 0.0)
    # keys every 100 ms so the interpolated knee fold keeps the soles inside the ground rule
    for t, f in ((0.1 * k, 0.1 * k) for k in range(1, 11)):
        death.key("body", t, (DEATH_PITCH * f, 0.0, DEATH_ROLL * f), (0.0, 0.0, leg_drop(DEATH_THIGH * f, DEATH_SHIN * f)))
        for side, sgn in (("r", 1.0), ("l", -1.0)):
            death.key(f"{side}_thigh", t, ((-DEATH_PITCH + DEATH_THIGH) * f, sgn * 4.0 * f, -DEATH_ROLL * f))
            death.key(f"{side}_shin", t, (DEATH_SHIN * f, 0.0, 0.0))
            death.key(f"{side}_foot", t, (-(DEATH_THIGH + DEATH_SHIN) * f, 0.0, 0.0))
            death.key(f"{side}_shoulder", t, (DEATH_SH[0] * f, sgn * DEATH_SH[1] * f, 0.0))
            death.key(f"{side}_forearm", t, (DEATH_FA * f, 0.0, 0.0))
    clips.append(death)

    cancel = skel.AnimationClip("cancel", 0.3, purpose="tools return to rest from the work lean; drill spin stops on travel/cancel (GAP-01)")
    lean(cancel, 0.0, -6.0)
    arms(cancel, 0.0, (30.0, -10.0, 0.0), (5.0, 0.0, 0.0), (30.0, 10.0, 0.0), (5.0, 0.0, 0.0))
    lean(cancel, 0.3, 0.0)
    arms(cancel, 0.3)
    clips.append(cancel)

    restore = skel.AnimationClip("restore", 0.0, purpose="single-frame rest pose used when reconstructing presentation from saved state; never replays a one-shot")
    rest(restore, 0.0)   # identity key on every bone except root (no clip keys root)
    clips.append(restore)
    return clips


# --- manifest and CLI ---------------------------------------------------------------------------
def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "legs": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_thigh_thigh_strut"))},
        "tool_arms": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_forearm_forearm_strut")),
                      "right": "drill" if any(c == "r_forearm_drill_bit" for c in comps) else None, "left": "gripper" if any(c.startswith("l_forearm_gripper_finger") for c in comps) else None},
        "rear_canisters": {"contract": 3, "built": sum(1 for c in comps if c.startswith("body_canister_") and c.endswith(("01", "02", "03")))},
        "optical_mast": {"contract": 1, "built": sum(1 for c in comps if c == "body_optical_mast")},
        "feet": {"built": sum(1 for c in comps if c.endswith("_foot_foot_sole"))},
        "shoulder_discs": {"built": sum(1 for c in comps if c == "body_shoulder_disc")},
        "bones": {"contract": 12, "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": ["Harvest_Tether_Muzzle", "Cargo_Drop_Anchor", "Center_Hitbox_Socket"], "built": [sk.name for sk in m.sockets]},
        "tracks": {"contract": ["idle", "move", "turn", "stop", "damage", "death", "cancel", "restore", "gather", "carry", "deliver", "build", "repair_when_authorized"],
                   "built": [c.name for c in clips]},
    }


def main_cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--skinned", action="store_true", help="also write the skinned GLB with clips (requires the implemented skeletal kit)")
    args = parser.parse_args()
    export_dir = os.path.join(HERE, "export")
    review_dir = os.path.join(args.evidence_dir, "review")
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)

    outputs, review = [], []
    inventory = None
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, counts, sockets_on_bones = assemble(lod)
        if lod == 0:
            inventory = contract_inventory(m, s, clips)
        ex = export_mesh(m)
        base = os.path.join(export_dir, f"{ASSET}_LOD{lod}")
        # Rest-pose static exports (always): review OBJ (3 slots) and a static GLB (2 slots).
        rev = os.path.join(review_dir, f"{ASSET}_rest_LOD{lod}.obj")
        review.append({"path": rev, "sha256": m.write_obj(rev, header_lines=[f"Rest stance LOD{lod}; review only"]), "lod": lod, "triangles": m.triangle_count()})
        if lod == 0:
            un = unloaded_mesh(m)
            rev_un = os.path.join(review_dir, f"{ASSET}_rest_unloaded_LOD0.obj")
            review.append({"path": rev_un, "sha256": un.write_obj(rev_un, header_lines=["Rest stance LOD0 with the cargo hidden (canister components removed); review only"]), "lod": 0, "triangles": un.triangle_count(), "state": "unloaded"})
        glb = ex.write_glb(base + "_static.glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod, "note": "rest-pose static export; the skinned export carries the rig"}, include_collision=False)
        outputs.append({"path": os.path.relpath(base + "_static.glb", HERE), "sha256": glb, "lod": lod, "mesh": ASSET, "kind": "static rest pose", "triangles": ex.triangle_count(),
                        "by_slot": ex.triangle_count_by("slot"), "by_bone": counts, "bounds_cm": ex.bounds(), "sockets": [{"name": sk.name, "position_cm": sk.position, "yaw_deg": sk.yaw_deg, "bone": sockets_on_bones.get(sk.name)} for sk in ex.sockets]})
        if args.skinned:
            skinned = skel.write_skinned_glb(ex, s, base + ".glb", animations=clips, include_collision=False, sockets_on_bones=sockets_on_bones,
                                             extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod})
            outputs.append({"path": os.path.relpath(base + ".glb", HERE), "sha256": skinned, "lod": lod, "mesh": ASSET, "kind": "skinned + clips", "clips": [c.name for c in clips]})
        # Compatibility parts for the existing code-driven M01 rig (hip/knee/ankle pivots at the origin, limb along +X).
        for part_builder, name in ((part_upper_leg, "UpperLeg"), (part_lower_leg, "LowerLeg"), (part_foot, "Foot")):
            part = export_mesh(part_builder(lod))
            ppath = os.path.join(export_dir, f"SM_EBS_MER_UNT_001_{name}_LOD{lod}.glb")
            outputs.append({"path": os.path.relpath(ppath, HERE), "sha256": part.write_glb(ppath, extras={"production_id": PRODUCTION_ID, "revision": REVISION, "lod": lod, "compatibility": "M01 articulated static-part rig (EchoesEntityView M01SurveyorParts)"}, include_collision=False),
                            "lod": lod, "mesh": part.name, "kind": "compatibility part", "triangles": part.triangle_count()})

    m0 = assemble(0)[0]
    m1 = assemble(1)[0]
    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    measurements = concept_measurements(m0)
    manifest = {
        "author": AUTHOR, "creator": AUTHOR, "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "asset_name": ASSET,
        "revision": REVISION, "kit_revision": kit.KIT_REVISION, "skel_revision": skel.SKEL_REVISION, "stage": "BLOCKOUT (concept-v3)",
        "stage_boundary": "Concept-matched rest-stance geometry, 12-bone rig definition and keyframed clip data; skinned export on the verified skeletal kit; no textures, no import, no acceptance.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters at unit scale 1.0 (runtime PresentationScale 1.5 for workers)", "axes": "+X forward, +Y right, +Z up", "pivot": "ground-contact centre (root)"},
        "scale_basis": {"concept": "concept-fidelity.md: knuckle-walker measured on EBS-CON-MER-UNT-001 (meridian-units.png top-left) at H = 176 cm at the mast top",
                        "canon": "about as tall as a person and half again as wide at the shoulder (Bible line 509)",
                        "height_cm": m0.bounds()[1][2], "width_across_arms_cm": m0.bounds()[1][1] - m0.bounds()[0][1], "torso_width_cm": TORSO[1],
                        "status": "CONCEPT-MEASURED BLOCKOUT (concept-v3)"},
        "concept_measurements": measurements,
        "compatibility_parts": compatibility_parts_record(),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": b.head, "purpose": b.purpose} for b in skeleton.bones], "rest_stance": rest_positions(),
                "policy": "identity rest orientation; hinge pitch about +Y at the joint; no decorative bones; drill spin via material; canisters rigid with the body and emptied by the authoritative transfer (visibility/material), not by animation"},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "loop": c.loop, "bones": sorted(c.tracks), "purpose": c.purpose} for c in clips],
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(), "lod0_cap": 4500, "lod1_cap": 1800,
                    "lod0_within_cap": m0.triangle_count() <= 4500, "lod1_within_cap": m1.triangle_count() <= 1800,
                    "emissive_area_fraction_lod0": measurements["emissive_area_fraction"], "emissive_cap": 0.05},
        "material_slots": [FRAME, CERAMIC], "material_slot_policy": "2 slots (contract max 2); status band, canister windows, lens, tool strips, brass trim and team colour are texture channels of the ceramic slot; the review OBJ keeps a third pseudo slot so renders show the cyan",
        "component_inventory": inventory,
        "outputs": outputs, "review": review,
        "tools": {"builder_sha256": builder_sha, "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "concept_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/site/assets/concepts/meridian-units.png", "region": [0, 0, 0.5, 0.5], "sha256_prefix": "427e60cd27bd78e9", "status": "KEEP as DESIGN_IDENTITY (EBS-CON-MER-UNT-001)"},
            "candidate_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/surveyor-review/surveyor-reference.png", "sha256": "98c3a0ef2300178942d30e0f2ff8d725ef94a51d38877639638c3cd90b6fd913", "status": "DERIVED_REFERENCE_FOR_REVIEW (front, rear, gathering, delivery); the derived turnaround in the same folder was rejected (canister drift, reversed side labels)"},
            "fidelity_target": {"path": os.path.join(HERE, "concept-fidelity.md")},
            "canon_row": {"path": "Docs/Archive/DevelopmentBible.md", "line": 509},
            "book": {"paragraphs": [143, 1536, 1607], "sha256": "994e7df5d37a6ef1532c65a7b742d71e81b4ea7752311f5e5acc2e04c817ef83"},
            "gameplay_record": {"path": "Content/Data/Source/units.json", "id": "mc_surveyor (SPEC-UNIT-001: 360 cm/s, logistics footprint 1, work rate 10, cargo 10)"},
            "contract": {"path": "Docs/VisualAssetPipeline/motion/gap-decisions.json", "package_id": PACKAGE_ID},
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }
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
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"], "inventory": {k: v.get("built") for k, v in inventory.items() if isinstance(v, dict)}, "outputs": len(outputs),
                      "measurements": {k: (round(v, 3) if isinstance(v, float) else v) for k, v in measurements.items()}}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
