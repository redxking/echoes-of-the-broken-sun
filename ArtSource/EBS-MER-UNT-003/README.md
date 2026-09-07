---
title: EBS-MER-UNT-003 Bulwark Team — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-MC-BULWARK-TEAM
production_asset_id: EBS-MER-UNT-003
production_maturity: BLOCKOUT
revision: ebs-mer-unt-003-concept-v3
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
| Requirement card | `REL-FAC-025.MC.BULWARK.ASSET`: LOD0 ≤ 8,200 / LOD1 ≤ 3,600 tris; 2048² PBR; matte roughness floor ≥ 0.85; deployed 120° directional gradient with chromatic impact ripples; **18-bone** mechanical transformation rig; 20-tick deploy, 15-tick pack; **sub-object separation for `Left_Shield_Panel` and `Right_Shield_Panel`** |
| Requirement rows | `SPEC-UNIT-003` (Deploy Barrier: 20-tick setup, 120° frontal arc, 40% reduction, 35% speed, 15-tick pack; rear hits bypass all shield reduction), `REL-FAC-025.MC.BULWARK`, `REL-ART-005` (authored LOD0/LOD1, no popping), `REL-ART-006` (Meridian engineered load paths: orthogonal frames, machined plates, structural rails, exposed conduit lines) |
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
the wall is gone from the front read entirely (0.0% field from +X) — the six cells fold back
flanking the chassis and the silhouette collapses into a compact crawler, wider than tall, that a
player must not confuse with a deployed one. The one read this blockout does **not** deliver on
geometry alone is the reference's *see-through* packed cage: see §8 deviation 16.

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

## 4. Geometry and rig (revision `ebs-mer-unt-003-concept-v2`)

**Rest pose is DEPLOYED** — one flat frontal face — so the card's `Left_Shield_Panel` /
`Right_Shield_Panel` sub-objects export flat and the `Shield_Face_Center` socket sits on the face.
PACKED is a posed state produced by the two wing-root bones alone (`PACK_YAW` 80°, translation
(−119, ±25, −80) cm), the same transform the `deploy` and `pack` clips key.

| Group | Build |
|---|---|
| Chassis | 220 × 280 × 167 cm charcoal load frame with the **−X face omitted** (exposed rear access), pale ceramic deck (top face = 284.8, the measured chassis top), flank plates, low plates, top and mid structural rails, three vertical strakes and one cyan conduit line per side, a segmented prow (upper/lower plates, rail, block, buttresses, corner posts and vents), an underslung belly band with two axle beams, and a rear bay of machinery, racks, drums, posts, a duct, a pale floor plate and a cyan core, all visible through the open rear plane |
| Operator stations ×2 | At (18, ±52.7, 268). Ceramic torso, charcoal collar, backpack, two shoulder blocks, an octagonal pale cowl, a **cyan visor strip** on the +X face and a frame arch over the cowl. **Cowl** tops at 334.5 cm — 58.8 cm below the wall top, the reference's 97 px. The frame arch is the station's tallest part at **347.5 cm**, 45.8 cm below the wall top, inside the reference's 47…99 px machinery band. Both numbers are measured over every `op_l_*`/`op_r_*` component and both are in the manifest; reading `op_r_cowl` alone understated the station by 13 cm |
| Barrier cells ×6 | Each an 82.0 × 280.0 × 28.0 cm framed panel. Front to back along +X: a **cyan pane rim** (2.0 cm band, world x 186.1…189.0, flush with the frame's front face — the reference's lit rim, and a *front* cue), the **translucent-blue field slab** (172…178), an **opaque charcoal back plate** covering the whole pane opening (165…171) and the chamfered charcoal ring frame around all of it (161…189, 10 cm border, 15 cm corner chamfer). Two hinge barrels stand off the panel's **back** face on the inboard vertical edge, inset 6 cm so they stay inside the cell body, and four corner castings stand 3 cm proud in Z only. Nothing on a cell crosses into the 1.333 cm reveal of its neighbour, so `Left_Shield_Panel` and `Right_Shield_Panel` never sweep through each other. Cells 01–03 on the anatomical left wing (−Y), 04–06 on the right (+Y); the 03/04 seam is `y = 0` |
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
`Cell_01`…`Cell_06` at each cell's inboard hinge edge on the matching cell bone.

**Material slots (4).** `MI_EBS_MER_UnitFrame` (charcoal machined frame, hubs, hinges, treads **and
the six pane back plates**), `MI_EBS_MER_UnitCeramic` (pale ceramic plates), `MI_EBS_MER_ShieldField`
(the six cell fields only — translucent blue, the 120° directional gradient and the chromatic impact
ripple live here), `MI_EBS_MER_StatusCyan` (**1.906%** of LOD0 area: two operator visors, emitter
lens, six cell rims, chassis bay core, two flank conduits — the full list, §2).

