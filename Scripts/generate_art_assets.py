"""Generate the authored Echoes roster, world, and presentation mesh set.

Run this script only through Scripts/generate_art_assets.sh.  The generated
assets are ordinary StaticMesh and Material assets; Geometry Scripting and
Python are editor-time dependencies, not runtime authority.
"""

from __future__ import annotations

import math
import os
import sys
from dataclasses import dataclass
from typing import Callable, Sequence

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import echoes_texture_synth as texture_synth


ART_ROOT = "/Game/Art/Generated"
TEXTURE_ROOT = f"{ART_ROOT}/Textures"
SURFACE_TEXTURED_REVISION = "surface-textured-v7"
MATERIAL_PATH = f"{ART_ROOT}/Materials/M_EchoesSurface"
WORLD_MATERIAL_PATH = f"{ART_ROOT}/Materials/M_EchoesWorldSurface"
WORLD_MATERIAL_ASSET_REVISION = "world-surface-textured-v6"
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
# Glass Scar bank shelf: vitrified plate with strata faces; replaces the v1 shelf whose edge
# spires and strata slabs stood proud of the plate and read as a slab mosaic (gate 50).
GLASS_SCAR_SHELF_ASSET_REVISION = "glass-scar-shelf-vitrified-v2"
# Broken Sun: a fractured stellar sphere (crust plates over a molten core, shards drifting in
# three dimensions) replacing the v1 flattened disc with rings (gate 50).
BROKEN_SUN_SKY_ASSET_REVISION = "broken-sun-sky-fractured-v3"
VFX_ROOT = f"{ART_ROOT}/VFX"
VFX_MATERIAL_PATH = f"{ART_ROOT}/Materials/M_EchoesPresentationVFX"
PRESENTATION_VFX_ASSET_REVISION = "selection-command-vfx-v2"
DESTRUCTION_VFX_ASSET_REVISION = "destruction-vfx-v1"
ROSTER_ASSET_REVISION = "roster-silhouette-v2"

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
    # Upper thigh strut (DARK mechanical chassis)
    box(mesh, (42.0, width, width), (-5.0, y, hip_z - 15.0), DARK, (55.0, 0.0, 0.0))
    # Lower shin plate (LIGHT ceramic armor)
    box(mesh, (46.0, width, width), (foot_x - 8.0, y, 27.0), LIGHT, (-57.0, 0.0, 0.0))
    # Articulated knee armor plate (PRIMARY team accent)
    box(mesh, (16.0, width + 3.0, 14.0), (foot_x - 6.0, y, 38.0), PRIMARY, (-25.0, 0.0, 0.0))
    # Hip pivot core (GLOW power hub)
    sphere(mesh, width * 0.72, (2.0, y, hip_z), GLOW, high_detail=False)
    # Foot ground clamp (DARK) with front toe plate (LIGHT)
    box(mesh, (32.0, width + 8.0, 10.0), (foot_x, y, 7.0), DARK)
    box(mesh, (14.0, width + 6.0, 6.0), (foot_x + 12.0, y, 6.0), LIGHT)


