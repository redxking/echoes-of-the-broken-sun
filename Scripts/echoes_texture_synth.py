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
REVISION_TEXTURES = "surface-textures-v8"


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
            tone = 0.06 + band * 0.028 + ash * 0.04
            track = math.exp(-((v - 0.5) ** 2) / 0.02) * (0.5 + 0.5 * fbm(u * 30, 0.5, seed + 13))
            value = tone + track * 0.03
            base.append((
                _clamp8(value * 1.02),
                _clamp8(value * 0.99),
                _clamp8(value * 0.94),
            ))
            height.append(band * 0.2 + ash * 0.3 - track * 0.15)
            rough = 0.92 - track * 0.22 + ash * 0.08
            mre.append((_clamp8(0.03), _clamp8(max(0.0, min(1.0, rough))), 0))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 1.3)}


def glass_scar_ground() -> dict[str, list[tuple[int, int, int]]]:
    """Glass Scar ground: dark vitrified basalt laced with golden fracture
    veins that echo the Broken Sun, per the site hero reference."""
    seed = 404
    crack = [0.0] * (SIZE * SIZE)
    rng = Xorshift(seed)
    for _ in range(6):
        x = rng.next_float() * SIZE
        y = rng.next_float() * SIZE
        angle = rng.next_float() * math.tau
        for _step in range(1200):
            angle += (rng.next_float() - 0.5) * 0.16
            x = (x + math.cos(angle) * 1.8) % SIZE
            y = (y + math.sin(angle) * 1.8) % SIZE
            for ox in (-2, -1, 0, 1, 2):
                for oy in (-2, -1, 0, 1, 2):
                    dist = abs(ox) + abs(oy)
                    if dist > 2:
                        continue
                    index = int(y + oy) % SIZE * SIZE + int(x + ox) % SIZE
                    fall = (1.0, 0.55, 0.25)[dist]
                    crack[index] = min(1.0, crack[index] + 0.4 * fall)
    base, height, mre = [], [], []
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            body = 0.020 + fbm(u * 10, v * 10, seed + 5) * 0.026
            vein = crack[y * SIZE + x]
            glow = vein ** 2.2
            r = body * 0.9 + glow * 0.50
            g = body * 1.0 + glow * 0.22
            b = body * 1.5 + glow * 0.06
            base.append((_clamp8(r), _clamp8(g), _clamp8(b)))
            height.append(-vein * 0.22 + fbm(u * 18, v * 18, seed + 8) * 0.18)
            rough = 0.85 + fbm(u * 24, v * 24, seed + 11) * 0.1 + vein * 0.05
            mre.append((
                _clamp8(0.02),
                _clamp8(max(0.05, min(1.0, rough))),
                _clamp8(glow),
            ))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 0.9)}


# --- Tiling helpers for the v8 families --------------------------------------
def _scatter(seed: int, count: int) -> list[tuple[float, float]]:
    rng = Xorshift(seed)
    return [(rng.next_float() * SIZE, rng.next_float() * SIZE) for _ in range(count)]


def _wrapped_distance(px: float, py: float, qx: float, qy: float) -> float:
    dx = abs(px - qx)
    dy = abs(py - qy)
    if dx > SIZE * 0.5:
        dx = SIZE - dx
    if dy > SIZE * 0.5:
        dy = SIZE - dy
    return math.sqrt(dx * dx + dy * dy)


def _nearest_two(px: float, py: float, points: list[tuple[float, float]]) -> tuple[int, float, float]:
    """Index and distance of the nearest point, plus the second distance."""
    best_index, best, second = -1, 1e9, 1e9
    for index, (qx, qy) in enumerate(points):
        d = _wrapped_distance(px, py, qx, qy)
        if d < best:
            second = best
            best, best_index = d, index
        elif d < second:
            second = d
    return best_index, best, second


def _periodic_value_noise(x: float, y: float, seed: int, period_x: int, period_y: int) -> float:
    """Value noise whose lattice repeats at the supplied integer periods."""
    ix, iy = int(math.floor(x)), int(math.floor(y))
    fx, fy = x - ix, y - iy
    sx = fx * fx * (3.0 - 2.0 * fx)
    sy = fy * fy * (3.0 - 2.0 * fy)
    a = _hash2(ix % period_x, iy % period_y, seed)
    b = _hash2((ix + 1) % period_x, iy % period_y, seed)
    c = _hash2(ix % period_x, (iy + 1) % period_y, seed)
    d = _hash2((ix + 1) % period_x, (iy + 1) % period_y, seed)
    return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy


