---
name: echoes-unreal-mcp-editor-inspection
description: Inspect Echoes editor-internal assets, actors, components, Sequencer, and runtime state through Unreal MCP or editor automation without treating that inspection as OS-level input or human play evidence.
metadata:
  author: Angelis Pseftis
---

# Echoes Unreal MCP editor inspection

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, applicable current `Docs/` directives/ledgers, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`. Do not launch the Editor, run automation, or mutate assets unless a coordinator-issued heavy lease and a live owned lane cover the exact action/path.

Use this skill only for editor-internal observability: asset/component bindings, world placement, Sequencer state, presentation adapters, logs, and controlled preview. MCP/editor automation cannot prove OS-level mouse/keyboard delivery, focus behavior, menu operability, controller behavior, or human-style gameplay. Route those claims to `echoes-mouse-keyboard-playtest`.

Acceptance: recorded build/map/asset context, exact inspected property/event, editor evidence class, observed result, and untested boundary. Route visual/audio/accessibility/evidence respectively to `echoes-realtime-visual-review`, `echoes-audio-listening-review`, `echoes-ui-accessibility-playtest`, and `echoes-evidence-gate-review`. Stop for held heavy lock, missing lane, stale build identity, unavailable editor connection, or a request that crosses presentation into simulation mutation.
