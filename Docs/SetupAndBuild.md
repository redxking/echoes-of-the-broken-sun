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

The initial development host is a MacBook Pro (`MacBookPro18,1`) with an Apple M1 Pro, 10 CPU cores, 16 GPU cores, 16 GB unified memory, Metal 4, and macOS 26.6.2. Xcode 26.6 is installed and `xcode-select -p` currently resolves to `/Applications/Xcode.app/Contents/Developer`. Apple's separately delivered Metal Toolchain build `17F109` is installed and `xcrun metal` resolves. Epic Games Launcher 20.2.4 completed Unreal Engine 5.8.2 after an authorized Docker builder-cache cleanup recovered 39.47 GB of rebuildable space. About 60.1 GiB remained free at the 0.31.0 source-evidence checkpoint. That is effectively the packaging floor and lacks a safe transient margin. The exact 1.1 GB 0.23.1 rebuildable staging tree was moved recoverably to `/Users/angelispseftis/.Trash/Echoes-StagedBuilds-Mac-20260829T060555Z`. Older staging trees and superseded archives remain in Trash; they continue to occupy space until Trash is emptied.

The official Epic installer endpoint was downloaded and the disk image verified. The retained file is `~/Downloads/EpicInstaller-20.1.4.dmg`, 114,676,675 bytes, SHA-256 `5c4f204ed623b01890f26cc99d4af657c3fbd6be1d04be7fed176ddbc94b1259`. Because the installed launcher is newer, do not install this backup over it unless repairing the launcher becomes necessary.

## Supported production baseline

The project pins Unreal Engine 5.8. Epic's version-specific macOS table lists Xcode 26.0 as the minimum and 26.1.1 as recommended, and explicitly marks 26.4 incompatible. The locally installed Xcode 26.6 has passed project generation, arm64 Development Editor compilation, thirteen Unreal automation tests, null-RHI and rendered startup, and a local Development cook/package. That is positive evidence on this host, not a clean-machine, Developer ID signing, notarization, performance, or full compatibility result. Xcode 26.1.1 remains Epic's recommended support baseline.

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

The script compiles with Apple Clang, writes to a temporary directory beneath `${TMPDIR:-/tmp}`, removes that directory on exit, and runs optimized, debug, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. All three current configurations pass 27/27 suites. Coverage includes four-player visibility and victory, schema-20 snapshot/replay round trips, authored-rule consumption and persistence, production, logistics, all seven bounded faction mechanics, faction research, attack-move, hold, guard, patrol, deterministic obstacle routing, path-cache invalidation after terrain mutation, Future Wells, replay, and adversarial input bounds. Research coverage includes prerequisites, costs, producer occupancy, current/future-unit effects, Kharuun adaptation interaction, destroyed-producer interruption, no-refund semantics, scoped player-view state, snapshot restoration, and replay equality. Those results validate only the named engine-independent paths; they do not validate Unreal rendering, multiplayer transport, packaging, balance, or every hostile input. LeakSanitizer is disabled because it is unavailable in Apple's macOS AddressSanitizer runtime.

The isolated native scale profile is run separately so compiler/test contention does not contaminate timing evidence:

```sh
./Scripts/profile_sim.sh
```

It compiles an optimized profile-only harness and writes the ignored machine-readable result to `BuildArtifacts/Performance/native_sim_profile.json`. The current accepted run covers a 64×64 map, 400 units split evenly across four teams, 20 Hz, and 100 path requests. Four-team visibility refresh measured 0.088500 ms p95; 100 cold path requests measured 0.411375 ms p95; the 400-unit tick with 100 moving units measured 0.133583 ms p95; and the schema-9 state checksum measured 0.034102 ms p95. Every implemented native budget passed. This is not an Unreal frame/GPU, rendered-unit, networked-multiplayer, soak, or release profile.

The packaged Metal profile is a separate gate and defaults to the accepted 0.23.1 package. Run both the normal playable fixture and the explicit active-combat scale fixture:

```sh
./Scripts/profile_packaged_macos.sh
ECHOES_PROFILE_STRESS400=1 ./Scripts/profile_packaged_macos.sh
```

The wrapper validates the manifested, signed package; requests native 2560×1440 exclusive fullscreen; applies medium scalability groups, native resolution quality, and TAA; enables Unreal CSV GPU statistics; captures 600 frames; discards the first 120 as warm-up; samples process RSS independently; and writes the raw Unreal CSV, runtime log, RSS samples, and machine-readable summary beneath `BuildArtifacts/Performance/Packaged/<UTC timestamp>`. Exclusive fullscreen is intentional: the installed UE 5.8 Metal implementation forces display synchronization for windowed games, which makes raw frame/GPU counters include the 60 Hz presentation wait even when `r.VSync=0`. The normal run covers the 25-entity playable placeholder. `ECHOES_PROFILE_STRESS400=1` requires exact 400-unit/four-team/401-view readiness, 396 broad-order, and nonzero-damage markers. That fixture includes actual combat, placeholder health feedback, reduced-flashing-aware pulses, lightweight atmosphere, and the tactical overview; it still omits final effects and formations and is not soak, clean-machine, or release qualification.