def _periodic_fbm(x: float, y: float, seed: int, period_x: int, period_y: int, octaves: int = 4) -> float:
    """Tileable fractal noise; each octave preserves the base repeat."""
    total, amplitude, frequency, norm = 0.0, 1.0, 1, 0.0
    for octave in range(octaves):
        total += amplitude * _periodic_value_noise(
            x * frequency,
            y * frequency,
            seed + octave,
            period_x * frequency,
            period_y * frequency,
        )
        norm += amplitude
        amplitude *= 0.5
        frequency *= 2
    return total / norm


def ceramic_service() -> dict[str, list[tuple[int, int, int]]]:
    """Muted service ceramic: one continuous maintained surface with
    fine aggregate, shallow pits, and traffic-rubbed directional wear.

    This deliberately carries no construction grid or marked panels: the
    loading-apron mesh supplies its own seams and physical detail.
    """
    seed = 1001
    pits = _scatter(seed + 7, 24)
    base, height, mre = [], [], []
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            aggregate = _periodic_fbm(u * 7, v * 7, seed, 7, 7, 4) - 0.5
            fine_aggregate = _periodic_fbm(u * 42, v * 42, seed + 5, 42, 42, 3) - 0.5
            # Fine aligned rubbing left by repeated wheeled service traffic.
            # It is irregular and deliberately low contrast, never a stripe grid.
            rub = _periodic_fbm(u * 5, v * 96, seed + 11, 5, 96, 3) - 0.5
            rub_gate = _periodic_fbm(u * 3, v * 3, seed + 17, 3, 3, 3)
            rub *= 0.55 + rub_gate * 0.45
            _index, distance, _second = _nearest_two(x, y, pits)
            pit = max(0.0, 1.0 - distance / 5.5)
            pit = pit * pit * (3.0 - 2.0 * pit)

            body = 0.625 + aggregate * 0.050 + fine_aggregate * 0.022 + rub * 0.018
            body -= pit * 0.040
            base.append((
                _clamp8(body + 0.010),
                _clamp8(body + 0.002),
                _clamp8(body - 0.012),
            ))
            height.append(aggregate * 0.050 + fine_aggregate * 0.018 + rub * 0.010 - pit * 0.075)
            roughness = 0.775 + aggregate * 0.045 + fine_aggregate * 0.025 - rub * 0.050 + pit * 0.045
            mre.append((_clamp8(0.0), _clamp8(max(0.70, min(0.85, roughness))), 0))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 0.65)}


def compact_metal() -> dict[str, list[tuple[int, int, int]]]:
    """Compact machined metal: brushed grain, milled panel seams, and a
    painted status band whose edges have chipped back to bare metal.
    Engineered load paths — nothing organic, every seam a real joint."""
    seed = 505
    base, height, mre = [], [], []
    panel_u, panel_v = 5, 3
    band_lo, band_hi = 0.62, 0.72
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            pu, pv = (u * panel_u) % 1.0, (v * panel_v) % 1.0
            seam = min(pu, 1 - pu, pv, 1 - pv)
            seam_mask = max(0.0, 1.0 - seam / 0.018)
            brush = 0.5 + 0.5 * math.sin(v * SIZE * 1.7 + fbm(u * 3, v * 140, seed + 1) * 6.0)
            grain = fbm(u * 180, v * 6, seed + 2) * 0.12 + brush * 0.05
            in_band = band_lo <= v <= band_hi
            chip = fbm(u * 70, v * 70, seed + 4)
            band_edge = min(abs(v - band_lo), abs(v - band_hi))
            chipped = in_band and chip > 0.66 - min(0.22, (0.012 / max(band_edge, 0.002)) * 0.08)
            painted = in_band and not chipped
            tone = 0.70 + grain
            if painted:
                r, g, b = 0.90 + grain * 0.4, 0.91 + grain * 0.4, 0.92 + grain * 0.4
            else:
                r, g, b = tone * 0.97, tone, tone * 1.03
            r *= 1.0 - seam_mask * 0.55
            g *= 1.0 - seam_mask * 0.55
            b *= 1.0 - seam_mask * 0.55
            base.append((_clamp8(r), _clamp8(g), _clamp8(b)))
            height.append(-seam_mask * 0.6 + (0.08 if painted else 0.0) + grain * 0.3)
            metallic = 0.10 if painted else 0.86
            rough = (0.52 if painted else 0.36) + (0.16 if chipped else 0.0) + seam_mask * 0.2 + grain * 0.4
            mre.append((_clamp8(metallic), _clamp8(min(1.0, rough)), 0))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 2.2)}


