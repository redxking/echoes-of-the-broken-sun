---
title: Echoes of the Broken Sun Setup and Build Guide
author: Angelis Pseftis
creator: Angelis Pseftis
status: Authoritative
created: 2026-08-28
updated: 2026-08-29
---

# Setup and Build Guide

This is the single authoritative setup and build guide. Edit it in place; do not create version-copy guides.

## Verified host state

The initial development host is a MacBook Pro (`MacBookPro18,1`) with an Apple M1 Pro, 10 CPU cores, 16 GPU cores, 16 GB unified memory, Metal 4, and macOS 26.6.2. Xcode 26.6 is installed and `xcode-select -p` currently resolves to `/Applications/Xcode.app/Contents/Developer`. Apple's separately delivered Metal Toolchain build `17F109` is installed and `xcrun metal` resolves. Epic Games Launcher 20.2.4 completed Unreal Engine 5.8.2 after an authorized Docker builder-cache cleanup recovered 39.47 GB of rebuildable space. About 72 GiB remained free after retaining the accepted development packages at the latest observation. The exact 1.1 GB 0.13.0, 0.14.0, and 0.15.0 rebuildable staging trees were moved to `/Users/angelispseftis/.Trash/Echoes-StagedBuilds-Mac-20260829T034623Z`, `/Users/angelispseftis/.Trash/Echoes-StagedBuilds-Mac-20260829T035936Z`, and `/Users/angelispseftis/.Trash/Echoes-StagedBuilds-Mac-20260829T041257Z`; all remain recoverable and continue to occupy space until Trash is emptied.

The official Epic installer endpoint was downloaded and the disk image verified. The retained file is `~/Downloads/EpicInstaller-20.1.4.dmg`, 114,676,675 bytes, SHA-256 `5c4f204ed623b01890f26cc99d4af657c3fbd6be1d04be7fed176ddbc94b1259`. Because the installed launcher is newer, do not install this backup over it unless repairing the launcher becomes necessary.

## Supported production baseline

The project pins Unreal Engine 5.8. Epic's version-specific macOS table lists Xcode 26.0 as the minimum and 26.1.1 as recommended, and explicitly marks 26.4 incompatible. The locally installed Xcode 26.6 has passed project generation, arm64 Development Editor compilation, eleven Unreal automation tests, null-RHI and rendered startup, and a local Development cook/package. That is positive evidence on this host, not a clean-machine, Developer ID signing, notarization, performance, or full compatibility result. Xcode 26.1.1 remains Epic's recommended support baseline.

Epic recommends Apple Silicon M3 and 32 GB for UE 5.8. The inspected M1 Pro and 16 GB configuration meets Epic's lower hardware tier for supported rendering features, but it is below the recommendation. Nanite and Virtual Shadow Maps are disabled because Epic limits them to M2 or newer on Mac and still labels that support beta. Lumen software ray tracing may be evaluated; hardware ray tracing and MegaLights are not Mac targets.

Primary references:

