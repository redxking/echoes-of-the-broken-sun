# Unreal Editor integration research: a smaller action surface for AI-assisted production

**Author and owner:** Angelis Pseftis
**Research access date:** 2026-09-05
**Status:** Codex server registration and external-cache launcher installed; editor build and live MCP protocol checks passed. Refreshing MCP tools in the already-running desktop client remains a user step. No performance experiment executed.
**Authority:** research note under [AGENTS.md](../AGENTS.md) and the [document authority map](README.md). Game requirements and acceptance remain in the master/state records.

## Decision

The practical way to reduce time spent on one-off Unreal Python scripts is to give the AI a persistent connection to the open Editor and a deliberately small collection of typed, project-specific actions. Each action should describe one repeatable outcome, accept structured inputs, use a tested undo transaction for supported edits or an explicit recovery procedure for other changes, return a compact readback, and emit enough evidence to decide whether the next action is needed. The AI should then compose those actions and inspect results instead of recreating long scripts for familiar operations.

For Echoes, first connect and evaluate the **existing Epic Unreal MCP integration**. The local installation and project settings already contain the required foundation; a replacement plugin or engine upgrade is not the first step. The connection gap below is verified. Whether fixing it reduces total production time still needs measurement.

This is preferable to unrestricted `execute_python` access as the normal workflow. Python remains valuable for an unusual bulk operation, asset migration, or deterministic generator, but its code should become a reviewed, versioned batch action after the first successful use rather than being regenerated every session.

## Initial audit before setup

Inspected the dirty working tree and host configuration on 2026-09-05 before the authorized setup below. These initial observations explain the connection gap; later setup results supersede them where stated.

| Observation | Evidence and implication |
|---|---|
| UE 5.8.2 is installed, changelist 56702186. | Read `/Users/Shared/Epic Games/UE_5.8/Engine/Build/Build.version`. The installed `ModelContextProtocol.uplugin` and `ToolsetRegistry.uplugin` exist and identify Experimental plugins. |
| Echoes already enables MCP and All Toolsets for the Editor. | [Project descriptor](../EchoesOfTheBrokenSun.uproject) associates UE 5.8 and enables `ModelContextProtocol`, `AllToolsets`, and `PythonScriptPlugin`. |
| A project MCP endpoint is already declared. | [Project MCP config](../.mcp.json) points `unreal-mcp` to `http://127.0.0.1:8000/mcp`. This JSON file alone is not a verified Codex connection. |
| Shared automatic startup is deliberately disabled. | [Editor settings](../Config/DefaultEditorPerProjectUserSettings.ini) set `bAutoStartServer=False`, port 8000, `/mcp`, and tool search. The adjacent comment preserves machine-local startup so cook/commandlet sessions do not compete for the live editor port. Keep this safeguard. |
| Codex has no explicit Unreal server entry in the inspected config. | Read only server metadata from `/Users/angelispseftis/.codex/config.toml`; workspace and `Project/.codex/config.toml` are absent. This task's available tools include no Unreal-specific interface. Docker MCP config search found no Unreal entry; this is not an exhaustive audit of every possible external client. |
| The configured endpoint was unavailable at the observation time. | `lsof -nP -iTCP:8000 -sTCP:LISTEN` returned no listener. A TCP connection to `127.0.0.1:8000` at 2026-09-05 14:40:38 UTC was refused. No MCP initialization or editor tool call was performed. |
| Reusable work exists, but much of it is reached through command-line scripts. | [Art wrapper](../Scripts/generate_art_assets.sh) launches `UnrealEditor-Cmd` with `-ExecutePythonScript`, selected by `ECHOES_*_ONLY` flags. [Art generator](../Scripts/generate_art_assets.py) contains reusable recipes. No project custom MCP plugin/action catalog was found in the inspected paths. |
| Some fresh-process steps are intentional. | [Art purge code](../Scripts/purge_stale_art_masters.py) and [audio generator](../Scripts/generate_audio_assets.py) document package/object lifetime problems when deleting and recreating assets in one editor session. Preserve those recovery boundaries instead of moving every job into the live Editor. |