def kharuun_mineral() -> dict[str, list[tuple[int, int, int]]]:
    """Kharuun grown mineral: warped strata bands in charcoal and dark
    umber, with translucent amber nodules that carry the family's glow.
    Inhabited and maintained — the bands are worn smooth along the grain."""
    seed = 606
    nodules = _scatter(seed + 7, 34)
    base, height, mre = [], [], []
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            warp = fbm(u * 5, v * 5, seed + 1) * 0.9
            strata = math.sin((v * 9.0 + warp) * math.tau)
            band = 0.5 + 0.5 * strata
            amber_layer = max(0.0, strata - 0.55) / 0.45
            grit = fbm(u * 60, v * 60, seed + 3)
            _index, d, _second = _nearest_two(x, y, nodules)
            nodule = max(0.0, 1.0 - d / 22.0)
            nodule_core = nodule * nodule
            r = 0.60 + band * 0.20 + amber_layer * 0.16 + grit * 0.05
            g = 0.58 + band * 0.18 + amber_layer * 0.06 + grit * 0.045
            b = 0.57 + band * 0.15 - amber_layer * 0.08 + grit * 0.04
            r = r * (1.0 - nodule) + (0.96 + nodule_core * 0.04) * nodule
            g = g * (1.0 - nodule) + (0.70 + nodule_core * 0.12) * nodule
            b = b * (1.0 - nodule) + (0.34 + nodule_core * 0.10) * nodule
            base.append((_clamp8(r), _clamp8(g), _clamp8(b)))
            height.append(band * 0.28 + nodule_core * 0.6 + grit * 0.12)
            rough = 0.64 + grit * 0.14 - amber_layer * 0.1
            rough = rough * (1.0 - nodule) + 0.26 * nodule
            emissive = nodule_core * 0.9
            mre.append((_clamp8(0.04), _clamp8(max(0.05, min(1.0, rough))), _clamp8(emissive)))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 2.4)}


def choir_coherent() -> dict[str, list[tuple[int, int, int]]]:
    """Hollow Choir coherent-light surface: a near-black body carrying a
    luminous edge lattice, every line accompanied by an offset duplicate —
    the deliberate contradiction of a structure held possible."""
    seed = 707
    base, height, mre = [], [], []
    cells = 6
    offset = 0.035
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            drift = fbm(u * 3, v * 3, seed + 1) * 0.08
            cu, cv = ((u + drift) * cells) % 1.0, ((v - drift) * cells) % 1.0
            edge = min(cu, 1 - cu, cv, 1 - cv)
            line = max(0.0, 1.0 - edge / 0.012)
            du, dv = ((u + drift + offset) * cells) % 1.0, ((v - drift + offset * 0.6) * cells) % 1.0
            edge2 = min(du, 1 - du, dv, 1 - dv)
            line2 = max(0.0, 1.0 - edge2 / 0.008) * 0.55
            lattice = min(1.0, line + line2)
            body = 0.56 + fbm(u * 18, v * 18, seed + 4) * 0.10
            r = body * 0.97 + lattice * 0.40
            g = body * 0.95 + lattice * 0.36
            b = body * 1.04 + lattice * 0.44
            base.append((_clamp8(r), _clamp8(g), _clamp8(b)))
            height.append(line * 0.35 - line2 * 0.2)
            rough = 0.18 + fbm(u * 40, v * 40, seed + 6) * 0.1 + lattice * 0.12
            mre.append((_clamp8(0.0), _clamp8(min(1.0, rough)), _clamp8(lattice)))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 1.8)}


