#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-UNT-001 — the Kharuun Assemblies Tender.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md`: the tender-candidate sheet replaces the spider machine with a stocky
humanoid cultivator in living basalt strata — amber seam lines and wrist nodules, a forked resonance
staff, a woven sling of Matter across the back, bare feet. `REL-ART-007` governs the form language
(faceted basalt strata, hexagonal columns, translucent amber nodules, living root anchors) and makes
machined panels or industrial bolts a design failure, so nothing here is a plate or a fastener.

Usage:
  python3 build_tender.py --evidence-dir "<root>/EBS-KHA-UNT-001" [--skinned]
  python3 build_tender.py --evidence-dir "<root>/EBS-KHA-UNT-001" --check

Units: centimetres. +X forward, +Y right (anatomical right), +Z up. Root at the ground-contact centre.
Nanite off. No root motion: the circling walk of `grow` is authored in place.
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
PACKAGE_ID = "EBS-PKG-KA-TENDER"
PRODUCTION_ID = "EBS-KHA-UNT-001"
ASSET = "SK_EBS_KHA_UNT_001"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_001/"
REVISION = "ebs-kha-unt-001-concept-v1"

STRATA = "MI_EBS_KHA_Strata"     # faceted basalt body
FIBRE = "MI_EBS_KHA_Fibre"       # woven mineral-fibre sling, kilt wrap and belt
AMBER = "MI_EBS_KHA_Amber"       # seam lines, wrist nodules, staff bead (the only emissive)

H = 190.0                        # crown height, PROVISIONAL (concept-fidelity.md)
PELVIS_Z, SPINE_Z, CHEST_Z, HEAD_Z = 95.0, 112.0, 132.0, 158.0
HEAD_C, HEAD_SIZE = 172.0, (24.0, 22.0, 32.0)
CHEST_SIZE = (28.0, 40.0, 38.0)   # narrower than the shoulder span so the arms read clear of the torso
PELVIS_SIZE = (26.0, 42.0, 26.0)
SHOULDER_Y, SHOULDER_Z = 31.0, 148.0  # arms outboard of the chest; the concept's shoulders are broad
ELBOW_Z, WRIST_Z = 118.0, 88.0
UPPER_W, FORE_W = 13.0, 16.0     # the forearm is visibly heavier: "working strata"
HAND_SIZE = (12.0, 11.0, 18.0)
HIP_Y = 12.0
KNEE_Z, ANKLE_Z = 50.0, 10.0
THIGH_W, SHIN_W = 19.0, 16.0
FOOT_SIZE = (34.0, 17.0, 10.0)   # bare wide foot, sole on z = 0
STAFF_LEN = 180.5                # 0.95 H
STAFF_R = 3.4
SLING_NODULES = 5

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "ground contact centre; never carries simulation translation"),
    ("pelvis", "root", (0.0, 0.0, PELVIS_Z), "hips"),
    ("spine", "pelvis", (0.0, 0.0, SPINE_Z), "lower spine"),
    ("chest", "spine", (0.0, 0.0, CHEST_Z), "chest and shoulders; the sling rides here"),
    ("head", "chest", (0.0, 0.0, HEAD_Z), "faceted crown"),
    ("r_upper", "chest", (0.0, SHOULDER_Y, SHOULDER_Z), "right upper arm"),
    ("r_fore", "r_upper", (0.0, SHOULDER_Y + 4.0, ELBOW_Z), "right forearm, thickened working strata"),
    ("r_hand", "r_fore", (0.0, SHOULDER_Y + 6.0, WRIST_Z), "right hand; carries the staff"),
    ("l_upper", "chest", (0.0, -SHOULDER_Y, SHOULDER_Z), "left upper arm"),
    ("l_fore", "l_upper", (0.0, -SHOULDER_Y - 4.0, ELBOW_Z), "left forearm, thickened working strata"),
    ("l_hand", "l_fore", (0.0, -SHOULDER_Y - 6.0, WRIST_Z), "left hand"),
    ("r_thigh", "pelvis", (0.0, HIP_Y, PELVIS_Z - 3.0), "right thigh"),
    ("r_shin", "r_thigh", (0.0, HIP_Y, KNEE_Z), "right shin"),
    ("r_foot", "r_shin", (0.0, HIP_Y, ANKLE_Z), "right foot, bare sole"),
    ("l_thigh", "pelvis", (0.0, -HIP_Y, PELVIS_Z - 3.0), "left thigh"),
    ("l_shin", "l_thigh", (0.0, -HIP_Y, KNEE_Z), "left shin"),
    ("l_foot", "l_shin", (0.0, -HIP_Y, ANKLE_Z), "left foot, bare sole"),
    ("staff", "r_hand", (0.0, SHOULDER_Y + 8.0, WRIST_Z), "resonance staff, parented to the working hand"),
    ("sling", "chest", (-14.0, 0.0, CHEST_Z + 4.0), "woven sling and its Matter load; hidden when empty"),
]

