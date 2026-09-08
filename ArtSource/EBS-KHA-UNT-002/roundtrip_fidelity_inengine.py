"""Read back current Unreal UV and vertex data. Author: Angelis Pseftis."""
import os,json,unreal
from pathlib import Path
out=Path(os.environ['EBS_ROUNDTRIP_OUT']);out.mkdir(parents=True,exist_ok=True)
root=os.environ['EBS_MESH_ROOT'];report={'author':'Angelis Pseftis','exports':[],'errors':[]}
for state in ('Baseline','Carapace','Striker'):
 name='SK_EBS_KHA_UNT_002_'+state;mesh=unreal.load_asset(root+'/'+name)
 task=unreal.AssetExportTask();task.object=mesh;task.filename=str(out/(state+'.glb'));task.automated=True;task.prompt=False;task.replace_identical=True
 task.exporter=unreal.GLTFSkeletalMeshExporter();task.options=unreal.GLTFExportOptions()
 task.options.set_editor_property('export_vertex_colors',True)
 task.options.set_editor_property('bake_material_inputs',unreal.GLTFMaterialBakeMode.DISABLED)
 ok=unreal.Exporter.run_asset_export_task(task)
 report['exports'].append({'state':state,'ok':bool(ok),'errors':list(task.errors)})
 if not ok:report['errors'].append(state)
(out/'roundtrip-report.json').write_text(json.dumps(report,indent=2)+'\n')
if report['errors']:raise RuntimeError(str(report['errors']))
