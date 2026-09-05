---
name: echoes-skirmish-full-match-playtest
description: Play a complete visible Glass Scar player-versus-AI skirmish as a bounded evidence session, including setup, faction, AI, combat, outcome, and recovery observations.
metadata:
  author: Angelis Pseftis
---

# Echoes skirmish full-match playtest

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, the skirmish rules in `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/ProjectLedger.md`, the exact package identity, and [echoes-session-control](../echoes-session-control/SKILL.md). Require a real GUI-control interface with fresh screenshots/state and real mouse/keyboard input; otherwise stop. Configure only through rendered setup controls and record map, faction, AI personality/difficulty, resources, victory condition, display/accessibility settings, seed if exposed, and package identity.

Play from briefing/deployment to visible terminal outcome and return/redeploy route. Observe economy, scouting/fog, selection/order feedback, combat readability, Well decision, AI behavior, sound, UI clarity, performance hitches, pause/options, save/load if in scope, and victory/defeat explanation. Never use source-derived coordinates, Unreal MCP, debugger, command console, direct sim controls, injected saves, or debug flags.

Retain timestamps, input log, state captures, audio observations, duration, result, defects, and whether the match stayed active. One agent-played faction/match proves only that recorded path; it does not establish balance, all factions, ordinary-human play, or release readiness.

Read [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before launch, confirm an exclusive resource reservation, and record and release the reservation after the match. Stop for a missing, held, or unrelated reservation. Package provenance is `echoes-package-provenance`; interpret evidence through `echoes-evidence-gate-review`; human closure is separate.
