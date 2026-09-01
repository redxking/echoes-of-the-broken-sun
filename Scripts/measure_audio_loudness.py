"""Loudness and true-peak measurement for the registered audio sources.

Implements ITU-R BS.1770-4 integrated loudness (K-weighting, 400 ms gated
blocks, absolute -70 LUFS and relative -10 LU gates) and a 4x-oversampled
true-peak estimate, in pure Python over the deterministic WAV sources under
`Content/Audio/Source`. No third-party audio library is used, so the
measurement itself is reproducible from project code alone.

    python3 Scripts/measure_audio_loudness.py [--source-dir DIR] [--json OUT]

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import json
import math
import os
import struct
import sys
import wave


def biquad_coefficients(sample_rate: float) -> tuple[tuple, tuple]:
    """The two K-weighting stages from BS.1770-4, redesigned for the rate."""
    # Stage 1: spherical-head high-shelf.
    f0 = 1681.9744509555319
    gain_db = 3.99984385397
    q = 0.7071752369554193
    k = math.tan(math.pi * f0 / sample_rate)
    vh = 10.0 ** (gain_db / 20.0)
    vb = vh ** 0.499666774155
    a0 = 1.0 + k / q + k * k
    shelf_b = (
        (vh + vb * k / q + k * k) / a0,
        2.0 * (k * k - vh) / a0,
        (vh - vb * k / q + k * k) / a0,
    )
    shelf_a = (
        1.0,
        2.0 * (k * k - 1.0) / a0,
        (1.0 - k / q + k * k) / a0,
    )
    # Stage 2: RLB high-pass.
    f0 = 38.13547087602444
    q = 0.5003270373238773
    k = math.tan(math.pi * f0 / sample_rate)
    a0 = 1.0 + k / q + k * k
    hp_b = (1.0 / a0, -2.0 / a0, 1.0 / a0)
    hp_a = (
        1.0,
        2.0 * (k * k - 1.0) / a0,
        (1.0 - k / q + k * k) / a0,
    )
    return (shelf_b, shelf_a), (hp_b, hp_a)


def apply_biquad(samples: list[float], b: tuple, a: tuple) -> list[float]:
    out = [0.0] * len(samples)
    x1 = x2 = y1 = y2 = 0.0
    b0, b1, b2 = b
    _, a1, a2 = a
    for index, x0 in enumerate(samples):
        y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        out[index] = y0
        x2, x1 = x1, x0
        y2, y1 = y1, y0
    return out


def read_wav(path: str) -> tuple[list[list[float]], int]:
    with wave.open(path, "rb") as reader:
        channels = reader.getnchannels()
        rate = reader.getframerate()
        width = reader.getsampwidth()
        frames = reader.readframes(reader.getnframes())
    if width != 2:
        raise RuntimeError(f"Expected 16-bit PCM: {path}")
    count = len(frames) // 2
    per_channel: list[list[float]] = [[] for _ in range(channels)]
    for index in range(count):
        value = struct.unpack_from("<h", frames, index * 2)[0] / 32768.0
        per_channel[index % channels].append(value)
    return per_channel, rate


def integrated_lufs(per_channel: list[list[float]], rate: int) -> float:
    (shelf_b, shelf_a), (hp_b, hp_a) = biquad_coefficients(float(rate))
    weighted = [
        apply_biquad(apply_biquad(channel, shelf_b, shelf_a), hp_b, hp_a)
        for channel in per_channel
    ]
    block = int(round(0.4 * rate))
    hop = int(round(0.1 * rate))
    length = min(len(channel) for channel in weighted)
    if length < block:
        block = length
        hop = max(1, length)
    block_loudness: list[float] = []
    position = 0
    while position + block <= length:
        power = 0.0
        for channel in weighted:
            segment = channel[position : position + block]
            power += sum(v * v for v in segment) / block
        if power > 0.0:
            block_loudness.append(-0.691 + 10.0 * math.log10(power))
        position += hop
    if not block_loudness:
        return float("-inf")
    gated = [l for l in block_loudness if l > -70.0]
    if not gated:
        return float("-inf")
    mean_power = sum(10.0 ** ((l + 0.691) / 10.0) for l in gated) / len(gated)
    relative_gate = -0.691 + 10.0 * math.log10(mean_power) - 10.0
    gated = [l for l in block_loudness if l > relative_gate]
    if not gated:
        return float("-inf")
    mean_power = sum(10.0 ** ((l + 0.691) / 10.0) for l in gated) / len(gated)
    return -0.691 + 10.0 * math.log10(mean_power)


def true_peak_dbtp(per_channel: list[list[float]], oversample: int = 4) -> float:
    """4x linear-interpolated inter-sample peak. A conservative estimate:
    linear interpolation understates some inter-sample peaks relative to the
    BS.1770 polyphase filter, so treat results near the ceiling as suspect."""
    peak = 0.0
    for channel in per_channel:
        previous = 0.0
        for value in channel:
            for step in range(1, oversample + 1):
                t = step / oversample
                interpolated = previous + (value - previous) * t
                peak = max(peak, abs(interpolated))
            previous = value
    if peak <= 0.0:
        return float("-inf")
    return 20.0 * math.log10(peak)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", default=None)
    parser.add_argument("--json", default=None, help="Write results as JSON")
    arguments = parser.parse_args()
    source_dir = arguments.source_dir
    if source_dir is None:
        root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        source_dir = os.path.join(root, "Content", "Audio", "Source")

    results = []
    for name in sorted(os.listdir(source_dir)):
        if not name.endswith(".wav"):
            continue
        path = os.path.join(source_dir, name)
        per_channel, rate = read_wav(path)
        lufs = integrated_lufs(per_channel, rate)
        peak = true_peak_dbtp(per_channel)
        results.append(
            {
                "cue": name[:-4],
                "channels": len(per_channel),
                "sampleRate": rate,
                "integratedLufs": None if math.isinf(lufs) else round(lufs, 2),
                "truePeakDbtp": None if math.isinf(peak) else round(peak, 2),
            }
        )
        print(
            f"[ECHOES_AUDIO_LOUDNESS] cue={name[:-4]} channels={len(per_channel)} "
            f"integratedLufs={'-inf' if math.isinf(lufs) else f'{lufs:.2f}'} "
            f"truePeakDbtp={'-inf' if math.isinf(peak) else f'{peak:.2f}'}"
        )

    if arguments.json:
        with open(arguments.json, "w", encoding="utf-8") as output:
            json.dump(
                {"method": "BS.1770-4 K-weighted gated integration; 4x linear-interpolated inter-sample peak", "cues": results},
                output,
                indent=2,
            )
    print(f"[ECHOES_AUDIO_LOUDNESS_READY] cues={len(results)}")


if __name__ == "__main__":
    main()