The durability gate runs the exact manifested scale package for 60 minutes in a 1280×720 windowed Metal process, samples RSS and CPU every five seconds, discards the first 120 seconds for steady-state memory analysis, requires the exact scale marker, rejects fatal/GPU/out-of-memory logs, and writes its evidence beneath `BuildArtifacts/Performance/Soak/<UTC timestamp>`:

```sh
./Scripts/soak_packaged_macos.sh
```

The enforced local bounds are survival for the requested duration, peak process RSS no greater than 10 GiB, no more than 64 MiB growth between the first and final steady-state windows, and a fitted steady-state growth rate no greater than 128 MiB/hour. These are project gates for detecting substantial retention on this fixture, not proof of leak freedom or release stability. The accepted 60-minute observation remains the exact 0.15.0 stationary proxy package: it survived, peaked at 633.078 MiB RSS, and its steady-state windows decreased rather than grew. Its raw log and samples were preserved, but the wrapper's final reporting process was disrupted because the script was edited while running; the checked-in reporting block was then reapplied to those exact preserved inputs. Concurrent builds contaminated CPU sampling, so no CPU conclusion is accepted. The result does not qualify 0.23.1 or its active-combat workload; the script now defaults to 0.23.1 for the next uncontended durability run.

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
./Scripts/run_research_presentation_smoke.sh
./Scripts/run_research_interruption_smoke.sh
./Scripts/run_stress_smoke.sh
```

Run the alternate local-faction bootstrap gate with `ECHOES_LOCAL_FACTION=Kharuun ECHOES_RUNTIME_LOG="$PWD/BuildArtifacts/KharuunRuntimeSmoke.log" ./Scripts/run_runtime_smoke.sh`. This command-line selector exists for unattended evidence; the player-visible path is Tab on the title or operations brief.

`run_research_presentation_smoke.sh` is a Development-only completion fixture. It supplies controlled local resources, queues the first faction technology through the ordinary deterministic command path, and requires ready, active, and complete markers. `run_research_interruption_smoke.sh` adds a third controlled player with 32 ordinary combat units, schedules ordinary attack commands against the producer, and requires active, interrupted, no-refund, and automatically presented archive markers while rejecting completion. `ECHOES_LOCAL_FACTION=Kharuun` exercises the other local faction. Both command-line modes are compiled out of Shipping behavior and must not be described as ordinary player progression or release behavior.

`build_editor.sh` and `package_macos.sh` default to four concurrent Unreal build actions because the engine-selected ten-action default caused severe memory and swap contention on this 16 GB host. Set `ECHOES_MAX_PARALLEL_ACTIONS` to another positive integer only for an intentional host-specific experiment; both wrappers reject invalid values.

The observed current 0.44.0 source results are: `EchoesOfTheBrokenSunEditor Mac Development` compiled and linked for arm64; twenty-seven adversarial content-compiler tests passed; the native core passed 27/27 in optimized, debug, and sanitizer configurations; and twenty-two Unreal automation tests passed 22/22 with no test warning or error. Reviewed pack SHA-256 remains `100f1fcd184cf94fe9b21d3f591714a2e33cc92b60f018bc6523868675156fa0`; the digest-sidecar file SHA-256 remains `9fa9b2d4effc7131def4266bc6092613272218a933043db59e883ec9a3ceb68d`; snapshot/replay schema is 20. Clean source commit `eb063c6296b7da4a37fe4bc7323256e2535448a3` produced evidence hashes `e191d165dc1ad5e645c10540f9b920114e1966db677a02c10d34c53b683b718a` for the automation index, `83948ff317cbb33289414dcc6b0cbe708cb5527faabb0a583038fa54f159423c` and `8445542506b0faa9ba9c24cd56e027fb69c83c37dbf1da8521e3637f49d69411` for normal Meridian/Kharuun runtime, `40cd527c459b51eca92041a835d9916abe162f6eabd19ffecc2d2f11e9ca3ba8` and `a3147a1fb8b1e2e1cb1d600e70b1582ea0e6ef333309e63b688aff65a21abb10` for controlled completion, `34b71bd4921da30d0cd4a7a25cd3f200e7fede76538e1decd6ad671395cadfe4` and `a92def8cb720dfb0d48bbdc0452ba7a651e145ac118067f9653645fa49a5b380` for controlled interruption, and `ae61e7523e43e683525041b04e570669871db779fd340dd4785028446676cf84` for stress runtime. Automation covers schema 20 plus the prior mappings, focus, modal, geometry, and research behavior. Both normal faction directions booted under null RHI; both controlled completion directions reached complete; both controlled interruption directions used ordinary combat to destroy the producer, retained the no-refund result, and presented the archive; stress retained 400 units, four teams, 401 visibility-scoped views, 396 broad orders, and active combat. This establishes deterministic and integration-level interruption feedback, not actual pointer delivery, ordinary player-driven completion/interruption, balance, package behavior, network transport, broader gameplay completeness, or clean-machine installation.

## Rendered prototype boundary

The exact 0.23.1 packaged process was identified by its full executable path and visually inspected through Metal at native 2560×1440. Its responsive Glass Scar operations brief was legible without observed clipping, paused deployment, and transitioned to the battlefield and tactical HUD on Enter. The 0.23.0 package had exposed a real defect: its briefing retained a 920-pixel cap and rendered too small on the Retina display. That package is superseded; the proportional 0.23.1 layout is the accepted packaged result. Versions 0.40.0 through 0.43.0 retain their recorded faction, research, archive, accessibility, focus, active, and completion evidence. Version 0.44.0 used the explicit non-release interruption fixture in an unattended 1600×900 Metal session. At deterministic tick 83, the paused archive rendered Prismatic Targeting in a red interrupted state with `producer destroyed; costs lost`, kept Tier 2 locked behind its prerequisite, and showed a fully visible `RESEARCH INTERRUPTED Prismatic Targeting NO REFUND` HUD line. No clipping or overlap was observed after shortening that field message and rerendering. Screenshot SHA-256 is `76c78c35872dcb38a95e3545d396b3d71290cd9b81fcf01fd0ea36a665ad9a03`; visual-log SHA-256 is `bb7c82f1e56aa08fd32034a1704093624054a37a0d0692d7c1cc9cc0d4586f8a`. This accepts controlled rendered interruption and no-refund feedback, not ordinary player-driven interruption, pointer activation, balance, or package behavior. Version 0.44.0 has not passed package or packaged-performance gates.

## Package and smoke-test macOS

The package wrapper requires at least 60 GiB free, creates a new archive rather than mixing with an existing one, performs Unreal build/cook/stage/PAK/package/archive, validates the arm64 binary, bundle identity, version, and cooked containers, applies a local ad-hoc signature to the completed archive, verifies the signature seal, runs both the normal and active-combat null-RHI packaged smoke gates, and writes a SHA-256 content manifest with the source commit, clean/dirty state, configuration, engine, and Xcode identity:

```sh
./Scripts/package_macos.sh
```

An explicit new archive path may be supplied as the first argument. Existing targets are refused and left untouched. A previously built package can be checked independently:

```sh
./Scripts/run_packaged_smoke.sh \
  BuildArtifacts/Packages/Mac-Development-v0.23.1-mission-briefing/EchoesOfTheBrokenSun.app
