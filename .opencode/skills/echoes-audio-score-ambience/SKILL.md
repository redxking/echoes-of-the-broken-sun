---
name: echoes-audio-score-ambience
description: Create or review Echoes score, ambience, music states, and world soundscape with registered sources, dynamic-range behavior, and listening validation in gameplay context.
metadata:
  author: Angelis Pseftis
---

# Echoes score and ambience

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Track B and §7, `Docs/Archive/DevelopmentBible.md` (§Art and audio), `Docs/AudioDirection.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/TechnicalArchitecture.md` (§Audio), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Require a live lane before mutation.

Use the registered deterministic audio generator by default; source changes regenerate outputs and are recorded with revision, provenance, and byte-idempotence. Any commissioned, licensed, or local-generative exception needs a recorded owner decision, method, license, and rationale before use. Never claim ownership or clearance from a filename or prompt alone.

Bind score/ambience to authoritative presentation events without changing simulation. Listen in the actual map and action context at normal, reduced-dynamic-range, paused, combat-heavy, and interruption/restart states. Maintain intelligibility priority for gameplay cues and voice; test loops, transitions, ducking, silence, and device/volume changes where supported.

Acceptance output: registered family and source record, event map, listening notes/capture boundary, accessibility behavior, and evidence status. Exclude unregistered audio, soundtrack-release claims, and visual-only review. Stop for missing rights, unowned hooks, nonreproducible generation, or an unlistened transition.

This combined skill routes score state to `echoes-score-music-state` and environmental sound to `echoes-ambience-world-soundscape`. Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Before generation, build, Editor/runtime listening, or other heavy work, read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md`; use `echoes-audio-listening-review` and `echoes-evidence-gate-review`.
