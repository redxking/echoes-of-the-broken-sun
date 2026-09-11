# Echoes continuation handoff — spent

**Author and owner:** Angelis Pseftis
**Issued:** 2026-09-06
**Closed:** 2026-09-09
**Standing:** spent one-shot handoff, retained as history. Do not open a session from this prompt.

This handoff was written for a Gemini continuation that never took the work. Development continued on
`main` without it, and the Gemini Vision capability it assumes was removed from scope on the day it was
written — see [SeeLoopWorkflow.md](SeeLoopWorkflow.md).

Everything below describes the state of the checkout on 2026-09-06: a pinned base commit, evidence
directories from that date, process identifiers, and specialists whose work was already stopped. None of
it is current. To start work now, use [AgentProjectBrief.md](../../Prompts/AgentProjectBrief.md) with
[GameDevelopmentWorkflow.md](../../Prompts/GameDevelopmentWorkflow.md), and take current state from
[DeliveryPlan.md](../../DeliveryPlan.md) and [RequirementsState.md](../../RequirementsState.md).

The owner decisions recorded under "Latest owner decisions" below remain owner decisions and are not
withdrawn by this closure; the Gemini Vision entry among them is the one that retired the See Loop.

## Assignment and authority (historical, 2026-09-06)

Continue the existing Echoes development task in place. Finish the approved P0–P4 obligations and close applicable failures before P5. Do not restart completed work or create another delivery plan. Immediate work is the gated tutorial described below. Only Angelis accepts requirements or readiness. This handoff is a continuation aid, not a replacement requirements master.

Checkout: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project`.
Branch: `main`. Base before this handoff commit: `1a60cb1fecdd5a709f940726a6a3e15b0fc378ff`.
Remote: `https://github.com/redxking/echoes-of-the-broken-sun.git`.
The owner requested committing all local project changes and pushing to main. Read the resulting commit with `git log -1`; do not assume the old base is the current candidate. The accumulated batch includes gameplay, compatibility, input/camera, tutorial observers, narrative/audio, fixtures and documentation. Preserve every existing change, including unrelated work. Do not reset or regenerate everything as cleanup.

Read these existing authoritative files first:

- `AGENTS.md`, `Docs/README.md`, `Docs/AgentSkillRouting.md`.
- `Docs/Requirements.md` (normative), `Docs/RequirementsState.md` (evidence and owner decisions), `Docs/DeliveryPlan.md` (existing sequence).
- [GameDevelopmentWorkflow.md](../../Prompts/GameDevelopmentWorkflow.md), [P0P4CodeReviewPrompt.md](../../Prompts/P0P4CodeReviewPrompt.md).
- `Docs/OpeningAndTutorialScript.md`, `Docs/Archive/DevelopmentBible.md` and relevant architecture references through the authority map.
- Applicable canonical skills under `.opencode/skills/`, especially session control, heavy-run coordination, Unreal runtime integration, input controls, tutorial and GUI readiness. Discover exact names from the routing map.

One integration/write owner for this checkout. Previous root owned all writes/builds/game runs. Specialists `compatibility_audit` (Mencius) and `fixture_audit` (Nash) were read-only; their retained work is complete or stopped. No delegated edits await merging. Recheck live processes before taking resource ownership. Codex will not continue implementation after this handoff unless the owner resumes it.

## Latest owner decisions — do not ask again

- Zoom in/out, minimap corner navigation and Command+F work, confirmed by Angelis. Leave these working behaviors unchanged unless a new reproducible defect requires repair.
- Owner uses a Mac keyboard. Do not require End or assume PC modifier labels. Command+F is the working physical Mac chord. Internal UE modifier flags may differ; derive visible prompts from active bindings/platform.
- Owner clicked Start tutorial and Deploy, then intentionally pressed Escape to bypass the unfinished cutscene. Latest direction: leave that cutscene aside and continue tutorial work. A purple frame was observed earlier, but it is not an established camera defect. No new cinematic repair was made in response. Preserve pre-existing cinematic changes.
- Gemini Vision integration was explicitly removed from scope. Do not restore it, retry its authentication, request secrets, or treat unavailable Vision/audio capture as a pass. Direct inspection remains available; audio acceptance needs actual listening evidence.
- Angelis can perform testing. Give one concrete mouse/keyboard action, wait for Done, then inspect the actual result. Do not treat a screenshot, seeded completion, controller call, headless run, or log as proof of physical interaction.
- Proceed autonomously with routine fixes. No P5 until P0–P4 applicable gates are closed. Do not claim all code audited or all phases complete from test counts.

## Current evidence and unresolved boundary

Evidence root on this Mac:
`BuildArtifacts/Evidence/p0-p3-readiness-20260906/` (relative to checkout).
This directory is ignored local evidence, not automatically backed up by pushing source. A remote Gemini session needs these artifacts separately to inspect them; otherwise treat results below as retained historical reports, not freshly verified. Preserve all failed and partial runs.

