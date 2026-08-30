---
title: Echoes of the Broken Sun Asset and License Register
author: Angelis Pseftis
creator: Angelis Pseftis
status: Authoritative
created: 2026-08-28
updated: 2026-08-29
---

# Asset and License Register

This is the single authoritative provenance register and is edited in place. An asset is distributable only when its source, author, license or assignment, modification record, and shipped files are recorded here. Repository presence alone does not establish distribution rights.

| ID | Asset or family | Source | Rights holder / creator | License or authorization | Use | Status |
|---|---|---|---|---|---|---|
| DATA-001 | Faction, unit, structure, and Future Well source definitions | Original project data | Angelis Pseftis | Project-owned original work | Simulation and balance | Approved source data |
| CODE-001 | Project gameplay and simulation source | Original project code | Angelis Pseftis | Project-owned original work | Runtime and tests | Approved source code |
| UE-ENGINE | Unreal Engine runtime/editor | Epic Games | Epic Games | Applicable Unreal Engine EULA | Engine dependency; not vendored | UE 5.8.2 installed locally |
| PLACEHOLDER-001 | Engine basic-shape geometry, default materials, and debug primitives | Unreal Engine installation | Epic Games | Applicable Unreal Engine EULA | Development placeholder only | In use by the runtime prototype; not final art |
| ART-001 | Sixteen procedural static-mesh roster candidates and shared surface material under `Content/Art/Generated` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Runtime roster presentation | Integrated development candidate; not final art |
| ART-002 | Four-part procedural Future Well landmark under `Content/Art/Generated/World/Landmarks` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Runtime Dormant, Harvest, Preserve, and Reshape presentation | Integrated vertical-slice candidate; not final art |
| ART-003 | Seven procedural Glass Scar terrain, route, and Matter-deposit meshes plus a shared world-surface material under `Content/Art/Generated/World` and `Content/Art/Generated/Materials` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry operations | Runtime Glass Scar environment and resource presentation | Integrated vertical-slice candidate; not final art |
| ART-004 | Production-oriented Ash Cut route kit: revised two-LOD mesh, UV-driven master material, and four material instances under `Content/Art/Generated` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Runtime Ash Cut route presentation and production-pipeline acceptance | Integrated production-oriented candidate; not final environment art |
| ART-005 | Eight selection/command presentation meshes and one shared emissive material under `Content/Art/Generated/VFX` and `Content/Art/Generated/Materials` | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Selected-entity and accepted-order presentation | Integrated production-oriented mesh-VFX candidate; not final effects |
| ART-006 | Three destruction-state presentation meshes under `Content/Art/Generated/VFX`, using the ART-005 shared emissive material | `Scripts/generate_art_assets.py`, executed in Unreal Engine 5.8.2 | Angelis Pseftis | Project-owned original work; generated only from project code and Unreal-provided geometry/material operations | Presentation after an authoritative unit/structure removal | Integrated geometry-driven destruction candidate; not final effects |
| CONCEPT-001 | Four 2x2 Meridian/Kharuun unit and structure presentation sheets under `site/assets/concepts` | OpenAI image generation through Codex, 2026-08-29; exact prompts below; no source images | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Arsenal visual targets | Development concept reference; not a runtime or production asset |
| CONCEPT-002 | Four-state Future Well presentation sheet at `site/assets/concepts/future-well-states.png` | OpenAI image generation through Codex, 2026-08-29; exact prompt below; no source image | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Future Well visual target | Development concept reference; not a runtime or production asset |
| CONCEPT-003 | Four-view Glass Scar environment and route presentation sheet at `site/assets/concepts/glass-scar-routes.png` | OpenAI image generation through Codex, 2026-08-29; exact prompt below; no source image | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Glass Scar visual target | Development concept reference; not a runtime or production asset |
| CAPTURE-001 | Meridian and Kharuun in-engine roster captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-001 | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development capture; not package or final-art evidence |
| CAPTURE-002 | Dormant, Harvest, Preserve, and Reshape Future Well captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-002 and the non-shipping art-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development captures; not package, gameplay-usability, or final-art evidence |
| CAPTURE-003 | Glass Scar overview, Ash Cut, Buried Causeway, and Folded Verge captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-002, ART-003, and the non-shipping environment-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development captures; not package, gameplay-usability, or final-art evidence |
| CAPTURE-004 | Exact-source 0.68.0 Ash Cut Metal review frame retained with local acceptance evidence | Local UE 5.8.2 Metal editor run using ART-004 and the non-shipping environment-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Internal production-route acceptance evidence | Exact-source isolated review capture; not package, combat-readability, or final-art evidence |
| CAPTURE-005 | Exact-source 0.69.0 standard and reduced-accessibility selection/command Metal review frames retained with local acceptance evidence | Local UE 5.8.2 Metal editor runs using ART-005 and the non-shipping presentation-VFX review fixture | Angelis Pseftis | Project-owned derivative captures; Unreal Engine subject to its applicable EULA | Internal presentation/accessibility acceptance evidence | Exact-source isolated review captures; not package, combat-usability, or final-effects evidence |
| CAPTURE-006 | Exact-source 0.70.0 standard and reduced-accessibility destruction-state Metal review frames retained with local acceptance evidence | Local UE 5.8.2 Metal editor runs using ART-006 and the non-shipping destruction-VFX review fixture | Angelis Pseftis | Project-owned derivative captures; Unreal Engine subject to its applicable EULA | Internal destruction/accessibility acceptance evidence | Exact-source isolated review captures; not package, broad combat-load, audio, or final-effects evidence |

