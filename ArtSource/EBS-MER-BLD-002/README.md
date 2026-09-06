---
title: EBS-MER-BLD-002 Power Link — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
package: EBS-PKG-MC-POWER-LINK
production_asset_id: EBS-MER-BLD-002
production_maturity: BLOCKOUT
revision: ebs-mer-bld-002-blockout-v3
canon_status: CANDIDATE (delegated design selection; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-BLD-002 Power Link — production source

This folder is the single authoritative source record for the Power Link production asset. It is
edited in place; Git retains history. Everything here is bounded by the shared
[agent contract](../../AGENTS.md), the [authority map](../../Docs/README.md), and the frozen
preparation records under [Docs/VisualAssetPipeline](../../Docs/VisualAssetPipeline/README.md).
Maturity for the production lane is recorded in [../production-ledger.json](../production-ledger.json);
the frozen contract in `motion/gap-decisions.json` and the concept register are not modified.

## 1. Source contract (what this asset is bound to)

| Binding | Value | Identity |
|---|---|---|
| Package contract | `production_policy[EBS-PKG-MC-POWER-LINK]`, gaps GAP-01/02/03 | [gap-decisions.json](../../Docs/VisualAssetPipeline/motion/gap-decisions.json) |
| Reserved production ID | `EBS-MER-BLD-002`, planned `SM_EBS_MER_BLD_002` under `/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_002/` | contract `reserved_production_asset_id`, `planned_primary_asset_name`, `planned_unreal_folder` |
| Concept inputs | `EBS-CON-MER-BLD-002` (original sheet, REWORK), `EBS-CON-MER-BLD-006` (annotated study, REWORK) | [concept-register.json](../../Docs/VisualAssetPipeline/concept-register.json) |
| Selected candidate | `power-link-maintenance-complete.png` | sha256 `b3816b9e…83bf20` (generated image, `CANDIDATE_FOR_OWNER_REVIEW`; rights: project-generated under the owner's authorization, see its `generation-record.json`) |
| Construction reference | `motion/construction/power-link.svg`, `component-geometry.json` | sha256 `c8abc04a…796bfc`, `fcec87e7…3875554c` (dimensionless topology only) |
| Canon row | DevelopmentBible.md line 514, `SPEC-BLD-015.MC.LINK` | file sha256 `e237a4e1…3cfe8c` |
| Book | paragraphs 135 and 169 (numbered ceramic skins, held conduit, load collar; relay collar click) | `book-source.docx` sha256 `994e7df5…17ef83` |
| Gameplay record | `Content/Data/Source/buildings.json` `mc_power_link`: 2×2 cells, 450 HP, 500 cm sight, 100 ticks, logistics 6 | sha256 `aba1b64b…6ed26e` |
| Requirements | `SPEC-STR-002`, `SPEC-BLD-015.MC.LINK`, `REL-BLD-015.MC.LINK.ASSET` (LOD0 ≤3,500 / LOD1 ≤1,200 tris, 1024² PBR, non-color grid markings, static, degradation below 30% HP), `REL-ART-028`, `REL-ART-004` (≤15% emissive area), `SPEC-VISD-003`, `SPEC-ART-001..004`, `SPEC-VISD-008`, `SPEC-CMB-009`/`REL-BLD-014` (destruction clearance, 200-tick cosmetic debris) | Requirements.md at worktree HEAD |
| Unreal references | EBS-UE-001/002/003/007/008/011/012/014/015/016/029/033/039/040 | [unreal-references.json](../../Docs/VisualAssetPipeline/unreal-references.json) |

Authority conflicts touching this package: none of the three recorded conflicts (Future Well,
Resonant, Phase Anchor) involve the Power Link. The six prepared master amendments remain unapplied;
`DESTRUCTION-CLEARANCE` and `MERIDIAN-BUDGETS` describe the future direction this source already
respects (immediate authoritative clearance; 8,000/3,500 global ceiling with the tighter 3,500/1,200
card applying here).

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Power Link is the Compact's supply node: it extends the powered build area, Matter
drop-off coverage and Logistics along a chain of pylons from the Anchor (`SPEC-STR-002`, Bible
"Power links and supply nodes make production, repair, sensors and long-range support reliable within
the network"). It is maintained inheritance: a pylon that a crew who never met its builder can strip
to numbered panels, loosen at the load collar and re-plug at the base (book ¶135). It belongs beside
every Compact base in M01–M15 and skirmish; what must be absent is anything that reads as a weapon,
a reactor, a trophy crystal or an unmaintainable monolith. No lore, capability or consequence beyond
the master is introduced here; network radius, connection state and health remain simulation-owned.

**DETAIL.** Large scale: a slim vertical column (about 3.9× its base width) on a low charcoal plinth,
one conductor collar at roughly three-quarters height, two base couplings on the left and right flanks
with paired conduits leaving the footprint. This silhouette is what reads at 3,800 uu / 45–60° tilt.
Medium: four numbered ceramic access panels on the service face, four exposed corner rails, horizontal
seams between panel rows, the foundation ring, pale ceramic cladding on the plinth flanks, the rear
redundant conduit pair with clamps. Fine (close camera only): fastener bosses, numeral label plates,
port collars, indicator strips, the internal conduit bundle behind panels 02/03. Material logic:
`ceramic_civic` skins over a charcoal load frame (Bible "How they build"); wear belongs at panel
edges, the collar and coupling ports where hands and loads have been.

**ACTION.** Visible states: construction, operational-connected, operational-disconnected,
interrupted/recovery, damaged (a collar segment dark, a conduit hanging, panels off), critical
degradation (<30% HP), destruction (engineered collapse; authoritative occupancy clears immediately;
cosmetic debris ≤200 ticks), restore. Motion: none of the primary mass moves; connection gain/loss
drives collar and conduit pulse; panels and conduit stubs are separate parts so damage/maintenance
can remove or hang them without deforming the mast. Sound: the Bible's thin electrical sustain only
while connected, at `Collar_Center`; silence otherwise is intentional. Accessibility: connection
state is carried by luminance (lit vs unlit collar segments, verified in the monochrome render) and
by geometry (hanging conduit, missing panels), never by hue alone; reduced motion holds the pulse
steady. Boundaries: the cosmetic conduit span, debris and effects never own collision, navigation,
selection or network authority (GAP-03).

**REVIEW.** Internal review of these fields and their traceability was performed against the sources
in §1 before geometry was authored (this document, 2026-09-06). Open items are in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 2×2 cells = **400 × 400 cm** | `buildings.json` cells; `TileWorldSize = 200.0f` in `Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h`, `kPresentationTileWorldUnits = 200` |
| Units / axes / pivot | centimeters; +X forward (service face), +Y right, +Z up; pivot at ground-contact centre | contract `import_policy` |
| Height | **1,238 cm — PROVISIONAL BLOCKOUT ESTIMATE** | No authority states a height. The proportions are the contract's construction reference (`motion/construction/component-geometry.json`, the GAP-01 geometry source: base 1.0, shaft 0.46 wide, top at 3.8, collar 2.77–2.93, panels 0.40 × 0.78) mapped to a 320 cm base unit; the contract excludes painted measurements. The painted candidate is about twice as stocky (column ≈2.6–2.8:1, total ≈2.0–2.4× base). ComponentDesignCatalog's "18-metre hexagonal pylon" is a subordinate proposal, not adopted. |
| Noted discrepancy | `SPEC-SKM-011` says "64×64 tiles at 100 cm simulation scale" while presentation places tiles at 200 cm | The mesh follows the presentation tile constant the runtime uses to place structures; recorded, not resolved here. |

## 4. Geometry (revision `ebs-mer-bld-002-blockout-v3`)

v1 → v2 changed only the glTF socket-node encoding. v2 → v3 (after the internal gate review, §6.2): plinth
widened to 360 cm so the base announces the 2×2 footprint on both axes; panels narrowed to 100 × 200 cm so
they sit inside the 106 cm flat face without touching the corner rails; a ceramic **team band** (stripes on
the cladding plates and a band under the collar) added as the ownership-colour carrier for the texture's
team mask; the two status lights the candidate does not show (cap tell-tale, bay indicator) removed; the
rails stop 2 cm below the cap plane and the ports stand 2 cm proud of the couplings (no coincident faces);
the maintenance state hangs the unplugged conduit from its port instead of detaching it; unused glTF
materials dropped and collision meshes given a material index; unique UV0 atlas added (§4.1).

Deterministic generator: [build_power_link.py](build_power_link.py) on [../tools/ebs_meshkit.py](../tools/ebs_meshkit.py)
(Python 3 standard library; no DCC application exists on this workstation, so the generator is the
editable native source). Exact dimensions, counts, hashes and bounds are in
[build-manifest.json](build-manifest.json); regeneration under an unchanged revision is byte-identical
(`--check`).

| Contract component | Built | Where |
|---|---|---|
| 4 numbered panels (01 top → 04 bottom) | 4 | part mesh `SM_EBS_MER_BLD_002_Panel` at sockets `Panel_01..04` on the +X service face |
| 1 collar assembly | 1 | octagonal ring at z 880–950 with 8 cyan conductor segments and 4 gussets |
| 2 base couplings | 2 | `coupling_left` (−Y) and `coupling_right` (+Y), two ports each, ceramic top plate, indicator strip |
| 4 conduits | 4 | part mesh `SM_EBS_MER_BLD_002_ConduitStub` at sockets `Conduit_Left_01/02`, `Conduit_Right_01/02`; 90 cm physical stubs, `Span_End` socket for the cosmetic span |
| Maintenance opening (GAP-02) | yes | service bay behind panels 02/03 with an internal three-conduit bundle |
| Canon extras | rear redundant conduit pair with clamps and junction; four exposed corner rails; foundation ring; plinth cladding | Bible "conduits run in redundant pairs on the outside of walls", `REL-ART-028` load frames |

| Mesh | LOD0 tris | LOD1 tris | Slots |
|---|---|---|---|
| `SM_EBS_MER_BLD_002` (main) | 1,180 | 876 | ceramic / frame / status |
| `SM_EBS_MER_BLD_002_Panel` (×4) | 136 | 12 | ceramic / frame |
| `SM_EBS_MER_BLD_002_ConduitStub` (×4) | 84 | 64 | frame / status |
| **Assembled** | **2,060 ≤ 3,500** | **1,180 ≤ 1,200** | 3 provisional slots |

Material slots: `MI_EBS_MER_CeramicCivic` (existing `T_EchoesCeramicCivic` family),
`MI_EBS_MER_CompactFrame` (charcoal machined metal), `MI_EBS_MER_StatusCyan` (state-masked emissive
for collar segments, indicator strips, conduit pulse strips, bay indicator, cap tell-tale).
UV0 is a unique, non-overlapping atlas shared by the main mesh and both parts at both LODs (§4.1); UV1 is
a non-overlapping per-polygon cell layout reserved for lightmaps. Simple collision: two `UBX_` boxes
(plinth, mast) for asset inspection only; the runtime presentation component disables collision.

Sockets on the main mesh: `Panel_01..04`, `Conduit_Left_01/02` (yaw −90°), `Conduit_Right_01/02`
(yaw +90°), `Collar_Center`, `Cap_Top`, `Bay_Center`. Socket names are the contract's selected
adapter names; they become real Unreal sockets only at import.

Export recipe: OBJ in the Unreal frame (renderer container); GLB in the glTF frame using the inverse
of the installed 5.8.2 Interchange conversion `UE = (X_g, Z_g, Y_g)` (so `glTF = (X, Z, Y)/100`,
metres), triangle winding re-derived per triangle from the stored outward normal. Sockets are
`SOCKET_<name>` child nodes of the mesh node (Interchange attaches sockets by parent chain when a file
holds more than one mesh, and collision boxes are meshes), encoded with node scale (−1, 1, 1) and
rotation `q_y(−yaw)·(q_y(180)·q_x(−90))` to cancel the reflection that `ImportSockets()` applies
through the axis-conversion inverse; the encoding was established and verified by probe imports
(`import/probe-sweep`, `import/probe-verify` in the evidence root). Collision boxes are `UBX_<mesh>_NN`
sibling nodes. LOD1 is a separate GLB attached at import.

### 4.1 UV atlas and bake manifest

`pack_atlas` in the mesh kit gives every polygon its own upright planar chart (walls read top-down),
deduplicates LOD0/LOD1 twins, and shelf-packs the charts at one uniform texel density into a 1024²
atlas; the numeral label plate gets a four-cell strip so one panel mesh can show 01–04 through a
material cell offset. The packing is recorded in [bake-manifest.json](bake-manifest.json) (chart rects,
world frames, families, decal rules) and is the input of the texture baker
([../tools/ebs_texbake.py](../tools/ebs_texbake.py)). Density and coverage are in the build manifest.

### 4.2 Textures and material plan (material-and-state half of the pilot)

Baked from the atlas by [../tools/ebs_texbake.py](../tools/ebs_texbake.py) (pure Python, world-space
sampling of the project's registered `ceramic_civic` / `compact_metal` recipes so adjacent charts stay
continuous; report in `textures/bake-report.json` with per-map hashes and channel statistics):

| Map (1024², 8-bit) | Channels | Use |
|---|---|---|
| `T_EBS_MER_BLD_002_BaseColor` | sRGB colour: pale ceramic, charcoal machined frame, cyan status elements; numeral strip 01–04 (4 cells) on the label plate; non-colour grid markings and edge wear on the panel plates; bottom-20 cm scuff on walls | base colour |
| `T_EBS_MER_BLD_002_Normal` | tangent-space, Unreal/DirectX green-down; bevels, machining, wear pits | normal |
| `T_EBS_MER_BLD_002_MRE` | R metallic (ceramic 0.04, frame 0.85), G roughness (ceramic 0.34 rising with wear, frame 0.45–0.6), B emissive mask (status elements) | the project's `_MRE` packing (`echoes_texture_synth.py`) |
| `T_EBS_MER_BLD_002_StateMask` | R collar-segment id band (k/8 per segment 1–8), G indicator / conduit-pulse strips (pulse strip carries a 0→1 gradient along its length), B team-band mask | state and ownership drive |
| `T_EBS_MER_BLD_002_AtlasDebug` | chart rectangles by slot with checkers | in-engine UV verification only |

Material contract for the integration task (the existing `M_EchoesSurface` master exposes `Color`,
`Metallic`, `Roughness`, `EmissiveStrength`, `UVScale` and one family's BaseColor/MRE/Normal samplers;
a production master or instance family adds the per-asset samplers and these parameters):

| Parameter | Type | Driven by |
|---|---|---|
| `BaseColor`, `Normal`, `MRE`, `StateMask` | texture | the four maps above; same maps on all three slots, slot difference is which mask channels apply |
| `TeamColor` | vector | owner colour (the runtime already sets `Color` on presentation materials); applied where StateMask.B > 0 |
| `Connected` | scalar 0/1 | authoritative network connection: collar segments (R band) and G strips lit only when 1 |
| `CollarSegments` | scalar 0–8 | optional progressive lighting of segments 1..k during connection gain / loss |
| `PulsePhase` | scalar | conduit pulse travelling along the G gradient while `Connected`; held steady under reduced motion |
| `PanelCell` (per panel instance) | scalar 0–3 | selects the numeral cell 01–04 by offsetting the label-plate UVs |
| `Damage` | scalar 0–1 | darkens one collar segment and raises roughness/soot where MRE.B == 0 below the 30% critical threshold (`REL-ART-027`) |
| `ConstructionProgress` | scalar 0–1 | world-Z clip of the ceramic slot during construction so the frame rails and plinth show first |

None of these parameters exist in the game yet; they are the presentation interface this asset was
authored against, and every value must be read from authoritative state (event rule).

## 5. States and required tracks (static structure; `REL-BLD-015.MC.LINK.ASSET` .ANIM_RIG = NOT APPLICABLE)

| Track (contract) | Presentation plan | Authority read |
|---|---|---|
| construction | staged reveal: plinth → frame rails → shaft rows → collar → panels, driven by authoritative progress; construction frame is the charcoal frame slot | `SPEC-BLD-004` progress |
| operational / connection_gain / connection_loss | collar segments, coupling strips and conduit pulse strips lit (connected) or unlit (disconnected) within one tick of the authoritative change | network connection state |
| interrupted / recovery | reveal pauses without decay; resumes from current progress | construction pause state |
| damage | below thresholds: a collar segment dark, one conduit stub hangs at its coupling socket, panels 02/03 removed exposing the bay | health bands |
| destruction | authoritative removal and footprint clearance immediately; cosmetic collapse debris ≤200 ticks | `SPEC-CMB-009`, `REL-BLD-014` (amendment `DESTRUCTION-CLEARANCE` pending) |
| restore | rebuild from current state; no one-shot replay; audio sustain only if connected | replay/load |

None of these are implemented yet; they are the plan the part/socket split and the status slot were
authored for. The candidate maintenance state is already representable (see renders).

## 6. Review evidence (blockout stage)

Evidence root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-BLD-002/`
(`receipt.json` inventories every file with SHA-256; `review/` holds the assembled OBJs;
`scenes/` the renderer inputs; `renders/<state>/` the PNGs and `render-manifest.json`).

Rendered with [../tools/ebs_render.py](../tools/ebs_render.py) (pure Python, Unreal camera
conventions, adversarially verified against the engine headers): orthographic front/right/rear/left/top
with feature edges, and tactical views reproducing the game camera (`EchoesRTSCameraPawn.cpp`: spring arm
3,800 uu at pitch −48° default / −60° gameplay, yaw −45°; `Camera->ProjectionMode = Orthographic` with
`OrthoWidth = 2·arm·tan(55°/2) = 3,956 cm`; with the engine's default `AspectRatio_MaintainYFOV` and no
project override that width spans the image height, so the 16:9 frame covers 7,033 × 3,956 cm), plus
far zoom (7,600 uu), a reverse angle, a monochrome pass, a four-pylon crowd scene and the maintenance
and disconnected states. The frame-axis behaviour is derived from engine source, not yet confirmed by
an in-editor capture; a first perspective-projection pass was superseded by these orthographic renders.

Checks performed:

- Component counts, panel order, bay span, socket pairing/orientation, budgets, footprint containment,
  pivot, slot count, glTF axis rule, glTF winding, OBJ frame, deterministic bytes: 15 tests in
  [test_power_link_build.py](test_power_link_build.py), all passing.
- Emissive pixel share of the mesh area (unlit render, magenta-keyed ground, exact colour counts): 2.0%
  default tactical, 2.2% gameplay tactical, 2.5% far (6,200 uu), 1.6% front orthographic — under the 15%
  ceiling (`renders/area_check/emissive-area.json`; the first method counted the lit ground and was wrong).
- Visual inspection of every render by the author: silhouette, four panel rows, lit collar, couplings
  and stubs read at both tactical pitches with the pylon as pale ceramic over a charcoal base; the base
  separates from charcoal ground only after the ceramic cladding was added; the maintenance state shows the
  open bay, both removed panels and the conduit hanging from its port (`maintenance/tactical_near.png`,
  `tactical_service_face.png`).
- Monochrome pass: connection state remains readable by luminance alone.

What this evidence is not: no in-engine render, no crowded combat scene with units, no fog, no
selection halo, no textures, no measured performance, no human recognition test, no owner review.

### 6.2 Internal gate review (three lenses, adversarial refutation)

An independent three-lens review (art fidelity, technical contract, gameplay readability) of the v1/v2
package, with every finding attacked by two skeptics, is retained as
`gate-review-blockout-internal.json` in the evidence root (internal QA, not a gate pass). Disposition:

| Finding (upheld unless noted) | Disposition in v3 |
|---|---|
| Tactical renders used a perspective camera; the game camera is orthographic | Renderer verified against the engine headers and re-based on the orthographic spring-arm framing (§6). |
| Emissive-area method counted the lit ground plane | Unlit magenta-keyed method; share 2.0–2.5% at tactical framing, 1.6% front (`renders/area_check/emissive-area.json`). |
| Ceramic mast read as charcoal (far-side key light, no sRGB) | Key light from the camera side; sRGB output; the pylon now reads pale ceramic over a charcoal base. |
| Unplugged conduit detached from its coupling (GAP-01 traceability) | Hangs from its port at −30° pitch to the ground. |
| No ownership/team-colour carrier | Ceramic team band (cladding stripes, under-collar band) masked in the texture's B channel. |
| Sockets absent / mis-rotated after import; LOD1 sections on slot 0 | Fixed in v2 (encoding, probes) and the LOD1 slot restore; all checks pass (§6.1). |
| Base under-announces the footprint on X; panel wider than the flat face; coincident faces; far zoom 7,600 unreachable (max 6,200); unused glTF material; extra status lights | Plinth 360; panel 100 wide; rails/ports offset; far view at 6,200; materials trimmed; cap tell-tale and bay indicator removed. |
| Mast proportion "twice the candidate" — **refuted** by both skeptics | The blockout matches the contract's construction reference (3.8:1), which GAP-01 names as the geometry source; the painting is ~2× stockier. Recorded as an owner question (§8). |
| Service face invisible at default placement — refuted as already recorded | Unchanged; see §8 decision 1. |

### 6.1 Isolated Unreal import (installed 5.8.2, sandbox project)

Evidence: `import/` under the evidence root — `heavy-run-receipt.json` (reservation and outcome, runs
1–6), `import-job.json`, `import-report.json` (final clean run of the v3 exports), `UnrealEditor-Cmd-import-run*.log`,
and the socket probes `probe/`, `probe-sweep/`, `probe-verify/` that established the socket encoding.
Sandbox: `IsolatedPreview/EBSPreview/EBSPreview.uproject` (Blueprint-only, PythonScriptPlugin +
Interchange, Nanite/VSM off); the shared game project was not opened. Tool:
[../tools/ue_import_inspect.py](../tools/ue_import_inspect.py) run through `UnrealEditor-Cmd -nullrhi
-unattended -ExecutePythonScript`, importing with an explicit `InterchangeGenericAssetsPipeline`
(static meshes only, collision by mesh name, sockets on, Nanite off, no generated lightmap UVs, normals
not recomputed, materials not imported) passed through `InterchangePipelineStackOverride`, then LOD1
through `StaticMeshEditorSubsystem.import_lod` with a by-name section-to-slot restore.

| Check (final clean run) | Main | Panel | Conduit stub |
|---|---|---|---|
| Scale/axes: bounds max (cm) | (188, 200, 1238) | (7.5, 50, 100) | (92, 17, 17) |
| LOD0 / LOD1 triangles vs export | 1,180 / 876 ✓ | 136 / 12 ✓ | 84 / 64 ✓ |
| Sections → slots LOD0 / LOD1 | [0,1,2] / [0,1,2] | [0,1] / [0] | [0,1] / [0,1] |
| UV channels, lightmap index | 2, index 1 | 2, index 1 | 2, index 1 |
| Sockets (name, location, rotation, scale) | 11, all exact, unit scale | `Label` | `Span_End` |
| Simple collision | 2 boxes (UBX) | 0 | 0 |
| Nanite | off | off | off |
| Replace-reimport | identical counts/bounds/sockets | identical | identical |

Importer facts established for this engine build: glTF metres import at 1 m = 100 cm; sockets attach
only through the mesh node's parent chain when a file holds more than one mesh; `ImportSockets()`
applies the axis-conversion inverse (a reflection) to socket transforms, so a plain node imports with
rotator (0, 180, −90) and X scale −1 — the kit's encoding cancels it (verified at yaw 0/45/90/−90/180);
the custom-LOD import path merges any `UBX_` mesh into the LOD and maps all sections to slot 0, hence
collision only in the LOD0 file and the by-name slot restore. Limits: no in-engine render (`-nullrhi`),
default lightmap resolution (4) and LOD screen sizes untouched, materials left as the engine default
because the production material instances do not exist yet, and "reimport" here means replace-import
of the same file, not the asset's Reimport action.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-BLD-002"
python3 build_power_link.py --evidence-dir "<evidence root>/EBS-MER-BLD-002"
# isolated import (exclusive editor run; see heavy-run receipt):
# EBS_IMPORT_JOB=<root>/EBS-MER-BLD-002/import/import-job.json "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#   <root>/IsolatedPreview/EBSPreview/EBSPreview.uproject -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
#   -ExecutePythonScript=../tools/ue_import_inspect.py -abslog=<root>/EBS-MER-BLD-002/import/UnrealEditor-Cmd-import.log
python3 build_power_link.py --evidence-dir "<evidence root>/EBS-MER-BLD-002" --check
python3 test_power_link_build.py
python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-MER-BLD-002/scenes/connected.json" --out "<evidence root>/EBS-MER-BLD-002/renders/connected"
```

## 8. Defects, decisions and remaining work

Decisions made under the owner's delegation (routine implementation choices):

1. Service face on +X (contract "+X forward"). The default RTS camera (yaw −45°) sees the −X and +Y
   faces, so at default placement the numbered panels face away; the collar, couplings and conduits
   carry the identity from every side. The integration task may spawn Power Links with yaw 180° or
   rely on `SPEC-BLD-001` rotation. Recorded, not changed here.
2. Height 1,238 cm follows the contract's construction reference (3.8 × base). The selected painting is
   about twice as stocky and the other lane's settlement study also draws a shorter pylon; the reviewers'
   occlusion estimate is ~4–5 tiles of ground strip behind the mast at the default pitch (the orthographic
   frame is 7,033 cm wide, so the mast itself is ~1% of the frame). **OWNER-QUESTION:** keep the slim
   3.8:1 reference proportion (canon "slim pylon", book "slender pylons") or move toward the painted
   candidate's ~2.2× base? Height is one constant in the generator; UVs and sockets regenerate.
3. Panels and conduit stubs are separate part meshes (precedent: the M01 Surveyor/Bulwark articulation
   parts in `Scripts/generate_art_assets.py`) so damage/maintenance states move geometry rather than
   faking it with lights.
4. The isolated source is a deterministic generator, not a DCC file, because no DCC application is
   installed; the glTF export is the exchange format for Interchange.

Open items:

- LOD1 sits at 1,190 of 1,200 triangles; any LOD1 addition must remove something else.
- Textures (1024² base colour / normal / packed MR plus the state and numeral mask), material
  instances against the existing master materials, and the non-colour grid markings are the next
  stage (ART_ALPHA prerequisites).
- In-engine rendered inspection (Static Mesh Editor and a lit level at the game camera) still needs a
  GPU editor run; the headless import above proves structure, not appearance.
- Socket encoding is importer-build-specific (5.8.2): re-run `socket_probe.py` after any engine change.
- Confirm the orthographic frame axis (7,033 × 3,956 cm at the 3,800 preset) with one in-editor capture
  of `AEchoesRTSCameraPawn` before using these renders for framing or occlusion decisions.
- Gameplay proxy integration, three-gate review, and owner acceptance remain outside this lane.
