---
title: Concept-to-Unreal visual asset pipeline
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: Future production preparation; no integration authorization
---

# Concept-to-Unreal visual asset pipeline

This workstream preserves existing visual designs and makes their future production traceable. It does
not approve artwork, commission replacement art, change the game plan, or authorize Unreal integration.
The authoritative object inventory is [concept-register.json](concept-register.json). Edit that file in
place; do not regenerate it from filenames or replace it with a new numbered inventory.

[DiscoveryReport.md](DiscoveryReport.md) records coverage, evidence limits, conflicts, and owner decisions.
[sources.json](sources.json) inventories retained image representations, provenance identities, code graphics,
archive members, references, and locally reachable history. [site-discovery.json](site-discovery.json)
records the live crawl. [local-supplement.json](local-supplement.json) records ignored/untracked visual files.
[validation.json](validation.json) is a derived audit result, not an acceptance decision.

## Authority and isolation

The shared [agent contract](../../AGENTS.md) and [authority map](../README.md) still control.
[Requirements.md](../Requirements.md) owns behavior and numeric gates;
[RequirementsState.md](../RequirementsState.md) owns requirement lifecycle and owner acceptance;
[DevelopmentBible.md](../Archive/DevelopmentBible.md) owns creative canon.
[AssetRegister.md](../Archive/AssetRegister.md) remains the production provenance/rights record.
This folder adds concept-object identities and a future workflow; it does not replace any of those sources.
The novel, promotional captions, annotated images, and website illustrations cannot override them.

The initial work was isolated on `docs/concept-production-pipeline`, based on `1a60cb1`, in
`Worktrees/concept-production-pipeline`. The active main checkout was read only. Its runtime, Content,
configuration, generated data, scripts, CI, staged index, and shared documentation were not edited by this
workstream. Changes here are confined to this new documentation/data/tooling folder. Integration of these
commits into main is a later coordinated action; no push or merge is implied.

The existing AssetRegister and Requirements files had active edits. Conflicts found by this inventory are
therefore recorded locally as `DEC-*` proposals, not inserted into the active requirement-state file.
On future integration, the current owner can resolve them in the existing authoritative state record and
add a pointer from Docs/README without creating a competing decision authority.

## Identity and counting

| Record | Identity and rule |
|---|---|
| Concept object | `EBS-CON-<DOMAIN>-<TYPE>-NNN`; append-only allocation. Rename the label without changing ID. |
| Production asset | `EBS-<DOMAIN>-<TYPE>-NNN`; allocate only after a future approved gameplay mapping and scope. It is not automatically the concept's number. |
| Stateful family | `EBS-FAM-FWL-001` binds the Future Well studies and four explicit states. Family membership does not approve every variant. |
| Source payload | `EBS-SRC-<SHA256-prefix>` identifies bytes; different encodings have different identities. Exact copies share identity and retain every location. |
| Code graphic | `EBS-GFX-<SHA256-prefix>` identifies SVG/canvas markup; the containing source-file hash also binds executable drawing code. |
| Text-only specification | `EBS-SPC-<DOMAIN>-NNN`; tracked outside image-derived object counts until visual material exists. |

Domains are MER, KHA, HOL, WRL, FWL, GLS, CHR, UIX. Types are BLD, UNT, VEH, ENV, PRP,
SYS, CHR, VFX, UI. Domain is the design's subject, not necessarily a playable faction. CHR separates
characters from units. A component such as a rifle or shield has its own record when individually
identifiable, but this does not require a separate mesh. Repeated instances of the same design within one
image count once. A second visual design for the same game entity keeps its own concept ID and mapping.
A scene composition and its reusable elements are explicitly separate records; object count is not a
count of unique production deliverables. Public engine captures and textures remain reference evidence,
not replacement concepts. Blank document thumbnails and generic framework graphics are excluded from
concept counts with a disposition retained in the report.

