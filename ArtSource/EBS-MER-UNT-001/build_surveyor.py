#!/usr/bin/env python3
"""EBS-MER-UNT-001 Surveyor: deterministic editable source geometry, 12-bone rig and clips (blockout).

Author: Angelis Pseftis.

Package contract: Docs/VisualAssetPipeline/motion/gap-decisions.json
  production_policy[package_id == "EBS-PKG-MC-SURVEYOR"], gap GAP-01 (three rear canisters
  in one transverse cradle; anatomical right arm drill, left gripper with recessed palm
  welder; two planted feet; tools stop on travel/cancel; delivery empties cargo only on an
  authorized transfer). Prepared amendment MERIDIAN-ANATOMY: two articulated legs and
  planted feet, not treads.
Canon row: Docs/Archive/DevelopmentBible.md line 509 (SPEC-UNIT-001): "about as tall as a
  person and half again as wide at the shoulder", pale ceramic torso shell over a charcoal
  frame, two tool arms, small optical mast, rear cargo cradle with cyan-white Matter
  canisters, cyan status band across the chest.
Requirement card: REL-FAC-025.MC.SURVEYOR.ASSET: LOD0 <= 4,500 / LOD1 <= 1,800 tris, 2048^2
  maps, team colour through a mask, emissive <= 5%, 12-bone kinematic rig, sockets
  Harvest_Tether_Muzzle / Cargo_Drop_Anchor / Center_Hitbox_Socket.
Existing envelope: the runtime's welded Surveyor placeholder (~180 cm tall, ~100 cm across
  the arms; Scripts/generate_art_assets.py meridian_surveyor_body / paired_leg) and the M01
  articulated parts (hip-pivoted upper leg 42 cm, knee-pivoted lower leg 46 cm, foot 32 cm,
  all authored along local +X). Workers draw at PresentationScale 1.5 in EchoesEntityView.

Every dimension is a PROVISIONAL BLOCKOUT ESTIMATE within that envelope.

Rig (12 bones, identity rest orientation, heads in the crouched rest stance):
  root, body, r_thigh, r_shin, r_foot, l_thigh, l_shin, l_foot, r_shoulder, r_forearm,
  l_shoulder, l_forearm. The optical mast and cradle are rigid with the body; the drill
  spin is a material effect (no thirteenth bone).
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
REVISION = "ebs-mer-unt-001-blockout-v2"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_001/"

CERAMIC = "MI_EBS_MER_UnitCeramic"    # pale ceramic shell; status band, canister windows and team mask live in its texture channels
FRAME = "MI_EBS_MER_UnitFrame"        # charcoal frame, joints, tools
STATUS_REVIEW = "MI_EBS_MER_StatusCyan"  # review-only pseudo slot: folded into CERAMIC for the export (2-slot contract)

# --- provisional blockout dimensions (cm, authored scale 1.0; runtime draws workers at 1.5) ---
HIP_Y, HIP_Z = 21.0, 76.0
THIGH_LEN, SHIN_LEN, FOOT_LEN = 42.0, 46.0, 32.0
DELIVER_DROP_CM = -12.2  # body drop that keeps the soles on z = 0 for the deliver dip (thigh +12, shin -22, foot +10)
THIGH_PITCH = 40.0      # rest stance: thigh swung forward-down 40 deg from vertical
ANKLE_Z = 10.0
LEG_W = 11.0
PELVIS = (30.0, 44.0, 14.0)             # x, y, z size; centred at HIP_Z - 3
TORSO = (60.0, 50.0, 44.0)              # centred at z 102 (80..124)
TORSO_Z = 102.0
SHOULDER = (10.0, 48.0, 108.0)          # right shoulder head; left mirrors Y
UPPER_ARM_LEN, FOREARM_LEN = 40.0, 46.0
ARM_W = 9.0
MAST_BASE = (-14.0, -12.0, 124.0)       # rear-left of the torso top
MAST_H, MAST_HEAD = 40.0, (16.0, 12.0, 12.0)
CRADLE = (-31.0, 0.0, 104.0)            # transverse cradle centre behind the torso
CANISTER_R, CANISTER_H, CANISTER_PITCH = 8.5, 30.0, 19.0
DRILL_LEN, GRIPPER_LEN = 40.0, 22.0


def rest_positions():
    """Joint heads in the crouched rest stance (right side; left mirrors Y)."""
    thigh = (THIGH_LEN * math.sin(math.radians(THIGH_PITCH)), 0.0, -THIGH_LEN * math.cos(math.radians(THIGH_PITCH)))
    knee = (thigh[0], HIP_Y, HIP_Z + thigh[2])
    dz = knee[2] - ANKLE_Z
    dx = -math.sqrt(max(SHIN_LEN ** 2 - dz ** 2, 1.0))
    ankle = (knee[0] + dx, HIP_Y, ANKLE_Z)
    # Merge pitch for a part authored along local +X: the angle of the limb direction in the XZ plane.
    thigh_dir_pitch = math.degrees(math.atan2(thigh[2], thigh[0]))
    shin_dir_pitch = math.degrees(math.atan2(-dz, dx))
    upper = (14.0, 6.0, -math.sqrt(UPPER_ARM_LEN ** 2 - 14.0 ** 2 - 6.0 ** 2))
    elbow = (SHOULDER[0] + upper[0], SHOULDER[1] + upper[1], SHOULDER[2] + upper[2])
    fore = (36.0, 0.0, -math.sqrt(FOREARM_LEN ** 2 - 36.0 ** 2))
    wrist = (elbow[0] + fore[0], elbow[1] + fore[1], elbow[2] + fore[2])
    return {"knee": knee, "ankle": ankle, "thigh_dir_pitch": thigh_dir_pitch, "shin_dir_pitch": shin_dir_pitch,
            "elbow": elbow, "wrist": wrist, "upper_dir_pitch": math.degrees(math.atan2(upper[2], upper[0])),
            "fore_dir_pitch": math.degrees(math.atan2(fore[2], fore[0]))}


def build_skeleton() -> skel.Skeleton:
    p = rest_positions()
    s = skel.Skeleton(root="root")
    s.add("root", None, (0.0, 0.0, 0.0), "ground contact centre; never carries simulation translation")
    s.add("body", "root", (0.0, 0.0, HIP_Z), "pelvis and torso as one rigid mass; mast and cradle ride on it")
    for side, sign in (("r", 1.0), ("l", -1.0)):
        s.add(f"{side}_thigh", "body", (0.0, sign * HIP_Y, HIP_Z), "hip hinge (pitch) and turn (yaw)")
        s.add(f"{side}_shin", f"{side}_thigh", (p["knee"][0], sign * HIP_Y, p["knee"][2]), "knee hinge (pitch)")
        s.add(f"{side}_foot", f"{side}_shin", (p["ankle"][0], sign * HIP_Y, p["ankle"][2]), "ankle; planted foot")
    for side, sign in (("r", 1.0), ("l", -1.0)):
        s.add(f"{side}_shoulder", "body", (SHOULDER[0], sign * SHOULDER[1], SHOULDER[2]), "shoulder (pitch/yaw)")
        s.add(f"{side}_forearm", f"{side}_shoulder", (p["elbow"][0], sign * p["elbow"][1], p["elbow"][2]), "elbow (pitch); carries the tool")
    return s


# --- parts authored in their local frames (pivot at the joint, limb along +X) ---------------
def part_upper_leg(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_UpperLeg")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    m.box((THIGH_LEN / 2.0, 0.0, 0.0), (THIGH_LEN, LEG_W, LEG_W), frame, "thigh_strut")
    m.box((THIGH_LEN * 0.55, 0.0, 1.0), (THIGH_LEN * 0.6, LEG_W + 6.0, LEG_W + 4.0), ceramic, "thigh_plate")
    if lod == 0:
        m.tube((0.0, -LEG_W / 2.0 - 3.0, 0.0), (0.0, LEG_W / 2.0 + 3.0, 0.0), 7.0, 8, frame, "hip_hub", caps=True)
    return m


def part_lower_leg(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_LowerLeg")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    m.box((SHIN_LEN / 2.0, 0.0, 0.0), (SHIN_LEN, LEG_W, LEG_W), frame, "shin_strut")
    m.box((SHIN_LEN * 0.5, 0.0, -1.0), (SHIN_LEN * 0.62, LEG_W + 5.0, LEG_W + 3.0), ceramic, "shin_plate")
    if lod == 0:
        m.tube((0.0, -LEG_W / 2.0 - 3.0, 0.0), (0.0, LEG_W / 2.0 + 3.0, 0.0), 7.5, 8, frame, "knee_hub", caps=True)
        m.box((8.0, 0.0, 0.0), (16.0, LEG_W + 3.0, 14.0), ceramic, "knee_plate")
    return m


def part_foot(lod: int) -> kit.Mesh:
    """Ankle at the origin, sole resting on local Z = -ANKLE_Z (so the foot stands on z=0 in the assembly)."""
    m = kit.Mesh("EBS_MER_UNT_001_Foot")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    m.box((6.0, 0.0, -ANKLE_Z + 5.0), (FOOT_LEN, LEG_W + 8.0, 10.0), frame, "foot_sole")
    m.box((12.0, 0.0, -ANKLE_Z + 11.0), (14.0, LEG_W + 6.0, 6.0), ceramic, "foot_cap")
    if lod == 0:
        m.box((-6.0, 0.0, -ANKLE_Z + 12.0), (8.0, LEG_W + 2.0, 8.0), frame, "heel_block")
    return m


def part_upper_arm(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_UpperArm")
    frame, ceramic = m.slot(FRAME), m.slot(CERAMIC)
    m.box((UPPER_ARM_LEN / 2.0, 0.0, 0.0), (UPPER_ARM_LEN, ARM_W, ARM_W), frame, "upper_arm_strut")
    m.box((UPPER_ARM_LEN * 0.5, 0.0, 1.0), (UPPER_ARM_LEN * 0.55, ARM_W + 5.0, ARM_W + 3.0), ceramic, "upper_arm_plate")
    if lod == 0:
        m.tube((0.0, -ARM_W / 2.0 - 3.0, 0.0), (0.0, ARM_W / 2.0 + 3.0, 0.0), 6.5, 8, frame, "shoulder_hub", caps=True)
    return m


def part_forearm(lod: int, tool: str) -> kit.Mesh:
    m = kit.Mesh(f"EBS_MER_UNT_001_Forearm_{tool}")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    sides = 8 if lod == 0 else 6
    m.box((FOREARM_LEN / 2.0, 0.0, 0.0), (FOREARM_LEN, ARM_W, ARM_W), frame, "forearm_strut")
    m.box((FOREARM_LEN * 0.45, 0.0, 1.0), (FOREARM_LEN * 0.5, ARM_W + 5.0, ARM_W + 3.0), ceramic, "forearm_plate")
    if lod == 0:
        m.tube((0.0, -ARM_W / 2.0 - 3.0, 0.0), (0.0, ARM_W / 2.0 + 3.0, 0.0), 6.0, 8, frame, "elbow_hub", caps=True)
    if tool == "drill":
        # Rotary drill: housing, spindle collar, tapered bit (a cone approximated by stepped tubes at LOD0).
        m.tube((FOREARM_LEN - 4.0, 0.0, 0.0), (FOREARM_LEN + 10.0, 0.0, 0.0), 8.0, sides, frame, "drill_housing", caps=True)
        m.box((FOREARM_LEN + 3.0, 0.0, 7.0), (10.0, 4.0, 2.0), status, "drill_status_strip")
        steps = 4 if lod == 0 else 2
        for k in range(steps):
            x0 = FOREARM_LEN + 10.0 + k * (DRILL_LEN - 10.0) / steps
            x1 = FOREARM_LEN + 10.0 + (k + 1) * (DRILL_LEN - 10.0) / steps
            r = 6.0 - k * (4.5 / steps)
            m.tube((x0, 0.0, 0.0), (x1, 0.0, 0.0), r, sides, frame, "drill_bit", caps=(k == steps - 1))
        m.sockets.append(kit.Socket("Harvest_Tether_Muzzle", (FOREARM_LEN + DRILL_LEN, 0.0, 0.0), 0.0, "drill tip; harvest / Network Repair particle stream origin"))
    else:
        # Gripper: two fingers on a knuckle block, recessed palm welder between them.
        m.box((FOREARM_LEN + 3.0, 0.0, 0.0), (8.0, ARM_W + 4.0, ARM_W + 2.0), frame, "gripper_knuckle")
        for sy in (-1.0, 1.0):
            m.box((FOREARM_LEN + 7.0 + GRIPPER_LEN / 2.0, sy * 6.0, 0.0), (GRIPPER_LEN, 3.0, 6.0), frame, f"gripper_finger_{'r' if sy > 0 else 'l'}")
            if lod == 0:
                m.box((FOREARM_LEN + 7.0 + GRIPPER_LEN - 3.0, sy * 4.5, 0.0), (6.0, 2.5, 5.0), frame, f"gripper_tip_{'r' if sy > 0 else 'l'}")
        m.box((FOREARM_LEN + 9.0, 0.0, 0.0), (5.0, 4.0, 4.0), status, "palm_welder")
    return m


# --- body (torso, pelvis, mast, cradle) in world rest frame -----------------------------------
def build_body(lod: int) -> kit.Mesh:
    m = kit.Mesh("EBS_MER_UNT_001_Body")
    frame, ceramic, status = m.slot(FRAME), m.slot(CERAMIC), m.slot(STATUS_REVIEW)
    hi = lod == 0
    sides = 8 if hi else 6
    m.box((0.0, 0.0, HIP_Z - 3.0), PELVIS, frame, "pelvis")
    # Torso: charcoal frame core with a pale ceramic shell on the front, top and flanks.
    m.box((0.0, 0.0, TORSO_Z), (TORSO[0] - 6.0, TORSO[1] - 6.0, TORSO[2]), frame, "torso_frame")
    m.box((TORSO[0] / 2.0 - 3.0, 0.0, TORSO_Z + 2.0), (6.0, TORSO[1] - 8.0, TORSO[2] - 8.0), ceramic, "torso_shell_front")
    for sy in (-1.0, 1.0):
        m.box((2.0, sy * (TORSO[1] / 2.0 - 3.0), TORSO_Z), (TORSO[0] - 14.0, 6.0, TORSO[2] - 6.0), ceramic, "torso_shell_side")
    m.box((0.0, 0.0, TORSO_Z + TORSO[2] / 2.0 - 2.0), (TORSO[0] - 12.0, TORSO[1] - 12.0, 4.0), ceramic, "torso_shell_top")
    # Chest status band (cyan) and optic window.
    m.box((TORSO[0] / 2.0 + 0.5, 0.0, TORSO_Z - 8.0), (1.0, 34.0, 5.0), status, "chest_status_band")
    m.box((TORSO[0] / 2.0 + 0.5, 8.0, TORSO_Z + 8.0), (1.0, 10.0, 8.0), status, "optic_window")
    if hi:
        # Team band: a raised ceramic stripe across each shoulder plate (masked in the texture).
        for sy in (-1.0, 1.0):
            m.box((-2.0, sy * (TORSO[1] / 2.0 + 0.5), TORSO_Z + 16.0), (24.0, 1.0, 5.0), ceramic, "team_band")
        # Repaired shoulder patch (book paragraph 1606): a newer plate bolted over the right shoulder.
        m.box((4.0, TORSO[1] / 2.0 + 1.0, TORSO_Z + 6.0), (14.0, 2.0, 12.0), ceramic, "repair_patch")
    # Optical mast: post and sensor head.
    m.box((MAST_BASE[0], MAST_BASE[1], MAST_BASE[2] + MAST_H / 2.0), (6.0, 6.0, MAST_H), frame, "optical_mast")
    m.box((MAST_BASE[0] + 3.0, MAST_BASE[1], MAST_BASE[2] + MAST_H + MAST_HEAD[2] / 2.0), MAST_HEAD, ceramic, "optical_mast_head")
    m.box((MAST_BASE[0] + 3.0 + MAST_HEAD[0] / 2.0 + 0.5, MAST_BASE[1], MAST_BASE[2] + MAST_H + MAST_HEAD[2] / 2.0), (1.0, 6.0, 5.0), status, "optical_lens")
    # Rear cargo cradle: one transverse frame with exactly three canisters (GAP-01).
    m.box((CRADLE[0], CRADLE[1], CRADLE[2] - CANISTER_H / 2.0 - 2.0), (22.0, 3 * CANISTER_PITCH + 6.0, 4.0), frame, "cradle_floor")
    m.box((CRADLE[0] - 10.0, CRADLE[1], CRADLE[2]), (3.0, 3 * CANISTER_PITCH + 6.0, CANISTER_H + 6.0), frame, "cradle_back")
    for k, y in enumerate((-CANISTER_PITCH, 0.0, CANISTER_PITCH)):
        z0, z1 = CRADLE[2] - CANISTER_H / 2.0, CRADLE[2] + CANISTER_H / 2.0
        m.tube((CRADLE[0], y, z0), (CRADLE[0], y, z1), CANISTER_R, sides, ceramic, f"canister_{k + 1:02d}", caps=True)
        m.box((CRADLE[0] - CANISTER_R - 0.5, y, CRADLE[2]), (1.0, 6.0, CANISTER_H - 10.0), status, f"canister_{k + 1:02d}_window")
        if hi:
            m.tube((CRADLE[0], y, z1), (CRADLE[0], y, z1 + 3.0), CANISTER_R - 3.0, sides, frame, f"canister_{k + 1:02d}_cap", caps=True)
    m.sockets.append(kit.Socket("Cargo_Drop_Anchor", (CRADLE[0], 0.0, CRADLE[2]), 180.0, "cargo cradle centre; delivery latch effect and cargo visibility anchor"))
    m.sockets.append(kit.Socket("Center_Hitbox_Socket", (0.0, 0.0, TORSO_Z), 0.0, "damage acknowledgement and selection/health reference"))
    return m


# --- assembly ----------------------------------------------------------------------------------
BONE_OF_COMPONENT_PREFIX = {}


def assemble(lod: int):
    """Rest-stance mesh with every polygon bound to its bone, plus the skeleton."""
    p = rest_positions()
    s = build_skeleton()
    m = kit.Mesh(ASSET)
    for name in (FRAME, CERAMIC, STATUS_REVIEW):
        m.slot(name)
    body = build_body(lod)
    m.merge(body, component_prefix="body_")
    for side, sign in (("r", 1.0), ("l", -1.0)):
        hip = s.get(f"{side}_thigh").head
        knee = s.get(f"{side}_shin").head
        ankle = s.get(f"{side}_foot").head
        # limbs point down: local +X rotated by pitch -(90 - swing) about +Y
        m.merge(part_upper_leg(lod), translate=hip, pitch_deg=p["thigh_dir_pitch"], component_prefix=f"{side}_thigh_")
        m.merge(part_lower_leg(lod), translate=knee, pitch_deg=p["shin_dir_pitch"], component_prefix=f"{side}_shin_")
        m.merge(part_foot(lod), translate=ankle, component_prefix=f"{side}_foot_")
        shoulder = s.get(f"{side}_shoulder").head
        elbow = s.get(f"{side}_forearm").head
        m.merge(part_upper_arm(lod), translate=shoulder, pitch_deg=p["upper_dir_pitch"], yaw_deg=sign * 8.0, component_prefix=f"{side}_shoulder_")
        tool = "drill" if side == "r" else "gripper"
        m.merge(part_forearm(lod, tool), translate=elbow, pitch_deg=p["fore_dir_pitch"], component_prefix=f"{side}_forearm_", include_sockets=True)
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


# --- clips (Unreal rotators in degrees; seconds) --------------------------------------------------
def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def leg_cycle(clip, side, phase, amp=28.0, lift=40.0, body_pitch=None):
        # thigh swings forward (+pitch) then back; shin folds during the swing; foot stays level.
        # body_pitch(t) is the torso lean keyed on the body bone; the thigh counters it so the
        # stride is measured from the vertical, not from the leaning torso.
        T = clip.duration_s
        for i in range(0, 9):
            t = i / 8.0
            th = amp * math.cos(2 * math.pi * (t + phase))
            swing = max(0.0, math.sin(2 * math.pi * (t + phase)))  # >0 while the leg swings forward
            sh = -8.0 - lift * swing
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

    def dip(clip, t, pitch, thigh, shin, drop):
        # Knee-bend crouch with level soles: foot = -(thigh + shin); drop is the body translation that
        # returns the soles to z = 0 for that bend (derived numerically from the rest positions).
        clip.key("body", t, (pitch, 0.0, 0.0), (0.0, 0.0, drop))
        for side in ("r", "l"):
            clip.key(f"{side}_thigh", t, (-pitch + thigh, 0.0, 0.0))
            clip.key(f"{side}_shin", t, (shin, 0.0, 0.0))
            clip.key(f"{side}_foot", t, (-(thigh + shin), 0.0, 0.0))

    def rest(clip, t, bones=None):
        for b in bones or [b.name for b in skeleton.bones if b.name != "root"]:
            clip.key(b, t, (0.0, 0.0, 0.0))

    idle = skel.AnimationClip("idle", 2.0, loop=True, purpose="standing service frame: breathing lean, tools at rest, feet planted")
    for i, t in enumerate((0.0, 1.0, 2.0)):
        lean(idle, t, 1.0 if i == 1 else 0.0)
        idle.key("r_shoulder", t, (3.0 if i == 1 else 0.0, 0.0, 0.0))
        idle.key("l_shoulder", t, (3.0 if i == 1 else 0.0, 0.0, 0.0))
    clips.append(idle)

    move = skel.AnimationClip("move", 0.5, loop=True, purpose="walk cycle authored for 360 cm/s (two 90 cm strides); play rate scales with authoritative velocity; <= 2 cm vertical bob")
    move_pitch = lambda t: 1.5 * math.sin(4 * math.pi * t)  # noqa: E731
    leg_cycle(move, "r", 0.0, body_pitch=move_pitch)
    leg_cycle(move, "l", 0.5, body_pitch=move_pitch)
    for i in range(0, 9):
        t = i / 8.0
        bounce = 2.0 * abs(math.sin(2 * math.pi * t))
        move.key("body", t * 0.5, (move_pitch(t), 0.0, 2.0 * math.cos(2 * math.pi * t)), (0.0, 0.0, bounce))
        move.key("r_shoulder", t * 0.5, (-10.0 * math.cos(2 * math.pi * t), 0.0, 0.0))
        move.key("l_shoulder", t * 0.5, (10.0 * math.cos(2 * math.pi * t), 0.0, 0.0))
    clips.append(move)

    carry = skel.AnimationClip("carry", 0.5, loop=True, purpose="loaded walk: same cycle, body pitched forward, arms tucked; cargo visibility follows the authoritative load")
    carry_pitch = lambda t: -4.0 + 1.0 * math.sin(4 * math.pi * t)  # noqa: E731
    leg_cycle(carry, "r", 0.0, amp=24.0, lift=34.0, body_pitch=carry_pitch)
    leg_cycle(carry, "l", 0.5, amp=24.0, lift=34.0, body_pitch=carry_pitch)
    for i in range(0, 9):
        t = i / 8.0
        carry.key("body", t * 0.5, (carry_pitch(t), 0.0, 1.5 * math.cos(2 * math.pi * t)), (0.0, 0.0, 1.5 * abs(math.sin(2 * math.pi * t))))
        carry.key("r_shoulder", t * 0.5, (10.0, 0.0, 0.0))   # arms tucked: upper arm raised, forearm folded up
        carry.key("l_shoulder", t * 0.5, (10.0, 0.0, 0.0))
        carry.key("r_forearm", t * 0.5, (20.0, 0.0, 0.0))
        carry.key("l_forearm", t * 0.5, (20.0, 0.0, 0.0))
    clips.append(carry)

    turn = skel.AnimationClip("turn", 0.5, loop=True, purpose="stationary shuffle while the runtime sweeps heading (SPEC-MOV-010); no root yaw in the clip")
    for i, t in enumerate((0.0, 0.125, 0.25, 0.375, 0.5)):
        # torso sway in yaw only: a body roll would move the hips vertically (hip offset 21 cm) and
        # lift or sink the planted sole; yaw keeps every sole level while the feet shuffle.
        turn.key("body", t, (0.0, 3.0 if i % 2 else 0.0, 0.0))
        turn.key("r_thigh", t, (8.0 if i % 4 == 1 else 0.0, 0.0, 0.0))
        turn.key("l_thigh", t, (8.0 if i % 4 == 3 else 0.0, 0.0, 0.0))
        turn.key("r_shin", t, (-14.0 if i % 4 == 1 else 0.0, 0.0, 0.0))
        turn.key("l_shin", t, (-14.0 if i % 4 == 3 else 0.0, 0.0, 0.0))
        turn.key("r_foot", t, (6.0 if i % 4 == 1 else 0.0, 0.0, 0.0))   # level sole on the lifted shuffle step
        turn.key("l_foot", t, (6.0 if i % 4 == 3 else 0.0, 0.0, 0.0))
    clips.append(turn)

    stop = skel.AnimationClip("stop", 0.3, purpose="short settle on stop (Bible): forward lean and recover, feet planted")
    lean(stop, 0.0, -3.0)
    lean(stop, 0.15, 1.0)
    lean(stop, 0.3, 0.0)
    clips.append(stop)

    gather = skel.AnimationClip("gather", 1.0, loop=True, purpose="drill down at the deposit ahead of the feet (tip ~4-8 cm above ground, 80-88 cm forward); drill spin is a material effect gated by this state")
    for i, t in enumerate((0.0, 0.5, 1.0)):
        lean(gather, t, -8.0, stance_yaw=6.0)
        gather.key("r_shoulder", t, (40.0 + (2.0 if i == 1 else 0.0), -6.0, 0.0))
        gather.key("r_forearm", t, (-60.0 + (3.0 if i == 1 else 0.0), 0.0, 0.0))
        gather.key("l_shoulder", t, (-10.0, 8.0, 0.0))
    clips.append(gather)

    deliver = skel.AnimationClip("deliver", 0.8, purpose="cradle presentation: knees dip and the torso tilts back to present the rear cradle (the arms cannot reach it); cargo empties only on the authorized transfer event")
    rest(deliver, 0.0, ("body", "r_thigh", "l_thigh", "r_shin", "l_shin", "r_foot", "l_foot", "r_shoulder", "l_shoulder"))
    for t in (0.3, 0.5):
        dip(deliver, t, 8.0, 12.0, -22.0, DELIVER_DROP_CM)
        deliver.key("r_shoulder", t, (15.0, 0.0, 0.0))
        deliver.key("l_shoulder", t, (15.0, 0.0, 0.0))
    rest(deliver, 0.8, ("body", "r_thigh", "l_thigh", "r_shin", "l_shin", "r_foot", "l_foot", "r_shoulder", "l_shoulder"))
    clips.append(deliver)

    build = skel.AnimationClip("build", 1.0, loop=True, purpose="both arms extended forward to the footprint (Bible build pose), slight forward lean; welder effect on the left palm")
    for i, t in enumerate((0.0, 0.5, 1.0)):
        lean(build, t, -6.0)
        for side in ("r", "l"):
            build.key(f"{side}_shoulder", t, (38.0 + (3.0 if i == 1 else 0.0), 0.0, 0.0))
            build.key(f"{side}_forearm", t, (20.0 - (4.0 if i == 1 else 0.0), 0.0, 0.0))
    clips.append(build)

    repair = skel.AnimationClip("repair_when_authorized", 1.0, loop=True, purpose="welder arm only (Bible); the drill stays at rest")
    for i, t in enumerate((0.0, 0.5, 1.0)):
        lean(repair, t, -3.0)
        repair.key("l_shoulder", t, (34.0 + (3.0 if i == 1 else 0.0), -10.0, 0.0))
        repair.key("l_forearm", t, (16.0 - (3.0 if i == 1 else 0.0), 0.0, 0.0))
        repair.key("r_shoulder", t, (-6.0, 6.0, 0.0))
    clips.append(repair)

    damage = skel.AnimationClip("damage", 0.3, purpose="flinch on authoritative damage: torso jolts back, arms lift; no displacement")
    lean(damage, 0.0, 0.0); lean(damage, 0.1, 8.0, roll=2.0); lean(damage, 0.3, 0.0)
    for side in ("r", "l"):
        damage.key(f"{side}_shoulder", 0.0, (0.0, 0.0, 0.0)); damage.key(f"{side}_shoulder", 0.1, (12.0, 0.0, 0.0)); damage.key(f"{side}_shoulder", 0.3, (0.0, 0.0, 0.0))
    clips.append(damage)

    death = skel.AnimationClip("death", 1.0, purpose="engineered collapse: knees fold, the frame drops and settles back onto its cradle, tools splay to the ground; final pose held (cosmetic debris <= 200 ticks)")
    rest(death, 0.0)
    for t, f in ((0.4, 0.5), (1.0, 1.0)):
        death.key("body", t, (35.0 * f, 0.0, 10.0 * f), (0.0, 0.0, -54.0 * f))
        for side, sgn in (("r", 1.0), ("l", -1.0)):
            death.key(f"{side}_thigh", t, (55.0 * f, sgn * 10.0 * f, 0.0))
            death.key(f"{side}_shin", t, (-110.0 * f, 0.0, 0.0))
            death.key(f"{side}_foot", t, (30.0 * f, 0.0, 0.0))
            death.key(f"{side}_shoulder", t, (50.0 * f, sgn * 25.0 * f, 0.0))
            death.key(f"{side}_forearm", t, (-78.0 * f, 0.0, 0.0))
    clips.append(death)

    cancel = skel.AnimationClip("cancel", 0.3, purpose="tools return to rest from the work lean; drill spin stops on travel/cancel (GAP-01)")
    lean(cancel, 0.0, -6.0)
    for side in ("r", "l"):
        cancel.key(f"{side}_shoulder", 0.0, (30.0, 0.0, 0.0)); cancel.key(f"{side}_shoulder", 0.3, (0.0, 0.0, 0.0))
        cancel.key(f"{side}_forearm", 0.0, (15.0, 0.0, 0.0)); cancel.key(f"{side}_forearm", 0.3, (0.0, 0.0, 0.0))
    lean(cancel, 0.3, 0.0)
    clips.append(cancel)

    restore = skel.AnimationClip("restore", 0.0, purpose="single-frame rest pose used when reconstructing presentation from saved state; never replays a one-shot")
    for b in skeleton.bones:
        restore.key(b.name, 0.0, (0.0, 0.0, 0.0))
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
    manifest = {
        "author": AUTHOR, "creator": AUTHOR, "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "asset_name": ASSET,
        "revision": REVISION, "kit_revision": kit.KIT_REVISION, "skel_revision": skel.SKEL_REVISION, "stage": "BLOCKOUT",
        "stage_boundary": "Rest-stance geometry, 12-bone rig definition and keyframed clip data; skinned export only when the skeletal kit is verified; no textures, no import, no acceptance.",
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters at unit scale 1.0 (runtime PresentationScale 1.5 for workers)", "axes": "+X forward, +Y right, +Z up", "pivot": "ground-contact centre (root)"},
        "scale_basis": {"canon": "about as tall as a person and half again as wide at the shoulder (Bible line 509)", "existing_envelope": "welded placeholder ~180 cm tall; M01 parts thigh 42 / shin 46 / foot 32 cm",
                        "height_cm": m0.bounds()[1][2], "shoulder_width_cm": 2 * (SHOULDER[1] + ARM_W / 2.0 + 5.0), "status": "PROVISIONAL BLOCKOUT ESTIMATE"},
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": b.head, "purpose": b.purpose} for b in skeleton.bones], "rest_stance": rest_positions(),
                "policy": "identity rest orientation; hinge pitch about +Y at the joint; no decorative bones; drill spin via material; canisters rigid with the body and emptied by the authoritative transfer (visibility/material), not by animation"},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "loop": c.loop, "bones": sorted(c.tracks), "purpose": c.purpose} for c in clips],
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(), "lod0_cap": 4500, "lod1_cap": 1800,
                    "lod0_within_cap": m0.triangle_count() <= 4500, "lod1_within_cap": m1.triangle_count() <= 1800},
        "material_slots": [FRAME, CERAMIC], "material_slot_policy": "2 slots (contract max 2); status band, canister windows, lens and team colour are texture channels of the ceramic slot; the review OBJ keeps a third pseudo slot so renders show the cyan",
        "component_inventory": inventory,
        "outputs": outputs, "review": review,
        "tools": {"builder_sha256": builder_sha, "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "candidate_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/surveyor-review/surveyor-reference.png", "sha256": "98c3a0ef2300178942d30e0f2ff8d725ef94a51d38877639638c3cd90b6fd913", "status": "DERIVED_REFERENCE_FOR_REVIEW; the derived turnaround in the same folder was rejected (canister drift, reversed side labels)"},
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
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"], "inventory": {k: v.get("built") for k, v in inventory.items() if isinstance(v, dict)}, "outputs": len(outputs)}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
