# Campaign and skirmish environment concept collection

**Author and owner:** Angelis Pseftis  
**Created:** 2026-09-06  
**Status:** Candidate environment studies; no canon acceptance, production-stage advancement, navigation certification or Unreal integration.

This collection develops the terrain and environmental objects for all fifteen campaign locations and the three named offline skirmish maps. The review gallery presents one landscape sheet and three detail panels per location. The machine-readable brief and object supplement preserve each design's identity and source. Repeated instances are not counted as new concepts.

The owner requested the full campaign and skirmish concept set after the faction-base studies. These new images supplement existing intellectual property. Older images remain available even when they contain placeholders. A canonical place name does not make every shape in a new illustration canonical.

## Authority and isolation

The live primary checkout supplies the creative and behavioral references: [Requirements](../../Requirements.md), [Development Bible](../../Archive/DevelopmentBible.md), [MapConcepts](../../MapConcepts.md), [ArtDirection](../../ArtDirection.md), and [VisualTargetSpecification](../../VisualTargetSpecification.md). Exact observed paths, revisions, hashes and dirty state are retained in [source-snapshot.json](source-snapshot.json). These relative documentation links resolve within this branch; the snapshot identifies the live primary versions actually consulted if the branch differs.

Work is isolated on `docs/map-environment-concepts`, based on `8d9ba30`, in the separate map-environment-concepts worktree. The primary checkout's gameplay/HUD work and the concept-production-pipeline checkout's model work are outside this change. No Content, Source, Config, map, Blueprint, build script, workflow or existing concept has been changed. Remote image generation does not launch the shared Unreal Editor or a local model/render workload.

The archive location of the Development Bible does not retire its creative authority. The book-informed earlier selections supply visual history; they do not override current game requirements. Prompts are retained in [map-concepts.json](map-concepts.json). The new images are derived composition studies, not replacements for original approved references.

## Review the set

Open [review.html](review.html) for the full gallery. Each card identifies the story place, terrain purpose, three detail studies, and branch limitation. Full-resolution images are retained in `images/` with provenance receipts; original generated outputs are also copied into the external-drive evidence collection. New artwork stays CANDIDATE / NOT_STARTED until a later authorized review and production package establish otherwise.

| Code | Place | Distinct environmental composition |
|---|---|---|
| M01 | Glass Scar evacuation margin | Archive working court, separate Well approach and withdrawal threshold between vitrified shoulders. |
| M02 | Shivergrass migration basin | Open grass lanes, stepped basalt divider and worked observation sills. |
| M03 | Reserve service district | Distributed cistern, transit and archive maintenance branches. |
| M04 | Unburied Road | Asymmetric supported mineral vaults, rooting docks and terminal custody niche. |
| M05 | Line of Parity | Long neutral corridor, repaired revetments and offset witness stations. |
| M06 | Sector 9 edged absence | Diagonal bounded absence, terminated services and distinct archive/extraction routes. |
| M07 | Listening-Spine ridge | Resonant sockets, separated witness approaches and low confluence hollow. |
| M08 | Confluence verge | Offset frames above stable terraces and two public-contact approaches. |
| M09 | Authority Exchange | Formal central allocation court with three separated district fronts. |
| M10 | Liability district and Well court | Maintained deferred frontage, sequential spine settings and separate Well court. |
| M11 | Census Forecourt | Public contribution and evidence stations connected through exposed adapters. |
| M12 | Demonstrator Spine | Bounded instruments, independent readbacks and separate Well activation area. |
| M13 | Crownfall public index | Tiered accessible record infrastructure and separated witness platforms. |
| M14 | Command-crisis basin | Distinct Possible/Manifest terraces, command position and crisis-anchor setting. |
| M15 | Solar Fall accord dais | Approach anchor, three construction languages and an empty pre-ending mount. |
| SK01 | Glass Scar | Three named crossings through the central ridge; one central Well; six deposits. |
| SK02 | Crownfall Basin | Twin vertical ridges, three gates and shelf walls; north-offset Well; eight deposits. |
| SK03 | Confluence Ring | Four cardinal entrances; one central Well; eight deposits. |

