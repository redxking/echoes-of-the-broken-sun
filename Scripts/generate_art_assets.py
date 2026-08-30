"""Generate the authored Echoes roster, world, and presentation mesh set.

Run this script only through Scripts/generate_art_assets.sh.  The generated
assets are ordinary StaticMesh and Material assets; Geometry Scripting and
Python are editor-time dependencies, not runtime authority.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Callable, Sequence

import unreal


ART_ROOT = "/Game/Art/Generated"
MATERIAL_PATH = f"{ART_ROOT}/Materials/M_EchoesSurface"
WORLD_MATERIAL_PATH = f"{ART_ROOT}/Materials/M_EchoesWorldSurface"
ASH_CUT_MATERIAL_PATH = f"{ART_ROOT}/Materials/M_GlassScarAshCut"
ASH_CUT_MATERIAL_INSTANCE_PATHS = (
    f"{ART_ROOT}/Materials/MI_GlassScarAshCut_Basalt",
    f"{ART_ROOT}/Materials/MI_GlassScarAshCut_Ash",
    f"{ART_ROOT}/Materials/MI_GlassScarAshCut_Glass",
    f"{ART_ROOT}/Materials/MI_GlassScarAshCut_Vein",
)
ASH_CUT_ASSET_REVISION = "ash-cut-production-v1"
BURIED_CAUSEWAY_MATERIAL_PATH = f"{ART_ROOT}/Materials/M_GlassScarBuriedCauseway"
BURIED_CAUSEWAY_MATERIAL_INSTANCE_PATHS = (
    f"{ART_ROOT}/Materials/MI_GlassScarBuriedCauseway_Stone",
    f"{ART_ROOT}/Materials/MI_GlassScarBuriedCauseway_Recess",
    f"{ART_ROOT}/Materials/MI_GlassScarBuriedCauseway_Ceramic",
    f"{ART_ROOT}/Materials/MI_GlassScarBuriedCauseway_Conduit",
)
BURIED_CAUSEWAY_ASSET_REVISION = "buried-causeway-production-v1"
FOLDED_VERGE_MATERIAL_PATH = f"{ART_ROOT}/Materials/M_GlassScarFoldedVerge"
FOLDED_VERGE_MATERIAL_INSTANCE_PATHS = (
    f"{ART_ROOT}/Materials/MI_GlassScarFoldedVerge_Obsidian",
    f"{ART_ROOT}/Materials/MI_GlassScarFoldedVerge_Rift",
    f"{ART_ROOT}/Materials/MI_GlassScarFoldedVerge_Ceramic",
    f"{ART_ROOT}/Materials/MI_GlassScarFoldedVerge_Phase",
)
FOLDED_VERGE_ASSET_REVISION = "folded-verge-production-v1"
VFX_ROOT = f"{ART_ROOT}/VFX"
VFX_MATERIAL_PATH = f"{ART_ROOT}/Materials/M_EchoesPresentationVFX"
PRESENTATION_VFX_ASSET_REVISION = "selection-command-vfx-v1"
DESTRUCTION_VFX_ASSET_REVISION = "destruction-vfx-v1"

PRIMARY = 0
DARK = 1
LIGHT = 2
GLOW = 3


@dataclass(frozen=True)
class AssetSpec:
    faction: str
    category: str
    name: str
    display_name: str
    role: str
    builder: Callable[[unreal.DynamicMesh, bool], None]

    @property
    def asset_name(self) -> str:
        return f"SM_{self.faction}_{self.name}"

    @property
    def asset_path(self) -> str:
        return f"{ART_ROOT}/{self.faction}/{self.category}/{self.asset_name}"


@dataclass(frozen=True)
class VfxAssetSpec:
    name: str
    display_name: str
    role: str
    builder: Callable[[unreal.DynamicMesh, bool], None]
    revision: str = PRESENTATION_VFX_ASSET_REVISION

    @property
    def asset_name(self) -> str:
        return f"SM_VFX_{self.name}"

    @property
    def asset_path(self) -> str:
        return f"{VFX_ROOT}/{self.asset_name}"


def transform(
    location: tuple[float, float, float] = (0.0, 0.0, 0.0),
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
    scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
) -> unreal.Transform:
    return unreal.Transform(
        location=unreal.Vector(*location),
        rotation=unreal.Rotator(*rotation),
        scale=unreal.Vector(*scale),
    )


def primitive_options(material_id: int) -> unreal.GeometryScriptPrimitiveOptions:
    return unreal.GeometryScriptPrimitiveOptions(material_id=material_id)


def box(
    mesh: unreal.DynamicMesh,
    size: tuple[float, float, float],
    at: tuple[float, float, float],
    material_id: int = PRIMARY,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> None:
    mesh.append_box(
        primitive_options(material_id),
        transform(at, rotation),
        size[0],
        size[1],
        size[2],
        0,
        0,
        0,
        unreal.GeometryScriptPrimitiveOriginMode.CENTER,
    )


def cylinder(
    mesh: unreal.DynamicMesh,
    radius: float,
    height: float,
    at: tuple[float, float, float],
    material_id: int = PRIMARY,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
    sides: int = 12,
    scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
) -> None:
    mesh.append_cylinder(
        primitive_options(material_id),
        transform(at, rotation, scale),
        radius,
        height,
        sides,
        0,
        True,
        unreal.GeometryScriptPrimitiveOriginMode.CENTER,
    )


def cone(
    mesh: unreal.DynamicMesh,
    base_radius: float,
    top_radius: float,
    height: float,
    at: tuple[float, float, float],
    material_id: int = PRIMARY,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
    sides: int = 8,
) -> None:
    mesh.append_cone(
        primitive_options(material_id),
        transform(at, rotation),
        base_radius,
        top_radius,
        height,
        sides,
        1,
        True,
        unreal.GeometryScriptPrimitiveOriginMode.CENTER,
    )


def sphere(
    mesh: unreal.DynamicMesh,
    radius: float,
    at: tuple[float, float, float],
    material_id: int = PRIMARY,
    scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
    high_detail: bool = True,
) -> None:
    mesh.append_sphere_lat_long(
        primitive_options(material_id),
        transform(at, scale=scale),
        radius,
        8 if high_detail else 5,
        12 if high_detail else 8,
        unreal.GeometryScriptPrimitiveOriginMode.CENTER,
    )


def torus(
    mesh: unreal.DynamicMesh,
    major_radius: float,
    minor_radius: float,
    at: tuple[float, float, float],
    material_id: int = PRIMARY,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
    high_detail: bool = True,
) -> None:
    revolve = unreal.GeometryScriptRevolveOptions()
    mesh.append_torus(
        primitive_options(material_id),
        transform(at, rotation),
        revolve,
        major_radius,
        minor_radius,
        18 if high_detail else 10,
        6 if high_detail else 4,
        unreal.GeometryScriptPrimitiveOriginMode.CENTER,
    )


def radial_box(
    mesh: unreal.DynamicMesh,
    angle_degrees: float,
    radius: float,
    size: tuple[float, float, float],
    z: float,
    material_id: int,
    pitch: float = 0.0,
) -> None:
    angle = math.radians(angle_degrees)
    box(
        mesh,
        size,
        (math.cos(angle) * radius, math.sin(angle) * radius, z),
        material_id,
        (pitch, angle_degrees, 0.0),
    )


def radial_tangent_box(
    mesh: unreal.DynamicMesh,
    angle_degrees: float,
    radius: float,
    size: tuple[float, float, float],
    z: float,
    material_id: int,
    pitch: float = 0.0,
) -> None:
    angle = math.radians(angle_degrees)
    box(
        mesh,
        size,
        (math.cos(angle) * radius, math.sin(angle) * radius, z),
        material_id,
        (pitch, angle_degrees + 90.0, 0.0),
    )


def paired_leg(
    mesh: unreal.DynamicMesh,
    y: float,
    hip_z: float,
    foot_x: float,
    heavy: bool = False,
) -> None:
    width = 15.0 if heavy else 11.0
    box(mesh, (42.0, width, width), (-5.0, y, hip_z - 15.0), DARK, (55.0, 0.0, 0.0))
    box(mesh, (46.0, width, width), (foot_x - 8.0, y, 27.0), LIGHT, (-57.0, 0.0, 0.0))
    sphere(mesh, width * 0.72, (2.0, y, hip_z), GLOW, high_detail=False)
    box(mesh, (30.0, width + 8.0, 10.0), (foot_x, y, 7.0), DARK)


def meridian_surveyor(mesh: unreal.DynamicMesh, high: bool) -> None:
    box(mesh, (58.0, 48.0, 36.0), (0.0, 0.0, 78.0), LIGHT)
    box(mesh, (48.0, 38.0, 24.0), (-7.0, 0.0, 104.0), DARK, (-8.0, 0.0, 0.0))
    box(mesh, (8.0, 34.0, 15.0), (27.0, 0.0, 95.0), GLOW)
    paired_leg(mesh, -21.0, 69.0, 18.0)
    paired_leg(mesh, 21.0, 69.0, 18.0)
    for side in (-1.0, 1.0):
        box(mesh, (43.0, 10.0, 10.0), (15.0, side * 39.0, 79.0), DARK, (0.0, side * 8.0, side * 15.0))
        box(mesh, (38.0, 8.0, 8.0), (46.0, side * 48.0, 62.0), LIGHT, (0.0, side * 8.0, side * 30.0))
        cone(mesh, 8.0, 2.0, 22.0, (65.0, side * 52.0, 54.0), GLOW, (90.0, 0.0, 0.0), 6)
        cylinder(mesh, 10.0, 31.0, (-27.0, side * 18.0, 91.0), DARK, sides=8)
        cylinder(mesh, 6.0, 25.0, (-27.0, side * 18.0, 92.0), GLOW, sides=8)
    box(mesh, (8.0, 8.0, 55.0), (-18.0, 0.0, 137.0), DARK)
    box(mesh, (15.0, 8.0, 28.0), (-18.0, 0.0, 168.0), GLOW)
    if high:
        box(mesh, (30.0, 5.0, 5.0), (-3.0, 0.0, 153.0), LIGHT)
        sphere(mesh, 6.0, (12.0, 0.0, 153.0), GLOW, high_detail=False)


def meridian_lancer(mesh: unreal.DynamicMesh, high: bool) -> None:
    paired_leg(mesh, -15.0, 72.0, 10.0)
    paired_leg(mesh, 15.0, 72.0, 10.0)
    box(mesh, (42.0, 38.0, 52.0), (-2.0, 0.0, 98.0), LIGHT)
    box(mesh, (34.0, 30.0, 20.0), (2.0, 0.0, 132.0), DARK)
    box(mesh, (10.0, 23.0, 11.0), (20.0, 0.0, 133.0), GLOW)
    for side in (-1.0, 1.0):
        sphere(mesh, 15.0, (-1.0, side * 26.0, 112.0), LIGHT, scale=(1.0, 0.75, 0.8), high_detail=False)
        box(mesh, (35.0, 9.0, 9.0), (15.0, side * 31.0, 94.0), DARK, (0.0, 0.0, side * 35.0))
    box(mesh, (106.0, 12.0, 13.0), (51.0, -6.0, 90.0), DARK, (0.0, 0.0, -7.0))
    box(mesh, (83.0, 7.0, 7.0), (64.0, -6.0, 93.0), GLOW, (0.0, 0.0, -7.0))
    cone(mesh, 9.0, 3.0, 36.0, (116.0, -6.0, 82.0), LIGHT, (0.0, 90.0, 0.0), 6)
    if high:
        box(mesh, (7.0, 4.0, 35.0), (-18.0, -18.0, 125.0), GLOW)
        box(mesh, (26.0, 6.0, 5.0), (18.0, 10.0, 112.0), LIGHT)


def meridian_bulwark(mesh: unreal.DynamicMesh, high: bool) -> None:
    box(mesh, (74.0, 92.0, 28.0), (-12.0, 0.0, 58.0), DARK)
    for side in (-1.0, 1.0):
        paired_leg(mesh, side * 31.0, 55.0, 4.0, heavy=True)
        box(mesh, (38.0, 32.0, 45.0), (-5.0, side * 24.0, 88.0), LIGHT)
        box(mesh, (13.0, 19.0, 10.0), (17.0, side * 24.0, 104.0), GLOW)
    for index, y in enumerate((-58.0, -29.0, 0.0, 29.0, 58.0)):
        z = 65.0 + (8.0 if index in (1, 3) else 0.0)
        box(mesh, (12.0, 27.0, 57.0), (55.0, y, z), LIGHT, (0.0, 0.0, -8.0))
        box(mesh, (14.0, 21.0, 45.0), (62.0, y, z), GLOW, (0.0, 0.0, -8.0))
    cylinder(mesh, 18.0, 75.0, (24.0, 0.0, 71.0), DARK, (90.0, 0.0, 0.0), 10)
    if high:
        torus(mesh, 24.0, 3.0, (18.0, 0.0, 79.0), GLOW, (90.0, 0.0, 0.0))
        for side in (-1.0, 1.0):
            box(mesh, (38.0, 6.0, 6.0), (33.0, side * 47.0, 43.0), GLOW)


def meridian_relay_skiff(mesh: unreal.DynamicMesh, high: bool) -> None:
    box(mesh, (88.0, 34.0, 22.0), (0.0, 0.0, 54.0), DARK, (-5.0, 0.0, 0.0))
    cone(mesh, 28.0, 4.0, 92.0, (27.0, 0.0, 55.0), LIGHT, (0.0, 90.0, 0.0), 6)
    sphere(mesh, 24.0, (-5.0, 0.0, 67.0), GLOW, scale=(1.35, 0.72, 0.62), high_detail=high)
    for side in (-1.0, 1.0):
        box(mesh, (82.0, 42.0, 8.0), (-5.0, side * 42.0, 54.0), LIGHT, (0.0, side * 18.0, side * 4.0))
        box(mesh, (56.0, 5.0, 5.0), (6.0, side * 48.0, 58.0), GLOW, (0.0, side * 18.0, side * 4.0))
        cylinder(mesh, 8.0, 18.0, (-29.0, side * 28.0, 42.0), DARK, sides=8)
        cylinder(mesh, 5.0, 5.0, (-29.0, side * 28.0, 31.0), GLOW, sides=8)
    torus(mesh, 39.0, 2.5, (-18.0, 0.0, 92.0), GLOW, high_detail=high)
    box(mesh, (5.0, 5.0, 38.0), (-18.0, 0.0, 90.0), DARK)
    if high:
        cylinder(mesh, 9.0, 27.0, (-46.0, 0.0, 55.0), LIGHT, (90.0, 0.0, 0.0), 8)


def meridian_anchor(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 154.0, 28.0, (0.0, 0.0, 14.0), DARK, sides=6)
    cylinder(mesh, 128.0, 42.0, (0.0, 0.0, 42.0), LIGHT, sides=12)
    cylinder(mesh, 78.0, 64.0, (0.0, 0.0, 82.0), DARK, sides=8)
    for angle in range(0, 360, 60):
        radial_box(mesh, angle, 144.0, (105.0, 48.0, 38.0), 42.0, LIGHT)
        radial_box(mesh, angle, 148.0, (77.0, 7.0, 8.0), 70.0, GLOW)
        if high:
            radial_box(mesh, angle, 198.0, (34.0, 34.0, 18.0), 14.0, DARK)
    cylinder(mesh, 40.0, 112.0, (0.0, 0.0, 132.0), LIGHT, sides=8)
    torus(mesh, 51.0, 6.0, (0.0, 0.0, 116.0), GLOW, high_detail=high)
    cone(mesh, 33.0, 3.0, 90.0, (0.0, 0.0, 227.0), GLOW, sides=8)
    if high:
        for angle in range(30, 360, 60):
            radial_box(mesh, angle, 94.0, (42.0, 12.0, 38.0), 91.0, DARK, -25.0)


def meridian_power_link(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 74.0, 24.0, (0.0, 0.0, 12.0), DARK, sides=8)
    cylinder(mesh, 50.0, 130.0, (0.0, 0.0, 80.0), LIGHT, sides=8)
    cylinder(mesh, 29.0, 154.0, (0.0, 0.0, 147.0), DARK, sides=10)
    for z, radius in ((55.0, 61.0), (118.0, 52.0), (184.0, 43.0)):
        torus(mesh, radius, 5.0, (0.0, 0.0, z), GLOW, high_detail=high)
    for angle in range(45, 360, 90):
        radial_box(mesh, angle, 99.0, (78.0, 12.0, 10.0), 22.0, DARK)
        radial_box(mesh, angle, 139.0, (25.0, 25.0, 22.0), 11.0, LIGHT)
    cone(mesh, 29.0, 2.0, 62.0, (0.0, 0.0, 255.0), GLOW, sides=8)
    if high:
        for angle in range(0, 360, 90):
            radial_box(mesh, angle, 43.0, (7.0, 7.0, 92.0), 146.0, LIGHT)


def meridian_array_foundry(mesh: unreal.DynamicMesh, high: bool) -> None:
    box(mesh, (330.0, 230.0, 28.0), (0.0, 0.0, 14.0), DARK)
    box(mesh, (275.0, 150.0, 68.0), (-12.0, 0.0, 62.0), LIGHT)
    box(mesh, (240.0, 72.0, 58.0), (10.0, 0.0, 119.0), DARK)
    box(mesh, (246.0, 10.0, 12.0), (10.0, 0.0, 151.0), GLOW)
    for x in (-108.0, -38.0, 38.0, 108.0):
        box(mesh, (18.0, 205.0, 100.0), (x, 0.0, 94.0), DARK)
        box(mesh, (8.0, 175.0, 8.0), (x, 0.0, 148.0), GLOW)
    box(mesh, (48.0, 106.0, 10.0), (158.0, 0.0, 18.0), LIGHT, (0.0, 0.0, -10.0))
    if high:
        for side in (-1.0, 1.0):
            for x in (-108.0, -36.0, 36.0, 108.0):
                cylinder(mesh, 9.0, 32.0, (x, side * 92.0, 54.0), GLOW, sides=8)
        torus(mesh, 42.0, 4.0, (-120.0, 0.0, 116.0), GLOW, (90.0, 0.0, 0.0))


def meridian_aegis_post(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 78.0, 28.0, (0.0, 0.0, 14.0), DARK, sides=8)
    for angle in range(45, 360, 90):
        radial_box(mesh, angle, 84.0, (68.0, 28.0, 26.0), 22.0, LIGHT, -8.0)
    cylinder(mesh, 50.0, 78.0, (0.0, 0.0, 63.0), LIGHT, sides=10)
    torus(mesh, 54.0, 6.0, (0.0, 0.0, 92.0), GLOW, high_detail=high)
    cylinder(mesh, 39.0, 64.0, (0.0, 0.0, 124.0), DARK, sides=10)
    box(mesh, (145.0, 35.0, 38.0), (46.0, 0.0, 153.0), LIGHT, (0.0, 0.0, 4.0))
    box(mesh, (122.0, 12.0, 12.0), (75.0, 0.0, 158.0), GLOW, (0.0, 0.0, 4.0))
    cone(mesh, 17.0, 5.0, 58.0, (146.0, 0.0, 164.0), GLOW, (0.0, 90.0, 0.0), 6)
    if high:
        torus(mesh, 26.0, 3.0, (11.0, 0.0, 154.0), GLOW, (90.0, 0.0, 0.0))
        box(mesh, (8.0, 8.0, 80.0), (-43.0, 0.0, 142.0), LIGHT)


def kharuun_tender(mesh: unreal.DynamicMesh, high: bool) -> None:
    sphere(mesh, 37.0, (0.0, 0.0, 78.0), DARK, scale=(1.25, 0.95, 1.1), high_detail=high)
    cone(mesh, 29.0, 8.0, 54.0, (9.0, 0.0, 108.0), LIGHT, (-18.0, 0.0, 0.0), 7)
    box(mesh, (8.0, 30.0, 8.0), (18.0, 0.0, 101.0), GLOW, (0.0, 0.0, -18.0))
    for angle in (45.0, 135.0, 225.0, 315.0):
        a = math.radians(angle)
        cone(mesh, 13.0, 6.0, 62.0, (math.cos(a) * 34.0, math.sin(a) * 34.0, 39.0), LIGHT, (55.0, angle, 0.0), 6)
        sphere(mesh, 10.0, (math.cos(a) * 55.0, math.sin(a) * 55.0, 13.0), DARK, scale=(1.4, 0.8, 0.5), high_detail=False)
    for side in (-1.0, 1.0):
        cylinder(mesh, 5.0, 72.0, (31.0, side * 24.0, 61.0), DARK, (64.0, 0.0, 0.0), 6)
        cone(mesh, 9.0, 2.0, 30.0, (62.0, side * 24.0, 31.0), GLOW, (78.0, 0.0, 0.0), 6)
    if high:
        for y in (-19.0, 0.0, 19.0):
            sphere(mesh, 10.0, (-35.0, y, 86.0), GLOW, scale=(0.7, 0.7, 1.25), high_detail=False)


def kharuun_riftstalker(mesh: unreal.DynamicMesh, high: bool) -> None:
    sphere(mesh, 34.0, (0.0, 0.0, 72.0), DARK, scale=(1.85, 0.7, 0.72), high_detail=high)
    cone(mesh, 29.0, 8.0, 75.0, (34.0, 0.0, 75.0), LIGHT, (0.0, 90.0, 0.0), 7)
    for x in (-27.0, 29.0):
        for side in (-1.0, 1.0):
            box(mesh, (55.0, 10.0, 12.0), (x, side * 27.0, 45.0), DARK, (58.0, side * 14.0, side * 12.0))
            cone(mesh, 12.0, 4.0, 49.0, (x + 17.0, side * 43.0, 19.0), LIGHT, (65.0, side * 15.0, 0.0), 6)
    box(mesh, (118.0, 13.0, 15.0), (11.0, 0.0, 112.0), LIGHT, (0.0, 0.0, 3.0))
    box(mesh, (104.0, 6.0, 6.0), (20.0, 0.0, 115.0), GLOW, (0.0, 0.0, 3.0))
    cone(mesh, 16.0, 2.0, 48.0, (91.0, 0.0, 119.0), GLOW, (0.0, 90.0, 0.0), 6)
    if high:
        for x in (-28.0, 0.0, 28.0):
            cone(mesh, 11.0, 1.0, 29.0, (x, 0.0, 104.0), GLOW, (0.0, 0.0, 0.0), 5)


def kharuun_cairnback(mesh: unreal.DynamicMesh, high: bool) -> None:
    sphere(mesh, 58.0, (0.0, 0.0, 66.0), DARK, scale=(1.45, 1.05, 0.72), high_detail=high)
    for x in (-46.0, 0.0, 46.0):
        for side in (-1.0, 1.0):
            box(mesh, (48.0, 15.0, 18.0), (x, side * 51.0, 39.0), DARK, (55.0, side * 10.0, side * 18.0))
            cone(mesh, 16.0, 5.0, 44.0, (x + 8.0, side * 65.0, 17.0), LIGHT, (65.0, side * 10.0, 0.0), 6)
    for x, z, scale_x in ((-48.0, 91.0, 1.0), (-14.0, 105.0, 1.2), (24.0, 102.0, 1.1), (57.0, 88.0, 0.9)):
        sphere(mesh, 34.0, (x, 0.0, z), LIGHT, scale=(scale_x, 1.55, 0.38), high_detail=False)
    box(mesh, (120.0, 7.0, 7.0), (5.0, 0.0, 99.0), GLOW, (0.0, 0.0, 2.0))
    cone(mesh, 31.0, 8.0, 52.0, (77.0, 0.0, 67.0), PRIMARY, (0.0, 90.0, 0.0), 7)
    if high:
        for x in (-48.0, -14.0, 24.0, 57.0):
            cone(mesh, 9.0, 1.0, 24.0, (x, 0.0, 123.0), GLOW, sides=5)


def kharuun_resonant(mesh: unreal.DynamicMesh, high: bool) -> None:
    cone(mesh, 27.0, 5.0, 92.0, (0.0, 0.0, 103.0), LIGHT, sides=7)
    sphere(mesh, 20.0, (0.0, 0.0, 116.0), GLOW, scale=(0.8, 0.8, 1.2), high_detail=high)
    for angle in (30.0, 150.0, 270.0):
        a = math.radians(angle)
        box(mesh, (82.0, 9.0, 11.0), (math.cos(a) * 31.0, math.sin(a) * 31.0, 61.0), DARK, (63.0, angle, 0.0))
        cone(mesh, 10.0, 2.0, 55.0, (math.cos(a) * 60.0, math.sin(a) * 60.0, 26.0), LIGHT, (75.0, angle, 0.0), 6)
        cylinder(mesh, 2.5, 72.0, (math.cos(a) * 62.0, math.sin(a) * 62.0, 19.0), GLOW, (78.0, angle, 0.0), 5)
    for side in (-1.0, 1.0):
        box(mesh, (11.0, 9.0, 118.0), (-5.0, side * 30.0, 131.0), LIGHT, (side * 17.0, 0.0, side * 8.0))
        box(mesh, (5.0, 5.0, 93.0), (-2.0, side * 25.0, 133.0), GLOW, (side * 17.0, 0.0, side * 8.0))
    if high:
        cone(mesh, 11.0, 1.0, 45.0, (0.0, 0.0, 174.0), GLOW, sides=6)


def kharuun_memory_hearth(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 155.0, 30.0, (0.0, 0.0, 15.0), DARK, sides=12)
    torus(mesh, 121.0, 16.0, (0.0, 0.0, 42.0), LIGHT, high_detail=high)
    cylinder(mesh, 82.0, 48.0, (0.0, 0.0, 54.0), PRIMARY, sides=10)
    for angle in range(0, 360, 45):
        radial_box(mesh, angle, 117.0, (86.0, 31.0, 24.0), 52.0, LIGHT, -15.0)
        radial_box(mesh, angle, 96.0, (51.0, 7.0, 8.0), 74.0, GLOW, 22.0)
        cone(mesh, 21.0, 4.0, 85.0, (math.cos(math.radians(angle)) * 93.0, math.sin(math.radians(angle)) * 93.0, 107.0), DARK, (-12.0, angle, 0.0), 7)
    sphere(mesh, 43.0, (0.0, 0.0, 96.0), GLOW, scale=(1.0, 1.0, 1.18), high_detail=high)
    torus(mesh, 52.0, 5.0, (0.0, 0.0, 94.0), LIGHT, high_detail=high)
    if high:
        cone(mesh, 28.0, 4.0, 72.0, (0.0, 0.0, 161.0), LIGHT, sides=7)


def kharuun_waystone(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 68.0, 24.0, (0.0, 0.0, 12.0), DARK, sides=7)
    for angle in (30.0, 150.0, 270.0):
        radial_box(mesh, angle, 74.0, (92.0, 25.0, 24.0), 28.0, DARK, -24.0)
        a = math.radians(angle)
        cone(mesh, 18.0, 5.0, 87.0, (math.cos(a) * 69.0, math.sin(a) * 69.0, 61.0), LIGHT, (42.0, angle, 0.0), 7)
    cone(mesh, 56.0, 14.0, 188.0, (0.0, 0.0, 116.0), LIGHT, sides=7)
    cone(mesh, 35.0, 5.0, 162.0, (0.0, 0.0, 124.0), DARK, sides=7)
    box(mesh, (11.0, 11.0, 149.0), (17.0, 0.0, 128.0), GLOW, (0.0, 0.0, -5.0))
    sphere(mesh, 23.0, (0.0, 0.0, 112.0), GLOW, scale=(0.9, 0.9, 1.5), high_detail=high)
    if high:
        for angle in (0.0, 120.0, 240.0):
            a = math.radians(angle)
            cone(mesh, 13.0, 2.0, 68.0, (math.cos(a) * 41.0, math.sin(a) * 41.0, 145.0), PRIMARY, (-12.0, angle, 0.0), 6)


def kharuun_growth_basin(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 148.0, 26.0, (0.0, 0.0, 13.0), DARK, sides=12)
    torus(mesh, 111.0, 24.0, (0.0, 0.0, 42.0), LIGHT, high_detail=high)
    cylinder(mesh, 70.0, 22.0, (0.0, 0.0, 28.0), GLOW, sides=12)
    for angle in range(0, 360, 45):
        a = math.radians(angle)
        sphere(mesh, 25.0, (math.cos(a) * 99.0, math.sin(a) * 99.0, 76.0), DARK, scale=(0.72, 0.72, 1.22), high_detail=high)
        cone(mesh, 21.0, 4.0, 66.0, (math.cos(a) * 121.0, math.sin(a) * 121.0, 83.0), LIGHT, (-18.0, angle, 0.0), 7)
        if high:
            sphere(mesh, 12.0, (math.cos(a) * 97.0, math.sin(a) * 97.0, 79.0), GLOW, scale=(0.7, 0.7, 1.15), high_detail=False)
    torus(mesh, 69.0, 5.0, (0.0, 0.0, 56.0), GLOW, high_detail=high)


def kharuun_listening_spine(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 72.0, 24.0, (0.0, 0.0, 12.0), DARK, sides=9)
    for angle in range(0, 360, 60):
        radial_box(mesh, angle, 76.0, (79.0, 21.0, 18.0), 20.0, LIGHT, -15.0)
    cone(mesh, 48.0, 10.0, 232.0, (0.0, 0.0, 128.0), DARK, sides=7)
    cone(mesh, 26.0, 3.0, 198.0, (0.0, 0.0, 144.0), LIGHT, sides=7)
    box(mesh, (8.0, 8.0, 205.0), (11.0, 0.0, 138.0), GLOW, (0.0, 0.0, -3.0))
    for index, z in enumerate((88.0, 126.0, 164.0, 202.0)):
        length = 90.0 - index * 10.0
        side = -1.0 if index % 2 else 1.0
        box(mesh, (length, 12.0, 9.0), (side * length * 0.38, 0.0, z), LIGHT, (0.0, 0.0, side * 24.0))
        box(mesh, (length * 0.72, 5.0, 4.0), (side * length * 0.38, 0.0, z + 5.0), GLOW, (0.0, 0.0, side * 24.0))
    if high:
        torus(mesh, 46.0, 4.0, (0.0, 0.0, 70.0), GLOW)
        cone(mesh, 17.0, 1.0, 78.0, (0.0, 0.0, 278.0), LIGHT, sides=6)


def world_future_well_base(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Neutral foundation: neither Meridian-built nor Kharuun-grown."""
    cylinder(mesh, 238.0, 24.0, (0.0, 0.0, 12.0), DARK, sides=16)
    cylinder(mesh, 207.0, 14.0, (0.0, 0.0, 30.0), PRIMARY, sides=16)
    cylinder(mesh, 76.0, 28.0, (0.0, 0.0, 39.0), DARK, sides=12)
    torus(mesh, 78.0, 7.0, (0.0, 0.0, 54.0), GLOW, high_detail=high)
    torus(mesh, 148.0, 5.0, (0.0, 0.0, 38.0), LIGHT, high_detail=high)
    for angle in range(0, 360, 60):
        radial_box(mesh, angle, 151.0, (134.0, 18.0, 10.0), 39.0, LIGHT)
        radial_box(mesh, angle, 178.0, (111.0, 6.0, 5.0), 47.0, GLOW)
        radial_box(mesh, angle, 242.0, (58.0, 52.0, 18.0), 18.0, DARK)
        a = math.radians(angle)
        pylon_x = math.cos(a) * 188.0
        pylon_y = math.sin(a) * 188.0
        cone(
            mesh,
            38.0,
            18.0,
            172.0,
            (pylon_x, pylon_y, 122.0),
            PRIMARY,
            (-10.0, angle + 180.0, 0.0),
            6,
        )
        cone(
            mesh,
            20.0,
            8.0,
            132.0,
            (pylon_x - math.cos(a) * 10.0,
             pylon_y - math.sin(a) * 10.0,
             132.0),
            LIGHT,
            (-10.0, angle + 180.0, 0.0),
            6,
        )
        radial_box(mesh, angle, 178.0, (8.0, 13.0, 126.0), 126.0, GLOW, -10.0)
        if high:
            radial_tangent_box(mesh, angle + 15.0, 218.0, (54.0, 18.0, 15.0), 45.0, PRIMARY)
            radial_tangent_box(mesh, angle - 15.0, 218.0, (54.0, 12.0, 8.0), 52.0, LIGHT)


