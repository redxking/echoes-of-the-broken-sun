#!/usr/bin/env python3
"""Cut every package's registered concept-input region out of the verified source image.

Author: Angelis Pseftis.
Usage:
  python3 ebs_concept_crops.py --packages Docs/VisualAssetPipeline/reference-packages.json \
      --source-dir "<checkout>/site/assets/concepts" --out "<evidence>/concept-crops/all"

For each package and each concept input the registered source file is located under --source-dir,
its sha256 is checked against the registered source_sha256 (a mismatch or an LFS pointer is an
ERROR, never a silent substitution), JPEG sources are converted to PNG with macOS `sips`, and the
normalized bbox is cut and written as <out>/<PACKAGE_ID>/<CONCEPT_ID>.png (nearest-neighbour upscale
to at least --min-size on the long side, deterministic). A crops-manifest.json records every crop
with its source hash, bbox, decision and output hash. Stdlib only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ebs_sheet  # noqa: E402  (PNG read/write)


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def is_lfs_pointer(path: str) -> bool:
    with open(path, "rb") as handle:
        return handle.read(40).startswith(b"version https://git-lfs")


def to_png(path: str, tmpdir: str) -> str:
    if path.lower().endswith(".png"):
        return path
    out = os.path.join(tmpdir, os.path.splitext(os.path.basename(path))[0] + ".png")
    subprocess.run(["sips", "-s", "format", "png", path, "--out", out], check=True, capture_output=True)
    return out


def crop(png: str, bbox, out: str, min_size: int) -> dict:
    w, h, bpp, rows = ebs_sheet.read_png(png)
    x0, y0, x1, y1 = int(w * bbox[0]), int(h * bbox[1]), int(w * bbox[2]), int(h * bbox[3])
    cw, ch = max(1, x1 - x0), max(1, y1 - y0)
    scale = max(1, int(round(min_size / max(cw, ch)))) if max(cw, ch) < min_size else 1
    out_rows = []
    for y in range(y0, y1):
        row = rows[y]
        line = bytearray()
        for x in range(x0, x1):
            px = row[x * bpp:x * bpp + 3]
            line += px * scale
        for _ in range(scale):
            out_rows.append(bytes(line))
    ebs_sheet.write_png(out, cw * scale, ch * scale, out_rows)
    return {"pixel_box": [x0, y0, x1, y1], "scale": scale, "size": [cw * scale, ch * scale]}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--packages", required=True)
    parser.add_argument("--source-dir", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--min-size", type=int, default=1000)
    parser.add_argument("--only", action="append", default=[], help="package id(s) to process; default all")
    args = parser.parse_args()
    with open(args.packages, "r", encoding="utf-8") as handle:
        packages = json.load(handle)["packages"]
    items = packages.items() if isinstance(packages, dict) else [(p["package_id"], p) for p in packages]
    manifest = {"author": "Angelis Pseftis", "creator": "Angelis Pseftis", "source_dir": args.source_dir, "packages_record": args.packages,
                "packages_record_sha256": sha256_file(args.packages), "crops": [], "errors": []}
    tmpdir = tempfile.mkdtemp(prefix="ebs-crops-")
    try:
        for package_id, package in items:
            if args.only and package_id not in args.only:
                continue
            for ci in package.get("concept_inputs") or []:
                src = ci["source"]
                path = os.path.join(args.source_dir, src["source_filename"])
                entry = {"package_id": package_id, "concept_id": ci["concept_id"], "decision": ci.get("decision"), "use": ci.get("use"),
                         "source_filename": src["source_filename"], "registered_sha256": src.get("source_sha256"), "bbox_normalized": src.get("bbox_normalized"),
                         "locator": src.get("locator")}
                if not os.path.exists(path):
                    manifest["errors"].append({**entry, "error": "source file missing"})
                    continue
                if is_lfs_pointer(path):
                    manifest["errors"].append({**entry, "error": "source is an LFS pointer, not the image"})
                    continue
                actual = sha256_file(path)
                if src.get("source_sha256") and actual != src["source_sha256"]:
                    manifest["errors"].append({**entry, "error": f"sha256 mismatch: actual {actual}"})
                    continue
                bbox = src.get("bbox_normalized") or [0, 0, 1, 1]
                out_dir = os.path.join(args.out, package_id)
                os.makedirs(out_dir, exist_ok=True)
                out = os.path.join(out_dir, f"{ci['concept_id']}.png")
                info = crop(to_png(path, tmpdir), bbox, out, args.min_size)
                entry.update({"actual_sha256": actual, "output": out, "output_sha256": sha256_file(out), **info})
                manifest["crops"].append(entry)
                print(f"{package_id} {ci['concept_id']} {ci.get('decision')} -> {out} {info['size']}")
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "crops-manifest.json"), "w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=1)
    print(f"crops {len(manifest['crops'])} errors {len(manifest['errors'])}")
    for e in manifest["errors"]:
        print("ERROR", e["package_id"], e["concept_id"], e["error"])
    return 1 if manifest["errors"] else 0


if __name__ == "__main__":
    sys.exit(main())
