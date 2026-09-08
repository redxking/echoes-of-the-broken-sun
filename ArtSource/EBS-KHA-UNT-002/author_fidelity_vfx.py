"""Author actual sandbox Niagara shard graph. Author: Angelis Pseftis."""
import unreal,os,json
from pathlib import Path
out=Path(os.environ['EBS_VFX_OUT']);out.mkdir(parents=True,exist_ok=True)
root=os.environ.get('EBS_VFX_ROOT','/Game/Echoes/RiftProduction/VFX/Qualified')
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.MaterialEditingLibrary
name='M_EBS_KHA_Shard'
mat=unreal.load_asset(root+'/'+name)
if mat is None:
 mat=tools.create_asset(name,root,unreal.Material,unreal.MaterialFactoryNew())
 mat.set_editor_property('used_with_niagara_sprites',True)
 mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
 mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
 color=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
 color.set_editor_property('constant',unreal.LinearColor(.50,.22,.06,1))
 lib.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 uv=lib.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate)
 shape=lib.create_material_expression(mat,unreal.MaterialExpressionCustom)
 shape.set_editor_property('code','return step(abs(UV.x-0.5)*4.0+abs(UV.y-0.5),0.48);')
 shape.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
 inp=unreal.CustomInput();inp.set_editor_property('input_name','UV');shape.set_editor_property('inputs',[inp])
 lib.connect_material_expressions(uv,'',shape,'UV');lib.connect_material_property(shape,'',unreal.MaterialProperty.MP_OPACITY_MASK)
 lib.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
result=json.loads(unreal.RiftNiagaraAuthoringLibrary.author_shard(root+'/NS_EBS_KHA_RiftShard',mat.get_path_name()))
(out/'niagara-authoring-report.json').write_text(json.dumps(result,indent=2)+'\n')
if not result['success']:raise RuntimeError(str(result['errors']))
