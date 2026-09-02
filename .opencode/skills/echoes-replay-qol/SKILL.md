---
name: echoes-replay-qol
description: "Implement or assess Echoes replay integrity and quality-of-life behavior without conflating playback convenience with deterministic proof."
metadata:
  author: Angelis Pseftis
---

# Echoes replay and quality of life

Read `CLAUDE.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify lease/worktree/branch/SHA before mutation.

Bind replay inputs/state to content/build identity and checksum per architecture. Define playback controls, pause/seek availability only where authorized, mismatch refusal, corruption recovery, accessibility, and user-visible limits. UI playback state may render authority but cannot rewrite commands, simulation, or checksum. Use source data/compiler paths; never hand-edit replay/generated records.

Test deterministic equivalence and corrupt/mismatch refusal separately from player usability. Route persistence to `echoes-save-progression-recovery`, GUI playback to `echoes-gui-control-readiness`, evidence to `echoes-evidence-gate-review`, and final owner judgment to `echoes-human-acceptance-session`. Stop on no authorized QoL scope, replay architecture ambiguity, lease conflict, or unobservable failure reason.
