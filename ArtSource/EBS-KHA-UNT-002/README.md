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

### 9.4 Material

`M_EBS_KHA_UNT_002_Art` (built in the capture run, not yet a production master): BaseColor →
`lerp(TeamColor, StateMask.B)` → `lerp(core tone, 0.45 × swept × (1 − MoltProgress))` → base colour;
Normal and MRE.G/MRE.R straight to normal, roughness and metallic; emissive = `MRE.B × Broken-Sun Amber ×
EmissiveStrength × (1 + swept × (1 − MoltProgress))`; `swept = COLOR_0.R ≥ 1.02 − 1.04 × MoltProgress`
(the vertex-ID convention, §8). Parameters: `MoltProgress` (authoritative, written from outside; the
graph has no clock), `TeamColor` (default cyan is a **placeholder**, not a Kharuun team colour),
`EmissiveStrength` (0.5). Every connection is checked and any failure is recorded by pin name.

### 9.5 In-engine capture (final: `textures/inengine/`)

Unreal rendered the textured unit offscreen: ticks 0/40/80 for all three variants at tactical
(1400 cm, −55°) and close (520 cm, −18°) framing, 18 frames, exposure chosen by a bracket keyed to the
ground (EV +11: ground 104, body 71). Readiness was gated on the property the capture exists to show —
the tick-0 and tick-40 instances rendering differently on the crest above the team band (67 → 100) —
and passed on the first poll. Measured on the close frames against the ground: foreleg 0.40×, lower
shells 0.32–0.37×, the team band 1.6–1.7×, the crest above the band 0.68× at tick 0 and 0.98× at tick
40 (the window tint) and 0.30× on the carapace variant's fresh plates; the brightest seam pixels
221/187/154 — amber, not clipped. At the tactical camera all three variants read as a dark silhouette
with the ownership band legible at once; the carapace variant is distinguishable by its heavier back,
the striker's vanes only faintly (`sheet_tactical_t080_three_variants.png`).

**Seven captures were needed, and each taught something now in the ledger.** (1) A key light at the
Art Direction's intensity 10 into an LDR SceneCapture with no eye adaptation rendered a uniform tan.
(2) A guessed fixed exposure rendered black. (3) A bracket keyed to the *creature* lifted a charcoal
body to the ground's brightness and hid the next fault. (4) `TC_MASKS` textures sampled as
`LinearColor` are a compile error that renders the **default material** with no visible error and
every instance identical — found by a per-channel unlit diagnostic (PF-018); textures also streamed in
at 32² until `never_stream` was set (PF-019). (5) With those fixed the body still read 0.8× the
ground: a lit-shading bisection rendered every input combination dark, and a bisection of the art
graph by *wiring* showed the team lerp was the one lifting step — because the probe sat on the crest,
a whole-shell team carrier rendering the team colour by design (PF-020). (6) The crest carrier became
a **band** across its middle 40%; the seams still clipped white at emissive 4, 1.2 and 0.5 alike —
the baked amber *base* was a pale 0.55–1.0 linear that the amber key drove to white regardless of
emissive. (7) The base is now the Art Direction ember weighting; the glow rides the emissive mask
(`ebs-texbake-v3`), and the seams read amber.

**Two decisions taken here, recorded for gate review, not accepted:** ownership rides a band across
the crest plus the prow rather than the whole crest (readability kept, obsidian identity kept); the
amber seams are ember-dark in albedo and glow through the emissive.

**Outstanding.** Integration with a running simulation driving `MoltProgress`; a production master
material and a real Kharuun `TeamColor`; gate-review judgements at gameplay distance on the fracture
cell frequency (12 cm cells read as bold plates at close range, as intended "faceted crystalline",
but that is a judgement), on seam visibility at tactical distance (they are faint by design under the
≤15% ceiling), and on the striker variant's distinction at tactical distance; and the provisional
card reading. Superseded bakes and their captures are retained as `textures_v1..v3_superseded/`.

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


