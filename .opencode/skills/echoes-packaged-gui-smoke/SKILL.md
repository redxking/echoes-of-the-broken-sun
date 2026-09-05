---
name: echoes-packaged-gui-smoke
description: Smoke-test a provenance-verified packaged Echoes app through visible macOS player surfaces using fresh screenshot/state and real input events.
metadata:
  author: Angelis Pseftis
---

# Echoes packaged GUI smoke

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read `Docs/DemoRecoveryDirective.md`, the package verifier procedure in `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Require a callable GUI-control interface that returns fresh visible state/screenshots and can issue real mouse and keyboard events. If unavailable, stop and request a human or suitable tool; do not fabricate a run.

Verify the exact app bundle, package manifest/hash, commit, signature state, launch path, macOS user, display mode, and clean/known save state. Launch normally from the Finder/app surface where the gate requires it. Confirm title, selectable operation, briefing, deployment, pause/return, and clean exit by visible behavior and actual input. Use no Unreal MCP, console, debug flags, source-level shortcut, save injection, or internal command path.

Capture full paths, timestamps, every input, fresh state after each meaningful action, audio observation, defects, and package identity. Screenshots alone are not play. Agent-operated input is rendered evidence only, not an unfamiliar-human test or owner acceptance.

Before launch, read [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md), confirm an exclusive resource reservation, and record and release the reservation. Do not proceed on a stale, assumed, or unconfirmed reservation. Route package identity to `echoes-package-provenance` and human conclusion to `echoes-human-acceptance-session`.
