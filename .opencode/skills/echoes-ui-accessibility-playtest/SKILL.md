---
name: echoes-ui-accessibility-playtest
description: Test Echoes rendered UI, keyboard navigation, input clarity, and implemented accessibility behavior through player surfaces rather than menus, source inspection, or screenshots alone.
metadata: { author: Angelis Pseftis }
---

# Echoes UI and accessibility playtest

Read `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, the interface/accessibility requirements in `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/ProjectLedger.md`, the exact package identity, and `../WorkstreamControl/ACTIVE_LANES.md`. Require a callable GUI control interface with fresh visible state and real mouse/keyboard events; otherwise stop and request a human/tool. Begin at title and test focus order, keyboard operation, pointer targets, modal entry/exit, pause, settings persistence, controller-free recovery, and visible error/rejected-action feedback.

For each exposed setting, establish default, change it through the rendered UI, observe the concrete altered behavior during play, then restore it. Cover HUD scale, high contrast, reduced motion, reduced flashing, ownership markers, subtitles/size/background where present, remapping where present, category volumes, reduced dynamic range, and offline pause. An unavailable setting is a recorded gap, not a pass.

Record display mode, package, inputs, focus state, before/after screenshots, dynamic observations, audio observations, timings, and defects. Never use debug flags, source changes, Unreal MCP, injected save/config, or internal commands. Agent operation is not a disabled-player study nor HUMAN ACCEPTED.

Read `../WorkstreamControl/HEAVY_RUN_LOCK.md` before any GUI action, acquire a current detailed Heavy-Run lease, and explicitly release it afterward. Do not reuse or self-invent one. Bind package identity with `echoes-package-provenance` and closure review with `echoes-evidence-gate-review`.
