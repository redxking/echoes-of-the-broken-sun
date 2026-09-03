---
name: echoes-ui-hud-menu-design
description: Design, implement, or review Echoes HUD, modal, briefing, lobby, command-deck, and menu flows for mouse, keyboard, controller, accessibility, and actual player task completion.
metadata:
  author: Angelis Pseftis
---

# Echoes UI, HUD, and menus

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Track G, `Docs/Requirements.md` when demo work is in scope, `Docs/Archive/DevelopmentBible.md` (§Interface and accessibility), `Docs/Archive/TechnicalArchitecture.md` (§UI and accessibility; §Input and camera), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Confirm live ownership before edits.

Design around an observable player task: discover, focus, select, command, pause, resume, recover, or understand an outcome. Preserve modal input capture and prevent click-through. Surface simulation-authoritative values only through approved adapters; UI never determines gameplay, fog, saves, replay, or checksums.

Test every changed flow using mouse and keyboard at minimum; include focus arrival, activation-click swallowing, Escape/back routing, hover/pressed feedback, resolution/scale, and high-contrast/non-color status indicators. For gamepad support, do not claim it works unless its actual navigation was exercised. Screenshots do not establish operability.

Acceptance output: task path, input/focus matrix, running interaction evidence, accessibility observations, and status. Exclude cosmetic-only signoff, source-of-truth changes, and requirements closure without owner acceptance. Stop for an unowned hotspot, untestable input path, hidden focus, or unsupported state data.

Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. This is a design router: send accessibility behavior to `echoes-ui-accessibility-playtest`, real mouse/keyboard flow to `echoes-mouse-keyboard-playtest`, evidence to `echoes-evidence-gate-review`, and owner decision to `echoes-human-acceptance-session`. Read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md` before packaged/runtime or GPU-heavy UI review.
