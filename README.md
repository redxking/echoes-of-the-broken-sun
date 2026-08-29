# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. Version 0.44.0 makes research interruption authoritative and visible. If a production structure is destroyed, the owning deterministic player state records the interrupted technology, the HUD reports `RESEARCH INTERRUPTED ... NO REFUND`, and the archive renders the affected tier in a distinct interrupted state until another valid project starts. The owning view, snapshot, replay, validation, and checksum paths use schema 20. The 0.43 keyboard focus and exact-tier activation behavior remains. Two non-Shipping fixtures separately exercise normal completion and ordinary-combat producer destruction; both are evidence infrastructure, not release or ordinary player behavior. The canonical content SHA-256 remains `100f1fcd184cf94fe9b21d3f591714a2e33cc92b60f018bc6523868675156fa0`. At clean source commit `eb063c6296b7da4a37fe4bc7323256e2535448a3`, 27/27 content tests passed, the deterministic core passed 27/27 suites in optimized, debug, and sanitizer configurations, the arm64 Development Editor built, Unreal automation passed 22/22 without warnings or errors, both normal faction directions, both controlled completion directions, both controlled interruption directions, and the 400-unit/four-team runtime gate passed. An unattended Metal session at 1600×900 accepted the red interrupted tier, locked prerequisite, concise no-refund HUD state, and pause-safe archive without observed clipping or overlap. Actual coordinate-click delivery, ordinary player-driven interruption/completion, a complete player-driven match, and balance remain unaccepted. Campaign, multiplayer, and replay UI remain hidden. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.44.0 has not been packaged or profiled because the current 66 GiB free space is only about 6 GiB above the packaging floor and remains an unsafe transient margin. Final effects, formations, networked multiplayer, clean-machine use, Developer ID signing, notarization, final art/audio, complete accessibility/settings implementation, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 66 GiB free at the 0.44.0 source-evidence checkpoint. That is only about 6 GiB above the project's 60 GiB packaging floor and still provides no safe transient margin; the safer sustained-production target is 100 GiB. Rebuildable staging trees and superseded packages remain recoverable in Trash and still occupy space until Trash is emptied. New packaging remains deferred until meaningful headroom is reclaimed. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, twenty-two Unreal automation tests, null-RHI startup for both selectable faction directions and the 400-unit/four-team fixture, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
3. Complete pointer-driven selection, placement, production, movement, combat, victory, and restart validation as one rendered match. Then replace technical primitives and procedural atmosphere with an authored vertical-slice roster, effects, terrain, interface, and audio under the performance and provenance gates.
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
