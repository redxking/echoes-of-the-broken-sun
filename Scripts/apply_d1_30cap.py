import re
import sys

def replace_in_file(filepath, pattern, replacement):
    with open(filepath, 'r') as f:
        content = f.read()
    new_content = re.sub(pattern, replacement, content, flags=re.MULTILINE)
    if new_content == content:
        print(f"Warning: No changes made to {filepath}")
    else:
        with open(filepath, 'w') as f:
            f.write(new_content)
        print(f"Updated {filepath}")

# Update Requirements.md
req_path = 'Docs/Requirements.md'
# Insert SPEC-RES-008 under SPEC-RES-007
pattern = r'(\* \*\*SPEC-RES-007.*?\n)'
replacement = r'\1* **SPEC-RES-008 — Mobile Entity Limit:** Each player is limited to a hard cap of exactly 30 controllable mobile entities at any time, including workers and command characters. Buildings and static defenses are excluded from this count. This constraint operates concurrently with the 200 Logistics ceiling; if either is exhausted, further production or deployment of mobile units is blocked.\n'
replace_in_file(req_path, pattern, replacement)

# Update RequirementsState.md TBR-SCP-003
reqstate_path = 'Docs/RequirementsState.md'
pattern2 = r'a Unity-specific 30-entity memory or SC2 supply count does not amend this Unreal master\.'
replacement2 = r'the 30-entity hard cap is binding design direction and applies concurrently with the 200 Logistics ceiling.'
replace_in_file(reqstate_path, pattern2, replacement2)

# Update SC2GameplayGapAudit.md
gap_path = 'Docs/SC2GameplayGapAudit.md'
pattern3 = r'A remembered Unity-specific 30-entity limit was not applied to this checkout\.'
replacement3 = r'The owner-approved 30-entity limit is binding design direction, operating concurrently with the 200 Logistics cap.'
replace_in_file(gap_path, pattern3, replacement3)