def world_future_well_orbit(mesh: unreal.DynamicMesh, high: bool) -> None:
    segment_step = 24 if high else 36
    skipped = {72, 96, 252, 276} if high else {72, 252}
    for angle in range(0, 360, segment_step):
        if angle in skipped:
            continue
        segment_length = 65.0 if high else 90.0
        radial_tangent_box(
            mesh,
            float(angle),
            158.0,
            (segment_length, 22.0, 16.0),
            0.0,
            LIGHT,
        )
        radial_tangent_box(
            mesh,
            float(angle),
            158.0,
            (segment_length * 0.78, 6.0, 5.0),
            10.0,
            GLOW,
        )
        if high and angle % 48 == 0:
            radial_box(mesh, float(angle), 158.0, (28.0, 9.0, 34.0), 2.0, DARK)


def world_future_well_core(mesh: unreal.DynamicMesh, high: bool) -> None:
    sides = 8 if high else 6
    cone(mesh, 3.0, 34.0, 92.0, (0.0, 0.0, -46.0), GLOW, sides=sides)
    cone(mesh, 34.0, 3.0, 92.0, (0.0, 0.0, 46.0), GLOW, sides=sides)
    torus(mesh, 39.0, 4.0, (0.0, 0.0, 0.0), LIGHT, (90.0, 0.0, 0.0), high)
    shard_angles = range(0, 360, 45 if high else 90)
    for index, angle in enumerate(shard_angles):
        a = math.radians(angle)
        radius = 50.0 + (index % 2) * 17.0
        z = -30.0 + (index % 3) * 31.0
        cone(
            mesh,
            9.0,
            2.0,
            42.0,
            (math.cos(a) * radius, math.sin(a) * radius, z),
            DARK,
            (12.0, angle, 18.0),
            5,
        )


