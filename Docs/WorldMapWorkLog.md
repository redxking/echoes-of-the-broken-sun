# World and presentation work — continuation record

**Author and owner:** Angelis Pseftis\
**Updated:** 2026-09-04\
**Status:** Partially complete; operational handoff, not requirements or acceptance authority.

The owner directs autonomous implementation and quality decisions across maps, graphics,
images, effects and audio. Do not stop for intermediate owner review. Deliver the finished
experience for review only after internal qualification; never label internal QA human acceptance.
Preserve this objective across context windows and update this file in place.

## Current resumption point — 2026-09-04

**Active production checkpoint:** M01 has 306 blocked cells in all three doctrines and 30
landmark records (including the shared archive apron and loading face); all three canyon crossings and required sites remain. World kits v9,
Meridian armor v4 and occupied evacuation props v6 are generated. Exposure correction
reduced opening mean luma 88.4 to 63.2; this does not establish full SDR qualification.

Continuous M01 cliffs replace isolated tile rocks. The presentation adapter uses only known
blocked cells, omits chasm/solid-landmark/passable cells, and rebuilds affected 8×8 sections.
No collision, overlaps, navigation or shadows are added; simulation-core source is unchanged.
The first version's 85-test suite passed after an older visibility test was updated to inspect
both actual geometry paths. V2 adds mask-scoped inward fractures, shared relief and a cliff
MID; its build and 85/85 Unreal suite passed with zero reported warnings/errors and sampled
player SaveGames unchanged (`build-continuous-cliffs-v2.log`,
`unreal-continuous-cliffs-v2.log`, `continuous-cliffs-v2-automation.json`).

Opening, archive and Well captures in `m01-continuous-cliffs-v2-captures/` were inspected.
They remain below the concept bar: shadow faces lose almost all detail, crowns still read
as platforms, and taller walls obscure loading fittings. V3 work now lowers weathered rims
and creates a separate original basalt shader. V3 built in 28.14 seconds and passed 85/85 Unreal tests with zero reported warnings/errors
and unchanged sampled player saves (`build-continuous-cliffs-v3.log`,
`unreal-continuous-cliffs-v3.log`, `continuous-cliffs-v3-automation.json`). All three v3
captures were inspected: shoulders and fittings read better, but the first dedicated shader
had artificial contour bands. `cliff-surface-3d-basalt-v2` now uses quieter narrow bedding
seams and has generated successfully; its rendered comparison is pending.

The first material-normal diagnostic failed because the console view name was wrong and
retained an ordinary lit frame. `m01-cliff-normal-diagnostic-retry/` uses the installed engine's
`VisualizeBuffer` mode; its rendered WorldNormal frame confirms outward wall normals and
actual crown relief. It diagnoses the v2 binary while v3 source edits are pending; the capture
retains the binary hash. The capture tool now refuses a buffer capture without the engine's
successful mode marker. World-XY ground textures stretch on cliff walls; replace that shader
use rather than repeatedly increasing the whole scene exposure.

M01's 28 canonical voice lines were generated as private candidates. Exact installed Kokoro
model and voices hashes were matched against streamed official release bytes, with primary
license/voice documents retained in `kokoro-provenance/`. V1 signal inspection found two PCM
full-scale samples in `nar_m01_line_mara_004`; retain v1 as failed headroom evidence. The driver
now applies peak protection before PCM conversion, preserves partial failed output, and v2
synthesis completed: 28 files, 109.95 seconds, 48 kHz mono PCM24, zero full-scale samples;
measured sample peaks range from -6.64 to -2.61 dBFS. The manifest and `signal-check.json`
retain identities and limits. This is not true-peak or loudness qualification. Candidates are
not runtime-bound, registered final audio, or listening qualification. No owner listening claim is made.

The archive loading apron is now authored under `m01-evacuation-props-v7`. It occupies
open cells x18–22,y16–19 with pivot (0,-1) half-tiles from anchor (20,18); all three doctrines
were checked. The compiler emits full footprints and preserves single-cell records. Four
old overlapping paving records were removed. Runtime requires every footprint cell to be
known and compatible and suppresses underlying ground patches. The apron stays below 4 cm;
registration fields/load rails retain the same positions across LODs. Its first import
failed the LOD1 section check; the corrected recipe passed (`evacuation-v7-engine-retry.log`).
Six compiler tests and three geometry tests pass. The apron runtime built in 29.79 seconds (`build-m01-apron-v1.log`). Opening and archive
were captured/inspected in `m01-apron-v1-captures/`; measured mean luma is 63.9 and 68.1.
The apron establishes the working surface, but cargo fields need darker material slots.
Expanded fog/pivot/terrain tests are pending the combined loading-face build.

**Immediate next:** finish v8 archive loading-face geometry and compiler footprint-to-mesh
checks, generate the six evacuation meshes, build the combined runtime, capture the archive,
run the 85-test runtime suite, and retain results. Then qualify the composed loading
site before continuing the full M01 route/action/audio experience.
The Well already has a dais and causeway; do not duplicate it based on the opening screenshot,
where fog/distance keep it out of view. Other maps remain production candidates. Do not edit
C++ while a build is running, and run only one heavy job at a time.

The owner challenged the limited visible progress. The production priority is now a
convincing M01 environment against its story/concept references before scaling art patterns
to M02–M15. Correctness work is necessary but does not substitute for composition, landforms,
architecture, texture detail, motion or sound. Avoid spending the whole pass on infrastructure.

World kits v6 and evacuation props v4 regenerated successfully; the editor build and 84/84
Unreal tests passed (`unreal-world-winding-v6.log`, retained automation JSON). Fresh
`world-winding-v6-captures` were inspected and still fail the visual bar: crushed shading,
flat ground, repeated rock/grass placement and primitive unit forms. Winding was a real source
error, but its repair did not resolve the black materials. Native baseline remains 96/96 in
optimized/debug/ASan+UBSan; no simulation-core changes are intended.

`material-bindings-audit.json` records actual material graphs and assignments. M01 equipment
was multiplying muted ceramic tints by the very dark basalt texture. Source now binds its four
material instances to the registered civic surface master, UVScale1 and zero emissive;
revision `m01-evacuation-material-v2`. Regeneration finished successfully; engine evidence is `m01-civic-material-v2-engine.log`.

`meridian-forward-v3-engine.log` confirms two corrected meshes generated. The old shield
cylinders were near horizontal-faced because yaw was used instead of pitch. Bulwark cells
now use pitch -90 and outward yaw cant; Lancer muzzle and Bulwark projectors point +X.
`test_meridian_facing.py` passed two geometry-axis checks. Preview headings now point toward
the dais and match their preview deployment directions. These C++ changes built successfully in `build-m01-facing-civic-fill.log` (47.08s);
both captures in `m01-facing-civic-fill-captures` completed and were inspected. Do not describe generator tests as visual qualification.

Sky-fill-v1 build/captures completed, but shaded faces remained black. The source now raises
public sky fill to8 and adds a fixed cool lower-hemisphere bounce; this latest setting is
built and rendered: unit shadow faces now retain cool detail. The rock surfaces remain too dark. The original sky capture default150k threshold excluded the33–42k dome;
current threshold30k/emissive-only excludes hidden battlefield state. IsSky is not required
for this ordinary captured-scene path; do not adopt the read-only agent's real-time-capture
inference as a verified defect.

Next: prioritize substantial M01 landform,
archive/withdrawal architecture and surface craft. M02 read-only design is a broad migration
basin with rain-cut basalt shelves and survey loops, preserving all Waystone/account sites
and inherited route semantics. Other fourteen sources remain integration candidates.

## Authority and boundaries

Read `Docs/README.md`, `AGENTS.md`, `CLAUDE.md` and the selected project skills.
`Requirements.md` binds behavior; the Development Bible binds creative intent;
`RequirementsState.md` records lifecycle state. Concepts are visual references, not authority
to invent mechanics. Preserve simulation isolation, navigation truth, scoped visibility,
accessibility variants, palette, matte ground, emissive limits and registered provenance.
The owner explicitly overrode stopping for missing WorkstreamControl lane files.
Use one heavy Unreal job at a time and preserve unrelated changes.

## Checkout and ownership

Branch: `release/world-map-concept-pass`; starting commit:
`fc05cdf08191649363fb774ec88ad19d96c37a37`.
Preexisting modifications are captured in
`BuildArtifacts/Evidence/world-map-concept-pass/preexisting.patch` and `preexisting-status.txt`.
HUD, controller, campaign rewards/progression and related tests belong to preexisting work.
The test wrapper also had preexisting edits: stage only the added world-kit test expectation.
The simulation core remains unchanged. The later campaign-map scope extension includes authoritative campaign world sources and their runtime/checkpoint bindings; preserve the independent frozen skirmish source.

## Implemented, with limits

