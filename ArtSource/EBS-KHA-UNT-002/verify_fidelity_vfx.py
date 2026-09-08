"""Measure actual sandbox CPU Niagara particles. Author: Angelis Pseftis."""
import unreal,os,json,math
from pathlib import Path
out=Path(os.environ['EBS_VFX_OUT']);out.mkdir(parents=True,exist_ok=True)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world,"fx.Niagara.ForceWaitForCompilationOnActivate 1")
rt=unreal.RenderingLibrary.create_render_target2d(world,1000,600,unreal.TextureRenderTargetFormat.RTF_RGBA8)
camera=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(300,-650,300))
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(300,-650,300),unreal.Vector(300,0,100)),False)
camera.capture_component2d.set_editor_property('texture_target',rt)
camera.capture_component2d.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
camera.capture_component2d.capture_scene()
asset=unreal.load_asset(os.environ.get('EBS_VFX_ROOT','/Game/Echoes/RiftProduction/VFX/Qualified')+'/NS_EBS_KHA_RiftShard')
material=unreal.load_asset(os.environ.get('EBS_VFX_ROOT','/Game/Echoes/RiftProduction/VFX/Qualified')+'/M_EBS_KHA_Shard')
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.RiftNiagaraAuthoringLibrary.finish_preview_compilation()
report={'author':'Angelis Pseftis','scope':'sandbox CPU Niagara simulation; not gameplay/performance acceptance','directions':[],'errors':[]}
origin=unreal.Vector(0,0,100)
if os.environ.get('EBS_MESH_ROOT'):
 mesh=unreal.load_asset(os.environ['EBS_MESH_ROOT']+'/SK_EBS_KHA_UNT_002_Baseline')
 actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor,unreal.Vector(0,0,0))
 actor.skeletal_mesh_component.set_skeletal_mesh_asset(mesh)
 origin=actor.skeletal_mesh_component.get_socket_location('VFX_Muzzle_Shard_01')
 report['launch']={'socket':'VFX_Muzzle_Shard_01','origin_cm':[origin.x,origin.y,origin.z],'mesh':mesh.get_path_name()}
for vel in [(1200,0,0),(0,1200,0),(-1200,0,0)]:
 c=unreal.NiagaraFunctionLibrary.spawn_system_at_location(world,asset,origin,auto_destroy=False,auto_activate=False,pre_cull_check=False)
 if c is None:
  report['errors'].append('Niagara spawn returned None; rendered editor required');continue
 c.set_force_solo(True)
 c.set_variable_vec3('User.ShardVelocity',unreal.Vector(*vel));c.activate(True)
 row={'velocity_input':vel,'samples':[]};report['directions'].append(row)
 try:
  for tick in range(1,13):
   c.advance_simulation(6,1/60)
   data=json.loads(unreal.RiftNiagaraAuthoringLibrary.read_particles(c));data['time_s']=tick*.1;row['samples'].append(data)
   if vel==(1200,0,0) and tick<=5:
    camera.capture_component2d.capture_scene()
    unreal.RenderingLibrary.export_render_target(world,rt,str(out),f'shard_t{tick:02}.png')
   if 'error' in data:raise RuntimeError(data['error'])
  active=[s for s in row['samples'] if s['particles']]
  if len(active)<2:raise RuntimeError('Not enough observed particle samples to measure motion')
  for a,b in zip(active,active[1:]):
   if len(a['particles'])!=1 or len(b['particles'])!=1:raise RuntimeError('Not exactly one shard')
   dt=b['time_s']-a['time_s'];pa=a['particles'][0];pb=b['particles'][0]
   for i,axis in enumerate(['x','y','z']):
    measured=(pb[axis+'_cm']-pa[axis+'_cm'])/dt
    if abs(measured-vel[i])>1:raise RuntimeError(f'{axis} measured {measured}, expected {vel[i]}')
  if row['samples'][6]['particles']:raise RuntimeError('Shard survived beyond lifetime')
  row['motion_result']='PASS'
 except Exception as e:row['error']=str(e);report['errors'].append(str(e))
 c.deactivate()
 (out/'niagara-simulation-report.json').write_text(json.dumps(report,indent=2)+'\n')
(out/'niagara-simulation-report.json').write_text(json.dumps(report,indent=2)+'\n')
if report['errors']:raise RuntimeError(str(report['errors']))
