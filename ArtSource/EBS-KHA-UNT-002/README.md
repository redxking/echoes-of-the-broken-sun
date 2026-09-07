---
title: EBS-KHA-UNT-002 Riftstalker — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-RIFTSTALKER
production_asset_id: EBS-KHA-UNT-002
production_maturity: BLOCKOUT
revision: ebs-kha-unt-002-concept-v1
canon_status: CANDIDATE (built to the riftstalker candidate against canon SPEC-UNIT-006); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-UNT-002 Riftstalker — production source

Single authoritative source record for the Kharuun skirmisher. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
[`REL-FAC-025.KA.RIFTSTALKER.ASSET`](../kharuun-asset-cards.md); maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-RIFTSTALKER` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept input | `EBS-CON-KHA-UNT-002` |
| Selected candidate | `…/concept-discovery-20260906/riftstalker-review/riftstalker-candidate.png` (SKIRMISHER, MOVING FIRE, SHOULDER DETAIL) |
| Canon | `SPEC-UNIT-006`: a lean, long-limbed warform with a low forward posture, a faceted carapace in charcoal with amber seams, and a shoulder-mounted shard-caster that fires while it moves. Fires on the move with a short sidestep after each shot. Molt at a Growth Basin shows a visible carapace or striker change |
| Gameplay record | `units.json` `ka_riftstalker`: mobile skirmisher, 125 HP, 410 cm/s, 1,050 cm sight, population 2, 14 damage at 500 cm on a 22-tick cooldown |
| Asset card | **`REL-ART-005.KA.RIFTSTALKER` — AUTHORITATIVE**, in `Docs/Requirements.md`. My provisional `REL-FAC-025.KA.RIFTSTALKER.ASSET` is superseded for this asset. See §8.7 |
| Production ID | `EBS-KHA-UNT-002`, planned `SK_EBS_KHA_UNT_002` under `/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_002/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Riftstalker is the faction's harasser. It must read as something that keeps moving and
cannot hold a line, and it must not be confusable with the Cairnback, which is the faction's heavy
quadruped. What must be absent: mass. Anything that makes it look like it could take a frontal fight is
wrong.

**DETAIL.** Large scale: a low body on four long legs, longer than it is tall. Medium: overlapping
carapace shells stepping down toward a forward prow, high outboard knees, a shoulder slot for the
shard-caster. Fine: plate chipping, amber seam lines, foot wear — texture.

**ACTION.** Moves at 410 cm/s on a diagonal gait. Fires without breaking that gait, then takes canon's
short sidestep. Molts at a Growth Basin with a visible carapace or striker change. Dies by folding.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Standing height | 201 cm | between the Tender's 190 cm crown and the Lancer's 213 cm, so the roster reads consistently |
| Body length | 295 cm = 1.47 of the height | traced on the candidate's MOVING FIRE side view (1.46) |
| Stance width | 218 cm | four legs with knees carried outboard |
| Units / axes / pivot | cm; +X forward, +Y right, +Z up; pivot at the ground-contact centre between the feet | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-kha-unt-002-concept-v1`)

Generator: [build_riftstalker.py](build_riftstalker.py). One skinned mesh `SK_EBS_KHA_UNT_002`. Slots:
`MI_EBS_KHA_Strata`, `MI_EBS_KHA_Amber`. Rig, from the corrected card: `root`, `body`, a two-bone prow,
a two-bone caster, and four legs of hip, upper, lower and foot — 22 bones. Sockets (5):
`Shard_Caster_Muzzle`, `Caster_Mount`, `Target_Anchor_Center`, `Molt_Carapace_Anchor`,
`Molt_Striker_Anchor`. Clips: `idle`, `move`, `fire_on_the_move`, `sidestep`, `molt`, `death`.
Budgets: LOD0 542 baseline and 602 in the heaviest molt state, LOD1 346, against the card's
6,000 / 2,600. Amber is 3.1% of surface area against the `REL-ART-029` ceiling of 15%.

**It fires without changing its gait.** A test compares every leg track between `move` and
`fire_on_the_move` and requires them identical, so the caster can never end up fighting the walk. That
is a **regression check only**. It does not prove runtime behaviour, and simultaneous locomotion and
firing still has to be verified through turns, stops, targeting changes and animation transitions
(owner ruling 2026-09-07).

**Triangle counts far under the ceiling are headroom, not sufficiency.** Being inside the budget says
nothing about whether the silhouette, joints and carapace carry enough detail; that is judged against
the concept at gameplay distance and has not been judged yet (owner ruling 2026-09-07).

## 5. States

| State | Read |
|---|---|
| baseline | the unmolted form |
| carapace_molt | thicker shells with added plates over them |
| striker_molt | a longer caster housing with added vanes |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-UNT-002/` (`receipt.json`, `review/`
assemblies for the three states, LOD1 and six sampled poses, `scenes/`, `renders/`). Renders:
orthographic side, front and top with a 180 cm reference figure, a three-quarter, a shoulder close-up
answering the candidate's own SHOULDER DETAIL panel, and a tactical pass. Six comparison sheets in
`renders/concept-compare/`. Checks: 18 structural tests in
[test_riftstalker_build.py](test_riftstalker_build.py) — four legs and no neck bone, four driven
segments per leg, bone heads at the proximal end of each segment, the traced length ratio and the
forward slope living in the rest geometry, amber being the only emissive and under the ceiling, the
caster clearing the prow, the gait being identical whether or not it fires, the stride being pitch and
not yaw, **no frame of any clip passing through the ground**, the descent solver measuring the real
rig, molt states adding visible geometry, frame alignment, sockets, the provisional budget, LOD1
keeping four legs and the caster, and determinism — all passing. Not yet: textures, in-engine capture,
gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-KHA-UNT-002"
python3 build_riftstalker.py --evidence-dir "<evidence root>/EBS-KHA-UNT-002" --skinned
python3 build_riftstalker.py --evidence-dir "<evidence root>/EBS-KHA-UNT-002" --check
python3 test_riftstalker_build.py
for s in baseline carapace_molt striker_molt lod1 pose_move_025 pose_move_050 \
         pose_fire_on_the_move_025 pose_sidestep_044 pose_molt_050 pose_death_100; do \
  python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-KHA-UNT-002/scenes/$s.json" \
    --out "<evidence root>/EBS-KHA-UNT-002/renders/$s"; done
