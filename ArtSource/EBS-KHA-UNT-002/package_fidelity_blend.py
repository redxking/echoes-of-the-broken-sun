"""Package rigged GLB plus preview materials for editing, not acceptance.
Author: Angelis Pseftis
"""
import bpy,json,pathlib,sys
root=pathlib.Path(sys.argv[sys.argv.index('--')+1])
bpy.ops.wm.open_mainfile(filepath=str(root/'baseline_lod0_review.blend'))
materials=[m.copy() for m in bpy.data.objects['Riftstalker'].data.materials]
for m in materials:m.use_fake_user=True
for obj in list(bpy.data.objects):bpy.data.objects.remove(obj,do_unlink=True)
bpy.ops.import_scene.gltf(filepath=str(root/'baseline_lod0.glb'))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and any(m.type=='ARMATURE' for m in o.modifiers)]
rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE']
assert len(meshes)==1 and len(rigs)==1,([o.name for o in meshes],[o.name for o in rigs])
assert len(rigs[0].data.bones)==22,len(rigs[0].data.bones)
m=meshes[0];m.data.materials.clear()
for mat in materials:m.data.materials.append(mat)
# The glTF importer binds COLOR_0 into its own materials. These review nodes use
# the baked atlas; molt and team animation are deliberately not claimed here.
assert len(m.data.uv_layers)>0
assert len(m.data.color_attributes)>0
socket_names=sorted(o.name for o in bpy.context.scene.objects if o.name.startswith('SOCKET_'))
assert len(socket_names)==5,socket_names
report={'author':'Angelis Pseftis','status':'EDITABLE_PILOT_NOT_PRODUCTION',
 'mesh_count':len(meshes),'bone_count':len(rigs[0].data.bones),
 'sockets':socket_names,'actions':sorted(a.name for a in bpy.data.actions),
 'uv_layers':[u.name for u in m.data.uv_layers],
 'color_attributes':[a.name for a in m.data.color_attributes],
 'limits':'Preview uses baked MRE roughness directly; no shader clamp. No Unreal evidence. Molt/team shaders not bound in Blender.'}
bpy.context.scene['author']='Angelis Pseftis';bpy.context.scene['creator']='Angelis Pseftis'
bpy.context.scene['status']=report['status']
bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Riftstalker_fidelity_editable.blend'))
(root/'blender-import-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
