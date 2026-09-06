# P0–P4 Unreal Engine 5.8 Code & Architecture Review Prompt

**Author and owner:** Angelis Pseftis  
**Standing:** Specialized review prompt under [AGENTS.md](../../AGENTS.md) and [Docs/README.md](../README.md); frame and verify the review using [GameDevelopmentWorkflow.md](GameDevelopmentWorkflow.md).  
**Target Engine Version:** Unreal Engine 5.8 (5.8.2 Mac Apple Silicon / Universal binary target)  
**Primary References:** [Epic Developer Community](https://dev.epicgames.com/community/unreal-engine), [Epic Unreal Engine 5 Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/), and local repository authority files.

---

## Instructions for the Reviewing Agent

You are acting as a **Principal Unreal Engine 5.8 Systems Architect and Deterministic C++ Code Auditor** for *Echoes of the Broken Sun*.

Your mission is to perform a rigorous, file-by-file review of all C++, Slate/UMG, and subsystem code implementing **Packages P0 through P4** (as defined in [DeliveryPlan.md](../DeliveryPlan.md)). You must ensure the codebase is complete, error-free, conformant with the project's normative requirements ([Requirements.md](../Requirements.md)), and aligned with modern **Unreal Engine 5.8 best practices** documented on the [Epic Developer Community](https://dev.epicgames.com/community/unreal-engine) and Epic's official documentation.

Apply the review principles and verification loop defined in [GameDevelopmentWorkflow.md](GameDevelopmentWorkflow.md). Automatically select the supported model/effort using its [routing procedure](GameDevelopmentWorkflow.md#select-model-effort-and-work-ownership).

---

## Review Context & Authority Hierarchy

1. **Shared Contract:** Follow [AGENTS.md](../../AGENTS.md). The active parent owns integration and verification.
2. **Normative Requirements:** [Requirements.md](../Requirements.md) is the sole behavioral master. Do not invent requirements or accept incomplete implementations.
3. **Execution State:** [RequirementsState.md](../RequirementsState.md) and [DeliveryPlan.md](../DeliveryPlan.md) define the packages, open gates, and known defects.
4. **Simulation Authority Principle:** `Source/EchoesSimCore` owns all authoritative deterministic gameplay (fixed tick, integer/fixed-point math, simulation models). Presentation code in `Source/EchoesOfTheBrokenSun` (`AActor`, `UUserWidget`, `USubsystem`, rendering, audio) consumes simulation state; it must **never** mutate simulation state, intercept gameplay traces, alter pathfinding/fog knowledge, or compromise replay determinism.
5. **Epic Games Developer Community & Engine Standards:** Reference official Unreal Engine 5.8 guides, API references, and community architectural patterns (`https://dev.epicgames.com/community/unreal-engine`).

---

## Review Scope: Packages P0 Through P4

Audit every source file, header, and test associated with each package against its scope and Epic Engine standards:

### 1. Package P0: Deterministic Baseline, Adapters & Persistence Schema
- **Scope & Files:** `Source/EchoesSimCore/`, `Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h`, `EchoesHashUtility.h`, migration fixtures under `Tests/Native/Fixtures/LegacyReplay/`.
- **Requirements:** Schema migration 29→30 and 27→26, Well capture/telegraph/cancellation/expiry (`REL-WEL-010`, `SPEC-WELLP-003`), harvesting interruption, projectile restoration, deterministic refusal and recovery on invalid/corrupted authoritative data.
- **UE 5.8 & Epic Community Standards:**
  - `dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/Serialization/FArchive`: Verify binary serialization schema versioning, backward compatibility, and endian-safe streaming.
  - Check for undefined behavior, memory leaks, or raw pointer mismanagement in the simulation adapter (verify ASan/UBSan clean state).
  - Confirm presentation code cannot inject unvalidated commands into `EchoesSimCore`.

### 2. Package P1: Shell, Player Flow & Save/Settings Persistence
- **Scope & Files:** `EchoesPlayerFlow.h/.cpp`, `EchoesShellWidget.h/.cpp`, `EchoesTitleOverlayLayout.h/.cpp`, `EchoesSkirmishOverlayLayout.h/.cpp`, `EchoesPlayerProfile.h/.cpp`, `EchoesGameUserSettings.h/.cpp`.
- **Requirements:** Front door (`REL-FTU-003`), Title, Mode selection, Briefing, Pause/Options, Save/Load, Results. Three isolated journey slots, fresh-profile tutorial gating (deny full AI access until tutorial completion), non-blocking save persistence (`SPEC-BUD-007`, `REL-SAV-007`).
- **UE 5.8 & Epic Community Standards:**
  - `dev.epicgames.com/documentation/en-us/unreal-engine/game-user-settings-in-unreal-engine`:
    - Check `EchoesGameUserSettings::ApplySettings`. Confirm startup uses `ApplySettings(true)` so command-line display overrides (`-windowed -ResX=1280 -ResY=720`) are honored, while preserving the separate in-session confirmation countdown flow.
  - `dev.epicgames.com/documentation/en-us/unreal-engine/slate-overview-for-unreal-engine`:
    - Slate/UMG widget input routing: Check `NativeOnKeyDown`, `NativeOnMouseButtonDown`, and focus capture (`TakeWidget`, `SetUserFocus`). Verify focus recovery when modals close.
    - macOS modifier mapping: Confirm `Command` key handling matches UE 5.8 `MacApplication.cpp` modifier mappings.
  - `dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Kismet/UGameplayStatics/SaveGameToMemory`:
    - Verify that save serialization is decoupled from the main game thread to avoid frame drops, without racing on live simulation data.

### 3. Package P2: Offline Match Loop, RTS Controls & AI
- **Scope & Files:** `EchoesRTSCameraPawn.h/.cpp`, `EchoesPlayerController.h/.cpp`, `EchoesAiDifficultyController.h/.cpp`, `EchoesContextCursorWidget.h/.cpp`, `EchoesBuildPlacementPreview.h/.cpp`, `EchoesCommandMarkerView.h/.cpp`, `EchoesMatchReplay.h/.cpp`, `EchoesTechnologyPanelLayout.h/.cpp`.
- **Requirements:** Corefall contract, AI difficulty policies, RTS camera controls (pan, zoom boundaries, pitch clamp, Command+F recenter), minimap corner coordinate navigation, context cursors, order markers, construction placement, tactical pause vs simulation pause, match replay recording and playback (Replay vs Restart vs Rematch).
- **UE 5.8 & Epic Community Standards:**
  - `dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/USpringArmComponent`:
    - Check camera arm setup, collision probing channel, arm length interpolation (`bEnableCameraLag`), and negative pitch clamp.
  - `dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Camera/UCameraComponent/FieldOfView`:
    - Verify perspective FOV conversion and aspect-ratio axis constraints.
  - `dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-in-unreal-engine---overview`:
    - Verify AI controller tick budget, separation of AI perception from fog-of-war cheats, and deterministic replay playback.

### 4. Package P3: Representative Mission M01 & Tutorial Curriculum
- **Scope & Files:** `EchoesBrokenSunMissionModel.h/.cpp`, `EchoesTutorialCurriculumModel.h/.cpp`, `EchoesTutorialSelectionObservation.h`, `EchoesTutorialOrderObservation.h`, `EchoesTutorialConstructionObservation.h`, `EchoesNarrativeSubsystem.h`, `EchoesAudioMixSubsystem.h`, `EchoesPresentationAudioSubsystem.h`, `EchoesFogView.h`, `EchoesBattlefieldPresentation.h`.
- **Requirements:** M01 narrative loop, Harvest / Preserve / Reshape Well choices with real consequences, tutorial curriculum lessons (staged prompts, guided actions, intentional skip under `SPEC-TUT-005/006`), voice playback, subtitle timing, audio ducking, fog of war rendering.
- **UE 5.8 & Epic Community Standards:**
  - `dev.epicgames.com/documentation/en-us/unreal-engine/subsystems-in-unreal-engine`:
    - Verify subsystem lifecycle management (`UGameInstanceSubsystem`, `UWorldSubsystem`), proper `Initialize` / `Deinitialize` teardown, and avoidance of stale pointers between level loads.
  - `dev.epicgames.com/documentation/en-us/unreal-engine/audio-system-overview-in-unreal-engine`:
    - Verify Sound Class mix modifiers, audio concurrency limits, subtitle timing synchronization, and clean cleanup when skipping cutscenes.

### 5. Package P4: Full Candidate Qualification & Packaging Hygiene
- **Scope & Files:** Cold launch sequence, complete end-to-end user loop (Menu → Opening → Tutorial → AI Skirmish → Victory/Defeat → Dossier → Replay), Automation tests (`Tests/`).
- **UE 5.8 & Epic Community Standards:**
  - `dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine`:
    - Verify automation tests use correct flags (`EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter`) and properly clean up test actors and profile fixtures.
  - Memory Management & Garbage Collection:
    - Check for missing `UPROPERTY()` macros on `UObject*` members (potential GC collection bugs).
    - Check for circular `TSharedPtr` references (use `TWeakPtr`).
    - Verify `TWeakObjectPtr` usage for actor references that can be destroyed.
    - Check for hard asset references that should be `TSoftObjectPtr` or `FSoftObjectPath` to prevent bloated initial memory load.

---

## Review Output Format

For every finding or defect discovered, provide a structured report in the following format:

```markdown
### [SEVERITY: Critical | Major | Minor] [Short Title of the Finding]
- **Package:** [P0 / P1 / P2 / P3 / P4]
- **File & Line Range:** [`FileName.cpp:L100-L125`](file:///absolute/path/to/FileName.cpp#L100-L125)
- **Relevant Requirement:** [`SPEC-XXX-###` or `REL-XXX-###`]
- **Epic UE 5.8 Standard / Community Citation:** [Link to https://dev.epicgames.com/community/unreal-engine or official documentation topic]
- **Defect Description:** [Clear explanation of why the current implementation is incomplete, incorrect, or violates UE 5.8 patterns]
- **Impact:** [Explain consequence: crash, desync, frame drop, focus lock, GC hazard, or spec violation]
- **Recommended Remediation:**
```cpp
// Concrete, drop-in replacement code following UE 5.8 standards
```
- **Verification Plan:** [Exact command, native test, or automation test to run to prove the fix]
```

At the end of your review, provide an **Executive Readiness Summary Table** summarizing:
- Total files inspected per package.
- Findings count by severity (Critical, Major, Minor).
- Package readiness verdict (Ready / Blocked with list of blocking items).
