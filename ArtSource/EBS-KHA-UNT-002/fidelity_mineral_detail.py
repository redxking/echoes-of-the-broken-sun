#!/usr/bin/env python3
"""Deterministic world-space mineral detail for the Riftstalker pilot.

This module deliberately carries no colour, emissive, roughness, UV, or atlas
logic.  It supplies two scalar fields that an obsidian recipe can use after it
has selected its charcoal base value:

``height_cm``
    Shallow positive laminations/chips and narrow negative fracture planes.
``value_mod``
    A small linear-light adjustment for the same physical detail.  The caller
    clamps the result to the asset's charcoal range (0.02..0.07).

The default is one common world-space field.  Adjacent facets therefore see
the same strata and fracture planes instead of restarting their detail on each
UV island.  ``component`` is optional and is intended only for isolated,
non-contiguous set dressing where a repeat offset is useful; leave it ``None``
for connected Riftstalker geometry.

The field is faceted mineral, not organic noise: it combines a small fixed set
of interrupted oblique fracture planes, irregular 3--6 cm lamination, and
small chipped planes.  It contains no colour and no cellular/Voronoi distance
field, so it cannot produce amber outlines or a regular cell grid.

Author: Angelis Pseftis
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import math
from typing import Any

import numpy as np

AUTHOR = "Angelis Pseftis"
REVISION = "ebs-riftstalker-mineral-detail-v2"


@dataclass(frozen=True)
class MineralDetailParameters:
    """Physical scales in centimetres and conservative modulation limits."""

    lamination_cm: float = 5.1
    fracture_count: int = 48
    fracture_depth_cm: float = 0.34
    lamination_height_cm: float = 0.045
    chip_height_cm: float = 0.055
    fracture_value: float = 0.018
    lamination_value: float = 0.0015
    chip_value: float = 0.0030


DEFAULT_PARAMETERS = MineralDetailParameters()


def _component_phase(component: str | None) -> float:
    """Stable optional phase; Python's randomized ``hash`` is never used."""
    if component is None:
        return 0.0
    digest = hashlib.sha256(component.encode("utf-8")).digest()
    return int.from_bytes(digest[:4], "big") / 2**32


