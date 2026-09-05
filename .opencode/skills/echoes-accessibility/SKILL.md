---
name: echoes-accessibility
description: Design, implement, or audit behaviorally effective Echoes accessibility across visual, input, motion, flashing, audio, subtitles, and readable state—not settings that merely exist.
metadata:
  author: Angelis Pseftis
---

# Echoes accessibility

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Track G, `Docs/Archive/DevelopmentBible.md` (§Interface and accessibility), `Docs/Archive/TechnicalArchitecture.md` (§UI and accessibility; §Audio), `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Check path ownership before changing any shared system.

For each affected feature define the user-facing behavior change and its equivalent gameplay information: high contrast, non-color ownership/status markers, text scale, focus visibility, reduced motion, reduced flashing, reduced dynamic range, captions/subtitles, and input alternatives. Classify observations only through the [RequirementsState status vocabulary](../../../Docs/RequirementsState.md#state-vocabulary); runtime observation alone cannot establish owner acceptance, compliance, or closure.

Evaluate across affected screens and gameplay states, including HUD, menus, briefing/cinematics, combat congestion, warning/error, pause, and recovery. Preserve simulation authority and do not remove essential feedback without an equivalent. Pair visual changes with audio/subtitle alternatives when the original signal was single-channel.

Acceptance output: feature-to-behavior matrix, actual runtime evidence, known limitations, and ledger-ready status. Exclude compliance claims, platform certification claims, and owner acceptance claims absent direct evidence. Stop for unowned paths, inaccessible source state, an option with no demonstrable behavior, or a required alternative that cannot be implemented safely.

Route runtime evaluation to `echoes-ui-accessibility-playtest`, player input to `echoes-mouse-keyboard-playtest`, gate evaluation to `echoes-evidence-gate-review`, and owner acceptance to `echoes-human-acceptance-session`. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before runtime, packaged, Editor, or GPU-intensive review.
