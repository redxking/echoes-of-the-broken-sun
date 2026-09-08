"""Generate an isolated, rigged fidelity pilot. Author: Angelis Pseftis."""
import pathlib, json, hashlib, argparse
import build_riftstalker as base
import fidelity_geometry
import fidelity_motion

# Trial rest joints; the previous blockout and its exports remain unchanged.
# Folded directional knees are authored rest joints, not skin-only decoration.
base.KNEE_Z = 97.0
base.BONES = [(n,p, ((4. if n.startswith('f') else -106.),
                    (-82. if n.startswith(('fl','rl')) else 82.),
                    (97. if n.startswith('f') else 106.)) if n.endswith('_lower') else h,why)
              for n,p,h,why in base.BONES]
REVISION='ebs-riftstalker-fidelity-pilot-v2'

def assemble(lod,state):
    m=fidelity_geometry.build(base.kit,base,lod,state)
    base.bind(m);m.vertex_colors=base.vertex_colors_for(m.components())
    # Source shell_01 is at the tail. Higher R must begin at shell_05, the nose.
    for c,value in list(m.vertex_colors.items()):
        if c.startswith('shell_'):
            r=.30+.30*(int(c.split('_')[1])-1)/(base.CARAPACE_SHELLS-1)
            m.vertex_colors[c]=(round(r,4),value[1],value[2],1.)
    for name,(bone,pos,yaw,purpose) in base.SOCKETS.items():
        m.sockets.append(base.kit.Socket(name,pos,yaw,purpose))
    return m

def scene_data(m):
    scene={'vertices':[],'faces':[],'slots':m.slots,'colors':[], 'components':[], 'uv':[]}
    for p in m.polygons:
        offset=len(scene['vertices']);scene['vertices'].extend(p.points)
        scene['faces'].append({'vertices':list(range(offset,offset+len(p.points))),'material':p.slot})
        scene['colors'].extend([m.vertex_color(p.component)]*len(p.points));scene['components'].append(p.component)
        scene['uv'].extend(p.uv_override or [(0,0)]*len(p.points))
    return scene

def generate(out):
    out.mkdir(parents=True,exist_ok=True)
    report={'author':base.AUTHOR,'creator':base.AUTHOR,'revision':REVISION,
        'status':'GEOMETRY_PILOT_NOT_ACCEPTED','meshes':[],
        'limits':'New rig pose, UVs, geometry and motions require fresh Unreal verification. Applicable authored action set requires engine and gameplay-distance verification.'}
    skeleton=base.build_skeleton();clips=fidelity_motion.clips(base)
    report['motion_contract']=fidelity_motion.motion_contract()
    report['animations']=[{'name':c.name,'duration_s':c.duration_s,'loop':c.loop,'purpose':c.purpose} for c in clips]
    meshes={(state,lod):assemble(lod,state) for state in base.STATES for lod in (0,1)}
    atlas=base.kit.pack_atlas([(m,lod) for (state,lod),m in meshes.items()],size=2048)
    base.kit.write_bake_manifest(str(out/'bake-manifest.json'),atlas,extras={
        'production_asset_id':base.PRODUCTION_ID,'revision':REVISION,'card':base.CARD,
        'slot_families':{base.STRATA:'kharuun_obsidian',base.AMBER:'kharuun_amber'},
        'team_components':['shell_03'],'team_band':{'shell_03':[.40,.60]},'molt_blend_size':512,
        'emissive_ceiling':{'fraction':.15,'source':base.CARD}})
    report['uv']={k:atlas[k] for k in ('size','density_px_per_cm','gutter_px','used_fraction')};report['uv']['charts']=len(atlas['charts'])
    for (state,lod),m in meshes.items():
        stem=f'{state}_lod{lod}';m.write_obj(str(out/(stem+'.obj')))
        base.skel.write_skinned_glb(m,skeleton,str(out/(stem+'.glb')),animations=clips,include_collision=False,
            sockets_on_bones={n:v[0] for n,v in base.SOCKETS.items()},
            extras={'author':base.AUTHOR,'production_id':base.PRODUCTION_ID,'status':'GEOMETRY_PILOT_NOT_ACCEPTED','state':state,'lod':lod})
        (out/(stem+'.json')).write_text(json.dumps(scene_data(m)))
        report['meshes'].append({'state':state,'lod':lod,'triangles':m.triangle_count(),'bounds_cm':m.bounds(),
            'sha256':hashlib.sha256((out/(stem+'.obj')).read_bytes()).hexdigest(),
            'glb_sha256':hashlib.sha256((out/(stem+'.glb')).read_bytes()).hexdigest()})
    rest=meshes[('baseline',0)]
    for name,fraction in [('move',.25),('molt',.5),('death',1.)]:
        clip=next(c for c in clips if c.name==name)
        m=base.skel.pose_mesh(rest,skeleton,base.sample_pose(clip,fraction));m.vertex_colors=dict(rest.vertex_colors)
        (out/f'pose_{name}.json').write_text(json.dumps(scene_data(m)))
    (out/'geometry-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'status':report['status'],'mesh_count':len(meshes),'uv':report['uv']},indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--out',required=True);a=p.parse_args();generate(pathlib.Path(a.out))
