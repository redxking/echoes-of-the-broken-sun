---
name: echoes-subtitle-caption-runtime
description: Implement or inspect Echoes runtime subtitle and caption display, timing, interruption, and accessibility behavior from canonical source and actual playback.
metadata:
  author: Angelis Pseftis
---

# Echoes subtitle and caption runtime

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/CharacterVoiceIdentityBible.md`, the authoritative narrative source, and [echoes-session-control](../echoes-session-control/SKILL.md). Mutation requires established task ownership.

Canonical dialogue remains authoritative; do not rewrite mission source or hand-edit generated output. Bind speaker/text/timing to the actual runtime voice/event and retain pause, skip, interruption, replay, recovery, safe-area, scale, contrast, and enable/disable behavior. Captions must not disclose information the authoritative game has not exposed.

Acceptance: canonical-source mapping, playback timing/readability observations, accessibility behavior, and evidence boundary. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before editor/packaged playback. Route to `echoes-ui-accessibility-playtest`, `echoes-audio-listening-review`, and `echoes-evidence-gate-review`. Stop for source mismatch, unowned surface, missing sync, behaviorless option, or no playback evidence.