- Six deterministic biome mesh families, each with formation and low ground relief, two LODs.
- Terrain palette and tile visibility integration; collision, navigation, overlap and shadows disabled.
- Ordinary gameplay terrain now receives the local player's scoped view. Explicit nonshipping
  Glass Scar authoring fixtures retain their unscoped terrain.
- Glass Scar-only canyon composition, deeper banks, sparse relief and public distant scenery.
- Removed bright shelf bars and raised tiled slab clutter; ground material roughness clamp.
- Empty landmark packs now report pending instead of misleading dressing-ready success.

These are candidates. All-site landmarks, natural terrain forms, composition, final lighting,
effects, animation, audio and packaged visual qualification remain unfinished.

## Evidence and current defect

Evidence directory: `BuildArtifacts/Evidence/world-map-concept-pass/`.
Earlier in this task, native tests passed 96 in each of three configurations; Unreal tests
passed 80 before the latest terrain scoping/composition changes. Latest editor build passed.
Geometry recipe tests passed three cases. Rerun relevant tests after final changes.

`glass-scar-depth.png` is an invalid material-quality preview: its log reports
`M_EchoesWorldSurface` failed Metal compilation with `Missing Clamp input`.
The generator had not checked a failed connection. Repair now uses the first expression pin,
checks return values and retained linkage, and repairs the existing clamp in place.
`material-connection-repair.log` records regeneration; inspect its outcome before proceeding.
Do not use the checker-material capture as evidence of authored color or lighting quality.

## Next actions

1. Verify clamp repair, render Glass Scar again, reject any fallback material or pending shader capture.
2. Improve irregular cliff forms, paved causeway/dais, fractured sun and site-specific landmarks.
3. Render all biome families and campaign sites in context; fix route and fog inconsistencies.
4. Audit and complete audio, VFX, animation and visual accessibility using their specific workflows.
5. Run current-source regression checks and packaged captures, motion and audio measurements.
   Keep physical interaction, listening, performance and human acceptance evidence distinct.

Do not reduce acceptance criteria to fit current output. Update concrete findings, decisions,
commands and next steps here before compaction; verify live state when resuming.

## 2026-09-04 continuation update

The owner extended scope explicitly to all visuals, graphics, images, effects and audio,
with autonomous implementation and no intermediate owner review. The handoff does not
claim this work is complete.

World shader connection defect is fixed and Metal rendered without fallback. Source now
checks material connection return values and compilation errors. Current geometry is
world-kits v2, shelf v5, Buried Causeway v2, Broken Sun v4; sky-gradient v1 is generated.
Latest editor build (`build-site-palette.log`) succeeded. Current dirty-tree Unreal report
created 2026.09.04-19.38.10 reports 80 successes, zero warnings/failures/not-run cases;
wrapper save-isolation result must also be read. The earlier full-floor expectation was
updated because all maps now use an authored scoped substrate; pointer collision remains tested.

`glass-scar-fractures.png` is a valid authoring preview before the latest sky/lighting changes:
mean luma 64.1, zero measured clipped pixels, but still visibly unfinished composition.
`shivergrass-start.png` shows why the material pass is necessary: generic molten fissures
and sparse dark grass. Runtime now binds registered ash/civic/verge textures by biome and
disables false ground glow outside Glass Scar; this latest binding passed automation but
needs a rendered capture. Sky uses a dedicated gradient instead of ground textures.
The unused visible Engine floor is hidden on every site while its pointer traces remain.
Glass Scar route props are hidden for other campaign biomes.

Audio read-only audit found no dialogue playback/assets (308 authored lines absent), no
runtime caller activating dialogue ducking, shared rather than nine faction/archetype fire
families, and only five ambience beds. Requirements.md REL-AUD-002 takes precedence over
the older conflicting AudioDirection ducking settings: SFX/music -6dB, 300ms attack,
500ms release. Local Kokoro tooling exists under ../Tools/kokoro; use registered voice
workflow before production. Available tools currently expose no audible listening route;
do not mistake structural audio tests or waveform measurements for listening evidence.

Next: render latest Glass Scar and biome materials, improve boundary scenery/landmarks,
then continue the remaining presentation work. Preserve unrelated HUD/campaign changes.


## Continuation checkpoint — 2026-09-04, fog and capture correction

All six images in `biome-captures` were rendered and inspected. They remain visibly
unfinished: sparse grass, absent cavern enclosure in the starting view, repetitive
civic paving, oversized bright ability rings, and insufficient site landmarks.
They are authoring evidence, not completion. Camera edge panning shifted the captures;
`EchoesRTSCameraPawn::Tick` now freezes motion in non-shipping art-review mode only.

A read-only audit found that the chasm bypassed the tile visibility scope. Current
source caches chasm transforms and gates each mesh footprint plus its local scar-band
classification against known tiles; fissure lights require their influence footprint
known. Explored terrain remains visible; unexplored/reset hides it. The authored
substrate now fills known land independently of these larger canyon pieces. The new
WorldKitVisibility regression exercises chasm meshes/lights through unexplored,
visible, explored, and reset states. Build/test results for this correction are pending
in `build-fog-camera.log`; do not reuse the earlier 80-test pass as current evidence.

Shelf fracture wedges exceeded their stated 780 cm footprint. Recipe is corrected
and the new footprint assertion passes; shelf revision is v6 in source, regeneration
still pending. Capture script now retains the binary diff, untracked source hashes,
and UTC capture times for future runs.

Concurrent document consolidation moved operating authority into root AGENTS.md;
CLAUDE is a redirect. Preserve the other writer's documentation and skill edits.
The read-only agent is now extracting all fifteen map identities from canon while
the parent owns implementation. No staging, commit, or push has been performed.


## Owner clarification and production-plan checkpoint — 2026-09-04

Owner reiterates: each M01–M15 needs its own story-driven map, connected in a large Soryn
adventure; strong characters/backstories; meaningful macro and fine detail; role-readable
units/buildings; coherent, smooth actions and sound. No intermediate owner review requested.
The master already records this in SPEC-MAP-004, SPEC-CAM-041/042, SPEC-VISD-008 and
SPEC-ART-004. Do not duplicate requirements or interpret the MMO analogy as MMO mechanics.

`Docs/MapConcepts.md` now has a fifteen-row current production brief, with story/character
stakes, unique spatial purpose, meaningful details/sound, continuity, and explicit excluded
claims. Internal source review covered all rows; corrected M01 Talar attribution, M02
unmodeled markers/Vaultback objectives, and M07 singular Listening Spine. Historical concept
studies are explicitly subordinate where they contradict current mission/canon contracts.
`RequirementsState.md` records planning IN PROGRESS and no accepted map/visual completion.

Fog/camera build succeeded, and Unreal 80/80 passed with real saves unchanged. Retained report:
`automation-before-perimeter.json` (2026.09.04-19.52.56). This is not the latest source verdict.
Current source/asset revisions: world kits v3, shelf v6, Causeway v2, Sun v4, sky v1. World-kits
v3 generation engine log is retained. Grass now uses pale bent ribbons rather than tiny dark
extrusions, with a neutral white albedo input; runtime binds the leaf material separately.
Public perimeter formations add site-scaled silhouettes entirely beyond playable bounds;
new automation checks every public instance bounding box against the playable rectangle.
Civic base albedo is reduced. `build-perimeter-grass.log` succeeded; the fresh full Unreal
suite is running in `unreal-perimeter-regression.log` (exec session 96985). Next run stable
captures of Glass Scar/Shivergrass/cavern/civic, inspect, and iterate; no quality pass yet.

Asset provenance for the new kit and sky now uses unique ART-WORLD-001/002. Historical
ART-017/018 and ART-022 identifiers already collide within the register; preserved old records,
corrected our new references, and noted the conflict instead of silently reassigning history.
Read-only agent is researching the smallest safe path to fifteen unique mission source/map
bindings, identifying current layout duplication and validation points. Parent owns all writes.


## Live coordination and latest findings — 2026-09-04 20:16 UTC

Separate task `Sync AI guidance docs` (thread 01a06de3-9af6-70e2-9284-e9f3f713019c)
now owns Requirements.md, MapConcepts.md, MapTechnicalBlueprint.md, DocumentationAudit.md,
and RequirementsState.md. DO NOT edit those until coordinated. It preserves our fifteen-row
brief and final state entry and prepares M01's qualification brief. It will run no heavy jobs
or runtime edits. This task retains runtime/source/assets/AssetRegister/WorldMapWorkLog.
Prioritize representative M01 end-to-end qualification before scaling final art production.

Clean Unreal report20.06.26:80/80, zero warnings/errors, save guard clean (unreal-leaf-regression.log;
retained automation-before-leaf-lod.json). Stable-world-captures contains four latest images;
all inspected and still deficient. Perimeter scenery is outside the starting gameplay frame;
cavern still has no vault silhouette there, civic remains generic paving with oversized bright
Aegis ring, grass remained black despite correct MID parameters, M01 sun clipped and flat banks.

