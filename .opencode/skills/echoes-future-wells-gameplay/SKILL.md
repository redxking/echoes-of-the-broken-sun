---
name: echoes-future-wells-gameplay
description: "Implement or verify the canonical Echoes Future Well choices, costs, telegraphs, effects, and player readability while preserving owner-controlled pinned values."
metadata:
  author: Angelis Pseftis
---

# Echoes Future Wells gameplay

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md` §13, and [echoes-session-control](../echoes-session-control/SKILL.md); verify exact current text, owner authority, task ownership, worktree, branch, and SHA before mutation.

Current §13 values are pinned for live verification, not assumed change authority: Harvest 180-tick telegraph / 500 Dawn; Preserve 15 Dawn per 300 ticks / 1,400 cm radius; Reshape 120 Dawn / 180-tick telegraph / 1,800-tick manifestation. Changes require Angelis approval. Keep Well state, costs, timing, outcomes, save/replay, and checksums in deterministic simulation; render telegraphs, UI, VFX, and sound from authority only.

Test selection, affordability, timing, interruption, invalid choice, serialization/replay, and visible/audio/accessibility feedback. Route economy to `echoes-economy-logistics`, world effects to `echoes-world-level-design`, runtime/UI to `echoes-unreal-runtime-integration` and `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop for a value change request without owner approval, canonical conflict, ownership conflict, or missing physical observation route.
