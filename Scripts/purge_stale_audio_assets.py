"""Delete imported audio assets whose recorded revision is stale.

Runs in its own editor session before the import pass: deleting and then
re-importing the same asset path inside one editor session strands the
package name, so the purge and the import never share a session.
Author: Angelis Pseftis.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import echoes_audio_synth as synth

ASSET_ROOT = "/Game/Audio/Generated"

purged = 0
kept = 0
for spec in synth.CUES:
    path = f"{ASSET_ROOT}/{spec.asset_name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    asset = unreal.EditorAssetLibrary.load_asset(path)
    revision = unreal.EditorAssetLibrary.get_metadata_tag(
        asset, "Echoes.AssetRevision"
    )
    if revision == spec.revision:
        kept += 1
        continue
    if not unreal.EditorAssetLibrary.delete_asset(path):
        raise RuntimeError(f"Could not purge stale audio asset: {path}")
    purged += 1
    unreal.log(
        f"[ECHOES_AUDIO_PURGE] path={path} recorded={revision} "
        f"expected={spec.revision}"
    )
unreal.log(f"[ECHOES_AUDIO_PURGE_READY] purged={purged} kept={kept}")
