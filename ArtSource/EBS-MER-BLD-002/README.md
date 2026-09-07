---
title: EBS-MER-BLD-002 Power Link — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
package: EBS-PKG-MC-POWER-LINK
production_asset_id: EBS-MER-BLD-002
production_maturity: BLOCKOUT
revision: ebs-mer-bld-002-concept-v3
canon_status: CANDIDATE (delegated design selection; not owner acceptance)
concept_target: concept-fidelity.md (AUTHORITATIVE, owner ruling 2026-09-06: the concepts define what the asset looks like; §Proportion 1/3/6 amended 2026-09-07 from the candidate pixels)
status: Isolated production source rebuilt to the concept images and re-proportioned from the candidate pixels; no Unreal integration authorization
---

# EBS-MER-BLD-002 Power Link — production source

This folder is the single authoritative source record for the Power Link production asset. It is
edited in place; Git retains history. **The look is bound to [concept-fidelity.md](concept-fidelity.md)**
(owner ruling 2026-09-06: the selected concept images define what every asset looks like; gameplay rules
bound the concept but never replace it; every forced deviation is written down in §8 with the rule cited). Everything here is bounded by the shared
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

**DETAIL.** Large scale: a pale octagonal ceramic column 3.9× its shaft width (2.2× the plinth) on a
low two-step charcoal plinth, one broad conductor collar at 0.72 of the height, two coupling blocks
standing at the front-left and front-right plinth corners with paired conduit stubs leaving toward the
footprint edge. This silhouette is what reads at 3,800 uu / 45–60° tilt. Medium: four near-square
numbered ceramic access panels on the service face (01 above the collar, 02–04 below), thin charcoal
ribs on the four chamfers, the three collar courses, the cap plate, the pale insulator rings that
segment the conduits. Fine (close camera only): fastener bosses, centred numeral cells, port collars,
indicator strips, the fitted conduit bundle behind panels 02/03. Material logic: `ceramic_civic` skins
over a charcoal load frame (Bible "How they build"); wear belongs at panel edges, the collar and
coupling ports where hands and loads have been.

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
| Height | **780 cm — CONCEPT MEASUREMENT** (shaft W = 200 cm, H = 3.90 W) | `concept-fidelity.md` §Proportion 1 (amended 2026-09-07): the candidate's tower measures 3.55 raw / 3.5–4.2 W pitch-corrected in the main view and 3.4–3.7 raw in the disconnected view; the plinth announces the 2×2 footprint at 360 cm = 1.8 W (main view 2.0, disconnected 1.5), so W = 200 and the cap top lands at 780. This replaces the concept-v2 850 cm at W 180 (4.72 W, from the doc's earlier 4.6–5.0 band that the pixels do not reproduce) and the blockout-v3 construction-reference height of 1,238 cm at a 150 cm shaft (8:1); the construction reference still governs component order (topology), not proportion. |
| Noted discrepancy | `SPEC-SKM-011` says "64×64 tiles at 100 cm simulation scale" while presentation places tiles at 200 cm | The mesh follows the presentation tile constant the runtime uses to place structures; recorded, not resolved here. |

## 4. Geometry (revision `ebs-mer-bld-002-concept-v3`)

**Concept rework.** `concept-v1` rebuilt the asset to the concept images (octagonal section, four
numbered panels **01 above the collar, 02–04 below it**, ONE broad three-course load collar with four cyan
strips, a low octagonal cap with a square fastener plate and no crystal or antenna, a two-step charcoal
plinth with two coupling blocks at the front corners, conduit stubs that end on the footprint edge; the
v3 foundation ring, plinth cladding, rear conduit pair, four shaft rows and grooves were dropped because
the concepts do not show them). `concept-v2` was the pixel pass against the comparison sheets (thicker
stubs with clamp rings, larger ports, couplings to the fidelity size, centred numeral cells, narrower
chamfer rails, wider collar strips). `concept-v3` re-derives the proportion from the candidate pixels
(§8.1 D10): shaft W = 200 cm, cap top 780 cm (3.90 W; measured band 3.5–4.2 W), plinth 360 = 1.8 W,
upper step 260 = 1.3 W, collar 270 = 1.35 W at 0.724 H, panels 124 × 135 cm (0.92 aspect, the four
plates cover 93 % of the shaft height); conduits Ø 32 = 0.16 W measured against the across-flats width
(§8.1 D2, `concept-fidelity.md` §6 amended) with pale ceramic insulator rings and end nuts; couplings
0.9 × 0.37 × 0.5 W on the lower step, hanging 20 cm past the plinth-top edge / chamfer and 12 cm past the
plinth's ground edge at the front (§8.1 D8); the open bay's bundle carries pale clamp rings and a pale
cross-bar; the maintenance hanging conduit clears the step edge and reaches the ground through a second,
chained instance of the same stub part (§8.1 D3 — the concept-v2 single stub at −70° ended INSIDE the
lower step at z ≈ 30, which is why it never read); `--check` is read-only (temporary build, full
comparison). Blockout history (v1 socket encoding, v2 → v3 gate dispositions) is in Git and in §6.2;
the v3 import evidence in §6.1 predates the concept rework.