SOCKETS = {
    "Harvest_Tether_Muzzle": ("staff", (0.0, SHOULDER_Y + 8.0, WRIST_Z + STAFF_LEN * 0.52), 0.0,
                              "staff head: the gather and grow effect origin"),
    "Cargo_Drop_Anchor": ("sling", (-20.0, 0.0, CHEST_Z + 6.0), 180.0,
                          "sling centre: Matter load reference; the load empties only on an authorised transfer"),
    "Center_Hitbox_Socket": ("chest", (0.0, 0.0, CHEST_Z + 6.0), 0.0, "torso centre"),
}


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def _limb(m, slot, comp, top, bottom, width, taper=1.0):
    """A faceted limb segment between two world points, square in section."""
    zc = (top[2] + bottom[2]) / 2.0
    length = top[2] - bottom[2]
    m.box(((top[0] + bottom[0]) / 2.0, (top[1] + bottom[1]) / 2.0, zc), (width * taper, width, length), slot, comp)


def build_body(lod: int, loaded: bool = True) -> kit.Mesh:
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    fibre = m.slot(FIBRE)
    amber = m.slot(AMBER)
    fine = lod == 0

    # torso: pelvis, chest, head — faceted blocks, broader at the shoulder than the hip
    m.box((0.0, 0.0, PELVIS_Z + 2.0), PELVIS_SIZE, strata, "pelvis_block")
    m.box((0.0, 0.0, (SPINE_Z + CHEST_Z) / 2.0 + 6.0), CHEST_SIZE, strata, "chest_block")
    m.box((0.0, 0.0, HEAD_Z - 10.0), (16.0, 18.0, 10.0), strata, "neck_shoulders")   # a real neck gap: the head must read
    m.box((0.0, 0.0, HEAD_C), HEAD_SIZE, strata, "head_block")
    m.box((0.0, 0.0, CHEST_Z + 16.0), (24.0, 52.0, 12.0), strata, "shoulder_yoke")   # the broad Kharuun shoulder line
    if fine:
        m.box((6.0, 0.0, HEAD_C + 4.0), (10.0, 20.0, 12.0), strata, "head_brow")
        for i, z in enumerate((CHEST_Z + 2.0, CHEST_Z + 14.0)):
            m.box((14.0, 0.0, z), (4.0, 34.0, 3.0), amber, f"chest_seam_{i + 1:02d}")

    # arms: upper arm, thickened forearm, hand; amber nodules at each wrist
    for side, sign in (("r", 1.0), ("l", -1.0)):
        _limb(m, strata, f"{side}_upper_arm", (0.0, sign * SHOULDER_Y, SHOULDER_Z), (0.0, sign * (SHOULDER_Y + 4.0), ELBOW_Z), UPPER_W)
        _limb(m, strata, f"{side}_forearm", (0.0, sign * (SHOULDER_Y + 4.0), ELBOW_Z), (0.0, sign * (SHOULDER_Y + 6.0), WRIST_Z), FORE_W)
        m.box((0.0, sign * (SHOULDER_Y + 6.0), WRIST_Z - HAND_SIZE[2] / 2.0), HAND_SIZE, strata, f"{side}_hand")
        m.box((0.0, sign * (SHOULDER_Y + 6.0), WRIST_Z + 5.0), (FORE_W + 3.0, FORE_W + 3.0, 7.0), fibre, f"{side}_wrist_cuff")
        if fine:
            for i in range(3):
                m.box((5.0, sign * (SHOULDER_Y + 6.0 + i * 0.0), WRIST_Z + 5.0 + (i - 1) * 2.4), (4.0, 5.0, 3.2), amber, f"{side}_wrist_nodule_{i + 1:02d}")
        m.box((0.0, sign * SHOULDER_Y, SHOULDER_Z + 3.0), (UPPER_W + 6.0, UPPER_W + 8.0, 12.0), strata, f"{side}_shoulder_cap")

    # legs: thigh, shin, bare wide foot with the sole on z = 0
    for side, sign in (("r", 1.0), ("l", -1.0)):
        _limb(m, strata, f"{side}_thigh", (0.0, sign * HIP_Y, PELVIS_Z - 3.0), (0.0, sign * HIP_Y, KNEE_Z), THIGH_W)
        _limb(m, strata, f"{side}_shin", (0.0, sign * HIP_Y, KNEE_Z), (0.0, sign * HIP_Y, ANKLE_Z), SHIN_W)
        m.box((4.0, sign * HIP_Y, FOOT_SIZE[2] / 2.0), FOOT_SIZE, strata, f"{side}_foot")
        if fine:
            m.box((0.0, sign * HIP_Y, KNEE_Z), (SHIN_W + 4.0, SHIN_W + 4.0, 8.0), strata, f"{side}_knee")

    # wrapped kilt and belt: woven fibre, not a plate
    m.box((0.0, 0.0, PELVIS_Z - 14.0), (30.0, 46.0, 30.0), fibre, "kilt_wrap")
    m.box((0.0, 0.0, PELVIS_Z + 3.0), (32.0, 48.0, 8.0), fibre, "belt")
    if fine:
        m.box((16.0, 14.0, PELVIS_Z - 2.0), (8.0, 12.0, 12.0), fibre, "belt_pouch")

    # woven sling across the back, and the Matter it carries (a separate component group)
    m.box((-16.0, 0.0, CHEST_Z + 6.0), (10.0, 30.0, 34.0), fibre, "sling_basket")
    m.box((2.0, 0.0, CHEST_Z + 16.0), (34.0, 12.0, 6.0), fibre, "sling_strap", yaw_deg=0.0)
    if loaded:
        for i in range(SLING_NODULES):
            a = -14.0 + i * 7.0
            m.box((-20.0, a, CHEST_Z + 20.0 + (i % 2) * 4.0), (9.0, 8.0, 9.0), strata, f"sling_matter_{i + 1:02d}")

    # resonance staff: a plain shaft with a two-pronged forked head and an amber bead in the fork
    sx, sy = 0.0, SHOULDER_Y + 8.0
    m.box((sx, sy, WRIST_Z + STAFF_LEN * 0.10), (STAFF_R * 2, STAFF_R * 2, STAFF_LEN * 0.86), strata, "staff_shaft")
    for i, dy in enumerate((-5.0, 5.0)):
        m.box((sx, sy + dy, WRIST_Z + STAFF_LEN * 0.55), (4.0, 4.0, 22.0), strata, f"staff_prong_{i + 1:02d}")
    m.box((sx, sy, WRIST_Z + STAFF_LEN * 0.50), (5.0, 6.0, 6.0), amber, "staff_bead")
    return m


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith("staff_"):
            by_component[c] = "staff"
        elif c.startswith("sling_"):
            by_component[c] = "sling"
        elif c.startswith("head_") or c == "neck_shoulders":
            by_component[c] = "head"
        elif c.startswith(("chest_", "belt", "kilt")) or c == "chest_block":
            by_component[c] = "chest" if c.startswith("chest_") else "pelvis"
        elif c == "pelvis_block":
            by_component[c] = "pelvis"
        else:
            for side in ("r", "l"):
                if c.startswith(f"{side}_upper_arm") or c.startswith(f"{side}_shoulder_cap"):
                    by_component[c] = f"{side}_upper"
                elif c.startswith(f"{side}_forearm") or c.startswith(f"{side}_wrist"):
                    by_component[c] = f"{side}_fore"
                elif c.startswith(f"{side}_hand"):
                    by_component[c] = f"{side}_hand"
                elif c.startswith(f"{side}_thigh"):
                    by_component[c] = f"{side}_thigh"
                elif c.startswith(f"{side}_shin") or c.startswith(f"{side}_knee"):
                    by_component[c] = f"{side}_shin"
                elif c.startswith(f"{side}_foot"):
                    by_component[c] = f"{side}_foot"
    return skel.bind_polygons(m, "pelvis", by_component)


