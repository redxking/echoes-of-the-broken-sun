---
name: echoes-build-automation
description: Build a narrow Echoes Unreal or simulation slice on the Mac while preserving path ownership, baseline evidence, and reproducible logs; not for packaging or release claims.
metadata:
  author: Angelis Pseftis
---

# Echoes build automation

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Before acting, read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md`, the current limitations and evidence register in `Docs/Archive/ProjectLedger.md`, [echoes-session-control](../echoes-session-control/SKILL.md), and [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md). Confirm an exclusive resource reservation before any build, then record and release the reservation; never use a stale or unconfirmed reservation. Select one unblocked directive gate and define its observable check before editing. Do not take paths owned by another task.

Use the smallest applicable Mac command: `Scripts/test_sim.sh`, `Scripts/test_content.sh`, `Scripts/build_editor.sh`, or the exact focused automation named by the gate. For Unreal tests set `TMPDIR` as documented in [Docs/Archive/SetupAndBuild.md](../../../Docs/Archive/SetupAndBuild.md). Bind every retained log to commit, dirty-tree status, target/configuration, command, exit status, and UTC time. A compile is not runtime evidence; a test is not packaged evidence.

Stop below the documented storage gates, on a missing Seagate volume, an ownership conflict, or an untriaged baseline failure. Never relax a test, timeout, or gate. Record only results actually read this session, including warnings and claim limits, in the controlling ledger/evidence location when authorized.
