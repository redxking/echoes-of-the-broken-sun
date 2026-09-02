---
name: echoes-subtitle-caption-runtime
description: Implement or inspect Echoes runtime subtitle and caption display, timing, interruption, and accessibility behavior from canonical source and actual playback.
metadata:
  author: Angelis Pseftis
---

# Echoes subtitle and caption runtime

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/CharacterVoiceIdentityBible.md`, the authoritative narrative source, the current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Mutation requires a granted lane.

Canonical dialogue remains authoritative; do not rewrite mission source or hand-edit generated output. Bind speaker/text/timing to the actual runtime voice/event and retain pause, skip, interruption, replay, recovery, safe-area, scale, contrast, and enable/disable behavior. Captions must not disclose information the authoritative game has not exposed.

Acceptance: canonical-source mapping, playback timing/readability observations, accessibility behavior, and evidence boundary. Read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md` before editor/packaged playback. Route to `echoes-ui-accessibility-playtest`, `echoes-audio-listening-review`, and `echoes-evidence-gate-review`. Stop for source mismatch, unowned surface, missing sync, behaviorless option, or no playback evidence.
