---
title: EBS-MER-UNT-003 Bulwark Team — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-MC-BULWARK-TEAM
production_asset_id: EBS-MER-UNT-003
production_maturity: BLOCKOUT
revision: ebs-mer-unt-003-concept-v5
canon_status: CANDIDATE (KEEP direction under delegation; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-UNT-003 Bulwark Team — production source

Single authoritative source record for the Bulwark Team production asset. Edited in place; bounded
by the shared [agent contract](../../AGENTS.md), the [authority map](../../Docs/README.md) and the
frozen preparation records under [Docs/VisualAssetPipeline](../../Docs/VisualAssetPipeline/README.md).
The visual target is [concept-fidelity.md](concept-fidelity.md) in this folder; the deviations from
it, and the rule that forced each one, are in §8.

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-MC-BULWARK-TEAM` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json): `canon_production_brief` (Bible line 511), `gameplay_contract` (`mc_bulwark_team`), `reference_items_required` (front/side/rear/top at a shared scale, material zones, damage and construction sequences, selection and ownership at tactical camera, socket/event map, locomotion and action states) |
| Review decision | [review-selections.json](../../Docs/VisualAssetPipeline/review-selections.json) `EBS-CON-MER-UNT-003` = **KEEP**, use `DESIGN_IDENTITY`: "Retain the broad two-operator shield chassis and articulated barrier wings… Production must show one directional front, vulnerable rear and a packed state; the luminous panels cannot imply all-round invulnerability." |
| Reserved production ID | `EBS-MER-UNT-003`, planned `SK_EBS_MER_UNT_003` under `/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_003/` |
| Concept input | `EBS-CON-MER-UNT-003`, `site/assets/concepts/meridian-units.png` bottom-left `[0, 0.5, 0.5, 1]`, sha256 `427e60cd27bd78e9…`; crop at `…/concept-crops/all/EBS-PKG-MC-BULWARK-TEAM/EBS-CON-MER-UNT-003.png` |
| **Production target** | `…/concept-discovery-20260906/bulwark-review/bulwark-centered-wings-reference.png`, sha256 `6a8c5cef6697f84f…` — the **owner-corrected** derived reference ("center the 6 panes so it looks like 3 come from left 3 come from right"): DEPLOYED wall, PACKED TRAVEL, PACKED REAR VIEW |
| Superseded references | `bulwark-six-panel-reference.png` (`7574643acfde08f0…`, wall off-centre) and `bulwark-reference.png` (`55e15667ecf2c6a9…`, five visible panes). Recorded defects, not targets. |
| Canon row | DevelopmentBible.md line 511 (`SPEC-UNIT-003`): "A wide, low, two-operator chassis with a central emitter and six framed barrier cells on hinged wings. Packed, the wings tuck along the chassis and it reads as a heavy crawler; deployed, the wings unfold into a single directional shield face taller than the operators… the exposed rear says *flank me*." |
| Gameplay record | `Content/Data/Source/units.json` `mc_bulwark_team`: 130/25, 260 HP, 230 cm/s, 850 cm sight, population 3, 140-tick production, 10 damage at 300 cm every 24 ticks; `deployment` = cover_depth 350 cm, **cover_half_width 250 cm**, 40% reduction, 35% move speed |
| Requirement card | `REL-FAC-025.MC.BULWARK.ASSET`: LOD0 ≤ 8,200 / LOD1 ≤ 3,600 tris; 2048² PBR; matte roughness floor ≥ 0.85; deployed 120° directional gradient with chromatic impact ripples; **18-bone** mechanical transformation rig; 20-tick deploy, 15-tick pack; **sub-object separation for `Left_Shield_Panel` and `Right_Shield_Panel`** — still **two** sub-objects, while the ruling of 2026-09-07 adds a third; `Docs/` is outside this package's edit boundary, so the card row must be amended by the coordinator (§8.5 **note 21**) |
| Owner ruling | **2026-09-07**, implemented in this revision (§4, §8.5). **Verbatim**, character for character, from the authoritative record `ArtSource/production-ledger.json` → `owner_rulings[0].rulings[2].ruling`: "Third, separately controlled barrier sub-object. Chassis, hinged wings and six structural frames stay physically present, three panes from each side, centred when deployed. The field slabs and the removable pane assembly are separated so they disappear while packed. The component contract is updated explicitly. Visibility follows authoritative deployment state including cancellation and save restoration. The added component does not increase the whole-unit triangle budget." A **separate field** of the same ledger entry, `owner_rulings[0].boundary` — *not* part of the ruling sentence: "Clarifies production direction. The owner states these answers did not edit the cards or the runtime in that turn, and they are not evidence of completed integration." ~~concept-v4 printed an imperative-mood rewrite of the ruling here and in §8.4 under the label "the owner's words, verbatim", with a reworded copy of the boundary field folded inside the quotation marks.~~ **Struck 2026-09-07** (§8.5); `test_the_package_quotes_the_ruling_verbatim_where_it_claims_to` re-reads the ledger and fails if this row drifts from it again. |
| Requirement rows | `SPEC-UNIT-003` (Deploy Barrier: 20-tick setup, 120° frontal arc, 40% reduction, 35% speed, 15-tick pack; rear hits bypass all shield reduction), `REL-FAC-025.MC.BULWARK`, `REL-ART-005` (authored LOD0/LOD1, no popping), `REL-ART-006` (Meridian engineered load paths: orthogonal frames, machined plates, structural rails, exposed conduit lines), **`REL-ART-028`** (Meridian Roster Engineering form Language, `Docs/Requirements.md:2379`: whole-unit LOD0 ≤ **8,000** / LOD1 ≤ **3,500** — tighter than the card, and both bind) |
| Runtime envelope | If the runtime classes the Bulwark `EntityType::HeavyUnit`, `EchoesEntityView.cpp:1817` draws it at `PresentationScale` 1.75. A unit's simulation footprint half-extent is `kFixedScale / 8` (`EchoesContentSubsystem.cpp:363`) = a 25 cm square; the visual overhangs it. Both are recorded conflicts (§8, OWNER-QUESTION A and D). |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Bulwark Team is the Compact's heavy directional screen: a two-operator crawler that
walks its barrier wings into position, anchors, and unfolds them into one flat wall so Lancers can
trade from behind cover (`SPEC-UNIT-003`). It is the only Compact unit whose *facing* is a resource:
40% damage reduction inside a 120° frontal arc, 35% movement while deployed, and rear hits that
bypass the reduction entirely. Every visual decision serves one question a player asks in half a
second at the tactical camera: **which way is it facing, and where is the open side?**

**READ.** From the front, one flat luminous wall of six framed cells, wider than anything else on the
field, standing clear of the ground on four block legs, with the two operator cowls visible through
the translucent cells and the emitter lens burning at the wall's base on the centreline. From the
rear, no wall face at all: charcoal plate backs, an open chassis bay with the machinery showing, and
the operators exposed on the deck. **That rear read is geometry, not a material promise** — every
cell carries an opaque charcoal back plate behind its field, so a rear camera sees 0.0% barrier
field and 0.7% cyan (§6, measured by depth-sorted raster census, not by counting polygons). Packed,
the wall is gone entirely — from the front (0.0% field from +X) **and from the flanks (0.0%)**: the
six structural frames fold back along the chassis as **open cages** with the ground visible through
them, exactly as the reference draws them, because the barrier pane assembly is a separate object the
runtime hides while packed (owner ruling 2026-09-07, §4.1). The silhouette collapses into a compact
crawler, wider than tall, that a player must not confuse with a deployed one. What this blockout does
**not** deliver is the *timing* of that hide — the visibility contract in §4.2 is written, not wired —
and the reference's 2048² surface density, which is texture work (§8.8).

**FUNCTION MADE VISIBLE.** The wall is 500 cm across because that is exactly the cover it grants
(`cover_half_width_cm` 250, doubled). The 03/04 seam falls on the chassis centreline between the two
operators, so the wall's centre and the unit's facing vector are the same line. The wings are hinged
and their travel is the 20-tick deploy: anticipation, swing, contact, settle. Nothing about the
packed state reads protective.

**DISCIPLINE.** Meridian engineered form language (`REL-ART-006`): orthogonal frames, thick machined
plates over a charcoal load frame, structural rails and exposed conduit lines. Pale ceramic and
charcoal carry the mass. Cyan (`MI_EBS_MER_StatusCyan`, **1.906%** of the LOD0 surface area) is on
exactly six families, all of them listed here because a palette claim that omits any of them is a
false claim: the **two operator visors**, the **emitter lens**, the **six barrier-cell rims**, the
**chassis bay core** inside the open rear, and the **two flank conduit lines**. The bay core and the
flank conduits are the reference's own machined-conduit glow (visible in its PACKED REAR panel and
along the hips in PACKED TRAVEL); they carry 26.7% of the cyan area and are the only cyan the rear
shows — §8 deviation 18. The barrier field itself is a translucent blue material, not an emissive
one, and its 120° gradient means the rear never lights; the six cell rims light and darken **with**
the field, not with the status channel (§8 deviation 15).

## 3. Scale basis

`W = 500 cm`, the deployed wall width, is fixed by the deployment record: `cover_half_width_cm` 250,
doubled. Everything else is a measured ratio of `W` taken off the owner-corrected reference with a
standard-library PNG scan (wall outer frame 825 × 462 px, ground row 882, foreground threshold
sum(RGB) < 430 against a ~588 paper ground), so **1 reference px = 0.60606 cm**. The measurements are
carried in `build_bulwark.MEASURED_PX` and re-derived in the generator rather than eyeballed.

| Measure | Reference px | Ratio of W | Built (cm) |
|---|---|---|---|
| Deployed wall width | 825 (x 151…975) | 1.000 | 498.67 (six 82.0 cells at pitch 83.333) |
| Deployed wall height | 462 (y 233…695) | 0.560 | 280.0 |
| Wall bottom above ground | 187 (695…882) | 0.227 | 113.3 |
| Wall top above ground | 649 | 0.787 | 393.3 |
| Cell pitch | 135.4 | 0.164 | 83.33 (cell body 82.0) |
| Cell field | 105 × 427 | 0.127 × 0.518 | 62 × 260 |
| Operator station centres | ±87 | ±0.106 | ±52.7 (105.4 cm apart) |
| Operator **cowl** tops | wall top − 97 | 0.669 | 334.5 |
| Operator **station** top (frame arch over the cowl) | wall top − 76 (inside the reference's 47…99 px machinery band) | 0.695 | 347.5 |
| Chassis top | wall top − 179 | 0.570 | 284.8 |
| Chassis width | — (fidelity file item 1) | 0.44 | 220.0 |
| Leg hub centres | ±204 | ±0.247 | ±123.6 |
| Pad centres | ±217 | ±0.263 | ±131.5 |
| Pad width / length | 137 wide | 0.166 / 0.192 | 76 (+6 treads) / 96 |
| Packed silhouette aspect (w/h) | 1.176 (travel) … 1.444 (rear) | — | **1.309** |
| Packed cell height / packed height | 308/370 = 0.832 | — | 280 / 347.5 = 0.806 |

**Authored envelope.** Deployed 350 (X) × 498.7 (Y) × 396.3 (Z) cm; packed 350 × 454.7 × 347.5 cm
(the corner castings no longer stand proud in Y — §8 deviation 17). Pivot at
the ground-contact centre, +X forward (the shield faces +X), +Y right, +Z up, cm, Nanite off.
A 180 cm reference figure stands in every orthographic review scene.

## 4. Geometry, component contract and rig (revision `ebs-mer-unt-003-concept-v5`)

**Rest pose is DEPLOYED** — one flat frontal face — so the card's `Left_Shield_Panel` /
`Right_Shield_Panel` sub-objects export flat and the `Shield_Face_Center` socket sits on the face.
PACKED is a posed state produced by the two wing-root bones alone (`PACK_YAW` 80°, translation
(−119, ±25, −80) cm), the same transform the `deploy` and `pack` clips key.

### 4.1 Component contract (owner ruling 2026-09-07 — stated explicitly, and in `build-manifest.json`)

The barrier is **three objects, not one**. The table is the contract: what each object carries, and
the **authoritative deployment state** in which it is drawn. It is also machine-readable —
`build-manifest.json` → `component_contract.objects` — and asserted by
`test_bulwark_build.BarrierPaneSubObject` and `ManifestAgreement`.

| Export | Class | Carries | Visible while | LOD0 / LOD1 tris |
|---|---|---|---|---|
| `SK_EBS_MER_UNT_003` | skinned, 18-bone rig, 10 clips | chassis and open rear bay; two operator stations; four block legs; central emitter; two hinged wings (arms, webs, struts); **the six structural cell frames** — chamfered charcoal ring, hinge barrels, corner castings | **PACKED, DEPLOYING, DEPLOYED, PACKING** — physically present in both states, three frames per side, centred when deployed | 2,898 / 1,126 |
| `SK_EBS_MER_UNT_003_Barrier_Panes` | skinned, **the same** 18-bone rig and the same 10 clips (runtime follower) | the removable pane assembly and nothing else: six translucent **field slabs**, six **cyan pane rims**, six opaque charcoal **back plates** (18 components) | **DEPLOYED only** — hidden while PACKED, DEPLOYING and PACKING (endpoints in §4.2) | 720 / **360** |
| `SM_EBS_MER_UNT_003_Left_Shield_Panel` | static, pivot at the left wing-root hinge | left wing arm, web and struts; **cell frames 01–03**. No pane geometry | PACKED, DEPLOYING, DEPLOYED, PACKING | 496 / 240 |
| `SM_EBS_MER_UNT_003_Right_Shield_Panel` | static, pivot at the right wing-root hinge | right wing arm, web and struts; **cell frames 04–06**. No pane geometry | PACKED, DEPLOYING, DEPLOYED, PACKING | 496 / 240 |

**Why `SK_`.** ~~"Why `SK_`, not the ruling's suggested `SM_`. The ruling grants the name ('you
decide, but justify and use one name'). The assembly is skinned to the shared skeleton — the ruling's
own requirement…"~~ **Struck 2026-09-07.** That attribution was false and it is the reason this
paragraph is rewritten rather than deleted. The authoritative ruling
(`ArtSource/production-ledger.json` → `owner_rulings[0].rulings[2].ruling`, quoted verbatim in §1)
**names no asset, grants no naming licence and states no skinning requirement**; the string
`SM_EBS_MER_UNT_003_Barrier_Panes` and the phrase "you decide, but justify and use one name" came
from the production lane's own package brief for this revision, not from the owner. There is
therefore **no deviation from the ruling's letter to record** — the ruling has no letter on this
point.

The name is a free engineering choice, and it stands on its own reasoning: the assembly is
**skinned** to the shared 18-bone skeleton and bound to the six cell bones — a decision taken here,
so that hiding it cannot disturb the frames, the deploy or the pack — and `SM_`/`SK_` is this
project's static/skinned mesh-class prefix, so `SM_` would misname a skinned asset; the two wing
panels are `SM_` precisely because they are static, re-pivoted re-exports. One name is used
everywhere: constant `build_bulwark.BARRIER_PART`, both export stems, the manifest, this table and
the tests.

**How it follows the frames.** Every pane polygon is bound to the same `cell_01`…`cell_06` bone as the
frame it fills, on a skeleton asserted bone-for-bone identical to the main mesh's, and the export's
pivot is the shared root — not a re-pivoted hinge. So the runtime drives it as a follower skinned
component off the main mesh's pose, and hiding it moves nothing: the hiding test hashes the
main-mesh geometry with the assembly drawn and with it hidden, deployed and packed, and the two
hashes match. The pane-tracking test checks every pane stays inside its own frame's box
(±1 cm) at seven sampled poses across `deploy`, `pack`, `idle_packed`, `idle_deployed` and `death`.

**Coverage, measured.** `build_bulwark.pane_coverage_measure` samples each frame's inner opening
(260 samples per cell) and asks a +X camera what it sees, twice: on the frames-only main mesh the
opening is open — the samples land on the chassis, the operators or the sky, never on that cell — and
on the shipped deployed assembly **1,512 of 1,512** opening samples are that cell's own rim or field.
All six frames fully covered, none partially.

**LOD1 of the pane assembly is authored, not duplicated.** ~~concept-v4 shipped
`SK_EBS_MER_UNT_003_Barrier_Panes_LOD1` byte-identical to its LOD0 — 720/720, 2,160 verts, the same
153,321 bytes apart from one comment line: a duplicate asset with no reduction, against
`REL-ART-005`'s authored-LOD0/LOD1 requirement.~~ **Struck 2026-09-07 (review finding).** The
"panes" half of `part_cell` now reads `lod` as the "frame" half always did, and reduces **120 → 60
triangles per cell, 720 → 360 for the assembly (0.500)** — in line with the main mesh's 0.389 and the
wing sub-objects' 0.484. What it drops is only surface no LOD1 camera can reach, so the two cues §6
certifies are pixel-identical and the silhouette does not move (asserted at both LODs by
`test_the_pane_assembly_lod1_is_an_authored_reduction_not_a_duplicate` and
`test_the_lod1_reduction_keeps_both_certified_cues`):

| Element | LOD0 | LOD1 | What LOD1 drops |
|---|---|---|---|
| cyan pane rim | 64 | 16 | the rim keeps **only its front annulus**, coplanar with the LOD0 rim's front face; its 2.9 cm depth, inner and outer walls and back annulus sit inside the frame opening and are never visible |
| field slab | 28 | 22 | the slab's **back** cap — the opaque back plate covers it at every angle, at both LODs |
| back plate | 28 | 22 | the plate's **front** cap — the field slab covers it. Its **back** cap, the surface check 4 measures from −X, is kept |

The assembly's bounds, its 18 components and every rim's front plane are identical at both LODs, so
there is no LOD pop on the front or the rear cue. Both files are still exported: LOD0 and LOD1 are
now genuinely different meshes and the coordinator's import job should carry both.

### 4.2 The visibility contract (a written contract; **this blockout cannot enforce it**)

> The barrier pane assembly is visible **if and only if the authoritative deployment state is
> DEPLOYED**. It is hidden while PACKED, and hidden through the DEPLOYING and PACKING transitions.
>
> **The endpoints, stated once.** The rule is the *state*, not the clip. The DEPLOYED state is
> **entered at `deploy` t = 1.0** and **left at `pack` t > 0**, so `deploy` t = 1.0 and `pack`
> t = 0.0 are DEPLOYED frames, not transition frames, and the assembly is drawn in exactly those
> two. Every other frame of `deploy` and `pack` is a transition frame and it is hidden; `cancel`
> never reaches DEPLOYED at all. There is no frame where the state is DEPLOYED and the assembly is
> hidden, and none where it is not DEPLOYED and the assembly is drawn.

~~concept-v4 stated the rule in §4.2 and then contradicted it inside a single sentence in §6, and its
test docstring disagreed with its own assertions; the `pack` t = 0 case was disclosed nowhere in
prose.~~ **Struck 2026-09-07 (review finding).** The paragraph above is the one statement, and
`build_bulwark.pane_assembly_visible`, `visibility_contract()["state_endpoints"]`, §6 and
`test_the_visibility_contract_follows_the_authoritative_state_not_the_clip` now repeat this wording.

* **Driven by** the authoritative simulation deployment state, read when that state changes. **Never**
  by an animation notify, a montage or clip event, a presentation-only toggle, or by which clip
  happens to be playing.
* **Cancellation.** A deploy cancelled mid-swing never reaches DEPLOYED, so the assembly is never
  shown and there is nothing to clean up; `cancel` returns the frames to the travel profile.
* **Save restoration.** On load the saved deployment state sets the visibility directly, before the
  first frame is drawn. No clip has to play and no notify has to fire for a packed unit to load with
  open cages or a deployed unit to load with its wall.
* **Where the change lands.** The single visibility change coincides with the authoritative cover
  state change (the 40% frontal reduction coming on or going off), which the player is already being
  told about, and the field material's own fade covers it. The looser alternative — hide only while
  PACKED, visible through both transitions — was considered and **rejected**: it puts the change at
  the *end* of the pack, where the measured flank silhouette loses 47.1% of its area in one frame
  (`packed_state.packed_flank_visibility_panes_shown`).
* **Not enforced here.** This is a **written contract, not an implemented one.** This blockout ships
  the separation the rule needs — a third object that can be hidden without disturbing the frames —
  and nothing more. Wiring visibility to the authoritative state, and proving it survives cancellation
  and save restoration, is **integration work in the runtime**. The owner's ruling says the same thing:
  "This is my selected implementation direction, not evidence of completed integration."

The contract is carried in code (`build_bulwark.visibility_contract`, `pane_assembly_visible`) so that
**every review still in §6 is drawn under it** — no still can show a barrier the authoritative state
would not be showing (`test_every_review_pose_is_drawn_under_the_visibility_contract`).

| Group | Build |
|---|---|
| Chassis | 220 × 280 × 167 cm charcoal load frame with the **−X face omitted** (exposed rear access), pale ceramic deck (top face = 284.8, the measured chassis top), flank plates, low plates, top and mid structural rails, three vertical strakes and one cyan conduit line per side, a segmented prow (upper/lower plates, rail, block, buttresses, corner posts and vents), an underslung belly band with two axle beams, and a rear bay of machinery, racks, drums, posts, a duct, a pale floor plate and a cyan core, all visible through the open rear plane |
| Operator stations ×2 | At (18, ±52.7, 268). Ceramic torso, charcoal collar, backpack, two shoulder blocks, an octagonal pale cowl, a **cyan visor strip** on the +X face and a frame arch over the cowl. **Cowl** tops at 334.5 cm — 58.8 cm below the wall top, the reference's 97 px. The frame arch is the station's tallest part at **347.5 cm**, 45.8 cm below the wall top, inside the reference's 47…99 px machinery band. Both numbers are measured over every `op_l_*`/`op_r_*` component and both are in the manifest; reading `op_r_cowl` alone understated the station by 13 cm |
| Barrier cells ×6 | Each an 82.0 × 280.0 × 28.0 cm framed panel, now split across two objects (§4.1). **In the main mesh (the structural frame):** the chamfered charcoal ring (x 161…189, 10 cm border, 15 cm corner chamfer), two hinge barrels standing off the panel's **back** face on the inboard vertical edge, inset 6 cm so they stay inside the cell body, and four corner castings 3 cm proud in Z only. **In the pane assembly:** front to back along +X, a **cyan pane rim** (2.0 cm band, world x 186.1…189.0, flush with the frame's front face — the reference's lit rim, and a *front* cue), the **translucent-blue field slab** (172…178) and an **opaque charcoal back plate** covering the whole pane opening (165…171). All three sit inside the ring's inner opening, so the assembly fills the cage exactly and hiding it leaves an open frame. Nothing on a cell crosses into the 1.333 cm reveal of its neighbour, so `Left_Shield_Panel` and `Right_Shield_Panel` never sweep through each other. Cells 01–03 on the anatomical left wing (−Y), 04–06 on the right (+Y); the 03/04 seam is `y = 0` |
| Barrier wings ×2 | Hinge barrel at the chassis front shoulder (128, ±118, 250), two arm rails plus a ceramic web out to the wall's rear face, a ceramic strut down the chassis flank and a strut cap |
| Legs ×4 | Hub disc (r 26.7) at (±123.6, z 92) on a charcoal mount up to the chassis underside, a plated thigh with a rail and a conduit, an ankle block, a ceramic boot, a 96 × 76 × 42 cm pale pad at ±131.5 and three charcoal tread blocks plus a toe plate, flush with `z = 0` |
| Central emitter | Octagonal charcoal housing (r 19) at (184, 0, 114) on a stalk off the prow, ceramic collar, **cyan lens** (r 8) facing +X. It straddles the wall's bottom rail on the 03/04 seam when deployed and reads as the nose lens when packed |

**Rig — 18 bones, identity rest orientation, no root motion**

| Bone | Parent | Head (cm) | Purpose |
|---|---|---|---|
| `root` | — | (0, 0, 0) | ground-contact centre |
| `chassis` | root | (0, 0, 150) | carries every other bone |
| `operator_l` / `operator_r` | chassis | (18, ∓52.7, 268) | operator stations |
| `leg_fl` / `leg_fr` / `leg_rl` / `leg_rr` | chassis | (76 or −98, ±123.6, 92) | block legs, hub pivots |
| `emitter` | chassis | (184, 0, 114) | emitter housing; drives `Emitter_Muzzle` |
| `shield_anchor` | chassis | (175, 0, 253.3) | deployed face centre; drives `Shield_Face_Center`, the impact-ripple origin and the cover volume |
| `wing_root_l` / `wing_root_r` | chassis | (128, ∓118, 250) | barrier wing hinges |
| `cell_01`…`cell_06` | the matching wing root | (175, hinge y, 253.3) | per-cell hinges, inboard vertical edge |

**Sockets** (exact names from the fidelity target): `Target_Anchor_Center` (0, 0, 200) on `chassis`;
`Emitter_Muzzle` (198, 0, 114) on `emitter`; `Shield_Face_Center` (189, 0, 253.3) on `shield_anchor`;
`Cell_01`…`Cell_06` at each cell's inboard hinge edge on the matching cell bone. All nine ride the
main skinned mesh. The pane assembly re-declares the seven that concern it — `Shield_Face_Center` and
`Cell_01`…`Cell_06`, on the same bones at the same positions — so it is self-describing if inspected
alone; the two shield-panel sub-objects each carry their three `Cell_*` sockets, as before.

**Material slots (4), shared by both skinned objects.** `MI_EBS_MER_UnitFrame` (charcoal machined
frame, hubs, hinges, treads, the six structural cell frames **and the six pane back plates**),
`MI_EBS_MER_UnitCeramic` (pale ceramic plates), `MI_EBS_MER_ShieldField` (the six cell field slabs
only — translucent blue, the 120° directional gradient and the chromatic impact ripple live here),
`MI_EBS_MER_StatusCyan` (**1.906%** of the whole unit's LOD0 area: two operator visors, emitter lens,
six cell rims, chassis bay core, two flank conduits — the full list, §2). The main mesh uses three
slots (frame, ceramic, cyan) and **carries no `MI_EBS_MER_ShieldField` section at all**; the pane
assembly uses three (field, cyan, frame). A Bulwark drawn without its pane assembly therefore cannot
show a barrier field — asserted, per LOD, in `test_the_barrier_field_slot_lives_only_in_the_pane_assembly`.

**Budgets — the split (owner ruling: the added component does not increase the whole-unit budget).**
Two ceilings bind and both hold: the card's 8,200 / 3,600 and `REL-ART-028`'s tighter **8,000 / 3,500**.

| | LOD0 | LOD1 |
|---|---|---|
| `SK_EBS_MER_UNT_003` main mesh | 2,898 | 1,126 |
| `SK_EBS_MER_UNT_003_Barrier_Panes` | 720 | **360** |
| **Whole unit drawn (main + panes)** | **3,618** | **1,486** |
| `SM_…_Left_Shield_Panel` (re-export) | 496 | 240 |
| `SM_…_Right_Shield_Panel` (re-export) | 496 | 240 |
| All four exports summed | 4,610 | 1,966 |
| Cap: card / `REL-ART-028` | 8,200 / **8,000** | 3,600 / **3,500** |

At LOD0 the whole unit is **3,618** — *exactly* what concept-v3's single mesh carried. The split moved
geometry between objects and created none (`test_the_split_did_not_grow_the_whole_unit` re-derives the
720 from six identical per-cell pane builds and checks the main mesh's cell triangles equal six frame
builds). At LOD1 it **fell** from concept-v3's 1,846 to **1,486**, because concept-v5 authors a real
reduction for the pane assembly (720 → 360; ~~concept-v4 shipped an LOD1 identical to its LOD0~~ —
struck 2026-09-07, §4.1). The two wing sub-objects lost their pane geometry with the main mesh, so
even the literal sum of all four exports **fell**, 5,330 → 4,610 at LOD0 and 3,046 → 1,966 at LOD1.
Whole-unit LOD1/LOD0 ratio **0.411** (was 0.510 while the panes did not reduce). Two UBX collision boxes (chassis, stance) ride the LOD0 static export only.
LOD1 keeps the pane rims, the pane back plates, the operator frame arches, the flank conduits and the
bay core so that the front cue, the rear cue and the station silhouette do not pop at the transition
(§8.12). Note for the coordinator: `build-manifest.json` → `budgets.lod0_triangles` remains the **main
mesh** because `ebs_make_import_job.py` feeds it to the import inspector as the expectation for
`SK_EBS_MER_UNT_003`; the ruling's number is `budgets.whole_unit_lod0_triangles`, and
`budgets.lod0_within_cap` / `lod1_within_cap` report the whole unit against both ceilings.

## 5. Clips (keyframed, Unreal rotators; §4 rig)

20 simulation ticks = 1 s (`SPEC-UNIT-003`: 140-tick production = 7.0 s). Chassis `z` in every clip is
**solved**, not authored: `solve_ground` rewrites the chassis track so the lowest pad corner sits on
`z = 0` at every key, then densely re-checks the interpolation and raises the bracketing keys until
the whole clip stays on the ground. Every sampled pose lands in `[-0.001, +0.29] cm`.

| Clip | s / ticks | Loop | Content |
|---|---|---|---|
| `idle_packed` | 2.0 / 40 | yes | travel profile held; slow hydraulic settle |
| `move_packed` | 1.2 / 24 | yes | four-leg crawler shuffle at 230 cm/s |
| `deploy` | **1.0 / 20** | no | anticipation (wings load outward 4.5°), swing (42%), contact (3.5% overshoot past flat), settle to the flat frontal wall |
| `idle_deployed` | 2.4 / 48 | yes | low field hum; the cells breathe ±0.45° on their hinges |
| `drag_deployed` | 2.0 / 40 | yes | 35%-speed drag; the wall stays flat and frontal |
| `pack` | **0.75 / 15 authored → 0.7667 exported (23 f)** | no | reverse with a 4% overshoot into the cradle (the canon clank) |
| `damage` | 0.5 / 10 | no | chassis jolt and cell shudder; the cyan ripple is material work, not geometry |
| `death` | 1.6 / 32 | no | the frame tips 12.5° forward onto the front pads, the wings splay 18° open and drop 38 cm |
| `cancel` | 0.35 / 7 authored → 0.3667 exported (11 f) | no | a deploy interrupted mid-swing returns to the travel profile |
| `restore` | 0.6 / 12 | no | back to the deployed idle after damage; the wall re-seats flat |

~~The `pack` and `cancel` cells printed the authored durations alone.~~ **Struck 2026-09-07 (review
finding):** they were the pre-retime numbers the concept-v3 amendment superseded, and the shipped
GLBs contradicted them. `ebs-skelkit-v2` refuses any clip whose duration is not a whole frame at
30 fps, so both were snapped up to the next whole frame; the exported values above are what
`SK_EBS_MER_UNT_003_LOD0.glb` and `SK_EBS_MER_UNT_003_Barrier_Panes_LOD0.glb` actually carry. Full
reasoning and the UE 5.8.2 evidence: the **concept-v3 amendment** at the end of this file.

`pack` at t = 1.0 reproduces the `packed_pose()` transform to within 0.05 cm (asserted). The impact
ripple, the 120° gradient and the field's dark state are material work on `MI_EBS_MER_ShieldField`;
no geometry implements them.

## 6. Review evidence (concept-v5 blockout stage)

Evidence root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-003/`

* `export/` (in this package) — 18 files: `SK_EBS_MER_UNT_003_LOD0/1.glb` (skinned, 10 clips, 9 sockets),
  `…_static.glb` (deployed rest, UBX collision on LOD0), `…LOD0/1.obj`,
  `SM_EBS_MER_UNT_003_Left/Right_Shield_Panel_LOD0/1.glb|.obj` (pivot at the wing-root hinge) and
  **`SK_EBS_MER_UNT_003_Barrier_Panes_LOD0/1.glb|.obj`** — the third sub-object, skinned to the same
  18-bone rig with the same 10 clips, pivot at the shared root (§4.1).
* `review/` — 20 assemblies: the **shipped** `deployed` (main mesh + pane assembly) and `packed`
  (assembly hidden) meshes at both LODs, three non-shipped review variants (captioned in every sheet
  that uses them — see the labelling rule below), and 13 posed clip stills, each drawn under the
  visibility contract.
* `scenes/` + `renders/` — 28 scenes, 28 render sets (150 PNGs; 174 under `renders/` in total, with the 16 sheets below and the 8 captioned copies in `renders/labelled/`): orthographic front/right/rear/left/top at
  authored scale with the 180 cm figure; the project's RTS tactical framing copied verbatim from
  `EBS-MER-UNT-001/scenes/rest_tactical.json` (pitch −48/−60, yaw −45, arm 3800/1400, fov 55,
  orthographic, `AEchoesRTSCameraPawn`); two close three-quarter views matched to the reference's
  PACKED TRAVEL and PACKED REAR panels; LOD1 sheets; every clip key pose; a context scene with one
  deployed and two packed Bulwarks and two Surveyors; and both states again at `PresentationScale` 1.75.
* `renders/concept-compare/` — 16 sheets, one per fidelity check plus the identity, LOD, pose,
  deploy-sequence, presentation-scale, context and **`barrier_sub_object`** sheets; and beside each
  sheet its **`<sheet>.tiles.json` sidecar**, naming every tile in composition order with its kind
  (reference / shipped / non-shipped) and its caption.
* `renders/labelled/` — the 8 captioned copies of the non-shipped tiles that the sheets are
  composed from. The original render sets are never modified.

**Two rules govern what a sheet may show** (both are stated in `make_sheets.py` and both are review
findings against earlier revisions, which composed the check-3 and check-4 sheets entirely from
variants with the failing geometry deleted):

1. **Every fidelity check is judged on a SHIPPED review assembly.** `deployed` is the main mesh **with**
   the pane assembly; `packed` is the main mesh with the assembly **hidden** — both are what the
   authoritative state draws. Three non-shipped tiles exist; each may only ever appear *beside* a
   shipped tile, and none of them is what makes a check pass:
   * `deployed_glass_cutaway` — the field slabs **and the opaque back plates** cut. **Not** a runtime
     state: a stand-in for translucency (the review renderer has none), so the chassis and the two
     operators read through the pane as the reference draws them through the glass. ~~"the field
     polygons cut … so the operators read through the pane exactly as the reference draws them"~~ —
     **struck 2026-09-07 (review finding):** cutting the field alone left the six charcoal back
     plates in place, so the tile showed a flat plate behind every cyan rim and *no operator at all*
     — an unlit, field-off barrier, exactly the class of variant the ruling forbids as a stand-in for
     a state. Measured then and now on the same 1,188-sample +X ray cast over the six frame openings
     (`build_bulwark.cutaway_read_measure`, in the manifest under `review_variants`): concept-v4,
     **1,140 samples (96.0%) frontmost on that cell's own back plate and 0 on any operator, chassis
     or sky**; concept-v5, **0 blocked by a back plate, 1,140 (96.0%) reading through to the chassis,
     the operators, the wings or the sky** — 92 of them onto an operator component — with the
     remaining 48 on the cells' own corner castings, which are frame, not opening.
   * `deployed_frames_only` — **separation control.** The deployed pose with the assembly hidden, so
     the six frames the third sub-object fills can be seen on their own.
   * `packed_panes_shown` — **integration failure control.** The packed pose with the assembly still
     drawn: what concept-v3 shipped, and what the runtime must not do.

   **They are labelled in the image, not only here.** ~~"three clearly labelled non-shipped tiles";
   "each is labelled"~~ — **struck 2026-09-07 (review finding):** that was true of this prose and
   false of the artifact. `ArtSource/tools/ebs_sheet.py` defines `read_png`, `write_png`, `fit` and
   `main` and has no text-drawing code at all, so no composed sheet carried a caption; a row-by-row
   scan of `barrier_sub_object.png` found every gutter band a single flat `RGB(52,52,56)` and no
   glyph anywhere, and the only text in any check sheet was burnt into the upstream concept art. The
   consequence was that `check3_packed_travel.png` — the sheet for the very ruling item about the
   packed read — ended on **two uncaptioned tiles of a packed Bulwark with a fully lit barrier**,
   the failure control, sitting directly below three shipped packed tiles. The sheet tool is
   read-only here, so `make_sheets.py` now carries its own 5×7 bitmap blitter: `label_png` burns
   **NOT SHIPPED — TRANSLUCENCY STAND-IN / SEPARATION CONTROL / INTEGRATION FAILURE CONTROL** into a
   warning band across the top of a **copy** of the render (`renders/labelled/`), and every sheet is
   composed from that copy, so the label is in the sheet's own pixels. Every sheet also gets a
   `<sheet>.tiles.json` sidecar naming each tile in order. Asserted by `test_bulwark_build`
   `SheetTileLabelling`: every non-shipped tile of every sheet has a caption, every shipped tile has
   none, the band is drawn and the render below it is byte-identical to the original.
   The `packed_field_dark` material-state assembly of concept-v3 is **gone**, along with its two
   scenes and render sets. Under the owner ruling the packed read is geometry, so no field-off variant
   stands in for a shipped state — which is exactly what the ruling forbids.
2. **The first tile is a reference tile** on `check1`…`check5b`, `concept_identity` and
   `owner_correction_centred` — the concept crop or the owner-corrected reference, cropped to the
   panel being judged. The **eight** supplementary sheets (`barrier_sub_object`, `deploy_sequence`,
   `deploy_sequence_quarter`, `pose_sheet_right`, `pose_sheet_tactical_near`, `lod0_vs_lod1`,
   `presentation_scale`, `states_and_context`) lead with the build's own renders and are **not**
   fidelity evidence.

**Posed stills obey the visibility contract.** Every clip sample is rendered with the pane assembly
shown only where the authoritative state is DEPLOYED. In the same words as §4.2: the DEPLOYED state
is **entered at `deploy` t = 1.0** and **left at `pack` t > 0**, so `deploy` t = 1.0 and `pack`
t = 0.0 are DEPLOYED frames and the assembly is drawn in them, while every other frame of `deploy`
and `pack` is a transition frame and it is hidden. Hidden, therefore, in `idle_packed`,
`move_packed`, `cancel` and in every transition frame of `deploy` and `pack`; shown in
`idle_deployed`, `drag_deployed`, `damage`, `death`, `restore` and at the two endpoint frames
`deploy` t = 1.00 and `pack` t = 0.00 — the `deploy` t = 1.00 sample is the one that ends the deploy
sequence on the finished wall instead of on a last mid-swing frame of six empty cages.
~~concept-v4 wrote "hidden … through the whole of `deploy` and `pack`; shown in … and at `deploy`
t = 1.00" — both halves of one sentence — and disclosed the `pack` t = 0 case nowhere.~~ **Struck
2026-09-07 (review finding).**

The rear and front reads are measured, not asserted: `build_bulwark.visibility_census` depth-sorts
every polygon along a view axis and reports the material slot that is *frontmost* at each of ~26,000
raster samples across the silhouette. That is the property the checks claim; a placement assertion
("no cyan polygon behind plane X") cannot fail for this geometry and is not used.

**Fidelity checklist** (each judged by looking at the sheet, not at prose about it)

| # | Check | Sheet | Verdict |
|---|---|---|---|
| 1 | Deployed front: one flat face of six cells, three per side, seam on the centreline | `check1_deployed_front.png` | **MET** — six 82.0 cm cells at pitch 83.333, span 498.7 cm, all within a 28 cm slab at x 161…189, seam at y 0.000, 3 + 3. All twelve cyan pane rims are frontmost from +X (53.4% field, 5.3% cyan of the front silhouette); the previous revision had them on the back face and showed 0 |
| 2 | Deployed side: face forward of the chassis, chassis and legs behind, wall taller than the operators | `check2_deployed_side.png` | **MET** — the whole face lies ahead of the chassis front plane (x 161 > 130); wall top 393.3 vs operator cowls 334.5 (58.8 cm, reference 59) and the station's frame arch 347.5 (45.8 cm, inside the reference's 28.5…60.0 cm band) |
| 3 | Packed three-quarter: cells folded **as open frames** along the chassis sides, compact crawler | `check3_packed_travel.png`, `barrier_sub_object.png` | **MET on the read; PARTIAL on surface only** — the fold, three folded cells per side, the flanking placement, the compact crawler read and the 1.309 aspect all match, the frontal face is gone (0.0% field from +X), and **the cages are now see-through on the SHIPPED packed mesh**: 0.0% barrier field on both flanks, with that flank's three `cell_*_frame` components still frontmost and the ground visible through them. It is geometry, not a material state — the pane assembly is a separate object and the packed mesh contains no pane component at all. Tiles 5–6 are the failure control (the same pose with the assembly drawn: 47.1%), so the 0.0% cannot pass vacuously. The one remaining gap is the reference's surface density, which is 2048² texture work (§8.8) |
| 4 | Rear view in both states: open rear access, no shield, no cyan face | `check4_rear_both_states.png` | **MET** — measured by census on the SHIPPED meshes at both LODs. LOD0: deployed rear 0.0% barrier field and 0.729% cyan, packed rear 0.0% and 0.977%. LOD1: deployed rear 0.0% and 1.23%, packed rear 0.0% and 1.82% — the cyan share rises only because LOD1 has less total surface, not because more cyan is drawn. **Zero** cell field or rim components are frontmost from −X in either state or either LOD. The cyan that remains is the bay core inside the open rear and the two flank conduit lines, both of which the reference also shows. Deployed, this is geometry — the opaque pane back plates, which travel *with* the pane assembly so a hidden assembly can never leave a lit field with its backing gone. Packed, there is no pane geometry to hide |
| 5 | Tactical framing: facing and deployed footprint unmistakable; the rear says "flank me" | `check5_tactical_read.png`, `check5_tactical_context.png` | **MET** — at the gameplay framing the deployed unit reads as a lit wall with a clear normal; the packed one has no face at all; the context sheet separates one deployed from two packed at a glance. Both sheets now lead with the reference's PACKED TRAVEL panel |
| — | Design identity against the KEEP concept | `concept_identity.png` | **PARTIAL by decision** — palette, ceramic-over-charcoal mass, chunky machined legs and cyan discipline are retained; the concept's *wrapping hexagonal* panels are not built, because the review decision forbids panels that imply all-round invulnerability and the owner correction made the flat centred wall the target (§8.3) |
| — | Owner correction actually applied | `owner_correction_centred.png` | **MET** — the superseded six-panel step put the wall off-centre; this build's wall centre and the chassis centreline are the same line (seam y 0.000, wall centre y 0.000) |
| — | Owner ruling 2026-09-07: the third sub-object | `barrier_sub_object.png` | **IMPLEMENTED as a blockout separation, NOT as integration** — the sheet pairs deployed-with-assembly against deployed-frames-only, and shipped-packed against packed-with-the-assembly-still-drawn. What it certifies is the separation and the two reads. What it cannot certify is that the runtime hides the assembly on the authoritative state; that is §4.2 and it is unimplemented |