Deterministic generator: [build_power_link.py](build_power_link.py) on [../tools/ebs_meshkit.py](../tools/ebs_meshkit.py)
(Python 3 standard library; no DCC application exists on this workstation, so the generator is the
editable native source). Exact dimensions, counts, hashes and bounds are in
[build-manifest.json](build-manifest.json); regeneration under an unchanged revision is byte-identical
(`--check` rebuilds into a temporary directory and compares every export, review assembly and atlas hash
without writing into the package or the evidence directory).

| Contract component | Built | Where (concept-v3) |
|---|---|---|
| 4 numbered panels (01 above the collar, 02–04 below) | 4 | part mesh `SM_EBS_MER_BLD_002_Panel` (124 × 135 × 6 cm, 0.173 H, aspect 0.92) at sockets `Panel_01..04` on the +X service face, centres z 678 / 447 / 306 / 165 (6 cm seams); four corner fastener bosses; centred 56 × 44 cm numeral cell (`Label` socket) |
| 1 collar assembly | 1 | octagonal collar 270 cm across (1.35 W) at z 532–598 (0.724 H, 0.085 H tall) in three courses with two grooves; four cyan strips 106 × 20 cm on the middle course of the wide faces (0.59 of the 180 cm face) |
| 2 base couplings | 2 | `coupling_left` (−Y) / `coupling_right` (+Y): 180 × 74 × 100 cm closed blocks (0.9 / 0.37 / 0.5 W) on the lower step (z 50–150) at x 12–192, outer face at |y| 140, inner face at |y| 66 (clears panel 04); two Ø 40 ports each at z 100 and a port status lamp at z 138 |
| 4 conduits | 4 | part mesh `SM_EBS_MER_BLD_002_ConduitStub` (Ø 32 cm = 0.16 W, 56 cm charcoal jacket + 2 cm pale end nut, two pale clamp rings, a pulse window between them) at sockets `Conduit_Left_01/02` (x 68 / 136), `Conduit_Right_01/02`; `Span_End` lands exactly on the footprint edge (|y| 200) |
| Maintenance opening (GAP-02) | yes | service bay behind panels 02/03 (z 238.5–514.5, 100 wide, 30 deep) with an internal three-conduit bundle: charcoal tubes, pale ceramic clamp rings and cross-bar (`service_bay_fittings`) |
| Frame | 4 chamfer rails (14 cm wide, 4 cm proud, below and above the collar), 4 foot brackets, charcoal panel backing quads, two-step plinth with a chamfered skirt, cap plate | Bible "How they build" (ceramic skins over a charcoal load frame); `REL-ART-028` |
| Team band | 1 | narrow ceramic band at z 521–529 under the collar (team-mask carrier, `REL-ART-028`); not in the concepts, see §8 |

| Mesh | LOD0 tris | LOD1 tris | Slots |
|---|---|---|---|
| `SM_EBS_MER_BLD_002` (main) | 858 | 426 | ceramic / frame / status |
| `SM_EBS_MER_BLD_002_Panel` (×4) | 86 | 12 | ceramic / frame |
| `SM_EBS_MER_BLD_002_ConduitStub` (×4) | 110 | 42 | frame / status / ceramic |
| **Assembled** | **1,642 ≤ 3,500** | **642 ≤ 1,200** | 3 provisional slots |

