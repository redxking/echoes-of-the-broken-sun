# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype moving into its Glass Scar vertical slice, not a completed game. UE 5.8.2 is installed; the deterministic core supports four player slots and passes 17/17 hardened suites in optimized, debug, and sanitizer configurations; the arm64 Development Editor target builds; and Unreal automation passes 12/12 with no warnings or errors. Version 0.22.1 adds persistent camera and HUD accessibility settings, non-color team identities, accessible damage feedback, lightweight reduced-motion-aware Glass Scar atmosphere, and a fog-respecting tactical overview. Its 400-unit/four-team fixture queues 396 broad attack-move orders and enters actual combat. In the exact packaged Metal stress profile at native 2560×1440, medium groups, TAA, and exclusive fullscreen, 480 post-warm-up frames measured 10.9302 ms frame, 10.9176 ms render-thread, 10.3787 ms GPU, and 0.9985 ms game-thread p95; sampled resident memory peaked at 645.578 MiB, and every applied threshold passed. A same-build 25-entity baseline also passed. A separate one-hour 0.15.0 stationary proxy soak survived and passed its process-memory bounds, but it predates broad combat orders and atmosphere; concurrent builds contaminated its CPU samples, so those CPU values are rejected. The latest accepted self-contained package is the clean-source 0.22.1 tactical-overview build at commit `b854ef2`, with verified arm64 identity, version, local ad-hoc signature seal, content manifest, normal and active-combat packaged startup, bounded performance evidence, and exact-build visual inspection. Final effects, formations, complete pointer interaction, a rendered end-to-end match, networked multiplayer, clean-machine use, Developer ID signing, notarization, final art/audio, complete accessibility/settings implementation, and release qualification remain open.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with a generated arena, visibility-scoped placeholder entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. The rendered scene is explicitly labeled as an active-development playable-systems build; the engine primitives remain placeholders rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 63 GiB free after retaining the accepted development packages. Rebuildable staging trees, including the exact 0.22.1 tree at `/Users/angelispseftis/.Trash/Echoes-StagedBuilds-Mac-20260829T053425Z`, and the rejected stale-metadata 0.22.0 archive are recoverable in Trash and still occupy space until Trash is emptied. Current free space is above the project's 40 GiB prototype-build stop threshold and preferred 60 GiB packaging headroom, but below the safer 100 GiB sustained-production target. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, twelve Unreal automation tests, null-RHI startup for both normal and 400-unit/four-team fixtures, and local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
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
