"""Deterministic authored material source for cosmetic exposed basalt cliffs.

Author: Angelis Pseftis.

The material deliberately uses world-space procedural geology.  It has no ground
texture or emissive path: vertical cut faces retain the procedural mesh's real
normals and receive their illumination from the scene rig.
"""

from __future__ import annotations

import unreal


MATERIAL_DIRECTORY = "/Game/Art/Generated/Materials"
MATERIAL_NAME = "M_EchoesCliffSurface"
MATERIAL_PATH = f"{MATERIAL_DIRECTORY}/{MATERIAL_NAME}"
REVISION = "cliff-surface-3d-basalt-v4"


def _node(material: unreal.Material, expression_type, x: int, y: int):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_type, x, y
    )
    if node is None:
        raise RuntimeError(f"Could not create {expression_type} on {MATERIAL_NAME}")
    return node


def _connect(source, source_output: str, destination, destination_input: str) -> None:
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, source_output, destination, destination_input
    ):
        raise RuntimeError(
            f"Could not connect {source.get_name()}.{source_output} to "
            f"{destination.get_name()}.{destination_input}"
        )


def _connect_property(source, source_output: str, property_name) -> None:
    if not unreal.MaterialEditingLibrary.connect_material_property(
        source, source_output, property_name
    ):
        raise RuntimeError(f"Could not connect {source.get_name()} to {property_name}")