Measured: main-mesh bounds (−180, −180, 0) → (192, 180, 780) cm, i.e. **780 cm tall, 360 cm across the
plinth on Y and 372 cm on X** (the coupling feet reach x 192) inside the 400 × 400 footprint; the stubs
reach exactly |y| = 200. Ratios in `build-manifest.json` `concept_proportions`: H/W 3.90 (target 3.8–4.0,
measured band 3.5–4.2), plinth 1.8 W, collar centre 0.724 H, collar 1.35 W wide, panels 0.173 H and 0.92
aspect, plinth 0.118 H, coupling 0.9 / 0.37 / 0.5 W (overhang 20 cm past the plinth-top chamfer, 12 cm
past the ground edge), conduit 0.16 W, numeral cell 0.45 × 0.33 of the plate.

Material slots: `MI_EBS_MER_CeramicCivic` (existing `T_EchoesCeramicCivic` family),
`MI_EBS_MER_CompactFrame` (charcoal machined metal), `MI_EBS_MER_StatusCyan` (state-masked emissive
for the four collar strips, the two port status lamps and the four conduit pulse windows). The stub part
now carries all three slots (pale ceramic insulator rings and end nut on a charcoal jacket).
UV0 is a unique, non-overlapping atlas shared by the main mesh and both parts at both LODs (§4.1); UV1 is
a non-overlapping per-polygon cell layout reserved for lightmaps. Simple collision: two `UBX_` boxes
(plinth incl. the coupling feet, mast) for asset inspection only; the runtime presentation component
disables collision. The assembled REVIEW OBJs (not exports) write the numeral label cells under a
review-only material `REVIEW_LabelCell` so the untextured renders show the pale label the bake paints;
the exports keep the cells on the frame slot, which is the slot family the baker paints the numerals under
(§8.1 D4).

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
atlas (concept-v3: 495 charts at 0.604 px/cm, 70.1% used); the numeral label plate gets a four-cell strip
(4 × 34 × 27 px) so one panel mesh can show 01–04 through a material cell offset. The packing is recorded in [bake-manifest.json](bake-manifest.json) (chart rects,
world frames, families, decal rules) and is the input of the texture baker
([../tools/ebs_texbake.py](../tools/ebs_texbake.py)). Density and coverage are in the build manifest.

### 4.2 Textures and material plan (material-and-state half of the pilot)

Baked from the atlas by [../tools/ebs_texbake.py](../tools/ebs_texbake.py) (pure Python, world-space
sampling of the project's registered `ceramic_civic` / `compact_metal` recipes so adjacent charts stay
continuous; report in `textures/bake-report.json` with per-map hashes and channel statistics):

| Map (1024², 8-bit) | Channels | Use |
|---|---|---|
| `T_EBS_MER_BLD_002_BaseColor` | sRGB colour: pale ceramic, charcoal machined frame, cyan status elements; numeral strip 01–04 (4 cells, pale label with 19 px dark seven-segment glyphs) on the label plate; non-colour grid markings and edge wear on the panel plates; bottom-20 cm scuff on walls | base colour |
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
| damage | below thresholds: a collar segment dark, one conduit stub unplugged and hanging from its coupling socket with a second stub instance chained from its `Span_End` to the ground, panels 02/03 removed exposing the bay | health bands |
| destruction | authoritative removal and footprint clearance immediately; cosmetic collapse debris ≤200 ticks | `SPEC-CMB-009`, `REL-BLD-014` (amendment `DESTRUCTION-CLEARANCE` pending) |
| restore | rebuild from current state; no one-shot replay; audio sustain only if connected | replay/load |

None of these are implemented yet; they are the plan the part/socket split and the status slot were
authored for. The candidate maintenance state is representable with the exported parts (see renders);
the shed panels of that state lie outside the 2×2 footprint as cosmetic debris (§8.1 D11).

## 6. Review evidence (concept rework; blockout-stage import history retained)

