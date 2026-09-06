#!/usr/bin/env python3
"""Validate retained M01 voice candidates and optionally prepare import sources.

The default mode is read-only. Pass ``--write`` to copy the already-generated,
retained WAV candidates into the registered audio source tree and write the
deterministic runtime/import manifest plus its SHA-256 sidecar.

This tool verifies source and candidate identity. It does not generate voice,
qualify audio, register assets, import into Unreal, or establish listening or
runtime acceptance.

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import shutil
import tempfile
import wave
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
MAX_JSON_BYTES = 2_000_000
MISSION_OPERATION = "CampaignPrologue"
MISSION_ID = "WhatTheLedgerKeeps"
SOURCE_RELATIVE = Path("Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json")
PACK_RELATIVE = Path("Content/Narrative/Generated/EchoesNarrativePack.json")
PACK_SIDECAR_RELATIVE = Path("Content/Narrative/Generated/EchoesNarrativePack.json.sha256")
OUTPUT_RELATIVE = Path("Content/Audio/Source/Narrative/m01_voice_bindings.json")
CANDIDATE_RELATIVE = Path(
    "BuildArtifacts/Evidence/world-map-concept-pass/m01-voice-candidates-v2"
)
PROVENANCE_RELATIVE = Path(
    "BuildArtifacts/Evidence/world-map-concept-pass/kokoro-provenance/README.md"
)
ASSET_ROOT = "/Game/Audio/Voice"
EXPECTED_LINE_COUNT = 28
EXPECTED_AUDIO = {"sample_rate_hz": 48_000, "channels": 1, "subtype": "PCM_24"}
VOICE_PINS = {
    "spk_mara_vey": ("Mara Vey", "af_sarah", 1.0),
    "spk_talar_venn": ("Talar Venn", "am_michael", 1.0),
    "spk_oruun_seven_stones": ("Oruun-of-Seven-Stones", "bm_george", 0.92),
}


class VoiceBindingError(RuntimeError):
    """A source, candidate, manifest, or audio identity check failed."""


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise VoiceBindingError(f"duplicate JSON key: {key!r}")
        result[key] = value
    return result


def load_json(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise VoiceBindingError(f"required JSON file is absent: {path}")
    size = path.stat().st_size
    if size <= 0 or size > MAX_JSON_BYTES:
        raise VoiceBindingError(f"JSON size outside (0, {MAX_JSON_BYTES}]: {path}")
    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=lambda token: (_ for _ in ()).throw(
                VoiceBindingError(f"non-finite JSON number: {token}")
            ),
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise VoiceBindingError(f"could not load JSON {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise VoiceBindingError(f"expected JSON object: {path}")
    return value


def _require(value: Any, expected_type: type, path: str) -> Any:
    if expected_type is int and isinstance(value, bool):
        raise VoiceBindingError(f"{path}: expected int")
    if not isinstance(value, expected_type):
        raise VoiceBindingError(f"{path}: expected {expected_type.__name__}")
    return value


def _exact_keys(value: Any, expected: set[str], path: str) -> dict[str, Any]:
    record = _require(value, dict, path)
    actual = set(record)
    if actual != expected:
        raise VoiceBindingError(
            f"{path}: fields differ (missing={sorted(expected - actual)}, "
            f"unknown={sorted(actual - expected)})"
        )
    return record


def _safe_child(root: Path, relative: str | Path, label: str) -> Path:
    root = root.resolve()
    relative_path = Path(relative)
    if relative_path.is_absolute():
        raise VoiceBindingError(f"{label}: expected a relative path")
    resolved = (root / relative_path).resolve()
    try:
        resolved.relative_to(root)
    except ValueError as exc:
        raise VoiceBindingError(f"{label}: path escapes its root") from exc
    return resolved


def _index_unique(records: Any, key: str, path: str) -> dict[str, dict[str, Any]]:
    items = _require(records, list, path)
    result: dict[str, dict[str, Any]] = {}
    for index, item in enumerate(items):
        record = _require(item, dict, f"{path}[{index}]")
        identifier = _require(record.get(key), str, f"{path}[{index}].{key}")
        if identifier in result:
            raise VoiceBindingError(f"{path}[{index}].{key}: duplicate {identifier!r}")
        result[identifier] = record
    return result


def inspect_wav(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise VoiceBindingError(f"candidate WAV is absent: {path}")
    try:
        with wave.open(str(path), "rb") as stream:
            channels = stream.getnchannels()
            sample_width = stream.getsampwidth()
            sample_rate = stream.getframerate()
            frames = stream.getnframes()
            compression = stream.getcomptype()
    except (OSError, EOFError, wave.Error) as exc:
        raise VoiceBindingError(f"invalid candidate WAV {path}: {exc}") from exc
    if channels != EXPECTED_AUDIO["channels"]:
        raise VoiceBindingError(f"candidate WAV must be mono: {path}")
    if sample_width != 3:
        raise VoiceBindingError(f"candidate WAV must be PCM 24-bit: {path}")
    if sample_rate != EXPECTED_AUDIO["sample_rate_hz"]:
        raise VoiceBindingError(f"candidate WAV must be 48000 Hz: {path}")
    if frames <= 0 or compression != "NONE":
        raise VoiceBindingError(f"candidate WAV must be nonempty uncompressed PCM: {path}")
    return {
        "sample_rate_hz": sample_rate,
        "channels": channels,
        "subtype": "PCM_24",
        "frames": frames,
        "duration_seconds": frames / sample_rate,
    }


def _load_current_contract(project_root: Path) -> tuple[dict[str, Any], dict[str, Any], str, str]:
    source_path = project_root / SOURCE_RELATIVE
    pack_path = project_root / PACK_RELATIVE
    source = load_json(source_path)
    pack = load_json(pack_path)
    source_sha256 = sha256_file(source_path)
    pack_sha256 = sha256_file(pack_path)

    sidecar_path = project_root / PACK_SIDECAR_RELATIVE
    if not sidecar_path.is_file() or sidecar_path.read_text(encoding="utf-8").strip() != pack_sha256:
        raise VoiceBindingError("current narrative pack SHA-256 sidecar does not match the pack")
    binding = _require(source.get("runtime_binding"), dict, "source.runtime_binding")
    if binding.get("mission_id") != MISSION_ID or binding.get("operation_mode") != MISSION_OPERATION:
        raise VoiceBindingError("source mission identity does not match M01 CampaignPrologue")
    operations = _require(pack.get("operations"), dict, "pack.operations")
    operation = _require(operations.get(MISSION_OPERATION), dict, f"pack.operations.{MISSION_OPERATION}")
    if operation.get("mission_id") != MISSION_ID:
        raise VoiceBindingError("current narrative pack M01 mission identity differs from source")
    return source, operation, source_sha256, pack_sha256


def build_manifest(
    project_root: Path,
    candidate_root: Path | None = None,
    candidate_manifest_override: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """Return a fully validated deterministic manifest without writing files."""
    project_root = project_root.resolve()
    candidate_root = (candidate_root or (project_root / CANDIDATE_RELATIVE)).resolve()
    candidate_manifest_path = candidate_root / "manifest.json"
    candidate_manifest = (
        candidate_manifest_override
        if candidate_manifest_override is not None
        else load_json(candidate_manifest_path)
    )
    source, operation, source_sha256, pack_sha256 = _load_current_contract(project_root)

    source_speakers = _index_unique(source.get("speakers"), "id", "source.speakers")
    source_triggers = _index_unique(source.get("triggers"), "id", "source.triggers")
    source_lines = _index_unique(source.get("lines"), "id", "source.lines")
    pack_lines = _index_unique(operation.get("lines"), "id", "pack.operation.lines")
    candidate_lines = _index_unique(candidate_manifest.get("lines"), "line_id", "candidate.lines")
    expected_ids = set(source_lines)
    if len(expected_ids) != EXPECTED_LINE_COUNT:
        raise VoiceBindingError(f"source M01 line count must be {EXPECTED_LINE_COUNT}")
    for label, records in (("pack", pack_lines), ("candidate", candidate_lines)):
        if set(records) != expected_ids:
            raise VoiceBindingError(
                f"{label} line IDs differ from source "
                f"(missing={sorted(expected_ids - set(records))}, "
                f"unknown={sorted(set(records) - expected_ids)})"
            )

    rights = _require(candidate_manifest.get("rights"), dict, "candidate.rights")
    provenance_path = project_root / PROVENANCE_RELATIVE
    if not provenance_path.is_file():
        raise VoiceBindingError(f"retained provenance/license pointer is absent: {provenance_path}")
    provenance_sha256 = sha256_file(provenance_path)
    if rights.get("provenance_receipt_sha256") != provenance_sha256:
        raise VoiceBindingError("candidate provenance/license pointer SHA-256 mismatch")

    bindings: list[dict[str, Any]] = []
    asset_paths: set[str] = set()
    wav_paths: set[str] = set()
    for ordinal, line_id in enumerate(source_lines, start=1):
        source_line = source_lines[line_id]
        pack_line = pack_lines[line_id]
        candidate = candidate_lines[line_id]
        speaker_id = _require(source_line.get("speaker_id"), str, f"source.lines.{line_id}.speaker_id")
        if speaker_id not in VOICE_PINS or speaker_id not in source_speakers:
            raise VoiceBindingError(f"{line_id}: speaker is not one of the approved M01 voice pins")
        speaker, voice_id, speed = VOICE_PINS[speaker_id]
        if source_speakers[speaker_id].get("display_name") != speaker:
            raise VoiceBindingError(f"{line_id}: source speaker identity differs from approved pin")
        text = _require(source_line.get("source_text"), str, f"source.lines.{line_id}.source_text")
        trigger_id = _require(source_line.get("trigger_id"), str, f"source.lines.{line_id}.trigger_id")
        source_signal = _require(
            source_triggers.get(trigger_id, {}).get("runtime_signal"),
            str,
            f"source.triggers.{trigger_id}.runtime_signal",
        )
        voice_hook = _require(
            _require(source_line.get("voice_hook"), dict, f"source.lines.{line_id}.voice_hook").get("id"),
            str,
            f"source.lines.{line_id}.voice_hook.id",
        )
        if pack_line.get("speaker") != speaker or pack_line.get("text") != text:
            raise VoiceBindingError(f"{line_id}: current pack speaker/text differs from source")
        runtime_signal = _require(pack_line.get("signal"), str, f"pack.lines.{line_id}.signal")
        if runtime_signal != source_signal:
            raise VoiceBindingError(f"{line_id}: current pack signal differs from current source trigger")

        if candidate.get("speaker") != speaker:
            raise VoiceBindingError(f"{line_id}: candidate speaker differs from current source")
        if candidate.get("text") != text:
            raise VoiceBindingError(f"{line_id}: candidate text differs from current source")
        if candidate.get("text_sha256") != sha256_text(text):
            raise VoiceBindingError(f"{line_id}: candidate text SHA-256 mismatch")
        if candidate.get("logical_audio_hook") != voice_hook:
            raise VoiceBindingError(f"{line_id}: candidate logical audio hook differs from source")
        if candidate.get("voice") != voice_id or not math.isclose(
            float(candidate.get("speed", -1.0)), speed, rel_tol=0.0, abs_tol=1e-9
        ):
            raise VoiceBindingError(f"{line_id}: candidate voice pin differs from approved profile")

        candidate_wav = _safe_child(candidate_root, candidate.get("path", ""), f"{line_id}.path")
        wav_info = inspect_wav(candidate_wav)
        wav_sha256 = sha256_file(candidate_wav)
        if candidate.get("sha256_file") != wav_sha256:
            raise VoiceBindingError(f"{line_id}: candidate WAV SHA-256 mismatch")
        for key in ("sample_rate_hz", "channels", "subtype", "frames"):
            if candidate.get(key) != wav_info[key]:
                raise VoiceBindingError(f"{line_id}: candidate WAV {key} metadata mismatch")
        if not math.isclose(
            float(candidate.get("duration_seconds", -1.0)),
            wav_info["duration_seconds"],
            rel_tol=0.0,
            abs_tol=1e-9,
        ):
            raise VoiceBindingError(f"{line_id}: candidate WAV duration mismatch")

        wav_relative = Path("Content/Audio/Source/Narrative") / f"{voice_hook}.wav"
        asset_path = f"{ASSET_ROOT}/{voice_hook}.{voice_hook}"
        if asset_path in asset_paths or wav_relative.as_posix() in wav_paths:
            raise VoiceBindingError(f"{line_id}: duplicate voice asset or WAV path")
        asset_paths.add(asset_path)
        wav_paths.add(wav_relative.as_posix())
        bindings.append(
            {
                "ordinal": ordinal,
                "line_id": line_id,
                "speaker_id": speaker_id,
                "speaker": speaker,
                "text": text,
                "text_sha256": sha256_text(text),
                "runtime_signal": runtime_signal,
                "voice_hook": voice_hook,
                "voice_id": voice_id,
                "speed": speed,
                "wav_path": wav_relative.as_posix(),
                "wav_sha256": wav_sha256,
                "duration_seconds": wav_info["duration_seconds"],
                "sample_rate_hz": wav_info["sample_rate_hz"],
                "channels": wav_info["channels"],
                "subtype": wav_info["subtype"],
                "asset_path": asset_path,
                "candidate_source_path": candidate_wav.relative_to(candidate_root).as_posix(),
                "candidate_status": "unqualified_pending_listening_and_asset_registration",
            }
        )

    return {
        "schema_version": SCHEMA_VERSION,
        "author": "Angelis Pseftis",
        "status": "prepared_candidates_not_imported_or_listening_qualified",
        "pack_path": PACK_RELATIVE.as_posix(),
        "pack_sha256": pack_sha256,
        "source_path": SOURCE_RELATIVE.as_posix(),
        "source_sha256": source_sha256,
        "candidate_manifest_path": (
            candidate_manifest_path.relative_to(project_root).as_posix()
            if candidate_manifest_path.is_relative_to(project_root)
            else str(candidate_manifest_path)
        ),
        "candidate_manifest_sha256": sha256_file(candidate_manifest_path),
        "provenance": {
            "candidate_root": (
                candidate_root.relative_to(project_root).as_posix()
                if candidate_root.is_relative_to(project_root)
                else str(candidate_root)
            ),
            "license_pointer": PROVENANCE_RELATIVE.as_posix(),
            "license_pointer_sha256": provenance_sha256,
            "rights_status": rights.get("status"),
            "listening_status": candidate_manifest.get("listening_status"),
        },
        "lines": bindings,
    }


MANIFEST_KEYS = {
    "schema_version", "author", "status", "pack_path", "pack_sha256", "source_path",
    "source_sha256", "candidate_manifest_path", "candidate_manifest_sha256", "provenance", "lines",
}
LINE_KEYS = {
    "ordinal", "line_id", "speaker_id", "speaker", "text", "text_sha256", "runtime_signal",
    "voice_hook", "voice_id", "speed", "wav_path", "wav_sha256", "duration_seconds",
    "sample_rate_hz", "channels", "subtype", "asset_path", "candidate_source_path", "candidate_status",
}


def validate_prepared_manifest(
    manifest: dict[str, Any], project_root: Path, *, verify_prepared_wavs: bool = True
) -> None:
    """Fail closed against current source, pack, provenance, and prepared WAVs."""
    project_root = project_root.resolve()
    _exact_keys(manifest, MANIFEST_KEYS, "manifest")
    if manifest["schema_version"] != SCHEMA_VERSION or manifest["author"] != "Angelis Pseftis":
        raise VoiceBindingError("manifest schema/author identity mismatch")
    if manifest["status"] != "prepared_candidates_not_imported_or_listening_qualified":
        raise VoiceBindingError("manifest status overstates or differs from the preparation contract")
    for field, expected in (("pack_path", PACK_RELATIVE), ("source_path", SOURCE_RELATIVE)):
        if manifest[field] != expected.as_posix():
            raise VoiceBindingError(f"manifest.{field}: unexpected authoritative path")
        actual = sha256_file(_safe_child(project_root, manifest[field], f"manifest.{field}"))
        if manifest[field.replace("path", "sha256")] != actual:
            raise VoiceBindingError(f"manifest.{field}: authoritative SHA-256 mismatch")
    provenance = _exact_keys(
        manifest["provenance"],
        {"candidate_root", "license_pointer", "license_pointer_sha256", "rights_status", "listening_status"},
        "manifest.provenance",
    )
    if provenance["license_pointer"] != PROVENANCE_RELATIVE.as_posix():
        raise VoiceBindingError("manifest provenance/license pointer differs from registered receipt")
    if provenance["license_pointer_sha256"] != sha256_file(project_root / PROVENANCE_RELATIVE):
        raise VoiceBindingError("manifest provenance/license SHA-256 mismatch")
    expected_candidate_root = CANDIDATE_RELATIVE.as_posix()
    expected_candidate_manifest = (CANDIDATE_RELATIVE / "manifest.json").as_posix()
    if provenance["candidate_root"] != expected_candidate_root:
        raise VoiceBindingError("manifest candidate root differs from the retained evidence location")
    if manifest["candidate_manifest_path"] != expected_candidate_manifest:
        raise VoiceBindingError("manifest candidate source path differs from retained evidence")
    candidate_manifest_path = project_root / expected_candidate_manifest
    if manifest["candidate_manifest_sha256"] != sha256_file(candidate_manifest_path):
        raise VoiceBindingError("manifest candidate source SHA-256 mismatch")
    candidate_manifest = load_json(candidate_manifest_path)
    candidate_rights = _require(candidate_manifest.get("rights"), dict, "candidate.rights")
    if provenance["rights_status"] != candidate_rights.get("status"):
        raise VoiceBindingError("manifest rights status differs from retained candidate evidence")
    if provenance["listening_status"] != candidate_manifest.get("listening_status"):
        raise VoiceBindingError("manifest listening status differs from retained candidate evidence")
    candidate_lines = _index_unique(candidate_manifest["lines"], "line_id", "candidate.lines")

    source, operation, source_sha256, pack_sha256 = _load_current_contract(project_root)
    if manifest["source_sha256"] != source_sha256 or manifest["pack_sha256"] != pack_sha256:
        raise VoiceBindingError("manifest source or pack identity is stale")
    source_speakers = _index_unique(source["speakers"], "id", "source.speakers")
    source_lines = _index_unique(source["lines"], "id", "source.lines")
    pack_lines = _index_unique(operation["lines"], "id", "pack.lines")
    lines = _require(manifest["lines"], list, "manifest.lines")
    if len(lines) != EXPECTED_LINE_COUNT:
        raise VoiceBindingError(f"manifest.lines: expected {EXPECTED_LINE_COUNT}")
    ids: set[str] = set()
    assets: set[str] = set()
    for index, raw in enumerate(lines):
        line = _exact_keys(raw, LINE_KEYS, f"manifest.lines[{index}]")
        line_id = _require(line["line_id"], str, f"manifest.lines[{index}].line_id")
        if line_id in ids or line_id not in source_lines or line_id not in pack_lines:
            raise VoiceBindingError(f"manifest.lines[{index}].line_id: duplicate or unknown")
        ids.add(line_id)
        if line["ordinal"] != index + 1:
            raise VoiceBindingError(f"manifest.lines[{index}].ordinal: noncanonical order")
        source_line = source_lines[line_id]
        speaker_id = source_line["speaker_id"]
        speaker = source_speakers[speaker_id]["display_name"]
        pack_line = pack_lines[line_id]
        expected = {
            "speaker_id": speaker_id,
            "speaker": speaker,
            "text": source_line["source_text"],
            "text_sha256": sha256_text(source_line["source_text"]),
            "runtime_signal": pack_line["signal"],
            "voice_hook": source_line["voice_hook"]["id"],
        }
        for field, value in expected.items():
            if line[field] != value:
                raise VoiceBindingError(f"manifest.lines[{index}].{field}: current source mismatch")
        if pack_line["speaker"] != speaker or pack_line["text"] != line["text"]:
            raise VoiceBindingError(f"manifest.lines[{index}]: current pack/source mismatch")
        pinned_speaker, voice_id, speed = VOICE_PINS[speaker_id]
        if speaker != pinned_speaker or line["voice_id"] != voice_id or not math.isclose(
            float(line["speed"]), speed, rel_tol=0.0, abs_tol=1e-9
        ):
            raise VoiceBindingError(f"manifest.lines[{index}]: approved voice pin mismatch")
        candidate = candidate_lines.get(line_id)
        if candidate is None:
            raise VoiceBindingError(f"manifest.lines[{index}]: retained candidate is absent")
        candidate_expected = {
            "speaker": line["speaker"],
            "text": line["text"],
            "text_sha256": line["text_sha256"],
            "logical_audio_hook": line["voice_hook"],
            "voice": line["voice_id"],
            "sha256_file": line["wav_sha256"],
        }
        for field, value in candidate_expected.items():
            if candidate.get(field) != value:
                raise VoiceBindingError(
                    f"manifest.lines[{index}]: retained candidate {field} mismatch"
                )
        if line["candidate_source_path"] != candidate.get("path"):
            raise VoiceBindingError(
                f"manifest.lines[{index}].candidate_source_path: retained candidate mismatch"
            )
        if line["candidate_status"] != "unqualified_pending_listening_and_asset_registration":
            raise VoiceBindingError(f"manifest.lines[{index}].candidate_status: overstates evidence")
        expected_asset = f"{ASSET_ROOT}/{line['voice_hook']}.{line['voice_hook']}"
        if line["asset_path"] != expected_asset or expected_asset in assets:
            raise VoiceBindingError(f"manifest.lines[{index}].asset_path: invalid or duplicate")
        assets.add(expected_asset)
        duration = line["duration_seconds"]
        if isinstance(duration, bool) or not isinstance(duration, (int, float)) or not math.isfinite(duration):
            raise VoiceBindingError(f"manifest.lines[{index}].duration_seconds: invalid")
        if duration <= 0.0 or duration > 60.0:
            raise VoiceBindingError(f"manifest.lines[{index}].duration_seconds: outside bounds")
        expected_wav_path = f"Content/Audio/Source/Narrative/{line['voice_hook']}.wav"
        if line["wav_path"] != expected_wav_path:
            raise VoiceBindingError(f"manifest.lines[{index}].wav_path: unexpected source path")
        if verify_prepared_wavs:
            wav_path = _safe_child(project_root, line["wav_path"], f"manifest.lines[{index}].wav_path")
            wav_info = inspect_wav(wav_path)
            if line["wav_sha256"] != sha256_file(wav_path):
                raise VoiceBindingError(f"manifest.lines[{index}].wav_sha256: mismatch")
            for field in ("sample_rate_hz", "channels", "subtype"):
                if line[field] != wav_info[field]:
                    raise VoiceBindingError(f"manifest.lines[{index}].{field}: WAV mismatch")
            if not math.isclose(
                duration, wav_info["duration_seconds"], rel_tol=0.0, abs_tol=1e-9
            ):
                raise VoiceBindingError(f"manifest.lines[{index}].duration_seconds: WAV mismatch")
    if ids != set(source_lines):
        raise VoiceBindingError("manifest line coverage differs from current M01 source")


def canonical_manifest_bytes(manifest: dict[str, Any]) -> bytes:
    return (json.dumps(manifest, indent=2, sort_keys=True, ensure_ascii=False) + "\n").encode("utf-8")


def _atomic_write(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary_name, path)
    except BaseException:
        try:
            os.unlink(temporary_name)
        except FileNotFoundError:
            pass
        raise


def write_prepared_sources(manifest: dict[str, Any], project_root: Path, candidate_root: Path) -> None:
    project_root = project_root.resolve()
    candidate_root = candidate_root.resolve()
    for line in manifest["lines"]:
        source = _safe_child(candidate_root, line["candidate_source_path"], f"{line['line_id']}.candidate")
        destination = _safe_child(project_root, line["wav_path"], f"{line['line_id']}.wav_path")
        destination.parent.mkdir(parents=True, exist_ok=True)
        if destination.is_file() and sha256_file(destination) == line["wav_sha256"]:
            continue
        descriptor, temporary_name = tempfile.mkstemp(
            prefix=f".{destination.name}.", dir=destination.parent
        )
        os.close(descriptor)
        try:
            shutil.copyfile(source, temporary_name)
            if sha256_file(Path(temporary_name)) != line["wav_sha256"]:
                raise VoiceBindingError(f"{line['line_id']}: copied WAV SHA-256 mismatch")
            os.replace(temporary_name, destination)
        except BaseException:
            try:
                os.unlink(temporary_name)
            except FileNotFoundError:
                pass
            raise

    manifest_path = project_root / OUTPUT_RELATIVE
    payload = canonical_manifest_bytes(manifest)
    digest = hashlib.sha256(payload).hexdigest()
    _atomic_write(manifest_path, payload)
    _atomic_write(Path(f"{manifest_path}.sha256"), f"{digest}\n".encode("ascii"))
    loaded = load_json(manifest_path)
    validate_prepared_manifest(loaded, project_root)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--candidate-root", type=Path)
    parser.add_argument(
        "--write",
        action="store_true",
        help="copy validated retained WAVs and write the manifest plus SHA-256 sidecar",
    )
    arguments = parser.parse_args()
    project_root = arguments.root.resolve()
    candidate_root = (
        arguments.candidate_root.resolve()
        if arguments.candidate_root
        else (project_root / CANDIDATE_RELATIVE).resolve()
    )
    try:
        if arguments.write and candidate_root != (project_root / CANDIDATE_RELATIVE).resolve():
            raise VoiceBindingError(
                "--write requires the registered retained M01 candidate evidence root"
            )
        manifest = build_manifest(project_root, candidate_root)
        if arguments.write:
            write_prepared_sources(manifest, project_root, candidate_root)
            action = "written"
        else:
            action = "validated_read_only"
    except (OSError, ValueError, VoiceBindingError) as exc:
        print(f"M01_VOICE_BINDINGS_FAILED {exc}")
        return 1
    print(
        "M01_VOICE_BINDINGS_OK "
        f"action={action} lines={len(manifest['lines'])} "
        f"pack_sha256={manifest['pack_sha256']} source_sha256={manifest['source_sha256']} "
        "listening_qualified=false imported=false"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
