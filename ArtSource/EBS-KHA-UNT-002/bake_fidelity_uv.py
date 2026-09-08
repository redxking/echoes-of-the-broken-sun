"""Bake through an already-qualified UV layout. Author: Angelis Pseftis."""
import bpy,sys,pathlib,json
sys.path.insert(0,str(pathlib.Path(__file__).parent));import fidelity_uv_finish as uv
args=sys.argv[sys.argv.index('--')+1:];source=pathlib.Path(args[0]);out=pathlib.Path(args[1])
bpy.ops.wm.open_mainfile(filepath=str(out/'Riftstalker_uv_representatives.blend'))
obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
bpy.context.scene.cycles.samples=1
bpy.context.scene.render.threads_mode='FIXED';bpy.context.scene.render.threads=4
textures=uv._bake_transfer(source,out,obj,obj.data.uv_layers['UV_Source'],obj.data.uv_layers['UV_Target'],2048)
p=out/'uv-production-report.json';report=json.loads(p.read_text());report['textures']=textures;report['texture_transfer']='COMPLETED_REQUIRES_RENDERED_QA';report['texture_source']=str(source);p.write_text(json.dumps(report,indent=2)+'\n')
