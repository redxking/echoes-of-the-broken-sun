# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** Unreal-ready production foundation and deterministic simulation spike in progress; no Unreal Editor build has yet been produced or run.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project scaffold, Apple Silicon configuration, source-controlled balance definitions, and a tool-independent C++ simulation core with native tests. Test results are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 installation is staged but paused because the host ran critically low on space. The host is a 16 GB M1 Pro Mac. The existing Epic Games Launcher is version 20.2.4. A verified official installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

Before Unreal integration:

1. Free at least 60 GB before resuming the installer; 100 GB is the safer working target for the engine, derived data, project assets, and packaged builds. This is an engineering recommendation, not an Epic-published minimum.
2. Resume the UE 5.8.2 install through Epic Games Launcher.
3. Install or select Epic's supported Xcode toolchain and its Metal component. Xcode 26.6 exists locally, but Epic recommends 26.1.1 for UE 5.8; compatibility of 26.6 has not been verified.
4. Follow [Docs/SetupAndBuild.md](Docs/SetupAndBuild.md), then run the recorded smoke and simulation tests.

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