The working diagnosis is a missing active Codex-to-editor connection plus insufficient reuse of project actions. This explains a plausible source of scripting overhead; it does not quantify how much time is spent on code generation versus engine work.

## Connection and workflow proposal

1. Coordinate access to the intended interactive Echoes Editor. Preserve the shared auto-start setting. Start its MCP server explicitly with `ModelContextProtocol.StartServer 8000`; confirm with local process inspection that the listener PID/executable and project command line belong to the intended UnrealEditor before connecting, then read back its project/map identity. Keep the unauthenticated server enabled only during coordinated use. For subsequent launches, use the interactive launcher described below.
2. Add the local Streamable HTTP server in the desktop client's MCP settings, or the appropriate trusted Codex configuration, and reconnect. The relevant TOML entry is shown below. Preserve other server settings. Codex uses `config.toml`; the existing project `.mcp.json` is not sufficient evidence that it has been registered. [OpenAI MCP configuration](https://learn.chatgpt.com/docs/extend/mcp?surface=cli).
3. Confirm discovery and an actual read-only selection/actor/property query. Read back project/map identity; connection health alone is insufficient. Inspect which built-in tools are available before designing custom wrappers.
4. For repetitive operations, expose narrowly scoped reusable actions through the existing Toolset Registry. Proposed actions include `inspect_selection`, `apply_review_profile`, `validate_selected_assets`, and `capture_review_evidence`. These names describe proposed project tools, not tools already implemented.
5. Give each action explicit scope, stable object identifiers, parameter validation, completion/error reporting, and a compact result. Use job status for long operations and one editor mutation queue. Batch related work inside a bounded tool; avoid hundreds of fine-grained client round trips. A timed-out write needs state reconciliation before retry to prevent duplicate changes.
6. Route routine editor actions to discovered tools, repeatable asset work to registered recipes, and genuinely new behavior to source-code work. Keep visual inspection and physical-input tests where each provides necessary evidence. Reuse working scripts; create a new one only when no existing operation fits.

```toml
[mcp_servers.unreal-mcp]
url = "http://127.0.0.1:8000/mcp"
```

The local desktop/CLI route matches this Mac-hosted Editor. A hosted web chat does not read the Mac's Codex configuration. Adding a local URL there would not establish this connection. [OpenAI MCP client distinctions](https://learn.chatgpt.com/docs/extend/mcp?surface=cli).

## Installed setup and external storage

On 2026-09-05, the authorized setup added `unreal-mcp` to the existing user Codex configuration with `codex mcp add unreal-mcp --url http://127.0.0.1:8000/mcp`. `codex mcp get unreal-mcp` confirms an enabled Streamable HTTP entry; existing MCP server entries were preserved. This registration alone does not prove that an already-running desktop task has refreshed its tools. If they are absent, use Codex Settings → MCP → Restart after the editor server is ready. Computer-use access to the Codex application is unavailable in this environment, so that desktop control cannot be clicked by this task.

Use [open_editor.command](../Scripts/open_editor.command) for subsequent interactive Echoes sessions. It starts the official installed Editor with the external `.uproject`, `-ModelContextProtocolStartServer`, `-ModelContextProtocolPort=8000`, and a supported `-LocalDataCachePath` override. It rejects a missing engine/module and an already-running Editor or port 8000 listener. The wrapper contains no asset-generation code and takes no commandlet arguments. Reuse an existing coordinated session instead of launching a duplicate.

