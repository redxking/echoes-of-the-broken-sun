---
name: echoes-evidence-gate-review
description: Adversarially review an Echoes directive or ledger gate against retained evidence, provenance, and stated claim limits without modifying requirements to fit results.
metadata:
  author: Angelis Pseftis
---

# Echoes evidence-gate review

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md`, the controlling gate text, `Docs/Archive/ProjectLedger.md`, [echoes-session-control](../echoes-session-control/SKILL.md), and each cited artifact. Build a clause-by-clause trace: required condition, artifact path/hash, observed result, identity binding, and remaining gap. Re-run a narrow read-only verifier where possible.

Check exact commit/dirty tree, package identity where player-facing, command/configuration, dates, log completeness, warnings/errors, resource conditions, and whether a human/UI claim actually used rendered physical input. Do not accept screenshots as proof of interactive behavior and do not treat model-operated input as unfamiliar-human evidence.

Use the [RequirementsState status vocabulary](../../../Docs/RequirementsState.md#state-vocabulary) and report only observed evidence. Internal review is internal QA, never independent validation. Never edit gate wording, evidence, or ledger to manufacture closure. Route player-surface claims to `echoes-packaged-gui-smoke`, `echoes-mouse-keyboard-playtest`, or `echoes-human-acceptance-session`; package claims to `echoes-package-provenance`.
