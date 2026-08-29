# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. Version 0.55.0 adds a contextual tactical command deck that derives exact combat, Worker, Command Core, Array Foundry, formation, control-group, and faction-special labels from the current local selection. Version 0.54.0 retains destination-aligned Box, Line, and Wedge layouts; version 0.53.0 retains accessibility-aware accepted-order markers; version 0.52.0 retains the faction/role silhouette layer; version 0.51.0 retains the first battlefield-composition pass; and version 0.50.0 retains the verified interaction, research, AI-opening, and bounded victory paths. Source commit `aebb4be899456d98b75f95a01db9b931162a1c46` passes 27/27 content tests, 27/27 deterministic suites in optimized, debug, and sanitizer configurations, the arm64 Development Editor build, 25/25 Unreal automation tests without project warnings or errors, and normal Meridian and Kharuun runtime starts. The canonical content SHA-256 remains `100f1fcd184cf94fe9b21d3f591714a2e33cc92b60f018bc6523868675156fa0`. An ordinary 1600×900 Metal run rendered unclipped combat, Worker, Command Core, and Array Foundry decks, exercised the high-contrast branch, and updated the combat deck from Box to Line with Wedge named as the next formation. This accepts that bounded code-only interface layer, not every selection combination, UI scale, resolution, pointer-delivery path, final UI/art/audio, broad usability, package performance, or release qualification. Exact coordinate-click delivery and ordinary player-driven research interruption remain unaccepted. Campaign, multiplayer, and replay UI remain hidden. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.55.0 has not been packaged or profiled because about 53 GiB is free, below the 60 GiB packaging floor. Final formation behavior under pressure, effects, networked multiplayer, clean-machine use, Developer ID signing, notarization, complete accessibility/settings implementation, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 53 GiB free at the 0.55.0 source-evidence checkpoint, below the project's 60 GiB packaging floor; the safer sustained-production target is 100 GiB. Rebuildable staging trees and superseded packages remain recoverable in Trash and still occupy space until Trash is emptied. New packaging remains deferred until meaningful headroom is reclaimed. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, twenty-five Unreal automation tests, null-RHI startup for both selectable faction directions and the 400-unit/four-team fixture, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
3. Retain exact coordinate-pointer delivery as a separate acceptance gate, exercise ordinary player-driven research interruption, and continue replacing technical primitives with authored effects, materials, and audio under the performance, provenance, and accessibility gates. The first code-only Scar composition is accepted at version 0.51.0, faction/role silhouettes at 0.52.0, accepted-order markers at 0.53.0, five-unit destination layouts at 0.54.0, and a contextual command-deck hierarchy at 0.55.0; none is final art, broad usability, or broad formation qualification. The bounded keyboard-driven construction, production, research-completion, Future Well, tactical-control, victory, result, and post-result restart paths remain accepted at version 0.50.0.
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
