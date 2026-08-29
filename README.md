# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. Version 0.48.0 retains deterministic Tab/Backspace selection and the F3–F6 Kharuun aliases while adding a fair keyboard tactical reticle: Home enables or exits it, arrows move it in bounded 64-pixel screen steps, Space issues a context order, and existing target-dependent command keys use the reticle while it is active. The reticle traces only the rendered visibility channel at its screen position and does not read hidden simulation state. In ordinary rendered matches, direct application input moved the reticle, issued movement and attack-move, completed a Meridian Bulwark deploy/pack/redeploy cycle, placed a new Array Foundry construction site, restored the deterministic start through R, and queued a Lancer from the original Array Foundry. Earlier ordinary Kharuun evidence accepts F3 Waystone migration; F4 outside a Growth Basin and F6 without a valid target remain rejection-path evidence rather than successful adaptation or cover. The new construction site was accepted and rendered but its completion was not retained as evidence. Research remains schema-20 authoritative and the canonical content SHA-256 remains `100f1fcd184cf94fe9b21d3f591714a2e33cc92b60f018bc6523868675156fa0`. Source commit `9a68d6a6df9995c004b15edc77a0f4564c759f5b` passes 27/27 content tests, 27/27 deterministic suites in optimized, debug, and sanitizer configurations, the arm64 Development Editor build, 22/22 Unreal automation tests without warnings or errors, both normal faction runtime starts, the controlled Kharuun systems fixture, and the 400-unit/four-team combat stress gate. Exact coordinate-click delivery, successful ordinary Kharuun adaptation/cover, new-site construction completion, a complete player-driven victory/result/restart operation, and balance remain unaccepted. Campaign, multiplayer, and replay UI remain hidden. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.48.0 has not been packaged or profiled because only about 59 GiB is free, below the 60 GiB packaging floor. Final effects, formations, networked multiplayer, clean-machine use, Developer ID signing, notarization, final art/audio, complete accessibility/settings implementation, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 59 GiB free at the 0.48.0 source-evidence checkpoint, below the project's 60 GiB packaging floor; the safer sustained-production target is 100 GiB. Rebuildable staging trees and superseded packages remain recoverable in Trash and still occupy space until Trash is emptied. New packaging remains deferred until meaningful headroom is reclaimed. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, twenty-two Unreal automation tests, null-RHI startup for both selectable faction directions and the 400-unit/four-team fixture, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
3. Complete new-site construction, Kharuun adaptation/cover, production completion, victory, result presentation, and post-result restart as one rendered match; retain coordinate-pointer delivery as a separate acceptance gate. Then replace technical primitives and procedural atmosphere with an authored vertical-slice roster, effects, terrain, interface, and audio under the performance and provenance gates.
4. Follow [Docs/SetupAndBuild.md](Docs/SetupAndBuild.md) and record every accepted result in the ledger.

## Repository map

- `Source/EchoesSimCore`: deterministic, engine-independent RTS simulation
- `Source/EchoesOfTheBrokenSun`: Unreal presentation and interaction module
- `Content/Data/Source`: authoritative source data for factions, units, structures, technologies, and Future Wells
- `Config`: macOS/Apple Silicon Unreal defaults
- `Tests/Native`: simulation tests runnable before Unreal is installed
- `Docs/DevelopmentBible.md`: game, universe, campaign, UI, audio, accessibility, and player-experience design
- `Docs/TechnicalArchitecture.md`: simulation, Unreal integration, AI, networking, save, replay, and build architecture
- `Docs/ProjectLedger.md`: decisions, roadmap, acceptance evidence, risks, performance budgets, and known limitations
- `Docs/AssetRegister.md`: asset provenance and distribution status

All project documents are authoritative files edited in place. No document is a claim of production readiness.