```

## 8. Vertex ID channels (card `.MESH_PROP`), verified 2026-09-07

The card requires "3 vertex ID coloring channels for public molting phase transitions", and canon
`REL-FAC-009` makes the molt an **80-tick public window** between baseline, Carapace and Striker. The
channels therefore have to carry the transition, not only the end states.

| Channel | Meaning |
|---|---|
| **R** | **Molt sweep order.** How early a region takes the molt-window treatment — the 512² translucent core blending skin mask `.TEX_MAPS` calls for. Seams and new growth 1.0, prow 0.85, shells graded 0.60 nose to 0.30 tail, underbody 0.20, caster 0.15, legs 0.0. This is the channel that carries the public 80-tick transition. |
| **G** | Carapace adaptation membership: 1.0 on `molt_plate_*`. |
| **B** | Striker adaptation membership: 1.0 on `molt_striker_vane_*`. |
| **A** | Reserved, always 1.0. |

**Team ownership is in none of them.** Team colour stays in its own material parameter and mask
texture, so a reader never has to disambiguate who owns a unit from what it has adapted into. Alpha is
reserved rather than left spare so it cannot later be quietly taken for team. Two tests assert both.

This replaces an earlier mapping that used R as a "baseline chassis" membership flag — already implied
by `G == 0 and B == 0`, and carrying nothing for the transition.

### The route, verified end to end

`ebs_meshkit.Mesh.vertex_colors` is an **optional** per-component mapping. When it is empty no
`COLOR_0` attribute is written at all, so every other package exports byte for byte as before — all 21
re-checked. Both `write_glb` and `write_skinned_glb` emit it.

**Attribute presence is not verification.** One skinned GLB per molt state was imported headlessly,
then exported back out of Unreal with `GLTFSkeletalMeshExporter` and `COLOR_0` read as *values*:

| State | Authored distinct | Imported distinct | Missing | Unexpected |
|---|---|---|---|---|
| baseline | 10 | 10 | none | none |
| carapace_molt | 11 | 11 | none | none |
| striker_molt | 11 | 11 | none | none |

`G` is non-zero only in carapace, `B` only in striker, alpha 1.0 throughout.

**Set equality does not establish placement**, so every imported vertex was matched to its authored
position within 0.06 cm and its colour compared against the colours authored there: **LOD0 1056 of 1056
and LOD1 696 of 696 placed and coloured correctly, zero mismatches**.

**What was not tested: engine LOD reduction.** Both imported meshes report a single LOD — the two
shipped LODs are authored and imported as separate assets — so no generated LOD chain exists and none
was exercised. An earlier claim that "LOD generation altered nothing" is withdrawn: nothing generated
LODs. If a reduction chain is added, placement must be re-verified on every generated level.

Round-trip export is the method that worked here, not the only way to inspect Unreal's vertex colours —
an engine-side reader of the render data would also serve and was not attempted. What made the
round-trip attractive is that 5.8 exposes no Python accessor for the values.

### Team identification

Team ownership is delivered by a dedicated `TeamColor` vector parameter modulating a team mask in the
packed utility channel of the 2048² stack — a texture and parameter path entirely disjoint from
`COLOR_0`. Ownership is fixed for a unit's life while adaptation changes during play and is
opponent-facing; sharing a channel would make a molting unit's owner ambiguous exactly when the public
window makes it most worth reading. **Defined, not built** — it belongs to the texture pass and gates
material acceptance.

### Sweep convention

**Higher R means earlier treatment**, so `threshold = 1.02 - progress * 1.04` and a surface is swept
where `R >= threshold`. The threshold runs outside `[0, 1]` deliberately: progress 0 must sweep
*nothing* (the maximum authored R is 1.0) and progress 1 must sweep *everything* including the R = 0
legs, and a threshold confined to the unit range cannot do both. The 0.02 margin is five times the
1/255 quantisation step.

**A real defect was found here.** The first material compared with a strict `>` and wired the `A == B`
branch to zero, so **the R = 0 legs never completed at tick 80**. It compiled and was assigned — which
is precisely why "compiles and is assigned" was not evidence of correct response. Checked after
quantisation, the ten levels now cross at ten distinct progress values with a smallest margin of 3.92
ticks, against a quantisation step worth about 0.3 ticks (`sweep-boundaries.json`).

### Rendered in engine

Unreal rendered the response offscreen with a real RHI: **ticks 0/20/40/60/80 for all three variants at
two framings** — tactical (1400 cm arm, −55°) and close (520 cm, −18°) — 30 frames, plus a six-step
**interruption and restoration** sequence. On screen, tick 0 sweeps nothing and tick 80 sweeps
everything, legs included.

The interrupted frame returns to baseline with no partial sweep retained and no adaptation geometry,
and restoration **restarts from zero rather than resuming** — canon `REL-FAC-009` cancels the adaptation
without refund. That is what shows the material holds no clock: every frame is a pure function of a
`MoltProgress` parameter written from outside, one MaterialInstanceConstant per value. The material
**consumes authoritative progress and creates no gameplay timing of its own**.

**Still outstanding:** this is a debug material whose base colour *is* the channels, rendered through an
editor SceneCapture. The shipped art material, and integration with a running simulation driving
`MoltProgress` from the real 80-tick window, are untested — the captures show the material's behaviour,
not the game's.

Evidence: `…/EBS-KHA-UNT-002/vertex-id-route/` — `channel-verification.json`, `material-response.json`,
`route-report.json`, `sweep/`.

## 9. Textures (BLOCKOUT bake, revision concept-v2, `ebs-texbake-v2`)

Built against the authoritative card's `.TEX_MAPS` / `.MAT_RULE` and `REL-ART-029`, read **provisionally**:
the card names the maps and the ceiling; the surface recipes are this pipeline's, not an authored
requirement.

### 9.1 UV atlas

`concept-v2` changes **only UV0**: `pack_atlas` gives every polygon of the baseline LOD0/LOD1 and both
molt-state LOD0 meshes its own chart in one 2048² atlas — 448 charts at 2.03 px/cm, 81% used, molt parts
included, identical polygons sharing one chart. Triangle counts, bounds, sockets, rig, clips and
`COLOR_0` are unchanged; `test_v2_changed_only_uv0` pins v1's numbers. Each chart carries its authored
vertex colour so the baker's StateMask can mirror it. Recorded in
[bake-manifest.json](bake-manifest.json) and in `--check`; re-imported clean (22 bones, 5 sockets,
1056 / 696 vertices).

### 9.2 Surface families (added to the shared baker)

| Family | Slot | What it is |
|---|---|---|
| `kharuun_obsidian` | `MI_EBS_KHA_Strata` | Opaque volcanic value mask over a charcoal body, warped strata bands every 48 cm, a fractured cell field for the card's high-frequency detail normal, micro-noise grit. **No emissive**: the crack bottoms carry only an ember-dim, matte amber tint. |
| `kharuun_amber` | `MI_EBS_KHA_Amber` | Broken-Sun Amber seams and the caster slot — **the only emissive**, brightest on the seam's centre line. |

Unit rules: molt plates and striker vanes bake as fresh growth (fewer fractures, lighter, smoother);
feet and lower legs take ground dust in the bottom 25 cm; the caster slot carries a breech-to-muzzle
heat gradient; the prow and crest are team-colour carriers.

**The obsidian body sits inside the Charcoal anchor.** `Docs/ArtDirection.md` anchors charcoal at
0.02–0.07 linear. The first bake put the median plate at 0.070 with half the charts above it — the
strata band and an over-strong ember tint (+0.175 R at full strength) lifted it. With the body lowered
and the ember cut to "ember-dim", the painted plates measure 0.029 / 0.043 / 0.065 linear
(min / median / max), **311 of 311 obsidian charts inside the anchor**; amber charts 0.60–0.74.

### 9.3 The stack

| Map | Channels |
|---|---|
| `T_EBS_KHA_UNT_002_BaseColor` (2048², sRGB) | charcoal obsidian with strata and ember-tinted fractures; amber seams |
| `T_EBS_KHA_UNT_002_Normal` (2048²) | tangent-space, Unreal/DirectX green-down: strata, fracture cells, grit |
| `T_EBS_KHA_UNT_002_MRE` (2048²) | R metallic 0 · G roughness (obsidian ~0.52, amber ~0.35) · **B emissive mask: seams and caster slot only** |
| `T_EBS_KHA_UNT_002_StateMask` (2048²) | R molt sweep order **mirrored from `COLOR_0.R`** (`COLOR_0` stays authoritative) · G translucent core blend on new growth · B team carrier |
| `T_EBS_KHA_UNT_002_MoltBlend` (512²) | the card's secondary translucent core blending skin mask: the StateMask box-filtered |
| `T_EBS_KHA_UNT_002_AtlasDebug` | chart rectangles by family; UV verification only |

**The amber ceiling is measured on the baked mask, by area.** The report counts painted polygon texels
whose `MRE.B` is non-zero: **3.8% of painted area** against the card's ≤15%, agreeing with the slot
geometry's 3.1% within a gutter. 448 of 448 charts painted, none unmatched or skipped, deterministic;
bake 104 s. Seven texture tests (`test_riftstalker_textures.py`) check the report against this manifest
and revision, the full stack with recorded hashes, the ceiling, emissive living only on amber charts
(sampled at every chart centre), StateMask.R mirroring `COLOR_0.R` within 8-bit at every chart centre,
team carriers, and determinism.

### 9.4 Material and in-engine capture

Recorded in §9.5 below once the capture lands; the pipeline findings it produced are in the ledger.

## 9. Decisions and open items

1. **It is a quadruped, and I corrected my card rather than the model.** The provisional card first
   described a biped with a long neck and 24 bones, written before I traced the candidate. Both the
   SKIRMISHER and MOVING FIRE views show four legs and no neck. The card now specifies a 22-bone
   quadruped, and a test asserts the skeleton has four legs and no neck or head bone.
2. **Three animation bugs, each found by measurement rather than by eye.**
   - Every fold I wrote first went into the **yaw** slot. `ebs_skelkit` takes `(pitch, yaw, roll)`, so
     the legs swung sideways instead of stepping. A test now requires the stride to be pure pitch.
   - The **bone heads sat at the distal end** of each segment, so rotating a thigh pivoted it about the
     knee and tore the leg off the body in the death pose. Heads are now proximal, and a test checks it.
   - The collapse clips **drove the feet through the ground**. Fixed by solving each key's descent
     against its own leg fold.
3. **The descent solver measures the rig; two analytic attempts were wrong.** The first inverted the
   pitch sign. The second bisected the ankle's height, which is not monotonic in the fold, so it
   returned its own search bound as an answer. Both produced folds that pushed the feet down. The
   working version poses the actual mesh and measures it, and a test asserts a folded leg supports a
   deeper settle than a straight one.
4. **The death pose leaves one leg raised: an OPEN BLOCKOUT DEFECT, not accepted animation.** A thigh
   45 / shin 25 fold reads better, with the legs collapsing under the body, but its interpolated frames
   sink 3.6 cm through the floor. Ground contact wins over the preferred fold at blockout. Owner ruling
   2026-09-07: acceptable as an open defect, and **both** are to be resolved later through a supported
   collapse pose rather than by choosing between them. `acceptance.animation` is `NOT_ACCEPTED`.
5. **The ground test carries a 0.05 cm tolerance.** The deepest sampled frame of the collapse sits
   0.18 mm below zero and does not move with any margin I apply, so it is numeric rather than a real
   penetration. The tolerance is stated in the test rather than hidden by rounding.
6. **The anatomy correction is history, not competing guidance.** The superseded biped description is
   kept on the card and in the manifest only so the change is traceable, explicitly marked as
   superseded, and carries its source references: the candidate's SKIRMISHER and MOVING FIRE views, the
   generation record, and the registered concept crop. The quadruped plan is the sole anatomy.
7. **Monochrome separation from the Cairnback is a requirement, and its baseline exists.** Both are
   Kharuun quadrupeds and must be distinguishable in monochrome at tactical distance, where colour and
   emissive count for nothing. This package now renders a monochrome tactical pass and a monochrome
   side pass as the baseline for that comparison. The Cairnback build must establish its contrasting
   silhouette — broader, heavier mass and a different stance and gait — **before** it is detailed.
8. Open: textures (2048² packed PBR with micro-noise normals), the collapse pose, verification of
   locomotion and firing together, a detail judgement against the concept at gameplay distance, the
   monochrome comparison against the Cairnback, in-engine capture, gate reviews, incorporation of the
   provisional card into the authoritative requirements, owner acceptance.

7. **I missed an authoritative card, and reported that none existed.** On 2026-09-07 I told the owner
   that no Kharuun asset card was in `Docs/Requirements.md`, and the ruling to author a provisional
   Kharuun set was given on that premise. A full audit of every asset-card heading found
   **`REL-ART-005.KA.RIFTSTALKER`**, which governs this asset. The other seven Kharuun packages are
   genuinely uncovered, so the rest of the provisional set stands, but this one is superseded.

   | Card clause | This build |
   |---|---|
   | LOD0 ≤7,500 / LOD1 ≤3,200 | 602 / 346 — within |
   | Amber ≤15% of surface area | 3.1% — within |
   | Hard-faceted, no smoothed topology | faceted throughout — within |
   | Sockets `VFX_Muzzle_Shard_01`, `VFX_Molt_Origin_Base`, `Target_Hitbox_Center` | **now emitted**; the build's earlier names are kept as aliases |
   | 14-bone kinematic rig | **non-compliant as written**: 22 bones, selected by the owner. Amendment proposed |
   | 3 vertex ID channels for molt phases | **not implemented, not waived** — see §8.8 |
   | 512² translucent core blend for molts | pending at the texture stage |

> **OWNER-QUESTION A — RESOLVED 2026-09-07.** Asked: confirm the quadruped and amend the card's bone
> count, or direct a 14-bone rig. Owner ruling: **keep the quadruped and select the existing 22-bone
> rig as the production direction**, with no limb segment or independent caster aim removed merely to
> reach 14. A targeted amendment is prepared in
> [card-amendment-REL-ART-005.KA.RIFTSTALKER.md](card-amendment-REL-ART-005.KA.RIFTSTALKER.md),
> generated from the generator's own bone table so it cannot drift, and carrying the full hierarchy and
> each bone's purpose. **`Docs/Requirements.md` is not amended**, so this asset remains non-compliant
> with the `.ANIM_RIG` clause as written until it is, and the manifest says so.

**Retraction.** I earlier wrote that the card's 14-bone figure was "consistent with the biped my
provisional card first described". That was an inference about the card's provenance, not something I
established, and the owner directed that it not be claimed. It is withdrawn. What is established: the
card says 14, the selected concept is four-legged, and the owner has selected the 22-bone rig.

8. **The three vertex ID channels are required work, and the export path cannot carry them yet.**
   The card's `.MESH_PROP` requires three vertex ID colouring channels for public molting phase
   transitions. Neither the rig decision nor a clean import waives them. The mapping is now defined:

   | Channel | Meaning | Present in |
   |---|---|---|
   | R | baseline chassis, in every phase | `baseline`, `carapace_molt`, `striker_molt` |
   | G | carapace-molt geometry | `carapace_molt` |
   | B | striker-molt geometry | `striker_molt` |

   The blocker is real and stated: `ArtSource/tools/ebs_meshkit.py` emits `POSITION`, `NORMAL`,
   `TEXCOORD_0` and `TEXCOORD_1` only. There is no `COLOR_0` attribute in the export path, so the
   channels cannot reach a GLB, an import or a material. Adding `COLOR_0` to the kit is the
   prerequisite, and three verifications remain after it: the channel present in the export, surviving
   the import into vertex colours, and read by a molt-phase material with visibly distinct phases. A
   test asserts the mapping exists and that `verified` is false, so this cannot quietly become "done".