Actual grass cause verified with Unreal asset audit: ShivergrassGround LOD0 material slots
[0,1,2], LOD1 [0]. UE LOD import compacted material IDs. Generator now pins every ShivergrassGround
section to material2 on every LOD; world-kits revision v4. Test now reads runtime render sections,
not merely material parameters. Generation session49836/logleaf-lod-generation.log running;
then rebuild, rerun appropriate tests and capture grass again. This source state is not yet verified.

Coordinator's stale World test was repaired. Comments broke the extractor, and frozen historical
map descriptor preceded commit2b3024f's MAP-001 fairness relocations (two Dropoffs/four resources).
Tests/World/test_glass_scar_map_pack.py now recognizes comments, allows exactly those traced
coordinate changes, preserves frozen descriptor/digest/snapshot checks, verifies actual spawn
uniqueness/passability/reachability and recomputes equal sorted Core/haul distance ladders.
10/10 passed in glass-scar-descriptor-regression.log. Coordinator notified.

Fifteen-map architecture audit: only GlassScar compiler/header bound today. M01/03 share base165;
M02 GlassScar+branch; M04–09 share UnburiedRoad branch; M10–15 share LumeReach223. 384 appears only
in obsolete concept text, not current map pack. No new manifest/compiler/runtime cutover yet.
Proposed next implementation: world-only registry, strict parameterized per-map contracts and
hash/header generation, adapter selected by existing operation; preserve mission models, objective
semantics and save formats. Test every branch via pure MissionModel plan getters plus live entity
scan for actual spawn/resources/interfaces. All required API details were provided by read-only
agent audio_gap_inventory and remain in task history; that agent is now idle and reusable.

## Continuing production — 2026-09-04 20:40 UTC

Owner's fifteen-place/story/craftsmanship direction remains active; no final visual/audio
completion or owner acceptance. The documentation task finished its coordinated rewrite;
read current MapConcepts/MapTechnicalBlueprint M01 brief before production. It released
shared documentation ownership. Requirements registry now uses ART030 for passability;
audio ducking/economy contradictions remain TBR-DOC003/004 in that separate task.

Leaf v4 rendered capture still black although actual render LOD indices passed. Captured
`grass-unlit-diagnosis/shivergrass.png`: pale leaves confirm lighting defect. Read-only agent
traced one-sided/copied reversed thin planes, terrain normal mapping and absent tangent
recomputation. v5 generator removes coplanar reverse faces, recomputes world-kit tangents,
creates dedicated two-sided foliage `M_EchoesShivergrassLeaf` (ART-WORLD004). Generation
succeeded (Saved/Logs/ArtAssetGeneration.log marker20.34.49); new source has not yet been
built or captured lit. Preserve engine log before next generation. v4 focused visibility
test passed; full80/80 at20.06.26 remains older evidence.

Dedicated `SM_VFX_AbilityRangeRing` ART-WORLD003 generated successfully: 50cm outer radius,
1cm band,128/96 segments, no selection brackets. EntityView now uses it for Aegis/supply,
scales from sim rules, restrains steady emission. PoweredAegis test additionally checks
actual bounds/rule radius/material and no collision. Build-ability-ring.log succeeded
67.79s, but new radius test and lit screenshot not yet run. Existing capture after that
build was Unlit grass only. All original selected-unit glyph/pick geometry preserved.

campaign_map_compiler agent owns new Campaign source contracts/manifest + generated outputs
and compiler/schema/tests. Its foundation five synthetic tests passed, now authoring all15
real terrain candidates and expanding fixed/branch anchor coverage. DO NOT edit its files
until it reports done. A parent --check at20:40 raced active source edits and correctly
refused a transient M02hash mismatch; rerun after agent settles, not a final source verdict.

Parent owns new EchoesCampaignTerrainBinding.h/.cpp and test. Adapter includes generated
Campaign header, checks identity/census/4-neighbour connectivity before writes; only fresh
64x64 simulation with no entities/ticks allowed. StartScenario selects via existing stable
mission ID; M01 uses identical Preserve opening variant before any founding decision,
later missions use inherited doctrine. No SimCore change, save or mission-model changes.
Campaign initialization scans actual initial entity centers for terrain passability before
views. Existing GlassScar skirmish parity remains; campaign parity now targets its own M01.
New CampaignTerrain UE test covers45variants,15identities/layouts,invalid IDs/doctrine/grid.
All these integration changes await build/regression. run_unreal_tests.sh expectedlist now81.

Current next steps: wait compiler worker completion; --check real source; build directly
with UE Build.sh (content wrapper already ran at ability build, rerun affected tests only);
full UE regression is justified by all-mission terrain cutover. Fix failures rather than
weakening source/mission assertions. Then capture lit grass + M01 normal camera and inspect.
Audio, bespoke landmarks, cinematic/dialogue binding, motion/roster detail, full15map art,
physical/package/performance qualification still outstanding; six kits are not finalmaps.

## Regression failure and correction — 2026-09-04 20:47 UTC

Integrated build (campaign terrain + foliage v5) succeeded52.45s. First81-test suite
FAILED and CRASHED, notqualified: map center guard refused ordinary enemy spawns on
blocked cells in multiple new layouts. SevenAccountsMissionTest then called
GetSimulation()->SaveSnapshot afterfailedstart; retained main-thread stack proves
that null path. Parent fixed the test to return afterfailedinitialization, keeping
itsfailedassertion. Crash/report/log/saveisolation retained under
`campaign-terrain-first-crash/` and `campaign-terrain-first-save-isolation/`.
Realplayer-savebefore/aftermanifestscmpidentical; crashedtestleftGUID-scopedstorage,
wrappercorrectlyreportedcleanupfailureandcleanedtemporarysuite. No player saves touched.

Sourcecompilerworker owns immediate deploymentclearancecorrection (notbroadartredesignyet).
It must validate both ordinaryforces with exact rolefootprints, additionalmissionspawns,
resource/Well/interfaceclearance, regeneratehashes. Parentadded runtimefullterrainfootprint
checkusingConfig.archetypes and nativefixedpublicfootprints before view/readiness; doesnot
changeSimCore orimposeoverlapcheck. ExistinggenericCore/Barracksplacementsalreadyoverlap
bylargeauthoredfootprints; thatseparatedeploymentissueisnotclaimedfixed.

M10/11/12 tests had obsolete223sharedLumecensusasserts. Parentreplacedwithdedicatedmission
contractcensus +everytileparity, preservingfullchecks. Other165assertsareSkirmish andkept.
Currentall15layoutoverviewPNG shows sparse/genericintegrationcandidates, notadequatefinal
mapdesign. Workerprovidedbespokespatialrecipes foreachmission; subsequentproductionmust
shapevalleys/vaults/civicdistricts/courts, withrealprimary/flankroutesandfocallandmarks.
Firstgetdeployment/missionregressionsclean, thensculptandrenderiteratively. Noartacceptance.

## Campaign map admission and checkpoint integration — 2026-09-04

The compiler now checks complete ordinary deployment footprints and mission-specific
semantic clearances for all 15 maps and three founding doctrines. Runtime initialization
checks every initial entity footprint against terrain before creating views. This does not
resolve the separate, preexisting overlap between some starting structures.

Campaign saves now carry an outer map identity envelope: mission, doctrine, map ID,
source SHA-256 and terrain identity SHA-256, with bounded parsing and CRC. All four save,
autosave, validation and load paths use it. Dynamic snapshot terrain remains intact; an
older/unbound or mismatched map checkpoint is refused before live simulation replacement.
The new admission tests also require state/checksum preservation after refusal. Skirmish
keeps its independent format. Replay identity requires a separate audit.

The first integration compile failed on a const FString move and access to the private
operation-to-mission mapping. Both are corrected; the mapping is now a public, read-only
stable identity helper. `build-campaign-clearance-checkpoints-final.log` records successful
editor build in 12.07 seconds. Earlier failed logs are retained. Full 83-test Unreal suite
is running in exec session 66557, logging to `unreal-campaign-map-envelope-regression.log`.
Existing inner-checkpoint mutation fixtures need inspection under the new outer envelope;
preserve their specific negative-case coverage rather than accepting outer CRC rejection.

Native checks passed 96/96 in optimized, debug, and address/undefined-sanitizer builds in
`native-campaign-cutover.log`. No simulation-core files changed. Foliage v5 and the thin
ability-range ring are generated and built but still need lit visual inspection. Next:
complete regression corrections, then capture `shivergrass` and `m01-play`, and produce the
M01 evacuation outpost/archive/causeway composition against its current production brief.
The remaining maps, unit/building detail, motion, audio and release qualification remain open.

## M01 source production and recovery corrections — 2026-09-04

Campaign source now emits named Well anchors for M01–M09. M02/M04/M05 use32,29 so the
inherited center crossing32,32 can remain closed where required. M01/M03/M06–M09 retain
32,32. Runtime resolves those source anchors before the ordinary Future Well spawn;
mission-specific M10+ plans and frozen skirmish remain separate. All initial entity
footprints still undergo terrain validation.

