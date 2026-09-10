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
from pathlib import Path

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import prepare_m01_voice_bindings as bindings

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# load_json takes a Path and calls .is_file() on it; an os.path.join result
# is a str and fails with AttributeError before reading anything.
MANIFEST_PATH = Path(PROJECT_ROOT) / bindings.OUTPUT_RELATIVE

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
    deleted = unreal.EditorAssetLibrary.delete_asset(path)
    # delete_asset has returned True while leaving the .uasset untouched on
    # disk, so the next session re-read the stale revision and the import
    # refused. Trust the filesystem, not the return value: confirm the package
    # is gone and remove the file directly if it is not.
    package_file = os.path.join(
        PROJECT_ROOT, "Content", path.replace("/Game/", "", 1) + ".uasset")
    if os.path.exists(package_file):
        try:
            os.remove(package_file)
            unreal.log(
                f"[ECHOES_VOICE_PURGE_FILE] path={path} "
                f"delete_asset={deleted} removed_on_disk=true"
            )
        except OSError as error:
            raise RuntimeError(
                f"Could not purge stale voice asset {path}: "
                f"delete_asset={deleted}, file remains: {error}"
            )
    elif not deleted:
        raise RuntimeError(f"Could not purge stale voice asset: {path}")
    purged += 1
    unreal.log(
        f"[ECHOES_VOICE_PURGE] path={path} recorded={recorded} "
        f"expected={expected}"
    )
survivors = [
    line["asset_path"].rsplit(".", 1)[0]
    for line in manifest["lines"]
    if os.path.exists(os.path.join(
        PROJECT_ROOT, "Content",
        line["asset_path"].rsplit(".", 1)[0].replace("/Game/", "", 1) + ".uasset"))
    and (line.get("mastered_sha256") or line["wav_sha256"]) not in ("",)
]
stale_survivors = []
for line in manifest["lines"]:
    path = line["asset_path"].rsplit(".", 1)[0]
    package_file = os.path.join(
        PROJECT_ROOT, "Content", path.replace("/Game/", "", 1) + ".uasset")
    if not os.path.exists(package_file):
        continue
    asset = unreal.EditorAssetLibrary.load_asset(path)
    recorded = unreal.EditorAssetLibrary.get_metadata_tag(asset, "Echoes.AssetRevision") if asset else None
    expected = f"m01-voice-{line.get('mastered_sha256') or line['wav_sha256']}"
    if recorded != expected:
        stale_survivors.append(path)
if stale_survivors:
    raise RuntimeError(
        f"{len(stale_survivors)} stale voice asset(s) survived the purge: "
        f"{stale_survivors[:3]}"
    )
unreal.log(f"[ECHOES_VOICE_PURGE_READY] purged={purged} kept={kept}")