## Fidelity production pilot — 2026-09-08

**Author and owner:** Angelis Pseftis. Status: **GEOMETRY PILOT; NOT PRODUCTION OR ACCEPTED**.

The owner handed off all 21 packages for production, starting with Riftstalker. The source work is
isolated in `Worktrees/art-concept-fidelity-production` at base `dbe4ddb`; the previous art worktree's
three dirty files and original blockout exports remain unchanged. The existing master requirements,
creative canon and owner decisions still govern. This section is the current pilot record, edited in
place; historical blockout and detail-pass receipts above retain their original scope.

The selected `riftstalker-review/riftstalker-candidate.png` was visually compared with the previous
in-Unreal chamfered-box render. The dominant problem was the silhouette and construction of the forms,
not unused triangle budget. `fidelity_geometry.py` now authors closed faceted tapered plates,
wrapped side armor, joint covers and tapered limb cores. `build_fidelity_pilot.py` preserves component
identities, the 22 selected bone names, five socket names and separate adaptation channels in six
candidate GLB/OBJ exports (three states, two LODs). These exports do not replace the prior deliverables.

The pilot changes knee rest height from 152 to 112 cm to remove the nearly horizontal upper limbs;
it retains the bone hierarchy but is not animation-compatible merely because names match.
`fidelity_motion.py` therefore supplies five experimental clips using explicit ankle targets.
The first solver used the wrong pitch sign and failed floor checks; its failed measurements remain in
the evidence folder. The correction uses the mesh kit's actual positive-pitch convention. Sidestep and
the remaining production actions are pending; no placeholder clip claims their behavior is implemented.
The original clips remain with the original blockout.

The shell ordering is explicitly corrected in the pilot: shell_01 is at the tail, shell_05 at the nose.
Higher R starts earlier, so their R values now rise from 0.30 at the tail to 0.60 at the nose. G/B
remain adaptation membership, alpha stays 1.0, and team identification stays in a separate texture band.
Shape variation is seeded per plate identity and dimensions so inserting a molt plate cannot change
unrelated limbs through random-stream advancement. A fresh UV atlas binds all six geometry variants;
old textures and old Unreal round-trip receipts do not validate these new outputs.

Evidence is retained at `BuildArtifacts/Evidence/riftstalker-fidelity-20260908/` in the enclosing workspace.
Iterations 01–05 retain failed or superseded comparisons; iteration06 contains the current six source exports and fresh atlas bake.
Blender renders use baked atlas maps, a preview-only roughness floor of 0.85, DirectX-to-Blender normal conversion, and controlled CPU rendering. They are actual renders of
modeled geometry, not generated concept pictures, and are not Unreal or packaged-game evidence.

The pilot still needs source-to-Unreal verification, material/texture qualification, full motion and
transitions, LOD readability, physics/VFX, gameplay binding, formal gates and owner review. The rig
amendment remains distinct from these visual studies. No other asset is promoted on the strength of
this pilot and this method is not yet approved for scaling across the other twenty packages.


### First pilot receipt and visual disposition (iteration06, historical)

The seven checks in `test_fidelity_pilot.py` passed, including 2,910 sampled mesh poses across
three states, two LODs and five experimental clips. Repeat exports are byte-identical. Baseline
LOD0/LOD1 are 4,216/1,368 triangles; Carapace 4,376/1,448; Striker 4,280/1,400. These counts
meet the card ceilings but do not establish adequate modeling. The baseline Blender import has one
skinned mesh, 22 bones, five exact socket objects and five named actions. Its importer-created
Icosphere is a bone display helper, not a second skinned asset.

`fidelity-pilot-receipt.json` records source/concept/generated hashes and bounded test results.
`iteration06/Riftstalker_fidelity_editable.blend` is an editable rigged pilot with packed preview
maps. The six GLBs carry skinning, animations, UVs and vertex channels; their texture maps are separate.
The static `baseline_lod0_review.blend` is a render scene, not the editable rig package.

