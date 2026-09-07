#!/usr/bin/env python3
"""Compose the Surveyor concept-comparison sheets with exact, recorded ebs_sheet.py commands.

Author: Angelis Pseftis. Usage: python3 make_sheets.py --evidence-root <evidence root>
Every sheet's tile order, --cols, --cell and --crop are fixed here so the sheet hashes reproduce
from the record alone (README section 7); the command lines are printed as they run.
Standard library only; ebs_sheet.py (../tools) does the compositing.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SHEET = os.path.join(os.path.dirname(HERE), "tools", "ebs_sheet.py")
PACKAGE = "EBS-MER-UNT-001"
REAR_CROP, GATHER_CROP, DELIVERY_CROP = "0.60,0.02,1.0,0.31", "0.60,0.31,1.0,0.63", "0.60,0.63,1.0,0.95"
POSE_ORDER = ["pose_move_025", "pose_move_050", "pose_carry_025", "pose_turn_025", "pose_stop_000", "pose_gather_050", "pose_build_050",
              "pose_repair_when_authorized_050", "pose_deliver_050", "pose_damage_033", "pose_death_100", "pose_cancel_000"]


def sheets(root: str) -> list:
    r = os.path.join(root, PACKAGE, "renders")
    concept = os.path.join(root, "concept-crops", "surveyor_concept_2x.png")
    reference = os.path.join(root, "concept-crops", "surveyor-reference.png")
    out = os.path.join(r, "concept-compare")
    return [
        ("concept_vs_rest.png", 2, 800, None, [concept, f"{r}/rest/front.png", f"{r}/rest/right.png", f"{r}/rest_tactical/tactical_near.png"]),
        ("concept_vs_tactical.png", 2, 800, None, [concept, f"{r}/rest_tactical/tactical_gameplay.png", f"{r}/rest_tactical/tactical_default.png", f"{r}/rest_tactical/tactical_front_quarter.png"]),
        ("reference_rear_vs_rear_top.png", 2, 800, REAR_CROP, [reference, f"{r}/rest/rear.png", f"{r}/rest/top.png", f"{r}/rest/left.png"]),
        ("reference_gathering_vs_gather.png", 2, 800, GATHER_CROP, [reference, f"{r}/pose_gather_050/right.png", f"{r}/pose_gather_050/tactical_near.png", f"{r}/pose_gather_050_tactical/tactical_gameplay.png"]),
        ("reference_delivery_vs_deliver.png", 2, 800, DELIVERY_CROP, [reference, f"{r}/pose_deliver_050/right.png", f"{r}/pose_deliver_050_unloaded/right.png", f"{r}/pose_deliver_050_unloaded/tactical_near.png"]),
        ("reference_delivery_vs_deliver_rear.png", 2, 800, DELIVERY_CROP, [reference, f"{r}/pose_deliver_050_unloaded/front.png", f"{r}/rest/rear.png", f"{r}/pose_deliver_050_unloaded_tactical/tactical_near.png"]),
        ("pose_sheet_right.png", 4, 480, None, [f"{r}/{p}/right.png" for p in POSE_ORDER]),
        ("pose_sheet_tactical_near.png", 4, 480, None, [f"{r}/{p}/tactical_near.png" for p in POSE_ORDER]),
        ("lod0_vs_lod1.png", 2, 640, None, [f"{r}/rest/front.png", f"{r}/lod1/front.png", f"{r}/rest/right.png", f"{r}/lod1/right.png"]),
        ("context_and_death.png", 2, 800, None, [f"{r}/context/tactical_gameplay.png", f"{r}/context/tactical_near.png", f"{r}/pose_death_100_tactical/tactical_gameplay.png", f"{r}/pose_build_050_tactical/tactical_gameplay.png"]),
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
