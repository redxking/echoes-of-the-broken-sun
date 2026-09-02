---
name: echoes-gui-control-readiness
description: "Determine whether a live macOS GUI-control route can produce fresh, attributable Echoes pointer, keyboard, visual, and audio observations; if any capability is absent, stop with BLOCKED and never claim play, PASS, or human acceptance."
metadata:
  author: Angelis Pseftis
---

# Echoes GUI control readiness

Read live `CLAUDE.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/GameCompletionDirective.md`, `Docs/InitialReleaseRequirements.md`, `Docs/Archive/SetupAndBuild.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`; verify exact worktree/package identity and lane before launching or interacting with a build.

Verify—not assume—a callable macOS UI tool, fresh screenshot/state capture, real pointer and keyboard event delivery, Screen Recording permission, Accessibility permission, and an audio observation route. Bind captures to time, package/build identity, screen/window state, input sequence, and observer. Synthetic or agent GUI input is its own evidence class and never human play.

Do not install tools, grant permissions, claim a screenshot is current, or claim that a route proves gameplay. Capability inspection alone does not take the heavy lock, but any game launch or delivered input requires a current detailed lease acquired through `echoes-heavy-run-coordination` and explicitly released afterward. Stop if the lease, capability, attribution, visible state, or audio observation route is absent; record the precise missing prerequisite and use `BLOCKED`, never a fabricated play/pass/acceptance status. Route valid runs to the relevant implementation skill, `echoes-evidence-gate-review`, then owner-only `echoes-human-acceptance-session`.