## ART-001 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`.
- Output: one shared material plus sixteen named static meshes: Surveyor, Lancer, Bulwark, Relay Skiff, Anchor, Power Link, Array Foundry, Aegis Post, Tender, Riftstalker, Cairnback, Resonant, Memory Hearth, Waystone, Growth Basin, and Listening Spine.
- Each roster mesh is authored from project-defined primitive composition, contains two LODs and four material zones, and uses visibility-selection collision. Generation does not import Marketplace, stock, scanned, or third-party source geometry.
- The Unreal asset metadata records `Creator=Angelis Pseftis`, `Provenance=Original procedural geometry generated in-project`, and `Status=Vertical-slice candidate; not final art`.

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
- The ART-003 Ash Cut counts and hash identify the superseded 0.65.0 candidate retained in historical evidence. Version 0.68.0 replaces that asset path; its current counts and hash are registered under ART-004.

## ART-004 generation record

- Exact accepted source: version 0.68.0 commit `ee06313a41cb289279bc89711a72fb9f83eddbd8`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `576b3ecd33e6aaf3746e0a96754c7a99d986588a954e8068171a194707b5741d`.
- Output: revised `SM_World_GlassScarAshCut`, `M_GlassScarAshCut`, and `MI_GlassScarAshCut_Basalt`, `MI_GlassScarAshCut_Ash`, `MI_GlassScarAshCut_Glass`, and `MI_GlassScarAshCut_Vein`.
- The route mesh contains a continuous seven-segment bed, staggered bank strata, broken edge slabs, three paired shard-fin landmarks, and a continuous emissive seam. LOD0/LOD1 contain 1,360/680 triangles. Both LODs have UV0 surface mapping and UV1 lightmap data. The mesh contains four material zones and one simple box collision primitive. Runtime collision, overlap generation, and navigation influence are disabled before presentation; authoritative route/pathing state is unchanged.
- The material family uses project-authored UV-driven noise variation and distinct basalt, ash, glass, and emissive-vein physical parameters. It uses no imported scan, stock texture, Marketplace asset, generated image, or third-party source material.
- Revision `ash-cut-production-v1` is recorded on the mesh, master material, and instances. An immediate exact-source regeneration retained identical tracked bytes and a clean checkout.
- SHA-256 values are Ash Cut mesh `4c0ce206abbac9232aee88baeaaf09eade37bed78f7c9c0f943565c5343e55da`, master material `d0d50c601ee299ef2c57bc0136e4b13a121b9668e7fdf53673f1e8d493216955`, Ash instance `370363ea2ee889194896703d890ead5e44c5264ffe6c494b091801c862d80649`, Basalt instance `6f7d7a757801ceaf0829dabcc07031cd9b90a1dce13e5869b550c1a225e139fa`, Glass instance `ae128d015bc3470cd83cdd22dcb2460f5a04d6eec824a490e75a6c9b38a59c92`, and Vein instance `d555c69c31d6777acfcec5d2e2221286b72dbb23ed9980dbf08c11769e96d722`.
- Exact-commit Metal log and 1,319 × 768 frame SHA-256 values are `546d5d7ac530a901663c5b9c8a75447142479d00d80e5c3a83a2900302fd079f` and `20e60a75bc22d1b09d756efe5ce6b1b4e035a3470574709933c038a688d1c4ac`. The isolated frame accepts the intended layered route/material separation at one review camera. It does not qualify production textures, broad cameras, combat-load readability, destruction, VFX/audio, package performance, or final art.

