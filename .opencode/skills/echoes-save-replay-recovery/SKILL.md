---
name: echoes-save-replay-recovery
description: "Implement or verify Echoes save, replay, checkpoint, and recovery behavior with deterministic integrity, version boundaries, and player-visible failure handling."
metadata:
  author: Angelis Pseftis
---

# Echoes save, replay, and recovery

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Use this compatibility router only to choose the narrow work skill.

Save and campaign progression: [echoes-save-progression-recovery](../echoes-save-progression-recovery/SKILL.md). Replay behavior and player quality of life: [echoes-replay-qol](../echoes-replay-qol/SKILL.md).

This router does not authorize cross-domain changes or replace the selected skill's required evidence.
