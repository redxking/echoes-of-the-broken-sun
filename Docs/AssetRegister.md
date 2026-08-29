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
| CONCEPT-001 | Four 2x2 Meridian/Kharuun unit and structure presentation sheets under `site/assets/concepts` | OpenAI image generation through Codex, 2026-08-29; exact prompts below; no source images | Direction and project authorship: Angelis Pseftis; generated output: OpenAI service | Account plan was not exposed by the tool and is not inferred; retain for project concept presentation pending release-rights review | Public Arsenal visual targets | Development concept reference; not a runtime or production asset |
| CAPTURE-001 | Meridian and Kharuun in-engine roster captures under `site/assets/engine` | Local UE 5.8.2 Metal editor runs using ART-001 | Angelis Pseftis | Project-owned derivative capture; Unreal Engine subject to its applicable EULA | Public implementation evidence | Current-source development capture; not package or final-art evidence |

## ART-001 generation record

- Generator: `Scripts/generate_art_assets.py`; wrapper: `Scripts/generate_art_assets.sh`.
- Output: one shared material plus sixteen named static meshes: Surveyor, Lancer, Bulwark, Relay Skiff, Anchor, Power Link, Array Foundry, Aegis Post, Tender, Riftstalker, Cairnback, Resonant, Memory Hearth, Waystone, Growth Basin, and Listening Spine.
- Each roster mesh is authored from project-defined primitive composition, contains two LODs and four material zones, and uses visibility-selection collision. Generation does not import Marketplace, stock, scanned, or third-party source geometry.
- The Unreal asset metadata records `Creator=Angelis Pseftis`, `Provenance=Original procedural geometry generated in-project`, and `Status=Vertical-slice candidate; not final art`.

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

## Rules

- No copied game names, characters, dialogue, silhouettes, UI layouts, music, maps, or distinctive assets are authorized.
- Marketplace, stock, generated, public-domain, contractor, and commissioned assets require an individual entry before use.
- A verbal claim of permission is insufficient for commercial distribution; retain the license, assignment, or written authorization.
- Placeholders must remain visibly and textually labeled in development builds and must not be described as final art or audio.
- Source files and export settings belong in the registered asset family. A rendered derivative does not erase its source-license obligations.
- AI-assisted asset generation requires the exact service, plan when exposed, date, prompt/source inputs, output terms, and human modification record. CONCEPT-001 records the available generation evidence and remains a development reference rather than a production asset.

## Current evidence boundary

No final models, textures, animation, music, voice acting, sound effects, cinematic art, typefaces, or third-party plugins have been added. ART-001 replaces the baseline unit and structure primitives with distinct project-authored static-mesh candidates, but does not establish final texture, animation, destruction, performance, or player-readability quality. Terrain, resources, Future Wells, fog, atmosphere, damage pulse, health bars, ownership markers, and tactical minimap remain project-code or Engine-provided prototype presentation. Visual and audio quality requirements have not been validated.
