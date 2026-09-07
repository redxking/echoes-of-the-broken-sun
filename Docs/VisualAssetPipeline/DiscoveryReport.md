---
title: Visual source discovery and production readiness report
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: Inventory and future-pipeline delivery; production decisions remain open
---

# Visual source discovery and production readiness report

The inspected repository/site boundary contains **27 retained raster concept originals**. They are
represented by **53 distinct encoded image payloads** (originals plus WebP derivatives) across **159
tracked file locations**. The concept register identifies **162 visual objects**, including distinct scene
elements, components, alternate studies and website graphics. This is not 162 approved production assets.
No source image was edited or regenerated, and no concept was advanced into production.

The machine-readable record is [concept-register.json](concept-register.json), with the future workflow in
[README.md](README.md). Counts below refer to the snapshot described here, not an ongoing monitor.

## Discovery scope and evidence

The baseline and remote main observed during discovery were `1a60cb1fecdd5a709f940726a6a3e15b0fc378ff`.
The source checkout contained unrelated active changes. [authority-snapshot.json](authority-snapshot.json)
records hashes and dirty-state distinctions for controlling references. The isolated branch contains only
this new folder; it does not merge, push, or modify the active phase's implementation.

| Source population | Observed coverage |
|---|---|
| Tracked visual files, all names/extensions | 244 locations; 102 distinct payload hashes. No unresolved LFS pointers in the inspected primary checkout. |
| Concept imagery | 159 locations; 53 distinct payloads; 27 retained originals. Three website trees share delivery copies. |
| Public engine imagery | 54 locations; 18 distinct payloads; 10 PNG capture originals plus 8 WebP derivatives. All 10 originals visually inspected as implementation references. |
| Production texture reference | 30 PNG maps in 10 material families; visually inspected without modifying them. |
| Standalone SVG | One generic rounded-block favicon, excluded from game-concept count. |
| Embedded code graphics | 12 SVG/canvas occurrences across source/archive copies: 5 unique character crests and one radar implementation. Radar's six terrain grammars and symbols have individual object records. |
| Reachable Git history | 109 Git blobs resolving to 108 payload hashes; 6 additional historical texture payloads. Existing local LFS payloads resolved read only. No additional historical concept-art payload found. |
| Embedded archive imagery | Superseded requirements DOCX contains one blank thumbnail; no concept content observed. |
| Ignored/untracked local imagery | 878 files: 876 implementation captures and 2 byte-matched website build copies. All enumerated/hashed; captures not individually requalified. |
| Live public site | 104 recursively discovered resources fetched with HTTP 200, including linked page/query variants, CSS, scripts and images; no unvisited crawl queue. Every fetched image hash matches the retained source inventory. |
| External references | Project GitHub link and Google Fonts stylesheet only in the observed public reference graph; no external concept image discovered. |