**Art gate not passed; do not scale this recipe to the remaining twenty assets.** The current
plate generator improves the shoulder crown but does not finish the concept: the caster still reads
as a beak, distal legs remain too straight, and similarly sized shingles obscure the hierarchy of
large mantle shields and subordinate fracture plates. The next modeling operation must explicitly
resolve those three forms before adding surface detail. A read-only independent visual review
reached the same disposition. None of the other twenty packages was promoted or modified.

The fresh 2,048 atlas has 5,686 charts, 1.1018 px/cm density and 2 px gutters. The baker reports
2.0911% emissive painted polygon area over the combined atlas; this is not separate per-state
surface-area qualification. The pre-existing baker's mineral roughness recipe is below the supplied
0.85 charcoal floor. The preview clamps roughness; the texture files remain nonconforming. The
albedo-only diagnostic establishes correct base-color UV mapping, not final shaded appearance.
Full material, LOD, motion and engine acceptance remain open. No new Unreal import or game capture
was performed, and old engine receipts cannot validate this changed geometry or rest pose.

Reproduce from this worktree using a new evidence output directory:

```sh
python3 -B ArtSource/EBS-KHA-UNT-002/build_fidelity_pilot.py --out "$EBS_PILOT_OUT"
python3 -B ArtSource/EBS-KHA-UNT-002/test_fidelity_pilot.py
```

Bake with the existing `ArtSource/tools/ebs_texbake.py` CLI against that output's
`bake-manifest.json`, then run Blender in background CPU mode with `render_fidelity_pilot.py --
<output-directory> baseline_lod0`. `package_fidelity_blend.py -- <output-directory>` packages the
baseline rig with those review materials. These commands create pilot artifacts only; they do not
write production Unreal assets. The reconciliation audit found **eleven** provisional cards in the
current source bundle, not the handoff's ten; no normative card or lifecycle status was changed here.


### Current refinement — iteration12, source 9e29e93

The owner requested a substantially better model after reviewing the first pilot. The current
geometry replaces the repeated small roof plates with five larger ridged shields and swept side
leaves, folds the knees, lowers the prow beneath an exposed firing channel, replaces the square
muzzle insert with a fractured aperture, and covers the ankle shafts. These are authored mesh
changes. Existing gameplay data, placement rules, original exports and the other twenty packages
remain untouched.

Front knee rest heads are now X=4, Y=±82, Z=97 cm; rear heads X=-106, Y=±82, Z=106 cm. Hip and
ankle attachment points remain unchanged. The 22-bone hierarchy and five socket names remain. The
firing aperture terminates at the existing muzzle socket, X=94, Z=176 cm. Iteration07's knees crossed
in side view and its toe edges went slightly below ground; that candidate is retained as rejected
comparison evidence. Iteration08 corrected the stance; iteration09 covered the ankles; iteration10
replaced the engineered-looking muzzle insert.

The final geometry passes the seven pilot tests in `refinement-tests-final.log` (126.497 seconds),
including all 2,910 sampled poses and byte-identical repeat generation. This is structural evidence,
not visual animation acceptance. Before the final LOD correction, baseline LOD0/LOD1 were 7,040/2,304 triangles; Carapace 7,440/2,464;
Striker 7,200/2,368. Iteration11 retains identical Amber landmark geometry at both LODs, reducing
over-subdivided thin LOD0 seams and restoring the missing LOD1 seams. Current counts are
6,464/2,688 baseline, 6,864/2,848 Carapace and 6,624/2,752 Striker, against 7,500/3,200. The five experimental clips remain incomplete relative to the
production action contract.

