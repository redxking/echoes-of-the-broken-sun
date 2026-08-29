#!/usr/bin/env python3
"""Create a valid bounded campaign ledger for isolated runtime smoke tests."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


CHOICES = {"Harvest": 1, "Preserve": 2, "Reshape": 3}
CHOICE_MASKS = {"Harvest": 1, "Preserve": 2, "Reshape": 4}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--choice", choices=CHOICES, default="Preserve")
    parser.add_argument("--through-mission", type=int, choices=(1, 2, 3), default=1)
    args = parser.parse_args()

    records = [struct.pack(
        "<BBBBIQQ",
        1,
        CHOICES[args.choice],
        0x07,
        0x0F,
        20,
        120,
        0x7A11A2,
    )]
    if args.through_mission >= 2:
        records.append(struct.pack(
            "<BBBBIQQ",
            2,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x0F,
            20,
            420,
            0x7A11A3,
        ))
    if args.through_mission >= 3:
        records.append(struct.pack(
            "<BBBBIQQ",
            3,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x1F,
            20,
            720,
            0x7A11A4,
        ))
    header = b"ECHOCPG1" + struct.pack("<HH", 1, len(records))
    payload = header + b"".join(records)
    encoded = payload + struct.pack("<I", zlib.crc32(payload) & 0xFFFFFFFF)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(encoded)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