M06 Preserve now has a two-tile civic service spine through its bounded absence connecting
archive approach to shelter/extraction, avoiding the forced exposed escort detour. M05
Preserve retains only the central inherited crossing. Earlier failing regression evidence
is retained in `campaign-map-envelope-automation.json` (70/83) and
`campaign-regression-repairs-automation.json` (79/83). The updated checkpoint tests preserve
inner topology/schema/ledger negative coverage by unwrapping and rewrapping their fixtures;
they also test the new outer map refusal independently. Native96/96x3 remains the current
native baseline; these corrections change no simulation-core code.

M01 now has an archive bay L-shape in source, continuous fitted paving between withdrawal
and archive approach, and30 placement records in the compiled landmark header. Four mesh
families are generated under ART-WORLD-005, revisionm01-evacuation-props-v3. Solid hardware
occupies only blocked cells and replaces generic rock silhouettes; paving is flush on
passable cells. Runtime gates every record by known terrain and removes conflicting props
when terrain changes. New MissionLandmarkVisibility automation tests knowledge reset,
explored retention, live terrain changes, mission reset, collision/nav/overlap/shadow hygiene.

The first evacuation generation stopped on sparse material section indexing; v2 also
failed strengthened winding checks. v3 corrects narrow-beam chamfers, fitted panel size,
face-projected UVs and sparse material bindings. `evacuation-props-v3-engine.log` confirms
four meshes/twoLODs/no collision; Python geometry3/3 passed with winding and UV-area checks.
The thin range indicator v1 remained too bright in a lit view; v2 now uses a steady unlit
shader and a0.36cm source band at the same50cm outer radius. Generation is retained in
`range-boundary-v2-engine.log`. New art awaits current-binary lit capture.

`foliage-range-lit-captures/shivergrass.png` was inspected: foliage now renders pale, fixing
black blades. M01 capture in the same directory still shows flat terrain, repeated rock
clusters and a shiny v1 range ring; those observations are defects, not visual acceptance.
These captures predate M01 landmarks and v2 range. No listening, physical-input, packaged
campaign, performance or owner-acceptance claim is supported by them.

## Dark-face diagnosis after M01 lit capture

`m01-evacuation-lit-captures/m01-play.png` was inspected. The v2 range boundary is now thin
and subdued, but the paving and archive hardware render black. That result exposed a shared
custom-mesh winding error: Unreal GeometryCore VectorUtil.h80–96 uses edge2.Cross(edge1),
while our buffer fans used mathematical CCW with outward edge1.Cross(edge2) normals.
World kits v6 and evacuation props v4 reverse the submitted triangles. Fractured nonplanar
rock caps also need individual triangle normals instead of copying the first cap triangle's
normal across the face. Four world-kit and three evacuation tests now pass engine-convention
winding, UV area and original bounds/determinism checks. Descendant shelf7/sun5/causeway3
revisions force regeneration. Current84/84 suite predates this mesh fix; reinspection pending.


## 2026-09-04 — response to visible-progress concern

Latest M01 and Glass Scar captures finished with exit0 and were inspected internally.
M01 paving and archive apparatus now show their ceramic material; the corrected shield faces
read forward, and cool fill reveals previously black unit sides. This is a visible correction,
not concept-grade environment completion. M01 remains a flat field with repeated small rocks;
the Glass Scar fixture still has broad blank banks and crude low-detail distant silhouettes.
The concept target requires substantial environment asset production and composed landforms.
The execution error was spending too much of the pass on infrastructure and small repairs;
measure the next milestone by a convincing in-engine M01 environment and its story-specific
architecture, not by edit count or passing automation. Preserve all fifteen distinct mission
maps in scope. Do not ask the owner to review intermediate passes.

No heavy process remains from this checkpoint. Latest geometry/source checks and build passed;
the 84-test suite predates the final forward-axis/material/fill changes. Rendered stills do not
prove motion, sound, packaged behavior, physical interaction or owner acceptance.


## Active loading-face implementation — 2026-09-04

The source-verified x23–27,y19 strip is blocked in all three M01 doctrines. A single
`ArchiveLoadingFace` at anchor25,19 replaces two cradles and their intermediate conduit.
The 5×1 shared geometry must stay within X±499/Y±99/Z0–360 cm; its continuous retaining base
makes the blocked footprint legible. Supported overhead rail, attached load lines and retained
archive cassettes explain the installation. The runtime uses the same full-footprint fog gate
as the apron. Geometry worker owns only evacuation recipes/tests; compiler worker owns only
presentation source/compiler/generated header/compiler tests; parent owns integration.

A parent review found the first generalized compiler accepted mismatched declared dimensions
or shifted pivots. It is being tightened to require exact recipe dimensions, yaw0, and
2*anchor+pivot==footprint_min+footprint_max on each axis. Do not build/render the new shared
geometry until these footprint-to-mesh checks and negative tests pass.

## 2026-09-04 — live Unreal surface iteration

The loading-face source work above is now integrated in the current editor binary. This
pass repaired a directly observed material fallback, replaced the service ceramic's fine
tile pattern, exposed buried apron markings, and quieted the basalt material. Source recipes
and generated assets were updated together: evacuation props v9, service materials v4,
cliff material v4 and the original ServiceCeramic texture family. Six meshes regenerated
with two LODs and no simple collision; three existing geometry tests passed.

The current editor-hosted M01 preview moved only the owned carrier through the existing
ordinary scout command. The carrier reached recovery and the HUD reported recovered/intact;
the apron and loading face appeared as their footprints became known. The retained frame is
`BuildArtifacts/Evidence/editor-visual-pass-20260904T235321Z/m01-archive-approach.png`.
The same directory records source/asset identity and the direct editor results. No mission
terrain, objective anchor, movement, collision or fog rule was changed by this surface pass.

The owner directed that the editor stay open. Subsequent material revisions used Play/Stop
and live asset updates in the same process. Desktop-injected clicks did not reliably select
the carrier, so this pass does not claim physical-input verification. M01 still needs stronger
large-form environment composition and full journey/audio/performance review; the fifteen-map
delivery and owner acceptance remain open. A material correction is not map completion.

The retained archive frame measures mean luma 60.8 and a clipped fraction of 0.00029
(about 0.029 percent). Mean exposure falls within the art-direction reference window, but
clipping exceeds its 0.005-percent threshold; this frame is not exposure-qualified.

## 2026-09-05 — M02 migration stonework and live campaign presentation

M02 now has a source-authored 47-record presentation pack: eight three-cell observation
sills, seven rooting shoulders and 32 low passage slabs. Every footprint is checked against
all three doctrine variants of the unchanged migration-basin terrain. The first small sill
looked like a stacked marker in the asset editor; recipe v2 widens it into a six-metre ledge
with an irregular crest and embedded polished shoulders. Three generated meshes have two
LODs and no simple collision. Three geometry checks and nine compiler checks passed.

The landmark compiler and runtime now select a mission-specific pack and verify its terrain
source identity. Full-footprint fog and terrain checks remain in force. A review caught an
M01-specific suppression condition that would have hidden ordinary M02 blocked terrain; the
source now limits that suppression to the M01 continuous-cliff compositor.

The editor remained open while the native module recompiled and reloaded successfully.
`Echoes.EditorPreviewMission` selects an explicitly requested mission between PIE runs through
the normal prerequisite checks. M02 art review uses a synthetic Preserve prologue fixture
in the session's isolated save directory, derived from the existing Seven Accounts test.
It is not earned campaign progress, a mission completion result or playtest evidence.
The fixture identity and limited scope are recorded in `editor_session/m02-preview-fixture.json`
under the current evidence root. Complete-scene review and later-map production continue.

## 2026-09-05 — owner-directed planning reset

The owner directed complete, detailed planning and sequential delivery one map at a time.
M01 is the only active map. M02/M03 assets and partial preview-helper edits are preserved and parked.
The detailed M01 production baseline is maintained in MapConcepts.md, with exact scene regions,
source-pinned routes, reuse/revision decisions, audiovisual/state treatment, review views and B1–B5
build packets. MapTechnicalBlueprint's stale dedicated-M01 source pointer was corrected in place.
No asset generation, native build or editor restart is part of this planning change.

A source-only route check confirmed the planned carrier, worker and withdrawal centerlines are
passable in all three initial M01 masks (19/24/17 cardinal edges). This is not a timing, collision-radius,
combat or runtime traversal result. B1 whole-map composition is next after its bounded preview setup;
no new-map or fine-detail production starts before the M01 packet sequence closes.

## 2026-09-05 — M01 B1 source change prepared

The owner explicitly directed building M01 before the other maps. M01 remains active; all later-map
production stays parked. The first B1 change lowers the loading-court-facing basalt profile while
retaining the taller back crest and every authoritative blocked cell. Two competing east-return frame
records were removed; the loading face and apron remain. The nine presentation compiler tests pass.
The proposed extra approach paving was rejected because those cells already belong to the apron.

