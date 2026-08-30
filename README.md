# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype entering its authored Glass Scar vertical slice, not a completed or publicly playable game. Version 0.83.0 at exact clean game-source commit `4f2cf6958f8c59b2e174c504940bef82d9151b73` retains the 0.81 remote-order/complete-match path and 0.82 delivery-fault matrix, then adds one bounded disconnect/reconnect path. The authority reserves seat 1 for 120 real seconds, keeps that seat out of AI control, validates a fixed-length opaque bearer credential through a reliable pre-compatibility RPC, rotates it after use, restores the client batch sequence, derives the exact next simulation command sequence from authority state, and sends a fresh scoped keyframe. A separate invalid-client process was rejected without consuming the legitimate reservation; a later fresh client resumed at batch 2/sequence 1, admitted sequence 2, observed execution, and completed normally. Token values were absent from the server evidence log. The six-case delivery matrix and controlled 24-actor match passed again. Content passed 27/27; native simulation/protocol coverage passed 28/28 in optimized, debug, and sanitizer configurations; the arm64 editor built; 37/37 Unreal automation tests passed with zero warnings or errors; and the normal Meridian runtime passed. This is a localhost Development bearer-resume fixture, not identity authentication, secure credential storage, encrypted transport, separate-machine or internet qualification. Spectators, private invitations, matchmaking, operating-system traffic conditioning, hostile-traffic and abuse qualification, and security review remain open. The 0.81 Metal battlefield review remains accepted at its recorded boundary. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.83.0 has not been packaged, profiled, soaked, notarized, or clean-machine qualified.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with visibility-scoped entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. All sixteen current faction units and structures use distinct project-authored static-mesh candidates. The Future Well has a four-part authored landmark candidate whose Dormant, Harvest, Preserve, and Reshape states differ by geometry as well as color and motion. The Glass Scar has authored shelf, ridge, shard, route, Matter-deposit, and shared-surface candidates; selection, accepted orders, and destruction feedback use registered authored mesh-VFX candidates; command/removal events use a first registered presentation-audio candidate family. Fog, character animation, production textures, final destruction/particle effects, music, voice, ambience, and the final effects mix remain prototype or unimplemented presentation rather than final art/audio. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 66 GiB free at the 0.65.0 source-evidence checkpoint, above the project's 60 GiB packaging floor but below the safer 100 GiB sustained-production target. The connected Seagate APFS drive holds the linked package archive; keep it mounted when using `BuildArtifacts/Packages`. No package or profile run was needed for this bounded mission, art, and site integration. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, thirty-seven Unreal automation tests, null-RHI startup for both selectable faction directions and all seven implemented campaign missions, the controlled prologue-completion path, and the 400-unit/four-team fixture, plus local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
3. Qualify the accepted delivery and reconnect behavior under operating-system traffic conditioning and then separate machines without weakening authority ownership. Replace the Development bearer-resume fixture with authenticated identity, protected credential storage, and encrypted transport before internet-facing claims. Private-lobby/invite behavior, spectators, abuse controls, and security qualification remain mandatory later gates. In parallel, extend the accepted seven-case pointer combat/Guard matrix into unaided-human usability and dynamic resize coverage, and continue replacing technical primitives under the performance, provenance, and accessibility gates.
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
