---
name: echoes-workstream-integration
description: "Integrate one bounded Echoes lane slice while protecting active leases, dirty owner work, source provenance, and evidence continuity."
metadata:
  author: Angelis Pseftis
---

# Echoes workstream integration

Use when handing off, reviewing, freezing, or integrating a bounded lane slice; not for taking over adjacent lanes.

1. Read live `CLAUDE.md`, `Docs/GameCompletionDirective.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify the exact lease, base SHA, worktree, branch, and frozen scope before any mutation.
2. Compare only the leased paths against the declared base. Preserve all pre-existing dirty paths and other lanes' changes. Never use broad staging, history rewrite, or conflict resolution that changes an unowned file.
3. Confirm every changed source-data path was compiled through its official pipeline and every generated artifact is traceable to source and digest. Simulation authority cannot be moved into presentation during integration.
4. Review the acceptance card against the actual diff, command output, artifact hashes, and required evidence class. Record mismatches as defects or unproven—not as completion.
5. Stop on moved interfaces, lease conflicts, missing base identity, an unmounted volume, failed baseline, or an owner-only choice. Escalate with exact paths and options.

## Integration result

Return the exact scope, SHA/patch identity, checks read, evidence location, unresolved risks, and bounded status. Route evidence to `echoes-evidence-gate-review`; if player-facing, route physical verification through `echoes-gui-control-readiness`, then owner review through `echoes-human-acceptance-session`. An integration receipt never proves player experience by itself.
