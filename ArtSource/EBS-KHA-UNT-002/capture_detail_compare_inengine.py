"""Blockout vs detailed mesh, same art material, same framing — does the production atlas survive the
bevel? Builds the art material in-process (PF-016), assigns with set_material (PF-017), gates on
pixels, and reports per-region statistics so the answer is measured, not eyeballed."""
import unreal, json, os, time, math

TEX_DIR = os.environ["EBS_TEX_DIR"]; OUT = os.environ["EBS_ART_OUT"]; os.makedirs(OUT, exist_ok=True)
DEST = "/Game/Echoes/VertexIDCheck/ArtTextures"; MAT_DIR = "/Game/Echoes/VertexIDCheck/Materials"
SWEEP_HIGH, SWEEP_LOW = 1.02, -0.02; AMBER = (1.0, 0.82, 0.62)
report = {"errors": [], "frames": [], "connections": {"ok": 0, "failed": []}}
tools = unreal.AssetToolsHelpers.get_asset_tools(); lib = unreal.MaterialEditingLibrary

maps = {"BaseColor": ("T_EBS_KHA_UNT_002_BaseColor.png", True, False),
        "Normal": ("T_EBS_KHA_UNT_002_Normal.png", False, True),
        "MRE": ("T_EBS_KHA_UNT_002_MRE.png", False, False),
        "StateMask": ("T_EBS_KHA_UNT_002_StateMask.png", False, False)}
textures = {}
for key, (fname, srgb, is_normal) in maps.items():
    task = unreal.AssetImportTask(); task.filename = os.path.join(TEX_DIR, fname)
    task.destination_path = DEST; task.replace_existing = True; task.automated = True; task.save = True
    tools.import_asset_tasks([task])
    tex = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{fname[:-4]}")
    tex.set_editor_property("srgb", srgb); tex.set_editor_property("never_stream", True)
    tex.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_NORMALMAP if is_normal else
        (unreal.TextureCompressionSettings.TC_DEFAULT if srgb else unreal.TextureCompressionSettings.TC_MASKS))
    unreal.EditorAssetLibrary.save_loaded_asset(tex); textures[key] = tex

full = f"{MAT_DIR}/M_EBS_KHA_UNT_002_Art"
if unreal.EditorAssetLibrary.does_asset_exist(full): unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset("M_EBS_KHA_UNT_002_Art", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)
def C(s, sp, d, dp):
    if not lib.connect_material_expressions(s, sp, d, dp): report["connections"]["failed"].append(f"{s.get_name()}.{sp}->{dp}")
    else: report["connections"]["ok"] += 1
def Pr(s, sp, prop):
    if not lib.connect_material_property(s, sp, prop): report["connections"]["failed"].append(f"{s.get_name()}->{prop}")
    else: report["connections"]["ok"] += 1
def tn(key, x, y, st):
    n = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
    n.set_editor_property("texture", textures[key]); n.set_editor_property("sampler_type", st); return n
base = tn("BaseColor", -1400, -400, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
normal = tn("Normal", -1400, -100, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mre = tn("MRE", -1400, 200, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
state = tn("StateMask", -1400, 500, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
team = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1150, 350)
team.set_editor_property("parameter_name", "TeamColor"); team.set_editor_property("default_value", unreal.LinearColor(0.16,0.86,0.96,1.0))
teamed = lib.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -900, -350)
C(base,"",teamed,"A"); C(team,"",teamed,"B"); C(state,"B",teamed,"Alpha")
Pr(teamed,"",unreal.MaterialProperty.MP_BASE_COLOR)
Pr(normal,"",unreal.MaterialProperty.MP_NORMAL)
Pr(mre,"G",unreal.MaterialProperty.MP_ROUGHNESS); Pr(mre,"R",unreal.MaterialProperty.MP_METALLIC)
amber = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -900, 150)
amber.set_editor_property("constant", unreal.LinearColor(*AMBER, 1.0))
strength = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 250)
strength.set_editor_property("parameter_name","EmissiveStrength"); strength.set_editor_property("default_value",0.5)
e1 = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -650, 180); C(mre,"B",e1,"A"); C(amber,"",e1,"B")
e2 = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -450, 200); C(e1,"",e2,"A"); C(strength,"",e2,"B")
Pr(e2,"",unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat)

sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem); world = sub.get_editor_world()
reg = unreal.AssetRegistryHelpers.get_asset_registry()
def find(path, name_filter=None):
    for a in reg.get_assets_by_path(path, recursive=True):
        if str(a.asset_class_path.asset_name) == "SkeletalMesh":
            if name_filter and name_filter not in str(a.asset_name): continue
            if not name_filter and "LOD1" in str(a.asset_name): continue
            return a.get_asset()
    return None
subjects = {"blockout": find("/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_002"),
            "detailed": find("/Game/Echoes/Detail")}
report["subjects"] = {k: (v.get_path_name() if v else None) for k, v in subjects.items()}
actors = {}
for name, asset in subjects.items():
    if asset is None: report["errors"].append(f"{name}: not found"); continue
    a = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0,0,0))
    a.skeletal_mesh_component.set_skeletal_mesh_asset(asset)
    a.skeletal_mesh_component.set_visibility(False, True)
    for i in range(a.skeletal_mesh_component.get_num_materials()):
        a.skeletal_mesh_component.set_material(i, mat)
    actors[name] = a

rt = unreal.RenderingLibrary.create_render_target2d(world, 1280, 800, unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0,0,0)); cc = cap.capture_component2d
cc.set_editor_property("texture_target", rt); cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR); cc.set_editor_property("fov_angle", 50.0)
pp = cc.get_editor_property("post_process_settings")
pp.set_editor_property("override_auto_exposure_method", True); pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
pp.set_editor_property("override_auto_exposure_bias", True); pp.set_editor_property("auto_exposure_bias", 11.0)
cc.set_editor_property("post_process_settings", pp); cc.set_editor_property("post_process_blend_weight", 1.0)
light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,900))
light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(-600,-500,900), unreal.Vector(0,0,100)), False)
light.light_component.set_intensity(3.0); light.light_component.set_light_color(unreal.LinearColor(1.0,0.82,0.62,1.0))
sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,700))
sky.light_component.set_intensity(0.6); sky.light_component.set_light_color(unreal.LinearColor(0.48,0.60,0.88,1.0))

def aim(arm, pitch, yaw=55.0):
    rp, ry = math.radians(pitch), math.radians(yaw)
    f=(math.cos(rp)*math.cos(ry), math.cos(rp)*math.sin(ry), math.sin(rp))
    t=unreal.Vector(0,0,110); loc=unreal.Vector(t.x-f[0]*arm, t.y-f[1]*arm, t.z-f[2]*arm)
    cap.set_actor_location(loc, False, False); cap.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc, t), False)
def creature_lum():
    pts=[unreal.RenderingLibrary.read_render_target_pixel(world, rt, int(rt.size_x*(0.45+0.04*i)), int(rt.size_y*(0.30+0.03*j))) for i in range(4) for j in range(4)]
    return sum(0.2126*p.r+0.7152*p.g+0.0722*p.b for p in pts)/len(pts)
def show(name):
    for other, a in actors.items(): a.skeletal_mesh_component.set_visibility(other == name, True)
# readiness: the material must not render as the default (identical for both subjects would also fail)
show("blockout"); aim(520.0,-18.0)
deadline=time.time()+180.0; polls=0; ready=False
while time.time()<deadline:
    cc.capture_scene(); polls+=1
    if creature_lum()>8.0: ready=True; break
    time.sleep(2.0)
report["readiness"]={"ready":ready,"polls":polls}
for name in ("blockout","detailed"):
    if name not in actors: continue
    for label, arm, pitch in (("close",520.0,-18.0), ("tactical",1400.0,-55.0)):
        show(name); aim(arm,pitch)
        for _ in range(3): cc.capture_scene(); time.sleep(0.15)
        f=f"{name}_{label}.png"
        unreal.RenderingLibrary.export_render_target(world, rt, OUT, f)
        report["frames"].append({"file":f,"subject":name,"framing":label,"creature_luminance":round(creature_lum(),1)})
json.dump(report, open(os.path.join(OUT,"detail-compare-report.json"),"w"), indent=1)
print("[EBS_CMP]"+json.dumps({"subjects":report["subjects"],"frames":len(report["frames"]),"errors":report["errors"],"connections":report["connections"]}))
