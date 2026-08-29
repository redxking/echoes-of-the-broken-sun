---
title: Echoes of the Broken Sun Project Ledger
author: Angelis Pseftis
creator: Angelis Pseftis
status: Authoritative
created: 2026-08-28
updated: 2026-08-28
---

# Project Ledger

This is the single authoritative production roadmap, decision log, risk register, acceptance register, performance budget, test record, and known-limitations report. It is edited in place. Evidence entries describe only observed results at the named boundary.

## Status at a glance

The project is in **Milestone 2 — Unreal playable-systems prototype**. The hardened engine-independent simulation passes 12/12 optimized, debug, and sanitizer suites, including deterministic routing around a blocked terrain wall. Unreal Engine 5.8.2 project generation succeeds; the Mac Development Editor target compiles and links for arm64; three Unreal automation tests pass, including production, logistics, pause/restart, entity visibility lifecycle, and 4,096-tile fog/shroud accounting with persistent exploration; and null-RHI bootstrap reaches the first fixed tick with 25 entities and 10 visible views at 20 Hz. The current fog boundary, HUD, and pause feedback have been visually observed. A self-contained 0.3.0 arm64 Mac Development application is traceable to clean source commit `08f1bab` and passed local structural, signature-seal, content-manifest, fog, and packaged-startup checks. This does not yet establish complete manual playability, a qualified end-to-end match, navigation/fog performance at production scale, multiplayer, Developer ID signing, notarization, clean-machine compatibility, or distribution readiness.

The user deleted the first local project workspace during storage cleanup and explicitly directed that the game repository be remade. The current repository and authoritative files were recreated at the same path. The original local Git history and filesystem metadata did not survive that deletion; this limitation cannot be repaired by claiming continuity that does not exist. All subsequent changes remain in the current authoritative files.

## Decisions

### D-001 — Select Unreal Engine 5.8 as the production target

Use the precompiled Unreal Engine 5.8 launcher build and a C++ project. Keep deterministic game state in an engine-independent module. Do not build the engine from source unless a specific engine modification becomes necessary.

The project prioritizes high-fidelity 3D presentation, Metal support, mature navigation and AI facilities, profiling, automation, replays, and separate-process multiplayer testing. The user authorized and began Unreal installation. Unity is more comfortable on the inspected 16 GB M1 Pro, and Godot has a smaller storage/licensing footprint; neither removes the custom deterministic RTS work. Unreal must pass a measured Mac technical spike. Failure to meet frame-time, memory, build, or iteration budgets reopens the decision before content expansion.

### D-002 — Separate simulation from Unreal presentation

Commands, economy, combat resolution, Future Well state, AI-visible information, replay records, save schema, and match hashes live in a standard C++20 core without Unreal object, physics, navigation, animation, or wall-clock dependencies. Unreal owns input translation, camera, view actors, animation, effects, audio, UI, asset loading, and platform integration.

### D-003 — Use deterministic command simulation as match authority

The initial multiplayer model is host-authoritative deterministic command simulation with input delay, periodic state hashes, validated command envelopes, snapshots for reconnect/recovery, and a retained replay stream. Dedicated-server evolution remains possible. Pure peer lockstep makes recovery and cheat containment harder; fully replicated actor state does not by itself provide deterministic replay.

### D-004 — Optimize for the inspected M1 Pro without declaring it the shipping baseline

Use native arm64 builds, disable Nanite and Virtual Shadow Maps, evaluate Lumen software lighting only under profiling, and retain a conventional fallback. The M1 Pro/16 GB host is below Epic's M3/32 GB recommendation; meeting a minimum tier does not establish 60 FPS.

### D-005 — Treat Future Well outcomes as explicit state transitions

Harvest is irreversible after its telegraphed commit; Preserve is a contestable recurring state; Reshape is a time-bounded map-authored manifestation with deterministic expiration and displacement. Campaign consequences record facts rather than a moral score.

## Milestones and acceptance gates

### M0 — Environment and engine decision

- Inspect hardware, macOS, storage, Xcode, Metal, Git/LFS, engines, and workspace.
- Check current engine trade space against official sources.
- Record engine/source-build decision and limits.
- Separate interactive account/installer gates from automated work.

**State:** Complete for the technical spike. Xcode 26.6 passed local generation, compilation, automation, null-RHI/rendered startup, and Development cook/package gates; Epic's recommended 26.1.1 baseline, clean-machine compatibility, and representative performance remain open.

