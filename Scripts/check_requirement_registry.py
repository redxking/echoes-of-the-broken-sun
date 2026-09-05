#!/usr/bin/env python3
"""Check requirement identity and rebuild the navigation index in place.

Author: Angelis Pseftis
This validates registry structure, not requirement meaning, evidence or acceptance.
"""
from pathlib import Path
from collections import Counter, defaultdict
import argparse
import re

ROOT = Path(__file__).resolve().parents[1]
MASTER = ROOT / 'Docs/Requirements.md'
ID = r'(?:SPEC|DEMO|REL)-[A-Z0-9]+-\d{3}(?:\.[A-Z0-9_]+)*'
START = re.compile(r'^\s*(?:[*-]\s+|#{1,6}\s+|\|\s*)[\*`]*(' + ID + r')(?=\s|[\*`:|])')
CARD = re.compile(r'^#{1,6} \[(?:Acceptance Card|Asset Card|Interface Sheet): (' + ID + r')(?= —)')
INDEX = re.compile(r'^\| `(' + ID + r')` \| ([^|]*)\| ([^|]*)\|$', re.M)


def parse(text):
    """Recognize the master's existing bullet, heading and first-column definitions."""
    body, index = text.split('# Identifier index', 1)
    records = []
    section = 'Authority'
    fence = None
    for line_no, line in enumerate(body.splitlines(), 1):
        stripped = line.lstrip()
        if stripped.startswith(('```', '~~~')):
            marker = stripped[:3]
            fence = None if fence == marker else marker if fence is None else fence
            continue
        if fence:
            continue
        match = START.match(line) or CARD.match(line)
        if match:
            identifier = match[1]
            rest = line[match.end():].lstrip('*` :—|').strip()
            # A definition must have content. Naked cross-reference list entries do not qualify.
            if rest:
                records.append({'id': identifier, 'line': line_no, 'text': rest, 'section': section})
        elif re.match(r'^#{1,3} ', line):
            section = re.sub(r'^#+\s*', '', line).replace('|', '/')
    rows = [(m[1], m[2].strip(), m[3].strip()) for m in INDEX.finditer(index)]
    return body, records, rows


def validate(text):
    body, records, rows = parse(text)
    errors = []
    if any(ord(char) < 32 and char != "\n" for char in text):
        errors.append("Unexpected control character in master; inspect corrupted math commands/units.")
    by_id = defaultdict(list)
    for record in records:
        by_id[record['id']].append(record)
        if record['text'].rstrip().endswith('...'):
            errors.append(f"Truncated definition {record['id']}: line {record['line']}")
    for identifier, definitions in sorted(by_id.items()):
        if len(definitions) > 1:
            errors.append(f'Duplicate definition {identifier}: lines ' + ', '.join(str(r['line']) for r in definitions))
    parents = {i for i in by_id if '.' not in i}
    for identifier in sorted(by_id):
        if identifier.split('.')[0] not in parents:
            errors.append(f'No parent definition: {identifier}')
    indexed = Counter(row[0] for row in rows)
    for identifier, count in sorted(indexed.items()):
        if count != 1:
            errors.append(f'Duplicate index entry: {identifier}')
    for identifier in sorted(parents - set(indexed)):
        errors.append(f'Missing index entry: {identifier}')
    for identifier in sorted(set(indexed) - parents):
        errors.append(f'Index has no parent definition: {identifier}')
    expected = Counter('-'.join(identifier.split('-')[:2]) for identifier in parents
                       if identifier.startswith(('REL-', 'DEMO-')))
    family_map = body.split('## Crosswalk', 1)[1].split('\n---', 1)[0]
    actual = {m[1]: int(m[2]) for m in re.finditer(
        r'^\| `((?:REL|DEMO)-[A-Z0-9]+)-\*` \| (\d+) \|', family_map, re.M)}
    if actual != dict(expected):
        errors.append('Family navigation counts do not match registered parent identities.')
    return errors, records, rows


def write_index(text):
    body, records, _ = parse(text)
    parents = [r for r in records if '.' not in r['id']]
    if len({r['id'] for r in parents}) != len(parents):
        raise ValueError('Resolve duplicate parent definitions before rebuilding the index.')
    lines = ['# Identifier index', '',
             'Navigation generated from the parent definitions by `Scripts/check_requirement_registry.py`.',
             'Retired identifiers remain indexed for traceability. Counts establish structural coverage only;',
             'they do not prove semantic consistency, implementation, evidence or owner acceptance.', '',
             '| ID | Requirement | Section |', '|---|---|---|']
    for r in sorted(parents, key=lambda item: item['id']):
        title = r['text'].split('**')[0].split(' | ')[0].strip()
        if 'Retired ambiguous identifier' in r['text']:
            title = 'Retired ambiguous identifier; see body for all titled successors'
        elif 'SUPERSEDED BY' in r['text']:
            title += ' ' + r['text'].split('SUPERSEDED BY', 1)[1].split(';', 1)[0].strip()
        title = re.sub(r'[*`]', '', title).replace('|', '/').replace('\n', ' ')[:150]
        lines.append(f"| `{r['id']}` | {title} | {r['section']} |")
    return body.rstrip() + '\n\n' + '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-index', action='store_true', help='rebuild only the master navigation index in place')
    args = parser.parse_args()
    text = MASTER.read_text()
    if args.write_index:
        text = write_index(text)
        MASTER.write_text(text)
    errors, records, rows = validate(text)
    if errors:
        print('\n'.join(errors))
        return 1
    counts = Counter(r['id'].split('-')[0] for r in records if '.' not in r['id'])
    print(f'Requirement registry checks passed: {len(rows)} parent definitions/index rows, '
          f'{len(records) - len(rows)} subordinate definitions; {dict(sorted(counts.items()))}.')
    print('Boundary: identity/index checks only; semantics, evidence and acceptance require separate review.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
