---
title: Echoes of the Broken Sun Asset and License Register
author: Angelis Pseftis
creator: Angelis Pseftis
status: Authoritative asset provenance register
created: 2026-08-28
updated: 2026-09-04
---

# Asset and License Register

This is the single authoritative provenance register and is edited in place. An asset is distributable only when its source, author, license or assignment, modification record, and shipped files are recorded here. Repository presence alone does not establish distribution rights.

[Requirements.md](../Requirements.md) owns product criteria; [RequirementsState.md](../RequirementsState.md)
owns lifecycle and owner acceptance. Verification notes in asset rows describe only their cited source,
revision, and evidence boundary. Follow [AGENTS.md](../../AGENTS.md) for generation, ownership, and claims.

| ID | Asset or family | Source | Rights holder / creator | License or authorization | Use | Status |
|---|---|---|---|---|---|---|
| DATA-001 | Faction, unit, structure, and Future Well source definitions | Original project data | Angelis Pseftis | Project-owned original work | Simulation and balance | Approved source data |
| CODE-001 | Project gameplay and simulation source | Original project code | Angelis Pseftis | Project-owned original work | Runtime and tests | Approved source code |
| UE-ENGINE | Unreal Engine runtime/editor | Epic Games | Epic Games | Applicable Unreal Engine EULA | Engine dependency; not vendored | UE 5.8.2 installed locally |
| PLACEHOLDER-001 | Engine basic-shape geometry, default materials, and debug primitives | Unreal Engine installation | Epic Games | Applicable Unreal Engine EULA | Development placeholder only | In use by the runtime prototype; not final art |
| ART-001 | Twenty-four procedural static-mesh roster candidates (8 Meridian, 8 Kharuun, 8 Choir) and shared surface material under `Content/Art/Generated` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Runtime roster presentation | Integrated development candidate (revision `roster-silhouette-v2`); not final art |
| ART-002 | Four-part procedural Future Well landmark under `Content/Art/Generated/World/Landmarks` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Runtime Dormant, Harvest, Preserve, and Reshape presentation | Integrated vertical-slice candidate; not final art |
| ART-003 | Seven procedural Glass Scar terrain, route, and Matter-deposit meshes plus a shared world-surface material under `Content/Art/Generated/World` and `Content/Art/Generated/Materials` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Runtime Glass Scar environment and resource presentation | Integrated vertical-slice candidate; not final art |
| ART-004 | Production-oriented Ash Cut route kit: revised two-LOD mesh, UV-driven master material, and four material instances under `Content/Art/Generated` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Runtime Ash Cut route presentation and production-pipeline acceptance | Integrated production-oriented candidate; not final environment art |
| ART-005 | Nine selection/command presentation meshes and one shared emissive material under `Content/Art/Generated/VFX` and `Content/Art/Generated/Materials` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Selected-entity and accepted-order presentation | Integrated production-oriented mesh-VFX candidate; not final effects |
| ART-006 | Three destruction-state presentation meshes under `Content/Art/Generated/VFX`, using the ART-005 shared emissive material | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Presentation after an authoritative unit/structure removal | Integrated geometry-driven destruction candidate; not final effects |
| ART-007 | Production-oriented Buried Causeway route kit: revised two-LOD mesh, UV-driven master material, and four material instances under `Content/Art/Generated` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Runtime Buried Causeway presentation and production-pipeline acceptance | Integrated production-oriented candidate; not final environment art |
| ART-008 | Production-oriented Folded Verge route kit: revised two-LOD mesh, UV-driven master material, and four material instances under `Content/Art/Generated` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Runtime Folded Verge presentation and production-pipeline acceptance | Integrated production-oriented candidate; not final environment art |
| AUDIO-001 | Three mono PCM sources under `Content/Audio/Source` and imported SoundWave assets under `Content/Audio/Generated` | `Scripts/generate_audio_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no recordings, samples, models, or third-party source audio | Accepted-command and faction-distinct functional-loss confirmation | Integrated presentation-audio candidate; not final mix or complete audio family |
| AUDIO-002 | Twelve interface and alert cues (`UI_*`, `ALERT_*`) under `Content/Audio/Source` and imported SoundWave assets under `/Game/Audio/Generated` | `Scripts/echoes_audio_synth.py` revision `interface-audio-v1`, imported through `Scripts/generate_audio_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no recordings, samples, models, or third-party source audio | Menu focus/select/confirm/reject, panel open/close, brief advance, and five rate-limited alerts | Integrated interface audio system (Gate 15 / B5); routing to Interface submix, 60 ms UI rate limiting, 4 s alert cooldown with terminal alert exemption, volume muting; verified via `Echoes.Runtime.Audio.InterfaceCues` PASS |
| AUDIO-003 | Eighteen gameplay cues (`SFX_Weapon*`, `SFX_Impact*`, Matter, construction, production, research, Well, Reshape) under `Content/Audio/Source` and imported SoundWave assets | `Scripts/echoes_audio_synth.py` revision `gameplay-audio-v1`, imported through `Scripts/generate_audio_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no recordings, samples, models, or third-party source audio | Authoritative gameplay event confirmation | Integrated gameplay audio system (Gate 14 / B4); routing to Effects submix, 300–4,200 uu linear attenuation, combat-load admission cooldowns, and non-cheating observer dispatch; verified via `Echoes.Runtime.Audio.GameplayCues` PASS |
| AUDIO-004 | Fifteen music cues (title, three faction themes, three act beds, tension/combat layers, victory/defeat, four ending stingers) as looping/one-shot stereo sources and imported SoundWave assets | `Scripts/echoes_audio_synth.py` revision `music-v3` (music-v1 masters re-ceilinged by transparent gain trim so 4x-oversampled true peak ≤ −1 dBTP, B6), imported through `Scripts/generate_audio_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no recordings, samples, models, or third-party source audio | Title/faction/act music, threat layers, and result punctuation | Integrated musical score (Gate 12 / B2); routing to Music submix, crossfade between contexts, true peak ≤ −1.41 dBTP; verified via `Echoes.Runtime.Audio.MusicAmbience` PASS |
| AUDIO-005 | Five looping stereo ambience beds (Glass Scar, Lume Reach, ark-city, Crownfall, Future Well proximity) and imported SoundWave assets | `Scripts/echoes_audio_synth.py` revision `ambience-v1`, imported through `Scripts/generate_audio_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no recordings, samples, models, or third-party source audio | Site ambience and Well proximity presentation | Integrated site ambience system (Gate 13 / B3); routing to Ambience submix, seamless site crossfade, Well proximity layer, true peak ≤ −2.98 dBTP; verified via `Echoes.Runtime.Audio.MusicAmbience` PASS |
| AUDIO-006 | Fourteen music-v4 cues: tension and combat layers for all six faction pairings plus brief and results underscores, as looping stereo sources and imported SoundWaves | `Scripts/echoes_audio_synth.py` revision `music-v4` (music-v2 masters re-ceilinged by transparent gain trim so 4x-oversampled true peak ≤ −1 dBTP, B6), imported through `Scripts/generate_audio_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no recordings, samples, models, or third-party source audio | Matchup-specific threat layers and screen underscores | Integrated faction-pairing threat score (Gate 12 / B2); order-independent matchup pairing, true peak ≤ −1.41 dBTP; verified via `Echoes.Runtime.Audio.MusicAmbience` PASS |
| AUDIO-007 | Four looping stereo biome ambience beds (`AMB_ShivergrassSteppe`, `AMB_CavernArtery`, `AMB_FoundryVoid`, `AMB_SolarFallDais`) completing the Soryn planetary biome set, under `Content/Audio/Source` and imported SoundWave assets | `Scripts/echoes_audio_synth.py` revision `biome-ambience-v1`, imported through `Scripts/generate_audio_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no recordings, samples, models, or third-party source audio | Regional biome site ambience and campaign immersion | Authorized site ambience specification (Gate 13 / B3); BS.1770-4 compliant (-16 LUFS envelope, true peak ≤ -1.0 dBTP) |
| ART-011 | Twelve deterministic surface-texture maps (`surface-textures-v5`): the three A3 families plus the Glass Scar ground family — dark vitrified basalt laced with sparse golden fracture arteries after the site hero reference: ceramic civic paneling, vitrified glass with magenta micro-fracture, and causeway ash strata — base color, packed metallic/roughness/emissive, and normal per family, 512², imported as `Texture2D` under `/Game/Art/Generated/Textures` | `Scripts/echoes_texture_synth.py` (pure-Python deterministic synthesis, per-family seeds, byte-idempotent PNG) imported through `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no photographs, scans, models, or third-party source imagery | A3 surface families driving `M_EchoesSurface` (`surface-textured-v6`: ceramic defaults sampled under per-instance tint, UVScale 0.01, MRE-driven roughness/emissive, normal map); per-family instance overrides for glass and ash pending | Rendered compilation-gated battlefield capture accepted (clipped 0.00427%); remaining A3 families and per-family Metal composition reviews open |
| ART-012 | Fifteen deterministic surface-texture maps completing the A3 family set (`surface-textures-v8`; the twelve ART-011 maps regenerate byte-identically under the new revision): Compact machined metal with milled panel seams, brushed grain, and a chipped painted status band; Kharuun grown mineral with warped strata and translucent amber nodules carrying an emissive mask; Hollow Choir coherent-light surface — near-black body under a luminous edge lattice whose every line has an offset duplicate; Matter deposit crystal with wrap-aware facet planes, bright cleavage edges, and a cyan-white interior glow mask; and Folded Verge plate scoring with settled ash drifts — base color, packed metallic/roughness/emissive, and normal per family, 512², imported as `Texture2D` under `/Game/Art/Generated/Textures`. All five are authored as modulation maps around the ceramic albedo so the accepted per-slot tint tables keep their meaning | `Scripts/echoes_texture_synth.py` (pure-Python deterministic synthesis, per-family seeds 505–909, byte-idempotent PNG) imported through `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original deterministic synthesis; no photographs, scans, models, or third-party source imagery | `M_EchoesSurface` `surface-textured-v7` adds `EmissiveTint`, `MaskedEmissiveStrength`, and a Fresnel `ViewShift` path (defaults render every pre-v7 instance unchanged); the entity view binds Compact metal + ceramic plates to Meridian bodies, mineral to Kharuun, coherent-light to the Choir (ViewShift 0.6, zeroed under reduced motion), and crystal to Matter deposits. `T_EchoesVergeScored` is imported and registered but not yet bound to the Folded Verge route master | Rendered editor review captures at 1920×1080 measured (Meridian roster clipped 0.0001% / mean luma 62.8; Kharuun roster 0.0001% / 63.9; Choir roster 0.0001% / 62.7; whole-basin overview 0.0000% / 32.2 (far-zoom framing, not a gameplay-window measure)); isolated per-family Metal composition review, packaged capture, high-contrast capture, and route-kit binding still open (ledger ART-A3-002) |
| ART-013 | Procedural Broken Sun celestial sky object `SM_World_BrokenSunSky` under `Content/Art/Generated/World/Environment` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Celestial sky presentation of the shattered star Soryn | Integrated vertical-slice candidate; not final art |
| ART-014 | Procedural code-driven component motion families in `AEchoesEntityView` (walkers, hover, idle micro-motion, tactical states, worker gathering) | `Source/EchoesOfTheBrokenSun/Private/EchoesEntityView.cpp` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work | Presentation-only component motion keyed to authoritative state with zero simulation touch and reduced-motion compliance | Integrated production motion system (Gate 7 / A5); verified via `Echoes.Runtime.Presentation.MotionFamilies`, 96/96 native sim tests, and runtime smoke |
| ART-015 | Procedural code-driven combat and interaction feedback effect views (`AEchoesCombatEffectView`, `AEchoesEntityView` gather/construction/reshape effects) | `Source/EchoesOfTheBrokenSun/Private/EchoesCombatEffectView.cpp` and `EchoesEntityView.cpp` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work | Presentation-only combat effects (weapon beams, muzzles, impact bursts, gather beams, assembly fields, Reshape sigils) with zero simulation touch, pooled allocation, and reduced motion/flashing compliance | Integrated production combat effects system (Gate 8 / A6); verified via `Echoes.Runtime.Presentation.CombatEffects`, 96/96 native sim tests, and runtime smoke |
| ART-016 | Production fog of war and memory shroud presentation views (`AEchoesFogView`) across the authoritative 64×64 visibility grid | `Source/EchoesOfTheBrokenSun/Private/EchoesFogView.cpp` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work | Presentation-only fog and shroud (deep charcoal basalt unexplored volume with magenta fracture bleed; desaturated slate-indigo explored memory layer over persistent geometry; full palette visible) with zero simulation touch, disabled collision/nav/shadows/overlaps, and reduced motion/flashing compliance | Integrated production fog and shroud system (Gate 9 / A7); synchronization measured within ≤1.5 ms budget across 4,096 tiles; verified via `Echoes.Runtime.Presentation.ProductionFog`, 96/96 native sim tests, and runtime smoke |
| ART-017 | Procedural Shivergrass resonance clumps (`SM_World_ShivergrassClump_01..03`) and Vaultback megafauna environmental shell props under `Content/Art/Generated/World/Shivergrass` | `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Presentation-only dressing for Shivergrass Steppes (M02, M07) | Authorized biome dressing specification (Gate 6 / A4) |
| ART-018 | Procedural subterranean crystal geode columns (`SM_World_GeodePillar_01..02`), transit span bridges (`SM_World_CrystalSpan_Bridge`), and stalactite clusters under `Content/Art/Generated/World/Caverns` | `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Presentation-only dressing for Subterranean Caverns (M04, M12) | Authorized biome dressing specification (Gate 6 / A4) |
| ART-019 | Procedural brutalist ark-city concrete buttresses (`SM_World_ConcreteButtress_01`), industrial silos, and census vault grates under `Content/Art/Generated/World/ArkCity` | `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Presentation-only dressing for Sheared Ark-City Foundries (M03, M06, M10) | Authorized biome dressing specification (Gate 6 / A4) |
| ART-020 | Procedural acoustic resonator monoliths (`SM_World_AcousticMonolith_01..02`) and temporal refractor spurs under `Content/Art/Generated/World/Void` | `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Presentation-only dressing for Crownfall Void Horizons (M05, M08, M09, M11, M14) | Authorized biome dressing specification (Gate 6 / A4) |
| ART-021 | Procedural obsidian dais platform tiles (`SM_World_ObsidianDaisTile_01..02`) and coronal flare emitters under `Content/Art/Generated/World/SolarDais` | `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Presentation-only dressing for Solar Fall Dais (M13, M15) | Authorized biome dressing specification (Gate 6 / A4) |
| ART-022 | Procedural skirmish competitive props: destructible frostshard drifts and acoustic line-of-sight pillars under `Content/Art/Generated/World/Skirmish` | `Scripts/generate_art_assets.py` in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Presentation-only dressing for Skirmish Theaters (S01–S06) | Authorized biome dressing specification (Gate 6 / A4) |
| ART-WORLD-001 | Six original formation/low-relief kit families and walk substrate, plus scoped runtime dressing | `Scripts/echoes_world_kits.py`, `Scripts/generate_art_assets.py`; `soryn-world-kits-v5` | Angelis Pseftis | Project-owned original geometry; Unreal engine operations and neutral white texture under UE-ENGINE | Reusable world presentation foundation | Generated candidate; per-mission composition and final qualification incomplete |
| ART-WORLD-002 | `M_EchoesSky`, altitude gradient | `Scripts/generate_art_assets.py`; `soryn-sky-gradient-v1` | Angelis Pseftis | Project-owned original material graph | Shared outdoor sky presentation | Generated and editor rendered; final sky composition incomplete |
| ART-WORLD-003 | `SM_VFX_AbilityRangeRing`, true-radius power/supply boundary | `Scripts/generate_art_assets.py`; `ability-range-ring-v1` | Angelis Pseftis | Project-owned original geometry and material | State-bound ability range | Generated; radius automation and rendered review pending |
| ART-WORLD-004 | `M_EchoesShivergrassLeaf`, two-sided foliage | `Scripts/generate_art_assets.py`; `shivergrass-leaf-v1` | Angelis Pseftis | Project-owned original shader; no source texture | Silver foliage lighting | Generated; lit runtime verification pending |
| CONCEPT-001 | Four 2x2 Meridian/Kharuun unit and structure presentation sheets under `site/assets/concepts` | OpenAI image generation through Codex, 2026-08-29; exact prompts below; no source images | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Arsenal visual targets | Development concept reference; not a runtime or production asset |
| CONCEPT-002 | Four-state Future Well presentation sheet at `site/assets/concepts/future-well-states.png` | OpenAI image generation through Codex, 2026-08-29; exact prompt below; no source image | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Future Well visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-003 | Four-view Glass Scar environment and route presentation sheet at `site/assets/concepts/glass-scar-routes.png` | OpenAI image generation through Codex, 2026-08-29; exact prompt below; no source image | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Glass Scar visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-004 | Composed Glass Scar vertical-slice presentation render at `site/assets/concepts/target-render-vertical-slice.jpg` (1,376 × 768) | Owner-supplied concept image placed in the repository on 2026-09-04; generation service, prompt, and source inputs are not recorded in the repository (see the CONCEPT-004 provenance record below) | Direction and project authorship: Angelis Pseftis; generated output: service not recorded | Retain for project concept presentation only; provenance record incomplete — owner to record service, date, prompt, and output terms before any release-rights review | Public composed visual target (site vision page and home feature) | Development concept reference; not a runtime, production, or gameplay-capture asset; PROVENANCE INCOMPLETE |
| CONCEPT-005 | Painted hero backdrop `site/hero-soryn.png` (1,672 × 941) used as the home hero and story-feature background and mirrored under `website/public/` | Owner-supplied concept image first committed on 2026-08-29 (commit `8dd0ef6`); generation service, prompt, and source inputs are not recorded in the repository (see the CONCEPT-005 provenance record below) | Direction and project authorship: Angelis Pseftis; generated output: service not recorded | Retain for project concept presentation only; provenance record incomplete — owner to record service, date, prompt, and output terms before any release-rights review | Public site backdrop; art-direction reference for the Glass Scar identity theme (ledger ART-A4-001) | Development concept reference; not a runtime, production, or gameplay-capture asset; PROVENANCE INCOMPLETE |
| CONCEPT-006 | Soryn Continental Cartography and Regional Artery Map at `site/assets/concepts/soryn-world-map.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-04; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and tactical atlas | Public Continental World Map visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-007 | Shivergrass Basin & Vaultback Prairie Environment Target at `site/assets/concepts/shivergrass-basin.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-04; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and tactical atlas | Public Mission 02 Shivergrass Basin visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-008 | Subterranean Cavern Artery & Crystal Road Target at `site/assets/concepts/unburied-road-caverns.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-04; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and tactical atlas | Public Mission 04 Unburied Road visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-009 | Ark-City Sector 9 Sheared Foundation Void Target at `site/assets/concepts/arkcity-census-void.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-04; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and tactical atlas | Public Mission 06 Ark-City Sector 9 visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-010 | Solar Fall Dais & Shattered Broken Sun Target at `site/assets/concepts/broken-sun-solar-dais.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-04; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and tactical atlas | Public Mission 15 Solar Fall Dais visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-011 | Original Modern Floating Contextual Command Arc HUD at `site/assets/concepts/echoes-hud-modern-arc.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and UI design | Public Modern HUD visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-012 | Traditional 3-Box Command Console Baseline at `site/assets/concepts/echoes-hud-traditional-console.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for comparative analysis against SC2 baseline | Public Traditional HUD baseline reference | Development concept reference; not a runtime or production asset |
| CONCEPT-013 | Diegetic Ark-City Command Bridge Main Menu at `site/assets/concepts/echoes-main-menu-bridge.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and UI design | Public Main Menu visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-014 | Traditional Left-Stacked Shell Menu Baseline at `site/assets/concepts/echoes-main-menu-traditional.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for comparative analysis against SC2 baseline | Public Traditional Menu baseline reference | Development concept reference; not a runtime or production asset |
| CONCEPT-015 | Tri-Faction Strategic War Room & Selection at `site/assets/concepts/echoes-faction-selection.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and UI design | Public Faction Selection visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-016 | Future Well Celestial Battle Landmark at `site/assets/concepts/echoes-future-well-landmark.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and landmark design | Public Future Well Landmark visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-017 | Meridian Compact Industrial Structures Architecture Sheet at `site/assets/concepts/echoes-meridian-structures.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and structure design | Public Meridian Structures visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-018 | Kharuun Assemblies Living Geological Structures Architecture Sheet at `site/assets/concepts/echoes-kharuun-structures.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and structure design | Public Kharuun Structures visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-019 | Hollow Choir Phase-Uncertain Structures Architecture Sheet at `site/assets/concepts/echoes-choir-structures.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and structure design | Public Hollow Choir Structures visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-020 | Planetary Campaign Operations Map & Mission Dossier at `site/assets/concepts/echoes-campaign-map.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and UI design | Public Campaign Operations Map visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-021 | Mara's Field Operations Ledger / In-Game Pause Menu at `site/assets/concepts/echoes-pause-field-ledger.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and UI design | Public Field Pause Menu visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-022 | Post-Match Combat Debriefing & Telemetry Curves at `site/assets/concepts/echoes-post-match-debriefing.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and UI design | Public Post-Match Debriefing visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-023 | System Configuration, Loudness & Accessibility Terminal at `site/assets/concepts/echoes-settings-accessibility.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and UI design | Public Settings & Accessibility visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-024 | Tri-Faction Combat Visual Effects Grammar at `site/assets/concepts/echoes-combat-vfx-grammar.jpg` (1,376 × 768) and WebP | Project concept generation, 2026-09-05; prompt recorded below | Direction and project authorship: Angelis Pseftis | Retain for project concept presentation and VFX design | Public Combat VFX Grammar visual target | Development concept reference; not a runtime or production asset |
| CAPTURE-001 | Meridian and Kharuun in-engine roster captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-001 | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development capture; not package or final-art evidence |
| CAPTURE-002 | Dormant, Harvest, Preserve, and Reshape Future Well captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-002 and the non-shipping art-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development captures; not package, gameplay-usability, or final-art evidence |
| CAPTURE-003 | Glass Scar overview, Ash Cut, Buried Causeway, and Folded Verge captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-002, ART-003, and the non-shipping environment-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development captures; not package, gameplay-usability, or final-art evidence |
| CAPTURE-004 | Exact-source 0.68.0 Ash Cut Metal review frame retained with local acceptance evidence | Local UE 5.8.2 Metal editor run using ART-004 and the non-shipping environment-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Internal production-route acceptance evidence | Exact-source isolated review capture; not package, combat-readability, or final-art evidence |
| CAPTURE-005 | Exact-source 0.69.0 standard and reduced-accessibility selection/command Metal review frames retained with local acceptance evidence | Local UE 5.8.2 Metal editor runs using ART-005 and the non-shipping presentation-VFX review fixture | Angelis Pseftis | Project-owned derivative captures; Unreal Engine subject to its applicable EULA | Internal presentation/accessibility acceptance evidence | Exact-source isolated review captures; not package, combat-usability, or final-effects evidence |
| CAPTURE-006 | Exact-source 0.70.0 standard and reduced-accessibility destruction-state Metal review frames retained with local acceptance evidence | Local UE 5.8.2 Metal editor runs using ART-006 and the non-shipping destruction-VFX review fixture | Angelis Pseftis | Project-owned derivative captures; Unreal Engine subject to its applicable EULA | Internal destruction/accessibility acceptance evidence | Exact-source isolated review captures; not package, broad combat-load, audio, or final-effects evidence |
| CAPTURE-007 | Exact-source 0.72.0 Buried Causeway Metal review frame retained with local acceptance evidence | Local UE 5.8.2 Metal editor run using ART-007 and the non-shipping environment-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Internal production-route acceptance evidence | Exact-source isolated review capture; not package, combat-readability, or final-art evidence |
| CAPTURE-008 | Exact-source 0.73.0 Folded Verge Metal review frame retained with local acceptance evidence | Local UE 5.8.2 Metal editor run using ART-008 and the non-shipping environment-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Internal production-route acceptance evidence | Exact-source isolated review capture; not package, combat-readability, or final-art evidence |
| CAPTURE-009 | Exact-source 0.74.0 pointer combat/Guard Metal review frame retained with local acceptance evidence | Local UE 5.8.2 Metal editor run using ART-001, ART-003, ART-005, and the non-shipping pointer-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Internal exact-coordinate controller/simulation integration evidence | Exact-source controlled review capture; not OS-injected input, unaided-human usability, adverse camera/UI-scale, package, balance, or final-effects evidence |
| CAPTURE-010 | Exact-package 0.93.0 Glass Scar overview and ordinary battlefield Metal frames retained with local acceptance evidence | Local Mac-arm64 Development package from clean commit `dab16f281b9fe3fe84f463b556fe23dd56bd36ca`, using ART-001 through ART-003 and the non-shipping review cameras | Angelis Pseftis | Project-owned derivative captures; Unreal Engine subject to its applicable EULA | Internal packaged `M_EchoesWorldSurface` instancing-compatibility evidence | Exact-package local Metal captures; not final lighting, UI, texture, performance, clean-machine, signing, notarization, or release evidence |

