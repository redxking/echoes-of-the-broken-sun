---
name: echoes-camera-navigation
description: "Implement or validate Echoes camera movement, framing, zoom, navigation, focus, and accessibility behavior through real input and gameplay readability checks."
metadata:
  author: Angelis Pseftis
---

# Echoes camera and navigation

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA before mutation.

Define input routes, focus/modal/Escape behavior, bounds, zoom, follow/frame rules, motion reduction, camera-relative input, resize/display behavior, and recovery after menus/cinematics/load. Camera is presentation: it cannot alter deterministic simulation state, fog authority, saves, replays, or checksums.

Test across relevant resolution/window and accessibility settings with actual pointer/keyboard through `echoes-gui-control-readiness`; observe selection/combat readability rather than screenshots alone. Route selection links to `echoes-selection-movement-pathing`, UI to the player-facing implementation owner, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on shared input path ownership, unavailable GUI capability, untested transition, or accessibility behavior that is cosmetic only.
