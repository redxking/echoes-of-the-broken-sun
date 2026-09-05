---
name: echoes-replay-qol
description: "Implement or assess Echoes replay integrity and quality-of-life behavior without conflating playback convenience with deterministic proof."
metadata:
  author: Angelis Pseftis
---

# Echoes replay and quality of life

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA before mutation.

Bind replay inputs/state to content/build identity and checksum per architecture. Define playback controls, pause/seek availability only where authorized, mismatch refusal, corruption recovery, accessibility, and user-visible limits. UI playback state may render authority but cannot rewrite commands, simulation, or checksum. Use source data/compiler paths; never hand-edit replay/generated records.

Test deterministic equivalence and corrupt/mismatch refusal separately from player usability. Route persistence to `echoes-save-progression-recovery`, GUI playback to `echoes-gui-control-readiness`, evidence to `echoes-evidence-gate-review`, and final owner judgment to `echoes-human-acceptance-session`. Stop on no authorized QoL scope, replay architecture ambiguity, ownership conflict, or unobservable failure reason.
