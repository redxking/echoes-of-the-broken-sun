"""Lit-shading bisection: which input lifts a 0.045-albedo body to 0.8x the ground's brightness?
Each variant is its own Material (wiring differs), built in-process, read back as creature pixels."""
import unreal, json, os, time, math
OUT = os.environ["EBS_DIAG_OUT"]; os.makedirs(OUT, exist_ok=True)
DEST = "/Game/Echoes/VertexIDCheck/ArtTextures"; MAT_DIR = "/Game/Echoes/VertexIDCheck/Materials"
lib = unreal.MaterialEditingLibrary; tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"variants": {}, "errors": []}
tex = {k: unreal.EditorAssetLibrary.load_asset(f"{DEST}/T_EBS_KHA_UNT_002_{k}") for k in ("BaseColor", "MRE", "Normal")}
for t in tex.values():
    t.set_editor_property("never_stream", True)
report["normal_props"] = {p: str(tex["Normal"].get_editor_property(p)) for p in ("srgb", "compression_settings", "flip_green_channel")}

def build(name, use_normal, rough_mode, specular, flat_base=None):
    full = f"{MAT_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    m = tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_skeletal_mesh", True)
    if flat_base is not None:
        c = lib.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, -200)
        c.set_editor_property("constant", unreal.LinearColor(*flat_base, 1.0))
        lib.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        b = lib.create_material_expression(m, unreal.MaterialExpressionTextureSample, -400, -200)
        b.set_editor_property("texture", tex["BaseColor"]); b.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        lib.connect_material_property(b, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if use_normal:
        n = lib.create_material_expression(m, unreal.MaterialExpressionTextureSample, -400, 100)
        n.set_editor_property("texture", tex["Normal"]); n.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        lib.connect_material_property(n, "", unreal.MaterialProperty.MP_NORMAL)
    if rough_mode == "mre":
        r = lib.create_material_expression(m, unreal.MaterialExpressionTextureSample, -400, 400)
        r.set_editor_property("texture", tex["MRE"]); r.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        lib.connect_material_property(r, "G", unreal.MaterialProperty.MP_ROUGHNESS)
        lib.connect_material_property(r, "R", unreal.MaterialProperty.MP_METALLIC)
    elif rough_mode is not None:
        r = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 400)
        r.set_editor_property("r", float(rough_mode))
        lib.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if specular is not None:
        sp = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 520)
        sp.set_editor_property("r", float(specular))
        lib.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
    lib.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

variants = {
    "flat045_rough1_spec0": build("M_B_Flat045", False, 1.0, 0.0, flat_base=(0.045, 0.040, 0.037)),
    "flat045_default":      build("M_B_Flat045Def", False, None, None, flat_base=(0.045, 0.040, 0.037)),
    "base_only_default":    build("M_B_Base", False, None, None),
    "base_rough1_spec0":    build("M_B_BaseR1S0", False, 1.0, 0.0),
    "base_normal":          build("M_B_BaseN", True, None, None),
    "base_mre":             build("M_B_BaseMRE", False, "mre", None),
    "base_normal_mre":      build("M_B_BaseNMRE", True, "mre", None),
    "base_normal_mre_spec0": build("M_B_BaseNMRES0", True, "mre", 0.0),
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
pp.set_editor_property("override_auto_exposure_bias", True); pp.set_editor_property("auto_exposure_bias", 11.0)
cc.set_editor_property("post_process_settings", pp); cc.set_editor_property("post_process_blend_weight", 1.0)
light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 900))
light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(-600, -500, 900), unreal.Vector(0, 0, 100)), False)
light.light_component.set_intensity(3.0); light.light_component.set_light_color(unreal.LinearColor(1.0, 0.82, 0.62, 1.0))
sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 700))
sky.light_component.set_intensity(0.6); sky.light_component.set_light_color(unreal.LinearColor(0.48, 0.60, 0.88, 1.0))
arm, pitch, yaw = 520.0, -18.0, 55.0
rp, ry = math.radians(pitch), math.radians(yaw)
f = (math.cos(rp) * math.cos(ry), math.cos(rp) * math.sin(ry), math.sin(rp))
target = unreal.Vector(0, 0, 110); loc = unreal.Vector(target.x - f[0] * arm, target.y - f[1] * arm, target.z - f[2] * arm)
cap.set_actor_location(loc, False, False); cap.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc, target), False)
def creature_mean():
    pts = [unreal.RenderingLibrary.read_render_target_pixel(world, rt, int(rt.size_x * (0.45 + 0.05 * i)), int(rt.size_y * (0.30 + 0.05 * j))) for i in range(4) for j in range(4)]
    return [round(sum(p.r for p in pts) / 16, 1), round(sum(p.g for p in pts) / 16, 1), round(sum(p.b for p in pts) / 16, 1)]
def ground_mean():
    p = unreal.RenderingLibrary.read_render_target_pixel(world, rt, int(rt.size_x * 0.06), int(rt.size_y * 0.06)); return [p.r, p.g, p.b]
for name, m in variants.items():
    for i in range(comp_m.get_num_materials()):
        comp_m.set_material(i, m)
    for _ in range(4):
        cc.capture_scene(); time.sleep(0.15)
    unreal.RenderingLibrary.export_render_target(world, rt, OUT, f"bisect_{name}.png")
    report["variants"][name] = {"creature_mean": creature_mean(), "ground": ground_mean()}
json.dump(report, open(os.path.join(OUT, "bisect-report.json"), "w"), indent=1)
print("[EBS_BISECT] " + json.dumps(report["variants"]))
