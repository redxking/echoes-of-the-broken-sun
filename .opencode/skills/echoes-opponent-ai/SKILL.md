---
name: echoes-opponent-ai
description: "Build and evaluate Echoes skirmish or mission opposition as an authorized, deterministic, observable player-facing opponent—not merely a passing behavior tree."
metadata:
  author: Angelis Pseftis
---

# Echoes opponent AI

Use for AI lifecycle, strategy, tactics, difficulty, mission opponents, telemetry, or AI-versus-player validation.

1. Read live `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify exact lease/worktree/branch/dirty paths before mutation.
2. Keep AI decisions within the authoritative simulation contract and legal information boundaries. No hidden presentation-state shortcuts, non-deterministic timing dependence, or privileged fog/command access unless explicitly authorized.
3. Define scenario, map, faction, difficulty, seed, time budget, opponent objective, expected decision observables, and failure/recovery behavior before implementation. Instrument decisions without changing game outcome.
4. Run deterministic/automated scenarios, then a player-facing match or mission path when judging pressure, clarity, fairness, or fun. Inspect loss/win, stall, exploit, and recovery cases.
5. Stop for data/canon uncertainty, AI lane conflict, unmeasurable difficulty target, hidden-information ambiguity, or any requested redesign outside `Docs/Archive/DevelopmentBible.md`.

## Acceptance checks

Record revision, scenario matrix, seeds, telemetry, outcomes, known exploits, and evidence class. Route balance to `echoes-balance-analysis`, player matches to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.
