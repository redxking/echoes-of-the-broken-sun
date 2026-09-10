import re

filepath = 'Docs/RequirementsState.md'
with open(filepath, 'r') as f:
    content = f.read()

pattern = r'(\* \*\*TBR-SCP-003.*?)OPEN;(.*?Recommendation:.*?\.)'
replacement = r'\1CLOSED; The owner explicitly approved the twelve-unit/twelve-building compact roster baseline. Strategic depth will be derived from this compact roster under the 30-entity limit.\2'
new_content = re.sub(pattern, replacement, content, flags=re.MULTILINE|re.DOTALL)

with open(filepath, 'w') as f:
    f.write(new_content)