| Storage | Location and behavior |
|---|---|
| Authoritative project, assets, source, configuration, compiled modules, intermediate files, saves and evidence | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project` remains the working project. No copy to the internal drive is required. |
| New writable DDC and Zen cache for launcher sessions | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/LocalCache` and its `Zen` child. The actual editor startup log confirms both paths and a healthy Zen service. First use compiles missing shaders. |
| Installed engine and toolchain | Existing `/Users/Shared/Epic Games/UE_5.8` and Xcode remain installed. Unreal may read its installed engine DDC pak. |
| Small host integration/runtime files | Existing Codex settings, Unreal user settings/logs, and Zen executable installation remain on the internal drive. Existing older cache data was preserved; no disk-space reclamation is claimed. |

Keep the archive drive mounted at its existing path for the full editor/build session. Opening `.uproject` directly through Epic/Finder does not inherit the launcher's cache flag or MCP startup flag. Use the launcher to retain these choices. Do not set global DDC preferences or alter `[Zen.AutoLaunch] DataPath` to force this: installed source resolves `UEditorSettings` through a project-agnostic user config, and changing Zen's default path may trigger migration cleanup. The command-line override gives Echoes its external cache without that migration. Build/test wrappers retain their existing isolation and cache policies.

The missing game editor module required two narrow repairs before launch: synchronize the network build identity and test fixture with the existing snapshot schema 27, and qualify the non-static collapsed-Well check through the current simulation instance. The existing content preflight and `Scripts/build_editor.sh` then passed. A read-only internal review independently checked the identity digest and null-safe selection repair. No simulation rules were changed by this setup task; other active game-development work remains separately owned.

The shared [MCP inspection skill](../.opencode/skills/echoes-unreal-mcp-editor-inspection/SKILL.md) now routes routine editor work through live tool discovery and existing typed tools before creating scripts. It requires readback, source-generator consistency, and explicit recovery for permitted mutations. Programmatic batching is not evidence of atomicity or automatic undo.

Live protocol verification completed on 2026-09-05 against editor PID 94857, with the listener bound to `127.0.0.1:8000`. Initialization negotiated MCP `2025-11-25`; discovery exposed the three meta tools and 55 toolsets. Native calls returned `/Engine/Maps/Entry`, an empty actor selection, the viewport camera transform, and `true` for the existing external project asset `/Game/Audio/Generated/AMB_FutureWell`. Viewport and whole-editor PNG captures were decoded and visually inspected. The Entry scene is mostly empty; these images prove capture operation, not game visual quality. Camera readback remained unchanged. No gameplay, asset, or source mutation was performed by these tool checks.

**Observed capture compatibility issue.** In this installed UE 5.8.2 build, calling `CaptureViewport` with only `bShowUI` failed because omitted optional parameters needed defaults. A schema-conforming retry succeeded with the current `GetCameraTransform` result passed as `captureTransform`, and explicit `annotations` values: `gridSpacing=0`, `gridExtent=0`, `gridHeight=0`, `maxLabelDistance=0`, `classFilter={"refPath":"/Script/Engine.Actor"}`, `maxLabels=0`. `CaptureEditorImage` succeeded with empty arguments. Retain live discovery and this observed workaround until an engine update is checked; do not patch the installed engine or assume every optional field works identically.

Setup evidence is retained under `BuildArtifacts/Evidence/unreal-mcp-setup-20260905T144942Z`. These results concern local configuration, compilation, storage routing, and editor connectivity. They do not establish gameplay acceptance, packaged-release readiness, or measured productivity gains.

## What Epic provides now

Epic's Experimental MCP server offers editor tools, custom Python/C++ toolsets, and Codex configuration generation. It supports selective toolsets and on-demand discovery through `list_toolsets`, `describe_toolset`, and `call_tool`. Calls execute serially on the game thread; keep one editor mutation queue. Its localhost server has no authentication and must remain local. APIs are evolving. [Epic Unreal MCP](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor).

Use built-in tools first. Add a reviewed project action only for a repeated gap: for example, a named review capture or validation of selected generated assets. Prefer typed parameters and structured results. Refresh tool discovery after changes. This is an engineering recommendation; some actions still require Python internally, and MCP alone does not remove compile, import, shader, or rendering time.

