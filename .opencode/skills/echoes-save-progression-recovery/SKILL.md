---
name: echoes-save-progression-recovery
description: "Implement or verify Echoes campaign and player progression persistence, checkpoint recovery, corruption handling, and player-facing continuation paths."
metadata:
  author: Angelis Pseftis
---

# Echoes save, progression, and recovery

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA and isolated storage before mutation.

Persist authoritative deterministic state, campaign ledger/progression, content identity, and integrity checks as architecture requires. Presentation state is not authority. Define first run, save/load, checkpoint, failure/retry, interrupted write, missing/stale/mismatched/corrupt data, migration, privacy, and player recovery UI. Never silently fall back on corrupted content.

Use isolated fixtures and retained evidence; test checksum/replay where applicable, packaged recovery, and physical UI path via `echoes-gui-control-readiness`. Route replay behavior to `echoes-replay-qol`, security/privacy to `echoes-security-privacy`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on owner-held progression code, migration policy ambiguity, corrupted real user data risk, or absent recovery observation.
