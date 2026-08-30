# Echoes of the Broken Sun

**Author and creator:** Angelis Pseftis  
**Current state:** This is a verified Unreal playable-systems prototype entering its authored Glass Scar vertical slice, not a completed or publicly playable game. Version 0.67.0 at exact source commit `b4a6ce21c6d170de17c08c64868fa04c94705831` adds player-facing restoration of the validated prior campaign generation. The title shows active and prior record counts, requires a 30-second two-press confirmation through Page Up (with F11 retained as an alternate), atomically activates the prior generation, and retains the replaced active generation as the next backup so the operation is reversible. Exact-generation validation fails closed, and a corrupt primary can no longer displace the last valid backup during a later save. Content passed 27/27 with unchanged canonical SHA-256 `100f1fcd184cf94fe9b21d3f591714a2e33cc92b60f018bc6523868675156fa0`; native simulation passed 27/27 in optimized, debug, and sanitizer configurations; the arm64 editor built; 34/34 Unreal automation tests passed without warning or error; and the normal Meridian runtime booted. An exact-commit Metal run restored a seven-record prior generation over an empty active generation through the visible player control, retained the empty generation as backup, rendered the resulting `ACTIVE 7` / `RESTORE PRIOR 0` state, and exited normally. This closes one-generation backup restoration, not a campaign-slot browser, Mission 08, the remaining campaign, physical-human usability, current packaging, multiplayer, final art/audio, or release readiness. The latest accepted package remains clean-source 0.23.1 at commit `98a9b83`; version 0.67.0 has not been packaged, profiled, soaked, notarized, or clean-machine qualified.

*Echoes of the Broken Sun* is an original science-fantasy real-time strategy game for macOS. Its central strategic resource, the Future Well, forces a player to choose between immediate power, sustained possibility, and temporary transformation of the battlefield.

The repository is intentionally evidence-bounded. A file or menu stub is not treated as a working feature. The current implementation includes an Unreal Engine 5.8 project, Apple Silicon configuration, source-controlled balance definitions, a tool-independent deterministic C++ simulation, and a runtime Unreal adapter with visibility-scoped entity views, RTS camera, selection/controller layer, HUD, opponent AI, construction, one-slot unit production, logistics capacity, pause/restart, Command-Core victory state, and Future Well orders. All sixteen current faction units and structures use distinct project-authored static-mesh candidates. The Future Well has a four-part authored landmark candidate whose Dormant, Harvest, Preserve, and Reshape states differ by geometry as well as color and motion. The Glass Scar now has authored shelf, ridge, shard, route, Matter-deposit, and shared-surface candidates. Fog, effects, animation, production textures, destruction, and audio remain prototype presentation rather than final art. Test results and remaining limitations are recorded in [Docs/ProjectLedger.md](Docs/ProjectLedger.md).

## Current development gate

Unreal Engine 5.8.2 is installed at `/Users/Shared/Epic Games/UE_5.8` and its completed launcher manifest has been inspected. The separately delivered Apple Metal Toolchain is installed. The host is a 16 GB M1 Pro Mac with about 66 GiB free at the 0.65.0 source-evidence checkpoint, above the project's 60 GiB packaging floor but below the safer 100 GiB sustained-production target. The connected Seagate APFS drive holds the linked package archive; keep it mounted when using `BuildArtifacts/Packages`. No package or profile run was needed for this bounded mission, art, and site integration. A verified official launcher-installer backup exists at `~/Downloads/EpicInstaller-20.1.4.dmg`, but it should not replace the newer installed launcher.

The current Unreal integration gate is:

1. Stop prototype builds below 40 GiB free, restore at least 60 GiB before large imports or release packaging, and target 100 GiB for sustained production. These are project engineering thresholds, not Epic-published minimums.
2. Keep full Xcode selected. Xcode 26.6 has passed project generation, arm64 editor compilation, thirty-four Unreal automation tests, null-RHI startup for both selectable faction directions and all seven implemented campaign missions, the controlled prologue-completion path, and the 400-unit/four-team fixture, plus local Development cooking/packaging; Epic still recommends 26.1.1, and clean-machine use remains unverified.
3. Extend the accepted pointer gate into combat, Guard, adverse camera/UI-scale conditions, and unaided human usability; continue replacing technical primitives with authored effects, materials, and audio under the performance, provenance, and accessibility gates. The first code-only Scar composition is accepted at version 0.51.0, faction/role silhouettes at 0.52.0, accepted-order markers at 0.53.0, five-unit destination layouts at 0.54.0, a contextual command-deck hierarchy at 0.55.0, an edge-aware anonymous-contact cue at 0.56.0, the first bounded campaign prologue at 0.57.0, consequence-consuming missions through the first seven-record Kharuun listening operation at 0.65.0, exact-coordinate pointer delivery plus player-driven research interruption at 0.66.0, and reversible prior-generation campaign restoration at 0.67.0. None is final art or broad usability, formation, campaign, or package qualification. The bounded keyboard-driven construction, production, research-completion, Future Well, tactical-control, victory, result, and post-result restart paths remain accepted at version 0.50.0.
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
