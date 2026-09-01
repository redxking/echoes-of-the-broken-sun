"""Generate and import every registered Echoes audio source.

Run only through Scripts/generate_audio_assets.sh. The PCM sources are
deterministically synthesized from project code by Scripts/echoes_audio_synth.py
and imported here as ordinary SoundWave assets. They contain no sample,
recording, or model output from a third party.

Synthesis lives in echoes_audio_synth so the content suite can prove
determinism and byte-idempotence without a built editor. This module owns only
the editor-side import, the metadata, and the audit.

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import hashlib
import os
import sys

import unreal

_SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
if _SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, _SCRIPTS_DIR)

import echoes_audio_synth as synth  # noqa: E402


SOURCE_DIR = os.path.join(unreal.Paths.project_content_dir(), "Audio", "Source")
ASSET_ROOT = "/Game/Audio/Generated"


def asset_path(spec: synth.CueSpec) -> str:
    return f"{ASSET_ROOT}/{spec.asset_name}"


def source_path(spec: synth.CueSpec) -> str:
    return os.path.join(SOURCE_DIR, f"{spec.asset_name}.wav")


def import_sound(spec: synth.CueSpec) -> unreal.SoundWave:
    """Import one cue, reusing an asset already at the recorded revision."""
    path = asset_path(spec)
    existing = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else None
    )
    if existing is not None:
        if not isinstance(existing, unreal.SoundWave):
            raise RuntimeError(f"Audio asset path is not a SoundWave: {path}")
        revision = unreal.EditorAssetLibrary.get_metadata_tag(
            existing, "Echoes.AssetRevision"
        )
        category = unreal.EditorAssetLibrary.get_metadata_tag(
            existing, "Echoes.AudioCategory"
        )
        if revision == spec.revision and category == spec.category:
            unreal.log(f"[ECHOES_AUDIO_ASSET] path={path} action=reused")
            return existing
        if not unreal.EditorAssetLibrary.delete_asset(path):
            raise RuntimeError(f"Could not replace audio asset: {path}")

    task = unreal.AssetImportTask()
    task.filename = source_path(spec)
    task.destination_path = ASSET_ROOT
    task.destination_name = spec.asset_name
    task.replace_existing = False
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported_paths = list(task.get_editor_property("imported_object_paths"))
    if not imported_paths:
        raise RuntimeError(f"Unreal did not import {source_path(spec)}")
    sound = unreal.load_asset(imported_paths[0])
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError(f"Imported audio is not a SoundWave: {imported_paths[0]}")

    # A looping bed must loop; a one-shot must not. The flag is part of the
    # asset contract, not a runtime decision.
    sound.set_editor_property("looping", bool(spec.looping))

    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.Creator", "Angelis Pseftis")
    unreal.EditorAssetLibrary.set_metadata_tag(
        sound, "Echoes.Provenance", "Original deterministic project synthesis"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.Role", spec.role)
    unreal.EditorAssetLibrary.set_metadata_tag(
        sound, "Echoes.AudioCategory", spec.category
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        sound, "Echoes.RuntimeAuthority", "Presentation only"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        sound, "Echoes.Status", "Audio candidate; not final mix"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.AssetRevision", spec.revision)
    unreal.EditorAssetLibrary.save_loaded_asset(sound, False)
    return sound


def main() -> None:
    imported: list[unreal.SoundWave] = []
    per_category: dict[str, int] = {name: 0 for name in synth.CATEGORIES}
    revisions: set[str] = set()

    for spec in synth.CUES:
        payload = synth.render(spec)
        action = synth.write_if_changed(source_path(spec), payload)
        digest = hashlib.sha256(payload).hexdigest()
        peak, rms = synth.measure_peak_and_rms(payload)
        if peak > synth.PEAK_CEILING + 1e-6:
            raise RuntimeError(
                f"Cue exceeds the true-peak ceiling: {spec.asset_name} peak={peak}"
            )
        unreal.log(
            f"[ECHOES_AUDIO_SOURCE] cue={spec.asset_name} category={spec.category} "
            f"revision={spec.revision} action={action} channels={spec.channels} "
            f"bytes={len(payload)} peak={peak:.5f} rms={rms:.5f} sha256={digest}"
        )
        imported.append(import_sound(spec))
        per_category[spec.category] += 1
        revisions.add(spec.revision)

    if len(imported) != len(synth.CUES) or any(
        not isinstance(asset, unreal.SoundWave) for asset in imported
    ):
        raise RuntimeError("Registered audio audit failed")

    # Dialogue ships as on-screen text with subtitles; the demo authorizes no
    # spoken or synthesized speech, so its category is legitimately empty.
    missing = [
        name
        for name, count in per_category.items()
        if count == 0 and name != synth.CATEGORY_DIALOGUE
    ]
    if missing:
        raise RuntimeError(f"Audio categories with no registered cue: {missing}")

    category_summary = " ".join(
        f"{name}={per_category[name]}" for name in synth.CATEGORIES
    )
    unreal.log(
        "[ECHOES_AUDIO_ASSET_READY] "
        f"cues={len(imported)} revisions={len(revisions)} sourceRate={synth.SAMPLE_RATE} "
        f"{category_summary} "
        "sourcesOriginal=true thirdPartySamples=false "
        "runtimeRoutingValidated=false runtimeConcurrencyValidated=false "
        "finalAudio=false"
    )


if __name__ == "__main__":
    main()
