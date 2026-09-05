---
name: echoes-construction-production
description: "Implement or verify Echoes building placement, queues, spawn rules, production failure states, and player feedback without bypassing simulation authority."
metadata:
  author: Angelis Pseftis
---

# Echoes construction and production

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); confirm task ownership, worktree, branch, and SHA and data-source ownership before mutation.

Specify valid placement, footprint, collision, navigation authority, resources, prerequisites, queue/order, cancellation, spawn, blocked conditions, destruction, save/replay behavior, game-feel feedback, and stable player reason for every rejection. Simulation owns placement and production; terrain/nav and presentation cannot silently substitute rules. Compile tunables from authoritative source, never hand-edit generated output.

Exercise direct rules plus Unreal command/UI feedback and physical input where relevant. Route resource behavior to `echoes-economy-logistics`, tech gates to `echoes-research-technology`, build/runtime to `echoes-unreal-runtime-integration`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on shared hotspot, undefined canonical rule, incompatible save/replay change, or missing accepted failure check.
