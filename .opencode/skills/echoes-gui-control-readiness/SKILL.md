---
name: echoes-gui-control-readiness
description: "Determine whether a live macOS GUI-control route can produce fresh, attributable Echoes pointer, keyboard, visual, and audio observations; if any capability is absent, stop with BLOCKED and never claim play, PASS, or human acceptance."
metadata:
  author: Angelis Pseftis
---

# Echoes GUI control readiness

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use to determine whether a live GUI route can provide attributable pointer, keyboard, and visual observations. It is a read-only capability check unless a game is launched or input is delivered.

Verify a callable UI interface, fresh visible state/capture, real pointer and keyboard delivery, and the relevant macOS permissions. Require an audio observation route only when the requested task needs audio evidence. Bind any capture to time, package/build identity, window state, input sequence, and observer. Synthetic or agent GUI input is its own evidence class and never human play.

Capability inspection does not need heavy-run coordination. A launch or delivered input does: reserve the resource through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md), retain the resulting evidence, and report an unavailable capability precisely. Do not claim play, pass, or human acceptance from capability alone.