Methods included extension-independent source-reference searches in tracked HTML/CSS/JS/TSX/Markdown/JSON,
image-directory enumeration, archive-member inspection, reachable Git-object inspection, hash comparison,
review of canon/art/map/catalog/provenance documents, and recursive HTTPS crawling of the
[public site](https://redxking.github.io/echoes-of-the-broken-sun/).
Every retained raster concept original was visually inspected; detailed roster/sheet identification also
used the retained prompts and caption evidence. All five crests and six radar terrain modes were rendered
and visually inspected. This does not certify their typography, gameplay accuracy or accessibility.

The discovery limit is explicit: the crawler covers reachable same-site references, not unknown orphan
remote URLs. The local walk excludes named engine/dependency caches and does not follow directory
symlinks. It does not search unrelated personal storage or claim complete visual review of the 876
ignored captures. The inventory is complete for the stated tracked/reachable-source scan; semantic object
identification is a reviewed inventory, not an exhaustive segmentation of every incidental background
silhouette. Unidentified figures remain UNKNOWN and their whole source scenes are preserved.

## Retained originals and decomposition

Counts include scene composition records, meaningful constituent designs, alternate studies, and
components; repeated instances of a design in a sheet are not new production assets. WebP and mirror
copies do not multiply concept counts. Every row has at least one concept-object reference.

| Retained original under site/ | Object records |
|---|---:|
| `assets/concepts/arkcity-census-void.jpg` | 5 |
| `assets/concepts/broken-sun-solar-dais.jpg` | 4 |
| `assets/concepts/echoes-campaign-map.jpg` | 4 |
| `assets/concepts/echoes-choir-structures.jpg` | 4 |
| `assets/concepts/echoes-combat-vfx-grammar.jpg` | 9 |
| `assets/concepts/echoes-faction-selection.jpg` | 8 |
| `assets/concepts/echoes-future-well-landmark.jpg` | 4 |
| `assets/concepts/echoes-hud-modern-arc.jpg` | 6 |
| `assets/concepts/echoes-hud-traditional-console.jpg` | 4 |
| `assets/concepts/echoes-kharuun-structures.jpg` | 4 |
| `assets/concepts/echoes-main-menu-bridge.jpg` | 5 |
| `assets/concepts/echoes-main-menu-traditional.jpg` | 3 |
| `assets/concepts/echoes-meridian-structures.jpg` | 4 |
| `assets/concepts/echoes-pause-field-ledger.jpg` | 4 |
| `assets/concepts/echoes-post-match-debriefing.jpg` | 7 |
| `assets/concepts/echoes-settings-accessibility.jpg` | 4 |
| `assets/concepts/future-well-states.png` | 4 |
| `assets/concepts/glass-scar-routes.png` | 4 |
| `assets/concepts/kharuun-structures.png` | 4 |
| `assets/concepts/kharuun-units.png` | 8 |
| `assets/concepts/meridian-structures.png` | 4 |
| `assets/concepts/meridian-units.png` | 8 |
| `assets/concepts/shivergrass-basin.jpg` | 5 |
| `assets/concepts/soryn-world-map.jpg` | 8 |
| `assets/concepts/target-render-vertical-slice.jpg` | 11 |
| `assets/concepts/unburied-road-caverns.jpg` | 6 |
| `hero-soryn.png` | 7 |

The remaining 14 objects derive from the five inline crests and radar implementation. Nine text-only
specifications (five characters, four Choir units) are kept outside these image-derived object counts.
The Future Well has one family record and four state objects, plus linked alternate visual studies;
the unidentified hero landmark is not included in the family.

## Coverage and gaps

| Domain | Objects |
|---|---:|
| MER | 30 |
| KHA | 26 |
| HOL | 10 |
| FWL | 7 |
| GLS | 8 |
| WRL | 30 |
| UIX | 47 |
| CHR | 4 |

| Object type | Objects |
|---|---:|
| UNT | 20 |
| BLD | 20 |
| PRP | 21 |
| SYS | 7 |
| ENV | 27 |
| VFX | 11 |
| UI | 51 |
| CHR | 4 |
| VEH | 1 |

Canon status: **145 CANDIDATE, 11 UNKNOWN, 6 EXPLORATORY, 0 CANON_APPROVED, 0 SUPERSEDED**.
Production maturity: **162 NOT_STARTED**. All production IDs remain unallocated. Readability and
performance are NOT_EVALUATED. Nothing in this report grants owner acceptance or release rights.

**52 objects have supported gameplay-entity or functional-system context; 110 are unmapped.** This includes
alternative depictions of the same entity and functional UI contexts; it is not 52 unique gameplay
entities. Source evidence supports artwork for 8 of the 12 current unit identities and all 12 current
building identities. The four Choir units have textual specifications only. The supported mappings do
not approve the pictured geometry, weapon, menu layout, metrics or dimensions.

Major remaining visual/reference gaps are dedicated Choir unit sheets; dedicated character modeling
references beyond Mara's menu portrayal; front/rear/side/top, construction, damage and animation packages;
per-asset material/performance budgets; detailed Lume Reach and mission-specific environmental reference
packages; and explicit selection of variants where the catalog and sheets disagree. The current 15-map
campaign is not visually covered merely because a six-grammar radar or six broad environment studies
label all missions. Established neutral/other-world elements such as the illustrated Vaultbacks, convoy
insignia, Matter, Broken Sun and geographic landmarks are retained; unidentified figures are not invented
as additional factions or civilizations.

## Placeholder and replacement assessment

The owner clarified during this work that some existing images are placeholders awaiting better concepts.
[visual-use-assessment.json](visual-use-assessment.json) records a separate assessment for all 27 raster
originals and ten public capture originals. No current concept is implicitly selected for production.
This judgment is independent from canon status and maturity.

| Internal assessment of raster originals | Count | Treatment |
|---|---:|---|
| Placeholder replacement candidates | 8 | Weak UI/map mockups: preserve history and useful information categories; develop a better original concept before use. |
| Mixed references requiring rework | 7 | Useful elements coexist with scale, silhouette, clutter, identity or state problems; retain those elements and revise the design. |
| Atmosphere references only | 4 | Environment mood and composition remain useful; they are not modeling or gameplay-geometry specifications. |
| Retain for design review | 8 | Stronger separated object/state/route references and world targets; still unapproved and incomplete for production. |

The weakest studies include the traditional console/menu, faction selection, campaign map, pause,
debrief, settings and continental-map mockups. They carry invented labels or metrics, cramped text,
derivative framing, or unsupported geographic detail. The annotated structure sheets and dramatic
Well spiral need substantial review despite their polish. The original isolated roster/building sheets,
four-state Well and route sheet generally preserve clearer design information. This is an internal
art-direction judgment, not an invented per-image owner approval or rejection.

All ten public engine captures are `IMPLEMENTATION_PLACEHOLDER_REFERENCE`: crude generated forms,
fixture backgrounds and debug overlays are implementation history, not desired final-art targets.
No image is deleted, replaced or regenerated by this classification. DEC-013 requires explicit selection
and replacement prioritization before later production. Useful sub-elements of weak images remain tracked.

## Decisions before production

These OPEN decisions are local handoff records because the authoritative state file is actively owned
by the gameplay workstream. They create no normative requirement change. The concept register refers to
the relevant decision IDs; DEC-010..012 also apply at workstream level.

### DEC-001 — Visual canon approval

No explicit approval of these visual designs was found in the reviewed source records. Entity existence and a public visual target are insufficient.

Owner: approve, retain candidate, classify exploratory, or explicitly supersede each visual variant. Record exact concept ID, source hash and decision date.

Evidence: Docs/Archive/AssetRegister.md:55–80; Docs/README.md authority map.

### DEC-002 — Production provenance and rights

CONCEPT-001..003 retain prompts/service but not the account-plan evidence; CONCEPT-004/005 explicitly lack service, prompt, inputs and terms. Later generation/specification rows do not establish complete production-use rights.

Owner/provenance custodian: complete intended-use records in the existing AssetRegister before production use. Preserve source credits and source files.

Evidence: Docs/Archive/AssetRegister.md:283–459.

### DEC-003 — Reference completeness and production scope

No complete per-object orthographic/dimension/socket/budget package was discovered for this thread. All production IDs are unallocated and maturity remains NOT_STARTED.

Future production owner: authorize a bounded asset package, resolve shape/scale, and prepare reviewed references using the pipeline specification.

Evidence: README.md reference-package specification.

### DEC-004 — Building dimensions conflict

ComponentDesignCatalog gives Anchor 6x6, Foundry 8x6, Memory Hearth 7x7 and very large Kharuun heights; current source/master detail retains Anchor/Hearth 5x5 and production buildings 4x4. Other structure footprints also differ.

Owner/design authority: resolve the dimensional brief; do not bake image or catalog dimensions into models. Registry stores observed gameplay footprints separately from TBD production dimensions.

Evidence: Docs/ComponentDesignCatalog.md:23–99; Content/Data/Source/buildings.json; Docs/Requirements.md SPEC-STR-001..012.

### DEC-005 — Unit silhouette and roster conflicts

The original Surveyor/Lancer/Bulwark sheet depicts biped/operator/platform forms. ComponentDesignCatalog describes tracked Surveyor, wheeled Lancer, tank Bulwark and names Atlas Mech, Shardbearer and alternate Choir units. Those labels are not the established source roster.

Owner: choose approved form for each existing entity; treat unsupported designs as candidates, not roster additions or authorization to alter gameplay.

Evidence: Docs/ComponentDesignCatalog.md:114–131; Docs/Archive/AssetRegister.md:287–341; Content/Data/Source/units.json.

### DEC-006 — Future Well variants and state continuity

The four-state sheet, composed-target rings and celestial spiral are different studies. The hero landscape central light is unidentified and is not assigned to the Well family.

Owner: select common geometry/state grammar and explicit variant relationships. Preserve all alternatives. Review collapse/temporary-crossing transitions against the current Well contract.

Evidence: Docs/Archive/AssetRegister.md:343–364,428; Docs/Requirements.md SPEC-WEL-004 and SPEC-WELLP-001..003.

### DEC-007 — Geographic and mission interpretation

Map/landscape concepts and website radar contain geographic names, mission labels, routes and structures. These are not authoritative passability, current map topology or proof of the depicted historical event.

Owner/map designer: bind each approved environment element to current mission/canon source and retain uncertain scene details without inventing geography. Lume Reach has map-symbol coverage, not a dedicated detailed environment image.

Evidence: Docs/MapConcepts.md; Docs/MapTechnicalBlueprint.md; Docs/Archive/DevelopmentBible.md; site/atlas.html.

### DEC-008 — UI labels, identity and numeric conflicts

Traditional UI sheets are comparative studies. Concept images contain unsupported labels such as Atlas Mech and Lyra, and a -24 LKFS calibration display. Catalog UI requirement pointers do not consistently match the master: SPEC-UI-002 is Selection fields, SPEC-UI-006 is Remapping; do not adopt caption IDs as acceptance.

Owner/UI designer: choose original UI direction, reconcile labels/metrics against current requirements, and identify or reject unnamed portrait/unit portrayals. No franchise layout or unsupported ability is adopted.

Evidence: Docs/Archive/AssetRegister.md:423–436; Docs/Requirements.md SPEC-UI-002/006, SPEC-ACC-002/003; Docs/VisualTargetSpecification.md.

### DEC-009 — VFX capability versus visual grammar

Weapon/shield/phase illustrations suggest effects, not additional authoritative powers, collision, damage or world-state changes. The catalog SPEC-VFX-001 caption does not establish a current master binding.

Future VFX owner: bind each accepted effect to an existing event and state, identify sockets/timing/accessibility variants, and measure cost under current visual requirements.

Evidence: Docs/ArtDirection.md Effects grammar; Docs/Requirements.md SPEC-VISD-007 and SPEC-ART-002.

### DEC-010 — Missing dedicated character and Choir unit images

CONCEPT-025 contains five character design specifications and crests; CONCEPT-026 contains four Choir unit specifications. Neither is a retained raster sheet. Mara has a bridge-menu portrayal; crests do not supply anatomy or modeling views.

Owner: approve future missing-art/reference work when the plan permits. Do not generate replacements for existing concepts.

Evidence: Docs/Archive/AssetRegister.md:437–438; site/characters.html; website/app/page.tsx.

### DEC-011 — Existing production asset crosswalk

145 tracked Content/Art .uasset paths have existing AssetRegister provenance but no newly allocated production IDs in this thread. Ten public capture originals are linked only as comparison evidence.

Future integration owner: reconcile existing registry entries and source/generator paths to approved concept and production IDs without renaming or regenerating current assets.

Evidence: implementation-links.json; Docs/Archive/AssetRegister.md ART/CAPTURE entries.

### DEC-012 — Remaining evidence boundaries

876 local development captures are enumerated and hashed, not individually visually requalified. Engine caches/dependencies, directory symlink targets, unavailable remote branches and non-repository personal art stores are outside the scan.

If broader archival review is needed, authorize and scope that specific source set. Never use these capture counts as proof of runtime or art acceptance.

Evidence: local-supplement.json; sources.json scope.

### DEC-013 — Placeholder versus intended final design

Owner clarification: some retained images are placeholders for better concepts. All visual objects remain
UNSELECTED. Use the per-source assessment to prepare later selection/replacement decisions, retain useful
sub-elements and preserve every original. No replacement artwork is authorized in this workstream.

## Verification and non-interference

The scoped inventory tests passed **27/27**, and links/authorship/JSON/whitespace checks passed for the
new deliverables. The project-wide `check_agent_docs.py` found **21 pre-existing evidence links missing
from the isolated checkout**; no new-folder link error was reported. These shared historical links were
left unchanged under the non-interference boundary.

The retained [validation.json](validation.json) reports schema/semantic/source checks and exact counts.
The companion negative tests exercise missing metadata, duplicate IDs, bad source references/hashes,
missing locators, invalid crop bounds, unsupported approval/mapping claims, absent dependencies,
domain mismatch and source drift. These are inventory integrity checks, not Unreal tests.
[implementation-links.json](implementation-links.json) links ten public captures to concept context and
lists 145 existing art asset paths for a future crosswalk. It explicitly retains the existing AssetRegister
as their provenance authority instead of claiming those assets have no provenance.

Visual-review artifacts are retained at:
`/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/`.
They include concept contact sheets, engine/material/historical-reference contacts, the character page,
and six radar renders. These are derived QA artifacts; the original images and this single authoritative
report remain in their existing locations. This evidence directory is local and is not made remotely
backed up by the documentation commits. [verification-receipt.json](verification-receipt.json) retains
artifact hashes, executed checks and the actual evidence boundary.

Deliberately unchanged: Content, Source, Config, project files, maps, Blueprints, meshes, textures,
materials, Niagara/VFX, gameplay data, generators, build scripts, workflows/CI, active delivery/state/art
register documents, main's staged work and all current implementation artifacts. No Unreal build,
launch, import, regeneration, package, production transition, owner acceptance, merge or push occurred.