Epic's older Remote Control API is useful, but it solves a different problem. It offers a REST-like HTTP and WebSocket control surface, can expose Blueprint/Python-accessible functions and properties, and supports Remote Control Presets without code. A preset can publish property-change events over WebSocket. It is Beta and its quick-start warns not to expose the server to the public Internet; start locally and use a protected LAN or VPN only when a remote workflow is justified. [Remote Control overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-control-for-unreal-engine), [WebSocket reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-control-api-websocket-reference-for-unreal-engine), and [security guidance](https://dev.epicgames.com/documentation/unreal-engine/remote-control-quick-start-for-unreal-engine) (accessed 2026-09-05).

Use Remote Control only for a small, explicit control panel or feedback feed: for example, a selected review camera, named lighting values, a capture trigger, and an approved presentation profile. Enable `generateTransaction` for Remote Control changes so the Editor records an undoable change; Epic documents undo history and Multi-User replication for those transactions. Do not use broad object paths, remote console execution, or experimental routes as a substitute for project authority and review. [Remote Control HTTP reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-control-api-http-reference-for-unreal-engine) (accessed 2026-09-05).

Editor Utility Widgets and Editor Utility Blueprints provide the complementary human-facing layer. Epic recommends a dockable Editor Utility Widget for most editor scripting work, and supports startup objects plus the Editor Utility Subsystem for registering panels. These are appropriate for a human-reviewed batch panel with named actions, selected-scope previews, progress, cancel, dry-run, and evidence export. They are editor-only. [Epic Editor scripting with Blueprints](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-blueprints) (accessed 2026-09-05).

Python itself is editor-only and cannot supply runtime gameplay behavior. It reflects Editor APIs exposed to Blueprints and supports reusable scripts under project content paths, but a public execution scope can redefine variables and functions, which Epic notes can cause unexpected behavior. Treat generated Python as privileged operational code: use a constrained module path, explicit asset allowlists, a dry-run/readback mode, a transaction, and an output receipt. [Epic Python Editor scripting](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python) (accessed 2026-09-05).

## Alternatives and current community evidence

Community MCP implementations illustrate useful design patterns but do not establish an equivalent compatibility or security baseline.

