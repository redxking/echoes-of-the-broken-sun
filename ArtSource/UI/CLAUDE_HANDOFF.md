# Continue the Echoes HUD and in-game menus

**Author and owner:** Angelis Pseftis  
**Handoff date:** 2026-09-09  
**Status:** Work in progress. Revised source is not compiled, integrated, or visually accepted.

You are continuing an existing implementation, not starting a redesign. The owner's request is:
“Turn the HUD to look like our concept. All in-game menus. Do not stop until complete. I want all HUD work completed.”

Finish the real Unreal HUD and all menu routes against the approved concept. Preserve simulation authority, existing gameplay, concurrent changes, and evidence. Do not call source, tests, or headless imports a rendered or physical-input pass. Do not resume the earlier Riftstalker/art-model task.

## Locations and immediate ownership boundary

Workspace root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun`

- Shared main checkout: `Project/`.
- HUD working checkout: `Worktrees/hud-concept-completion/`.
- HUD branch: `ui/concept-console-completion`, based on `6b559889a4e7b0ed7fd86aee37b64728bd516017`.
- HUD evidence: `BuildArtifacts/Evidence/hud-concept-completion-20260909/`, relative to workspace root, outside Project.
- This authoritative handoff is `ArtSource/UI/CLAUDE_HANDOFF.md` in the HUD checkout. Update it in place if necessary; do not create competing handoff versions.

**The HUD checkout contains preexisting dirty copies of other lanes' work. Its entire git diff is NOT the HUD patch. Never copy whole runtime files into main, stage all changes, reset, clean, or overwrite another agent's work. No HUD commits have been created.**

Coordination caution: Environment subsequently requested another bounded height-only v7 build/native/render window. No grant or incoming Claude ownership acknowledgment has been received by this stopped HUD session. Recheck live ownership; the earlier release below is historical, not proof that the slot remains available.

Prior coordination update: **Environment explicitly released main/source/build/editor resources**, reporting no remaining editor/compiler/build process. This Codex HUD session remains stopped and has not claimed the slot. Incoming Claude/HUD must explicitly claim ownership after checking for any intervening owner, before integration, build, Unreal launch, PIE, hotload, or GUI interaction. This document does not itself claim the lease.

Environment integrated servicev5, cliff validation, protected evacuation generation, world provenance test, and bankfootv4. Its reported build/Tools20/World99/material readback and ten bindings/native map3 results are recorded in workspace `BuildArtifacts/Evidence/m01-environment-completion-20260909/integrated-environment-candidate.json` and `environment-integration-review.json`. Preserve that new60-file identity when merging HUD. No HUD source/assets were changed by Environment. Nine protected stills and normal M01/keyboard pan were observed, but pointer/modifier/zoom were unreliable; the480-second timeout and cleanup are retained. V042 and full physical route acceptance remain open. Verify retained evidence rather than treating this coordination report as HUD qualification.

Relevant Codex task IDs, if available through the app:

- HUD/current task: `01a0772f-7d98-7cd2-8d56-2254d585cf17`.
- Backend Actions: `01a0864a-3a9d-7e13-9c8b-1f20767f2514`.
- Environment: `01a0864b-7819-7693-a995-3d654f05173f`.
- Backend recovery/abilities: `01a07796-6bc8-7b13-b1de-f6d05fa67231`.
- Finite resource depletion: `01a086c9-824b-7810-9197-59fb1b7613bc`.

Backend's latest candidate5 handoff is `Project/BuildArtifacts/Evidence/backend-actions-20260909/receipt.json`, with `source-manifest.json` and `backend-actions-source.patch` adjacent. Read the actual files and coordinate with Environment/current owner. Backend made no HUD edits. Its new protected rendered-run helper is `run_protected_player_route.py`; locate and read it before use. Backend reports historical/current two-process routes with normal exits and archival cleanup, but positive construction/cancellation physical pointer interaction remains unqualified. It also observed fresh skirmish setup after returning to menu apparently reuse existing state; explicit Restart reset correctly. Investigate that shell route independently.

## Authority and visual targets

First read `Project/AGENTS.md`, `Project/Docs/README.md`, applicable skill routing and game-development workflow. Then inspect current branch, dirty paths, recent commits, processes, and ownership. Normative requirements are `Docs/Requirements.md`; lifecycle/evidence/acceptance lives in `Docs/RequirementsState.md`. Consult `DeliveryPlan.md`, `VisualTargetSpecification.md`, and relevant UI skills. Keep author/creator metadata solely Angelis Pseftis.

Original concepts in Project:

- `site/assets/concepts/echoes-hud-traditional-console.jpg`
- `site/assets/concepts/echoes-main-menu-bridge.jpg`

SPEC-UI-007 and the September 8 owner direction in VisualTargetSpecification govern the bottom console: minimap left, selected-unit information center, commands right, compact objectives and global telemetry, substantial terrain visibility. Charcoal panels, cyan framing, amber focus; high contrast and scale remain functional. Do not invent the concept's incidental Atlas Mech, shields, resources, or capabilities.

Key contracts: SPEC-UI-006/007; SPEC-HUD-001..007; REL-UI-019/020/026/028; REL-ECO-015..017; DEMO-INP-001/010/013/014/015; SPEC-PRD-002. Every displayed action must work with mouse and keyboard. No keyboard-only workaround for broken clicks.

## What is actually verified

Candidate1 was integrated in main under an earlier exclusive window:

- Native vector command glyphs, concept-styled shared shell and tactical console.
- Actual per-selection health values/fills, semantic action labels and hotkeys.
- Derived text-free Command Bridge plate behind real title buttons.
- Editor build passed; texture import saved successfully.
- Focused second run: **11 passed, 0 failed**, protected automation exit0.
- Initial first run had a detached Slate auto-wrap fixture failure; a second layout/paint pass corrected it without weakening pointer assertions.

Evidence: `build-01.log`, `build-02.log`, `import-01/`, `focused-01/`, `focused-02/`, `identity-02.json`, `verification.json`, `session.md`, `rendered-01/` under the HUD evidence root.

The first actual rendered run showed title, keyboard Options, 80/100/150% scale, high contrast, tutorial briefing and unselected battlefield. It exposed oversized panels and undersized console text. **Pointer clicks/drags did not establish activation** on Options, slider, or tutorial Anchor. Cause remains unproven. No selection/command/pause/save/replay/results/multiplayer physical pass is claimed.

Rendered game exited normally0; the old wrapper returned8 because normal settings/tutorial use created scoped save files before cleanup. Protected paths and cleanup passed. This was not a crash or an empty-scope automation pass. Screenshots from that run are retained in the Codex conversation, not claimed as local image files.

Candidate1 uses a static derived plate, **not the specified real-time 3D Command Bridge scene**. Source is `ArtSource/UI/command-bridge-background.png`, provenance in `provenance.json`; imported main texture is `Content/Art/UI/T_EBS_CommandBridge.uasset`, referenced at `/Game/Art/UI/T_EBS_CommandBridge.T_EBS_CommandBridge`. Preserve originals.

## Revised source now staged in the HUD checkout

Everything below is **NOT_BUILT / tests NOT_RUN / rendered NOT_RUN** unless a later retained result establishes otherwise.

1. **Console:** smaller bottom band (`min(252*scale, height*.38)`), larger minimap, larger text/glyphs, compact upper resource strip with three values and source-backed faction/match context, transparent normal console backing (0.18), opaque high contrast. Shared HudLayout owns pointer bounds, tutorial skip clearance, and scale handling.
2. **Battlefield Menu:** real focusable semantic `OpenPauseMenu`, existing TogglePauseMenu authority, immediate show/hide, no replay/modal leakage. Online menu remains local and does not pause simulation.
3. **Shared menus:** deliberate two-line title, compact title/briefing heights, grouped Options and paired controls, scale endpoint tolerance, Home/End focus navigation, stable history/resource reading widgets and scroll position.
4. **Controls:** live action/axis enumeration, grouped binding rows, key/mouse/wheel capture, modifier handling, collision rejection before mutation, cancellation, confirmed default reset, current command-tile/Help prompts. Stable binding identity rejects stale edits. Default config mapping is read from authored layers, not saved overrides. Packaged fallback uses the Input branch's CombinedStaticLayers only.
5. **Persistence correction:** DefaultInput.ini explicitly allows `/Script/Engine.InputSettings` under `[SectionsToSave]`. UE5.8 otherwise excludes these mappings from writes. Persistence tests now inspect exact independently loaded serialized mappings, not merely any changed bytes; rejected/cancelled paths remain byte-identical. The test refuses writes outside protected `-UserDir`. Actual restart qualification is still required.
6. **Input routing:** context-safe shared Tab (faction cycle only title/briefing; subgroup/owned selection only gameplay); Shift+Tab reverse, Backspace retained; campaign map explicit F1 because raw M stole BuildUtility. Replay allowed bindings now read live input settings. Preserve concurrent backend controller changes during merge.
7. **Pointer contract:** Shell ActivateButtonUnderLocation now accepts only absolute Slate screen coordinates; removed local-coordinate reinterpretation and unrelated current-cursor fallback. Controller fallback passes Slate coordinates. This corrects ambiguity; it does not prove the earlier pointer failure is resolved. Added opt-in semantic activation trace.
8. **Command history:** recipient-scoped backend events, filters before body, queued/applied/no-effect distinct, actual carried costs/refunds/progress, explicit history loss, stable updates. No invented source location or recovery button. Online nested Options/Controls/History preserve continuity warnings.
9. **Maintenance deck:** worker Repair action, cursor-targeted through qualified RepairAtCursor; conditional cancellation for one owned unfinished structure; matching glyphs/prompts/tests. Label is **REPAIR**, not “repair/assist”: RepairAtCursor requests Repair; existing RMB context handles construction assistance. Controls calls legacy RestartScenario binding “Repair target / restart after results”.
10. **Selection identity:** content catalog's existing faction/entity binding tables promoted to shared FindUnit/FindBuilding overloads. All24 roster names and authored roles reach live/replay/network selection. Meridian Lancer and all Kharuun/Choir names no longer fall back to generic type. Existing dynamic purpose text preserved.
11. **Resource monitor:** typed PlayerView model, restricted network keyframe overload, new shell and Pause entry. Liquid funds, worker phases/assignments, paid active investment versus uninvested waiting requests, active reserved Logistics, connected Relay capacity, and owned timers. Unavailable income/deposit/route/other fields explicitly stay unavailable. No client-side global Simulation lookup. Preserve next opportunity is activation+interval in exposed tick coordinates; added actual simulation-step credit boundary test. Payout is conditional on uncontested ownership. Resource-strip focus/click affordance was the last worker edit; inspect completion/status and tests before relying on it.

Main source areas (under `Source/EchoesOfTheBrokenSun/`):

- Existing: FieldHudView/Widget, ShellWidget, PlayerShell, PlayerFieldHud, PlayerController, PlayerFlow, HudLayout, CommandDeckModel, ContentSubsystem, HudGlyph, and their relevant tests.
- New: InputBindingModel, InputPrompt, PlayerControls, FeedbackHistoryModel, PlayerFeedbackHistory, ResourceMonitorModel, PlayerResourceMonitor.
- New tests: BattlefieldMenu, ControlsShell/Persistence, InputBindingModel/InputPrompt, FeedbackHistoryModel/Shell, OnlineLocalMenuRoute, TabContextDispatch, ResourceMonitorModel/Shell (and any final resource-affordance test).
- Also `Config/DefaultInput.ini`, `ArtSource/UI/README.md` and this handoff.

Do not mistake preexisting dirty checkpoint, sim-core, network, power-network, placement, narrative, or environment files for HUD-owned changes.

## Integration hazards and retained baselines

`revision2-source.patch` and `revision2-source-manifest.json` are **OUTDATED**. They predate maintenance, selection catalog, resource monitor, and final pointer/persistence changes. Regenerate an explicit ownership manifest and context deltas; do not blindly apply them.

Never rerun initial `prepare_handoff.py --apply`: it belongs to candidate1 and can overwrite newer backend work.

Useful HUD evidence baselines:

- `polish-baseline/`, `console-polish-baseline/`
- `copy-baseline/`, `copy-edits.json`
- `prompt-baseline/`, `feedback-history-shell-baseline/`
- `input-dispatch-source-baselines/` (exact/reconstructed pre-replay and pre-Tab/F1 controller/config)
- `online-local-menu-route/`, `battlefield-menu-affordance/`
- `maintenance-deck/` (test baseline reconstructed by removing only maintenance assertions; existing Bulwark tests retained)
- `pointer-coordinate-contract/`, `persistence-save-policy/`
- `resource-monitor-model/`, `resource-monitor-shell/`, `resource-monitor-affordance/`
- `rebased-baseline/ArtSource/UI/README.md` (use this, not older README baseline)
- Selection baselines are currently inside the HUD checkout at `BuildArtifacts/evidence/selection-catalog/pre-edit/`; note lowercase `evidence` and different root.

Last read-only patch check before the later edits failed only Controller.h context because main has newer FormationLayout/GameplayFeedbackPacket includes. `header-merge-review/EchoesPlayerController.h` is a proposed three-way merge that was clean at its captured identity, **not applied and now stale**. Recompute against fresh main. Preserve all backend fields/functions/includes.

FieldHudView's “THE AUTHORITY CONTINUES” → “THE MATCH CONTINUES” copy correction already exists in main candidate1. Do not duplicate or reverse it while generating deltas.

The isolated checkout intentionally lacks some newer backend definitions: maintenance implementation/declarations and gameplay-feedback controller plumbing. Thus it cannot qualify these new consumers by compiling old isolated source alone. Merge with qualified current-main contracts under ownership, or construct a separately coordinated integrated candidate. Never replace main's controller/header with this older whole file.

## Outstanding dependencies and known gaps

- Backend API: `GetGameplayFeedback()` and `IsGameplayFeedbackRecoveryPending()` unchanged; verify current headers. Events are recipient-scoped, transient, loss-aware, and do not imply command completion.
- Qualified main maintenance file: `Private/EchoesPlayerMaintenance.cpp`; methods RepairAtCursor, TryIssueWorkerMaintenanceContext, IssueSelectedWorkerMaintenance, CancelSelectedConstruction, and RepairOrRestartPressed. Preserve backend's actual bindings.
- Exact 30/60-second Matter/Dawn credit telemetry is **not implemented yet**. Backend queued it separately. Actual Matter delivery receipts exist; Harvest/Preserve Dawn credit receipts need instrumentation. Never infer income from liquid balance changes.
- Finite-resource lane's isolated patch passed seven native debug cases but is not main/Unreal qualified. Patch: workspace `BuildArtifacts/Evidence/resource-depletion-20260909/resource-depletion.patch`, SHA256 `fd8d08695f6ffcb600e73b2babe886cfb95ec9a21cc6ddcf31414d668dc7f269`. Current main non-owned resourceRemaining may still be positive→1. Keep deposit amounts unavailable until atomic qualified integration. Network and remembered stock quantities remain absent.
- Resource monitor still lacks several REL-ECO-017 fields, including exact income, saturation/depletion, blocked/route time, drop-off details, some research/affordability and network detail. Unavailable labels are honest interim state, not completion.
- Detailed selection purpose/use/limitation/counterplay remains unfinished. **Do not repeat the earlier false conclusion that authoritative prose does not exist.** Catalog lacks prose, but Requirements §12 SPEC-UNIT001..012 and SPEC-BLD015..017/SPEC-STR001..012 contain Player Purpose, Strategic Playbook, limits and counterplay. Trace these and check runtime capability before adding concise guidance.
- Final resource-strip affordance and route tests may need finishing; inspect latest worker status.
- Static title plate does not satisfy real-time3D bridge requirement.
- Actual pointer activation is unresolved, and most full menu/resize/accessibility routes are untested on revision2.
- Backend's fresh-skirmish-after-menu state-reuse observation needs investigation.

## Execute next

1. At handoff, all three HUD subagents were observed interrupted (no active writers). Inspect current files and newest statuses; finish any interrupted atomic edit without discarding changes. The latest bounded Controls/menu risk review was interrupted before a final report; do not assume it passed.
2. Read authority and fresh owner/resource state. Environment has reported release; explicitly claim the slot and check for intervening ownership before main integration/build/editor work. Preserve unrelated dirt and never kill an editor you do not own.
3. Review the source-only candidate for actual compile/routing defects. Rebuild the precise HUD manifest from retained baselines. Merge context deltas with qualified backend source; resolve header overlaps explicitly.
4. Run the editor build using the repository workflow. Run focused meaningful suites covering HUD widgets/layout, shell routes, canonical catalog resolution, controls persistence, pointer routing, feedback/resource scope, maintenance, camera and checkpoint recovery. New tests currently have no passing result. Fix failures, then broaden only as justified by overlap.
5. Use the newly qualified protected rendered-run helper. Preserve normal user saves/settings, use an isolated UserDir, retain command/log/identity/cleanup results. Do not recycle stale save-wrapper assumptions.
6. Launch a rendered candidate with `-EchoesShellInputTrace`. Check `[ECHOES_SHELL_INPUT]` move/hover/click/activate/controller traces and `[ECHOES_M01_POINTER_PRESS]` for actual pointer position/viewport/modal diagnostics. No press trace means delivery failed before the relevant route; a trace is not itself a visible activation pass. CUA or native tool availability does not prove input delivery.
7. Verify against original concepts at representative gameplay camera and 80/100/150% scale, high contrast, live resize, keyboard focus and physical pointer. Refine what is actually rendered.
8. Complete the full route matrix: title/campaign/skirmish/briefing; selected and mixed-unit console, command targeting, resource monitor; pause/resume; Options/Controls/remap/reset/restart persistence; display confirmation and timeout rollback; save/load/recovery/destructive confirmations; help/credits/errors; results/charts/rematch; replay browser/transport/perspective/seek; campaign map; multiplayer entry/lobby/local menu/reconnect; technology; tutorial dialogs/skip. Disabled reasons, Escape, pointer capture and modal swallowing must remain correct.
9. Close backend telemetry and source-supported semantic gaps without inventing data or capabilities. Keep network/fog boundaries intact.
10. Commit only owned changes in small logical commits after appropriate verification. Coordinate integration and any push with current ownership. Qualify the same packaged candidate through rendered physical-input routes and owner acceptance; do not claim 100% without evidence.

At each gate state what changed, what failed and why, what now passes, and what remains. The owner authorized continuation; routine reversible implementation choices do not require repeated approval. A genuine lease conflict or owner-acceptance gate must remain explicit.

---

## Continuation status — 2026-09-09, Claude HUD lane

Revision 2 is now integrated into main, compiles, and runs the full automation suite end to end. It was
never built before this pass. Evidence root for this continuation:
`BuildArtifacts/Evidence/hud-concept-completion-20260909/claude-continuation/`.

**Suite:** 133 declared tests, 125 pass, 8 fail, 0 not run. Sandbox isolation guards all passed and the
scoped save directory was empty after the run. Progression across runs is in `suite-result.json`.

**The blocker that hid everything else.** `RefreshContactWidgets` emptied its widget pool whenever the HUD
surface is hidden, then still walked every contact and consumed one widget per valid one, indexing an empty
array and aborting the process. It killed the automation worker, so the first run produced no report at all.
This reaches the player, not only the test. Written up in `CRASH-contact-widgets.md`, with the reverted-edit
reproduction and the instrumented runs retained beside it.

**Also fixed** (see `FIXES.md` for the full list and reasoning): a duplicate `GetGameplayFeedback()`
declaration and a duplicate test helper that both broke the build; canonical roster names lost when a caller
has no catalog; the campaign map taken off F1, which `SPEC-CTL-014` reserves; Control and Command inverted
when capturing a rebind on macOS; the replay camera gate requiring exact modifier equality; entity ids
printed with locale grouping; the console shrunk below the height its own selection text needs; and
control groups 1 to 4 silently disabled because the Mission 15 chords masked them, which predates this lane.

**SPEC-HUD-003 is implemented.** All 24 roster entities now carry purpose, strong use, limitation,
counterplay and an authored role, traced to `SPEC-UNIT-001..012` and the section 13 structure records.
`Echoes.Runtime.UI.SelectionGuidanceCoverage` guards it. Why this lives in the presentation layer rather
than the content catalog is recorded in `DECISION-selection-text-location.md`: the catalog digest is
serialized into snapshots, replays and the network handshake, so interface prose there would spend replay
compatibility.

**Eight failures remain**, all genuine revision-2 defects rather than merge damage. Exact assertions are in
`suite-result.json`. The online nested-menu route is the one to take first: `Back` from Options does not
return to the online local menu, and three further assertions cascade from it.

**Superseded below: the rendered pass has now run.** Original note: This session has no macOS application-control grant, so pointer
delivery could not be exercised. See `gui-control-readiness.md`. Note that the environment lane independently
reported unreliable pointer and zoom behaviour in its own run at 17:00 UTC, which supports input delivery
rather than HUD code as the cause of the earlier unresolved pointer failure. The shell input trace
(`-EchoesShellInputTrace`) and `run_protected_player_route.py` are both in place for that run.

**Ownership.** The exclusive main build and editor slot was claimed at 17:05 UTC after the environment lane
recorded its window complete; basis and process scan are in `slot-claim.json`. Thirteen non-HUD paths in the
worktree were stale copies of other lanes' work, including the environment lane's cliff mesh and four
generated materials; they were restored from main before integration so that work was not reverted.

### Rendered pass — 2026-09-09, completed

The owner granted Mac control, so the rendered pass ran. Full detail in `RENDERED-PASS.md`,
`POINTER-RESOLVED.md` and `rendered-01/identity.json`.

**The open pointer question is answered.** Pointer activation works. A background application click reached
nothing and produced no trace; a display-scope click at the same control produced `mouse_move`, `hover` and
`activate`, and the screen changed. The earlier failure was input delivery, not the HUD. A pointer result
from this project means nothing unless the run shows an `[ECHOES_SHELL_INPUT]` press line.

**`SPEC-UI-006` is verified end to end for the first time**: open Controls, capture a key, rejection before
applying naming both conflicting commands, cancel preserving the original, a successful apply reported as
"Option+H applied", persistence into the sandbox `Input.ini` rather than real settings, and a confirmed
restore-defaults that focuses Cancel rather than Confirm. Mac renders `bAlt` as Option and `bCtrl` as
Command, which is why the Mission 15 prompt resolves its chord live instead of printing a literal.

**The battlefield console renders as intended**: Menu top left, compact Matter/Dawn/Logistics strip with
faction and match context, wide visible terrain, minimap left, selection centre, command card right with a
glyph and its hotkey. The authored role reaches the player as `HEADQUARTERS DROP-OFF`.

**Four rendered defects are open**, listed in `RENDERED-PASS.md`. Two matter most. Escape neither cancels the
capture dialog nor navigates back, because `Pause Scenario — Escape` consumes it, so two screens print an
instruction that does not work. And the selection card now overflows the console and clips mid-sentence at
single selection, which is a direct consequence of adding the `SPEC-HUD-003` fields; the layout test checks
panel geometry and cannot see text overflow, so it passes while the player cannot read the line.
