"""Deterministic synthesis of every registered Echoes audio source.

This module is pure Python. It imports nothing from Unreal and can be run with
the system interpreter, so audio sources can be generated, hashed, and tested
without a built editor:

    python3 Scripts/echoes_audio_synth.py --write

`Scripts/generate_audio_assets.py` runs inside the editor and imports the
sources this module writes. Keeping the two apart lets the content suite prove
determinism and byte-idempotence directly.

Every waveform here is generated from project code. No sample, recording, or
model output from a third party is used, and none may be added.

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import array
import hashlib
import io
import math
import os
import struct
import wave
from dataclasses import dataclass, field
from typing import Callable, Sequence

SAMPLE_RATE = 48_000

CATEGORY_EFFECTS = "effects"
CATEGORY_INTERFACE = "interface"
CATEGORY_MUSIC = "music"
CATEGORY_AMBIENCE = "ambience"
CATEGORY_DIALOGUE = "dialogue"

CATEGORIES = (
    CATEGORY_EFFECTS,
    CATEGORY_INTERFACE,
    CATEGORY_MUSIC,
    CATEGORY_AMBIENCE,
    CATEGORY_DIALOGUE,
)

# Revision strings gate regeneration. Changing synthesis for a family means a
# new revision here, a regeneration run, and an AssetRegister.md entry.
REVISION_PRESENTATION = "presentation-audio-v1"
REVISION_INTERFACE = "interface-audio-v1"
REVISION_GAMEPLAY = "gameplay-audio-v1"
REVISION_MUSIC = "music-v1"
REVISION_AMBIENCE = "ambience-v1"

PEAK_CEILING = 0.96


# ---------------------------------------------------------------------------
# Deterministic primitives
# ---------------------------------------------------------------------------


def lcg_noise(index: int, seed: int) -> float:
    """Reproducible white noise in [-1, 1] from an integer index."""
    state = (index * 1103515245 + seed * 12345 + 1013904223) & 0x7FFFFFFF
    state = (state ^ (state >> 13)) & 0x7FFFFFFF
    state = (state * 1664525 + 1013904223) & 0x7FFFFFFF
    return ((state >> 8) & 0xFFFF) / 32768.0 - 1.0


def smooth_envelope(t: float, duration: float, attack: float, release: float) -> float:
    attack_gain = min(1.0, max(0.0, t / attack)) if attack > 0.0 else 1.0
    remaining = max(0.0, duration - t)
    release_gain = min(1.0, remaining / release) if release > 0.0 else 1.0
    return attack_gain * attack_gain * release_gain * release_gain


def adsr(t: float, duration: float, attack: float, decay: float, sustain: float, release: float) -> float:
    if t < 0.0 or t > duration:
        return 0.0
    if t < attack:
        x = t / attack if attack > 0.0 else 1.0
        return x * x
    if t < attack + decay:
        x = (t - attack) / decay if decay > 0.0 else 1.0
        return 1.0 + (sustain - 1.0) * x
    tail_start = duration - release
    if t >= tail_start and release > 0.0:
        x = (duration - t) / release
        return sustain * x * x
    return sustain


def exp_decay(t: float, tau: float) -> float:
    return math.exp(-t / tau) if tau > 0.0 else 0.0


def soft_clip(x: float) -> float:
    """Bounded, monotonic, and cheap. Keeps peaks under the ceiling."""
    if x > 1.0:
        x = 1.0
    elif x < -1.0:
        x = -1.0
    return 1.5 * x - 0.5 * x * x * x


class OnePole:
    """Single-pole low-pass, used to keep synthesized timbres from glaring."""

    def __init__(self, cutoff_hz: float) -> None:
        omega = 2.0 * math.pi * cutoff_hz / SAMPLE_RATE
        self.a = omega / (omega + 1.0)
        self.z = 0.0

    def step(self, x: float) -> float:
        self.z += self.a * (x - self.z)
        return self.z


class OnePoleHigh:
    def __init__(self, cutoff_hz: float) -> None:
        self.low = OnePole(cutoff_hz)

    def step(self, x: float) -> float:
        return x - self.low.step(x)


class CombDelay:
    """Fixed-length feedback delay. Deterministic, no allocation per sample."""

    def __init__(self, delay_seconds: float, feedback: float, damping_hz: float = 6000.0) -> None:
        self.buffer = array.array("d", [0.0] * max(1, int(delay_seconds * SAMPLE_RATE)))
        self.index = 0
        self.feedback = feedback
        self.damper = OnePole(damping_hz)

    def step(self, x: float) -> float:
        delayed = self.buffer[self.index]
        self.buffer[self.index] = x + self.damper.step(delayed) * self.feedback
        self.index += 1
        if self.index >= len(self.buffer):
            self.index = 0
        return delayed


# ---------------------------------------------------------------------------
# Buffers
# ---------------------------------------------------------------------------


class Buffer:
    """Interleaved float buffer with a channel count. Additive by design."""

    def __init__(self, duration: float, channels: int) -> None:
        self.channels = channels
        self.frames = int(round(duration * SAMPLE_RATE))
        self.data = array.array("d", [0.0] * (self.frames * channels))

    def add(self, frame: int, channel: int, value: float) -> None:
        if 0 <= frame < self.frames:
            self.data[frame * self.channels + channel] += value

    def add_stereo(self, frame: int, left: float, right: float) -> None:
        if 0 <= frame < self.frames:
            base = frame * self.channels
            self.data[base] += left
            if self.channels > 1:
                self.data[base + 1] += right

    def normalize(self, target_peak: float = 0.89) -> float:
        peak = 0.0
        for value in self.data:
            magnitude = value if value >= 0.0 else -value
            if magnitude > peak:
                peak = magnitude
        if peak <= 1e-9:
            return 0.0
        gain = target_peak / peak
        for i in range(len(self.data)):
            self.data[i] *= gain
        return peak

    def to_wave_bytes(self) -> bytes:
        payload = io.BytesIO()
        with wave.open(payload, "wb") as writer:
            writer.setnchannels(self.channels)
            writer.setsampwidth(2)
            writer.setframerate(SAMPLE_RATE)
            frames = bytearray()
            for value in self.data:
                clipped = soft_clip(value)
                if clipped > PEAK_CEILING:
                    clipped = PEAK_CEILING
                elif clipped < -PEAK_CEILING:
                    clipped = -PEAK_CEILING
                frames.extend(struct.pack("<h", int(round(clipped * 32767.0))))
            writer.writeframes(bytes(frames))
        return payload.getvalue()


# ---------------------------------------------------------------------------
# Instrument voices
#
# The Bible fixes the palette: Compact uses measured pulse, prepared piano,
# restrained brass, and mechanical resonance. Kharuun uses interlocking rhythms
# and resonant stone or ceramic timbres. Choir harmony resolves in more than one
# direction before committing.
# ---------------------------------------------------------------------------


def voice_prepared_piano(buffer: Buffer, start: float, freq: float, duration: float, gain: float, pan: float, seed: int) -> None:
    """Struck string with an inharmonic damper buzz — the Compact's signature."""
    start_frame = int(start * SAMPLE_RATE)
    frame_count = int(duration * SAMPLE_RATE)
    partials = ((1.0, 1.0, 2.6), (2.01, 0.42, 1.5), (3.04, 0.24, 0.9), (4.10, 0.13, 0.6), (6.23, 0.07, 0.35))
    buzz_freq = freq * 7.31
    left_gain = gain * math.sqrt(max(0.0, 0.5 - 0.5 * pan))
    right_gain = gain * math.sqrt(max(0.0, 0.5 + 0.5 * pan))
    for i in range(frame_count):
        t = i / SAMPLE_RATE
        strike = 1.0 if t < 0.0015 else 0.0
        sample = 0.0
        for ratio, amplitude, tau in partials:
            sample += amplitude * math.sin(math.tau * freq * ratio * t) * exp_decay(t, tau)
        buzz = math.sin(math.tau * buzz_freq * t) * exp_decay(t, 0.09) * 0.11
        noise = lcg_noise(i + seed, seed) * strike * 0.5
        value = (sample * 0.34 + buzz + noise) * adsr(t, duration, 0.002, 0.05, 0.55, min(0.6, duration * 0.5))
        buffer.add_stereo(start_frame + i, value * left_gain, value * right_gain)