Evidence root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-BLD-002/`
(`receipt.json` inventories every file with SHA-256; `review/` holds the assembled OBJs;
`scenes/` the renderer inputs; `renders/<state>/` the PNGs and `render-manifest.json`;
`renders/concept-compare/` the concept-versus-render sheets of §6.4). Every render folder was regenerated
from the concept-v3 review geometry: 55 views in nine scenes, every PNG listed in its folder's
`render-manifest.json` (the two blockout-v3 leftovers `connected/tactical_reverse.png` and
`maintenance/tactical_gameplay.png` that the concept-v2 receipt had inventoried were deleted; the
maintenance gameplay framing is now a manifest view). The `import/` folder is the blockout-v3 import run
(§6.1) and has NOT been repeated for the concept revisions.

### 6.4 Concept comparison sheets and fidelity checklist (concept-v3)

Sheets composed with [../tools/ebs_sheet.py](../tools/ebs_sheet.py) from the hash-verified concept crops
(`<evidence root>/concept-crops/`) and the `concept_connected` / `concept_disconnected` /
`concept_maintenance` scenes (candidate-sheet elevation: front-left and front-right at −25°, arm 1,000 cm;
orthographic front, left and collar close-ups; the game camera at the 1,400 uu near preset; a 560 cm base
close-up). The disconnected concept scene renders the status strips dark grey (`Connected` = 0), as painted.

| Sheet | Concept side | Render side |
|---|---|---|
| `01-front-quarter.png` | candidate, connected | `concept_connected/front_quarter` |
| `02-service-face.png` | candidate tower | `concept_connected/front` (ortho +X) |
| `03-collar.png` | candidate collar; concept A | `concept_connected/collar_close` |
| `04-maintenance.png` | candidate, maintenance/damage | `concept_maintenance/front_quarter`, `front_quarter_right` |
| `04b-maintenance-complete.png` | maintenance-complete image | `concept_maintenance/front_quarter`, `tactical_near_gameplay` |
| `05-tactical.png` | concept B pylon | `concept_connected/tactical_near_gameplay`, `connected/tactical_default`, `crowd/tactical_default` |
| `06-disconnected.png` | candidate, disconnected | `concept_disconnected/front_quarter`, `tactical_near_gameplay` |
| `07-side.png` | candidate, connected | `concept_connected/left` (ortho −Y) |
| `08-base-couplings.png` | candidate base | `concept_connected/front_quarter_right`, `connected/tactical_near` |
| `09-base-close.png` | candidate base | `concept_connected/base_close`, `concept_maintenance/base_close` |

Fidelity checklist (`concept-fidelity.md` §Fidelity checks), judged on the pixels of those sheets:

| Check | Result | Evidence |
|---|---|---|
| Front-quarter: octagonal ceramic tower ≈ 3.8–4 W tall (measured band 3.5–4.2) on a two-step plinth with two couplings | **MET** | `01-front-quarter.png`: pale octagonal shaft at H/W 3.90 with thin dark chamfer ribs, one dark collar with lit strips, low cap with a pale square plate, two-step charcoal plinth, coupling blocks standing at both front corners with segmented stubs (pale insulator rings) leaving sideways. Candidate measurement in `concept-fidelity.md` §Proportion 1; the concept-v2 tower (4.72 W) was 20–35 % more slender than the painting (§8.1 D10). |
| Service face: panels 01 / collar / 02 / 03 / 04 in that order with numeral cells | **MET** | `02-service-face.png`: four near-square plates (0.92 aspect) top-to-bottom covering 93 % of the shaft with the collar between 01 and 02, corner bosses, one centred pale numeral cell per plate (0.45 × 0.33 of the plate; review-only material, §8.1 D4); the baked `T_EBS_MER_BLD_002_BaseColor` strip reads "01 02 03 04" in 19 px glyphs (`bake-report.json` rule `panel_label_numerals`). |
| Collar reads as the single bright band at ~0.72 H; no crystal cap | **MET** | `03-collar.png`: one three-course collar at 0.724 H, the cyan strip spans 0.59 of its face as painted; `01`/`05`/`06`: no second ring, no crystal, no antenna; cap = plate + square fastener plate (`test_cap_is_a_low_plate_with_a_square_fastener_plate_and_no_crystal`). |
| Maintenance: panel 02 off and leaning, bay open, conduit hanging | **MET (representation), PARTIAL (cable form)** | `04-maintenance.png` / `04b` / `09-base-close.png` (right cell): panel 02 leans on the right coupling at 35°, panel 03 lies flat, the two-panel bay is open with the three-conduit bundle and its pale clamp rings reading against the recess; the unplugged `Conduit_Right_02` stub droops from its port at −41° over the plinth-step edge and a second instance of the same stub part, chained from its `Span_End`, drops the pale end nut to ≈ 7 cm above the ground beside the plinth corner — the hang now reads at the 1,000 cm elevation and at the 1,400 uu game framing (`04b`, right cell). It is two straight segments, not the candidate's coiled cable (§8.1 D3; `_ConduitHang` part raised as an owner question). |
| Tactical game framing: the pylon reads "cables, not turret"; conduits leave the base | **PARTIAL** | `05-tactical.png`: at the 1,400 uu near preset the column, lit band, coupling feet and pale-ringed stubs read and nothing reads as a barrel or turret head; at the 3,800 uu default framing the pylon is ≈ 45 px tall and the stubs a few px, so the "cables" read there depends on the runtime span drawn from `Span_End` (§8.1 D1). `crowd/tactical_default.png`: five pylons read as identical pale columns with lit bands. |

Checks performed on concept-v3:

- Structure: 42 tests in [test_power_link_build.py](test_power_link_build.py) (contract counts, panel order
  and shaft coverage, collar band, strip placement, plinth and coupling size, coupling overhang against the
  octagon, closed underside, ports inside the face and clear of the lamp, panel-04 clearance, conduit
  diameter and segmentation with pale rings, numeral-cell placement and the review-only label material,
  bay fittings, hanging conduit clear of the plinth and inside the footprint, rail width, budgets, footprint,
  sockets, glTF axis/winding, determinism, `generate`/`--check` self-consistency and read-only behaviour),
  all passing; `build_power_link.py --check` reports no drift.
- Emissive pixel share of the mesh area (unlit magenta-keyed method, `renders/area_check/emissive-area.json`):
  1.52% default tactical, 1.53% gameplay tactical, 1.54% far, 1.32% front orthographic — under the 15%
  ceiling (`REL-ART-004`).
- Texture bake: 495 charts painted, none unmatched or skipped, deterministic (`textures/bake-report.json`).
- Visual inspection of every sheet and of the base close-ups, maintenance gameplay framing, monochrome and
  LOD1 renders by the author, recorded in the checklist above.

Rendered with [../tools/ebs_render.py](../tools/ebs_render.py) (pure Python, Unreal camera
conventions, adversarially verified against the engine headers): orthographic front/right/rear/left/top
with feature edges, and tactical views reproducing the game camera (`EchoesRTSCameraPawn.cpp`: spring arm
3,800 uu at pitch −48° default / −60° gameplay, yaw −45°; `Camera->ProjectionMode = Orthographic` with
`OrthoWidth = 2·arm·tan(55°/2) = 3,956 cm`; with the engine's default `AspectRatio_MaintainYFOV` and no
project override that width spans the image height, so the 16:9 frame covers 7,033 × 3,956 cm), plus
far zoom (6,200 uu, the camera's maximum), the service-face reverse angle (yaw 135°), a monochrome pass,
a four-pylon crowd scene and the maintenance and disconnected states. The frame-axis behaviour is derived
from engine source, not yet confirmed by an in-editor capture; a first perspective-projection pass was
superseded by these orthographic renders.

Blockout-v3 checks (history; superseded numbers): 15 structural tests; emissive share 2.0–2.5% at the
tactical framing; monochrome pass readable by luminance alone (`connected/tactical_mono.png` is regenerated
for concept-v3 and still separates the lit collar from the unlit one by luminance).

What this evidence is not: no in-engine render, no Unreal import of the concept revisions, no crowded
combat scene with units, no fog, no selection halo, no textured render (the maps in §4.2 are baked and
verified as files, not yet seen on the mesh in-engine), no measured performance, no human recognition test,
no owner review.

### 6.3 Texture bake verification (files, not in-engine)

`textures/` holds the five 1024² maps of §4.2 with `bake-report.json` (concept-v3: 495 charts painted,
none unmatched or skipped, per-map SHA-256 and channel statistics; deterministic re-bake). The independent
verification pass below was performed on the blockout-v3 bake (722 charts); the concept-v2 and concept-v3
bakes ran the same tool revision on the new atlases and only the numeral-strip finding changed (glyphs are
now 19 px in a 27 px cell; the concept-v3 strip was re-read from the baked map). Every chart centroid
carried its family's paint, channel packing and normal-map handedness
were as specified, and no blocking or major defect was found. Minor defects recorded for the ART_ALPHA bake: gutter dilation measured from edge
lines (4–5 px outside oblique corners, bound is 3 px); `polygon_fraction` in the report over-counts
half-pixel edge texels; the four panel-plate rim charts bake ~55% darker than the plate face because
bevel, recipe wear and edge wear stack on a 6 cm face; per-chart tone hashing steps the team band and
plate faces by ±0.04; the conduit pulse gradient follows image u, so it runs the opposite way on the
−Y/bottom strip faces; normal-map bevels are steeper than the recipe reference at 2.33 cm/texel; charts
padded to the 4 px minimum sample world positions off the face; numeral glyphs are 8 px in a 9 px cell
(legible, at the limit of 0.43 px/cm). The contract's 2048² stack is an ART_ALPHA deliverable; 1024² is
the blockout-stage proxy at the same atlas.

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

### 6.1 Isolated Unreal import (installed 5.8.2, sandbox project) — blockout-v3 exports, NOT re-run for concept-v3

The bounds, counts and socket positions in this table are those of the v3 exports (1,238 cm mast);
the concept-v3 exports use the same file layout, socket names and encoding (the stub part now carries a
third material section), so the importer facts below still apply, but the import itself must be repeated
before any integration claim.

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
python3 build_power_link.py --evidence-dir "<evidence root>/EBS-MER-BLD-002" --check   # read-only: temporary build, full hash comparison
python3 test_power_link_build.py
python3 ../tools/ebs_texbake.py --manifest bake-manifest.json --out textures
for s in concept_connected concept_disconnected concept_maintenance connected disconnected maintenance lod1 crowd area_check; do
  python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-MER-BLD-002/scenes/$s.json" --out "<evidence root>/EBS-MER-BLD-002/renders/$s"; done
# comparison sheets (crops are normalized x0,y0,x1,y1 of the first image; the exact commands are in receipt.json):
python3 ../tools/ebs_sheet.py --out "<evidence root>/EBS-MER-BLD-002/renders/concept-compare/01-front-quarter.png" --cols 2 --cell 720 \
  --crop 0.08,0.02,0.40,0.74 "<evidence root>/concept-crops/power-link-candidate.png" "<evidence root>/EBS-MER-BLD-002/renders/concept_connected/front_quarter.png"
python3 ../tools/make_evidence_receipt.py --evidence-dir "<evidence root>/EBS-MER-BLD-002" --package EBS-MER-BLD-002 --manifest build-manifest.json --stage "CONCEPT-V3" ...
```

