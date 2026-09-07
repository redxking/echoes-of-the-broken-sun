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
  * every fidelity check is judged on the SHIPPED review assembly (`deployed` = main mesh + barrier
    pane assembly; `packed` = main mesh with the assembly hidden). Three non-shipped tiles exist and
    each may only ever appear BESIDE a shipped tile, never in its place:
    `deployed_glass_cutaway` (field slabs AND back plates cut, a stand-in for translucency),
    `deployed_frames_only` (SEPARATION CONTROL: the deployed pose with the assembly hidden) and
    `packed_panes_shown` (INTEGRATION FAILURE CONTROL: the packed pose with the assembly still
    drawn - what concept-v3 shipped). The `packed_field_dark` material-state tile is gone: under the
    owner ruling of 2026-09-07 the packed read is geometry, so no field-off variant stands in for a
    shipped state.

LABELLING (concept-v5, review finding). ~~"each is labelled"~~ was **struck 2026-09-07**: it was a
statement about this file's prose, not about the artifact. `ArtSource/tools/ebs_sheet.py` has no
text-drawing capability at all - read_png, write_png, fit, main and nothing else - so nothing in any
composed sheet carried a caption, and check3_packed_travel.png ended on two uncaptioned tiles of a
packed Bulwark with a fully lit barrier: the integration failure control, sitting directly below
three shipped packed tiles with nothing in the image to tell them apart. The tool is read-only here,
so the caption is burned in BEFORE tiling, by this file:
  * ``label_png`` draws a warning band and a 5x7 bitmap caption into a COPY of the render, written to
    ``renders/labelled/<variant>__<view>.png``; the original render is never modified;
  * every non-shipped tile in every sheet is composed from that captioned copy, so the label is in
    the sheet's own pixels and survives being looked at without this file;
  * every sheet also gets a sidecar ``renders/concept-compare/<sheet>.tiles.json`` naming each tile
    in order, with its kind (reference / shipped / non-shipped) and its caption.
Standard library only.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
import ebs_sheet as sheet_tool  # noqa: E402  (read_png / write_png only; the tool is read-only here)

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
         "pose_deploy_100", "pose_idle_deployed_050", "pose_drag_deployed_025", "pose_pack_055", "pose_damage_020",
         "pose_death_100", "pose_cancel_000", "pose_restore_000"]

# --- the three non-shipped review variants and the caption each tile carries ---------------------
# Keyed by the render-set directory name (the parent directory of every tile PNG). A tile from any
# other directory is a shipped state or the reference panel and is composed uncaptioned.
NON_SHIPPED = {
    "deployed_glass_cutaway": "NOT SHIPPED - TRANSLUCENCY STAND-IN",
    "deployed_glass_cutaway_tactical": "NOT SHIPPED - TRANSLUCENCY STAND-IN",
    "deployed_frames_only": "NOT SHIPPED - SEPARATION CONTROL",
    "deployed_frames_only_tactical": "NOT SHIPPED - SEPARATION CONTROL",
    "packed_panes_shown": "NOT SHIPPED - INTEGRATION FAILURE CONTROL",
    "packed_panes_shown_tactical": "NOT SHIPPED - INTEGRATION FAILURE CONTROL",
}
BAND_RGB = (168, 46, 32)      # warning red; nothing in the palette or the ground is near it
TEXT_RGB = (255, 244, 238)

# 5x7 bitmap font, one row string per scanline, '#' set. Only the characters the captions above use
# are defined; a caption asking for anything else raises rather than dropping a glyph silently.
FONT_5X7 = {
    " ": ("     ", "     ", "     ", "     ", "     ", "     ", "     "),
    "-": ("     ", "     ", "     ", "#####", "     ", "     ", "     "),
    "A": (" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"),
    "B": ("#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "),
    "C": (" ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####"),
    "D": ("#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "),
    "E": ("#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"),
    "F": ("#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "),
    "G": (" ####", "#    ", "#    ", "#  ##", "#   #", "#   #", " ####"),
    "H": ("#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"),
    "I": ("#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"),
    "L": ("#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"),
    "N": ("#   #", "##  #", "# # #", "#  ##", "#   #", "#   #", "#   #"),
    "O": (" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "),
    "P": ("#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "),
    "R": ("#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"),
    "S": (" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "),
    "T": ("#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "),
    "U": ("#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "),
    "Y": ("#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "),
}


def variant_of(path: str) -> str:
    """The render-set directory a tile PNG comes from ('deployed', 'packed_panes_shown', ...)."""
    return os.path.basename(os.path.dirname(path))