def voice_brass(buffer: Buffer, start: float, freq: float, duration: float, gain: float, pan: float) -> None:
    """Restrained brass: harmonic stack, slow attack, narrow vibrato."""
    start_frame = int(start * SAMPLE_RATE)
    frame_count = int(duration * SAMPLE_RATE)
    shaper = OnePole(2400.0)
    left_gain = gain * math.sqrt(max(0.0, 0.5 - 0.5 * pan))
    right_gain = gain * math.sqrt(max(0.0, 0.5 + 0.5 * pan))
    for i in range(frame_count):
        t = i / SAMPLE_RATE
        vibrato = 1.0 + 0.0032 * math.sin(math.tau * 4.6 * t)
        phase = math.tau * freq * vibrato * t
        sample = (
            math.sin(phase)
            + 0.52 * math.sin(2.0 * phase)
            + 0.31 * math.sin(3.0 * phase)
            + 0.17 * math.sin(4.0 * phase)
            + 0.08 * math.sin(5.0 * phase)
        )
        value = shaper.step(sample * 0.27) * adsr(t, duration, 0.09, 0.12, 0.72, 0.28)
        buffer.add_stereo(start_frame + i, value * left_gain, value * right_gain)


def voice_ceramic(buffer: Buffer, start: float, freq: float, duration: float, gain: float, pan: float) -> None:
    """Kharuun ceramic resonance: inharmonic bell partials, long tail."""
    start_frame = int(start * SAMPLE_RATE)
    frame_count = int(duration * SAMPLE_RATE)
    partials = ((1.0, 1.0, 1.9), (1.51, 0.62, 1.4), (2.47, 0.38, 1.0), (3.42, 0.19, 0.7), (5.11, 0.09, 0.45))
    left_gain = gain * math.sqrt(max(0.0, 0.5 - 0.5 * pan))
    right_gain = gain * math.sqrt(max(0.0, 0.5 + 0.5 * pan))
    for i in range(frame_count):
        t = i / SAMPLE_RATE
        sample = 0.0
        for ratio, amplitude, tau in partials:
            sample += amplitude * math.sin(math.tau * freq * ratio * t + ratio) * exp_decay(t, tau)
        value = sample * 0.3 * adsr(t, duration, 0.003, 0.08, 0.5, min(0.8, duration * 0.6))
        buffer.add_stereo(start_frame + i, value * left_gain, value * right_gain)


def voice_stone(buffer: Buffer, start: float, freq: float, duration: float, gain: float, pan: float, seed: int) -> None:
    """Struck stone: pitched thump plus filtered grit. Kharuun rhythm bed."""
    start_frame = int(start * SAMPLE_RATE)
    frame_count = int(duration * SAMPLE_RATE)
    body = OnePole(freq * 3.4)
    left_gain = gain * math.sqrt(max(0.0, 0.5 - 0.5 * pan))
    right_gain = gain * math.sqrt(max(0.0, 0.5 + 0.5 * pan))
    for i in range(frame_count):
        t = i / SAMPLE_RATE
        thump = math.sin(math.tau * freq * t * (1.0 - 0.22 * min(1.0, t / 0.05))) * exp_decay(t, 0.075)
        grit = body.step(lcg_noise(i + seed, seed)) * exp_decay(t, 0.035)
        value = (0.72 * thump + 0.36 * grit) * 0.5
        buffer.add_stereo(start_frame + i, value * left_gain, value * right_gain)


def voice_choir_pad(buffer: Buffer, start: float, freq: float, duration: float, gain: float, detune: float, seed: int) -> None:
    """Hollow Choir: a stack that will not settle on one resolution."""
    start_frame = int(start * SAMPLE_RATE)
    frame_count = int(duration * SAMPLE_RATE)
    shaper_left = OnePole(3200.0)
    shaper_right = OnePole(2900.0)
    for i in range(frame_count):
        t = i / SAMPLE_RATE
        drift = 1.0 + 0.0018 * math.sin(math.tau * 0.23 * t + seed)
        a = math.sin(math.tau * freq * drift * t)
        b = math.sin(math.tau * freq * (1.0 + detune) * t + 1.1)
        c = math.sin(math.tau * freq * 1.4983 * t + 0.6)
        d = math.sin(math.tau * freq * 0.6674 * t + 2.2)
        swell = adsr(t, duration, duration * 0.28, duration * 0.1, 0.8, duration * 0.34)
        left = shaper_left.step(0.42 * a + 0.3 * c + 0.18 * d) * swell * gain
        right = shaper_right.step(0.42 * b + 0.3 * d + 0.18 * c) * swell * gain
        buffer.add_stereo(start_frame + i, left, right)


def voice_mechanical(buffer: Buffer, start: float, freq: float, duration: float, gain: float, pan: float, seed: int) -> None:
    """Mechanical resonance: metallic comb over a damped impulse."""
    start_frame = int(start * SAMPLE_RATE)
    frame_count = int(duration * SAMPLE_RATE)
    comb = CombDelay(1.0 / max(40.0, freq), 0.86, 5200.0)
    left_gain = gain * math.sqrt(max(0.0, 0.5 - 0.5 * pan))
    right_gain = gain * math.sqrt(max(0.0, 0.5 + 0.5 * pan))
    for i in range(frame_count):
        t = i / SAMPLE_RATE
        excite = lcg_noise(i + seed, seed) * exp_decay(t, 0.012)
        value = comb.step(excite) * 0.6 * adsr(t, duration, 0.001, 0.03, 0.35, min(0.5, duration * 0.5))
        buffer.add_stereo(start_frame + i, value * left_gain, value * right_gain)


def voice_sub(buffer: Buffer, start: float, freq: float, duration: float, gain: float) -> None:
    """Measured low pulse. Carries the Compact's sense of scheduled time."""
    start_frame = int(start * SAMPLE_RATE)
    frame_count = int(duration * SAMPLE_RATE)
    for i in range(frame_count):
        t = i / SAMPLE_RATE
        value = math.sin(math.tau * freq * t) * adsr(t, duration, 0.01, 0.1, 0.6, duration * 0.4) * gain
        buffer.add_stereo(start_frame + i, value, value)


# ---------------------------------------------------------------------------
# Pitch helpers
# ---------------------------------------------------------------------------


def note(semitones_from_a4: float) -> float:
    return 440.0 * (2.0 ** (semitones_from_a4 / 12.0))


# Soryn's modal centre: D dorian with a raised fourth for the broken-sun colour.
MERIDIAN_SCALE = (-7, -5, -4, -2, 0, 2, 3, 5)      # D E F G A B C D relative to A4
KHARUUN_SCALE = (-12, -10, -9, -7, -5, -4, -2, 0)  # A aeolian
CHOIR_SCALE = (-8, -6, -4, -3, -1, 1, 3, 4)        # ambiguous, two resolutions


# ---------------------------------------------------------------------------
# Existing presentation cues — reproduced verbatim so presentation-audio-v1
# regenerates byte-identically. Do not alter these four functions.
# ---------------------------------------------------------------------------