## 7. Reproduction

```
cd ArtSource/EBS-MER-UNT-003
EV="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-003"

python3 build_bulwark.py --evidence-dir "$EV"          # exports, review assemblies, build-manifest.json
python3 build_bulwark.py --evidence-dir "$EV" --check   # read-only: rebuilds into a temp dir, compares hashes
python3 -m unittest test_bulwark_build -v               # 95 structural tests
python3 make_scenes.py --evidence-dir "$EV" --render    # 28 scenes, 28 render sets (ebs_render.py)
python3 make_sheets.py --evidence-root "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z"
python3 "../tools/make_evidence_receipt.py" --evidence-dir "$EV" --package EBS-MER-UNT-003 \
  --manifest build-manifest.json --stage "BLOCKOUT (concept-v5: review corrections to the concept-v4 record of the owner ruling 2026-09-07)" \
  --command "…" --note "…"
```

**Confirming the tests would fail on the old contract.** The ruling's tests are only worth their
words if concept-v3 fails them. Reconstruct concept-v3's geometry distribution by reverting the split
alone — put the rim, field slab and back plate back inside `part_cell(..., "frame")` at LOD0
resolution for both LODs, and let `barrier_assembly` return an empty mesh — and run this same test
module against it. The reconstruction is faithful (main mesh **3,618 / 1,846**, exactly concept-v3's
counts) and, **re-run at concept-v5, 18 of the 95 tests fail** (16 failures, 2 errors; 5 skipped
because the scratch reconstruction carries no `build-manifest.json`). ~~"18 of the 81"~~ was the
concept-v4 measurement. The failures include every claim the ruling makes, plus the pane assembly's
authored LOD1: the packed mesh carrying no field
slab, the packed flanks being open cages by geometry, the main mesh carrying no pane geometry in
either state or LOD, the assembly carrying the panes and nothing else, the assembly covering all six
frames, the assembly following the same cell bones, the shield-field slot living only in the
assembly, the whole-unit LOD ceilings, the split not growing the whole unit, the three-sub-object
separation, every review pose obeying the visibility contract, every field slab being backed by an
opaque plate, and the pane assembly's LOD1 being a reduction rather than a duplicate. Running the module against the *unmodified*
concept-v3 source (`git show HEAD:ArtSource/EBS-MER-UNT-003/build_bulwark.py`) fails too — 12 of 58
collected — but mostly with `AttributeError`, which proves less; the reverted-split run is the
meaningful control because it isolates the contract from the API.