- [Unreal Engine 5.8 release](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available)
- [Epic macOS development requirements](https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine?lang=en-US)
- [Epic Apple Silicon and universal binaries guidance](https://dev.epicgames.com/documentation/unreal-engine/supporting-universal-binaries-for-macos-in-unreal-engine?lang=en-US)
- [Epic installation workflow](https://dev.epicgames.com/documentation/en-us/unreal-engine/install-unreal-engine)
- [Apple Xcode system requirements](https://developer.apple.com/xcode/system-requirements)
- [Apple Xcode resources](https://developer.apple.com/xcode/resources/)

## Environment setup and residual gate

### 1. Create storage headroom

The environment check stops prototype builds below 40 GiB free, warns below the preferred 60 GiB headroom, and treats 100 GiB as the safer sustained-production target. Restore at least 60 GiB before large imports or release packaging, or use a fast APFS external development volume. These thresholds are project engineering recommendations intended to avoid build and packaging failures, not Epic official minimums.

The largest inspected local storage area was Docker's managed data, about 65 GB. With explicit user authorization, only Docker builder cache was pruned; the command reported 39.47 GB reclaimed. Images, containers, and volumes were not targeted. Do not delete `~/Library/Containers/com.docker.docker` manually; volumes may contain user data. Application Support and caches require application-specific review rather than wholesale deletion.

### 2. Select the Xcode toolchain

The installed Xcode 26.6 and Metal Toolchain are being used for the technical spike. The local generation, compilation, automation, rendering, cooking, packaging, and startup evidence described above is positive, but Epic's supported recommendation remains Xcode 26.1.1. If later tests expose a toolchain issue—or a strictly support-aligned baseline is required—obtain 26.1.1 from Apple's authenticated developer downloads and install it alongside 26.6.

For project-local commands, avoid silently changing every developer tool on the Mac:

```sh
export ECHOES_XCODE="/Applications/Xcode.app/Contents/Developer"
export DEVELOPER_DIR="$ECHOES_XCODE"
xcodebuild -version
xcrun metal -v
```

If Unreal Launcher or Editor does not honor that environment, select the same Xcode under Xcode Settings → Locations → Command Line Tools. A system-wide `xcode-select` change affects other projects and should be deliberate.

### 3. Verify Unreal Engine 5.8.2

The completed install is at `/Users/Shared/Epic Games/UE_5.8`. The launcher manifest reports version `5.8.2-56702186+++UE5+Release-5.8-Mac`, `bIsIncompleteInstall=false`, and `InstallSize=45,344,581,313`; the filesystem occupied about 43 GiB at inspection. The `UnrealEditor` executable contains both arm64 and x86_64 slices. Strict deep code-signature verification exits nonzero because the installed nested `libsteam_api.dylib` is reported modified or invalid. Launcher verification, compilation, and the recorded runtime boot still succeeded, so that signature result is retained as an unresolved distribution-integrity issue rather than treated as proof of runtime failure.

The Launcher installation and sign-in are interactive and may require account verification, license acceptance, and macOS security approval. Those states cannot be inferred from the launcher merely existing.

## Native simulation test

The deterministic core is deliberately testable without Unreal:

```sh
./Scripts/test_sim.sh
```

The script compiles with Apple Clang, writes to a temporary directory beneath `${TMPDIR:-/tmp}`, removes that directory on exit, and runs optimized, debug, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. All three current configurations pass 17/17 suites. Coverage includes four-player visibility and victory, schema-9 four-player snapshot round trips, production, logistics, persistence, attack-move, hold, guard, patrol, deterministic obstacle routing, path-cache invalidation after terrain mutation, Future Wells, replay, and adversarial input bounds. Those results validate only the named engine-independent paths; they do not validate Unreal rendering, multiplayer transport, packaging, or every hostile input. LeakSanitizer is disabled because it is unavailable in Apple's macOS AddressSanitizer runtime.

The isolated native scale profile is run separately so compiler/test contention does not contaminate timing evidence:

```sh
./Scripts/profile_sim.sh
```

It compiles an optimized profile-only harness and writes the ignored machine-readable result to `BuildArtifacts/Performance/native_sim_profile.json`. The current accepted run covers a 64×64 map, 400 units split evenly across four teams, 20 Hz, and 100 path requests. Four-team visibility refresh measured 0.088500 ms p95; 100 cold path requests measured 0.411375 ms p95; the 400-unit tick with 100 moving units measured 0.133583 ms p95; and the schema-9 state checksum measured 0.034102 ms p95. Every implemented native budget passed. This is not an Unreal frame/GPU, rendered-unit, networked-multiplayer, soak, or release profile.

The packaged Metal profile is a separate gate and defaults to the accepted 0.15.0 package. Run both the normal playable fixture and the explicit scale fixture:

```sh
./Scripts/profile_packaged_macos.sh
ECHOES_PROFILE_STRESS400=1 ./Scripts/profile_packaged_macos.sh
```

The wrapper validates the manifested, signed package; requests native 2560×1440 exclusive fullscreen; applies medium scalability groups, native resolution quality, and TAA; enables Unreal CSV GPU statistics; captures 600 frames; discards the first 120 as warm-up; samples process RSS independently; and writes the raw Unreal CSV, runtime log, RSS samples, and machine-readable summary beneath `BuildArtifacts/Performance/Packaged/<UTC timestamp>`. Exclusive fullscreen is intentional: the installed UE 5.8 Metal implementation forces display synchronization for windowed games, which makes raw frame/GPU counters include the 60 Hz presentation wait even when `r.VSync=0`. The normal run covers the 25-entity playable placeholder; `ECHOES_PROFILE_STRESS400=1` requires the exact 400-unit/four-team/401-view readiness marker and records that fixture separately. The latter proves visible-proxy and four-team fog scale at its named boundary, not authored weather/effects, formations, representative broad combat orders, soak stability, or release qualification.

The durability gate runs the exact manifested scale package for 60 minutes in a 1280×720 windowed Metal process, samples RSS and CPU every five seconds, discards the first 120 seconds for steady-state memory analysis, requires the exact scale marker, rejects fatal/GPU/out-of-memory logs, and writes its evidence beneath `BuildArtifacts/Performance/Soak/<UTC timestamp>`:

```sh
./Scripts/soak_packaged_macos.sh
```

The enforced local bounds are survival for the requested duration, peak process RSS no greater than 10 GiB, no more than 64 MiB growth between the first and final steady-state windows, and a fitted steady-state growth rate no greater than 128 MiB/hour. These are project gates for detecting substantial retention on this fixture, not proof of leak freedom or release stability.

## Generate and build the Unreal project

Define the installed engine and full Xcode, then use the repository wrappers:

```sh
export UE_ROOT="/Users/Shared/Epic Games/UE_5.8"
export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"
./Scripts/check_environment.sh
./Scripts/generate_project_files.sh
./Scripts/build_editor.sh
./Scripts/run_unreal_tests.sh
./Scripts/run_runtime_smoke.sh
./Scripts/run_stress_smoke.sh
```

`build_editor.sh` and `package_macos.sh` default to four concurrent Unreal build actions because the engine-selected ten-action default caused severe memory and swap contention on this 16 GB host. Set `ECHOES_MAX_PARALLEL_ACTIONS` to another positive integer only for an intentional host-specific experiment; both wrappers reject invalid values.

The observed current results are: workspace generation succeeded; `EchoesOfTheBrokenSunEditor Mac Development` compiled and linked for arm64; eleven Unreal automation tests passed 11/11 with no test warning or error; and the normal null-RHI game smoke selected `EchoesGameMode`, created a 25-entity scenario with 10 initially visible entity views, the 165-tile Glass Scar layout, three crossings, and a 4,096-tile fog/shroud surface at 20 Hz, emitted every required readiness marker, and exited cleanly. The separate stress smoke created 400 owned units, 100 per active player across four teams, one central Future Well, and 401 visibility-scoped view actors, then emitted `[ECHOES_STRESS_READY] units=400 teams=4 entities=401 visibleViews=401`. The new scale automation independently verifies those counts and the ongoing initial outcome. The complete-skirmish test remains green and verifies that the seven-unit strike force rallies before crossing hostile territory. These checks do not validate pointer-delivered orders in a rendered match, authored representative combat/weather behavior, network transport, soak stability, broader gameplay completeness, or clean-machine installation.

## Rendered prototype boundary

Local Metal SM5 Editor runs and exact packages through 0.14.0 were visually inspected at the runtime-generated arena. Terrain edges, fog boundary, entities, selection/platform presentation, tactical HUD, and feedback surface remained intact without observed clipping or an obvious TAA regression. The HUD displays `[F]` attack-move, `[T]` patrol, `[H]` hold, `[J]` guard, `[X]` stop, construction/production, control groups, checkpoints, and Well choices. The exact 0.15.0 normal and scale packages were independently executed and profiled through Metal, but the desktop capture service resolved the retained 0.14.0 bundle when asked to inspect 0.15.0 because all milestone applications share one bundle identifier. No 0.15.0 screenshot is therefore accepted. Actual patrol/routing/combat semantics remain automation-covered rather than manually accepted. The desktop automation bridge still cannot reliably deliver pointer clicks or drag gestures into the Metal window, so selected-unit patrol/hold, pointer-targeted guard, construction, context movement, gathering, production selection, attack-move delivery, control-group assignment with selected units, and combat remain unaccepted as manual interactions. These are bounded presentation observations, not evidence of a complete rendered playable loop or final terrain/fog/UI art.

## Package and smoke-test macOS

The package wrapper requires at least 60 GiB free, creates a new archive rather than mixing with an existing one, performs Unreal build/cook/stage/PAK/package/archive, validates the arm64 binary, bundle identity, version, and cooked containers, applies a local ad-hoc signature to the completed archive, verifies the signature seal, launches a three-second null-RHI smoke test, and writes a SHA-256 content manifest with the source commit, clean/dirty state, configuration, engine, and Xcode identity:

```sh
./Scripts/package_macos.sh
```

An explicit new archive path may be supplied as the first argument. Existing targets are refused and left untouched. A previously built package can be checked independently:

```sh
./Scripts/run_packaged_smoke.sh \
  BuildArtifacts/Packages/Mac-Development-v0.15.0-four-team-scale/EchoesOfTheBrokenSun.app
```

The current observed local artifact is a 748 MB self-contained arm64 Development application with bundle identifier `com.angelispseftis.echoesofthebrokensun`, short version `0.15.0`, and five PAK/IoStore container files. Unreal's Xcode packaging phase seals the app before the final PAK update, so the wrapper reseals the completed archive with the checked-in development entitlements and verifies it afterward. The exact archive passed strict deep-signature verification and normal packaged startup. Its adjacent manifest records clean source commit `8841176b668a49ebd712c57416458c61db4ce0d1`, UE 5.8.2, Xcode 26.6, configuration and platform identity, and the SHA-256 of every regular file and symlink in the app. The manifest digest is `f4c020be5ffbc83277871c0d3b614b35a4aa381d4c76f920e08e705364356129`. The exact package's normal 25-entity Metal profile passed at 10.471 ms frame, 10.460 ms render-thread, 9.936 ms GPU, 0.888 ms game-thread p95, and 612.328 MiB peak sampled RSS. Its separate 400-unit/four-team profile also passed at 10.490 ms frame, 10.484 ms render-thread, 9.963 ms GPU, 1.205 ms game-thread p95, and 632.906 MiB peak sampled RSS. The exact 1.1 GB staging directory was moved recoverably to Trash after qualification; the accepted archive was preserved.

## Distribution boundary

A free Apple Account is enough for local Xcode development. The current package is signed only ad hoc so it can be validated locally; that is not an identity-bearing distribution signature. Developer ID signing, notarization, and Mac App Store distribution require the appropriate Apple Developer Program credentials. No Developer ID signing, notarization, App Store readiness, or compatibility beyond the tested host may be claimed until observed and recorded.