### M1 — Production baseline and simulation spike

- Isolated repository without credentials or machine-specific build paths.
- Authoritative design, architecture, setup, evidence, and asset records.
- Data definitions for both slice factions and Future Wells.
- Standard C++20 core builds with Apple Clang without Unreal.
- Tests prove deterministic command ordering/checksums, economy, placement/construction, combat, visibility, all Well modes, bounded AI inputs, replay, and versioned snapshot behavior.
- Failures return stable reason identifiers.

**State:** Substantially complete for the technical spike. Commit `adad96e` records the historical 7/7 baseline; the current integrated hardening passes 12/12 in all three configurations. Content-pack import, authenticated multiplayer-seat binding, replay work limits, and stable execution-time rejection reporting remain open.

### M2 — Unreal technical prototype

- UE 5.8 C++ editor target builds and launches natively on arm64.
- A prototype map exposes camera, box selection, context orders, pathing, economy, construction, production, combat, fog, victory, and restart.
- Unreal remains a view/controller over the tested simulation; frame rate does not change checksums.
- Stress scene records CPU, GPU, memory, navigation, and game-thread measurements.
- Menus do not advertise unavailable modes.

**State:** In progress. Compilation, module loading, class/bootstrap automation, deterministic obstacle routing, world-level visibility actor lifecycle, 4,096-tile fog/shroud with explored-state persistence, timed worker/soldier production, logistics capacity, pause, deterministic restart, derived Command-Core victory, null-RHI scenario creation, rendered startup, current-source packaging, and bounded keyboard controls pass at their recorded boundaries. Construction hotkeys and production controls are wired, but exact pointer-driven construction/production/combat and a full rendered match have not been accepted. Broader orders, navigation/fog stress measurement, and final fog art remain open.

### M3 — Vertical slice

- Meridian Compact and Kharuun Assemblies are playable with slice rosters.
- The Glass Scar supports a complete skirmish and prologue.
- Standard AI gathers, scouts, expands, evaluates the Well, fights, and retreats without hidden income or vision.
- All Well modes are telegraphed, deterministic, and strategically distinct.
- Save/load, pause, checkpoints, settings, victory/defeat, tutorialization, and implemented accessibility controls work.
- Art/audio are registered final assets or labeled placeholders.
- A native Apple Silicon development build is produced from documented steps.
- Manual, automated, and performance evidence is recorded.

**State:** Not started.

### M4 — Architecture and performance gate

- Slice meets approved budgets or an explicit redesign is accepted.
- Path bursts, fog, formations, combat, transformation, saving, and replay seek are profiled.
- A 60-minute soak characterizes memory growth/cache behavior.
- Content multiplication is blocked until critical weaknesses are resolved.

**State:** Not started.

### M5 — Multiplayer and replay

- 1v1 works between separate processes; 2v2 works at target scale.
- Version rejection, validation, desync detection, reconnect, disconnect, surrender, draw, lobbies, spectator state, and replay are tested.
- Loss, latency, reordering, and recovery are recorded.
- Separate-machine evidence exists before completion is claimed.

**State:** Not started.

### M6 — Content completion

- Hollow Choir, supported maps, technologies, AI personalities, 12–18 missions, cinematics/interludes, and ending logic are implemented.
- Every mission is traversed from clean progression and supported saves.
- Content/balance data remains synchronized with player information.

**State:** Not started.

### M7 — Release qualification

- Clean Mac build/install/launch succeeds.
- Campaign, skirmish matrix, multiplayer, saves, settings, controls, accessibility, performance, recovery, and rights pass release plans.
- Critical crash, progression, data-loss, and desync failures are resolved.
- Signing/notarization and distribution are verified if in scope.
- Remaining limitations are visible and accurate.

**State:** Not started.

## Initial performance budgets

These are pre-spike targets, not measurements.

| Area | Slice budget | Measurement boundary |
|---|---:|---|
| Display | 60 FPS, 16.67 ms frame | M1 Pro, 2560×1440, medium preset |
| Game-thread simulation | ≤4.0 ms p95 | 400 active units, 20 Hz simulation |
| Render thread + GPU | ≤11.0 ms p95 | Slice map, representative combat/weather |
| Fog/visibility | ≤1.5 ms p95 | 400 units, four teams |
| Path burst | ≤6.0 ms, amortized | 100 simultaneous destinations |
| Resident memory | ≤10 GB after warm-up | Native development game, editor excluded |
| Match command traffic | ≤32 Kbit/s/client average | 2v2; snapshots separate |
| Save checkpoint | ≤250 ms; no UI stall >50 ms | Slice campaign/match state |
| Replay hash | Every 20 ticks, ≤0.25 ms p95 | Authoritative state only |