def meridian_surveyor(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Surveyor: compact engineering exoframe with twin tool arms and sensor mast."""
    # 1. Main Torso & Cockpit Chassis
    box(mesh, (58.0, 48.0, 36.0), (0.0, 0.0, 78.0), LIGHT)
    box(mesh, (48.0, 38.0, 24.0), (-7.0, 0.0, 104.0), DARK, (-8.0, 0.0, 0.0))
    box(mesh, (36.0, 42.0, 16.0), (4.0, 0.0, 102.0), PRIMARY, (-12.0, 0.0, 0.0))
    box(mesh, (10.0, 34.0, 14.0), (27.0, 0.0, 95.0), GLOW)

    # 2. Bipedal Locomotion
    paired_leg(mesh, -21.0, 69.0, 18.0)
    paired_leg(mesh, 21.0, 69.0, 18.0)

    # 3. Articulated Manipulator Arms (Right: Rotary Drill, Left: Pincer)
    # Right arm: Rotary Matter Drill
    box(mesh, (43.0, 10.0, 10.0), (15.0, 39.0, 79.0), DARK, (0.0, 8.0, 15.0))
    box(mesh, (38.0, 8.0, 8.0), (46.0, 48.0, 62.0), LIGHT, (0.0, 8.0, 30.0))
    cone(mesh, 10.0, 2.0, 28.0, (68.0, 52.0, 54.0), GLOW, (90.0, 0.0, 0.0), 6)
    cylinder(mesh, 10.0, 12.0, (55.0, 50.0, 58.0), PRIMARY, (90.0, 0.0, 0.0), 8)
    # Left arm: Heavy Pincer Claw
    box(mesh, (43.0, 10.0, 10.0), (15.0, -39.0, 79.0), DARK, (0.0, -8.0, -15.0))
    box(mesh, (38.0, 8.0, 8.0), (46.0, -48.0, 62.0), LIGHT, (0.0, -8.0, -30.0))
    box(mesh, (18.0, 4.0, 8.0), (66.0, -44.0, 56.0), DARK, (0.0, 15.0, -20.0))
    box(mesh, (18.0, 4.0, 8.0), (66.0, -52.0, 56.0), DARK, (0.0, -15.0, 20.0))
    sphere(mesh, 5.0, (56.0, -48.0, 58.0), GLOW, high_detail=False)

    # 4. Rear Matter Harvest Canisters
    for side in (-1.0, 1.0):
        cylinder(mesh, 11.0, 34.0, (-28.0, side * 18.0, 90.0), DARK, sides=8)
        cylinder(mesh, 7.0, 26.0, (-28.0, side * 18.0, 90.0), GLOW, sides=8)
        box(mesh, (8.0, 16.0, 10.0), (-24.0, side * 18.0, 104.0), PRIMARY)

    # 5. Fold-Out Survey Mast & Prismatic Optics
    box(mesh, (8.0, 8.0, 55.0), (-18.0, 0.0, 137.0), DARK)
    box(mesh, (15.0, 8.0, 28.0), (-18.0, 0.0, 168.0), GLOW)
    if high:
        box(mesh, (32.0, 6.0, 6.0), (-3.0, 0.0, 153.0), LIGHT)
        sphere(mesh, 7.0, (13.0, 0.0, 153.0), GLOW, high_detail=False)
        box(mesh, (10.0, 10.0, 6.0), (-3.0, 0.0, 153.0), PRIMARY)


def meridian_lancer(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Lancer: tall line infantry marksman with precision rail-lance."""
    # 1. Bipedal Chassis
    paired_leg(mesh, -15.0, 72.0, 10.0)
    paired_leg(mesh, 15.0, 72.0, 10.0)

    # 2. Torso Cuirass & Pauldrons
    box(mesh, (42.0, 38.0, 52.0), (-2.0, 0.0, 98.0), LIGHT)
    box(mesh, (28.0, 32.0, 18.0), (8.0, 0.0, 106.0), PRIMARY)
    box(mesh, (34.0, 30.0, 20.0), (2.0, 0.0, 132.0), DARK)
    box(mesh, (12.0, 24.0, 10.0), (20.0, 0.0, 133.0), GLOW)
    box(mesh, (4.0, 4.0, 22.0), (-6.0, 12.0, 146.0), DARK)

    # 3. Shoulder Pauldrons
    for side in (-1.0, 1.0):
        sphere(mesh, 16.0, (-1.0, side * 26.0, 112.0), LIGHT, scale=(1.05, 0.75, 0.8), high_detail=False)
        box(mesh, (16.0, 14.0, 8.0), (-1.0, side * 27.0, 118.0), PRIMARY)
        box(mesh, (35.0, 9.0, 9.0), (15.0, side * 31.0, 94.0), DARK, (0.0, 0.0, side * 35.0))

    # 4. Precision Rail-Lance Rifle
    box(mesh, (116.0, 12.0, 14.0), (56.0, -6.0, 90.0), DARK, (0.0, 0.0, -7.0))
    box(mesh, (92.0, 7.0, 7.0), (70.0, -6.0, 93.0), GLOW, (0.0, 0.0, -7.0))
    for rx in (42.0, 68.0, 94.0):
        box(mesh, (12.0, 16.0, 18.0), (rx, -6.0, 91.0), LIGHT, (0.0, 0.0, -7.0))
        box(mesh, (6.0, 18.0, 20.0), (rx, -6.0, 91.0), PRIMARY, (0.0, 0.0, -7.0))
    cone(mesh, 10.0, 3.0, 38.0, (124.0, -6.0, 81.0), LIGHT, (0.0, 90.0, 0.0), 6)
    sphere(mesh, 4.0, (144.0, -6.0, 81.0), GLOW, high_detail=False)

    # 5. Backpack Capacitor
    box(mesh, (16.0, 22.0, 32.0), (-24.0, 0.0, 106.0), DARK)
    cylinder(mesh, 6.0, 26.0, (-24.0, 0.0, 106.0), GLOW, (90.0, 0.0, 0.0), 8)
    if high:
        box(mesh, (8.0, 5.0, 36.0), (-18.0, -18.0, 125.0), GLOW)
        box(mesh, (28.0, 8.0, 6.0), (18.0, 10.0, 112.0), LIGHT)


def meridian_bulwark(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Bulwark: heavy two-operator mobile defensive screen platform with hexagonal energy barrier."""
    # 1. Broad Heavy Undercarriage
    box(mesh, (78.0, 96.0, 30.0), (-12.0, 0.0, 58.0), DARK)
    box(mesh, (52.0, 72.0, 20.0), (-22.0, 0.0, 74.0), PRIMARY)
    for side in (-1.0, 1.0):
        paired_leg(mesh, side * 34.0, 55.0, 4.0, heavy=True)
        box(mesh, (40.0, 34.0, 46.0), (-5.0, side * 26.0, 88.0), LIGHT)
        box(mesh, (14.0, 20.0, 10.0), (18.0, side * 26.0, 104.0), GLOW)
        box(mesh, (20.0, 12.0, 28.0), (-5.0, side * 44.0, 88.0), PRIMARY)

    # 2. Central Heavy Shield Core Emitter & Articulated Pivot
    cylinder(mesh, 24.0, 86.0, (26.0, 0.0, 72.0), DARK, (90.0, 0.0, 0.0), 10)
    torus(mesh, 28.0, 5.0, (24.0, 0.0, 72.0), GLOW, (90.0, 0.0, 0.0), high)
    cylinder(mesh, 14.0, 22.0, (46.0, 0.0, 72.0), LIGHT, (0.0, 90.0, 0.0), 8)
    sphere(mesh, 10.0, (56.0, 0.0, 72.0), GLOW, high_detail=high)

    # 3. Hexagonal Holographic Energy Shield Barrier Wings (Matching Concept Target)
    # Left and Right curved wings, each hosting 3 interconnected hexagonal energy cells
    for side in (-1.0, 1.0):
        # Structural Outrigger Arms
        box(mesh, (38.0, 12.0, 12.0), (32.0, side * 38.0, 72.0), DARK, (0.0, 0.0, side * 15.0))
        box(mesh, (28.0, 8.0, 8.0), (38.0, side * 48.0, 72.0), LIGHT, (0.0, 0.0, side * 15.0))

        # 3 Hexagonal Barrier Cells per wing (Center, Upper, Outer)
        cell_specs = (
            (56.0, side * 34.0, 72.0, side * 8.0, 29.0),
            (56.0, side * 34.0, 114.0, side * 8.0, 26.0),
            (50.0, side * 64.0, 92.0, side * 20.0, 25.0),
        )
        for cx, cy, cz, cyaw, radius in cell_specs:
            # Outer Beveled Frame (Dark & Ceramic)
            cylinder(mesh, radius + 4.0, 7.0, (cx, cy, cz), DARK, (0.0, 90.0, cyaw), sides=6)
            cylinder(mesh, radius + 1.0, 5.0, (cx + 1.0, cy, cz), LIGHT, (0.0, 90.0, cyaw), sides=6)
            # Radiant Cyan Holographic Energy Cell
            cylinder(mesh, radius - 2.0, 9.0, (cx + 2.0, cy, cz), GLOW, (0.0, 90.0, cyaw), sides=6)
            # Inner Concentric Honeycomb Ring
            if high:
                cylinder(mesh, radius * 0.55, 11.0, (cx + 2.5, cy, cz), GLOW, (0.0, 90.0, cyaw), sides=6)

    # 4. Outrigger Projector Pylons & Upper Sensors
    if high:
        for side in (-1.0, 1.0):
            box(mesh, (42.0, 8.0, 8.0), (35.0, side * 50.0, 44.0), GLOW)
            cone(mesh, 9.0, 2.0, 32.0, (56.0, side * 52.0, 126.0), PRIMARY, (0.0, 90.0, 0.0), 6)
            cylinder(mesh, 8.0, 35.0, (-38.0, side * 40.0, 42.0), DARK, (25.0, 0.0, 0.0), 8)


def meridian_relay_skiff(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Relay Skiff: fast triangular hovering reconnaissance craft with antenna halo."""
    # 1. Main Lifting Fuselage
    box(mesh, (92.0, 36.0, 24.0), (0.0, 0.0, 54.0), DARK, (-5.0, 0.0, 0.0))
    cone(mesh, 30.0, 4.0, 96.0, (28.0, 0.0, 55.0), LIGHT, (0.0, 90.0, 0.0), 6)
    sphere(mesh, 25.0, (-5.0, 0.0, 67.0), GLOW, scale=(1.35, 0.72, 0.62), high_detail=high)
    box(mesh, (45.0, 28.0, 8.0), (-12.0, 0.0, 68.0), PRIMARY)

    # 2. Swept Delta Winglets with Beveled Chines
    for side in (-1.0, 1.0):
        box(mesh, (86.0, 44.0, 8.0), (-5.0, side * 44.0, 54.0), LIGHT, (0.0, side * 18.0, side * 4.0))
        box(mesh, (58.0, 6.0, 6.0), (6.0, side * 50.0, 58.0), GLOW, (0.0, side * 18.0, side * 4.0))
        box(mesh, (32.0, 14.0, 6.0), (-18.0, side * 52.0, 56.0), PRIMARY, (0.0, side * 18.0, side * 4.0))
        cylinder(mesh, 9.0, 22.0, (-30.0, side * 28.0, 42.0), DARK, sides=8)
        cylinder(mesh, 6.0, 8.0, (-30.0, side * 28.0, 30.0), GLOW, sides=8)
        cylinder(mesh, 7.0, 6.0, (-42.0, side * 28.0, 42.0), GLOW, (90.0, 0.0, 0.0), 8)

    # 3. Concentric Orbital Sensor Halo
    torus(mesh, 42.0, 3.0, (-18.0, 0.0, 94.0), GLOW, high_detail=high)
    torus(mesh, 30.0, 2.0, (-18.0, 0.0, 94.0), PRIMARY, high_detail=high)
    box(mesh, (6.0, 6.0, 42.0), (-18.0, 0.0, 90.0), DARK)
    if high:
        sphere(mesh, 8.0, (18.0, 0.0, 40.0), GLOW, high_detail=False)
        cylinder(mesh, 10.0, 28.0, (-48.0, 0.0, 55.0), LIGHT, (90.0, 0.0, 0.0), 8)


def meridian_anchor(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Anchor: hexagonal headquarters and network root with civic command bastions."""
    # Stepped Hexagonal Foundation
    cylinder(mesh, 158.0, 30.0, (0.0, 0.0, 15.0), DARK, sides=6)
    cylinder(mesh, 132.0, 44.0, (0.0, 0.0, 44.0), LIGHT, sides=12)
    cylinder(mesh, 82.0, 66.0, (0.0, 0.0, 84.0), DARK, sides=8)
    cylinder(mesh, 90.0, 14.0, (0.0, 0.0, 68.0), PRIMARY, sides=6)

    # 6 Radial Conduit Arms & Perimeter Bastions
    for angle in range(0, 360, 60):
        radial_box(mesh, angle, 146.0, (108.0, 50.0, 40.0), 44.0, LIGHT)
        radial_box(mesh, angle, 150.0, (80.0, 8.0, 10.0), 72.0, GLOW)
        radial_box(mesh, angle, 160.0, (30.0, 24.0, 16.0), 52.0, PRIMARY)
        if high:
            radial_box(mesh, angle, 202.0, (36.0, 36.0, 20.0), 15.0, DARK)

    # Central Command Beacon Tower
    cylinder(mesh, 42.0, 116.0, (0.0, 0.0, 134.0), LIGHT, sides=8)
    torus(mesh, 54.0, 6.0, (0.0, 0.0, 118.0), GLOW, high_detail=high)
    torus(mesh, 46.0, 4.0, (0.0, 0.0, 150.0), PRIMARY, high_detail=high)
    cone(mesh, 34.0, 3.0, 94.0, (0.0, 0.0, 230.0), GLOW, sides=8)
    if high:
        for angle in range(30, 360, 60):
            radial_box(mesh, angle, 96.0, (44.0, 14.0, 40.0), 92.0, DARK, -25.0)


def meridian_power_link(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Power Link: supply and network node with layered power rings."""
    # Foundation
    cylinder(mesh, 76.0, 26.0, (0.0, 0.0, 13.0), DARK, sides=8)
    cylinder(mesh, 52.0, 134.0, (0.0, 0.0, 82.0), LIGHT, sides=8)
    cylinder(mesh, 56.0, 16.0, (0.0, 0.0, 48.0), PRIMARY, sides=8)
    cylinder(mesh, 30.0, 158.0, (0.0, 0.0, 149.0), DARK, sides=10)

    # Concentric Power Rings at 3 Elevations
    for z, radius in ((56.0, 62.0), (120.0, 54.0), (186.0, 44.0)):
        torus(mesh, radius, 5.0, (0.0, 0.0, z), GLOW, high_detail=high)
        torus(mesh, radius - 8.0, 2.5, (0.0, 0.0, z), PRIMARY, high_detail=high)

    # 4 Radial Ground Busbars & Transformers
    for angle in range(45, 360, 90):
        radial_box(mesh, angle, 102.0, (80.0, 14.0, 12.0), 23.0, DARK)
        radial_box(mesh, angle, 142.0, (26.0, 26.0, 24.0), 12.0, LIGHT)
        radial_box(mesh, angle, 120.0, (20.0, 8.0, 14.0), 32.0, GLOW)

    # Crown Emitter Shroud
    cone(mesh, 30.0, 2.0, 64.0, (0.0, 0.0, 258.0), GLOW, sides=8)
    if high:
        for angle in range(0, 360, 90):
            radial_box(mesh, angle, 44.0, (8.0, 8.0, 94.0), 148.0, LIGHT)


def meridian_array_foundry(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Array Foundry: unit production hall with visible gantry cranes and fabrication laser rails."""
    # Foundation & Assembly Floor
    box(mesh, (336.0, 236.0, 30.0), (0.0, 0.0, 15.0), DARK)
    box(mesh, (280.0, 154.0, 70.0), (-12.0, 0.0, 64.0), LIGHT)
    box(mesh, (244.0, 74.0, 60.0), (10.0, 0.0, 120.0), DARK)
    # Dual Central Assembly Slipways
    box(mesh, (250.0, 14.0, 14.0), (10.0, 20.0, 152.0), GLOW)
    box(mesh, (250.0, 14.0, 14.0), (10.0, -20.0, 152.0), GLOW)
    box(mesh, (260.0, 22.0, 10.0), (10.0, 0.0, 150.0), PRIMARY)

    # 4 Transverse Gantry Arches
    for x in (-110.0, -40.0, 40.0, 110.0):
        box(mesh, (20.0, 210.0, 104.0), (x, 0.0, 96.0), DARK)
        box(mesh, (10.0, 180.0, 10.0), (x, 0.0, 150.0), GLOW)
        box(mesh, (22.0, 30.0, 24.0), (x, 90.0, 120.0), PRIMARY)
        box(mesh, (22.0, 30.0, 24.0), (x, -90.0, 120.0), PRIMARY)

    # Loading Ramp
    box(mesh, (50.0, 110.0, 12.0), (162.0, 0.0, 19.0), LIGHT, (0.0, 0.0, -10.0))
    if high:
        for side in (-1.0, 1.0):
            for x in (-110.0, -38.0, 38.0, 110.0):
                cylinder(mesh, 10.0, 34.0, (x, side * 94.0, 56.0), GLOW, sides=8)
        torus(mesh, 44.0, 4.5, (-122.0, 0.0, 118.0), GLOW, (90.0, 0.0, 0.0))


def meridian_aegis_post(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Meridian Aegis Post: network-powered defense turret with twin rail-barrels and ground cleats."""
    # Ground Anchor Cleats
    cylinder(mesh, 80.0, 30.0, (0.0, 0.0, 15.0), DARK, sides=8)
    for angle in range(45, 360, 90):
        radial_box(mesh, angle, 86.0, (70.0, 30.0, 28.0), 23.0, LIGHT, -8.0)
        radial_box(mesh, angle, 92.0, (40.0, 14.0, 14.0), 30.0, PRIMARY)

    # Turret Pedestal
    cylinder(mesh, 52.0, 80.0, (0.0, 0.0, 65.0), LIGHT, sides=10)
    torus(mesh, 56.0, 6.0, (0.0, 0.0, 94.0), GLOW, high_detail=high)
    cylinder(mesh, 41.0, 66.0, (0.0, 0.0, 126.0), DARK, sides=10)

    # Armored Rotating Turret Housing
    box(mesh, (148.0, 38.0, 40.0), (48.0, 0.0, 155.0), LIGHT, (0.0, 0.0, 4.0))
    box(mesh, (60.0, 42.0, 30.0), (10.0, 0.0, 156.0), PRIMARY, (0.0, 0.0, 4.0))
    # Twin Rail-Cannon Barrels
    for side in (-1.0, 1.0):
        box(mesh, (126.0, 10.0, 10.0), (78.0, side * 12.0, 158.0), GLOW, (0.0, 0.0, 4.0))
        cone(mesh, 14.0, 4.0, 60.0, (148.0, side * 12.0, 164.0), LIGHT, (0.0, 90.0, 0.0), 6)
        sphere(mesh, 5.0, (180.0, side * 12.0, 166.0), GLOW, high_detail=False)
    if high:
        torus(mesh, 28.0, 3.5, (12.0, 0.0, 156.0), GLOW, (90.0, 0.0, 0.0))
        box(mesh, (10.0, 10.0, 84.0), (-45.0, 0.0, 144.0), LIGHT)


def kharuun_tender(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Tender: cultivator four-limbed worker with seed baskets and resonant cutters."""
    # 1. Main Mineral Carapace & Core
    sphere(mesh, 38.0, (0.0, 0.0, 78.0), DARK, scale=(1.25, 0.95, 1.1), high_detail=high)
    cone(mesh, 30.0, 8.0, 56.0, (10.0, 0.0, 108.0), LIGHT, (-18.0, 0.0, 0.0), 7)
    box(mesh, (10.0, 32.0, 10.0), (20.0, 0.0, 102.0), GLOW, (0.0, 0.0, -18.0))
    sphere(mesh, 18.0, (-12.0, 0.0, 88.0), PRIMARY, scale=(1.1, 0.85, 0.8), high_detail=high)

    # 2. Four Articulated Walking Limbs
    for angle in (45.0, 135.0, 225.0, 315.0):
        a = math.radians(angle)
        cone(mesh, 14.0, 6.0, 64.0, (math.cos(a) * 35.0, math.sin(a) * 35.0, 40.0), LIGHT, (55.0, angle, 0.0), 6)
        sphere(mesh, 11.0, (math.cos(a) * 56.0, math.sin(a) * 56.0, 13.0), DARK, scale=(1.4, 0.8, 0.5), high_detail=False)
        sphere(mesh, 6.0, (math.cos(a) * 24.0, math.sin(a) * 24.0, 60.0), PRIMARY, high_detail=False)

    # 3. Forward Cultivator Tendrils
    for side in (-1.0, 1.0):
        cylinder(mesh, 6.0, 74.0, (32.0, side * 24.0, 61.0), DARK, (64.0, 0.0, 0.0), 6)
        cone(mesh, 10.0, 2.0, 32.0, (64.0, side * 24.0, 30.0), GLOW, (78.0, 0.0, 0.0), 6)

    # 4. Flank Seed-Crystal Panniers
    for y in (-26.0, 26.0):
        box(mesh, (28.0, 12.0, 18.0), (-16.0, y, 78.0), LIGHT)
        cone(mesh, 8.0, 2.0, 22.0, (-14.0, y, 92.0), GLOW, (0.0, 0.0, 0.0), 5)
    if high:
        for y in (-19.0, 0.0, 19.0):
            sphere(mesh, 11.0, (-36.0, y, 86.0), GLOW, scale=(0.7, 0.7, 1.25), high_detail=False)


def kharuun_riftstalker(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Riftstalker: sleek predatory quadruped skirmisher with dorsal resonance rail spine."""
    # 1. Sleek Predator Basalt Body
    sphere(mesh, 35.0, (0.0, 0.0, 72.0), DARK, scale=(1.90, 0.72, 0.74), high_detail=high)
    cone(mesh, 30.0, 8.0, 78.0, (36.0, 0.0, 76.0), LIGHT, (0.0, 90.0, 0.0), 7)
    sphere(mesh, 14.0, (62.0, 0.0, 74.0), GLOW, scale=(1.2, 0.6, 0.5), high_detail=False)
    box(mesh, (45.0, 18.0, 14.0), (-8.0, 0.0, 86.0), PRIMARY)

    # 2. Four Reverse-Jointed Pounce Limbs
    for x in (-28.0, 30.0):
        for side in (-1.0, 1.0):
            box(mesh, (56.0, 11.0, 13.0), (x, side * 28.0, 46.0), DARK, (58.0, side * 14.0, side * 12.0))
            cone(mesh, 13.0, 4.0, 52.0, (x + 18.0, side * 44.0, 20.0), LIGHT, (65.0, side * 15.0, 0.0), 6)
            cone(mesh, 7.0, 1.0, 20.0, (x + 36.0, side * 44.0, 8.0), DARK, (0.0, 90.0, 0.0), 5)

    # 3. Dorsal Resonance Rail Canal (Spine of Amber Crystals)
    box(mesh, (120.0, 14.0, 16.0), (12.0, 0.0, 112.0), LIGHT, (0.0, 0.0, 3.0))
    box(mesh, (108.0, 7.0, 7.0), (22.0, 0.0, 115.0), GLOW, (0.0, 0.0, 3.0))
    cone(mesh, 17.0, 2.0, 52.0, (94.0, 0.0, 120.0), GLOW, (0.0, 90.0, 0.0), 6)
    for x in (-32.0, -10.0, 12.0, 34.0, 56.0):
        cone(mesh, 8.0, 1.0, 26.0, (x, 0.0, 122.0), GLOW, (0.0, 0.0, 0.0), 5)
    if high:
        for x in (-28.0, 0.0, 28.0):
            box(mesh, (14.0, 18.0, 6.0), (x, 0.0, 105.0), PRIMARY)


def kharuun_cairnback(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Cairnback: massive six-limbed assault screen with tiered basalt strata and magma seams."""
    # 1. Massive Basalt Fortress Dome & Overlapping Slabs
    sphere(mesh, 60.0, (0.0, 0.0, 66.0), DARK, scale=(1.50, 1.10, 0.74), high_detail=high)
    for x, z, scale_x in ((-50.0, 92.0, 1.0), (-16.0, 106.0, 1.22), (24.0, 103.0, 1.12), (58.0, 89.0, 0.92)):
        sphere(mesh, 35.0, (x, 0.0, z), LIGHT, scale=(scale_x, 1.58, 0.38), high_detail=False)
        box(mesh, (24.0, 68.0, 8.0), (x, 0.0, z + 12.0), PRIMARY)

    # 2. Six Heavy Rooted Pillar Legs
    for x in (-48.0, 0.0, 48.0):
        for side in (-1.0, 1.0):
            box(mesh, (50.0, 16.0, 19.0), (x, side * 52.0, 40.0), DARK, (55.0, side * 10.0, side * 18.0))
            cone(mesh, 17.0, 5.0, 46.0, (x + 8.0, side * 66.0, 18.0), LIGHT, (65.0, side * 10.0, 0.0), 6)
            box(mesh, (28.0, 22.0, 10.0), (x + 8.0, side * 70.0, 6.0), DARK)

    # 3. Thermal Seams & Brow Wedge
    box(mesh, (124.0, 8.0, 8.0), (5.0, 0.0, 100.0), GLOW, (0.0, 0.0, 2.0))
    cone(mesh, 33.0, 8.0, 54.0, (80.0, 0.0, 68.0), PRIMARY, (0.0, 90.0, 0.0), 7)
    box(mesh, (20.0, 36.0, 12.0), (92.0, 0.0, 66.0), DARK)
    if high:
        for x in (-48.0, -16.0, 24.0, 58.0):
            cone(mesh, 10.0, 1.0, 26.0, (x, 0.0, 125.0), GLOW, sides=5)
            box(mesh, (8.0, 48.0, 4.0), (x, 0.0, 96.0), GLOW)


def kharuun_resonant(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Resonant: slender tripod reconnaissance strider with suspended amethyst geode and acoustic fins."""
    # 1. Central Resonant Chamber & Suspended Geode
    cone(mesh, 28.0, 5.0, 94.0, (0.0, 0.0, 104.0), LIGHT, sides=7)
    sphere(mesh, 22.0, (0.0, 0.0, 118.0), GLOW, scale=(0.82, 0.82, 1.25), high_detail=high)
    torus(mesh, 28.0, 4.0, (0.0, 0.0, 118.0), PRIMARY, high_detail=high)

    # 2. Graceful Tripod Limbs with Ground-Contact Feelers
    for angle in (30.0, 150.0, 270.0):
        a = math.radians(angle)
        box(mesh, (84.0, 10.0, 12.0), (math.cos(a) * 32.0, math.sin(a) * 32.0, 62.0), DARK, (63.0, angle, 0.0))
        cone(mesh, 11.0, 2.0, 58.0, (math.cos(a) * 62.0, math.sin(a) * 62.0, 27.0), LIGHT, (75.0, angle, 0.0), 6)
        cylinder(mesh, 3.0, 76.0, (math.cos(a) * 64.0, math.sin(a) * 64.0, 20.0), GLOW, (78.0, angle, 0.0), 5)

    # 3. High-Aspect Acoustic Tuning Fins
    for side in (-1.0, 1.0):
        box(mesh, (12.0, 10.0, 122.0), (-5.0, side * 31.0, 132.0), LIGHT, (side * 17.0, 0.0, side * 8.0))
        box(mesh, (6.0, 6.0, 96.0), (-2.0, side * 26.0, 134.0), GLOW, (side * 17.0, 0.0, side * 8.0))
        box(mesh, (14.0, 8.0, 24.0), (-5.0, side * 31.0, 160.0), PRIMARY, (side * 17.0, 0.0, side * 8.0))
    if high:
        cone(mesh, 12.0, 1.0, 48.0, (0.0, 0.0, 176.0), GLOW, sides=6)


def kharuun_memory_hearth(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Memory Hearth: circular headquarters and adaptation hearth with blooming mineral petals."""
    # Stepped Concentric Basalt Terraces
    cylinder(mesh, 158.0, 32.0, (0.0, 0.0, 16.0), DARK, sides=12)
    torus(mesh, 124.0, 17.0, (0.0, 0.0, 43.0), LIGHT, high_detail=high)
    cylinder(mesh, 84.0, 50.0, (0.0, 0.0, 55.0), PRIMARY, sides=10)

    # 8 Blooming Mineral Petal Buttresses
    for angle in range(0, 360, 45):
        radial_box(mesh, angle, 120.0, (88.0, 33.0, 26.0), 54.0, LIGHT, -15.0)
        radial_box(mesh, angle, 98.0, (53.0, 8.0, 9.0), 76.0, GLOW, 22.0)
        radial_box(mesh, angle, 135.0, (40.0, 20.0, 20.0), 40.0, PRIMARY, -10.0)
        cone(mesh, 22.0, 4.0, 88.0, (math.cos(math.radians(angle)) * 95.0, math.sin(math.radians(angle)) * 95.0, 109.0), DARK, (-12.0, angle, 0.0), 7)

    # Interior Sunken Magma Hearth & Suspended Memory Geode
    sphere(mesh, 45.0, (0.0, 0.0, 98.0), GLOW, scale=(1.0, 1.0, 1.20), high_detail=high)
    torus(mesh, 54.0, 6.0, (0.0, 0.0, 96.0), LIGHT, high_detail=high)
    torus(mesh, 42.0, 4.0, (0.0, 0.0, 96.0), PRIMARY, high_detail=high)
    if high:
        cone(mesh, 30.0, 4.0, 75.0, (0.0, 0.0, 164.0), LIGHT, sides=7)


def kharuun_waystone(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Waystone: mobile supply monolith with 3 folded root-legs and central amber core."""
    cylinder(mesh, 70.0, 26.0, (0.0, 0.0, 13.0), DARK, sides=7)
    # 3 Rooted Mineral Legs with Transformation Hinges
    for angle in (30.0, 150.0, 270.0):
        radial_box(mesh, angle, 76.0, (94.0, 27.0, 26.0), 29.0, DARK, -24.0)
        radial_box(mesh, angle, 90.0, (40.0, 18.0, 18.0), 45.0, PRIMARY, -15.0)
        a = math.radians(angle)
        cone(mesh, 19.0, 5.0, 90.0, (math.cos(a) * 71.0, math.sin(a) * 71.0, 63.0), LIGHT, (42.0, angle, 0.0), 7)

    # Tapering Obelisk with Amber Veins
    cone(mesh, 58.0, 14.0, 192.0, (0.0, 0.0, 118.0), LIGHT, sides=7)
    cone(mesh, 36.0, 5.0, 166.0, (0.0, 0.0, 126.0), DARK, sides=7)
    box(mesh, (12.0, 12.0, 152.0), (18.0, 0.0, 130.0), GLOW, (0.0, 0.0, -5.0))
    sphere(mesh, 25.0, (0.0, 0.0, 114.0), GLOW, scale=(0.9, 0.9, 1.5), high_detail=high)
    if high:
        for angle in (0.0, 120.0, 240.0):
            a = math.radians(angle)
            cone(mesh, 14.0, 2.0, 70.0, (math.cos(a) * 42.0, math.sin(a) * 42.0, 147.0), PRIMARY, (-12.0, angle, 0.0), 6)


def kharuun_growth_basin(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Growth Basin: circular cultivation nursery with 8 gestation alcoves and molten amber pool."""
    # Outer Basalt Wall & Terraced Ring
    cylinder(mesh, 152.0, 28.0, (0.0, 0.0, 14.0), DARK, sides=12)
    torus(mesh, 114.0, 25.0, (0.0, 0.0, 43.0), LIGHT, high_detail=high)
    cylinder(mesh, 72.0, 24.0, (0.0, 0.0, 29.0), GLOW, sides=12)

    # 8 Ordered Cocoon Alcoves
    for angle in range(0, 360, 45):
        a = math.radians(angle)
        sphere(mesh, 26.0, (math.cos(a) * 102.0, math.sin(a) * 102.0, 78.0), DARK, scale=(0.72, 0.72, 1.22), high_detail=high)
        cone(mesh, 22.0, 4.0, 68.0, (math.cos(a) * 124.0, math.sin(a) * 124.0, 85.0), LIGHT, (-18.0, angle, 0.0), 7)
        radial_box(mesh, float(angle), 138.0, (32.0, 20.0, 16.0), 70.0, PRIMARY)
        if high:
            sphere(mesh, 13.0, (math.cos(a) * 100.0, math.sin(a) * 100.0, 81.0), GLOW, scale=(0.7, 0.7, 1.15), high_detail=False)

    torus(mesh, 71.0, 6.0, (0.0, 0.0, 58.0), GLOW, high_detail=high)


def kharuun_listening_spine(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Kharuun Listening Spine: tall seismic sensor obelisk with staggered acoustic resonator plates."""
    # Rooted Bedrock Anchor
    cylinder(mesh, 74.0, 26.0, (0.0, 0.0, 13.0), DARK, sides=9)
    for angle in range(0, 360, 60):
        radial_box(mesh, angle, 78.0, (82.0, 23.0, 20.0), 21.0, LIGHT, -15.0)
        radial_box(mesh, angle, 90.0, (30.0, 15.0, 14.0), 26.0, PRIMARY)

    # Tapering Obelisk Body
    cone(mesh, 50.0, 10.0, 236.0, (0.0, 0.0, 130.0), DARK, sides=7)
    cone(mesh, 28.0, 3.0, 202.0, (0.0, 0.0, 146.0), LIGHT, sides=7)
    box(mesh, (10.0, 10.0, 210.0), (12.0, 0.0, 140.0), GLOW, (0.0, 0.0, -3.0))

    # 4 Staggered Acoustic Resonator Flanges
    for index, z in enumerate((90.0, 128.0, 166.0, 204.0)):
        length = 92.0 - index * 10.0
        side = -1.0 if index % 2 else 1.0
        box(mesh, (length, 14.0, 10.0), (side * length * 0.38, 0.0, z), LIGHT, (0.0, 0.0, side * 24.0))
        box(mesh, (length * 0.74, 6.0, 5.0), (side * length * 0.38, 0.0, z + 5.0), GLOW, (0.0, 0.0, side * 24.0))
        box(mesh, (24.0, 16.0, 12.0), (side * length * 0.72, 0.0, z + 8.0), PRIMARY, (0.0, 0.0, side * 24.0))
    if high:
        torus(mesh, 48.0, 4.5, (0.0, 0.0, 72.0), GLOW)
        cone(mesh, 18.0, 1.0, 82.0, (0.0, 0.0, 282.0), LIGHT, sides=6)


def choir_threadkeeper(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Threadkeeper: coherence worker holding a suspended light field between opposing gyroscope rings."""
    # 1. Obsidian Footing & Frame
    cylinder(mesh, 21.0, 68.0, (0.0, 0.0, 74.0), DARK, sides=10)
    sphere(mesh, 25.0, (0.0, 0.0, 112.0), PRIMARY, scale=(0.78, 0.78, 1.12), high_detail=high)
    torus(mesh, 38.0, 4.5, (0.0, 0.0, 104.0), LIGHT, (20.0, 0.0, 0.0), high)
    torus(mesh, 32.0, 3.5, (0.0, 0.0, 104.0), GLOW, (-20.0, 90.0, 0.0), high)

    # 2. Tripod Probe Legs
    for angle in (30.0, 150.0, 270.0):
        a = math.radians(angle)
        box(mesh, (50.0, 10.0, 10.0), (math.cos(a) * 28.0, math.sin(a) * 28.0, 42.0), DARK, (58.0, angle, 0.0))
        cone(mesh, 10.0, 2.0, 44.0, (math.cos(a) * 48.0, math.sin(a) * 48.0, 18.0), LIGHT, (72.0, angle, 0.0), 6)
        sphere(mesh, 4.0, (math.cos(a) * 58.0, math.sin(a) * 58.0, 5.0), GLOW, high_detail=False)

    # 3. Forward Phase Probes
    box(mesh, (64.0, 7.0, 7.0), (36.0, 0.0, 70.0), GLOW, (0.0, 0.0, 12.0))
    cone(mesh, 6.0, 1.0, 28.0, (68.0, 0.0, 76.0), PRIMARY, (0.0, 90.0, 0.0), 6)
    if high:
        for z in (62.0, 84.0, 128.0):
            sphere(mesh, 6.0, (0.0, 0.0, z), GLOW, high_detail=False)


def choir_intervalist(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Intervalist: phase skirmisher with dual offset combat silhouettes and phase bridge."""
    # Dual offset dart hulls
    for side in (-1.0, 1.0):
        sphere(mesh, 26.0, (-5.0, side * 20.0, 82.0), DARK, scale=(1.28, 0.64, 1.0), high_detail=high)
        cone(mesh, 18.0, 5.0, 60.0, (4.0, side * 21.0, 116.0), LIGHT, (-9.0, 0.0, 0.0), 7)
        box(mesh, (54.0, 9.0, 11.0), (8.0, side * 35.0, 48.0), PRIMARY, (61.0, side * 10.0, side * 8.0))
        cone(mesh, 10.0, 2.0, 45.0, (26.0, side * 46.0, 19.0), GLOW, (73.0, side * 12.0, 0.0), 6)
        box(mesh, (32.0, 4.0, 18.0), (-16.0, side * 28.0, 96.0), PRIMARY, (0.0, 0.0, side * 25.0))

    # Central Phase Bridge & Twin Lances
    torus(mesh, 44.0, 4.5, (0.0, 0.0, 92.0), GLOW, (90.0, 0.0, 0.0), high)
    box(mesh, (122.0, 12.0, 13.0), (45.0, 0.0, 101.0), LIGHT, (0.0, 0.0, 5.0))
    box(mesh, (100.0, 6.0, 6.0), (50.0, 0.0, 105.0), GLOW, (0.0, 0.0, 5.0))
    cone(mesh, 12.0, 2.0, 48.0, (112.0, 0.0, 111.0), PRIMARY, (0.0, 90.0, 0.0), 6)
    if high:
        cone(mesh, 8.0, 1.0, 36.0, (-22.0, 0.0, 132.0), GLOW, (0.0, -90.0, 0.0), 5)


def choir_lacuna_warden(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Lacuna Warden: broad controller shell wrapped around a deliberate central void."""
    # Outer toroidal hull surrounding empty lacuna
    torus(mesh, 72.0, 22.0, (0.0, 0.0, 74.0), DARK, (90.0, 0.0, 0.0), high)
    torus(mesh, 54.0, 8.0, (0.0, 0.0, 74.0), GLOW, (90.0, 0.0, 0.0), high)
    torus(mesh, 80.0, 5.0, (0.0, 0.0, 74.0), PRIMARY, (90.0, 0.0, 0.0), high)

    # Sweeping lateral horn outriggers
    for side in (-1.0, 1.0):
        sphere(mesh, 40.0, (-9.0, side * 50.0, 76.0), PRIMARY, scale=(1.25, 0.72, 0.82), high_detail=high)
        box(mesh, (70.0, 19.0, 21.0), (-16.0, side * 68.0, 43.0), DARK, (52.0, side * 12.0, side * 12.0))
        cone(mesh, 18.0, 5.0, 54.0, (2.0, side * 83.0, 18.0), LIGHT, (69.0, side * 12.0, 0.0), 7)
        box(mesh, (75.0, 15.0, 17.0), (32.0, side * 36.0, 105.0), LIGHT, (0.0, side * 9.0, side * 8.0))
        box(mesh, (45.0, 8.0, 8.0), (45.0, side * 42.0, 108.0), GLOW, (0.0, side * 9.0, side * 8.0))

    # Orbiting lacuna shards (suspended inside the void)
    sphere(mesh, 22.0, (0.0, 0.0, 75.0), GLOW, scale=(0.72, 0.72, 1.35), high_detail=high)
    if high:
        for angle in range(0, 360, 45):
            radial_box(mesh, float(angle), 82.0, (26.0, 6.0, 6.0), 92.0, GLOW)


def choir_afterimage(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Afterimage: fast support craft with displaced luminous second hull."""
    # Dual offset phase hulls
    for offset, material in ((-15.0, DARK), (15.0, LIGHT)):
        cone(mesh, 25.0, 3.0, 108.0, (16.0 + offset, offset * 0.65, 58.0), material, (0.0, 90.0, 0.0), 7)
        box(mesh, (76.0, 25.0, 8.0), (-3.0 + offset, offset * 0.65, 52.0), material, (0.0, offset * 0.35, 0.0))
    sphere(mesh, 24.0, (0.0, 0.0, 66.0), PRIMARY, scale=(1.35, 0.70, 0.65), high_detail=high)
    # Dual interlocking phase rings
    torus(mesh, 44.0, 3.5, (-15.0, 0.0, 78.0), GLOW, (20.0, 0.0, 0.0), high)
    torus(mesh, 35.0, 3.0, (15.0, 0.0, 78.0), GLOW, (-20.0, 0.0, 0.0), high)
    for side in (-1.0, 1.0):
        box(mesh, (73.0, 34.0, 7.0), (-11.0, side * 40.0, 54.0), LIGHT, (0.0, side * 14.0, side * 4.0))
        box(mesh, (55.0, 6.0, 5.0), (-3.0, side * 46.0, 58.0), GLOW, (0.0, side * 14.0, side * 4.0))
        box(mesh, (30.0, 16.0, 6.0), (-20.0, side * 42.0, 56.0), PRIMARY, (0.0, side * 14.0, side * 4.0))


def choir_concordance(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Concordance: tri-fold headquarters holding a floating crown around a radiant empty lacuna."""
    cylinder(mesh, 156.0, 28.0, (0.0, 0.0, 14.0), DARK, sides=12)
    torus(mesh, 120.0, 16.0, (0.0, 0.0, 40.0), PRIMARY, high_detail=high)
    torus(mesh, 86.0, 7.0, (0.0, 0.0, 48.0), GLOW, high_detail=high)

    # 3 Soaring Crystal Pylons Arching Inward
    for angle in (30.0, 150.0, 270.0):
        a = math.radians(angle)
        cone(mesh, 36.0, 8.0, 206.0, (math.cos(a) * 93.0, math.sin(a) * 93.0, 128.0), LIGHT, (-10.0, angle, 0.0), 8)
        box(mesh, (12.0, 12.0, 158.0), (math.cos(a) * 86.0, math.sin(a) * 86.0, 134.0), GLOW, (-10.0, angle, 0.0))
        radial_box(mesh, angle, 156.0, (88.0, 36.0, 24.0), 32.0, DARK)
        radial_box(mesh, angle, 120.0, (30.0, 24.0, 30.0), 80.0, PRIMARY)

    # Suspended Floating Crown & Central Lacuna
    sphere(mesh, 38.0, (0.0, 0.0, 118.0), PRIMARY, scale=(0.82, 0.82, 1.18), high_detail=high)
    torus(mesh, 50.0, 5.5, (0.0, 0.0, 118.0), GLOW, (90.0, 0.0, 0.0), high)
    if high:
        torus(mesh, 58.0, 4.5, (0.0, 0.0, 118.0), LIGHT, (0.0, 90.0, 0.0), high)


def choir_interval_loom(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Interval Loom: supply node with interlocking orthogonal gyroscopic phase rings."""
    cylinder(mesh, 74.0, 26.0, (0.0, 0.0, 13.0), DARK, sides=10)
    cylinder(mesh, 44.0, 132.0, (0.0, 0.0, 78.0), PRIMARY, sides=10)
    for tilt, yaw, radius in ((24.0, 0.0, 68.0), (-24.0, 90.0, 55.0)):
        torus(mesh, radius, 6.5, (0.0, 0.0, 115.0), LIGHT, (tilt, yaw, 0.0), high)
    torus(mesh, 40.0, 4.5, (0.0, 0.0, 115.0), GLOW, (90.0, 0.0, 0.0), high)
    for angle in (45.0, 135.0, 225.0, 315.0):
        radial_box(mesh, angle, 91.0, (63.0, 22.0, 20.0), 24.0, DARK, -18.0)
        radial_box(mesh, angle, 112.0, (28.0, 16.0, 24.0), 45.0, PRIMARY)
        a = math.radians(angle)
        cone(mesh, 13.0, 3.0, 63.0, (math.cos(a) * 78.0, math.sin(a) * 78.0, 55.0), LIGHT, (49.0, angle, 0.0), 6)
    sphere(mesh, 19.0, (0.0, 0.0, 115.0), GLOW, high_detail=high)


def choir_chorus_loom(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Chorus Loom: stepped hexagonal amphitheater with alternating crystal tuning towers."""
    cylinder(mesh, 150.0, 30.0, (0.0, 0.0, 15.0), DARK, sides=12)
    torus(mesh, 113.0, 20.0, (0.0, 0.0, 45.0), PRIMARY, high_detail=high)
    cylinder(mesh, 67.0, 36.0, (0.0, 0.0, 40.0), GLOW, sides=12)
    for index, angle in enumerate(range(0, 360, 60)):
        a = math.radians(angle)
        height = 116.0 + (index % 2) * 36.0
        cone(mesh, 24.0, 5.0, height, (math.cos(a) * 103.0, math.sin(a) * 103.0, 58.0 + height * 0.5), LIGHT, (-13.0, angle, 0.0), 7)
        box(mesh, (8.0, 8.0, height * 0.74), (math.cos(a) * 98.0, math.sin(a) * 98.0, 62.0 + height * 0.5), GLOW, (-13.0, angle, 0.0))
        radial_box(mesh, float(angle), 150.0, (78.0, 26.0, 24.0), 30.0, DARK)
        radial_box(mesh, float(angle), 125.0, (28.0, 18.0, 28.0), 60.0, PRIMARY)
    torus(mesh, 75.0, 5.5, (0.0, 0.0, 87.0), GLOW, high_detail=high)
    if high:
        sphere(mesh, 27.0, (0.0, 0.0, 91.0), PRIMARY, scale=(0.78, 0.78, 1.22), high_detail=True)


def choir_phase_anchor(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Choir Phase Anchor: coherence pylon with dual counter-rotating phase containment toruses."""
    cylinder(mesh, 78.0, 26.0, (0.0, 0.0, 13.0), DARK, sides=10)
    for angle in range(0, 360, 90):
        radial_box(mesh, float(angle), 83.0, (76.0, 22.0, 20.0), 20.0, PRIMARY, -14.0)
    cone(mesh, 44.0, 7.0, 220.0, (0.0, 0.0, 128.0), LIGHT, sides=8)
    cone(mesh, 24.0, 2.0, 188.0, (0.0, 0.0, 139.0), DARK, sides=8)
    box(mesh, (9.0, 9.0, 177.0), (13.0, 0.0, 139.0), GLOW, (0.0, 0.0, -4.0))
    torus(mesh, 57.0, 5.5, (0.0, 0.0, 119.0), GLOW, (23.0, 0.0, 0.0), high)
    torus(mesh, 46.0, 4.5, (0.0, 0.0, 119.0), PRIMARY, (-23.0, 90.0, 0.0), high)
    sphere(mesh, 18.0, (0.0, 0.0, 119.0), GLOW, high_detail=high)
    if high:
        cone(mesh, 14.0, 1.0, 74.0, (0.0, 0.0, 272.0), LIGHT, sides=6)


def world_future_well_base(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Neutral stepped stone dais with sunken glowing aperture and low-profile gimbal mounts."""
    # Stepped circular dais tiers rising from the causeway deck
    cylinder(mesh, 280.0, 14.0, (0.0, 0.0, 7.0), DARK, sides=24 if high else 16)
    cylinder(mesh, 235.0, 14.0, (0.0, 0.0, 21.0), PRIMARY, sides=24 if high else 16)
    cylinder(mesh, 185.0, 10.0, (0.0, 0.0, 33.0), LIGHT, sides=20 if high else 14)
    cylinder(mesh, 135.0, 8.0, (0.0, 0.0, 42.0), DARK, sides=18 if high else 12)

    # Sunken glowing well aperture in the center
    cylinder(mesh, 95.0, 26.0, (0.0, 0.0, 31.0), GLOW, sides=16 if high else 12)
    cylinder(mesh, 60.0, 34.0, (0.0, 0.0, 24.0), DARK, sides=12)

    # Radiant energy channels carved into the dais tiers
    torus(mesh, 145.0, 4.5, (0.0, 0.0, 43.0), GLOW, high_detail=high)
    torus(mesh, 205.0, 3.5, (0.0, 0.0, 29.0), GLOW, high_detail=high)

    # Low-profile perimeter gimbal mount pedestals (unobstructed core view)
    for angle in range(0, 360, 60):
        radial_box(mesh, float(angle), 210.0, (75.0, 28.0, 24.0), 30.0, DARK)
        radial_box(mesh, float(angle), 215.0, (55.0, 14.0, 18.0), 42.0, PRIMARY)
        radial_box(mesh, float(angle), 180.0, (65.0, 6.0, 6.0), 40.0, GLOW)
        if high:
            radial_tangent_box(mesh, angle + 14.0, 245.0, (48.0, 16.0, 14.0), 16.0, LIGHT)
            radial_tangent_box(mesh, angle - 14.0, 245.0, (48.0, 16.0, 14.0), 16.0, LIGHT)
            # Radiant runic conduits radiating outward to the bridge edges
            radial_box(mesh, float(angle), 260.0, (60.0, 8.0, 5.0), 18.0, GLOW)


def world_future_well_orbit(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Concentric bronze/stone gimbal rings rotating around the suspended crystal core."""
    # Outer gimbal ring
    torus(mesh, 155.0, 6.5, (0.0, 0.0, 95.0), PRIMARY, (18.0, 24.0, 0.0), high_detail=high)
    # Middle gimbal ring with opposing tilt
    torus(mesh, 120.0, 5.5, (0.0, 0.0, 95.0), LIGHT, (-24.0, 65.0, 0.0), high_detail=high)
    # Inner gimbal ring
    torus(mesh, 88.0, 4.5, (0.0, 0.0, 95.0), PRIMARY, (38.0, -45.0, 0.0), high_detail=high)

    # Radiant energy emitter nodes / alignment brackets along the rings
    for angle in range(0, 360, 45 if high else 90):
        a = math.radians(angle)
        ox = math.cos(a) * 155.0
        oy = math.sin(a) * 155.0
        box(mesh, (14.0, 14.0, 14.0), (ox, oy * 0.91, 95.0 + oy * 0.31), GLOW, (18.0, 24.0, float(angle)))
        if high:
            mx = math.cos(a) * 120.0
            my = math.sin(a) * 120.0
            box(mesh, (10.0, 10.0, 10.0), (mx * 0.91, my, 95.0 - mx * 0.41), GLOW, (-24.0, 65.0, float(angle)))


def world_future_well_core(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Suspended obsidian diamond monolith with glowing amber heart and orbiting satellite shards."""
    sides = 8 if high else 6
    # Upper pyramid / facet
    cone(mesh, 46.0, 2.0, 95.0, (0.0, 0.0, 142.0), DARK, sides=sides)
    # Lower inverted pyramid / facet
    cone(mesh, 2.0, 46.0, 95.0, (0.0, 0.0, 48.0), DARK, sides=sides)

    # Radiant internal energy heart glowing through the crystal junction
    sphere(mesh, 24.0, (0.0, 0.0, 95.0), GLOW, high_detail=high)
    torus(mesh, 47.0, 4.0, (0.0, 0.0, 95.0), GLOW, (0.0, 0.0, 0.0), high_detail=high)
    torus(mesh, 38.0, 3.0, (0.0, 0.0, 95.0), LIGHT, (90.0, 0.0, 0.0), high_detail=high)

    # Orbiting satellite crystal shards suspended in the field
    shard_angles = range(0, 360, 45 if high else 90)
    for index, angle in enumerate(shard_angles):
        a = math.radians(angle)
        radius = 64.0 + (index % 3) * 16.0
        z = 70.0 + (index % 4) * 14.0
        cone(
            mesh,
            11.0,
            2.0,
            48.0,
            (math.cos(a) * radius, math.sin(a) * radius, z),
            DARK if index % 2 == 0 else GLOW,
            (15.0, float(angle + 30.0), 20.0),
            6 if high else 5,
        )


def world_future_well_glyph(mesh: unreal.DynamicMesh, high: bool) -> None:
    for angle in range(0, 360, 45):
        radial_box(mesh, float(angle), 116.0, (185.0, 13.0, 7.0), 0.0, GLOW)
        radial_box(mesh, float(angle), 205.0, (72.0, 5.0, 4.0), 5.0, LIGHT)
        if high:
            radial_tangent_box(mesh, float(angle), 205.0, (38.0, 12.0, 6.0), 3.0, DARK)
    torus(mesh, 91.0, 5.0, (0.0, 0.0, 2.0), GLOW, high_detail=high)


def world_glass_scar_shelf(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Vitrified basalt bank: a flush charcoal plate with hairline fracture relief, sheer strata
    faces on all four sides, and molten amber fissures in the faces and across the surface.

    Nothing stands proud of the plate beyond low relief, so tiled instances read as one ground
    rather than a mosaic of slabs; the silhouette work lives on the cliff faces."""
    # Foundation block descending into the chasm, flush with the plate edges
    box(mesh, (780.0, 780.0, 400.0), (0.0, 0.0, -180.0), DARK)
    # Walking plate, same footprint as the foundation so no rim shows at tile seams
    box(mesh, (780.0, 780.0, 38.0), (0.0, 0.0, 20.0), PRIMARY)

    # Low fracture relief: broad shards of the plate lifted 3 to 5 units, slightly tilted
    for x, y, sx, sy, lift, tilt_roll, tilt_pitch, yaw in (
        (-190.0, -150.0, 300.0, 240.0, 4.0, 0.6, -0.4, 11.0),
        (170.0, 120.0, 260.0, 300.0, 3.0, -0.5, 0.5, -17.0),
        (-40.0, 210.0, 220.0, 170.0, 5.0, 0.4, 0.7, 29.0),
        (210.0, -210.0, 200.0, 180.0, 3.0, -0.6, -0.3, -33.0),
    ):
        box(mesh, (sx, sy, 8.0), (x, y, 36.0 + lift), PRIMARY, (tilt_roll, tilt_pitch, yaw))

    # Surface fissures: amber hairlines set into the plate, a hand proud of it
    for x, y, length, yaw in (
        (-120.0, 40.0, 420.0, 24.0),
        (150.0, -60.0, 360.0, -38.0),
        (40.0, 230.0, 240.0, 71.0),
        (-230.0, -230.0, 200.0, -62.0),
    ):
        box(mesh, (length, 5.0, 6.0), (x, y, 39.0), GLOW, (0.0, 0.0, yaw))
    if high:
        for x, y, length, yaw in (
            (60.0, 110.0, 180.0, 8.0),
            (-260.0, 150.0, 150.0, -80.0),
            (240.0, 240.0, 120.0, 40.0),
        ):
            box(mesh, (length, 4.0, 5.0), (x, y, 39.0), GLOW, (0.0, 0.0, yaw))

    # Strata bands: full-perimeter ledges at several depths so every face reads as layered rock
    for z, thickness, proud, material in (
        (-36.0, 26.0, 10.0, PRIMARY),
        (-104.0, 42.0, 18.0, DARK),
        (-176.0, 28.0, 8.0, PRIMARY),
        (-256.0, 50.0, 22.0, DARK),
        (-338.0, 24.0, 12.0, PRIMARY),
    ):
        box(mesh, (780.0 + 2.0 * proud, 780.0 + 2.0 * proud, thickness), (0.0, 0.0, z), material)

    # Face fissures: molten amber slabs lying in each cliff face, tilted along the face
    face = 390.0
    for x, z, length, tilt in (
        (-140.0, -70.0, 300.0, 22.0),
        (120.0, -150.0, 260.0, -18.0),
        (-60.0, -290.0, 340.0, 12.0),
    ):
        # +/-Y faces: thin in Y, tilted about Y (pitch)
        for side in (-1.0, 1.0):
            box(mesh, (length, 10.0, 16.0), (x, side * face, z), GLOW, (0.0, tilt, 0.0))
        # +/-X faces: thin in X, tilted about X (roll)
        for side in (-1.0, 1.0):
            box(mesh, (10.0, length, 16.0), (side * face, x, z - 30.0), GLOW, (tilt, 0.0, 0.0))

    # Face relief: broken buttresses standing out of the lower faces
    if high:
        for x, z, w, h, proud in (
            (-250.0, -240.0, 120.0, 260.0, 26.0),
            (90.0, -300.0, 160.0, 180.0, 30.0),
            (260.0, -200.0, 100.0, 300.0, 22.0),
        ):
            for side in (-1.0, 1.0):
                box(mesh, (w, proud, h), (x, side * (face + proud * 0.5), z), DARK)
                box(mesh, (proud, w, h), (side * (face + proud * 0.5), -x, z - 20.0), DARK)


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
    """Faceted radiant cyan Matter crystal cluster sprouting dynamically from mineralized host rock."""
    # Mineral host rock base slabs
    cylinder(mesh, 138.0, 26.0, (0.0, 0.0, 13.0), PRIMARY, sides=14 if high else 10)
    cylinder(mesh, 115.0, 16.0, (12.0, -8.0, 24.0), DARK, sides=12 if high else 8)
    torus(mesh, 120.0, 11.0, (0.0, 0.0, 20.0), DARK, high_detail=high)

    # Dynamic multi-faceted crystal spires
    spires = (
        (0.0, 0.0, 48.0, 3.0, 275.0, -4.0, 8.0, 12.0, True),
        (-58.0, 28.0, 34.0, 2.5, 205.0, 16.0, -22.0, -15.0, False),
        (54.0, -35.0, 32.0, 2.5, 185.0, -18.0, 28.0, 20.0, False),
        (46.0, 52.0, 26.0, 2.0, 145.0, 12.0, 48.0, -10.0, False),
        (-52.0, -48.0, 24.0, 2.0, 130.0, -14.0, -44.0, 25.0, False),
        (18.0, -68.0, 22.0, 2.0, 120.0, -22.0, 6.0, -18.0, False),
        (-22.0, 65.0, 21.0, 2.0, 115.0, 20.0, -8.0, 16.0, False),
    )
    for x, y, radius, top_r, height, pitch, yaw, roll, is_core in spires:
        cone(
            mesh,
            radius,
            top_r,
            height,
            (x, y, 22.0 + height * 0.48),
            GLOW if is_core else LIGHT,
            (pitch, yaw, roll),
            8 if high else 6,
        )
        if is_core or high:
            cone(
                mesh,
                radius * 0.55,
                1.0,
                height * 0.88,
                (x, y, 24.0 + height * 0.44),
                GLOW,
                (pitch, yaw, roll),
                6 if high else 5,
            )

    # Radiating basal crystal needle shards
    for angle in range(15, 360, 45 if high else 90):
        radial_box(mesh, float(angle), 105.0, (76.0, 11.0, 9.0), 24.0, GLOW, pitch=-18.0)
        if high:
            radial_box(mesh, float(angle + 22.5), 118.0, (52.0, 8.0, 7.0), 20.0, LIGHT, pitch=-24.0)


def world_broken_sun_sky(mesh: unreal.DynamicMesh, high: bool) -> None:
    """The Broken Sun: a fractured stellar sphere. A molten core shows through the cracks of a
    broken dark crust of tangent plates, and Dawnshards drift away from it in every direction.

    v2 replaces the flattened disc-and-rings of v1: the concept target reads the sun as a
    cracked sphere with no ring, lit from within."""
    core_radius = 1200.0
    # 1. Molten core, seen only through the crust gaps
    sphere(mesh, core_radius, (0.0, 0.0, 0.0), GLOW, high_detail=high)

    # 2. Broken crust: tangent plates on a Fibonacci sphere with gaps between them.
    #    Rotator order is (roll, pitch, yaw); yaw/pitch turn the plate's local X onto the
    #    outward normal, so the plate's thin axis points along the normal.
    plate_count = 76 if high else 48
    golden = math.pi * (3.0 - math.sqrt(5.0))
    for index in range(plate_count):
        z = 1.0 - 2.0 * (index + 0.5) / plate_count
        ring = math.sqrt(max(0.0, 1.0 - z * z))
        theta = golden * index
        normal = (ring * math.cos(theta), ring * math.sin(theta), z)
        yaw = math.degrees(math.atan2(normal[1], normal[0]))
        pitch = math.degrees(math.asin(normal[2]))
        roll = float((index * 29) % 44) - 22.0
        width = 250.0 + float((index * 37) % 210)
        height = 210.0 + float((index * 53) % 200)
        offset = core_radius * 1.02
        at = (normal[0] * offset, normal[1] * offset, normal[2] * offset)
        box(mesh, (70.0, width, height), at, DARK, (roll, pitch, yaw))
        # A hot inner rim on every third plate: the crust is thinnest where it just broke
        if index % 3 == 0:
            box(mesh, (30.0, width * 0.6, height * 0.6), at, LIGHT, (roll, pitch, yaw))

    # 3. Dawnshards drifting away from the sphere in three dimensions: a dark body with
    #    an incandescent inner spike, larger and further out the longer they have drifted.
    shard_count = 16 if high else 10
    for index in range(shard_count):
        z = 1.0 - 2.0 * (index + 0.5) / shard_count
        ring = math.sqrt(max(0.0, 1.0 - z * z))
        theta = golden * index + 1.7
        direction = (ring * math.cos(theta), ring * math.sin(theta), z * 0.55)
        dist = 1750.0 + float((index * 173) % 900)
        at = (direction[0] * dist, direction[1] * dist, direction[2] * dist)
        size = 260.0 + float((index * 61) % 180)
        rotation = (float((index * 31) % 50) - 25.0, float((index * 47) % 40) - 20.0, math.degrees(theta))
        cone(mesh, size * 0.5, size * 0.12, size * 1.5, at, DARK, rotation, 6 if high else 5)
        cone(mesh, size * 0.32, size * 0.05, size * 1.25, (at[0], at[1], at[2] + 30.0), GLOW, rotation, 5)

    # 4. Embers: small glowing fragments far out, high detail only
    if high:
        for index in range(18):
            theta = golden * index * 1.3 + 0.4
            z = math.sin(index * 0.9) * 0.6
            ring = math.sqrt(max(0.0, 1.0 - z * z))
            dist = 2900.0 + float((index * 97) % 500)
            at = (ring * math.cos(theta) * dist, ring * math.sin(theta) * dist, z * dist)
            cone(mesh, 70.0, 12.0, 170.0, at, GLOW, (float(index * 23 % 60), float(index * 41 % 90), 0.0), 5)


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
    """A monumental paved causeway bridge spanning the Glass Scar chasm with an expansive central circular dais, stone roadway deck, and deep vertical piers."""
    # 1. Central Circular Foundation Dais (where Future Well sits)
    cylinder(mesh, 620.0, 50.0, (0.0, 0.0, 25.0), DARK, sides=32 if high else 20)
    cylinder(mesh, 560.0, 24.0, (0.0, 0.0, 42.0), PRIMARY, sides=32 if high else 20)
    cylinder(mesh, 480.0, 14.0, (0.0, 0.0, 52.0), LIGHT, sides=32 if high else 20)
    torus(mesh, 475.0, 5.5, (0.0, 0.0, 58.0), GLOW, high_detail=high)
    torus(mesh, 555.0, 6.0, (0.0, 0.0, 48.0), GLOW, high_detail=high)
    torus(mesh, 615.0, 8.0, (0.0, 0.0, 40.0), DARK, high_detail=high)
    # Circular dais perimeter parapets
    for angle in range(25, 155, 30):
        for flip in (-1.0, 1.0):
            radial_box(mesh, angle * flip + 90.0, 595.0, (55.0, 32.0, 46.0), 55.0, DARK)
            radial_box(mesh, angle * flip + 90.0, 595.0, (35.0, 18.0, 16.0), 76.0, LIGHT)

    # 2. Linear Causeway Roadway Spans (North and South of the Dais)
    for sign in (-1.0, 1.0):
        # Base structural bed
        box(mesh, (540.0, 450.0, 48.0), (0.0, sign * 680.0, 20.0), DARK)
        # Paved roadway deck
        box(mesh, (460.0, 440.0, 24.0), (0.0, sign * 680.0, 46.0), LIGHT)
        box(mesh, (380.0, 420.0, 10.0), (0.0, sign * 680.0, 56.0), PRIMARY)
        # Flanking stone parapets
        for side in (-1.0, 1.0):
            box(mesh, (48.0, 440.0, 64.0), (side * 245.0, sign * 680.0, 56.0), DARK)
            box(mesh, (26.0, 440.0, 18.0), (side * 245.0, sign * 680.0, 88.0), LIGHT)
            box(mesh, (10.0, 440.0, 8.0), (side * 215.0, sign * 680.0, 57.0), GLOW)
        # Paved slab divisions
        for y_offset in (-140.0, 0.0, 140.0):
            box(mesh, (420.0, 12.0, 7.0), (0.0, sign * 680.0 + y_offset, 58.0), DARK)
            box(mesh, (28.0, 120.0, 8.0), (0.0, sign * 680.0 + y_offset, 60.0), GLOW)

    # 3. Massive Vertical Chasm Piers Descending into the Abyss
    # Under central dais: massive circular foundation column
    cylinder(mesh, 440.0, 700.0, (0.0, 0.0, -350.0), DARK, sides=24 if high else 16)
    cylinder(mesh, 360.0, 650.0, (0.0, 0.0, -365.0), PRIMARY, sides=20 if high else 12)
    # Buttress piers under north and south spans
    for sign in (-1.0, 1.0):
        box(mesh, (440.0, 200.0, 680.0), (0.0, sign * 680.0, -340.0), DARK)
        box(mesh, (360.0, 160.0, 640.0), (0.0, sign * 680.0, -350.0), PRIMARY)
        for side in (-1.0, 1.0):
            box(mesh, (80.0, 220.0, 500.0), (side * 240.0, sign * 680.0, -260.0), DARK, (0.0, 0.0, side * 6.0))
            if high:
                box(mesh, (16.0, 180.0, 420.0), (side * 220.0, sign * 680.0, -260.0), GLOW, (0.0, 0.0, side * 6.0))
    # Approach abutments at bridge ends
    for sign in (-1.0, 1.0):
        box(mesh, (500.0, 120.0, 36.0), (0.0, sign * 890.0, 16.0), DARK)


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


def vfx_command_attack(mesh: unreal.DynamicMesh, high: bool) -> None:
    """Direct-target spearhead, distinct from the area attack-move cross."""
    command_sigil_base(mesh, high)
    box(mesh, (58.0, 8.0, 5.0), (-4.0, 0.0, 4.0), PRIMARY)
    box(mesh, (34.0, 8.0, 5.0), (20.0, 11.0, 4.0), PRIMARY, (0.0, 44.0, 0.0))
    box(mesh, (34.0, 8.0, 5.0), (20.0, -11.0, 4.0), PRIMARY, (0.0, -44.0, 0.0))
    if high:
        torus(mesh, 17.0, 2.6, (-17.0, 0.0, 4.0), PRIMARY, high_detail=True)


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
    VfxAssetSpec("CommandAttack", "Direct-attack command sigil", "accepted visible-target attack confirmation", vfx_command_attack),
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
    AssetSpec("Choir", "Units", "Threadkeeper", "Threadkeeper", "worker and coherence tender", choir_threadkeeper),
    AssetSpec("Choir", "Units", "Intervalist", "Intervalist", "phase skirmisher", choir_intervalist),
    AssetSpec("Choir", "Units", "LacunaWarden", "Lacuna Warden", "recovery controller", choir_lacuna_warden),
    AssetSpec("Choir", "Units", "Afterimage", "Afterimage", "misdirection support", choir_afterimage),
    AssetSpec("Choir", "Structures", "Concordance", "Concordance", "headquarters and collective root", choir_concordance),
    AssetSpec("Choir", "Structures", "IntervalLoom", "Interval Loom", "supply and coherence interval", choir_interval_loom),
    AssetSpec("Choir", "Structures", "ChorusLoom", "Chorus Loom", "production and research", choir_chorus_loom),
    AssetSpec("Choir", "Structures", "PhaseAnchor", "Phase Anchor", "coherence structure", choir_phase_anchor),
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
    AssetSpec("World", "Environment", "BrokenSunSky", "Broken Sun celestial sky", "signature fractured sun sky object", world_broken_sun_sky),
    AssetSpec("World", "Resources", "MatterDeposit", "Matter deposit", "neutral gatherable resource landmark", world_matter_deposit),
)


def texture_source_dir() -> str:
    return os.path.join(
        unreal.SystemLibrary.get_project_content_directory(),
        "Art", "Source", "Textures",
    )


def import_surface_textures() -> dict[str, unreal.Texture2D]:
    """Synthesize (byte-idempotent) and import the A3 surface texture maps."""
    source_dir = texture_source_dir()
    os.makedirs(source_dir, exist_ok=True)
    imported: dict[str, unreal.Texture2D] = {}
    for family in texture_synth.FAMILIES:
        maps = texture_synth.render_family(family)
        for suffix, payload in maps.items():
            file_path = os.path.join(source_dir, f"{family}_{suffix}.png")
            if not (
                os.path.exists(file_path)
                and open(file_path, "rb").read() == payload
            ):
                with open(file_path, "wb") as output:
                    output.write(payload)
            asset_name = f"{family}_{suffix}"
            asset_path = f"{TEXTURE_ROOT}/{asset_name}"
            existing = (
                unreal.EditorAssetLibrary.load_asset(asset_path)
                if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
                else None
            )
            if existing is not None and unreal.EditorAssetLibrary.get_metadata_tag(
                existing, "Echoes.AssetRevision"
            ) == texture_synth.REVISION_TEXTURES:
                imported[asset_name] = existing
                unreal.log(
                    f"[ECHOES_ART_TEXTURE] path={asset_path} action=reused"
                )
                continue
            task = unreal.AssetImportTask()
            task.filename = file_path
            task.destination_path = TEXTURE_ROOT
            task.destination_name = asset_name
            task.replace_existing = existing is not None
            task.automated = True
            task.save = False
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            texture = unreal.load_asset(asset_path)
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError(f"Texture import failed: {asset_path}")
            if suffix == "Normal":
                texture.set_editor_property(
                    "compression_settings",
                    unreal.TextureCompressionSettings.TC_NORMALMAP,
                )
                texture.set_editor_property("srgb", False)
                texture.set_editor_property("flip_green_channel", True)
            elif suffix == "MRE":
                texture.set_editor_property(
                    "compression_settings",
                    unreal.TextureCompressionSettings.TC_MASKS,
                )
                texture.set_editor_property("srgb", False)
            for tag, value in (
                ("Echoes.Creator", "Angelis Pseftis"),
                ("Echoes.Provenance", "Original deterministic project synthesis"),
                ("Echoes.RuntimeAuthority", "Presentation only"),
                ("Echoes.AssetRevision", texture_synth.REVISION_TEXTURES),
            ):
                unreal.EditorAssetLibrary.set_metadata_tag(texture, tag, value)
            unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
            imported[asset_name] = texture
            unreal.log(
                f"[ECHOES_ART_TEXTURE] path={asset_path} action=imported"
            )
    unreal.log(
        f"[ECHOES_ART_TEXTURES_READY] families={len(texture_synth.FAMILIES)} "
        f"maps={len(imported)} revision={texture_synth.REVISION_TEXTURES}"
    )
    return imported


def _texture_parameter(
    material: unreal.Material,
    name: str,
    texture: unreal.Texture2D,
    x: int,
    y: int,
    sampler: unreal.MaterialSamplerType,
) -> unreal.MaterialExpressionTextureSampleParameter2D:
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, x, y
    )
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", texture)
    node.set_editor_property("sampler_type", sampler)
    return node


def rebuild_textured_surface_master(
    material: unreal.Material, textures: dict[str, unreal.Texture2D]
) -> None:
    """Rebuild M_EchoesSurface in place around the A3 texture maps.

    Parameter names stay exactly compatible with every existing instance:
    Color, Metallic, Roughness, EmissiveStrength keep their meanings; the
    ceramic family is the default map set and instances may override the
    texture parameters per family later.
    """
    lib = unreal.MaterialEditingLibrary

    color = lib.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -900, -260
    )
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property(
        "default_value", unreal.LinearColor(0.18, 0.48, 0.58, 1.0)
    )
    metallic = lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -900, -40
    )
    metallic.set_editor_property("parameter_name", "Metallic")
    metallic.set_editor_property("default_value", 0.25)
    roughness = lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -900, 60
    )
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.42)
    emission = lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -900, -150
    )
    emission.set_editor_property("parameter_name", "EmissiveStrength")
    emission.set_editor_property("default_value", 0.0)
    uv_scale = lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -1140, 170
    )
    uv_scale.set_editor_property("parameter_name", "UVScale")
    uv_scale.set_editor_property("default_value", 0.01)

    texcoord = lib.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -1140, 260
    )
    uv = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -980, 220
    )
    lib.connect_material_expressions(texcoord, "", uv, "A")
    lib.connect_material_expressions(uv_scale, "", uv, "B")

    base_map = _texture_parameter(
        material, "BaseColorMap",
        textures["T_EchoesCeramicCivic_BaseColor"], -760, 180,
        unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
    )
    mre_map = _texture_parameter(
        material, "MREMap",
        textures["T_EchoesCeramicCivic_MRE"], -760, 420,
        unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
    )
    normal_map = _texture_parameter(
        material, "NormalMap",
        textures["T_EchoesCeramicCivic_Normal"], -760, 660,
        unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
    )
    for node in (base_map, mre_map, normal_map):
        lib.connect_material_expressions(uv, "", node, "UVs")

    tinted = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -420, -160
    )
    lib.connect_material_expressions(color, "", tinted, "A")
    lib.connect_material_expressions(base_map, "RGB", tinted, "B")
    lib.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_bias = lib.create_material_expression(
        material, unreal.MaterialExpressionAdd, -600, 470
    )
    rough_bias.set_editor_property("const_b", 0.5)
    lib.connect_material_expressions(mre_map, "G", rough_bias, "A")
    rough_mul = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -420, 430
    )
    lib.connect_material_expressions(roughness, "", rough_mul, "A")
    lib.connect_material_expressions(rough_bias, "", rough_mul, "B")
    lib.connect_material_property(
        rough_mul, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    lib.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC
    )
    lib.connect_material_property(
        normal_map, "", unreal.MaterialProperty.MP_NORMAL
    )

    emissive_gain = lib.create_material_expression(
        material, unreal.MaterialExpressionAdd, -600, 560
    )
    emissive_gain.set_editor_property("const_b", 1.0)
    lib.connect_material_expressions(mre_map, "B", emissive_gain, "A")
    emissive_scaled = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -420, 540
    )
    lib.connect_material_expressions(emission, "", emissive_scaled, "A")
    lib.connect_material_expressions(emissive_gain, "", emissive_scaled, "B")
    emissive = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -240, -60
    )
    lib.connect_material_expressions(tinted, "", emissive, "A")
    lib.connect_material_expressions(emissive_scaled, "", emissive, "B")

    # surface-textured-v7: a mask-driven emissive path independent of the
    # team tint, so nodules, lattice edges, and crystal interiors glow in
    # their family colour on any slot. EmissiveTint defaults to white and
    # MaskedEmissiveStrength to zero, so every pre-v7 instance renders
    # unchanged. ViewShift blends a Fresnel term into the masked glow for
    # the Choir's view-shifting surfaces; the entity view zeroes it under
    # reduced motion so the surface holds steady for that setting.
    emissive_tint = lib.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -900, 820
    )
    emissive_tint.set_editor_property("parameter_name", "EmissiveTint")
    emissive_tint.set_editor_property(
        "default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    masked_strength = lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -900, 920
    )
    masked_strength.set_editor_property("parameter_name", "MaskedEmissiveStrength")
    masked_strength.set_editor_property("default_value", 0.0)
    view_shift = lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -900, 1020
    )
    view_shift.set_editor_property("parameter_name", "ViewShift")
    view_shift.set_editor_property("default_value", 0.0)
    fresnel = lib.create_material_expression(
        material, unreal.MaterialExpressionFresnel, -760, 1020
    )
    fresnel.set_editor_property("exponent", 3.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.08)
    view_blend = lib.create_material_expression(
        material, unreal.MaterialExpressionLinearInterpolate, -600, 1000
    )
    view_blend.set_editor_property("const_a", 1.0)
    lib.connect_material_expressions(fresnel, "", view_blend, "B")
    lib.connect_material_expressions(view_shift, "", view_blend, "Alpha")
    mask_gain = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -600, 880
    )
    lib.connect_material_expressions(mre_map, "B", mask_gain, "A")
    lib.connect_material_expressions(masked_strength, "", mask_gain, "B")
    tinted_map = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -600, 780
    )
    lib.connect_material_expressions(base_map, "RGB", tinted_map, "A")
    lib.connect_material_expressions(emissive_tint, "", tinted_map, "B")
    masked_gained = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -420, 820
    )
    lib.connect_material_expressions(tinted_map, "", masked_gained, "A")
    lib.connect_material_expressions(mask_gain, "", masked_gained, "B")
    masked_emissive = lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -240, 860
    )
    lib.connect_material_expressions(masked_gained, "", masked_emissive, "A")
    lib.connect_material_expressions(view_blend, "", masked_emissive, "B")
    emissive_total = lib.create_material_expression(
        material, unreal.MaterialExpressionAdd, -60, 0
    )
    lib.connect_material_expressions(emissive, "", emissive_total, "A")
    lib.connect_material_expressions(masked_emissive, "", emissive_total, "B")
    lib.connect_material_property(
        emissive_total, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )

    lib.layout_material_expressions(material)
    lib.recompile_material(material)
    unreal.EditorAssetLibrary.set_metadata_tag(
        material, "Echoes.AssetRevision", SURFACE_TEXTURED_REVISION
    )
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    unreal.log(
        f"[ECHOES_SURFACE_TEXTURED] path={MATERIAL_PATH} "
        f"revision={SURFACE_TEXTURED_REVISION} action=rebuilt"
    )


