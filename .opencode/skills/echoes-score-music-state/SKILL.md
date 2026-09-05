---
name: echoes-score-music-state
description: Design or verify Echoes music-state logic, transitions, and mix priority from authoritative presentation events without claiming a generated cue is accepted in play.
metadata:
  author: Angelis Pseftis
---

# Echoes score music state

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/AudioDirection.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Writes require a current task ownership.

Map only confirmed presentation events to score states. Music cannot decide gameplay, command results, narrative outcome, fog, saves, replay, or checksums. Default to registered deterministic generation; record sources, revision, idempotence, rights, and exceptions before use. Specify start, loop, stinger, transition, interruption, pause/resume, defeat/victory, silence, ducking, and reduced-dynamic-range behavior.

Acceptance: event/state matrix, registered asset basis, in-context listening evidence, transition/failure notes, and bounded status. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before generated audio, build, Editor/runtime listening. Route to `echoes-audio-listening-review` then `echoes-evidence-gate-review`. Stop for missing rights/provenance, unowned hooks, unlistened transition, or source state not exposed by the adapter.

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
