---
name: echoes-economy-logistics
description: "Implement or verify authorized Matter, Dawn, income, hauling, storage, and logistics behavior as deterministic Echoes simulation rules with player-visible feedback."
metadata:
  author: Angelis Pseftis
---

# Echoes economy and logistics

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA before mutation.

Define source-authoritative costs, rates, capacities, ownership, invalid requests, depletion, stalls, and recovery. Rules live in deterministic simulation; runtime UI/audio only render authority. Tune source data through the official compiler, never generated catalogs. Test tick-level accounting, path/logistics failure, save/replay continuity, and player-visible affordability/rejection feedback.

Route construction to `echoes-construction-production`, Well costs to `echoes-future-wells-gameplay`, build/runtime to `echoes-unreal-runtime-integration`, physical play to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on missing canon values, shared live task ownership, imbalance target without approved metric, or untestable persistence behavior.

## Accepted worker direction

Use SPEC-RES-003..007 as the canonical extraction, routing, depletion and reservation contract.
One extraction position limits simultaneous work, not total assigned haulers. The owner repeated this
exclusive-extraction instruction on 2026-09-05; it supersedes the earlier two-position baseline. Preserve faction logistics
identity and visible work, waiting, cargo and failure states. Respect stable explicit delivery assignments
and known reachable fallback; retain cargo when no destination is valid. Exhausted sites remain recognizable
under observation and fog memory. Previously reserved production may finish after supply loss.

The baseline is approved for implementation/testing, not demonstrated balanced. Compare delivered income
and route congestion, expansion, harassment and faction matchups before balance acceptance. Resolve the
separately tracked work-rate/cargo discrepancy before changing those numeric values.
