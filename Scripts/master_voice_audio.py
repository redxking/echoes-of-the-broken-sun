#!/usr/bin/env python3
"""Loudness-master generated voice WAVs to the per-bus targets.

The synthesis pipeline peak-protects to -3 dBFS and says so in its own
comment: "peak protection only, not final loudness normalization". The
loudness stage was never built, which is why all 28 bound M01 sources
measured -23.99..-20.12 LUFS (mean -22.14) against a -16.0 LUFS voice
target - below the -21 LUFS ambience bed they have to be intelligible over.

This stage supplies what was missing: gain to the bus target, then a
look-ahead true-peak limiter so the gain cannot clip. Naive normalization
would put all 28 lines above the -1 dBTP ceiling, the worst reaching
+4.97 dBTP, so the limiter is required rather than optional.

Measurement is delegated to Scripts/measure_audio_loudness.py - the project's
BS.1770-4 implementation - so mastering and verification cannot disagree.
Note that its 4x linear-interpolated true peak understates some inter-sample
peaks relative to the BS.1770 polyphase filter, so the limiter aims a little
below the ceiling to keep real peaks inside it.

    Tools/kokoro/venv/bin/python Scripts/master_voice_audio.py \
        --source-dir Content/Audio/Source/Narrative [--write] [--json OUT]

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import json
import sys
from datetime import UTC, datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))

import measure_audio_loudness as mal  # noqa: E402

# AudioDirection.md section 7 per-bus source targets.
BUS_TARGETS = {
    "voice": (-16.0, 0.5),
    "alert": (-14.0, 1.0),
}
CEILING_DBTP = -1.0
# The estimator understates inter-sample peaks, so leave headroom under it.
LIMITER_AIM_DBTP = -1.2
SUBTYPE = "PCM_24"


def bus_for(name: str) -> str:
    """Annunciator lines master to the alert row, not the voice row."""
    return "alert" if "_ann_" in name or name.startswith("ann_") else "voice"


def measure(path: Path) -> tuple[float, float]:
    per_channel, rate = mal.read_wav(str(path))
    return mal.integrated_lufs(per_channel, rate), mal.true_peak_dbtp(per_channel)


def limit_true_peak(samples, rate: int, aim_dbtp: float):
    """Look-ahead limiter on a 4x-oversampled peak envelope.

    Speech has a wide crest factor, so the several dB of reduction the gain
    stage demands lands on brief transients only. Attack is short enough to
    catch them and release long enough that the gain envelope does not pump
    across a syllable.
    """
    import numpy as np

    ceiling = 10.0 ** (aim_dbtp / 20.0)
    oversampled = np.abs(np.interp(
        np.arange(0, len(samples) - 1, 0.25),
        np.arange(len(samples)),
        samples))
    if oversampled.size == 0:
        return samples, 0.0
    # Fold the 4 sub-steps back onto their source sample, keeping the max.
    folded = np.zeros(len(samples))
    folded[:len(samples) - 1] = oversampled.reshape(-1, 4).max(axis=1)
    folded[-1] = abs(samples[-1])
    folded = np.maximum(folded, np.abs(samples))

    required = np.minimum(1.0, ceiling / np.maximum(folded, 1e-12))
    lookahead = max(1, int(0.0015 * rate))
    # Sliding minimum over the look-ahead window: start reducing before the peak.
    padded = np.concatenate([required, np.ones(lookahead)])
    windowed = np.minimum.reduce(
        [padded[offset:offset + len(required)] for offset in range(lookahead + 1)])

    release = np.exp(-1.0 / (0.060 * rate))
    envelope = np.empty_like(windowed)
    current = 1.0
    for index, target in enumerate(windowed):
        current = target if target < current else target + (current - target) * release
        envelope[index] = current
    reduction_db = -20.0 * np.log10(max(float(envelope.min()), 1e-12))
    return samples * envelope, reduction_db


def master_one(path: Path, write: bool) -> dict:
    import numpy as np
    import soundfile as sf

    bus = bus_for(path.stem)
    target, tolerance = BUS_TARGETS[bus]
    before_lufs, before_tp = measure(path)

    samples, rate = sf.read(str(path), dtype="float64", always_2d=False)
    if samples.ndim != 1:
        raise RuntimeError(f"expected mono voice source: {path}")

    # Integrated loudness moves exactly with a linear gain, so one measured
    # value determines the gain; the limiter then costs a little of it back.
    gain_db = target - before_lufs
    staged = samples * (10.0 ** (gain_db / 20.0))
    staged, reduction_db = limit_true_peak(staged, rate, LIMITER_AIM_DBTP)

    record = {
        "cue": path.stem, "bus": bus, "target_lufs": target,
        "before": {"integrated_lufs": round(before_lufs, 2),
                   "true_peak_dbtp": round(before_tp, 2)},
        "applied_gain_db": round(gain_db, 2),
        "limiter_reduction_db": round(reduction_db, 2),
    }

    if not write:
        record["status"] = "DRY_RUN"
        return record

    scratch = path.with_suffix(".mastering.wav")
    sf.write(str(scratch), staged, rate, subtype=SUBTYPE)
    after_lufs, after_tp = measure(scratch)
    # One corrective pass if the limiter pulled loudness out of tolerance.
    if abs(after_lufs - target) > tolerance / 2.0:
        staged = staged * (10.0 ** ((target - after_lufs) / 20.0))
        staged, extra = limit_true_peak(staged, rate, LIMITER_AIM_DBTP)
        record["limiter_reduction_db"] = round(reduction_db + extra, 2)
        record["corrective_gain_db"] = round(target - after_lufs, 2)
        sf.write(str(scratch), staged, rate, subtype=SUBTYPE)
        after_lufs, after_tp = measure(scratch)

    scratch.replace(path)
    record["after"] = {"integrated_lufs": round(after_lufs, 2),
                       "true_peak_dbtp": round(after_tp, 2)}
    record["within_target"] = abs(after_lufs - target) <= tolerance
    record["within_ceiling"] = after_tp <= CEILING_DBTP
    record["status"] = "MASTERED"
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", required=True)
    parser.add_argument("--write", action="store_true",
                        help="master in place; without it the run is a dry run")
    parser.add_argument("--json", default=None)
    args = parser.parse_args()

    source = Path(args.source_dir).resolve()
    paths = sorted(source.glob("*.wav"))
    if not paths:
        print(f"no WAV sources under {source}", file=sys.stderr)
        return 2

    records = [master_one(path, args.write) for path in paths]
    for record in records:
        after = record.get("after")
        tail = (f"-> {after['integrated_lufs']:+.2f} LUFS "
                f"{after['true_peak_dbtp']:+.2f} dBTP" if after else "(dry run)")
        print(f"[ECHOES_VOICE_MASTER] cue={record['cue']} bus={record['bus']} "
              f"was={record['before']['integrated_lufs']:+.2f} "
              f"gain={record['applied_gain_db']:+.2f} "
              f"limited={record['limiter_reduction_db']:.2f} {tail}", flush=True)

    mastered = [r for r in records if r.get("status") == "MASTERED"]
    compliant = [r for r in mastered if r.get("within_target") and r.get("within_ceiling")]
    print(f"[ECHOES_VOICE_MASTER_READY] cues={len(records)} mastered={len(mastered)} "
          f"withinTargetAndCeiling={len(compliant)}")

    if args.json:
        payload = {
            "schema": "echoes-voice-mastering-v1",
            "author": "Angelis Pseftis",
            "created_utc": datetime.now(UTC).isoformat(),
            "measurement_tool": "Scripts/measure_audio_loudness.py (ITU-R BS.1770-4)",
            "bus_targets_lufs": {k: {"target": v[0], "tolerance": v[1]}
                                 for k, v in BUS_TARGETS.items()},
            "true_peak_ceiling_dbtp": CEILING_DBTP,
            "limiter_aim_dbtp": LIMITER_AIM_DBTP,
            "listeningVerified": False,
            "evidence_class": "measured audio qualification; not a listening review",
            "cues": records,
        }
        Path(args.json).write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        print(f"[ECHOES_VOICE_MASTER_JSON] {Path(args.json).resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
