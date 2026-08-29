---
title: Echoes of the Broken Sun Setup and Build Guide
author: Angelis Pseftis
creator: Angelis Pseftis
status: Authoritative
created: 2026-08-28
updated: 2026-08-28
---

# Setup and Build Guide

This is the single authoritative setup and build guide. Edit it in place; do not create version-copy guides.

## Verified host state

The initial development host is a MacBook Pro (`MacBookPro18,1`) with an Apple M1 Pro, 10 CPU cores, 16 GPU cores, 16 GB unified memory, Metal 4, and macOS 26.6.2. Xcode 26.6 is installed and `xcode-select -p` currently resolves to `/Applications/Xcode.app/Contents/Developer`. Apple's separately delivered Metal Toolchain build `17F109` is installed and `xcrun metal` resolves. Epic Games Launcher 20.2.4 completed Unreal Engine 5.8.2 after an authorized Docker builder-cache cleanup recovered 39.47 GB of rebuildable space. About 82 GiB remained free after retaining the current 0.3.0 Mac Development package and removing rebuildable staging and Echoes-only Xcode index data at the latest observation.

The official Epic installer endpoint was downloaded and the disk image verified. The retained file is `~/Downloads/EpicInstaller-20.1.4.dmg`, 114,676,675 bytes, SHA-256 `5c4f204ed623b01890f26cc99d4af657c3fbd6be1d04be7fed176ddbc94b1259`. Because the installed launcher is newer, do not install this backup over it unless repairing the launcher becomes necessary.

## Supported production baseline

The project pins Unreal Engine 5.8. Epic's version-specific macOS table lists Xcode 26.0 as the minimum and 26.1.1 as recommended, and explicitly marks 26.4 incompatible. The locally installed Xcode 26.6 has passed project generation, arm64 Development Editor compilation, four Unreal automation tests, null-RHI and rendered startup, and a local Development cook/package. That is positive evidence on this host, not a clean-machine, Developer ID signing, notarization, performance, or full compatibility result. Xcode 26.1.1 remains Epic's recommended support baseline.

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

The script compiles with Apple Clang, writes to a temporary directory beneath `${TMPDIR:-/tmp}`, removes that directory on exit, and runs optimized, debug, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. All three current configurations pass 13/13 suites. The added coverage includes timed worker/soldier production, logistics capacity, active-queue snapshot/replay persistence, Command-Core victory, deterministic routing around an otherwise blocking terrain wall, and attack-move acquisition, objective resumption, and stop interruption. Those results validate only the named engine-independent paths; they do not validate navigation at production scale, Unreal rendering, multiplayer transport, packaging, or every hostile input. LeakSanitizer is disabled because it is unavailable in Apple's macOS AddressSanitizer runtime.

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
```

The observed current results are: workspace generation succeeded; `EchoesOfTheBrokenSunEditor Mac Development` compiled and linked for arm64; four Unreal automation tests passed 4/4 with no test warning or error; and the null-RHI game smoke selected `EchoesGameMode`, created a 25-entity scenario with 10 initially visible entity views and a 4,096-tile fog/shroud surface at 20 Hz, emitted the environment/fog/simulation/boot-ready/first-tick markers, and exited cleanly. `Echoes.Runtime.Bootstrap.ClassesAndCore` covers class registration, required input mappings—including `[F]` attack-move and `[X]` stop—and a one-tick portable-core call. `Echoes.Runtime.Gameplay.ProductionPauseRestart` uses the real world subsystem to prove pause, worker and soldier production, logistics accounting, and deterministic restart. `Echoes.Runtime.Gameplay.CompleteSkirmish` proves one composed automated match: drop-off construction, legitimate Future Well scouting and capture, reinforcement production, group rally, one-order attack-move engagement, opposing Command-Core destruction, post-victory simulation freeze, and restart. `Echoes.Runtime.Visibility.ActorLifecycle` proves fog tile accounting, explored-area growth, visible-to-shrouded transition, actor creation on reveal, destruction when hidden, and single-actor recreation on reentry. The null-RHI smoke covers bootstrap only. These results do not validate a pointer-driven rendered match, sustained simulation, production-scale navigation/fog performance, broader gameplay completeness, or clean-machine installation.

## Rendered prototype boundary

Local Metal SM5 editor runs were visually inspected at the runtime-generated arena. The current HUD displays resources, logistics, match state, construction/production hotkeys, Well choices, and the `PLAYABLE SYSTEMS BUILD — ACTIVE DEVELOPMENT` boundary. The tile-instanced fog surface visibly separates the local known area from unrevealed territory without hiding friendly units or the HUD. Pause was observed to freeze the deterministic tick with visible feedback, and restart restored tick zero, initial resources, active state, and the initial logistics count. A prior run also recorded the `2` key changing the Future Well protocol to Preserve. The desktop automation bridge still cannot reliably deliver pointer clicks or drag gestures into the Metal window, so exact-build selection, construction placement, context movement, gathering, production selection, and combat remain unaccepted as manual interactions. These are rendered startup and bounded keyboard observations, not evidence of a complete playable loop, final fog art, or performance.

## Package and smoke-test macOS

The package wrapper requires at least 60 GiB free, creates a new archive rather than mixing with an existing one, performs Unreal build/cook/stage/PAK/package/archive, validates the arm64 binary, bundle identity, version, and cooked containers, applies a local ad-hoc signature to the completed archive, verifies the signature seal, launches a three-second null-RHI smoke test, and writes a SHA-256 content manifest with the source commit, clean/dirty state, configuration, engine, and Xcode identity:

```sh
./Scripts/package_macos.sh
```

An explicit new archive path may be supplied as the first argument. Existing targets are refused and left untouched. A previously built package can be checked independently:

```sh
./Scripts/run_packaged_smoke.sh \
  BuildArtifacts/Packages/Mac-Development-v0.3.0-navigation-fog/EchoesOfTheBrokenSun.app
```

The current observed local artifact is a 748 MB self-contained arm64 Development application with bundle identifier `com.angelispseftis.echoesofthebrokensun`, short version `0.3.0`, and five PAK/IoStore container files. Unreal's Xcode packaging phase seals the app before the final PAK update, so the wrapper reseals the completed archive with the checked-in development entitlements and verifies it afterward. The exact archive passed strict deep-signature verification and emitted the environment, 4,096-tile fog, simulation, boot-ready, and first-tick markers when its packaged executable was launched. Its adjacent manifest records clean source commit `08f1babd45fea9168ba237deed965fb4f3215594`, UE 5.8.2, Xcode 26.6, configuration and platform identity, and the SHA-256 of every regular file and symlink in the app. The manifest digest is `b44eb98f61ec86142ffba88912c90df1eea830b34039c5a47145dc56cdbed47b`.

## Distribution boundary

A free Apple Account is enough for local Xcode development. The current package is signed only ad hoc so it can be validated locally; that is not an identity-bearing distribution signature. Developer ID signing, notarization, and Mac App Store distribution require the appropriate Apple Developer Program credentials. No Developer ID signing, notarization, App Store readiness, or compatibility beyond the tested host may be claimed until observed and recorded.
