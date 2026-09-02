---
name: echoes-input-controls
description: Implement or verify Echoes mouse, keyboard, camera, selection, command, pause, and controller interaction semantics against real playable paths rather than synthetic evidence alone.
metadata:
  author: Angelis Pseftis
---

# Echoes input and controls

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Tracks E/F/G, `Docs/DemoReadinessRequirements.md` where demo work is in scope, `Docs/Archive/DevelopmentBible.md` (§Combat and controls, §Player quick start), `Docs/Archive/TechnicalArchitecture.md` (§Input and camera), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Take no write without a current lane lease.

Keep input translation at the engine adapter boundary. Commands must retain simulation validation, stable ordering, authority, and replay/checksum behavior; input convenience must not bypass those contracts. Specify the intended outcome for press, release, drag, double-click, modifiers, focus activation, modal capture, pause/Escape, invalid target, and lost focus.

Exercise the packaged or editor game with a human-style mouse and keyboard sequence when available: move camera, select, drag-select, issue commands, cancel, pause/resume, and recover from a menu. Synthetic tests establish a narrower class only. Record hardware, build, map, input sequence, observed result, and untested states.

Acceptance output: interaction contract, execution trace or retained manual evidence, automated evidence if applicable, and truthful status. Exclude control-remapping promises without an owner decision and input changes outside lane ownership. Stop if command authority, modal semantics, or real-input validation is unavailable.

Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Route human-style device delivery to `echoes-mouse-keyboard-playtest`, accessibility checks to `echoes-ui-accessibility-playtest`, and evidence to `echoes-evidence-gate-review`. Any Editor or packaged execution must first read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md`.
