#!/usr/bin/env python3
"""Check the gameplay audit's master/state capture, not gameplay completion.

Author: Angelis Pseftis
"""
from collections import Counter
from pathlib import Path
import re

from check_requirement_registry import ID, parse

ROOT = Path(__file__).resolve().parents[1]
DISPOSITIONS = {'BOUND', 'DECISION', 'MIXED'}


def section(text, name, errors):
    begin, end = f'<!-- SC2_{name}_BEGIN -->', f'<!-- SC2_{name}_END -->'
    if text.count(begin) != 1 or text.count(end) != 1:
        errors.append(f'SC2 {name}: requires one begin/end marker pair')
        return ''
    if text.index(begin) >= text.index(end):
        errors.append(f'SC2 {name}: reversed marker pair')
        return ''
    return text.split(begin, 1)[1].split(end, 1)[0]


def same_inventory(expected, actual, label, errors):
    for key, count in Counter(expected).items():
        if count != 1:
            errors.append(f'{label}: duplicate audit reference {key}')
    for key, count in Counter(actual).items():
        if count != 1:
            errors.append(f'{label}: duplicate master reference {key}')
    for key in sorted(set(expected) - set(actual)):
        errors.append(f'{label}: unmapped audit reference {key}')
    for key in sorted(set(actual) - set(expected)):
        errors.append(f'{label}: master reference missing from audit {key}')


def validate_documents(master, state, audit):
    errors = []
    try:
        _, records, _ = parse(master)
    except ValueError as exc:
        return [f'SC2 capture: cannot read requirement registry: {exc}']
    registered = {record['id'] for record in records}
    audit_rows = re.findall(r'^\| (\d{3}) \| .*? \| ([MPVQD]) \|', audit, re.M)
    events = re.findall(r'^\| (F\d{2}) \|', audit, re.M)
    if not audit_rows or not events:
        errors.append('SC2 capture: functional or feedback audit inventory is empty')
    audit_keys = [key for key, _ in audit_rows]
    # Retain the adopted baseline; additions must be mapped too.
    for key in (f'{number:03}' for number in range(1, 163)):
        if key not in audit_keys:
            errors.append(f'SC2 capture: adopted functional reference removed: {key}')
    for key in (f'F{number:02}' for number in range(1, 33)):
        if key not in events:
            errors.append(f'SC2 capture: adopted feedback reference removed: {key}')

    functional = section(master, 'FUNCTIONAL', errors)
    mapped = re.findall(
        r'^\| Audit (\d{3}) \| ([^|]+) \| ([^|]+) \| ([^|]+) \|$',
        functional, re.M)
    same_inventory(audit_keys, [row[0] for row in mapped], 'Functional coverage', errors)
    for key, title, bindings, disposition in mapped:
        disposition = disposition.strip()
        ids = re.findall(ID, bindings)
        decisions = re.findall(r'TBR-[A-Z0-9]+-\d{3}', bindings)
        if not ids or not title.strip():
            errors.append(f'Audit {key}: empty capability or requirement binding')
        if disposition not in DISPOSITIONS:
            errors.append(f'Audit {key}: invalid disposition {disposition}')
        if decisions and disposition == 'BOUND':
            errors.append(f'Audit {key}: unresolved decision cannot be plain BOUND')
        if disposition in {'DECISION', 'MIXED'} and not decisions:
            errors.append(f'Audit {key}: decision disposition requires a named TBR')
        for identifier in ids:
            if identifier not in registered:
                errors.append(f'Audit {key}: unknown requirement {identifier}')
        for decision in decisions:
            if not re.search(r'^\* \*\*' + re.escape(decision) + r' —', state, re.M):
                errors.append(f'Audit {key}: missing state decision record {decision}')

    feedback = section(master, 'FEEDBACK', errors)
    leaves = re.findall(
        r'^\| (SPEC-UI-008\.F\d{2}) \| (F\d{2}) \| ([^|]+) \| ([^|]+) \| ([^|]+) \| ([^|]+) \|$',
        feedback, re.M)
    same_inventory(events, [row[1] for row in leaves], 'Feedback coverage', errors)
    for leaf, event, trigger, response, recovery, parentage in leaves:
        if leaf != 'SPEC-UI-008.' + event or leaf not in registered:
            errors.append(f'{event}: missing/mismatched registered feedback leaf {leaf}')
        if not all(field.strip() for field in (trigger, response, recovery)):
            errors.append(f'{event}: incomplete trigger/response/recovery')
        parents = re.findall(ID, parentage)
        if not parents:
            errors.append(f'{event}: missing exact parentage')
        for parent in parents:
            if parent not in registered:
                errors.append(f'{event}: unknown parent {parent}')

    for parent in ('SPEC-UI-008', 'SPEC-UI-009', 'SPEC-CTL-020',
                   'SPEC-MOV-014', 'SPEC-CMD-011.ATTACK', 'REL-MP-020', 'REL-MP-021'):
        if parent not in registered or parent not in state:
            errors.append(f'SC2 capture: missing master/state intake identity {parent}')
    return errors


def validate(root=ROOT):
    paths = [root / 'Docs' / name for name in
             ('Requirements.md', 'RequirementsState.md', 'SC2GameplayGapAudit.md')]
    missing = [f'SC2 capture: missing {path.name}' for path in paths if not path.is_file()]
    if missing:
        return missing
    return validate_documents(*(path.read_text() for path in paths))


def main():
    errors = validate()
    if errors:
        print('\n'.join(errors))
        return 1
    print('Gameplay audit traceability passed: all functional items and feedback events '
          'have master coverage and named decisions have state records.')
    print('Boundary: document coverage only; no gameplay or acceptance claim.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