def is_non_shipped(path: str) -> bool:
    return variant_of(path) in NON_SHIPPED


def caption_for(path: str):
    """The caption burned into this tile, or None when the tile is a shipped state or the reference."""
    return NON_SHIPPED.get(variant_of(path))


def kind_of(path: str) -> str:
    if is_non_shipped(path):
        return "non-shipped"
    if os.path.dirname(path) == REVIEW_DIR or "concept-crops" in path:
        return "reference"
    return "shipped"


def _rgb_rows(width: int, bpp: int, rows) -> list:
    """Mutable RGB scanlines; an RGBA source is composited over the sheet ground."""
    if bpp == 3:
        return [bytearray(r) for r in rows]
    out = []
    for r in rows:
        line = bytearray(width * 3)
        for x in range(width):
            a = r[x * 4 + 3] / 255.0
            for c in range(3):
                line[x * 3 + c] = int(round(r[x * 4 + c] * a + sheet_tool.GROUND[c] * (1.0 - a)))
        out.append(line)
    return out


def label_png(src: str, dst: str, text: str) -> str:
    """Write a copy of ``src`` with ``text`` burned into a warning band across its top.

    The source render is never modified. The band is sized from the image so the caption survives
    ebs_sheet.py's nearest-neighbour fit into a 480-760 px cell: at 1920x1080 the glyphs are 35x49 px
    and still ~13x18 px in a 700 px cell."""
    width, height, bpp, rows = sheet_tool.read_png(src)
    pixels = _rgb_rows(width, bpp, rows)
    scale = max(3, round(width / 260.0))
    pad = 2 * scale
    band_h = 7 * scale + 2 * pad
    for y in range(min(band_h, height)):
        line = pixels[y]
        for x in range(width):
            line[x * 3], line[x * 3 + 1], line[x * 3 + 2] = BAND_RGB
    x_cursor = pad
    for ch in text:
        glyph = FONT_5X7.get(ch)
        if glyph is None:
            raise SystemExit(f"make_sheets: no 5x7 glyph for {ch!r} (caption {text!r})")
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit != "#":
                    continue
                for sy in range(scale):
                    y = pad + gy * scale + sy
                    if y >= height:
                        continue
                    line = pixels[y]
                    for sx in range(scale):
                        x = x_cursor + gx * scale + sx
                        if x >= width:
                            continue
                        line[x * 3], line[x * 3 + 1], line[x * 3 + 2] = TEXT_RGB
        x_cursor += 6 * scale
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    sheet_tool.write_png(dst, width, height, [bytes(p) for p in pixels])
    return dst


