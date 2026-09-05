---
name: echoes-formations-unit-cohesion
description: "Develop or assess authorized Echoes group movement, formation, escort, spacing, cohesion, and recovery behavior under deterministic simulation constraints."
metadata:
  author: Angelis Pseftis
---

# Echoes formations and unit cohesion

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA before mutation.

Define formation/escort eligibility, anchor, spacing, blocked routing, casualties, regrouping, command replacement, fog limits, performance, and persistence. Keep all formation state and outcomes deterministic; camera/VFX/audio may expose status but never decide it. Use authoritative source data and compilers for tunables.

Check deterministic replay/checksum behavior, obstacle and casualty recovery, stress/performance, visible readability, and physical control when player-facing. Route base movement to `echoes-selection-movement-pathing`, combat interactions to `echoes-combat-targeting-damage`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on unclear canonical behavior, active movement live task ownership, nondeterminism, or missing scenario metrics.
