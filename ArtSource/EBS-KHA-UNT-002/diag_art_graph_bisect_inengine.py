"""Bisect the ART graph in-engine: base+normal+MRE (proved dark) then add one stage at a time -
team lerp, window lerp, emissive chain - each its own Material, read back as creature pixels at
MoltProgress 0. Whichever stage lifts the body names the cause."""
import unreal, json, os, time, math
OUT = os.environ["EBS_DIAG_OUT"]; os.makedirs(OUT, exist_ok=True)
DEST = "/Game/Echoes/VertexIDCheck/ArtTextures"; MAT_DIR = "/Game/Echoes/VertexIDCheck/Materials"
lib = unreal.MaterialEditingLibrary; tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"variants": {}, "errors": [], "connections_failed": []}
tex = {k: unreal.EditorAssetLibrary.load_asset(f"{DEST}/T_EBS_KHA_UNT_002_{k}") for k in ("BaseColor", "MRE", "Normal", "StateMask")}
for t in tex.values():
    t.set_editor_property("never_stream", True)
SWEEP_HIGH, SWEEP_LOW = 1.02, -0.02

def build(name, team=False, window=False, emissive=False, team_alpha_const=None):
    full = f"{MAT_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    m = tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("used_with_skeletal_mesh", True)
    def C(src, sp, dst, dp):
        if not lib.connect_material_expressions(src, sp, dst, dp):
            report["connections_failed"].append(f"{name}: {src.get_name()}.{sp} -> {dst.get_name()}.{dp}")
    def P(src, sp, prop):
        if not lib.connect_material_property(src, sp, prop):
            report["connections_failed"].append(f"{name}: {src.get_name()}.{sp} -> {prop}")
    def tn(key, x, y, sampler):
        n = lib.create_material_expression(m, unreal.MaterialExpressionTextureSample, x, y)
        n.set_editor_property("texture", tex[key]); n.set_editor_property("sampler_type", sampler); return n
    base = tn("BaseColor", -1400, -400, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    normal = tn("Normal", -1400, -100, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    mre = tn("MRE", -1400, 200, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    P(normal, "", unreal.MaterialProperty.MP_NORMAL)
    P(mre, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    P(mre, "R", unreal.MaterialProperty.MP_METALLIC)
    colour = base
    if team:
        state = tn("StateMask", -1400, 500, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        tc = lib.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -1150, 350)
        tc.set_editor_property("parameter_name", "TeamColor"); tc.set_editor_property("default_value", unreal.LinearColor(0.16, 0.86, 0.96, 1.0))
        lerp = lib.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -900, -350)
        C(colour, "", lerp, "A"); C(tc, "", lerp, "B")
        if team_alpha_const is None:
            C(state, "B", lerp, "Alpha")
        else:
            k = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -1100, -300); k.set_editor_property("r", float(team_alpha_const)); C(k, "", lerp, "Alpha")
        colour = lerp
    if window:
        vc = lib.create_material_expression(m, unreal.MaterialExpressionVertexColor, -1400, 800)
        progress = lib.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -1400, 1000)
        progress.set_editor_property("parameter_name", "MoltProgress"); progress.set_editor_property("default_value", 0.0)
        span = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -1400, 1100); span.set_editor_property("r", SWEEP_HIGH - SWEEP_LOW)
        high = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -1400, 1180); high.set_editor_property("r", SWEEP_HIGH)
        scaled = lib.create_material_expression(m, unreal.MaterialExpressionMultiply, -1150, 1050); C(progress, "", scaled, "A"); C(span, "", scaled, "B")
        thr = lib.create_material_expression(m, unreal.MaterialExpressionSubtract, -950, 1080); C(high, "", thr, "A"); C(scaled, "", thr, "B")
        one = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -950, 850); one.set_editor_property("r", 1.0)
        zero = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -950, 920)
        swept = lib.create_material_expression(m, unreal.MaterialExpressionIf, -700, 900)
        C(vc, "R", swept, "A"); C(thr, "", swept, "B"); C(one, "", swept, "A > B"); C(one, "", swept, "A == B"); C(zero, "", swept, "A < B")
        inv_p = lib.create_material_expression(m, unreal.MaterialExpressionOneMinus, -950, 1250); C(progress, "", inv_p, "")
        win = lib.create_material_expression(m, unreal.MaterialExpressionMultiply, -700, 700); C(swept, "", win, "A"); C(inv_p, "", win, "B")
        half = lib.create_material_expression(m, unreal.MaterialExpressionConstant, -700, 780); half.set_editor_property("r", 0.45)
        soft = lib.create_material_expression(m, unreal.MaterialExpressionMultiply, -550, 720); C(win, "", soft, "A"); C(half, "", soft, "B")
        core = lib.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -900, -200); core.set_editor_property("constant", unreal.LinearColor(0.55, 0.28, 0.10, 1.0))
        lerp2 = lib.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -450, -300)
        C(colour, "", lerp2, "A"); C(core, "", lerp2, "B"); C(soft, "", lerp2, "Alpha")
        colour = lerp2
        if emissive:
            amber = lib.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -900, 150); amber.set_editor_property("constant", unreal.LinearColor(1.0, 0.82, 0.62, 1.0))
            strength = lib.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 250)
            strength.set_editor_property("parameter_name", "EmissiveStrength"); strength.set_editor_property("default_value", 4.0)
            e1 = lib.create_material_expression(m, unreal.MaterialExpressionMultiply, -650, 180); C(mre, "B", e1, "A"); C(amber, "", e1, "B")
            e2 = lib.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 200); C(e1, "", e2, "A"); C(strength, "", e2, "B")
            boost = lib.create_material_expression(m, unreal.MaterialExpressionAdd, -450, 500); C(one, "", boost, "A"); C(win, "", boost, "B")
            e3 = lib.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 250); C(e2, "", e3, "A"); C(boost, "", e3, "B")
            P(e3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    P(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    lib.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

variants = {
    "A_base_normal_mre":           build("M_G_A"),
    "B_plus_team_statemaskB":      build("M_G_B", team=True),
    "B2_plus_team_alpha0":         build("M_G_B2", team=True, team_alpha_const=0.0),
    "C_plus_window":               build("M_G_C", team=True, window=True),
    "D_plus_emissive_full":        build("M_G_D", team=True, window=True, emissive=True),
}
sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem); world = sub.get_editor_world()
reg = unreal.AssetRegistryHelpers.get_asset_registry()
mesh = [a for a in reg.get_assets_by_path("/Game/Echoes/VertexIDCheck/carapace_molt", recursive=True) if str(a.asset_class_path.asset_name) == "SkeletalMesh"][0].get_asset()
actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0))
comp_m = actor.skeletal_mesh_component; comp_m.set_skeletal_mesh_asset(mesh)
rt = unreal.RenderingLibrary.create_render_target2d(world, 1280, 800, unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0)); cc = cap.capture_component2d
cc.set_editor_property("texture_target", rt); cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR); cc.set_editor_property("fov_angle", 50.0)
pp = cc.get_editor_property("post_process_settings")
pp.set_editor_property("override_auto_exposure_method", True); pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
pp.set_editor_property("override_auto_exposure_bias", True); pp.set_editor_property("auto_exposure_bias", 11.0)
cc.set_editor_property("post_process_settings", pp); cc.set_editor_property("post_process_blend_weight", 1.0)
light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 900))
light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(-600, -500, 900), unreal.Vector(0, 0, 100)), False)
light.light_component.set_intensity(3.0); light.light_component.set_light_color(unreal.LinearColor(1.0, 0.82, 0.62, 1.0))
sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 700)); sky.light_component.set_intensity(0.6); sky.light_component.set_light_color(unreal.LinearColor(0.48, 0.60, 0.88, 1.0))
arm, pitch, yaw = 520.0, -18.0, 55.0; rp, ry = math.radians(pitch), math.radians(yaw)
f = (math.cos(rp) * math.cos(ry), math.cos(rp) * math.sin(ry), math.sin(rp)); target = unreal.Vector(0, 0, 110)
loc = unreal.Vector(target.x - f[0] * arm, target.y - f[1] * arm, target.z - f[2] * arm)
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
    unreal.RenderingLibrary.export_render_target(world, rt, OUT, f"graph_{name}.png")
    report["variants"][name] = {"creature_mean": creature_mean(), "ground": ground_mean()}
json.dump(report, open(os.path.join(OUT, "graph-bisect-report.json"), "w"), indent=1)
print("[EBS_GRAPH] " + json.dumps(report))
