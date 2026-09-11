#!/usr/bin/env python3
"""Render Docs/CONTEXT.md, the start-of-session packet, from the live records.

Author: Angelis Pseftis

    build_context.py          write Docs/CONTEXT.md
    build_context.py --print  write it and print it (used by the SessionStart hook)

Sources, in order: git identity of the checkout; the DeliveryPlan "Active execution state" table;
the newest dated RequirementsState headings; the most recently touched rows of the structured
state table. The packet is a generated view under 2K tokens. Editing it by hand is pointless:
edit DeliveryPlan.md or record state with Scripts/record_state.py, then rerun this.
"""
from __future__ import annotations

import argparse
import datetime as dt
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'Docs' / 'DeliveryPlan.md'
STATE = ROOT / 'Docs' / 'RequirementsState.md'
OUT = ROOT / 'Docs' / 'CONTEXT.md'
FULL_ROWS = {'First unfinished package', 'Next exact action after resume', 'Next dependency', 'Ownership'}
CLIP = 420


def git(*args) -> str:
    try:
        return subprocess.check_output(['git', *args], cwd=ROOT, text=True, stderr=subprocess.DEVNULL).strip()
    except Exception:
        return ''


def active_state() -> list[tuple[str, str]]:
    text = PLAN.read_text(encoding='utf-8')
    m = re.search(r'^## Active execution state\n(.*?)(?=^## )', text, re.S | re.M)
    if not m:
        return []
    rows = []
    for line in m[1].splitlines():
        if line.startswith('| ') and not line.startswith('| Field') and not line.startswith('|---'):
            cells = [c.strip() for c in line.strip().strip('|').split('|')]
            if len(cells) >= 2:
                rows.append((cells[0], cells[1]))
    return rows


def clip(field: str, value: str) -> str:
    value = value.replace('**', '')
    if field in FULL_ROWS or len(value) <= CLIP:
        return value
    cut = value[:CLIP]
    cut = cut[:cut.rfind('. ') + 1] if '. ' in cut[CLIP // 2:] else cut
    return cut.rstrip() + ' … (full text: DeliveryPlan.md, Active execution state)'


def latest_state_headings(n=4) -> list[str]:
    heads = [l[3:].strip() for l in STATE.read_text(encoding='utf-8').splitlines()
             if l.startswith('## ') and re.search(r'20\d\d-\d\d-\d\d', l)]
    def key(h):
        d = re.search(r'20\d\d-\d\d-\d\d', h)
        return d[0] if d else ''
    return sorted(heads, key=key, reverse=True)[:n]


def recent_state_rows(n=8) -> list[str]:
    lines = STATE.read_text(encoding='utf-8').splitlines()
    try:
        start = lines.index('## Current state by ID')
    except ValueError:
        return []
    rows = []
    for l in lines[start:]:
        if l.startswith('## ') and l != '## Current state by ID':
            break
        if l.startswith('| `'):
            cells = [c.strip() for c in l.strip().strip('|').split('|')]
            rows.append((cells[5] if len(cells) > 5 else '', f'{cells[0]} {cells[1]} · {cells[2]} · {cells[5] if len(cells) > 5 else ""}'))
    return [r[1] for r in sorted(rows, reverse=True)[:n]]


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--print', action='store_true')
    a = p.parse_args()

    branch = git('rev-parse', '--abbrev-ref', 'HEAD')
    head = git('rev-parse', '--short', 'HEAD')
    upstream = git('rev-parse', '--short', '@{u}')
    dirty = [l for l in git('status', '--porcelain').splitlines() if l.strip()]
    dirty_docs = [l[3:] for l in dirty if l[3:].startswith(('Docs/', 'AGENTS.md', 'Scripts/', 'Source/', 'Config/'))]
    now = dt.datetime.now(dt.timezone.utc).strftime('%Y-%m-%d %H:%M UTC')

    out = [
        '# Session context packet',
        '',
        '**Author and owner:** Angelis Pseftis',
        f'**Generated:** {now} by `Scripts/build_context.py`. Do not edit; regenerate.',
        '',
        'Read this, then fetch only the requirement records you touch:',
        '`python3 Scripts/req.py both <ID> [<ID>...]` · `req.py family <FAM>` · `req.py search "<text>"` · '
        '`req.py section "<heading>"`. Do not open Requirements.md or RequirementsState.md whole. '
        'Record outcomes with `python3 Scripts/record_state.py --id <ID> --state <STATE> --class <CLASS> '
        '--evidence <path> --note "<what>"` and rerun this script before handing off. '
        'Rules: [AGENTS.md](../AGENTS.md); authority map: [Docs/README.md](README.md); skills: '
        '[AgentSkillRouting.md](AgentSkillRouting.md).',
        '',
        '## Checkout',
        '',
        f'- Branch `{branch}` at `{head}`' + (f' (upstream `{upstream}`)' if upstream else '') +
        f'; {len(dirty)} dirty paths' + (f', of which non-asset: {", ".join(f"`{d}`" for d in dirty_docs[:8])}' if dirty_docs else '') + '.',
        '- Another lane may own the dirty paths. Preserve them; stage only your own files by path.',
        '',
        '## Active execution state (from DeliveryPlan.md)',
        '',
    ]
    for field, value in active_state():
        out.append(f'- **{field}.** {clip(field, value)}')
    out += ['', '## Newest RequirementsState entries', '']
    out += [f'- {h}' for h in latest_state_headings()]
    rows = recent_state_rows()
    if rows:
        out += ['', '## Recently recorded state rows', ''] + [f'- {r}' for r in rows]
    out += ['', '## Before you stop', '',
            '- `record_state.py` for every ID whose evidence state you changed; only Angelis sets acceptance.',
            '- Update the DeliveryPlan active-state table if the next action changed, then rerun `build_context.py`.',
            '- New evidence goes under `BuildArtifacts/Evidence/<gate>-<UTC>/`; cite its absolute path.',
            '']
    text = '\n'.join(out)
    OUT.write_text(text, encoding='utf-8')
    if a.print:
        print(text)
    else:
        print(f'wrote {OUT.relative_to(ROOT)} ({len(text) // 4} tokens approx)')


if __name__ == '__main__':
    main()