def world_future_well_glyph(mesh: unreal.DynamicMesh, high: bool) -> None:
    for angle in range(0, 360, 45):
        radial_box(mesh, float(angle), 116.0, (185.0, 13.0, 7.0), 0.0, GLOW)
        radial_box(mesh, float(angle), 205.0, (72.0, 5.0, 4.0), 5.0, LIGHT)
        if high:
            radial_tangent_box(mesh, float(angle), 205.0, (38.0, 12.0, 6.0), 3.0, DARK)
    torus(mesh, 91.0, 5.0, (0.0, 0.0, 2.0), GLOW, high_detail=high)


def world_glass_scar_shelf(mesh: unreal.DynamicMesh, high: bool) -> None:
    """A broad, broken plate used to compose the two sides of the impact basin."""
    box(mesh, (780.0, 780.0, 54.0), (0.0, 0.0, -27.0), PRIMARY)
    box(mesh, (720.0, 700.0, 18.0), (6.0, -8.0, 9.0), DARK, (0.0, 2.5, 0.0))
    for x, y, yaw, sx, sy in (
        (-255.0, -222.0, 8.0, 250.0, 205.0),
        (208.0, -232.0, -7.0, 292.0, 190.0),
        (-230.0, 214.0, -5.0, 285.0, 218.0),
        (224.0, 220.0, 6.0, 266.0, 205.0),
    ):
        box(mesh, (sx, sy, 12.0), (x, y, 22.0), LIGHT, (0.0, yaw, 0.0))
    for x, y, yaw, length in (
        (-170.0, -40.0, 24.0, 270.0),
        (95.0, 120.0, -31.0, 230.0),
        (205.0, -92.0, 14.0, 180.0),
    ):
        box(mesh, (length, 12.0, 6.0), (x, y, 30.0), GLOW, (0.0, yaw, 0.0))
    if high:
        for angle in range(0, 360, 45):
            a = math.radians(angle)
            cone(
                mesh,
                22.0,
                5.0,
                58.0,
                (math.cos(a) * 350.0, math.sin(a) * 350.0, 12.0),
                DARK,
                (12.0, float(angle), 5.0),
                6,
            )


def world_glass_scar_ridge(mesh: unreal.DynamicMesh, high: bool) -> None:
    """One tile-sized cliff tooth; instances preserve the authoritative grid."""
    box(mesh, (188.0, 188.0, 42.0), (0.0, 0.0, 12.0), PRIMARY, (0.0, 4.0, 0.0))
    box(mesh, (176.0, 152.0, 22.0), (-3.0, 5.0, 40.0), DARK, (0.0, -6.0, 0.0))
    for x, y, radius, height, pitch, yaw in (
        (-48.0, -26.0, 45.0, 154.0, -8.0, -18.0),
        (18.0, 22.0, 52.0, 196.0, 6.0, 11.0),
        (61.0, -18.0, 32.0, 126.0, -11.0, 24.0),
    ):
        cone(mesh, radius, 7.0, height, (x, y, 48.0 + height * 0.5), DARK, (pitch, yaw, 0.0), 6)
    box(mesh, (154.0, 8.0, 7.0), (0.0, -47.0, 57.0), GLOW, (0.0, 9.0, 0.0))
    if high:
        cone(mesh, 18.0, 3.0, 78.0, (-72.0, 48.0, 72.0), LIGHT, (13.0, -14.0, 0.0), 5)
        cone(mesh, 15.0, 2.0, 66.0, (72.0, 43.0, 62.0), LIGHT, (-9.0, 19.0, 0.0), 5)