## 8. Defects, decisions and remaining work

### 8.1 Deviations from the concept images (each with the rule that forced it)

| # | What the concept shows | What is built | Rule / reason |
|---|---|---|---|
| D1 | Long conduit runs leaving the base and continuing out of frame (candidate, connected) | 58 cm stubs (Ø 32) from the coupling ports to `Span_End` exactly on the 2×2 footprint edge (|y| = 200) | Footprint containment: `buildings.json` `mc_power_link` 2×2 cells, `SPEC-STR-002`; `concept-fidelity.md` §6 "end at the footprint edge (the runtime draws the link between neighbours)"; GAP-03: the cosmetic span carries no collision, navigation or network authority |
| D2 | Conduit thickness | Ø 32 cm = 0.16 W | Not a rule: `concept-fidelity.md` §6 originally wrote "Ø ≈ 0.09 W" without a measurement and concept-v2 built 0.133 W against a re-targeted test. Measured 2026-09-07 on the candidate main view (jackets 28–31 px, columns x 60–180 / rows 673–763, against the across-flats W = 176 px of §Proportion 1 → 0.16–0.18 W, ± 10 % perspective → 0.15–0.18 W); under the owner ruling the pixels beat the doc's number, so the **authoritative target was amended** (§6 of the doc, method inline) and the test now binds to the amended target, not to the build. The amendment is raised as an owner question (§8.2). |
| D3 | Maintenance: a long coiled cable hanging from the right coupling to the ground with a large connector | The unplugged `Conduit_Right_02` stub (58 cm) droops from its port at −41° over the plinth-step edge; a second instance of the SAME stub part is chained from its `Span_End` (world yaw −28°, pitch −44°) so the pale end nut rests ≈ 7 cm above the ground beside the plinth corner, 0.8 cm clear of the skirt, inside the footprint (max |coordinate| 199.9 cm). Two straight segments, not a coil. | Contract inventory: 4 conduits as ONE part mesh (`SM_EBS_MER_BLD_002_ConduitStub`); a dedicated sagging `_ConduitHang` part is outside the contract's part list (GAP-01) and is raised as an owner question (§8.2). Geometry: a single 58 cm stub cannot leave a port at |y| 142 above the 50 cm step, clear the step edge at |y| 172 and reach the ground (the path is ≥ 80 cm) — the concept-v2 stub at −70° ended inside the step at z ≈ 30, which is why no render showed it. The chained instance stands in for the runtime span segment that would otherwise be drawn from `Span_End`; the exported part inventory is unchanged. |
| D4 | Numerals painted directly on the pale plate in a condensed sans | A 56 × 44 cm label cell on the FRAME slot in the export; the bake paints it as a pale label with dark 19 px seven-segment glyphs; the assembled review OBJs write the cell under the review-only material `REVIEW_LabelCell` (rendered a hair darker than the plate) so the untextured renders show the pale label rather than a dark window | The baker (`ebs_texbake.py`, read-only tool) paints the numeral rule only under the `compact_metal` family, so moving the cell to the ceramic slot would drop the glyphs from the bake; the review renderer colours by material name only, and the baker is Python-stdlib (seven-segment glyphs, no font rasterizer); the numeral must live in the texture so one panel mesh serves 01–04 (`PanelCell` parameter, §4.2). |
| D5 | Disconnected: collar strips dark grey | Same strip geometry; lit/unlit is the material's `Connected` scalar on the status mask; the disconnected review scenes (`disconnected`, `concept_disconnected`) render the status material dark grey with no emissive | `REL-ART-004` (≤ 15% emissive area; measured 1.3–1.5%) and the event rule: state is read from authoritative network state, never baked into geometry. |
| D6 | Team-colour: nothing in the concepts | Narrow ceramic team band under the collar (z 521–529) as the ownership mask carrier | `REL-ART-028` (ownership colour carrier); `SPEC-VISD-003`; kept from v3 because the team mask needs a surface. |
| D7 | Service face toward the viewer | Service face on +X; the default camera (yaw −45°) sees the −X / +Y faces | Contract `import_policy` "+X forward"; placement yaw belongs to the integration task (`SPEC-BLD-001`). |
| D8 | Couplings are outboard feet protruding sideways past the plinth corners by ≈ 0.4–0.5 W; ≈ 0.9 × 0.5 × 0.5 W | 180 × 74 × 100 cm closed blocks (0.9 × 0.37 × 0.5 W) on the lower step: outer face at |y| 140, inner face at |y| 66, x 12–192, i.e. hanging 20 cm past the plinth-top edge / chamfer and 12 cm past the plinth's ground edge at the FRONT, not sideways | Footprint containment (`SPEC-STR-002`, 400 cm) plus D1: the outer face must sit at |y| 140 so that port (+2) + 56 cm stub + nut (+2) end on the footprint edge — a sideways protrusion past the 360 cm plinth would leave ≤ 20 cm for the stubs; the 0.37 W depth follows from that face and the 4 cm clearance to panel 04 (the concept-v2 block ran 5 cm into the plate). The forward overhang is measured against the octagon outline (`coupling_overhang_cm` in the manifest; the concept-v2 note gave the X-axis 4 cm only, its outer corner was 7 cm past the chamfer). |
| D9 | Segmented conduit rings on every conduit at every distance | LOD1 stub is a 6-sided jacket with the pale end nut only; LOD1 collar is one course without grooves; LOD1 bay is a single charcoal block | `REL-BLD-015.MC.LINK.ASSET` LOD1 ≤ 1,200 tris (642 used); LOD1 detail is not resolved at the far framing. |
| D10 | Tower proportion | 780 cm on a 200 cm shaft = 3.90 W (concept-v2: 850 / 180 = 4.72 W) | Not a rule: the fidelity doc's 4.6–5.0 W band was not reproducible from the candidate pixels (main view 3.55 raw, disconnected 3.4–3.7 raw, 3.5–4.2 W pitch-corrected; `concept-fidelity.md` §Proportion 1 carries the measurement) and the built tower was 20–35 % more slender than the painting; **the authoritative target was amended to 3.8–4.0 W** under the owner ruling and the build re-derived (W from the 1.8 W plinth that announces the footprint). Raised as an owner question (§8.2). |
| D11 | Maintenance: shed panels lying flat / leaning beside the base | Panel 02 leans on the right coupling's +X face (x 192–270), panel 03 lies flat at (270, −60); both lie past the 200 cm footprint edge (assembly bounds x 354.6) | The plinth leaves 20 cm of footprint margin, so 124 × 135 cm plates cannot lie inside it; they are cosmetic debris with no collision, navigation or network authority (GAP-03, `REL-BLD-014` cosmetic debris ≤ 200 ticks). The hanging conduit, which belongs to the structure, stays inside (test). |
| D12 | Panel plates read slightly wider than tall (≈ 0.62 W × 0.55 W, w/h 1.05–1.15) | 124 × 135 cm (w/h 0.92) | The candidate's silhouette (3.5–4.2 W) and its panel stacking (four near-square plates filling the shaft, ≈ 3.3 W) are not mutually consistent; at 3.9 W the plates must be 0.68 W tall to cover the shaft as painted (93 % covered). Width is bound by the 130 cm wide face of the octagon. Recorded, not a rule. |

