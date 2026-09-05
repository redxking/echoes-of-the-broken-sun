---
name: echoes-unreal-mcp-editor-inspection
description: Connect to Echoes Unreal MCP and reuse native tools for assets, actors, components, Sequencer, and editor state before writing scripts; keep editor inspection separate from physical-input or human-play evidence.
metadata:
  author: Angelis Pseftis
---

# Echoes Unreal MCP editor inspection

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, [echoes-session-control](../echoes-session-control/SKILL.md), and [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md). Do not launch the Editor, run automation, or mutate assets unless a task-owner-confirmed heavy resource reservation and a current task ownership cover the exact action/path.

Use this skill only for editor-internal observability: asset/component bindings, world placement, Sequencer state, presentation adapters, logs, and controlled preview. MCP/editor automation cannot prove OS-level mouse/keyboard delivery, focus behavior, menu operability, controller behavior, or human-style gameplay. Route those claims to `echoes-mouse-keyboard-playtest`.

Acceptance: recorded build/map/asset context, exact inspected property/event, editor evidence class, observed result, and untested boundary. Route visual/audio/accessibility/evidence respectively to `echoes-realtime-visual-review`, `echoes-audio-listening-review`, `echoes-ui-accessibility-playtest`, and `echoes-evidence-gate-review`. Stop for held heavy-resource reservation, missing task ownership, stale build identity, unavailable editor connection, or a request that crosses presentation into simulation mutation.

## Connect, discover, act, verify

1. Reuse the coordinated interactive editor. When opening one is authorized and none is running, use [open_editor.command](../../../Scripts/open_editor.command). It opens the external project, keeps new DDC/Zen cache beside it under `LocalCache`, and starts MCP for this interactive process only. Keep the drive mounted. Leave shared `bAutoStartServer=False`; commandlets must not compete for port 8000. See [integration setup and storage](../../../Docs/UnrealEditorIntegrationResearch.md) for connection recovery and limitations.
2. Confirm the listener belongs to the intended editor and binds locally. Query the current level and project context before edits. Use the registered `unreal-mcp` server at `http://127.0.0.1:8000/mcp`. Configuration presence, a running process, and a successful tool call are separate checks. If the current client lacks the registered tools, reconnect its MCP configuration; do not silently fall back to generating long scripts for routine inspection.
3. With tool search enabled, call `list_toolsets`, then `describe_toolset` only for the relevant toolset. Use the returned schema with `call_tool`. Prefer existing scene/actor/asset/property/camera/capture tools; do not assume remembered names or argument casing still match. Refresh discovery after plugin or engine changes.
4. Perform one bounded action, then inspect its result. Reuse reviewed project generators for repeatable asset work. Write a new script or project tool only for a demonstrated gap, and retain it for reuse. Preserve existing fresh-process purge/reimport workflows where object lifetime requires them. For repeated bulk work, batch within explicit scope and report partial failure; batching does not imply atomicity or automatic undo.
5. Before a permitted edit, identify exact objects, expected properties, affected files, and a tested undo or restore path. Do not retry a timed-out write until state has been reconciled. Preserve deterministic source-of-truth generators; editor tweaks alone do not update their source. Retain concise property/asset readback and visual evidence when relevant, plus failures and untested boundaries. Successful editor inspection does not establish game or release acceptance.
