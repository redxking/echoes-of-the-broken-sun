---
name: echoes-visual-direction-lighting
description: Design or review Echoes lighting, composition, palette, and environmental readability without altering deterministic play state or treating a screenshot as player validation.
metadata:
  author: Angelis Pseftis
---

# Echoes visual direction and lighting

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), then Track A in `Docs/GameCompletionDirective.md`, `Docs/Archive/DevelopmentBible.md` (§Creative direction, §Art and audio), `Docs/ArtDirection.md`, `Docs/Archive/TechnicalArchitecture.md` (§Presentation), `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Before any mutation, confirm live task ownership owns every target path; otherwise produce a proposal only.

Build lighting and visual hierarchy from the Bible's world/faction language and the current directive—not generic fantasy conventions. Presentation must be spawned from authoritative state and must not affect collision, overlaps, navigation, shadows, fog authority, saves, replays, or checksums. Keep Nanite and VSM off unless the owner records a changed baseline decision.

For each slice, state the gameplay question it improves (selection, terrain, threat, objective, faction, or modal state), affected camera distances, and accessibility response. Render the actual camera paths at target quality and inspect in motion at normal play pace; screenshots are supporting evidence only. Record exact build, map, settings, hardware, and capture boundary in the ledger/evidence location required by the directive.

Before Editor, runtime, GPU-intensive capture, or profiling, coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md). Route rendered visual assessment to [echoes-realtime-visual-review](../echoes-realtime-visual-review/SKILL.md), evidence classification to [echoes-evidence-gate-review](../echoes-evidence-gate-review/SKILL.md), and owner review to [echoes-human-acceptance-session](../echoes-human-acceptance-session/SKILL.md).

Acceptance output: scoped implementation or review, before/after rendered evidence, visual-readability observations, and a truthful evidence status. Exclude simulation, generated-content compiler bypasses, and unregistered assets. Stop when ownership is absent, the source basis is unclear, a visual change masks gameplay information, or required rendered review cannot run.

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
