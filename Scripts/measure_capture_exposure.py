"""Objective exposure measurement for a rendered PNG capture.

Reports the clipped-highlight fraction (pixels at or above the clip
threshold in every channel), the near-clip fraction, mean luma, and a
16-bucket luma histogram, so exposure work is judged by numbers rather than
impression. Pure Python (zlib PNG decode for 8-bit RGB/RGBA), no
third-party imaging dependency.

    python3 Scripts/measure_capture_exposure.py <capture.png> [...]

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import struct
import sys
import zlib


def read_png_rgb(path: str) -> tuple[int, int, bytes, int]:
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise RuntimeError(f"Not a PNG: {path}")
    position = 8
    width = height = bit_depth = color_type = None
    idat = bytearray()
    while position < len(data):
        length = struct.unpack_from(">I", data, position)[0]
        kind = data[position + 4 : position + 8]
        payload = data[position + 8 : position + 8 + length]
        if kind == b"IHDR":
            width, height, bit_depth, color_type = struct.unpack_from(
                ">IIBB", payload
            )[:4]
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break
        position += 12 + length
    if width is None or bit_depth != 8 or color_type not in (2, 6):
        raise RuntimeError(
            f"Expected 8-bit RGB/RGBA PNG: {path} depth={bit_depth} color={color_type}"
        )
    channels = 3 if color_type == 2 else 4
    raw = zlib.decompress(bytes(idat))
    stride = width * channels
    out = bytearray(width * height * channels)
    previous = bytearray(stride)
    offset = 0
    for row in range(height):
        filter_type = raw[offset]
        offset += 1
        line = bytearray(raw[offset : offset + stride])
        offset += stride
        if filter_type == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif filter_type == 2:
            for i in range(stride):
                line[i] = (line[i] + previous[i]) & 0xFF
        elif filter_type == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + previous[i]) >> 1)) & 0xFF
        elif filter_type == 4:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                up = previous[i]
                up_left = previous[i - channels] if i >= channels else 0
                p = left + up - up_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - up_left)
                predictor = left if pa <= pb and pa <= pc else up if pb <= pc else up_left
                line[i] = (line[i] + predictor) & 0xFF
        out[row * stride : (row + 1) * stride] = line
        previous = line
    return width, height, bytes(out), channels


def measure(path: str) -> dict:
    width, height, pixels, channels = read_png_rgb(path)
    total = width * height
    clipped = 0
    near_clip = 0
    luma_sum = 0.0
    histogram = [0] * 16
    for index in range(total):
        base = index * channels
        r, g, b = pixels[base], pixels[base + 1], pixels[base + 2]
        if r >= 254 and g >= 254 and b >= 254:
            clipped += 1
        if r >= 245 and g >= 245 and b >= 245:
            near_clip += 1
        luma = 0.2126 * r + 0.7152 * g + 0.0722 * b
        luma_sum += luma
        histogram[min(15, int(luma) >> 4)] += 1
    return {
        "path": path,
        "width": width,
        "height": height,
        "clippedFraction": clipped / total,
        "nearClipFraction": near_clip / total,
        "meanLuma": luma_sum / total,
        "histogram": [round(count / total, 5) for count in histogram],
    }


def main() -> None:
    for path in sys.argv[1:]:
        result = measure(path)
        print(
            f"[ECHOES_CAPTURE_EXPOSURE] path={result['path']} size={result['width']}x{result['height']} "
            f"clipped={result['clippedFraction']:.5f} nearClip={result['nearClipFraction']:.5f} "
            f"meanLuma={result['meanLuma']:.1f}"
        )
        print(f"  histogram(16)={result['histogram']}")


if __name__ == "__main__":
    main()
