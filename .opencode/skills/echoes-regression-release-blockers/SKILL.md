---
name: echoes-regression-release-blockers
description: Assess Echoes regression and release-blocker status from current evidence, known defects, package provenance, and human-acceptance boundaries without declaring release closure.
metadata:
  author: Angelis Pseftis
---

# Echoes regression and release blockers

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md`, `Docs/Archive/ProjectLedger.md`, [echoes-session-control](../echoes-session-control/SKILL.md), [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md), package provenance, latest defect triage, and controlling gate evidence. Build a current matrix: release-critical path, package identity, latest regression boundary, open S0–S4 defects, human/clean-machine evidence, ownership, and next verification. Do not infer that a historical green suite covers a changed package or player path.

Treat any known S0 or S1 on a release-critical path as a blocker until corrected and re-evidenced. Zero known S2 may remain on a release-critical path absent an explicit Angelis waiver bound to the exact defect/build. S2–S4 still require documented impact, ownership, workaround status, and gate relevance; documentation alone is not a waiver and defects do not silently disappear. Re-run work only with established task ownership and, for build/package/profile/soak/GUI/player/audio/visual activity, an exclusive resource reservation that is recorded and released afterward.

Use the [RequirementsState status vocabulary](../../../Docs/RequirementsState.md#state-vocabulary) and report only observed evidence. This is assessment, not authority to ship, upload, publish, or edit gates.