A 60 FPS editor view is not equivalent to a packaged native result.

## Test and evidence register

| ID | Date | Boundary | Test | Observed result | Claim limit |
|---|---|---|---|---|---|
| ENV-001 | 2026-08-28 | M1 Pro 10CPU/16GPU, 16 GB, macOS 26.6.2 | Hardware/tool inventory | Metal 4; Xcode 26.6 installed | Environment only; no UE performance result |
| ENV-002 | 2026-08-28 | Internal APFS SSD | Initial storage inspection | About 38 GB initially free; Docker-managed data about 65 GB was the largest review target | Point-in-time inspection only; no data was removed during this inspection, and the later authorized cleanup is recorded separately in STORAGE-001 |
| INSTALL-001 | 2026-08-28 | Epic Games Launcher | UE 5.8.2 manifest and installed binary inspection | `/Users/Shared/Epic Games/UE_5.8`; version `5.8.2-56702186+++UE5+Release-5.8-Mac`; 45,344,581,313 bytes; `bIsIncompleteInstall=false`; editor has arm64 and x86_64 slices | Launcher completion and binary architecture only; nested signature check and project runtime are separate |
| DOWNLOAD-001 | 2026-08-28 | Official Epic DMG | `hdiutil verify` + SHA-256 | Valid image; SHA-256 `5c4f204ed623b01890f26cc99d4af657c3fbd6be1d04be7fed176ddbc94b1259` | Download integrity only |
| RECOVERY-001 | 2026-08-28 | Local workspace | User deletion and explicit remake instruction | Game repository recreated at the original path | Original Git/filesystem history was lost; unrelated deleted projects were not recreated |
| STORAGE-001 | 2026-08-28 | Docker builder cache | User-authorized `docker builder prune --all --force` | 39.47 GB of rebuildable build cache reclaimed; free space subsequently observed above 77 GiB | Command targeted builder cache only; future Docker builds may re-download/rebuild layers |
| STORAGE-002 | 2026-08-28 | Echoes-only Xcode derived data and generated workspace index | Closed Xcode, stopped the project's indexing workers, and removed only the two identified Echoes DerivedData directories plus `Intermediate/ProjectFiles/build` | Free space recovered from about 51 GiB during indexing to about 83 GiB after cleanup and verification | Rebuildable project index data only; source, authored assets, packages, unrelated Xcode data, and Docker data were not touched; Xcode may recreate indexes when the generated workspace is reopened |
| SIM-001 | 2026-08-28 | Commit `adad96e`; Apple Clang 21; arm64 | `./Scripts/test_sim.sh`, C++20, `-Wall -Wextra -Wpedantic -Werror` | 7/7 suites passed: fixed tick movement; canonical ordering/determinism; economy/build/placement; combat; fog/non-cheating AI; all Well choices; snapshot/replay | Engine-independent core only; no Unreal, navigation-scale, UI, multiplayer-transport, or performance validation |
| SIM-002 | 2026-08-28 | Commit `adad96e`; Apple Clang 21; arm64 | AddressSanitizer + UndefinedBehaviorSanitizer build of the same native suite | 7/7 suites passed; no sanitizer finding observed | Covered test paths only; not a proof that all inputs are memory-safe |
| TOOLCHAIN-001 | 2026-08-28 | Xcode 26.6 selected per command | `xcodebuild -showComponent MetalToolchain`; `xcrun metal -v` | Metal Toolchain build `17F109`, identifier `com.apple.dt.toolchain.Metal.32023.883`, installed and resolvable | Component availability only; Xcode 26.6 remains outside Epic's recommended 26.1.1 baseline |
| GEN-001 | 2026-08-28 | UE 5.8.2 + Xcode 26.6 | `./Scripts/generate_project_files.sh` after UE 5.8 target/config correction | Xcode workspace generation succeeded for game and editor targets | Generation only; Epic installation emits a missing MetalShaderConverter include-directory warning; later build/runtime evidence is recorded separately |
| ENV-003 | 2026-08-28 | Current local host | `xcode-select -p`, storage observation after the current package, test evidence, staging cleanup, and Echoes-only Xcode index cleanup | Full Xcode selected at `/Applications/Xcode.app/Contents/Developer`; about 82 GiB free | Point-in-time local state after final verification; above the 40 GiB prototype stop threshold and 60 GiB packaging headroom, below the 100 GiB sustained-production target; no clean-machine result |
| SIGNATURE-001 | 2026-08-28 | Installed UE 5.8.2 tree | `codesign --verify --deep --strict` | Nonzero exit; nested `libsteam_api.dylib` reported modified or invalid | Distribution-integrity warning; launcher verification, project compilation, and runtime boot still succeeded |
| SIM-003 | 2026-08-28 | Current integrated native tree; Apple Clang 21; arm64 | Optimized strict, debug strict, and AddressSanitizer + UndefinedBehaviorSanitizer strict runs | 10/10 suites passed in all three configurations, including numeric/public-input hardening, sequence/build hardening, and adversarial snapshot/ID bounds | Covered native paths only; macOS leak detection unavailable; no Unreal rendering, transport, or performance conclusion |
| BUILD-001 | 2026-08-28 | UE 5.8.2, Xcode 26.6, Mac Development, arm64 | `./Scripts/build_editor.sh` with hot reload disabled | `EchoesOfTheBrokenSunEditor` compiled and linked; project and simulation-core editor dylibs plus target receipt produced | Local incremental Development Editor build; not a cook, package, clean-machine build, or warning-free claim |
| AUTO-001 | 2026-08-28 | MacEditor, null RHI | `./Scripts/run_unreal_tests.sh` | `Echoes.Runtime.Bootstrap.ClassesAndCore`: 1 succeeded, 0 failed, 0 warnings/errors | Registers five Unreal classes and advances a small portable simulation one tick; not a gameplay or rendered test |
| AUTO-002 | 2026-08-28 | Current integrated MacEditor tree, temporary game world, null RHI | `./Scripts/run_unreal_tests.sh` | 2/2 succeeded with 0 failed and 0 warnings/errors; `Echoes.Runtime.Visibility.ActorLifecycle` additionally proved view creation on reveal, destruction when hidden, and exactly one distinct actor on reentry | Real subsystem and actor lifecycle at prototype scale; not a rendered fog surface, long soak, multiplayer, or performance test |
| RUNTIME-001 | 2026-08-28 | `/Engine/Maps/Entry`, Mac Development Editor, null RHI | `./Scripts/run_runtime_smoke.sh` | arm64 modules loaded; `EchoesGameMode` selected; 23-entity scenario initialized with 9 visible views at 20 Hz; environment, simulation, boot-ready, and first-fixed-tick markers emitted; benchmark exited 0 | Bootstrap only; no rendered output, manual input, sustained correctness, gameplay completion, or performance conclusion |
| RENDER-001 | 2026-08-28 | Local M1 Pro, Metal SM5 Development Editor runtime | Rendered arena inspection plus keyboard input | HUD, cyan local units, orange matter, home platform, placeholder geometry, and `RUNTIME TECHNICAL PROTOTYPE` label observed; `2` changed the Future Well protocol to Preserve and was logged | Rendered startup and one keyboard command only; desktop automation could not reliably inject pointer gestures, and no final art, complete controls, true fog/shroud surface, or performance conclusion is claimed |
| SIM-004 | 2026-08-28 | Current integrated native tree; snapshot/replay version 3; Apple Clang 21; arm64 | Optimized strict, debug strict, and AddressSanitizer + UndefinedBehaviorSanitizer strict runs | 11/11 suites passed in all three configurations; new coverage proves timed worker/soldier production, logistics reservation/capacity, active-queue snapshot continuity, and Command-Core victory | Covered engine-independent paths only; no rendered full-match, navigation-scale, multiplayer, or performance conclusion |
| AUTO-003 | 2026-08-28 | Current MacEditor tree; temporary game worlds; null RHI | `./Scripts/run_unreal_tests.sh` | 3/3 succeeded with 0 failed and 0 warnings/errors; new gameplay test proved pause, worker/soldier production, logistics accounting, deterministic restart, and required hotkey mappings through real Unreal integration | Automated subsystem-scale result; not a manual full match, pointer-input acceptance, stress, or clean-machine result |
| RUNTIME-002 | 2026-08-28 | `/Engine/Maps/Entry`, current Mac Development Editor, null RHI | `./Scripts/run_runtime_smoke.sh` | 25-entity scenario initialized with 10 visible views at 20 Hz; all four readiness markers emitted and process exited 0 | Bootstrap only; no rendered interaction, sustained correctness, gameplay completion, or performance conclusion |
| RENDER-002 | 2026-08-28 | Current local M1 Pro, Metal SM5 Development Editor runtime | Rendered HUD inspection and keyboard pause/restart | Current active-development HUD, resources, logistics `9/12`, construction/production controls, and match state observed; `P` froze the deterministic tick with `PAUSED`; `R` restored initial state and active ticking | Pointer injection again failed, so selection, placement, production selection, context orders, and combat remain unaccepted manually; no performance or final-art conclusion |
| SIM-005 | 2026-08-28 | Current integrated native tree; Apple Clang 21; arm64 | Optimized strict, debug strict, and AddressSanitizer + UndefinedBehaviorSanitizer strict runs, including an 11-tile wall detour | 12/12 suites passed in all three configurations; the unit reached the destination without entering blocked terrain and a duplicate simulation produced the same checksum | Prototype-scale breadth-first routing only; moving-unit avoidance, path caching, large-map stress, formation movement, and frame-time qualification remain open |
| AUTO-004 | 2026-08-28 | Current MacEditor tree; temporary game world; null RHI | Enhanced `Echoes.Runtime.Visibility.ActorLifecycle` plus the complete `Echoes` automation set | 3/3 succeeded with 0 failed and 0 warnings/errors; fog view accounted for all 4,096 tiles, initial visible/unexplored states, exploration growth, visible-to-shrouded transition, hidden actor removal, and reentry recreation | Lifecycle and state-boundary result; no rendered quality, production-scale performance, manual scouting, or final-art conclusion |
| RUNTIME-003 | 2026-08-28 | `/Engine/Maps/Entry`, current Mac Development Editor, null RHI | `./Scripts/run_runtime_smoke.sh` | Fog readiness reported 4,096 tiles: 272 visible, 0 explored, 3,824 unexplored; 25-entity/10-view scenario and first fixed tick initialized; process exited 0 | Bootstrap only; no rendered interaction, exploration movement, sustained cost, or performance conclusion |
| RENDER-003 | 2026-08-28 | Current local M1 Pro, Metal SM5 Development Editor runtime | Rendered visibility-surface inspection and keyboard pause | A tile-instanced boundary visibly separated the local known area from unrevealed space; friendly units and the HUD remained readable; pause froze tick 780 with feedback | Visual inspection of the placeholder surface only; color/material polish, explored-shroud transition under manual scouting, pointer input, full match, and performance remain unaccepted |
| PKG-001 | 2026-08-28 | Clean source commit `2c8ddba1d74c38d183f0af7df62d3b54fea4e4e2`; local M1 Pro; UE 5.8.2; Xcode 26.6; Mac Development arm64 | Unreal build/cook/stage/PAK/package/archive, structural inspection, local ad-hoc reseal, strict deep verification, content manifest, digest verification, and `./Scripts/run_packaged_smoke.sh` | 748 MB self-contained `.app`; bundle `com.angelispseftis.echoesofthebrokensun`; short version `0.1.0`; five cooked container files; native arm64 executable; corrected signature valid; packaged executable emitted all four startup markers and exited 0; manifest SHA-256 `97ba923b912a4d0ad4cb9c9318d0a3f6cbd64daf3b0c69615e258a5458395e9e` | Local Development artifact only; ad-hoc signature is not Developer ID signing; no notarization, clean-machine launch, installer, rendered packaged interaction, performance, universal binary, or release conclusion |
| PKG-002 | 2026-08-28 | Clean source commit `21469a4f702ff284c85104c97aa9b85f63fc2b6e`; local M1 Pro; UE 5.8.2; Xcode 26.6; Mac Development arm64 | Full rebuild/cook/stage/PAK/package/archive; version, structure, architecture, strict signature, content-manifest digest, and packaged runtime checks | 748 MB self-contained `.app`; bundle `com.angelispseftis.echoesofthebrokensun`; short version `0.2.0`; five cooked container files; native arm64 executable; local ad-hoc signature valid; 25-entity/10-view packaged startup emitted all four markers and exited 0; manifest SHA-256 `482b0e7e5db955675d07c9edd82da12114076c057250c243744432a095c49269` | Historical playable-systems Development artifact superseded by PKG-003; not Developer ID signed or notarized; no clean-machine, installer, rendered packaged interaction, full-match, performance, universal-binary, or release conclusion |
| PKG-003 | 2026-08-28 | Clean source commit `08f1babd45fea9168ba237deed965fb4f3215594`; local M1 Pro; UE 5.8.2; Xcode 26.6; Mac Development arm64 | Full rebuild/cook/stage/PAK/package/archive; version, structure, architecture, strict signature, content-manifest digest, and packaged runtime with fog checks | 748 MB self-contained `.app`; bundle `com.angelispseftis.echoesofthebrokensun`; short version `0.3.0`; five cooked container files; native arm64 executable; local ad-hoc signature valid; 4,096-tile fog plus 25-entity/10-view startup emitted all required markers and exited 0; manifest SHA-256 `b44eb98f61ec86142ffba88912c90df1eea830b34039c5a47145dc56cdbed47b` | Current navigation/fog Development artifact; not Developer ID signed or notarized; no clean-machine, installer, rendered packaged interaction, full-match, performance, universal-binary, or release conclusion |

