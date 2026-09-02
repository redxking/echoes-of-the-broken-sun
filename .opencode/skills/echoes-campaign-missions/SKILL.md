---
name: echoes-campaign-missions
description: "Implement or validate one Echoes campaign operation, its objectives, state transitions, checkpoints, and win/loss paths against authorized mission contracts."
metadata:
  author: Angelis Pseftis
---

# Echoes campaign missions

Use for M01–M15 mission systems, objective triggers, campaign progression, briefing-to-results flow, or mission verification.

1. Read live `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, `Content/Narrative/Source`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify lease/worktree/branch/dirty paths before mutation.
2. Preserve mission contract IDs, source pins, campaign order, objective semantics, and canonical outcomes. Mission/narrative source changes go through the approved source compiler; never patch compiled packs or relax validators.
3. Specify the ordinary-player path from empty ledger/entry through briefing, action, objective completion, failure, retry/checkpoint, result, and next-state continuity. Include all required loss and recovery conditions.
4. Validate deterministic mission logic and then exercise the rendered physical-input path. Record which mission sections were automated, GUI-driven, or observed by a human; screenshots alone do not prove the flow.
5. Stop for a frozen contract, narrative/canon ambiguity, campaign lease conflict, owner decision on pacing/outcome, or inability to test the real entry path.

## Acceptance checks

Capture mission ID, source digest, save/progression state, input method, build/commit, objective and recovery outcomes, logs/captures, and evidence class. Route narrative to `echoes-narrative-character-writing`, persistence to `echoes-save-progression-recovery`, GUI exercise to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.