No new runtime or visual result is claimed: the open editor's scripting callback was stopped during
the workflow pause, and injected clicks still did not reach Unreal's command field. A one-command manual
reconnection was requested. The composition reload is prepared in the session command queue; it has not
yet run. Existing isolated M01 autosaves and the labeled M02 synthetic art fixture were moved intact
into `editor_session/preserved-before-m01-b1`, leaving the isolated M01 preview save directory fresh.
`editor_session/m01-b1-prepared.json` records the changed source hashes and current execution boundary.

## 2026-09-05 — editor restart repaired; M01 reopened

The owner requested repair after a failed restart. The retained UnrealBuildTool log identified an
invalid LexFromString include. A full rebuild also exposed unity-build collisions in checkpoint
serialization helpers and the campaign terrain namespace, plus a missing terrain-view subsystem
include. These source issues were corrected without changing serialization or simulation rules.
World/Source JSON compiler inputs are now excluded from automatic DataTable import, addressing the
separate logged failed reimport. Texture and mesh importing remain enabled.

The editor target rebuilt successfully, and the editor restarted once under this explicit request.
The scripting connection is restored; the earlier manual reconnection request is superseded. M01 PIE
loaded the expected terrain source (306 blocked cells), all 28 authored landmark records and mission
objectives. The opening scene was visually inspected and retained as m01-restarted.jpg. The editor
remains open with the preview paused. The B1 geometry changes are now compiled and loaded; complete
scene composition review and M01 completion remain outstanding.

Echoes.Runtime.Persistence.CampaignMapCheckpoint passed in the restarted editor.
Echoes.Runtime.Map.CompiledMapBinding refused setup because this art-preview session uses a save
directory outside the platform temporary root; it did not execute its map assertions. No broader
automation or mission-completion result is claimed. Repair logs, build results, launch command, binary
hash and screenshot are retained under
BuildArtifacts/Evidence/editor-restart-repair-20260905T014144Z/.

## 2026-09-05 — M01 object finish queue

The owner directed focused completion of every M01 object. The existing M01 brief and B1–B5
checks remain the map scope; this queue controls object ownership and prevents unrelated polish.
The root task is the sole editor and source writer; other-map work stays parked. Finish each
object's applicable construction, material, placement, state, and review checks before closing
its row. Agent inspection is recorded separately from packaged/human/owner acceptance.

| Object / family and all M01 instances | Current focus / outstanding work | State |
|---|---|---|
| ArchiveFrame — east outpost, archive north, withdrawal west, two archive lip frames | Post/crossbeam and rail joints corrected; east-outpost and archive close views inspected; final packaged review pending | AGENT VERIFIED — construction |
| ArchiveLoadingFace — five-cell loading installation | Connected supports and suspended fixture inspected in archive view; final route/state review pending | AGENT VERIFIED — construction |
| ArchiveApron — recovery court | Panel/repair/rail surfaces inspected; intentional submillimetre separation retained; final route review pending | AGENT VERIFIED — surface construction |
| ServiceConduit — west/east outpost, archive return, withdrawal return, causeway shoulder | Tube ends/fittings verified; E1/E2 cover all placements. F2 shows the causeway manifold above the corrected shoulder (V030); route/package and alternate-angle checks remain | IN PROGRESS — placement |
| ArchiveCradle — west outpost | Saddles, returning retention bands and cabinet footing rebuilt; lower west bank exposes the grounded assembly in close view | AGENT VERIFIED — construction |
| RoutePaving — return cells6–16, archive17, causeway29/32/35 | Broad slab construction retained; E1 shows flush return-to-apron transition and three separated Well route marks. Ordinary traversal and packaged review remain | IN PROGRESS |
| Basalt banks, fracture and crossing masses | Loading court improved; inspect whole route, all gaps and terrain/fog edge | IN PROGRESS |
| Ground, resources, horizon and sky | Sampled exposure and four edges inspected; J2 actual6200 corners expose exterior-backing gap V041, source corrected pending reinspection. Resource/material/sky state coverage remains | IN PROGRESS |
| Meridian Core, Foundry, Dropoff and utility structure | M01 contact, operating states, feedback and function review | OPEN |
| Carrier, workers, line units, heavy unit and observed opponent actors | Bulwark folding/facing reinspected I1; undercarriage and other states open. Surveyor diagonal-turn/reduced-motion restoration replants remain V036; other role/action and sound coverage open | IN PROGRESS |
| Future Well and protocol effects | Dormant/Harvest/Preserve/Reshape, ownership, warning and fallback review | OPEN |
| Mission interface, story and sound | Existing script binding, actual voice, skip/resume, accessible cues and result outcomes | OPEN |
| Complete M01 journey | Focused branch/fog/save checks, packaged route, performance and required human/owner evidence | OPEN |

Prior object-construction evidence: `BuildArtifacts/Evidence/m01-object-finish-20260905T015437Z/`.
Active continuation: `BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z/`, with per-instance checks and linked E1/E2 captures. The original receipts remain retained.
The opening and archive previews use pitch−48°, yaw−45°, FOV55 and arm3800. Archive review
uses ordinary automated move orders for the owned mobile group; it is an editor composition
diagnostic, not a physical-input mission playthrough or carrier-only movement test.

### Object-pass result and retained limits

Four M01 prop families now have source-backed corrections: ArchiveFrame joints,
ServiceConduit receiving connections, ArchiveCradle saddles/retention/cabinet footing,
and RoutePaving slab scale and wear layout. ArchiveLoadingFace and ArchiveApron were
inspected without unnecessary geometry changes. All six generated v13 meshes retain
two LODs and zero simple collision; the three existing recipe checks passed.

Lower foreground shoulders at both outposts now expose the small equipment while
preserving the existing blocked cells, cliff footprint and fog mask. The native module
rebuilt and loaded as libUnrealEditor-EchoesOfTheBrokenSun-9298.dylib without closing
Unreal. ContinuousCliffGeometry reported Success, but hot reload retained duplicate
automation registrations; this is not fresh-process qualification of the new local profile.

Pixel diagnostics located the retained captures' white clipping in the resource readout
and minimap camera marker. These now use the existing theme's primary color; selection
marker fill follows the same theme. High-contrast theme behavior is retained in source,
not newly qualified. No global lighting or material compensation was applied. Final
1920×1080 standard-theme captures measured zero clipped pixels: east outpost mean57.94,
west outpost54.98, Well approach55.03 and archive60.81. These four static editor views
satisfy the reference exposure window for those frames only.

The archive preview remains paused in the open editor. Group scout moves are ordinary
automated commands used to reveal inspection areas; they do not establish a continuous
carrier/worker mission journey. Two injected clicks in the Well view produced no observed
selection response; cause is unestablished and no physical-input pass is claimed.

Next object focus: finish the Well precinct and its three paving placements as an integrated
site, then actor/protocol motion and feedback. World/route closure, story/voice/sound,
accessibility interaction, the packaged mission journey, performance and required human/owner
evidence remain open. The map is not complete and no requirement acceptance state is promoted.
See session.json, static-object-identities.json, live-rebuild.json, final-view-exposure.json
and before/after captures in the current object-finish evidence root.

## 2026-09-05 — M01 complete-inventory production continuation

The owner requested every visual aspect of M01 and then directed continuation.
Root remains the sole source/editor writer on `release/world-map-concept-pass`,
base commit `fc05cdf08191649363fb774ec88ad19d96c37a37`, with pre-existing dirty work
preserved. Editor PID21554 remains open. The dedicated M01 terrain source still
hashes `8ae50fa5adf740f0f7f0508c151e82c4e86b7f3a1e70cf323717ee536418669b`;
objectives, initial masks, routes and deployment are unchanged. The live presentation
source has28 records, including15 paving instances, rather than the older30-record
planning baseline. Three Well paving records now use yaw90 to follow the approach.

Active evidence root:
`BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z/`.
Its inventory and defect register extend the existing object-finish queue. B1 remains
in progress. B2–B5 and later-map production have not been advanced by these captures.

The first matched views showed a machined bank bevel, lit unknown shroud, opaque
remembered ground, bright rectangular scar trays, floating bed accents, and crossing
details beyond the knowledge edge. Native/editor revisions now supply an irregular
inward bank crest, a grounded wall foot, M01 unlit opaque unknown fog, and a static
translucent remembered-ground tint. The v2 tint exposed cube-side seams; v3 limits
opacity to the upward face and the repeat view removes that false grid. Existing
M01 legacy scar accents are retired in favor of grounded registered basalt. Three
crossing actors now require a known full footprint. The retained v3 fracture view
shows the distant magenta crossing concealed. A partially known span still loses its
bed: a cell-scoped bed correction is building and has not yet been reinspected.

