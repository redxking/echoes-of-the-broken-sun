"""Validate and import prepared narrative voice WAVs as Unreal SoundWave assets.

Run inside Unreal Editor Python only after ``prepare_m01_voice_bindings.py
--write`` succeeds. This importer performs no synthesis and does not qualify
voice performance, mix, subtitle synchronization, or listening quality.

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import math
import sys
from pathlib import Path
from typing import Any

import unreal


SCRIPTS_DIR = Path(__file__).resolve().parent
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import prepare_m01_voice_bindings as bindings  # noqa: E402


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
MANIFEST_PATH = PROJECT_ROOT / bindings.OUTPUT_RELATIVE
MANIFEST_SIDECAR = Path(f"{MANIFEST_PATH}.sha256")


def _manifest_digest() -> str:
    if not MANIFEST_PATH.is_file() or not MANIFEST_SIDECAR.is_file():
        raise RuntimeError(
            "Prepared voice manifest or SHA-256 sidecar is absent; run "
            "Scripts/prepare_m01_voice_bindings.py --write first"
        )
    actual = bindings.sha256_file(MANIFEST_PATH)
    recorded = MANIFEST_SIDECAR.read_text(encoding="ascii").strip()
    if recorded != actual or len(recorded) != 64:
        raise RuntimeError("Prepared voice manifest SHA-256 sidecar mismatch")
    return actual


def _package_path(object_path: str) -> str:
    package, separator, object_name = object_path.rpartition(".")
    if not separator or not package.startswith(f"{bindings.ASSET_ROOT}/"):
        raise RuntimeError(f"Invalid narrative voice asset object path: {object_path}")
    if package.rsplit("/", 1)[-1] != object_name:
        raise RuntimeError(f"Asset package/object names differ: {object_path}")
    return package


def _expected_tags(line: dict[str, Any], manifest: dict[str, Any], manifest_sha256: str) -> dict[str, str]:
    return {
        "Echoes.Creator": "Angelis Pseftis",
        "Echoes.Author": "Angelis Pseftis",
        "Echoes.Provenance": manifest["candidate_manifest_path"],
        "Echoes.CandidateManifestSha256": manifest["candidate_manifest_sha256"],
        "Echoes.LicensePointer": manifest["provenance"]["license_pointer"],
        "Echoes.LicensePointerSha256": manifest["provenance"]["license_pointer_sha256"],
        "Echoes.AudioCategory": "Dialogue",
        "Echoes.RuntimeAuthority": "Presentation only",
        "Echoes.Status": "Voice candidate; pending directed listening qualification",
        "Echoes.LineId": line["line_id"],
        "Echoes.SpeakerId": line["speaker_id"],
        "Echoes.Speaker": line["speaker"],
        "Echoes.VoiceHook": line["voice_hook"],
        "Echoes.VoiceId": line["voice_id"],
        "Echoes.VoiceSpeed": format(line["speed"], ".12g"),
        "Echoes.SourceTextSha256": line["text_sha256"],
        "Echoes.WavSha256": line["wav_sha256"],
        "Echoes.DurationSeconds": format(line["duration_seconds"], ".12g"),
        "Echoes.NarrativePackSha256": manifest["pack_sha256"],
        "Echoes.NarrativeSourceSha256": manifest["source_sha256"],
        "Echoes.BindingManifestSha256": manifest_sha256,
        "Echoes.CandidateSource": line["candidate_source_path"],
        # Keyed to the mastered bytes: a re-master must produce a new
        # revision, or the importer reuses the stale asset silently.
        "Echoes.AssetRevision": f"m01-voice-{line.get('mastered_sha256') or line['wav_sha256']}",
    }


_BINDING_REVISION_TAGS = (
    "Echoes.NarrativePackSha256",
    "Echoes.NarrativeSourceSha256",
    "Echoes.BindingManifestSha256",
)


def _sound_duration(sound: unreal.SoundWave, line_id: str) -> float:
    try:
        duration = float(sound.get_editor_property("duration"))
    except (TypeError, ValueError, RuntimeError) as exc:
        raise RuntimeError(f"{line_id}: imported SoundWave duration is unavailable") from exc
    if not math.isfinite(duration) or duration <= 0.0 or duration > 60.0:
        raise RuntimeError(f"{line_id}: imported SoundWave duration is invalid: {duration}")
    return duration


def _validate_sound_identity(
    sound: unreal.SoundWave,
    line: dict[str, Any],
    expected_tags: dict[str, str],
    *,
    allow_binding_revision_refresh: bool = False,
) -> None:
    for tag, expected in expected_tags.items():
        actual = unreal.EditorAssetLibrary.get_metadata_tag(sound, tag)
        if allow_binding_revision_refresh and tag in _BINDING_REVISION_TAGS:
            continue
        if actual != expected:
            raise RuntimeError(
                f"{line['line_id']}: existing SoundWave metadata mismatch for {tag}: "
                f"recorded={actual!r} expected={expected!r}"
            )
    duration = _sound_duration(sound, line["line_id"])
    # SoundWave duration is stored as a float; tolerate only sub-millisecond
    # serialization variance from the exact WAV frame count.
    if not math.isclose(duration, line["duration_seconds"], rel_tol=0.0, abs_tol=0.001):
        raise RuntimeError(
            f"{line['line_id']}: existing SoundWave duration differs from manifest "
            f"({duration} vs {line['duration_seconds']})"
        )
    if bool(sound.get_editor_property("looping")):
        raise RuntimeError(f"{line['line_id']}: narrative voice SoundWave must not loop")


def _refresh_binding_revision_tags(
    sound: unreal.SoundWave,
    line: dict[str, Any],
    expected_tags: dict[str, str],
) -> bool:
    """Refresh pack-level identity after the immutable voice identity validates."""

    _validate_sound_identity(
        sound,
        line,
        expected_tags,
        allow_binding_revision_refresh=True,
    )
    changed = False
    for tag in _BINDING_REVISION_TAGS:
        expected = expected_tags[tag]
        if unreal.EditorAssetLibrary.get_metadata_tag(sound, tag) != expected:
            unreal.EditorAssetLibrary.set_metadata_tag(sound, tag, expected)
            changed = True
    if changed:
        if not unreal.EditorAssetLibrary.save_loaded_asset(sound, False):
            raise RuntimeError(
                f"{line['line_id']}: could not save refreshed binding metadata"
            )
        _validate_sound_identity(sound, line, expected_tags)
    return changed


def import_line(
    line: dict[str, Any],
    manifest: dict[str, Any],
    manifest_sha256: str,
) -> tuple[unreal.SoundWave, str]:
    object_path = line["asset_path"]
    package_path = _package_path(object_path)
    expected_tags = _expected_tags(line, manifest, manifest_sha256)
    existing = (
        unreal.EditorAssetLibrary.load_asset(package_path)
        if unreal.EditorAssetLibrary.does_asset_exist(package_path)
        else None
    )
    if existing is not None:
        if not isinstance(existing, unreal.SoundWave):
            raise RuntimeError(f"Voice asset path is not a SoundWave: {package_path}")
        refreshed = _refresh_binding_revision_tags(
            existing, line, expected_tags
        )
        return existing, (
            "refreshed_binding_metadata" if refreshed else "reused_verified"
        )

    source_path = (PROJECT_ROOT / line["wav_path"]).resolve()
    expected_wav_sha256 = line.get("mastered_sha256") or line["wav_sha256"]
    if bindings.sha256_file(source_path) != expected_wav_sha256:
        raise RuntimeError(f"{line['line_id']}: WAV changed after manifest validation")
    asset_name = line["voice_hook"]
    task = unreal.AssetImportTask()
    task.filename = str(source_path)
    task.destination_path = bindings.ASSET_ROOT
    task.destination_name = asset_name
    task.replace_existing = False
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported_paths = list(task.get_editor_property("imported_object_paths"))
    if len(imported_paths) != 1:
        raise RuntimeError(
            f"{line['line_id']}: Unreal import returned {len(imported_paths)} objects"
        )
    sound = unreal.load_asset(imported_paths[0])
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError(f"{line['line_id']}: imported object is not a SoundWave")
    if sound.get_path_name() != object_path:
        raise RuntimeError(
            f"{line['line_id']}: imported object path differs from manifest "
            f"({sound.get_path_name()} vs {object_path})"
        )
    sound.set_editor_property("looping", False)
    duration = _sound_duration(sound, line["line_id"])
    if not math.isclose(duration, line["duration_seconds"], rel_tol=0.0, abs_tol=0.001):
        raise RuntimeError(
            f"{line['line_id']}: imported duration differs from validated WAV "
            f"({duration} vs {line['duration_seconds']})"
        )
    for tag, value in expected_tags.items():
        unreal.EditorAssetLibrary.set_metadata_tag(sound, tag, value)
    if not unreal.EditorAssetLibrary.save_loaded_asset(sound, False):
        raise RuntimeError(f"{line['line_id']}: could not save imported SoundWave")
    _validate_sound_identity(sound, line, expected_tags)
    return sound, "imported_candidate"


def main() -> None:
    manifest_sha256 = _manifest_digest()
    manifest = bindings.load_json(MANIFEST_PATH)
    # Validate the complete set before loading or importing any asset.
    bindings.validate_prepared_manifest(manifest, PROJECT_ROOT)
    imported = 0
    reused = 0
    refreshed = 0
    for line in manifest["lines"]:
        sound, action = import_line(line, manifest, manifest_sha256)
        if not isinstance(sound, unreal.SoundWave):
            raise RuntimeError(f"{line['line_id']}: final asset audit failed")
        imported += int(action == "imported_candidate")
        reused += int(action == "reused_verified")
        refreshed += int(action == "refreshed_binding_metadata")
        unreal.log(
            f"[ECHOES_NARRATIVE_VOICE] line={line['line_id']} "
            f"asset={line['asset_path']} action={action} wav_sha256={line['wav_sha256']}"
        )
    if imported + reused + refreshed != len(manifest["lines"]):
        raise RuntimeError("Narrative voice import count mismatch")
    unreal.log(
        "[ECHOES_NARRATIVE_VOICE_READY] "
        f"lines={len(manifest['lines'])} imported={imported} reused_verified={reused} "
        f"binding_metadata_refreshed={refreshed} "
        f"manifest_sha256={manifest_sha256} listening_qualified=false"
    )


if __name__ == "__main__":
    main()
