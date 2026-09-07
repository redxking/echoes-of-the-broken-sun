#!/usr/bin/env python3
"""Compose review PNGs into one comparison sheet (pure Python, stdlib only).

Author: Angelis Pseftis.
Usage:
  python3 ebs_sheet.py --out sheet.png --cols 2 --cell 640 IMAGE [IMAGE ...]
  python3 ebs_sheet.py --out sheet.png --cols 2 --cell 640 --crop 0,0,0.5,0.5 SHEET.png IMAGE ...

Every input is fitted (nearest-neighbour, aspect preserved) into a square cell of --cell pixels on a
neutral grey ground, laid out in --cols columns in the given order. --crop applies a normalized
[x0,y0,x1,y1] crop to the FIRST input only (used to cut a quadrant out of a concept sheet). 8-bit RGB
and RGBA PNGs are accepted; alpha is composited over the ground. The sheet is deterministic.
"""
from __future__ import annotations

import argparse
import struct
import sys
import zlib

GROUND = (52, 52, 56)


def read_png(path: str):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"not a PNG: {path}")
    pos, idat, w, h, ct, bd = 8, [], 0, 0, 0, 8
    while pos < len(data):
        ln = struct.unpack(">I", data[pos:pos + 4])[0]
        typ = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + ln]
        if typ == b"IHDR":
            w, h, bd, ct = struct.unpack(">IIBB", body[:10])
            if bd != 8 or ct not in (2, 6):
                raise SystemExit(f"unsupported PNG (bit depth {bd}, colour type {ct}): {path}")
        elif typ == b"IDAT":
            idat.append(body)
        pos += 12 + ln
    raw = zlib.decompress(b"".join(idat))
    bpp = 3 if ct == 2 else 4
    stride = w * bpp
    rows, prev, i = [], bytearray(stride), 0
    for _ in range(h):
        f = raw[i]
        i += 1
        line = bytearray(raw[i:i + stride])
        i += stride
        if f == 1:
            for x in range(bpp, stride):
                line[x] = (line[x] + line[x - bpp]) & 255
        elif f == 2:
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 255
        elif f == 3:
            for x in range(stride):
                line[x] = (line[x] + (((line[x - bpp] if x >= bpp else 0) + prev[x]) >> 1)) & 255
        elif f == 4:
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                b = prev[x]
                c = prev[x - bpp] if x >= bpp else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        rows.append(bytes(line))
        prev = line
    return w, h, bpp, rows


def write_png(path: str, w: int, h: int, rows) -> None:
    raw = b"".join(b"\x00" + r for r in rows)

    def chunk(t, b):
        return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b) & 0xFFFFFFFF)

    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def fit(w, h, bpp, rows, crop, cell):
    x0, y0, x1, y1 = (0, 0, w, h) if crop is None else (int(w * crop[0]), int(h * crop[1]), int(w * crop[2]), int(h * crop[3]))
    cw, ch = max(1, x1 - x0), max(1, y1 - y0)
    scale = min(cell / cw, cell / ch)
    ow, oh = max(1, int(cw * scale)), max(1, int(ch * scale))
    padx, pady = (cell - ow) // 2, (cell - oh) // 2
    ground = bytes(GROUND) * cell
    out = [bytearray(ground) for _ in range(cell)]
    for oy in range(oh):
        sy = y0 + min(ch - 1, int(oy / scale))
        row = rows[sy]
        line = out[pady + oy]
        for ox in range(ow):
            sx = x0 + min(cw - 1, int(ox / scale))
            px = row[sx * bpp:sx * bpp + bpp]
            if bpp == 4 and px[3] < 255:
                a = px[3] / 255.0
                px = bytes(int(px[k] * a + GROUND[k] * (1 - a)) for k in range(3))
            o = (padx + ox) * 3
            line[o:o + 3] = px[:3]
    return out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--cols", type=int, default=2)
    parser.add_argument("--cell", type=int, default=640)
    parser.add_argument("--crop", default=None, help="normalized x0,y0,x1,y1 applied to the first image only")
    parser.add_argument("images", nargs="+")
    args = parser.parse_args()
    crop = tuple(float(v) for v in args.crop.split(",")) if args.crop else None
    tiles = []
    for index, path in enumerate(args.images):
        w, h, bpp, rows = read_png(path)
        tiles.append(fit(w, h, bpp, rows, crop if index == 0 else None, args.cell))
    cols = max(1, args.cols)
    nrows = (len(tiles) + cols - 1) // cols
    out_rows = []
    for r in range(nrows):
        group = tiles[r * cols:(r + 1) * cols]
        blank = bytes(GROUND) * args.cell
        for y in range(args.cell):
            out_rows.append(b"".join(bytes(t[y]) for t in group) + blank * (cols - len(group)))
    write_png(args.out, args.cell * cols, args.cell * nrows, out_rows)
    print(f"{args.out} {args.cell * cols}x{args.cell * nrows} ({len(tiles)} tiles)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
