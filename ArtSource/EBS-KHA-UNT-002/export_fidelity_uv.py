"""Apply verified corner UVs through the original skeletal exporter. Author: Angelis Pseftis."""
import argparse,json,hashlib,pathlib
import build_fidelity_pilot as p

def export(source,overrides,out):
 out.mkdir(parents=True,exist_ok=True)
 skeleton=p.base.build_skeleton(); clips=p.fidelity_motion.clips(p.base)
 report={'author':p.base.AUTHOR,'creator':p.base.AUTHOR,'status':'UV_APPLIED_ENGINE_REVALIDATION_REQUIRED','meshes':[]}
 for state in p.base.STATES:
  for lod in (0,1):
   stem=f'{state}_lod{lod}';old=(source/(stem+'.json')).read_bytes();data=json.loads(old)
   record=json.loads((overrides/(stem+'.uv-override.json')).read_text())
   assert hashlib.sha256(old).hexdigest()==record['source_sha256'],'stale UV source'
   mesh=p.assemble(lod,state);uv=record['polygon_corner_uv']
   assert len(mesh.polygons)==len(uv)==len(data['faces'])
   for poly,corners,face,component in zip(mesh.polygons,uv,data['faces'],data['components']):
    assert poly.component==component
    assert [list(x) for x in poly.points]==[data['vertices'][i] for i in face['vertices']]
    assert len(corners)==len(poly.points)
    assert all(0<=x<=1 and 0<=y<=1 for x,y in corners)
    poly.uv_override=[tuple(x) for x in corners]
   p.base.skel.write_skinned_glb(mesh,skeleton,str(out/(stem+'.glb')),animations=clips,include_collision=False,
       sockets_on_bones={n:v[0] for n,v in p.base.SOCKETS.items()},extras={'author':p.base.AUTHOR,'production_id':p.base.PRODUCTION_ID,'state':state,'lod':lod,'uv_source_sha256':record['source_sha256']})
   mesh.write_obj(str(out/(stem+'.obj')))
   (out/(stem+'.json')).write_text(json.dumps(p.scene_data(mesh)))
   report['meshes'].append({'state':state,'lod':lod,'triangles':mesh.triangle_count(),'glb_sha256':hashlib.sha256((out/(stem+'.glb')).read_bytes()).hexdigest()})
 report['animations']=[{'name':c.name,'duration_s':c.duration_s} for c in clips]
 report['motion_contract']=p.fidelity_motion.motion_contract()
 (out/'uv-export-report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('--source',type=pathlib.Path,required=True);a.add_argument('--overrides',type=pathlib.Path,required=True);a.add_argument('--out',type=pathlib.Path,required=True);v=a.parse_args();export(v.source,v.overrides,v.out)