`opening-baseline.png` is a wrong-frame diagnostic: camera movement while paused
left an archive view. `opening-fresh.png` is the valid initial static opening.
The `b1-*` group traversal captures issue ordinary Move to all8 mobile units; their
clumping at the Well is not a carrier-only mission route, a protocol commitment or
movement-quality acceptance. Capture timing uses wall time and shader stalls changed
simulation tick alignment in the v3 set; compare composition with that limitation.

M01 shroud v3 generated with opaque/unlit unknown and translucent/unlit explored
materials, both instancing-enabled. Native builds1949 and8580 succeeded; the later
bed/PIE branch-review build is pending. Source landmark checks previously passed9/9.
Native fog, terrain, interaction and persistence tests still need current-source
execution. All screenshots are EDT, not packaged or owner-accepted evidence.

A PIE-only `Echoes.EditorPrologueCompletionChoice` setting is prepared for the
existing controlled ordinary-command completion sequence:0 off;1 Harvest;2 Preserve;
3 Reshape. It snapshots on scenario start, is ignored outside PIE, keeps the
command-line Preserve fixture, and cannot change a running branch. This supports
inspection without closing the editor. The fixture still grants controlled resources;
its future evidence must say so. Fresh PIE and retained isolated ledger generations
are required between branches. UI overflow/subtitle defects, Well glint, full actor
motion, boundary/sky, audio, accessibility and packaged checks remain open.

### B1 ravine admission and capture preparation follow-up

The `b1-bed-*` views ran module `libUnrealEditor-EchoesOfTheBrokenSun-5648.dylib`
(build succeeded in170.20seconds). The registered exposure measurement retained in
`b1-bed-exposure.json` gives opening59.5603, archive54.5139, Well54.5019 and
fracture52.7409 mean luma; every whole-frame clipped fraction is0. These are EDT
frame measurements including HUD/fog, not performance or owner acceptance.

The remaining indigo ravine area was isolated with diagnostic074: temporarily hiding
only the fog actor removed the black unknown layer but left the blue area unchanged.
The actor was restored. Audit075 compares the runtime bed and unknown-tile transforms
against the source:245 expected bed cells,118 known/visible,127 unknown/hidden and
zero admission mismatches (`b1-ravine-admission-audit.json`). The exposed area is sky
seen below the old shroud bottom at−16cm; M01's authored bed remains−1750cm. The
source correction extends only M01's unknown volume to−1766cm and preserves top184cm,
XY coverage, simulation knowledge and other-map behavior. Matched reinspection and
native execution are pending; the bed-depth value has not been changed.

The bounded `Echoes.EditorCaptureM01` command prepares real-time PIE viewport video
at24/30fps, retains HUD/input and playback volume, drains queued frames, and restores
the previous viewport sizing policy. It uses an even capture size for the Mac codec,
records the actual movie path/bytes, and explicitly carries no audio or performance
claim. Its first native build failed because the engine's public video-protocol header
requires an explicit AVIWriter dependency; that failure is retained as
`build-capture-9710-failed.log`. The dependency correction and PIE-only ordinary-camera
restoration are rebuilding. Neither compiled source nor the command receipt establishes
a playable movie; test recordings still require duration/frame and visual inspection.

The normal camera's source bounds are1400–6200, while earlier art diagnostics used
up to6000. The full ordinary maximum remains a required inspection. B1 remains active;
B2–B5 and the existing object queue have not been promoted.

### 2026-09-05 — B1 ravine backing and motion calibration

**Author and owner:** Angelis Pseftis. Evidence remains SRC/EDT; B1 is active.
The successful 3318 module extended M01 unknown concealment below the ravine bed.
The remaining sky wedge passed through an unsealed retaining-bank interval. The 8831
module adds a continuous registered backing inside the same blocked rim cells, with
exact knowledge admission and no collision/navigation changes. The matched
`b1-bank-fracture.png` closes that wedge. Its flat vertical amber streaking is recorded
as M01-V012 for B2; closure of the aperture does not establish finished bank materials.

Motion calibration exposed two independent installed-engine capture limitations.
The first 1336-pixel captures suffered CoreVideo row-padding shear. The next 1280-pixel
captures decoded without shear but retained a magenta border because the native Slate
rectangle and resized render target differed. All four calibration clips are retained
with explicit failed qualification records; none is accepted mission motion evidence.
`Scripts/inspect_review_movie.swift` sequentially decodes frames through AVFoundation
and retains duration/frame counts and three sampled frames. The 4496 capture helper
now refuses mismatched native/Slate dimensions, widths not divisible by64, or odd
heights. It does not resize the viewport. This supersedes the earlier description of
even-dimension capture resizing. The ordinary play window is the next calibration gate.

Source-only actor inventory confirms12 local and11 initial Kharuun opponents, plus
eight Matter deposits and one Future Well. Registered role forms and generic visible
states exist, but full role/action motion inspection remains open. Archive recovery is
a mission-position predicate; no authoritative archive-cargo attachment state exists.
Production progress, resource depletion and some role-specific operating forms still
need presentation review. No loaded-archive mesh or invented commitment/interruption
state is claimed by this pass. `visual-inventory.json` and `defects.json` carry these
open checks. Later-map production remains parked.

### 2026-09-05 — B2 surface, B3 branch motion and B4 field readability

**Author and owner:** Angelis Pseftis. M01 only; B1 whole-boundary coverage and the
existing object queue remain active. The current dirty base remains fc05cdf.
Modules6484/3138 reuse the registered cliff-surface-3d-basalt-v4 material on all four
M01 ravine-backing slots, set M01 ground to a25m period, and reduce only the Well basin
body to metallic0.08/roughness0.78. The actual Harvest tactical view retains ceramic
detail without the earlier broad white glare. A matched wide backing view is still open.

Floating PIE and native-window calibration resolved the failed movie geometry.
The capture helper refuses a Scene/Slate mismatch and never resizes during recording;
a separate guarded editor-only resize reaches exact1280×720. Preserve, Harvest and
Reshape controlled routes are retained under sibling `m01-motion-*` evidence roots,
with identities in the current visual evidence root. These use ordinary simulation
commands and isolated saves, with explicit fixture economy/camera changes. They are
EDT evidence, without audio, performance, physical-input or packaged acceptance.

The0495 Reshape run holds only the fixture's withdrawal command, leaving simulation
active. The120.04-second clip decodes2880 frames. Viewed frames6/7 show17/12seconds
remaining;8/10 show the4/1-second warning;12 shows expired; the ordinary withdrawal
then completes at tick2215 and commits one isolated ledger record. The Well center
view does not prove all altered cells or occupied-cell fallback. Its purple core still
needs explicit expired-state review. A CUA Tab selected the owned Command Core;
selection title/integrity and its production card were visible. This is synthetic GUI
input evidence, not Angelis's physical review.

B4 corrections wrap actual font widths, keep status and subtitles in separate lanes,
flow result copy above unchanged buttons, and resolve ledger tokens from frozen result
fields. Harvest150% high-contrast and Reshape150% standard views were inspected.
The first150% field capture exposed command-strip/selection overflow, retained as
M01-V017.0495 corrects those visible overflows. A further source correction aligns HUD
and deck geometry with the full0.8–1.5 settings range; native endpoint/hit-area checks
and the80/100/120/150 render sweep remain pending. Requirements stay IN PROGRESS.


### 2026-09-05 — B1 perimeter sweep and branch-specific narrative correction

**Author and owner:** Angelis Pseftis. Current evidence remains SRC, native automation
and EDT. M01 is IN PROGRESS; no packaged or owner acceptance is asserted.

The `m01-motion-region-boundary-scale-20260905T054138Z-ABB36A96` movie is a valid
1280×720 raster. All17 sampled frames were inspected: opening, archive court and
loading approach, withdrawal, Well, fracture, all four camera-limit edges,80/100/120/150%
HUD, actual Reshape warning/expiry and successful withdrawal. The wide fracture view
no longer shows the amber-stretched backing. The3019 inactive-Well clip shows the
core dimming and the orbit retaining its last pose at actual expiry. The fresh native
`native-focused-20260905T053724Z` report executes23 checks:22 pass, with only the stale
landmark cross-mission expectation failing. Its corrected test passes separately in
`native-focused-20260905T055221Z`; both save guards pass. The corrected cliff-normal
classification and M01 expiry regression are among those native passes.

The sweep also exposes three defects, retained in the existing `defects.json`:
M01-V022 plays other Well branches' dialogue during Reshape; M01-V023 exposes a blue
background gap and a constant-width basalt row at the map edges; M01-V024 places the
80% HUD's third objective below its panel. The M01 narrative source now assigns one
signal to each branch trio and keeps common withdrawal separate. Dispatch reads the
current owned Well, before the campaign ledger exists. Approved dialogue and canon
hashes are preserved; the narrative compiler regenerates its pack and digest. The
source validator retains its stricter canon, speaker, branch-parity and binding-status
constraints. Authored voice/cinematic metadata remains a separate unresolved record.

