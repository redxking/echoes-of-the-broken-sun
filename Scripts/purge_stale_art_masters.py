"""Delete art master materials whose recorded revision is stale.

Runs in its own editor session before the art generator (the same
two-session discipline as the audio pipeline): deleting and recreating one
asset path inside a single session strands the package, and rebuilding a
loaded material's expression graph in place asserts on rooted objects.
Author: Angelis Pseftis.
"""
import unreal

STALE_MASTERS = {
    "/Game/Art/Generated/Materials/M_EchoesSurface": "surface-textured-v7",
    "/Game/Art/Generated/Materials/M_EchoesWorldSurface": "world-surface-textured-v6",
}

purged = 0
kept = 0
for path, expected_revision in STALE_MASTERS.items():
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    asset = unreal.EditorAssetLibrary.load_asset(path)
    revision = unreal.EditorAssetLibrary.get_metadata_tag(
        asset, "Echoes.AssetRevision"
    )
    if revision == expected_revision:
        kept += 1
        continue
    if not unreal.EditorAssetLibrary.delete_asset(path):
        raise RuntimeError(f"Could not purge stale art master: {path}")
    purged += 1
    unreal.log(
        f"[ECHOES_ART_PURGE] path={path} recorded={revision} "
        f"expected={expected_revision}"
    )
unreal.log(f"[ECHOES_ART_PURGE_READY] purged={purged} kept={kept}")
