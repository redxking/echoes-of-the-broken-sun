#!/usr/bin/env python3
"""Claude Code PreToolUse hook: refuse whole-file reads of the requirements masters.

Author: Angelis Pseftis

Reads the hook JSON on stdin. Blocks (exit 2, reason on stderr) when the Read tool targets
Docs/Requirements.md or Docs/RequirementsState.md without an offset/limit window, or when a
Bash command cats either file whole. Windowed reads, greps, and Scripts/req.py are allowed.
The masters keep sole authority; this only steers agents to the record-level view.
"""
import json
import re
import sys

MASTERS = ('Docs/Requirements.md', 'Docs/RequirementsState.md')
HINT = ('Do not read the requirements masters whole. Use: python3 Scripts/req.py both <ID> ... | '
        'family <FAM> | search "<text>" | section "<heading>"  (Read with offset+limit is allowed).')


def main():
    try:
        data = json.load(sys.stdin)
    except Exception:
        return 0
    tool = data.get('tool_name', '')
    inp = data.get('tool_input', {}) or {}
    if tool == 'Read':
        path = str(inp.get('file_path', ''))
        if path.endswith(MASTERS) and 'baseline' not in path and not inp.get('limit'):
            print(HINT, file=sys.stderr)
            return 2
    elif tool == 'Bash':
        cmd = str(inp.get('command', ''))
        for m in MASTERS:
            name = m.split('/')[-1]
            if re.search(r'\bcat\b[^|;&]*' + re.escape(name), cmd) and 'baseline' not in cmd:
                print(HINT, file=sys.stderr)
                return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