`fidelity_texbake.py` is a package-local surface adapter. It preserves the shared baker and replaces
only obsidian BaseColor, tangent normal and MRE pixels with a common world-space strata field,
sparse interrupted fissures and fine grit. It validates the decoded encoded colour and baked
roughness, and compares state-mask and Amber-pixel hashes before/after. The review shader now reads
baked roughness directly; it no longer conceals low roughness with a shader clamp. This does not
implement an Unreal faction master material or close the team/molt rendering gate.

The historical iteration12 evidence output is `BuildArtifacts/Evidence/riftstalker-fidelity-20260908/iteration12`; the continuation below records the current pass.
The shared atlas remains a limitation: 8,944 per-face charts, 0.8861 px/cm, and two-pixel gutters.
A more efficient production unwrap and texture validation through mip/LOD transitions remain open.
No prior Unreal import receipt applies to these changed surfaces and rest joints.


Iteration10's first strata texture met the numeric limits but still read as flat clay in its actual
render. Recipe `ebs-riftstalker-mineral-refine-v2` increases tonal variation within the charcoal range,
adds 5.2 cm lamination and 5.8 cm grit, and keeps fissures dark and interrupted. The final LOD change
passes four targeted checks in `landmark-tests.log`, including exact Amber geometry correspondence,
component binding/budgets, GLB contract fields and repeatability. These supplement the earlier full
pose sweep; they do not establish game-camera readability or texture mip acceptance.


The full 2048² recipe-v2 bake passes its encoded-pixel checks: obsidian colour spans
0.020289–0.068478 linear, roughness never falls below 217/255, and obsidian emissive is zero.
StateMask and Amber pixel hashes match before and after replacement. Source/output hashes and
measurements are retained in `iteration11/textures/fidelity-texture-refinement-report.json`.
These results close the pilot's baked charcoal-roughness defect; they do not accept overall material
quality or the Unreal shader. The original recipe-v1 full bake and its weak-looking render remain
in iteration10 rather than being overwritten.


The adaptation review caught a visible attachment defect in iteration11: its added Carapace shields
read as a suspended cap. Iteration12 lowers their bases by 11 cm so they intersect the underlying
mantle. Triangle counts and named channels remain unchanged; `landmark-tests-final.log` repeats the
four affected export/LOD checks successfully. The earlier full pose sweep remains applicable to the
unchanged joints and clips; the added plating is rigid to the body and remains high on the chassis.
The baseline and Striker geometry are unchanged by this final attachment adjustment.

For the current surface recipe, bake with the package-local adapter (the shared baker alone recreates
the earlier surface). With `EBS_PILOT_OUT` set to a fresh evidence directory:

```sh
python3 -B ArtSource/EBS-KHA-UNT-002/build_fidelity_pilot.py --out "$EBS_PILOT_OUT"
/Users/angelispseftis/.venvs/ebs-art/bin/python ArtSource/EBS-KHA-UNT-002/fidelity_texbake.py \
  --manifest "$EBS_PILOT_OUT/bake-manifest.json" --out "$EBS_PILOT_OUT/textures" --size 2048
/Applications/Blender.app/Contents/MacOS/Blender --background --threads 4 --python-exit-code 1 \
  --python ArtSource/EBS-KHA-UNT-002/render_fidelity_pilot.py -- "$EBS_PILOT_OUT" baseline_lod0
/Applications/Blender.app/Contents/MacOS/Blender --background --threads 4 --python-exit-code 1 \
  --python ArtSource/EBS-KHA-UNT-002/package_fidelity_blend.py -- "$EBS_PILOT_OUT"
```

The editable Blender artifact packs StateMask and MoltBlend alongside BaseColor, Normal and MRE.
Importer-created bone-display helpers are removed by their actual rig references, preventing them
from being mistaken for an export mesh. All gameplay-facing material bindings still need Unreal
implementation and verification.


