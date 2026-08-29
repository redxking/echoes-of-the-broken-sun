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
./Scripts/run_kharuun_systems_smoke.sh
./Scripts/run_stress_smoke.sh
```

Run the alternate local-faction bootstrap gate with `ECHOES_LOCAL_FACTION=Kharuun ECHOES_RUNTIME_LOG="$PWD/BuildArtifacts/KharuunRuntimeSmoke.log" ./Scripts/run_runtime_smoke.sh`. This command-line selector exists for unattended evidence; the player-visible path is Tab on the title or operations brief.

`run_research_presentation_smoke.sh` is a Development-only completion fixture. It supplies controlled local resources, queues the first faction technology through the ordinary deterministic command path, and requires ready, active, and complete markers. `run_research_interruption_smoke.sh` adds a third controlled player with 32 ordinary combat units, schedules ordinary attack commands against the producer, and requires active, interrupted, no-refund, and automatically presented archive markers while rejecting completion. `ECHOES_LOCAL_FACTION=Kharuun` exercises the other local faction. Both command-line modes are compiled out of Shipping behavior and must not be described as ordinary player progression or release behavior.

`run_kharuun_systems_smoke.sh` is a Development-only Kharuun presentation fixture. It queues ordinary Waystone migration, Warform adaptation, Cairnback mineral-cover, and hidden-hostile movement commands, then requires all four states, one anonymous vibration contact, no source disclosure, and automatic pause. It is integration evidence, not ordinary player execution, balance, or release behavior.

`build_editor.sh` and `package_macos.sh` default to four concurrent Unreal build actions because the engine-selected ten-action default caused severe memory and swap contention on this 16 GB host. Set `ECHOES_MAX_PARALLEL_ACTIONS` to another positive integer only for an intentional host-specific experiment; both wrappers reject invalid values.

The next paragraph preserves the complete 0.45.0 matrix as historical evidence; later paragraphs record the focused 0.46.0 through 0.50.0 checkpoints and the current 0.51.0 source/presentation checkpoint.

The observed current 0.45.0 source results are: `EchoesOfTheBrokenSunEditor Mac Development` compiled and linked for arm64; twenty-seven adversarial content-compiler tests passed; the native core passed 27/27 in optimized, debug, and sanitizer configurations; and twenty-two Unreal automation tests passed 22/22 with no test warning or error. Reviewed pack SHA-256 remains `100f1fcd184cf94fe9b21d3f591714a2e33cc92b60f018bc6523868675156fa0`; the digest-sidecar file SHA-256 remains `9fa9b2d4effc7131def4266bc6092613272218a933043db59e883ec9a3ceb68d`; snapshot/replay schema remains 20. Clean source commit `a7a23179aaae270b1789ddb6521210726ce76f25` produced evidence hashes `a4979a4fe861f68944d8f02572d7008b05571cc12ec226285560a464fce0fb60` for the automation index, `a2e36fc02849095254401e56d42630a5f3885db9407544996e56b4aaa0dc80ce` and `5d16c2af233031df732aaff236e80a8cd8905c3318694f7422731a9c9ac4ff20` for normal Meridian/Kharuun runtime, `a936765448bb0405b4c6365cf4b9b8dd51b14f4636d8d0508994c3f45e80adcc` and `a5c9e3b4597f896379ff50a6c3bbb4654f55b669bd0719ed80ba31d2bdbd8aa7` for controlled completion, `6c911dbe8231f96e4d9385f500dc85d3a9a2010c328dded4b7c08314ac9c31ec` and `d08148c02c6f2e74c92f19eb8be419e8a3eeefab79edbc8000a92eda00668630` for controlled interruption, `233eac2d8e86404dba26f89f0545e2728310dc44c74b2db1e3ca98a8179d87c0` for the controlled Kharuun systems fixture, and `d3f7a5d0614908a233e0e8a1442fba7af2517c178a966faeb75d5d66d766592e` for stress runtime. Automation covers schema 20, Tab/Backspace mappings and traversal, and the prior modal, geometry, and research behavior. Both normal faction directions booted under null RHI; both controlled completion and interruption directions passed; the Kharuun fixture observed migration, adaptation, mineral cover, and one anonymous undisclosed vibration contact; stress retained 400 units, four teams, 401 visibility-scoped views, 396 broad orders, and active combat. This establishes clean-source integration behavior, not pointer delivery, ordinary player-driven special-system or research use, balance, package behavior, network transport, broader gameplay completeness, or clean-machine installation.

Version 0.46.0 adds no deterministic state or content change. At clean source commit `38e9f70abe9d23d447584f9cb72bcae95c9f9c11`, the content compiler passed 27/27, the native core passed 27/27 in optimized, debug, and sanitizer configurations, the arm64 Development Editor built, and Unreal automation passed 22/22 without warning or error. Automation-index SHA-256 is `6fc88968ffd2b885f38e5d709961e08b5c5294975c3124b6f3211adf0fddc30a`. The focused Kharuun normal runtime, controlled systems fixture, and stress logs hash to `3ac17ea770af3ddd3a448b6638fbd4c5c4c98958b33bb1be29595d4166918578`, `431571e6962cf052e6bf575aa34e77358601a6846d8ed4a88226822393d0fd26`, and `cd7f4b6e0651d45c39096df62f2d4195af554f9fceb859a95f098954541d429d`. These are focused regression gates; the broader 0.45.0 matrix remains separately recorded.

Version 0.48.0 changes only controller, input, HUD, mapping-test, and project-version integration; deterministic rules, schema 20, and canonical content are unchanged. At source commit `9a68d6a6df9995c004b15edc77a0f4564c759f5b`, the content compiler passed 27/27, the native core passed 27/27 in optimized, debug, and sanitizer configurations, the arm64 Development Editor built, and Unreal automation passed 22/22 without warning or error. Automation-index SHA-256 is `0dbc02e4047e0b7e252556ae430a4e35e67647e90bccf643fb6a617850423a1c`. Meridian/Kharuun normal runtime hashes are `c1baaeaa455952c846d49e5258a9294b5a82573e33e90ea133559d0cdf69c53a` and `f57bcdf63ff0f6b2b556659623cd889eb52af9a1933d740aca90ebe3252d3882`; controlled Kharuun systems and stress hashes are `404c97c3efed37195aacd02114d790a6306420df608060a3ec439656a3761ed5` and `9674f18485d5248e52e2f4575f07f303d1fe166317ce7cd9c0f6472f99b596f4`. These are current-source integration gates, not package, balance, sustained-runtime, or release qualification.

Version 0.49.0 adds only selected-owned-view camera/reticle snap, visible selected-Warform state, mapping coverage, and the project-version change. At source commit `905e26ff6cf8631bf9743b6e890e74917f77c88d`, the content compiler and native core retained their 27/27 results in all three native configurations, the arm64 Development Editor built, and Unreal automation passed 22/22 without warning or error. Automation-index SHA-256 is `3d2fbf0b7678de7634c923e24186a9d580e35576e354f314dc6d7c7f7275a57a`. Meridian/Kharuun normal runtime hashes are `14d44c44aa664b576325ac962849dc8e7b28e15234ff2b03745550102f9c2491` and `5374741cf8958c98b3abe780057671ca27806ae432dfc026b53cb2dbea09590f`; controlled Kharuun systems and stress hashes are `e34dd6b44fba6f88f5bc17bd2afb5dfa2d4b008907025a91e43b1db8045b7239` and `585b25703395c982f5bfdbb2736cba5c65b325497d6c48abf30e575c1613f3b8`. These remain current-source integration gates rather than package, balance, sustained-runtime, or release qualification.

Version 0.50.0 adds deterministic F7 selection of all local alive combat entities that have owned presentation views, selected-group centroid snap through End, and a five-minute Adaptive opening posture around an owned command core. The posture continues visible-threat defense, economy, construction, production, and research while suppressing anonymous vibration pursuit and recalling combat units more than nine tiles from the core; ordinary behavior resumes at tick 6,000. At source commit `cf00559e7431288a4dbbd23073afa24f07976309`, the content compiler passed 27/27 with canonical pack SHA-256 `100f1fcd184cf94fe9b21d3f591714a2e33cc92b60f018bc6523868675156fa0`; the native core passed 27/27 in optimized, debug, and AddressSanitizer/UndefinedBehaviorSanitizer configurations; the arm64 Development Editor built; and Unreal automation passed 22/22 without warning or error. Automation-index SHA-256 is `54ff1dee3ed5751c72306d88dffd413693d5a2987362389ea84b849612f7f8d5`. Meridian/Kharuun normal runtime hashes are `5fb41e6524374a8c4c8f96ca04ed2d02795f057d9e8c6f2f11e51041a24f89d4` and `84430aa132b35e4e41f986d9ed9d25bfcf2a44a083d34e33a3c52c05e3cbe07a`; controlled Kharuun systems and stress hashes are `a129fd617f5fd3d63ef297e784254ccb4fb76104cb4fc54ed0829064543e38c4` and `587c9031461d172bc767187dac1ae49bcc7d38b37140eb8773a545a28334c038`. These accept current-source integration and the specifically tested posture behavior, not broad balance, package performance, sustained runtime, or release qualification.

## Rendered prototype boundary

The exact 0.23.1 packaged process was identified by its full executable path and visually inspected through Metal at native 2560×1440. Its responsive Glass Scar operations brief was legible without observed clipping, paused deployment, and transitioned to the battlefield and tactical HUD on Enter. The 0.23.0 package had exposed a real defect: its briefing retained a 920-pixel cap and rendered too small on the Retina display. That package is superseded; the proportional 0.23.1 layout is the accepted packaged result. Versions 0.40.0 through 0.44.0 retain their recorded faction, research, archive, accessibility, focus, completion, and interruption evidence. Version 0.45.0 used the explicit non-release Kharuun systems fixture in an unattended 1600×900 Metal session. The final frame rendered the selected Listening Spine and its selection ring, exact Tab/Backspace feedback, an amber `VIBRATION CONTACTS` alert with approximate/anonymous/no-direct-target language, and a distinct tactical-overview diamond without observed clipping or overlap. Live application delivery observed Tab advance and Backspace reverse; Shift+Tab had reproduced as forward traversal and was replaced. Screenshot SHA-256 is `75ad5aadb5d65383c83c4bcacdcf79fb3ec7bee2007274d48ed0fcce3df70d19`; visual-log SHA-256 is `4d0f0e631d157196e0ff787a60b9879eaded8f2a8bfb226420a1799f3df63405`. This accepts controlled rendered selection and sensor-alert presentation, not the offscreen world-marker label, pointer activation, ordinary player-driven Kharuun-system use, balance, or package behavior. Version 0.45.0 has not passed package or packaged-performance gates.

Version 0.46.0 ordinary-match input used Enter to deploy, Tab to select the Kharuun Waystone, and F3 to start migration through the normal controller and simulation command path. The rendered HUD exposed `[F3] Waystone [F4/F5] Warform [F6] Cover` and the successful state-change feedback without observed clipping. F4 reached ordinary validation and returned `OUTSIDE_MOLT_SITE`; F6 reached ordinary validation and returned `NO_WORLD_HIT`. These are correct bounded rejections, not successful adaptation or cover. Screenshot SHA-256 is `975bff1d072a2708d2756538f381cc4a3b9fd157a7e9bb9ef541bd6dd9251d25`; ordinary-input log SHA-256 is `96920024a16ba1be9e0edb2a80aea2bbbadc01f064e9d0d87d091bbf82b3da9e`. This accepts direct ordinary Waystone input and the two rejection paths, not pointer targeting, successful ordinary adaptation/cover, package behavior, or a complete match.

Version 0.48.0 ordinary-match input used Home to enable a cyan keyboard reticle, arrow keys to move it to `(+128,+64)` pixels, and the existing target-dependent command path to trace only the visibility channel beneath that screen location. A separate Meridian run selected Bulwark Team entity 10 through Tab and used Backslash to deploy toward the reticle, pack after completion, and redeploy; all three controller reports recorded zero rejected commands. The moved-reticle screenshot/log SHA-256 values are `743fc462d32ea1cc0364e6b74140240eb88b09d98a10cf81c779b4c35406c9f7` and `3c7d1151b97773f91afa7236f3138b1a684a45f7cbe1ff493bb6f053e5bce6fd`. The accepted Bulwark screenshot/log hashes are `1d062f3d912ee382ca4363e297bd168d1781d3b449bafb1f1908fefc95fa022a` and `8910356124d91bd09283a42e441629e78bf5d01fd153db63b87c9bb739467cf2`. This accepts ordinary rendered reticle movement and the Bulwark transition cycle, not exact coordinate-pointer delivery, building placement, Kharuun adaptation/cover, victory/restart, package behavior, broad usability, balance, or final presentation.

A subsequent source-identical 0.48.0 Meridian run selected Worker entity 4, moved the reticle to `(+192,+128)` pixels, and used B to queue an Array Foundry at open visible ground. The site rendered at the reticle and resources changed from 500 Matter/30 Dawnshards to 320/0. The run later used R to restore the deterministic initial state, selected the original Array Foundry, and used E to queue one Lancer; the HUD rendered 19% progress and the resources changed to 415/10. Construction/production log SHA-256 is `462ac9341d10112a08b592cc044db62457f589b2f2fb4c223260e79fb4cd4f57`; placement and production screenshot hashes are `3b0c43b3fb1d6197060fed97b3aa7c06518a751124b14237f0c040a8b70ff350` and `806febb2e10cc9286681409df6a1489e538b4f83a08234925fc383167a4add99`. This accepts ordinary placement, production queueing/progress presentation, and direct restart delivery. The new site was not observed to completion, the produced unit was not observed emerging, and restart did not follow a result screen; those gates remain open.

Version 0.49.0 ordinary Kharuun input selected Cairnback entity 10, used End to center the camera and reticle on that selected owned presentation actor, then used two Right steps and one Up step to reach clear nearby ground. F6 raised one visible mineral barrier, deducted 15 Dawnshards, and reported zero rejection. After restart, entity 9 accepted F4; the selected line rendered `molting CARAPACE` and then `CARAPACE` after the authored 80-tick transition. Snap/ordinary-input log SHA-256 is `fc54431f795a3f747c05fab487ac65596662c29b91d80972975a355ab881832f`; snap, cover, adaptation-start, and adaptation-complete screenshot hashes are `852ba17b56fad84b1b32229dd1a52a8bd918d20b5abdf5267189053cf276e0f9`, `46597e61c491d5be03b156ee3c1d5bf5cc84b383dd0b862ddad24c0c52f3d5ca`, `6bf39af8bda0ec1d7e956d31c7840effea8460ff485194d7542e77ce11321768`, and `3ee671dc20e86ad1e28d90eaedf97f0fdbb329a1c97ad2293048bead924e9876`. This accepts ordinary successful Cairnback cover and completed Carapace adaptation through direct application input. It does not establish exact coordinate-pointer delivery, balance, package behavior, broad usability, or final presentation.

Version 0.50.0 ordinary rendered input first completed the previously queued Array Foundry and observed a queued Lancer emerge. A fresh deterministic run then built and completed a Power Link, revealed and harvested the central Future Well, produced four Lancers from the original Array Foundry, used F7 to select the nine-unit visible owned combat force, and used End plus bounded keyboard attack-move destinations to reach and destroy the Kharuun Command Core. Six selected units survived; the victory result rendered at deterministic tick 10,307; Enter redeployed the operation at tick 23 with 500 Matter, 30 Dawnshards, 13/18 logistics, no selection, and the Future Well unlocated. The rendered-session log SHA-256 is `ec5825a825152d4bb7507adccbfc71b50b77ec9c5ceb2db81e1de1fa75ecdf82`. Array Foundry completion and produced-Lancer screenshots hash to `a8269ac08ddb97f70b7883ecb4d0742f9db0c713f39d900c21571166f70e16e3` and `54702b9418dab2a99e4610bb2181c3327db8082804810ebfe35c7619ddc64d5b`. Future Well, four-Lancer completion, nine-unit selection, victory, and redeployment screenshots hash to `5732ab7579aec5933bc56fc85adde8b68f67e4e6ea7b34f0c31afbf0d64b2b15`, `71814b3bd73daf8b876274cb20bf92a7ce2ee18908c452d221a1455594e3a1b1`, `2f56049b9b787ba984243c3fae060547374debb12cf8d463f5017a2b7e49378b`, `2e3e49ba7423ca4e30447da98f0efc74533b91ed2516f318dfbfcd233a87fef3`, and `d7806290fd3625f5efdf27ff2b0da11ad7f4878a67c32099609c049cf3c6b64f`. The log also retains earlier diagnostic placement failures and an exposed-worker loss; only the final baseline-to-victory sequence is accepted. This establishes one bounded player-driven path, not general balance, exact coordinate-pointer delivery, package behavior, broad usability, final presentation, or release readiness.

A separate ordinary 0.50.0 run used the same player-driven Well route to fund research, selected the original Array Foundry, opened the F2 technology archive, and used Enter to queue Prismatic Targeting. Matter/Dawn changed from 500/530 to 380/490, the field HUD rendered 48% progress, and the archive then rendered Tier 1 complete with Horizon Lattice ready. Enter queued Tier 2; after its authored 220 ticks the archive rendered both tiers complete with 290 Matter and 435 Dawn remaining. No Development fixture or direct state mutation was used. Unreal log SHA-256 is `d260d1aa343e3d35f3b270c4fb1a53fe7729caf034bcbe3bc3f01d263b5abbd3`; ready, started, progress, Tier-1-complete, and both-complete screenshots hash to `5b76620649f0f3825460c2879b8245e3408c6153741372709caec4eb579a55be`, `5d6c0f3630f3ae1fac6b65f80bab8fe3ba334ba40815a95ffa34ef4e57bf0733`, `0f7f274b60bc2ed717e5faf20fbc361fc71ecef8df66bb4c89ab021f6967144a`, `f5171990db881a8a2b0a9a99355ef040c6acd18f20c358be6ce3b2e03ae7fd0f`, and `0a8c43f214ab895042c5ddd85bd6a2bf9a3bf9eb2c84bea3dd3cdc2ffa0331c4`. This accepts ordinary player-driven Meridian research completion through both authored tiers; the next paragraph closes the corresponding Kharuun completion gate. Ordinary interruption, broad usability, balance, package behavior, and release readiness remain separate gates.

The symmetric ordinary Kharuun run selected the Growth Basin and completed Echo Cartography followed by Ancestral Edge through the same F2/Enter flow after player-driven Well Harvest. The final archive rendered both tiers complete at 290 Matter, 435 Dawn, and research 2/2; the concurrent vibration alert remained approximate, anonymous, and without a direct target. Unreal log SHA-256 is `38d4419c79d5aea800d0a141d4b2bc811662daa864e95a7e05e7fb0715899042`; ready, Tier-1-complete, and both-complete screenshots hash to `3cef4c3e5a4bd7588ab508caef783c07ebe3f244c0baf12a473bce5d5171830a`, `9809f3996cc4c4003cef5b5b399ede8ebc8902291726e21d0fe76b4750ad08d3`, and `b79fb68a8b24fb0dc56fee986728a869e62aa7165f7f03d2af960b528e84a195`. Ordinary research completion is therefore accepted for both playable factions. Ordinary interruption, broad usability, balance, package behavior, and release readiness remain separate gates.

Version 0.51.0 source commit `83e53720b1e0357cc972567e141ebfc140831ebd` adds the first code-only Glass Scar composition without changing deterministic rules: seven non-colliding fracture bands, twelve non-colliding shard silhouettes, and five shadow-free local lights. Content passed 27/27, the native core passed 27/27 in optimized, debug, and sanitizer configurations, the arm64 Development Editor built, Unreal automation passed 22/22 with index SHA-256 `11025e5ff35cb112e7f5536ecc425981d2eac951975ada0ff5e4fd64b2e4cfeb`, and normal Meridian/Kharuun runtimes passed with log hashes `c20cbf9ff04340cdcbff9d54b2f0fcf45351535c3f8d15ab7b2495b177342f6d` and `4a23fe043d04e63f2e84dc0f2876ba88f4a230af1d588c5c37a7043ecbbfb767`. An ordinary Meridian run moved a Relay Skiff until the central span was legitimately visible and centered the selected owned view. The accepted Metal frame rendered the magenta/copper seam, authoritative broken tiles, and shard silhouettes; log and screenshot SHA-256 are `3bed54f6f1c8e0eab2e1f803204538ec4174892c2417d12361f6f4040943e057` and `cafe1bf006eb1f54f3fa9fa9a8277618d9f5d0c81904e195b2cd3099d47596d8`. The earlier fog-obscured camera attempt is diagnostic only. This accepts a bounded terrain-composition layer, not final terrain assets, package performance, broad visual usability, or release readiness.

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
