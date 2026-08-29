# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. Version 0.38.0 retains distinct deterministic identities and production for all sixteen named Meridian Compact and Kharuun Assemblies slice archetypes and implements six bounded faction mechanics. In addition to Bulwark cover, Relay logistics, Waystone migration, warform adaptation, and Cairnback mineral cover, moving hostile mobile units now emit short-lived vibration signatures. Kharuun Resonants detect them within 2,200 cm and Listening Spines within 2,600 cm. Contacts linger for 40 ticks after actual movement, are quantized to 200-cm cells, disappear without detector coverage or when the source becomes ordinarily visible, and disclose no entity ID, faction, type, health, order, or exact position. Adaptive AI can investigate a signature only with attack-move to the approximate point; it cannot directly target the hidden source. The HUD has labeled, non-color diamond-wave presentation for world and tactical-overview contacts. The canonical content SHA-256 is `c5f516ab05fcdcb06716b2d6a6786612ba4cb19267ac7fd3bbd4f206efedd1db`; rules, schema-17 snapshots/replays, player views, AI observations, and checksums carry all six profiles and public states. The deterministic core passes 25/25 suites in optimized, debug, and sanitizer configurations; the arm64 Development Editor builds; Unreal automation passes 19/19 without warnings or errors; and normal plus 400-unit/four-team runtime gates remain green. Direct Kharuun player selection remains absent from the current Meridian-only operation, so valid player-side Waystone, adaptation, mineral-cover, and vibration-presentation use is not yet reachable in the standard scenario. The short normal runtime did not observe live AI adaptation, cover, or vibration response. Powered Aegis behavior and broader faction systems remain unimplemented. Campaign, multiplayer, and replay UI remain hidden. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.38.0 has not been packaged or profiled because the current 66 GiB free space is only about 6 GiB above the packaging floor and remains an unsafe transient margin. Final effects, formations, complete pointer interaction, a rendered end-to-end player-driven match, networked multiplayer, clean-machine use, Developer ID signing, notarization, final art/audio, complete accessibility/settings implementation, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 66 GiB free at the 0.38.0 source-evidence checkpoint. That is only about 6 GiB above the project's 60 GiB packaging floor and still provides no safe transient margin; the safer sustained-production target is 100 GiB. Rebuildable staging trees and superseded packages remain recoverable in Trash and still occupy space until Trash is emptied. New packaging remains deferred until meaningful headroom is reclaimed. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, nineteen Unreal automation tests, null-RHI startup for both normal and 400-unit/four-team fixtures, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
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