## Risks

| ID | Risk | Likelihood / impact | Control |
|---|---|---|---|
| R-001 | Derived data, assets, builds, or packaging exhaust remaining disk | High / Critical | Stop prototype builds below 40 GiB; restore ≥60 GiB before large imports or release packaging; target ≥100 GiB sustained-production headroom |
| R-002 | UE 5.8 cook/package/rendered workloads fail with Xcode 26.6 | Medium / High | Retain limited positive evidence; install recommended 26.1.1 side by side if a toolchain issue appears or support alignment is required |
| R-003 | M1 Pro/16 GB performance is inadequate | High / High | Disable unsupported features, measure early, reopen D-001 if budgets fail |
| R-004 | Unit pathing/fog saturates game thread | High / High | Batched paths, spatial partitioning, scheduled visibility, flow-field spike |
| R-005 | Unreal state leaks into authoritative outcomes | Medium / Critical | One-way adapters, deterministic tests, replay hashes, review |
| R-006 | Reshape strands units nondeterministically | Medium / High | Authored transitions/fallbacks, expiry warnings, adversarial tests |
| R-007 | AI gains hidden engine information | Medium / High | AI consumes player-view snapshots only; disclose assisted modifiers |
| R-008 | Host manipulation or disconnect stalls matches | Medium / High | Validate commands, snapshots/hashes, reconnect, server option |
| R-009 | Art scope outruns engineering proof | High / High | Placeholder labels, silhouette gate, rights register, performance gate |
| R-010 | Campaign branching exceeds capacity | High / High | Bounded consequence variables and branch budget after slice |
| R-011 | Asset rights are incomplete | Medium / Critical | No distribution without retained license/assignment evidence |
| R-012 | Menus/docs imply incomplete modes work | Medium / High | Feature flags off, acceptance-led status, evidence review |
| R-013 | Workspace cleanup removes authoritative work | Medium / Critical | Current Git history, scoped cleanup, verify exact targets before deletion; remote backup requires separate authorization |

