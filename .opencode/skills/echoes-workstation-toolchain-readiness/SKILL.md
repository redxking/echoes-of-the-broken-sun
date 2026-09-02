---
name: echoes-workstation-toolchain-readiness
description: "Establish whether the live Mac workstation can safely run a named Echoes build, content, packaging, or verification command without altering project state."
metadata:
  author: Angelis Pseftis
---

# Echoes workstation toolchain readiness

Read live `CLAUDE.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/GameCompletionDirective.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Confirm the exact worktree/SHA, mounted Seagate volume, required free space, host tool versions, no conflicting editor/process, and lease before any command that mutates build outputs.

Check the precise command, inputs, expected outputs, environment traps (including documented `TMPDIR` and `GIT_*` handling), available storage, target hardware/architecture, device/display assumptions, thermal/headroom observation plan, and retained-log location. A tool present on PATH is not proof that the toolchain, project generation, or target build works. Never change source, generated artifacts, Xcode/Unreal settings, or dependencies under this skill.

Route a permitted build to `echoes-heavy-run-coordination` and `echoes-unreal-runtime-integration`, then its results to `echoes-evidence-gate-review`; player-facing packages also route to `echoes-gui-control-readiness` and `echoes-human-acceptance-session`. Stop on missing capability, stale identity, active heavy lease, low storage, or owner-held environment decision.
