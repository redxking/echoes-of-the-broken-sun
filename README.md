# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. UE 5.8.2 is installed; the deterministic core supports four player slots and passes 17/17 hardened suites in optimized, debug, and sanitizer configurations; the arm64 Development Editor target builds; and Unreal automation passes 13/13 with no warnings or errors. Version 0.30.0 expands the strict authored-content catalog to the complete named slice-roster definitions from the design bible: Surveyor, Lancer, Bulwark Team, Relay Skiff, Anchor, Power Link, Array Foundry, and Aegis Post for the Meridian Compact; Tender, Riftstalker, Cairnback, Resonant, Memory Hearth, Waystone, Growth Basin, and Listening Spine for the Kharuun Assemblies. Display names, roles, costs, health, movement, sight, attacks, logistics, and footprints are canonical data. Eight adversarial compiler tests now enforce worker, combat/support, headquarters, logistics, production, and fourth-role coverage. Build and package entry points regenerate the pack; match startup rejects missing, incomplete, or digest-mismatched content; and runtime gates require SHA-256 `edfd061c610e096f8df16a4fd8e4dfa2ca67af8d4d77aee414e792003c683d05`. These are complete roster definitions, not complete playable faction mechanics: the simulation still uses technical-fixture statistics, and deployment, cover, mobile rooting, adaptation, and vibration detection remain unimplemented. Version 0.30.0 retains the redacted AI boundary, deterministic Adaptive expansion/retreat, cursor policy, and feature-gated title-to-briefing-to-deployment flow. Campaign, multiplayer, and replay remain hidden. Presentation is unchanged from the bounded 0.27.0 Metal inspection. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.30.0 has not been packaged or profiled. Final effects, formations, complete pointer interaction, a rendered end-to-end player-driven match, networked multiplayer, clean-machine use, Developer ID signing, notarization, final art/audio, complete accessibility/settings implementation, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with 59.997 GiB free at the 0.30.0 evidence checkpoint after retaining the accepted development packages. Rebuildable staging trees and superseded packages remain recoverable in Trash and still occupy space until Trash is emptied. Current free space is below the project's 60 GiB packaging boundary and far below the safer 100 GiB sustained-production target, so new packaging is blocked until space is reclaimed. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, thirteen Unreal automation tests, null-RHI startup for both normal and 400-unit/four-team fixtures, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
3. Complete pointer-driven selection, placement, production, movement, combat, victory, and restart validation as one rendered match. Then replace technical primitives and procedural atmosphere with an authored vertical-slice roster, effects, terrain, interface, and audio under the performance and provenance gates.
4. Follow [Docs/SetupAndBuild.md](Docs/SetupAndBuild.md) and record every accepted result in the ledger.

## Repository map

- `Source/EchoesSimCore`: deterministic, engine-independent RTS simulation
- `Source/EchoesOfTheBrokenSun`: Unreal presentation and interaction module
- `Content/Data/Source`: authoritative source data for factions, units, structures, and Future Wells
- `Config`: macOS/Apple Silicon Unreal defaults
- `Tests/Native`: simulation tests runnable before Unreal is installed
- `Docs/DevelopmentBible.md`: game, universe, campaign, UI, audio, accessibility, and player-experience design
- `Docs/TechnicalArchitecture.md`: simulation, Unreal integration, AI, networking, save, replay, and build architecture
- `Docs/ProjectLedger.md`: decisions, roadmap, acceptance evidence, risks, performance budgets, and known limitations
- `Docs/AssetRegister.md`: asset provenance and distribution status

All project documents are authoritative files edited in place. No document is a claim of production readiness.