def assemble(lod: int, loaded: bool = True):
    m = build_body(lod, loaded)
    s = build_skeleton()
    counts = bind(m)
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    sockets_on_bones = {n: v[0] for n, v in SOCKETS.items()}
    return m, s, counts, sockets_on_bones


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def new(name, duration, loop, purpose):
        c = skel.AnimationClip(name, duration, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    def lean(clip, t, pitch, roll=0.0):
        """Torso lean with the feet planted: the thighs counter the pelvis pitch."""
        clip.key("pelvis", t, (pitch, 0.0, roll))
        clip.key("r_thigh", t, (-pitch, 0.0, 0.0))
        clip.key("l_thigh", t, (-pitch, 0.0, 0.0))

    idle = new("idle", 2.0, True, "standing cultivator: slow breath, staff planted, feet flat")
    for i, t in enumerate((0.0, 1.0, 2.0)):
        lean(idle, t, 1.5 if i == 1 else 0.0)
        idle.key("chest", t, (1.0 if i == 1 else 0.0, 0.0, 0.0))
        idle.key("r_upper", t, (0.0, 0.0, 2.0 if i == 1 else 0.0))

    move = new("move", 0.6, True, "walk at 390 cm/s; play rate follows the authoritative velocity (REL-ART-009)")
    for i in range(0, 7):
        t = i / 6.0 * 0.6
        p = i / 6.0
        swing = 26.0 * math.cos(2 * math.pi * p)
        move.key("r_thigh", t, (swing, 0.0, 0.0))
        move.key("l_thigh", t, (-swing, 0.0, 0.0))
        move.key("r_shin", t, (-max(0.0, -swing) * 0.9 - 6.0, 0.0, 0.0))
        move.key("l_shin", t, (-max(0.0, swing) * 0.9 - 6.0, 0.0, 0.0))
        move.key("r_foot", t, (10.0, 0.0, 0.0))
        move.key("l_foot", t, (10.0, 0.0, 0.0))
        move.key("r_upper", t, (-swing * 0.35, 0.0, 0.0))
        move.key("l_upper", t, (swing * 0.35, 0.0, 0.0))
        move.key("pelvis", t, (2.0, 0.0, 1.5 * math.sin(2 * math.pi * p)), (0.0, 0.0, 1.5 * abs(math.sin(2 * math.pi * p))))

    turn = new("turn", 0.6, True, "stationary shuffle while the runtime sweeps heading; no root yaw")
    for i, t in enumerate((0.0, 0.2, 0.4, 0.6)):
        turn.key("pelvis", t, (0.0, 3.0 if i % 2 else 0.0, 0.0))
        turn.key("r_thigh", t, (7.0 if i == 1 else 0.0, 0.0, 0.0))
        turn.key("l_thigh", t, (7.0 if i == 3 else 0.0, 0.0, 0.0))
        turn.key("r_shin", t, (-12.0 if i == 1 else 0.0, 0.0, 0.0))
        turn.key("l_shin", t, (-12.0 if i == 3 else 0.0, 0.0, 0.0))
        turn.key("r_foot", t, (5.0 if i == 1 else 0.0, 0.0, 0.0))
        turn.key("l_foot", t, (5.0 if i == 3 else 0.0, 0.0, 0.0))

    stop = new("stop", 0.4, False, "short settle: a forward lean recovered, staff re-planted")
    lean(stop, 0.0, -4.0); lean(stop, 0.2, 1.5); lean(stop, 0.4, 0.0)

    # gather: the candidate's GATHERING panel — one knee down, both hands on the staff, pressed into
    # the strata ahead. KNEEL_* are solved numerically in gather_pose() so the down knee rests on the
    # ground and nothing pierces it.
    gather = new("gather", 1.2, True, "kneeling press of the staff into the strata (canon); the crumble is an effect")
    for i, t in enumerate((0.0, 0.6, 1.2)):
        push = 4.0 if i == 1 else 0.0
        # Solved analytically in the XZ plane (forward kinematics on the hip-knee-ankle chain) so the
        # trailing knee rests ON the ground, its foot lifts behind, and the leading sole is planted
        # flat 59 cm ahead: drop -40, trailing 16/94, leading -68/64 with a level foot.
        gather.key("pelvis", t, (-16.0, 0.0, 0.0), (0.0, 0.0, KNEEL_DROP))
        gather.key("chest", t, (-14.0 - push, 0.0, 0.0))
        gather.key("r_thigh", t, (16.0, 0.0, 0.0))
        gather.key("r_shin", t, (94.0, 0.0, 0.0))
        gather.key("r_foot", t, (0.0, 0.0, 0.0))
        gather.key("l_thigh", t, (-68.0, 0.0, 0.0))
        gather.key("l_shin", t, (64.0, 0.0, 0.0))
        gather.key("l_foot", t, (20.0, 0.0, 0.0))
        gather.key("r_upper", t, (48.0 + push, 0.0, 0.0))
        gather.key("r_fore", t, (-26.0, 0.0, 0.0))
        gather.key("l_upper", t, (44.0 + push, -14.0, 0.0))
        gather.key("l_fore", t, (-22.0, 0.0, 0.0))
        gather.key("staff", t, (-58.0, 0.0, 0.0))

    grow = new("grow", 1.6, True, "slow circling walk that leaves the organism's first ring (canon); authored in place")
    for i in range(0, 5):
        t = i / 4.0 * 1.6
        p = i / 4.0
        swing = 15.0 * math.cos(2 * math.pi * p)
        # +4 cm of ride height keeps the swinging sole clear of the ground through the cycle
        grow.key("pelvis", t, (-6.0, 4.0 * math.sin(2 * math.pi * p), 0.0), (0.0, 0.0, 4.0))
        grow.key("chest", t, (-5.0, -6.0, 0.0))
        grow.key("r_thigh", t, (swing + 6.0, 0.0, 0.0))
        grow.key("l_thigh", t, (-swing + 6.0, 0.0, 0.0))
        grow.key("r_shin", t, (-max(0.0, -swing) * 0.8 - 8.0, 0.0, 0.0))
        grow.key("l_shin", t, (-max(0.0, swing) * 0.8 - 8.0, 0.0, 0.0))
        grow.key("r_foot", t, (18.0, 0.0, 0.0))
        grow.key("l_foot", t, (18.0, 0.0, 0.0))
        grow.key("r_upper", t, (26.0, 0.0, 0.0))
        grow.key("staff", t, (-18.0, 0.0, 0.0))

    deliver = new("deliver", 0.8, False, "the sling is presented and its Matter released; the load empties only on the authorised transfer")
    for t, pitch, arm in ((0.0, 0.0, 0.0), (0.3, -14.0, 40.0), (0.5, -14.0, 40.0), (0.8, 0.0, 0.0)):
        lean(deliver, t, pitch)
        deliver.key("l_upper", t, (arm, -20.0 if arm else 0.0, 0.0))
        deliver.key("l_fore", t, (-arm * 0.6, 0.0, 0.0))

    stabilize = new("stabilize_scar", 1.6, True, "continuous channel over a scarred tile (SPEC-UNIT-005); staff held out, both hands")
    for i, t in enumerate((0.0, 0.8, 1.6)):
        lean(stabilize, t, -10.0)
        stabilize.key("chest", t, (-8.0, 0.0, 0.0))
        stabilize.key("r_upper", t, (54.0 + (3.0 if i == 1 else 0.0), 0.0, 0.0))
        stabilize.key("r_fore", t, (-18.0, 0.0, 0.0))
        stabilize.key("l_upper", t, (50.0, -16.0, 0.0))
        stabilize.key("l_fore", t, (-16.0, 0.0, 0.0))
        stabilize.key("staff", t, (-48.0, 0.0, 0.0))

    damage = new("damage", 0.4, False, "flinch: the frame rocks back, the staff arm lifts; no displacement")
    lean(damage, 0.0, 0.0); lean(damage, 0.1, 7.0); lean(damage, 0.4, 0.0)
    for b in ("r_upper", "l_upper"):
        damage.key(b, 0.0, (0.0, 0.0, 0.0)); damage.key(b, 0.1, (14.0, 0.0, 0.0)); damage.key(b, 0.4, (0.0, 0.0, 0.0))

    death = new("death", 1.2, False, "the cultivator folds to the ground and settles; held pose, cosmetic debris <= 200 ticks")
    # The fold is solved on the same XZ chain as `gather`: at the end the knees rest at z 9 and the
    # soles at z 14, so the body settles onto its side without a limb passing through the ground.
    # The drop LAGS the fold: dropping in step with it drives a half-folded leg through the ground.
    # (time, limb fold, pelvis drop, ankle): the ankle waits until the leg has lifted, or the sole
    # tips through the ground in the first tenth of a second, and the pelvis rises ~3 cm as the
    # body pivots off its feet before it drops (a negative drop factor).
    for t, f, d, ankle in ((0.0, 0.0, 0.0, 0.0), (0.25, 0.42, -0.06, 0.0), (0.6, 0.78, 0.30, 0.55), (1.2, 1.0, 1.0, 1.0)):
        death.key("pelvis", t, (18.0 * f, 0.0, 8.0 * f), (0.0, 0.0, -46.0 * d))
        death.key("chest", t, (-26.0 * f, 0.0, 0.0))
        death.key("head", t, (-18.0 * f, 0.0, 0.0))
        for side in ("r", "l"):
            death.key(f"{side}_thigh", t, (32.0 * f, 0.0, 0.0))
            death.key(f"{side}_shin", t, (45.0 * f, 0.0, 0.0))
            death.key(f"{side}_foot", t, (25.0 * ankle, 0.0, 0.0))
            death.key(f"{side}_upper", t, (30.0 * f, 0.0, 0.0))
            death.key(f"{side}_fore", t, (-20.0 * f, 0.0, 0.0))
        death.key("staff", t, (-96.0 * f, 0.0, 0.0))   # the staff falls flat beside the body

    cancel = new("cancel", 0.4, False, "work interrupted: the staff returns to rest from any work pose")
    for b, start in (("r_upper", 40.0), ("l_upper", 36.0), ("staff", -40.0), ("chest", -10.0)):
        cancel.key(b, 0.0, (start, 0.0, 0.0)); cancel.key(b, 0.4, (0.0, 0.0, 0.0))
    lean(cancel, 0.0, -8.0); lean(cancel, 0.4, 0.0)

    restore = new("restore", 0.0, False, "single-frame rest pose for reconstruction from saved state; never replays a one-shot")
    for b in skeleton.bones:
        if b.name == "root":
            continue          # no clip keys root: the runtime owns translation and facing
        restore.key(b.name, 0.0, (0.0, 0.0, 0.0))

    for c in clips:
        if not skel.is_frame_aligned(c.duration_s):
            skel.retime_clip(c, skel.frame_aligned_duration(c.duration_s))
    return clips


# The kneel drop is solved once, numerically, so the down knee rests on the ground in `gather`.
KNEEL_DROP = -40.0


def sample_pose(clip: skel.AnimationClip, fraction: float) -> dict:
    """Linear sample of a clip at a normalized time — the interpolation the glTF samplers use."""
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
            k = keys[-1]
            for a, b in zip(keys, keys[1:]):
                if a.time_s <= t <= b.time_s:
                    f = 0.0 if b.time_s == a.time_s else (t - a.time_s) / (b.time_s - a.time_s)
                    pose[bone] = tuple(a.rotation_deg[i] + (b.rotation_deg[i] - a.rotation_deg[i]) * f for i in range(3)) + \
                                 tuple(a.translation_cm[i] + (b.translation_cm[i] - a.translation_cm[i]) * f for i in range(3))
                    break
            continue
        pose[bone] = (*k.rotation_deg, *k.translation_cm)
    return pose


def posed(lod: int, pose: dict, loaded: bool = True) -> kit.Mesh:
    m, s, _c, socks = assemble(lod, loaded)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("idle", 0.5), ("move", 0.25), ("move", 0.5), ("turn", 0.25), ("stop", 0.0),
                ("gather", 0.5), ("grow", 0.25), ("deliver", 0.5), ("stabilize_scar", 0.5),
                ("damage", 0.25), ("death", 1.0), ("cancel", 0.0)]



