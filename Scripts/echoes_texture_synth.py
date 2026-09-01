"""Deterministic texture synthesis for the registered surface families (A3).

Pure Python. Every map derives from an explicit per-family seed through an
in-file PRNG and noise stack, so regeneration is byte-idempotent under the
recorded revision strings: fixed zlib level, fixed filter, no timestamps.
Author: Angelis Pseftis.
"""
from __future__ import annotations

import math
import struct
import zlib

SIZE = 512
REVISION_TEXTURES = "surface-textures-v2"


# --- Deterministic PRNG / noise -------------------------------------------
def _hash2(ix: int, iy: int, seed: int) -> float:
    h = (ix * 374761393 + iy * 668265263 + seed * 2246822519) & 0xFFFFFFFF
    h = (h ^ (h >> 13)) * 1274126177 & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0


def value_noise(x: float, y: float, seed: int) -> float:
    ix, iy = int(math.floor(x)), int(math.floor(y))
    fx, fy = x - ix, y - iy
    sx = fx * fx * (3.0 - 2.0 * fx)
    sy = fy * fy * (3.0 - 2.0 * fy)
    a = _hash2(ix, iy, seed)
    b = _hash2(ix + 1, iy, seed)
    c = _hash2(ix, iy + 1, seed)
    d = _hash2(ix + 1, iy + 1, seed)
    return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy


def fbm(x: float, y: float, seed: int, octaves: int = 4) -> float:
    total, amplitude, frequency, norm = 0.0, 1.0, 1.0, 0.0
    for octave in range(octaves):
        total += amplitude * value_noise(x * frequency, y * frequency, seed + octave)
        norm += amplitude
        amplitude *= 0.5
        frequency *= 2.0
    return total / norm


class Xorshift:
    def __init__(self, seed: int) -> None:
        self.state = (seed * 2654435761 + 1) & 0xFFFFFFFF

    def next_float(self) -> float:
        s = self.state
        s ^= (s << 13) & 0xFFFFFFFF
        s ^= s >> 17
        s ^= (s << 5) & 0xFFFFFFFF
        self.state = s
        return s / 0xFFFFFFFF


# --- PNG encoding ----------------------------------------------------------
def encode_png(pixels: list[tuple[int, int, int]], size: int = SIZE) -> bytes:
    raw = bytearray()
    for y in range(size):
        raw.append(0)
        for x in range(size):
            raw.extend(pixels[y * size + x])
    compressed = zlib.compress(bytes(raw), 6)

    def chunk(tag: bytes, payload: bytes) -> bytes:
        body = tag + payload
        return struct.pack(">I", len(payload)) + body + struct.pack(
            ">I", zlib.crc32(body) & 0xFFFFFFFF
        )

    header = struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", compressed)
        + chunk(b"IEND", b"")
    )


def _clamp8(value: float) -> int:
    return max(0, min(255, int(round(value * 255.0))))


def height_to_normal(height: list[float], strength: float, size: int = SIZE) -> list[tuple[int, int, int]]:
    pixels = []
    for y in range(size):
        for x in range(size):
            left = height[y * size + (x - 1) % size]
            right = height[y * size + (x + 1) % size]
            up = height[((y - 1) % size) * size + x]
            down = height[((y + 1) % size) * size + x]
            dx = (left - right) * strength
            dy = (up - down) * strength
            dz = 1.0
            length = math.sqrt(dx * dx + dy * dy + dz * dz)
            pixels.append((
                _clamp8(dx / length * 0.5 + 0.5),
                _clamp8(dy / length * 0.5 + 0.5),
                _clamp8(dz / length * 0.5 + 0.5),
            ))
    return pixels


# --- Family recipes --------------------------------------------------------
def ceramic_civic() -> dict[str, list[tuple[int, int, int]]]:
    """Pale ceramic civic paneling: beveled panel grid, wear at edges."""
    seed = 101
    base, height, mre = [], [], []
    panels = 8
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            pu, pv = (u * panels) % 1.0, (v * panels) % 1.0
            edge = min(pu, 1 - pu, pv, 1 - pv)
            bevel = min(1.0, edge / 0.06)
            panel_id = int(u * panels) * 31 + int(v * panels)
            tone = 0.86 + 0.08 * (_hash2(panel_id, 7, seed) - 0.5)
            grain = fbm(u * 34, v * 34, seed + 3) * 0.05
            wear = max(0.0, 0.22 - edge * 2.2) * fbm(u * 90, v * 90, seed + 9)
            value = max(0.0, tone * bevel ** 0.12 + grain - wear * 0.8)
            base.append((_clamp8(value), _clamp8(value * 0.997), _clamp8(value * 0.993)))
            height.append(bevel * 0.7 + grain)
            rough = 0.34 + wear * 1.6 + (1 - bevel) * 0.22
            mre.append((_clamp8(0.04), _clamp8(min(1.0, rough)), 0))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 2.6)}