def matter_crystal() -> dict[str, list[tuple[int, int, int]]]:
    """Matter deposit crystal: tiled facets with bright cleavage edges and
    a cyan-white interior glow that pools toward each facet's centre."""
    seed = 808
    facets = _scatter(seed + 3, 52)
    slopes = [(Xorshift(seed + 11 + i).next_float() - 0.5, Xorshift(seed + 37 + i).next_float() - 0.5) for i in range(len(facets))]
    base, height, mre = [], [], []
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            index, d, second = _nearest_two(x, y, facets)
            edge = max(0.0, 1.0 - (second - d) / 4.0)
            interior = max(0.0, 1.0 - d / max(12.0, second * 0.9))
            fx, fy = facets[index]
            sx, sy = slopes[index]
            dx, dy = x - fx, y - fy
            if dx > SIZE * 0.5:
                dx -= SIZE
            elif dx < -SIZE * 0.5:
                dx += SIZE
            if dy > SIZE * 0.5:
                dy -= SIZE
            elif dy < -SIZE * 0.5:
                dy += SIZE
            plane = (dx * sx + dy * sy) / SIZE
            shimmer = fbm(u * 30, v * 30, seed + 5) * 0.06
            r = 0.50 + interior * 0.22 + edge * 0.34 + shimmer
            g = 0.60 + interior * 0.30 + edge * 0.36 + shimmer
            b = 0.64 + interior * 0.32 + edge * 0.36 + shimmer
            base.append((_clamp8(r), _clamp8(g), _clamp8(b)))
            height.append(plane * 4.0 - edge * 0.5)
            rough = 0.12 + edge * 0.22 + shimmer
            emissive = interior ** 1.6 * 0.85 + edge * 0.3
            mre.append((_clamp8(0.05), _clamp8(min(1.0, rough)), _clamp8(min(1.0, emissive))))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 1.6)}


def verge_scored() -> dict[str, list[tuple[int, int, int]]]:
    """Folded Verge plate wear: parallel scoring where displaced plates have
    ground across each other, with ash drifts settled in the gouges.
    Wear is history — the scoring follows the plates' travel, not the grid."""
    seed = 909
    base, height, mre = [], [], []
    angle = 0.42
    ca, sa = math.cos(angle), math.sin(angle)
    for y in range(SIZE):
        for x in range(SIZE):
            u, v = x / SIZE, y / SIZE
            along = (u * ca + v * sa)
            across = (-u * sa + v * ca)
            jitter = fbm(along * 6, across * 90, seed + 2) * 0.012
            score_phase = ((across + jitter) * 54.0) % 1.0
            score = max(0.0, 1.0 - abs(score_phase - 0.5) / 0.09)
            score_gate = 1.0 if fbm(along * 2.5, across * 14, seed + 5) > 0.42 else 0.0
            score *= score_gate
            drift = max(0.0, fbm(u * 7, v * 7, seed + 8) - 0.55) / 0.45
            drift *= 1.0 - score * 0.5
            plate = 0.74 + fbm(u * 22, v * 22, seed + 1) * 0.12
            r = plate * 1.02 - score * 0.26 + drift * 0.16
            g = plate * 1.0 - score * 0.26 + drift * 0.15
            b = plate * 1.04 - score * 0.26 + drift * 0.13
            base.append((_clamp8(r), _clamp8(g), _clamp8(b)))
            height.append(-score * 0.5 + drift * 0.25)
            rough = 0.82 - score * 0.3 + drift * 0.14
            mre.append((_clamp8(0.03), _clamp8(max(0.05, min(1.0, rough))), 0))
    return {"BaseColor": base, "MRE": mre, "Normal": height_to_normal(height, 1.9)}


FAMILIES = {
    "T_EchoesCeramicCivic": ceramic_civic,
    "T_EchoesServiceCeramic": ceramic_service,
    "T_EchoesVitrifiedGlass": vitrified_glass,
    "T_EchoesCausewayAsh": causeway_ash,
    "T_EchoesGlassScarGround": glass_scar_ground,
    "T_EchoesCompactMetal": compact_metal,
    "T_EchoesKharuunMineral": kharuun_mineral,
    "T_EchoesChoirCoherent": choir_coherent,
    "T_EchoesMatterCrystal": matter_crystal,
    "T_EchoesVergeScored": verge_scored,
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
    only = set(sys.argv[2:])
    os.makedirs(out_dir, exist_ok=True)
    for family in FAMILIES:
        if only and family not in only:
            continue
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
