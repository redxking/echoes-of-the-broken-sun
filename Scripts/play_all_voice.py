#!/usr/bin/env python3
"""Play the campaign voice corpus in narrative order, for one listening pass.

Aesthetic judgement of the voices is the owner's and nobody else's. This
script exists so that pass is one sitting rather than a project: it walks the
authored lines in narrative order, prints who is speaking and what the line
says, and plays the bound asset for it.

    python3 Scripts/play_all_voice.py --list
    python3 Scripts/play_all_voice.py --surface demo.tutorial
    python3 Scripts/play_all_voice.py --speaker "Mara Vey" --start 12
    python3 Scripts/play_all_voice.py --playlist ~/Desktop/echoes-voice.m3u

`--playlist` writes an M3U with each line's speaker and text as its title, so
the pass can happen in an ordinary audio player with seek and skip instead of
in a terminal. `--list` prints the running order and plays nothing.

This script records no verdict and changes no state. It is a listening aid,
not a gate, and it cannot mark anything accepted.

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PACK = ROOT / "Content/Narrative/Generated/EchoesNarrativePack.json"
VOICE_DIR = ROOT / "Content/Audio/Source/Narrative"
BINDINGS = VOICE_DIR / "m01_voice_bindings.json"


def hook_for_bound_lines() -> dict[str, str]:
    """M01 assets are named by logical audio hook, not by line id."""
    if not BINDINGS.is_file():
        return {}
    body = json.loads(BINDINGS.read_text(encoding="utf-8"))
    mapping = {}
    for row in body.get("lines", []):
        asset = row.get("asset_path", "")
        if row.get("line_id") and asset:
            mapping[row["line_id"]] = asset.rsplit(".", 1)[-1]
    return mapping


def running_order() -> list[dict]:
    """Authored lines in narrative order: the demo surfaces, then the operations."""
    pack = json.loads(PACK.read_text(encoding="utf-8"))
    hooks = hook_for_bound_lines()
    rows: list[dict] = []

    def emit(surface: str, lines: list[dict]) -> None:
        for line in lines:
            if not (line.get("speaker") and line.get("text")):
                continue
            stem = hooks.get(line["id"], line["id"])
            path = VOICE_DIR / f"{stem}.wav"
            rows.append({"surface": surface, "line_id": line["id"],
                         "speaker": line["speaker"], "text": line["text"],
                         "path": path, "voiced": path.is_file()})

    for surface in ("tutorial", "system_voice"):
        body = pack.get("demo", {}).get(surface)
        if body:
            emit(f"demo.{surface}", body.get("lines", []))
    for operation, body in sorted(pack.get("operations", {}).items()):
        emit(operation, body.get("lines", []))
    return rows


def duration_of(path: Path) -> float:
    try:
        with wave.open(str(path), "rb") as reader:
            return reader.getnframes() / reader.getframerate()
    except Exception:
        return 0.0


def write_playlist(rows: list[dict], target: Path) -> int:
    entries = [row for row in rows if row["voiced"]]
    lines = ["#EXTM3U"]
    for row in entries:
        title = f"{row['surface']} | {row['speaker']}: {row['text']}"
        lines.append(f"#EXTINF:{int(round(duration_of(row['path'])))},{title}")
        lines.append(str(row["path"].resolve()))
    target.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return len(entries)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--surface", default=None, help="e.g. demo.tutorial, CampaignPrologue")
    parser.add_argument("--speaker", default=None)
    parser.add_argument("--start", type=int, default=1, help="1-based index into the order")
    parser.add_argument("--list", action="store_true", help="print the order and play nothing")
    parser.add_argument("--playlist", default=None, help="write an M3U instead of playing")
    parser.add_argument("--include-unvoiced", action="store_true")
    args = parser.parse_args()

    rows = running_order()
    total_authored = len(rows)
    voiced = sum(1 for row in rows if row["voiced"])
    if args.surface:
        rows = [row for row in rows if row["surface"] == args.surface]
    if args.speaker:
        rows = [row for row in rows if row["speaker"] == args.speaker]
    if not args.include_unvoiced:
        rows = [row for row in rows if row["voiced"]]
    rows = rows[max(0, args.start - 1):]

    print(f"[ECHOES_VOICE_PLAYALL] authored={total_authored} voiced={voiced} "
          f"selected={len(rows)} listeningVerified=false")
    if not rows:
        print("nothing selected", file=sys.stderr)
        return 1

    if args.playlist:
        target = Path(args.playlist).expanduser().resolve()
        count = write_playlist(rows, target)
        print(f"[ECHOES_VOICE_PLAYLIST] entries={count} path={target}")
        return 0

    if args.list:
        for index, row in enumerate(rows, start=args.start):
            mark = " " if row["voiced"] else "!"
            print(f"{mark}{index:4d}  {row['surface']:<28} {row['speaker']:<24} {row['text'][:70]}")
        return 0

    player = shutil.which("afplay")
    if not player:
        print("afplay not found; use --playlist and an audio player instead", file=sys.stderr)
        return 2

    print("Ctrl-C stops. Nothing here records a verdict.\n")
    for index, row in enumerate(rows, start=args.start):
        print(f"{index:4d}/{len(rows) + args.start - 1}  {row['surface']}  "
              f"{row['speaker']}\n      \"{row['text']}\"", flush=True)
        if not row["voiced"]:
            print("      (no asset — unvoiced)", flush=True)
            continue
        try:
            subprocess.run([player, str(row["path"])], check=False)
        except KeyboardInterrupt:
            print("\nstopped", flush=True)
            return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
