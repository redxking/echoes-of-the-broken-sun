---
name: echoes-heavy-run-coordination
description: "Coordinate a single exclusive Echoes heavy run with explicit acquisition, isolation, evidence retention, and release—not concurrent execution."
metadata:
  author: Angelis Pseftis
---

# Echoes heavy-run coordination

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Use before a build, cook, package, editor/runtime launch, profiling, soak, GPU-intensive capture, port-bound automation, or GUI run. It coordinates exclusive resource use; it does not authorize source changes or acceptance.

Before launch, identify the task owner, worktree and commit, dirty state, operation, resources (ports, editor, GPU, storage, save location), expected evidence, timeout, and cleanup/recovery plan. Confirm with current live coordination that those resources are available. Do not infer availability from a retired lock file or the absence of a process.

During and after the run, preserve failed artifacts, record command/configuration, environment, output, and hashes where applicable in `BuildArtifacts/Evidence/<gate>-<UTC>/`. Check for residual processes and scoped storage, then record the outcome and release the reservation. A run ending does not prove the requirement or owner acceptance.
