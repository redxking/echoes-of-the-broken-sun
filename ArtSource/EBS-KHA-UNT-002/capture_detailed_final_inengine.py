"""Render the detailed Riftstalker with its own transferred + baked maps.

Base colour = transferred BaseColor, darkened by baked AO and lifted at the curvature peaks (the
chamfer highlights the concept shows); normal = baked tangent normal from the high-poly; roughness and
metallic from the transferred MRE; emissive = MRE.B x Broken-Sun Amber. Team tint on StateMask.B.
"""
import unreal, json, os, time, math
TEX = os.environ["EBS_DTEX"]; OUT = os.environ["EBS_DOUT"]; os.makedirs(OUT, exist_ok=True)
DEST = "/Game/Echoes/DetailTextures"; MAT_DIR = "/Game/Echoes/VertexIDCheck/Materials"
report = {"errors": [], "frames": [], "connections": {"ok": 0, "failed": []}}
tools = unreal.AssetToolsHelpers.get_asset_tools(); lib = unreal.MaterialEditingLibrary
maps = {"BaseColor": ("T_EBS_KHA_UNT_002_DetailBaseColor.png", True, False),
        "Normal": ("T_EBS_KHA_UNT_002_DetailNormal.png", False, True),
        "MRE": ("T_EBS_KHA_UNT_002_DetailMRE.png", False, False),
        "StateMask": ("T_EBS_KHA_UNT_002_DetailStateMask.png", False, False),
        "AO": ("T_EBS_KHA_UNT_002_DetailAO.png", False, False),
        "Curvature": ("T_EBS_KHA_UNT_002_DetailCurvature.png", False, False)}
tex = {}
for key, (fname, srgb, is_normal) in maps.items():
    path = os.path.join(TEX, fname)
    if not os.path.exists(path): report["errors"].append("missing " + fname); continue
    t = unreal.AssetImportTask(); t.filename = path; t.destination_path = DEST
    t.replace_existing = True; t.automated = True; t.save = True
    tools.import_asset_tasks([t])
    a = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{fname[:-4]}")
    a.set_editor_property("srgb", srgb); a.set_editor_property("never_stream", True)
    a.set_editor_property("compression_settings",
        unreal.TextureCompressionSettings.TC_NORMALMAP if is_normal else
        (unreal.TextureCompressionSettings.TC_DEFAULT if srgb else unreal.TextureCompressionSettings.TC_MASKS))
    unreal.EditorAssetLibrary.save_loaded_asset(a); tex[key] = a
full = f"{MAT_DIR}/M_EBS_KHA_UNT_002_Detail"
if unreal.EditorAssetLibrary.does_asset_exist(full): unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset("M_EBS_KHA_UNT_002_Detail", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)
def C(s,sp,d,dp):
    if lib.connect_material_expressions(s,sp,d,dp): report["connections"]["ok"]+=1
    else: report["connections"]["failed"].append(f"{s.get_name()}.{sp}->{dp}")
def Pr(s,sp,pr):
    if lib.connect_material_property(s,sp,pr): report["connections"]["ok"]+=1
    else: report["connections"]["failed"].append(f"{s.get_name()}->{pr}")
def tn(k,x,y,st):
    n=lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample,x,y)
    n.set_editor_property("texture", tex[k]); n.set_editor_property("sampler_type", st); return n
