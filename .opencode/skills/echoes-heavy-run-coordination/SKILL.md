---
name: echoes-heavy-run-coordination
description: "Coordinate a single leased Echoes heavy run with explicit acquisition, isolation, evidence retention, and release—not concurrent execution."
metadata:
  author: Angelis Pseftis
---

# Echoes heavy-run coordination

Read live `CLAUDE.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/GameCompletionDirective.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`.

Before acquiring or running, require the lock's request fields exactly: lane/task, worktree, branch/SHA, operation, expected outputs, ports/GPU use, start time, timeout, and cleanup/rollback. Confirm `State: FREE`; do not infer availability from an absent process. A holder must preserve failed artifacts, verify scoped storage and residual processes, and explicitly release by recording outcome, evidence location, cleanup/rollback, and release time in the lock. Process disappearance is not release.

Use this only for defined heavy classes in the lock policy. Capture target hardware, resolution/device route, thermal or sustained-load observation when the requirement calls for it. Never edit source/generated content as part of coordination; source authority and simulation authority remain with the implementation lane. Route build/runtime execution to its domain skill, GUI/player observation to `echoes-gui-control-readiness`, evidence to `echoes-evidence-gate-review`, and owner acceptance to `echoes-human-acceptance-session`. Stop for HELD state, incomplete request fields, conflicting editor/ports, storage risk, or missing owner authorization.
