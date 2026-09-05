---
name: echoes-cinematics-sequencer
description: Create or review Echoes in-engine Sequencer cinematics with canonical story control, runtime skip/recovery, subtitle/audio alignment, and no simulation-authority leakage.
metadata:
  author: Angelis Pseftis
---

# Echoes cinematics and Sequencer

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Track D, `Docs/Archive/DevelopmentBible.md` (§Campaign outline, §Writing rules), `Docs/OpeningAndTutorialScript.md`, `Docs/CharacterVoiceIdentityBible.md`, `Docs/Archive/TechnicalArchitecture.md` (§Presentation, §Match lifecycle), `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Do not mutate without a live task ownership.

Use in-engine cinematic assets and canonical sources. Cinematics may observe authoritative game state and present it, but cannot advance simulation, determine mission success, alter commands/fog/saves/replay/checksums, or hide an unhandled failure. Treat skip, pause, restart, load, interruption, input restoration, and subtitle/voice timing as required runtime paths.

Render and play the real sequence on the target build path. Review composition, camera motion, reduced motion/flashing, audio mix, captions, legal asset provenance, and transition back to player control. Retain evidence that identifies build, sequence, path, settings, and actual execution—not just an editor screenshot.

Acceptance output: canonical-source map, Sequencer/runtime path, playback evidence including skip/recovery, accessibility observations, and status. Exclude pre-rendered-video substitution, unauthorized narrative additions, and cinematic-only proof of campaign completion. Stop for no canonical basis, unresolved rights, broken return to control, or missing task ownership.

Route Editor-internal checks to `echoes-unreal-mcp-editor-inspection`, player control to `echoes-mouse-keyboard-playtest`, visual/audio runtime review to `echoes-realtime-visual-review` and `echoes-audio-listening-review`, then gate through `echoes-evidence-gate-review`. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before editor/runtime work.

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