Every source reference includes the path, filename, revision, hash, URL where relevant, and an object
locator. Coordinates use normalized `[left, top, right, bottom]`, origin at the image's top left. Regions
are approximate inventory locators, not extraction masks. Before production, approve an exact crop in
original pixel coordinates and ensure the entire silhouette is retained. SVG locators identify the
opening element and line; canvas locators also identify the draw function/branch.

When a source changes, retain its old revision and the same concept ID; append a new source reference and
state the relationship. Never use a rescan to renumber objects. Exact-byte duplicates are mechanically
detectable; visually similar or superseded designs require comparison and an explicit disposition.

## Visual-use intent and two independent state axes

Inventory inclusion is not selection for use. [visual-use-assessment.json](visual-use-assessment.json)
separately identifies placeholder/replacement candidates, mixed studies, atmosphere references and stronger
references to retain for review. All initial objects are `visual_use_status: UNSELECTED`. Internal quality
assessment is not owner approval. Weak sources retain their IDs and useful sub-elements; replace or
redesign them only in a later authorized task. An implementation screenshot is reference evidence even
when its pictured art is a placeholder.

Canon status is `CANON_APPROVED`, `CANDIDATE`, `EXPLORATORY`, `SUPERSEDED`, or `UNKNOWN`.
`CANON_APPROVED` needs an explicit owner decision referring to the visual design, not just proof that the
named unit exists in canon. `SUPERSEDED` needs a successor and decision; a newer filename is insufficient.
`EXPLORATORY` retains comparative work. `UNKNOWN` means identity or standing cannot be established.

Production maturity is `NOT_STARTED`, `REFERENCE_READY`, `BLOCKOUT`, `GAMEPLAY_PROXY`, `ART_ALPHA`,
`ART_BETA`, `PRODUCTION`, `UE_INTEGRATED`, or `VERIFIED`. All initial concepts are `NOT_STARTED` for this
future production thread. Existing procedural assets and editor captures do not establish that a particular
concept has passed through this thread. `REFERENCE_READY` requires the reviewed package below, rather
than merely a beautiful image. `CANON_APPROVED / NOT_STARTED` is valid.

The register holds `null` for unallocated production IDs and unconfirmed canonical names, `TBD` for
undecided production values, and `NOT_EVALUATED` for readability/performance. These are distinct from
zero, not applicable, rejection, and acceptance. Record an explicit reason when a future field is N/A.

## Digital thread and stage exits

SOURCE CONCEPT → CONCEPT OBJECT → CANON REVIEW → GAMEPLAY MAPPING → PRODUCTION ASSET ID →
REFERENCE PACKAGE → BLOCKOUT → GAMEPLAY PROXY → ART ALPHA → ART BETA → PRODUCTION →
UNREAL INTEGRATION → RTS READABILITY → PERFORMANCE VALIDATION → ACCEPTANCE.

| Stage | Evidence required to leave it in a future authorized task |
|---|---|
| Source / object | Immutable source identity, all identifiable designs separated, original retained, locator checked, provenance gaps identified. |
| Canon review | Owner decision on this visual design and variant relationship, with authority references; no unresolved canon conflict. |
| Gameplay mapping | Exact entity/system and requirements, role, dimensions/footprint, allowed states, interfaces, map context, explicit non-gameplay parts. |
| Production ID | Stable allocation and target owner; existing asset registry cross-link; no naming collision. Multiple concepts can inform one approved asset. |
| Reference package | Complete and internally consistent reference manifest, dimensions, views and interfaces; rights disposition cleared for the intended production use. |
| Blockout | Source-led geometry and silhouette/footprint comparison at gameplay camera. No detailed art before role and scale are established. |
| Gameplay proxy | Authorized integration task proves selection, traces, navigation, construction/production, state and attachment interfaces with proxy art. |
| Art alpha | Primary proportions, materials, rig and key states faithfully match the approved source; intentional deviations recorded. |
| Art beta | Complete applicable animation/state/destruction set, team/accessibility variants, material/texture/LOD treatment; defects triaged. |
| Production | Art and source-package review complete, required budgets met in the intended asset context, reproducible import/generation recipe retained. |
| Unreal integration | Registered source/generator revision, import options, dependencies, runtime binding, cook/package results and rollback evidence. |
| RTS readability | Representative combat/selection/state evidence at approved camera distances and accessibility settings; Art and Gameplay Gate records. |
| Performance validation | Target platform/configuration, roster density, frame and memory measurements, shader/material cost and streaming behavior; Technical Gate record. |
| Acceptance | All three gates and required human/owner evidence tied to the exact asset, source revision and package. Only Angelis accepts. |