def _constant(material: unreal.Material, value: float, x: int, y: int):
    node = _node(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def _noise(material: unreal.Material, position, scale: float, x: int, y: int):
    node = _node(material, unreal.MaterialExpressionNoise, x, y)
    node.set_editor_property("scale", scale)
    node.set_editor_property("quality", 2)
    node.set_editor_property("levels", 3)
    node.set_editor_property("output_min", 0.0)
    node.set_editor_property("output_max", 1.0)
    node.set_editor_property("turbulence", True)
    _connect(position, "", node, "")
    return node


def _existing_or_new() -> unreal.Material:
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        existing = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
        if not isinstance(existing, unreal.Material):
            raise RuntimeError(f"Existing asset is not a material: {MATERIAL_PATH}")
        if unreal.EditorAssetLibrary.get_metadata_tag(existing, "Echoes.AssetRevision") == REVISION:
            return existing
        unreal.MaterialEditingLibrary.delete_all_material_expressions(existing)
        return existing

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME,
        MATERIAL_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(f"Could not create {MATERIAL_PATH}")
    return material


def create_cliff_material() -> unreal.Material:
    """Create or return the exact-revision, world-space exposed-cliff master."""
    material = _existing_or_new()
    if unreal.EditorAssetLibrary.get_metadata_tag(material, "Echoes.AssetRevision") == REVISION:
        return material

    lib = unreal.MaterialEditingLibrary
    color = _node(material, unreal.MaterialExpressionVectorParameter, -1120, -180)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(.026, .027, .029, 1.0))

    metallic = _node(material, unreal.MaterialExpressionScalarParameter, -180, 260)
    metallic.set_editor_property("parameter_name", "Metallic")
    metallic.set_editor_property("default_value", .02)
    roughness = _node(material, unreal.MaterialExpressionScalarParameter, -180, 350)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", .90)

    world_position = _node(material, unreal.MaterialExpressionWorldPosition, -1120, 120)
    world_z = _node(material, unreal.MaterialExpressionComponentMask, -940, 140)
    world_z.set_editor_property("r", False)
    world_z.set_editor_property("g", False)
    world_z.set_editor_property("b", True)
    world_z.set_editor_property("a", False)
    _connect(world_position, "", world_z, "")

    # Broad, warped bedding: Z establishes geological layering while a 3D noise
    # field offsets each interval so it reads as cut basalt rather than built stripes.
    z_frequency = _constant(material, .006, -760, 220)
    z_scaled = _node(material, unreal.MaterialExpressionMultiply, -560, 165)
    _connect(world_z, "", z_scaled, "A")
    _connect(z_frequency, "", z_scaled, "B")
    bedding_warp = _noise(material, world_position, .0018, -760, 70)
    bedding_warp_scale = _constant(material, .20, -560, 65)
    bedding_warp_scaled = _node(material, unreal.MaterialExpressionMultiply, -380, 105)
    _connect(bedding_warp, "", bedding_warp_scaled, "A")
    _connect(bedding_warp_scale, "", bedding_warp_scaled, "B")
    bedding_coordinate = _node(material, unreal.MaterialExpressionAdd, -180, 130)
    _connect(z_scaled, "", bedding_coordinate, "A")
    _connect(bedding_warp_scaled, "", bedding_coordinate, "B")
    bedding_wave = _node(material, unreal.MaterialExpressionSine, 0, 130)
    bedding_wave.set_editor_property("period", 1.0)
    _connect(bedding_coordinate, "", bedding_wave, "")
    bedding_abs = _node(material, unreal.MaterialExpressionAbs, 180, 130)
    _connect(bedding_wave, "", bedding_abs, "")
    bedding_power = _node(material, unreal.MaterialExpressionPower, 360, 130)
    bedding_power.set_editor_property("const_exponent", 22.0)
    _connect(bedding_abs, "", bedding_power, "Base")

    dark_bed_multiplier = _node(material, unreal.MaterialExpressionConstant3Vector, -560, -190)
    dark_bed_multiplier.set_editor_property("constant", unreal.LinearColor(.88, .88, .89, 1.0))
    dark_bed = _node(material, unreal.MaterialExpressionMultiply, -360, -160)
    _connect(color, "", dark_bed, "A")
    _connect(dark_bed_multiplier, "", dark_bed, "B")
    bedding_color = _node(material, unreal.MaterialExpressionLinearInterpolate, 550, -100)
    _connect(color, "", bedding_color, "A")
    _connect(dark_bed, "", bedding_color, "B")
    _connect(bedding_power, "", bedding_color, "Alpha")

    # Fine 3D grain gently breaks value within each bed.  It is intentionally
    # bounded so terrain stays behind units at tactical distance.
    grain = _noise(material, world_position, .028, -180, -300)
    grain_low = _constant(material, .82, 0, -300)
    grain_high = _constant(material, 1.0, 0, -360)
    grain_range = _node(material, unreal.MaterialExpressionLinearInterpolate, 180, -300)
    _connect(grain_low, "", grain_range, "A")
    _connect(grain_high, "", grain_range, "B")
    _connect(grain, "", grain_range, "Alpha")
    grained_color = _node(material, unreal.MaterialExpressionMultiply, 740, -100)
    _connect(bedding_color, "", grained_color, "A")
    _connect(grain_range, "", grained_color, "B")

    # Broad geometry supplies the broken silhouette. Keep surface variation quiet:
    # warped sine fractures made the tactical view resemble contour lines.
    final_color = grained_color

    _connect_property(final_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    _connect_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    _connect_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    lib.layout_material_expressions(material)
    errors = lib.recompile_material(material)
    if errors:
        raise RuntimeError(f"{MATERIAL_NAME} compilation failed: {errors}")
    expected_outputs = (
        (unreal.MaterialProperty.MP_BASE_COLOR, final_color),
        (unreal.MaterialProperty.MP_METALLIC, metallic),
        (unreal.MaterialProperty.MP_ROUGHNESS, roughness),
    )
    for property_name, expected_node in expected_outputs:
        if lib.get_material_property_input_node(material, property_name) != expected_node:
            raise RuntimeError(f"{MATERIAL_NAME} lost required graph output {property_name}")
    if lib.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ) is not None:
        raise RuntimeError(f"{MATERIAL_NAME} must not contain an emissive path")

    unreal.EditorAssetLibrary.set_metadata_tag(material, "Echoes.Creator", "Angelis Pseftis")
    unreal.EditorAssetLibrary.set_metadata_tag(
        material,
        "Echoes.Provenance",
        "Original deterministic world-position basalt cliff material authored in-project",
    )
    unreal.EditorAssetLibrary.set_metadata_tag(material, "Echoes.Status", "M01 presentation candidate")
    unreal.EditorAssetLibrary.set_metadata_tag(material, "Echoes.AssetRevision", REVISION)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError(f"Could not save {MATERIAL_PATH}")
    return material