`--check` never rewrites an export: it rebuilds into a temporary directory and compares every export
and review hash with `build-manifest.json`. `make_scenes.py --dry-run` and `make_sheets.py --dry-run`
print the exact `ebs_render.py` / `ebs_sheet.py` command lines so the renders and sheets reproduce
from the record alone. Standard library only; no DCC and no engine is involved at this stage.

## 8. Decisions, deviations and open items

### 8.1 Rig and contract

1. **Eighteen bones.** The card requires an 18-bone transformation rig; the fidelity file's prose list
   ("root, chassis, two operator bones, four leg bones, two wing-root bones and six cell bones")
   enumerates **16**. The remaining two are `emitter` and `shield_anchor`, each of which drives one of
   the card's named sockets and each of which must move independently of the chassis shell (the
   emitter tracks the seam, the anchor carries the impact-ripple origin and the cover volume). Rule
   cited: `REL-FAC-025.MC.BULWARK.ASSET` .ANIM_RIG. OWNER-QUESTION B.
2. **The packed transform needs a translation, not only a hinge.** Each wing spans from the
   centreline to its outer edge (0 → ±249 cm), because the deployed wall must be continuous across the
   03/04 seam. A pure rotation about any fixed chassis hinge therefore cannot put a wing along the
   chassis *side*: rotating about a centreline hinge lands both wings on the centreline, and rotating
   about a side hinge either overshoots the centre or leaves a 300 cm gap in the middle. The card's
   own wording is the mechanism ("uncoupling side shielding to form a rigid frontal wall"), so the
   wing-root bones **yaw 80° and translate** (−119, ±25, −80) cm. Both parts are keyframed by `deploy`
   and `pack`; nothing else in the rig moves.
