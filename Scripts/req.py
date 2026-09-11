#!/usr/bin/env python3
"""Query the requirements masters by ID instead of reading them whole.

Author: Angelis Pseftis

    req.py get SPEC-MOV-006 [ID...]      body of each record with its sub-clauses
    req.py state SPEC-MOV-006 [ID...]    current state row plus dated RequirementsState entries naming it
    req.py both ID...                    get + state
    req.py family MOV [--ns SPEC|REL]    one line per record in a family
    req.py search "string-pull"          records whose body mentions the text
    req.py section "§7.3"                a heading span from Requirements.md (capped)
    req.py index                         ID -> title -> section table, one line per record

Views only. Docs/Requirements.md and Docs/RequirementsState.md remain the sole authorities;
a mismatch between this output and the masters is a defect in this script.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQ = ROOT / 'Docs' / 'Requirements.md'
STATE = ROOT / 'Docs' / 'RequirementsState.md'

ID = r'(?:SPEC|REL|DEMO|TBR)-[A-Z]+-\d{3}'
START = re.compile(r'^\s*(?:[*-]\s+|#{1,6}\s+|\|\s*)[\*`]*(' + ID + r')(?=\s|[\*`:|])')
CARD = re.compile(r'^#{1,6} \[(?:Acceptance Card|Asset Card|Interface Sheet): (' + ID + r')(?= —)')
SUB = re.compile(r'^\s*(?:[*-]\s+)?[\*`]*(' + ID + r')\.[A-Z_]+')
HEADING = re.compile(r'^(#{1,6})\s+(.*)')
ANY_ID = re.compile(ID)
DATE = re.compile(r'20\d\d-\d\d-\d\d')
VOCAB = ['HUMAN REJECTED — CHANGES REQUIRED', 'HUMAN ACCEPTED', 'AWAITING HUMAN ACCEPTANCE',
         'EVIDENCE READY', 'AGENT VERIFIED', 'IMPLEMENTED — NOT YET VERIFIED', 'IMPLEMENTED',
         'IN PROGRESS', 'COMPLETE', 'BLOCKED', 'SUPERSEDED BY', 'WITHDRAWN', 'FAILED', 'OPEN']


def _lines(path: Path) -> list[str]:
    return path.read_text(encoding='utf-8').splitlines()


def parse_requirements(lines: list[str]) -> dict:
    """Return {id: {'line', 'end', 'section', 'title'}} for every definition in the master."""
    records: dict[str, dict] = {}
    section = 'Authority'
    fence = None
    order: list[str] = []
    for n, line in enumerate(lines, 1):
        s = line.lstrip()
        if s.startswith(('```', '~~~')):
            fence = None if fence else s[:3]
            continue
        if fence:
            continue
        if line.startswith('# Identifier index'):
            break
        h = HEADING.match(line)
        if h:
            section = h[2].strip()
            if order:
                records[order[-1]].setdefault('end', n - 1)
            continue
        if SUB.match(line):
            continue
        m = START.match(line) or CARD.match(line)
        if m:
            rest = line[m.end():].lstrip('*` :—|').strip()
            if not rest:
                continue
            rid = m[1]
            if order:
                records[order[-1]].setdefault('end', n - 1)
            if rid in records:  # keep first definition; note duplicates
                records[rid].setdefault('duplicates', []).append(n)
                order.append(rid + '#dup')
                records[rid + '#dup'] = {'line': n}
                continue
            title = re.split(r'[:.]\s|\*\*', rest, maxsplit=1)[0].strip().rstrip(':')
            records[rid] = {'line': n, 'section': section, 'title': title}
            order.append(rid)
    if order:
        records[order[-1]].setdefault('end', len(lines))
    return {k: v for k, v in records.items() if '#dup' not in k}


def record_body(lines: list[str], rec: dict) -> str:
    body = lines[rec['line'] - 1:rec['end']]
    while body and not body[-1].strip():
        body.pop()
    return '\n'.join(body)


def state_entries(lines: list[str], rid: str) -> tuple[dict | None, list[dict]]:
    """Return (current-state table row, dated entries) mentioning rid."""
    row = None
    entries: list[dict] = []
    heading = ''
    hdate = ''
    in_table = False
    for n, line in enumerate(lines, 1):
        h = HEADING.match(line)
        if h:
            heading = h[2].strip()
            d = DATE.search(heading)
            hdate = d[0] if d else ''
            in_table = heading.startswith('Current state by ID')
            continue
        if rid not in line:
            continue
        if in_table and line.startswith('| `' + rid + '`'):
            cells = [c.strip() for c in line.strip().strip('|').split('|')]
            row = {'id': rid, 'state': cells[1] if len(cells) > 1 else '', 'class': cells[2] if len(cells) > 2 else '',
                   'evidence': cells[3] if len(cells) > 3 else '', 'commit': cells[4] if len(cells) > 4 else '',
                   'date': cells[5] if len(cells) > 5 else '', 'note': cells[6] if len(cells) > 6 else '', 'line': n}
            continue
        ldate = DATE.search(line)
        verdict = next((v for v in VOCAB if v in line), '')
        entries.append({'line': n, 'heading': heading, 'date': (ldate[0] if ldate else hdate),
                        'verdict': verdict, 'text': line.strip()})
    return row, entries


def cmd_get(args, req):
    recs = parse_requirements(req)
    for rid in args.ids:
        rec = recs.get(rid)
        if not rec:
            print(f'## {rid}\n(not defined in Requirements.md)\n')
            continue
        print(f'## {rid} — {rec["title"]}\nSection: {rec["section"]} · Requirements.md:{rec["line"]}-{rec["end"]}')
        if rec.get('duplicates'):
            print(f'Also defined at lines {rec["duplicates"]} (duplicate definition; report it)')
        print()
        print(record_body(req, rec))
        print()


def cmd_state(args, state):
    for rid in args.ids:
        row, entries = state_entries(state, rid)
        print(f'## {rid} — state')
        if row:
            print(f'Current: **{row["state"]}** · class {row["class"] or "—"} · evidence {row["evidence"] or "—"} · '
                  f'commit {row["commit"] or "—"} · {row["date"]} (RequirementsState.md:{row["line"]})')
            if row['note']:
                print(f'Note: {row["note"]}')
        else:
            print('Current: no structured row yet (record defaults apply: OPEN unless a dated entry below says otherwise)')
        if not entries:
            print('No dated entries name this ID.\n')
            continue
        entries.sort(key=lambda e: (e['date'], e['line']), reverse=True)
        shown = entries[:args.limit]
        print(f'Dated entries ({len(entries)} total, newest {len(shown)} shown):')
        for e in shown:
            v = f' [{e["verdict"]}]' if e['verdict'] else ''
            txt = e['text'] if len(e['text']) <= args.width else e['text'][:args.width] + '…'
            print(f'- {e["date"] or "undated"}{v} · "{e["heading"][:70]}" · L{e["line"]}: {txt}')
        print()


def cmd_family(args, req):
    recs = parse_requirements(req)
    for rid, rec in recs.items():
        ns, fam, _ = rid.split('-')
        if fam == args.family.upper() and (not args.ns or ns == args.ns.upper()):
            print(f'{rid}  {rec["title"]}  ({rec["section"]}, L{rec["line"]})')


def cmd_search(args, req):
    recs = parse_requirements(req)
    pat = re.compile(re.escape(args.text), re.I)
    for rid, rec in recs.items():
        body = record_body(req, rec)
        if pat.search(body):
            print(f'{rid}  {rec["title"]}  ({rec["section"]}, L{rec["line"]})')


def cmd_section(args, req):
    start = None
    level = 0
    for n, line in enumerate(req, 1):
        h = HEADING.match(line)
        if not h:
            continue
        if start is None:
            if args.text.lower() in h[2].lower():
                start, level = n, len(h[1])
        elif len(h[1]) <= level:
            end = n - 1
            break
    else:
        end = len(req)
    if start is None:
        sys.exit(f'no heading contains {args.text!r}')
    span = req[start - 1:end]
    print(f'Requirements.md:{start}-{end} ({len(span)} lines' + (f', showing first {args.limit}' if len(span) > args.limit else '') + ')')
    print('\n'.join(span[:args.limit]))


def cmd_index(args, req):
    recs = parse_requirements(req)
    if args.json:
        print(json.dumps(recs, indent=1))
        return
    for rid, rec in recs.items():
        print(f'{rid}\t{rec["title"]}\t{rec["section"]}\t{rec["line"]}')


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)
    for name in ('get', 'state', 'both'):
        s = sub.add_parser(name)
        s.add_argument('ids', nargs='+')
        s.add_argument('--limit', type=int, default=8, help='dated entries to show per ID')
        s.add_argument('--width', type=int, default=300, help='characters per dated entry')
    s = sub.add_parser('family'); s.add_argument('family'); s.add_argument('--ns')
    s = sub.add_parser('search'); s.add_argument('text')
    s = sub.add_parser('section'); s.add_argument('text'); s.add_argument('--limit', type=int, default=120)
    s = sub.add_parser('index'); s.add_argument('--json', action='store_true')
    a = p.parse_args()
    a.ids = [i.upper() for i in getattr(a, 'ids', [])]
    req = _lines(REQ)
    if a.cmd in ('get', 'both'):
        cmd_get(a, req)
    if a.cmd in ('state', 'both'):
        cmd_state(a, _lines(STATE))
    if a.cmd == 'family':
        cmd_family(a, req)
    if a.cmd == 'search':
        cmd_search(a, req)
    if a.cmd == 'section':
        cmd_section(a, req)
    if a.cmd == 'index':
        cmd_index(a, req)


if __name__ == '__main__':
    main()