These are workflow exit checks, subordinate to existing requirements; they do not establish new numeric
budgets. A later plan must authorize production, integration, builds, and use of shared editor/GPU resources.
The same file is revised in place throughout; Git retains history. Derived renders and reports are QA
outputs, never new document authorities.

## Reference-package specification

Keep an approved package manifest keyed by production ID and linked concept IDs. Retain original source
paths and hashes instead of silently copying a delivery WebP over the retained PNG/JPEG. The manifest must
identify who reviewed each item, when, and the exact source revision.

| Package field | Required treatment |
|---|---|
| Original and object locator | Approved original, exact pixel crop, unmodified source hash, object/variant IDs and visual comparison. |
| Views | Front/side/rear/top and underside/detail where needed to resolve geometry. Record missing views as blockers, not guessed surfaces. |
| Derived references | Each orthographic or AI-assisted view labeled DERIVED_REFERENCE with parent source, method, inputs, date, terms and review. Original remains the design authority. |
| Context and dimensions | Story/mission/faction, role, height/width/depth in cm, gameplay footprint and exclusion zones; cite current master/source values and resolve contradictions. |
| Silhouette | Role-defining masses, negative space, proportions, view-distance limits and prohibited changes. |
| Materials | Material zones and PBR intent, roughness/metallic/emissive treatment, texel density and approved budgets; no arbitrary values copied from image labels. |
| Motion | Moving components, rig strategy, rotation limits, anticipation/action/recovery, locomotion/turn/work and interruption behavior. |
| Interfaces | Pivot, forward/up orientation, sockets, muzzle/effect/audio/selection/health attachments, spawn/rally exits and selection bounds. |
| States | Construction, operational/offline, production, damaged/destruction, rooted/mobile, faction adaptations, Well protocols and transitions as applicable. |
| Collision/navigation | Approved collision primitives, query/selection behavior, gameplay footprint source, navigation restrictions and cosmetic exclusions. |
| Team/readability | Shape-first faction/role/ownership, selection, damage, weapon and state recognition; color-vision/high-contrast/reduced-motion/flashing variants. |
| Unreal target | Approved asset name/path, LOD/Nanite strategy, materials/textures, dependencies, source/import recipe and rollback reference. |
| Provenance | Rights holder/creator where evidenced, service/license/terms/consent, generation inputs and revision, restrictions, approval for intended use. |

Future Well state records share one identity and common geometry/interfaces. Preserve Dormant, Harvest,
Preserve and Reshape references and design their transitions together. Harvest's permanent collapse and
Reshape's temporary crossing must follow the current simulation contracts (`SPEC-WEL-004`,
`SPEC-WELLP-001..003`). A luminous generic landmark is not automatically another approved Well variant.

## Three acceptance gates

| Gate | Review content | Evidence and acceptance boundary |
|---|---|---|
| Art | Faction grammar, proportions, silhouette, material language, source fidelity, contextual detail and world consistency. | Side-by-side source/asset comparison, multiple views, tactical framing, documented deviations; owner review remains required. |
| Gameplay | Faction/role/team/weapon recognition; footprint and selection clarity; collision/navigation; construction, damage and Well-state communication; occlusion and hierarchy. | Representative gameplay distances and combat loads, grayscale/high contrast, selection and public-state transitions, applicable human recognition checks. `SPEC-ART-001` requires recognition within one second for its listed fields. A still image cannot demonstrate the whole gate. |
| Technical | UE name/path, scale/pivot/orientation, collision, LOD/Nanite, materials/textures, rig/animation, packaging, platform performance, dependencies and provenance. | Source hashes, generator/import revision, validators, actual target-platform measurements, package identity and rights record. Editor success is not packaged verification. |

