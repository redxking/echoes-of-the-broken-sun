---
name: echoes-economy-logistics
description: "Implement or verify authorized Matter, Dawn, income, hauling, storage, and logistics behavior as deterministic Echoes simulation rules with player-visible feedback."
metadata:
  author: Angelis Pseftis
---

# Echoes economy and logistics

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify exact lease/worktree/branch/SHA before mutation.

Define source-authoritative costs, rates, capacities, ownership, invalid requests, depletion, stalls, and recovery. Rules live in deterministic simulation; runtime UI/audio only render authority. Tune source data through the official compiler, never generated catalogs. Test tick-level accounting, path/logistics failure, save/replay continuity, and player-visible affordability/rejection feedback.

Route construction to `echoes-construction-production`, Well costs to `echoes-future-wells-gameplay`, build/runtime to `echoes-unreal-runtime-integration`, physical play to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on missing canon values, shared lease, imbalance target without approved metric, or untestable persistence behavior.
