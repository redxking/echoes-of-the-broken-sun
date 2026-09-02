---
name: echoes-packaged-gui-smoke
description: Smoke-test a provenance-verified packaged Echoes app through visible macOS player surfaces using fresh screenshot/state and real input events.
metadata: { author: Angelis Pseftis }
---

# Echoes packaged GUI smoke

Read `Docs/DemoRecoveryDirective.md`, the package verifier procedure in `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Require a callable GUI-control interface that returns fresh visible state/screenshots and can issue real mouse and keyboard events. If unavailable, stop and request a human or suitable tool; do not fabricate a run.

Verify the exact app bundle, package manifest/hash, commit, signature state, launch path, macOS user, display mode, and clean/known save state. Launch normally from the Finder/app surface where the gate requires it. Confirm title, selectable operation, briefing, deployment, pause/return, and clean exit by visible behavior and actual input. Use no Unreal MCP, console, debug flags, source-level shortcut, save injection, or internal command path.

Capture full paths, timestamps, every input, fresh state after each meaningful action, audio observation, defects, and package identity. Screenshots alone are not play. Agent-operated input is rendered evidence only, not an unfamiliar-human test or owner acceptance.

Before launch, read `../WorkstreamControl/HEAVY_RUN_LOCK.md`, acquire a current detailed Heavy-Run lease, and explicitly release it. Do not proceed on a stale, assumed, or self-invented lease. Route package identity to `echoes-package-provenance` and human conclusion to `echoes-human-acceptance-session`.