base=tn("BaseColor",-1600,-400,unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
normal=tn("Normal",-1600,-100,unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mre=tn("MRE",-1600,200,unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
state=tn("StateMask",-1600,500,unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
ao=tn("AO",-1600,800,unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
curv=tn("Curvature",-1600,1100,unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
# base x AO (cavity darkening)
occl=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-1200,-300); C(base,"",occl,"A"); C(ao,"R",occl,"B")
# lift the chamfer peaks: curvature above mid brightens the edge, which is the concept's plate read
half=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,-1400,-60); half.set_editor_property("r",0.5)
edge=lib.create_material_expression(mat,unreal.MaterialExpressionSubtract,-1200,-40); C(curv,"R",edge,"A"); C(half,"",edge,"B")
gain=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,-1400,20); gain.set_editor_property("r",1.1)
edge2=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-1000,-20); C(edge,"",edge2,"A"); C(gain,"",edge2,"B")
lifted=lib.create_material_expression(mat,unreal.MaterialExpressionAdd,-820,-200); C(occl,"",lifted,"A"); C(edge2,"",lifted,"B")
team=lib.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-1000,400)
team.set_editor_property("parameter_name","TeamColor"); team.set_editor_property("default_value",unreal.LinearColor(0.16,0.86,0.96,1.0))
teamed=lib.create_material_expression(mat,unreal.MaterialExpressionLinearInterpolate,-600,-250)
C(lifted,"",teamed,"A"); C(team,"",teamed,"B"); C(state,"B",teamed,"Alpha")
Pr(teamed,"",unreal.MaterialProperty.MP_BASE_COLOR)
Pr(normal,"",unreal.MaterialProperty.MP_NORMAL)
Pr(mre,"G",unreal.MaterialProperty.MP_ROUGHNESS); Pr(mre,"R",unreal.MaterialProperty.MP_METALLIC)
amber=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-1000,200)
amber.set_editor_property("constant",unreal.LinearColor(1.0,0.82,0.62,1.0))
st=lib.create_material_expression(mat,unreal.MaterialExpressionScalarParameter,-1000,300)
st.set_editor_property("parameter_name","EmissiveStrength"); st.set_editor_property("default_value",0.5)
e1=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-750,220); C(mre,"B",e1,"A"); C(amber,"",e1,"B")
e2=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-550,240); C(e1,"",e2,"A"); C(st,"",e2,"B")
Pr(e2,"",unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat)
sub=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem); world=sub.get_editor_world()
reg=unreal.AssetRegistryHelpers.get_asset_registry()
asset=[a for a in reg.get_assets_by_path("/Game/Echoes/Detail",recursive=True) if str(a.asset_class_path.asset_name)=="SkeletalMesh"][0].get_asset()
actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0,0,0))
actor.skeletal_mesh_component.set_skeletal_mesh_asset(asset)
for i in range(actor.skeletal_mesh_component.get_num_materials()):
    actor.skeletal_mesh_component.set_material(i, mat)
rt=unreal.RenderingLibrary.create_render_target2d(world,1280,800,unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0,0,0)); cc=cap.capture_component2d
cc.set_editor_property("texture_target",rt); cc.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR); cc.set_editor_property("fov_angle",50.0)
pp=cc.get_editor_property("post_process_settings")
pp.set_editor_property("override_auto_exposure_method",True); pp.set_editor_property("auto_exposure_method",unreal.AutoExposureMethod.AEM_MANUAL)
pp.set_editor_property("override_auto_exposure_bias",True); pp.set_editor_property("auto_exposure_bias",11.0)
cc.set_editor_property("post_process_settings",pp); cc.set_editor_property("post_process_blend_weight",1.0)
light=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,900))
light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(-600,-500,900), unreal.Vector(0,0,100)),False)
light.light_component.set_intensity(3.0); light.light_component.set_light_color(unreal.LinearColor(1.0,0.82,0.62,1.0))
sky=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,700))
sky.light_component.set_intensity(0.6); sky.light_component.set_light_color(unreal.LinearColor(0.48,0.60,0.88,1.0))
def aim(arm,pitch,yaw=55.0):
    rp,ry=math.radians(pitch),math.radians(yaw)
    f=(math.cos(rp)*math.cos(ry),math.cos(rp)*math.sin(ry),math.sin(rp)); t=unreal.Vector(0,0,110)
    loc=unreal.Vector(t.x-f[0]*arm,t.y-f[1]*arm,t.z-f[2]*arm)
    cap.set_actor_location(loc,False,False); cap.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc,t),False)
def lum():
    pts=[unreal.RenderingLibrary.read_render_target_pixel(world,rt,int(rt.size_x*(0.45+0.04*i)),int(rt.size_y*(0.30+0.03*j))) for i in range(4) for j in range(4)]
    return sum(0.2126*p.r+0.7152*p.g+0.0722*p.b for p in pts)/len(pts)
aim(520.0,-18.0); deadline=time.time()+180; polls=0; ready=False
while time.time()<deadline:
    cc.capture_scene(); polls+=1
    if lum()>8.0: ready=True; break
    time.sleep(2.0)
report["readiness"]={"ready":ready,"polls":polls}
for label,arm,pitch in (("close",520.0,-18.0),("threequarter",760.0,-25.0),("tactical",1400.0,-55.0)):
    aim(arm,pitch)
    for _ in range(3): cc.capture_scene(); time.sleep(0.15)
    f=f"detailed_final_{label}.png"; unreal.RenderingLibrary.export_render_target(world,rt,OUT,f)
    report["frames"].append({"file":f,"framing":label,"creature_luminance":round(lum(),1)})
json.dump(report,open(os.path.join(OUT,"detailed-final-report.json"),"w"),indent=1)
print("[EBS_FINAL]"+json.dumps({"frames":len(report["frames"]),"errors":report["errors"],"connections":report["connections"],"readiness":report["readiness"]}))
