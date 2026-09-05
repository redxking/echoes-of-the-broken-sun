---
name: echoes-campaign-human-playthrough
description: Evidence a visible fresh-ledger Echoes campaign journey through rendered UI and physical-input-equivalent mouse/keyboard events while preserving the distinction between agent operation and human acceptance.
metadata:
  author: Angelis Pseftis
---

# Echoes campaign journey playthrough

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/GameCompletionDirective.md`, the campaign contracts in `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/TechnicalArchitecture.md`, the exact `echoes-package-provenance` record, and [echoes-session-control](../echoes-session-control/SKILL.md). Require a GUI-control interface with fresh state/screenshots and actual mouse/keyboard events. If absent, stop and request human/tool access. Start from a verified empty player ledger through the normal title/new-campaign surface; record save location and isolation.

For every mission record the mission and map IDs, source and compiled map identities, title, inherited choices, briefing comprehension, deployment, core objective, visible input sequence, audio/subtitle/cinematic behavior, terminal result, continuation, save transition, and exact package identity. Validate `SPEC-MAP-004`, `SPEC-CAM-041`, and `SPEC-CAM-042` against all 15 distinct M01–M15 map identities and the observed connected-geography, story, and character-continuity cues; display-name uniqueness alone is insufficient. Work only through player-facing UI. No Unreal MCP, console/debug mode, direct simulation, hidden commands, save injection, test route, or source-guided coordinate shortcut. A failure/retry belongs in the record.

Capture timings, input/state trail, audio observations, defects, package and save hashes, and final ledger result. A model-operated route is rendered evidence, not proof that an unfamiliar person can complete the campaign and never `HUMAN ACCEPTED`.

Before any rendered campaign run, read [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md), confirm an exclusive resource reservation, and record and release the reservation. Never use a stale or unconfirmed reservation. Verify package identity through `echoes-package-provenance`; route human acceptance only to `echoes-human-acceptance-session`.