## What the sheets establish

The upper scene is a proposed environmental composition. The lower panels develop individually identifiable surfaces, structural pieces, props or interface assemblies. Their object locators refer to original pixels, not model-ready cutouts. Compound panels also receive child records where they contain independent reusable designs. A montage is never treated as one anonymous asset.

The three skirmish resource and passage counts are brief requirements from SPEC-SKM-011..013. Image review checks visible interpretation, but the raster is not an exact placement contract. Tile bounds, collision, line of sight, mirrored approach times and resource timing remain in authored map data and later measured tests. Campaign compositions likewise preserve the mission sequence without inventing coordinate authority. The M01 study is an evacuation layout, not a reskin of the skirmish Glass Scar.

Existing faction architecture appears only where civic history, an interface or the story needs it. Neutral geological and civic kits do not acquire player ownership merely because the same materials occur in a faction. Environmental status markers cannot impersonate selection, damage, resource alerts or territorial power links.

## Shared environment kits and motion

| Kit | Reusable content | Proposed motion, effects and sound | Constraint before production |
|---|---|---|---|
| Glass and basalt | Cliff faces, supported shelves, route shoulders, fractured edges and floor transitions | Sparse wind dust and contact-dependent grit/stone footsteps | Quiet walkable ground; no effect-created obstacles or misleading passability. |
| Shivergrass | Low clumps, compressed margins, exposed root mats and polished migration shoulders | Low-amplitude wind bend and sparse grass friction | Never reveal hidden entities or add unmodeled cover/stealth. Distant animation must not shimmer. |
| Civic service | Ceramic paving, cistern walls, ducts, transit coffers, cassette banks and repair fittings | Restrained shutters, service indicators and separate plant/servo ambience | Indicators consume authoritative state. Keep maintenance access, scale and district identity readable. |
| Mineral infrastructure | Load-bearing ribs, rooting docks, maintained sockets and strata platforms | Small settling/root-contact motion and bounded resonance | No organic limb motion implying a new creature; rooted load paths remain credible. |
| Neutral interfaces | Witness furniture, adapter joints, index modules and calibration instruments | Anticipation, measured action, stable hold, receipt and refusal cues where gameplay requires them | Bind each cue to an existing interaction; no invented records, outcomes or timers. |
| Choir-adjacent terrain | Offset frames, paired supports and pale panes | Contained phase filaments and subtle non-walkable offsets | Solid ground does not flicker or teleport. Effects cannot change fog, collision or navigation. |
| Matter deposits | Embedded cyan-white outcrops and depleted ground fittings | Restrained resource-state presentation | Existing resource identity, quantities and depletion authority remain unchanged. |
| Future Well setting | Geological bowl surround, civic paving transition and restrained approach dressing | Reuse the existing Dormant/Harvest/Preserve/Reshape family and approved spent-state decisions | One family. No new destruction behavior. A dormant illustration does not eliminate later state references. |
| Crownfall sky and boundary | Distant stellar fragments, ash horizons, coronal rifts and bounded void edges | Slow atmospheric movement, sparse harmonics and controlled sky contrast | Cinematic sky may be dramatic; tactical units and ground remain legible. No new gameplay weather. |

These are design proposals, not implemented animation systems. Each future asset package must choose static, shader, skeletal or mechanical animation from the actual function. Pivot, attachment, collision, audio trigger, fog behavior, state inputs and reduced-effect presentation must be specified before import. Reuse [the existing motion contracts](../motion/README.md) and [Unreal reference collection](../UnrealReferences.md); this pass does not create another technical authority.

## Digital thread and production handoff