## ART-005 generation and CAPTURE-005 evidence record

- Exact accepted source: version 0.69.0 commit `21957310e62ffdf4407b3f63846d681496919973`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `af5aced9242d74685cee66e754325c4b3704d07c41c155e3495955de7d1547f3`.
- Output: `SM_VFX_SelectionHalo`, `SM_VFX_CommandMove`, `SM_VFX_CommandAttackMove`, `SM_VFX_CommandPatrol`, `SM_VFX_CommandGuard`, `SM_VFX_CommandBuild`, `SM_VFX_CommandInteract`, and `SM_VFX_CommandOrbit` under `Content/Art/Generated/VFX`, plus `M_EchoesPresentationVFX` under `Content/Art/Generated/Materials`.
- Every mesh contains two LODs and no simple collision. LOD0/LOD1 triangle counts are Selection Halo 360/180; Move 300/150; Attack-move 320/160; Patrol 336/168; Guard 324/162; Build 336/168; Interact 708/354; and Orbit 68/64. Runtime components additionally disable collision, overlaps, navigation influence, decals, and shadows as appropriate. The actors and material parameters are presentation only; command acceptance and simulation authority remain outside this asset family.
- SHA-256 values are material `c339b9a3534d96fbe0f55936e0f77d0802626ae7dc53043ff98e11f840448cfc`; Selection Halo `99f16528c08dc6a7750d176df15143c5c9837d6ce1aa464f4fea5b578a429e22`; Move `293d20749574938ac75e445206298f3cf06eadf4bcc1440628ea98f184b7231e`; Attack-move `f93997eb2d036fe9a360d2429cca9179f2d92ab7d317885fb6822099fab3b84f`; Patrol `6c45d82685df0fcfc10e6a32464aa7290b02e60ae10affdaf902341a309f536d`; Guard `f98e85c4f428a459d487bd72e5a05e58541b6fdf3cb3b3b199735493dec1f476`; Build `ff1087dd88e2184ba0335bfdfd7eefb076441a8fcdc1321ab21611d6fe080acf`; Interact `dfb1d1aa72a6c0d065119be8326e83660dd8befb116bad550d4c128dd771d968`; and Orbit `72763349bbe2c147ddf751b12b055092be103b66e329654df1823e99bae0cd17`.
- The non-shipping review fixture rendered four selected Meridian candidates and all six command identities in separate standard and reduced-accessibility Metal runs at 1,600 × 900. Standard/reduced PNG SHA-256 values are `bcee86afc155c84227f5a08c7a39ed105491efddb59e32a595c8525ef6aced17` and `38c99ce3d984883beccf1f090b755c694fc6a8b2c6cede5cdab289421277d2b7`; corresponding log SHA-256 values are `00a56579b0e2b94179cddda1cea29127195dc921f5f1c1401d615be3b6e325ee` and `15edfa859abdb541c399150d979d63417161b651d8c66bbae39ce15315b355fa`. The initial reduced-run black startup frame was rejected; only the later stable rendered scene was accepted. Both runs exited normally with no queried project warning/error, fatal, assertion, or ensure marker.
- These records accept project ownership/provenance, exact asset identity, structural LOD/collision properties, one isolated standard composition, and one isolated reduced-accessibility composition. They do not establish Niagara/particle quality, transparency, audio confirmation, broad combat/camera readability, every accessibility combination, packaged behavior, performance, or final effects.

## ART-006 generation and CAPTURE-006 evidence record