def world_glass_scar_shard(mesh: unreal.DynamicMesh, high: bool) -> None:
    box(mesh, (230.0, 190.0, 32.0), (0.0, 0.0, 2.0), PRIMARY, (0.0, 7.0, 0.0))
    for x, y, base_radius, height, pitch, yaw, material in (
        (0.0, 0.0, 62.0, 344.0, -7.0, 8.0, DARK),
        (-68.0, 22.0, 34.0, 214.0, 14.0, -19.0, LIGHT),
        (62.0, -32.0, 29.0, 178.0, -16.0, 27.0, DARK),
        (48.0, 54.0, 20.0, 126.0, 9.0, 42.0, LIGHT),
    ):
        cone(
            mesh,
            base_radius,
            4.0,
            height,
            (x, y, 18.0 + height * 0.5),
            material,
            (pitch, yaw, 0.0),
            6 if high else 5,
        )
    box(mesh, (16.0, 8.0, 248.0), (8.0, -43.0, 148.0), GLOW, (-6.0, 10.0, 0.0))
    if high:
        box(mesh, (9.0, 7.0, 142.0), (-58.0, 1.0, 115.0), GLOW, (14.0, -19.0, 0.0))


def world_matter_deposit(mesh: unreal.DynamicMesh, high: bool) -> None:
    cylinder(mesh, 128.0, 24.0, (0.0, 0.0, 12.0), PRIMARY, sides=12 if high else 8)
    torus(mesh, 104.0, 9.0, (0.0, 0.0, 26.0), DARK, high_detail=high)
    for index, (x, y, radius, height, pitch, yaw) in enumerate((
        (0.0, 0.0, 42.0, 226.0, -5.0, 8.0),
        (-52.0, 24.0, 30.0, 166.0, 12.0, -18.0),
        (48.0, -31.0, 27.0, 148.0, -13.0, 24.0),
        (42.0, 46.0, 21.0, 112.0, 7.0, 43.0),
        (-46.0, -43.0, 19.0, 96.0, -9.0, -39.0),
    )):
        cone(
            mesh,
            radius,
            3.0,
            height,
            (x, y, 25.0 + height * 0.5),
            GLOW if index == 0 else LIGHT,
            (pitch, yaw, 0.0),
            7 if high else 5,
        )
    for angle in range(0, 360, 60 if high else 120):
        radial_box(mesh, float(angle), 94.0, (88.0, 9.0, 7.0), 29.0, GLOW)


