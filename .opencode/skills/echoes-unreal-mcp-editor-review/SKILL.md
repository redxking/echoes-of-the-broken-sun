---
name: echoes-unreal-mcp-editor-review
description: Route Echoes Unreal Editor and MCP inspection separately from real OS-level mouse-and-keyboard play, with evidence classes and mutation authority kept explicit.
metadata:
  author: Angelis Pseftis
---

# Echoes Unreal MCP and editor review

Read `CLAUDE.md`, the live directive, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/TechnicalArchitecture.md`, the relevant requirement ledger, and `../WorkstreamControl/ACTIVE_LANES.md` before invoking the editor. This is primarily a review/evidence skill; do not mutate project assets or source unless an ACTIVE lane explicitly owns each target path.

Use the Mac for Unreal, packaging, and editor automation. This is an umbrella/router, not proof of player input: Unreal MCP/editor automation cannot prove OS-level mouse/keyboard delivery or human-style play. Route editor-internal inspection to `echoes-unreal-mcp-editor-inspection`, and real device-driven play to `echoes-mouse-keyboard-playtest`.

Treat evidence classes separately: source inspection, automated test, editor preview, rendered capture, and human-style gameplay are not interchangeable. Screenshots may show a frame but cannot prove input, flow, timing, sound, recovery, or playability. Capture exact map/mode, settings, hardware, sequence, observed outcomes, failures, and abort conditions. Respect the heavy-run lock and existing lane rules.

Acceptance output: reproducible review protocol, retained observed evidence, defect list or bounded observed result, and explicit untested conditions. Exclude release/readiness claims, independent-validation claims, and hidden state inference. Stop if the editor/build cannot start, the volume/memory/storage preconditions fail, input control is unavailable, the heavy lock is held, or any action would exceed lane authority.

Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Always read `../WorkstreamControl/HEAVY_RUN_LOCK.md`; acquire a coordinator-issued lease or stop before Editor, packaged runtime, GPU-heavy review, build, or automation. Route visual/audio/accessibility/evidence/owner-signoff respectively to `echoes-realtime-visual-review`, `echoes-audio-listening-review`, `echoes-ui-accessibility-playtest`, `echoes-evidence-gate-review`, and `echoes-human-acceptance-session`.