| Option | What primary source establishes | Fit and limitation |
|---|---|---|
| Epic Unreal MCP | Official local MCP, custom Toolset Registry extension, tool search, Codex configuration support. Experimental and evolving. | Preferred pilot baseline if the installed engine supports it. |
| Epic Remote Control + Editor Utility Widget | Official HTTP/WebSocket/preset control plus a dockable human UI. Both Remote Control and some routes are Beta/experimental. | Strong for a few curated review/batch controls; not general AI authoring. |
| [Claireon](https://github.com/believer-oss/claireon) | MIT source; read/edit asset tools, PIE, trace analysis, Python execution, asset locks, and tool discovery. Its README says tested UE 5.5+ but Windows-only testing; other platforms are untested. | Useful reference for narrow discovery and per-asset locking. It is not a justified Mac adoption without a compatibility build and security review. |
| [UEMCP](https://github.com/atomantic/UEMCP) | MIT source; Node MCP server plus Python Editor plugin with dedicated wrappers. README asserts up to 85% less generated code, but that is a repository assertion, not an independent benchmark. | A potentially useful architecture reference; adds Node, a listener, and setup-script supply-chain surface. Do not install from this research. |
| [Autonomix](https://github.com/PRQELT/Autonomix) | MIT source; broad in-editor agent/tool surface, viewport capture, PIE automation, audit/backup/approval concepts. README lists UE 5.3–5.5 testing and Windows Visual Studio prerequisites. | Useful for risk tiers, transaction wrapping, checkpoints, and visual feedback. Mac and current-project compatibility are not evidenced here. |

No reviewed community repository should be treated as production-ready from README claims alone. Before any adoption: pin an exact commit, inspect license and dependency manifests, build against the selected UE/macOS version in a disposable project, test localhost binding and access controls, review filesystem/process/network tools, verify undo/cancel/error behavior, and retain results. Apply the existing project ownership and authorization rules to changes; a new integration does not expand them.

## Research evidence and its limits

Cutscene Agent is particularly relevant to editor efficiency: it describes bidirectional MCP tools that let agents operate the engine and observe scene state, with visual feedback and editable engine-native cinematics. That supports investigating a tool-plus-observation loop. Its evaluation concerns cutscene generation, not production RTS development or measured savings on this Mac. [Cutscene Agent, April 2026 preprint](https://arxiv.org/abs/2604.25318).

The research supports a closed loop—structured tools, readback, and test/visual feedback—more strongly than it supports autonomous end-to-end game production. AutoUE reports an Unreal multi-agent prototype that decomposes scene, gameplay, and test-generation work and uses retrieved Unreal documentation to reduce tool-use hallucinations. It is a research demonstration on generated games, not evidence that its approach improves a live RTS codebase. [AutoUE paper](https://arxiv.org/abs/2603.07106) (accessed 2026-09-05).

More caution is warranted for native code. GameEngineBench evaluates 110 scoped UE5 C++ tasks across nine real projects; its authors report a best pass@1 of 55.5% and 31 tasks unsolved by every evaluated configuration. The result does not measure this project or the official MCP integration, but it is concrete evidence against treating an agent's first C++ edit as sufficient verification. [GameEngineBench](https://arxiv.org/abs/2607.03525) (accessed 2026-09-05).

No source reviewed here provides a controlled comparison showing that a particular Unreal MCP plugin improves game quality, throughput, or reliability on macOS. The appropriate claim is narrower: typed, reusable actions can remove repeated prompt-to-script translation, while transaction, readback, and evidence gates make their effects inspectable.

## Recommended pilot

**Scope.** Select one reversible M01 presentation workflow: inspect the currently selected actors, apply an approved temporary lighting or camera review profile on a disposable test level, capture a viewport image and structured actor/property readback, then restore or undo. It must not modify simulation, narrative source, requirements, runtime code, package settings, or production assets.

**Implementation sequence.** Reuse the installed UE 5.8.2 and enabled Epic plugins in a disposable test project/level. Establish the connection using the procedure above, then test existing tools before writing custom ones. For any missing repeated action, add a small project-owned wrapper. Mutations validate scope and use a tested transaction or explicit restore path; verify the resulting values. Capture observes the scene and writes only to the designated evidence location. If a task requires the existing purge/reimport lifecycle, retain that isolated process rather than forcing an in-session substitute.

**Measures.** Compare five paired repetitions of one bounded task: one present-method run and one bridge run per pair. Restore the same initial state for each run, alternate method order, and record cold-start versus warm-cache conditions separately. Hold the task, agent/model, engine, schemas, configuration and machine constant. Record elapsed request-to-verified-evidence time, script lines newly generated, failures, corrective iterations, restoration success, and intended/actual affected assets. Also record initial connection/wrapper implementation time and maintenance effort. For positive measured per-task savings, estimate break-even repetitions as setup time divided by savings per task; label that estimate and its workload assumptions. This small local pilot cannot establish general reliability or statistical superiority.

**Exit gate.** Predeclare the minimum useful time saving for the intended workload before testing. Advance only if all runs preserve scope and recover correctly, the bridge introduces no additional failures or corrective work, visual/readback quality remains equivalent, and savings meet that threshold with an acceptable estimated break-even. Faster calls alone are insufficient when total setup or rework dominates. Otherwise retain the reviewed-script process and revise the candidate action contract from the evidence. These are proposed pilot criteria, not added game release requirements.