3. **Packed cells drop 80 cm.** Measured on the reference: in PACKED TRAVEL the folded cage tops sit
   26 px *below* the operator cowl tops and the cage bottoms 36 px *above* the pads, so the cells
   cannot stay at their deployed height. Built: cells 33.3…313.3 cm packed against 113.3…393.3 deployed.
4. **Collision.** Two UBX boxes (chassis, stance) ride the LOD0 *static* export only; the skinned
   export carries none, following EBS-MER-UNT-001. A physics asset and the deployed-state cover volume
   are integration work, not blockout work.

### 8.2 Measurement conflicts inside the target record

5. **Operator and chassis heights.** [concept-fidelity.md](concept-fidelity.md) states "chassis+legs
   height ≈ 0.40 W (200 cm) to the operator tops". A pixel scan of the same owner-corrected reference
   puts the operator cowl tops 97 px below the wall top and the chassis top 179 px below it, i.e. at
   **334.5 cm (0.669 W)** and **284.8 cm (0.570 W)**. The panel is near-orthographic (cells 01 and 06
   render 105 and 104 px wide, 423 and 423 px tall), so foreshortening cannot account for the
   difference. The owner ruling — the concepts define what the asset should be — and the task
   instruction that decisions come from the pixels put the measurement first, so the build follows the
   pixels. `concept-fidelity.md` is **left unedited**; the conflict is OWNER-QUESTION C.