### 8.2 Decisions

Decisions made under the owner's delegation (routine implementation choices):

1. Service face on +X (contract "+X forward"). The default RTS camera (yaw −45°) sees the −X and +Y
   faces, so at default placement the numbered panels face away; the collar, couplings and conduits
   carry the identity from every side. The integration task may spawn Power Links with yaw 180° or
   rely on `SPEC-BLD-001` rotation. Recorded, not changed here.
2. Height 780 cm on a 200 cm shaft (3.90 W) is the concept measurement re-derived from the candidate pixels
   (`concept-fidelity.md` §Proportion 1, amended 2026-09-07; §8.1 D10), applied under the owner ruling of
   2026-09-06 that the concepts define the look; the former OWNER-QUESTION about the 3.8:1
   construction-reference proportion (1,238 cm) is closed by that ruling. The mast is ~1% of the 7,033 cm
   orthographic frame at the default preset either way.
3. Panels and conduit stubs are separate part meshes (precedent: the M01 Surveyor/Bulwark articulation
   parts in `Scripts/generate_art_assets.py`) so damage/maintenance states move geometry rather than
   faking it with lights.
4. The isolated source is a deterministic generator, not a DCC file, because no DCC application is
   installed; the glTF export is the exchange format for Interchange.
5. Three items of the AUTHORITATIVE fidelity target were amended from the pixels (tower 3.8–4.0 W, panels
   near-square, conduit Ø 0.16 W) with the measurement written into the doc, rather than leaving the build
   or the tests to contradict the doc. The owner ruling puts the pixels above the doc's numbers, but the
   doc is the owner's target record, so the amendments are batched as an OWNER-QUESTION through the
   coordinator (below) and would be reverted with a rebuild if the owner rules otherwise.