def command_confirm(t: float, _: int) -> float:
    duration = 0.14
    envelope = smooth_envelope(t, duration, 0.008, 0.07)
    primary = math.sin(math.tau * 720.0 * t)
    upper = math.sin(math.tau * 1080.0 * t + 0.35)
    return 0.38 * envelope * (0.72 * primary + 0.28 * upper)


def meridian_destruction(t: float, sample_index: int) -> float:
    duration = 0.46
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


def choir_destruction(t: float, sample_index: int) -> float:
    duration = 0.58
    progress = min(1.0, t / duration)
    envelope = smooth_envelope(t, duration, 0.018, 0.31)
    falling_phase = math.tau * (392.0 * t - 96.0 * t * t / duration)
    divergent = (
        math.sin(falling_phase + 0.42 * math.sin(math.tau * 7.0 * t))
        + math.sin(1.031 * falling_phase + 1.1 - 0.7 * progress)
    )
    interval = math.sin(math.tau * (611.0 - 170.0 * progress) * t + 0.8)
    deterministic_grit = (((sample_index * 1664525 + 1013904223) >> 17) & 0x7FFF) / 16384.0 - 1.0
    return 0.44 * envelope * (
        0.49 * divergent + 0.21 * interval + 0.045 * deterministic_grit
    )


# ---------------------------------------------------------------------------
# Interface cues
# ---------------------------------------------------------------------------


def ui_hover(t: float, sample_index: int) -> float:
    duration = 0.045
    envelope = smooth_envelope(t, duration, 0.004, 0.03)
    return 0.22 * envelope * math.sin(math.tau * 1480.0 * t)


def ui_select(t: float, sample_index: int) -> float:
    duration = 0.085
    envelope = smooth_envelope(t, duration, 0.003, 0.05)
    body = math.sin(math.tau * 980.0 * t) + 0.4 * math.sin(math.tau * 1470.0 * t + 0.2)
    return 0.3 * envelope * body


def ui_confirm(t: float, sample_index: int) -> float:
    duration = 0.2
    envelope = smooth_envelope(t, duration, 0.006, 0.11)
    step = 0.0 if t < 0.055 else 1.0
    frequency = 660.0 + 330.0 * step
    body = math.sin(math.tau * frequency * t) + 0.3 * math.sin(math.tau * frequency * 1.5 * t + 0.4)
    return 0.32 * envelope * body


def ui_reject(t: float, sample_index: int) -> float:
    """Distinct from every accepted cue: falling, narrow, slightly detuned."""
    duration = 0.26
    envelope = smooth_envelope(t, duration, 0.005, 0.15)
    frequency = 330.0 - 90.0 * min(1.0, t / duration)
    body = math.sin(math.tau * frequency * t) + 0.55 * math.sin(math.tau * frequency * 1.06 * t + 1.3)
    grit = lcg_noise(sample_index, 7717) * 0.05 * exp_decay(t, 0.04)
    return 0.34 * envelope * (0.72 * body + grit)


def ui_menu_open(t: float, sample_index: int) -> float:
    duration = 0.3
    envelope = smooth_envelope(t, duration, 0.02, 0.18)
    sweep = 240.0 + 520.0 * min(1.0, t / duration)
    return 0.26 * envelope * (math.sin(math.tau * sweep * t) + 0.25 * math.sin(math.tau * sweep * 2.01 * t))


def ui_menu_close(t: float, sample_index: int) -> float:
    duration = 0.3
    envelope = smooth_envelope(t, duration, 0.012, 0.2)
    sweep = 760.0 - 500.0 * min(1.0, t / duration)
    return 0.26 * envelope * (math.sin(math.tau * sweep * t) + 0.25 * math.sin(math.tau * sweep * 2.01 * t))


def ui_brief_advance(t: float, sample_index: int) -> float:
    duration = 0.24
    envelope = smooth_envelope(t, duration, 0.008, 0.14)
    return 0.28 * envelope * (
        math.sin(math.tau * 523.25 * t) + 0.4 * math.sin(math.tau * 783.99 * t + 0.3)
    )


# ---------------------------------------------------------------------------
# Alerts — brief and rate-limited, each identifiable without colour or text
# ---------------------------------------------------------------------------


def alert_under_attack(t: float, sample_index: int) -> float:
    duration = 0.62
    envelope = smooth_envelope(t, duration, 0.006, 0.2)
    pulse = 1.0 if (t % 0.2) < 0.1 else 0.35
    body = math.sin(math.tau * 392.0 * t) + 0.5 * math.sin(math.tau * 466.16 * t + 0.7)
    return 0.42 * envelope * pulse * body


def alert_structure_lost(t: float, sample_index: int) -> float:
    duration = 0.7
    envelope = smooth_envelope(t, duration, 0.01, 0.34)
    frequency = 294.0 - 88.0 * min(1.0, t / duration)
    body = math.sin(math.tau * frequency * t) + 0.42 * math.sin(math.tau * frequency * 1.51 * t + 0.9)
    grit = lcg_noise(sample_index, 4231) * 0.06 * exp_decay(t, 0.12)
    return 0.4 * envelope * (0.78 * body + grit)


def alert_production_complete(t: float, sample_index: int) -> float:
    duration = 0.4
    envelope = smooth_envelope(t, duration, 0.006, 0.2)
    step = 0.0 if t < 0.12 else 1.0
    frequency = 587.33 + 196.0 * step
    return 0.34 * envelope * (math.sin(math.tau * frequency * t) + 0.3 * math.sin(math.tau * frequency * 2.0 * t))


def alert_research_complete(t: float, sample_index: int) -> float:
    duration = 0.55
    envelope = smooth_envelope(t, duration, 0.01, 0.26)
    stage = 0 if t < 0.14 else (1 if t < 0.28 else 2)
    frequency = (523.25, 659.25, 783.99)[stage]
    return 0.33 * envelope * (math.sin(math.tau * frequency * t) + 0.26 * math.sin(math.tau * frequency * 3.0 * t))


def alert_capacity_low(t: float, sample_index: int) -> float:
    duration = 0.45
    envelope = smooth_envelope(t, duration, 0.008, 0.22)
    frequency = 220.0 if (t % 0.15) < 0.075 else 233.08
    return 0.36 * envelope * (math.sin(math.tau * frequency * t) + 0.35 * math.sin(math.tau * frequency * 2.02 * t))


# ---------------------------------------------------------------------------
# Gameplay cues
# ---------------------------------------------------------------------------


def weapon_light(t: float, sample_index: int) -> float:
    duration = 0.13
    envelope = smooth_envelope(t, duration, 0.001, 0.09)
    crack = lcg_noise(sample_index, 991) * exp_decay(t, 0.012)
    tone = math.sin(math.tau * (1650.0 - 900.0 * min(1.0, t / duration)) * t)
    return 0.44 * envelope * (0.46 * crack + 0.54 * tone)


def weapon_line(t: float, sample_index: int) -> float:
    duration = 0.2
    envelope = smooth_envelope(t, duration, 0.0015, 0.13)
    crack = lcg_noise(sample_index, 1777) * exp_decay(t, 0.02)
    body = math.sin(math.tau * (880.0 - 480.0 * min(1.0, t / duration)) * t)
    sub = math.sin(math.tau * 96.0 * t) * exp_decay(t, 0.05)
    return 0.5 * envelope * (0.38 * crack + 0.44 * body + 0.24 * sub)


def weapon_heavy(t: float, sample_index: int) -> float:
    duration = 0.34
    envelope = smooth_envelope(t, duration, 0.002, 0.2)
    crack = lcg_noise(sample_index, 3313) * exp_decay(t, 0.035)
    body = math.sin(math.tau * (420.0 - 250.0 * min(1.0, t / duration)) * t)
    sub = math.sin(math.tau * 58.0 * t) * exp_decay(t, 0.12)
    return 0.56 * envelope * (0.34 * crack + 0.4 * body + 0.34 * sub)


