---
name: echoes-formations-unit-cohesion
description: "Develop or assess authorized Echoes group movement, formation, escort, spacing, cohesion, and recovery behavior under deterministic simulation constraints."
metadata:
  author: Angelis Pseftis
---

# Echoes formations and unit cohesion

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify lease/worktree/branch/SHA before mutation.

Define formation/escort eligibility, anchor, spacing, blocked routing, casualties, regrouping, command replacement, fog limits, performance, and persistence. Keep all formation state and outcomes deterministic; camera/VFX/audio may expose status but never decide it. Use authoritative source data and compilers for tunables.

Check deterministic replay/checksum behavior, obstacle and casualty recovery, stress/performance, visible readability, and physical control when player-facing. Route base movement to `echoes-selection-movement-pathing`, combat interactions to `echoes-combat-targeting-damage`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on unclear canonical behavior, active movement lease, nondeterminism, or missing scenario metrics.
