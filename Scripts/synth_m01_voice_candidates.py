#!/usr/bin/env python3
"""Create unbound, review-only M01 Kokoro voice candidates.

This script intentionally reads the authored M01 contract as well as the
digest-verified runtime projection.  The projection owns the runtime text and
speaker triples; the authored contract owns the logical audio hooks which are
not emitted into the runtime projection.  Outputs are evidence candidates,
not registered or runtime-bound audio assets.

Run dry-run first.  Synthesis is explicit with ``--write`` and requires the
project-local Kokoro virtual environment.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
from datetime import UTC, datetime
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_SOURCE = ROOT / "Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json"
DEFAULT_PACK = ROOT / "Content/Narrative/Generated/EchoesNarrativePack.json"
DEFAULT_KOKORO = ROOT.parent / "Tools" / "kokoro"
EXPECTED_PACK_SHA256 = "3e1bea0807f6f40c23f4e0054af5885e7d3e499f4b75551b35e2036429405c3e"
VERIFIED_RELEASE_SHA256 = {
    "kokoro-v1.0.onnx": "7d5df8ecf7d4b1878015a32686053fd0eebe2bc377234608764cc0ef3636a6c5",
    "voices-v1.0.bin": "bca610b8308e8d99f32e6fe4197e7ec01679264efed0cac9140fe9c29f1fbf7d",
}
TARGET_RATE = 48_000
TARGET_SUBTYPE = "PCM_24"
ONNX_RELEASE_URL = "https://github.com/thewh1teagle/kokoro-onnx/releases/tag/model-files-v1.0"
MODEL_CARD_URL = "https://huggingface.co/hexgrad/Kokoro-82M"

# Character bible candidate mapping. These are calibration candidates;
# they remain subject to listening review before any registration or binding.
VOICES = {
    "Mara Vey": ("af_sarah", 1.0),
    "Talar Venn": ("am_michael", 1.0),
    "Oruun-of-Seven-Stones": ("bm_george", 0.92),
}


class PreflightError(RuntimeError):
    """An input, provenance, or output-safety condition was not met."""


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise PreflightError(f"cannot read JSON {path}: {error}") from error
    if not isinstance(data, dict):
        raise PreflightError(f"JSON root must be an object: {path}")
    return data


def git_identity() -> dict[str, str]:
    """Capture the source identity without making repository changes."""
    def read(*command: str) -> str:
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        return result.stdout.strip() if result.returncode == 0 else "UNAVAILABLE"

    return {
        "head": read("git", "rev-parse", "HEAD"),
        "dirty_paths_sha256": hashlib.sha256(
            read("git", "status", "--porcelain=v1").encode("utf-8")
        ).hexdigest(),
    }


def read_weight_record(weights_path: Path) -> dict[str, str]:
    records: dict[str, str] = {}
    try:
        rows = weights_path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise PreflightError(f"cannot read weights record {weights_path}: {error}") from error
    for row in rows:
        fields = row.split()
        if len(fields) != 2 or len(fields[0]) != 64:
            raise PreflightError(f"invalid SHA-256 row in {weights_path}: {row!r}")
        records[fields[1].removeprefix("*")] = fields[0].lower()
    return records


def canonical_lines(source: dict[str, Any], pack: dict[str, Any]) -> list[dict[str, Any]]:
    """Return and validate the 28 authored M01 lines against the projection."""
    source_lines = source.get("lines")
    speakers = source.get("speakers")
    if not isinstance(source_lines, list) or not isinstance(speakers, list):
        raise PreflightError("M01 source must contain lines and speakers arrays")
    names = {speaker.get("id"): speaker.get("display_name") for speaker in speakers}
    if len(names) != 3 or any(not isinstance(name, str) for name in names.values()):
        raise PreflightError("M01 source speaker identity is incomplete")
    try:
        projected = pack["operations"]["CampaignPrologue"]["lines"]
    except (KeyError, TypeError) as error:
        raise PreflightError("compiled pack lacks CampaignPrologue lines") from error
    if not isinstance(projected, list):
        raise PreflightError("compiled CampaignPrologue lines must be an array")
    if len(source_lines) != 28 or len(projected) != 28:
        raise PreflightError(f"M01 requires exactly 28 lines (source={len(source_lines)}, pack={len(projected)})")

    selected: list[dict[str, Any]] = []
    seen_ids: set[str] = set()
    seen_hooks: set[str] = set()
    for index, (authored, runtime) in enumerate(zip(source_lines, projected, strict=True), start=1):
        if not isinstance(authored, dict) or not isinstance(runtime, dict):
            raise PreflightError(f"line {index} is not an object")
        line_id = authored.get("id")
        speaker = names.get(authored.get("speaker_id"))
        text = authored.get("source_text")
        hook = authored.get("voice_hook", {}).get("id") if isinstance(authored.get("voice_hook"), dict) else None
        triple = (runtime.get("id"), runtime.get("speaker"), runtime.get("text"))
        if not all(isinstance(value, str) and value for value in (line_id, speaker, text, hook)):
            raise PreflightError(f"line {index} lacks id, speaker, source text, or voice hook")
        if triple != (line_id, speaker, text):
            raise PreflightError(f"source/pack triple mismatch at line {index}: {line_id}")
        if line_id in seen_ids or hook in seen_hooks:
            raise PreflightError(f"duplicate M01 line or logical hook at line {index}: {line_id} / {hook}")
        if not hook.startswith("aud_m01_vo_"):
            raise PreflightError(f"unexpected M01 voice hook at line {index}: {hook}")
        if speaker not in VOICES:
            raise PreflightError(f"no pinned candidate voice for {speaker!r} ({line_id})")
        voice, speed = VOICES[speaker]
        selected.append({
            "ordinal": index,
            "line_id": line_id,
            "logical_audio_hook": hook,
            "speaker": speaker,
            "text": text,
            "text_sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
            "runtime_signal": runtime.get("signal"),
            "voice": voice,
            "speed": speed,
        })
        seen_ids.add(line_id)
        seen_hooks.add(hook)
    return selected


def preflight(args: argparse.Namespace) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    source = args.source.resolve()
    pack = args.pack.resolve()
    kokoro_dir = args.kokoro_dir.resolve()
    output = args.output.resolve()
    for path in (source, pack):
        if not path.is_file():
            raise PreflightError(f"required input is missing: {path}")
    # Refuse any existing target even in dry-run: production invocation cannot
    # accidentally overwrite earlier candidate evidence.
    if output.exists():
        raise PreflightError(f"refusing existing output path: {output}")
    if args.target_rate != TARGET_RATE or args.pcm_format != TARGET_SUBTYPE:
        raise PreflightError("M01 candidates are fixed at 48 kHz mono PCM_24")
    source_doc = load_json(source)
    pack_doc = load_json(pack)
    pack_sha = sha256_file(pack)
    if pack_sha != args.expected_pack_sha256.lower():
        raise PreflightError(f"compiled pack SHA-256 mismatch: expected {args.expected_pack_sha256}, got {pack_sha}")
    sidecar = pack.with_suffix(pack.suffix + ".sha256")
    if sidecar.is_file():
        recorded = sidecar.read_text(encoding="utf-8").split()[0].lower()
        if recorded != pack_sha:
            raise PreflightError(f"compiled pack sidecar mismatch: {sidecar}")

    weights_file = kokoro_dir / "weights.sha256"
    model = kokoro_dir / "kokoro-v1.0.onnx"
    voices = kokoro_dir / "voices-v1.0.bin"
    records = read_weight_record(weights_file)
    hashes: dict[str, str] = {}
    for artifact in (model, voices):
        if not artifact.is_file():
            raise PreflightError(f"Kokoro artifact is missing: {artifact}")
        expected = records.get(artifact.name)
        actual = sha256_file(artifact)
        if expected is None or expected != actual or actual != VERIFIED_RELEASE_SHA256[artifact.name]:
            raise PreflightError(f"Kokoro artifact hash mismatch: {artifact.name}")
        hashes[artifact.name] = actual
    afconvert = shutil.which("afconvert")
    if not afconvert:
        raise PreflightError("afconvert is required for 24 kHz to 48 kHz PCM conversion")

    selection = canonical_lines(source_doc, pack_doc)
    context = {
        "source": str(source),
        "source_sha256": sha256_file(source),
        "pack": str(pack),
        "pack_sha256": pack_sha,
        "pack_sha256_sidecar": str(sidecar) if sidecar.is_file() else None,
        "weights_record": str(weights_file),
        "model": str(model),
        "voices": str(voices),
        "artifact_sha256": hashes,
        "git": git_identity(),
        "afconvert": afconvert,
        "target": {"sample_rate_hz": TARGET_RATE, "channels": 1, "subtype": TARGET_SUBTYPE},
    }
    return selection, context


def create_kokoro(model: Path, voices: Path, cpu_threads: int):
    """Create a one-session, serial CPU inference path for later --write use."""
    try:
        import onnxruntime as ort
        from kokoro_onnx import Kokoro
    except ImportError as error:
        raise PreflightError(
            "--write requires the project Kokoro venv; use Tools/kokoro/venv/bin/python"
        ) from error
    options = ort.SessionOptions()
    options.intra_op_num_threads = cpu_threads
    options.inter_op_num_threads = 1
    options.execution_mode = ort.ExecutionMode.ORT_SEQUENTIAL
    session = ort.InferenceSession(str(model), sess_options=options, providers=["CPUExecutionProvider"])
    return Kokoro.from_session(session, str(voices)), ort.__version__


def pcm_sha256(path: Path) -> str:
    import soundfile as sf

    samples, _ = sf.read(str(path), dtype="int32", always_2d=True)
    return hashlib.sha256(samples.tobytes()).hexdigest()


def inspect_final_wav(path: Path) -> dict[str, Any]:
    import soundfile as sf

    info = sf.info(str(path))
    if info.format != "WAV" or info.samplerate != TARGET_RATE or info.channels != 1 or info.subtype != TARGET_SUBTYPE:
        raise PreflightError(f"converted audio does not meet 48 kHz mono PCM_24: {path} ({info})")
    return {
        "format": info.format,
        "sample_rate_hz": info.samplerate,
        "channels": info.channels,
        "subtype": info.subtype,
        "frames": info.frames,
        "duration_seconds": info.duration,
        "sha256_file": sha256_file(path),
        "sha256_decoded_pcm_i32": pcm_sha256(path),
    }


def rights_record() -> dict[str, Any]:
    receipt = ROOT / "BuildArtifacts/Evidence/world-map-concept-pass/kokoro-provenance/README.md"
    return {
        "status": "PRIVATE_CANDIDATES_WITH_RETAINED_UPSTREAM_PROVENANCE",
        "official_model_card": MODEL_CARD_URL,
        "official_onnx_distribution_release": ONNX_RELEASE_URL,
        "model_family_license": "Apache-2.0 declared in Hexgrad model card",
        "wrapper_license": "MIT; distinct from model-family license",
        "local_blob_identity": "Both installed artifacts match SHA-256 of streamed official release downloads; retained 2026-09-04.",
        "provenance_receipt": str(receipt),
        "provenance_receipt_sha256": sha256_file(receipt),
        "release_registration": "PENDING asset register, notices and final listening qualification",
    }


def write_candidates(args: argparse.Namespace, selection: list[dict[str, Any]], context: dict[str, Any]) -> None:
    import soundfile as sf

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    wav_dir = output / "wav"
    wav_dir.mkdir()
    try:
        engine, onnxruntime_version = create_kokoro(Path(context["model"]), Path(context["voices"]), args.cpu_threads)
        outputs: list[dict[str, Any]] = []
        with tempfile.TemporaryDirectory(prefix="echoes-m01-voice-", dir=output) as scratch_text:
            scratch = Path(scratch_text)
            for row in selection:
                samples, source_rate = engine.create(row["text"], voice=row["voice"], speed=row["speed"], lang="en-us")
                if source_rate != 24_000:
                    raise PreflightError(f"unexpected Kokoro source rate for {row['line_id']}: {source_rate}")
                temporary = scratch / f"{row['logical_audio_hook']}.24k.wav"
                final = wav_dir / f"{row['logical_audio_hook']}.{row['line_id']}.{row['voice']}.wav"
                import numpy as np
                samples = np.asarray(samples, dtype=np.float64)
                if samples.size == 0 or not np.isfinite(samples).all():
                    raise PreflightError(f"invalid generated samples: {row['line_id']}")
                source_peak = float(np.max(np.abs(samples)))
                if source_peak == 0:
                    raise PreflightError(f"silent generated line: {row['line_id']}")
                # Reserve 3 dB before PCM quantization and sample-rate conversion.
                # This is peak protection only, not final loudness normalization.
                gain = min(1.0, (10.0 ** (-3.0 / 20.0)) / source_peak)
                sf.write(str(temporary), samples * gain, source_rate, subtype=TARGET_SUBTYPE)
                subprocess.run(
                    [context["afconvert"], "-f", "WAVE", "-d", "LEI24@48000", "-c", "1", "-r", "127", str(temporary), str(final)],
                    check=True,
                    capture_output=True,
                    text=True,
                )
                outputs.append({**row, "source_peak_linear": source_peak, "peak_protection_gain": gain, "path": str(final.relative_to(output)), **inspect_final_wav(final)})
                print(f"M01_VOICE_LINE_CREATED {row['ordinal']}/28 {row['line_id']}", flush=True)
        manifest = {
            "schema": "echoes-m01-voice-candidates-v1",
            "author": "Angelis Pseftis",
            "created_utc": datetime.now(UTC).isoformat(),
            "candidate_status": "UNBOUND_PENDING_LISTENING_AND_RIGHTS_REVIEW",
            "not_asset_registered": True,
            "not_runtime_bound": True,
            "listening_status": "PENDING_OWNER_LISTENING_REVIEW",
            "mix_ducking_status": "UNRESOLVED_NOT_SELECTED_BY_THIS_TOOL",
            "rights": rights_record(),
            "environment": {
                "python": sys.version,
                "platform": platform.platform(),
                "onnxruntime": onnxruntime_version,
                "cpu_threads": args.cpu_threads,
                "execution": "one CPU session; serial line synthesis",
                "converter": f"{context['afconvert']} -f WAVE -d LEI24@48000 -c 1 -r 127",
            },
            "inputs": context,
            "lines": outputs,
        }
        (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    except Exception as error:
        # Keep partial evidence without presenting it as a finished candidate set.
        (output / "failure.json").write_text(json.dumps({
            "status": "FAILED_PARTIAL_OUTPUT_NOT_QUALIFIED",
            "failed_utc": datetime.now(UTC).isoformat(),
            "error_type": type(error).__name__,
            "error": str(error),
            "inputs": context,
        }, indent=2) + "\n", encoding="utf-8")
        raise


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--pack", type=Path, default=DEFAULT_PACK)
    parser.add_argument("--expected-pack-sha256", default=EXPECTED_PACK_SHA256)
    parser.add_argument("--kokoro-dir", type=Path, default=Path(os.environ.get("ECHOES_KOKORO_DIR", DEFAULT_KOKORO)))
    parser.add_argument("--output", type=Path, required=True, help="new evidence directory; existing paths are refused")
    parser.add_argument("--target-rate", type=int, default=TARGET_RATE)
    parser.add_argument("--pcm-format", default=TARGET_SUBTYPE)
    parser.add_argument("--cpu-threads", type=int, default=1, choices=range(1, 5), metavar="1..4")
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--dry-run", action="store_true", help="validate only (default)")
    action.add_argument("--write", action="store_true", help="synthesize review-only WAV candidates")
    return parser.parse_args()


def main() -> int:
    args = arguments()
    try:
        selection, context = preflight(args)
        if not args.write:
            print(
                f"M01_VOICE_PREFLIGHT_OK lines={len(selection)} pack_sha256={context['pack_sha256']} "
                f"model_sha256={context['artifact_sha256']['kokoro-v1.0.onnx']} "
                "rights=PENDING listening=PENDING output=REFUSED_IF_EXISTS"
            )
            return 0
        write_candidates(args, selection, context)
        print(f"M01_VOICE_CANDIDATES_OK lines={len(selection)} output={args.output.resolve()} status=UNBOUND_PENDING_REVIEW")
        return 0
    except (PreflightError, subprocess.CalledProcessError, OSError) as error:
        print(f"M01_VOICE_CANDIDATES_REFUSED: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