./Scripts/run_packaged_stress_smoke.sh \
  BuildArtifacts/Packages/Mac-Development-v0.23.1-mission-briefing/EchoesOfTheBrokenSun.app
```

The current observed local artifact is a self-contained arm64 Development application with bundle identifier `com.angelispseftis.echoesofthebrokensun`, short version `0.23.1`, and five PAK/IoStore container files. Unreal's Xcode packaging phase seals the app before the final PAK update, so the wrapper reseals the completed archive with the checked-in development entitlements and verifies it afterward. The exact archive passed strict deep-signature verification plus normal and active-combat packaged startup. Its adjacent manifest records clean source commit `98a9b83916dadb93ec5205d91102ab98d4ac9364`, UE 5.8.2, Xcode 26.6, configuration and platform identity, and the SHA-256 of every regular file and symlink in the app. The manifest digest is `4c3768649005d3d4970f3ecea6d7ddda360200e4d06033916bb25fa4f656e70e`; normal and stress smoke logs hash to `9fe6b4cf10f62f8cdd7376b0e55c25a4b1b79716358926de68503c9e76a9f949` and `bc17cac8af5e81cf20056884707c8172b37fac677f3615c94dc0716a98b455d6`. The exact package's normal 25-entity Metal profile passed at 10.9075 ms frame, 10.8941 ms render-thread, 10.3518 ms GPU, 0.9148 ms game-thread p95, and 613.922 MiB peak sampled RSS. Its separate active-combat 400-unit/four-team profile passed at 10.9612 ms frame, 10.9498 ms render-thread, 10.4105 ms GPU, 1.0914 ms game-thread p95, and 647.031 MiB peak sampled RSS. Summary hashes are `8d2ce40804d99c97efc6d441f4c0daabdad654d2fabd35e21b501ac1684516aa` and `c6db371285bf3294c82fe480c03154ed53457c0438c2b2d799848bf902793b2f` respectively. The exact rebuildable staging directory was moved recoverably to the Trash path recorded above after qualification; the accepted archive was preserved.

## Distribution boundary

A free Apple Account is enough for local Xcode development. The current package is signed only ad hoc so it can be validated locally; that is not an identity-bearing distribution signature. Developer ID signing, notarization, and Mac App Store distribution require the appropriate Apple Developer Program credentials. No Developer ID signing, notarization, App Store readiness, or compatibility beyond the tested host may be claimed until observed and recorded.