def slot_area_fraction(m: kit.Mesh, slot_name: str) -> float:
    """Share of the mesh's SURFACE AREA carried by one slot.

    The asset cards cap emissive by mesh surface area, not by triangle count. On a blockout this
    distinction matters: the body is a handful of large boxes while the amber seams and nodules are
    many small ones, so the triangle share (20%) is four times the area share.
    """
    total = 0.0
    per = 0.0
    index = m.slots.index(slot_name) if slot_name in m.slots else None
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

def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "humanoid_body": {"contract": 1, "built": 1 if "chest_block" in comps else 0,
                          "parts": ["pelvis_block", "chest_block", "head_block"]},
        "thickened_forearms": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_forearm"))},
        "wrist_nodules": {"contract": 2, "built": len({c[:1] for c in comps if "_wrist_nodule_" in c})},
        "resonance_staff": {"contract": 1, "built": 1 if "staff_shaft" in comps else 0,
                            "forked_head": sum(1 for c in comps if c.startswith("staff_prong_")), "bead": "staff_bead" in comps},
        "woven_sling": {"contract": 1, "built": 1 if "sling_basket" in comps else 0,
                        "matter_nodules": sum(1 for c in comps if c.startswith("sling_matter_"))},
        "kilt_wrap": {"contract": 1, "built": 1 if "kilt_wrap" in comps else 0},
        "bare_feet": {"contract": 2, "built": sum(1 for c in comps if c.endswith("_foot"))},
        "bones": {"built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": [sk.name for sk in m.sockets]},
        "tracks": {"contract": ["idle", "move", "turn", "stop", "gather", "grow", "deliver", "stabilize_scar",
                                "damage", "death", "cancel", "restore"], "built": [c.name for c in clips]},
        "forbidden": {"rule": "REL-ART-007.FAIL: machined metallic panels or industrial bolts fail the Kharuun rules",
                      "panel_or_bolt_components": [c for c in comps if any(w in c for w in ("panel", "bolt", "plate", "rivet"))]},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    shoulder = m.component_bounds("neck_shoulders")
    return {
        "height_cm": round(z1, 2), "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        # the concept's "shoulder width" is the body's shoulder line (the yoke), not the span across
        # the arms; both are reported so neither number can stand in for the other by accident
        "shoulder_width_cm": round(m.component_bounds("shoulder_yoke")[1][1] - m.component_bounds("shoulder_yoke")[0][1], 2),
        "shoulder_width_over_H": round((m.component_bounds("shoulder_yoke")[1][1] - m.component_bounds("shoulder_yoke")[0][1]) / H, 4),
        "arm_span_cm": round(m.component_bounds("r_shoulder_cap")[1][1] - m.component_bounds("l_shoulder_cap")[0][1], 2),
        "head_height_over_H": round(HEAD_SIZE[2] / H, 4),
        "leg_length_over_H": round((PELVIS_Z - 3.0) / H, 4),
        "staff_length_over_H": round(STAFF_LEN / H, 4),
        "feet_on_the_ground": abs(z0) < 1e-6,
        "_shoulder_block": [round(v, 2) for v in shoulder[1]],
    }


