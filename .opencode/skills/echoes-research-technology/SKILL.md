---
name: echoes-research-technology
description: "Implement or verify authorized Echoes technology prerequisites, unlocks, costs, effects, and persistence with deterministic data and observable player affordances."
metadata:
  author: Angelis Pseftis
---

# Echoes research and technology

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA before mutation.

Model prerequisite graph, cost, queue, completion, effect scope, denial reason, cancellation, faction limits, and save/replay compatibility in source-authoritative deterministic rules. Compile data; never patch generated catalogs. Test cycles/missing dependencies, duplicated application, rollback/cancel, serialization, and player-facing unlock/rejection feedback.

Route production interactions to `echoes-construction-production`, balance to `echoes-balance-analysis`, Unreal/UI work to `echoes-unreal-runtime-integration` and `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on canon ambiguity, tech tree redesign, ownership conflict, or unproven effect scope.