def _smoothstep(edge0: float, edge1: float, value: np.ndarray) -> np.ndarray:
    if edge1 <= edge0:
        raise ValueError("smoothstep edges must be ordered")
    t = np.clip((value - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _noise(x: np.ndarray, y: np.ndarray, seed: float) -> np.ndarray:
    """Smooth lattice value noise, used only to break otherwise planar detail."""
    ix = np.floor(x)
    iy = np.floor(y)
    fx = x - ix
    fy = y - iy

    def h(a: np.ndarray, b: np.ndarray) -> np.ndarray:
        return np.mod(np.sin(a * 127.1 + b * 311.7 + seed * 19.19) * 43758.5453123, 1.0)

    sx = fx * fx * (3.0 - 2.0 * fx)
    sy = fy * fy * (3.0 - 2.0 * fy)
    a = h(ix, iy)
    b = h(ix + 1.0, iy)
    c = h(ix, iy + 1.0)
    d = h(ix + 1.0, iy + 1.0)
    return (a * (1.0 - sx) + b * sx) * (1.0 - sy) + (c * (1.0 - sx) + d * sx) * sy


def _scalar_hash(index: int, lane: int, phase: float) -> float:
    """Stable scalar pseudo-random value for a fixed fracture-plane set."""
    return math.modf(math.sin((index + 1) * 12.9898 + lane * 78.233 + phase * 37.719) * 43758.5453123)[0] % 1.0


def mineral_detail_layers(
    x_cm: Any,
    y_cm: Any,
    z_cm: Any,
    *,
    component: str | None = None,
    parameters: MineralDetailParameters = DEFAULT_PARAMETERS,
) -> dict[str, np.ndarray]:
    """Return world-space layers suitable for debugging or material baking.

    Inputs are broadcast through NumPy.  With ``component=None`` the result is
    a common field over all geometry, which is the production default.
    """
    x, y, z = np.broadcast_arrays(np.asarray(x_cm, dtype=np.float64),
                                  np.asarray(y_cm, dtype=np.float64),
                                  np.asarray(z_cm, dtype=np.float64))
    phase = _component_phase(component)

    # Jagged 3--6 cm lamination.  The major coordinate follows world up but
    # carries a small lateral shift so the bands look cleaved, not sinusoidal.
    lamination_period = parameters.lamination_cm
    lam_bend = (_noise(x / 29.0 + phase, y / 29.0 - phase, 11.0) - 0.5) * 2.4
    lam_coord = (z + lam_bend + x * 0.045) / lamination_period
    lam_fraction = np.mod(lam_coord, 1.0)
    lamination = np.clip(1.0 - np.abs(lam_fraction - 0.52) / 0.30, 0.0, 1.0)
    lamination *= 0.40 + 0.60 * _smoothstep(0.30, 0.72, _noise(x / 21.0, y / 21.0, 17.0))

    # Angular 3--6 cm chips.  A shallow triangular plane is only retained in
    # sparse patches, so it reads as fractured mineral rather than a grid.
    chip_axis = (x * 0.61 - y * 0.37 + z * 0.19) / 6.3
    chip_fraction = np.mod(chip_axis + phase * 0.17, 1.0)
    chip_face = np.clip(1.0 - np.abs(chip_fraction - 0.5) / 0.38, 0.0, 1.0)
    chip_gate = _smoothstep(0.57, 0.79, _noise(x / 24.0 + z / 92.0, y / 24.0, 23.0))
    chips = chip_face * chip_gate

    # Sparse interrupted oblique fracture planes.  Each plane has distinct
    # direction, slope, width, offset and segment gate.  This is a finite
    # fracture set, never a nearest-feature/cellular pattern.
    fracture = np.zeros_like(x)
    for i in range(parameters.fracture_count):
        angle = math.tau * _scalar_hash(i, 0, phase)
        nx = math.cos(angle)
        ny = math.sin(angle)
        nz = (_scalar_hash(i, 1, phase) - 0.5) * 0.36
        offset = (_scalar_hash(i, 2, phase) - 0.5) * 520.0
        width = 0.85 + _scalar_hash(i, 3, phase) * 1.35
        signed = nx * x + ny * y + nz * z - offset
        plane = np.clip(1.0 - np.abs(signed) / width, 0.0, 1.0)
        along = -ny * x + nx * y + z * nz
        segment_noise = _noise(along / (27.0 + _scalar_hash(i, 4, phase) * 28.0),
                              signed / 18.0 + i * 0.71, 31.0 + i)
        broken = _smoothstep(0.59, 0.78, segment_noise)
        fracture = np.maximum(fracture, plane * broken)

    height_cm = (lamination * parameters.lamination_height_cm +
                 chips * parameters.chip_height_cm -
                 fracture * parameters.fracture_depth_cm)
    value_mod = (lamination * parameters.lamination_value +
                 chips * parameters.chip_value -
                 fracture * parameters.fracture_value)
    return {
        "height_cm": height_cm,
        "value_mod": value_mod,
        "fracture": fracture,
        "lamination": lamination,
        "chips": chips,
    }


def mineral_detail(
    x_cm: Any,
    y_cm: Any,
    z_cm: Any,
    *,
    component: str | None = None,
    parameters: MineralDetailParameters = DEFAULT_PARAMETERS,
) -> tuple[np.ndarray, np.ndarray]:
    """Return ``(height_cm, value_mod)`` for the given world positions.

    Typical obsidian use::

        height_cm, value_mod = mineral_detail(world_x, world_y, world_z)
        charcoal_linear = np.clip(base_charcoal + value_mod, 0.02, 0.07)
        normal_height += height_cm

    Keep Amber completely separate.  The field itself never assigns colour,
    emissive, metallic or roughness values.
    """
    layers = mineral_detail_layers(x_cm, y_cm, z_cm, component=component, parameters=parameters)
    return layers["height_cm"], layers["value_mod"]


if __name__ == "__main__":
    # A tiny deterministic inspection, deliberately not an atlas bake/render.
    gx, gy = np.meshgrid(np.linspace(-100.0, 100.0, 33), np.linspace(-100.0, 100.0, 33))
    height, value = mineral_detail(gx, gy, np.full_like(gx, 145.0))
    print(
        f"[EBS_MINERAL_DETAIL] revision={REVISION} height_cm={height.min():.4f}..{height.max():.4f} "
        f"value_mod={value.min():.5f}..{value.max():.5f}"
    )
