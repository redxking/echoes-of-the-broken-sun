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

The project is in **Milestone 1 — production baseline and deterministic simulation spike**. An isolated Git repository, UE 5.8 project descriptor, initial Mac settings, source balance data, and authoritative design records exist. The engine-independent C++ simulation and tests are under construction. Unreal Engine 5.8.2 reached about 64% installation before it was suspended to prevent the disk from filling. No Unreal project build, editor launch, playable Unreal map, packaged application, performance result, multiplayer result, signing, notarization, or distribution state has been demonstrated.

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

**State:** Substantially complete. Installation and storage remain unresolved.

### M1 — Production baseline and simulation spike

- Isolated repository without credentials or machine-specific build paths.
- Authoritative design, architecture, setup, evidence, and asset records.
- Data definitions for both slice factions and Future Wells.
- Standard C++20 core builds with Apple Clang without Unreal.
- Tests prove deterministic command ordering/checksums, economy, placement/construction, combat, visibility, all Well modes, bounded AI inputs, replay, and versioned snapshot behavior.
- Failures return stable reason identifiers.

**State:** In progress. Acceptance requires completed test output and source review below.

### M2 — Unreal technical prototype

- UE 5.8 C++ editor target builds and launches natively on arm64.
- A prototype map exposes camera, box selection, context orders, pathing, economy, construction, production, combat, fog, victory, and restart.
- Unreal remains a view/controller over the tested simulation; frame rate does not change checksums.
- Stress scene records CPU, GPU, memory, navigation, and game-thread measurements.
- Menus do not advertise unavailable modes.

**State:** Not started; engine installation and toolchain validation are prerequisites.

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
| ENV-001 | 2026-08-28 | M1 Pro 10CPU/16GPU, 16 GB, macOS 26.6.2 | Hardware/tool inventory | Metal 4; Xcode 26.6 installed; active path was CLT; separately delivered Metal Toolchain uninstalled | Environment only; no UE performance result |
| ENV-002 | 2026-08-28 | Internal APFS SSD | Storage inspection | About 38 GB initially free; Docker-managed data about 65 GB was the largest review target | Point-in-time; no data removed by the agent |
| INSTALL-001 | 2026-08-28 | Epic Games Launcher | UE 5.8.2 observation | Target `/Users/Shared/Epic Games/UE_5.8`; manifest final size 45,344,581,313 bytes; about 64% before safe suspension as free space reached 11–14 GiB | Paused/incomplete, not installed or verified |
| DOWNLOAD-001 | 2026-08-28 | Official Epic DMG | `hdiutil verify` + SHA-256 | Valid image; SHA-256 `5c4f204ed623b01890f26cc99d4af657c3fbd6be1d04be7fed176ddbc94b1259` | Download integrity only |
| RECOVERY-001 | 2026-08-28 | Local workspace | User deletion and explicit remake instruction | Game repository recreated at the original path | Original Git/filesystem history was lost; unrelated deleted projects were not recreated |

## Risks

| ID | Risk | Likelihood / impact | Control |
|---|---|---|---|
| R-001 | Engine install exhausts disk | High / Critical | Resume only with ≥60 GB free; target ≥100 GB working headroom |
| R-002 | UE 5.8 fails with Xcode 26.6 | Medium / High | Install pinned 26.1.1 side by side; 26.6 is unverified, not proven incompatible |
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

- Unreal Engine installation and supported Xcode/Metal toolchain are not accepted.
- The Unreal module has not been compiled, linked, opened, or run.
- No map, camera, presentation, navigation adapter, UI, effect, or audio has been observed in engine.
- Source JSON exists, but automated Unreal import/schema validation is not implemented.
- Native simulation does not prove engine integration, player experience, visual fidelity, or performance.
- No final art/audio asset is registered.
- Campaign exists only as design.
- Skirmish UI, save/load UI, settings, and accessibility controls are not implemented.
- Multiplayer transport, separate-process execution, reconnect, spectators, and separate-machine tests do not exist.
- No packaged app, clean build, signing, notarization, or installation test exists.
- No claim is made about 60 FPS, supported Mac range, App Store readiness, commercial readiness, or completion.

## Immediate next task

Complete the native simulation tests and evidence review while storage is resolved. Then resume UE 5.8.2, build the editor target, correct API issues against the installed hotfix, and create the minimum generated prototype map before adding visual content.
