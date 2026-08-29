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
| CONCEPT-001 | Four 2x2 Meridian/Kharuun unit and structure presentation sheets under `site/assets/concepts` | OpenAI image generation through Codex, 2026-08-29; exact prompts below; no source images | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Arsenal visual targets | Development concept reference; not a runtime or production asset |
| CONCEPT-002 | Four-state Future Well presentation sheet at `site/assets/concepts/future-well-states.png` | OpenAI image generation through Codex, 2026-08-29; exact prompt below; no source image | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Future Well visual target | Development concept reference; not a runtime or production asset |
| CAPTURE-001 | Meridian and Kharuun in-engine roster captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-001 | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development capture; not package or final-art evidence |
| CAPTURE-002 | Dormant, Harvest, Preserve, and Reshape Future Well captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-002 and the non-shipping art-review fixture | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development captures; not package, gameplay-usability, or final-art evidence |

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

## Rules

- No copied game names, characters, dialogue, silhouettes, UI layouts, music, maps, or distinctive assets are authorized.
- Marketplace, stock, generated, public-domain, contractor, and commissioned assets require an individual entry before use.
- A verbal claim of permission is insufficient for commercial distribution; retain the license, assignment, or written authorization.
- Placeholders must remain visibly and textually labeled in development builds and must not be described as final art or audio.
- Source files and export settings belong in the registered asset family. A rendered derivative does not erase its source-license obligations.
- AI-assisted asset generation requires the exact service, plan when exposed, date, prompt/source inputs, output terms, and human modification record. CONCEPT-001 and CONCEPT-002 record the available generation evidence and remain development references rather than production assets.

## Current evidence boundary

No final models, textures, animation, music, voice acting, sound effects, cinematic art, typefaces, or third-party plugins have been added. ART-001 replaces the baseline unit and structure primitives with distinct project-authored static-mesh candidates. ART-002 replaces the Future Well basic-shape placeholder with a four-part landmark and geometry-distinct states. Neither family establishes final texture, animation, destruction, sustained performance, broad player readability, package behavior, or production quality. Terrain, resources, fog, atmosphere, damage pulse, health bars, ownership markers, and tactical minimap remain project-code or Engine-provided prototype presentation. Visual and audio quality requirements have not been validated.