## Known limitations

- Unreal Engine installation and the Metal Toolchain are present; Xcode 26.6 has limited positive local evidence but is not Epic's recommended 26.1.1 baseline.
- The Unreal modules compile, link, load, bootstrap, and render a runtime-generated scenario; only rendered startup and one keyboard command have been accepted, not complete manual interaction.
- Camera, selection, context-order, construction/production hotkeys, pause/restart, HUD, entity-view, arena, lighting, and placeholder fog/shroud presentation code exists. Views are scoped to currently visible entities and explored terrain persists in the presentation, but final fog art and exact-build pointer selection, placement, production, and context orders remain unverified.
- Source JSON exists, but automated Unreal import/schema validation is not implemented.
- The core currently uses technical-spike fixture statistics rather than loading the source JSON; those values are not approved balance data and may diverge until the content compiler is implemented.
- Native simulation does not prove engine integration, player experience, visual fidelity, or performance.
- No final art/audio asset is registered.
- Campaign exists only as design.
- Skirmish UI, save/load UI, settings, and accessibility controls are not implemented.
- Multiplayer transport, separate-process execution, reconnect, spectators, and separate-machine tests do not exist.
- A self-contained local Mac Development package exists and has a verified ad-hoc signature seal; no Developer ID signature, notarization, installer, clean-machine build/install/launch, or packaged rendered-interaction test exists.
- No claim is made about 60 FPS, supported Mac range, App Store readiness, commercial readiness, or completion.

## Immediate next task

Complete pointer-driven selection, placement, production, combat, victory, and restart validation as one rendered match. Then profile obstacle routing and fog/shroud at the target unit and map scale before expanding visual content.