| Evidence directory | Result and scope |
|---|---|
| `tutorial-baseline-full-1` | 115/116; Bootstrap fixture chord mismatch. Repaired and subsequently tested below. |
| `tutorial-input-focused-1` | Bootstrap and narrative binding: 2 clean passes after correcting fixture arguments and Mac labels. |
| `legacy-continuation-save-1` | Authentic legacy continuation second-save failure reproduced, then repaired. `native-qualified.log`: 108/108 optimized, 108/108 debug, 108/108 ASAN/UBSAN. `fixture-build.log`: editor build succeeded after shared fixture hardening. |
| `legacy-checkpoint-focused-1` | Actual Unreal QuickSaveLoad adapter regression passed using authentic historical baseline. |
| `tutorial-integrated-full-2` | 116/116 clean, exact expected inventory, no test warnings/errors; wrapper exit 0 and isolated-save cleanup passed. `candidate-identity.json` freezes source/config/native-fixture/library hashes; checked unchanged after run. `inventory-validation.json` records expected=116, exact_inventory=true, all_clean=true. |
| `tutorial-integrated-player-2` | Rendered cold launch exposed ignored window command-line settings. `window-repair-build.log` subsequently succeeded for the narrow startup correction. |
| `window-startup-focused-1` | Wrong test filter matched none; exit 3. NOT a pass. Actual test name is `Echoes.Runtime.UI.PlayerShellRoutes`, but unattended tests skip the changed local rendered startup branch. |
| `window-startup-player-1` | Repaired rendered cold launch reached title with outer window 1280x752 and content1280x720. `source-build-sha256.json`, `window-observation.json`, `window-title.png`, `route-observations.md`, `PlayerRoute.log` retain scope. Title → tutorial briefing observed after owner click. Owner subsequently reported Deploy and Escape bypass. Tutorial completion not verified. |

The full 116-test result PRECEDES the single startup ApplySettings correction. Full integration requalification after that correction remains pending. New gated tutorial behavior and lessons 6–10 remain unimplemented. The connected journey, applicable package/audio/human gates and overall P0–P4 audit remain open. This is not a release-ready candidate.

Earlier partial build/native runs and two wrong filter attempts are retained; do not promote them to success. Verify subsequent results, not just a prior “fixed” report.

## Repairs already made — preserve them

1. `EchoesRuntimeSmokeTest.cpp`: helper signature was `(Action, Key, bControl=false, bShift=false)` but test called F,false,true, asking for Shift+F. Corrected to F,true,false. Existing config was correct. `EchoesNarrativeSubsystem.cpp::ResolveInputTokens` now labels Mac internal Ctrl as Command, Alt as Option, Cmd as Control; narrative test expects platform-correct physical chord.
2. Authentic schema29/replay26 continuation could save schema30 with stale legacy network fields and fail reloading: `snapshot Meridian network state is invalid`. `Simulation::SaveSnapshot(uint64_t* snapshotStateChecksum = nullptr) const` now normalizes a copy for current serialization, returns the exact serialized-state checksum, and leaves live historical replay semantics unchanged. `CaptureReplayBaseline` normalizes when starting a new current replay. `EncodeImmutableCheckpoint` consumes the emitted checksum; retained historical replay prefix compares against `ReplayStateChecksum()`, not a checksum from a different schema. Strict assertions remain.
3. Authentic historical fixtures are in `Tests/Native/Fixtures/LegacyReplay/`. Do not manufacture historical bytes by editing a current schema label. Schema29 checkpoint writer used hash-verified archived source from `source-12-cpp.tar`, SHA256 `8d66e298d9148e7a482a80665a36b299af839ebf0c89343fc04d5c617a40cf4e`. Historical Simulation.cpp SHA256 `77671d04888089ff52125520dc9cbdf9c5c33b2510d884151ae769818b0a153f`; header SHA256 `45c636fa269d29c3ddeed0ec30f918583814a9fdf75efaca46464e3e08abdc21`. Check fixture README/receipts for complete provenance.
4. Game-context historical fixture uses 64x64, 20Hz, seed0xE0C0B5A1, MeridianCompact/KharuunAssemblies, actual terrain prerequisites; checksum5897829099007494806, finalTick0, replayVersion26. QuickSaveLoad test clears backups/staging before reloading the new primary so fallback cannot mask failure. Native test verifies second save, unchanged live legacy checksum, prefix continuation and zero-tick rebase.
5. `Private/Tests/EchoesPreservedTestFile.h` replaces 17 unsafe preservation helpers. Requires current automation test and successful capture; every declaration guards IsReady and returns before mutation on setup failure. Failed capture reports `[FIXTURE_CAPTURE_FAILED]`; failed restoration reports `[FIXTURE_RESTORE_FAILED]`. Destructor does not overwrite unreadable originals. QuickSaveLoad cleanup verifies deletion and stops on failure.
6. `EchoesPlayerShell.cpp::InitializePlayerProfile`: rendered non-PIE startup `Settings->ApplySettings(false)` changed to true to honor explicit Windowed/ResX/ResY flags. In-session options confirmation remains intact. Installed engine source establishes the cause; actual cold launch confirms window dimensions. Full subsequent suite still pending.

