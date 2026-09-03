---
name: echoes-gameplay-mechanics
description: "Develop and validate one authorized Echoes player mechanic from command semantics through physical-input play without confusing simulation proof with usability."
metadata:
  author: Angelis Pseftis
---

# Echoes gameplay mechanics

Use as the cross-mechanic router when a slice spans several systems. Route focused work to `echoes-input-controls`, `echoes-selection-movement-pathing`, `echoes-formations-unit-cohesion`, `echoes-combat-targeting-damage`, `echoes-economy-logistics`, `echoes-construction-production`, `echoes-research-technology`, `echoes-future-wells-gameplay`, or `echoes-camera-navigation`; do not collapse their separate acceptance gates into this umbrella.

1. Read live `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify exact lease/worktree/branch/dirty paths before mutation.
2. State the player intent, valid/rejected inputs, authoritative command semantics, visible/audio acknowledgment, stable rejection reason, accessibility variants, and failure/recovery behavior before coding.
3. Put rule resolution in simulation and map physical input to validated commands through the Unreal adapter. UI, effects, sound, and camera may reflect state but cannot create authority.
4. Test in layers: simulation semantics, adapter/UI behavior, then packaged physical mouse/keyboard play where the requirement concerns the player. Exercise focus, modal, drag/click, rapid input, pause/Escape, failure, and recovery paths relevant to the slice.
5. Do not accept screenshot-only proof, synthetic input as human play, or a successful action without testing denial feedback. Stop for a shared input hotspot, missing lease, or owner decision on controls/remapping.

## Acceptance checks

Capture exact requirement IDs, test/build commands, input method, build/commit, observed outcomes, and evidence class. Route physical control to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`; owner-only human play acceptance is distinct from automated or agent GUI evidence.
