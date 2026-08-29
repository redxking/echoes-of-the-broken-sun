# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. UE 5.8.2 is installed; the native simulation passes 13/13 hardened suites in optimized, debug, and sanitizer configurations; the arm64 Development Editor target builds; and Unreal automation passes 6/6 with no warnings or errors. The current scenario contains the authored Glass Scar topology: a 165-tile fractured transit band, a central Future Well crossing, and two side-cavern routes. A map-specific test proves the exact terrain layout, presentation parity, and deterministic unit routing from the southern basin to the northern basin. The complete automated skirmish still builds a drop-off, captures the Well, produces and rallies seven soldiers, crosses the Scar, reaches victory from one attack-move order per soldier, freezes after the result, and restarts. Version 0.7.0 adds transactional checkpoint persistence: `[K]` validates and commits a deterministic snapshot while retaining one prior generation, and `[L]` restores the newest compatible checkpoint or falls back to that backup when the primary is corrupt. Load reconstructs disposable terrain, fog, and entity views and resumes local command sequencing without serializing Unreal actors. It retains authoritative health bars, `[F]` attack-move, `[X]` stop, fog/shroud, construction, production, context orders, pause, and restart. Rendered Editor and packaged-app inspection each confirm a checkpoint commit and exact tick rollback without disturbing normal lit rendering. The latest accepted self-contained 0.7.0 package is traceable to clean source commit `bcdb6d9`. Complete pointer interaction, a rendered end-to-end match, navigation-scale performance, clean-machine use, Developer ID signing, notarization, final art/audio, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 79 GiB free after retaining the current 0.7.0 package and removing its exact rebuildable staging directory. That is above the project's 40 GiB prototype-build stop threshold and its preferred 60 GiB packaging headroom, but below the safer 100 GiB sustained-production target. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, five Unreal automation tests, null-RHI and rendered startup, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use plus representative performance workloads remain unverified.
3. Complete pointer-driven construction, production, movement, combat, victory, and restart validation as one rendered match; then profile obstacle routing and fog/shroud at the target unit and map scale before content expansion.
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