## ART-001 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`.
- Output: one shared material plus twenty-four named static meshes across three factions:
  - Meridian Compact: Surveyor, Lancer, Bulwark, Relay Skiff, Anchor, Power Link, Array Foundry, Aegis Post.
  - Kharuun Assemblies: Tender, Riftstalker, Cairnback, Resonant, Memory Hearth, Waystone, Growth Basin, Listening Spine.
  - Hollow Choir: Threadkeeper, Intervalist, Lacuna Warden, Afterimage, Concordance, Interval Loom, Chorus Loom, Phase Anchor.
- Each roster mesh is authored from project-defined primitive composition, contains two LODs and four material zones with explicit Zone 0 (`PRIMARY`) team-color armor plates and status cowlings, and uses visibility-selection collision. Generation does not import Marketplace, stock, scanned, or third-party source geometry.
- Revision `roster-silhouette-v2` is tagged on all 24 roster assets via metadata tag `Echoes.AssetRevision`.
- The Unreal asset metadata records `Creator=Angelis Pseftis`, `Provenance=Original procedural geometry generated in-project`, and `Status=Vertical-slice art candidate; not final art`.

## ART-002 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`.
- Output: `SM_World_FutureWellBase`, `SM_World_FutureWellOrbit`, `SM_World_FutureWellCore`, and `SM_World_FutureWellGlyph` under `Content/Art/Generated/World/Landmarks`.
- The Base asset forms the radial foundation and six broken pylons. Orbit supplies segmented counter-rotating rings. Core supplies the faceted central aperture and suspended shards. Glyph supplies authored ground paths. Runtime transforms combine those four parts into geometry-distinct Dormant, Harvest, Preserve, and Reshape states.
- Each asset has two LODs, four material zones, and visibility-selection collision. Generation uses only project-authored primitive composition and Unreal-provided geometry operations; it imports no Marketplace, stock, scanned, generated-image, or third-party source geometry.
- Recorded LOD0/LOD1 triangle counts are Base 1,472/736; Orbit 396/198; Core 552/276; and Glyph 504/252. SHA-256 values are Base `d9072bd72ab8db31da814647724bd12cbd35f2e43d2a1db5611ba5057e63ebe6`, Orbit `7a2bb3f92aefca22317a11550ce4e8f5234cfb7880d8d09918ea671df749374f`, Core `928bfad25cb17d6282d2a22d1c99b28a7e1b6e8ea50f7591192b2fdbf3577a0d`, and Glyph `4d5e61a3cdfaae2e5435a9b352ade37d5e38ec54266947b36d68dd2cce5d9ff0`.

## ART-003 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`; accepted generation-log SHA-256 `ae77b6c2550e866c5670b2c772964198e7bb814b33e828b9608d17e87506b549`.
- Output: `SM_World_GlassScarShelf`, `SM_World_GlassScarRidge`, `SM_World_GlassScarShard`, `SM_World_GlassScarAshCut`, `SM_World_GlassScarBuriedCauseway`, and `SM_World_GlassScarFoldedVerge` under `Content/Art/Generated/World/Environment`; `SM_World_MatterDeposit` under `Content/Art/Generated/World/Resources`; and `M_EchoesWorldSurface` under `Content/Art/Generated/Materials`.
- Shelf, ridge, shard, and route actors replace visible Engine-primitive terrain dressing while remaining non-colliding presentation. The authored Matter-deposit mesh replaces the visible resource-node primitive. The unchanged deterministic grid, resource state, pathing, placement, line-of-sight, and collision-floor authority remain outside these assets.
- Each mesh contains two LODs and four material zones. Recorded LOD0/LOD1 triangle counts are Shelf 396/198; Ridge 204/102; Shard 180/90; Ash Cut 516/258; Buried Causeway 420/210; Folded Verge 524/262; and Matter Deposit 546/272.
- SHA-256 values are Shelf `33a269540156f3e83e98235da21813791597a12ac4fda138c3c8dcddcd7639d1`, Ridge `f5c6a9cab7b06fb7c6fcf2d201566bafa9dc41bb64ea9e7f80eb2e09bef6795f`, Shard `7ca7ee6e05e58ee583cb4114f3903226b0d5c1c82a0f1effcb5f0c62d96d6c67`, Ash Cut `7482c7ece6823dbffd15b13867a8bc1d6c30cfbd42a8d119eb7361e58f78db`, Buried Causeway `5bb20566ce4cceff1144e91ffd501f7f55194783bca5d46c027fed39741bfe0d`, Folded Verge `ad7e8bf4d19c95ca7e92a409c7ed2faa426563711d17ff7670c49c252f3a71f6`, Matter Deposit `9338bd089ffcbcd4d18ee67f012334ba41446eaff759fa48526b132d2bf66c84`, and world material `80e7dc20a8d497770f20030918d4336a3690751df75637f41542204515c3aa5b`.
- The ART-003 Ash Cut and Buried Causeway counts and hashes identify superseded 0.65.0 candidates retained in historical evidence. Versions 0.68.0 and 0.72.0 replace those asset paths; current counts and hashes are registered under ART-004 and ART-007.
- Revision `world-surface-instancing-v1` at exact implementation commit `dab16f281b9fe3fe84f463b556fe23dd56bd36ca` repairs the existing `M_EchoesWorldSurface` in place, enables and verifies `MATUSAGE_INSTANCED_STATIC_MESHES`, and makes the generator fail closed if the usage or save cannot be established. A post-commit generation reported `action=reused instancedStaticMeshes=true`, retained byte-identical material SHA-256 `21af585b2be0c076b5caa8386ccdbbbfc1df8cd215ef67c4703c2d61ab2b79e1`, and left the checkout clean.
- CAPTURE-010 used the exact clean-source Development package manifest SHA-256 `5dbd1071af7da9d563a69f825a44ef099013ecd01eabedf3c88af845bca1f0ee`. Both Metal sessions exited normally and contained none of the prior instancing/default-material fallback markers. Overview and ordinary-battlefield PNG SHA-256 values are `618fe0082d44cbc6cc31cad49298675341088626b5ca70c995b8c3dc9535592f` and `c731be4e9100bdf521be706db43e372238994f0c994a925a9560faf5ffd28b17`; corresponding runtime-log SHA-256 values are `bda1dcd7579603dd863655f2aab48d26cf1ae7bbc5432aaec10e9083d7bd5dac` and `f42c8df6361355037821784ebbf191bf4b9747a9c4640710dc366622b6366870`. Visual inspection found no checkerboard or missing-material substitution. The ordinary frame also exposes unresolved blown highlights, a dense debug-like control overlay, and prototype-state text; it is not final-quality art evidence.

## ART-004 generation record