**Budgets.** LOD0 **3,618** tris (cap 8,200), LOD1 **1,846** (cap 3,600), ratio 0.510. Sub-objects
856 / 600 tris each. Two UBX collision boxes (chassis, stance) ride the LOD0 static export only.
LOD1 costs more than the previous revision on purpose: the pane rims, the pane back plates, the
operator frame arches, the flank conduits and the bay core are all kept so that the front cue, the
rear cue and the station silhouette do not pop at the transition (§8.12).

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
| `pack` | **0.75 / 15** | no | reverse with a 4% overshoot into the cradle (the canon clank) |
| `damage` | 0.5 / 10 | no | chassis jolt and cell shudder; the cyan ripple is material work, not geometry |
| `death` | 1.6 / 32 | no | the frame tips 12.5° forward onto the front pads, the wings splay 18° open and drop 38 cm |
| `cancel` | 0.35 / 7 | no | a deploy interrupted mid-swing returns to the travel profile |
| `restore` | 0.6 / 12 | no | back to the deployed idle after damage; the wall re-seats flat |

`pack` at t = 1.0 reproduces the `packed_pose()` transform to within 0.05 cm (asserted). The impact
ripple, the 120° gradient and the field's dark state are material work on `MI_EBS_MER_ShieldField`;
no geometry implements them.

## 6. Review evidence (concept-v2 blockout stage)

