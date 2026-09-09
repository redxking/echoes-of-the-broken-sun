"""Import registered UI source art; author: Angelis Pseftis."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
source = root / "ArtSource/UI/command-bridge-background.png"
if not source.is_file():
    raise RuntimeError(f"Missing registered HUD source: {source}")
task = unreal.AssetImportTask()
task.filename = str(source)
task.destination_path = "/Game/Art/UI"
task.destination_name = "T_EBS_CommandBridge"
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset("/Game/Art/UI/T_EBS_CommandBridge")
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError("Command Bridge import did not produce a Texture2D")
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("srgb", True)
unreal.EditorAssetLibrary.set_metadata_tag(texture, "Echoes.Creator", "Angelis Pseftis")
unreal.EditorAssetLibrary.set_metadata_tag(texture, "Echoes.SourceConcept", "CONCEPT-013")
unreal.EditorAssetLibrary.set_metadata_tag(texture, "Echoes.Source", "ArtSource/UI/command-bridge-background.png")
unreal.EditorAssetLibrary.set_metadata_tag(texture, "Echoes.Status", "Derived menu backdrop; visual review pending")
if texture.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
    raise RuntimeError("Command Bridge UI texture group did not persist")
if not texture.get_editor_property("srgb"):
    raise RuntimeError("Command Bridge color space did not persist")
if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
    raise RuntimeError("Command Bridge texture save failed")
unreal.log("[ECHOES_HUD_ART] Command Bridge Texture2D imported and saved")