def create_surface_material(
    textures: dict[str, unreal.Texture2D]
) -> unreal.Material:
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        existing = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
        if isinstance(existing, unreal.Material):
            revision = unreal.EditorAssetLibrary.get_metadata_tag(
                existing, "Echoes.AssetRevision"
            )
            if revision != SURFACE_TEXTURED_REVISION:
                raise RuntimeError(
                    "Stale surface master survived the purge pass: "
                    f"{MATERIAL_PATH} (recorded {revision})"
                )
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
    rebuild_textured_surface_master(material, textures)
    return material


def create_world_surface_material() -> unreal.Material:
    action = "created"
    if unreal.EditorAssetLibrary.does_asset_exist(WORLD_MATERIAL_PATH):
        material = unreal.EditorAssetLibrary.load_asset(WORLD_MATERIAL_PATH)
        if not isinstance(material, unreal.Material):
            raise RuntimeError(
                f"Existing asset is not a Material: {WORLD_MATERIAL_PATH}"
            )
        action = "reused"
    else:
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

        # A3 ground layer: ash strata maps sampled by world position under
        # the existing UE noise macro-variation (baked identity, live breakup).
        lib = unreal.MaterialEditingLibrary
        ground_textures = import_surface_textures()
        wp_mask = lib.create_material_expression(
            material, unreal.MaterialExpressionComponentMask, -700, 320
        )
        wp_mask.set_editor_property("r", True)
        wp_mask.set_editor_property("g", True)
        wp_mask.set_editor_property("b", False)
        wp_mask.set_editor_property("a", False)
        lib.connect_material_expressions(world_position, "", wp_mask, "")
        world_uv_scale = lib.create_material_expression(
            material, unreal.MaterialExpressionScalarParameter, -700, 420
        )
        world_uv_scale.set_editor_property("parameter_name", "WorldUVScale")
        world_uv_scale.set_editor_property("default_value", 0.0004)
        world_uv = lib.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -540, 360
        )
        lib.connect_material_expressions(wp_mask, "", world_uv, "A")
        lib.connect_material_expressions(world_uv_scale, "", world_uv, "B")
        ash_base = _texture_parameter(
            material, "GroundBaseColorMap",
            ground_textures["T_EchoesGlassScarGround_BaseColor"], -380, 320,
            unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
        )
        ash_mre = _texture_parameter(
            material, "GroundMREMap",
            ground_textures["T_EchoesGlassScarGround_MRE"], -380, 560,
            unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
        )
        ash_normal = _texture_parameter(
            material, "GroundNormalMap",
            ground_textures["T_EchoesGlassScarGround_Normal"], -380, 800,
            unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
        )
        for node in (ash_base, ash_mre, ash_normal):
            lib.connect_material_expressions(world_uv, "", node, "UVs")
        textured_base = lib.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -120, -140
        )
        lib.connect_material_expressions(color_variation, "", textured_base, "A")
        lib.connect_material_expressions(ash_base, "", textured_base, "B")
        rough_bias = lib.create_material_expression(
            material, unreal.MaterialExpressionAdd, -220, 600
        )
        rough_bias.set_editor_property("const_b", 0.5)
        lib.connect_material_expressions(ash_mre, "G", rough_bias, "A")
        textured_rough = lib.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -80, 520
        )
        lib.connect_material_expressions(roughness_variation, "", textured_rough, "A")
        lib.connect_material_expressions(rough_bias, "", textured_rough, "B")

        unreal.MaterialEditingLibrary.connect_material_property(
            textured_base, "", unreal.MaterialProperty.MP_BASE_COLOR
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            metallic, "", unreal.MaterialProperty.MP_METALLIC
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            textured_rough, "", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            ash_normal, "", unreal.MaterialProperty.MP_NORMAL
        )
        # Golden fracture glow: steady vein emissive under reduced-flashing
        # limits, added to the tint-driven emissive path.
        glow_strength = lib.create_material_expression(
            material, unreal.MaterialExpressionScalarParameter, -220, 760
        )
        glow_strength.set_editor_property("parameter_name", "GroundGlowStrength")
        glow_strength.set_editor_property("default_value", 0.9)
        glow_masked = lib.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -80, 700
        )
        lib.connect_material_expressions(ash_mre, "B", glow_masked, "A")
        lib.connect_material_expressions(glow_strength, "", glow_masked, "B")
        vein_glow = lib.create_material_expression(
            material, unreal.MaterialExpressionMultiply, 40, 620
        )
        lib.connect_material_expressions(ash_base, "", vein_glow, "A")
        lib.connect_material_expressions(glow_masked, "", vein_glow, "B")
        combined_emissive = lib.create_material_expression(
            material, unreal.MaterialExpressionAdd, 160, -60
        )
        lib.connect_material_expressions(emissive_color, "", combined_emissive, "A")
        lib.connect_material_expressions(vein_glow, "", combined_emissive, "B")

        unreal.MaterialEditingLibrary.connect_material_property(
            combined_emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
        unreal.MaterialEditingLibrary.layout_material_expressions(material)

    usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    needs_save = action == "created"
    if not unreal.MaterialEditingLibrary.has_material_usage(material, usage):
        unreal.MaterialEditingLibrary.set_base_material_usage(
            material, usage, True
        )
        if action == "reused":
            action = "repaired"
        needs_save = True
    if not unreal.MaterialEditingLibrary.has_material_usage(material, usage):
        raise RuntimeError(
            "M_EchoesWorldSurface did not retain InstancedStaticMeshes usage"
        )

    if (
        unreal.EditorAssetLibrary.get_metadata_tag(
            material, "Echoes.AssetRevision"
        )
        != WORLD_MATERIAL_ASSET_REVISION
    ):
        if action == "reused":
            action = "repaired"
        needs_save = True

    if needs_save:
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
        unreal.EditorAssetLibrary.set_metadata_tag(
            material, "Echoes.AssetRevision", WORLD_MATERIAL_ASSET_REVISION
        )
        unreal.MaterialEditingLibrary.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
            raise RuntimeError("Could not save M_EchoesWorldSurface")

    unreal.log(
        "[ECHOES_WORLD_SURFACE_READY] "
        f"revision={WORLD_MATERIAL_ASSET_REVISION} action={action} "
        "instancedStaticMeshes=true"
    )
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
            action = "reused"
            existing_material = existing.get_material(0)
            if (
                existing_material is None
                or existing_material.get_path_name() != material.get_path_name()
            ):
                existing.set_material(0, material)
                unreal.EditorAssetLibrary.save_loaded_asset(existing, False)
                action = "rebound-material"
            unreal.log(
                "[ECHOES_PRESENTATION_VFX_ASSET] "
                f"path={spec.asset_path} display={spec.display_name} "
                f"lods={existing.get_num_lods()} "
                f"lod0Triangles={existing.get_num_triangles(0)} "
                f"lod1Triangles={existing.get_num_triangles(1)} action={action}"
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
    environment_revisions = {
        "GlassScarShelf": GLASS_SCAR_SHELF_ASSET_REVISION,
        "BrokenSunSky": BROKEN_SUN_SKY_ASSET_REVISION,
    }
    environment_revision = environment_revisions.get(spec.name)
    roster_factions = ("Meridian", "Kharuun", "Choir")
    is_roster_unit = spec.faction in roster_factions
    expected_revision = (
        route_revision
        if is_production_route
        else environment_revision
        if environment_revision is not None
        else (ROSTER_ASSET_REVISION if is_roster_unit else None)
    )
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
            if expected_revision is not None and revision != expected_revision:
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
    elif environment_revision is not None:
        unreal.EditorAssetLibrary.set_metadata_tag(
            asset, "Echoes.AssetRevision", environment_revision
        )
    elif is_roster_unit:
        unreal.EditorAssetLibrary.set_metadata_tag(
            asset, "Echoes.AssetRevision", ROSTER_ASSET_REVISION
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
        "[ECHOES_ART_BEGIN] generating 24 roster assets, 4 Future Well assets, "
        "8 Glass Scar environment assets, 8 selection/command VFX assets, "
        "and 3 destruction VFX assets"
    )
    surface_textures = import_surface_textures()
    surface_material = create_surface_material(surface_textures)
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
        len(presentation_vfx_assets) != 9
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
        f"revision={PRESENTATION_VFX_ASSET_REVISION} assets=9 selection=1 "
        "commands=7 orbit=1 lods=2 simpleCollision=0 "
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
    roster_assets = [
        asset
        for asset, spec in zip(generated, ASSETS)
        if spec.faction in ("Meridian", "Kharuun", "Choir")
    ]
    if (
        len(roster_assets) != 24
        or any(asset.get_num_lods() != 2 for asset in roster_assets)
        or any(
            unreal.EditorAssetLibrary.get_metadata_tag(
                asset, "Echoes.AssetRevision"
            )
            != ROSTER_ASSET_REVISION
            for asset in roster_assets
        )
    ):
        raise RuntimeError(f"Roster asset audit failed: count={len(roster_assets)}")
    unreal.log(
        "[ECHOES_ROSTER_READY] "
        f"revision={ROSTER_ASSET_REVISION} assets=24 lods=2 runtimeAuthority=presentation"
    )
    unreal.log(
        f"[ECHOES_ART_COMPLETE] generated={len(generated) + len(presentation_vfx_assets) + len(destruction_vfx_assets)} "
        f"roster=24 landmarks=4 environment=8 vfx=9 destructionVfx=3 material={MATERIAL_PATH} "
        f"worldMaterial={WORLD_MATERIAL_PATH} vfxMaterial={VFX_MATERIAL_PATH}"
    )


if __name__ == "__main__":
    main()
