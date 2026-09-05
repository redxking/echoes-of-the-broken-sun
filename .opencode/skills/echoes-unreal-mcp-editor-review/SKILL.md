---
name: echoes-unreal-mcp-editor-review
description: Route Echoes Unreal Editor and MCP inspection separately from real OS-level mouse-and-keyboard play, with evidence classes and mutation authority kept explicit.
metadata:
  author: Angelis Pseftis
---

# Echoes Unreal MCP and editor review

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), the live directive, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/TechnicalArchitecture.md`, the relevant requirement ledger, and [echoes-session-control](../echoes-session-control/SKILL.md) before invoking the editor. This is primarily a review/evidence skill; do not mutate project assets or source unless live task ownership explicitly owns each target path.

Use the Mac for Unreal, packaging, and editor automation. This is an umbrella/router, not proof of player input: Unreal MCP/editor automation cannot prove OS-level mouse/keyboard delivery or human-style play. Route editor-internal inspection to `echoes-unreal-mcp-editor-inspection`, and real device-driven play to `echoes-mouse-keyboard-playtest`.

Treat evidence classes separately: source inspection, automated test, editor preview, rendered capture, and human-style gameplay are not interchangeable. Screenshots may show a frame but cannot prove input, flow, timing, sound, recovery, or playability. Capture exact map/mode, settings, hardware, sequence, observed outcomes, failures, and abort conditions. Use current heavy-run coordination and established task-ownership rules.

Acceptance output: reproducible review protocol, retained observed evidence, defect list or bounded observed result, and explicit untested conditions. Exclude release/readiness claims, independent-validation claims, and hidden state inference. Stop if the editor/build cannot start, the volume/memory/storage preconditions fail, input control is unavailable, the heavy-resource reservation is held, or any action would exceed task authority.

Always read [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md); confirm an exclusive resource reservation or stop before Editor, packaged runtime, GPU-heavy review, build, or automation. Route visual/audio/accessibility/evidence/owner-signoff respectively to `echoes-realtime-visual-review`, `echoes-audio-listening-review`, `echoes-ui-accessibility-playtest`, `echoes-evidence-gate-review`, and `echoes-human-acceptance-session`.
