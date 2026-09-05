---
name: echoes-workstream-integration
description: "Integrate one bounded Echoes work slice while protecting active ownership boundaries, dirty owner work, source provenance, and evidence continuity."
metadata:
  author: Angelis Pseftis
---

# Echoes workstream integration

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use when handing off, reviewing, freezing, or integrating a bounded work slice; not for taking over adjacent lanes.

1. Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Verify the live task ownership, base SHA, worktree, branch, and frozen scope before any mutation.
2. Compare only the owned paths against the declared base. Preserve all pre-existing dirty paths and other contributors' changes. Never use broad staging, history rewrite, or conflict resolution that changes an unowned file.
3. Confirm every changed source-data path was compiled through its official pipeline and every generated artifact is traceable to source and digest. Simulation authority cannot be moved into presentation during integration.
4. Review the acceptance card against the actual diff, command output, artifact hashes, and required evidence class. Record mismatches as defects or unproven—not as completion.
5. Stop on moved interfaces, ownership conflict, missing base identity, an unmounted volume, failed baseline, or an owner-only choice. Escalate with exact paths and options.

## Integration result

Return the exact scope, SHA/patch identity, checks read, evidence location, unresolved risks, and bounded status. Route evidence to `echoes-evidence-gate-review`; if player-facing, route physical verification through `echoes-gui-control-readiness`, then owner review through `echoes-human-acceptance-session`. An integration receipt never proves player experience by itself.
