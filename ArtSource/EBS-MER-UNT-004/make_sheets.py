#!/usr/bin/env python3
"""Compose the Relay Skiff concept-comparison sheets with exact, recorded ebs_sheet.py commands.

Author: Angelis Pseftis. Usage: python3 make_sheets.py --evidence-root <evidence root>
Step 1 cuts nine panels (the candidate's main three-quarter, SIDE VIEW and TACTICAL SILHOUETTE, the
side and top panels again at the comparison aspect, its nose and mast detail crops, the lancer style
reference and the superseded concept) into renders/concept-compare/panels/ as single-tile sheets, so the
crop rectangles are recorded once and every later sheet places a panel verbatim in its first cell.
Step 2 composes twelve sheets: one per fidelity check (concept-fidelity.md) plus the mast, style,
pose, LOD and context sheets. Tile order, --cols, --cell and --crop are fixed here so the sheet hashes reproduce from the
record alone (README section 7); every command is printed as it runs. Standard library only.

Sheet geometry (concept-v2). ebs_sheet.py fits every input into a SQUARE cell, so a landscape concept
panel beside a portrait render lands at two different scales with most of the canvas empty - the
concept-v1 sheets could not be read side by side. The two checks that compare a panel with a render now
use the `compare` scenes (build_relay_skiff.COMPARE_VIEWS: hull horizontal, bow to the image right, tight
margin, 1280 x 900) and the panels below are cut to the same 1.422 aspect AND the same object-to-frame
share, so panel and render fill their cells identically:
  * side:  concept object 533 x 350 px in a 554 x 390 crop (0.962 / 0.897 of the frame); the render puts
    the 378 cm hull across 393 cm of frame width (0.962) and 256 cm of 276 cm of height (0.927).
  * top:   concept object 533 x 198 px in a 554 x 387 crop (0.962 / 0.512); the render is 0.962 / 0.511.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SHEET = os.path.join(os.path.dirname(HERE), "tools", "ebs_sheet.py")
PACKAGE = "EBS-MER-UNT-004"
CANDIDATE = ("/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/"
             "concept-discovery-20260906/relay-skiff-review/relay-skiff-candidate.png")
LANCER = ("/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/"
          "concept-discovery-20260906/lancer-review/lancer-candidate.png")
SUPERSEDED = ("/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/"
              "asset-production-20260906T221157Z/concept-crops/all/EBS-PKG-MC-RELAY-SKIFF/EBS-CON-MER-UNT-004.png")

# Panel crops on relay-skiff-candidate.png (1536x1024), normalized x0,y0,x1,y1.
PANELS = [
    ("three_quarter.png", CANDIDATE, "0.02,0.08,0.58,0.96", 900),   # main three-quarter
    ("side.png", CANDIDATE, "0.605,0.30,1.0,0.52", 900),            # SIDE VIEW panel
    ("top.png", CANDIDATE, "0.60,0.615,0.985,0.865", 900),          # TACTICAL SILHOUETTE panel
    # the same two panels at the comparison aspect and object share (see the module docstring):
    # side  x 940..1494, y 140..530 ; top  x 944..1498, y 568..955  of the 1536 x 1024 candidate
    ("side_compare.png", CANDIDATE, "0.61198,0.13672,0.97266,0.51758", 900),
    ("top_compare.png", CANDIDATE, "0.61458,0.55469,0.97526,0.93262", 900),
    ("nose.png", CANDIDATE, "0.33,0.52,0.62,0.88", 900),            # nose, emitter bay, forward pods
    ("mast.png", CANDIDATE, "0.06,0.10,0.36,0.60", 900),            # relay mast, dish, whips, cable loop
    ("lancer_style.png", LANCER, "0.03,0.08,0.40,0.95", 900),       # faction/style reference
    ("superseded.png", SUPERSEDED, "0.0,0.0,1.0,1.0", 900),         # EBS-CON-MER-UNT-004, REPLACE (history only)
]
POSE_ORDER = ["pose_idle_025", "pose_move_050", "pose_turn_025", "pose_stop_040",
              "pose_relay_extend_100", "pose_relay_hold_050", "pose_relay_expiry_070", "pose_attack_anticipation_100",
              "pose_attack_execution_030", "pose_damage_033", "pose_death_100", "pose_cancel_000"]


def sheets(root: str):
    r = os.path.join(root, PACKAGE, "renders")
    out = os.path.join(r, "concept-compare")
    p = os.path.join(out, "panels")
    return [
        # fidelity check 1: side view - long thin hull floating clear, mast and dish dominant, cradle on the deck.
        # Panel and renders are at one scale and one orientation (see the module docstring).
        ("check1_side.png", 2, 900, [f"{p}/side_compare.png", f"{r}/compare/side_plate.png",
                                     f"{r}/compare_unloaded/side_plate.png", f"{r}/compare_lod1/side_plate.png"]),
        # fidelity check 2: tactical top silhouette - stretched hexagon, mast at the quarter, cradle centred,
        # and the relay disc reading as a disc
        ("check2_top.png", 2, 900, [f"{p}/top_compare.png", f"{r}/compare/top_plate.png",
                                    f"{r}/compare_unloaded/top_plate.png", f"{r}/compare_lod1/top_plate.png"]),
        # the turnaround plan and the move still, kept from concept-v1 so the sheets still carry them
        ("check1_side_turnaround.png", 2, 900, [f"{r}/rest/right.png", f"{r}/rest/left.png",
                                                f"{r}/pose_move_050/right.png", f"{r}/rest/top_plan.png"]),
        # fidelity check 3: six lift pods under the hull edges; the emitter small and forward
        ("check3_pods_and_emitter.png", 2, 900, [f"{p}/nose.png", f"{r}/rest/front.png", f"{r}/pose_attack_execution_030/front.png", f"{r}/rest/right.png"]),
        # fidelity check 4: the cradle-hidden variant reads as an empty strapped deck
        ("check4_loaded_vs_unloaded.png", 2, 900, [f"{p}/three_quarter.png", f"{r}/rest/right.png",
                                                   f"{r}/rest_unloaded/right.png", f"{r}/rest_unloaded/top.png"]),
        ("check4_loaded_vs_unloaded_tactical.png", 2, 900, [f"{r}/rest_tactical/tactical_close.png", f"{r}/rest_unloaded_tactical/tactical_close.png",
                                                            f"{r}/rest_tactical/tactical_gameplay.png", f"{r}/rest_unloaded_tactical/tactical_gameplay.png"]),
        # fidelity check 5: tactical framing - a light utility scout, never a hero body or a gunship
        ("check5_tactical.png", 2, 900, [f"{p}/three_quarter.png", f"{r}/rest_tactical/tactical_close.png",
                                         f"{r}/rest_tactical/tactical_gameplay.png", f"{r}/rest_tactical/tactical_mono.png"]),
        # mast, dish, whips and cable loop against the concept's own detail panel
        ("mast_and_dish.png", 2, 900, [f"{p}/mast.png", f"{r}/rest/rear.png", f"{r}/pose_relay_extend_100/front.png", f"{r}/pose_relay_extend_100/right.png"]),
        # material language of the faction reference, and the superseded concept this replaced
        ("style_and_history.png", 2, 900, [f"{p}/lancer_style.png", f"{r}/rest_tactical/tactical_front_quarter.png",
                                           f"{p}/superseded.png", f"{r}/rest_tactical/tactical_close.png"]),
        ("pose_sheet_right.png", 4, 480, [f"{r}/{s}/right.png" for s in POSE_ORDER]),
        ("pose_sheet_tactical_near.png", 4, 480, [f"{r}/{s}/tactical_near.png" for s in POSE_ORDER]),
        ("lod0_vs_lod1.png", 2, 640, [f"{r}/rest/front.png", f"{r}/lod1/front.png", f"{r}/rest/right.png", f"{r}/lod1/right.png"]),
        ("context_and_states.png", 2, 900, [f"{r}/context/tactical_gameplay.png", f"{r}/context/tactical_near.png",
                                            f"{r}/pose_death_100_tactical/tactical_gameplay.png", f"{r}/pose_relay_extend_100_tactical/tactical_gameplay.png"]),
    ], out


def run(cmd, dry):
    print(" ".join(f'"{c}"' if " " in c else c for c in cmd))
    if not dry:
        subprocess.run(cmd, check=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", required=True)
    parser.add_argument("--dry-run", action="store_true", help="print the commands only")
    args = parser.parse_args()
    root = os.path.abspath(args.evidence_root)
    plan, out_dir = sheets(root)
    panels_dir = os.path.join(out_dir, "panels")
    os.makedirs(panels_dir, exist_ok=True)
    for name, source, crop, cell in PANELS:
        run([sys.executable, SHEET, "--out", os.path.join(panels_dir, name), "--cols", "1", "--cell", str(cell), "--crop", crop, source], args.dry_run)
    for name, cols, cell, images in plan:
        run([sys.executable, SHEET, "--out", os.path.join(out_dir, name), "--cols", str(cols), "--cell", str(cell)] + images, args.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
