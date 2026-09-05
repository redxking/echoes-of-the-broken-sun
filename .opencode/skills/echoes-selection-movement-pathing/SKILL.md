---
name: echoes-selection-movement-pathing
description: "Implement or validate Echoes physical selection, command dispatch, movement, collision, pathing, and rejection feedback across simulation and real player input."
metadata:
  author: Angelis Pseftis
---

# Echoes selection, movement, and pathing

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify active task ownership, worktree, branch, and SHA before mutation.

Specify click/drag/modifier semantics, UI occlusion, focus behavior, legal targets, selection authority, path validity, collision, unreachable destination, command replacement, interrupted movement, game-feel/feedback response, and accessible feedback. Simulation owns legal commands/path consequences; Unreal translates physical input and renders it. Never substitute handler or screenshot proof for real pointer/keyboard exercise.

Run direct deterministic checks, adapter/UI checks, and a fresh GUI route through `echoes-gui-control-readiness`; then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Route group behavior to `echoes-formations-unit-cohesion` and camera behavior to `echoes-camera-navigation`. Stop on shared input ownership, unavailable GUI permissions, or unobservable rejection/recovery path.
