---
name: echoes-combat-targeting-damage
description: "Implement or verify Echoes combat targeting, damage, death, threat, feedback, and recovery as deterministic rules with readable player consequences."
metadata:
  author: Angelis Pseftis
---

# Echoes combat, targeting, and damage

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA before mutation.

Define target legality, visibility, range, priority, damage timing, armor/effects, destruction, invalid target response, threat feedback, death cleanup, save/replay, and accessibility. Simulation is authority; animations, sound, VFX, camera, and HUD must follow state and cannot change collision/nav/checksum outcome. Use compiled source data, not generated edits.

Test deterministic audit cases and runtime readability/game-feel feedback at combat speed; use `echoes-gui-control-readiness` for physical play/listening where applicable. Route balance to `echoes-balance-analysis`, group behavior to `echoes-formations-unit-cohesion`, evidence to `echoes-evidence-gate-review`, owner judgment to `echoes-human-acceptance-session`. Stop on unapproved mechanics, owner-held balance decision, ownership conflict, or no measurable player outcome.
