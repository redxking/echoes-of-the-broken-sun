---
name: echoes-ui-hud-menu-design
description: Design, implement, or review Echoes HUD, modal, briefing, lobby, command-deck, and menu flows for mouse, keyboard, controller, accessibility, and actual player task completion.
metadata:
  author: Angelis Pseftis
---

# Echoes UI, HUD, and menus

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Track G, `Docs/Requirements.md` when demo work is in scope, `Docs/Archive/DevelopmentBible.md` (§Interface and accessibility), `Docs/Archive/TechnicalArchitecture.md` (§UI and accessibility; §Input and camera), `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Confirm live ownership before edits.

Design around an observable player task: discover, focus, select, command, pause, resume, recover, or understand an outcome. Preserve modal input capture and prevent click-through. Surface simulation-authoritative values only through approved adapters; UI never determines gameplay, fog, saves, replay, or checksums.

Test every changed flow using mouse and keyboard at minimum; include focus arrival, activation-click swallowing, Escape/back routing, hover/pressed feedback, resolution/scale, and high-contrast/non-color status indicators. For gamepad support, do not claim it works unless its actual navigation was exercised. Screenshots do not establish operability.

Acceptance output: task path, input/focus matrix, running interaction evidence, accessibility observations, and status. Exclude cosmetic-only signoff, source-of-truth changes, and requirements closure without owner acceptance. Stop for an unowned hotspot, untestable input path, hidden focus, or unsupported state data.

This is a design router: send accessibility behavior to `echoes-ui-accessibility-playtest`, real mouse/keyboard flow to `echoes-mouse-keyboard-playtest`, evidence to `echoes-evidence-gate-review`, and owner decision to `echoes-human-acceptance-session`. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before packaged/runtime or GPU-heavy UI review.