def impact_hit(t: float, sample_index: int) -> float:
    duration = 0.16
    envelope = smooth_envelope(t, duration, 0.001, 0.1)
    strike = lcg_noise(sample_index, 5501) * exp_decay(t, 0.018)
    ring = math.sin(math.tau * 1240.0 * t + 0.4) * exp_decay(t, 0.045)
    return 0.42 * envelope * (0.58 * strike + 0.42 * ring)


def impact_shielded(t: float, sample_index: int) -> float:
    duration = 0.22
    envelope = smooth_envelope(t, duration, 0.002, 0.14)
    shimmer = math.sin(math.tau * 1860.0 * t) + 0.5 * math.sin(math.tau * 2790.0 * t + 0.6)
    return 0.36 * envelope * (0.7 * shimmer * exp_decay(t, 0.07) + 0.3 * lcg_noise(sample_index, 6607) * exp_decay(t, 0.02))


def gather_matter(t: float, sample_index: int) -> float:
    duration = 0.24
    envelope = smooth_envelope(t, duration, 0.004, 0.15)
    grind = lcg_noise(sample_index, 8123) * exp_decay(t, 0.09)
    tone = math.sin(math.tau * 1320.0 * t + 0.3) * exp_decay(t, 0.06)
    return 0.32 * envelope * (0.55 * grind + 0.45 * tone)


def deliver_matter(t: float, sample_index: int) -> float:
    duration = 0.3
    envelope = smooth_envelope(t, duration, 0.006, 0.18)
    step = 0.0 if t < 0.1 else 1.0
    frequency = 440.0 + 220.0 * step
    return 0.34 * envelope * (math.sin(math.tau * frequency * t) + 0.3 * math.sin(math.tau * frequency * 1.5 * t))


def construction_start(t: float, sample_index: int) -> float:
    duration = 0.42
    envelope = smooth_envelope(t, duration, 0.02, 0.24)
    rise = 180.0 + 260.0 * min(1.0, t / duration)
    grind = lcg_noise(sample_index, 2287) * 0.4 * exp_decay(t, 0.16)
    return 0.36 * envelope * (0.6 * math.sin(math.tau * rise * t) + grind)


def construction_complete(t: float, sample_index: int) -> float:
    duration = 0.5
    envelope = smooth_envelope(t, duration, 0.01, 0.3)
    settle = math.sin(math.tau * 330.0 * t) * exp_decay(t, 0.14)
    chime = math.sin(math.tau * 987.77 * t + 0.3) * exp_decay(t, 0.22)
    return 0.38 * envelope * (0.56 * settle + 0.44 * chime)


def production_complete(t: float, sample_index: int) -> float:
    duration = 0.36
    envelope = smooth_envelope(t, duration, 0.006, 0.2)
    hiss = lcg_noise(sample_index, 9973) * 0.35 * exp_decay(t, 0.08)
    tone = math.sin(math.tau * (523.25 + 130.0 * min(1.0, t / duration)) * t)
    return 0.36 * envelope * (0.62 * tone + hiss)


def research_start(t: float, sample_index: int) -> float:
    duration = 0.34
    envelope = smooth_envelope(t, duration, 0.02, 0.2)
    return 0.3 * envelope * (
        math.sin(math.tau * 349.23 * t) + 0.4 * math.sin(math.tau * 523.25 * t + 0.5)
    )


def research_interrupted(t: float, sample_index: int) -> float:
    """No refund. The cue must read as a stop, never as a completion."""
    duration = 0.38
    envelope = smooth_envelope(t, duration, 0.004, 0.22)
    frequency = 392.0 - 130.0 * min(1.0, t / duration)
    cut = 1.0 if t < 0.26 else 0.25
    return 0.34 * envelope * cut * (
        math.sin(math.tau * frequency * t) + 0.45 * math.sin(math.tau * frequency * 1.03 * t + 1.7)
    )


def well_claim(t: float, sample_index: int) -> float:
    duration = 0.9
    envelope = smooth_envelope(t, duration, 0.06, 0.44)
    swell = math.sin(math.tau * 174.61 * t) + 0.6 * math.sin(math.tau * 261.63 * t + 0.4)
    shimmer = math.sin(math.tau * 1046.5 * t + 0.9) * exp_decay(t, 0.4)
    return 0.4 * envelope * (0.6 * swell * 0.5 + 0.3 * shimmer)


def well_harvest(t: float, sample_index: int) -> float:
    """Immediate and permanent: a downward commitment that does not return."""
    duration = 1.1
    envelope = smooth_envelope(t, duration, 0.02, 0.5)
    frequency = 330.0 - 180.0 * min(1.0, t / duration)
    body = math.sin(math.tau * frequency * t) + 0.5 * math.sin(math.tau * frequency * 2.0 * t + 0.3)
    grit = lcg_noise(sample_index, 1213) * 0.12 * exp_decay(t, 0.3)
    return 0.44 * envelope * (0.66 * body * 0.6 + grit)


def well_preserve(t: float, sample_index: int) -> float:
    """Sustained possibility: a held interval that keeps paying out."""
    duration = 1.3
    envelope = smooth_envelope(t, duration, 0.14, 0.6)
    a = math.sin(math.tau * 293.66 * t)
    b = math.sin(math.tau * 440.0 * t + 0.2)
    c = math.sin(math.tau * 587.33 * t + 0.7) * (0.4 + 0.3 * math.sin(math.tau * 0.9 * t))
    return 0.4 * envelope * 0.42 * (a + 0.7 * b + 0.5 * c)


def well_reshape(t: float, sample_index: int) -> float:
    """Temporary transformation: unstable, and audibly on a clock."""
    duration = 1.2
    envelope = smooth_envelope(t, duration, 0.03, 0.52)
    wobble = 1.0 + 0.06 * math.sin(math.tau * 5.3 * t)
    a = math.sin(math.tau * 415.3 * t * wobble)
    b = math.sin(math.tau * 622.25 * t * wobble + 1.1)
    flicker = 0.6 + 0.4 * math.sin(math.tau * 11.0 * t)
    return 0.4 * envelope * 0.46 * flicker * (a + 0.66 * b)


def reshape_open(t: float, sample_index: int) -> float:
    duration = 0.8
    envelope = smooth_envelope(t, duration, 0.03, 0.4)
    sweep = 140.0 + 700.0 * min(1.0, t / duration)
    grit = lcg_noise(sample_index, 4649) * 0.3 * exp_decay(t, 0.25)
    return 0.42 * envelope * (0.6 * math.sin(math.tau * sweep * t) + grit)


def reshape_close(t: float, sample_index: int) -> float:
    duration = 0.8
    envelope = smooth_envelope(t, duration, 0.02, 0.44)
    sweep = 840.0 - 700.0 * min(1.0, t / duration)
    grit = lcg_noise(sample_index, 8291) * 0.3 * exp_decay(t, 0.3)
    return 0.42 * envelope * (0.6 * math.sin(math.tau * sweep * t) + grit)


# ---------------------------------------------------------------------------
# Music
# ---------------------------------------------------------------------------


def _measured_pulse(buffer: Buffer, bars: int, beats: int, beat: float, root: float, gain: float) -> None:
    for bar in range(bars):
        for step in range(beats):
            time = (bar * beats + step) * beat
            accent = 1.0 if step == 0 else (0.62 if step % 2 == 0 else 0.38)
            voice_sub(buffer, time, root * 0.5, beat * 0.9, gain * accent)