OWNER-QUESTIONS (raised through the coordinator, never directly):

- OQ-1: Confirm the amended proportion targets in `concept-fidelity.md` (tower 3.8–4.0 W → 780 cm on a
  200 cm shaft; panels near-square; conduit Ø 0.16 W) as the authoritative read of the candidate, or
  name the number to build to.
- OQ-2: May the package add a dedicated sagging `_ConduitHang` part for the damaged state (a two-segment
  or curved cable with the end connector on the ground)? It changes the contract's part inventory
  (GAP-01: 4 conduits as one stub part). Until answered, the damaged state uses a second instance of the
  existing stub part chained from `Span_End` (§8.1 D3).

Open items:

- Concept-v3 has not been imported into the sandbox Unreal project; §6.1 is the blockout-v3 run. Re-run
  `ue_import_inspect.py` (exclusive editor run) before any integration claim; the stub part's third
  material section (ceramic) is new since that run.
- The hanging cable is two straight stub segments; its end nut hovers ≈ 7 cm above the ground (the
  tightest angles that keep it clear of the skirt and inside the footprint, §8.1 D3).
- LOD1 sits at 642 of 1,200 triangles (headroom regained by the concept rebuild).
- Material instances against the existing master materials and an in-engine look at the baked maps are
  the next stage (ART_ALPHA prerequisites); the 2048² stack is still owed.
- In-engine rendered inspection (Static Mesh Editor and a lit level at the game camera) still needs a
  GPU editor run; the headless import above proves structure, not appearance.
- Socket encoding is importer-build-specific (5.8.2): re-run `socket_probe.py` after any engine change.
- Confirm the orthographic frame axis (7,033 × 3,956 cm at the 3,800 preset) with one in-editor capture
  of `AEchoesRTSCameraPawn` before using these renders for framing or occlusion decisions.
- Gameplay proxy integration, three-gate review, and owner acceptance remain outside this lane.
