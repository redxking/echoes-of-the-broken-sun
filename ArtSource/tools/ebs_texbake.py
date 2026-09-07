#!/usr/bin/env python3
"""ebs_texbake.py - per-asset UV-atlas texture baker for the isolated
asset-production pipeline.

Author: Angelis Pseftis

Bakes the 1024x1024 texture set of one production asset from its UV-atlas
bake manifest (``bake-manifest.json``).  Every chart of the atlas is painted
by evaluating the project's procedural surface recipes (the registered
families in ``Scripts/echoes_texture_synth.py``: ``ceramic_civic`` and
``compact_metal``, plus the status emissive) in WORLD SPACE, so adjacent
charts of one surface stay continuous, and then applying the chart-specific
decal rules (numbered panels, grid markings, collar segment ids, pulse
gradient, indicator strips, team band).

Standard library only.  Deterministic: identical manifest + size + maps give
byte-identical PNGs (fixed zlib level 6, filter 0, no timestamps in the
image payloads).

World frame: Unreal +X forward, +Y right, +Z up, centimetres.  Atlas pixel
origin top-left, v DOWN.  A texel (px, py) inside a chart cell maps to
    P = origin_cm + u_dir * ((px - rect.x) mod cell_w) / density
                  + v_dir * ((py - rect.y) / density)
                  + normal * n_offset
where ``n_offset`` restores the plane's coordinate along its normal from
``polygon_world`` (the manifest's ``origin_cm`` carries only the in-plane
origin).  Noise coordinates are the world position projected on the chart's
own axes (s = P.u_dir, t = P.v_dir) so charts that share a plane and axis
set sample one continuous field; the tiling period is 256 cm.

Outputs (RGB8 PNG):
    T_<asset>_BaseColor.png   sRGB-encoded colour
    T_<asset>_Normal.png      tangent-space normal, DirectX / Unreal
                              convention (+G = tilt toward +v_dir, image down)
    T_<asset>_MRE.png         R metallic, G roughness, B emissive mask
    T_<asset>_StateMask.png   R collar-segment id band, G pulse/indicator,
                              B team-colour mask
    T_<asset>_AtlasDebug.png  chart rectangles by slot colour, 1-px outline,
                              4x4 checker
    bake-report.json          hashes, channel statistics, coverage, rule
                              counts, timing

Usage:
    python3 ebs_texbake.py --manifest <bake-manifest.json> --out <dir>
                           [--size 1024] [--maps basecolor,normal,mre,statemask,debug]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
import sys
import time
import zlib

AUTHOR = "Angelis Pseftis"
TOOL_NAME = "ebs_texbake.py"
TOOL_REVISION = "ebs-texbake-v3"  # v3: ember-dark amber base (glow on the emissive), team_band rule; v2: kharuun families, unit StateMask, MoltBlend, emissive-area accounting

ALL_MAPS = ("basecolor", "normal", "mre", "statemask", "debug")
MAP_SUFFIX = {
    "basecolor": "BaseColor",
    "normal": "Normal",
    "mre": "MRE",
    "statemask": "StateMask",
    "debug": "AtlasDebug",
}

# World-space tiling of the noise fields (one noise period per 256 cm).
NOISE_PERIOD_CM = 256.0
# Panel period of the generic ceramic bevel grid (four panels per period).
CERAMIC_PANEL_CM = 64.0
# Seeds are those of the registered recipes in echoes_texture_synth.py.
SEED_CERAMIC = 101
SEED_METAL = 505
SEED_KHARUUN = 606  # the registered kharuun_mineral recipe's seed (echoes_texture_synth.py)
# Normal-map strengths mirror the recipes (height units are the recipes').
NORMAL_STRENGTH = {"ceramic_civic": 2.6, "compact_metal": 2.2, "status_emissive": 1.0,
                   "kharuun_obsidian": 2.4, "kharuun_amber": 1.2, None: 1.0}

# Kharuun unit families (REL-ART-029; card REL-ART-005.KA.RIFTSTALKER .TEX_MAPS / .MAT_RULE).
#   kharuun_obsidian  opaque volcanic value mask over a charcoal body (0.02-0.07 linear, the
#                     Charcoal anchor), warped strata bands, a high-frequency fractured cell field
#                     for the "fractured detail normal", micro-noise grit. NO emissive: the cracks
#                     carry only an ember-dim, matte amber tint in their bottoms.
#   kharuun_amber     Broken-Sun Amber seams: the ONLY emissive, so the <=15% ceiling is governed
#                     by the geometry that carries this slot. Key-light weighting (1.0, 0.82, 0.62),
#                     ember weighting (0.50, 0.22, 0.06) from Docs/ArtDirection.md.
OBSIDIAN_BODY = 0.026            # charcoal body, linear; with the band and grit the plates span ~0.03-0.07
OBSIDIAN_STRATA_CM = 48.0        # one strata band per 48 cm down the plate
OBSIDIAN_CELL_CM = 12.0          # fracture cell size
AMBER_KEY = (1.0, 0.82, 0.62)
AMBER_EMBER = (0.50, 0.22, 0.06)

# Neutral values for pixels no chart paints.
NEUTRAL_BASE_LINEAR = 0.5
NEUTRAL_ROUGHNESS = 0.8

STATUS_BASE = (0.16, 0.86, 0.96)
BOTTOM_WEAR_CM = 20.0

# Components that get subtle dust/scuff wear in their bottom 20 cm (wall charts).
BOTTOM_WEAR_PREFIXES = (
    "plinth_cladding", "plinth", "plinth_step", "foundation_ring", "shaft_row_",
    "shaft_groove_", "service_bay", "frame_rail_", "cap", "coupling_", "rear_",
    "conduit_",
)

# Debug-map slot colours (sRGB bytes).
DEBUG_SLOT_COLOURS = {
    "ceramic_civic": (236, 222, 170),
    "compact_metal": (96, 110, 128),
    "status_emissive": (40, 200, 230),
    "kharuun_obsidian": (58, 52, 50),
    "kharuun_amber": (250, 170, 60),
    None: (200, 60, 200),
}

# Optional test hook: callable(chart_job, px, py, lu_cm, lv_cm, P) -> float
# added to the procedural height field.  Left None in production.
EXTRA_HEIGHT = None

_MASK = 0xFFFFFFFF


# --- Deterministic noise (same lattice hash as echoes_texture_synth.py) -----
def _hash2(ix: int, iy: int, seed: int) -> float:
    h = (ix * 374761393 + iy * 668265263 + seed * 2246822519) & _MASK
    h = (h ^ (h >> 13)) * 1274126177 & _MASK
    return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0


def value_noise(x: float, y: float, seed: int) -> float:
    ix, iy = math.floor(x), math.floor(y)
    fx, fy = x - ix, y - iy
    sx = fx * fx * (3.0 - 2.0 * fx)
    sy = fy * fy * (3.0 - 2.0 * fy)
    a = _hash2(ix, iy, seed)
    b = _hash2(ix + 1, iy, seed)
    c = _hash2(ix, iy + 1, seed)
    d = _hash2(ix + 1, iy + 1, seed)
    return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy


def fbm(x: float, y: float, seed: int, octaves: int = 4) -> float:
    """Fractal value noise, numerically identical to the recipe's ``fbm``
    but with the lattice hash inlined (this is the per-texel hot path)."""
    total = 0.0
    amplitude = 1.0
    frequency = 1.0
    norm = 0.0
    floor = math.floor
    for octave in range(octaves):
        s = seed + octave
        xx = x * frequency
        yy = y * frequency
        ix = floor(xx)
        iy = floor(yy)
        fx = xx - ix
        fy = yy - iy
        sx = fx * fx * (3.0 - 2.0 * fx)
        sy = fy * fy * (3.0 - 2.0 * fy)
        row0 = iy * 668265263 + s * 2246822519
        row1 = row0 + 668265263
        col0 = ix * 374761393
        col1 = col0 + 374761393
        h = (col0 + row0) & _MASK
        h = (h ^ (h >> 13)) * 1274126177 & _MASK
        a = ((h ^ (h >> 16)) & 0xFFFF) / 65535.0
        h = (col1 + row0) & _MASK
        h = (h ^ (h >> 13)) * 1274126177 & _MASK
        b = ((h ^ (h >> 16)) & 0xFFFF) / 65535.0
        h = (col0 + row1) & _MASK
        h = (h ^ (h >> 13)) * 1274126177 & _MASK
        c = ((h ^ (h >> 16)) & 0xFFFF) / 65535.0
        h = (col1 + row1) & _MASK
        h = (h ^ (h >> 13)) * 1274126177 & _MASK
        d = ((h ^ (h >> 16)) & 0xFFFF) / 65535.0
        total += amplitude * ((a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy)
        norm += amplitude
        amplitude *= 0.5
        frequency *= 2.0
    return total / norm


# --- Colour helpers ---------------------------------------------------------
def _build_srgb_lut(steps: int = 4096) -> list[int]:
    lut = []
    for i in range(steps):
        v = i / (steps - 1)
        s = 12.92 * v if v <= 0.0031308 else 1.055 * (v ** (1.0 / 2.4)) - 0.055
        lut.append(max(0, min(255, int(round(s * 255.0)))))
    return lut


SRGB_LUT = _build_srgb_lut()
_SRGB_STEPS = len(SRGB_LUT) - 1


def linear_to_srgb8(v: float) -> int:
    if v <= 0.0:
        return 0
    if v >= 1.0:
        return 255
    return SRGB_LUT[int(v * _SRGB_STEPS + 0.5)]


def clamp8(v: float) -> int:
    return max(0, min(255, int(round(v * 255.0))))


# --- PNG ----------------------------------------------------------------------
def encode_png_rgb(buf: bytes | bytearray, width: int, height: int) -> bytes:
    """RGB8 PNG, filter 0 on every scanline, zlib level 6: deterministic."""
    stride = width * 3
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw += buf[y * stride:(y + 1) * stride]
    compressed = zlib.compress(bytes(raw), 6)

    def chunk(tag: bytes, payload: bytes) -> bytes:
        body = tag + payload
        return struct.pack(">I", len(payload)) + body + struct.pack(">I", zlib.crc32(body) & _MASK)

    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", compressed) + chunk(b"IEND", b"")


def decode_png_rgb(data: bytes) -> tuple[int, int, bytes]:
    """Minimal decoder for the PNGs this tool writes (filter 0 only)."""
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos = 8
    width = height = 0
    idat = bytearray()
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        if tag == b"IHDR":
            width, height = struct.unpack(">II", payload[:8])
        elif tag == b"IDAT":
            idat += payload
        elif tag == b"IEND":
            break
        pos += 12 + length
    raw = zlib.decompress(bytes(idat))
    stride = width * 3
    out = bytearray()
    for y in range(height):
        row = raw[y * (stride + 1):(y + 1) * (stride + 1)]
        if row[0] != 0:
            raise ValueError("unsupported PNG filter")
        out += row[1:]
    return width, height, bytes(out)


# --- Seven-segment numerals ---------------------------------------------------
# a top, b upper-right, c lower-right, d bottom, e lower-left, f upper-left, g mid
SEVEN_SEG = {
    "0": "abcdef", "1": "bc", "2": "abged", "3": "abgcd", "4": "fgbc",
    "5": "afgcd", "6": "afgedc", "7": "abc", "8": "abcdefg", "9": "abcdfg",
}


def glyph_layout(cell_w: int, cell_h: int, text: str, stroke: int = 2) -> tuple[list[tuple[int, int, int, int]], int, int]:
    """Segment rectangles (x0, y0, x1, y1 exclusive, cell-local px) for the
    seven-segment rendering of ``text`` centred in a cell.  Glyph height is
    ~70 % of the cell but never below 4 strokes, so 2-px strokes keep 1-px
    gaps between the three horizontal bars."""
    gh = max(int(round(0.7 * cell_h)), 4 * stroke)
    gh = min(gh, cell_h)
    dw = max(2 * stroke + 1, int(round(gh * 0.62)))
    gap = max(1, int(round(dw * 0.25)))
    total_w = len(text) * dw + (len(text) - 1) * gap
    x_off = max(0, (cell_w - total_w) // 2)
    y_off = max(0, (cell_h - gh) // 2)
    mid0 = (gh - stroke) // 2
    rects = []
    for i, ch in enumerate(text):
        segs = SEVEN_SEG.get(ch, "")
        x0 = x_off + i * (dw + gap)
        y0 = y_off
        if "a" in segs:
            rects.append((x0, y0, x0 + dw, y0 + stroke))
        if "g" in segs:
            rects.append((x0, y0 + mid0, x0 + dw, y0 + mid0 + stroke))
        if "d" in segs:
            rects.append((x0, y0 + gh - stroke, x0 + dw, y0 + gh))
        if "f" in segs:
            rects.append((x0, y0, x0 + stroke, y0 + mid0 + stroke))
        if "e" in segs:
            rects.append((x0, y0 + mid0, x0 + stroke, y0 + gh))
        if "b" in segs:
            rects.append((x0 + dw - stroke, y0, x0 + dw, y0 + mid0 + stroke))
        if "c" in segs:
            rects.append((x0 + dw - stroke, y0 + mid0, x0 + dw, y0 + gh))
    return rects, gh, total_w


def glyph_mask(cell_w: int, cell_h: int, text: str) -> list[int]:
    rects, _gh, _tw = glyph_layout(cell_w, cell_h, text)
    mask = [0] * (cell_w * cell_h)
    for x0, y0, x1, y1 in rects:
        for y in range(max(0, y0), min(cell_h, y1)):
            row = y * cell_w
            for x in range(max(0, x0), min(cell_w, x1)):
                mask[row + x] = 1
    return mask


# --- Chart preparation ----------------------------------------------------------
class ChartJob:
    """Precomputed per-chart frame, polygon edges and rule flags.

    ``vertex_color`` is the chart's authored COLOR_0 (RGBA) when the manifest carries one; the unit
    StateMask mirrors its R (molt sweep order) so texture and vertex data cannot disagree. COLOR_0
    stays authoritative. ``team`` marks a team-colour carrier chart (StateMask B)."""

    __slots__ = (
        "id", "component", "slot", "family", "rx", "ry", "rw", "rh", "cell_w", "cell_h",
        "cells", "origin", "u_dir", "v_dir", "normal", "size_cm", "poly_px", "edges",
        "rules", "collar_k", "bbox", "n_offset", "meshes", "vertex_color", "team", "team_band",
    )

    def __init__(self) -> None:
        self.vertex_color = None
        self.team = False
        self.team_band = None
        self.rules: list[str] = []
        self.collar_k = 0
        self.n_offset = 0.0
        self.edges: list[tuple[float, float, float]] = []


def _dot(a, b) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _polygon_edges(poly: list[tuple[float, float]]) -> list[tuple[float, float, float]] | None:
    n = len(poly)
    if n < 3:
        return None
    area = 0.0
    cx = cy = 0.0
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        area += x0 * y1 - x1 * y0
        cx += x0
        cy += y0
    if abs(area) < 1e-9:
        return None
    cx /= n
    cy /= n
    edges = []
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        ex, ey = x1 - x0, y1 - y0
        length = math.hypot(ex, ey)
        if length < 1e-9:
            continue
        nx, ny = -ey / length, ex / length
        c = -(nx * x0 + ny * y0)
        if nx * cx + ny * cy + c < 0:
            nx, ny, c = -nx, -ny, -c
        edges.append((nx, ny, c))
    return edges


def classify_chart(chart: dict, slot_families: dict) -> tuple[str | None, list[str], int]:
    """Return (family, matched rule names, collar segment index)."""
    component = str(chart.get("component", ""))
    slot = str(chart.get("slot", ""))
    family = slot_families.get(slot)
    normal = chart.get("normal", [0, 0, 1])
    cells = int(chart.get("cells", 1))
    rules: list[str] = []
    collar_k = 0
    if family is not None:
        rules.append("family:" + family)
    if component == "panel_plate":
        rules.append("panel_plate_edge_wear")
        if normal[0] > 0.9:
            rules.append("panel_plate_grid")
    if component == "panel_label" and cells == 4:
        rules.append("panel_label_numerals")
    if component.startswith("collar_segment_"):
        try:
            collar_k = int(component.rsplit("_", 1)[1])
        except ValueError:
            collar_k = 0
        rules.append("collar_segment")
    if component == "conduit_pulse_strip":
        rules.append("conduit_pulse")
    if component in ("coupling_left", "coupling_right") and family == "status_emissive":
        rules.append("coupling_indicator")
    if component == "team_band":
        rules.append("team_band")
    if family == "kharuun_obsidian":
        if component.startswith("molt_plate_") or component.startswith("molt_striker_vane_"):
            rules.append("kharuun_fresh_growth")
        if component.endswith("_foot") or component.endswith("_lower"):
            rules.append("kharuun_foot_wear")
    if family == "kharuun_amber":
        if component == "caster_slot":
            rules.append("kharuun_caster_heat")
        else:
            rules.append("kharuun_seam")
    if family == "status_emissive" and not any(
        r in rules for r in ("collar_segment", "conduit_pulse", "coupling_indicator")
    ):
        rules.append("status_generic_indicator")
    if component.startswith(BOTTOM_WEAR_PREFIXES) and abs(normal[2]) < 0.5 and family in (
        "ceramic_civic", "compact_metal"
    ) and component not in ("panel_plate", "panel_label", "team_band"):
        rules.append("bottom_wear")
    return family, rules, collar_k


def prepare_charts(manifest: dict, size: int) -> tuple[list[ChartJob], float, int]:
    atlas = manifest["atlas"]
    atlas_size = int(atlas.get("size", size))
    scale = size / float(atlas_size)
    density = float(atlas["density_px_per_cm"]) * scale
    gutter = int(atlas.get("gutter_px", 2))
    slot_families = manifest.get("slot_families", {})
    jobs = []
    for chart in atlas["charts"]:
        job = ChartJob()
        job.id = chart.get("id")
        job.component = str(chart.get("component", ""))
        job.slot = str(chart.get("slot", ""))
        job.meshes = list(chart.get("meshes", []))
        rx, ry, rw, rh = chart["rect_px"]
        job.rx, job.ry, job.rw, job.rh = rx * scale, ry * scale, rw * scale, rh * scale
        cw, ch = chart.get("cell_px", [rw, rh])
        job.cell_w, job.cell_h = cw * scale, ch * scale
        job.cells = max(1, int(chart.get("cells", 1)))
        job.origin = tuple(float(v) for v in chart["origin_cm"])
        job.u_dir = tuple(float(v) for v in chart["u_dir"])
        job.v_dir = tuple(float(v) for v in chart["v_dir"])
        job.normal = tuple(float(v) for v in chart["normal"])
        job.size_cm = tuple(float(v) for v in chart.get("size_cm", [job.cell_w / density, job.cell_h / density]))
        job.poly_px = [(float(u) * size, float(v) * size) for u, v in chart["polygon_uv"]]
        job.edges = _polygon_edges(job.poly_px) or []
        pw = chart.get("polygon_world") or []
        if pw:
            delta = (pw[0][0] - job.origin[0], pw[0][1] - job.origin[1], pw[0][2] - job.origin[2])
            job.n_offset = _dot(delta, job.normal)
        job.family, job.rules, job.collar_k = classify_chart(chart, slot_families)
        vc = chart.get("vertex_color")
        job.vertex_color = tuple(float(c) for c in vc) if vc else None
        job.team = job.component in set(manifest.get("team_components", []))
        # Optional band: {component: [v_start, v_end]} in 0..1 of the chart's v extent. A whole shell
        # as a carrier replaced the obsidian identity on the most visible surface; a band keeps the
        # ownership read and the surface.
        band = manifest.get("team_band", {}).get(job.component)
        job.team_band = (float(band[0]), float(band[1])) if band else None
        if job.team:
            job.rules.append("team_band" if band else "team_carrier")
        xs = [p[0] for p in job.poly_px]
        ys = [p[1] for p in job.poly_px]
        if xs:
            job.bbox = (min(xs), min(ys), max(xs) + (job.cells - 1) * job.cell_w, max(ys))
        else:
            job.bbox = None
        jobs.append(job)
    return jobs, density, gutter


# --- Bake ---------------------------------------------------------------------------
class BakeResult:
    def __init__(self, size: int) -> None:
        self.size = size
        n = size * size
        self.base = bytearray(bytes((linear_to_srgb8(NEUTRAL_BASE_LINEAR),) * 3) * n)
        self.normal = bytearray(bytes((128, 128, 255)) * n)
        self.mre = bytearray(bytes((0, clamp8(NEUTRAL_ROUGHNESS), 0)) * n)
        self.state = bytearray(n * 3)
        self.debug = bytearray(bytes((24, 24, 28)) * n)
        self.status = bytearray(n)  # 0 unpainted, 1 gutter, 2 polygon
        self.gutter_q = bytearray(n)
        self.rule_counts: dict[str, int] = {}
        self.unmatched: list[dict] = []
        self.charts_total = 0
        self.charts_skipped: list[dict] = []
        self.elapsed = 0.0
        # Painted-polygon texels whose MRE.B (emissive mask) is non-zero. At uniform density this
        # is the emissive fraction by WORLD AREA, which is what the <=15% ceiling is written against.
        self.emissive_polygon_px = 0

    def buffers(self) -> dict[str, bytearray]:
        return {
            "basecolor": self.base,
            "normal": self.normal,
            "mre": self.mre,
            "statemask": self.state,
            "debug": self.debug,
        }


def _bake_chart(job: ChartJob, res: BakeResult, density: float, gutter: int, extra_height) -> None:
    size = res.size
    if not job.edges or job.bbox is None:
        res.charts_skipped.append({"id": job.id, "component": job.component, "reason": "degenerate polygon"})
        return
    family = job.family
    rules = job.rules
    if family is None and not rules:
        return

    pad = gutter + 2  # gutter + 1 dilation, + 1 ring for the height derivative
    bx0, by0, bx1, by1 = job.bbox
    x0 = max(0, int(math.floor(bx0)) - pad)
    y0 = max(0, int(math.floor(by0)) - pad)
    x1 = min(size, int(math.ceil(bx1)) + pad)
    y1 = min(size, int(math.ceil(by1)) + pad)
    W = x1 - x0
    H = y1 - y0
    if W <= 0 or H <= 0:
        return
    npx = W * H

    rx, ry = job.rx, job.ry
    cell_w, cell_h = job.cell_w, job.cell_h
    cells = job.cells
    ox, oy, oz = job.origin
    ux, uy, uz = job.u_dir
    vx, vy, vz = job.v_dir
    nx_, ny_, nz_ = job.normal
    n_off = job.n_offset
    edges = job.edges
    gut_limit = -(gutter + 1) - 0.5
    inv_density = 1.0 / density
    inv_period = 1.0 / NOISE_PERIOD_CM
    inv_panel = 1.0 / CERAMIC_PANEL_CM
    size_w, size_h = job.size_cm

    is_ceramic = family == "ceramic_civic"
    is_metal = family == "compact_metal"
    is_status = family == "status_emissive"
    is_obsidian = family == "kharuun_obsidian"
    is_amber = family == "kharuun_amber"
    r_fresh = "kharuun_fresh_growth" in rules
    r_foot = "kharuun_foot_wear" in rules
    r_caster = "kharuun_caster_heat" in rules
    r_seam = "kharuun_seam" in rules
    vc = job.vertex_color
    seed_k = SEED_KHARUUN
    inv_strata = 1.0 / OBSIDIAN_STRATA_CM
    inv_cell = 1.0 / OBSIDIAN_CELL_CM
    tau = math.tau
    r_plate_wear = "panel_plate_edge_wear" in rules
    r_plate_grid = "panel_plate_grid" in rules
    r_numerals = "panel_label_numerals" in rules
    r_collar = "collar_segment" in rules
    r_pulse = "conduit_pulse" in rules
    r_coupling = "coupling_indicator" in rules
    r_team = "team_band" in rules
    r_status_generic = "status_generic_indicator" in rules
    r_bottom = "bottom_wear" in rules

    # Numeral masks per cell (cell-local pixel grid).
    numeral_masks = None
    icw = ich = 0
    if r_numerals:
        icw = max(1, int(round(cell_w)))
        ich = max(1, int(round(cell_h)))
        numeral_masks = [glyph_mask(icw, ich, "%02d" % (k + 1)) for k in range(cells)]

    # Plate grid stroke guards (never thinner than ~1.1 px at the bake density).
    min_stroke_cm = 1.1 * inv_density
    border_in = 6.0
    border_out = 6.0 + max(3.0, min_stroke_cm)
    rule_half = max(1.0, 0.55 * inv_density)
    bracket_in = 12.0
    bracket_out = 12.0 + max(3.0, min_stroke_cm)
    bracket_arm = 22.0

    collar_r = (job.collar_k / 8.0) if r_collar else 0.0

    fbm_ = fbm
    floor = math.floor
    sin = math.sin
    sqrt_ = math.sqrt
    hash2 = _hash2
    seed_c = SEED_CERAMIC
    seed_m = SEED_METAL

    h_loc = [0.0] * npx
    stat_loc = bytearray(npx)
    q_loc = bytearray(npx)
    rgb_loc = bytearray(npx * 3)
    mre_loc = bytearray(npx * 3)
    st_loc = bytearray(npx * 3)
    lut = SRGB_LUT
    steps = _SRGB_STEPS

    chart_hash = hash2(int(job.id) if isinstance(job.id, int) else 0, 7, seed_c)

    for ly in range(H):
        py = y0 + ly
        lv_px = py + 0.5 - ry
        lv_cm = lv_px * inv_density
        qy = py + 0.5
        row = ly * W
        for lx in range(W):
            px = x0 + lx
            du = px + 0.5 - rx
            k = int(du // cell_w)
            if k < 0:
                k = 0
            elif k >= cells:
                k = cells - 1
            lu_px = du - k * cell_w
            lu_cm = lu_px * inv_density
            qx = rx + lu_px
            d = 1e9
            for enx, eny, ec in edges:
                dd = enx * qx + eny * qy + ec
                if dd < d:
                    d = dd
            if d > -0.5:
                status = 2
            elif d > gut_limit:
                status = 1
            else:
                status = 0
            i = row + lx
            stat_loc[i] = status
            if status == 1:
                q_loc[i] = max(1, min(255, int((d + 4.5) * 40.0)))

            # World position of the texel and its in-plane noise coordinates.
            Px = ox + ux * lu_cm + vx * lv_cm + nx_ * n_off
            Py = oy + uy * lu_cm + vy * lv_cm + ny_ * n_off
            Pz = oz + uz * lu_cm + vz * lv_cm + nz_ * n_off
            s = Px * ux + Py * uy + Pz * uz
            t = Px * vx + Py * vy + Pz * vz
            u = s * inv_period
            v = t * inv_period

            metallic = 0.0
            emissive = 0.0
            sr = sg = sb = 0.0
            height = 0.0

            if is_ceramic:
                if r_plate_wear or r_team:
                    # The plate / band is one panel: edge measured to the chart edge.
                    edge_cm = lu_cm
                    if size_w - lu_cm < edge_cm:
                        edge_cm = size_w - lu_cm
                    if lv_cm < edge_cm:
                        edge_cm = lv_cm
                    if size_h - lv_cm < edge_cm:
                        edge_cm = size_h - lv_cm
                    if edge_cm < 0.0:
                        edge_cm = 0.0
                    edge = edge_cm * inv_panel
                    tone = 0.86 + 0.08 * (chart_hash - 0.5)
                else:
                    ps = s * inv_panel
                    pt = t * inv_panel
                    ips = floor(ps)
                    ipt = floor(pt)
                    pu = ps - ips
                    pv = pt - ipt
                    edge = pu
                    if 1.0 - pu < edge:
                        edge = 1.0 - pu
                    if pv < edge:
                        edge = pv
                    if 1.0 - pv < edge:
                        edge = 1.0 - pv
                    tone = 0.86 + 0.08 * (hash2(ips * 31 + ipt, 7, seed_c) - 0.5)
                grain = fbm_(u * 34.0, v * 34.0, seed_c + 3, 4) * 0.05
                if r_team:
                    bevel = 1.0
                    wear = 0.0
                else:
                    bevel = edge / 0.06
                    if bevel > 1.0:
                        bevel = 1.0
                    wear = 0.22 - edge * 2.2
                    if wear < 0.0:
                        wear = 0.0
                    else:
                        wear *= fbm_(u * 90.0, v * 90.0, seed_c + 9, 3)
                value = tone * (bevel ** 0.12) + grain - wear * 0.8
                height = bevel * 0.7 + grain
                rough = 0.34 + wear * 1.6 + (1.0 - bevel) * 0.22
                metallic = 0.04
                if r_plate_wear:
                    # Edge wear darkening within 4 cm of the plate edges.
                    if edge_cm < 4.0:
                        wf = (4.0 - edge_cm) * 0.25
                        wn = fbm_(u * 120.0, v * 120.0, seed_c + 13, 3)
                        value *= 1.0 - 0.32 * wf * (0.4 + 0.6 * wn)
                        rough += 0.3 * wf
                        height -= 0.12 * wf * wn
                    if r_plate_grid:
                        dark = False
                        # Thin dark border inset 6 cm from the plate edge.
                        if border_in <= edge_cm < border_out:
                            dark = True
                        # Horizontal rule at 25 % height (measured from the bottom).
                        elif border_in <= lu_cm <= size_w - border_in and abs((size_h - lv_cm) - 0.25 * size_h) < rule_half:
                            dark = True
                        else:
                            # Corner registration brackets (L shapes).
                            a = lu_cm if lu_cm < size_w - lu_cm else size_w - lu_cm
                            b = lv_cm if lv_cm < size_h - lv_cm else size_h - lv_cm
                            if (bracket_in <= a < bracket_out and bracket_in <= b < bracket_arm) or (
                                bracket_in <= b < bracket_out and bracket_in <= a < bracket_arm
                            ):
                                dark = True
                        if dark:
                            value = 0.12 + grain * 0.4
                            rough = 0.52
                            height -= 0.02
                if r_team:
                    rough = 0.4
                    sb = 1.0
                if r_bottom and Pz < BOTTOM_WEAR_CM:
                    f = (BOTTOM_WEAR_CM - Pz) / BOTTOM_WEAR_CM
                    if f > 1.0:
                        f = 1.0
                    dust = f * (0.5 + 0.5 * fbm_(u * 40.0, v * 40.0, seed_c + 21, 3))
                    value *= 1.0 - 0.35 * dust
                    rough += 0.25 * dust
                    height -= 0.1 * dust
                if value < 0.0:
                    value = 0.0
                cr, cg, cb = value, value * 0.997, value * 0.993
            elif is_metal:
                if r_numerals:
                    # Numeral label plate: pale ceramic-tone label with dark glyphs.
                    cx = int(lu_px)
                    cy = int(lv_px)
                    grain = fbm_(u * 34.0, v * 34.0, seed_c + 3, 3) * 0.05
                    value = 0.86 + grain
                    rough = 0.4
                    metallic = 0.04
                    height = grain * 0.5
                    if 0 <= cx < icw and 0 <= cy < ich and numeral_masks[k][cy * icw + cx]:
                        value = 0.10 + grain * 0.3
                        rough = 0.5
                        height -= 0.25
                    cr, cg, cb = value, value * 0.997, value * 0.993
                else:
                    ps = s / 51.2
                    pt = t / 85.3333
                    pu = ps - floor(ps)
                    pv = pt - floor(pt)
                    seam = pu
                    if 1.0 - pu < seam:
                        seam = 1.0 - pu
                    if pv < seam:
                        seam = pv
                    if 1.0 - pv < seam:
                        seam = 1.0 - pv
                    seam_mask = 1.0 - seam / 0.05
                    if seam_mask < 0.0:
                        seam_mask = 0.0
                    brush = 0.5 + 0.5 * sin(t * 1.05 + fbm_(u * 3.0, v * 140.0, seed_m + 1, 3) * 6.0)
                    grain = fbm_(u * 180.0, v * 6.0, seed_m + 2, 3) * 0.12 + brush * 0.05
                    grime = fbm_(u * 5.0, v * 5.0, seed_m + 7, 3)
                    tone = 0.05 + grain * 0.18
                    shade = (1.0 - seam_mask * 0.55) * (1.0 - grime * 0.25)
                    cr = tone * 0.95 * shade
                    cg = tone * shade
                    cb = tone * 1.06 * shade
                    height = -seam_mask * 0.6 + grain * 0.3 - grime * 0.05
                    metallic = 0.85 - seam_mask * 0.3
                    rough = 0.45 + grain * 0.6 + seam_mask * 0.2 + grime * 0.08
                    if r_bottom and Pz < BOTTOM_WEAR_CM:
                        f = (BOTTOM_WEAR_CM - Pz) / BOTTOM_WEAR_CM
                        if f > 1.0:
                            f = 1.0
                        dust = f * (0.5 + 0.5 * fbm_(u * 40.0, v * 40.0, seed_m + 21, 3))
                        cr *= 1.0 - 0.3 * dust
                        cg *= 1.0 - 0.3 * dust
                        cb *= 1.0 - 0.3 * dust
                        rough += 0.25 * dust
                        metallic -= 0.2 * dust
                        height -= 0.1 * dust
            elif is_obsidian:
                # Volcanic value mask: warped strata bands down the plate (v), one per 48 cm.
                warp = fbm_(u * 5.0, v * 5.0, seed_k + 1, 4) * 0.9
                strata = sin((t * inv_strata + warp) * tau)
                band = 0.5 + 0.5 * strata
                # High-frequency fractured detail: a cellular field, thin lines on the cell borders.
                cs = s * inv_cell
                ct = t * inv_cell
                ics = floor(cs)
                ict = floor(ct)
                d1 = 9.0
                d2 = 9.0
                for oy in (-1, 0, 1):
                    for ox in (-1, 0, 1):
                        gx = ics + ox
                        gy = ict + oy
                        fx = gx + hash2(gx, gy, seed_k + 11)
                        fy = gy + hash2(gx, gy, seed_k + 13)
                        ddx = fx - cs
                        ddy = fy - ct
                        dd = ddx * ddx + ddy * ddy
                        if dd < d1:
                            d2 = d1
                            d1 = dd
                        elif dd < d2:
                            d2 = dd
                crack = 1.0 - (sqrt_(d2) - sqrt_(d1)) / 0.14
                if crack < 0.0:
                    crack = 0.0
                elif crack > 1.0:
                    crack = 1.0
                grit = fbm_(u * 60.0, v * 60.0, seed_k + 3, 4)
                if r_fresh:
                    # New growth after a molt: fewer fractures, a touch lighter, smoother.
                    crack *= 0.4
                    value = OBSIDIAN_BODY + 0.010 + band * 0.022 + grit * 0.012
                    rough = 0.36 + grit * 0.10 + crack * 0.25
                else:
                    value = OBSIDIAN_BODY + band * 0.022 + grit * 0.012
                    rough = 0.42 + grit * 0.12 + crack * 0.25
                value *= 1.0 - crack * 0.6
                # Ember-dim, matte amber in the crack bottoms. Not emissive: the ceiling is the seams'.
                # "ember-dim": at full strength the tint adds ~0.07 linear to R, keeping the body in the anchor
                ember = crack * (0.5 + 0.5 * fbm_(u * 20.0, v * 20.0, seed_k + 5, 3)) * 0.14
                cr = value + ember * AMBER_EMBER[0]
                cg = value + ember * AMBER_EMBER[1]
                cb = value + ember * AMBER_EMBER[2]
                height = band * 0.10 - crack * 0.5 + grit * 0.08
                metallic = 0.0
                emissive = 0.0
                if r_foot and Pz < 25.0:
                    f = (25.0 - Pz) / 25.0
                    if f > 1.0:
                        f = 1.0
                    dust = f * (0.5 + 0.5 * fbm_(u * 40.0, v * 40.0, seed_k + 21, 3))
                    cr = cr * (1.0 - 0.3 * dust) + 0.05 * dust
                    cg = cg * (1.0 - 0.3 * dust) + 0.045 * dust
                    cb = cb * (1.0 - 0.3 * dust) + 0.04 * dust
                    rough += 0.3 * dust
                    height -= 0.1 * dust
            elif is_amber:
                # A seam is a thin strip: the core runs along its length, bright on the centre line.
                if size_w < size_h:
                    across = lu_cm / size_w if size_w > 1e-9 else 0.5
                    along = lv_cm / size_h if size_h > 1e-9 else 0.0
                else:
                    across = lv_cm / size_h if size_h > 1e-9 else 0.5
                    along = lu_cm / size_w if size_w > 1e-9 else 0.0
                core = 1.0 - abs(across - 0.5) * 2.0
                if core < 0.0:
                    core = 0.0
                flicker = fbm_(u * 40.0, v * 40.0, seed_k + 9, 3)
                glow = 0.55 + 0.45 * core
                # The BASE is ember-dark (the Art Direction vein weighting, 0.50/0.22/0.06, scaled): a pale
                # amber base rendered near-white under the amber key at any emissive strength. The glow
                # itself rides the emissive mask, not the albedo.
                ember_scale = 0.35 + 0.35 * core
                cr = AMBER_EMBER[0] * ember_scale * (0.85 + 0.15 * flicker)
                cg = AMBER_EMBER[1] * ember_scale * (0.85 + 0.15 * flicker)
                cb = AMBER_EMBER[2] * ember_scale
                metallic = 0.0
                rough = 0.26 + (1.0 - core) * 0.2
                emissive = 0.55 + 0.45 * core
                height = -0.2 * (1.0 - core)
                if r_caster:
                    # Muzzle heat carrier: G gradient along the slot, 0 at the breech, 1 at the muzzle.
                    sg = along
                    if sg < 0.0:
                        sg = 0.0
                    elif sg > 1.0:
                        sg = 1.0
            elif is_status:
                cr, cg, cb = STATUS_BASE
                metallic = 0.0
                rough = 0.3
                emissive = 1.0
                height = 0.0
            else:
                cr = cg = cb = NEUTRAL_BASE_LINEAR
                rough = NEUTRAL_ROUGHNESS

            if r_collar:
                sr = collar_r
                sg = 0.0
            if r_pulse:
                sg = (lu_px + 0.5) / (cell_w + 1.0)
                if sg < 0.0:
                    sg = 0.0
                elif sg > 1.0:
                    sg = 1.0
            if r_coupling or r_status_generic:
                sg = 1.0
            if vc is not None:
                # Unit StateMask: R mirrors the authored COLOR_0.R (molt sweep order; COLOR_0 stays
                # authoritative). G is the translucent core blend for the 80-tick window: on new
                # growth (G or B set in COLOR_0) a soft field that peaks at the chart centre, so the
                # skin reads as forming from a core outward; elsewhere 0.
                sr = vc[0]
                if vc[1] > 0.5 or vc[2] > 0.5:
                    cxn = (lu_cm / size_w - 0.5) * 2.0 if size_w > 1e-9 else 0.0
                    cyn = (lv_cm / size_h - 0.5) * 2.0 if size_h > 1e-9 else 0.0
                    rr = cxn * cxn + cyn * cyn
                    sg = 1.0 - rr
                    if sg < 0.0:
                        sg = 0.0
            if job.team:
                if job.team_band is None:
                    sb = 1.0
                else:
                    tv = lv_cm / size_h if size_h > 1e-9 else 0.0
                    sb = 1.0 if job.team_band[0] <= tv <= job.team_band[1] else 0.0

            if extra_height is not None:
                height += extra_height(job, px, py, lu_cm, lv_cm, (Px, Py, Pz))

            h_loc[i] = height
            i3 = i * 3
            rgb_loc[i3] = 0 if cr <= 0.0 else (255 if cr >= 1.0 else lut[int(cr * steps + 0.5)])
            rgb_loc[i3 + 1] = 0 if cg <= 0.0 else (255 if cg >= 1.0 else lut[int(cg * steps + 0.5)])
            rgb_loc[i3 + 2] = 0 if cb <= 0.0 else (255 if cb >= 1.0 else lut[int(cb * steps + 0.5)])
            mre_loc[i3] = max(0, min(255, int(metallic * 255.0 + 0.5)))
            mre_loc[i3 + 1] = max(0, min(255, int(rough * 255.0 + 0.5)))
            mre_loc[i3 + 2] = max(0, min(255, int(emissive * 255.0 + 0.5)))
            st_loc[i3] = max(0, min(255, int(sr * 255.0 + 0.5)))
            st_loc[i3 + 1] = max(0, min(255, int(sg * 255.0 + 0.5)))
            st_loc[i3 + 2] = max(0, min(255, int(sb * 255.0 + 0.5)))

    # Pass 2: normals from the local height field (differentiated along the
    # atlas axes, which are the chart's u_dir / v_dir), then write globals.
    strength = NORMAL_STRENGTH.get(family, 1.0)
    base = res.base
    normal = res.normal
    mre = res.mre
    state = res.state
    gstatus = res.status
    gq = res.gutter_q
    sqrt = math.sqrt
    for ly in range(H):
        row = ly * W
        gy = (y0 + ly) * size
        ym = row - W if ly > 0 else row
        yp = row + W if ly < H - 1 else row
        for lx in range(W):
            i = row + lx
            status = stat_loc[i]
            if status == 0:
                continue
            gi = gy + x0 + lx
            if status == 1:
                gs = gstatus[gi]
                if gs == 2 or (gs == 1 and gq[gi] >= q_loc[i]):
                    continue
                gstatus[gi] = 1
                gq[gi] = q_loc[i]
            else:
                gstatus[gi] = 2
                gq[gi] = 255
                if mre_loc[i * 3 + 2] > 0:
                    res.emissive_polygon_px += 1
            left = h_loc[i - 1] if lx > 0 else h_loc[i]
            right = h_loc[i + 1] if lx < W - 1 else h_loc[i]
            up = h_loc[ym + lx]
            down = h_loc[yp + lx]
            dx = (left - right) * strength
            dy = (up - down) * strength
            length = sqrt(dx * dx + dy * dy + 1.0)
            g3 = gi * 3
            i3 = i * 3
            normal[g3] = max(0, min(255, int((dx / length * 0.5 + 0.5) * 255.0 + 0.5)))
            normal[g3 + 1] = max(0, min(255, int((dy / length * 0.5 + 0.5) * 255.0 + 0.5)))
            normal[g3 + 2] = max(0, min(255, int((1.0 / length * 0.5 + 0.5) * 255.0 + 0.5)))
            base[g3:g3 + 3] = rgb_loc[i3:i3 + 3]
            mre[g3:g3 + 3] = mre_loc[i3:i3 + 3]
            state[g3:g3 + 3] = st_loc[i3:i3 + 3]


def _draw_debug(job: ChartJob, res: BakeResult) -> None:
    size = res.size
    buf = res.debug
    colour = DEBUG_SLOT_COLOURS.get(job.family, DEBUG_SLOT_COLOURS[None])
    x0 = max(0, int(round(job.rx)))
    y0 = max(0, int(round(job.ry)))
    x1 = min(size, int(round(job.rx + job.cell_w * job.cells)))
    y1 = min(size, int(round(job.ry + job.rh)))
    if x1 <= x0 or y1 <= y0:
        return
    w = x1 - x0
    h = y1 - y0
    dark = tuple(int(c * 0.7) for c in colour)
    outline = (255, 255, 255) if job.family is not None else (255, 0, 255)
    for y in range(y0, y1):
        cy = ((y - y0) * 4) // h
        for x in range(x0, x1):
            if x == x0 or y == y0 or x == x1 - 1 or y == y1 - 1:
                c = outline
            else:
                cx = ((x - x0) * 4) // w
                c = colour if (cx + cy) % 2 == 0 else dark
            g3 = (y * size + x) * 3
            buf[g3] = c[0]
            buf[g3 + 1] = c[1]
            buf[g3 + 2] = c[2]


def bake(manifest: dict, size: int = 1024, maps=None, extra_height=None) -> BakeResult:
    """Bake all charts of ``manifest`` into ``size``-square buffers."""
    if extra_height is None:
        extra_height = EXTRA_HEIGHT
    started = time.perf_counter()
    jobs, density, gutter = prepare_charts(manifest, size)
    res = BakeResult(size)
    res.charts_total = len(jobs)
    want = set(maps) if maps else set(ALL_MAPS)
    need_bake = bool(want & {"basecolor", "normal", "mre", "statemask"})
    for job in jobs:
        for rule in job.rules:
            res.rule_counts[rule] = res.rule_counts.get(rule, 0) + 1
        if job.family is None and not job.rules:
            res.unmatched.append({"id": job.id, "component": job.component, "slot": job.slot})
        if need_bake:
            _bake_chart(job, res, density, gutter, extra_height)
        if "debug" in want:
            _draw_debug(job, res)
    res.elapsed = time.perf_counter() - started
    return res


# --- Reporting ----------------------------------------------------------------------
def channel_stats(buf: bytes | bytearray) -> dict:
    out = {}
    n = len(buf) // 3
    for ci, name in enumerate("RGB"):
        ch = buf[ci::3]
        out[name] = {
            "min": min(ch),
            "mean": round(sum(ch) / max(1, n), 3),
            "max": max(ch),
        }
    return out


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for block in iter(lambda: fh.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def write_outputs(res: BakeResult, out_dir: str, asset_id: str, maps, manifest_path: str | None,
                  manifest: dict, wall_seconds: float) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    prefix = "T_" + asset_id.replace("-", "_")
    want = list(maps) if maps else list(ALL_MAPS)
    buffers = res.buffers()
    report_maps = {}
    written = {}
    for name in want:
        if name not in buffers:
            continue
        png = encode_png_rgb(buffers[name], res.size, res.size)
        fname = f"{prefix}_{MAP_SUFFIX[name]}.png"
        path = os.path.join(out_dir, fname)
        with open(path, "wb") as fh:
            fh.write(png)
        written[name] = path
        report_maps[name] = {
            "file": fname,
            "bytes": len(png),
            "sha256": sha256_bytes(png),
            "channels": channel_stats(buffers[name]),
        }
    n = res.size * res.size
    polygon_px = res.status.count(2)
    gutter_px = res.status.count(1)
    report = {
        "author": AUTHOR,
        "creator": AUTHOR,
        "tool": TOOL_NAME,
        "tool_revision": TOOL_REVISION,
        "tool_sha256": sha256_file(os.path.abspath(__file__)),
        "manifest": os.path.abspath(manifest_path) if manifest_path else None,
        "manifest_sha256": sha256_file(manifest_path) if manifest_path else None,
        "production_asset_id": asset_id,
        "manifest_revision": manifest.get("revision"),
        "kit_revision": manifest.get("kit_revision"),
        "size": res.size,
        "maps": report_maps,
        "coverage": {
            "polygon_fraction": round(polygon_px / n, 6),
            "painted_fraction": round((polygon_px + gutter_px) / n, 6),
            "polygon_pixels": polygon_px,
            "gutter_pixels": gutter_px,
            "manifest_used_fraction": manifest.get("atlas", {}).get("used_fraction"),
        },
        "emissive": {
            "polygon_pixels_with_emissive_mask": res.emissive_polygon_px,
            "fraction_of_painted_polygon_area": round(res.emissive_polygon_px / max(1, polygon_px), 6),
            "note": ("At uniform texel density this is the emissive fraction by world surface area, "
                     "measured on the BAKED mask rather than inferred from slot geometry."),
        },
        "charts_total": res.charts_total,
        "rule_counts": dict(sorted(res.rule_counts.items())),
        "charts_unmatched": res.unmatched,
        "charts_skipped": res.charts_skipped,
        "elapsed_seconds": round(wall_seconds, 3),
        "bake_seconds": round(res.elapsed, 3),
        "conventions": {
            "basecolor": "sRGB-encoded",
            "normal": "tangent-space, DirectX/Unreal (+G = tilt toward +v_dir, image down); flat = (128,128,255)",
            "mre": "R metallic, G roughness, B emissive mask (linear)",
            "statemask": ("R collar segment k/8 (structures) or molt sweep order mirrored from COLOR_0 (units), "
                          "G pulse/indicator (structures) or translucent core blend (units), B team mask (linear)"),
            "noise_space": "world cm projected on chart u_dir/v_dir, period 256 cm",
        },
    }
    blend_size = int(manifest.get("molt_blend_size", 0) or 0)
    if blend_size and "statemask" in buffers:
        # The card's secondary translucent core blending skin mask: the StateMask box-filtered to
        # blend_size^2 (R sweep order, G core blend, B team), written as its own map.
        src = buffers["statemask"]
        S = res.size
        k = max(1, S // blend_size)
        out = bytearray(blend_size * blend_size * 3)
        inv = 1.0 / (k * k)
        for by in range(blend_size):
            for bx in range(blend_size):
                acc0 = acc1 = acc2 = 0
                for yy in range(by * k, by * k + k):
                    row = yy * S
                    for xx in range(bx * k, bx * k + k):
                        i3 = (row + xx) * 3
                        acc0 += src[i3]
                        acc1 += src[i3 + 1]
                        acc2 += src[i3 + 2]
                o3 = (by * blend_size + bx) * 3
                out[o3] = int(acc0 * inv + 0.5)
                out[o3 + 1] = int(acc1 * inv + 0.5)
                out[o3 + 2] = int(acc2 * inv + 0.5)
        png = encode_png_rgb(out, blend_size, blend_size)
        fname = f"{prefix}_MoltBlend.png"
        with open(os.path.join(out_dir, fname), "wb") as fh:
            fh.write(png)
        report_maps["moltblend"] = {"file": fname, "bytes": len(png), "sha256": sha256_bytes(png),
                                    "channels": channel_stats(out), "size": blend_size,
                                    "derivation": f"StateMask box-filtered {S} -> {blend_size}"}
        report["maps"] = report_maps
    report_path = os.path.join(out_dir, "bake-report.json")
    with open(report_path, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2, sort_keys=False)
        fh.write("\n")
    written["report"] = report_path
    return report


# --- CLI ------------------------------------------------------------------------------
def parse_args(argv=None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Bake a per-asset texture set from a UV-atlas bake manifest.")
    parser.add_argument("--manifest", required=True, help="bake-manifest.json path")
    parser.add_argument("--out", required=True, help="output directory")
    parser.add_argument("--size", type=int, default=1024, help="atlas size in pixels (default 1024)")
    parser.add_argument(
        "--maps",
        default=",".join(ALL_MAPS),
        help="comma list of maps to write: " + ",".join(ALL_MAPS),
    )
    return parser.parse_args(argv)


def main(argv=None) -> int:
    args = parse_args(argv)
    maps = [m.strip().lower() for m in args.maps.split(",") if m.strip()]
    bad = [m for m in maps if m not in ALL_MAPS]
    if bad:
        print(f"[EBS_TEXBAKE_ERROR] unknown map(s): {','.join(bad)}", file=sys.stderr)
        return 2
    if args.size < 8 or args.size > 8192:
        print("[EBS_TEXBAKE_ERROR] --size must be within 8..8192", file=sys.stderr)
        return 2
    with open(args.manifest, "r", encoding="utf-8") as fh:
        manifest = json.load(fh)
    asset_id = str(manifest.get("production_asset_id", "EBS_ASSET"))
    started = time.perf_counter()
    res = bake(manifest, size=args.size, maps=maps)
    report = write_outputs(res, args.out, asset_id, maps, args.manifest, manifest, time.perf_counter() - started)
    for name, entry in report["maps"].items():
        print(f"[EBS_TEXBAKE] map={entry['file']} bytes={entry['bytes']} sha256={entry['sha256']}")
    cov = report["coverage"]
    print(
        f"[EBS_TEXBAKE_READY] asset={asset_id} size={args.size} charts={report['charts_total']} "
        f"polygon_fraction={cov['polygon_fraction']} painted_fraction={cov['painted_fraction']} "
        f"unmatched={len(report['charts_unmatched'])} bake_seconds={report['bake_seconds']} "
        f"elapsed_seconds={report['elapsed_seconds']}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
