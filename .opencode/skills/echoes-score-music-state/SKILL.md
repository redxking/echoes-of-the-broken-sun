---
name: echoes-score-music-state
description: Design or verify Echoes music-state logic, transitions, and mix priority from authoritative presentation events without claiming a generated cue is accepted in play.
metadata:
  author: Angelis Pseftis
---

# Echoes score music state

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/AudioDirection.md`, the current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Writes require a live granted lane.

Map only confirmed presentation events to score states. Music cannot decide gameplay, command results, narrative outcome, fog, saves, replay, or checksums. Default to registered deterministic generation; record sources, revision, idempotence, rights, and exceptions before use. Specify start, loop, stinger, transition, interruption, pause/resume, defeat/victory, silence, ducking, and reduced-dynamic-range behavior.

Acceptance: event/state matrix, registered asset basis, in-context listening evidence, transition/failure notes, and bounded status. Read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md` before generated audio, build, Editor/runtime listening. Route to `echoes-audio-listening-review` then `echoes-evidence-gate-review`. Stop for missing rights/provenance, unowned hooks, unlistened transition, or source state not exposed by the adapter.
