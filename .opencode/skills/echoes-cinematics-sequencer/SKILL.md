---
name: echoes-cinematics-sequencer
description: Create or review Echoes in-engine Sequencer cinematics with canonical story control, runtime skip/recovery, subtitle/audio alignment, and no simulation-authority leakage.
metadata:
  author: Angelis Pseftis
---

# Echoes cinematics and Sequencer

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Track D, `Docs/Archive/DevelopmentBible.md` (§Campaign outline, §Writing rules), `Docs/OpeningAndTutorialScript.md`, `Docs/CharacterVoiceIdentityBible.md`, `Docs/Archive/TechnicalArchitecture.md` (§Presentation, §Match lifecycle), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Do not mutate without a live lane.

Use in-engine cinematic assets and canonical sources. Cinematics may observe authoritative game state and present it, but cannot advance simulation, determine mission success, alter commands/fog/saves/replay/checksums, or hide an unhandled failure. Treat skip, pause, restart, load, interruption, input restoration, and subtitle/voice timing as required runtime paths.

Render and play the real sequence on the target build path. Review composition, camera motion, reduced motion/flashing, audio mix, captions, legal asset provenance, and transition back to player control. Retain evidence that identifies build, sequence, path, settings, and actual execution—not just an editor screenshot.

Acceptance output: canonical-source map, Sequencer/runtime path, playback evidence including skip/recovery, accessibility observations, and status. Exclude pre-rendered-video substitution, unauthorized narrative additions, and cinematic-only proof of campaign completion. Stop for no canonical basis, unresolved rights, broken return to control, or absent lane.

Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Route Editor-internal checks to `echoes-unreal-mcp-editor-inspection`, player control to `echoes-mouse-keyboard-playtest`, visual/audio runtime review to `echoes-realtime-visual-review` and `echoes-audio-listening-review`, then gate through `echoes-evidence-gate-review`. Read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md` before editor/runtime work.
