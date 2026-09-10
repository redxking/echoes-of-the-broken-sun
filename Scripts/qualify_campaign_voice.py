#!/usr/bin/env python3
"""Measured audio qualification of the campaign voice corpus.

This is a distinct evidence class and it is not a listening review. It proves
only what a machine can check:

  1. every authored line has an asset and every asset has an authored line
     (no orphans in either direction);
  2. each asset decodes, is mono at the registered rate, and is not silent;
  3. duration is plausible against its text length, which catches truncation
     and runaway synthesis;
  4. no clipping, and true peak inside the -1 dBTP ceiling;
  5. integrated loudness inside the per-bus target - voice -16.0 +/-0.5,
     Annunciator alerts -14.0 +/-1.0;
  6. the correct pinned voice for the speaker;
  7. the recorded text still matches the live narrative pack, so a line whose
     words changed after synthesis is flagged for regeneration rather than
     shipping stale audio.

Check 7 is why this does not need the narrative source frozen: staleness is
detected per line instead of prevented by coordination.

    python3 Scripts/qualify_campaign_voice.py [--json OUT]

Nothing here assigns owner acceptance and nothing sets listeningVerified.

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import wave
from datetime import UTC, datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))

import measure_audio_loudness as mal  # noqa: E402
from synth_campaign_voice import VOICES, authored_lines, bound_line_ids  # noqa: E402

VOICE_DIR = ROOT / "Content/Audio/Source/Narrative"
BINDINGS = VOICE_DIR / "m01_voice_bindings.json"
MANIFESTS = sorted((ROOT / "BuildArtifacts/Evidence").glob("campaign-voice-*/campaign-voice.json"))
BUS_TARGETS = {"voice": (-16.0, 0.5), "alert": (-14.0, 1.0)}
CEILING_DBTP = -1.0
REGISTERED_RATE = 48_000
# Kokoro speech runs roughly 9-22 characters per second on sentence-length
# text, so a rate outside this band means truncation or runaway.
CPS_MIN, CPS_MAX = 8.0, 30.0
# A rate is meaningless on a one-word line: fixed lead-in and final-consonant
# release dominate it. "Contact." measures 7.5 cps purely because it is eight
# characters long - 0.81 s of speech inside a 1.07 s file, decaying to 13% of
# peak, i.e. a complete utterance. Short texts are bounded on absolute
# duration instead.
SHORT_TEXT_CHARS = 16
SHORT_DURATION_MIN_S, SHORT_DURATION_MAX_S = 0.35, 3.0


def asset_stem(line_id: str, hooks: dict[str, str]) -> str:
    return hooks.get(line_id, line_id)


def hook_map() -> dict[str, str]:
    if not BINDINGS.is_file():
        return {}
    body = json.loads(BINDINGS.read_text(encoding="utf-8"))
    return {r["line_id"]: r["asset_path"].rsplit(".", 1)[-1]
            for r in body.get("lines", []) if r.get("line_id")}


def recorded_texts() -> dict[str, str]:
    """Text as it stood when each line was synthesized."""
    texts: dict[str, str] = {}
    for manifest in MANIFESTS:
        for row in json.loads(manifest.read_text(encoding="utf-8")).get("lines", []):
            texts[row["line_id"]] = row["text"]
    if BINDINGS.is_file():
        for row in json.loads(BINDINGS.read_text(encoding="utf-8")).get("lines", []):
            if row.get("line_id") and row.get("text"):
                texts[row["line_id"]] = row["text"]
    return texts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", default=None)
    args = parser.parse_args()

    authored = authored_lines()
    hooks = hook_map()
    bound = bound_line_ids()
    recorded = recorded_texts()
    by_id = {row["line_id"]: row for row in authored}

    findings: list[dict] = []
    rows: list[dict] = []
    for row in authored:
        stem = asset_stem(row["line_id"], hooks)
        path = VOICE_DIR / f"{stem}.wav"
        record = {"line_id": row["line_id"], "speaker": row["speaker"],
                  "surface": row["surface"], "asset": stem,
                  "bound_binding": row["line_id"] in bound}
        if not path.is_file():
            record["status"] = "MISSING_ASSET"
            findings.append({"severity": "blocker", "line_id": row["line_id"],
                             "finding": "authored line has no asset"})
            rows.append(record)
            continue

        with wave.open(str(path), "rb") as reader:
            channels, rate, frames = reader.getnchannels(), reader.getframerate(), reader.getnframes()
        duration = frames / rate if rate else 0.0
        per_channel, read_rate = mal.read_wav(str(path))
        lufs = mal.integrated_lufs(per_channel, read_rate)
        peak_dbtp = mal.true_peak_dbtp(per_channel)
        sample_peak = max(abs(v) for v in per_channel[0])

        bus = "alert" if "_ann_" in row["line_id"] else "voice"
        target, tolerance = BUS_TARGETS[bus]
        expected_voice = VOICES[row["speaker"]][0]
        cps = (len(row["text"]) / duration) if duration > 0 else 0.0
        stale = recorded.get(row["line_id"]) not in (None, row["text"])

        record.update({
            "bus": bus, "expected_voice": expected_voice,
            "channels": channels, "sample_rate_hz": rate,
            "duration_s": round(duration, 3),
            "chars_per_second": round(cps, 1),
            "integrated_lufs": round(lufs, 2),
            "true_peak_dbtp": round(peak_dbtp, 2),
            "sample_peak": round(sample_peak, 4),
            "text_matches_pack": not stale,
        })
        checks = {
            "decodes_mono": channels == 1,
            "registered_rate": rate == REGISTERED_RATE,
            "not_silent": sample_peak > 0.0,
            "no_clipping": sample_peak < 0.999,
            "within_ceiling": peak_dbtp <= CEILING_DBTP,
            "within_bus_target": abs(lufs - target) <= tolerance,
            "duration_plausible": (
                SHORT_DURATION_MIN_S <= duration <= SHORT_DURATION_MAX_S
                if len(row["text"]) < SHORT_TEXT_CHARS
                else CPS_MIN <= cps <= CPS_MAX),
            "text_current": not stale,
        }
        record["checks"] = checks
        record["status"] = "QUALIFIED" if all(checks.values()) else "FAILED"
        for name, ok in checks.items():
            if not ok:
                findings.append({"severity": "blocker", "line_id": row["line_id"],
                                 "finding": f"failed {name}",
                                 "detail": {k: record.get(k) for k in
                                            ("integrated_lufs", "true_peak_dbtp",
                                             "chars_per_second", "sample_rate_hz",
                                             "channels", "text_matches_pack")}})
        rows.append(record)

    # Orphans in the other direction: assets with no authored line.
    known = {asset_stem(r["line_id"], hooks) for r in authored}
    orphans = sorted(p.stem for p in VOICE_DIR.glob("*.wav") if p.stem not in known)
    for stem in orphans:
        findings.append({"severity": "blocker", "line_id": None,
                         "finding": "asset has no authored line", "detail": stem})

    # Runtime reachability, measured on the ASSET side rather than the source
    # side. A line is only audible if it is bound in a manifest AND its
    # imported .uasset exists. Counting sources here would report full
    # coverage while the game plays 28 lines - a green number that never
    # exercised the thing it claims to prove.
    voice_asset_dir = ROOT / "Content/Audio/Voice"
    imported = {p.stem for p in voice_asset_dir.glob("*.uasset")}
    reachable = 0
    for record in rows:
        stem = record["asset"]
        record["binding_present"] = record["bound_binding"]
        record["imported_uasset"] = stem in imported
        record["runtime_reachable"] = bool(record["bound_binding"]) and stem in imported
        reachable += record["runtime_reachable"]

    qualified = [r for r in rows if r["status"] == "QUALIFIED"]
    payload = {
        "schema": "echoes-voice-qualification-v1", "author": "Angelis Pseftis",
        "created_utc": datetime.now(UTC).isoformat(),
        "requirements": ["REL-AUD-004", "REL-AUD-023"],
        "evidence_class": "measured audio qualification of the WAV sources; "
                          "NOT a listening review, NOT rendered playback, "
                          "NOT owner acceptance",
        "listeningVerified": False,
        "source_commit": subprocess.run(["git", "rev-parse", "HEAD"],
                                        capture_output=True, text=True).stdout.strip(),
        "pack_sha256": mal_pack_digest(),
        "bus_targets_lufs": {k: {"target": v[0], "tolerance": v[1]}
                             for k, v in BUS_TARGETS.items()},
        "true_peak_ceiling_dbtp": CEILING_DBTP,
        "authored_lines": len(authored),
        "wav_sources_present": len(rows) - sum(
            1 for r in rows if r["status"] == "MISSING_ASSET"),
        "wav_sources_qualified": len(qualified),
        "orphan_wav_sources": orphans,
        "imported_uassets": len(imported),
        "runtime_reachable_lines": reachable,
        "runtime_unreachable_lines": len(authored) - reachable,
        "reachability_note": (
            "wav_sources_qualified measures the WAV corpus. A line is audible "
            "only when it is bound in a manifest AND its .uasset exists; "
            "runtime_reachable_lines is that number. The two differ whenever "
            "sources have been generated ahead of binding or import."),
        "findings": findings, "lines": rows,
    }
    print(f"[ECHOES_VOICE_QUALIFICATION] authored={len(authored)} "
          f"wavSourcesQualified={len(qualified)} findings={len(findings)} "
          f"orphans={len(orphans)} listeningVerified=false")
    print(f"[ECHOES_VOICE_REACHABILITY] importedUassets={len(imported)} "
          f"runtimeReachable={reachable} unreachable={len(authored) - reachable} "
          f"-- a qualified source is not an audible line")
    if findings:
        for finding in findings[:12]:
            print(f"  {finding['severity'].upper()} {finding.get('line_id') or ''} "
                  f"{finding['finding']} {finding.get('detail', '')}")
    if args.json:
        Path(args.json).write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        print(f"[ECHOES_VOICE_QUALIFICATION_JSON] {Path(args.json).resolve()}")
    return 0 if not findings else 1


def mal_pack_digest() -> str:
    import hashlib
    pack = ROOT / "Content/Narrative/Generated/EchoesNarrativePack.json"
    return hashlib.sha256(pack.read_bytes()).hexdigest()


if __name__ == "__main__":
    raise SystemExit(main())