The final iteration12 full-resolution bake repeats the encoded range and roughness results above,
with 1,037,449 painted obsidian pixels checked. `refinement-bake-seated.log` and the iteration12
refinement report retain this result separately. The preview study uses a 16 m orthographic frame
for the distant view; it is a camera study, not the game's validated tactical camera. Material and
geometric improvement do not close full animation, UV efficiency, state communication, in-engine
capture or owner acceptance.

Saved-file read-back passed: one mesh, 22 bones, five sockets, five actions and all five maps packed; scene author/creator both identify Angelis Pseftis. `iteration12/editable-readback.json` retains that check. The blend is an editable refinement, not an accepted production package.


## 11. Production continuation — 2026-09-08

**Art acceptance remains open. This is not a production-ready asset.** The owner requested continued
refinement through completion. The current geometry still shows broad plate surfaces compared with the
selected concept, so successful source/import checks do not close the art gate. The selected source remains on this isolated branch; the other twenty packages have not been promoted
through production.

Evidence root: `BuildArtifacts/Evidence/riftstalker-production-20260908` in the workspace supporting
location. The sandbox is `Sandbox/EBSRiftProduction.uproject`; no game project Content, Source, Config,
map, workflow, or gameplay data was changed. Main checkout HUD edits observed during this pass belong
to other development and were preserved.

The latest fully source-tested geometry checkpoint is `source21`, committed as `c404d85` (surface/UV
source subsequently recorded in `a1fea0a`). Its UV-applied exports are in `production21`:

| State | LOD0 triangles | LOD1 triangles |
|---|---:|---:|
| Baseline | 6,476 | 2,832 |
| Carapace | 6,876 | 2,992 |
| Striker | 6,636 | 2,896 |

Eight source tests pass (`source21-full-tests.log`, 368.866 s), including all state/LOD animation ground
checks and repeatability. Six motion-contract tests separately pass. Thirteen named clips cover the
applicable unit actions; an invented active ability is not added to this passive Slipfire unit. Movement
is authored against 410 cm/s, with the Carapace playback multiplier specified separately. Firing retains
the locomotion leg tracks. Facing-settle clips return to neutral because simulation owns actual facing.
The death body lowers 110 cm while the feet remain grounded in source sampling.

`final21-import.json` reports no errors: three SkeletalMeshes, three Skeletons, and 39 sequences.
`final21-physics-lod/physics-lod-report.json` reports two authored LODs per mesh, full-precision UVs on
both, and assigned compatible engine-generated PhysicsAssets. Generated physics bodies are **not**
ragdoll/contact acceptance. Authored LOD import is **not** automatic reduction or transition acceptance.
The 22-bone amendment is recorded in this branch's Requirements.md under the owner's standing ruling;
it has not been merged into main by this pass.

### Texture transfer and rendered checks

`uv21/uv-qa.json` reports zero positive-area overlaps on all six layouts. The shared layout has 1,647
islands and about 1.08 px/cm over unique representative geometry; the earlier under-1,000-island target
was not met. The experimental tree unwrap was rejected for overlaps. The Cycles transfer route was
withdrawn after it produced empty masks and a flat normal map despite completing. Its entry point now
refuses that route rather than silently returning a bake.

`transfer_fidelity_textures.py` transfers matching polygon UVs directly, with 2x raster supersampling,
normal tangent-frame conversion, and two output pixels of dilation. It checks that emissive, team, core,
and normal data survive. Twelve source triangles with degenerate source UVs use explicitly reported
geometric normals; micro-normal detail on those triangles is not claimed. `production21/textures/`
contains the maps and transfer report. The surface check records one remaining zero Amber caster
centroid and sampled source/color boundary issues. The source bake's charcoal-range check must not be
misrepresented as complete transferred-atlas compliance.

`final21-capture/art-capture-report.json` has zero reported connection/evaluation errors, 30 material
frames and 57 animation poses. A previous capture was rejected because a Clamp input connection failed
and poses stayed at rest. Changing away from single-node mode before each override forces the sequence
and sample time to reinitialize; the receipt now checks the actual 110 cm death displacement. Earlier
captures with empty maps and temporary checker materials remain as failure evidence.

