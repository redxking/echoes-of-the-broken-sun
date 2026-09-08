#!/usr/bin/env python3
"""Riftstalker-only mineral texture refinement adapter.

This does not modify :mod:`ebs_texbake`.  It asks the shared baker for the
complete atlas first, then replaces only the final, visible
``kharuun_obsidian`` chart pixels.  Amber, StateMask, team/molt data and all
non-obsidian material families remain byte-for-byte as supplied by the shared
baker.

The replacement is intentionally restrained: broad directional strata, small
value drift and sparse interrupted fissures.  It removes the earlier cellular
Voronoi crack grid, which made the Riftstalker read as a tiled game prop rather
than layered volcanic mineral.

Author: Angelis Pseftis
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import sys
import time
from pathlib import Path

import numpy as np
from PIL import Image

AUTHOR = "Angelis Pseftis"
REVISION = "ebs-riftstalker-mineral-refine-v2"

HERE = Path(__file__).resolve().parent
TOOLS = HERE.parent / "tools"
sys.path.insert(0, str(TOOLS))
import ebs_texbake as baker  # noqa: E402


def sha256(data: bytes | bytearray) -> str:
    return hashlib.sha256(data).hexdigest()


def _noise(x: np.ndarray, y: np.ndarray, seed: float) -> np.ndarray:
    """Deterministic, smooth value noise without a cellular distance field."""
    ix = np.floor(x)
    iy = np.floor(y)
    fx = x - ix
    fy = y - iy

    def h(ax: np.ndarray, ay: np.ndarray) -> np.ndarray:
        # A continuous-looking deterministic lattice hash.  It is deliberately
        # independent of the shared baker's Voronoi implementation.
        return np.mod(np.sin(ax * 127.1 + ay * 311.7 + seed * 19.19) * 43758.5453123, 1.0)

    sx = fx * fx * (3.0 - 2.0 * fx)
    sy = fy * fy * (3.0 - 2.0 * fy)
    a = h(ix, iy)
    b = h(ix + 1.0, iy)
    c = h(ix, iy + 1.0)
    d = h(ix + 1.0, iy + 1.0)
    return (a * (1.0 - sx) + b * sx) * (1.0 - sy) + (c * (1.0 - sx) + d * sx) * sy


def _ownership(jobs: list[baker.ChartJob], size: int, gutter: int) -> np.ndarray:
    """Reproduce the shared baker's chart/gutter precedence as chart indices."""
    owner = np.full((size, size), -1, dtype=np.int32)
    owner_status = np.zeros((size, size), dtype=np.uint8)
    gutter_q = np.zeros((size, size), dtype=np.uint8)
    for ji, job in enumerate(jobs):
        if not job.edges or job.bbox is None:
            continue
        x0, y0, x1, y1 = _job_window(job, size, gutter)
        if x1 <= x0 or y1 <= y0:
            continue
        xx, yy = np.meshgrid(np.arange(x0, x1, dtype=np.float64) + 0.5,
                             np.arange(y0, y1, dtype=np.float64) + 0.5)
        distance = np.full(xx.shape, np.inf, dtype=np.float64)
        for nx, ny, c in job.edges:
            distance = np.minimum(distance, nx * xx + ny * yy + c)
        polygon = distance > -0.5
        gutter_mask = (~polygon) & (distance > (-(gutter + 1) - 0.5))
        dst_status = owner_status[y0:y1, x0:x1]
        dst_owner = owner[y0:y1, x0:x1]
        dst_q = gutter_q[y0:y1, x0:x1]
        # The shared baker always lets a later polygon paint over an earlier
        # chart, including an earlier polygon; a gutter may never paint over a
        # polygon and must have a strictly higher priority than a prior gutter.
        dst_owner[polygon] = ji
        dst_status[polygon] = 2
        dst_q[polygon] = 255
        q = np.clip((distance + 4.5) * 40.0, 1.0, 255.0).astype(np.uint8)
        replace_gutter = gutter_mask & (dst_status != 2) & (q > dst_q)
        dst_owner[replace_gutter] = ji
        dst_status[replace_gutter] = 1
        dst_q[replace_gutter] = q[replace_gutter]
    return owner