Each new ID is stable across renames. This isolated collection allocates environment compositions under EBS-CON-WRL-ENV-101 onward and detail designs under EBS-CON-WRL-PRP-101 onward, with explicit subtype and reuse relations. WRL denotes the environmental collection; regional/faction grammar is recorded separately. A civic prop in a world scene is not automatically a newly approved Meridian production asset.

Before integration, collision-check these IDs against the then-current authoritative register and reconcile any independently allocated IDs through explicit aliases. Do not silently renumber references. The [concept-object supplement](concept-object-register.json) retains source-image hashes, locators, mapping evidence, parent/child relations, canon status, production status and unknown production fields. It is a scoped addition, not a competing replacement for [the existing register](../concept-register.json).

Follow the existing [pipeline specification](../README.md): source concept → identified object → canon review → gameplay mapping → production asset identity → reference package → blockout → gameplay proxy → art alpha/beta → production → authorized Unreal integration → RTS readability → performance → acceptance.

For each selected terrain or object design, the later reference package must supply dimensions, authoritative footprint, orthographic views or cross sections where needed, material callouts, scale and pivot, sockets, construction/damage/state variants, collision and navigation disposition, team/selection interaction, animation timing and audio/VFX interfaces. Unknown values remain TBD. Supplemental generated views are derived references and must remain tied to the selected original.

Build one representative segment of each regional kit before producing an entire map. Review at tactical, close gameplay and cinematic distances. Blockout and gameplay testing precede expensive detail. Terrain art cannot move current objectives or passages merely to match the illustration.

Acceptance remains three separate gates:

- **Art:** place identity, faction/civic grammar where relevant, proportions, silhouette, support, materials, concept fidelity and world consistency.
- **Gameplay:** route and footprint truth, occlusion, unit and selection readability, interaction/state clarity, collision/navigation and branch fidelity at actual cameras.
- **Technical:** approved Unreal naming/path, scale, pivot, bounds, collision, LOD/Nanite decision, material/texture limits, animation, packaging, provenance and target-platform performance.

No gate is satisfied by the presence of an attractive concept sheet.

## Retained decisions and remaining variants

| Record | Treatment in this pass | Later action |
|---|---|---|
| MAP-DEC-001 — Branch identity | M02 and M06–M08 preserve inherited route alternatives. A representative composition does not select canon. | Bind selected route and objective coordinates before map production; derive branch-specific sheets when shape or access changes materially. |
| MAP-DEC-002 — District allocation | M09 explicitly illustrates Life Support + Transit powered and Archive deferred; M10 uses that same illustrative deferred frontage. | Retain the other two allocations and downstream combinations; never portray all three as powered. |
| MAP-DEC-003 — Ending identity | M15 shows an empty pre-commitment resolution mount. | Develop each eligible conduit from the recorded ending contract; never combine the four endings into one machine or scene. |
| MAP-DEC-004 — Well failure-state conflict | SPEC-WEL-004 calls Wells indestructible; M10–M12 failure clauses mention destruction. No combat destruction is depicted or implemented. | Reconcile the controlling requirements before authoring failure variants or implementation. This local record does not change RequirementsState. |
| MAP-DEC-005 — Format coverage | This illustrated set covers the fifteen mission locations and three named offline skirmish maps. | Conquest's 25 sectors and team/FFA map-format bindings are additional registered work, not silently satisfied by these eighteen pictures. |
| MAP-DEC-006 — Dimensions and budgets | All model dimensions, texture allocations, LOD/Nanite decisions, collision and exact placements remain unclaimed. | Set them from current approved contracts and measured representative assets in the later integration task. |
| MAP-DEC-007 — Owner review | Every new image and detail remains CANDIDATE. | Record Angelis's selection or correction against the exact image hash. Praise of earlier faction-base sheets does not approve this new set. |

The read-only [audit](audit.py) checks inventory coverage, ID uniqueness, required metadata, local links, hashes, prompt/receipt binding and the preserved candidate boundary. It does not inspect Unreal, certify pixel geometry, decide canon, or write assets. Retained generation and inspection results distinguish requested features from what each image visibly achieved.