These are sandbox camera/lighting studies, not in-game tactical-camera qualification. The material's
core treatment is opaque-pass subsurface transmission from the 512px mask, not see-through geometry;
its interpretation against the translucent-core requirement remains open. Team ownership uses the
separate StateMask band and TeamColor parameter, never COLOR_0. Material damage is a presentation
parameter, not new simulation behavior. Art fidelity, practical team/state readability and performance
remain pending.

### Shard effect

The editor-only `ArtSource/tools/RiftNiagaraAuthoring` bridge authors an actual Niagara graph from a
factory emitter with explicit modules. It is installed only in this sandbox. The effect bursts one
velocity-aligned Amber shard at 1,200 cm/s, lasts 0.5 s, disables renderer/system shadows, and bounds
flight in all directions. `vfx-socket-launch/` measures +X, +Y and -X motion and captures a launch from
`VFX_Muzzle_Shard_01` at approximately (94, 0, 176) cm. Explicit material recompilation in the rendered
editor was required to replace the temporary checker material. No gameplay weapon binding, allocation
profiling, multiplayer load/performance acceptance, or finished molt Niagara effect is claimed.

The next gate is visual refinement against the actual concept, followed by representative camera and
state review. Texture readiness, rig/animation correctness, physics behavior, LOD transitions, performance,
provenance handoff and eventual authorized game integration remain separate gates.


The subsequent `final21-core` check replaces the earlier opaque core approximation with
`M_EBS_KHA_Core`, a translucent surface-forward material in the existing second material section.
Its 512px mask controls localized opacity through the molt window. Both saved section assignments
were read back, and the rendered capture report records no material-connection or pose errors.
Transparency sorting, overdraw and gameplay-distance clarity are still review items; this does not
close the art or performance gate. Daylight emissive brackets retain 20/100/500/2000 comparisons;
100 is the current provisional setting after the higher values washed the aperture toward white.
Inherited template lights are removed from the unsaved review scene so the declared gold/indigo setup
is the lighting actually used. The current material enforces the charcoal base range and a 0.85
roughness floor; those shader guarantees do not erase the recorded transferred-texture boundary defects.

### Selected checkpoint and rejected studies

The selected geometry is restored byte-for-byte to `c404d85` (source21). Later source22–25 clay
studies were rejected: extra armor was buried, stood upright as scraps, or read as separate tiles.
Those studies did not improve concept fidelity, even where structural tests passed. Their artifacts
remain in the evidence root, with the selection rationale in `visual-decision.json`. Exact uncommitted
generator snapshots for source22–24 were not retained; their meshes/renders are historical evidence,
not reproducible source checkpoints. Source25 retains its pre-build source snapshot.

The current procedural method has not reached the selected concept's sculpted quality. Continuing
to add plates or bevels is not demonstrated to close that gap. The editable source21 Blender package
is the retained handoff for deliberate form modeling and retopology, followed by fresh transfer and
verification. It contains the baseline mesh, 22-bone rig, five sockets, thirteen actions and five packed
maps; adaptation exports remain separate GLBs. This is an incomplete production pilot. Neither this
asset nor the other twenty packages is promoted to production acceptance by this record.

The machine-readable receipt now identifies production21 at its root. Earlier iteration12 facts are
preserved under `historical_passes`, so consumers cannot mistake old five-clip/import-pending records
for the current thirteen-clip sandbox evidence.

### Pending clay form study — form28

The current generator source contains an **unaccepted clay study**, while `production21` remains the
last selected imported checkpoint. Form28 flattens the pyramidal armor, uses blunt fractured
terminations, removes loose mantle overlays and cuts fracture fields into the plate surfaces. It
preserves Amber and foot landmarks, the existing rig, sockets and action definitions. Forms26–27 and
the source snapshots are retained in the same evidence root; no concept artwork was replaced.

