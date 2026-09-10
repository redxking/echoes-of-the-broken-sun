#!/usr/bin/env python3
"""Synthesize and master the whole campaign's authored voice lines.

`EchoesNarrativePack.json` carries 371 speaker-tagged lines across six
speakers - 316 in the fifteen operations plus 55 in the demo surfaces
(43 Mara tutorial, 12 Meridian Annunciator). Twenty-eight M01 lines were
voiced; the rest were silent, so M02-M15 never spoke.

Every speaker is already cast: five pinned in CharacterVoiceIdentityBible.md
and the Annunciator ruled af_sky @1.06. There is no casting work here.

Each line is synthesized with the project-local Kokoro release, upsampled to
the registered 48 kHz PCM_24 target, then loudness-mastered by
Scripts/master_voice_audio.py to its bus target with a true-peak limiter.
Kokoro emits 24 kHz natively, so the manifest records the assets as
upsampled-from-24k rather than implying a 48 kHz capture.

    Tools/kokoro/venv/bin/python Scripts/synth_campaign_voice.py [--write]
        [--limit N] [--only LINE_ID ...]

Output is measured audio qualification, never a listening review: the
manifest records listeningVerified=false and the lines stay unaccepted.

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import subprocess
import sys
import tempfile
from datetime import UTC, datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))

PACK = ROOT / "Content/Narrative/Generated/EchoesNarrativePack.json"
OUT_DIR = ROOT / "Content/Audio/Source/Narrative"
KOKORO = ROOT.parent / "Tools" / "kokoro"
SOURCE_RATE = 24_000
TARGET_RATE = 48_000
TARGET_SUBTYPE = "PCM_24"
VERIFIED_RELEASE_SHA256 = {
    "kokoro-v1.0.onnx": "7d5df8ecf7d4b1878015a32686053fd0eebe2bc377234608764cc0ef3636a6c5",
    "voices-v1.0.bin": "bca610b8308e8d99f32e6fe4197e7ec01679264efed0cac9140fe9c29f1fbf7d",
}
MODEL_CARD_URL = "https://huggingface.co/hexgrad/Kokoro-82M"

# Five pinned in CharacterVoiceIdentityBible.md; the Annunciator is ruled.
VOICES = {
    "Mara Vey": ("af_sarah", 1.00),
    "Talar Venn": ("am_michael", 1.00),
    "Oruun-of-Seven-Stones": ("bm_george", 0.92),
    "Neme": ("af_nicole", 0.95),
    "Chancellor Cael Rhyse": ("bm_lewis", 0.95),
    "Meridian Operations Annunciator": ("af_sky", 1.06),
}
BINDINGS = OUT_DIR / "m01_voice_bindings.json"


def bound_line_ids() -> set[str]:
    """Line ids already voiced under hook-named assets.

    The M01 set is named by logical audio hook, not by line id, so a naive
    existence check would regenerate all 28 under a second name and leave two
    assets competing for one bound line.
    """
    if not BINDINGS.is_file():
        return set()
    body = json.loads(BINDINGS.read_text(encoding="utf-8"))
    return {row["line_id"] for row in body.get("lines", []) if row.get("line_id")}


class PreflightError(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def authored_lines() -> list[dict]:
    pack = json.loads(PACK.read_text(encoding="utf-8"))
    rows: list[dict] = []
    for operation, body in sorted(pack.get("operations", {}).items()):
        for line in body.get("lines", []):
            if line.get("speaker") and line.get("text"):
                rows.append({"line_id": line["id"], "speaker": line["speaker"],
                             "text": line["text"], "surface": operation})
    for surface, body in sorted(pack.get("demo", {}).items()):
        for line in body.get("lines", []):
            if line.get("speaker") and line.get("text"):
                rows.append({"line_id": line["id"], "speaker": line["speaker"],
                             "text": line["text"], "surface": f"demo.{surface}"})
    unknown = sorted({r["speaker"] for r in rows} - set(VOICES))
    if unknown:
        raise PreflightError(f"uncast speakers in the pack: {unknown}")
    seen = set()
    for row in rows:
        if row["line_id"] in seen:
            raise PreflightError(f"duplicate line id in pack: {row['line_id']}")
        seen.add(row["line_id"])
        row["voice"], row["speed"] = VOICES[row["speaker"]]
    return rows


def preflight() -> dict:
    if not PACK.is_file():
        raise PreflightError(f"missing narrative pack: {PACK}")
    model, voices = KOKORO / "kokoro-v1.0.onnx", KOKORO / "voices-v1.0.bin"
    for path in (model, voices):
        if not path.is_file():
            raise PreflightError(f"missing Kokoro release file: {path}")
        actual = sha256_file(path)
        if actual != VERIFIED_RELEASE_SHA256[path.name]:
            raise PreflightError(
                f"{path.name} digest {actual} does not match the verified release")
    afconvert = "/usr/bin/afconvert"
    if not Path(afconvert).is_file():
        raise PreflightError("afconvert not found; required for the 48 kHz target")
    return {"pack": str(PACK), "pack_sha256": sha256_file(PACK),
            "model": str(model), "voices": str(voices), "afconvert": afconvert,
            "model_card": MODEL_CARD_URL,
            "weights_sha256": {k: VERIFIED_RELEASE_SHA256[k] for k in VERIFIED_RELEASE_SHA256}}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--limit", type=int, default=None)
    parser.add_argument("--only", nargs="*", default=None)
    parser.add_argument("--json", default=None)
    args = parser.parse_args()

    context = preflight()
    rows = authored_lines()
    if args.only:
        rows = [r for r in rows if r["line_id"] in set(args.only)]
    bound = bound_line_ids()
    pending = [r for r in rows
               if r["line_id"] not in bound
               and not (OUT_DIR / f"{r['line_id']}.wav").exists()]
    if args.limit:
        pending = pending[:args.limit]

    print(f"[ECHOES_CAMPAIGN_VOICE_PLAN] authored={len(rows)} "
          f"alreadyBound={len([r for r in rows if r['line_id'] in bound])} "
          f"alreadyPresent={len([r for r in rows if r['line_id'] not in bound and (OUT_DIR / (r['line_id'] + '.wav')).exists()])} "
          f"toSynthesize={len(pending)}")
    if not args.write:
        for row in pending[:5]:
            print(f"  would synthesize {row['line_id']} [{row['voice']}] {row['text'][:56]}")
        print("[ECHOES_CAMPAIGN_VOICE_DRYRUN] pass --write to synthesize")
        return 0

    import numpy as np
    import soundfile as sf
    from kokoro_onnx import Kokoro
    import master_voice_audio as mva

    engine = Kokoro(context["model"], context["voices"])
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    produced: list[dict] = []
    with tempfile.TemporaryDirectory(prefix="echoes-campaign-voice-") as scratch_dir:
        scratch = Path(scratch_dir)
        for index, row in enumerate(pending, start=1):
            samples, rate = engine.create(row["text"], voice=row["voice"],
                                          speed=row["speed"], lang="en-us")
            if rate != SOURCE_RATE:
                raise PreflightError(f"unexpected Kokoro rate for {row['line_id']}: {rate}")
            samples = np.asarray(samples, dtype=np.float64)
            if samples.size == 0 or not np.isfinite(samples).all():
                raise PreflightError(f"invalid samples for {row['line_id']}")
            peak = float(np.max(np.abs(samples)))
            if peak == 0.0:
                raise PreflightError(f"silent line: {row['line_id']}")
            # Peak-protect only; master_voice_audio owns final loudness.
            staged = scratch / f"{row['line_id']}.24k.wav"
            final = OUT_DIR / f"{row['line_id']}.wav"
            sf.write(str(staged), samples * min(1.0, 10 ** (-3.0 / 20.0) / peak),
                     rate, subtype=TARGET_SUBTYPE)
            subprocess.run([context["afconvert"], "-f", "WAVE", "-d",
                            f"LEI24@{TARGET_RATE}", "-c", "1", "-r", "127",
                            str(staged), str(final)],
                           check=True, capture_output=True, text=True)
            record = mva.master_one(final, write=True)
            duration = len(samples) / rate
            produced.append({**row, "path": str(final.relative_to(ROOT)),
                             "source_rate_hz": SOURCE_RATE,
                             "target_rate_hz": TARGET_RATE,
                             "sample_rate_provenance": "upsampled from 24 kHz Kokoro output",
                             "duration_s": round(duration, 3),
                             "chars_per_second": round(len(row["text"]) / duration, 1),
                             "mastering": record})
            print(f"[ECHOES_CAMPAIGN_VOICE_LINE] {index}/{len(pending)} {row['line_id']} "
                  f"{record['after']['integrated_lufs']:+.2f} LUFS "
                  f"{record['after']['true_peak_dbtp']:+.2f} dBTP", flush=True)

    compliant = sum(1 for r in produced
                    if r["mastering"].get("within_target") and r["mastering"].get("within_ceiling"))
    print(f"[ECHOES_CAMPAIGN_VOICE_READY] synthesized={len(produced)} "
          f"withinTargetAndCeiling={compliant} listeningVerified=false")

    if args.json:
        payload = {
            "schema": "echoes-campaign-voice-v1", "author": "Angelis Pseftis",
            "created_utc": datetime.now(UTC).isoformat(),
            "requirement": "REL-AUD-004",
            "candidate_status": "unqualified_pending_listening_and_asset_registration",
            "listeningVerified": False,
            "evidence_class": "measured audio qualification; not a listening review, "
                              "not rendered playback, not owner acceptance",
            "rights": {
                "model": "Kokoro-82M", "weights_license": "Apache-2.0",
                "commercial_use": "permitted under Apache-2.0",
                "voice_cloning": "none; model card states training excluded custom voice clones",
                "model_card": MODEL_CARD_URL,
                "local_only": True,
            },
            "environment": {"python": sys.version, "platform": platform.platform()},
            "inputs": context,
            "lines": produced,
        }
        Path(args.json).write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        print(f"[ECHOES_CAMPAIGN_VOICE_JSON] {Path(args.json).resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
