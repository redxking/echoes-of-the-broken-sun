#!/usr/bin/env python3
"""Check local agent-document routing and links; does not qualify game requirements.

Author: Angelis Pseftis
"""
from pathlib import Path
import re
import sys
from urllib.parse import unquote
from check_requirement_registry import validate as validate_registry

ROOT = Path(__file__).resolve().parents[1]
NEW_IDS = ('SPEC-MAP-004', 'SPEC-CAM-041', 'SPEC-CAM-042', 'SPEC-VISD-008', 'SPEC-ART-004')


def markdown_files():
    return sorted(set(ROOT.glob('*.md')) | set(ROOT.glob('Docs/**/*.md')) |
                  set(ROOT.glob('.opencode/skills/*/SKILL.md')) | set(ROOT.glob('.github/**/*.md')))


def without_fences(text):
    return re.sub(r'^(`{3,}|~{3,})[^\n]*\n.*?^\1\s*$', '', text, flags=re.M | re.S)


def headings(text):
    used = {}
    result = set()
    for heading in re.findall(r'^#{1,6}\s+(.+?)\s*#*$', without_fences(text), re.M):
        slug = re.sub(r'[^\w\- ]', '', heading.lower()).replace(' ', '-')
        count = used.get(slug, 0)
        used[slug] = count + 1
        result.add(slug + (f'-{count}' if count else ''))
    return result


def validate_game_workflow(root):
    """Keep shared entry points and task prompts routed to one workflow."""
    workflow = root / 'Docs/Prompts/GameDevelopmentWorkflow.md'
    if not workflow.is_file():
        return ['Missing canonical game development workflow: Docs/Prompts/GameDevelopmentWorkflow.md']
    errors = []
    entries = [root / name for name in ('AGENTS.md', 'Docs/README.md', 'Docs/AgentSkillRouting.md')]
    entries += sorted(path for path in (root / 'Docs/Prompts').glob('*.md') if path != workflow)
    for path in entries:
        text = path.read_text() if path.is_file() else ''
        visible = without_fences(re.sub(r'<!--.*?(?:-->|$)', '', text, flags=re.S))
        visible = re.sub(r'(`+)(?!`).*?\1(?!`)', '', visible, flags=re.S)
        destinations = re.findall(r'(?<!!)\[[^\]\n]+\]\(([^\n]+?)\)', visible)
        targets = [(path.parent / unquote(url.strip().strip('<>').partition('#')[0])).resolve()
                   for url in destinations if not re.match(r'^[a-zA-Z][a-zA-Z0-9+.-]*:', url.strip())]
        if workflow.resolve() not in targets:
            errors.append(f'{path.relative_to(root)}: missing canonical game development workflow link')
    return errors


def main():
    errors = validate_game_workflow(ROOT)
    files = markdown_files()
    local_links = 0
    for path in files:
        text = path.read_text()
        if not ('Angelis Pseftis' in text[:1800]):
            errors.append(f'{path.relative_to(ROOT)}: missing owner authorship near header')
        visible = without_fences(text)
        for match in re.finditer(r'(?<!!)\[[^\]\n]+\]\(([^\n]+?)\)', visible):
            url = match[1].strip().strip('<>')
            if re.match(r'^[a-zA-Z][a-zA-Z0-9+.-]*:', url):
                continue
            location, _, fragment = url.partition('#')
            target = (path.parent / unquote(location)).resolve() if location else path
            local_links += 1
            label = f'{path.relative_to(ROOT)} -> {url}'
            if not target.exists():
                errors.append(f'Missing local link: {label}')
            elif fragment and target.suffix.lower() == '.md':
                if unquote(fragment) not in headings(target.read_text()):
                    errors.append(f'Missing heading anchor: {label}')

    skills = sorted(ROOT.glob('.opencode/skills/*/SKILL.md'))
    required = ('../../../AGENTS.md', '../../../Docs/README.md', '../../../Docs/Requirements.md',
                '../../../Docs/RequirementsState.md', '../../../Docs/AgentSkillRouting.md')
    obsolete = ('WorkstreamControl', 'ACTIVE_LANES', 'HEAVY_RUN_LOCK',
                'coordinator-issued lane', 'current `Docs/` directives/ledgers')
    for path in skills:
        text = path.read_text()
        if not text.startswith('---\n') or '\n---\n' not in text[4:]:
            errors.append(f'{path.parent.name}: missing skill frontmatter')
            continue
        frontmatter = text.split('---', 2)[1]
        if f'name: {path.parent.name}\n' not in frontmatter or not re.search(r'^description: .+', frontmatter, re.M):
            errors.append(f'{path.parent.name}: missing or mismatched skill name/description')
        if not re.search(r'^  author: Angelis Pseftis$', frontmatter, re.M):
            errors.append(f'{path.parent.name}: incorrect author metadata')
        for pointer in required:
            if pointer not in text:
                errors.append(f'{path.parent.name}: missing canonical pointer {pointer}')
        for token in obsolete:
            if token in text:
                errors.append(f'{path.parent.name}: obsolete operating rule {token}')
        if re.search(r'`\[[^\n]+?\]\([^\n]+?\)`', text):
            errors.append(f'{path.parent.name}: Markdown link incorrectly inside code span')

    for bridge in ('.claude/skills', '.agents/skills'):
        path = ROOT / bridge
        if not path.is_symlink() or path.resolve() != ROOT / '.opencode/skills':
            errors.append(f'{bridge}: must resolve to canonical skill library')
    for entry in ('CLAUDE.md', 'GEMINI.md', '.github/copilot-instructions.md'):
        path = ROOT / entry
        if not path.exists() or 'AGENTS.md' not in path.read_text():
            errors.append(f'{entry}: missing shared-contract pointer')

    requirements = (ROOT / 'Docs/Requirements.md').read_text()
    state = (ROOT / 'Docs/RequirementsState.md').read_text()
    registry_errors, _, _ = validate_registry(requirements)
    errors.extend(registry_errors)
    for identifier in NEW_IDS:
        definition = re.findall(r'^\* \*\*' + re.escape(identifier) + r' —', requirements, re.M)
        index = re.findall(r'^\| `' + re.escape(identifier) + r'` \|', requirements, re.M)
        if len(definition) != 1 or len(index) != 1 or identifier not in state:
            errors.append(f'{identifier}: requires one definition, one index row, and a state record')
    active = [ROOT / name for name in ('Docs/Requirements.md', 'Docs/AgentSkillRouting.md',
                                      'Docs/GameCompletionDirective.md')]
    active += list(ROOT.glob('Docs/Prompts/*.md'))
    for path in active:
        if 'WorkstreamControl/' in path.read_text():
            errors.append(f'{path.relative_to(ROOT)}: obsolete active external-control path')
    if errors:
        print('\n'.join(errors))
        return 1
    print(f'Agent document checks passed: {len(files)} Markdown files, {len(skills)} skills, '
          f'{local_links} local links, five new requirement/state/index bindings.')
    print('Boundary: structural/routing checks only; historical evidence, semantic requirement conflicts, '
          'game behavior, visual/audio quality, and owner acceptance are not certified.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
