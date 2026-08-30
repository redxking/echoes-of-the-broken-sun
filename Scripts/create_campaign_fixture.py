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
    parser.add_argument(
        "--lume-choice",
        choices=CHOICES,
        help="Mission 10 Lume Well choice; defaults to the founding choice",
    )
    parser.add_argument(
        "--through-mission",
        type=int,
        choices=tuple(range(13)),
        default=1,
    )
    args = parser.parse_args()
    lume_choice = args.lume_choice or args.choice

    records = []
    if args.through_mission >= 1:
        records.append(struct.pack(
            "<BBBBIQQ",
            1,
            CHOICES[args.choice],
            0x07,
            0x0F,
            21,
            120,
            0x7A11A2,
        ))
    if args.through_mission >= 2:
        records.append(struct.pack(
            "<BBBBIQQ",
            2,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x0F,
            21,
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
            21,
            720,
            0x7A11A4,
        ))
    if args.through_mission >= 4:
        records.append(struct.pack(
            "<BBBBIQQ",
            4,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x1F,
            21,
            1040,
            0x7A11A5,
        ))
    if args.through_mission >= 5:
        records.append(struct.pack(
            "<BBBBIQQ",
            5,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x3F,
            21,
            1340,
            0x7A11A6,
        ))
    if args.through_mission >= 6:
        records.append(struct.pack(
            "<BBBBIQQ",
            6,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x3F,
            21,
            1640,
            0x7A11A7,
        ))
    if args.through_mission >= 7:
        records.append(struct.pack(
            "<BBBBIQQ",
            7,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x3F,
            21,
            1940,
            0x7A11A8,
        ))
    if args.through_mission >= 8:
        records.append(struct.pack(
            "<BBBBIQQ",
            8,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x3F,
            21,
            2240,
            0x7A11A9,
        ))
    if args.through_mission >= 9:
        records.append(struct.pack(
            "<BBBBIQQ",
            9,
            CHOICES[args.choice],
            CHOICE_MASKS[args.choice],
            0x7B,
            21,
            2540,
            0x7A11AA,
        ))
    if args.through_mission >= 10:
        records.append(struct.pack(
            "<BBBBIQQ",
            10,
            CHOICES[lume_choice],
            0x07,
            0xFF,
            21,
            2840,
            0x7A11AB,
        ))
    if args.through_mission >= 11:
        records.append(struct.pack(
            "<BBBBIQQ",
            11,
            CHOICES[lume_choice],
            CHOICE_MASKS[lume_choice],
            0xFF,
            21,
            3140,
            0x7A11AC,
        ))
    if args.through_mission >= 12:
        records.append(struct.pack(
            "<BBBBIQQ",
            12,
            CHOICES[lume_choice],
            CHOICE_MASKS[lume_choice],
            0xFF,
            21,
            3440,
            0x7A11AD,
        ))
    header = b"ECHOCPG1" + struct.pack("<HH", 1, len(records))
    payload = header + b"".join(records)
    encoded = payload + struct.pack("<I", zlib.crc32(payload) & 0xFFFFFFFF)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(encoded)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
