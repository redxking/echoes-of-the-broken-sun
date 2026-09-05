---
name: echoes-workstation-toolchain-readiness
description: "Establish whether the live Mac workstation can safely run a named Echoes build, content, packaging, or verification command without altering project state."
metadata:
  author: Angelis Pseftis
---

# Echoes workstation toolchain readiness

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/SetupAndBuild.md`, `Docs/GameCompletionDirective.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Confirm the exact worktree/SHA, mounted Seagate volume, required free space, host tool versions, no conflicting editor/process, and live task ownership before any command that mutates build outputs.

Check the precise command, inputs, expected outputs, environment traps (including documented `TMPDIR` and `GIT_*` handling), available storage, target hardware/architecture, device/display assumptions, thermal/headroom observation plan, and retained-log location. A tool present on PATH is not proof that the toolchain, project generation, or target build works. Never change source, generated artifacts, Xcode/Unreal settings, or dependencies under this skill.

Route a permitted build to `echoes-heavy-run-coordination` and `echoes-unreal-runtime-integration`, then its results to `echoes-evidence-gate-review`; player-facing packages also route to `echoes-gui-control-readiness` and `echoes-human-acceptance-session`. Stop on missing capability, stale identity, active heavy resource reservation, low storage, or owner-held environment decision.
