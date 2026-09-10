#!/usr/bin/env python3
"""Ratchet on unsupported verification claims in RequirementsState.md.

D8 (2026-09-10) ruled that a family row states counts only: verification is claimed for a
requirement by a dated per-ID entry, or it is not claimed. Measured against that rule, 373 of the
393 IDs family rows call AGENT VERIFIED have no such entry -- only 20 do -- and four of the
unbacked claims are contradicted by the source.

Failing the whole project on all 373 at once would block every lane for a debt none of them
created, so this is a RATCHET rather than a gate: the count may fall, never rise. A lane that
lands real work and appends its dated per-ID entry moves the number down and re-baselines it.
A lane that adds a new blanket family claim moves it up and is refused.

This checks document self-consistency only. It cannot tell whether a per-ID entry is TRUE -- an
entry that says IMPLEMENTED when the source disagrees passes here and is caught by reading source.
Nothing here grants acceptance; only Angelis does that.

Usage:
    python3 Scripts/check_requirement_evidence.py            # check against the baseline
    python3 Scripts/check_requirement_evidence.py --list      # print the unbacked worklist
    python3 Scripts/check_requirement_evidence.py --write     # re-baseline after real work lands
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
STATE = REPO / "Docs" / "RequirementsState.md"
BASELINE = REPO / "Docs" / "requirement-evidence-baseline.json"

# "| `SPEC-MOV-*` | 13 | 13 `AGENT VERIFIED` (`SPEC-MOV-001..013`) |"
CLAIM_ROW = re.compile(
    r"^\|\s*`(?P<family>[A-Z]+-[A-Z]+)-\*`\s*\|\s*(?P<declared>\d+)\s*\|\s*"
    r"(?P<claimed>\d+)\s*`?AGENT VERIFIED`?\s*\(\s*`(?P<prefix>[A-Z-]+)(?P<lo>\d+)\.\.(?P<hi>\d+)`\s*\)\s*\|"
)


def load_state() -> tuple[str, list[str]]:
    text = STATE.read_text(encoding="utf-8")
    return text, text.splitlines()


def audit() -> dict:
    text, lines = load_state()
    families: list[dict] = []
    for number, line in enumerate(lines, start=1):
        match = CLAIM_ROW.match(line)
        if not match:
            continue
        prefix = match.group("prefix")
        lo, hi = int(match.group("lo")), int(match.group("hi"))
        width = len(match.group("lo"))
        ids = [f"{prefix}{n:0{width}d}" for n in range(lo, hi + 1)]
        # An ID is "backed" only when it HEADS a per-ID entry -- a bullet or table row whose first
        # token is the identifier, which is the shape D8 asks for:
        #     * `SPEC-MOV-003` -- IMPLEMENTED, native evidence. ...
        #     | `SPEC-MOV-003` | ... |
        # Merely mentioning an ID in prose is not evidence. That distinction is not pedantry: the
        # first version of this check counted any mention, and the audit entry's own explanatory
        # text -- which names two IDs as examples of the counting bug -- promptly "backed" them and
        # improved the score by two. A guard that its own documentation can satisfy is not a guard.
        unbacked = []
        for identifier in ids:
            heads_entry = re.search(
                r"^\s*(?:[*+-]|\|)\s*`?" + re.escape(identifier) + r"`?(?![0-9A-Za-z-])",
                text,
                re.MULTILINE,
            )
            if not heads_entry:
                unbacked.append(identifier)
        families.append(
            {
                "family": match.group("family"),
                "line": number,
                "declared": int(match.group("declared")),
                "claimed_verified": int(match.group("claimed")),
                "unbacked": unbacked,
            }
        )
    total_claimed = sum(f["claimed_verified"] for f in families)
    total_unbacked = sum(len(f["unbacked"]) for f in families)
    return {
        "claim_rows": len(families),
        "claimed_verified_ids": total_claimed,
        "unbacked_ids": total_unbacked,
        "families": families,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="re-baseline to the current count")
    parser.add_argument("--list", action="store_true", help="print the unbacked worklist")
    args = parser.parse_args()

    if not STATE.exists():
        print(f"Requirement state file is missing: {STATE}", file=sys.stderr)
        return 2

    result = audit()
    current = result["unbacked_ids"]

    if args.list:
        for family in result["families"]:
            if family["unbacked"]:
                print(f"L{family['line']:<5} {family['family']:<14} "
                      f"{len(family['unbacked'])}/{family['claimed_verified']} unbacked: "
                      f"{', '.join(family['unbacked'])}")
        print(f"\n{current} of {result['claimed_verified_ids']} claimed-verified IDs "
              f"have no dated per-ID entry.")
        return 0

    if args.write:
        BASELINE.write_text(
            json.dumps(
                {
                    "author": "Angelis Pseftis",
                    "rule": "D8 2026-09-10 — a family row states counts only; verification is "
                            "claimed per requirement ID by a dated entry, or it is not claimed.",
                    "ratchet": "unbacked_ids may fall, never rise. Re-baseline only after real "
                               "per-ID entries land, never to make a failure go away.",
                    "boundary": "Document self-consistency only. A per-ID entry that contradicts "
                                "the source passes here; that is caught by reading source. This "
                                "grants no acceptance.",
                    "claimed_verified_ids": result["claimed_verified_ids"],
                    "unbacked_ids": current,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        print(f"Baseline written: {current} unbacked of {result['claimed_verified_ids']} claimed.")
        return 0

    if not BASELINE.exists():
        print(f"No baseline at {BASELINE}. Run with --write once to record the starting count.",
              file=sys.stderr)
        return 2

    baseline = json.loads(BASELINE.read_text(encoding="utf-8"))
    allowed = baseline["unbacked_ids"]

    if current > allowed:
        print(
            f"Unsupported verification claims increased: {current} unbacked, baseline {allowed}.\n"
            f"A family row may not carry verification (D8, 2026-09-10). Append a dated per-ID entry\n"
            f"naming the build, the check run and the evidence class, or do not claim it.\n"
            f"Run --list to see which IDs are unbacked.",
            file=sys.stderr,
        )
        return 1

    if current < allowed:
        print(f"Unsupported verification claims fell to {current} from {allowed}. "
              f"Re-baseline with --write to hold the gain.")
        return 0

    print(f"Requirement evidence ratchet held: {current} unbacked of "
          f"{result['claimed_verified_ids']} claimed-verified IDs across {result['claim_rows']} "
          f"family rows.")
    print("Boundary: document self-consistency only; a per-ID entry is not checked against source, "
          "and nothing here is acceptance.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