- Exact accepted source: version 0.68.0 commit `ee06313a41cb289279bc89711a72fb9f83eddbd8`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `576b3ecd33e6aaf3746e0a96754c7a99d986588a954e8068171a194707b5741d`.
- Output: revised `SM_World_GlassScarAshCut`, `M_GlassScarAshCut`, and `MI_GlassScarAshCut_Basalt`, `MI_GlassScarAshCut_Ash`, `MI_GlassScarAshCut_Glass`, and `MI_GlassScarAshCut_Vein`.
- The route mesh contains a continuous seven-segment bed, staggered bank strata, broken edge slabs, three paired shard-fin landmarks, and a continuous emissive seam. LOD0/LOD1 contain 1,360/680 triangles. Both LODs have UV0 surface mapping and UV1 lightmap data. The mesh contains four material zones and one simple box collision primitive. Runtime collision, overlap generation, and navigation influence are disabled before presentation; authoritative route/pathing state is unchanged.
- The material family uses project-authored UV-driven noise variation and distinct basalt, ash, glass, and emissive-vein physical parameters. It uses no imported scan, stock texture, Marketplace asset, generated image, or third-party source material.
- Revision `ash-cut-production-v1` is recorded on the mesh, master material, and instances. An immediate exact-source regeneration retained identical tracked bytes and a clean checkout.
- SHA-256 values are Ash Cut mesh `4c0ce206abbac9232aee88baeaaf09eade37bed78f7c9c0f943565c5343e55da`, master material `d0d50c601ee299ef2c57bc0136e4b13a121b9668e7fdf53673f1e8d493216955`, Ash instance `370363ea2ee889194896703d890ead5e44c5264ffe6c494b091801c862d80649`, Basalt instance `6f7d7a757801ceaf0829dabcc07031cd9b90a1dce13e5869b550c1a225e139fa`, Glass instance `ae128d015bc3470cd83cdd22dcb2460f5a04d6eec824a490e75a6c9b38a59c92`, and Vein instance `d555c69c31d6777acfcec5d2e2221286b72dbb23ed9980dbf08c11769e96d722`.
- Exact-commit Metal log and 1,319 × 768 frame SHA-256 values are `546d5d7ac530a901663c5b9c8a75447142479d00d80e5c3a83a2900302fd079f` and `20e60a75bc22d1b09d756efe5ce6b1b4e035a3470574709933c038a688d1c4ac`. The isolated frame accepts the intended layered route/material separation at one review camera. It does not qualify production textures, broad cameras, combat-load readability, destruction, VFX/audio, package performance, or final art.

## ART-007 generation and CAPTURE-007 evidence record

- Exact accepted source: version 0.72.0 commit `943d281b0a1506795dee7bab1ebcb1f9774c27a0`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `4acf3edc2e57b1908a4529149ca6393a4a0e48b662777691f45bc780698c58dc`.
- Output: revised `SM_World_GlassScarBuriedCauseway`, `M_GlassScarBuriedCauseway`, and `MI_GlassScarBuriedCauseway_Stone`, `MI_GlassScarBuriedCauseway_Recess`, `MI_GlassScarBuriedCauseway_Ceramic`, and `MI_GlassScarBuriedCauseway_Conduit`.
- The route mesh contains a recessed foundation, paired shoulders, seven ceramic deck bays, dark coffers, repeated ribs, inset conduits, broken parapets, and buried approach slabs. LOD0/LOD1 contain 1,200/600 triangles. Both LODs have UV0 surface mapping and UV1 lightmap data. The mesh contains four material zones and one simple box collision primitive. Runtime collision, overlap generation, and navigation influence are disabled before presentation; authoritative route/pathing state is unchanged.
- The material family uses an original project-authored UV/noise graph and distinct stone, recess, ceramic, and emissive-conduit physical parameters. It uses no imported scan, stock texture, Marketplace asset, generated image, or third-party source material. Revision `buried-causeway-production-v1` is recorded on the mesh, master, and instances. Immediate exact-source regeneration retained identical tracked bytes and a clean checkout.
- SHA-256 values are mesh `ad8c82bc034fd08aa939e5300e3f9cf2a5b517e597cfb33b9667f116a3eabfe8`; master material `3da4e20803fef335d5bc8cbbc6899f84f44e451c8e12fbe8e274b5c86e1bebbd`; Stone `3e827c84e88c9ca9a9ead26edce552fa4d00e9087758ae7d7417e3f724cd3619`; Recess `7b8afe2e789593b710db69239ede02981706f48e823a2c4eb3c67d0fc1d22a47`; Ceramic `35c69783b8d3486b8225c2ef7158f6fc81fc379024f6e6327ea65e8d98c49834`; and Conduit `8bbacef48b66d5ce144aff66c4c1748dcdfa102b2bd3da5c2857c40d11416a3e`.
- The exact-commit 1,600 × 900 Metal review used the widened 2,850-unit Buried Causeway camera, showed the full route without clipping, and exited normally. Log/frame SHA-256 values are `4c2fc6e40fab2c897d1f25763216fbbc54162a94fe1d10f3d71bb800c959fe0c` and `8214281cf70ae50b5475ba7e54dcba8b54af6e1f6a835ae2bf58ade22d54dd8e`. The earlier 2,300-unit frame was rejected because the route crossed the lower boundary. No queried project warning/error, fatal, assertion, or ensure marker was present.
- These records accept project ownership/provenance, exact asset identity, LOD/UV/material/collision structure, runtime isolation, and one isolated composition. They do not establish production textures, final surface response, ordinary navigation, broad combat readability, package performance, or final environment art.

## ART-008 generation and CAPTURE-008 evidence record

- Exact accepted source: version 0.73.0 commit `648d7c53337eab818e2b83cbf103bfc10c5db2fa`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `4fcfaf223b4fcfeb65473ac95c05b85084f8960779888529ea5b6d1320e6f064`.
- Output: revised `SM_World_GlassScarFoldedVerge`, `M_GlassScarFoldedVerge`, and `MI_GlassScarFoldedVerge_Obsidian`, `MI_GlassScarFoldedVerge_Rift`, `MI_GlassScarFoldedVerge_Ceramic`, and `MI_GlassScarFoldedVerge_Phase`.
- The route contains seven alternating displaced plates, dark hinge spans, bright phase seams, edge brackets, and asymmetric verge pylons. LOD0/LOD1 contain 1,236/618 triangles. Both LODs have UV0 surface mapping and UV1 lightmap data. The mesh contains four material zones and one simple collision primitive. Runtime collision, overlap generation, navigation influence, and route authority remain disabled.
- The material family uses an original project-authored UV/noise graph with distinct obsidian, rift, ceramic, and emissive-phase parameters. It uses no imported scan, stock texture, Marketplace asset, generated image, or third-party source material. Revision `folded-verge-production-v1` is recorded on the mesh, master, and instances. Immediate exact-source regeneration retained identical tracked bytes and a clean checkout.
- SHA-256 values are mesh `e647045efa198be3c9ed0bf1415713b78555c15ab66fcb20ba2abe59fc28a797`; master material `9cb364247935ebf78f2232eb88aa1f379e76941b4c63ac4fab254f06ff1c8017`; Obsidian `59f8bbfff5547f80b8cab4273f2f6e901f9700d12d01568b5f71dce2177050a8`; Rift `0229c097e7a0fb7056b32c5f377e48651d78a2394e7fa8ffa7db9b095c4603c0`; Ceramic `8bd0e4dfbacd018d4a28efef061eb479dca53cf2ffbf02709c7405a1900680bf`; and Phase `25658becb55ab5909f841dfff9bd6c3a1c154727255b4bbad4e61207dc52fff0`.
- The exact-commit 1,600 × 900 Metal review used the widened 3,350-unit Folded Verge camera, showed all seven plates and surrounding context without clipping, and exited normally. Log/frame SHA-256 values are `27d1536f345639adfe3d7dbfb0741b9c462ff9252a6d74d3a9306377546cee99` and `35ef1e8d5bb4c0f4f42e3ce35dc4233982fbea2a7ae993d8fec1458ba9a04798`. The earlier 2,300- and 2,850-unit frames were rejected because the lowest plate crossed the lower boundary. No queried project warning/error, fatal, assertion, or ensure marker was present.
- These records accept project ownership/provenance, exact asset identity, LOD/UV/material/collision structure, runtime isolation, and one isolated composition. They do not establish production textures, final surface response, ordinary navigation, broad combat readability, package performance, or final environment art.

## ART-005 generation and CAPTURE-005 evidence record

- Exact accepted source: version 0.69.0 commit `21957310e62ffdf4407b3f63846d681496919973`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `af5aced9242d74685cee66e754325c4b3704d07c41c155e3495955de7d1547f3`.
- Output: `SM_VFX_SelectionHalo`, `SM_VFX_CommandMove`, `SM_VFX_CommandAttack`, `SM_VFX_CommandAttackMove`, `SM_VFX_CommandPatrol`, `SM_VFX_CommandGuard`, `SM_VFX_CommandBuild`, `SM_VFX_CommandInteract`, and `SM_VFX_CommandOrbit` under `Content/Art/Generated/VFX`, plus `M_EchoesPresentationVFX` under `Content/Art/Generated/Materials`. Revision `selection-command-vfx-v2` separates direct Attack from Attack-move.
- Every mesh contains two LODs and no simple collision. LOD0/LOD1 triangle counts are Selection Halo 360/180; Move 300/150; direct Attack 516/258; Attack-move 320/160; Patrol 336/168; Guard 324/162; Build 336/168; Interact 708/354; and Orbit 68/64. Runtime components additionally disable collision, overlaps, navigation influence, decals, and shadows as appropriate. The actors and material parameters are presentation only; command acceptance and simulation authority remain outside this asset family.
- SHA-256 values are material `63183b522124289421d3aa79cedf8eb37953bd28c45a9c46e5af774505e4df6c`; Selection Halo `4076b529710480881d86185fc4cca85718e669ab7be7b7ac679c82d26239d175`; Move `3066ad151035d73a96ed2f306776fdaba1f4f9d791df1a3be478b812f9a7111e`; direct Attack `732b906672abb4cb50748de3555756c317db9d6932d16e4e5a56c41ab4636029`; Attack-move `3d710e646ed5b2b7577713e843e59e9fb11c291e3a6478b6f7d493dc8f453f2d`; Patrol `be214d423e20d861f29aeafb366606894b12bfd3545031c8d365ef5bc51e6270`; Guard `17ec556baae7289100b1011d1bf45b1863b18c9be88a1b551a92e75318ece23d`; Build `0cba54fb8ff5bdf056f27a57cb70bb372e8f8e809b2dffec6ae5cfe6c14b9e1b`; Interact `9aa69ecf894c5ad05c668e1d40a06eb2f71aa60fde0dbeb996768148264d345f`; and Orbit `61214c138c059c9cf6cf62c61103598eed707c9a61ab9244c3cb43ea505172cf`.
- Exact 0.74.0 regeneration reused all 39 tracked mesh assets without a repository diff. The generator reported nine VFX assets, seven command sigils, two LODs, no simple collision, presentation authority, steady reduced-motion behavior, and steady lower emission under reduced flashing. Generation-log SHA-256 is `42e6a3bbe8cee03a00aff7286702702fb1ba16ae962fadc8deee6f338e0f987a`.

## CAPTURE-009 evidence record

- Exact game-source commit: `006bfc3e62c44214e42348a5d91571153699f9e2`.
- The controlled 1,600 × 900 Metal run projected live entity views to screen coordinates and invoked the ordinary controller handlers. It selected Bulwark 3 at `(498,620)`, accepted pointer Guard on Surveyor 2 at `(404,407)`, observed authoritative `Guard` state, accepted a pointer direct Attack on visible Riftstalker 4 at `(800,111)`, and observed authoritative HP change from 125 to 115.
- Pointer log/frame SHA-256 values are `05f040584b93fc153ecba0fb822673f0fd6513e9d721301377f3e237ebcd1b8e` and `27c45b8462797d0b434e893d7043c5b6d174effe10a9d2252ea928238e89590a`. The frame was visually inspected without observed clipping; no queried project warning/error, fatal, assertion, or ensure marker was present.
- The harness moved Unreal's internal cursor and called normal controller methods; it did not inject operating-system events. The evidence does not qualify unaided-human use, adverse camera/UI-scale cases, balance, broad combat readability, package behavior, or final effects.
- The non-shipping review fixture rendered four selected Meridian candidates and all six command identities in separate standard and reduced-accessibility Metal runs at 1,600 × 900. Standard/reduced PNG SHA-256 values are `bcee86afc155c84227f5a08c7a39ed105491efddb59e32a595c8525ef6aced17` and `38c99ce3d984883beccf1f090b755c694fc6a8b2c6cede5cdab289421277d2b7`; corresponding log SHA-256 values are `00a56579b0e2b94179cddda1cea29127195dc921f5f1c1401d615be3b6e325ee` and `15edfa859abdb541c399150d979d63417161b651d8c66bbae39ce15315b355fa`. The initial reduced-run black startup frame was rejected; only the later stable rendered scene was accepted. Both runs exited normally with no queried project warning/error, fatal, assertion, or ensure marker.
- These records accept project ownership/provenance, exact asset identity, structural LOD/collision properties, one isolated standard composition, and one isolated reduced-accessibility composition. They do not establish Niagara/particle quality, transparency, audio confirmation, broad combat/camera readability, every accessibility combination, packaged behavior, performance, or final effects.

## ART-006 generation and CAPTURE-006 evidence record

- Exact accepted source: version 0.70.0 commit `848cd3f0c2a1d9200a8224a12b6cef3fe9f49c4d`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `4a9644fb594847b559615b354a3e908387ef35f53fc15a2ca2dff8253f4d3621`.
- Output: `SM_VFX_DestructionRing`, `SM_VFX_DestructionCore`, and `SM_VFX_DestructionShard` under `Content/Art/Generated/VFX`. The family reuses the project-owned `M_EchoesPresentationVFX` material registered with ART-005; no third-party source asset was added.
- Every mesh contains two LODs and no simple collision. LOD0/LOD1 triangle counts are Ring 312/156, Core 84/64, and Shard 36/36. Runtime components additionally disable collision, overlaps, navigation influence, decals, and shadows. The actor and material parameters are presentation only; authoritative damage, removal, visibility, save, replay, and checksum state remain outside this asset family.
- SHA-256 values are Ring `3f92dedaebdccc5fe70e2e6984ec7c082976fe0e0f86aee15fc033c9981363a7`; Core `4a4b9145e74a9ace4ae758cc941f67c3b0b1888dbd5b66e3b97c9869e17bfbbe`; and Shard `59ff0a6127d3e8452fe7a83cf881f1d1fa103770fe0f1c04da3ea8536da84a95`. A repeated exact-source generation was byte-idempotent and retained a clean tracked state.
- The non-shipping review fixture rendered Meridian and Kharuun Soldier, Heavy, and Core presentations in separate standard and reduced-accessibility Metal runs at 1,600 × 900. Standard/reduced PNG SHA-256 values are `37323803c430397ddcb13afef0f1429b07c0368529f325f6e55f287bd44085ff` and `90d1f13ff4caa5c08837fd963dffdc9709242c64c6a8968950d0d3329786eb0a`; corresponding log SHA-256 values are `af7f5ebee9cce830479dc0c2bae52496245a9bc26fcd3787eb2df49397bad280` and `dba40ec507052f912254168c8bde5ff7ebe2c5ba447e36c86c3743a70facb0a2`. An earlier clipped preflight composition, black startup frames, and a live frame captured after the review actors expired were rejected. Only the recentered saved compositions were accepted. Both exact runs exited normally with no queried project warning/error, fatal, assertion, or ensure marker.
- These records accept project ownership/provenance, exact asset identity, structural LOD/collision properties, one isolated standard composition, and one isolated reduced-accessibility composition. They do not establish transparent dissolve, Niagara debris/smoke, audio confirmation, broad simultaneous-combat/camera readability, every accessibility combination, packaged behavior, performance, or final effects.

## ART-013 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`.
- Output: `SM_World_BrokenSunSky` under `Content/Art/Generated/World/Environment`.
- The Broken Sun celestial sky object represents the shattered star Soryn at the heart of the setting. It features a central incandescent stellar core, deep basalt mantle crust blocks, orbiting Dawnshards, and outer coronal plasma arc rings.
- The asset has two LODs (LOD0: 1,597 vertices / 1,784 triangles; LOD1: 982 vertices / 892 triangles), four material zones (zone 0: deep space indigo; zone 1: fractured basalt crust; zone 2: warm amber coronal mantle; zone 3: radiant golden core), and visibility-selection collision with no runtime collision or navigation impact.
- Asset SHA-256 is `9821aeeaa951bd4bcf7f8104b458498db3d8c2c4c37f4cfd5ac486dea48c12f8`.