The perimeter correction adds four abutting public ground strips using the registered
WalkSurface, separate from the4096-cell visibility cache. M01 basalt strata now vary
width, aspect and height with lower broken feet; transformed mesh bounds keep every
formation outside playable cells. This changes scenery only. The objective rows now
use the actual panel height. Native regression and matched runtime reinspection of
these three corrections are pending at this entry. Whole-object, actor/action,
briefing/failure,1080p, packaged and performance coverage remains open.


### 2026-09-05 — M01 contact checks and capture qualification

**Author and owner:** Angelis Pseftis

Native NullRHI suite `native-focused-20260905T063309Z` passed all five exact tests (M01WorkContact, M01WellExpiry, MotionFamilies, CombatEffects, PointerSurfaceCoverage); real player save hashes/stat records remained unchanged. The target-bound Surveyor beam and compact M01 Well physical geometry are source/native evidence pending current editor inspection.

Loaded module 6508 C3 frames 3–8 show the Reshape trio followed by common withdrawal dialogue; frames 7–10 close the exposed boundary strip on all four matched edges, and frames 11–12 contain all objective rows at HUD 80%. Corners and rotated boundary views remain open. C3 also exposed worker/Well overlap (M01-V025).

The D2 landmark tour did not inspect its intended scenery: ordinary play had already lost the carrier at tick 6931. Retain it as bounded actual failure-overlay evidence, not B2 object evidence. D1 shows ordinary gather/delivery motion, but its failed sampling callback prevents a synchronized telemetry claim. The rigid Surveyor leg assembly remains M01-V029. Repeat B2 with ordinary reveal followed by an explicitly logged editor-only simulation pause. No owner or packaged acceptance is claimed.


### 2026-09-05 — M01 full registered landmark reveal and Surveyor derivative production

**Author and owner:** Angelis Pseftis

Module 4065 loaded at 06:37:35.967 UTC. A fresh ordinary M01 accepted Surveyor 6→(22,17) and Surveyor 5→(29,28); freeze at tick364 preserved the carrier at75 HP. The captured component inventory contains all28 registered instances: Cradle1, Frame5, Paving15, Conduit5, Apron1, LoadingFace1. E1 retained nine static camera views (66.04s,1280×720,1584 decoded frames) and E2 retained the return conduit, dormant Well and causeway close views. Every E1 sampled frame0–9 and E2 frames0–2 was visually reviewed. This is frozen ordinary-knowledge composition evidence, not motion or physical-input evidence.

Visible saddles, restraints, frame feet, load cable, loading rail and its piers are connected in the inspected views. Paving meets the apron flush. The causeway service manifold is mostly hidden by its surrounding bank (M01-V030); a smooth local shoulder reduction is source prepared. The dormant Well shows distracting ornament-shadow spots (V031), also prepared for correction. Current inventory states remain bounded per surface; component counts do not establish all-angle finish.

The original Surveyor form now has four M01 derivative assets generated through the narrow approved pipeline: body484/242 triangles, upper60/60, lower24/24, foot24/24; all two LODs, explicit material zones and collision0. Generation receipt and hashes are retained under `m01-visual-completion-20260905T024342Z`. Runtime two-link articulation is under native review. The original combined roster and later-map production are unchanged. Production/build work feedback, Bulwark deployment visibility and other M01 walker gaits remain V032–V034. No requirement COMPLETE or owner acceptance is granted.


### 2026-09-05 — M01 articulated work and 1080p interface inspection

**Author and owner:** Angelis Pseftis

Module2947 actually loaded at07:01:42.724UTC after the corrected build and seven fresh native checks passed (`native-focused-20260905T070020Z`, unchanged player saves). F1 records72.04seconds of ordinary Surveyor gathering/delivery with the new separate torso and six articulated limb components. Parent-reviewed normal, reduced-motion and restored-normal frames show connected assemblies and repeated foot lift;885 sparse telemetry samples expose candidate sharp-reversal endpoint jumps. The fixed landing target and emergency reach replant are recorded asV036. No universal no-slide/no-snap claim follows from this capture; a render-tick instrumented reversal correction is in progress.

F2 frames0–2 show the causeway manifold above its shallow bank and the dormant Well basin without ornament-shadow spots. These matched views correctV030/V031 within EDT; other angles and active branches remain open. F3 is an actual1920×1080,44.33second title/brief/pause/HUD tour. All seven sampled frames were inspected. It exposes duplicate title metadata, overlapping briefing copy,150% briefing overrun, incorrect Mara-carrier deployment labels, and a clipped pause-footer technical claim (V035/V037/V038). The ordinary UI shortcut also omits80/150 endpoints. M01-only measured text flow and shortcut corrections are source prepared, with render and mouse/keyboard reinspection pending. Canonical title/brief/deploy calls in F3 establish EDT path evidence, not physical input or owner review.

Each E2/F1/F2/F3 capture now has a bounded qualification receipt hashing the movie, capture metadata and contemporaneous source-identity receipt. Its association with the loaded module remains temporal; the recorder itself does not embed that module hash. Movies have no audio track and establish no platform performance result. B1–B5 and requirement states remain IN PROGRESS. Existing anchor, roster and Reshape decisions remain open; later-map production stays parked.

### 2026-09-05 — Briefing and pause reinspection; actor state work continues

**Author and owner:** Angelis Pseftis. M01 remains IN PROGRESS.

The hotloaded3712 module renders the M01 briefing and pause screen at native1280×720, HUD150%, high contrast, reduced motion and reduced flashing without the sampled bottom-row/button overlaps. The retained `m01-motion-ui-second-native720-150-20260905T080645Z-1F4FDF1F` movie and decoded frames0/1 show the correction. Briefing spacing uses measured glyph height and reserves the action area; the selected text size is retained. The pause screen uses a compact M01 field card. Earlier4646 movies remain the defect evidence, including genuine keyboard U/Return/P observations; they do not establish physical mouse acceptance.

A mouse attempt reaches the controller but resolves away from the visible target. M01-V039 records the unresolved coordinate/event-path diagnosis; editor-only instrumentation now includes the Slate cursor, geometry origin and viewport cache. Production pointer dispatch is unchanged. The first new UI capture was1280×730 and is qualified only at that measured size; the separate corrected capture explicitly verifies1280×720.

Native runs073434Z,075404Z and081339Z preserve failed Surveyor reversal results. In081339Z, four other focused tests pass, but the rig has four emergency replants and110.883cm planted-foot drift. A subsequent correction shares exact reach planning across restart and uninterrupted reversal, retains a pending restart until displayed movement begins, and strengthens event assertions. Its build and native outcome are recorded separately; the failed runs are not superseded as historical evidence.

The original Bulwark packed/deployed/movement baseline is retained in `m01-motion-bulwark-packed-deployed-baseline-20260905T081754Z-78E56E13`. Five sampled views show the same fully spread barrier in both states; the deployed cover component has scale0.001. M01-only body and hinged-wing derivative sources preserve the assembled approved form and the standard roster. Generation, runtime integration, state/facing checks and rendered reinspection remain outstanding. This is V032 production, not acceptance.

Source/module hashes and bounded qualifications remain in `m01-visual-completion-20260905T024342Z` and the individual capture directories. These movies have no audio track and provide no packaged performance result. B1–B5, broader actor/state coverage, package integration, the recorded authority/capability decisions and Angelis-only acceptance remain open. Later-map production remains parked.

### 2026-09-05 — Bulwark deployment reinspection and live diagonal gait regression

**Author and owner:** Angelis Pseftis. M01 remains IN PROGRESS.

The M01 Bulwark body and two hinged wings were generated successfully at08:33:48 UTC after retaining the first failed lower-LOD material audit. The generated revision is `m01-bulwark-deployment-parts-v1`; the original combined roster remains unchanged. Native084912Z passes the derivative, pooling and authoritative deployment checks with zero warnings/errors and unchanged real player saves. The corrected pool path clears child visibility again on an ineligible rebind. Module8594 loaded at08:51:51.132 UTC.

I1, `m01-motion-tested-actors-live-comparison-20260905T085741Z-3CB1DBE3`, retains120.375seconds at1280×720/24fps. Bulwark sampled frames1–8 show mirrored packed wings, connected intermediate folds, distinct deployed and packed silhouettes, and stable facing during translation and return. Locomotion undercarriage articulation remains open; these samples do not qualify combat, damage, death or complete continuous motion. V032 is partially corrected, not closed.

The same run exposes three real Surveyor emergency foot replants at10.2919,12.1765 and15.8792seconds, before reduced motion. The native084249Z rig pass therefore does not establish the actual diagonal gather/delivery route. V036 remains open. The initial visible diagonal departure now enters the existing reach-bounded planner, and the regression fixture starts at the observed worker/resource positions. New native and live results are recorded separately. No reach guard or gameplay authority is relaxed.

