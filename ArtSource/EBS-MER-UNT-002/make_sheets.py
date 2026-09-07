#!/usr/bin/env python3
"""Compose the Lancer concept-comparison sheets with exact, recorded ebs_sheet.py commands.

Author: Angelis Pseftis. Usage: python3 make_sheets.py --evidence-root <evidence root>
Every sheet's tile order, --cols, --cell and --crop are fixed here so the sheet hashes reproduce
from the record alone (README section 7); the command lines are printed as they run. The crop is
applied to the FIRST tile only (the concept panel). Standard library only; ../tools/ebs_sheet.py
does the compositing.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SHEET = os.path.join(os.path.dirname(HERE), "tools", "ebs_sheet.py")
PACKAGE = "EBS-MER-UNT-002"
CONCEPT_DIR = "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/lancer-review"
CANDIDATE = os.path.join(CONCEPT_DIR, "lancer-candidate.png")
TURNAROUND = os.path.join(CONCEPT_DIR, "lancer-derived-turnaround.png")

# normalized crops measured on the two concept sheets (1536x1024 each)
CROP_MAIN = "0.10,0.10,0.60,0.95"        # candidate: main braced view
CROP_SIDE = "0.57,0.09,1.00,0.58"        # candidate: braced side view
CROP_SIL = "0.62,0.58,0.98,0.95"         # candidate: tactical silhouette
CROP_T_FRONT = "0.10,0.07,0.46,0.52"     # turnaround: FRONT panel
CROP_T_LEFT = "0.52,0.09,0.90,0.51"      # turnaround: LEFT SIDE panel
CROP_T_REAR = "0.08,0.52,0.42,0.94"      # turnaround: REAR panel
CROP_T_TOP = "0.50,0.55,0.97,0.87"       # turnaround: TOP panel
CROP_T_INSET = "0.76,0.10,1.00,0.27"     # turnaround: HIP MOUNT / LEG ATTACHMENT inset

POSE_ORDER = ["pose_idle", "pose_move_contact", "pose_move_stance", "pose_move_pass", "pose_move_swing",
              "pose_turn_lift", "pose_stop_travel", "pose_stop_plant", "pose_fire_halt", "pose_fire_plant",
              "pose_fire_aim", "pose_fire_shot", "pose_fire_recover", "pose_damage", "pose_death_fold",
              "pose_death_hold", "pose_cancel_start"]


def sheets(root: str):
    r = os.path.join(root, PACKAGE, "renders")
    out = os.path.join(r, "concept-compare")
    return [
        ("candidate_side_vs_right.png", 2, 800, CROP_SIDE,
         [CANDIDATE, f"{r}/rest/right.png", f"{r}/pose_fire_aim/right.png", f"{r}/rest/left.png"]),
        ("candidate_main_vs_tactical.png", 2, 800, CROP_MAIN,
         [CANDIDATE, f"{r}/rest_tactical/tactical_front_quarter.png", f"{r}/rest_tactical/tactical_near.png", f"{r}/rest_tactical/tactical_default.png"]),
        ("candidate_silhouette_vs_tactical.png", 2, 800, CROP_SIL,
         [CANDIDATE, f"{r}/rest_tactical/tactical_mono_near.png", f"{r}/rest_tactical/tactical_front_quarter.png", f"{r}/context/tactical_near.png"]),
        ("turnaround_left_vs_right.png", 2, 800, CROP_T_LEFT,
         [TURNAROUND, f"{r}/rest/right.png", f"{r}/rest/left.png", f"{r}/pose_idle/right.png"]),
        ("turnaround_front_vs_front.png", 2, 800, CROP_T_FRONT,
         [TURNAROUND, f"{r}/rest/front.png", f"{r}/pose_fire_aim/front.png", f"{r}/scale_vs_surveyor/front.png"]),
        ("turnaround_rear_vs_rear.png", 2, 800, CROP_T_REAR,
         [TURNAROUND, f"{r}/rest/rear.png", f"{r}/rest/left.png", f"{r}/pose_fire_shot/right.png"]),
        ("turnaround_top_vs_top.png", 2, 800, CROP_T_TOP,
         [TURNAROUND, f"{r}/rest/top.png", f"{r}/pose_fire_aim/top.png", f"{r}/pose_move_stance/top.png"]),
        ("turnaround_inset_vs_mount.png", 2, 800, CROP_T_INSET,
         [TURNAROUND, f"{r}/rest/right.png", f"{r}/pose_fire_shot/right.png", f"{r}/pose_fire_plant/right.png"]),
        ("fire_sequence_right.png", 3, 620, None,
         [f"{r}/pose_fire_halt/right.png", f"{r}/pose_fire_plant/right.png", f"{r}/pose_fire_aim/right.png",
          f"{r}/pose_fire_shot/right.png", f"{r}/pose_fire_recover/right.png", f"{r}/pose_cancel_start/right.png"]),
        ("pose_sheet_right.png", 5, 460, None, [f"{r}/{p}/right.png" for p in POSE_ORDER]),
        ("pose_sheet_tactical_near.png", 5, 460, None, [f"{r}/{p}/tactical_near.png" for p in POSE_ORDER]),
        ("lod0_vs_lod1.png", 2, 700, None,
         [f"{r}/rest/front.png", f"{r}/lod1/front.png", f"{r}/rest/right.png", f"{r}/lod1/right.png"]),
        ("context_and_scale.png", 2, 800, None,
         [f"{r}/context/tactical_near.png", f"{r}/context/tactical_gameplay.png",
          f"{r}/scale_vs_surveyor/right.png", f"{r}/pose_death_hold_tactical/tactical_gameplay.png"]),
    ], out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", required=True)
    parser.add_argument("--dry-run", action="store_true", help="print the commands only")
    args = parser.parse_args()
    plan, out_dir = sheets(os.path.abspath(args.evidence_root))
    os.makedirs(out_dir, exist_ok=True)
    for name, cols, cell, crop, images in plan:
        cmd = [sys.executable, SHEET, "--out", os.path.join(out_dir, name), "--cols", str(cols), "--cell", str(cell)]
        if crop:
            cmd += ["--crop", crop]
        cmd += images
        print(" ".join(f'"{c}"' if " " in c else c for c in cmd))
        if not args.dry_run:
            subprocess.run(cmd, check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
