---
name: echoes-save-replay-recovery
description: "Implement or verify Echoes save, replay, checkpoint, and recovery behavior with deterministic integrity, version boundaries, and player-visible failure handling."
metadata:
  author: Angelis Pseftis
---

# Echoes save, replay, and recovery

Use for persistence containers, campaign progression, replay/checksum, corruption handling, migration, or recovery UX.

1. Read live `CLAUDE.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify exact lease/worktree/branch/dirty paths before mutation.
2. Preserve simulation authority: saves and replays bind deterministic state, commands, content/catalog identity, and checksums as the architecture requires. Presentation objects, audio, and camera state do not become authoritative persistence.
3. Define valid save/load, interrupted write, stale/mismatched content, corrupted container, missing asset, disk failure, incompatible version, replay divergence, and player recovery behavior before coding.
4. Test clean and adversarial paths using isolated temporary evidence data. Verify fail-closed behavior and the player-visible reason/recovery route; never convert corruption into silent fallback.
5. Stop if migration semantics, retention policy, user-data handling, owner-held progress code, or lease scope is unresolved.

## Acceptance checks

Record content/build identity, state/checksum equivalence, replay determinism where applicable, failure fixtures, recovery observations, and paths of retained evidence. Route the focused work to `echoes-save-progression-recovery` or `echoes-replay-qol`, physical recovery to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.
