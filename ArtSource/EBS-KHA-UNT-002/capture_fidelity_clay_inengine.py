"""Matched clay form review in the isolated art sandbox. Author: Angelis Pseftis.

EBS_CLAY_JOB is a JSON with out and meshes {label: Unreal asset path}.
Requires a rendered editor and the sandbox's existing compilation bridge.
No mesh/material assignment or map is saved. This is not gameplay acceptance.
"""
import json, math, os, pathlib
import unreal

if 'EBSRiftProduction' not in unreal.Paths.get_project_file_path():
    raise RuntimeError('Clay review is restricted to the EBSRiftProduction sandbox')
job=json.loads(pathlib.Path(os.environ['EBS_CLAY_JOB']).read_text())
out=pathlib.Path(job['out']);out.mkdir(parents=True,exist_ok=True)
world=unreal.EditorLevelLibrary.get_editor_world()
report={'author':'Angelis Pseftis','creator':'Angelis Pseftis','status':'SANDBOX_CLAY_REVIEW_ONLY','frames':[],'errors':[]}
for a in unreal.EditorLevelLibrary.get_all_level_actors():
    if isinstance(a,unreal.Light):unreal.EditorLevelLibrary.destroy_actor(a)
mat=unreal.new_object(unreal.Material)
lib=unreal.MaterialEditingLibrary
c=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
c.set_editor_property('constant',unreal.LinearColor(.34,.34,.34,1))
assert lib.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
r=lib.create_material_expression(mat,unreal.MaterialExpressionConstant)
r.set_editor_property('r',.85)
assert lib.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
mat.set_editor_property('used_with_skeletal_mesh',True);lib.recompile_material(mat)
for pos,intensity in [((600,-500,900),6000),((-500,600,500),2000)]:
    a=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(*pos))
    a.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*pos),unreal.Vector(0,0,110)),False)
    a.light_component.set_intensity(intensity);a.light_component.set_light_color(unreal.LinearColor(1,1,1,1))
rt=unreal.RenderingLibrary.create_render_target2d(world,1280,800,unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector())
capture=cap.capture_component2d;capture.set_editor_property('texture_target',rt)
capture.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
capture.set_editor_property('fov_angle',50)
pp=capture.get_editor_property('post_process_settings')
for name,value in [('override_auto_exposure_method',True),('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',0.)]:pp.set_editor_property(name,value)
capture.set_editor_property('post_process_settings',pp);capture.set_editor_property('post_process_blend_weight',1.)
unreal.SystemLibrary.execute_console_command(world,'r.AntiAliasingMethod 1')
unreal.RiftNiagaraAuthoringLibrary.finish_preview_compilation()
for label,path in job['meshes'].items():
    mesh=unreal.load_asset(path)
    if not isinstance(mesh,unreal.SkeletalMesh):raise RuntimeError('Missing skeletal mesh: '+path)
    a=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor,unreal.Vector())
    c=a.skeletal_mesh_component;c.set_skeletal_mesh_asset(mesh)
    for i in range(c.get_num_materials()):c.set_material(i,mat)
    for view,arm,pitch,yaw in [('threequarter',520,-18,125),('side',520,-5,90),('front',520,-5,180),('tactical',1400,-55,125)]:
        p,y=math.radians(pitch),math.radians(yaw);target=unreal.Vector(0,0,110)
        loc=unreal.Vector(-arm*math.cos(p)*math.cos(y),-arm*math.cos(p)*math.sin(y),110-arm*math.sin(p))
        cap.set_actor_location(loc,False,False);cap.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc,target),False)
        for _ in range(3):capture.capture_scene()
        name=f'{label}_{view}.png';unreal.RenderingLibrary.export_render_target(world,rt,str(out),name)
        report['frames'].append({'file':name,'mesh':path,'view':view,'arm_cm':arm,'pitch_deg':pitch,'yaw_deg':yaw})
    unreal.EditorLevelLibrary.destroy_actor(a)
(out/'clay-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('[EBS_CLAY] '+json.dumps(report))