def sheets(root: str):
    r = os.path.join(root, PACKAGE, "renders")
    concept = os.path.join(root, "concept-crops", "all", "EBS-PKG-MC-BULWARK-TEAM", "EBS-CON-MER-UNT-003.png")
    out = os.path.join(r, "concept-compare")
    return [
        # 1. deployed front: one flat face of six cells, three per side, seam on the centreline.
        #    Tile 2 is the SHIPPED mesh; tile 3 is the captioned translucency stand-in.
        ("check1_deployed_front.png", 2, 760, CROP_DEPLOYED,
         [REFERENCE, f"{r}/deployed/front.png", f"{r}/deployed_glass_cutaway/front.png", f"{r}/deployed/top.png"]),
        # 2. deployed side: face forward of the chassis, wall taller than the operators
        ("check2_deployed_side.png", 2, 760, CROP_DEPLOYED,
         [REFERENCE, f"{r}/deployed/right.png", f"{r}/deployed_glass_cutaway/right.png", f"{r}/deployed_tactical/tactical_front_quarter.png"]),
        # 3. packed three-quarter: folded OPEN frames along the chassis sides, compact crawler.
        #    Tiles 2-4 are the SHIPPED packed mesh (the pane assembly hidden by the owner ruling of
        #    2026-09-07); tiles 5-6 are the INTEGRATION FAILURE CONTROL - the same pose with the
        #    assembly still drawn, i.e. what concept-v3 shipped and what the runtime must not do.
        #    Both control tiles carry that caption in their own pixels.
        ("check3_packed_travel.png", 2, 700, CROP_TRAVEL,
         [REFERENCE, f"{r}/packed_tactical/concept_travel.png",
          f"{r}/packed/front.png", f"{r}/packed/right.png",
          f"{r}/packed_panes_shown_tactical/concept_travel.png", f"{r}/packed_panes_shown/right.png"]),
        # 4. rear in both states: open rear access, no shield, no cyan face. Every tile is a
        #    SHIPPED mesh - the rear read is now geometry (the pane back plates), not a material.
        ("check4_rear_both_states.png", 2, 700, CROP_REAR,
         [REFERENCE, f"{r}/deployed/rear.png", f"{r}/packed/rear.png", f"{r}/deployed_lod1/rear.png",
          f"{r}/packed_lod1/rear.png", f"{r}/packed_tactical/concept_rear.png"]),
        # 5. tactical framing: facing and deployed footprint unmistakable, the rear says "flank me"
        ("check5_tactical_read.png", 2, 700, CROP_TRAVEL,
         [REFERENCE, f"{r}/deployed_tactical_x175/tactical_gameplay.png", f"{r}/packed_tactical_x175/tactical_gameplay.png",
          f"{r}/deployed_tactical/tactical_near.png", f"{r}/packed_tactical/tactical_near.png",
          f"{r}/packed_panes_shown_tactical/tactical_near.png"]),
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
        # The barrier sub-object itself: what the third object contributes and what hiding it does.
        # A supplementary sheet - it leads with the build's own renders and certifies nothing.
        ("barrier_sub_object.png", 2, 700, None,
         [f"{r}/deployed/front.png", f"{r}/deployed_frames_only/front.png",
          f"{r}/packed/right.png", f"{r}/packed_panes_shown/right.png",
          f"{r}/packed_tactical/concept_travel.png", f"{r}/deployed_frames_only_tactical/concept_travel.png"]),
        # The deploy sequence now ends on the finished wall: the pane assembly appears at the moment
        # the authoritative state becomes DEPLOYED, which is the deploy@1.00 tile.
        ("deploy_sequence.png", 3, 560, None,
         [f"{r}/pose_idle_packed_000/front.png", f"{r}/pose_deploy_015/front.png", f"{r}/pose_deploy_055/front.png",
          f"{r}/pose_deploy_082/front.png", f"{r}/pose_deploy_100/front.png", f"{r}/pose_pack_055/front.png"]),
        ("deploy_sequence_quarter.png", 3, 560, None,
         [f"{r}/pose_idle_packed_000/concept_travel.png", f"{r}/pose_deploy_015/concept_travel.png", f"{r}/pose_deploy_055/concept_travel.png",
          f"{r}/pose_deploy_082/concept_travel.png", f"{r}/pose_deploy_100/concept_travel.png", f"{r}/pose_pack_055/concept_travel.png"]),
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
    root = os.path.abspath(args.evidence_root)
    plan, out_dir = sheets(root)
    labelled_dir = os.path.join(root, PACKAGE, "renders", "labelled")
    os.makedirs(out_dir, exist_ok=True)
    labelled = {}
    for name, cols, cell, crop, images in plan:
        tiles = []
        for image in images:
            caption = caption_for(image)
            if not caption:
                tiles.append(image)
                continue
            if image not in labelled:
                dst = os.path.join(labelled_dir, f"{variant_of(image)}__{os.path.basename(image)}")
                # --dry-run prints the path the sheet WOULD be composed from, so the printed command
                # lines reproduce the shipped sheets exactly rather than a captionless variant of them.
                labelled[image] = dst if args.dry_run else label_png(image, dst, caption)
                print(f"caption \"{caption}\" -> {labelled[image]}")
            tiles.append(labelled[image])
        cmd = [sys.executable, SHEET, "--out", os.path.join(out_dir, name), "--cols", str(cols), "--cell", str(cell)]
        if crop:
            cmd += ["--crop", crop]
        cmd += tiles
        print(" ".join(f'"{c}"' if " " in c else c for c in cmd))
        sidecar = {
            "sheet": name, "author": "Angelis Pseftis", "cols": cols, "cell_px": cell,
            "crop_first_tile": crop,
            "note": "Tile identity for this sheet, in composition order (row-major, `cols` per row). "
                    "Every non-shipped tile also carries its caption burned into its own pixels; the "
                    "captioned copy is renders/labelled/<variant>__<view>.png and the original render "
                    "is unmodified. No fidelity check is certified on a non-shipped tile.",
            "tiles": [{"index": i, "source": os.path.basename(os.path.dirname(p)) + "/" + os.path.basename(p),
                       "path": p, "kind": kind_of(p), "shipped": kind_of(p) == "shipped",
                       "caption": caption_for(p)}
                      for i, p in enumerate(images)],
        }
        if not args.dry_run:
            subprocess.run(cmd, check=True)
            with open(os.path.join(out_dir, name.replace(".png", ".tiles.json")), "w",
                      encoding="utf-8", newline="\n") as handle:
                handle.write(json.dumps(sidecar, indent=1) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
