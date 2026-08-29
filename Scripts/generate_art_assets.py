"""Generate the first authored-looking Echoes unit and structure mesh set in Unreal.

Run this script only through Scripts/generate_art_assets.sh.  The generated
assets are ordinary StaticMesh and Material assets; Geometry Scripting and
Python are editor-time dependencies, not runtime authority.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Callable

import unreal


ART_ROOT = "/Game/Art/Generated"
MATERIAL_PATH = f"{ART_ROOT}/Materials/M_EchoesSurface"

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
) -> None:
    mesh.append_cylinder(
        primitive_options(material_id),
        transform(at, rotation),
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
)


def create_surface_material() -> unreal.Material:
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        if not unreal.EditorAssetLibrary.delete_asset(MATERIAL_PATH):
            raise RuntimeError(f"Could not replace generated material {MATERIAL_PATH}")

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


def build_dynamic_mesh(spec: AssetSpec, high_detail: bool) -> unreal.DynamicMesh:
    mesh = unreal.DynamicMesh()
    mesh.enable_material_i_ds()
    spec.builder(mesh, high_detail)
    if mesh.is_empty():
        raise RuntimeError(f"Builder returned an empty mesh for {spec.display_name}")
    return mesh


def create_static_mesh(spec: AssetSpec, surface_material: unreal.Material) -> unreal.StaticMesh:
    if unreal.EditorAssetLibrary.does_asset_exist(spec.asset_path):
        if not unreal.EditorAssetLibrary.delete_asset(spec.asset_path):
            raise RuntimeError(f"Could not replace generated asset {spec.asset_path}")

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
        asset.add_material(surface_material)
    for material_index in range(4):
        asset.set_material(material_index, surface_material)

    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Creator", "Angelis Pseftis")
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Faction", spec.faction)
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Role", spec.role)
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Provenance", "Original scripted Unreal geometry")
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.Status", "Vertical-slice art candidate")
    unreal.EditorAssetLibrary.set_metadata_tag(asset, "Echoes.RuntimeAuthority", "Presentation only")
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
    unreal.log("[ECHOES_ART_BEGIN] generating 16 roster assets and 4 Future Well assets")
    for spec in ASSETS:
        if unreal.EditorAssetLibrary.does_asset_exist(spec.asset_path):
            if not unreal.EditorAssetLibrary.delete_asset(spec.asset_path):
                raise RuntimeError(f"Could not prepare generated asset path {spec.asset_path}")
    surface_material = create_surface_material()
    generated = [create_static_mesh(spec, surface_material) for spec in ASSETS]
    unreal.EditorAssetLibrary.save_loaded_assets(generated + [surface_material], False)
    unreal.log(
        f"[ECHOES_ART_COMPLETE] generated={len(generated)} "
        f"roster=16 landmarks=4 material={MATERIAL_PATH}"
    )


if __name__ == "__main__":
    main()