def vitrified_glass() -> dict[str, list[tuple[int, int, int]]]:
    """Deep charcoal vitrified glass with magenta micro-fracture veins."""
    seed = 202
    crack = [0.0] * (SIZE * SIZE)
    rng = Xorshift(seed)
    for _ in range(46):
        x = rng.next_float() * SIZE
        y = rng.next_float() * SIZE
        angle = rng.next_float() * math.tau
        for _step in range(400):
            angle += (rng.next_float() - 0.5) * 0.7
            x = (x + math.cos(angle) * 1.6) % SIZE
            y = (y + math.sin(angle) * 1.6) % SIZE
            for ox in (-1, 0, 1):
                for oy in (-1, 0, 1):
                    index = int(y + oy) % SIZE * SIZE + int(x + ox) % SIZE
                    fall = 1.0 if (ox == 0 and oy == 0) else 0.35
                    crack[index] = min(1.0, crack[index] + 0.5 * fall)
    base, height, mre = [], [], []
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            body = 0.045 + fbm(u * 12, v * 12, seed + 5) * 0.05
            vein = crack[y * SIZE + x]
            r = body + vein * 0.34
            g = body * 0.9 + vein * 0.05
            b = body * 1.15 + vein * 0.30
            base.append((_clamp8(r), _clamp8(g), _clamp8(b)))
            height.append(-vein * 0.5 + fbm(u * 20, v * 20, seed + 8) * 0.25)
            rough = 0.12 + fbm(u * 26, v * 26, seed + 11) * 0.18 + vein * 0.25
            mre.append((_clamp8(0.02), _clamp8(min(1.0, rough)), _clamp8(vein * 0.85)))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 3.2)}


def causeway_ash() -> dict[str, list[tuple[int, int, int]]]:
    """Basalt and ash strata with a foot-polished causeway track."""
    seed = 303
    base, height, mre = [], [], []
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            strata = fbm(u * 4 + fbm(u * 9, v * 9, seed + 2) * 0.7, v * 22, seed)
            band = 0.5 + 0.5 * math.sin(strata * 14.0)
            ash = fbm(u * 46, v * 46, seed + 6)
            tone = 0.06 + band * 0.045 + ash * 0.05
            track = math.exp(-((v - 0.5) ** 2) / 0.02) * (0.5 + 0.5 * fbm(u * 30, 0.5, seed + 13))
            value = tone + track * 0.05
            base.append((
                _clamp8(value * 1.02),
                _clamp8(value * 0.99),
                _clamp8(value * 0.94),
            ))
            height.append(band * 0.35 + ash * 0.4 - track * 0.3)
            rough = 0.88 - track * 0.5 + ash * 0.1
            mre.append((_clamp8(0.03), _clamp8(max(0.0, min(1.0, rough))), 0))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 2.2)}


FAMILIES = {
    "T_EchoesCeramicCivic": ceramic_civic,
    "T_EchoesVitrifiedGlass": vitrified_glass,
    "T_EchoesCausewayAsh": causeway_ash,
}


def render_family(name: str) -> dict[str, bytes]:
    maps = FAMILIES[name]()
    return {suffix: encode_png(pixels) for suffix, pixels in maps.items()}


if __name__ == "__main__":
    import hashlib
    import os
    import sys

    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "Content", "Art", "Source", "Textures",
    )
    os.makedirs(out_dir, exist_ok=True)
    for family in FAMILIES:
        for suffix, payload in render_family(family).items():
            path = os.path.join(out_dir, f"{family}_{suffix}.png")
            action = "written"
            if os.path.exists(path):
                with open(path, "rb") as existing:
                    if existing.read() == payload:
                        action = "reused"
            if action == "written":
                with open(path, "wb") as output:
                    output.write(payload)
            digest = hashlib.sha256(payload).hexdigest()
            print(
                f"[ECHOES_TEXTURE_SOURCE] map={family}_{suffix} "
                f"revision={REVISION_TEXTURES} action={action} "
                f"bytes={len(payload)} sha256={digest}"
            )
    print(f"[ECHOES_TEXTURE_SOURCE_READY] families={len(FAMILIES)} maps={len(FAMILIES) * 3} size={SIZE}")
