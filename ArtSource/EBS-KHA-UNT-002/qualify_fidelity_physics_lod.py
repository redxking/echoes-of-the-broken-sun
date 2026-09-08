"""Sandbox-only skeletal physics and imported LOD qualification. Author: Angelis Pseftis."""
import unreal, os, json
from pathlib import Path
root=os.environ['EBS_MESH_ROOT']; source=Path(os.environ['EBS_SOURCE_DIR']);out=Path(os.environ['EBS_QUALIFY_OUT']);out.mkdir(parents=True,exist_ok=True)
sub=unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
report={'author':'Angelis Pseftis','scope':'isolated sandbox; no gameplay collision/navigation changes','states':[],'errors':[]}
for state in ['baseline','carapace_molt','striker_molt']:
 row={'state':state};report['states'].append(row)
 try:
  stem='SK_EBS_KHA_UNT_002_'+{'baseline':'Baseline','carapace_molt':'Carapace','striker_molt':'Striker'}[state]
  mesh=unreal.load_asset(root+'/'+stem+'.'+stem)
  if not mesh: raise RuntimeError('missing imported mesh')
  row['mesh']=mesh.get_path_name();row['lod_count_before']=sub.get_lod_count(mesh)
  row['lod_import_result']=sub.import_lod(mesh,1,str(source/(state+'_lod1.glb')))
  row['lod_count_after']=sub.get_lod_count(mesh)
  if row['lod_import_result']!=1 or row['lod_count_after']!=2: raise RuntimeError('authored LOD1 import failed')
  row['full_precision_uvs']=[]
  for lod in range(2):
   settings=sub.get_lod_build_settings(mesh,lod)
   settings.set_editor_property('use_full_precision_u_vs',True)
   sub.set_lod_build_settings(mesh,lod,settings)
   actual=sub.get_lod_build_settings(mesh,lod).get_editor_property('use_full_precision_u_vs')
   if not actual:raise RuntimeError('Full precision UV build setting did not persist')
   row['full_precision_uvs'].append(bool(actual))
  physics=sub.create_physics_asset(mesh,True,0)
  if not physics: raise RuntimeError('PhysicsAsset creation returned None')
  row['physics_asset']=physics.get_path_name();row['physics_compatible']=sub.is_physics_asset_compatible(mesh,physics)
  row['physics_assignment']=str(mesh.get_editor_property('physics_asset').get_path_name())
  if not row['physics_compatible']: raise RuntimeError('incompatible PhysicsAsset')
  row['physics_note']='Engine-generated preview bodies; ragdoll/contact behavior still requires rendered simulation review.'
  unreal.EditorAssetLibrary.save_loaded_asset(physics);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 except Exception as e:
  row['error']=str(e);report['errors'].append(state+': '+str(e))
(out/'physics-lod-report.json').write_text(json.dumps(report,indent=2)+'\n')
if report['errors']: raise RuntimeError('; '.join(report['errors']))