def world_glass_scar_ash_cut(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Production-oriented Ash Cut trench with layered banks and a continuous bed."""
    segment_specs = (
        (-38.0, -720.0, -8.0, 0.86),
        (18.0, -480.0, 5.0, 0.92),
        (-26.0, -240.0, -4.0, 0.88),
        (12.0, 0.0, 3.0, 0.94),
        (34.0, 240.0, 7.0, 0.88),
        (-14.0, 480.0, -5.0, 0.92),
        (-32.0, 720.0, -7.0, 0.86),
    )
    for index, (x, y, yaw, width_scale) in enumerate(segment_specs):
        cylinder(
            mesh,
            268.0,
            38.0,
            (x, y, 1.0),
            DARK,
            (0.0, yaw, 0.0),
            8 if high else 6,
            (width_scale, 1.0, 1.0),
        )
        cylinder(
            mesh,
            224.0,
            14.0,
            (x, y, 28.0),
            LIGHT,
            (0.0, yaw + 3.0, 0.0),
            8 if high else 6,
            (width_scale * 0.92, 1.0, 1.0),
        )
        box(
            mesh,
            (14.0, 238.0, 7.0),
            (x - 34.0, y, 39.0),
            GLOW,
            (0.0, yaw + 2.0, 0.0),
        )
        if high and index < len(segment_specs) - 1:
            next_x, next_y, _, _ = segment_specs[index + 1]
            box(
                mesh,
                (172.0, 72.0, 10.0),
                ((x + next_x) * 0.5, (y + next_y) * 0.5, 20.0),
                LIGHT,
                (0.0, yaw, 0.0),
            )

    bank_y = (-660.0, -440.0, -220.0, 0.0, 220.0, 440.0, 660.0)
    for side in (-1.0, 1.0):
        for index, y in enumerate(bank_y):
            height = (118.0, 158.0, 136.0, 184.0, 148.0, 166.0, 122.0)[index]
            x = side * (286.0 + (index % 3) * 18.0)
            yaw = side * (-8.0 + index * 2.5)
            box(
                mesh,
                (112.0, 214.0, 46.0),
                (x, y, 24.0 + (index % 2) * 9.0),
                PRIMARY,
                (side * 7.0, yaw, side * 4.0),
            )
            box(
                mesh,
                (92.0, 188.0, 18.0),
                (x - side * 15.0, y + 8.0, 57.0 + (index % 2) * 9.0),
                DARK,
                (side * 7.0, yaw - side * 3.0, side * 4.0),
            )
            if high:
                box(
                    mesh,
                    (74.0, 142.0, 8.0),
                    (x - side * 24.0, y + 4.0, 73.0 + (index % 2) * 9.0),
                    LIGHT,
                    (side * 7.0, yaw - side * 2.0, side * 4.0),
                )

        for y, height, lean in ((-520.0, 214.0, 14.0), (40.0, 286.0, -9.0), (545.0, 194.0, 11.0)):
            cone(
                mesh,
                38.0,
                4.0,
                height,
                (side * 354.0, y, 36.0 + height * 0.5),
                GLOW if y == 40.0 else PRIMARY,
                (side * lean, side * 12.0, 0.0),
                7 if high else 5,
            )


def world_glass_scar_buried_causeway(mesh: unreal.DynamicMesh, high: bool) -> None:
    """A buried transit spine with a continuous deck and repeated civic ribs."""
    # The recessed foundation and paired shoulders preserve a single broad,
    # straight silhouette even when detail falls away at distance.
    box(mesh, (650.0, 1810.0, 52.0), (0.0, 0.0, -10.0), DARK)
    box(mesh, (568.0, 1760.0, 28.0), (0.0, 0.0, 20.0), PRIMARY)
    for side in (-1.0, 1.0):
        box(mesh, (54.0, 1740.0, 62.0), (side * 295.0, 0.0, 34.0), DARK)
        box(mesh, (28.0, 1680.0, 20.0), (side * 257.0, 0.0, 63.0), LIGHT)

    segment_y = (-720.0, -480.0, -240.0, 0.0, 240.0, 480.0, 720.0)
    for index, y in enumerate(segment_y):
        # Pale ceramic deck plates sit inside darker structural coffers.
        box(mesh, (520.0, 204.0, 22.0), (0.0, y, 43.0), LIGHT)
        box(mesh, (424.0, 164.0, 9.0), (0.0, y, 59.0), PRIMARY)
        box(mesh, (24.0, 188.0, 8.0), (0.0, y, 67.0), GLOW)
        for side in (-1.0, 1.0):
            box(
                mesh,
                (48.0, 104.0, 116.0),
                (side * 302.0, y, 72.0),
                DARK,
                (0.0, 0.0, side * (7.0 if index % 2 == 0 else -5.0)),
            )
            box(
                mesh,
                (34.0, 132.0, 38.0),
                (side * 246.0, y, 82.0),
                LIGHT,
                (0.0, 0.0, side * 3.0),
            )
            if high:
                box(
                    mesh,
                    (86.0, 28.0, 22.0),
                    (side * 205.0, y - 76.0, 69.0),
                    PRIMARY,
                    (0.0, side * 4.0, 0.0),
                )

        if high:
            # Transverse seams and inset side conduits reinforce the engineered
            # cadence without turning the route into a luminous runway.
            box(mesh, (476.0, 10.0, 7.0), (0.0, y + 92.0, 65.0), DARK)
            for side in (-1.0, 1.0):
                box(mesh, (8.0, 148.0, 6.0), (side * 174.0, y, 68.0), GLOW)

    # Broken parapet spans and buried approach slabs prevent sterile symmetry
    # while retaining the causeway's uninterrupted north-south read.
    for side in (-1.0, 1.0):
        for y, length, yaw in (
            (-620.0, 176.0, -3.0),
            (-170.0, 238.0, 2.0),
            (320.0, 192.0, -2.0),
            (675.0, 138.0, 4.0),
        ):
            box(
                mesh,
                (42.0, length, 74.0),
                (side * 329.0, y, 35.0),
                PRIMARY,
                (side * 2.0, yaw, side * 5.0),
            )
        box(mesh, (470.0, 132.0, 24.0), (0.0, side * 858.0, 8.0), DARK, (0.0, 0.0, 0.0))


def world_glass_scar_folded_verge(mesh: unreal.DynamicMesh, high: bool) -> None:
    """A displaced possibility road built from hinged, offset verge plates."""
    plate_specs = (
        (-112.0, -720.0, -13.0, 12.0),
        (96.0, -480.0, 15.0, 30.0),
        (-86.0, -240.0, -16.0, 48.0),
        (118.0, 0.0, 14.0, 64.0),
        (-92.0, 240.0, -15.0, 48.0),
        (102.0, 480.0, 13.0, 30.0),
        (-108.0, 720.0, -11.0, 12.0),
    )
    for index, (x, y, yaw, z) in enumerate(plate_specs):
        # A dark displaced foundation carries a smaller pale walking face; the
        # opposing yaw at each step makes the route readable without color.
        cylinder(
            mesh,
            292.0,
            34.0,
            (x, y, z),
            DARK,
            (0.0, yaw, 0.0),
            6 if high else 5,
            (0.88, 1.0, 1.0),
        )
        cylinder(
            mesh,
            244.0,
            14.0,
            (x, y, z + 28.0),
            LIGHT,
            (0.0, yaw + 8.0, 0.0),
            6 if high else 5,
            (0.84, 1.0, 1.0),
        )
        box(mesh, (330.0, 12.0, 8.0), (x, y, z + 40.0), GLOW, (0.0, yaw + 90.0, 0.0))
        for side in (-1.0, 1.0):
            box(
                mesh,
                (34.0, 118.0, 58.0),
                (x + side * 246.0, y, z + 18.0),
                PRIMARY,
                (side * 5.0, yaw, side * 9.0),
            )
            if high:
                box(
                    mesh,
                    (18.0, 76.0, 18.0),
                    (x + side * 204.0, y + side * 46.0, z + 46.0),
                    GLOW,
                    (0.0, yaw + side * 8.0, 0.0),
                )
        if index < len(plate_specs) - 1:
            nx, ny, _, nz = plate_specs[index + 1]
            box(
                mesh,
                (168.0, 198.0, 26.0),
                ((x + nx) * 0.5, (y + ny) * 0.5, (z + nz) * 0.5 + 7.0),
                PRIMARY,
                (0.0, (yaw + plate_specs[index + 1][2]) * 0.5, 0.0),
            )
            if high:
                box(
                    mesh,
                    (38.0, 180.0, 12.0),
                    ((x + nx) * 0.5, (y + ny) * 0.5, (z + nz) * 0.5 + 25.0),
                    GLOW,
                    (0.0, (yaw + plate_specs[index + 1][2]) * 0.5, 0.0),
                )
    for side in (-1.0, 1.0):
        for y, height, lean in (
            (-590.0, 176.0, 12.0),
            (-180.0, 226.0, -9.0),
            (210.0, 258.0, 14.0),
            (610.0, 188.0, -11.0),
        ):
            cone(
                mesh,
                34.0,
                4.0,
                height,
                (side * 352.0, y, 16.0 + height * 0.5),
                GLOW if y == 210.0 else DARK,
                (side * lean, side * 18.0, 0.0),
                7 if high else 5,
            )


def vfx_selection_halo(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Broken-sun selection halo with cardinal acquisition brackets."""
    torus(mesh, 50.0, 3.2, (0.0, 0.0, 2.0), high_detail=high)
    for angle in range(0, 360, 90):
        radial_tangent_box(
            mesh,
            float(angle),
            52.0,
            (21.0, 5.0, 4.0),
            2.0,
            PRIMARY,
        )
        radial_box(
            mesh,
            float(angle),
            62.0,
            (12.0, 4.0, 4.0),
            2.0,
            PRIMARY,
        )
    if high:
        for angle in range(45, 360, 90):
            radial_tangent_box(
                mesh,
                float(angle),
                49.0,
                (10.0, 3.0, 3.0),
                2.0,
                PRIMARY,
            )


def command_sigil_base(mesh: unreal.DynamicMesh, high: bool) -> None:
    torus(mesh, 43.0, 2.8, (0.0, 0.0, 2.0), high_detail=high)
    for angle in range(0, 360, 90):
        radial_tangent_box(
            mesh,
            float(angle),
            43.0,
            (13.0, 4.0, 4.0),
            2.0,
            PRIMARY,
        )


def vfx_command_move(mesh: unreal.DynamicMesh, high: bool) -> None:
    command_sigil_base(mesh, high)
    box(mesh, (46.0, 7.0, 5.0), (-6.0, 0.0, 4.0), PRIMARY)
    box(mesh, (25.0, 7.0, 5.0), (20.0, 9.0, 4.0), PRIMARY, (0.0, 42.0, 0.0))
    box(mesh, (25.0, 7.0, 5.0), (20.0, -9.0, 4.0), PRIMARY, (0.0, -42.0, 0.0))


def vfx_command_attack_move(mesh: unreal.DynamicMesh, high: bool) -> None:
    command_sigil_base(mesh, high)
    box(mesh, (68.0, 8.0, 5.0), (0.0, 0.0, 4.0), PRIMARY, (0.0, 45.0, 0.0))
    box(mesh, (68.0, 8.0, 5.0), (0.0, 0.0, 4.0), PRIMARY, (0.0, -45.0, 0.0))
    if high:
        cylinder(mesh, 8.0, 5.0, (0.0, 0.0, 4.0), PRIMARY, sides=8)


def vfx_command_patrol(mesh: unreal.DynamicMesh, high: bool) -> None:
    command_sigil_base(mesh, high)
    for x, yaw in ((-15.0, 0.0), (15.0, 180.0)):
        box(mesh, (28.0, 6.0, 5.0), (x, 0.0, 4.0), PRIMARY, (0.0, yaw, 0.0))
        box(mesh, (18.0, 6.0, 5.0), (x + (12.0 if yaw == 0.0 else -12.0), 7.0, 4.0), PRIMARY, (0.0, yaw + 42.0, 0.0))
        box(mesh, (18.0, 6.0, 5.0), (x + (12.0 if yaw == 0.0 else -12.0), -7.0, 4.0), PRIMARY, (0.0, yaw - 42.0, 0.0))


def vfx_command_guard(mesh: unreal.DynamicMesh, high: bool) -> None:
    command_sigil_base(mesh, high)
    box(mesh, (7.0, 52.0, 5.0), (-16.0, 0.0, 4.0), PRIMARY)
    box(mesh, (7.0, 52.0, 5.0), (16.0, 0.0, 4.0), PRIMARY)
    box(mesh, (38.0, 7.0, 5.0), (0.0, 22.0, 4.0), PRIMARY)
    box(mesh, (28.0, 7.0, 5.0), (-8.0, -21.0, 4.0), PRIMARY, (0.0, 28.0, 0.0))
    box(mesh, (28.0, 7.0, 5.0), (8.0, -21.0, 4.0), PRIMARY, (0.0, -28.0, 0.0))


def vfx_command_build(mesh: unreal.DynamicMesh, high: bool) -> None:
    command_sigil_base(mesh, high)
    for x, y, yaw in (
        (-20.0, -20.0, 0.0),
        (20.0, -20.0, 90.0),
        (20.0, 20.0, 180.0),
        (-20.0, 20.0, 270.0),
    ):
        box(mesh, (28.0, 7.0, 5.0), (x, y, 4.0), PRIMARY, (0.0, yaw, 0.0))
    box(mesh, (36.0, 7.0, 5.0), (0.0, 0.0, 4.0), PRIMARY, (0.0, 45.0, 0.0))
    box(mesh, (36.0, 7.0, 5.0), (0.0, 0.0, 4.0), PRIMARY, (0.0, -45.0, 0.0))


def vfx_command_interact(mesh: unreal.DynamicMesh, high: bool) -> None:
    command_sigil_base(mesh, high)
    torus(mesh, 15.0, 3.2, (-12.0, 0.0, 4.0), PRIMARY, high_detail=high)
    torus(mesh, 15.0, 3.2, (12.0, 0.0, 4.0), PRIMARY, high_detail=high)
    box(mesh, (30.0, 5.0, 5.0), (0.0, 0.0, 4.0), PRIMARY)


def vfx_command_orbit(mesh: unreal.DynamicMesh, high: bool) -> None:
    box(mesh, (32.0, 8.0, 5.0), (-4.0, 0.0, 2.0), PRIMARY)
    box(mesh, (22.0, 8.0, 5.0), (12.0, 7.0, 2.0), PRIMARY, (0.0, 38.0, 0.0))
    box(mesh, (22.0, 8.0, 5.0), (12.0, -7.0, 2.0), PRIMARY, (0.0, -38.0, 0.0))
    if high:
        cylinder(mesh, 5.0, 5.0, (-19.0, 0.0, 2.0), PRIMARY, sides=8)


def vfx_destruction_ring(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Fractured radial shock ring used only after a visible entity is removed."""
    torus(mesh, 48.0, 3.5, (0.0, 0.0, 2.0), high_detail=high)
    step = 45 if high else 90
    for angle in range(0, 360, step):
        radial_tangent_box(
            mesh,
            float(angle),
            49.0,
            (18.0, 5.0, 4.0),
            2.0,
            PRIMARY,
        )


def vfx_destruction_core(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Compact broken-sun ember that collapses without becoming gameplay state."""
    cylinder(mesh, 12.0, 8.0, (0.0, 0.0, 4.0), PRIMARY, sides=12 if high else 8)
    for yaw in (0.0, 60.0, 120.0):
        box(mesh, (38.0, 6.0, 6.0), (0.0, 0.0, 5.0), PRIMARY, (0.0, yaw, 0.0))


def vfx_destruction_shard(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Directional debris accent with a deliberately asymmetric silhouette."""
    box(mesh, (34.0, 9.0, 7.0), (0.0, 0.0, 3.0), PRIMARY, (0.0, 18.0, 0.0))
    box(mesh, (18.0, 7.0, 6.0), (13.0, 7.0, 4.0), PRIMARY, (0.0, 54.0, 0.0))
    if high:
        box(mesh, (12.0, 5.0, 5.0), (-13.0, -6.0, 5.0), PRIMARY, (0.0, -38.0, 0.0))


VFX_ASSETS = (
    VfxAssetSpec("SelectionHalo", "Selection halo", "persistent selected-entity readability", vfx_selection_halo),
    VfxAssetSpec("CommandMove", "Move command sigil", "accepted move confirmation", vfx_command_move),
    VfxAssetSpec("CommandAttackMove", "Attack-move command sigil", "accepted attack-move confirmation", vfx_command_attack_move),
    VfxAssetSpec("CommandPatrol", "Patrol command sigil", "accepted patrol confirmation", vfx_command_patrol),
    VfxAssetSpec("CommandGuard", "Guard command sigil", "accepted guard confirmation", vfx_command_guard),
    VfxAssetSpec("CommandBuild", "Build command sigil", "accepted build confirmation", vfx_command_build),
    VfxAssetSpec("CommandInteract", "Interact command sigil", "accepted interaction confirmation", vfx_command_interact),
    VfxAssetSpec("CommandOrbit", "Command orbit shard", "motion-readable command accent", vfx_command_orbit),
)

DESTRUCTION_VFX_ASSETS = (
    VfxAssetSpec(
        "DestructionRing",
        "Destruction shock ring",
        "visible authoritative-removal confirmation",
        vfx_destruction_ring,
        DESTRUCTION_VFX_ASSET_REVISION,
    ),
    VfxAssetSpec(
        "DestructionCore",
        "Destruction core ember",
        "low-frequency destruction focal point",
        vfx_destruction_core,
        DESTRUCTION_VFX_ASSET_REVISION,
    ),
    VfxAssetSpec(
        "DestructionShard",
        "Destruction debris shard",
        "non-authoritative directional debris accent",
        vfx_destruction_shard,
        DESTRUCTION_VFX_ASSET_REVISION,
    ),
)


ASSETS = (
    AssetSpec("Meridian", "Units", "Surveyor", "Surveyor", "worker engineer", meridian_surveyor),
    AssetSpec("Meridian", "Units", "Lancer", "Lancer", "ranged line unit", meridian_lancer),
    AssetSpec("Meridian", "Units", "Bulwark", "Bulwark Team", "deployable heavy screen", meridian_bulwark),
    AssetSpec("Meridian", "Units", "RelaySkiff", "Relay Skiff", "scout and temporary supply", meridian_relay_skiff),
    AssetSpec("Meridian", "Structures", "Anchor", "Anchor", "headquarters and network root", meridian_anchor),
    AssetSpec("Meridian", "Structures", "PowerLink", "Power Link", "supply and network node", meridian_power_link),
    AssetSpec("Meridian", "Structures", "ArrayFoundry", "Array Foundry", "production and research", meridian_array_foundry),
    AssetSpec("Meridian", "Structures", "AegisPost", "Aegis Post", "network-powered defense", meridian_aegis_post),
    AssetSpec("Kharuun", "Units", "Tender", "Tender", "worker cultivator", kharuun_tender),
    AssetSpec("Kharuun", "Units", "Riftstalker", "Riftstalker", "mobile skirmisher", kharuun_riftstalker),
    AssetSpec("Kharuun", "Units", "Cairnback", "Cairnback", "assault screen", kharuun_cairnback),
    AssetSpec("Kharuun", "Units", "Resonant", "Resonant", "vibration scout", kharuun_resonant),
    AssetSpec("Kharuun", "Structures", "MemoryHearth", "Memory Hearth", "headquarters and adaptation root", kharuun_memory_hearth),
    AssetSpec("Kharuun", "Structures", "Waystone", "Waystone", "mobile supply node", kharuun_waystone),
    AssetSpec("Kharuun", "Structures", "GrowthBasin", "Growth Basin", "production and adaptation", kharuun_growth_basin),
    AssetSpec("Kharuun", "Structures", "ListeningSpine", "Listening Spine", "vibration detection", kharuun_listening_spine),
    AssetSpec("World", "Landmarks", "FutureWellBase", "Future Well foundation", "signature world landmark foundation", world_future_well_base),
    AssetSpec("World", "Landmarks", "FutureWellOrbit", "Future Well orbit", "animated possibility orbit", world_future_well_orbit),
    AssetSpec("World", "Landmarks", "FutureWellCore", "Future Well core", "fractured unrealized-future core", world_future_well_core),
    AssetSpec("World", "Landmarks", "FutureWellGlyph", "Future Well ground glyph", "state-readable ground manifestation", world_future_well_glyph),
    AssetSpec("World", "Environment", "GlassScarShelf", "Glass Scar terrain shelf", "impact-basin ground plate", world_glass_scar_shelf),
    AssetSpec("World", "Environment", "GlassScarRidge", "Glass Scar cliff tooth", "blocked-terrain silhouette", world_glass_scar_ridge),
    AssetSpec("World", "Environment", "GlassScarShard", "Glass Scar shard cluster", "fracture landmark and route edge", world_glass_scar_shard),
    AssetSpec("World", "Environment", "GlassScarAshCut", "Ash Cut crossing", "irregular western route signature", world_glass_scar_ash_cut),
    AssetSpec("World", "Environment", "GlassScarBuriedCauseway", "Buried Causeway crossing", "straight central route signature", world_glass_scar_buried_causeway),
    AssetSpec("World", "Environment", "GlassScarFoldedVerge", "Folded Verge crossing", "offset eastern route signature", world_glass_scar_folded_verge),
    AssetSpec("World", "Resources", "MatterDeposit", "Matter deposit", "neutral gatherable resource landmark", world_matter_deposit),
)


def create_surface_material() -> unreal.Material:
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        existing = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
        if isinstance(existing, unreal.Material):
            unreal.log(f"[ECHOES_ART_MATERIAL] path={MATERIAL_PATH} action=reused")
            return existing
        raise RuntimeError(f"Existing asset is not a Material: {MATERIAL_PATH}")

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(
        "M_EchoesSurface",
        f"{ART_ROOT}/Materials",
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError("Could not create M_EchoesSurface")

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -520, -180
    )
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.18, 0.48, 0.58, 1.0))

    metallic = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, 20
    )
    metallic.set_editor_property("parameter_name", "Metallic")
    metallic.set_editor_property("default_value", 0.25)

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, 130
    )
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.42)

    emission = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, -70
    )
    emission.set_editor_property("parameter_name", "EmissiveStrength")
    emission.set_editor_property("default_value", 0.0)

    multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -240, -100
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(emission, "", multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.set_metadata_tag(material, "Echoes.Creator", "Angelis Pseftis")
    unreal.EditorAssetLibrary.set_metadata_tag(material, "Echoes.Provenance", "Original scripted Unreal material")
    unreal.EditorAssetLibrary.set_metadata_tag(material, "Echoes.Status", "Vertical-slice art candidate")
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    return material


def create_world_surface_material() -> unreal.Material:
    if unreal.EditorAssetLibrary.does_asset_exist(WORLD_MATERIAL_PATH):
        existing = unreal.EditorAssetLibrary.load_asset(WORLD_MATERIAL_PATH)
        if isinstance(existing, unreal.Material):
            unreal.log(
                f"[ECHOES_ART_MATERIAL] path={WORLD_MATERIAL_PATH} action=reused"
            )
            return existing
        raise RuntimeError(
            f"Existing asset is not a Material: {WORLD_MATERIAL_PATH}"
        )

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(
        "M_EchoesWorldSurface",
        f"{ART_ROOT}/Materials",
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError("Could not create M_EchoesWorldSurface")

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -740, -220
    )
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property(
        "default_value", unreal.LinearColor(0.055, 0.07, 0.08, 1.0)
    )

    world_position = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionWorldPosition, -760, 70
    )
    noise = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionNoise, -520, 35
    )
    noise.set_editor_property("scale", 180.0)
    noise.set_editor_property("quality", 2)
    noise.set_editor_property("levels", 3)
    noise.set_editor_property("output_min", 0.58)
    noise.set_editor_property("output_max", 1.0)
    noise.set_editor_property("turbulence", True)
    unreal.MaterialEditingLibrary.connect_material_expressions(
        world_position, "", noise, "Position"
    )

    color_variation = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -250, -155
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        color, "", color_variation, "A"
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        noise, "", color_variation, "B"
    )

    metallic = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, 230
    )
    metallic.set_editor_property("parameter_name", "Metallic")
    metallic.set_editor_property("default_value", 0.16)

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, 330
    )
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.68)
    roughness_variation = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -245, 260
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        roughness, "", roughness_variation, "A"
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        noise, "", roughness_variation, "B"
    )

    emission = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, -315
    )
    emission.set_editor_property("parameter_name", "EmissiveStrength")
    emission.set_editor_property("default_value", 0.0)
    emissive_color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -245, -300
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        color, "", emissive_color, "A"
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        emission, "", emissive_color, "B"
    )

    unreal.MaterialEditingLibrary.connect_material_property(
        color_variation, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness_variation, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.set_metadata_tag(
        material, "Echoes.Creator", "Angelis Pseftis"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        material,
        "Echoes.Provenance",
        "Original scripted Unreal world material",
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        material, "Echoes.Status", "Vertical-slice environment candidate"
    )
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    return material


def create_ash_cut_materials() -> tuple[unreal.MaterialInterface, ...]:
    """Create the UV-driven four-zone material family for the Ash Cut route."""
    master = (
        unreal.EditorAssetLibrary.load_asset(ASH_CUT_MATERIAL_PATH)
        if unreal.EditorAssetLibrary.does_asset_exist(ASH_CUT_MATERIAL_PATH)
        else None
    )
    if master is not None and not isinstance(master, unreal.Material):
        raise RuntimeError(
            f"Existing Ash Cut master is not a Material: {ASH_CUT_MATERIAL_PATH}"
        )

    if master is None:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        master = tools.create_asset(
            "M_GlassScarAshCut",
            f"{ART_ROOT}/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
        if master is None:
            raise RuntimeError("Could not create M_GlassScarAshCut")

        color = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionVectorParameter, -880, -220
        )
        color.set_editor_property("parameter_name", "Color")
        color.set_editor_property(
            "default_value", unreal.LinearColor(0.055, 0.038, 0.034, 1.0)
        )
        detail_color = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionVectorParameter, -880, -80
        )
        detail_color.set_editor_property("parameter_name", "DetailColor")
        detail_color.set_editor_property(
            "default_value", unreal.LinearColor(0.19, 0.085, 0.035, 1.0)
        )
        texture_coordinate = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionTextureCoordinate, -900, 100
        )
        texture_coordinate.set_editor_property("coordinate_index", 0)
        texture_coordinate.set_editor_property("u_tiling", 3.0)
        texture_coordinate.set_editor_property("v_tiling", 6.0)
        noise = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionNoise, -650, 70
        )
        noise.set_editor_property("scale", 3.6)
        noise.set_editor_property("quality", 2)
        noise.set_editor_property("levels", 4)
        noise.set_editor_property("output_min", 0.0)
        noise.set_editor_property("output_max", 1.0)
        noise.set_editor_property("turbulence", True)
        unreal.MaterialEditingLibrary.connect_material_expressions(
            texture_coordinate, "", noise, "Position"
        )
        detail_strength = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionScalarParameter, -650, -70
        )
        detail_strength.set_editor_property("parameter_name", "DetailStrength")
        detail_strength.set_editor_property("default_value", 0.34)
        detail_mask = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionMultiply, -410, 30
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            noise, "", detail_mask, "A"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            detail_strength, "", detail_mask, "B"
        )
        color_blend = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionLinearInterpolate, -150, -150
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            color, "", color_blend, "A"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            detail_color, "", color_blend, "B"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            detail_mask, "", color_blend, "Alpha"
        )

        metallic = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionScalarParameter, -400, 210
        )
        metallic.set_editor_property("parameter_name", "Metallic")
        metallic.set_editor_property("default_value", 0.08)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionScalarParameter, -400, 310
        )
        roughness.set_editor_property("parameter_name", "Roughness")
        roughness.set_editor_property("default_value", 0.78)
        emission = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionScalarParameter, -400, -280
        )
        emission.set_editor_property("parameter_name", "EmissiveStrength")
        emission.set_editor_property("default_value", 0.0)
        emissive_color = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionMultiply, -130, -280
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            color_blend, "", emissive_color, "A"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            emission, "", emissive_color, "B"
        )

        unreal.MaterialEditingLibrary.connect_material_property(
            color_blend, "", unreal.MaterialProperty.MP_BASE_COLOR
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            metallic, "", unreal.MaterialProperty.MP_METALLIC
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            emissive_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
        unreal.MaterialEditingLibrary.layout_material_expressions(master)
        unreal.MaterialEditingLibrary.recompile_material(master)
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.Creator", "Angelis Pseftis"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master,
            "Echoes.Provenance",
            "Original UV-driven Ash Cut material authored in-project",
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.Status", "Production route-kit candidate"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.AssetRevision", ASH_CUT_ASSET_REVISION
        )
        unreal.EditorAssetLibrary.save_loaded_asset(master, False)

    zone_specs = (
        (
            unreal.LinearColor(0.040, 0.026, 0.026, 1.0),
            unreal.LinearColor(0.17, 0.050, 0.022, 1.0),
            0.10,
            0.82,
            0.0,
            0.28,
        ),
        (
            unreal.LinearColor(0.15, 0.105, 0.075, 1.0),
            unreal.LinearColor(0.32, 0.18, 0.075, 1.0),
            0.03,
            0.91,
            0.0,
            0.42,
        ),
        (
            unreal.LinearColor(0.055, 0.018, 0.026, 1.0),
            unreal.LinearColor(0.44, 0.075, 0.025, 1.0),
            0.46,
            0.24,
            0.0,
            0.20,
        ),
        (
            unreal.LinearColor(0.50, 0.025, 0.012, 1.0),
            unreal.LinearColor(1.0, 0.24, 0.035, 1.0),
            0.18,
            0.31,
            3.4,
            0.50,
        ),
    )
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    instances: list[unreal.MaterialInterface] = []
    for path, values in zip(ASH_CUT_MATERIAL_INSTANCE_PATHS, zone_specs):
        instance = (
            unreal.EditorAssetLibrary.load_asset(path)
            if unreal.EditorAssetLibrary.does_asset_exist(path)
            else None
        )
        if instance is None:
            asset_name = path.rsplit("/", 1)[1]
            instance = tools.create_asset(
                asset_name,
                f"{ART_ROOT}/Materials",
                unreal.MaterialInstanceConstant,
                unreal.MaterialInstanceConstantFactoryNew(),
            )
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            raise RuntimeError(f"Ash Cut material instance is invalid: {path}")
        if (
            unreal.EditorAssetLibrary.get_metadata_tag(
                instance, "Echoes.AssetRevision"
            )
            == ASH_CUT_ASSET_REVISION
        ):
            instances.append(instance)
            continue
        unreal.MaterialEditingLibrary.set_material_instance_parent(instance, master)
        color_value, detail_value, metallic_value, roughness_value, emission_value, detail_value_strength = values
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, "Color", color_value
        )
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, "DetailColor", detail_value
        )
        for parameter_name, parameter_value in (
            ("Metallic", metallic_value),
            ("Roughness", roughness_value),
            ("EmissiveStrength", emission_value),
            ("DetailStrength", detail_value_strength),
        ):
            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
                instance, parameter_name, parameter_value
            )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.Creator", "Angelis Pseftis"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance,
            "Echoes.Provenance",
            "Original Ash Cut material instance authored in-project",
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.Status", "Production route-kit candidate"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.AssetRevision", ASH_CUT_ASSET_REVISION
        )
        unreal.EditorAssetLibrary.save_loaded_asset(instance, False)
        instances.append(instance)
    return tuple(instances)


def create_buried_causeway_materials() -> tuple[unreal.MaterialInterface, ...]:
    """Create the dedicated four-zone material family for the Buried Causeway."""
    master = (
        unreal.EditorAssetLibrary.load_asset(BURIED_CAUSEWAY_MATERIAL_PATH)
        if unreal.EditorAssetLibrary.does_asset_exist(BURIED_CAUSEWAY_MATERIAL_PATH)
        else None
    )
    if master is not None and not isinstance(master, unreal.Material):
        raise RuntimeError(
            "Existing Buried Causeway master is not a Material: "
            f"{BURIED_CAUSEWAY_MATERIAL_PATH}"
        )
    if master is None:
        # The route masters intentionally share the same compact UV/noise graph,
        # then diverge through registered instances. Duplicating the authored
        # graph avoids editor-only expression drift between route families.
        master = unreal.EditorAssetLibrary.duplicate_asset(
            ASH_CUT_MATERIAL_PATH, BURIED_CAUSEWAY_MATERIAL_PATH
        )
        if master is None or not isinstance(master, unreal.Material):
            raise RuntimeError("Could not create M_GlassScarBuriedCauseway")
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.Creator", "Angelis Pseftis"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master,
            "Echoes.Provenance",
            "Original UV-driven Buried Causeway material authored in-project",
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.Status", "Production route-kit candidate"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.AssetRevision", BURIED_CAUSEWAY_ASSET_REVISION
        )
        unreal.EditorAssetLibrary.save_loaded_asset(master, False)

    zone_specs = (
        (
            unreal.LinearColor(0.090, 0.082, 0.070, 1.0),
            unreal.LinearColor(0.22, 0.17, 0.115, 1.0),
            0.10,
            0.78,
            0.0,
            0.25,
        ),
        (
            unreal.LinearColor(0.026, 0.030, 0.034, 1.0),
            unreal.LinearColor(0.070, 0.082, 0.090, 1.0),
            0.34,
            0.30,
            0.0,
            0.20,
        ),
        (
            unreal.LinearColor(0.34, 0.29, 0.21, 1.0),
            unreal.LinearColor(0.64, 0.53, 0.36, 1.0),
            0.03,
            0.58,
            0.0,
            0.32,
        ),
        (
            unreal.LinearColor(0.025, 0.22, 0.31, 1.0),
            unreal.LinearColor(0.18, 0.72, 0.88, 1.0),
            0.18,
            0.25,
            2.3,
            0.18,
        ),
    )
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    instances: list[unreal.MaterialInterface] = []
    for path, values in zip(BURIED_CAUSEWAY_MATERIAL_INSTANCE_PATHS, zone_specs):
        instance = (
            unreal.EditorAssetLibrary.load_asset(path)
            if unreal.EditorAssetLibrary.does_asset_exist(path)
            else None
        )
        if instance is None:
            asset_name = path.rsplit("/", 1)[1]
            instance = tools.create_asset(
                asset_name,
                f"{ART_ROOT}/Materials",
                unreal.MaterialInstanceConstant,
                unreal.MaterialInstanceConstantFactoryNew(),
            )
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            raise RuntimeError(f"Buried Causeway material instance is invalid: {path}")
        if (
            unreal.EditorAssetLibrary.get_metadata_tag(
                instance, "Echoes.AssetRevision"
            )
            == BURIED_CAUSEWAY_ASSET_REVISION
        ):
            instances.append(instance)
            continue
        unreal.MaterialEditingLibrary.set_material_instance_parent(instance, master)
        (
            color_value,
            detail_value,
            metallic_value,
            roughness_value,
            emission_value,
            detail_value_strength,
        ) = values
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, "Color", color_value
        )
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, "DetailColor", detail_value
        )
        for parameter_name, parameter_value in (
            ("Metallic", metallic_value),
            ("Roughness", roughness_value),
            ("EmissiveStrength", emission_value),
            ("DetailStrength", detail_value_strength),
        ):
            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
                instance, parameter_name, parameter_value
            )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.Creator", "Angelis Pseftis"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance,
            "Echoes.Provenance",
            "Original Buried Causeway material instance authored in-project",
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.Status", "Production route-kit candidate"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.AssetRevision", BURIED_CAUSEWAY_ASSET_REVISION
        )
        unreal.EditorAssetLibrary.save_loaded_asset(instance, False)
        instances.append(instance)
    return tuple(instances)


def create_folded_verge_materials() -> tuple[unreal.MaterialInterface, ...]:
    """Create the dedicated four-zone material family for the Folded Verge."""
    master = (
        unreal.EditorAssetLibrary.load_asset(FOLDED_VERGE_MATERIAL_PATH)
        if unreal.EditorAssetLibrary.does_asset_exist(FOLDED_VERGE_MATERIAL_PATH)
        else None
    )
    if master is not None and not isinstance(master, unreal.Material):
        raise RuntimeError(
            f"Existing Folded Verge master is not a Material: {FOLDED_VERGE_MATERIAL_PATH}"
        )
    if master is None:
        master = unreal.EditorAssetLibrary.duplicate_asset(
            ASH_CUT_MATERIAL_PATH, FOLDED_VERGE_MATERIAL_PATH
        )
        if master is None or not isinstance(master, unreal.Material):
            raise RuntimeError("Could not create M_GlassScarFoldedVerge")
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.Creator", "Angelis Pseftis"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master,
            "Echoes.Provenance",
            "Original UV-driven Folded Verge material authored in-project",
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.Status", "Production route-kit candidate"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            master, "Echoes.AssetRevision", FOLDED_VERGE_ASSET_REVISION
        )
        unreal.EditorAssetLibrary.save_loaded_asset(master, False)

    zone_specs = (
        (
            unreal.LinearColor(0.030, 0.018, 0.044, 1.0),
            unreal.LinearColor(0.16, 0.035, 0.18, 1.0),
            0.42,
            0.22,
            0.0,
            0.22,
        ),
        (
            unreal.LinearColor(0.13, 0.018, 0.15, 1.0),
            unreal.LinearColor(0.42, 0.035, 0.34, 1.0),
            0.22,
            0.34,
            0.0,
            0.30,
        ),
        (
            unreal.LinearColor(0.29, 0.25, 0.32, 1.0),
            unreal.LinearColor(0.56, 0.43, 0.58, 1.0),
            0.04,
            0.62,
            0.0,
            0.24,
        ),
        (
            unreal.LinearColor(0.42, 0.015, 0.44, 1.0),
            unreal.LinearColor(0.96, 0.12, 0.68, 1.0),
            0.16,
            0.20,
            3.0,
            0.16,
        ),
    )
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    instances: list[unreal.MaterialInterface] = []
    for path, values in zip(FOLDED_VERGE_MATERIAL_INSTANCE_PATHS, zone_specs):
        instance = (
            unreal.EditorAssetLibrary.load_asset(path)
            if unreal.EditorAssetLibrary.does_asset_exist(path)
            else None
        )
        if instance is None:
            asset_name = path.rsplit("/", 1)[1]
            instance = tools.create_asset(
                asset_name,
                f"{ART_ROOT}/Materials",
                unreal.MaterialInstanceConstant,
                unreal.MaterialInstanceConstantFactoryNew(),
            )
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            raise RuntimeError(f"Folded Verge material instance is invalid: {path}")
        if (
            unreal.EditorAssetLibrary.get_metadata_tag(
                instance, "Echoes.AssetRevision"
            )
            == FOLDED_VERGE_ASSET_REVISION
        ):
            instances.append(instance)
            continue
        unreal.MaterialEditingLibrary.set_material_instance_parent(instance, master)
        (
            color_value,
            detail_value,
            metallic_value,
            roughness_value,
            emission_value,
            detail_value_strength,
        ) = values
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, "Color", color_value
        )
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, "DetailColor", detail_value
        )
        for parameter_name, parameter_value in (
            ("Metallic", metallic_value),
            ("Roughness", roughness_value),
            ("EmissiveStrength", emission_value),
            ("DetailStrength", detail_value_strength),
        ):
            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
                instance, parameter_name, parameter_value
            )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.Creator", "Angelis Pseftis"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance,
            "Echoes.Provenance",
            "Original Folded Verge material instance authored in-project",
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.Status", "Production route-kit candidate"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            instance, "Echoes.AssetRevision", FOLDED_VERGE_ASSET_REVISION
        )
        unreal.EditorAssetLibrary.save_loaded_asset(instance, False)
        instances.append(instance)
    return tuple(instances)


def create_presentation_vfx_material() -> unreal.Material:
    existing = (
        unreal.EditorAssetLibrary.load_asset(VFX_MATERIAL_PATH)
        if unreal.EditorAssetLibrary.does_asset_exist(VFX_MATERIAL_PATH)
        else None
    )
    if existing is not None and not isinstance(existing, unreal.Material):
        raise RuntimeError(
            f"Existing presentation VFX asset is not a Material: {VFX_MATERIAL_PATH}"
        )
    if existing is not None:
        revision = unreal.EditorAssetLibrary.get_metadata_tag(
            existing, "Echoes.AssetRevision"
        )
        if revision == PRESENTATION_VFX_ASSET_REVISION:
            unreal.log(
                f"[ECHOES_PRESENTATION_VFX_MATERIAL] path={VFX_MATERIAL_PATH} action=reused"
            )
            return existing
        if not unreal.EditorAssetLibrary.delete_asset(VFX_MATERIAL_PATH):
            raise RuntimeError(
                f"Could not replace presentation VFX material: {VFX_MATERIAL_PATH}"
            )

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(
        "M_EchoesPresentationVFX",
        f"{ART_ROOT}/Materials",
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError("Could not create M_EchoesPresentationVFX")

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -560, -160
    )
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property(
        "default_value", unreal.LinearColor(0.08, 0.92, 1.0, 1.0)
    )
    emission = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -560, -20
    )
    emission.set_editor_property("parameter_name", "EmissiveStrength")
    emission.set_editor_property("default_value", 2.0)
    emissive_color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -260, -80
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        color, "", emissive_color, "A"
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        emission, "", emissive_color, "B"
    )
    metallic = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -300, 100
    )
    metallic.set_editor_property("r", 0.15)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -300, 190
    )
    roughness.set_editor_property("r", 0.22)
    unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.set_metadata_tag(
        material, "Echoes.Creator", "Angelis Pseftis"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        material,
        "Echoes.Provenance",
        "Original emissive presentation material authored in-project",
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        material, "Echoes.Status", "Production-oriented presentation VFX candidate"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        material, "Echoes.RuntimeAuthority", "Presentation only"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        material, "Echoes.AssetRevision", PRESENTATION_VFX_ASSET_REVISION
    )
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    return material


def build_dynamic_mesh(
    spec: AssetSpec | VfxAssetSpec, high_detail: bool
) -> unreal.DynamicMesh:
    mesh = unreal.DynamicMesh()
    mesh.enable_material_i_ds()
    spec.builder(mesh, high_detail)
    if mesh.is_empty():
        raise RuntimeError(f"Builder returned an empty mesh for {spec.display_name}")
    return mesh


def create_presentation_vfx_mesh(
    spec: VfxAssetSpec, material: unreal.MaterialInterface
) -> unreal.StaticMesh:
    existing = (
        unreal.EditorAssetLibrary.load_asset(spec.asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(spec.asset_path)
        else None
    )
    if existing is not None and not isinstance(existing, unreal.StaticMesh):
        raise RuntimeError(f"Existing VFX asset is not a StaticMesh: {spec.asset_path}")
    if existing is not None:
        revision = unreal.EditorAssetLibrary.get_metadata_tag(
            existing, "Echoes.AssetRevision"
        )
        if revision == spec.revision:
            unreal.log(
                "[ECHOES_PRESENTATION_VFX_ASSET] "
                f"path={spec.asset_path} display={spec.display_name} "
                f"lods={existing.get_num_lods()} "
                f"lod0Triangles={existing.get_num_triangles(0)} "
                f"lod1Triangles={existing.get_num_triangles(1)} action=reused"
            )
            return existing
        if not unreal.EditorAssetLibrary.delete_asset(spec.asset_path):
            raise RuntimeError(f"Could not replace VFX asset: {spec.asset_path}")

    lod_zero = build_dynamic_mesh(spec, True)
    lod_one = build_dynamic_mesh(spec, False)
    options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions(
        enable_recompute_normals=False,
        enable_recompute_tangents=False,
        enable_nanite=False,
        enable_collision=False,
        collision_mode=unreal.CollisionTraceFlag.CTF_USE_DEFAULT,
    )
    asset, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh_lods(
        [lod_zero, lod_one], spec.asset_path, options
    )
    if asset is None or outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError(
            f"Presentation VFX mesh creation failed for {spec.asset_path}: {outcome}"
        )
    asset.set_material(0, material)
    mesh_editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mesh_editor.remove_collisions(asset)
    unreal.EditorAssetLibrary.set_metadata_tag(
        asset, "Echoes.Creator", "Angelis Pseftis"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Role", spec.role)
    unreal.EditorAssetLibrary.set_metadata_tag(
        asset, "Echoes.Provenance", "Original scripted presentation geometry"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        asset, "Echoes.Status", "Production-oriented presentation VFX candidate"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        asset, "Echoes.RuntimeAuthority", "Presentation only"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        asset, "Echoes.CollisionPolicy", "No asset or runtime collision"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        asset, "Echoes.AssetRevision", spec.revision
    )
    unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
    unreal.log(
        "[ECHOES_PRESENTATION_VFX_ASSET] "
        f"path={spec.asset_path} display={spec.display_name} "
        f"lods={asset.get_num_lods()} "
        f"lod0Triangles={asset.get_num_triangles(0)} "
        f"lod1Triangles={asset.get_num_triangles(1)} action=created"
    )
    return asset


def create_static_mesh(
    spec: AssetSpec, materials: Sequence[unreal.MaterialInterface]
) -> unreal.StaticMesh:
    if len(materials) != 4:
        raise RuntimeError(f"Exactly four materials are required for {spec.display_name}")
    route_revisions = {
        "GlassScarAshCut": ASH_CUT_ASSET_REVISION,
        "GlassScarBuriedCauseway": BURIED_CAUSEWAY_ASSET_REVISION,
        "GlassScarFoldedVerge": FOLDED_VERGE_ASSET_REVISION,
    }
    route_revision = route_revisions.get(spec.name)
    is_production_route = route_revision is not None
    route_labels = {
        "GlassScarAshCut": "Ash Cut",
        "GlassScarBuriedCauseway": "Buried Causeway",
        "GlassScarFoldedVerge": "Folded Verge",
    }
    route_label = route_labels.get(spec.name, spec.display_name)
    if unreal.EditorAssetLibrary.does_asset_exist(spec.asset_path):
        existing = unreal.EditorAssetLibrary.load_asset(spec.asset_path)
        if isinstance(existing, unreal.StaticMesh):
            revision = unreal.EditorAssetLibrary.get_metadata_tag(
                existing, "Echoes.AssetRevision"
            )
            if is_production_route and revision != route_revision:
                if not unreal.EditorAssetLibrary.delete_asset(spec.asset_path):
                    raise RuntimeError(
                        f"Could not replace legacy {route_label} asset: {spec.asset_path}"
                    )
                existing = None
        if isinstance(existing, unreal.StaticMesh):
            unreal.log(
                "[ECHOES_ART_ASSET] "
                f"path={spec.asset_path} display={spec.display_name} faction={spec.faction} "
                f"lods={existing.get_num_lods()} "
                f"lod0Vertices={existing.get_num_vertices(0)} lod0Triangles={existing.get_num_triangles(0)} "
                f"lod1Vertices={existing.get_num_vertices(1)} lod1Triangles={existing.get_num_triangles(1)} "
                "action=reused"
            )
            return existing
        if existing is not None:
            raise RuntimeError(f"Existing asset is not a StaticMesh: {spec.asset_path}")

    lod_zero = build_dynamic_mesh(spec, True)
    lod_one = build_dynamic_mesh(spec, False)
    options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions(
        enable_recompute_normals=False,
        enable_recompute_tangents=False,
        enable_nanite=False,
        enable_collision=True,
        collision_mode=unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
    )
    asset, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh_lods(
        [lod_zero, lod_one], spec.asset_path, options
    )
    if asset is None or outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError(f"Static-mesh creation failed for {spec.asset_path}: {outcome}")

    while len(asset.get_editor_property("static_materials")) < 4:
        asset.add_material(materials[0])
    for material_index in range(4):
        asset.set_material(material_index, materials[material_index])

    if is_production_route:
        mesh_editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        for lod_index in range(asset.get_num_lods()):
            if not mesh_editor.generate_box_uv_channel(
                asset,
                lod_index,
                0,
                unreal.Vector(0.0, 0.0, 80.0),
                unreal.Rotator(0.0, 0.0, 0.0),
                unreal.Vector(900.0, 1800.0, 420.0),
            ):
                raise RuntimeError(
                    f"Could not author {route_label} UV0 for LOD {lod_index}"
                )
            if mesh_editor.get_num_uv_channels(asset, lod_index) < 2:
                if not mesh_editor.add_uv_channel(asset, lod_index):
                    raise RuntimeError(
                        f"Could not add {route_label} lightmap UV for LOD {lod_index}"
                    )
            if not mesh_editor.generate_box_uv_channel(
                asset,
                lod_index,
                1,
                unreal.Vector(0.0, 0.0, 80.0),
                unreal.Rotator(0.0, 0.0, 0.0),
                unreal.Vector(1900.0, 1900.0, 1900.0),
            ):
                raise RuntimeError(
                    f"Could not seed {route_label} UV1 for LOD {lod_index}"
                )
        mesh_editor.set_generate_lightmap_uv(asset, True)
        mesh_editor.remove_collisions(asset)
        collision_index = mesh_editor.add_simple_collisions(
            asset, unreal.ScriptCollisionShapeType.BOX
        )
        if collision_index < 0:
            raise RuntimeError(f"Could not author {route_label} simple collision")
        body_setup = asset.get_editor_property("body_setup")
        if body_setup is None:
            raise RuntimeError(f"{route_label} static mesh has no body setup")
        body_setup.set_editor_property(
            "collision_trace_flag",
            unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX,
        )
        asset.set_editor_property("light_map_coordinate_index", 1)
        asset.set_editor_property("light_map_resolution", 128)
        uv_counts = [
            mesh_editor.get_num_uv_channels(asset, lod_index)
            for lod_index in range(asset.get_num_lods())
        ]
        if any(count < 2 for count in uv_counts):
            raise RuntimeError(
                f"{route_label} requires two UV channels per LOD: {uv_counts}"
            )

    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Creator", "Angelis Pseftis")
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Faction", spec.faction)
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Role", spec.role)
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Provenance", "Original scripted Unreal geometry")
    unreal.EditorAssetLibrary.set_metadata_tag(
        asset,
        "Echoes.Status",
        "Production route-kit candidate" if is_production_route else "Vertical-slice art candidate",
    )
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.RuntimeAuthority", "Presentation only")
    if is_production_route:
        unreal.EditorAssetLibrary.set_metadata_tag(
            asset, "Echoes.AssetRevision", route_revision
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            asset, "Echoes.UVPolicy", "UV0 tiled surface; UV1 generated lightmap"
        )
        unreal.EditorAssetLibrary.set_metadata_tag(
            asset, "Echoes.CollisionPolicy", "Authored simple box; runtime disabled"
        )
    unreal.EditorAssetLibrary.save_loaded_asset(asset, False)

    unreal.log(
        "[ECHOES_ART_ASSET] "
        f"path={spec.asset_path} display={spec.display_name} faction={spec.faction} "
        f"lods={asset.get_num_lods()} "
        f"lod0Vertices={asset.get_num_vertices(0)} lod0Triangles={asset.get_num_triangles(0)} "
        f"lod1Vertices={asset.get_num_vertices(1)} lod1Triangles={asset.get_num_triangles(1)}"
    )
    return asset


def main() -> None:
    unreal.log(
        "[ECHOES_ART_BEGIN] generating 16 roster assets, 4 Future Well assets, "
        "7 Glass Scar environment assets, 8 selection/command VFX assets, "
        "and 3 destruction VFX assets"
    )
    surface_material = create_surface_material()
    world_surface_material = create_world_surface_material()
    ash_cut_materials = create_ash_cut_materials()
    buried_causeway_materials = create_buried_causeway_materials()
    folded_verge_materials = create_folded_verge_materials()
    presentation_vfx_material = create_presentation_vfx_material()
    generated = [
        create_static_mesh(
            spec,
            ash_cut_materials
            if spec.name == "GlassScarAshCut"
            else (
                buried_causeway_materials
                if spec.name == "GlassScarBuriedCauseway"
                else (
                    folded_verge_materials
                    if spec.name == "GlassScarFoldedVerge"
                    else (
                        [world_surface_material] * 4
                        if spec.faction == "World"
                        else [surface_material] * 4
                    )
                )
            ),
        )
        for spec in ASSETS
    ]
    presentation_vfx_assets = [
        create_presentation_vfx_mesh(spec, presentation_vfx_material)
        for spec in VFX_ASSETS
    ]
    destruction_vfx_assets = [
        create_presentation_vfx_mesh(spec, presentation_vfx_material)
        for spec in DESTRUCTION_VFX_ASSETS
    ]
    ash_cut_asset = next(
        asset
        for asset, spec in zip(generated, ASSETS)
        if spec.name == "GlassScarAshCut"
    )
    mesh_editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    ash_cut_uvs = [
        mesh_editor.get_num_uv_channels(ash_cut_asset, lod_index)
        for lod_index in range(ash_cut_asset.get_num_lods())
    ]
    ash_cut_materials = [
        ash_cut_asset.get_material(index).get_path_name()
        for index in range(4)
        if ash_cut_asset.get_material(index) is not None
    ]
    ash_cut_collision_count = mesh_editor.get_simple_collision_count(ash_cut_asset)
    if (
        any(count < 2 for count in ash_cut_uvs)
        or len(ash_cut_materials) != 4
        or any("MI_GlassScarAshCut_" not in path for path in ash_cut_materials)
        or ash_cut_collision_count < 1
    ):
        raise RuntimeError(
            "Ash Cut route-kit audit failed: "
            f"uvs={ash_cut_uvs} materials={ash_cut_materials} "
            f"collision={ash_cut_collision_count}"
        )
    unreal.log(
        "[ECHOES_ASH_CUT_READY] "
        f"revision={ASH_CUT_ASSET_REVISION} lods={ash_cut_asset.get_num_lods()} "
        f"lod0Triangles={ash_cut_asset.get_num_triangles(0)} "
        f"lod1Triangles={ash_cut_asset.get_num_triangles(1)} "
        f"uvChannels={','.join(str(value) for value in ash_cut_uvs)} "
        f"materials={len(ash_cut_materials)} simpleCollision={ash_cut_collision_count} "
        "runtimeAuthority=presentation runtimeCollision=false"
    )
    buried_causeway_asset = next(
        asset
        for asset, spec in zip(generated, ASSETS)
        if spec.name == "GlassScarBuriedCauseway"
    )
    buried_causeway_uvs = [
        mesh_editor.get_num_uv_channels(buried_causeway_asset, lod_index)
        for lod_index in range(buried_causeway_asset.get_num_lods())
    ]
    buried_causeway_materials = [
        buried_causeway_asset.get_material(index).get_path_name()
        for index in range(4)
        if buried_causeway_asset.get_material(index) is not None
    ]
    buried_causeway_collision_count = mesh_editor.get_simple_collision_count(
        buried_causeway_asset
    )
    if (
        any(count < 2 for count in buried_causeway_uvs)
        or len(buried_causeway_materials) != 4
        or any(
            "MI_GlassScarBuriedCauseway_" not in path
            for path in buried_causeway_materials
        )
        or buried_causeway_collision_count < 1
    ):
        raise RuntimeError(
            "Buried Causeway route-kit audit failed: "
            f"uvs={buried_causeway_uvs} materials={buried_causeway_materials} "
            f"collision={buried_causeway_collision_count}"
        )
    unreal.log(
        "[ECHOES_BURIED_CAUSEWAY_READY] "
        f"revision={BURIED_CAUSEWAY_ASSET_REVISION} "
        f"lods={buried_causeway_asset.get_num_lods()} "
        f"lod0Triangles={buried_causeway_asset.get_num_triangles(0)} "
        f"lod1Triangles={buried_causeway_asset.get_num_triangles(1)} "
        f"uvChannels={','.join(str(value) for value in buried_causeway_uvs)} "
        f"materials={len(buried_causeway_materials)} "
        f"simpleCollision={buried_causeway_collision_count} "
        "runtimeAuthority=presentation runtimeCollision=false"
    )
    folded_verge_asset = next(
        asset
        for asset, spec in zip(generated, ASSETS)
        if spec.name == "GlassScarFoldedVerge"
    )
    folded_verge_uvs = [
        mesh_editor.get_num_uv_channels(folded_verge_asset, lod_index)
        for lod_index in range(folded_verge_asset.get_num_lods())
    ]
    folded_verge_materials = [
        folded_verge_asset.get_material(index).get_path_name()
        for index in range(4)
        if folded_verge_asset.get_material(index) is not None
    ]
    folded_verge_collision_count = mesh_editor.get_simple_collision_count(
        folded_verge_asset
    )
    if (
        any(count < 2 for count in folded_verge_uvs)
        or len(folded_verge_materials) != 4
        or any(
            "MI_GlassScarFoldedVerge_" not in path
            for path in folded_verge_materials
        )
        or folded_verge_collision_count < 1
    ):
        raise RuntimeError(
            "Folded Verge route-kit audit failed: "
            f"uvs={folded_verge_uvs} materials={folded_verge_materials} "
            f"collision={folded_verge_collision_count}"
        )
    unreal.log(
        "[ECHOES_FOLDED_VERGE_READY] "
        f"revision={FOLDED_VERGE_ASSET_REVISION} "
        f"lods={folded_verge_asset.get_num_lods()} "
        f"lod0Triangles={folded_verge_asset.get_num_triangles(0)} "
        f"lod1Triangles={folded_verge_asset.get_num_triangles(1)} "
        f"uvChannels={','.join(str(value) for value in folded_verge_uvs)} "
        f"materials={len(folded_verge_materials)} "
        f"simpleCollision={folded_verge_collision_count} "
        "runtimeAuthority=presentation runtimeCollision=false"
    )
    vfx_collision_counts = [
        mesh_editor.get_simple_collision_count(asset)
        for asset in presentation_vfx_assets
    ]
    if (
        len(presentation_vfx_assets) != 8
        or any(asset.get_num_lods() != 2 for asset in presentation_vfx_assets)
        or any(count != 0 for count in vfx_collision_counts)
        or any(
            asset.get_material(0) is None
            or "M_EchoesPresentationVFX" not in asset.get_material(0).get_path_name()
            for asset in presentation_vfx_assets
        )
    ):
        raise RuntimeError(
            "Presentation VFX audit failed: "
            f"assets={len(presentation_vfx_assets)} "
            f"lods={[asset.get_num_lods() for asset in presentation_vfx_assets]} "
            f"collision={vfx_collision_counts}"
        )
    unreal.log(
        "[ECHOES_PRESENTATION_VFX_READY] "
        f"revision={PRESENTATION_VFX_ASSET_REVISION} assets=8 selection=1 "
        "commands=6 orbit=1 lods=2 simpleCollision=0 "
        "runtimeAuthority=presentation reducedMotion=steady "
        "reducedFlashing=steadyLowEmission finalArt=false"
    )
    destruction_collision_counts = [
        mesh_editor.get_simple_collision_count(asset)
        for asset in destruction_vfx_assets
    ]
    if (
        len(destruction_vfx_assets) != 3
        or any(asset.get_num_lods() != 2 for asset in destruction_vfx_assets)
        or any(count != 0 for count in destruction_collision_counts)
        or any(
            asset.get_material(0) is None
            or "M_EchoesPresentationVFX" not in asset.get_material(0).get_path_name()
            for asset in destruction_vfx_assets
        )
    ):
        raise RuntimeError(
            "Destruction VFX audit failed: "
            f"assets={len(destruction_vfx_assets)} "
            f"lods={[asset.get_num_lods() for asset in destruction_vfx_assets]} "
            f"collision={destruction_collision_counts}"
        )
    unreal.log(
        "[ECHOES_DESTRUCTION_VFX_READY] "
        f"revision={DESTRUCTION_VFX_ASSET_REVISION} assets=3 lods=2 "
        "simpleCollision=0 runtimeAuthority=presentation "
        "reducedMotion=steady reducedFlashing=steadyLowEmission finalArt=false"
    )
    unreal.log(
        f"[ECHOES_ART_COMPLETE] generated={len(generated) + len(presentation_vfx_assets) + len(destruction_vfx_assets)} "
        f"roster=16 landmarks=4 environment=7 vfx=8 destructionVfx=3 material={MATERIAL_PATH} "
        f"worldMaterial={WORLD_MATERIAL_PATH} vfxMaterial={VFX_MATERIAL_PATH}"
    )


if __name__ == "__main__":
    main()