Evidence root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-003/`

* `export/` (in this package) — 14 files: `SK_EBS_MER_UNT_003_LOD0/1.glb` (skinned, 10 clips, 9 sockets),
  `…_static.glb` (deployed rest, UBX collision on LOD0), `…LOD0/1.obj`, and
  `SM_EBS_MER_UNT_003_Left/Right_Shield_Panel_LOD0/1.glb|.obj` (pivot at the wing-root hinge).
* `review/` — 18 assemblies: the **shipped** `deployed` and `packed` meshes at both LODs, two baked
  material states of that same geometry, and 12 posed clip stills.
* `scenes/` + `renders/` — 25 scenes, 25 render sets (133 PNGs, plus the 15 sheets below): orthographic front/right/rear/left/top at
  authored scale with the 180 cm figure; the project's RTS tactical framing copied verbatim from
  `EBS-MER-UNT-001/scenes/rest_tactical.json` (pitch −48/−60, yaw −45, arm 3800/1400, fov 55,
  orthographic, `AEchoesRTSCameraPawn`); two close three-quarter views matched to the reference's
  PACKED TRAVEL and PACKED REAR panels; LOD1 sheets; every clip key pose; a context scene with one
  deployed and two packed Bulwarks and two Surveyors; and both states again at `PresentationScale` 1.75.
* `renders/concept-compare/` — 15 sheets, one per fidelity check plus the identity, LOD, pose,
  deploy-sequence, presentation-scale and context sheets.

**Two rules govern what a sheet may show** (both are stated in `make_sheets.py` and both are review
findings against the previous revision, which composed the check-3 and check-4 sheets entirely from
variants with the failing geometry deleted):

1. **Every fidelity check is judged on a SHIPPED review assembly** (`deployed` / `packed`). The two
   baked states only ever appear as an extra, labelled tile beside the shipped one, never in its
   place, and neither of them is what makes a check pass:
   * `packed_field_dark` — the field slot and the six pane rims re-slotted to the charcoal frame
     material. **No geometry is removed**; this is the runtime's packed material state.
   * `deployed_glass_cutaway` — the field polygons cut. This is **not** a runtime state: it is a
     stand-in for translucency (the review renderer has none), so the operators read through the
     pane exactly as the reference draws them.
2. **The first tile is a reference tile** on `check1`…`check5b`, `concept_identity` and
   `owner_correction_centred` — the concept crop or the owner-corrected reference, cropped to the
   panel being judged. The six supplementary sheets (`deploy_sequence`, `deploy_sequence_quarter`,
   `pose_sheet_right`, `pose_sheet_tactical_near`, `lod0_vs_lod1`, `presentation_scale`,
   `states_and_context`) lead with the build's own renders and are **not** fidelity evidence.

The rear and front reads are measured, not asserted: `build_bulwark.visibility_census` depth-sorts
every polygon along a view axis and reports the material slot that is *frontmost* at each of ~26,000
raster samples across the silhouette. That is the property the checks claim; a placement assertion
("no cyan polygon behind plane X") cannot fail for this geometry and is not used.

**Fidelity checklist** (each judged by looking at the sheet, not at prose about it)

| # | Check | Sheet | Verdict |
|---|---|---|---|
| 1 | Deployed front: one flat face of six cells, three per side, seam on the centreline | `check1_deployed_front.png` | **MET** — six 82.0 cm cells at pitch 83.333, span 498.7 cm, all within a 28 cm slab at x 161…189, seam at y 0.000, 3 + 3. All twelve cyan pane rims are frontmost from +X (53.4% field, 5.3% cyan of the front silhouette); the previous revision had them on the back face and showed 0 |
| 2 | Deployed side: face forward of the chassis, chassis and legs behind, wall taller than the operators | `check2_deployed_side.png` | **MET** — the whole face lies ahead of the chassis front plane (x 161 > 130); wall top 393.3 vs operator cowls 334.5 (58.8 cm, reference 59) and the station's frame arch 347.5 (45.8 cm, inside the reference's 28.5…60.0 cm band) |
| 3 | Packed three-quarter: cells folded along the chassis sides, compact crawler | `check3_packed_travel.png` | **PARTIAL** — the fold, three folded cells per side, the flanking placement, the compact crawler read and the 1.309 aspect all match, and the frontal face is gone (0.0% field from +X). Two gaps: the reference's surface density is 2048² texture work (§8.8), and its cages are *see-through* where the shipped pose still carries the field slabs — 47.1% of the flank silhouette, 0.0% once the field material is off (`packed_field_dark`, tiles 5–6). §8 deviation 16, with the impossibility measured |
| 4 | Rear view in both states: open rear access, no shield, no cyan face | `check4_rear_both_states.png` | **MET** — measured by census on the SHIPPED meshes at both LODs: deployed rear 0.0% barrier field, 0.729% cyan; packed rear 0.0% and 0.977%; **zero** cell field or rim components are frontmost from −X in either state or either LOD. The cyan that remains is the bay core inside the open rear and the two flank conduit lines, both of which the reference also shows. This is geometry — the opaque pane back plates — not a material state |
| 5 | Tactical framing: facing and deployed footprint unmistakable; the rear says "flank me" | `check5_tactical_read.png`, `check5_tactical_context.png` | **MET** — at the gameplay framing the deployed unit reads as a lit wall with a clear normal; the packed one has no face at all; the context sheet separates one deployed from two packed at a glance. Both sheets now lead with the reference's PACKED TRAVEL panel |
| — | Design identity against the KEEP concept | `concept_identity.png` | **PARTIAL by decision** — palette, ceramic-over-charcoal mass, chunky machined legs and cyan discipline are retained; the concept's *wrapping hexagonal* panels are not built, because the review decision forbids panels that imply all-round invulnerability and the owner correction made the flat centred wall the target (§8.3) |
| — | Owner correction actually applied | `owner_correction_centred.png` | **MET** — the superseded six-panel step put the wall off-centre; this build's wall centre and the chassis centreline are the same line (seam y 0.000, wall centre y 0.000) |

## 7. Reproduction

```
cd ArtSource/EBS-MER-UNT-003
EV="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-003"

python3 build_bulwark.py --evidence-dir "$EV"          # exports, review assemblies, build-manifest.json
python3 build_bulwark.py --evidence-dir "$EV" --check   # read-only: rebuilds into a temp dir, compares hashes
python3 -m unittest test_bulwark_build -v               # 64 structural tests
python3 make_scenes.py --evidence-dir "$EV" --render    # 25 scenes, 25 render sets (ebs_render.py)
python3 make_sheets.py --evidence-root "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z"
python3 "../tools/make_evidence_receipt.py" --evidence-dir "$EV" --package EBS-MER-UNT-003 \
  --manifest build-manifest.json --stage "BLOCKOUT (concept-v2)" --command "…" --note "…"
