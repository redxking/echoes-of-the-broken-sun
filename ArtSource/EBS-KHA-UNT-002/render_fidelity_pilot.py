"""Blender CPU review of authored geometry; not Unreal evidence. Author: Angelis Pseftis."""
import bpy, json, sys, pathlib, math
from mathutils import Vector
args=sys.argv[sys.argv.index('--')+1:];root=pathlib.Path(args[0]);state=args[1] if len(args)>1 else 'baseline_lod0'
d=json.loads((root/(state+'.json')).read_text())
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
def material(name,color,rough=.85,emission=False):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough
    if emission:p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=.6
    return m
stone=material('Charcoal faceted mineral - review material',(.040,.042,.045))
amber=material('Amber seam - review material',(.40,.18,.035),emission=True)
mesh=bpy.data.meshes.new('Riftstalker fidelity source');mesh.from_pydata([tuple(c/100 for c in v) for v in d['vertices']],[],[f['vertices'] for f in d['faces']]);mesh.update()
obj=bpy.data.objects.new('Riftstalker',mesh);bpy.context.collection.objects.link(obj)
mesh.materials.append(stone);mesh.materials.append(amber)
for p,f in zip(mesh.polygons,d['faces']):p.material_index=f['material'];p.use_smooth=False
col=mesh.color_attributes.new(name='MoltChannels',type='FLOAT_COLOR',domain='POINT')
for v,c in zip(col.data,d['colors']):v.color=c
uv=mesh.uv_layers.new(name='UVMap')
for loop in mesh.loops:
    u,v=d['uv'][loop.vertex_index];uv.data[loop.index].uv=(u,1-v)
# Optional atlas review. Shader appearance is Blender evidence, not Unreal qualification.
textured=(root/'textures/T_EBS_KHA_UNT_002_BaseColor.png').is_file()
if textured:
    for mat in (stone,amber):
        nodes=mat.node_tree.nodes;links=mat.node_tree.links;p=nodes.get('Principled BSDF')
        def tex(suffix,color):
            im=bpy.data.images.load(str(root/f'textures/T_EBS_KHA_UNT_002_{suffix}.png'),check_existing=True)
            im.colorspace_settings.name='sRGB' if color else 'Non-Color'
            n=nodes.new('ShaderNodeTexImage');n.image=im;n.interpolation='Linear';return n
        base=tex('BaseColor',True);links.new(base.outputs['Color'],p.inputs['Base Color'])
        mre=tex('MRE',False);split=nodes.new('ShaderNodeSeparateColor');links.new(mre.outputs['Color'],split.inputs['Color'])
        links.new(split.outputs['Red'],p.inputs['Metallic'])
        links.new(split.outputs['Green'],p.inputs['Roughness'])
        p.inputs['Emission Color'].default_value=(.50,.22,.06,1);links.new(split.outputs['Blue'],p.inputs['Emission Strength'])
        normal=tex('Normal',False);nm=nodes.new('ShaderNodeNormalMap');nm.inputs['Strength'].default_value=.5
        # Source normal map is DirectX; Blender tangent normals require the opposite green sign.
        ns=nodes.new('ShaderNodeSeparateColor');links.new(normal.outputs['Color'],ns.inputs['Color'])
        inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1
        links.new(ns.outputs['Green'],inv.inputs[1]);nc=nodes.new('ShaderNodeCombineColor')
        links.new(ns.outputs['Red'],nc.inputs['Red']);links.new(inv.outputs[0],nc.inputs['Green']);links.new(ns.outputs['Blue'],nc.inputs['Blue'])
        links.new(nc.outputs['Color'],nm.inputs['Color']);links.new(nm.outputs['Normal'],p.inputs['Normal'])
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,0));floor=bpy.context.object;floor.name='Review ground';floor.data.materials.append(material('Review ground',(.20,.185,.16)))
def aim(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
def light(name,loc,energy,color,size):
    data=bpy.data.lights.new(name,'AREA');data.energy=energy;data.color=color;data.shape='DISK';data.size=size
    ob=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(ob);ob.location=loc;aim(ob,(0,0,1))
light('Gold key', (1,-4,7),450,(1,.82,.62),5)
light('Indigo fill',(-2,4,4),260,(.48,.60,.88),4)
light('Broad rim',(-4,-1,5),325,(1,.86,.70),3)
bpy.ops.object.camera_add(location=(5,-6,3.7));cam=bpy.context.object;aim(cam,(0,0,1.05));cam.data.type='ORTHO';cam.data.ortho_scale=5.8
s=bpy.context.scene;s.camera=cam;s.render.engine='CYCLES';s.cycles.device='CPU';s.cycles.samples=24;s.cycles.use_denoising=True
s.render.threads_mode='FIXED';s.render.threads=4;s.render.resolution_x=1200;s.render.resolution_y=900;s.render.resolution_percentage=100
s.world.color=(.12,.12,.12);s.view_settings.view_transform='AgX';s.render.image_settings.file_format='PNG'
s['author']='Angelis Pseftis';s['creator']='Angelis Pseftis';s['evidence']='Blender offline review only; not production accepted'
if '--tactical' in args:
    cam.location=(7,-9,12);aim(cam,(0,0,.8));cam.data.ortho_scale=16
    state=state+'_tactical'
if textured: bpy.ops.file.pack_all()
if '--albedo' in args:
    s.view_settings.view_transform='Standard'
    for mat in (stone,amber):
        nodes=mat.node_tree.nodes;links=mat.node_tree.links
        texnode=next(n for n in nodes if n.type=='TEX_IMAGE' and 'BaseColor' in n.image.name)
        em=nodes.new('ShaderNodeEmission');links.new(texnode.outputs['Color'],em.inputs['Color'])
        links.new(em.outputs[0],nodes.get('Material Output').inputs['Surface'])
    state=state+'_albedo'
s.render.filepath=str(root/(state+'_threequarter.png'));bpy.ops.wm.save_as_mainfile(filepath=str(root/(state+'_review.blend')));bpy.ops.render.render(write_still=True)
if '--tactical' not in args:
    cam.location=(0,-7,2.4);aim(cam,(0,0,1.05));s.render.filepath=str(root/(state+'_side.png'));bpy.ops.render.render(write_still=True)
