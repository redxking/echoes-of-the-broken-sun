---
name: echoes-material-texture-assets
description: Create or assess Echoes meshes, materials, textures, and environment asset families with registered provenance, Apple-Silicon-aware budgets, and source-to-generated integrity.
metadata:
  author: Angelis Pseftis
---

# Echoes materials, textures, and assets

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Track A and §7, `Docs/ArtDirection.md`, `Docs/Archive/DevelopmentBible.md` (§Ecology and architecture, §Art and audio), `Docs/Archive/AssetRegister.md`, `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Check a current task ownership before mutation.

Default to deterministic project generation through the registered art pipeline; edit source and regenerate, never hand-edit generated output. Before use, register each family and revision in `Docs/Archive/AssetRegister.md`, including source method, license/exception, rationale, outputs, and idempotence boundary. An exception for licensed, commissioned, or local generative work requires an owner-recorded decision before import. Do not scrape marketplaces or infer rights from file presence.

Evaluate materials at gameplay camera distance and scalable quality levels, not asset-browser beauty shots. Preserve readable faction/objective/ownership cues under fog, selection, high contrast, and color-vision alternatives. New presentation assets must disable collision, overlap, navigation influence, and shadows where applicable.

Acceptance output: registered source family, reproducible generation/import record, rendered in-game inspection, performance observations, and evidence classification. Exclude simulation/state edits and generated-output hand editing. Stop for missing provenance, an unrecorded exception, non-idempotent generation, unavailable path ownership, or an asset that fails readability/performance review.

Authority is exactly `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, the current `Docs/` directive/ledger, and [echoes-session-control](../echoes-session-control/SKILL.md). Delegate visual runtime review to `echoes-realtime-visual-review` and evidence closure to `echoes-evidence-gate-review`. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before generation, import, Editor launch, or GPU review.

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