Apply `SPEC-ART-001..004`, `SPEC-VISD-001..008`, applicable accessibility and platform-performance
requirements, [ArtDirection](../ArtDirection.md), and the relevant mission/roster contracts. Test near,
representative and far tactical views at the currently approved camera settings. Include crowded battles,
terrain occlusion, faction matchups, selected/unselected states, damaged/construction silhouettes and all
public Well states. Record camera parameters, resolution, unit density and accessibility mode. Cinematic
close-ups supplement this evidence; they cannot pass an RTS gate.

A rejected gate retains its evidence and defect list. Do not erase the original, change a threshold, or
mark maturity VERIFIED to conceal a failure. A passing structural validator proves only its named checks.

## Future Unreal import and integration specification

This is a proposed handoff contract for a later authorized task. Use the existing registered source and
generation paths; [GameCompletionDirective §7](../GameCompletionDirective.md#7-asset-pipeline-rules)
requires extending the existing generators, not building a competing runtime/import pipeline.

1. Confirm current checkout, plan package, source authority and exclusive editor/GPU ownership. Resolve
   every blocking decision and acquire intended-use provenance before production import.
2. Allocate the production ID and map it to the existing entity ID. Keep display names independent of
   both identities. Proposed UE names use existing prefixes such as `SM_`, `SK_`, `M_`, `MI_`, `T_` and
   underscore-safe domain/type/serial/part/state tokens, for example `SM_MER_BLD_001_Main`. This example
   creates no asset and renames none. Adopt exact names and `/Game/Art/...` destination only in that task.
3. Record DCC/source format, source scale in centimeters, axis conversion, pivot, baked transforms,
   materials, UVs, sockets, collision and animation options. Validate against current generator conventions;
   do not infer dimensions from perspective artwork. Future imports must reproduce these choices.
4. Retain authored LOD0/LOD1, UV0/material zones and the applicable UV1/collision contract. Current art
   direction keeps Nanite and Virtual Shadow Maps off for the M1 Pro baseline. Any different strategy needs
   measured target-platform evidence and an explicit adopted exception; do not turn them on automatically.
5. Keep cosmetic collision, overlap and navigation influence disabled under the existing contract. Bind
   gameplay footprint, selection queries, entity states, fog knowledge and replay presentation through
   current authorized interfaces. Do not add a second gameplay authority in animation or Blueprint code.
6. Bind construction/work/weapon/socket/state/audio/VFX events to authoritative events and inspect their
   timing, interruption, save/replay and accessibility behavior. Niagara use follows the existing exception
   policy; no effect may grant hidden information or change gameplay geometry accidentally.
7. Run appropriate import/source validation, reference-resolution and packaged-target checks. Retain
   source/generator/import hashes, asset dependency manifest, object/production IDs, measured budgets,
   screenshots, runtime evidence and package identity. Reimport under an unchanged declared recipe must
   meet the existing reproducibility contract or carry a documented limitation.
8. Restore a failed candidate through the controlled source revision and registered pipeline. Never repair
   generated assets by hand, overwrite another owner's checkout, or clean shared evidence as rollback.

## Read-only audit tooling

Run from this worktree with an explicit source checkout. The tool writes JSON to stdout only and does not
invoke Unreal, asset generators, Git mutations, installation, or production writes. Save derived audit
output only to this folder or a designated evidence directory. Python 3 is required; Pillow is optional
for dimensions, and its absence leaves dimensions unknown. Missing LFS payloads are reported explicitly.

```sh
python3 Docs/VisualAssetPipeline/audit.py discover --root /path/to/Project --history
python3 Docs/VisualAssetPipeline/audit.py crawl
python3 Docs/VisualAssetPipeline/audit.py supplement --root /path/to/Project
python3 Docs/VisualAssetPipeline/audit.py compare --before /path/to/old-scan.json --after /path/to/new-scan.json
python3 Docs/VisualAssetPipeline/audit.py validate --register Docs/VisualAssetPipeline/concept-register.json --sources Docs/VisualAssetPipeline/sources.json --root /path/to/Project
python3 Docs/VisualAssetPipeline/test_audit.py
```

Discovery examines all tracked extensions and text references, archive-contained visual members, SVG and
canvas markup, and optionally reachable historical Git objects/local LFS payloads. The live crawl follows
same-site references with bounded concurrency and records external links and unvisited resources. It does
not claim to crawl unknown orphan remote paths or runtime URLs absent from source. The supplement records
ignored captures and named dependency/cache exclusions.

Validation checks required fields, ID uniqueness and naming, enum values, source ID/path/hash bindings,
locators and bounds, dependencies, approval evidence, mapping evidence, and source drift. It reports
unmapped objects, concept sources without objects, production entries lacking provenance, and the decision
backlog. Exact duplicates/derivatives are represented in the source inventory. Semantic similarity,
canon approval, rights clearance, RTS readability and performance remain review tasks. Before adopting a
fresh discovery snapshot, compare paths/hashes and investigate new images; never overwrite the register
from the scan. No automation in this folder modifies production assets.


The schema validator implements only the keywords used in the retained schema and rejects unsupported
keywords. It is not a general-purpose JSON Schema engine. Evidence checks bind paths, hashes, locators,
entity IDs, decision references, family membership and production links. Owner canon/selection/supersession
evidence must reference a structured JSON decision containing decision ID, concept ID, exact source hash,
disposition, reviewer and date. This verifies the recorded binding, not the authenticity of a human
approval. Only an actual owner instruction can create that record. Maturity advancement requires a
matching stage receipt and allocated production record. Dirty source bytes are explicitly distinguished
from the HEAD baseline; unsmudged LFS pointers are identified without treating pointer bytes as artwork.

## Visual direction review board

Run from the isolated concept worktree, pointing at the checkout with retained original image bytes:

```sh
python3 Docs/VisualAssetPipeline/review_board.py --source-root '/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project'
```

Open <http://127.0.0.1:8846>. The first review covers 34 objects in 21 subjects: eight original
unit designs, twenty building studies, and six linked Future Well views. Approximate object regions
are shown alongside access to the full original. This is the priority review subset, not the entire
162-object register. Hollow Choir units lack dedicated retained concept sheets.

Keep retains a visual direction for further development; Rework preserves specified elements;
Replace flags a placeholder needing a better concept. Suggestions are assessment only. No choices
are preselected. Choose a disposition or press Save notes to persist edits. Family notes apply to
the Future Well as a whole. Navigation retains pending edits; reload requires saved edits.

The server reads artwork and writes only `review-selections.json`, a separate draft review record
with exact source hashes, timestamps and revision history. It does not modify the concept register,
canon, production maturity or Unreal assets. Export downloads a snapshot of saved review input.
The board rejects stale writes and register snapshot changes; reconcile existing decisions explicitly
before using a changed register. Run only one server per review record. The local session is not an
authenticated approval system. The server verifies source image hashes and accepts writes only from
its local browser session. Stop it with Ctrl-C when finished.

Run `python3 Docs/VisualAssetPipeline/test_review_board.py` for persistence checks. Browser interaction
checks use a separate synthetic record under the evidence directory, never the owner's live record.

The owner delegated the first complete board selection to a book-based review on 2026-09-06.
`book-based-review.json` retains the attached novel's SHA-256 identity, paragraph excerpts, individual
reasons, game-authority references and unresolved conflicts. The resulting 34 choices are assistant
judgments made under that delegation; they are not individual human approvals. Earlier owner choices,
notes and history remain in `review-selections.json`. This review selects 5 Keep, 23 Rework and 6 Replace;
Keep retains design identity for later development, not every painted detail or printed dimension.
