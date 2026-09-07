#!/usr/bin/env python3
"""Compose the EBS-MER-UNT-003 concept-comparison sheets with exact, recorded ebs_sheet.py commands.

Author: Angelis Pseftis.
Usage: python3 make_sheets.py --evidence-root <evidence root> [--dry-run]

Every sheet's tile order, --cols, --cell and --crop are fixed here so the sheet hashes reproduce
from the record alone (README section 7); the command lines are printed as they run.

Two rules govern the tiles, both of them enforced by review:
  * the first tile of every fidelity-check sheet (check1..check5b), of concept_identity and of
    owner_correction_centred is the concept crop or the owner-corrected reference, cropped to the
    panel the following views are judged against. The supplementary sheets (deploy_sequence,
    pose_sheet_*, lod0_vs_lod1, presentation_scale, states_and_context) lead with the build's own
    renders and are not fidelity evidence;
  * every fidelity check is judged on the SHIPPED review assembly (`deployed` / `packed`). The two
    baked material states - `packed_field_dark` (field and pane rims re-slotted to charcoal, no
    geometry removed) and `deployed_glass_cutaway` (field polygons cut, a stand-in for translucency)
    - only ever appear as extra tiles beside the shipped one, never in its place.
Standard library only.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SHEET = os.path.join(os.path.dirname(HERE), "tools", "ebs_sheet.py")
PACKAGE = "EBS-MER-UNT-003"
REVIEW_DIR = "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/bulwark-review"
REFERENCE = os.path.join(REVIEW_DIR, "bulwark-centered-wings-reference.png")
SIX_PANEL = os.path.join(REVIEW_DIR, "bulwark-six-panel-reference.png")
# Panel crops measured on the 1536x1024 reference sheet.
CROP_DEPLOYED = "0.09,0.21,0.66,0.88"
CROP_TRAVEL = "0.645,0.06,1.0,0.46"
CROP_REAR = "0.61,0.50,1.0,0.93"

POSES = ["pose_idle_packed_000", "pose_move_packed_025", "pose_deploy_015", "pose_deploy_055", "pose_deploy_082",
         "pose_idle_deployed_050", "pose_drag_deployed_025", "pose_pack_055", "pose_damage_020",
         "pose_death_100", "pose_cancel_000", "pose_restore_000"]


def sheets(root: str):
    r = os.path.join(root, PACKAGE, "renders")
    concept = os.path.join(root, "concept-crops", "all", "EBS-PKG-MC-BULWARK-TEAM", "EBS-CON-MER-UNT-003.png")
    out = os.path.join(r, "concept-compare")
    return [
        # 1. deployed front: one flat face of six cells, three per side, seam on the centreline.
        #    Tile 2 is the SHIPPED mesh; tile 3 is the translucency stand-in (field cut).
        ("check1_deployed_front.png", 2, 760, CROP_DEPLOYED,
         [REFERENCE, f"{r}/deployed/front.png", f"{r}/deployed_glass_cutaway/front.png", f"{r}/deployed/top.png"]),
        # 2. deployed side: face forward of the chassis, wall taller than the operators
        ("check2_deployed_side.png", 2, 760, CROP_DEPLOYED,
         [REFERENCE, f"{r}/deployed/right.png", f"{r}/deployed_glass_cutaway/right.png", f"{r}/deployed_tactical/tactical_front_quarter.png"]),
        # 3. packed three-quarter: folded frames along the chassis sides, compact crawler.
        #    Row 2 is the SHIPPED packed mesh; row 3 is the runtime state (field material off).
        ("check3_packed_travel.png", 2, 700, CROP_TRAVEL,
         [REFERENCE, f"{r}/packed_tactical/concept_travel.png",
          f"{r}/packed/front.png", f"{r}/packed/right.png",
          f"{r}/packed_field_dark_tactical/concept_travel.png", f"{r}/packed_field_dark/right.png"]),
        # 4. rear in both states: open rear access, no shield, no cyan face. Every tile is a
        #    SHIPPED mesh - the rear read is now geometry (the pane back plates), not a material.
        ("check4_rear_both_states.png", 2, 700, CROP_REAR,
         [REFERENCE, f"{r}/deployed/rear.png", f"{r}/packed/rear.png", f"{r}/deployed_lod1/rear.png",
          f"{r}/packed_tactical/concept_rear.png", f"{r}/packed_field_dark_tactical/concept_rear.png"]),
        # 5. tactical framing: facing and deployed footprint unmistakable, the rear says "flank me"
        ("check5_tactical_read.png", 2, 700, CROP_TRAVEL,
         [REFERENCE, f"{r}/deployed_tactical_x175/tactical_gameplay.png", f"{r}/packed_tactical_x175/tactical_gameplay.png",
          f"{r}/deployed_tactical/tactical_near.png", f"{r}/packed_tactical/tactical_near.png",
          f"{r}/packed_field_dark_tactical/tactical_near.png"]),
        ("check5_tactical_context.png", 2, 700, CROP_TRAVEL,
         [REFERENCE, f"{r}/context/tactical_near.png", f"{r}/context/tactical_gameplay.png",
          f"{r}/context/tactical_mono.png", f"{r}/context/tactical_flank.png", f"{r}/context/tactical_default.png"]),
        # design identity against the KEEP concept
        ("concept_identity.png", 2, 760, None,
         [concept, f"{r}/deployed_tactical/tactical_near.png", f"{r}/packed_tactical/concept_travel.png",
          f"{r}/deployed_glass_cutaway_tactical/tactical_near.png"]),
        # the earlier derived step (wall off-centre) beside the centred build, for the owner correction
        ("owner_correction_centred.png", 2, 760, CROP_DEPLOYED,
         [SIX_PANEL, f"{r}/deployed/front.png", f"{r}/deployed_glass_cutaway/front.png", f"{r}/deployed/rear.png"]),
        ("deploy_sequence.png", 3, 560, None,
         [f"{r}/pose_idle_packed_000/front.png", f"{r}/pose_deploy_015/front.png", f"{r}/pose_deploy_055/front.png",
          f"{r}/pose_deploy_082/front.png", f"{r}/pose_idle_deployed_050/front.png", f"{r}/pose_pack_055/front.png"]),
        ("deploy_sequence_quarter.png", 3, 560, None,
         [f"{r}/pose_idle_packed_000/concept_travel.png", f"{r}/pose_deploy_015/concept_travel.png", f"{r}/pose_deploy_055/concept_travel.png",
          f"{r}/pose_deploy_082/concept_travel.png", f"{r}/pose_idle_deployed_050/concept_travel.png", f"{r}/pose_pack_055/concept_travel.png"]),
        ("pose_sheet_right.png", 4, 480, None, [f"{r}/{p}/right.png" for p in POSES]),
        ("pose_sheet_tactical_near.png", 4, 480, None, [f"{r}/{p}/tactical_near.png" for p in POSES]),
        ("lod0_vs_lod1.png", 2, 700, None,
         [f"{r}/deployed/front.png", f"{r}/deployed_lod1/front.png", f"{r}/packed/right.png", f"{r}/packed_lod1/right.png"]),
        ("presentation_scale.png", 2, 760, None,
         [f"{r}/deployed_tactical/tactical_gameplay.png", f"{r}/deployed_tactical_x175/tactical_gameplay.png",
          f"{r}/packed_tactical/tactical_gameplay.png", f"{r}/packed_tactical_x175/tactical_gameplay.png"]),
        ("states_and_context.png", 2, 760, None,
         [f"{r}/deployed/front.png", f"{r}/packed/front.png",
          f"{r}/context/tactical_default.png", f"{r}/context/tactical_flank.png"]),
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
