"""Which texture reaches the surface? Unlit materials whose emissive IS one sampled map, captured on
the carapace mesh and read back as pixels. Exposure-independent RATIOS decide, not looks."""
import unreal, json, os, time, math
OUT = os.environ["EBS_DIAG_OUT"]; os.makedirs(OUT, exist_ok=True)
DEST = "/Game/Echoes/VertexIDCheck/ArtTextures"; MAT_DIR = "/Game/Echoes/VertexIDCheck/Materials"
lib = unreal.MaterialEditingLibrary; tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"variants": {}, "errors": []}
tex = {k: unreal.EditorAssetLibrary.load_asset(f"{DEST}/T_EBS_KHA_UNT_002_{k}") for k in ("BaseColor", "MRE", "StateMask", "Normal")}
report["textures_loaded"] = {k: (v is not None) for k, v in tex.items()}
for k, v in tex.items():
    if v is not None:
        report.setdefault("texture_props", {})[k] = {"srgb": bool(v.get_editor_property("srgb")),
                                                     "compression": str(v.get_editor_property("compression_settings")),
                                                     "size": [v.blueprint_get_size_x(), v.blueprint_get_size_y()]}

def unlit(name, wire):
    full = f"{MAT_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    m = tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_skeletal_mesh", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    wire(m)
    lib.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

def sample_wire(key, sampler):
    def w(m):
        n = lib.create_material_expression(m, unreal.MaterialExpressionTextureSample, -400, 0)
        n.set_editor_property("texture", tex[key]); n.set_editor_property("sampler_type", sampler)
        lib.connect_material_property(n, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return w
def vc_wire(m):
    n = lib.create_material_expression(m, unreal.MaterialExpressionVertexColor, -400, 0)
    lib.connect_material_property(n, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
def const_wire(m):
    n = lib.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    n.set_editor_property("constant", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
    lib.connect_material_property(n, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
def uv_wire(m):
    n = lib.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -400, 0)
    a = lib.create_material_expression(m, unreal.MaterialExpressionAppendVector, -200, 0)
    z = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 120)
    lib.connect_material_expressions(n, "", a, "A"); lib.connect_material_expressions(z, "", a, "B")
    lib.connect_material_property(a, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

variants = {
    "const_half": unlit("M_Diag_Const", const_wire),
    "basecolor": unlit("M_Diag_BaseColor", sample_wire("BaseColor", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)),
    "mre": unlit("M_Diag_MRE", sample_wire("MRE", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)),
    "statemask": unlit("M_Diag_StateMask", sample_wire("StateMask", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)),
    "vertexcolor": unlit("M_Diag_VertexColor", vc_wire),
    "uv0": unlit("M_Diag_UV0", uv_wire),
}

sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem); world = sub.get_editor_world()
reg = unreal.AssetRegistryHelpers.get_asset_registry()
mesh = [a for a in reg.get_assets_by_path("/Game/Echoes/VertexIDCheck/carapace_molt", recursive=True)
        if str(a.asset_class_path.asset_name) == "SkeletalMesh"][0].get_asset()
actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0))
comp_m = actor.skeletal_mesh_component; comp_m.set_skeletal_mesh_asset(mesh)
rt = unreal.RenderingLibrary.create_render_target2d(world, 1280, 800, unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
cc = cap.capture_component2d
cc.set_editor_property("texture_target", rt); cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
cc.set_editor_property("fov_angle", 50.0)
pp = cc.get_editor_property("post_process_settings")
pp.set_editor_property("override_auto_exposure_method", True); pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
pp.set_editor_property("override_auto_exposure_bias", True); pp.set_editor_property("auto_exposure_bias", 12.0)
cc.set_editor_property("post_process_settings", pp); cc.set_editor_property("post_process_blend_weight", 1.0)
arm, pitch, yaw = 520.0, -18.0, 55.0
rp, ry = math.radians(pitch), math.radians(yaw)
f = (math.cos(rp) * math.cos(ry), math.cos(rp) * math.sin(ry), math.sin(rp))
target = unreal.Vector(0, 0, 110); loc = unreal.Vector(target.x - f[0] * arm, target.y - f[1] * arm, target.z - f[2] * arm)
cap.set_actor_location(loc, False, False); cap.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc, target), False)

def creature_mean():
    pts = [unreal.RenderingLibrary.read_render_target_pixel(world, rt, int(rt.size_x * (0.45 + 0.05 * i)), int(rt.size_y * (0.30 + 0.05 * j)))
           for i in range(4) for j in range(4)]
    return [round(sum(p.r for p in pts) / 16, 1), round(sum(p.g for p in pts) / 16, 1), round(sum(p.b for p in pts) / 16, 1)]
def ground_mean():
    p = unreal.RenderingLibrary.read_render_target_pixel(world, rt, int(rt.size_x * 0.06), int(rt.size_y * 0.06)); return [p.r, p.g, p.b]

for name, m in variants.items():
    for i in range(comp_m.get_num_materials()):
        comp_m.set_material(i, m)
    for _ in range(4):
        cc.capture_scene(); time.sleep(0.15)
    unreal.RenderingLibrary.export_render_target(world, rt, OUT, f"diag_{name}.png")
    report["variants"][name] = {"creature_mean": creature_mean(), "ground": ground_mean(),
                                "material": comp_m.get_material(0).get_name()}
json.dump(report, open(os.path.join(OUT, "diag-report.json"), "w"), indent=1)
print("[EBS_DIAG] " + json.dumps(report["variants"]))
