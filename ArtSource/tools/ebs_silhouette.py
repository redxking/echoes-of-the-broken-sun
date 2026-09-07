#!/usr/bin/env python3
"""Compare two rendered silhouettes and report how separable they are.

Author: Angelis Pseftis.

The owner ruled on 2026-09-07 that roster members sharing an anatomy must remain distinguishable in
MONOCHROME at tactical distance, with colour, emissive and texture counting for nothing. This measures
that instead of asserting it: two renders taken from the SAME camera are reduced to occupancy masks,
aligned on their bounding boxes, and compared.

  python3 ebs_silhouette.py --a <a.png> --b <b.png> [--label-a NAME --label-b NAME] [--out report.json]

Reported per image: fill (how much of its own box the body occupies), aspect, and the vertical profile
in eight bands. Reported for the pair: intersection over union of the aligned masks, and the largest
band-by-band difference. A LOW IoU means two shapes a player can tell apart. Standard library only.
"""
from __future__ import annotations

import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ebs_sheet import read_png  # noqa: E402

BANDS = 8


def mask(path: str, background_tolerance: int = 26):
    """Occupancy mask plus its bounding box. The background is the render's own corner colour."""
    w, h, bpp, rows = read_png(path)
    bg = tuple(rows[0][0:3])

    def occupied(x, y):
        r = rows[y]
        i = x * bpp
        return sum(abs(r[i + k] - bg[k]) for k in range(3)) > background_tolerance

    points = [(x, y) for y in range(h) for x in range(w) if occupied(x, y)]
    if not points:
        raise SystemExit(f"no silhouette found in {path}")
    x0 = min(p[0] for p in points); x1 = max(p[0] for p in points)
    y0 = min(p[1] for p in points); y1 = max(p[1] for p in points)
    return set(points), (x0, y0, x1, y1), (w, h)


def normalised(points, box, grid: int = 96):
    """Resample a mask into a fixed grid over its own bounding box, so two renders can be compared
    on SHAPE rather than on how large each happened to appear."""
    x0, y0, x1, y1 = box
    span_x = max(1, x1 - x0)
    span_y = max(1, y1 - y0)
    cells = set()
    for x, y in points:
        cx = int((x - x0) * (grid - 1) / span_x)
        cy = int((y - y0) * (grid - 1) / span_y)
        cells.add((cx, cy))
    return cells


def profile(cells, grid: int = 96):
    """Width of the shape in each of BANDS horizontal bands, top to bottom, as a fraction of the grid."""
    out = []
    for band in range(BANDS):
        lo = band * grid // BANDS
        hi = (band + 1) * grid // BANDS
        xs = [c[0] for c in cells if lo <= c[1] < hi]
        out.append(round((max(xs) - min(xs) + 1) / grid, 4) if xs else 0.0)
    return out


def describe(path: str, grid: int = 96) -> dict:
    points, box, size = mask(path)
    cells = normalised(points, box, grid)
    x0, y0, x1, y1 = box
    return {"file": os.path.basename(path), "image_px": list(size),
            "bounds_px": [x0, y0, x1, y1],
            "aspect_w_over_h": round((x1 - x0 + 1) / max(1, y1 - y0 + 1), 4),
            "fill": round(len(cells) / (grid * grid), 4),
            "profile_top_to_bottom": profile(cells, grid),
            "_cells": cells}


def compare(a: dict, b: dict, grid: int = 96) -> dict:
    ca, cb = a["_cells"], b["_cells"]
    inter = len(ca & cb)
    union = len(ca | cb)
    band_gap = max(abs(x - y) for x, y in zip(a["profile_top_to_bottom"], b["profile_top_to_bottom"]))
    return {"intersection_over_union": round(inter / union, 4) if union else 0.0,
            "largest_band_difference": round(band_gap, 4),
            "aspect_difference": round(abs(a["aspect_w_over_h"] - b["aspect_w_over_h"]), 4),
            "fill_difference": round(abs(a["fill"] - b["fill"]), 4),
            "reading": ("A LOW intersection over union means the two shapes occupy different areas once "
                        "scaled to the same box, which is what monochrome separability at tactical "
                        "distance requires. This is a measurement, not a verdict.")}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--a", required=True)
    parser.add_argument("--b", required=True)
    parser.add_argument("--label-a", default="a")
    parser.add_argument("--label-b", default="b")
    parser.add_argument("--grid", type=int, default=96)
    parser.add_argument("--out")
    args = parser.parse_args()
    a = describe(args.a, args.grid)
    b = describe(args.b, args.grid)
    result = {"author": "Angelis Pseftis", "grid": args.grid,
              args.label_a: {k: v for k, v in a.items() if not k.startswith("_")},
              args.label_b: {k: v for k, v in b.items() if not k.startswith("_")},
              "comparison": compare(a, b, args.grid)}
    if args.out:
        os.makedirs(os.path.dirname(args.out), exist_ok=True)
        with open(args.out, "w", encoding="utf-8") as handle:
            json.dump(result, handle, indent=1, sort_keys=True)
    print(json.dumps(result["comparison"], indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