def export(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, _c, socks = assemble(lod, True)
        base = os.path.join(out_dir, f"{ASSET}_LOD{lod}")
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod, "author": AUTHOR}
        if skinned:
            digest = skel.write_skinned_glb(m, s, base + ".glb", animations=clips, extras=extras,
                                            include_collision=False, sockets_on_bones=socks)
            outputs.append({"mesh": ASSET, "lod": lod, "kind": "skinned + clips", "path": os.path.relpath(base + ".glb", HERE),
                            "sha256": digest, "triangles": m.triangle_count(), "bounds_cm": [list(p) for p in m.bounds()],
                            "section_slot_names": m.slots, "by_slot": m.triangle_count_by("slot"),
                            "clips": [c.name for c in clips],
                            "sockets": [{"name": sk.name, "bone": socks[sk.name], "position_cm": [round(v, 2) for v in sk.position],
                                         "yaw_deg": sk.yaw_deg, "purpose": sk.purpose} for sk in m.sockets]})
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose", f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ", "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for label, loaded in (("rest", True), ("rest_unloaded", False)):
        m, s, _c, socks = assemble(0, loaded)
        path = os.path.join(review, f"{ASSET}_{label}_LOD0.obj")
        review_rows.append({"name": label, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {label}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count()})
    by_name = {c.name: c for c in clips}
    for name, fraction in POSE_SAMPLES:
        mesh = posed(0, sample_pose(by_name[name], fraction))
        path = os.path.join(review, f"pose_{name}_{int(fraction * 100):03d}.obj")
        review_rows.append({"name": f"pose_{name}_{fraction:.2f}", "path": os.path.relpath(path, evidence_dir),
                            "sha256": mesh.write_obj(path, header_lines=[f"{ASSET} posed: {name} at {fraction:.2f}"]),
                            "lowest_z_cm": round(mesh.bounds()[0][2], 2)})
    m1 = assemble(1, True)[0]
    path = os.path.join(review, f"{ASSET}_rest_LOD1.obj")
    review_rows.append({"name": "rest_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]), "triangles": m1.triangle_count()})
    return {"outputs": outputs, "review": review_rows, "clips": clips}


def manifest(exported: dict) -> dict:
    m0, s, counts, _socks = assemble(0, True)
    m1 = assemble(1, True)[0]
    clips = exported["clips"]
    emissive = slot_area_fraction(m0, AMBER)
    emissive_tris = m0.triangle_count_by("slot").get(AMBER, 0) / max(1, m0.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"), "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right (anatomical right), +Z up",
                  "pivot": "ground-contact centre", "nanite": False},
        "scale_basis": {"canon": "SPEC-UNIT-005 (Bible line 551): a stocky Kharuun cultivator with a resonance staff and a woven sling",
                        "height_cm": H, "status": "PROVISIONAL; no Tender asset card exists (README section 8)"},
        "material_slots": [STRATA, FIBRE, AMBER],
        "material_slot_policy": "Faceted basalt strata, woven mineral fibre, amber seams and nodules. No machined panel or bolt anywhere (REL-ART-007).",
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "lod0_cap": 4500, "lod1_cap": 1800,
                    "cap_source": "No Tender card exists; bounded by the equivalent worker card REL-FAC-025.MC.SURVEYOR.ASSET (README section 8, OWNER-QUESTION A)",
                    "lod0_within_cap": m0.triangle_count() <= 4500, "lod1_within_cap": m1.triangle_count() <= 1800,
                    "emissive_area_fraction_lod0": round(emissive, 5), "emissive_cap": 0.05,
                    "emissive_triangle_fraction_lod0": round(emissive_tris, 5),
                    "emissive_measure": "surface area, as the card states; the triangle share is reported alongside because a blockout's small amber boxes inflate it",
                    "emissive_within_cap": emissive <= 0.05},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": "No root motion; the runtime owns facing and translation (REL-ART-009, SPEC-MOV-010)."},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "loop": c.loop, "purpose": c.purpose,
                   "bones": sorted(c.tracks)} for c in clips],
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "technical": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/tender-review/tender-candidate.png",
                            "concept": "EBS-CON-KHA-UNT-001 (REPLACE)", "canon": "DevelopmentBible.md line 551 (SPEC-UNIT-005)",
                            "gameplay": "Content/Data/Source/units.json ka_tender"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {STRATA: [0.27, 0.255, 0.24], FIBRE: [0.34, 0.30, 0.25], AMBER: [0.98, 0.62, 0.18],
                          "_default": [0.5, 0.5, 0.5]},
            "emissive": [AMBER], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.32, "key": 0.85, "color": [1.0, 0.86, 0.7],
                      "fill_color": [0.5, 0.55, 0.7], "fill": 0.24},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [200, 200], "footprint_color": [0.98, 0.62, 0.18]},
            "reference_figure": {"height_cm": 180, "position": [-150, 150, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15, "target": [0, 0, 95]},
             {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.15, "target": [0, 0, 95]},
             {"name": "rear", "type": "ortho", "from": "-X", "edges": True, "margin": 1.15, "target": [0, 0, 95]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.3, "target": [0, 0, 0]}]
    tactical = [{"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
                {"name": "tactical_near", "type": "persp", "pitch_deg": -34, "yaw_deg": 135, "arm_cm": 520, "fov_deg": 50, "target": [0, 0, 95]}]
    written = []
    def dump(name, meshes, views):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = views
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)
    dump("rest", [{"obj": f"../review/{ASSET}_rest_LOD0.obj"}], ortho + tactical)
    dump("rest_unloaded", [{"obj": f"../review/{ASSET}_rest_unloaded_LOD0.obj"}], [ortho[2], tactical[1]])
    dump("lod1", [{"obj": f"../review/{ASSET}_rest_LOD1.obj"}], ortho[:2] + [tactical[0]])
    for name, fraction in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [ortho[1], ortho[0], tactical[1]])
    return written