- Exact accepted source: version 0.70.0 commit `848cd3f0c2a1d9200a8224a12b6cef3fe9f49c4d`; generator and audit wrapper: `Scripts/generate_art_assets.py` and `Scripts/generate_art_assets.sh`; exact generation-log SHA-256 `4a9644fb594847b559615b354a3e908387ef35f53fc15a2ca2dff8253f4d3621`.
- Output: `SM_VFX_DestructionRing`, `SM_VFX_DestructionCore`, and `SM_VFX_DestructionShard` under `Content/Art/Generated/VFX`. The family reuses the project-owned `M_EchoesPresentationVFX` material registered with ART-005; no third-party source asset was added.
- Every mesh contains two LODs and no simple collision. LOD0/LOD1 triangle counts are Ring 312/156, Core 84/64, and Shard 36/36. Runtime components additionally disable collision, overlaps, navigation influence, decals, and shadows. The actor and material parameters are presentation only; authoritative damage, removal, visibility, save, replay, and checksum state remain outside this asset family.
- SHA-256 values are Ring `3f92dedaebdccc5fe70e2e6984ec7c082976fe0e0f86aee15fc033c9981363a7`; Core `4a4b9145e74a9ace4ae758cc941f67c3b0b1888dbd5b66e3b97c9869e17bfbbe`; and Shard `59ff0a6127d3e8452fe7a83cf881f1d1fa103770fe0f1c04da3ea8536da84a95`. A repeated exact-source generation was byte-idempotent and retained a clean tracked state.
- The non-shipping review fixture rendered Meridian and Kharuun Soldier, Heavy, and Core presentations in separate standard and reduced-accessibility Metal runs at 1,600 × 900. Standard/reduced PNG SHA-256 values are `37323803c430397ddcb13afef0f1429b07c0368529f325f6e55f287bd44085ff` and `90d1f13ff4caa5c08837fd963dffdc9709242c64c6a8968950d0d3329786eb0a`; corresponding log SHA-256 values are `af7f5ebee9cce830479dc0c2bae52496245a9bc26fcd3787eb2df49397bad280` and `dba40ec507052f912254168c8bde5ff7ebe2c5ba447e36c86c3743a70facb0a2`. An earlier clipped preflight composition, black startup frames, and a live frame captured after the review actors expired were rejected. Only the recentered saved compositions were accepted. Both exact runs exited normally with no queried project warning/error, fatal, assertion, or ensure marker.
- These records accept project ownership/provenance, exact asset identity, structural LOD/collision properties, one isolated standard composition, and one isolated reduced-accessibility composition. They do not establish transparent dissolve, Niagara debris/smoke, audio confirmation, broad simultaneous-combat/camera readability, every accessibility combination, packaged behavior, performance, or final effects.

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
- AI-assisted asset generation requires the exact service, plan when exposed, date, prompt/source inputs, output terms, and human modification record. CONCEPT-001, CONCEPT-002, and CONCEPT-003 record the available generation evidence and remain development references rather than production assets.

## Current evidence boundary

No final environment, character, animation, music, voice-acting, sound-effect, cinematic-art, typeface, or third-party-plugin family has been added. ART-001 replaces the baseline unit and structure primitives with distinct project-authored static-mesh candidates. ART-002 replaces the Future Well basic-shape placeholder with a four-part landmark and geometry-distinct states. ART-003 replaces visible Glass Scar shelves, ridges, shard fields, three route treatments, and Matter-node primitives with project-authored static-mesh candidates and a shared world material. ART-004 advances only the Ash Cut through a production-oriented topology, UV, collision, and dedicated-material pipeline. ART-005 replaces the selection halo and six accepted-order primitive compositions with authored emissive mesh candidates and explicit reduced-motion/reduced-flashing behavior. ART-006 adds geometry-driven functional-loss feedback after a previously visible authoritative entity is removed, with fog/load/restart exclusions and the same presentation-only accessibility boundary. The collision floor and every authoritative terrain, route, pathing, resource, placement, line-of-sight, command, damage, removal, save, replay, and checksum decision remain unchanged. These families do not establish completed production textures, character animation, transparent dissolve, Niagara debris/smoke, audio, sustained performance, broad player readability, package behavior, or final production quality. Fog, atmosphere, health bars, ownership markers, tactical minimap, and audio remain project-code or Engine-provided prototype presentation. Visual and audio quality requirements have not been fully validated.