## Immediate tutorial implementation

Controlling new requirements: `SPEC-TUT-005` and `SPEC-TUT-006` in the master. They were recorded, NOT implemented. Existing gameplay observers are useful foundations, not the requested finished onboarding.

Required player experience:

- Teach one small action at a time. Freeze unrelated state and block untaught general actions; permit the current action, necessary learned controls and safe pause/accessibility/exit.
- Darken the rest of the UI and spotlight the real target. Animate a ghost cursor/indicator demonstrating where to act; this presentation must never perform or credit the action.
- Advance only from genuine authoritative observation of the required player action. Show completion feedback/reward and the next task.
- Replace ambiguous “Marker A”/“Marker E” language with clear named Archive/Evacuation locations and explicit find → found feedback. Target actual world/minimap position, not decorative text detached from geometry.
- Top-right, low-emphasis but readable Hold to skip, away from spotlight. Continuous pointer or keyboard hold fills a circular meter over 1.5 seconds. Early release, focus/capture loss or interruption cancels; no accidental tap skip.
- Completed hold pauses and opens a small modal: Skip this step only / End all tutorials, with safe cancel. Step skip resolves prerequisites and advances but records skipped, never false mastery/reward. End immediately removes tutorial gates/restores control after choice. Restore prior pause/focus state correctly.
- Apply consistently to guided lessons; independent readiness challenge remains unguided. Remapped controls, Mac labels, reduced motion and accessibility remain usable.

Important integration traps from source review:

- `Private/EchoesPlayerTutorial.cpp` implements only Survey=1, Roster=2, Muster=4, Route=8, Reserve=16. `FEchoesTutorialPracticeState::ImplementedLessonMask=0x001f`. Remaining curriculum Link32, Foundry64, Probe128, Board256, Well512 is not wired.
- `FEchoesPlayerProfile` schema2/min1, AllMask0x03ff; fixed49-byte payload in `EchoesPlayerProfile.cpp`. `TutorialVerifiedMask` validation requires a contiguous low-bit prefix. Skipping cannot set a verified bit. Design explicit skipped progression/migration or a sound separate state model, preserving historical profiles and mastery requirements.
- `RequireOperationMastery` permits mastery or authorized training; existing opt-out does not automatically unlock campaign/skirmish. Do not silently remove readiness requirements when implementing End tutorials.
- `TickTutorialObservation` runs from `Bridge.OnFixedStepObserved`, requires authorized training, profile, bridge, camera, foreground and unpaused scenario, and returns for opening/modal/save failure. It observes once per sim tick (20Hz). Pausing the entire sim during required actions can deadlock dwell/action proof. Freeze unrelated behavior without disabling the required observation clock. Training opponent AI is already disabled.
- Survey stores pan/minzoom/maxzoom/recenter flags and requires three sites with30 consecutive ticks each, pan2tiles, recenter tolerance200cm. Expose read-only progress for staged prompts; do not weaken final predicates. Navigation provenance distinguishes player input from programmatic moves.
- Selection observer uses actual post-mutation selection and subsequent HUD frame publication. Order observers use accepted receipt sequence/origin, rejection and acknowledgement, bound actors/geometry/session/authority. Do not substitute button clicks for successful gameplay credit.
- `CommitTutorialLesson` is the genuine commit path with profile validation/persistence rollback. `ResetTutorialObservation` clears transient observers/receipts/IDs and increments session. Focus-loss handling must cancel transient holds/dwell without erasing durable profile progress.
- `IsModalOverlayVisible` alone does not pause simulation. Follow exact prior-pause restoration patterns used by production cancellation in `EchoesPlayerFieldHud.cpp`.
- `UEchoesFieldHudWidget` is an owned UMG widget, created by controller at viewport20. Minimap has NativePaint/pointer handlers; anchors approximately(.02,.72)–(.20,.98). Gate real input and command dispatch as well as HUD, so alternate routes cannot bypass tutorials. Use actual geometry for spotlight/hit testing.

## Research before code

Owner requires these phases, briefly, using current Epic docs and installed engine before implementation:

### Phase 1: Engine Standard Research

