---
name: echoes-mouse-keyboard-playtest
description: Execute a bounded visible-input Echoes playtest that proves player-surface mouse and keyboard interaction rather than controller automation or source-informed shortcuts.
metadata: { author: Angelis Pseftis }
---

# Echoes mouse and keyboard playtest

Read `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, the control/accessibility sections of `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/ProjectLedger.md`, the exact package identity, and `../WorkstreamControl/ACTIVE_LANES.md`. Require callable GUI control with fresh screenshots/state plus actual pointer and key events. Without it, stop and request human/tool access.

Start from the normal rendered title/briefing flow. Exercise visible selection, camera, contextual order, movement, construction/production where offered, Well interaction where reached, menus, pause, objectives, victory/failure/retry, and return path using only displayed affordances. Observe response latency, visible acknowledgement, audible feedback, rejected-action reason, cursor/focus, and whether controls remain usable under ordinary play.

Never use Unreal MCP, debug keys/flags, consoles, direct simulation calls, save injection, test harnesses, internal commands, or knowledge of source coordinates to bypass discovery. Record package, path, inputs, screen states, audio, timings, defects, and exact scope. Do not call this ordinary-player or human acceptance evidence.

Read `../WorkstreamControl/HEAVY_RUN_LOCK.md` before launch; acquire a current detailed lease and explicitly release it. Do not reuse or invent a lease. Require `echoes-package-provenance` for package identity; route closure review to `echoes-evidence-gate-review` and owner testing to `echoes-human-acceptance-session`.