def build_all(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    return manifest(export(evidence_dir, out_dir, skinned))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--skinned", action="store_true", help="also write the skinned GLBs with the clips")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            ev = os.path.join(tmp, "evidence"); os.makedirs(ev, exist_ok=True)
            fresh = build_all(ev, os.path.join(tmp, "export"), True)
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
                          "compared": {"outputs": len(fresh["outputs"]), "review": len(fresh["review_assemblies"])}}))
        return 0 if not drift and not missing else 1
    os.makedirs(args.evidence_dir, exist_ok=True)
    data = build_all(args.evidence_dir, os.path.join(HERE, "export"), args.skinned)
    scenes = write_scenes(args.evidence_dir)
    data["review"] = {"scenes": [os.path.relpath(s, args.evidence_dir) for s in scenes]}
    with open(manifest_path, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=1, sort_keys=True)
    lows = [r["lowest_z_cm"] for r in data["review_assemblies"] if "lowest_z_cm" in r]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"], "lod1": data["budgets"]["lod1_triangles"],
                      "height_cm": data["concept_measurements"]["height_cm"], "bones": len(data["rig"]["bones"]),
                      "clips": len(data["clips"]), "emissive": data["budgets"]["emissive_area_fraction_lod0"],
                      "lowest_posed_z_cm": min(lows) if lows else None, "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