Verify exact version, classes and headers; do not invent APIs. Installed engine previously checked: UE5.8.2 CL56702186, compatible55116800, `/Users/Shared/Epic Games/UE_5.8`.
Relevant installed sources: UMG/Public/Blueprint/UserWidget.h (NativeTick, NativeOnFocusLost, NativeOnKeyDown, NativeOnMouseCaptureLost), Input/Reply.h, Engine/Private/GameUserSettings.cpp and GameEngine.cpp, ApplicationCore MacApplication.cpp modifier mapping, Slate InputChord.cpp platform labels. UMG/Slate/SlateCore already used; verify Build.cs live.

Previously consulted official references (refresh affected APIs before editing):

- https://dev.epicgames.com/documentation/en-us/unreal-engine/design-guidelines-for-using-commonui-in-unreal-engine
- https://dev.epicgames.com/documentation/en-us/unreal-engine/input-fundamentals-for-commonui-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine?lang=en-US
- https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameUserSettings?lang=en-US

CommonUI guidance explicitly discusses PC RTS as potentially not benefiting from its routing. Do not migrate wholesale to CommonUI/GAS/Enhanced Input just to appear modern. Official Lyra documentation was reviewed; actual Lyra source was not downloaded/verified. Say which evidence you actually inspect.
Saving guide recommends async live-play saves; Echoes already uses immutable capture and background work. USaveGame alone does not solve custom deterministic schema/replay compatibility.

### Phase 2: Architecture Plan

Explain the bounded implementation within the EXISTING delivery plan: C++ authority/state/input versus UMG/Blueprint presentation/data, exact UCLASS/USTRUCT/dependencies and recovery model. Do not restart planning or rewrite approved requirements. EchoesSimCore remains deterministic gameplay authority.

### Phase 3: Code & Editor Implementation

Implement complete files in place with useful comments explaining lifecycle/macros/authority. State any actual Editor configuration needed. Avoid manual generated-asset patches; use registered source/generators.

## Qualification and operating commands

At handoff preparation the game process was PID16418, launched with isolated saves under `window-startup-player-1`. PID/session values are historical; rediscover them. Codex session54740 is not transferable to Gemini. Leave the user's current game intact until you need exclusive build/test resources, then close gracefully and confirm exit. Do not kill unrelated editor processes or run heavy editor/build/game tasks concurrently.

From checkout:

```sh
./Scripts/test_content.sh
./Scripts/build_editor.sh
./Scripts/test_sim.sh
```

Native script runs optimized/debug/sanitizer suites. Full Unreal:

```sh
env TMPDIR="$(getconf DARWIN_USER_TEMP_DIR)" \
ECHOES_AUTOMATION_REPORT_DIR="$PWD/BuildArtifacts/Evidence/p0-p3-readiness-20260906/gemini-integrated-1" \
./Scripts/run_unreal_tests.sh
```

Choose a fresh directory; inspect live script arguments. Runner uses save isolation and exact registered inventory. It can spend roughly2minutes checking nested report data after editor exit; do not assume hung or interrupt it unnecessarily. Parse index.json with utf-8-sig, per-test warnings/errors, exact inventory, wrapper exit and cleanup. For focused automation use `Scripts/echoes_test_sandbox.py`; inspect its help and verify exact test names with rg before launch. The actual native filter is `authentic schema29` with spaces; a prior hyphenated filter matched none.

Rendered executable:
`/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor`
Launch project with `-game -windowed -ResX=1280 -ResY=720 -ForceRes -nosplash`, fresh isolated `-EchoesSaveGameDirectory=...`, `-UserDir=...`, `-EchoesShellInputTrace`, `-abslog=...`. Quote paths with spaces. Recheck actual window/content dimensions after title loads, not just splash. Bringing Unreal forward and screencapture can inspect visible state; it cannot establish action attribution by itself.

Operating rule: reproduce a failure with the smallest meaningful test, fix its cause, pass affected checks, freeze source/build identity, build and run integration, then verify player-visible behavior on that candidate before adding another behavior batch. Preserve negative evidence. Never weaken assertions, change expected results merely to fit code, remove coverage or substitute fake historical fixtures.

For production queues, cancellation, rally, repair, combat and tutorial changes, check affected campaign behavior, AI cadence, resource accounting, save migration, replay execution/checksums, HUD state and lesson credit. Existing full-suite success does not certify every P0–P4 requirement. Use the master/state/DeliveryPlan to locate remaining exact acceptance gates.

Connected route remains required: cold launch → menu → tutorial → gameplay → pause/focus/modal recovery → save/load → results → replay → return to menu through actual rendered mouse/keyboard input on an identified candidate. Separate packaged, performance, audio/listening, uncoached human and owner gates as required. Do not report them closed from headless counts.

Update `Docs/RequirementsState.md` and `Docs/DeliveryPlan.md` in place after each bounded result. Retain historical failures and identity receipts. Run `python3 Scripts/check_agent_docs.py` after documentation edits; this proves structure/links only. Owner acceptance remains exclusively Angelis's.
