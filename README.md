# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype entering its authored Glass Scar vertical slice, not a completed or publicly playable game. Version 0.78.0 at exact game-source commit `02c0b62d75d16d6f30575d01f0a507e0fd68a136` adds the first real multiplayer transport slice: a Mac listen server binds the first remote owning connection to seat 1, requires the exact 0.77 compatibility manifest, sends one bounded visibility-scoped keyframe materialized only from `PlayerView`, admits one three-tick-delayed remote command through the authoritative simulation, and reports execution. A localhost test ran the server and client as separate Unreal processes; both received the same 4,537-byte keyframe and scoped digest, and the server verified the commanded actor at the requested position. Content passed 27/27; native simulation/protocol coverage passed 28/28 in optimized, debug, and sanitizer configurations; the arm64 editor built; 37/37 Unreal automation tests passed with zero warnings or errors; the normal Meridian runtime passed; and both network processes exited normally. This is one bounded remote-command path, not complete multiplayer: authentication, a player-facing lobby, host/local command parity, ongoing deltas or periodic keyframes, client world presentation, match completion, reconnect, spectators, separate-machine play, latency/loss testing, abuse controls, encryption, and security qualification remain open. The seven-case 0.76.0 pointer/HUD evidence remains accepted at its recorded boundary. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.78.0 has not been packaged, profiled, soaked, notarized, or clean-machine qualified.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with visibility-scoped entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. All sixteen current faction units and structures use distinct project-authored static-mesh candidates. The Future Well has a four-part authored landmark candidate whose Dormant, Harvest, Preserve, and Reshape states differ by geometry as well as color and motion. The Glass Scar has authored shelf, ridge, shard, route, Matter-deposit, and shared-surface candidates; selection, accepted orders, and destruction feedback use registered authored mesh-VFX candidates; command/removal events use a first registered presentation-audio candidate family. Fog, character animation, production textures, final destruction/particle effects, music, voice, ambience, and the final effects mix remain prototype or unimplemented presentation rather than final art/audio. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 66 GiB free at the 0.65.0 source-evidence checkpoint, above the project's 60 GiB packaging floor but below the safer 100 GiB sustained-production target. The connected Seagate APFS drive holds the linked package archive; keep it mounted when using `BuildArtifacts/Packages`. No package or profile run was needed for this bounded mission, art, and site integration. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, thirty-seven Unreal automation tests, null-RHI startup for both selectable faction directions and all seven implemented campaign missions, the controlled prologue-completion path, and the 400-unit/four-team fixture, plus local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
3. Extend the accepted 0.78.0 one-command localhost slice into a real ready/start lobby, identical delayed admission for host and remote commands, periodic scoped keyframes and deltas, client-side view presentation, acknowledgements, and loss/reordering recovery before adding reconnect or playable-multiplayer claims. Authentication, abuse controls, separate-machine testing, and security qualification remain later mandatory gates. In parallel, extend the accepted seven-case pointer combat/Guard matrix into unaided-human usability and dynamic resize coverage, and continue replacing technical primitives under the performance, provenance, and accessibility gates.
4. Follow [Docs/SetupAndBuild.md](Docs/SetupAndBuild.md) and record every accepted result in the ledger.

## Repository map

- `Source/EchoesSimCore`: deterministic, engine-independent RTS simulation
- `Source/EchoesOfTheBrokenSun`: Unreal presentation and interaction module
- `Content/Data/Source`: authoritative source data for factions, units, structures, technologies, and Future Wells
- `Content/Art/Generated`: registered vertical-slice mesh and material candidates
- `site`: static public game archive deployed through GitHub Pages
- `Config`: macOS/Apple Silicon Unreal defaults
- `Tests/Native`: simulation tests runnable before Unreal is installed
- `Docs/DevelopmentBible.md`: game, universe, campaign, UI, audio, accessibility, and player-experience design
- `Docs/TechnicalArchitecture.md`: simulation, Unreal integration, AI, networking, save, replay, and build architecture
- `Docs/ProjectLedger.md`: decisions, roadmap, acceptance evidence, risks, performance budgets, test record, and known limitations
- `Docs/AssetRegister.md`: asset provenance and distribution status

All project documents are authoritative files edited in place. No document is a claim of production readiness.