Actual CUA Return/Tab/Q deploys, selects the Command Core and produces Surveyor34 through ordinary commands. The deck and queued/progress text still display generic Worker (V040); M01-only role-name corrections are source prepared. CUA click/drag reaches selection input but both Slate and cached viewport coordinates stay250,533; V039 retains that unresolved evidence without an inferred production-pointer fix. No physical input acceptance is claimed.

I1's qualification hashes the movie, metadata, telemetry and8594 identity and records exactly which frames were inspected by the parent and internal specialist. It has no audio track and provides no packaged performance evidence. B1–B5, full actor/Well/interface coverage, required packaged checks and Angelis-only acceptance remain open.

### 2026-09-05 — Supported camera limits and authored role cards

**Author and owner:** Angelis Pseftis. M01 remains IN PROGRESS.

Module3408 loaded09:17:46.311 UTC after build166.47s and native091614Z six exact passes with zero warnings/errors and unchanged real saves. J1 confirms the Surveyor role name consistently in queued toast, progress and command card. J3/J4 show Foundry and Surveyor cards at100/150% with high-contrast endpoints and no sampled clipping. Ordinary CUA Tab selection is retained; the movies are frozen or controlled EDT views, not whole-mission mouse acceptance.

The actual diagonal route still replants at17.1964 and20.8503seconds and again at82.8997 after reduced-motion restoration. The new per-display-tick trace identifies a target-heading reversal preceding the displayed-root reversal; the correction uses one target-edge rephase and clears stale movement history on an accessibility transition. The native fixture now reproduces that delayed-display relationship and checks exact C0 entry, support retention and one-shot behavior. This is a new source correction; V036 stays open pending native and live reinspection.

J2 reaches the actual ordinary6200 maximum at all four corners and four edge midpoints, then1400/6200 arrival views. All ten samples were reviewed. Eastern/northeastern views expose the edge of the50m exterior substrate beneath distant basalt (V041). Source now extends those same four public strips to100m; no playable tile, fog knowledge or draw-count increase is introduced. Matched current-source reinspection remains pending. Unknown inner fog is not counted as missing terrain.

The existing inventory now contains explicit B1–B5, scenery, deployed role/action, per-protocol Well, UI/narrative/accessibility and packaged-check rows. UNINSPECTED, CAPABILITY_ABSENT, AUTHORITY_CONFLICT and bounded EDT observations are distinct. The packaging script was actually invoked: first refusal retained for inherited GIT_PAGER; a clean-environment retry exits10 because this source is not a detached dedicated linked worktree at pushed main. No package was produced or packaging safeguard weakened. Later-map production remains parked.

### 2026-09-05 — Camera evidence correction and keyboard help

**Author and owner:** Angelis Pseftis. M01 remains IN PROGRESS.

The supported-camera statement in the preceding entry is withdrawn. J2 and K1 used a positional Unreal Python rotation constructor that produced pitch−45/yaw0/roll−48, confirmed by actual component readback168. Those movies retain diagnostic value only; their exterior-gap observation does not establish a normal-camera defect. Their qualification records now state this explicitly. A replacement fixture uses named pitch/yaw/roll arguments and asserts the actual component rotation at every view. Its first repeat K3 was obscured by the pause overlay; the separate unobstructed repeat is required before any corner-coverage claim. The four-strip100m source change remains under reinspection.

Module4642 renders the new Home/Arrows/Space/End field help at1280×720,150% high contrast/reduced motion/reduced flashing. K2 sampled3.5sec shows all five field-key rows and the resume action without overlap. This is a bounded paused EDT layout observation. CUA Return actually resumes the overlay; physical input and the full resolution/accessibility journey remain open.

The runtime scenery register records17,073 instance slots in18 component families, including16,135 zero-scale slots. Counts include fog-scoped caches and do not represent17,073 visible objects or completed object inspections. The existing visual inventory links each family and retains per-instance finish as uninspected unless separately supported.

### 2026-09-05 — Unobstructed boundaries, bank correction and coordination

**Author and owner:** Angelis Pseftis. M01 remains IN PROGRESS.

K4, `m01-motion-normal-camera-unobstructed-corners-20260905T100142Z-3C2BE36B`, retains52.04seconds at1280×720. All ten sampled views were inspected: four corners and edge midpoints at6200, arrival at1400/6200. Actual pitch−48/yaw−45/roll0 is asserted per view. Current100m exterior ground continues beneath scenery to the sampled image edges; this qualifies the current source only and does not restore the withdrawn prior50m defect attribution. V042 records repeated large polygon caps, flat ground texture on vertical faces and weak depth between bank courses. The source correction assigns the already-registered cliff master to M01 public basalt, keeps the ground material on horizontal backing, and separates the low feet from a rear course700–1400cm outside the boundary. Instance count, scoped cells and collision/navigation roles are unchanged.

Build `build-rig-pelvis-banks.log` succeeds in23.81seconds after retaining a failed test-array syntax build. Fresh native101237Z passes seven terrain/fog/contact/motion-family/pooling checks with zero warnings; Surveyor fails five assertions. The failed J1-derived fixture inherited an earlier route's position, used a duplicate point for its target turn and skipped a recorded0.5841second bridge. The test-only correction repairs those transitions; the production pelvis solver remains unchanged. The prior failure is retained, and V036 has no pass claim from it. Real player save hashes/stat records remained unchanged.

K5/K6 retain bounded keyboard construction attempts. A selected worker was lost during a fixture transition; no successful construction or confirmation is credited. A concurrently resumed older task later attempted GUI control and started a Well preview during the build reservation. Both tasks explicitly coordinated: the older task confirmed no source/asset edits or builds and released all mutation. Its new evidence and overwritten session pointer are preserved; the current pointer is restored. Hotload177 safely refused active PIE, and only the released046 preview was ended. Editor21554 remains open. The continuous visual-completion task again owns all source integration, editor/GPU and heavy work.


### 2026-09-05 — Surveyor route correction and authored turning rate

**Author and owner:** Angelis Pseftis. M01 remains **IN PROGRESS**.

The continuation recovered the newer L1/L2 evidence before restarting production. L1's four emergency
foot replants occurred when a normal swing boundary was crossed between display frames but the old
contact was still evaluated at the start of the arc. The prepared correction evaluates the elapsed
portion of that ordinary swing while retaining exact continuity for explicit restart/reversal entries.
The build recorded in `build-normal-swing.log` completed in 63.62 seconds; native run 104801Z passed nine focused mission, terrain, fog, contact,
pooling and motion tests. The campaign-map source suite separately passed all nine tests.

Internal review also found a 360°/s pelvis cap and an exponential heading filter where SPEC-MOV-010
requires 720°/s. M01's heading and pelvis now use that authored rate, with the existing reach checks
and reduced-motion behavior retained. The build in `build-facing.log` completed in 79.18 seconds. Final native run 105653Z passed
four focused checks with zero test warnings/errors. The rig check proves 14.4 degrees of actual rendered
turn in 20 ms and drives ninety authoritative seconds of ordinary gathering: repeated cargo delivery,
planted support within 1 cm, and no emergency or discontinuity reset. Both native runs left real player
save hashes and filesystem records unchanged and their scoped storage empty.

Module 3467 loaded at 10:58:13.072 UTC. M1 records 110.04 seconds at 1280×720; 2,639 frames decode successfully.
The ordinary Gather command for Surveyor6/resource24 was accepted. Its 6,702 presentation samples over
112 seconds record 566 planned swing starts and 539 landings, zero emergency replants, zero discontinuity
resets, and no unexpected pose resets outside the reduced-motion transitions. The differing start and
landing totals include interrupted/rephased transfers; they are not a stride-completion score.
Parent review covered sampled frames 0–5, including the approach pair and earlier failure intervals;
internal specialist review covered 6–11, including reduced motion and restored tactical views. The
sampled assemblies remain connected. This corrects V036 for the retained route and the source/native
turn-rate defect V043. V044 retains weak lower-leg/sole contrast against shadow at tactical zoom.
Still samples do not establish every intermediate pose or universal no-slide/no-snap behavior.

Evidence is retained at the absolute resolved root
`/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/BuildArtifacts/Evidence/m01-continuation-20260905T104625Z`.
The movie and its hash-bound qualification are in the sibling
`m01-motion-normal-swing-facing-live-20260905T105851Z-13256180` directory. Module association uses the
actual load log and contemporaneous source hashes; the movie does not embed the module hash. It has
no audio track and establishes no packaged or platform-performance result.

The existing object queue continues with bank repetition/bare strips V042, lower-leg readability V044,
other actor/action states, pointer interaction V039, Well capability dependencies and the integrated
mission review. Gameplay layout acceptance, package qualification and Angelis-only acceptance remain
open. The concurrent harvesting-guidance append was preserved; no economy rules or later maps changed.
The editor is retained in the paused M01 review state, and the original build configuration is restored.