6. **The wall floats 113 cm above the ground.** That is the measured reference (187 px of open leg
   under the wall), not a modelling choice, and it is why the deployed unit is 396 cm tall. It leaves
   an open band under the shield; the 40% reduction is a simulation rule that does not care, but it is
   worth a look — OWNER-QUESTION E.

### 8.3 Deviations from the concept image, each with the rule that forced it

7. **The concept's wrapping hexagonal panel shell is not built.** `EBS-CON-MER-UNT-003` shows
   overlapping hexagonal luminous panels wrapping the chassis on both sides. The review decision on
   that exact concept forbids it — "the luminous panels cannot imply all-round invulnerability" — and
   the owner's correction made the flat, centred six-cell wall the production target. Retained from
   the concept: pale ceramic over charcoal, brass-trimmed machined plate language, the broad
   two-operator chassis, the chunky block legs with tracked pads, the octagonal nose emitter with its
   cyan lens, and the cyan discipline — 1.906% of the LOD0 area, on the six families §2 lists.
8. **Brass edge trim, bolt courses, plate seams, scuff noise and the team-colour mask are texture
   channels, not geometry** (2048² PBR stack in the card; LOD0 budget). This is the single largest
   remaining visual gap against the reference and the reason fidelity check 3 is PARTIAL.
9. **The emitter is nose-mounted on a stalk.** The reference draws it at the wall's base when
   deployed and on the chassis nose when packed. One part cannot be in both places unless it rides the
   chassis, so the housing sits at x 184 — straddling the wall's bottom rail on the seam in the
   deployed state and standing 54 cm proud of the chassis prow when packed. Both reads are in the
   sheets (`check1`, `check3`).
10. **Four material slots.** The card sets no slot ceiling for this asset and the barrier field needs
    its own translucent, gradient-driven material, so the export carries frame / ceramic / field /
    status-cyan. EBS-MER-UNT-001's two-slot rule is a Surveyor-card constraint and does not apply here.
11. **Non-shipped review tiles, and none of them certifies a check.** ~~`packed_field_dark` — the
    field slot and the six pane rims re-slotted to the charcoal frame material, no geometry removed:
    the runtime's packed material state.~~ **Struck 2026-09-07 (owner ruling).** The packed read is
    geometry now, so a field-off material variant would be exactly the "field-off review variant
    standing in for a shipped state" the ruling forbids; the assembly, its two scenes and its two
    render sets are deleted. What remains beside the shipped `deployed` and `packed` meshes:
    * `deployed_glass_cutaway` — the field slabs **and the opaque back plates** cut. **Not** a
      runtime state: a stand-in for translucency (`ebs_render.py` has none) so the chassis and the
      operators read through the pane as the reference draws them. ~~The back plates used to stay,
      which showed a flat plate and no operator at all.~~ Struck 2026-09-07 — measured in §6.
    * `deployed_frames_only` — separation control: the deployed pose with the pane assembly hidden.
    * `packed_panes_shown` — integration failure control: the packed pose with the assembly still
      drawn, i.e. what concept-v3 shipped.
    Every fidelity verdict in §6 is measured on a shipped mesh; these three are extra tiles, and
    since concept-v5 each of them carries **NOT SHIPPED** and its role burned into the tile image in
    every sheet that uses it (§6, `make_sheets.label_png`). ~~"labelled tiles"~~ was struck and made
    true on 2026-09-07: before that, no sheet carried any caption at all.
    **No geometry is deleted in the engine**: the runtime hides a component, it does not edit a mesh.
