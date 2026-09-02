---
name: echoes-accessibility
description: Design, implement, or audit behaviorally effective Echoes accessibility across visual, input, motion, flashing, audio, subtitles, and readable state—not settings that merely exist.
metadata:
  author: Angelis Pseftis
---

# Echoes accessibility

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Track G, `Docs/Archive/DevelopmentBible.md` (§Interface and accessibility), `Docs/Archive/TechnicalArchitecture.md` (§UI and accessibility; §Audio), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Check lane ownership before changing any shared system.

For each affected feature define the user-facing behavior change and its equivalent gameplay information: high contrast, non-color ownership/status markers, text scale, focus visibility, reduced motion, reduced flashing, reduced dynamic range, captions/subtitles, and input alternatives. Runtime observation can support at most `AGENT VERIFIED` or `EVIDENCE READY`; it never by itself establishes `HUMAN ACCEPTED`, compliance, or COMPLETE/PASS closure.

Evaluate across affected screens and gameplay states, including HUD, menus, briefing/cinematics, combat congestion, warning/error, pause, and recovery. Preserve simulation authority and do not remove essential feedback without an equivalent. Pair visual changes with audio/subtitle alternatives when the original signal was single-channel.

Acceptance output: feature-to-behavior matrix, actual runtime evidence, known limitations, and ledger-ready status. Exclude compliance claims, platform certification claims, and owner acceptance claims absent direct evidence. Stop for unowned paths, inaccessible source state, an option with no demonstrable behavior, or a required alternative that cannot be implemented safely.

Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Route runtime evaluation to `echoes-ui-accessibility-playtest`, player input to `echoes-mouse-keyboard-playtest`, gate evaluation to `echoes-evidence-gate-review`, and owner acceptance to `echoes-human-acceptance-session`. Read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md` before runtime, packaged, Editor, or GPU-intensive review.
