"""Generate and import the registered Echoes presentation-audio candidates.

Run only through Scripts/generate_audio_assets.sh. The mono PCM sources are
deterministically synthesized from project code and imported as ordinary
SoundWave assets. They do not contain samples or recordings from third parties.
"""

from __future__ import annotations

import hashlib
import io
import math
import os
import struct
import wave
from dataclasses import dataclass
from typing import Callable

import unreal


AUDIO_REVISION = "presentation-audio-v1"
SAMPLE_RATE = 48_000
SOURCE_DIR = os.path.join(unreal.Paths.project_content_dir(), "Audio", "Source")
ASSET_ROOT = "/Game/Audio/Generated"


@dataclass(frozen=True)
class CueSpec:
    asset_name: str
    duration_seconds: float
    role: str
    synth: Callable[[float, int], float]

    @property
    def source_path(self) -> str:
        return os.path.join(SOURCE_DIR, f"{self.asset_name}.wav")

    @property
    def asset_path(self) -> str:
        return f"{ASSET_ROOT}/{self.asset_name}"


def smooth_envelope(t: float, duration: float, attack: float, release: float) -> float:
    attack_gain = min(1.0, max(0.0, t / attack)) if attack > 0.0 else 1.0
    remaining = max(0.0, duration - t)
    release_gain = min(1.0, remaining / release) if release > 0.0 else 1.0
    return attack_gain * attack_gain * release_gain * release_gain


def command_confirm(t: float, _: int) -> float:
    duration = 0.14
    envelope = smooth_envelope(t, duration, 0.008, 0.07)
    primary = math.sin(math.tau * 720.0 * t)
    upper = math.sin(math.tau * 1080.0 * t + 0.35)
    return 0.38 * envelope * (0.72 * primary + 0.28 * upper)


def meridian_destruction(t: float, sample_index: int) -> float:
    duration = 0.46
    progress = min(1.0, t / duration)
    phase = math.tau * (430.0 * t - 125.0 * t * t / duration)
    envelope = smooth_envelope(t, duration, 0.012, 0.24)
    shimmer = math.sin(math.tau * 760.0 * t + 0.2 * math.sin(math.tau * 17.0 * t))
    pulse = math.sin(phase) + 0.32 * math.sin(2.01 * phase + 0.4)
    deterministic_grit = (((sample_index * 1103515245 + 12345) >> 16) & 0x7FFF) / 16384.0 - 1.0
    return 0.46 * envelope * (0.68 * pulse + 0.18 * shimmer + 0.06 * deterministic_grit)


def kharuun_destruction(t: float, sample_index: int) -> float:
    duration = 0.52
    progress = min(1.0, t / duration)
    frequency = 250.0 - 135.0 * progress
    phase = math.tau * (250.0 * t - 67.5 * t * t / duration)
    envelope = smooth_envelope(t, duration, 0.014, 0.29)
    ceramic = math.sin(1.51 * phase + 0.7) + 0.42 * math.sin(2.47 * phase)
    knock = math.sin(math.tau * frequency * t) * math.exp(-10.0 * t)
    deterministic_grit = (((sample_index * 214013 + 2531011) >> 16) & 0x7FFF) / 16384.0 - 1.0
    return 0.48 * envelope * (0.57 * ceramic + 0.28 * knock + 0.055 * deterministic_grit)


CUES = (
    CueSpec("SFX_CommandConfirm", 0.14, "Accepted local command confirmation", command_confirm),
    CueSpec("SFX_DestructionMeridian", 0.46, "Meridian functional-loss confirmation", meridian_destruction),
    CueSpec("SFX_DestructionKharuun", 0.52, "Kharuun functional-loss confirmation", kharuun_destruction),
)


def build_wave_bytes(spec: CueSpec) -> bytes:
    frame_count = int(round(spec.duration_seconds * SAMPLE_RATE))
    payload = io.BytesIO()
    with wave.open(payload, "wb") as writer:
        writer.setnchannels(1)
        writer.setsampwidth(2)
        writer.setframerate(SAMPLE_RATE)
        frames = bytearray()
        for sample_index in range(frame_count):
            t = sample_index / SAMPLE_RATE
            sample = max(-0.96, min(0.96, spec.synth(t, sample_index)))
            frames.extend(struct.pack("<h", int(round(sample * 32767.0))))
        writer.writeframes(bytes(frames))
    return payload.getvalue()


def write_if_changed(path: str, payload: bytes) -> str:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.exists(path):
        with open(path, "rb") as existing:
            if existing.read() == payload:
                return "reused"
    temporary = f"{path}.tmp"
    with open(temporary, "wb") as output:
        output.write(payload)
    os.replace(temporary, path)
    return "written"


def import_sound(spec: CueSpec) -> unreal.SoundWave:
    existing = (
        unreal.EditorAssetLibrary.load_asset(spec.asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(spec.asset_path)
        else None
    )
    if existing is not None:
        if not isinstance(existing, unreal.SoundWave):
            raise RuntimeError(f"Audio asset path is not a SoundWave: {spec.asset_path}")
        revision = unreal.EditorAssetLibrary.get_metadata_tag(
            existing, "Echoes.AssetRevision"
        )
        if revision == AUDIO_REVISION:
            unreal.log(f"[ECHOES_AUDIO_ASSET] path={spec.asset_path} action=reused")
            return existing
        if not unreal.EditorAssetLibrary.delete_asset(spec.asset_path):
            raise RuntimeError(f"Could not replace audio asset: {spec.asset_path}")

    task = unreal.AssetImportTask()
    task.filename = spec.source_path
    task.destination_path = ASSET_ROOT
    task.destination_name = spec.asset_name
    task.replace_existing = False
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported_paths = list(task.get_editor_property("imported_object_paths"))
    if not imported_paths:
        raise RuntimeError(f"Unreal did not import {spec.source_path}")
    sound = unreal.load_asset(imported_paths[0])
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError(f"Imported audio is not a SoundWave: {imported_paths[0]}")
    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.Creator", "Angelis Pseftis")
    unreal.EditorAssetLibrary.set_metadata_tag(
        sound, "Echoes.Provenance", "Original deterministic project synthesis"
    )
    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.Role", spec.role)
    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.RuntimeAuthority", "Presentation only")
    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.Status", "Audio candidate; not final mix")
    unreal.EditorAssetLibrary.set_metadata_tag(sound, "Echoes.AssetRevision", AUDIO_REVISION)
    unreal.EditorAssetLibrary.save_loaded_asset(sound, False)
    return sound


def main() -> None:
    imported: list[unreal.SoundWave] = []
    for spec in CUES:
        payload = build_wave_bytes(spec)
        action = write_if_changed(spec.source_path, payload)
        digest = hashlib.sha256(payload).hexdigest()
        unreal.log(
            f"[ECHOES_AUDIO_SOURCE] cue={spec.asset_name} action={action} "
            f"bytes={len(payload)} sha256={digest}"
        )
        imported.append(import_sound(spec))

    if len(imported) != 3 or any(not isinstance(asset, unreal.SoundWave) for asset in imported):
        raise RuntimeError("Presentation audio audit failed")
    unreal.log(
        "[ECHOES_AUDIO_READY] "
        f"revision={AUDIO_REVISION} cues=3 sourceRate={SAMPLE_RATE} channels=1 "
        "command2D=true destruction3D=true maxConcurrentBounded=true "
        "runtimeAuthority=presentation thirdPartySamples=false finalAudio=false"
    )


if __name__ == "__main__":
    main()
