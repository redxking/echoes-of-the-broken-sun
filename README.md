# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. UE 5.8.2 is installed; the deterministic core supports four player slots and passes 17/17 hardened suites in optimized, debug, and sanitizer configurations; the arm64 Development Editor target builds; and Unreal automation passes 10/10 with no warnings or errors. Version 0.14.0 advances snapshots and replays to schema 9 so all four player states, command sequences, and explored maps are serialized and checked. In the accepted isolated native 400-unit/four-team profile, visibility refresh measured 0.0885 ms p95 and every implemented native budget passed. In a separate packaged Metal profile of the current 25-entity placeholder scene at native 2560×1440, medium groups, TAA, and exclusive fullscreen, 480 post-warm-up frames measured 10.469 ms frame, 10.457 ms render-thread, 9.929 ms GPU, and 0.899 ms game-thread p95; sampled resident memory peaked at 612.3 MiB. Those packaged results clear the recorded thresholds only for that placeholder scene—not a representative 400-unit combat/weather workload, a soak, or release qualification. Deterministic patrol, hold, guard, control groups, attack-move, construction, production, fog/shroud, pause, restart, and transactional checkpoints remain intact, and the complete automated skirmish still reaches victory. The latest accepted self-contained package is the clean-source 0.14.0 four-player-core build at commit `de67359`, with verified arm64 identity, version, local ad-hoc signature seal, content manifest, packaged startup, bounded performance evidence, and rendered presentation inspection. Complete pointer interaction, a rendered end-to-end match, representative stress/soak qualification, networked multiplayer, clean-machine use, Developer ID signing, notarization, final art/audio, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 74 GiB free after retaining the accepted development packages. The exact 0.13.0 and 0.14.0 rebuildable staging trees are recoverable in Trash and each still occupies about 1.1 GB until Trash is emptied. Current free space is above the project's 40 GiB prototype-build stop threshold and its preferred 60 GiB packaging headroom, but below the safer 100 GiB sustained-production target. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, ten Unreal automation tests, null-RHI and rendered startup, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use plus representative performance workloads remain unverified.
3. Build the representative 400-unit combat/weather Unreal stress scene, run the 60-minute soak, and complete pointer-driven construction, production, movement, combat, victory, and restart validation as one rendered match before content expansion.
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