12. **LOD1 is a 196 → 83 component reduction in the main mesh, and 3,618 → 1,486 tris across the
    whole unit (ratio 0.411).** ~~"3,618 → 1,846 tris, ratio 0.510"~~ — **struck 2026-09-07:** that
    ratio was carried entirely by the main mesh (1,126/2,898 = 0.389) because the pane assembly did
    not reduce at all; it now reduces 720 → 360 (§4.1). What LOD1 drops:
    the 24 cell corner castings and 6 of the 12 cell hinge barrels; the whole chassis rear-bay
    *interior* except the bay blocks and the bay core (drums, ducts, floor, header, lintel, posts,
    racks, sill); six strakes, four ribs, four rails, the flank hatches, low plates and rear plates;
    the prow block, buttresses, corner posts, rail and vents; the axle covers, belly keel, deck kerbs
    and deck rear; all 12 leg treads, 4 toe plates, 4 hub caps, 4 thigh rails and 4 thigh conduits;
    the wing struts, strut caps, hinge barrels and arm webs; and the operator collars, backpacks,
    shoulders and chest plates. What it **keeps, on purpose**, because §6's checks are certified at
    both LODs: all six cells with their chamfered outline, **the twelve cyan pane rims** (reduced to
    their front annulus, on the same plane), **the six opaque pane back plates** (reduced to the
    walls and the back cap check 4 measures), the operator **frame arches** (the station's tallest part), the
    **two flank conduit lines** and the **chassis bay core**. Measured across the transition: wall
    span and wall top within 1 cm, operator station top identical at 347.5 cm, every bound within
    4 cm, and zero barrier panes frontmost from −X at either LOD (`REL-ART-005.AUTH`).
13. **Cell corner castings stand 3 cm proud in Z only.** They used to stand proud in Y as well, which
    made every cell overlap its neighbour by 4.66 cm across a 1.333 cm reveal and pushed the Left and
    Right shield panels 9.33 cm past the centreline into each other — the two sub-objects the card
    requires *for physical transformation* swept through one another during deploy and pack. Now the
    castings are flush in Y and the hinge barrels are inset 6 cm and moved to the panel's back face,
    so no cell component's AABB intersects another cell's (asserted for all 15 cell pairs) and each
    wing part stays strictly on its own side of `y = 0`. Cost: the deployed bounds read 498.7 × 396.3
    rather than 504.7 × 396.3 — the wall no longer has proud castings at its far left and right ends,
    only along its top and bottom edges.
14. **The pane rim is on the FRONT face and every pane has an opaque back plate.** The rim used to be
    modelled at the panel's rear (world x 160.1…163.0), so the reference's defining *front* cue was
    invisible from the front and fully lit from behind; with no back plate the rear of the wall was a
    fully lit six-cell barrier — 27.3% field and 4.15% cyan of the rear silhouette — which is the
    all-round-invulnerability read the KEEP review decision forbids. Built now: rim at 186.1…189.0
    (flush with the frame front), field at 172…178, opaque charcoal back plate at 165…171 covering
    the whole pane opening. Measured on the shipped mesh: rear 0.0% field / 0.729% cyan, front 53.4%
    field with all twelve rims frontmost. Rule cited: `EBS-CON-MER-UNT-003` KEEP —
    "the luminous panels cannot imply all-round invulnerability".
15. **The six pane rims are a state of the barrier, not of the status channel.** They are authored in
    `MI_EBS_MER_StatusCyan` so the review renderer draws them as the reference's bright rim, but they
    must light and darken **with** `MI_EBS_MER_ShieldField`, not with the visors and the lens — while
    the barrier is *knocked down but still deployed*, which the material owns. ~~The packed read
    depends on it (packed front 6.3% cyan with the barrier lit, 0.15% with it off).~~ **Struck
    2026-09-07 (owner ruling):** the packed case no longer depends on the material at all. The rims
    are inside the removable pane assembly, so hiding the assembly takes them with the field —
    measured 0.587% cyan on each packed flank and 0.977% on the packed rear, all of it the bay core
    and the flank conduits. The remaining requirement is a deployed-state one and is still runtime work.
16. ~~**The packed cages are solid, where the reference's are see-through — and no rotation or
    translation can fix it.**~~ **SUPERSEDED 2026-09-07 by the owner ruling** (§8.5): the barrier
    pane assembly is a third sub-object, so the shipped packed mesh carries no pane geometry and the
    cages *are* see-through — 0.0% barrier field on both flanks, measured on the shipped mesh. The
    analysis below is kept because it is still true and it is why the fix had to be a section
    contract rather than a modelling or rig change; only its **conclusion** is struck.
    The reference's PACKED TRAVEL and PACKED REAR panels show the folded
    cells as open frames with the paper ground visible straight through them; concept-v3's packed pose
    still carried the six field slabs and read 47.1% barrier field across each flank silhouette (that
    number is now the `packed_panes_shown` failure control). This was not a modelling oversight,
    it was a rig limitation, measured:
    * `ebs_skelkit.Keyframe` carries `rotation_deg` and `translation_cm` and **no scale track**
      (`ArtSource/tools/ebs_skelkit.py`, verified against the installed UE 5.8.2 importer), so a
      rigid 62 × 260 cm pane is present in every pose of the 18-bone rig the card fixes;
    * the packed fold turns each pane's normal to within 10° of ±Y, so the flank camera looks
      straight down the pane normal. A recess, a louvre, a back plate or backface culling can only
      hide a surface seen at a grazing angle; none of them can hide one seen face-on. The same pane
      normal is why the deployed *rear* read (viewed along the opposite direction of the same axis)
      **can** be fixed with a back plate and this one cannot;
    * stowing the panes on a non-cell bone was measured and rejected: six 62 × 260 × 6 cm panes do
      not fit inside a 220 × 280 × 167 cm chassis without a per-pane bone, and translating them below
      the ground plane would inflate the asset bounds and the culling volume by 390 cm.
    Two real remedies existed and both changed the asset's section contract, so both were owner calls
    (OWNER-QUESTION F): export the pane assembly as a third sub-object the runtime shows only while
    deployed, or give the pane hardware its own material section that the runtime hides when packed
    (`USkinnedMeshComponent::ShowMaterialSection`). ~~Until then the packed read is a **runtime
    requirement**: `MI_EBS_MER_ShieldField` must be fully off in the packed state.~~ **The owner chose
    the third sub-object (§8.5); it is built.** `check3_packed_travel.png` now shows the shipped mesh
    beside the failure control instead of beside a material state.
17. ~~**The barrier field is 7.03% of the LOD0 surface area and is never removed from the mesh.**~~
    **Struck 2026-09-07 (owner ruling).** It is 7.03% of the whole unit's LOD0 area and it is removed
    from the *drawn* unit while packed — by hiding a component, never by editing a mesh. The rule it
    protected still stands and is now stronger: any review of this asset that shows a packed or rear
    view must say which **object set** it is showing, and §6 rule 1 forbids a non-shipped tile from
    standing in for a shipped one. That rule exists because two earlier revisions broke it.
18. **Cyan is on six families, not three.** `concept-fidelity.md` §7 and the earlier §2 both said
    "cyan only on the visors, the emitter lens and the cell edges", but the build also carries
    `chassis_bay_core` (6,599 cm², 9.9% of the cyan area) and `chassis_conduit_l/r` (5,632 cm² each,
    8.4% each). They are kept, because the reference draws exactly this: a blue core glow inside the
    open rear bay in its PACKED REAR panel and blue conduit accents along the hips in PACKED TRAVEL,
    and `REL-ART-006` names "exposed conduit lines" as Meridian form language. §2 now lists all six
    families. `concept-fidelity.md` is left unedited (it is the target record) — OWNER-QUESTION C
    already carries one measurement conflict against it and this is the second.

### 8.4 Open items (owner questions, routed through the coordinator — not addressed to the owner here)

**F is ANSWERED** by the ruling of 2026-09-07 and implemented in this revision (§8.5). **A, B, C, D
and E remain OPEN**; the ruling did not touch them and nothing below has changed.

> **OWNER-QUESTION A — presentation scale against the authoritative cover.** If the runtime classes
> the Bulwark `EntityType::HeavyUnit`, `EchoesEntityView.cpp:1817` draws it at 1.75, so the authored
> 500 cm face would draw **875 cm** wide against a 500 cm authoritative `cover_half_width_cm × 2`.
> Options: (a) keep the concept-measured 500 cm asset and change the Bulwark's presentation scale to
> 1.0 (a runtime change), (b) author the face at 500/1.75 = 286 cm so the drawn wall equals the cover
> (breaks the fidelity file's scale basis), or (c) accept the mismatch as readability licence. Both
> reads are rendered side by side in `presentation_scale.png` and `check5_tactical_read.png`. Built as
> (a)-ready: authored at 500 cm, unscaled.
>
> **OWNER-QUESTION B — bone budget.** Confirm `emitter` and `shield_anchor` as bones 17 and 18. The
> card says 18; the fidelity file's list totals 16 (§8.1). If the owner wants a different pair, the two
> bones are one constant block away and no geometry moves.
>
> **OWNER-QUESTION C — the fidelity file's 0.40 W operator height.** The pixels of the same
> owner-corrected reference give 0.669 W (§8.2). Confirm the measurement and amend
> `concept-fidelity.md`, or rebuild the chassis and operators to 200 cm — which would put the wall
> 193 cm above the operators instead of the reference's 59 cm and change the whole silhouette.
>
> **OWNER-QUESTION D — footprint.** Deployed the unit is 505 × 350 cm on a Logistics Footprint 3,
> against a simulation footprint half-extent of `kFixedScale / 8` = a 25 cm square
> (`EchoesContentSubsystem.cpp:363`). Confirm the visual overhang is acceptable for a screen unit, or
> register a deployed footprint / cover volume as an integration task.
>
> **OWNER-QUESTION E — the open band under the wall.** The reference puts the wall bottom 113 cm off
> the ground with the legs fully visible under it. Confirm that is intended (it is what the pixels
> show) rather than a wall that reaches the ground when anchored.
>
> **OWNER-QUESTION F — how the barrier leaves the packed silhouette (§8.16). ANSWERED 2026-09-07 —
> option (b), implemented in concept-v4 and corrected in concept-v5 (§4.1, §4.2, §8.5).** The owner's
> words, **verbatim** — character for character from the authoritative record,
> `ArtSource/production-ledger.json` → `owner_rulings[0].rulings[2].ruling`:
>
> > "Third, separately controlled barrier sub-object. Chassis, hinged wings and six structural frames
> > stay physically present, three panes from each side, centred when deployed. The field slabs and
> > the removable pane assembly are separated so they disappear while packed. The component contract
> > is updated explicitly. Visibility follows authoritative deployment state including cancellation
> > and save restoration. The added component does not increase the whole-unit triangle budget."
>
> A **separate field** of the same ledger entry, `owner_rulings[0].boundary` — *not* part of the
> ruling sentence, and quoted here as its own line rather than inside the quotation marks above:
>
> > "Clarifies production direction. The owner states these answers did not edit the cards or the
> > runtime in that turn, and they are not evidence of completed integration."
>
> **Paraphrase** (labelled as one, kept because it is how the lane read the ruling): choose the third,
> separately controlled barrier sub-object; keep the chassis, hinged wings and six structural frames
> physically present, three panes from each side, centred when deployed; separate the field slabs and
> the removable pane assembly so they disappear while packed; update the component contract
> explicitly; visibility follows the authoritative deployment state including cancellation and save
> restoration; the added component does not increase the whole-unit triangle budget.
>
> ~~concept-v4 printed that paraphrase here under the label "The owner's words, verbatim", rewritten
> throughout to the imperative mood, with "associated" inserted and a reworded copy of the entry's
> separate `boundary` field ("This is my selected implementation direction, not evidence of completed
> integration.") folded inside the quotation marks.~~ **Struck 2026-09-07 (review finding).** The
> substance survived the rewrite; the label "verbatim" did not.
> `test_the_package_quotes_the_ruling_verbatim_where_it_claims_to` re-reads the ledger and fails if
> either quoted block drifts from it again.
>
> The question as posed: the reference's packed cages are see-through; concept-v3's packed pose read
> 47.1% barrier field across each flank because the card's 18-bone rotation/translation rig cannot
> remove a rigid pane from a pose. Options offered were (a) a runtime material requirement, (b) the
> third sub-object, (c) a per-pane material section. **(b) is built.** Both parts of the deployed
> read that used to depend on a material now depend on geometry, and the one thing that still depends
> on the runtime — *when* the assembly is shown — is written down in §4.2 as an unenforced contract,
> exactly as the ruling's last sentence requires.

### 8.5 Owner ruling of 2026-09-07 — the third, separately controlled barrier sub-object

Implemented in revision `ebs-mer-unt-003-concept-v4` and corrected in
`ebs-mer-unt-003-concept-v5`. The ruling text is quoted **verbatim** in §1 and in the
OWNER-QUESTION F block above, both taken character for character from
`ArtSource/production-ledger.json` → `owner_rulings[0].rulings[2].ruling`; the clause column below is
a **paraphrase for indexing**, not a quotation. ~~"Implemented verbatim"~~ — struck 2026-09-07: what
was implemented is faithful, but concept-v4 quoted a rewrite under the label "verbatim" and the label
is what is being corrected. What each clause became:

| Ruling clause | Where it is built | Evidence |
|---|---|---|
| "Choose the third, separately controlled barrier sub-object" | `build_bulwark.barrier_assembly`, exported as `SK_EBS_MER_UNT_003_Barrier_Panes_LOD0/1` in both `.glb` and `.obj` | §4.1 table; `component_contract` in the manifest |
| "Keep the chassis, hinged wings, and six structural frames physically present — three panes from each side, centered when deployed" | `part_cell(..., "frame")` stays in the main skinned mesh at both LODs and in both states | `test_the_six_structural_frames_stay_in_the_main_mesh_in_both_states`; `deployed_frames_only` and `packed` renders; seam y = 0.000, 3 + 3 |
| "Separate the field slabs and associated removable pane assembly so they disappear while packed" | `part_cell(..., "panes")` — rim, field slab, back plate — moved out entirely | packed shipped mesh: **0 field polygons, 0 pane components**, 0.0% field on both flanks; `check3_packed_travel.png` tiles 5–6 are the failure control |
| "Update the component contract explicitly" | §4.1 table here **and** `build-manifest.json` → `component_contract` | `test_the_manifest_states_the_component_contract_explicitly` |
| "Visibility must follow authoritative deployment state, including cancellation and save restoration" | §4.2, `visibility_contract()`, `pane_assembly_visible()` | Written contract only — **not implemented, not enforceable here**; every review still is drawn under it |
| "the added component does not increase the whole-unit triangle budget" | Whole unit 3,618 / **1,486**: LOD0 identical to concept-v3, LOD1 lower since concept-v5 authors the pane assembly's LOD1 (720 → 360). Both ceilings hold at both LODs | §4 budget table; `test_lod_ceilings_hold_for_the_WHOLE_UNIT_not_just_the_main_mesh` |
| "not evidence of completed integration" | §4.2 and §8.6; acceptance unchanged at NOT_EVALUATED / NOT_ACCEPTED | — |

~~**One deviation from the ruling's letter, taken under the licence it grants:** the name. The ruling
suggested `SM_EBS_MER_UNT_003_Barrier_Panes` and added "you decide, but justify and use one name". The
assembly is skinned to the shared skeleton — the ruling's own requirement — so the project's `SM_`
static-mesh prefix would misname it…~~

**Struck 2026-09-07 (review finding). There is no deviation, because there is no such clause.** The
authoritative ruling — `ArtSource/production-ledger.json` → `owner_rulings[0].rulings[2].ruling`,
quoted verbatim in §1 — **names no asset, grants no naming licence and states no skinning
requirement**. A search of the whole worktree and the whole evidence root for
`SM_EBS_MER_UNT_003_Barrier_Panes` and for "you decide, but justify and use one name" returns only
this package's own files asserting them; there is no source anywhere. Both came from the production
lane's own package brief for this revision and concept-v4 recorded them as the owner's words. That
was wrong, and recording a "deviation from the ruling's letter" against a clause the owner never
wrote was worse.

**What is true:** the ruling leaves the name entirely open, so `SK_EBS_MER_UNT_003_Barrier_Panes` is a
**free engineering choice**, justified on its own terms — the assembly is skinned to the shared
18-bone skeleton and bound to the six cell bones (this build's decision, so hiding it cannot disturb
the frames, the deploy or the pack), and `SM_`/`SK_` is this project's static/skinned mesh-class
prefix. One name everywhere: `build_bulwark.BARRIER_PART`, both export stems, the manifest, §4.1 and
the tests. Nothing in the ruling was reinterpreted. The same correction is carried in
`build_bulwark.py` at `BARRIER_PART` and in the evidence receipt's note 1, and
`test_words_the_owner_never_wrote_appear_only_inside_a_struck_correction` fails if either phrase is
ever restated as fact again.

**Three integration notes the coordinator needs.**

19. **The concept-v3 import record does not cover this revision, and the shared import-job tool cannot
    express it.** `import/import-report-ebs-mer-unt-003-concept-v3.json` was run against the
    single-skinned-mesh concept-v3 exports. `ArtSource/tools/ebs_make_import_job.py` assumes **one**
    skinned asset per package: it names skinned rows by LOD alone (`<asset>`, `<asset>_LOD1Source`),
    so a manifest with two skinned meshes produces a job with duplicate asset names — the barrier
    panes would be imported as if they were `SK_EBS_MER_UNT_003`. A concept-v4 job generated from
    this manifest was therefore **deleted rather than left in the evidence**, because a job that
    mis-names an asset is worse than no job. The tool is outside this package's edit boundary
    (`ArtSource/tools/` is read-only here), so extending it to a list of skinned assets — and adding
    an expectation row for `SK_EBS_MER_UNT_003_Barrier_Panes` — is a coordinator task that must land
    before this revision is imported.
20. **The runtime work this asset now waits on.** A follower skinned-mesh component for
    `SK_EBS_MER_UNT_003_Barrier_Panes` bound to the main mesh's pose, and its visibility driven from
    the authoritative deployment state per §4.2 — including the cancellation and save-restoration
    paths, which need their own tests in the runtime, not here.
21. **The requirement card still names two sub-objects; this revision ships three.**
    `Docs/Requirements.md:2397` (`REL-FAC-025.MC.BULWARK.ASSET .MESH_PROP`) reads: "Sub-object
    separation required for `Left_Shield_Panel` and `Right_Shield_Panel` components to handle
    physical transformation." The ruling of 2026-09-07 adds a third,
    `SK_EBS_MER_UNT_003_Barrier_Panes` (DEPLOYED-only, §4.1), and the owner's own boundary line says
    that ruling **did not edit the cards**. `Docs/` is outside this package's edit boundary, so the
    card row must be amended by the **coordinator** — a third sub-object, named, with its visibility
    state — **before the technical gate is evaluated**; until then the card and the shipped contract
    disagree and the gate would be judged against a stale row. Raised here for the same reason as
    note 19: this package cannot edit the file that has to change. ~~concept-v4 raised note 19 for
    the import tool but raised nothing for the card.~~ Struck 2026-09-07 (review finding).

### 8.6 Not built at this stage

Textures and the 2048² PBR stack; the 120° directional gradient and the chromatic impact ripple
(material work at `Shield_Face_Center`); Niagara concussive flash at `Emitter_Muzzle` and the
decoupled death debris (card .VFX_POLY); the selection and team-ownership treatment; a physics asset;
UV atlas packing. No Unreal import has been run from this package — the coordinator runs imports
serially. Acceptance: art gate NOT_EVALUATED, gameplay gate NOT_EVALUATED, technical gate
NOT_EVALUATED, owner NOT_ACCEPTED.

## Concept-v3 amendment — clip durations on the 30 fps frame grid (2026-09-07)

The concept-v2 exports imported into the sandbox project with **`pack` and `cancel`** missing: the Interchange
skeletal import creates no AnimSequence for a clip whose duration is not an integer number of frames at
30 fps, logs no warning and still reports success. A dedicated probe (twelve clips of identical shape,
durations from 1 to 30 frames including four half-frame values) reproduced it exactly: every whole-frame
clip imported with its exact length, every half-frame clip vanished. Evidence:
`…/asset-production-20260906T221157Z/skeletal-clip-duration-probe/` (`probe-record.json`).

concept-v3 therefore snaps every authored duration up to the next whole frame and scales the key times
with it, so the pose at any normalized time — and every posed review still — is unchanged. The rule is
now enforced in the shared kit: `ebs_skelkit.write_skinned_glb` refuses to write an unaligned clip
(`ANIMATION_FPS`, `frame_aligned_duration()`, `retime_clip()`, `SKELETAL_ENCODING['clip_duration']`,
kit revision `ebs-skelkit-v2`), and this package's tests assert that every clip is frame-aligned.

Retimed here: `pack` 0.75 s → 0.7667 s (23 frames) and `cancel` 0.35 s → 0.3667 s (11 frames). SPEC-UNIT-003's 15-tick pack is 22.5 frames and has no frame-exact representation at 30 fps; the next whole frame is authored and the runtime still drives the authoritative 15-tick timing (recorded as a deviation). The import of the concept-v3 exports reports 0 errors with every clip present
(`import/import-report-ebs-mer-unt-003-concept-v3.json`, run 2 of `import/heavy-run-receipt.json`).

## Concept-v4 amendment — the third barrier sub-object (owner ruling, 2026-09-07)

*This section is the dated record of what concept-v4 did, and the numbers in it are concept-v4's.
Five of its claims were corrected by review; see the concept-v5 amendment below.*

The owner answered OWNER-QUESTION F by selecting the third, separately controlled barrier sub-object.
concept-v4 implements it: the chassis, the two hinged wings and the **six structural cell frames**
stay physically present in `SK_EBS_MER_UNT_003` in both states, three per side, centred when deployed;
the **field slabs, cyan pane rims and opaque back plates** move into
`SK_EBS_MER_UNT_003_Barrier_Panes`, skinned to the same 18-bone rig and bound to the same
`cell_01`…`cell_06` bones, so the runtime can hide the whole assembly while packed without disturbing
the frames, the deploy or the pack.

What changed, measured: the shipped **packed** mesh now contains **no pane geometry at all** — 0 field
polygons, 0 pane components — and reads **0.0% barrier field on both flanks** where concept-v3 read
47.1%; that 47.1% survives as the `packed_panes_shown` failure control so the new number cannot pass
vacuously. The deployed read is unchanged: 53.4% field and all twelve pane rims frontmost from +X,
0.0% field and 0.729% cyan from −X, the wall flat and centred with the 03/04 seam on y = 0.000, and
**1,512 of 1,512** frame-opening samples covered by that cell's own pane. The whole-unit triangle
budget did not grow: **3,618 / 1,846**, identical to concept-v3, inside both the card's 8,200 / 3,600
and `REL-ART-028`'s tighter 8,000 / 3,500; the literal sum of all four exports fell from 5,330 to
4,610 at LOD0. (Concept-v5 authors the pane assembly's LOD1, so the whole unit is now **3,618 /
1,486** — see below.)

The `packed_field_dark` material-state review assembly, its two scenes and its two render sets are
**deleted** — under the ruling the packed read is geometry, so no field-off variant may stand in for a
shipped state. A `deploy` t = 1.00 pose sample is added so the deploy sequence ends on the finished
wall rather than on empty cages. Tests: 64 → **81** at concept-v4, **95** at concept-v5, of which 18
fail against a faithful reconstruction of the concept-v3 contract (§7).

**Boundary.** This is the separation, not the integration. The pane assembly's visibility must follow
the authoritative deployment state, including cancellation and save restoration (§4.2) — that is a
written contract this blockout cannot enforce, and the owner's ruling says so in its own last
sentence. `ebs_make_import_job.py` cannot yet express a package with two skinned assets (§8.5 note 19),
so no concept-v4 or concept-v5 import has been run and none should be until that tool is extended.
The requirement card still names two sub-objects and must be amended by the coordinator (§8.5
note 21). Acceptance is unchanged: art gate NOT_EVALUATED, gameplay gate NOT_EVALUATED, technical
gate NOT_EVALUATED, owner NOT_ACCEPTED.

## Concept-v5 amendment — review corrections to the concept-v4 record (2026-09-07)

Two independent verifications of concept-v4 were run against the shipped geometry, the exported
rigs and clips, the rendered pixels and the authoritative ruling text in
`ArtSource/production-ledger.json`. What the geometry does was not in dispute: the third sub-object,
the six frames staying in the main mesh, the packed mesh carrying no pane component, the coverage,
the bone binding and the budget all verified. What failed verification was the **record** — five
claims that the artifacts did not support — and one shipped file that carried no reduction. Nothing
below is deleted; every corrected claim is struck in place and dated, as this package's rule requires.

(Every cell in the middle column is a **withdrawn** concept-v4 claim, quoted so the correction can be
checked against it. None of them stands.)

| # | What concept-v4 claimed — withdrawn | What the artifact showed | Where the correction lands |
|---|---|---|---|
| 1 | ~~The ruling suggested `SM_EBS_MER_UNT_003_Barrier_Panes` and granted a naming licence, "you decide, but justify and use one name"; skinning to the shared skeleton was "the ruling's own requirement". A "deviation from the ruling's letter" was recorded against it~~ — **struck 2026-09-07** | **None of it is in the ruling.** `owner_rulings[0].rulings[2].ruling` names no asset, grants no licence and states no skinning requirement; the phrases came from the lane's own package brief. There was no clause to deviate from | §4.1 "Why `SK_`", §8.5, `build_bulwark.py` at `BARRIER_PART`, receipt note 1 — all struck and rewritten. `SK_` is now recorded as a free engineering choice with its own justification |
| 2 | §1 and §8.4 printed a restatement under the label "the owner's words, verbatim", rewritten to the imperative mood, with "associated" inserted and a reworded copy of the entry's separate `boundary` field folded inside the quotation marks | The substance survived; the label did not | §1 and §8.4 now carry the ledger string character for character, with the `boundary` field quoted as its own line and the paraphrase kept beside it and **labelled** as a paraphrase |
| 3 | `deployed_glass_cutaway` made "the operators read through the pane exactly as the reference draws them" | Cutting the field alone left the six opaque back plates: **1,140 of 1,188 opening samples (96.0%) frontmost on a back plate, 0 on any operator** — an unlit, field-off barrier, the class of variant the ruling forbids as a stand-in for a state | `glass_cutaway` now cuts the back plates too; `cutaway_read_measure` measures it (0 blocked, 1,140 reading through, 92 onto an operator); §6 and §8 item 11 restated; the tile re-rendered and re-sheeted |
| 4 | "three clearly labelled non-shipped tiles"; "each is labelled" (×3, plus `make_sheets.py`) | **No sheet carried any caption at all.** `ebs_sheet.py` has no text-drawing code; a row scan of `barrier_sub_object.png` found every gutter band one flat colour. `check3_packed_travel.png` ended on two uncaptioned failure-control tiles below three shipped packed tiles | `make_sheets.py` gained a 5×7 blitter: **NOT SHIPPED — …** is burned into a copy of every non-shipped render (`renders/labelled/`, 8 files) and every sheet is composed from the copy; each sheet also gets a `<sheet>.tiles.json` sidecar. Asserted by `SheetTileLabelling` |
| 5 | Nothing routed the requirement card's two-sub-object row to anyone, though note 19 routed the import tool | `Docs/Requirements.md:2397` still names two sub-objects while this package ships three, and `Docs/` is outside the edit boundary | **§8.5 note 21**, worded like note 19: the coordinator must amend the card row before the technical gate is evaluated |
| 6 | `SK_EBS_MER_UNT_003_Barrier_Panes_LOD1`, 720 tris — "disclosed" but unjustified | Byte-identical to LOD0 apart from one comment line: **no reduction at all**, against `REL-ART-005`'s authored-LOD0/LOD1 requirement | The "panes" half of `part_cell` now reads `lod`: **720 → 360** (§4.1 table of what each element drops). Whole unit LOD1 1,846 → **1,486**, ratio 0.510 → **0.411** |
| 7 | §4.2, §6 and the test docstring disagreed about the transition endpoints; the `pack` t = 0 case was in no prose at all | The code was consistent; the three prose statements were not | The endpoint rule is stated once in §4.2 and repeated word for word in §6, `visibility_contract()["state_endpoints"]` and the test docstring |
| 8 | The §5 clip table printed `pack` 0.75 s and `cancel` 0.35 s | The shipped GLBs carry 0.7667 s (23 f) and 0.3667 s (11 f) — the concept-v3 retime, disclosed 370 lines later | Both cells now print "authored → exported", with a pointer to the concept-v3 amendment |
| 9 | "The seven supplementary sheets (…)" | Eight are listed and eight are built | §6: "eight" |

**What did not change.** No shipped read moved. The deployed face is still flat, frontal and centred
with the 03/04 seam on `y = 0.000`; the rear is still 0.0% barrier field at both LODs in both states;
the packed flanks are still 0.0% field with the three cell frames frontmost; coverage is still
1,512 / 1,512; both ceilings still hold at both LODs, now with more headroom. `--check` reports
`ok` with zero drift, and the reconstruction control in §7 still fails 18 tests.

**One review item is not this package's to fix.** A verifier noted that `git status` in this worktree
shows 15 modified files under `ArtSource/EBS-MER-UNT-002/` alongside this package's own. They are a
concurrent Lancer lane's, not this one's; the receipt's `source_identity.dirty_paths` lists them, and
nothing is dirty under `Docs/`, `Content/`, `Source/`, `Config/`, `ArtSource/tools/` or
`ArtSource/production-ledger.json`. Confirming and landing them is a **coordinator** action. (Note
for reproduction: `git status` on this volume aborts on an LFS filter unless run with
`-c filter.lfs.process= -c filter.lfs.required=false`.)

**Boundary, unchanged.** This is still the separation, not the integration. The pane assembly's
visibility must follow the authoritative deployment state including cancellation and save
restoration (§4.2); that is a written contract this blockout cannot enforce. Acceptance: art gate
NOT_EVALUATED, gameplay gate NOT_EVALUATED, technical gate NOT_EVALUATED, owner NOT_ACCEPTED.
