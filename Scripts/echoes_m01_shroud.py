"""M01 knowledge-layer materials. Author: Angelis Pseftis.

Registered deterministic presentation source. The opaque shroud retains the
existing occluding volume; it has no lighting response, world noise or animation
that could be mistaken for geography. No visibility or simulation data enters it.
"""
import unreal

REVISION = "m01-shroud-unlit-v3"
EXPLORED_PATH = "/Game/Art/Generated/Materials/M_EchoesM01Explored"
MATERIAL_PATH = "/Game/Art/Generated/Materials/M_EchoesM01Shroud"


def create_material():
    _create_layer(EXPLORED_PATH, True)
    return _create_layer(MATERIAL_PATH, False)


def _create_layer(path, explored):
    lib = unreal.MaterialEditingLibrary
    material = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if material is not None:
        if not isinstance(material, unreal.Material):
            raise RuntimeError("M01 shroud path must contain a Material")
        if unreal.EditorAssetLibrary.get_metadata_tag(material,"Echoes.AssetRevision") == REVISION:
            return material
        lib.delete_all_material_expressions(material)
    else:
        directory,name=path.rsplit("/",1)
        material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,directory,unreal.Material,unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError("Cannot create M01 shroud material")
    material.set_editor_property("shading_model",unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode",unreal.BlendMode.BLEND_TRANSLUCENT if explored else unreal.BlendMode.BLEND_OPAQUE)
    lib.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    color=lib.create_material_expression(material,unreal.MaterialExpressionVectorParameter,-240,0)
    color.set_editor_property("parameter_name","Color")
    color.set_editor_property("default_value",unreal.LinearColor(.025,.007,.033,1))
    if not lib.connect_material_property(color,"",unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Cannot bind the M01 knowledge color")
    if explored:
        # Remembered ground remains visible through a quiet, static indigo tint.
        opacity=lib.create_material_expression(material,unreal.MaterialExpressionScalarParameter,-240,180)
        opacity.set_editor_property("parameter_name","MemoryOpacity")
        opacity.set_editor_property("default_value",.48)
        # A memory tile tints only its top face. Translucent cube sides otherwise
        # overlap at tile joints and print a false dark grid over remembered land.
        normal=lib.create_material_expression(material,unreal.MaterialExpressionPixelNormalWS,-700,330)
        vertical=lib.create_material_expression(material,unreal.MaterialExpressionComponentMask,-520,330)
        vertical.set_editor_property("r",False)
        vertical.set_editor_property("g",False)
        vertical.set_editor_property("b",True)
        top=lib.create_material_expression(material,unreal.MaterialExpressionClamp,-340,330)
        alpha=lib.create_material_expression(material,unreal.MaterialExpressionMultiply,-80,200)
        for source,output,target,input_name in ((normal,"",vertical,""),(vertical,"",top,""),(top,"",alpha,"A"),(opacity,"",alpha,"B")):
            if not lib.connect_material_expressions(source,output,target,input_name):
                raise RuntimeError("Cannot bind top-face memory tint")
        if not lib.connect_material_property(alpha,"",unreal.MaterialProperty.MP_OPACITY):
            raise RuntimeError("Cannot bind the M01 remembered-terrain opacity")
    errors=lib.recompile_material(material)
    if errors: raise RuntimeError(str(errors))
    if lib.get_material_property_input_node(material,unreal.MaterialProperty.MP_EMISSIVE_COLOR)!=color:
        raise RuntimeError("M01 shroud lost its color binding")
    for key,value in {
        "Echoes.Creator":"Angelis Pseftis",
        "Echoes.Provenance":"Original deterministic unlit M01 knowledge layers; unknown stays opaque, remembered ground uses static translucent tint; fog authority retained",
        "Echoes.AssetRevision":REVISION,
        "Echoes.RuntimeAuthority":"Presentation only",
        "Echoes.Status":"M01 presentation candidate"}.items():
        unreal.EditorAssetLibrary.set_metadata_tag(material,key,value)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material,False):
        raise RuntimeError("Cannot save M01 shroud")
    return material