## ART-017 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`; generation log `Saved/Logs/ArtAssetGeneration.log` dated 2026-09-04 reporting `[ECHOES_ART_COMPLETE] generated=48` with only the shelf rebuilt and the other 47 assets reused.
- Output: revised `SM_World_GlassScarShelf` under `Content/Art/Generated/World/Environment` at revision `glass-scar-shelf-vitrified-v2`, stamped in `Echoes.AssetRevision`. The generator now carries an environment-revision map alongside the route-kit map so a stale shelf is deleted and rebuilt rather than reused unconditionally.
- Why: the v1 shelf's edge spires and strata slabs stood proud of the walking plate, so tiled instances read as a mosaic of separate slabs at the gameplay camera (directive gate 50). The v2 shelf is a flush vitrified charcoal plate with hairline fracture relief and amber surface fissures, strata bands on all four faces, face fissures, and buttresses on the lower faces, so the silhouette work lives on the cliff and the ground reads as one surface.
- Two LODs (LOD0 1,008 vertices / 504 triangles; LOD1 528 vertices / 252 triangles), four material zones on the shared world-surface material, visibility-selection collision only; runtime collision, navigation, and route authority unchanged. Generation uses only project-authored primitive composition; no imported geometry.
- Asset SHA-256 `9215d2225372f4443df65b54b23cd1f4ba47f75f584fc41d725f9823dde09a67`. Supersedes the ART-003 shelf identity (`33a269540156f3e83e98235da21813791597a12ac4fda138c3c8dcddcd7639d1`, 396/198 triangles), which is retained in historical evidence. Gate 3 themed captures accepted 2026-09-01 were rendered with the v1 shelf; the live Glass Scar ground therefore changes appearance and gate 3 evidence is not carried for the shelf until re-captured.
- Evidence: authoring-preview fixture capture `BuildArtifacts/Evidence/VerticalSliceReview-shelf-v2-20260904.png` (editor build, 1920×1080, local). Owner review still owed.

