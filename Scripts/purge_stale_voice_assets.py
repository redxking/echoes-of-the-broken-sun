"""Delete imported narrative voice assets whose recorded revision is stale.

Runs in its own editor session before the voice import pass, for the reason
the generated-audio purge records: deleting and then re-importing the same
asset path inside one editor session strands the package name, so the purge
and the import never share a session.

The voice path did not need this until the asset revision became a function
of the mastered audio. It was previously keyed to the candidate digest, which
never changed, so a re-master produced an unchanged revision and would have
been silently reused. Keying it to the mastered bytes is what makes a
re-master visible - and it is what makes this purge necessary.

Author and owner: Angelis Pseftis.
"""
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import prepare_m01_voice_bindings as bindings

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MANIFEST_PATH = os.path.join(PROJECT_ROOT, str(bindings.OUTPUT_RELATIVE))

manifest = bindings.load_json(MANIFEST_PATH)
purged = 0
kept = 0
for line in manifest["lines"]:
    path = line["asset_path"].rsplit(".", 1)[0]
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    asset = unreal.EditorAssetLibrary.load_asset(path)
    recorded = unreal.EditorAssetLibrary.get_metadata_tag(
        asset, "Echoes.AssetRevision"
    )
    expected = f"m01-voice-{line.get('mastered_sha256') or line['wav_sha256']}"
    if recorded == expected:
        kept += 1
        continue
    if not unreal.EditorAssetLibrary.delete_asset(path):
        raise RuntimeError(f"Could not purge stale voice asset: {path}")
    purged += 1
    unreal.log(
        f"[ECHOES_VOICE_PURGE] path={path} recorded={recorded} "
        f"expected={expected}"
    )
unreal.log(f"[ECHOES_VOICE_PURGE_READY] purged={purged} kept={kept}")