def music_meridian(duration: float) -> Buffer:
    """Measured pulse, prepared piano, restrained brass, mechanical resonance."""
    buffer = Buffer(duration, 2)
    beat = 60.0 / 96.0
    beats_per_bar = 4
    bar = beat * beats_per_bar
    bars = int(duration / bar)
    root = note(MERIDIAN_SCALE[0])

    _measured_pulse(buffer, bars, beats_per_bar, beat, root, 0.3)

    motif = (0, 2, 4, 3, 2, 0, -3, 0)
    for index in range(bars * 2):
        time = index * (bar / 2.0)
        if time + 1.2 > duration:
            break
        degree = motif[index % len(motif)]
        pitch = note(MERIDIAN_SCALE[degree % len(MERIDIAN_SCALE)] + 12 * (degree // len(MERIDIAN_SCALE)))
        voice_prepared_piano(buffer, time, pitch, 1.15, 0.5, -0.22 if index % 2 == 0 else 0.22, 331 + index)

    for bar_index in range(bars):
        time = bar_index * bar
        if bar_index % 2 == 1 and time + bar < duration:
            chord = (0, 4, 7) if (bar_index // 2) % 2 == 0 else (0, 3, 7)
            for offset_index, offset in enumerate(chord):
                voice_brass(buffer, time, note(MERIDIAN_SCALE[0] + offset - 12), bar * 0.92, 0.24, -0.3 + 0.3 * offset_index)

    for bar_index in range(bars):
        if bar_index % 4 == 3:
            voice_mechanical(buffer, bar_index * bar + beat * 2.5, 168.0, 0.7, 0.3, 0.4, 2003 + bar_index)

    buffer.normalize(0.82)
    return buffer


def music_kharuun(duration: float) -> Buffer:
    """Interlocking rhythm over resonant stone and ceramic."""
    buffer = Buffer(duration, 2)
    beat = 60.0 / 104.0
    pattern_a = (1, 0, 1, 1, 0, 1, 0)      # 7 against 4
    pattern_b = (1, 0, 0, 1)
    root = note(KHARUUN_SCALE[0])

    step = 0
    time = 0.0
    while time < duration - 0.6:
        if pattern_a[step % len(pattern_a)]:
            voice_stone(buffer, time, root * 2.0, 0.3, 0.42, -0.34, 4001 + step)
        if pattern_b[step % len(pattern_b)]:
            voice_stone(buffer, time, root * 1.5, 0.26, 0.3, 0.34, 4507 + step)
        step += 1
        time = step * beat * 0.5

    melody = (0, 3, 5, 4, 7, 5, 3, 2)
    for index in range(int(duration / (beat * 2))):
        time = index * beat * 2.0
        if time + 1.4 > duration:
            break
        degree = melody[index % len(melody)]
        pitch = note(KHARUUN_SCALE[degree % len(KHARUUN_SCALE)] + 12)
        voice_ceramic(buffer, time, pitch, 1.35, 0.42, 0.18 if index % 3 == 0 else -0.18)

    for index in range(int(duration / (beat * 8))):
        time = index * beat * 8.0
        if time + 3.0 > duration:
            break
        voice_ceramic(buffer, time, note(KHARUUN_SCALE[0] - 12), 3.0, 0.3, 0.0)

    buffer.normalize(0.82)
    return buffer


def music_choir(duration: float) -> Buffer:
    """Harmony that resolves in more than one direction before committing."""
    buffer = Buffer(duration, 2)
    segment = duration / 6.0
    resolutions = ((0, 4, 7), (0, 3, 8), (0, 5, 9), (0, 4, 7), (0, 3, 8), (0, 4, 7))
    for index, chord in enumerate(resolutions):
        time = index * segment
        for offset_index, offset in enumerate(chord):
            pitch = note(CHOIR_SCALE[0] + offset - 12)
            detune = 0.0035 * (1 + offset_index)
            voice_choir_pad(buffer, time, pitch, segment * 1.35, 0.3, detune, 6101 + index * 7 + offset_index)

    for index in range(int(duration / 3.1)):
        time = index * 3.1 + 1.2
        if time + 2.0 > duration:
            break
        pitch = note(CHOIR_SCALE[(index * 3) % len(CHOIR_SCALE)] + 12)
        voice_ceramic(buffer, time, pitch, 2.0, 0.22, -0.4 if index % 2 else 0.4)

    buffer.normalize(0.78)
    return buffer


def music_title(duration: float) -> Buffer:
    """The broken sun over the Glass Scar: all three languages, none dominant."""
    buffer = Buffer(duration, 2)
    beat = 60.0 / 84.0
    bar = beat * 4.0
    bars = int(duration / bar)
    root = note(MERIDIAN_SCALE[0] - 12)

    for bar_index in range(bars):
        time = bar_index * bar
        voice_sub(buffer, time, root * 0.5, bar * 0.85, 0.26)

    for index in range(bars):
        time = index * bar
        if time + 3.2 > duration:
            break
        chord = ((0, 7, 12), (0, 5, 10), (0, 3, 10), (0, 7, 14))[index % 4]
        for offset_index, offset in enumerate(chord):
            voice_choir_pad(buffer, time, note(MERIDIAN_SCALE[0] + offset - 12), bar * 1.2, 0.2, 0.0028 * (offset_index + 1), 7001 + index * 5 + offset_index)

    theme = (0, 4, 3, 0, -3, 0, 2, 4, 3, 2, 0, -2)
    for index, degree in enumerate(theme):
        time = 4.0 + index * beat * 1.5
        if time + 1.6 > duration:
            break
        pitch = note(MERIDIAN_SCALE[degree % len(MERIDIAN_SCALE)] + 12 * (degree // len(MERIDIAN_SCALE)))
        voice_prepared_piano(buffer, time, pitch, 1.5, 0.46, 0.0, 7331 + index)

    for index in range(bars):
        if index % 3 == 2:
            voice_ceramic(buffer, index * bar + beat * 2.0, note(KHARUUN_SCALE[4] + 12), 2.2, 0.24, 0.35)
        if index % 4 == 1:
            voice_mechanical(buffer, index * bar + beat, 132.0, 0.9, 0.24, -0.35, 7559 + index)

    buffer.normalize(0.84)
    return buffer


def music_act(duration: float, act: int) -> Buffer:
    """Act beds. I holds; II erodes; III commits."""
    buffer = Buffer(duration, 2)
    beat = 60.0 / (88.0 + 6.0 * act)
    bar = beat * 4.0
    bars = int(duration / bar)
    scale = (MERIDIAN_SCALE, KHARUUN_SCALE, CHOIR_SCALE)[act - 1]
    root = note(scale[0] - 12)

    for bar_index in range(bars):
        time = bar_index * bar
        density = 1 if act == 1 else (2 if act == 2 else 4)
        for sub_index in range(density):
            voice_sub(buffer, time + sub_index * (bar / density), root * 0.5, (bar / density) * 0.86, 0.24)

    progressions = (
        ((0, 4, 7), (0, 5, 9), (0, 3, 7), (0, 4, 7)),
        ((0, 3, 7), (0, 3, 8), (0, 2, 7), (0, 5, 8)),
        ((0, 4, 7), (0, 6, 11), (0, 5, 9), (0, 7, 12)),
    )[act - 1]
    for bar_index in range(bars):
        chord = progressions[bar_index % len(progressions)]
        time = bar_index * bar
        if time + bar > duration:
            break
        for offset_index, offset in enumerate(chord):
            voice_choir_pad(buffer, time, note(scale[0] + offset - 12), bar * 1.15, 0.22, 0.003 * (offset_index + 1), 8101 + act * 31 + bar_index * 3 + offset_index)

    motif = ((0, 2, 4, 2), (0, -2, 3, 2), (0, 4, 7, 4))[act - 1]
    for index in range(bars * 2):
        time = index * (bar / 2.0)
        if time + 1.3 > duration:
            break
        degree = motif[index % len(motif)]
        pitch = note(scale[degree % len(scale)] + 12)
        if act == 2:
            voice_ceramic(buffer, time, pitch, 1.25, 0.3, -0.2 if index % 2 else 0.2)
        else:
            voice_prepared_piano(buffer, time, pitch, 1.25, 0.36, -0.2 if index % 2 else 0.2, 8501 + act * 17 + index)

    buffer.normalize(0.8)
    return buffer


def music_tension(duration: float) -> Buffer:
    """Layer one: something is coming. No resolution, no downbeat certainty."""
    buffer = Buffer(duration, 2)
    beat = 60.0 / 108.0
    root = note(MERIDIAN_SCALE[0] - 24)
    steps = int(duration / (beat * 0.5))
    for step in range(steps):
        time = step * beat * 0.5
        if step % 6 in (0, 3):
            voice_sub(buffer, time, root, beat * 0.45, 0.3)
        if step % 8 == 5:
            voice_mechanical(buffer, time, 210.0, 0.5, 0.2, 0.3 if step % 16 else -0.3, 9001 + step)
    for index in range(int(duration / 4.0)):
        time = index * 4.0
        if time + 4.4 > duration:
            break
        voice_choir_pad(buffer, time, note(MERIDIAN_SCALE[0] - 12), 4.4, 0.18, 0.006, 9311 + index)
        voice_choir_pad(buffer, time, note(MERIDIAN_SCALE[0] - 12) * 1.4983, 4.4, 0.12, 0.005, 9377 + index)
    buffer.normalize(0.7)
    return buffer


def music_combat(duration: float) -> Buffer:
    """Layer two: engaged. Rhythm carries it; the melody stays out of the way."""
    buffer = Buffer(duration, 2)
    beat = 60.0 / 132.0
    root = note(KHARUUN_SCALE[0] - 12)
    steps = int(duration / (beat * 0.5))
    for step in range(steps):
        time = step * beat * 0.5
        if step % 2 == 0:
            voice_stone(buffer, time, root * 2.0, 0.24, 0.42, -0.3, 10007 + step)
        if step % 3 == 1:
            voice_stone(buffer, time, root * 3.0, 0.2, 0.3, 0.3, 10259 + step)
        if step % 8 == 0:
            voice_sub(buffer, time, root * 0.5, beat * 1.6, 0.34)
    for index in range(int(duration / (beat * 4))):
        time = index * beat * 4.0
        if time + 1.6 > duration:
            break
        chord = ((0, 7), (0, 5), (0, 3), (0, 7))[index % 4]
        for offset in chord:
            voice_brass(buffer, time, note(KHARUUN_SCALE[0] + offset - 12), beat * 3.6, 0.24, 0.0)
    buffer.normalize(0.85)
    return buffer


def music_stinger(duration: float, kind: str) -> Buffer:
    """Short outcome cues. Each states a consequence, never a moral score."""
    buffer = Buffer(duration, 2)
    if kind == "victory":
        chord = (0, 4, 7, 12)
        for index, offset in enumerate(chord):
            voice_brass(buffer, index * 0.09, note(MERIDIAN_SCALE[0] + offset - 12), duration - index * 0.09, 0.3, -0.3 + 0.2 * index)
        voice_prepared_piano(buffer, 0.0, note(MERIDIAN_SCALE[0] + 12), duration * 0.8, 0.4, 0.0, 11003)
    elif kind == "defeat":
        chord = (0, 3, 7)
        for index, offset in enumerate(chord):
            voice_choir_pad(buffer, 0.0, note(MERIDIAN_SCALE[0] + offset - 24), duration, 0.3, 0.004 * (index + 1), 11311 + index)
        voice_ceramic(buffer, 0.2, note(MERIDIAN_SCALE[0] - 12), duration * 0.7, 0.3, 0.0)
    elif kind == "restoration":
        for index, offset in enumerate((0, 5, 9, 14)):
            voice_choir_pad(buffer, index * 0.3, note(CHOIR_SCALE[0] + offset - 12), duration - index * 0.3, 0.26, 0.0025 * (index + 1), 11617 + index)
    elif kind == "stabilization":
        for index, offset in enumerate((0, 7, 12)):
            voice_brass(buffer, index * 0.15, note(MERIDIAN_SCALE[0] + offset - 12), duration - index * 0.15, 0.26, 0.0)
        voice_mechanical(buffer, 0.1, 150.0, duration * 0.6, 0.24, 0.0, 11923)
    elif kind == "extinguishment":
        voice_sub(buffer, 0.0, note(MERIDIAN_SCALE[0] - 24), duration, 0.34)
        for index, offset in enumerate((0, 1, 6)):
            voice_choir_pad(buffer, 0.0, note(CHOIR_SCALE[0] + offset - 24), duration, 0.22, 0.007 * (index + 1), 12227 + index)
    else:  # open evolution
        for index, offset in enumerate((0, 4, 7, 11, 14)):
            voice_choir_pad(buffer, index * 0.22, note(CHOIR_SCALE[0] + offset - 12), duration - index * 0.22, 0.2, 0.0035 * (index + 1), 12539 + index)
    buffer.normalize(0.84)
    return buffer


# ---------------------------------------------------------------------------
# Ambience
# ---------------------------------------------------------------------------


def ambience_glass_scar(duration: float) -> Buffer:
    """Wind across vitrified glass, with the basin's structural resonance."""
    buffer = Buffer(duration, 2)
    frames = buffer.frames
    wind_left = OnePole(420.0)
    wind_right = OnePole(390.0)
    body_left = OnePole(120.0)
    body_right = OnePole(115.0)
    for i in range(frames):
        t = i / SAMPLE_RATE
        gust = 0.5 + 0.5 * math.sin(math.tau * 0.037 * t + 0.7) * math.sin(math.tau * 0.011 * t)
        left = wind_left.step(lcg_noise(i, 13001)) * (0.16 + 0.2 * gust)
        right = wind_right.step(lcg_noise(i, 13007)) * (0.16 + 0.2 * gust)
        left += body_left.step(lcg_noise(i, 13009)) * 0.16
        right += body_right.step(lcg_noise(i, 13037)) * 0.16
        shard = 0.0
        if (i % 96000) < 4800:
            phase = (i % 96000) / 4800.0
            shard = math.sin(math.tau * 2340.0 * t) * (1.0 - phase) * 0.045
        buffer.add_stereo(i, left + shard, right + shard * 0.6)
    buffer.normalize(0.52)
    return buffer


def ambience_lume_reach(duration: float) -> Buffer:
    """A settlement still running: low mechanical hum, distant activity."""
    buffer = Buffer(duration, 2)
    hum_left = OnePole(260.0)
    hum_right = OnePole(250.0)
    for i in range(buffer.frames):
        t = i / SAMPLE_RATE
        drone = 0.09 * (math.sin(math.tau * 58.0 * t) + 0.5 * math.sin(math.tau * 87.0 * t + 0.4))
        left = hum_left.step(lcg_noise(i, 14009)) * 0.13 + drone
        right = hum_right.step(lcg_noise(i, 14011)) * 0.13 + drone * 0.92
        if (i % 144000) < 9600:
            phase = (i % 144000) / 9600.0
            tap = math.sin(math.tau * 780.0 * t) * math.exp(-6.0 * phase) * 0.05
            left += tap
            right += tap * 0.7
        buffer.add_stereo(i, left, right)
    buffer.normalize(0.5)
    return buffer


def ambience_ark_city(duration: float) -> Buffer:
    """Districts on reserve power: an unsteady grid you can hear failing."""
    buffer = Buffer(duration, 2)
    grid_left = OnePole(180.0)
    grid_right = OnePole(172.0)
    for i in range(buffer.frames):
        t = i / SAMPLE_RATE
        sag = 1.0 + 0.05 * math.sin(math.tau * 0.13 * t) + 0.03 * math.sin(math.tau * 0.41 * t + 1.1)
        mains = 0.1 * math.sin(math.tau * 50.0 * sag * t) + 0.05 * math.sin(math.tau * 150.0 * sag * t)
        left = grid_left.step(lcg_noise(i, 15013)) * 0.11 + mains
        right = grid_right.step(lcg_noise(i, 15017)) * 0.11 + mains * 0.95
        if (i % 192000) < 2400:
            phase = (i % 192000) / 2400.0
            fault = lcg_noise(i, 15053) * (1.0 - phase) * 0.09
            left += fault
            right += fault * 0.5
        buffer.add_stereo(i, left, right)
    buffer.normalize(0.5)
    return buffer


def ambience_crownfall(duration: float) -> Buffer:
    """The index site. Wide, quiet, and listening back."""
    buffer = Buffer(duration, 2)
    air_left = OnePole(320.0)
    air_right = OnePole(300.0)
    tail_left = CombDelay(0.137, 0.62, 3200.0)
    tail_right = CombDelay(0.149, 0.6, 3000.0)
    for i in range(buffer.frames):
        t = i / SAMPLE_RATE
        breath = 0.5 + 0.5 * math.sin(math.tau * 0.023 * t)
        left = air_left.step(lcg_noise(i, 16019)) * (0.1 + 0.12 * breath)
        right = air_right.step(lcg_noise(i, 16033)) * (0.1 + 0.12 * breath)
        tone = 0.05 * math.sin(math.tau * 110.0 * t) * breath
        left = tail_left.step(left + tone) * 0.6 + left
        right = tail_right.step(right + tone) * 0.6 + right
        buffer.add_stereo(i, left, right)
    buffer.normalize(0.48)
    return buffer


def ambience_well(duration: float) -> Buffer:
    """The Future Well itself: a held possibility with a pulse under it."""
    buffer = Buffer(duration, 2)
    shaper_left = OnePole(900.0)
    shaper_right = OnePole(860.0)
    for i in range(buffer.frames):
        t = i / SAMPLE_RATE
        pulse = 0.5 + 0.5 * math.sin(math.tau * 0.31 * t)
        core = 0.13 * math.sin(math.tau * 146.83 * t) + 0.08 * math.sin(math.tau * 220.0 * t + 0.6)
        upper = 0.05 * math.sin(math.tau * 587.33 * t + 1.2) * pulse
        left = shaper_left.step(lcg_noise(i, 17021)) * 0.05 + core + upper
        right = shaper_right.step(lcg_noise(i, 17027)) * 0.05 + core * 0.94 + upper * 1.1
        buffer.add_stereo(i, left, right)
    buffer.normalize(0.5)
    return buffer


# ---------------------------------------------------------------------------
# Cue registry
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class CueSpec:
    asset_name: str
    category: str
    revision: str
    role: str
    duration_seconds: float = 0.0
    channels: int = 1
    sample_synth: Callable[[float, int], float] | None = None
    buffer_synth: Callable[[], Buffer] | None = None
    looping: bool = False
    tags: Sequence[str] = field(default_factory=tuple)

    @property
    def asset_path(self) -> str:
        return f"/Game/Audio/Generated/{self.asset_name}"


def _mono(name: str, category: str, revision: str, duration: float, role: str, synth, tags=()) -> CueSpec:
    return CueSpec(
        asset_name=name,
        category=category,
        revision=revision,
        role=role,
        duration_seconds=duration,
        channels=1,
        sample_synth=synth,
        tags=tuple(tags),
    )


def _stereo(name: str, category: str, revision: str, role: str, factory, looping: bool = False, tags=()) -> CueSpec:
    return CueSpec(
        asset_name=name,
        category=category,
        revision=revision,
        role=role,
        channels=2,
        buffer_synth=factory,
        looping=looping,
        tags=tuple(tags),
    )


CUES: tuple[CueSpec, ...] = (
    # presentation-audio-v1 — must regenerate byte-identically
    _mono("SFX_CommandConfirm", CATEGORY_INTERFACE, REVISION_PRESENTATION, 0.14, "Accepted local command confirmation", command_confirm),
    _mono("SFX_DestructionMeridian", CATEGORY_EFFECTS, REVISION_PRESENTATION, 0.46, "Meridian functional-loss confirmation", meridian_destruction),
    _mono("SFX_DestructionKharuun", CATEGORY_EFFECTS, REVISION_PRESENTATION, 0.52, "Kharuun functional-loss confirmation", kharuun_destruction),
    _mono("SFX_DestructionChoir", CATEGORY_EFFECTS, REVISION_PRESENTATION, 0.58, "Hollow Choir functional-loss confirmation", choir_destruction),

    # interface-audio-v1
    _mono("UI_Hover", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.045, "Menu focus moved", ui_hover),
    _mono("UI_Select", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.085, "Menu item selected", ui_select),
    _mono("UI_Confirm", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.2, "Menu choice committed", ui_confirm),
    _mono("UI_Reject", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.26, "Invalid action with a stable reason", ui_reject),
    _mono("UI_MenuOpen", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.3, "Modal panel opened", ui_menu_open),
    _mono("UI_MenuClose", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.3, "Modal panel closed", ui_menu_close),
    _mono("UI_BriefAdvance", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.24, "Brief or result advanced", ui_brief_advance),
    _mono("ALERT_UnderAttack", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.62, "Owned entity under attack", alert_under_attack, ("alert",)),
    _mono("ALERT_StructureLost", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.7, "Owned structure removed", alert_structure_lost, ("alert",)),
    _mono("ALERT_ProductionComplete", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.4, "Production finished", alert_production_complete, ("alert",)),
    _mono("ALERT_ResearchComplete", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.55, "Research project finished", alert_research_complete, ("alert",)),
    _mono("ALERT_CapacityLow", CATEGORY_INTERFACE, REVISION_INTERFACE, 0.45, "Logistics capacity nearly exhausted", alert_capacity_low, ("alert",)),

    # gameplay-audio-v1
    _mono("SFX_WeaponLight", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.13, "Scout and worker-scale weapon", weapon_light),
    _mono("SFX_WeaponLine", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.2, "Line unit weapon", weapon_line),
    _mono("SFX_WeaponHeavy", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.34, "Heavy screen weapon", weapon_heavy),
    _mono("SFX_ImpactHit", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.16, "Damage landed on a target", impact_hit),
    _mono("SFX_ImpactShielded", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.22, "Damage absorbed by adaptation or cover", impact_shielded),
    _mono("SFX_GatherMatter", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.24, "Matter gathered from a deposit", gather_matter),
    _mono("SFX_DeliverMatter", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.3, "Matter delivered to a dropoff", deliver_matter),
    _mono("SFX_ConstructionStart", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.42, "Construction began at a site", construction_start),
    _mono("SFX_ConstructionComplete", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.5, "Structure completed", construction_complete),
    _mono("SFX_ProductionComplete", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.36, "Produced unit emerged", production_complete),
    _mono("SFX_ResearchStart", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.34, "Technology project started", research_start),
    _mono("SFX_ResearchInterrupted", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.38, "Technology project interrupted without refund", research_interrupted),
    _mono("SFX_WellClaim", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.9, "Future Well claimed", well_claim),
    _mono("SFX_WellHarvest", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 1.1, "Future Well committed to Harvest", well_harvest),
    _mono("SFX_WellPreserve", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 1.3, "Future Well committed to Preserve", well_preserve),
    _mono("SFX_WellReshape", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 1.2, "Future Well committed to Reshape", well_reshape),
    _mono("SFX_ReshapeOpen", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.8, "Reshaped terrain opened", reshape_open),
    _mono("SFX_ReshapeClose", CATEGORY_EFFECTS, REVISION_GAMEPLAY, 0.8, "Reshaped terrain closed", reshape_close),

    # music-v1
    _stereo("MUS_Title", CATEGORY_MUSIC, REVISION_MUSIC, "Title and archive theme", lambda: music_title(40.0), looping=True),
    _stereo("MUS_Meridian", CATEGORY_MUSIC, REVISION_MUSIC, "Meridian Compact theme", lambda: music_meridian(32.0), looping=True),
    _stereo("MUS_Kharuun", CATEGORY_MUSIC, REVISION_MUSIC, "Kharuun Assemblies theme", lambda: music_kharuun(32.0), looping=True),
    _stereo("MUS_Choir", CATEGORY_MUSIC, REVISION_MUSIC, "Hollow Choir theme", lambda: music_choir(32.0), looping=True),
    _stereo("MUS_ActI", CATEGORY_MUSIC, REVISION_MUSIC, "Act I bed — Necessary Fires", lambda: music_act(32.0, 1), looping=True),
    _stereo("MUS_ActII", CATEGORY_MUSIC, REVISION_MUSIC, "Act II bed — The Cost of One Future", lambda: music_act(32.0, 2), looping=True),
    _stereo("MUS_ActIII", CATEGORY_MUSIC, REVISION_MUSIC, "Act III bed — Crownfall", lambda: music_act(32.0, 3), looping=True),
    _stereo("MUS_TensionLayer", CATEGORY_MUSIC, REVISION_MUSIC, "Tension layer", lambda: music_tension(24.0), looping=True),
    _stereo("MUS_CombatLayer", CATEGORY_MUSIC, REVISION_MUSIC, "Combat layer", lambda: music_combat(24.0), looping=True),
    _stereo("MUS_Victory", CATEGORY_MUSIC, REVISION_MUSIC, "Operation succeeded", lambda: music_stinger(6.0, "victory")),
    _stereo("MUS_Defeat", CATEGORY_MUSIC, REVISION_MUSIC, "Operation failed", lambda: music_stinger(6.0, "defeat")),
    _stereo("MUS_EndingRestoration", CATEGORY_MUSIC, REVISION_MUSIC, "Restoration ending", lambda: music_stinger(9.0, "restoration")),
    _stereo("MUS_EndingStabilization", CATEGORY_MUSIC, REVISION_MUSIC, "Controlled Stabilization ending", lambda: music_stinger(9.0, "stabilization")),
    _stereo("MUS_EndingExtinguishment", CATEGORY_MUSIC, REVISION_MUSIC, "Extinguishment ending", lambda: music_stinger(9.0, "extinguishment")),
    _stereo("MUS_EndingOpenEvolution", CATEGORY_MUSIC, REVISION_MUSIC, "Open Evolution ending", lambda: music_stinger(9.0, "evolution")),

    # ambience-v1
    _stereo("AMB_GlassScar", CATEGORY_AMBIENCE, REVISION_AMBIENCE, "Glass Scar basin bed", lambda: ambience_glass_scar(30.0), looping=True),
    _stereo("AMB_LumeReach", CATEGORY_AMBIENCE, REVISION_AMBIENCE, "Lume Reach settlement bed", lambda: ambience_lume_reach(30.0), looping=True),
    _stereo("AMB_ArkCity", CATEGORY_AMBIENCE, REVISION_AMBIENCE, "Ark-city reserve grid bed", lambda: ambience_ark_city(30.0), looping=True),
    _stereo("AMB_Crownfall", CATEGORY_AMBIENCE, REVISION_AMBIENCE, "Crownfall index bed", lambda: ambience_crownfall(30.0), looping=True),
    _stereo("AMB_FutureWell", CATEGORY_AMBIENCE, REVISION_AMBIENCE, "Future Well proximity bed", lambda: ambience_well(24.0), looping=True),
)


def cue_by_name(name: str) -> CueSpec:
    for spec in CUES:
        if spec.asset_name == name:
            return spec
    raise KeyError(name)


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------


def render(spec: CueSpec) -> bytes:
    if spec.buffer_synth is not None:
        return spec.buffer_synth().to_wave_bytes()
    if spec.sample_synth is None:
        raise RuntimeError(f"Cue has no synthesis path: {spec.asset_name}")
    frame_count = int(round(spec.duration_seconds * SAMPLE_RATE))
    payload = io.BytesIO()
    with wave.open(payload, "wb") as writer:
        writer.setnchannels(1)
        writer.setsampwidth(2)
        writer.setframerate(SAMPLE_RATE)
        frames = bytearray()
        for sample_index in range(frame_count):
            t = sample_index / SAMPLE_RATE
            sample = max(-0.96, min(0.96, spec.sample_synth(t, sample_index)))
            frames.extend(struct.pack("<h", int(round(sample * 32767.0))))
        writer.writeframes(bytes(frames))
    return payload.getvalue()


def source_path(source_dir: str, spec: CueSpec) -> str:
    return os.path.join(source_dir, f"{spec.asset_name}.wav")


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


def measure_peak_and_rms(payload: bytes) -> tuple[float, float]:
    with wave.open(io.BytesIO(payload), "rb") as reader:
        frames = reader.readframes(reader.getnframes())
    count = len(frames) // 2
    if count == 0:
        return 0.0, 0.0
    peak = 0
    total = 0.0
    for index in range(count):
        value = struct.unpack_from("<h", frames, index * 2)[0]
        magnitude = -value if value < 0 else value
        if magnitude > peak:
            peak = magnitude
        total += float(value) * float(value)
    return peak / 32768.0, math.sqrt(total / count) / 32768.0


def generate(source_dir: str, only: Sequence[str] | None = None, write: bool = True) -> list[dict]:
    records: list[dict] = []
    for spec in CUES:
        if only and spec.asset_name not in only:
            continue
        payload = render(spec)
        digest = hashlib.sha256(payload).hexdigest()
        action = write_if_changed(source_path(source_dir, spec), payload) if write else "rendered"
        peak, rms = measure_peak_and_rms(payload)
        records.append(
            {
                "asset": spec.asset_name,
                "category": spec.category,
                "revision": spec.revision,
                "channels": spec.channels,
                "bytes": len(payload),
                "sha256": digest,
                "peak": round(peak, 5),
                "rms": round(rms, 5),
                "looping": spec.looping,
                "action": action,
            }
        )
    return records


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", default=None, help="Content/Audio/Source directory")
    parser.add_argument("--write", action="store_true", help="Write WAV sources to disk")
    parser.add_argument("--only", nargs="*", default=None, help="Limit to named assets")
    arguments = parser.parse_args()

    source_dir = arguments.source_dir
    if source_dir is None:
        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        source_dir = os.path.join(project_root, "Content", "Audio", "Source")

    records = generate(source_dir, arguments.only, arguments.write)
    for record in records:
        print(
            f"[ECHOES_AUDIO_SOURCE] cue={record['asset']} category={record['category']} "
            f"revision={record['revision']} action={record['action']} channels={record['channels']} "
            f"bytes={record['bytes']} peak={record['peak']} rms={record['rms']} sha256={record['sha256']}"
        )
    print(
        f"[ECHOES_AUDIO_SOURCE_READY] cues={len(records)} sampleRate={SAMPLE_RATE} "
        "sourcesOriginal=true thirdPartySamples=false finalAudio=false"
    )


if __name__ == "__main__":
    main()