## ART-018 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`; generation log `Saved/Logs/ArtAssetGeneration.log` dated 2026-09-04 reporting `[ECHOES_ART_COMPLETE] generated=48` with only the sky object rebuilt.
- Output: revised `SM_World_BrokenSunSky` under `Content/Art/Generated/World/Environment` at revision `broken-sun-sky-fractured-v3` (stamped in `Echoes.AssetRevision`; `BrokenSunSky` joins the generator's environment-revision map). Revision v2 of the same day was an intermediate with oversized crust plates and is superseded without a retained asset.
- Why: the ART-013 sky object was a flattened disc with coronal rings and read as a pale plate with a Saturn ring in the composed frame (directive gate 50). CONCEPT-004 reads the Broken Sun as a cracked sphere lit from within with shards drifting away and no ring. v3 is a molten core sphere wrapped in 76 tangent crust plates on a Fibonacci sphere (thin, with gaps, every third carrying a hot inner rim), 16 Dawnshards drifting in three dimensions with incandescent inner spikes, and 18 far embers; the rings are removed.
- Two LODs (LOD0 4,130 vertices / 2,964 triangles; LOD1 2,762 vertices / 1,482 triangles), four material zones on the shared world-surface material, visibility-selection collision only; no runtime collision, navigation, or gameplay meaning. Generation uses only project-authored primitive composition; no imported geometry.
- Runtime binding: the sky object's core emissive is held at 2.4 (was 4.2) with the core hue (1.0, 0.56, 0.11) so the sphere reads gold through its cracks under the A1 rig instead of clipping to white; the review fixture places it 16 km out on the review axis at scale 0.85 so the whole sphere sits in the sky band above the far bank.
- Asset SHA-256 `60d4327b0faa3f946b199265811c3a59ef59ced3967557780085cea35132a0dc`. Supersedes the ART-013 identity (`9821aeeaa951bd4bcf7f8104b458498db3d8c2c4c37f4cfd5ac486dea48c12f8`, 1,784/892 triangles), retained in historical evidence.
- Evidence: authoring-preview fixture capture `BuildArtifacts/Evidence/release-gate50-composed-frame-20260904/fixture-sun-v3-1920x1080.png` and its concept pairing sheet (editor build, local). Owner review still owed.

## ART-019 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`; generation log `Saved/Logs/ArtAssetGeneration.log` dated 2026-09-04 (`[ECHOES_ART_COMPLETE] generated=48`).
- Output: revised `SM_World_GlassScarAshCut` at revision `ash-cut-production-v2` with its four material instances re-stamped at the same revision (`MI_GlassScarAshCut_Basalt`, `_Ash`, `_Glass`, `_Vein`); the master `M_GlassScarAshCut` is byte-identical to ART-004 and was reused.
- Why: with the scar band drawn as an open drop (ledger WORLD-A4-003), the Ash Cut trench floated over the chasm. v2 adds four basalt piers under the bed across the band (|y| < 500), each with flanking buttresses and, at LOD0, an amber fissure slab, reaching the chasm floor about 700 below. The trench bed, banks, slab divisions, and UV policy are unchanged.
- Two LODs (LOD0 2,778 vertices / 1,648 triangles; LOD1 1,686 vertices / 824 triangles), two UV channels per LOD, four material zones, authored simple collision with runtime collision disabled. Mesh SHA-256 `84fde2440415f09e18c1925542cf467906094c63f92f1c9f00d9567b0f335468`; instances `1b6e3183…` (Ash), `ee4ea7d6…` (Basalt), `fd316d25…` (Glass), `30b4a72e…` (Vein). Supersedes the ART-004 mesh identity, retained in historical evidence.
- Evidence: live-play capture `BuildArtifacts/Evidence/release-gate50-composed-frame-20260904/live-ashcut-piers-1920x1080.png` (scouted, gameplay camera, editor build, local). Owner review still owed.

## ART-020 generation record

- Generator and log as ART-019.
- Output: revised `SM_World_GlassScarFoldedVerge` at revision `folded-verge-production-v2` with its four material instances re-stamped (`MI_GlassScarFoldedVerge_Obsidian`, `_Rift`, `_Ceramic`, `_Phase`); the master `M_GlassScarFoldedVerge` is byte-identical to ART-007 and was reused.
- Why: as ART-019. Every displaced plate that stands over the scar band gets its own offset basalt pier to the chasm floor, following the plate's yaw and height, with an amber fissure slab at LOD0.
- Two LODs (LOD0 2,356 vertices / 1,416 triangles; LOD1 1,470 vertices / 708 triangles), two UV channels per LOD, four material zones, runtime collision disabled. Mesh SHA-256 `80962927c61a878747293359f906ea830e593b0a390509cdd411b68c0636cc0e`; instances `1938197b…` (Ceramic), `176c5952…` (Obsidian), `22d25c60…` (Phase), `d8665097…` (Rift). Supersedes the ART-007 mesh identity, retained in historical evidence.
- Evidence: fixture capture `fixture-piers-1920x1080.png` in the same evidence set; a Folded Verge live capture is still owed.

## ART-021 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh` (asset count raised 48→49, environment 8→9); generation log `Saved/Logs/ArtAssetGeneration.log` dated 2026-09-04.
- Output: new `SM_World_SkyDome` under `Content/Art/Generated/World/Environment` at revision `sky-dome-banded-v1` (in the generator's environment-revision map).
- Why: the sky behind the Glass Scar frame was the flat height-fog colour (directive gate 50); CONCEPT-004 reads a graded indigo sky with a warm haze under the Broken Sun. Rather than the engine atmosphere component, which adds a render pass on the M1 Pro budget, the sky is one inward-facing shell: four stacked open cone bands (horizon, low, deep, zenith) built with the primitive flip-orientation option, radius 42,000 at the horizon to a near-point zenith at 40,000 up, one material zone per band so the runtime grades it.
- Two LODs (LOD0 588 vertices / 768 triangles; LOD1 392 vertices / 384 triangles), four material zones on the shared world-surface material, no collision, navigation, or gameplay meaning. Runtime binding: self-lit band colours (0.11, 0.10, 0.27) → (0.012, 0.012, 0.055) at emissive 0.55 → 0.30, metallic 0, roughness 1, spawned at (0, 0, −1,500) on the Glass Scar preset; the first pass at emissive 0.9 pushed the fixture's mean luma to 72 and was reduced to stay inside the A1 window.
- Asset SHA-256 `288a2a8ad32d0671cb030f44039313e2fcdf32f63397c61bb93271da01fcf1e5`.
- Evidence: fixture and scouted live captures `fixture-skydome-*` / `live-well-skydome-*` under `BuildArtifacts/Evidence/release-gate50-composed-frame-20260904/` (editor build, local), exposure measured. Owner review still owed.

## FONT-001 typeface embedding record

- Decision: directive open decision 2, resolved by Angelis 2026-09-01 and executed 2026-09-04 under the standing lead-director mandate: **Space Grotesk** for interface chrome, **IBM Plex Mono** for tactical readouts.
- Source: the Google Fonts repository (`github.com/google/fonts`, `ofl/spacegrotesk` and `ofl/ibmplexmono`), fetched 2026-09-04 as shipped there: `SpaceGrotesk[wght].ttf` (variable weight), `IBMPlexMono-Regular/Medium/SemiBold/Bold.ttf`. Vendored under `Content/UI/Fonts/<family>/` with each family's `OFL.txt` beside the files.
- Licence: SIL Open Font License 1.1 for both (Space Grotesk © 2020 The Space Grotesk Project Authors; IBM Plex © 2017 IBM Corp., Reserved Font Name "Plex"). Free for commercial embedding in the game and the site; the OFL text ships with the files; no per-seat cost; attribution carried here.
- Method: no editor Font asset. `EchoesTypeface` (`Source/EchoesOfTheBrokenSun/Public/EchoesTypeface.h`) builds runtime-cached `UFont` objects from the vendored files at first use (chrome 10, chrome-large 14, readout 10 legacy sizes), logs `[ECHOES_TYPEFACE_READY]` per face, and falls back to the engine font with `[ECHOES_TYPEFACE_FALLBACK]` if a file is missing so text never disappears. `AEchoesHUD` draws all chrome through `Chrome()`/`ChromeLarge()` and the resource ledger through `Readout()`. `Config/DefaultGame.ini` stages `UI/Fonts` as UFS so the files ship in a package.
- Boundary: the site (`website/`) does not yet embed the faces; the Slate menus that use engine styles are not restyled by this record; A8's full interface art system remains open.

## AUDIO-001 generation and playback evidence record

- Exact accepted source: version 0.71.0 commit `5368aec5d86a6bf5566c3445890323432f8cba1f`; generator and audit wrapper: `Scripts/generate_audio_assets.py` and `Scripts/generate_audio_assets.sh`; exact generation-log SHA-256 `08b29f29cd005084c5ccb1d5c2d25fec760a4180258e14920bf51cacafd669b0`.
- Output: mono 16-bit 48 kHz `SFX_CommandConfirm.wav` (0.14 s), `SFX_DestructionMeridian.wav` (0.46 s), and `SFX_DestructionKharuun.wav` (0.52 s), plus matching imported SoundWave assets. The generator uses only authored mathematical oscillators, envelopes, and deterministic grit; it imports no recording, sample library, generated-model output, Marketplace asset, or other third-party audio.
- Source SHA-256 values are Command `f93b3da441e32314dd6659574c7baf7c0d9afb9ccfa0e9a98d03eb515a46f56f`; Meridian `a40381e576dcbbe5aec295407eef041fd0940d74a9d5f84accfd1b5c1e8aa31c`; and Kharuun `5857e8576befe3281a5ffebb06446128efba74ea2369770401ddb6f65d6c5bdd`. Imported SoundWave SHA-256 values are Command `054421bc25ba08a7ef220c02b5d2df2b32472ccedaab4cd861ec4bdf8a73f78f`; Meridian `8b6ccc8ad9bab38128cf3d36023d3b2128489593f32809c8a3862e19cf304108`; and Kharuun `3779c79835419e4f52f748a5e13a08d8744920098ebf8d532c5fc095feb56b95`. Repeated generation reused byte-identical sources and SoundWave assets and retained a clean tracked state.
- Integer PCM inspection recorded peak/RMS full-scale ratios of `0.374603/0.154609` for Command, `0.508728/0.181148` for Meridian, and `0.467834/0.155841` for Kharuun; absolute DC offset remained below `0.0015` full scale. These are structural signal measurements, not loudness, perceptual quality, hearing-safety, or mastering evidence.
- Exact standard and reduced-dynamic-range Metal reviews each loaded and played all three cues. Standard command/destruction multipliers were `0.56/0.96`; reduced values were `0.68/0.74`. Both reviews exposed effects-volume and reduced-dynamic-range labels, used the 80 ms command and 140 ms destruction rate limits, and exited normally. Standard/reduced log SHA-256 values are `524ff5433d2e3af796b6c38b9b7dbc6d27e80f53ecb939b61df75ce9869efa58` and `cbe16a799169795971501824dafdfb89d0b935d56859737e458b789f8230c654`. The first reduced visual inspection caught a transient clipped redraw and was rejected; the stable redraw was accepted. No queried project/audio warning/error, fatal, assertion, or ensure marker was present.
- These records accept provenance, exact source and asset identity, file structure, runtime loading, 2D/spatial routing, volume/mute and reduced-dynamic-range behavior, bounded rate limits, and two controlled live playback branches. They do not establish subjective listening quality, a final mix/master, music, ambience, voice, complete alerts, broad simultaneous-combat mixing, hearing-safety evaluation, package behavior, performance, or final audio.

## AUDIO-002 through AUDIO-005 generation record

- Generator: `Scripts/echoes_audio_synth.py` (pure Python, editor-independent); import and audit:
  `Scripts/generate_audio_assets.py` through `Scripts/generate_audio_assets.sh` in Unreal Engine 5.8.2.
- 2026-09-01: all 54 registered cues (4 `presentation-audio-v1`, 12 `interface-audio-v1`,
  18 `gameplay-audio-v1`, 15 `music-v1`, 5 `ambience-v1`) synthesized to
  `Content/Audio/Source` as 48 kHz 16-bit PCM WAV, mono for one-shots and stereo for music and
  ambience, every cue under the 0.96 true-peak ceiling. A second synthesis run reused all 54
  sources byte-identically (`action=reused` 54/54), proving byte-idempotence under the recorded
  revisions. Aggregate manifest SHA-256 over the per-cue sha256 lines:
  `10368c73bee52a344af5bd30103c01818c4fdc56c9d6e5bbd3b378971df1f560`; per-cue digests, byte
  sizes, and peak/RMS are in the retained generation log
  (`WorkstreamControl/evidence/demo-audio-families-*/audio-sources.log`).
- The same date's editor import produced the 54 SoundWave assets under `/Game/Audio/Generated`
  with creator, provenance, role, category, revision, and looping metadata; the wrapper audit
  (`cues=54 revisions=5 … sourcesOriginal=true thirdPartySamples=false`) passed, and an
  immediate re-run reused every asset without modification.
- No recording, sample library, generated-model output, Marketplace asset, or other third-party
  audio is present in any of these cues. Structural peak/RMS measurements are not loudness,
  perceptual-quality, hearing-safety, or mastering evidence. These families do not establish
  runtime event coverage, rendered playback, the qualified mix, packaged behavior, or final audio.

## AUDIO-006 generation record

- Generator: `Scripts/echoes_audio_synth.py` revision `music-v2`; import and audit through
  `Scripts/generate_audio_assets.sh` in Unreal Engine 5.8.2.
- 2026-09-01: fourteen cues added — `MUS_Tension{MM,MK,MC,KK,KC,CC}` and
  `MUS_Combat{MM,MK,MC,KK,KC,CC}` (24 s looping stereo; each blends its two factions'
  established materials without letting either dominate, mirrors intensifying one language),
  plus `MUS_BriefUnderscore` (36 s) and `MUS_ResultsUnderscore` (32 s). A second synthesis run
  reused all 68 sources byte-identically, and the editor import audit passed
  (`cues=68 revisions=6 … music=29`). Per-cue BS.1770-4 loudness and inter-sample peak are in
  the retained measurement log under the evidence directory.
- These cues do not establish rendered playback, the demonstrated in-session combat crossfade
  capture, loudness normalization to the −16 LUFS target, or final audio.

## CONCEPT-001 exact prompt record

The following are the complete text prompts supplied to the image-generation service. No reference image was supplied.

### Meridian units

```text
Create a polished 2x2 concept-art presentation sheet for four original real-time-strategy game units from the science-fantasy world "Echoes of the Broken Sun." Faction: Meridian Compact. Overall shape language: precise orthogonal engineering, modular repairable armor, exposed load paths and redundant conduits, graphite and warm ivory ceramic panels, cyan luminous status bands, restrained brass connectors, prismatic optics. Stylized realism, premium AAA strategy-game design, readable at a distant three-quarter isometric RTS camera, compact silhouettes, no existing franchise resemblance.

Quadrants with generous dark neutral separation and exactly one unit per quadrant:
top-left: Surveyor, a compact two-legged engineering exoframe with twin manipulator arms, fold-out sensor mast, rear matter canisters, practical and agile.
top-right: Lancer, a tall narrow armored line operator with a long crystalline precision rifle, stable firing stance, disciplined silhouette.
bottom-left: Bulwark Team, a broad heavy two-operator platform integrated with a segmented directional energy-shield projector, visibly slow and protective.
bottom-right: Relay Skiff, a fast fragile triangular hovering reconnaissance craft with an antenna halo, archive capsule, and light modular fins.

Show each as a clean full-body three-quarter hero render with subtle floor shadow, same scale convention, realistic physically based materials, deliberate production design, and no scenery. No title, no labels, no text, no logos, no border, no UI, no watermark.
```

### Kharuun units

```text
Create a polished 2x2 concept-art presentation sheet for four original real-time-strategy game units from the science-fantasy world "Echoes of the Broken Sun." Faction: Kharuun Assemblies. These are cultivated mineral-organic warforms built by a sophisticated mobile civilization—not primitive, feral, tribal, demonic, insectoid, or fantasy monsters. Shape language: layered basalt and ivory stone-ceramic growth, obsidian facets, warm amber resonant seams, limited violet crystal accents, intentional tuned symmetry mixed with grown asymmetry. Stylized realism, premium AAA strategy-game design, readable at a distant three-quarter isometric RTS camera, original silhouettes, no existing franchise resemblance.

Quadrants with generous dark neutral separation and exactly one unit per quadrant:
top-left: Tender, a compact four-limbed civic cultivator with careful tool tendrils, seed-crystal baskets, and a gentle repair posture.
top-right: Riftstalker, a lean swift quadruped skirmisher with a long resonant ranged organ, reverse-jointed stone limbs, and a mobile flanking silhouette.
bottom-left: Cairnback, a massive low six-limbed assault screen with layered stone carapace and a broad shield-like back, protective rather than monstrous.
bottom-right: Resonant, a slender tripod reconnaissance form with arched tuning fins and three delicate ground-contact vibration feelers.

Show each as a clean full-body three-quarter hero render with subtle floor shadow, same scale convention, realistic physically based materials, deliberate production design, and no scenery. No title, no labels, no text, no logos, no border, no UI, no watermark.
```

### Meridian structures

```text
Create a polished 2x2 concept-art presentation sheet for four original real-time-strategy game buildings from the science-fantasy world "Echoes of the Broken Sun." Faction: Meridian Compact. Overall design language: precise orthogonal engineering, modular repairable architecture, visible load-bearing frames, redundant exposed conduits, graphite and warm ivory ceramic panels, cyan luminous status bands, restrained brass connectors, prismatic optics. Stylized realism, premium AAA strategy-game production design, readable from a distant three-quarter isometric RTS camera, broad distinct footprints, no existing franchise resemblance.

Quadrants with generous dark neutral separation and exactly one complete building per quadrant:
top-left: Anchor, a broad hexagonal command headquarters and resource drop-off, central cyan beacon, radial conduit arms, civic and defensible.
top-right: Power Link, a compact supply and network pylon with layered cyan rings, exposed redundant cables, unmistakably connective.
bottom-left: Array Foundry, an elongated unit-production and research hall with visible assembly gantries, a luminous production spine, and modular hardpoints.
bottom-right: Aegis Post, a compact network-powered defensive turret with a telescoping prismatic firing array and grounded power tether.

Show each as a complete three-quarter isometric architectural render on a subtle floor with contact shadow, consistent scale convention, realistic physically based materials, deliberate silhouette, and no landscape. No people, no title, no labels, no text, no logos, no border, no UI, no watermark.
```

### Kharuun structures

```text
Create a polished 2x2 concept-art presentation sheet for four original real-time-strategy game buildings from the science-fantasy world "Echoes of the Broken Sun." Faction: Kharuun Assemblies. These are cultivated mineral-organic civic and military structures made by a sophisticated migratory civilization—not primitive huts, tribal fantasy, ruins, demonic architecture, insect nests, or uncontrolled growth. Shape language: layered basalt and ivory stone-ceramic growth, obsidian facets, warm amber resonant seams, limited violet crystal accents, tuned geometry, orderly maintenance, grown asymmetry. Stylized realism, premium AAA strategy-game production design, readable from a distant three-quarter isometric RTS camera, distinct footprints, original.

Quadrants with generous dark neutral separation and exactly one complete building per quadrant:
top-left: Memory Hearth, a broad circular headquarters and resource hearth with a central resonant chamber and layered mineral petals, civic and welcoming but resilient.
top-right: Waystone, a tall mobile supply monolith with three folded root-legs, visible rooted/mobile transformation joints, and a glowing amber core.
bottom-left: Growth Basin, an open ring-shaped production nursery with a lowered central bowl, ordered cocoon alcoves, and visible controlled molting channels.
bottom-right: Listening Spine, a tall ground-embedded detection spire with branching tuned acoustic fins and precise vibration vanes.

Show each as a complete three-quarter isometric architectural render on a subtle floor with contact shadow, consistent scale convention, realistic physically based materials, deliberate silhouette, and no landscape. No people, no title, no labels, no text, no logos, no border, no UI, no watermark.
```

## CONCEPT-002 exact prompt record

The following is the complete text prompt supplied to the image-generation service. No reference image was supplied. The service returned the source file under the Codex generated-image workspace; the unchanged project copy is `site/assets/concepts/future-well-states.png`, 1,254 × 1,254 pixels, SHA-256 `f95912742d42bd754e282e1a0a13e81ce07ee8a58ce3ef025569d1b483f9ed90`. The public page prefers a quality-88 WebP delivery derivative whose SHA-256 is `f34de0e41aa703ea7a2c44508d68f5138bdbb5a30b9226395c83e96cceff5417` and retains the PNG as its fallback and authoritative generated output.

```text
Use case: stylized-concept
Asset type: four-state game hero-object concept sheet for Unreal Engine production
Primary request: Design the Future Well, the signature neutral world object in the original science-fantasy RTS "Echoes of the Broken Sun." A Future Well is a physical wound in reality where unrealized futures condense. It must feel ancient, deliberate, and unlike either playable faction: not Meridian machinery and not Kharuun mineral-organic architecture.
Scene/backdrop: four equal quadrants on a restrained charcoal studio ground, generous separation, exactly one complete Future Well per quadrant, no surrounding scenery except a small neutral ground plane that shows its effect
Subject: the same Future Well design in four unmistakable states. Core design: a broad radial foundation of dark vitrified stone, six broken load-bearing pylons, a central vertical negative-space aperture holding a faceted fragment of impossible sunlight, two incomplete counter-rotating orbital rings, suspended black-glass shards, and fine luminous channels spreading into the ground. Distant three-quarter isometric RTS readability, broad 5x5-building footprint, strong silhouette.
State 1 dormant: rings nearly still and aligned, aperture mostly dark, six pylons open like a careful crown, sparse pale-gold light.
State 2 Harvest: structure visibly collapsing inward, aperture pinched into a sharp downward siphon, rings broken and descending, ground channels drained toward the center, intense white-gold energy with restrained ember-red hazard accents.
State 3 Preserve: structure stable and fully open, rings balanced in a wide halo, central light vertically suspended, branching possibility filaments rising and gently extending across the ground, warm gold plus clear cyan.
State 4 Reshape: structure and ground visibly reconfigured, rings tilted into intersecting planes, central shard split into offset afterimages, lateral luminous paths bend the nearby ground into a temporary crossing, violet-gold plus cyan.
Style/medium: premium AAA strategy-game production concept, stylized realism, realistic PBR surfaces, practical buildable geometry, clean full-object render
Composition/framing: consistent camera, scale, and orientation in all four quadrants; full object visible; no cropping; readable from an RTS camera
Lighting/mood: dark neutral studio lighting with controlled volumetric glow; mysterious and consequential, not horror
Color palette: black glass, charcoal basalt, aged pale metal, warm broken-sun gold; state colors augment but do not replace distinct geometry
Materials/textures: vitrified stone, weathered pale alloy, fractured black glass, concentrated luminous energy
Constraints: same asset identity in every state; geometric state differences must remain legible in grayscale; no people; no vehicles; no text; no labels; no title; no logos; no watermark; no existing franchise resemblance
Avoid: generic circular portal, fantasy rune circle, stargate, magic fountain, church altar, demonic imagery, faction logos, ornate filigree, tiny unreadable detail
```

## CAPTURE-002 evidence record

- Capture fixture: non-shipping `-EchoesFutureWellArtReview` presentation mode, requested 1,600 × 900 output, local Unreal Engine 5.8.2 MacEditor, Metal SM5.
- Public files and SHA-256 values: Dormant `48af0c4181e096814757f54137d1bd3edc319b7e691699e20d2d596f9d6898c8`; Harvest `6003aaf734b67852a406d444380aab44d300120579e093c0cb4f181fdbef1274`; Preserve `059fd8dd93ea545a91f1e26faf9cc12d93b137c4a1fa18b2734efbb08baaea28`; Reshape `6bd93ac7cbb9dae0dcd1646fbfc8300130c64d8fdbc2f32c88cb0280df259f18`.
- The public page prefers quality-84 WebP delivery derivatives and retains the PNG captures as fallbacks. WebP SHA-256 values are Dormant `1964b8758c23dc70a904cb2704e7d0a774fe58eb0e9244e04ed39fba377af5c1`, Harvest `7851fbe4165f8a274ea933f263cf9fcb00cd41bdffd0e88fdd8a0f569ff7345f`, Preserve `63cd4f5ec509dbfe4eedf9449b30fcd679333c1d7c7721cc8e767defa0ec0344`, and Reshape `2908294a6920009cd5ed8e2b8d2c596596e5a761efc9811569ba935de84044fd`.
- The fixture isolates the visual object and permits a presentation-only state override without mutating the authoritative simulation. These images demonstrate that all four runtime geometry states render in the current editor build. They do not demonstrate ordinary player acquisition, state-transition timing, gameplay readability under combat load, package behavior, final lighting, or production quality.

## CONCEPT-003 exact prompt record

The following is the complete text prompt supplied to the image-generation service. No reference image was supplied. The service returned the source file under the Codex generated-image workspace; the unchanged project copy is `site/assets/concepts/glass-scar-routes.png`, 1,254 × 1,254 pixels, SHA-256 `c8c06108f69e87412259f0015283c1ac50bfd2aead4138f17106734fd2f1e373`. The public page prefers a quality-88 WebP delivery derivative whose SHA-256 is `38685da5d4353450f4896b5a7e0cf0b0ac252dbff74fd1abf2a9807d888fba71` and retains the PNG as its fallback and authoritative generated output.

```text
Use case: stylized-concept
Asset type: four-view game environment concept sheet for Unreal Engine production
Primary request: Design the Glass Scar, the first battle map in the original science-fantasy real-time-strategy game "Echoes of the Broken Sun." The Glass Scar is a long impact basin cut through dark vitrified terrain after the star Soryn shattered. It must look like a tactically readable place with a history, not a generic arena: broken transit infrastructure, glassy cliffs, wind-combed ash, luminous Matter deposits, and a central neutral Future Well where unrealized futures condense.
Scene/backdrop: four equal quadrants with generous dark neutral separation and exactly one complete environment view per quadrant; no frame text. All views depict the same basin and material language.
Subject: quadrant one is the broad three-quarter isometric overview: two opposed starting shelves, a continuous fractured east-west scar between them, three clearly different north-south crossings, safe outer Matter deposits, a contested central Matter deposit, restrained Meridian orthogonal foundation dressing at one end, restrained Kharuun layered mineral-organic foundation dressing at the other, and the central Future Well as a broad dark radial foundation with six broken pylons, incomplete orbital rings, suspended black-glass shards, and impossible warm light.
Quadrant two is the Ash Cut crossing: a low raw cleft carved through black-glass ridge, scalloped scorched ground, exposed ash strata, leaning paired shard fins, and an unmistakable irregular passable trench silhouette.
Quadrant three is the Buried Causeway crossing: a broad straight recessed transit deck emerging from ash, heavy pale stone-ceramic slabs, repeated structural ribs and broken parapets, and an unmistakable continuous linear silhouette.
Quadrant four is the Folded Verge crossing: an impossible temporary zigzag made from offset vitrified plates folded out of adjacent terrain, split levels, displaced shard markers, and an unmistakable angular stepped silhouette.
Style/medium: premium strategy-game environment production concept, stylized realism, realistic PBR surfaces, practical buildable geometry, clean deliberate forms, original design, no existing-franchise resemblance
Composition/framing: consistent distant three-quarter isometric RTS camera and scale language; full route forms visible; navigable ground, blocked cliffs, resource deposits, faction-side landmarks, and the central objective must read immediately at combat distance
Lighting/mood: low broken-sun dusk, controlled amber and muted magenta light glowing from deep fractures, cool cyan-white Matter cores, thin windblown haze; solemn, strange, and beautiful rather than horror
Color palette: charcoal basalt, blue-black glass, gray ash, weathered pale transit ceramic, broken-sun amber, restrained magenta fracture light, cool cyan-white Matter
Materials/textures: fractured vitrified stone, matte ash, worn structural ceramic, sharp black glass, concentrated mineral light
Constraints: the three route identities must remain distinct in grayscale through geometry and silhouette; shape must carry gameplay meaning before color; no labels; no title; no text; no logos; no UI; no watermark; no people; no close-up characters; no generic fantasy ruins; no circular magic portal; no visual clutter that hides traversable ground
Avoid: checkerboard game tiles, placeholder cubes, featureless flat arena, neon cyberpunk city, lava level, demonic imagery, copied franchise motifs, tiny unreadable detail
```

## CONCEPT-004 provenance record

- File: `site/assets/concepts/target-render-vertical-slice.jpg`, 1,376 × 768 pixels, sRGB JPEG, SHA-256 `04ec9daa608ee6d5f15268b8fa7aa412d7eafe132dac5811681daa2ce105e62f`. First committed to `main` on 2026-09-04 (commit `04be980`); filesystem creation time 2026-09-04 06:22 local; no embedded creator, origin URL, or prompt metadata.
- Content: one three-quarter isometric composed scene of the Glass Scar — a Future Well on a stone causeway over a fractured chasm with golden fracture veins, a Meridian Compact force on the left, a Kharuun Assemblies force on the right, cyan Matter deposits, and the shattered Broken Sun above an indigo sky. It is a visual target for the finished composed frame, not an in-engine capture.
- Missing from the record: the exact generation service, account plan, date of generation, complete prompt or source inputs, output terms, and any human modification record. Until the owner supplies these, the image is a development concept reference only and must never be described as a screenshot, gameplay, or release media (REL-PUB-011).
- Public delivery derivative: quality-88 WebP `site/assets/concepts/target-render-vertical-slice.webp`, SHA-256 `2d5c7609dfe639e238aa2e07a3e5d98cdd6fa9ea95d07d7c6f263d38f0f9ebb1`; the JPEG remains the fallback and the retained source copy.

## CONCEPT-001 delivery derivatives

- On 2026-09-04 quality-88 WebP delivery derivatives were generated from the unchanged CONCEPT-001 PNG sheets for the public vision page; the PNG files remain the fallbacks and the authoritative generated outputs. WebP SHA-256 values are Meridian units `39884c8f2df4e963a44f23c52cc79008feb18881129526672cd76a3f653a64c9`; Meridian structures `ac8cf0d53376f677d7dff68a54ef009a631480d7cad3eab465bff39c6f37c6d2`; Kharuun units `f344cf4c8e7ab7531f655ecf37d4469f1c3e19e7a9e5697d3c68fab55c2780df`; Kharuun structures `5e30b92cbe20928d658a42f649eab326f66ac93697c66bebcb6af2b415ceb2d5`.

## CONCEPT-005 provenance record

- File: `site/hero-soryn.png`, 1,672 × 941 pixels, SHA-256 `327cbda80d68623b00d06a68b237a3343c2467751414ad7e7b2c8d51374dd2ae`. First committed on 2026-08-29 (`8dd0ef6`, “Place game site in Pages publish directory”); copies exist at `website/public/hero-soryn.png` and `website/public/archive-static/hero-soryn.png`. No embedded creator, origin URL, or prompt metadata.
- Content: a painted wide landscape — figures on a cliff edge looking over a fractured basin with a luminous central structure, a distant city, magenta crystal growths, and the shattered golden Broken Sun above. Used as a CSS background on the public site since 2026-08-29; on 2026-09-04 the home hero note states that this backdrop is concept art and not a screenshot.
- Missing from the record: the exact generation service, account plan, date of generation, complete prompt or source inputs, output terms, and any human modification record. Until the owner supplies these, the image is a development concept reference only and must never be described as a screenshot, gameplay, or release media (REL-PUB-011).

## CONCEPT-006 through CONCEPT-010 provenance records

- CONCEPT-006 (`site/assets/concepts/soryn-world-map.jpg` and `.webp`, 1,376 × 768): Soryn Continental Cartography and Regional Artery Map. Displays the planetary landmass, Broken Sun in the upper atmosphere, and arterial connections between Glass Scar, Lume Reach, Shivergrass Steppes, Subterranean Caverns, Sector 9 Foundries, and the Solar Fall Dais.
- CONCEPT-007 (`site/assets/concepts/shivergrass-basin.jpg` and `.webp`, 1,376 × 768): Mission 02 Shivergrass Basin visual target. Features rolling resonance prairies, rippling probability-sensitive vegetation, and grazing Vaultback megafauna under an amber Broken Sun.
- CONCEPT-008 (`site/assets/concepts/unburied-road-caverns.jpg` and `.webp`, 1,376 × 768): Mission 04 Unburied Road visual target. Features subterranean bioluminescent geode caverns, ancient transit spans, and glowing Matter crystals.
- CONCEPT-009 (`site/assets/concepts/arkcity-census-void.jpg` and `.webp`, 1,376 × 768): Mission 06 Ark-City Sector 9 visual target. Features sheared industrial foundations, brutalist monolithic silos, and the gaping census void below.
- CONCEPT-010 (`site/assets/concepts/broken-sun-solar-dais.jpg` and `.webp`, 1,376 × 768): Mission 15 Solar Fall Dais visual target. Features the geometric obsidian dais floating in the sub-solar void directly beneath the blinding golden coronary fracture of the Broken Sun.
- Authorship: Directed by Angelis Pseftis; created on 2026-09-04 as development concept targets for the Tactical Atlas and campaign world-building.

## CONCEPT-011 through CONCEPT-024 provenance records

- CONCEPT-011 (`site/assets/concepts/echoes-hud-modern-arc.jpg` and `.webp`, 1,376 × 768): Original Modern Floating Contextual Command Arc HUD. Demonstrates minimalist topographical contour radar, live Matter/Dawn/Logistics telemetry readouts, and a floating contextual command arc that opens the battlefield.
- CONCEPT-012 (`site/assets/concepts/echoes-hud-traditional-console.jpg` and `.webp`, 1,376 × 768): Traditional 3-Box Command Console Baseline. Demonstrates the legacy StarCraft II reference 3-box bottom console (Minimap, Unit Card, 3x4 Grid) used for comparative analysis.
- CONCEPT-013 (`site/assets/concepts/echoes-main-menu-bridge.jpg` and `.webp`, 1,376 × 768): Diegetic Ark-City Command Bridge Main Menu. Features Commander Mara Vey at an interactive holographic war-table on an observation deck overlooking the Broken Sun.
- CONCEPT-014 (`site/assets/concepts/echoes-main-menu-traditional.jpg` and `.webp`, 1,376 × 768): Traditional Left-Stacked Shell Menu Baseline. Demonstrates the legacy vertical button column inspired by Legacy of the Void.
- CONCEPT-015 (`site/assets/concepts/echoes-faction-selection.jpg` and `.webp`, 1,376 × 768): Tri-Faction Strategic War Room & Selection. Features holographic planetary projections and three faction pedestals with lore dossiers and unit showcases.
- CONCEPT-016 (`site/assets/concepts/echoes-future-well-landmark.jpg` and `.webp`, 1,376 × 768): Future Well Celestial Battle Landmark. Demonstrates the swirling Dawnshard anomaly, fortified perimeter, and environmental reactions.
- CONCEPT-017 (`site/assets/concepts/echoes-meridian-structures.jpg` and `.webp`, 1,376 × 768): Meridian Compact Industrial Structures Architecture Sheet (`SPEC-BLD-015`). Features Anchor HQ, Power Link Pylon, Array Foundry, and Aegis Post Turret.
- CONCEPT-018 (`site/assets/concepts/echoes-kharuun-structures.jpg` and `.webp`, 1,376 × 768): Kharuun Assemblies Living Geological Structures Architecture Sheet (`SPEC-BLD-016`). Features Memory Hearth HQ, Waystone Monolith, Growth Basin Terrace, and Listening Spine Needle.
- CONCEPT-019 (`site/assets/concepts/echoes-choir-structures.jpg` and `.webp`, 1,376 × 768): Hollow Choir Phase-Uncertain Structures Architecture Sheet (`SPEC-BLD-017`). Features Concordance Core HQ, Interval Loom Pylon, Chorus Loom Synthesis Basin, and Phase Anchor Turret.
- CONCEPT-020 (`site/assets/concepts/echoes-campaign-map.jpg` and `.webp`, 1,376 × 768): Planetary Campaign Operations Map & Mission Dossier (`SPEC-UI-002`). Features Soryn surface radar, route nodes, objective briefs, threat meters, and audio waveform.
- CONCEPT-021 (`site/assets/concepts/echoes-pause-field-ledger.jpg` and `.webp`, 1,376 × 768): Mara's Field Operations Ledger / In-Game Pause Menu (`SPEC-UI-007`). Features frosted charcoal glass overlay, live mission checklist, casualty telemetry, and quick actions.
- CONCEPT-022 (`site/assets/concepts/echoes-post-match-debriefing.jpg` and `.webp`, 1,376 × 768): Post-Match Combat Debriefing & Telemetry Curves (`SPEC-UI-008`). Features victory banner and interactive harvest/power telemetry curves.
- CONCEPT-023 (`site/assets/concepts/echoes-settings-accessibility.jpg` and `.webp`, 1,376 × 768): System Configuration, Loudness & Accessibility Terminal (`SPEC-UI-006`). Features high-contrast mode live preview, safety toggles, and -24 LKFS audio loudness calibration.
- CONCEPT-024 (`site/assets/concepts/echoes-combat-vfx-grammar.jpg` and `.webp`, 1,376 × 768): Tri-Faction Combat Visual Effects Grammar (`SPEC-VFX-001`). Features side-by-side comparison of Meridian cyan rail beams/shields, Kharuun magma blasts/basalt splinters, and Choir probability fractures/phantoms.
- Authorship: Directed by Angelis Pseftis; created on 2026-09-05 as development concept targets and visual design specifications.


## CAPTURE-003 evidence record

- Capture fixture: non-shipping `-EchoesGlassScarArtReview` presentation mode with `-EchoesGlassScarReview=Overview|AshCut|BuriedCauseway|FoldedVerge`, requested 1,600 × 900 output, local Unreal Engine 5.8.2 MacEditor, Metal SM5.
- Public PNG SHA-256 values are Overview `12590dd7f6b861b308f324748b33d15b17360909ffaebf39b9db1769bc4cda2f`; Ash Cut `fff303938f55183366a3652cad3969ca9b3400a5e25fa385df8a7f8f67085d53`; Buried Causeway `d5590b412244d7af763aaf9e0912575615467e7c26ebebff55d314397294cf49`; and Folded Verge `5935234004cce12afb11890461d5c994066cf21925af8728ac4f2affa4e3df09`.
- The public page prefers quality-84 WebP delivery derivatives and retains the PNG captures as fallbacks. WebP SHA-256 values are Overview `1b720b3155878f302bddf9bbfd20f3638cbe1d48b5581efead3ac3189100f4ba`; Ash Cut `c9a2747514584242c74ccc4ec1c7ecdd06d994263d89332d9fe50c53e08572ec`; Buried Causeway `03c01dc4bcf7da1593742c83b5b9dc8737911898dcd17baf7998b98ae678b7b9`; and Folded Verge `6cc80038fb9d227f2b2747ea63ba0260bf2aeba3275d3711d8a1cd04303f42ee`.
- Capture-log SHA-256 values are Overview `d849c41a04084ebd78d69f9ac5409fe29a2445d637d2fe74e08f442b3299d249`; Ash Cut `5775e2c852952aa5cd652dd9e741f7da85b551f9cff91c457326a9f2bed0bd8f`; Buried Causeway `599551c307ee85be0f75e77822b3e50a887488e8e8f203daf3c8ee9bef611ce5`; and Folded Verge `f78c21ca23490c2b1dbf43f3c447b9e05bcdf3a739d714b938b610b1d92efec2`.
- A post-integration Metal refresh against current source commit `981569b143761835cff9194cebd356ee5774ba57` produced the same accepted compositions with expected frame-level rendering variation. Refresh PNG SHA-256 values are Overview `88905df959f67ddf9a7c58d6be648b3925da053b6320356e52a25d1c1f9267d6`; Ash Cut `d9075c40bb70f94e87902cda27ef5b281e6ab59f298d4302d63de90e6e0e1c7e`; Buried Causeway `89a46753730a91479b0b20206d1c1696954fa4c9936bda7bc4d979ddf778109a`; and Folded Verge `b104befc3b37dbc05740f7df6b85a47a647ed50882bc3fbf33c6e577a38b9717`. Refresh-log SHA-256 values are Overview `f327999e59e2946cbbeefd60bb8de6bca0bd1b79894140458429aef945f01186`; Ash Cut `8c4393bf37d7d812c84072557219af3e8e689b6a58b7e76c17d7d93d21a977b6`; Buried Causeway `6b4d74c8829fec9bf6ac04794c1e6e63027624456f80c8b688adb7ed8f981841`; and Folded Verge `587c1218bbe52cab477f613e209b4f9055d3eb673fb1e3cf3b1d6f221cd17616`.
- The same current source was invoked with both the bare overview flag and `-EchoesGlassScarReview=AshCut`; game-mode and camera markers both retained `mode=AshCut`, and the resulting image matched the current Ash Cut refresh hash. The selector-precedence log SHA-256 is `8c4393bf37d7d812c84072557219af3e8e689b6a58b7e76c17d7d93d21a977b6`.
- The fixture isolates presentation and selects camera composition only. The accepted logs record `terrainComposition=glass_scar_v2`, authored routes, ridges, shards, shelves, Matter deposits, and `collisionAuthority=false routeAuthority=false finalArt=false`. These captures demonstrate current editor rendering of the authored environment candidates. They do not establish changed pathing, ordinary player route discovery, combat-load readability, package performance, final topology, production materials, VFX/audio, or release quality.

## Rules

- No copied game names, characters, dialogue, silhouettes, UI layouts, music, maps, or distinctive assets are authorized.
- Marketplace, stock, generated, public-domain, contractor, and commissioned assets require an individual entry before use.
- A verbal claim of permission is insufficient for commercial distribution; retain the license, assignment, or written authorization.
- Placeholders must remain visibly and textually labeled in development builds and must not be described as final art or audio.
- Source files and export settings belong in the registered asset family. A rendered derivative does not erase its source-license obligations.
- AI-assisted asset generation requires the exact service, plan when exposed, date, prompt/source inputs, output terms, and human modification record. CONCEPT-001, CONCEPT-002, and CONCEPT-003 record the available generation evidence and remain development references rather than production assets. CONCEPT-004 and CONCEPT-005 are recorded with incomplete provenance records and are bounded to concept presentation until the owner completes them.

## ART-016 implementation and verification record

- Generator: `Source/EchoesOfTheBrokenSun/Private/EchoesFogView.cpp` and `EchoesFogView.h`; automation test: `Source/EchoesOfTheBrokenSun/Private/Tests/EchoesProductionFogTest.cpp`.
- Presentation layer: Authoritative 64×64 visibility grid presentation with three-tier visual grammar:
  - **Unexplored**: Deep charcoal basalt volume (`Height: -16 to 184 uu`) with faint magenta fracture-bleed (`FLinearColor(0.06f, 0.012f, 0.08f, 1.0f)`).
  - **Explored**: Desaturated memory layer (`Height: 6 uu, Centre: 14 uu`) over persistent geometry with muted slate-indigo tint (`FLinearColor(0.015f, 0.022f, 0.035f, 0.72f)`), concealing unobserved actors while maintaining tactical terrain geometry.
  - **Visible**: Full palette without shroud.
- Component invariants: All layers strictly configure `ECollisionEnabled::NoCollision`, `CanEverAffectNavigation() == false`, `CastShadow = false`, `bGenerateOverlapEvents = false`.
- Accessibility: `ReducedFlashing` clamps fracture bleed emission to ≤ 0.1; `ReducedMotion` halts drift and phase coordinates.
- Performance budget: Full 4,096-tile grid synchronization completes in ≤ 1.5 ms; incremental sync under active combat averages < 0.2 ms.
- Zero Simulation Touch (SIM-002): Bit-for-bit checksum equality maintained across 40 simulation steps between fog-synced and headless simulations.
- Verification: Automation test `Echoes.Runtime.Presentation.ProductionFog` passed (Result={Success}, exit code 0); 96/96 native simulation tests passed across release, debug, and ASan/UBSan sanitizers; runtime smoke passed across all three factions.

## Current evidence boundary

No final environment, character, animation, music, voice-acting, complete sound-effect, cinematic-art, typeface, or third-party-plugin family has been added. ART-001 replaces the baseline unit and structure primitives with distinct project-authored static-mesh candidates. ART-002 replaces the Future Well basic-shape placeholder with a four-part landmark and geometry-distinct states. ART-003 replaces visible Glass Scar shelves, ridges, shard fields, three route treatments, and Matter-node primitives with project-authored static-mesh candidates and a shared world material. Its shared material now passes one exact-package local Metal instancing-compatibility gate, but the captured overexposure and prototype overlay remain open presentation debt. ART-004, ART-007, and ART-008 advance the Ash Cut, Buried Causeway, and Folded Verge through production-oriented topology, UV, collision, dedicated-material, provenance, regeneration, runtime-isolation, and isolated-review gates. ART-005 replaces the selection halo and seven accepted-order primitive compositions with authored emissive mesh candidates and explicit reduced-motion/reduced-flashing behavior. ART-006 adds geometry-driven functional-loss feedback after a previously visible authoritative entity is removed, with fog/load/restart exclusions and the same presentation-only accessibility boundary. AUDIO-001 adds original accepted-command and faction-distinct functional-loss cues with bounded spatial/rate-limit and effects-volume/reduced-dynamic-range behavior. The collision floor and every authoritative terrain, route, pathing, resource, placement, line-of-sight, command, damage, removal, save, replay, and checksum decision remain unchanged. These families do not establish completed production textures, final surface response, character animation, transparent dissolve, Niagara debris/smoke, final mixing/mastering, music, ambience, voice, complete alerts, sustained performance, broad player readability, general package behavior, or final production quality. Fog, atmosphere, health bars, ownership markers, tactical minimap, and most audio remain project-code, Engine-provided prototype presentation, or unimplemented. Visual and audio quality requirements have not been fully validated.


## ART-WORLD-001 — Soryn terrain kits (world-map concept pass)

**Author:** Angelis Pseftis. **State:** authored source; generation and rendered qualification pending.
Original deterministic project geometry in `Scripts/echoes_world_kits.py`, called exclusively by
`Scripts/generate_art_assets.py`. Revision `soryn-world-kits-v3`: twelve meshes under
`/Game/Art/Generated/World/Environment/SM_World_{Basalt,Shivergrass,Cavern,Civic,Choir,Solar}{Formation,Ground}`.
No external assets, licensed meshes, or generated concept pixels are imported. Shapes derive from
`Docs/MapConcepts.md` and the Track A site identities, subordinate to SPEC-VISD-001/006,
SPEC-SIM-002, REL-ART-015/016 and visibility authority. Formation footprints fit blocked cells;
ground relief stays below 20 cm. Four material zones use charcoal, ceramic, amber/magenta accents.
All runtime instances disable collision, overlap, navigation influence and shadows. Tall formations
and ground relief share the player's explored/visible tile gate. Static geometry does not pulse or drift.
Both detail levels have explicit recipes. Source repeatability, mesh generation, in-game appearance,
performance, per-site landmark composition and owner acceptance remain separate unproven gates.

`SM_World_GlassScarShelf` source revision advances to `glass-scar-shelf-fractured-v6`: flush walking surface with
irregular extruded strata below it; removes repeated luminous bars and raised rectangular plates.
This replaces the registered shelf in place and preserves its 780 cm footprint and 39 cm top datum.

World material revision `world-surface-textured-v7` adds a final shader roughness clamp to
[0.85, 1.0] after texture/noise modulation (REL-ART-003). The material graph is repaired in place,
retaining existing references. Parameter values alone had not enforced the surface floor.
Distant terrain-kit formations form a fixed public backdrop beyond the playable rectangle;
they disclose no live terrain or game state. Per-site landmark authoring is still outstanding.

ART-WORLD-001 revision v2 replaces stacked-disc natural formations with original closed fracture
polyhedra, explicit face normals and UVs. The revised Glass Scar shelf uses those fractured volumes below the
unchanged walking surface. Source remains deterministic; geometry contracts cover footprint,
height, face normals and repeatability. Runtime and packaged qualification remain separate.

ART-007 Buried Causeway advances to `buried-causeway-production-v2`: concentric fitted
stone pavers, weathered matte ceramic, narrow amber service conduits and recessed seams.
`Scripts/generate_art_assets.py` and `Scripts/echoes_world_kits.py` are the original source.
The prior blank disc and bright transverse cyan bars are retired. No gameplay footprint changes.

`SM_World_BrokenSunSky` advances to `broken-sun-sky-fractured-v4`: original spherical fracture
plates enclosing the amber core, irregular seams and drifting basalt Dawnshards. Tangent box
plates are retired. Both LODs use the same authored construction method. These are original
in-project geometry, with no third-party mesh, texture or concept-image pixels incorporated.

ART-WORLD-001 also registers `SM_World_WalkSurface`, an original 200 cm substrate generated by
`walk_surface` in `Scripts/echoes_world_kits.py`. Its top is exactly z=0; all depth is below.
Runtime instances follow the local player's terrain visibility and replace the visible Engine
cube on non-chasm sites. The existing invisible pointer-trace floor remains unchanged.

ART-WORLD-002 registers `M_EchoesSky` (`soryn-sky-gradient-v1`), an original in-project unlit
altitude gradient generated in `Scripts/generate_art_assets.py`. It replaces ground-texture
reuse on the sky dome, with continuous transitions across four existing bands. No third-party
image or shader source is used. The shared sky/celestial backdrop is now available to all
outdoor sites. Lighting uses the Art Direction gold key and indigo fill anchors.

ART-WORLD-001 revision v3 gives Shivergrass original bent, double-sided leaf ribbons,
with 64/24 leaves by LOD. Leaves use the Unreal Engine white texture solely as neutral
albedo input (UE-ENGINE license); their shape and coloring are project-authored. No
reference-image pixels, downloaded models or third-party foliage are used. Shelf v6
corrects the fracture wedges to the registered 780 cm footprint; geometry-contract
checks verify the bounds and 39 cm top datum. Public perimeter forms remain outside
playable cells; local detail and canyon instances follow known terrain. The six kit
families do not constitute fifteen finished mission maps.

Identifier note: this pass uses ART-WORLD-001/002 because historical ART-017/018 and
ART-022 labels already name different entries in the register table and generation
records. Those historical records are retained; the new IDs avoid expanding the collision.

ART-WORLD-003 registers `SM_VFX_AbilityRangeRing` (`ability-range-ring-v1`). Original
scripted torus geometry in `Scripts/generate_art_assets.py`, using the existing registered
project VFX material; no external source. The 50 cm outer radius and 1 cm band use
128/96 circumferential segments by LOD. Power/supply fields scale to authoritative
rule radii, retain state visibility, and use steady restrained emission. No collision,
overlap, navigation or shadow authority. This replaces oversized selection brackets
on ability fields; it does not alter the selected-unit glyph. Generation and rendered
qualification pending. Regenerate with `ECHOES_ABILITY_RING_ONLY=1 Scripts/generate_art_assets.sh`.

ART-WORLD-004 registers `M_EchoesShivergrassLeaf` (`shivergrass-leaf-v1`): original
project two-sided foliage shader, silver tint, .9 roughness, tinted transmission,
no external image, texture, or emissive source. A retained Unlit runtime capture
confirmed the v4 black ribbons carried the pale albedo; the defect was in lighting.
World-kit v5 removes coplanar reversed leaf duplicates, uses the dedicated foliage
shader, and computes tangents for world-kit meshes. Actual lit result remains pending.

## ART-WORLD-005 — M01 evacuation site

Original project-owned archive cradles, parked service frames, fitted route paving and
terminated conduit hardware are authored in `Scripts/echoes_evacuation_props.py`, revision
`m01-evacuation-props-v1`, through `Scripts/generate_art_assets.py`. Author: Angelis Pseftis.
No external asset or generated-image source is used; Unreal geometry operations retain
UE-ENGINE provenance. Outputs are `SM_World_M01ArchiveCradle`, `SM_World_M01ArchiveFrame`,
`SM_World_M01RoutePaving`, `SM_World_M01ServiceConduit`, and four `MI_M01Evacuation_*`
materials. The family explains loading, maintenance and worked access in the current M01
production brief; it adds no actor interaction or power signal.

Recipes provide two LODs, authored normals/UVs, four matte material zones and bounded
single-tile footprints. Solid hardware is restricted to blocked source cells; paving is
flush on passable terrain. Source placement is registered in
`Content/World/Source/Presentation/m01_evacuation_landmarks_v1.json`, compiled with its
terrain/source digests. Runtime must gate every record by explored visibility and current
terrain, disabling collision, overlap, navigation and shadows. Generation is idempotent
by asset revision; a recipe change requires a new revision and regeneration. Three Python
geometry checks passed; generated/runtime/rendered qualification remains pending.

ART-WORLD-005 revision `m01-evacuation-props-v3` corrects the narrow paving cross-member's
chamfer limit, fills panel bays, and projects UVs by face orientation to avoid collapsed
vertical-face UVs. Revision v1 stopped on sparse material-section validation; v2 was rejected
by the strengthened winding test. Both generation logs are retained. Source tests now check
triangle winding against authored normals and nonzero UV triangle area as well as bounds,
materials and deterministic LOD output. Sparse LOD material bindings are explicitly retained.

ART-WORLD-003 revision `ability-range-ring-v2` retains the exact 50 cm outer source radius
but narrows the band to 0.36 cm. `M_EchoesRangeBoundary` (`range-boundary-v1`) is an original
steady unlit Color×EmissiveStrength graph, preventing directional-light specular highlights
from overwhelming the terrain. Runtime power/supply fields use that mesh's material;
selection and combat effects retain their own materials. The prior lit capture showed the
v1 boundary remained too prominent. Source provenance remains Angelis Pseftis/project-owned;
this revision requires a new render and radius/material checks.

ART-WORLD-001 revision `soryn-world-kits-v6` and ART-WORLD-005 revision
`m01-evacuation-props-v4` correct custom buffer triangle winding to Unreal's left-handed
front-face convention. Local UE5.8 GeometryCore `Public/VectorUtil.h` lines80–96 defines
triangle normal as `(V2-V0).Cross(V1-V0)`; previous mathematical CCW fans disagreed with
supplied outward normals. Nonplanar fractured rock caps now receive per-triangle normals,
and dominant-face projection preserves UV area. The retained M01 v3 lit capture shows
black paving/hardware and is rejected as material/geometry quality evidence.

Affected shared descendants are shelf `glass-scar-shelf-fractured-v7`, fractured sun
`broken-sun-sky-fractured-v5`, and Buried Causeway `buried-causeway-production-v3`.
Four world-kit geometry tests and three evacuation tests pass, including engine-convention
winding and UV-area checks. Regeneration and lit reinspection are required before any claim
that the dark-face defect is fixed. No simulation, collision or navigation change is intended.


### 2026-09-04 — Meridian forward axes and M01 civic material correction

ART-001 retains its roster-silhouette-v2 baseline except Meridian Bulwark and Lancer,
which now record `meridian-forward-axis-v3`. The original generator used yaw instead of
pitch to orient local-Z shield cylinders and the forward rail muzzle. Updated Bulwark
faces and projectors and the Lancer muzzle point along mesh-local +X, with symmetric
outward shield cant. Two generator geometry-axis tests passed; retained generation evidence:
`BuildArtifacts/Evidence/world-map-concept-pass/meridian-forward-v3-engine.log`.
This is a geometry correction, not completed unit art or animation qualification.

ART-WORLD-005's four M01 material instances now use the registered civic surface master
instead of the basalt ground shader. Their source revision is `m01-evacuation-material-v2`;
geometry stays `m01-evacuation-props-v4`. UV scale is1, emission is0, and the ceramic/service
palette is unchanged. The actual prior graph/parameter evidence is retained in
`BuildArtifacts/Evidence/world-map-concept-pass/material-bindings-audit.json`.
The materials and geometry are original project work by Angelis Pseftis, using existing
registered texture sources. No external assets or licenses were introduced.


### 2026-09-04 — M01 evacuation architecture and geology production pass

ART-WORLD-005 now uses `m01-evacuation-props-v5`: archive load rails and retaining
straps, braced handling frames with parked trolley hardware, terminated service conduits,
and fitted paving repair keys. Four meshes retain two authored LODs and no collision.
The source is `Scripts/echoes_evacuation_props.py`; material revision remains
`m01-evacuation-material-v2`. `evacuation-v5-engine.log` records generation. Three
geometry-contract tests passed; visual detail and motion qualification remain open.

ART-WORLD-001 advanced through v7/v8 to `soryn-world-kits-v9`. The v7 all-tile fracture
plates were rejected after rendering because they looked like repeated paving. V8 removed
that overlap/repetition but its square rock crowns were also rejected. V9 uses broad,
cleaved basalt crowns, recessed strata and sparse low outcrops. The shared world shader
`world-surface-textured-v8` bounds texture modulation to preserve the authored palette.
Shelf revision is `glass-scar-shelf-fractured-v8`; celestial crust revision is
`broken-sun-sky-fractured-v6`. Generation receipts are `geology-v7-engine.log`,
`geology-v8-engine.log` and `geology-v9-engine.log` under
`BuildArtifacts/Evidence/world-map-concept-pass`. Four geometry-contract tests passed;
V9 awaits current rendered inspection. Prior screenshots are not visual acceptance.

ART-001's Meridian Bulwark/Lancer override is now `meridian-forward-axis-v4`.
Chamfered ceramic armor shells sit over dark load joints; corrected +X shield/muzzle
orientation is retained. `meridian-armor-v4-engine.log` records two generated meshes
with two LODs. The first wrapper returned1 solely because its success pattern still
expected v3; the engine's v4 completion marker and absence of material/geometry errors
were verified, and the wrapper pattern was corrected. V8 captures show the new armor;
full unit craft, animation and combat readability remain open.

All geometry and shader modifications above are original project work by Angelis Pseftis,
using previously registered textures and Unreal geometry operations. No new external
source asset or distribution license was introduced.


### 2026-09-04 — continuous M01 cliff source and occupied gantries

`Source/EchoesOfTheBrokenSun/Private/EchoesCliffMesh.h/.cpp` now generates M01's
continuous known-terrain cliff surfaces. This original project geometry is authored by
Angelis Pseftis and uses the installed Unreal ProceduralMeshComponent runtime module.
The adapter uses the registered world material; no new external art or texture source
is introduced. Geometry is disposable presentation derived from scoped terrain, with no
collision, navigation, overlap, shadow, save or checksum authority. Build succeeded;
rendered qualification remains pending at this record.

ART-WORLD-005 advances to `m01-evacuation-props-v6`. Gantries hold a retained equipment
load, making blocked terrain visually legible, and the trolley's load line reaches its
handling fixture. Source remains `Scripts/echoes_evacuation_props.py`, materials remain
`m01-evacuation-material-v2`; four meshes/twoLODs/no collision generated successfully in
`BuildArtifacts/Evidence/world-map-concept-pass/evacuation-v6-engine.log`.


### M01 continuous basalt surface production — 2026-09-04

**Author:** Angelis Pseftis. Original deterministic project source; no external art inputs.
The dedicated `M_EchoesCliffSurface` source in `Scripts/echoes_cliff_material.py` is registered
for the M01 evacuation margin under the existing world-art family. It separates cut basalt
bedding, darker cross-fractures and grain from the horizontally projected ground texture.
The shader must retain a charcoal body, matte roughness and non-emissive walls. Its first
revision is an authoring candidate, pending generation and rendered inspection; no acceptance
or performance claim is assigned here. Procedural geometry remains scoped to known blocked
terrain and cannot affect collision, navigation, fog authority or simulation results.

M01 voice preparation now has exact-byte model/distributor provenance in
`BuildArtifacts/Evidence/world-map-concept-pass/kokoro-provenance/`. The installed ONNX and
aggregate voices files match streamed official release SHA-256 values. Retained Hexgrad
model-family Apache-2.0 and wrapper MIT documents have distinct scopes. Generated takes remain
private evidence candidates, not final registered/runtime-bound audio. V1's full-scale PCM
samples require a protected retake; final listening, subtitle/action binding and mix checks
remain outstanding.


The first dedicated cliff material revision `cliff-surface-3d-basalt-v1` generated successfully
on 2026-09-04 (`cliff-material-v1-engine-retry.log` in the current evidence root). Stale graph
revisions rebuild within the same asset; no asset-reference replacement is required. Runtime
integration and rendered comparison remain pending at this entry.

Protected M01 voice retakes completed at `m01-voice-candidates-v2/`: 28 canonical lines,
109.95 seconds total, 48 kHz mono PCM24. File hashes and finite PCM streams passed; no PCM24
full-scale samples remain. Sample peaks span -6.64 to -2.61 dBFS after conversion. The driver
records source peak and applied gain; this is not integrated loudness, true-peak, perceptual
listening or runtime synchronization evidence. V1 is retained with the identified headroom
defect rather than treated as an accepted take.


### M01 archive working apron — 2026-09-04

Original project recipe `m01-evacuation-props-v7` adds `SM_World_M01ArchiveApron` as a fifth
family mesh. The civic work surface occupies the source-validated open footprint x18–22,
y16–19 around recovery. Local bounds X±499/Y±399/Z0–3.3 cm and the authored half-tile pivot
keep it flush with the movement plane. Panels, cargo-registration recesses, wear strips and
fitted repairs explain loading use without a new interaction or obstruction. Two LODs keep
the footprint and meaningful markings. All 20 cells must be known and compatible before it
appears; overlapping single-tile paving has been removed. Compiler tests and three prop
geometry tests pass; generation/render evidence is pending at this entry.

The basalt material advances to `cliff-surface-3d-basalt-v2`: wider, narrower-value bedding
seams replace the visually excessive contour bands. It generated successfully; renderer
comparison remains pending. These are authoring revisions, not final visual qualification.


### Composed archive loading face — 2026-09-04

`m01-evacuation-props-v8` adds the sixth mesh, `SM_World_M01ArchiveLoadingFace`, from original
project geometry. It replaces two cradles and their conduit on the source-verified blocked
strip x23–27,y19. A continuous retaining base explains the obstruction, while four restrained
cassettes, rear piers, supported overhead rail and attached parked trolley explain archive
handling. Cassette count/locations and support connections stay consistent between LODs.
All geometry remains inside the 5×1 footprint; the renderer gates all five cells. The compiler
now enforces actual recipe dimensions, zero yaw and the exact anchor/pivot-to-footprint center,
including negative tests, to prevent visual disclosure outside the checked footprint.

V8 also corrects apron material semantics: slot0 pale ceramic, slot1 dark metal/registration,
slot2 aged ceramic and slot3 muted non-emissive teal fittings. Generation and combined runtime
qualification are pending at this entry; no finished environmental or audio quality is claimed.

### M01 live editor surface pass — 2026-09-04

`m01-evacuation-material-v4` persists instanced-static-mesh usage on the civic master and
inherits it explicitly in the four service instances. A fresh editor launch confirmed the
saved flag on all four instances and removed the observed default-material fallback. The
instances now use the original `T_EchoesServiceCeramic` BaseColor/MRE/Normal maps: continuous
warm-grey aggregate, shallow pitting and restrained service wear, without the small panel
grid in the general civic texture. The new named family retains the existing texture-family
revision convention; its exact source and generated hashes are retained with this pass.

`m01-evacuation-props-v9` raises the apron registration fields, wear rails and repair plates
above the panel tops; they were previously partly or wholly buried. All six family meshes
were regenerated in the open editor with two LODs and zero simple collision. Existing
geometry checks passed bounds, winding, UV area, material zones and deterministic LODs.
`cliff-surface-3d-basalt-v4` uses a darker charcoal body, restrained grain/bedding and removes
the warped fracture pattern that read as contour lines. This changes material appearance,
not terrain, cliff silhouettes, collision or visibility rules.

Evidence: `BuildArtifacts/Evidence/editor-visual-pass-20260904T235321Z/`, especially
`m01-archive-approach.png`, `editor_session/editor-archive-approach.log` and
`surface-pass-manifest.json`. The carrier reached recovery through the existing ordinary
scout-command preview; the recovered/intact state and loading face were inspected in PIE.
This is an editor-rendered material/geometry correction, not complete-map, packaged-journey,
performance, audio, physical-input or owner-acceptance evidence. Original project source;
author and owner Angelis Pseftis; no external art dependency was introduced.

### M02 migration-lane stonework — 2026-09-05

Author and owner: Angelis Pseftis. Original project geometry, no external art dependency.
`Scripts/echoes_migration_props.py` revision `m02-migration-props-v2` defines
`SM_World_M02ObservationSill`, `SM_World_M02RootingShoulder` and
`SM_World_M02PassagePaving`. The first occupies a yaw-aware 3×1 blocked footprint;
shoulders occupy single blocked cells; paving stays below four centimetres on passable cells.
All three meshes were regenerated in the open editor with two LODs and zero simple collision.
Four `MI_M02Migration_*` instances inherit the original world-position basalt master, with
instanced-mesh usage retained and muted mineral values. Material revision is
`m02-migration-material-v1`; no emissive objective or ecological signal is introduced.

The 47-record M02 presentation source binds unchanged terrain SHA-256
`5024db41cd825e2e948a84860b9f58f2a71690a650929cafa80a24fef6458f6c`.
Three pure geometry checks and nine presentation compiler checks passed. These establish
bounds, faces, material zones, repeatable LODs and source/footprint integrity; they do not
establish complete-map quality, gameplay or owner acceptance. Evidence remains under
`BuildArtifacts/Evidence/editor-visual-pass-20260904T235321Z/`.

### M01 frame joint correction — 2026-09-05

ART-WORLD-005 revision `m01-evacuation-props-v10` seats the ArchiveFrame posts against
the main crossbeam and seats its upper running rail directly on that beam. This closes
two 2 cm construction gaps without moving a landmark or changing its footprint. Other
family geometry and materials remain identical. Original project-owned deterministic
source by Angelis Pseftis; same two-LOD, zero-collision pipeline and licensing basis.
The current object-finish session retains generation and rendered review evidence.

### M01 conduit contact correction — 2026-09-05

ART-WORLD-005 revision `m01-evacuation-props-v11` extends the two ServiceConduit tubes
into their tapered east-side receiving housings while preserving their original capped
west ends. Endpoints lie inside each fitting, avoiding a nominal face contact that would
still miss the taper. The original five placements, footprint, materials, static behavior,
two LODs and zero collision remain. Original deterministic project geometry by Angelis
Pseftis; no external dependency or rights change. Generation and scene inspection are
retained in the active M01 object-finish evidence root.

### M01 cradle support correction — 2026-09-05

ART-WORLD-005 revision `m01-evacuation-props-v12` gives each ArchiveCradle shell a
grounded transverse saddle connected to both load rails. One continuous retention band
replaces two overlapping horizontal bands per shell; its ends return to the rails. The
inspection cabinet gains a grounded footing and its panel seats on the cap. The source
retains the registered footprint, static archive-handling role, materials, high/low detail
structure and zero collision. Original project-owned deterministic geometry by Angelis
Pseftis, with unchanged licensing and no external dependency. Final generated identities
and close placement inspection belong to the active M01 object-finish evidence root.

### M01 service route surface finish — 2026-09-05

ART-WORLD-005 revision `m01-evacuation-props-v13` replaces RoutePaving's four small
quarter-panels and repeated corner fittings with one 198 cm service slab, two continuous
wear tracks along its working axis and one close-detail repair. The retained normal-camera
return view exposed the former small-grid repetition. All fifteen paving records, ≤4 cm
height, registered footprint, material zones and zero collision remain. Original
project-owned deterministic geometry by Angelis Pseftis; no external dependency.
The object-finish evidence root retains before/after views and the generated two-LOD audit.

### M01 outpost service recess visibility — 2026-09-05

The registered continuous-basalt compositor in `EchoesCliffMesh.cpp` lowers only the
foreground shoulders of the existing east18–20/9–12 and west0–2/10–13 outcrops, retaining
taller back crests. Close normal-camera evidence showed these banks obscuring the low
ServiceConduit and ArchiveCradle. No blocked/visible mask, X/Y footprint, landmark placement
or simulation state changes; original project geometry by Angelis Pseftis. The active
object-finish evidence root records source hashes, live rebuild and before/after inspection.

### M01 knowledge boundary and retaining-bank correction — 2026-09-05

Original source `Scripts/echoes_m01_shroud.py`, revision `m01-shroud-unlit-v1`, adds
`M_EchoesM01Shroud` to the existing registered M01 presentation pipeline. The material
is opaque, unlit, instancing-enabled and receives the same authoritative knowledge
color as the prior shroud. Only CampaignPrologue selects it; the complete occluding volume, visibility masks,
terrain hiding, collision, navigation and simulation remain unchanged. The material
repair does not replace the required volumetric occlusion with a ground decal.
It removes lighting/specular response from unknown space rather than inventing terrain.
Generation is revision-idempotent and checks its color graph output. No external assets
or new rights dependency are introduced. Author: Angelis Pseftis. Runtime review pending.

The existing `EchoesCliffMesh.cpp` M01 compositor now varies its inward perimeter erosion
and shoulder height continuously across chunks, with a ground-reaching wall foot. This
corrects the observed machined bevel and undercut-foot appearance. Only known blocked
cells emit geometry; tests and matched tactical reinspection remain required.

### M01 remembered terrain and fracture-bed correction — 2026-09-05

`Scripts/echoes_m01_shroud.py` revision `m01-shroud-unlit-v2` retains the opaque,
unlit `M_EchoesM01Shroud` and adds `M_EchoesM01Explored`, an unlit translucent
remembered-terrain tint with static opacity0.48. Both are original deterministic
project materials authored by Angelis Pseftis, generated by the existing art pipeline,
and selected only by M01. There is no third-party asset or rights dependency. The
existing opaque explored layer obscured the remembered ground; the new layer leaves
it readable. Unknown-space occlusion, tile transforms and simulation knowledge remain
unchanged. Revision metadata supplies an idempotent generation fast path; byte identity
and runtime blend behavior still require retained checks.

M01 retires the legacy `EchoesScarBand`, `EchoesGlassShard` and `EchoesScarGlow`
presentation actors. Their bright trays and unscoped shards were visible in the
wide fracture diagnostic. `EchoesTerrainView.cpp` instead reuses registered
`SM_World_BasaltFormation` in low bed fragments confined to fully blocked scar spans.
The existing full-footprint knowledge checks hide unknown fragments and lights.
M01 rim formations now start on the authored bed instead of floating1490cm above it (old base−260cm versus current bed−1750cm).
All instances retain cosmetic collision/navigation behavior. This is a composition
revision of an existing kit, not a seventh bespoke M01 prop family or a terrain edit.
Generation and native/runtime reinspection are retained in the active M01 evidence root.

M01 shroud revision `m01-shroud-unlit-v3` corrects the dark grid observed in the v2
explored overlay: opacity applies only to the upward face, so tile side faces do
not overlap and darken every joint. Unknown-space material and volume are unchanged.
The actual `SM_World_GlassScarFoldedVerge` crossing caused the remaining tall magenta
silhouettes at the knowledge edge. All three existing M01 route actors now share
full-footprint known-terrain admission at initial presentation and subsequent
visibility changes; remembered routes remain visible, and reset hides them.
Their source geometry, route/collision contract and other scenarios are preserved.

M01's bed now reuses registered `SM_World_WalkSurface` per known blocked bed cell at
the unchanged authored depth−1750cm. Span-wide fog admission previously suppressed
the bed beneath already known cells, exposing the blue background. The bed's static
material, full authoritative cell mask and all crossings remain unchanged; only
presentation subdivision and knowledge admission change. Reinspection is pending.

### M01 ravine concealment follow-up — 2026-09-05

Author and owner: Angelis Pseftis. `m01-shroud-unlit-v3` materials are unchanged.
The M01-only unknown instance volume now extends from−1766 to184cm to conceal
unknown terrain beneath the authored−1750cm ravine bed. Other-map instances retain
−16..184cm. No mesh/material regeneration, external asset, license or collision change
is introduced. `b1-ravine-admission-audit.json` in the current M01 visual evidence root
retains245-cell source/runtime admission analysis; rendered closure is still pending.

### 2026-09-05 — M01 B1 backing and capture provenance follow-up

Author and owner: Angelis Pseftis. Runtime module8831 adds M01 retaining-bank backing
using the existing registered WorldSurface mesh/material, inside unchanged blocked
rim cells. No external asset or new rights dependency is introduced. EDT bank/fracture
reinspection is retained under `BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z`;
vertical material streaking remains M01-V012. The diagnostic capture helper and native
Swift decoder are original project tooling. Four calibration movies failed full-frame
qualification and are explicitly excluded from mission-motion acceptance. The4496
helper rejects invalid capture geometry. Generator, source, editor and package
evidence remain separate; no package or owner acceptance is asserted.

### 2026-09-05 — M01 B2 material reuse and branch-capture provenance

Author and owner: Angelis Pseftis. M01 ChasmBed now uses the existing registered
`M_EchoesCliffSurface` / cliff-surface-3d-basalt-v4 in all four runtime slots. The mesh,
blocked mask, navigation and collision are unchanged; other map materials retain their
existing binding. M01 ground WorldUVScale is0.0004, while other profiles retain0.0012.
M01 Well body MIDs use metallic0.08/roughness0.78; protocol/core materials are unchanged.
No imported art, license, new rights dependency or generator bypass was introduced.
Receipts: `basalt-basin-ui-identity.json`, `state-ui-identity.json` and
`responsive-hud-identity.json` under the current M01 visual evidence root.

The original editor-only capture helper, isolated accessibility preview and guarded
native-window resize are review tooling. Full-frame1280×720 Harvest and Reshape clips
are retained with decoder metadata and sampled-frame qualification; earlier failed
calibrations remain explicitly excluded. These are EDT motion evidence only. Runtime
identity is the actual loaded suffixed module, not the stale hot-reload modules manifest.
Current-source native automation, packaged provenance and Angelis acceptance remain
separate gates.


### M01 public perimeter and narrative binding follow-up —2026-09-05

**Author and owner:** Angelis Pseftis. The current source reuses registered
`SM_World_WalkSurface` in a separate four-instance `M01ExteriorSkirt`, with Z scale1
and the existing M01 world-aligned material. M01 exterior basalt reuses registered
`SM_World_BasaltFormation` with bounded, deterministic two-course placement. No new
asset, rights source, later-map recipe or gameplay geometry is introduced. The
perimeter placements and HUD objective fit await compiled runtime reinspection.

M01 narrative source/validator now bind each Well branch trio to its own withdrawal
signal; common withdrawal remains generic. `Scripts/compile_narrative.py` emits the
updated pack and SHA sidecar. Approved prose is unchanged. Runtime text binding is
being verified independently from the source's still-pinned authored-unbound voice
and cinematic metadata; no audio synchronization or owner acceptance is implied.
Source and execution identities remain under the existing
`BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z` evidence root.

### M01 connected exterior basalt — 2026-09-05

**Author and owner:** Angelis Pseftis. Original project source; no external art inputs.
The existing continuous-basalt family adds `EchoesCliffMesh::BuildExteriorBank` for M01's
fixed public backdrop. Four joined sides replace that mission's enlarged, repeated
`SM_World_BasaltFormation` perimeter/horizon instances. Each side rises from the exterior
substrate into irregular strata, with shared corner coordinates and a ground-reaching outer
edge. The four registered walking-substrate strips remain. The existing registered
`M_EchoesCliffSurface` supplies the matte, non-emissive surface; no new asset family, imported
content, material recipe or rights dependency is introduced.

The separate `M01ExteriorBanks` procedural component receives no collision, overlap, navigation,
shadow or decal role. It reads only map dimensions and presentation scale, never hidden terrain
or simulation state, and clears on reinitialization outside M01. Interior fog-scoped cliffs and
all other map recipes are unchanged. Source review and the staged patch are retained in
`BuildArtifacts/Evidence/m01-terrain-environment-20260905T115824Z`. Compilation, native geometry
and visibility checks, and rendered review are pending at this entry; no acceptance is assigned.


### 2026-09-05 — M01 Surveyor articulated derivatives (source prepared)

**Author and owner:** Angelis Pseftis

Under existing original Surveyor provenance (ART-001), `Scripts/generate_art_assets.py` defines `m01-surveyor-articulation-v1`: M01SurveyorBody, M01SurveyorUpper, M01SurveyorLower and M01SurveyorFoot. These split the approved exoframe, joint, shin and clamp forms for presentation articulation in M01 only. Original combined Surveyor and the 24-role baseline remain unchanged. Four material zones retain their role assignments; sparse sections are explicitly remapped in both LODs. No external content, rights or canon is introduced.

Runtime binds all four parts atomically for Meridian M01 Workers only. Legs have no collision, overlap or navigation role; existing entity pick proxy remains. Source test four cases passed; generated assets, native motion/contact checks, current editor motion, packaged performance and owner review remain pending. Source-to-generated receipt will be appended in this same register after generation.


M01 Surveyor derivative generation completed 2026-09-05 06:51:14 UTC through `ECHOES_M01_SURVEYOR_PARTS_ONLY=1 Scripts/generate_art_assets.sh`. Four assets passed both-LOD zone/collision audit; no broad purge ran. Exact uasset and generator SHA-256 records: `BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z/surveyor-parts-identity.json`; retained Unreal log `surveyor-parts-unreal-generation.log`. Source/generated evidence only; locomotion remains under native/editor inspection.


### 2026-09-05 — M01 Bulwark deployment derivatives

**Author and owner:** Angelis Pseftis

Under the existing original Meridian Bulwark provenance, `m01-bulwark-deployment-parts-v1` separates the approved chassis and two framed barrier wings. The standard24-role roster remains unchanged. The three M01-only assets are `SM_Meridian_M01BulwarkBody`, `SM_Meridian_M01BulwarkLeftWing` and `SM_Meridian_M01BulwarkRightWing` under `/Game/Art/Generated/Meridian/Units`. Wing hinges use source coordinates26,−24,72 and26,24,72; zero relative rotation restores the approved deployed assembly. No external content or rights source is introduced.

Narrow generation succeeded at2026-09-05 08:33:48 UTC with two LODs and collision0. Chassis LOD0/1 has972/486 triangles; each wing392/196. Chassis sections use material zones0,1,2,3 in both LODs; wing LOD0 uses0,1,2,3 and LOD1 uses1,2,3 because the high-detail primary fittings are omitted. The first generation failed its low-LOD section audit and is retained; correction binds the actual sections to their proper material indices without adding geometry. Nine combined Surveyor/Bulwark source tests pass. Generated SHA-256 and source identities, both generation logs, and partial-attempt evidence are retained under `BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z`, notably `bulwark-parts-identity.json`.

Runtime integration uses inert children of the existing body transform, shared faction materials and authoritative deployed/facing state. Missing-part fallback is source-inspected only; current native and rendered state checks remain pending. This entry establishes source/generated provenance, not packaged visual quality or owner acceptance.

M01 deployment-part runtime follow-up, 2026-09-05: generated Bulwark revision `m01-bulwark-deployment-parts-v1` was bound in loaded module8594. Native084912Z derivative/pool/deployment checks pass with zero warnings/errors; I1 sampled frames1–8 show attached packed/unfolded/folding geometry and consistent facing. These are bounded native/EDT observations, not final locomotion, combat, material, package or owner acceptance. The linked qualification and source/generated/module hashes remain in `BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z` and `m01-motion-tested-actors-live-comparison-20260905T085741Z-3CB1DBE3`. No asset rights or authorship claim changes.

M01 public-bank material binding follow-up, 2026-09-05: the existing `M_EchoesCliffSurface` now shades M01 `BiomeHorizon`; horizontal exterior substrate retains its ground material. No new asset or rights source was introduced. The two existing placement courses receive independent deterministic variation and distinct outside-play depth bands; their124 total instances and inert roles remain. Native101237Z verifies bounds/material separation and non-M01 controls. Loaded4481 L2 all ten normal-camera samples show the material/depth correction, with repeated cap shapes and bare lanes still open asV042. This is source/native/bounded EDT progress, not finished bank composition or owner acceptance.
