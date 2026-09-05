---
name: echoes-ambience-world-soundscape
description: Create or inspect Echoes environmental ambience, spatial world sound, and biome/faction soundscape without obscuring gameplay, voice, or accessibility cues.
metadata:
  author: Angelis Pseftis
---

# Echoes ambience and world soundscape

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/AudioDirection.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Established task ownership is mandatory for mutation.

Use procedural-first registered sources and record exception method/license/rationale before import. Ambience may respond to exposed world/presentation state but cannot reveal fogged information or affect simulation, collision, navigation, command validity, saves, replay, or checksums. Preserve mix headroom for voice and gameplay feedback; provide reduced dynamic range behavior.

Acceptance: soundscape/event map, AssetRegister evidence, listening observations at normal and reduced dynamic range, interruption/loop checks, and evidence class. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before Editor/runtime listening or generation. Use `echoes-audio-listening-review` and `echoes-evidence-gate-review`. Stop for unavailable authority, concealed-state leak, missing license, unowned path, or no in-context listening.

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
