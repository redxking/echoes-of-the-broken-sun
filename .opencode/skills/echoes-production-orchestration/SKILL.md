---
name: echoes-production-orchestration
description: "Sequence one bounded Echoes release slice across design, implementation, verification, and owner acceptance within established path ownership."
metadata:
  author: Angelis Pseftis
---

# Echoes production orchestration

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Use to sequence one bounded requirement slice across authority, implementation, verification, and owner review.

Define the controlling IDs, exact source/design authority, accountable task owner, affected paths, test method, evidence root, resource need, and stop condition. Preserve simulation authority, registered-source generation, and existing work. Select the narrow domain skill for implementation, then perform the evidence class the requirement actually calls for; player-facing work may require physical GUI, rendered visual, or listening evidence, each distinct from owner acceptance.

Use [echoes-session-control](../echoes-session-control/SKILL.md) for overlapping-path ownership and [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) for exclusive resources. Missing historical coordination records do not block authorized safe work. Report observed state through [RequirementsState](../../../Docs/RequirementsState.md#state-vocabulary); owner acceptance and requirement completion remain Angelis decisions.
