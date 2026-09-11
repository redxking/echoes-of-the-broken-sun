#!/usr/bin/env python3
"""Record a requirement's current state as a structured row instead of free prose.

Author: Angelis Pseftis

    record_state.py --id SPEC-UI-007 --state "AGENT VERIFIED" --class PKG-REND \
        --evidence BuildArtifacts/Evidence/hud-console-20260911T135529Z --commit a8607f8 \
        --note "3x3 deck in frame at 1280x720; owner acceptance open"

Upserts one row per ID in the "Current state by ID" table near the top of Docs/RequirementsState.md
and appends a dated line to the "Structured state journal" at the end, so history is never lost.
Owner-only states (HUMAN ACCEPTED, HUMAN REJECTED — CHANGES REQUIRED, COMPLETE) require --owner,
which records that the value was given by Angelis; agents must not pass it on their own authority.
"""
from __future__ import annotations

import argparse
import datetime as dt
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / 'Docs' / 'RequirementsState.md'
TABLE_H = '## Current state by ID'
JOURNAL_H = '## Structured state journal'
AGENT_STATES = ['OPEN', 'IN PROGRESS', 'IMPLEMENTED', 'AGENT VERIFIED', 'EVIDENCE READY',
                'AWAITING HUMAN ACCEPTANCE', 'BLOCKED']
OWNER_STATES = ['HUMAN ACCEPTED', 'HUMAN REJECTED — CHANGES REQUIRED', 'COMPLETE']
CLASSES = ['PKG-PHYS', 'PKG-REND', 'PKG-AUTO', 'EDT', 'SRC', 'HUM', 'OWNER', 'NONE']
ID_RE = re.compile(r'^(?:SPEC|REL|DEMO|TBR)-[A-Z]+-\d{3}$')

TABLE_INTRO = f'''{TABLE_H}

Machine-maintained by `Scripts/record_state.py`; read it with `Scripts/req.py state <ID>`. One row per
requirement, the newest verdict wins, and every change also lands as a dated line in the
[structured state journal](#structured-state-journal). Rows use the [state vocabulary](#state-vocabulary);
owner-only values are written only on the owner's recorded instruction. IDs without a row keep the record
defaults and any dated entry below. This table is a view of decisions, not a new authority.

| ID | State | Class | Evidence | Commit | Date | Note |
|---|---|---|---|---|---|---|
'''


def git_head() -> str:
    try:
        return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], cwd=ROOT, text=True).strip()
    except Exception:
        return ''


def esc(s: str) -> str:
    return s.replace('|', '\\|').replace('\n', ' ').strip()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--id', required=True, action='append', help='requirement ID (repeatable)')
    p.add_argument('--state', required=True, choices=AGENT_STATES + OWNER_STATES)
    p.add_argument('--class', dest='cls', default='NONE', choices=CLASSES, help='evidence class')
    p.add_argument('--evidence', default='', help='retained evidence path (absolute or checkout-relative)')
    p.add_argument('--commit', default='', help='source commit; default: current HEAD')
    p.add_argument('--note', default='')
    p.add_argument('--owner', action='store_true', help='value was given by Angelis (required for owner-only states)')
    p.add_argument('--dry-run', action='store_true')
    a = p.parse_args()

    ids = [i.upper() for i in a.id]
    bad = [i for i in ids if not ID_RE.match(i)]
    if bad:
        sys.exit(f'not a requirement ID: {bad}')
    if a.state in OWNER_STATES and not a.owner:
        sys.exit(f'{a.state} is owner-only; pass --owner only when Angelis gave it')
    commit = a.commit or git_head()
    today = dt.date.today().isoformat()
    stamp = dt.datetime.now(dt.timezone.utc).strftime('%Y-%m-%dT%H:%MZ')
    owner_tag = ' (owner)' if a.owner else ''

    text = STATE.read_text(encoding='utf-8')
    lines = text.split('\n')

    # Ensure the table exists, placed right after the State vocabulary section.
    if TABLE_H not in text:
        idx = next(i for i, l in enumerate(lines) if l.startswith('## State vocabulary'))
        nxt = next(i for i in range(idx + 1, len(lines)) if lines[i].startswith('## '))
        lines[nxt:nxt] = TABLE_INTRO.rstrip('\n').split('\n') + ['']
    # Ensure the journal exists at the end.
    if JOURNAL_H not in '\n'.join(lines):
        while lines and not lines[-1].strip():
            lines.pop()
        lines += ['', JOURNAL_H, '',
                  'Append-only, written by `Scripts/record_state.py`. Each line: timestamp, IDs, state, class,',
                  'evidence, commit, note. Dated narrative sections above remain the place for reasoning.', '']

    t_start = lines.index(TABLE_H)
    hdr = next(i for i in range(t_start, len(lines)) if lines[i].startswith('|---'))
    t_end = hdr + 1
    while t_end < len(lines) and lines[t_end].startswith('|'):
        t_end += 1

    for rid in ids:
        row = (f'| `{rid}` | {esc(a.state)}{owner_tag} | {a.cls} | {esc(a.evidence) or "—"} | '
               f'{commit or "—"} | {today} | {esc(a.note) or "—"} |')
        existing = [i for i in range(hdr + 1, t_end) if lines[i].startswith(f'| `{rid}` |')]
        if existing:
            lines[existing[0]] = row
        else:
            lines.insert(t_end, row)
            t_end += 1
    # keep the table sorted by ID for stable diffs
    body = sorted(lines[hdr + 1:t_end])
    lines[hdr + 1:t_end] = body

    j = lines.index(JOURNAL_H)
    entry = (f'- {stamp} — {", ".join(f"`{i}`" for i in ids)} → **{a.state}**{owner_tag}; class {a.cls}; '
             f'evidence {a.evidence or "—"}; commit {commit or "—"}; {a.note or "no note"}')
    # append after the journal intro paragraph
    end = len(lines)
    while end > j and not lines[end - 1].strip():
        end -= 1
    lines[end:end] = [entry]
    out = '\n'.join(lines).rstrip('\n') + '\n'

    if a.dry_run:
        print(row)
        print(entry)
        return
    STATE.write_text(out, encoding='utf-8')
    print(f'recorded {ids} → {a.state} in {STATE.relative_to(ROOT)}')
    print('next: python3 Scripts/build_context.py  (refresh Docs/CONTEXT.md)')


if __name__ == '__main__':
    main()
