---
name: echoes-ambience-world-soundscape
description: Create or inspect Echoes environmental ambience, spatial world sound, and biome/faction soundscape without obscuring gameplay, voice, or accessibility cues.
metadata:
  author: Angelis Pseftis
---

# Echoes ambience and world soundscape

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/AudioDirection.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. A granted lane is mandatory for mutation.

Use procedural-first registered sources and record exception method/license/rationale before import. Ambience may respond to exposed world/presentation state but cannot reveal fogged information or affect simulation, collision, navigation, command validity, saves, replay, or checksums. Preserve mix headroom for voice and gameplay feedback; provide reduced dynamic range behavior.

Acceptance: soundscape/event map, AssetRegister evidence, listening observations at normal and reduced dynamic range, interruption/loop checks, and evidence class. Read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md` before Editor/runtime listening or generation. Use `echoes-audio-listening-review` and `echoes-evidence-gate-review`. Stop for unavailable authority, concealed-state leak, missing license, unowned path, or no in-context listening.