```

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
11. **Two baked material states are exported for review, and neither one certifies a check.**
    `ebs_render.py` has no translucency, so two states of the *same* geometry are baked into review
    OBJs beside the shipped `deployed` and `packed` meshes:
    * `packed_field_dark` — the field slot **and the six pane rims** re-slotted to the charcoal frame
      material. No geometry is removed. This is the runtime's packed state, and the rims are named
      here explicitly because they are `MI_EBS_MER_StatusCyan`, a different material from the field
      (deviation 15) — a previous variant removed them silently and that omission is exactly what
      made its rear tile pass.
    * `deployed_glass_cutaway` — the field polygons cut. **Not** a runtime state: a stand-in for
      translucency so the operators read through the pane as the reference draws them.
    Every fidelity verdict in §6 is measured on a shipped mesh; the two states are extra tiles.
    The runtime drives all of this with the field material; **no geometry is deleted in the engine.**
12. **LOD1 is a 196 → 83 component reduction (3,618 → 1,846 tris, ratio 0.510).** What it drops:
    the 24 cell corner castings and 6 of the 12 cell hinge barrels; the whole chassis rear-bay
    *interior* except the bay blocks and the bay core (drums, ducts, floor, header, lintel, posts,
    racks, sill); six strakes, four ribs, four rails, the flank hatches, low plates and rear plates;
    the prow block, buttresses, corner posts, rail and vents; the axle covers, belly keel, deck kerbs
    and deck rear; all 12 leg treads, 4 toe plates, 4 hub caps, 4 thigh rails and 4 thigh conduits;
    the wing struts, strut caps, hinge barrels and arm webs; and the operator collars, backpacks,
    shoulders and chest plates. What it **keeps, on purpose**, because §6's checks are certified at
    both LODs: all six cells with their chamfered outline, **the twelve cyan pane rims**, **the six
    opaque pane back plates**, the operator **frame arches** (the station's tallest part), the
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
    must light and darken **with** `MI_EBS_MER_ShieldField`, not with the visors and the lens. The
    packed read depends on it (packed front 6.3% cyan with the barrier lit, 0.15% with it off).
    Runtime requirement, recorded here because it is not something geometry can enforce inside a
    shared material slot. See OWNER-QUESTION F.
16. **The packed cages are solid, where the reference's are see-through — and no rotation or
    translation can fix it.** The reference's PACKED TRAVEL and PACKED REAR panels show the folded
    cells as open frames with the paper ground visible straight through them; the shipped packed pose
    still carries the six field slabs and reads 47.1% barrier field across each flank silhouette
    (0.0% once the field material is off — `packed_field_dark`). This is not a modelling oversight,
    it is a rig limitation, measured:
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
    Two real remedies exist and both change the asset's section contract, so both are owner calls
    (OWNER-QUESTION F): export the pane assembly as a third sub-object the runtime shows only while
    deployed, or give the pane hardware its own material section that the runtime hides when packed
    (`USkinnedMeshComponent::ShowMaterialSection`). Until then the packed read is a **runtime
    requirement**: `MI_EBS_MER_ShieldField` must be fully off in the packed state. Both reads are in
    `check3_packed_travel.png`, from the shipped mesh and from the dark state, side by side.
17. **The barrier field is 7.03% of the LOD0 surface area and is never removed from the mesh.** Any
    review of this asset that shows a packed or rear view without it must say which material state it
    is showing; §6 rule 1 exists because the previous revision did not.
18. **Cyan is on six families, not three.** `concept-fidelity.md` §7 and the earlier §2 both said
    "cyan only on the visors, the emitter lens and the cell edges", but the build also carries
    `chassis_bay_core` (6,599 cm², 9.9% of the cyan area) and `chassis_conduit_l/r` (5,632 cm² each,
    8.4% each). They are kept, because the reference draws exactly this: a blue core glow inside the
    open rear bay in its PACKED REAR panel and blue conduit accents along the hips in PACKED TRAVEL,
    and `REL-ART-006` names "exposed conduit lines" as Meridian form language. §2 now lists all six
    families. `concept-fidelity.md` is left unedited (it is the target record) — OWNER-QUESTION C
    already carries one measurement conflict against it and this is the second.

### 8.4 Open items (owner questions, routed through the coordinator — not addressed to the owner here)

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
> **OWNER-QUESTION F — how the barrier leaves the packed silhouette (§8.16).** The reference's packed
> cages are see-through; the shipped packed pose reads 47.1% barrier field across each flank because
> the card's 18-bone rotation/translation rig cannot remove a rigid pane from a pose. Three options,
> in increasing cost: (a) accept it as a runtime material requirement — `MI_EBS_MER_ShieldField` off
> while packed, which the `packed_field_dark` renders show, and which also has to cover the six pane
> rims that sit in the status slot (§8.15); (b) export the pane assembly (rims, field slabs, back
> plates) as a third sub-object beside `Left_Shield_Panel` and `Right_Shield_Panel`, which the runtime
> attaches at `Shield_Face_Center` and shows only while deployed — this makes the packed cages open by
> geometry and the deployed rear opaque by geometry, at the cost of the skinned asset no longer
> containing its own barrier; (c) give the pane hardware its own material section and hide that
> section when packed, which needs 5–6 material slots because one flat-shaded slot cannot be both a
> cyan rim and a charcoal back plate. Built as (a); (b) is the recommendation.

### 8.5 Not built at this stage

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