Three focused source tests and all six GLB contract checks pass. The CPU preview shows clearer slab
edges but still overly regular geometry; it does not establish concept fidelity. Full animation ground
sampling, new UV/material transfer and Unreal appearance remain unverified. The previous production21
checks do not transfer to this new geometry. `form28/pending-review.json` records exact counts and hashes.

`capture_fidelity_clay_inengine.py` prepares matched neutral-clay views in the isolated sandbox,
including the prior mesh and new mesh under identical settings. Its syntax was checked; it has not
executed. `form28/import-job.json` is ready. Unreal/Blender/GPU launch is waiting for Backend Chat to
release its integration reservation; this lane has not launched a conflicting job.


**Form28 engine follow-through, 16:32 UTC:** Backend Chat released a bounded render window.
The isolated editor imported all three states with zero inspection errors (three SkeletalMeshes and
39 animation sequences), then rendered eight matched neutral-clay views: before/after three-quarter,
side, front and distant tactical study. `form28/unreal-clay/clay-report.json` and PNGs retain the
comparison. The editor exited normally at 16:32:41 UTC and the resource reservation was released.

Visual review of all eight frames: flatter slab faces and clearer edge breaks are visible, but repeated
triangular fields look manufactured. The concept's irregular layered mineral form is not achieved.
Neither this clay material nor the distant camera study qualifies final materials or gameplay
readability. Form28 remains an unaccepted study; production21 remains the prior selected checkpoint.

### Form31 — mantle, prow and distal-leg refinement

Form31 is the current **source-checked, engine-pending study**. Authored chipped slab perimeters
replace the repeated triangular fields. Only the dorsal mantle uses a dominant slab and a smaller
forward-descending lobe; lateral wraps remain single pieces. The lower prow drops 8 cm, its side
plates and caster housings shorten, and the shin covers shorten to expose the narrow lower limbs.
Bone coordinates, sockets, Amber geometry, feet and gameplay data remain unchanged. A bounded
read-only review of the actual concept and form30 previews informed these three changes.

| State | LOD0 triangles | LOD1 triangles |
|---|---:|---:|
| Baseline | 5,232 | 2,352 |
| Carapace | 5,472 | 2,472 |
| Striker | 5,328 | 2,400 |

`form31/` retains six GLB/OBJ exports, source snapshot/hash, CPU clay preview, import job, and checks.
Three focused tests pass; all six exports retain 22 bones, five sockets, thirteen clips and COLOR_0.
All 234 endpoint/midpoint ground samples pass. This is not the full interpolated animation sweep.
The old UV texture transfer and physics/LOD receipts do not qualify this new topology.

The new form has only CPU preview evidence. Unreal/Blender/GPU remains reserved by Backend Chat
for Power Link integration qualification. No conflicting editor was launched. Form29–30 sources and
previews remain as studies; production21 remains the last selected imported checkpoint.


**Form31 full source verification:** all eight `test_fidelity_pilot` tests passed in 279.696 seconds
on source commit `2035e2b`, with file hashes checked afterward. The sweep evaluates 7,566 poses
(three states × two LODs × thirteen clips × 97 samples), and also checks rig/socket/channel exports,
exact Amber LOD geometry, firing-leg identity, unreachable-target refusal and repeatable generation.
`form31/full-tests.log` and `full-tests-receipt.json` retain the results. The attempted process-priority
adjustment was denied by the sandbox; the single Python process still ran and completed normally.

Offline firing and death poses are retained in `form31/cpu-poses/`. The death pose still has a raised
rear knee: passing ground clearance does not resolve collapse readability. The pending engine job
now includes baseline, Carapace and Striker plus the prior selected baseline, for sixteen matched
clay views. Backend Chat has not released its Power Link integration/editor reservation for that job.
