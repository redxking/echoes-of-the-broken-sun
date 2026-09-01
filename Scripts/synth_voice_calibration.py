"""Gate 17 calibration: deterministic Kokoro-82M synthesis of pack lines.

Reads the compiled narrative pack, synthesizes a fixed calibration set with
pinned per-character voices, and proves byte-reproducibility by synthesizing
every line twice and comparing digests. Author: Angelis Pseftis.
"""
import hashlib
import json
import pathlib
import sys

import numpy as np
import soundfile as sf
from kokoro_onnx import Kokoro

import os

ROOT = pathlib.Path(__file__).resolve().parent.parent
PACK = ROOT / "Content/Narrative/Generated/EchoesNarrativePack.json"
HERE = pathlib.Path(
    os.environ.get(
        "ECHOES_KOKORO_DIR", str(ROOT.parent / "Tools" / "kokoro")))
OUT = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "calibration"

# Pinned creative mapping, awaiting the owner's listening review (OD 1 path).
VOICES = {
    "Mara Vey": ("af_sarah", 1.0),
    "Oruun-of-Seven-Stones": ("bm_george", 0.92),
    "Talar Venn": ("am_michael", 1.0),
    "Neme": ("af_nicole", 0.95),
    "Chancellor Cael Rhyse": ("bm_lewis", 0.95),
}
# One early and one late line per speaker, chosen by stable line id.
CALIBRATION_LINE_IDS = [
    "nar_m01_line_mara_001",
    "nar_m03_line_op_grid_001",
    "nar_m05_line_op_terms_001",
    "nar_m05_line_complete_unsigned_002",
    "nar_m04_line_op_road_001",
    "nar_m04_line_complete_dispute_002",
    "nar_m06_line_op_names_001",
    "nar_m08_line_op_beside_001",
    "nar_m14_line_op_voices_001",
    "nar_m12_line_op_won_001",
]

pack = json.loads(PACK.read_text())
lines_by_id = {}
for op in pack["operations"].values():
    for line in op.get("lines", []):
        lines_by_id[line["id"]] = line

kokoro = Kokoro(str(HERE / "kokoro-v1.0.onnx"), str(HERE / "voices-v1.0.bin"))
OUT.mkdir(parents=True, exist_ok=True)
manifest = []
for line_id in CALIBRATION_LINE_IDS:
    line = lines_by_id.get(line_id)
    if line is None:
        print(f"SKIP {line_id}: not in pack")
        continue
    voice, speed = VOICES[line["speaker"]]
    digests = []
    for attempt in range(2):
        samples, sample_rate = kokoro.create(
            line["text"], voice=voice, speed=speed, lang="en-us")
        pcm = np.asarray(samples, dtype=np.float32).tobytes()
        digests.append(hashlib.sha256(pcm).hexdigest())
        if attempt == 0:
            wav = OUT / f"{line_id}.{voice}.wav"
            sf.write(str(wav), samples, sample_rate)
    reproducible = digests[0] == digests[1]
    manifest.append({
        "line_id": line_id, "speaker": line["speaker"], "voice": voice,
        "speed": speed, "sha256_pcm": digests[0],
        "reproducible": reproducible, "text": line["text"],
    })
    print(f"{'OK ' if reproducible else 'NONDET '}{line_id} voice={voice} sha={digests[0][:12]}")

(OUT / "calibration-manifest.json").write_text(
    json.dumps({"model": "Kokoro-82M (kokoro-v1.0.onnx)",
                "license": "Apache-2.0",
                "runtime": "kokoro-onnx 0.4.9, onnxruntime CPU, python 3.12",
                "lines": manifest}, indent=1) + "\n")
bad = [m for m in manifest if not m["reproducible"]]
print(f"CALIBRATION_{'OK' if not bad and len(manifest)==len(CALIBRATION_LINE_IDS) else 'INCOMPLETE'} lines={len(manifest)} nondeterministic={len(bad)}")