def _job_window(job: baker.ChartJob, size: int, gutter: int) -> tuple[int, int, int, int]:
    """The shared baker's bounded raster region for a chart and its gutter."""
    if job.bbox is None:
        return 0, 0, 0, 0
    pad = gutter + 2
    bx0, by0, bx1, by1 = job.bbox
    return (
        max(0, int(math.floor(bx0)) - pad),
        max(0, int(math.floor(by0)) - pad),
        min(size, int(math.ceil(bx1)) + pad),
        min(size, int(math.ceil(by1)) + pad),
    )


def _decode_srgb(values: np.ndarray) -> np.ndarray:
    """Decode texture bytes for the actual, written BaseColor range check."""
    srgb = values.astype(np.float64) / 255.0
    return np.where(srgb <= 0.04045, srgb / 12.92,
                    np.power((srgb + 0.055) / 1.055, 2.4))


def _chart_coordinates(job: baker.ChartJob, x: np.ndarray, y: np.ndarray, density: float) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Return chart-local cm and a single shared world-space field coordinate."""
    inv_density = 1.0 / density
    du = x + 0.5 - job.rx
    cell = np.floor(du / job.cell_w).astype(np.int32)
    cell = np.clip(cell, 0, job.cells - 1)
    lu = (du - cell * job.cell_w) * inv_density
    lv = (y + 0.5 - job.ry) * inv_density
    ox, oy, oz = job.origin
    ux, uy, uz = job.u_dir
    vx, vy, vz = job.v_dir
    nx, ny, nz = job.normal
    px = ox + ux * lu + vx * lv + nx * job.n_offset
    py = oy + uy * lu + vy * lv + ny * job.n_offset
    pz = oz + uz * lu + vz * lv + nz * job.n_offset
    return lu, lv, px, py, pz


def _refine_obsidian(res: baker.BakeResult, jobs: list[baker.ChartJob], density: float, gutter: int) -> dict:
    """Replace only final obsidian pixels and return direct texture measurements."""
    size = res.size
    owner = _ownership(jobs, size, gutter)
    obsidian_indices = [i for i, job in enumerate(jobs) if job.family == "kharuun_obsidian"]
    if not obsidian_indices:
        raise ValueError("manifest contains no kharuun_obsidian charts")

    base = np.frombuffer(res.base, dtype=np.uint8).reshape(size, size, 3)
    normal = np.frombuffer(res.normal, dtype=np.uint8).reshape(size, size, 3)
    mre = np.frombuffer(res.mre, dtype=np.uint8).reshape(size, size, 3)
    obsidian_mask = np.zeros((size, size), dtype=bool)
    painted_obsidian_mask = np.zeros((size, size), dtype=bool)
    linear_values: list[np.ndarray] = []

    for ji in obsidian_indices:
        job = jobs[ji]
        x0, y0, x1, y1 = _job_window(job, size, gutter)
        local_owner = owner[y0:y1, x0:x1]
        ys, xs = np.nonzero(local_owner == ji)
        if xs.size == 0:
            continue
        xs = xs + x0
        ys = ys + y0
        obsidian_mask[ys, xs] = True
        x = xs.astype(np.float64)
        y = ys.astype(np.float64)
        _lu, _lv, wx, wy, wz = _chart_coordinates(job, x, y, density)

        # One world-space field crosses chart boundaries.  Strata follow the
        # world-up direction around the creature, rather than restarting in
        # each triangle's arbitrary local UV frame.
        bend = (_noise(wx / 115.0, wy / 115.0, 31.0) - 0.5) * 11.0
        strata_phase = (wz + bend) / 38.0
        strata = 0.5 + 0.5 * np.sin(math.tau * strata_phase)
        broad = _noise(wx / 170.0, wz / 170.0, 47.0) - 0.5
        # 5.2 cm lamination remains resolvable at the ~0.9 px/cm atlas
        # density.  It is a directional mineral layer, not isotropic speckle.
        lam_bend = (_noise(wx / 32.0, wy / 32.0, 53.0) - 0.5) * 2.2
        lamination = 0.5 + 0.5 * np.sin(math.tau * ((wz + lam_bend) / 5.2))
        grit = _noise(wx / 5.8, wy / 5.8, 59.0) - 0.5

        # Sparse, interrupted fissures follow the strata direction.  They are
        # intentionally few and attenuated by a low-frequency gate.
        fissure_path = np.abs(np.sin(math.tau * ((wz + bend * 1.8 + wx * 0.085) / 79.0)))
        gate = _noise(wx / 92.0, wy / 92.0, 71.0)
        fissure = np.clip((0.060 - fissure_path) / 0.060, 0.0, 1.0)
        fissure *= np.clip((gate - 0.62) / 0.28, 0.0, 1.0)

        # Charcoal is measured in linear space.  No amber tint, metallic or
        # emissive is introduced on obsidian.  The strict 0.02..0.07 anchor is
        # enforced before sRGB encoding.
        lum = (0.030 + strata * 0.028 + broad * 0.014 +
               lamination * 0.010 + grit * 0.006 - fissure * 0.012)
        lum = np.clip(lum, 0.022, 0.068)
        rgb_linear = np.stack((lum * 0.94, lum * 0.98, lum * 1.02), axis=1)
        rgb_linear = np.clip(rgb_linear, 0.020, 0.070)
        linear_values.append(rgb_linear)
        srgb = np.where(rgb_linear <= 0.0031308, rgb_linear * 12.92,
                        1.055 * np.power(rgb_linear, 1.0 / 2.4) - 0.055)
        base[ys, xs] = np.rint(np.clip(srgb * 255.0, 0.0, 255.0)).astype(np.uint8)

        # The normal carries the broad layers and readable fine laminations.
        # It stays shallow enough to avoid a tiled-cell silhouette.
        # Per-point finite differences in the same directional field.  The
        # analytic derivatives avoid accidental atlas-axis seams.
        eps = 0.35
        def h_at(xx: np.ndarray, yy: np.ndarray, zz: np.ndarray) -> np.ndarray:
            bb = (_noise(xx / 115.0, yy / 115.0, 31.0) - 0.5) * 11.0
            st = 0.5 + 0.5 * np.sin(math.tau * ((zz + bb) / 38.0))
            br = _noise(xx / 170.0, zz / 170.0, 47.0) - 0.5
            lb = (_noise(xx / 32.0, yy / 32.0, 53.0) - 0.5) * 2.2
            lam = 0.5 + 0.5 * np.sin(math.tau * ((zz + lb) / 5.2))
            gr = _noise(xx / 5.8, yy / 5.8, 59.0) - 0.5
            path = np.abs(np.sin(math.tau * ((zz + bb * 1.8 + xx * 0.085) / 79.0)))
            gg = _noise(xx / 92.0, yy / 92.0, 71.0)
            fi = np.clip((0.060 - path) / 0.060, 0.0, 1.0) * np.clip((gg - 0.62) / 0.28, 0.0, 1.0)
            return st * 0.23 + br * 0.065 + lam * 0.070 + gr * 0.035 - fi * 0.20
        ux, uy, uz = job.u_dir
        vx, vy, vz = job.v_dir
        ds = (h_at(wx + ux * eps, wy + uy * eps, wz + uz * eps) -
              h_at(wx - ux * eps, wy - uy * eps, wz - uz * eps)) / (2.0 * eps)
        dt = (h_at(wx + vx * eps, wy + vy * eps, wz + vz * eps) -
              h_at(wx - vx * eps, wy - vy * eps, wz - vz * eps)) / (2.0 * eps)
        nx = -ds * 3.0
        ny = -dt * 3.0
        nz = np.ones_like(nx)
        inv_len = 1.0 / np.sqrt(nx * nx + ny * ny + nz * nz)
        normal[ys, xs, 0] = np.rint((nx * inv_len * 0.5 + 0.5) * 255.0).astype(np.uint8)
        normal[ys, xs, 1] = np.rint((ny * inv_len * 0.5 + 0.5) * 255.0).astype(np.uint8)
        normal[ys, xs, 2] = np.rint((nz * inv_len * 0.5 + 0.5) * 255.0).astype(np.uint8)

        roughness = np.clip(0.860 + (1.0 - strata) * 0.075 + fissure * 0.045 + grit * 0.020,
                            217.0 / 255.0, 0.985)
        mre[ys, xs, 0] = 0
        mre[ys, xs, 1] = np.rint(roughness * 255.0).astype(np.uint8)
        mre[ys, xs, 2] = 0

    if not linear_values:
        raise ValueError("no final visible kharuun_obsidian pixels")
    painted_obsidian_mask = obsidian_mask & (np.frombuffer(res.status, dtype=np.uint8).reshape(size, size) == 2)
    if not painted_obsidian_mask.any():
        raise ValueError("no painted kharuun_obsidian polygon pixels")
    linear_all = np.concatenate(linear_values, axis=0)
    roughness_all = mre[:, :, 1][painted_obsidian_mask]
    emissive_all = mre[:, :, 2][painted_obsidian_mask]
    encoded_linear = _decode_srgb(base[painted_obsidian_mask])
    return {
        "obsidian_final_pixels_including_gutter": int(obsidian_mask.sum()),
        "painted_obsidian_polygon_pixels": int(painted_obsidian_mask.sum()),
        "obsidian_chart_count": len(obsidian_indices),
        "authored_charcoal_linear_min": round(float(linear_all.min()), 6),
        "authored_charcoal_linear_max": round(float(linear_all.max()), 6),
        "encoded_charcoal_linear_min": round(float(encoded_linear.min()), 6),
        "encoded_charcoal_linear_max": round(float(encoded_linear.max()), 6),
        "painted_obsidian_roughness_min_u8": int(roughness_all.min()),
        "painted_obsidian_roughness_min_linear": round(float(roughness_all.min()) / 255.0, 6),
        "painted_obsidian_emissive_max_u8": int(emissive_all.max()),
    }


def _amber_bytes(res: baker.BakeResult, owner: np.ndarray, jobs: list[baker.ChartJob]) -> bytes:
    amber_ids = {i for i, job in enumerate(jobs) if job.family == "kharuun_amber"}
    mask = np.isin(owner, list(amber_ids))
    base = np.frombuffer(res.base, dtype=np.uint8).reshape(res.size, res.size, 3)
    normal = np.frombuffer(res.normal, dtype=np.uint8).reshape(res.size, res.size, 3)
    mre = np.frombuffer(res.mre, dtype=np.uint8).reshape(res.size, res.size, 3)
    return base[mask].tobytes() + normal[mask].tobytes() + mre[mask].tobytes()


def run(manifest_path: str, out_dir: str, size: int) -> dict:
    with open(manifest_path, "r", encoding="utf-8") as fh:
        manifest = json.load(fh)
    if size < 8 or size > 8192:
        raise ValueError("--size must be within 8..8192")
    blend_size = int(manifest.get("molt_blend_size", 0) or 0)
    if blend_size and (size < blend_size or size % blend_size):
        raise ValueError(
            f"--size must be an integer multiple of molt_blend_size ({blend_size}) "
            "so the shared baker can derive the blend mask"
        )
    started = time.perf_counter()
    # The shared baker remains the single source for amber, StateMask, debug
    # map, and all non-obsidian families.
    res = baker.bake(manifest, size=size, maps=baker.ALL_MAPS)
    jobs, density, gutter = baker.prepare_charts(manifest, size)
    owner = _ownership(jobs, size, gutter)
    preserved_before = {
        "state_mask": sha256(res.state),
        "amber_pixels": sha256(_amber_bytes(res, owner, jobs)),
    }
    measurements = _refine_obsidian(res, jobs, density, gutter)
    preserved_after = {
        "state_mask": sha256(res.state),
        "amber_pixels": sha256(_amber_bytes(res, owner, jobs)),
    }
    if preserved_before != preserved_after:
        raise AssertionError("refinement changed StateMask or final amber pixels")
    if (measurements["encoded_charcoal_linear_min"] < 0.02 or
            measurements["encoded_charcoal_linear_max"] > 0.07):
        raise AssertionError("encoded obsidian escaped charcoal linear range")
    if measurements["painted_obsidian_roughness_min_u8"] < 217:
        raise AssertionError("refined obsidian roughness floor was not met")
    if measurements["painted_obsidian_emissive_max_u8"] != 0:
        raise AssertionError("obsidian received an emissive mask")

    # Pillow validates that all three altered RGB buffers are real image-sized
    # matrices before the shared deterministic writer emits the deliverables.
    for buf in (res.base, res.normal, res.mre):
        image = Image.fromarray(np.frombuffer(buf, dtype=np.uint8).reshape(size, size, 3), "RGB")
        if image.size != (size, size):
            raise AssertionError("Pillow image-size validation failed")

    asset_id = str(manifest.get("production_asset_id", "EBS_ASSET"))
    report = baker.write_outputs(res, out_dir, asset_id, baker.ALL_MAPS, manifest_path,
                                 manifest, time.perf_counter() - started)
    refinement = {
        "author": AUTHOR,
        "creator": AUTHOR,
        "tool": Path(__file__).name,
        "revision": REVISION,
        "tool_sha256": baker.sha256_file(os.path.abspath(__file__)),
        "shared_baker": baker.TOOL_NAME,
        "shared_baker_revision": baker.TOOL_REVISION,
        "manifest": os.path.abspath(manifest_path),
        "manifest_sha256": baker.sha256_file(manifest_path),
        "production_asset_id": asset_id,
        "size": size,
        "method": "shared complete bake followed by final visible kharuun_obsidian replacement only",
        "removed_pattern": "cellular Voronoi crack grid",
        "added_pattern": "directional warped strata with sparse interrupted fissures",
        "preserved_before": preserved_before,
        "preserved_after": preserved_after,
        "preservation_verified": preserved_before == preserved_after,
        "measurements": measurements,
        "base_baker_report": os.path.join(out_dir, "bake-report.json"),
        "base_baker_written_map_checksums": {name: item["sha256"] for name, item in report["maps"].items()},
        "elapsed_seconds": round(time.perf_counter() - started, 3),
    }
    path = os.path.join(out_dir, "fidelity-texture-refinement-report.json")
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(refinement, fh, indent=2)
        fh.write("\n")
    return refinement


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Bake the Riftstalker pilot with refined Kharuun obsidian.")
    parser.add_argument("--manifest", required=True, help="Riftstalker fidelity bake-manifest.json")
    parser.add_argument("--out", required=True, help="Output directory")
    parser.add_argument("--size", type=int, default=2048, help="Atlas size (default: 2048)")
    args = parser.parse_args(argv)
    try:
        report = run(args.manifest, args.out, args.size)
    except (AssertionError, ValueError, OSError, json.JSONDecodeError) as exc:
        print(f"[EBS_RIFT_TEX_ERROR] {exc}", file=sys.stderr)
        return 2
    m = report["measurements"]
    print(
        f"[EBS_RIFT_TEX_READY] size={report['size']} obsidian_pixels={m['painted_obsidian_polygon_pixels']} "
        f"charcoal={m['encoded_charcoal_linear_min']}..{m['encoded_charcoal_linear_max']} "
        f"roughness_min={m['painted_obsidian_roughness_min_u8']}/255 "
        f"state_mask_preserved={report['preservation_verified']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
