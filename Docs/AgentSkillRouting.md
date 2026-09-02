# Echoes agent skill-routing contract

**Author and owner:** Angelis Pseftis

This is the shared project-level selection contract for Codex, Claude, and other supported coding agents.
The canonical skill bodies live under `.opencode/skills/`; `.agents/skills/` and `.claude/skills/` are
discovery bridges to that same library. Never maintain divergent copies.

## Selection rule

For every request, inspect skill names and frontmatter descriptions first. Select the smallest sufficient
combination that covers the requested outcome, the work's prerequisites, and its evidence burden. Read the
complete `SKILL.md` for every selected skill before acting. A skill's description decides whether it applies;
its body decides how the work is performed and what can be claimed.

Use a specific skill instead of a broader neighboring skill when it directly matches the task. Use both only
when they govern distinct parts of the work. Router or orchestration skills coordinate work; they do not
replace the implementation, playtest, evidence, or release skill for the actual task.

## Required combinations

- Any project mutation or consequential verification starts with `echoes-session-control`.
- Cross-track work also uses `echoes-production-orchestration`; lane integration or shared-file work also
  uses `echoes-workstream-integration`.
- Builds, Unreal automation, packaging, profiling, asset generation, soak testing, GPU review, and GUI play
  require `echoes-heavy-run-coordination` before the heavy run. Use the task-specific build, package,
  performance, stability, or playtest skill as well.
- Any live GUI or player-input claim uses `echoes-gui-control-readiness` first. Then use the narrowest relevant
  GUI skill, such as packaged smoke, mouse/keyboard playtest, full skirmish, campaign playthrough, tutorial,
  UI/accessibility, visual, audio, or human-acceptance review.
- Any release-gate, readiness, regression, or completion claim uses `echoes-evidence-gate-review` and, when
  defects or release closure are involved, `echoes-regression-release-blockers`.
- Requirements-to-evidence mapping uses `echoes-requirements-traceability` in addition to the domain skill.
- Third-party skills, plugins, models, or agent packages must use `echoes-third-party-agent-skill-review`
  before project-local installation or execution.
- Mac release work must route separately through the appropriate package-provenance, signing/notarization,
  clean-machine, performance, stability, and human-acceptance skills. One gate never substitutes for another.

## Evidence and authority

Screenshots alone do not prove interaction, playability, audio behavior, performance, or human acceptance.
Editor/MCP automation does not prove operating-system mouse and keyboard behavior. Agent-operated GUI evidence
does not become unfamiliar-human evidence. Only Angelis Pseftis may assign `HUMAN ACCEPTED` or
`HUMAN REJECTED — CHANGES REQUIRED`.

Use the status vocabulary required by the selected skills. Do not translate partial, blocked, prepared, or
agent-verified work into `COMPLETE`, unconditional `PASS`, release-ready, or human-accepted language.

## Conflict and scope changes

Higher-authority instructions and the live repository contract control. A skill may narrow execution and
claim boundaries but never grants new authority for credentials, publication, upload, signing, notarization,
deletion, or other external-state actions. If scope expands, pause the new portion, select and read the newly
applicable skills, and then continue. If two skills conflict, follow the more safety- or evidence-preserving
instruction and report the conflict rather than silently choosing the easier path.

