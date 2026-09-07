---
title: EBS-MER-UNT-001 Surveyor — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
package: EBS-PKG-MC-SURVEYOR
production_asset_id: EBS-MER-UNT-001
production_maturity: BLOCKOUT
revision: ebs-mer-unt-001-concept-v3
canon_status: CANDIDATE (KEEP direction under delegation; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-UNT-001 Surveyor — production source

Single authoritative source record for the Surveyor production asset (the rig/animation pilot of
the owner's production handoff). Edited in place; bounded by the shared [agent contract](../../AGENTS.md),
the [authority map](../../Docs/README.md) and the frozen preparation records under
[Docs/VisualAssetPipeline](../../Docs/VisualAssetPipeline/README.md). Production-lane maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `production_policy[EBS-PKG-MC-SURVEYOR]`, gap GAP-01 (three rear canisters in one transverse cradle; anatomical right arm drill, left gripper with recessed palm welder; two planted feet; tools stop on travel/cancel; delivery empties cargo only on an authorized transfer) in [gap-decisions.json](../../Docs/VisualAssetPipeline/motion/gap-decisions.json) |
| Prepared amendment | `MERIDIAN-ANATOMY`: two articulated legs and planted feet, not treads (prepared, not applied to the master) |
| Reserved production ID | `EBS-MER-UNT-001`, planned `SK_EBS_MER_UNT_001` under `/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_001/` |
| Concept inputs | `EBS-CON-MER-UNT-001` (original sheet, KEEP: compact worker frame, paired working arms, visible cargo; simplify the mast panel; loaded/unloaded cargo readable), `EBS-CON-MER-PRP-001` (manipulator tools) |
| Selected candidate | `surveyor-review/surveyor-reference.png`, sha256 `98c3a0ef…6fd913` (derived reference for review); the derived turnaround in the same folder is a **rejected** modelling reference (canister-count drift, reversed side labels) |
| Canon row | DevelopmentBible.md line 509 (`SPEC-UNIT-001`): "compact bipedal maintenance exoframe about as tall as a person and half again as wide at the shoulder: pale ceramic torso shell over a charcoal frame, two articulated tool arms (rotary drill on one, gripper/welder on the other), a small optical mast, and a rear cargo cradle holding cyan-white Matter canisters. Cyan status band across the chest." |
| Book | ¶142 walking worker frames that build and carry; ¶1535 drill too strong for the last layer, gripper lifts fragments by hand; ¶1606 fresh patch over a chipped shoulder, empty cargo cradle after delivery |
| Gameplay record | `SPEC-UNIT-001`: 360 cm/s, Logistics Footprint 1 (= population cost, see §3), work rate 10, cargo 10 Matter, unarmed; Network Repair channel within 200 cm |
| Requirement card | `REL-FAC-025.MC.SURVEYOR.ASSET`: LOD0 ≤4,500 / LOD1 ≤1,800 tris, 2048² stack, team colour by mask, emissive ≤5%, 12-bone kinematic rig, sockets `Harvest_Tether_Muzzle` / `Cargo_Drop_Anchor` / `Center_Hitbox_Socket`; `SPEC-ART-002` worker animation set; `SPEC-MOV-010` presentation facing (720°/s sweep, runtime-owned); `REL-ART-009` code-driven kinematic motion synchronized to authoritative velocity |
| Existing envelope | Visual placeholder `SM_Meridian_Surveyor` (~180 cm tall, ~100 cm across the arms) and the M01 articulated parts (`meridian_surveyor_body`, `m01_surveyor_upper/lower/foot`: thigh 42 cm, shin 46 cm, foot 32 cm, pivots at the origin along local +X) driven by `AEchoesEntityView::UpdateM01SurveyorRig` (`EchoesM01SurveyorRig.cpp`: two-bone IK with literal 42/46 at lines 446–449, ankle end at sole +10 cm at line 439, SoftReach 83.5 at line 229); workers draw at `PresentationScale` 1.5 (`EchoesEntityView.cpp:1809`). Simulation collision envelope of a unit: `Rules.footprintHalfExtentRaw = kFixedScale / 8` (`EchoesContentSubsystem.cpp:363`) with `kFixedScale` = 100 cm = one simulation tile (`EchoesGlassScarCompiledMapPack.h:21`) → a 25 cm square; the visual overhangs it (§3, §8.3) |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Surveyor is the Compact's ordinary worker: it gathers Matter, builds every Compact
structure, operates Future Wells and repairs the network (SPEC-UNIT-001). The book calls it a walking
worker frame; its whole body language is service reach. It is present in every Compact base and in
M01's evacuation. Nothing on it may read as a weapon; the drill and welder are tools.

**DETAIL.** Large scale: a crouched biped about person height, wider than tall across the arms, boxy
ceramic torso over a charcoal frame, rear rack with three canisters hung behind the torso, small mast.
Medium: two tool arms with distinct ends (tapered drill right, two-finger gripper left), planted block
feet under vertical leg columns, hip/knee/elbow hubs, chest status band. Fine (close camera): fastener
bosses, shoulder repair patch (¶1606), canister windows, palm welder. Material logic: ceramic plates
bolted over a serviceable frame.

**ACTION.** Tracks per contract: idle, move, turn, stop, damage, death, cancel, restore, gather,
carry, deliver, build, repair_when_authorized (§5). Drill spins only while gathering (material
effect gated by state); build extends both arms to the footprint; repair shows the welder arm only;
delivery empties the cradle only on the authoritative transfer. Sound (Bible): ceramic-on-stone
footfalls, dry rotary drill, soft cargo latch — specified, not produced. Accessibility: role reads from
silhouette (tools, cradle), state from posture and luminance, not hue. Boundaries: no root motion, no
simulation authority in animation; facing sweep stays runtime-owned (SPEC-MOV-010).

**REVIEW.** Fields checked against the sources in §1 before geometry (2026-09-06). Open items in §8.

## 3. Scale basis

Authority: [concept-fidelity.md](concept-fidelity.md) (owner ruling 2026-09-06: the concept images define what
the asset looks like; gameplay rules bound the concept, never replace it). Every value below is measured on
the built rest stance (`build-manifest.json` → `concept_measurements`); each row names the band it sits in.

| Item | Value | Basis |
|---|---|---|
| Authored height H | 176 cm (mast cap) | concept-fidelity.md: H measured on `EBS-CON-MER-UNT-001` at the mast top; canon "about as tall as a person"; existing placeholder ~180 |
| Width across the arms | 214.5 cm at rest = 1.22 H, inside the concept band 1.15–1.30 H; the figure is wider than tall | concept-fidelity.md item 8; canon "half again as wide at the shoulder" |
| Torso | 66 wide × 60 deep × 52 high (0.38 / 0.34 / 0.30 H), pitched 10° forward, hips at the rear half; frame z 87.7–144.3, rear top edge at 145.6 | concept-fidelity.md item 3 |
| Legs | vertical columns: thigh 43.9 cm (0.25 H) leaning 5° back, shin 41.8 cm (0.24 H) 11° forward-down, ankle 13 cm; knee hub at mid leg height (z 54 between hip 96 and ankle 13); feet 80 cm apart = 0.455 H (band ~0.45 H) | concept-fidelity.md item 2 as corrected on the concept pixels (§8.3) |
| Arms | upper arm 52.3 cm (0.30 H), forearm 42.8 cm (0.24 H) at 50.5° below horizontal, drill tool 37 cm (0.21 H) | concept-fidelity.md item 1 (arms as long as the legs) |
| Tool tips at rest | drill tip 79 cm ahead of the hip line, 7.4 cm above the ground (bands 60–90 cm, 5–15 cm) | concept-fidelity.md item 1 |
| Cargo cradle | tray floor at z 124 (shoulder-hub height, 0.65 of the torso height above its bottom edge), hung behind the rear face (x −60…−32), outer rail flush with the right flank (y 36 vs flank 33); canisters Ø 17.6 × 28 (0.10 / 0.16 H), caps at 154 = 8.4 cm above the torso's rear top edge | concept-fidelity.md item 4 as clarified (§8.3) |
| Draw scale | ×1.5 at runtime (`PresentationScale` for workers): 264 cm tall, 321.8 cm across the arms | `EchoesEntityView.cpp:1809` gate-50 readability rule; not part of the asset |
| Footprint | `SPEC-UNIT-001` "Logistics Footprint 1" is the **population cost** (`SPEC-RES-001` Logistics = population throughput ceiling; `REL-ECO-011.AUTH` reserves it per queued unit; `units.json` `population_cost: 1` → `Rules.populationCost`). The **simulation collision footprint** is a 25 cm square (`kFixedScale / 8` half extent on a 100 cm tile). The **visual envelope** — 214.5 cm across the arms (321.8 cm at ×1.5), build pose x −57…+143.5 (215 cm ahead of the pivot at ×1.5) — overhangs it; recorded as a deviation in §8.3 | `Requirements.md:575,677`, `EchoesContentSubsystem.cpp:361-363`, `EchoesGlassScarCompiledMapPack.h:21` |
| Units / axes / pivot | cm; +X forward, +Y right (anatomical right), +Z up; root at ground-contact centre; **Nanite OFF** (`import_policy.nanite`; M1 Pro baseline) | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-mer-unt-001-concept-v3`)

Generator: [build_surveyor.py](build_surveyor.py) on the mesh kit and the skeletal kit
([../tools/ebs_skelkit.py](../tools/ebs_skelkit.py)). The rest stance is the concept's knuckle-walker: a
dense torso block on top, pitched 10° forward; both arms hang forward-down-outward from large shoulder-hub
discs on the torso flanks and reach the ground ahead of the feet; two thick plated legs stand as near-vertical
columns under the rear half (knee hub at mid height, block foot centred under the ankle); the canister rack
hangs on the rear face at shoulder height. Limb parts are authored in their local frames (pivot at the joint,
limb along +X) and placed by the rest deltas — the legs with a (pitch, yaw, roll) frame so their plated front
faces point forward on the splayed vertical columns (`limb_frame` / `rolled`); the leg parts are also exported
unrotated as `SM_EBS_MER_UNT_001_UpperLeg/LowerLeg/Foot` for the runtime's existing code-driven M01 rig
(constants conflict recorded in §8.5).

| Concept item (concept-fidelity.md) | Built |
|---|---|
| 1 knuckle-walker stance | torso centre at z 116, pitch −10°; shoulders at (14, ±46, 112); elbows (24, ±74, 69); wrists (46, ±90, 36); drill tip (65, +104, 7.4) |
| 2 legs as vertical columns | hips (−14, ±28, 96) → knees (−18, ±40, 54) → ankles (−10, ±40, 13): knee 4 cm behind the hip, ankle 8 cm ahead of the knee; knee disc Ø 16 (0.09 H); 35 cm foot centred on the ankle (15 cm raised toe plate ahead, 9 cm heel block behind) |
| 3 torso | 66×60×52 frame with pale ceramic front/top/flank/rear plates (rear plate now symmetric); ONE cyan lens Ø 12 on the upper-left third of the front face, status band below; repair patch with four bolts on the upper-right plate (¶1606) |
| 4 cargo cradle | open tray with rails hung behind the rear face at z 120–129, offset to the anatomical right (y −22…36); two vertical hanger bars up the rear face to hook bars over the rear top edge (z 146–150) and a strut closing the gap to the rear shell; three upright canisters Ø 17.6 × 28 (`body_canister_01..03`, separately hideable) with rear cyan window strips; caps at 154 |
| 5 optical mast | 22 cm post + 12×10×10 camera head with a front lens on the rear-LEFT top; cap at H = 176; the concept's holographic panel is simplified away (review decision) |
| 6 right arm | drill housing Ø 16 with a cyan strip, five-step tapered bit; `Harvest_Tether_Muzzle` at the tip |
| 7 left arm | knuckle block, two long fingers with tips, thumb nub, recessed cyan palm welder; forearm status strip |
| 12 bones | root, body, r/l_thigh, r/l_shin, r/l_foot, r/l_shoulder, r/l_forearm; identity rest orientation, hinge pitch about +Y at each joint |
| Sockets | `Harvest_Tether_Muzzle` (r_forearm, drill tip (65.0, 103.8, 7.4)), `Cargo_Drop_Anchor` (body, cradle centre (−46, 7, 138)), `Center_Hitbox_Socket` (body, torso centre (0, 0, 116)) |

Budgets: LOD0 1,932 ≤ 4,500; LOD1 1,116 ≤ 1,800 (room reserved for ART_ALPHA plating detail). Emissive share
of the surface (cyan pseudo slot as a proxy) 1.6% ≤ 5%. Nanite OFF. Materials: two export slots
(`MI_EBS_MER_UnitFrame`, `MI_EBS_MER_UnitCeramic`); the status band, canister windows, lens, tool strips, brass
trim and the team band are texture channels of the ceramic slot (review OBJs keep a third pseudo slot so renders
show the cyan). Rest stance, bone heads, concept measurements, the compatibility-part record and clip definitions
are in [build-manifest.json](build-manifest.json). Rest bounds: x −59.8…67.6, y −108.4…106.1, z 0…176.

concept-v3 (this revision, 2026-09-06 review of concept-v2 against the concept pixels): the cradle moved from
a stack on top of the torso (floor z 148, caps 31 cm above the torso top, bracket 21 cm outboard of the flank)
to a rack hung on the rear at shoulder height; the legs changed from a forward-knee Z-crouch (knee 32 cm ahead
of the hip) to vertical columns; the forearm steepened from 43° to 50.5° so the drill tip lands inside the
60–90 cm band (79 cm; was 92); hips/thighs widened so the feet stand 0.455 H apart (was 0.39 H); canisters
grew to Ø 0.10 H × 0.16 H (was 0.09 × 0.15); gather, deliver, death and the turn shuffle were re-keyed (§5).
LOD0 +24 / LOD1 +24 triangles (hangers, larger canister row).

## 5. Clips (keyframed, Unreal rotators; §4 rig)

Every torso lean is keyed on `body` with the thighs countering it, so the soles stay on z = 0 (the legs hang
from the body bone); crouches solve the body drop from the rest joints (`leg_drop`) so the level soles return
to the ground. A vertical leg column only lowers when both segments leave the vertical, so the concept-v3
crouches carry real knee bends (thigh forward, shin back). No clip keys `root` (restore writes an identity key on
the other eleven bones); no clip moves the frame along the ground. Measured values are from the posed stills
(`review/pose-manifest.json`) and the 20 ms ground sweep in the tests.

| Track | Duration | Intent (concept-v3) |
|---|---|---|
| idle | 2.0 s loop | 1° breathing lean on the hips, 2° shoulder rise; tools resting just above the ground ahead |
| move | 0.5 s loop | walk cycle authored for 360 cm/s (two 90 cm strides): 22° thigh swing, 34° shin lift, ≤2 cm bob; shoulders counter-swing ±8° with the forearms folded 6° so the hanging tools clear the ground (lowest vertex −0.4 cm); play rate follows authoritative velocity (REL-ART-009) |
| carry | 0.5 s loop | loaded walk: body pitched 4° forward, arms tucked up (shoulder 25°, forearm 35°; drill tip at 74.5 cm); cargo visibility follows the authoritative load; lowest vertex −0.9 cm (trailing toe) |
| turn | 0.5 s loop | stationary shuffle (alternate feet lift 2.4 cm with the knee forward: thigh 28°, shin −34°, foot 6° keeps the sole level; 3° body yaw) while the runtime sweeps heading; no root yaw |
| stop | 0.3 s | 3° forward settle and recover (Bible) |
| gather | 1.0 s loop | GATHERING panel: knee-bend crouch (thigh 48°, shin −96°, drop 18.5 cm, top at 157.7 cm) with the body pitched 14° forward (24° total); right shoulder (71.5°, −5°), forearm −70°: elbow high at z 72, drill bit 60° into the ground 84 cm ahead of the toe (tip (91, 96, 1.4–2.6) with a ~1 cm pump); gripper down beside the frame (tips 7.7 cm up); drill spin is a material effect gated by this state |
| deliver | 0.8 s | DELIVERY panel: low squat (thigh 60°, shin −120°, drop 31 cm, top at 145.2 cm; body −2° for −12° total) with the upper arms level forward and both tools folded 60° down (shoulders (78°, ±10°), forearms −90°): drill tip (72.5, 115, 9.9) 66 cm ahead of the toe, gripper tips 17 cm up; keys every 50 ms in over 0.3 s and out over 0.3 s; the cradle's canisters are runtime-hidden on the authorized transfer (no arm reach required) |
| build | 1.0 s loop | 6° forward lean, both arms extended forward (shoulder 60°, yaw ∓20°): drill tip 143 cm ahead at 101 cm high; welder effect on the left palm |
| repair_when_authorized | 1.0 s loop | welder arm only (left shoulder 55°/25° yaw, forearm 12°); drill at rest (tip 3.4 cm up under the 3° lean) |
| damage | 0.3 s | torso jolts 8° back with a 2° roll, arms lift 12°; no displacement; lowest vertex −0.97 cm |
| death | 1.0 s | engineered collapse: knees fold (60°/−130°, drop 37 cm) while the torso pitches 45° forward onto the long arms (shoulders (115°, ∓20°), forearms −40°): elbows at z 38 under the torso front, torso front edge propped 27 cm up, drill tip on the ground 135 cm ahead (z 1.3); keys every 100 ms; top of the held pose 127.7 cm (0.73 H — the rear mast rises as the frame pitches) |
| cancel | 0.3 s | tools return to rest from the work lean |
| restore | single frame | rest pose for reconstruction from saved state; never replays a one-shot |

`build_surveyor.py --skinned` writes the skinned GLBs with these clips (`SK_EBS_MER_UNT_001_LOD0/1.glb`,
one skin, 12 joints, 13 animations, sockets as child nodes of their joints) on the verified skeletal
kit encodings (probe evidence under `EBS-MER-UNT-001/import/skeletal-probe/`). [pose_review.py](pose_review.py)
samples the clips at review fractions (linear interpolation, as the glTF samplers do) and bakes posed
review OBJs plus their render scenes (posed sockets carry the composed position and rotator); the deliver
sample is also baked with the cargo hidden (`pose_deliver_050_unloaded.obj`).

## 6. Review evidence (concept-v3 blockout stage)

Evidence root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-001/`
(`receipt.json` inventories files with SHA-256; `review/` rest-stance and posed OBJs; `scenes/`; `renders/`).

Renders (all 22 scenes regenerated for concept-v3): orthographic front/right/rear/left/top at authored scale
with a 180 cm reference figure (`renders/rest/`); tactical views at the game's orthographic framing with the
unit at ×1.5 (`renders/rest_tactical/`); LOD1 front/right; a context scene with five Surveyors beside the Power
Link (`../EBS-MER-BLD-002/review/SM_EBS_MER_BLD_002_assembly_connected_LOD0.obj`, sha256 recorded in the
receipt notes); posed stills (`renders/pose_<clip>_<fraction>/`: right, front and near tactical views for twelve
clip samples plus the unloaded deliver still, and ×1.5 gameplay-framing views for gather, build, deliver and death).

Concept comparison sheets (`renders/concept-compare/`, concept crop or reference panel in the first cell;
exact commands in [make_sheets.py](make_sheets.py) and §7):

| Sheet | Cells |
|---|---|
| `concept_vs_rest.png` | concept `EBS-CON-MER-UNT-001` crop · rest front · rest right · rest tactical_near |
| `concept_vs_tactical.png` | concept crop · tactical_gameplay · tactical_default · tactical_front_quarter (×1.5) |
| `reference_rear_vs_rear_top.png` | reference REAR VIEW panel · rest rear · rest top · rest left |
| `reference_gathering_vs_gather.png` | reference GATHERING panel · gather right · gather tactical_near · gather tactical_gameplay (×1.5) |
| `reference_delivery_vs_deliver.png` | reference DELIVERY panel · deliver right (loaded) · deliver right (cargo hidden) · deliver tactical_near (cargo hidden) |
| `reference_delivery_vs_deliver_rear.png` | reference DELIVERY panel · deliver front (cargo hidden) · rest rear · deliver tactical_near (cargo hidden, ×1.5) |
| `pose_sheet_right.png`, `pose_sheet_tactical_near.png` | twelve clip samples (move ×2, carry, turn / stop, gather, build, repair / deliver, damage, death, cancel) |
| `lod0_vs_lod1.png`, `context_and_death.png` | LOD1 beside LOD0 front/right; context gameplay/near, death and build at the gameplay framing |

Fidelity checks (concept-fidelity.md), decided on the sheets above:

| Check | Result | Pixel evidence |
|---|---|---|
| Front view: torso on top, arms hanging forward-down to the ground, legs at the rear, wider than tall | MET | `concept_vs_rest.png` cell 2 / `rest/front.png`: torso block above the hips, both arms hang down-outward with the drill tip and gripper at ground level (tips 7–14 cm up), two vertical leg columns under the rear half with the knee hubs at mid height, canister caps just visible above the torso top; span 214.5 cm vs 176 cm tall (1.22 H, band 1.15–1.30 H) |
| Right view: knuckle-walker profile (torso pitched, arm reach ahead of the knees, legs as columns) | MET | `concept_vs_rest.png` cell 3 / `rest/right.png`: torso pitched 10° nose-down; drill tip at x 65 = 83 cm ahead of the knee (x −18); thigh and shin stand as one near-vertical column from the hip (x −14) to the ankle (x −10) with the foot centred under it; the rack hangs behind the torso at shoulder height with the caps just above the torso top |
| Three upright canisters visible from the rear and top; mast on the opposite side | MET | `reference_rear_vs_rear_top.png`: rear view shows three cylinders with cyan windows in a tray at mid-torso height flush with the right flank and the mast head on the rear-left above them; top view shows the three circles at the rear behind the torso and the mast square at the rear-left; left view shows the tray behind the rear face, not on top |
| Eye and status band on the front face; drill right, gripper left (anatomical) | MET (lens side is a recorded deviation from the derived front panel, §8.3) | `rest/front.png`: single lens on the upper-left third of the front face (anatomical left = −Y, the viewer's right), status band across the chest below it, repair patch upper-right; drill on +Y (`Harvest_Tether_Muzzle` y = +103.8), two-finger gripper on −Y |
| Gather pose matches the GATHERING panel; deliver crouch matches DELIVERY | MET (arm angles bounded by the concept's arm lengths, §8.3) | `reference_gathering_vs_gather.png`: frame crouched and lowered (top 157.7 cm), elbow high above the drill housing, bit driven 60° into the ground 84 cm ahead of the toe; `reference_delivery_vs_deliver.png`: low squat (top 145.2 cm) with the upper arms level and both tools folded 60° down, tips 10–17 cm off the ground, and, in the cargo-hidden still, the empty open rack of the panel; the two poses read differently at the near tactical framing (`pose_sheet_tactical_near.png` row 2 col 2 vs row 3 col 1); the panel's drop tray is the drop point, not part of this asset |

Checks: 19 structural tests in [test_surveyor_build.py](test_surveyor_build.py) (inventory, 12-bone hierarchy
and sides, every polygon bound, socket names/bones, grounded planted stance with the concept span band
1.15–1.30 H and wider-than-tall, drill-tip band 5–15 cm / 60–90 cm, feet 0.40–0.50 H, cradle/mast/lens side
strings, legs as vertical columns with forward-facing plates and ankle-centred feet, cradle hung on the rear
at shoulder height with the caps 3–12 cm above the torso top and the tray flush with the flank, compatibility
parts against the runtime 42/46/+10 constants with the `requires_runtime_change` flag, budgets, two export
slots, clip inventory with no root track in any clip and an identity restore key, determinism, no vertex below
−1 cm in any clip sampled at 20 ms, soles within ±1.5 cm in every hold pose, gather drilling into the ground
ahead, deliver squat with tools folded down, collapse propped on the elbows with the drill on the ground), all
passing; `--check` reports no drift; author's inspection of every sheet.

Headless skeletal import (`import/`, UE 5.8.2, `-nullrhi`, sandbox project, Interchange with the generic
assets pipeline override) was run on revision `ebs-mer-unt-001-blockout-v2`: `SK_EBS_MER_UNT_001` and the
LOD1 source import as skeletal meshes with 12 bones in the manifest order and parents, reference pose
translations equal to the authored bone heads, the three sockets on their bones, two material slots by
name, and 13 AnimSequences per mesh whose evaluated end poses match the clip keys (`import/skeletal-report.json`).
It has NOT been re-run on concept-v3 (same bone names, parents, sockets and clip inventory; different bone
heads and keys; the restore clip no longer carries a root channel): the import evidence is stale for this
revision and is listed in §8 as open.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-UNT-001"
R="<evidence root>"   # /Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z
python3 build_surveyor.py --evidence-dir "$R/EBS-MER-UNT-001" --skinned
python3 build_surveyor.py --evidence-dir "$R/EBS-MER-UNT-001" --check
python3 test_surveyor_build.py
python3 pose_review.py --evidence-dir "$R/EBS-MER-UNT-001"
for s in "$R/EBS-MER-UNT-001/scenes/"*.json; do python3 ../tools/ebs_render.py --scene "$s" --out "$R/EBS-MER-UNT-001/renders/$(basename "$s" .json)"; done
python3 make_sheets.py --evidence-root "$R"   # runs the ten ebs_sheet.py commands below, in this order
cp "$R/EBS-MER-UNT-001/renders/concept-compare/pose_sheet_right.png" "$R/EBS-MER-UNT-001/renders/pose_sheet_right.png"
cp "$R/EBS-MER-UNT-001/renders/concept-compare/pose_sheet_tactical_near.png" "$R/EBS-MER-UNT-001/renders/pose_sheet_tactical_near.png"
python3 ../tools/make_evidence_receipt.py --evidence-dir "$R/EBS-MER-UNT-001" --package EBS-PKG-MC-SURVEYOR --manifest build-manifest.json --stage "BLOCKOUT (concept-v3)" --command ... --note ...   # the exact --command/--note strings are in receipt.json
EBS_IMPORT_JOB="$R/EBS-MER-UNT-001/import/skeletal-job.json" "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$R/IsolatedPreview/EBSPreview/EBSPreview.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None -ExecutePythonScript="<worktree>/ArtSource/tools/ue_import_inspect_skeletal.py"   # blockout-v2 run; re-run pending for concept-v3
```

Sheet commands (`P="$R/EBS-MER-UNT-001/renders"`, `C="$R/concept-crops"`, `O="$P/concept-compare"`; tile order as written):

```sh
python3 ../tools/ebs_sheet.py --out "$O/concept_vs_rest.png" --cols 2 --cell 800 "$C/surveyor_concept_2x.png" "$P/rest/front.png" "$P/rest/right.png" "$P/rest_tactical/tactical_near.png"
python3 ../tools/ebs_sheet.py --out "$O/concept_vs_tactical.png" --cols 2 --cell 800 "$C/surveyor_concept_2x.png" "$P/rest_tactical/tactical_gameplay.png" "$P/rest_tactical/tactical_default.png" "$P/rest_tactical/tactical_front_quarter.png"
python3 ../tools/ebs_sheet.py --out "$O/reference_rear_vs_rear_top.png" --cols 2 --cell 800 --crop 0.60,0.02,1.0,0.31 "$C/surveyor-reference.png" "$P/rest/rear.png" "$P/rest/top.png" "$P/rest/left.png"
python3 ../tools/ebs_sheet.py --out "$O/reference_gathering_vs_gather.png" --cols 2 --cell 800 --crop 0.60,0.31,1.0,0.63 "$C/surveyor-reference.png" "$P/pose_gather_050/right.png" "$P/pose_gather_050/tactical_near.png" "$P/pose_gather_050_tactical/tactical_gameplay.png"
python3 ../tools/ebs_sheet.py --out "$O/reference_delivery_vs_deliver.png" --cols 2 --cell 800 --crop 0.60,0.63,1.0,0.95 "$C/surveyor-reference.png" "$P/pose_deliver_050/right.png" "$P/pose_deliver_050_unloaded/right.png" "$P/pose_deliver_050_unloaded/tactical_near.png"
python3 ../tools/ebs_sheet.py --out "$O/reference_delivery_vs_deliver_rear.png" --cols 2 --cell 800 --crop 0.60,0.63,1.0,0.95 "$C/surveyor-reference.png" "$P/pose_deliver_050_unloaded/front.png" "$P/rest/rear.png" "$P/pose_deliver_050_unloaded_tactical/tactical_near.png"
python3 ../tools/ebs_sheet.py --out "$O/pose_sheet_right.png" --cols 4 --cell 480 "$P/pose_move_025/right.png" "$P/pose_move_050/right.png" "$P/pose_carry_025/right.png" "$P/pose_turn_025/right.png" "$P/pose_stop_000/right.png" "$P/pose_gather_050/right.png" "$P/pose_build_050/right.png" "$P/pose_repair_when_authorized_050/right.png" "$P/pose_deliver_050/right.png" "$P/pose_damage_033/right.png" "$P/pose_death_100/right.png" "$P/pose_cancel_000/right.png"
python3 ../tools/ebs_sheet.py --out "$O/pose_sheet_tactical_near.png" --cols 4 --cell 480 <the same twelve samples, tactical_near.png>
python3 ../tools/ebs_sheet.py --out "$O/lod0_vs_lod1.png" --cols 2 --cell 640 "$P/rest/front.png" "$P/lod1/front.png" "$P/rest/right.png" "$P/lod1/right.png"
python3 ../tools/ebs_sheet.py --out "$O/context_and_death.png" --cols 2 --cell 800 "$P/context/tactical_gameplay.png" "$P/context/tactical_near.png" "$P/pose_death_100_tactical/tactical_gameplay.png" "$P/pose_build_050_tactical/tactical_gameplay.png"
```

## 8. Decisions, deviations and open items

1. Twelve bones spend the budget on planted feet and elbows; the mast and cradle are rigid with the
   body and the drill spin is a material effect (REL-FAC-025 12-bone rule). The card's "rotates mechanical
   treads" is superseded by the prepared MERIDIAN-ANATOMY amendment (legs).
2. Concept rework history (owner ruling "look at the concepts", 2026-09-06): the upright blockout-v2 stance
   (hips under the torso, arms 124 cm across) was replaced by the concept's knuckle-walker (concept-v1: torso on
   top pitched 10°, hips at the rear, arms as long as the legs reaching the ground ahead); concept-v2 widened
   the arm span into the measured 1.15–1.30 H band; concept-v3 (this revision) re-read the pixels for the
   cradle placement, the leg silhouette, the drill-tip band, the foot stance and the three working poses.
   Two lines of [concept-fidelity.md](concept-fidelity.md) were corrected in the same pass and are marked
   in the file: item 2 (legs: vertical columns, thigh/shin ≈ 0.25/0.24 H, knee under/behind the hip, ankle
   under/ahead of the knee — the earlier "thigh forward-down, knees ahead of the hips" described a Z-crouch the
   pixels do not show), item 4 (rack hung on the rear at shoulder height, not stacked on top), and the bounding
   rule "footprint 1 tile" (Logistics Footprint 1 is the population cost, see §3). Upright variants remain one
   constant set away.
3. Deviations from the concept or the derived reference, each with the cause cited:
   - The tall holographic mast panel is not modelled (review decision on `EBS-CON-MER-UNT-001`: "simplify the
     decorative mast panel"; readability at the ×1.5 tactical framing). A short post and camera head stand in.
   - Brass edge trim, plate seams, bolts beyond the repair patch and the cyan strips are texture channels, not
     geometry (two-slot material rule and the LOD0 ≤ 4,500 budget; textures are an open item).
   - The derived reference's REAR VIEW mirrors the cradle and mast sides relative to the concept; the concept
     (KEEP, design identity) wins: cradle rear-right (+Y), mast rear-left (−Y).
   - **Lens side.** The derived front panel (`surveyor-reference.png` crop 0.14,0.12,0.50,0.50) places the lens on
     the viewer's left third, i.e. the anatomical right. The KEEP concept (`surveyor_concept_2x.png` crop
     0.45,0.26,0.72,0.52) is read the other way: the figure faces the viewer's right-front with its drill (right)
     arm nearest the camera, so the lit block on the right of the image is the front face and its far end — where
     the lens sits — is the anatomical left. The model follows the KEEP concept (lens at y −20…−9, anatomical
     left, −Y). Recorded as a deviation from the derived panel; if the owner reads the concept the other way the
     lens and repair patch mirror by one sign in `build_torso_block` (no other geometry moves).
   - **Gather and deliver arm angles.** The GATHERING panel drives the upper arm ~40° down and the bit ~67° down;
     the DELIVERY panel angles the upper arms ~30° down with the tools 45–70° down. With the concept's arm lengths
     (upper 0.30 H + forearm 0.24 H + tool 0.21 H, concept-fidelity.md item 1) and the frame lowered 18.5 cm, an
     upper arm at 40° puts the bit 10 cm under the ground, so the built gather keeps the bit at 60° with the tip
     on the ground and lets the upper arm ride at ~16° below horizontal with the elbow high above the housing;
     deliver holds the upper arms level so the 60°-down tools clear the ground by 10–17 cm. The readable
     content of both panels (bit into the ground ahead of the toe; squat with tools folded down) is met.
   - **Death hold height.** The torso is propped on the elbows (front edge 27 cm up, elbows at 38 cm) per the
     rig target "the long arms carry it"; the rear-mounted mast then rises to 127.7 cm (0.73 H), so the death
     test's ceiling moved from 0.65 H to 0.75 H (the torso itself lies at 27–100 cm).
   - **Visual envelope vs simulation footprint.** The simulation collision footprint of a unit is a 25 cm square
     (`kFixedScale / 8` half extent on a 100 cm tile; §1, §3) and the visual placeholder already overhung it
     (~100 cm across). The concept width wins (owner ruling: gameplay rules bound the concept, never replace it;
     the collision box is a pathing/hit quantity, not a visual bound): rest 214.5 cm across the arms (321.8 cm
     at ×1.5), build pose 215 cm ahead of the pivot at ×1.5. The integration task's `readability_test` must cover
     the cases this creates: adjacent workers overlapping arms at the gameplay framing (the context scene places
     five at ~260 cm spacing and their arms cross), selection/hover hit-testing on the overhang, and the
     gather/build reach entering a neighbour's footprint. Recorded as a deviation, not fabricated as an approved
     dimension (`ClaudeProductionHandoff.md:75`).
   - **Leg segment lengths.** Thigh 0.25 H / shin 0.24 H (concept pixels) rather than the earlier target's
     0.30 / 0.28 H: with the hip at z 96 and the ankle at 13 (unchanged) a vertical column is 83 cm long by
     construction; the earlier lengths were measured along the imagined Z-crouch.
   - The DELIVERY panel's ground tray is the drop point, a separate asset; the cradle empties by runtime
     visibility on the authorized transfer (GAP-01), never by animation, so the deliver clip is a squat only.
   - The concept's plated, chunky limbs are blocked as strut + plate boxes; plating depth is ART_ALPHA work
     under the reserved budget (LOD0 1,932 of 4,500).
4. Clip pass history: v1 clips broke the ground plane; v2 keyed leans with countering thighs and delivered
   as a knee dip; concept-v1/v2 re-tuned every clip to the knuckle-walker; concept-v3: crouches re-solved for
   the vertical legs (a column only lowers with a real knee bend — gather 48/−96, deliver 60/−120, death
   60/−130), gather re-keyed to drill into the ground ahead, deliver to fold both tools down, death to prop the
   torso on the elbows, the turn shuffle to lift the sole (2.4 cm), and deliver/death keyed every 50/100 ms so
   the interpolated knee bends keep the soles inside the ground rule. Tolerance: lowest vertex per clip at
   20 ms — move −0.4, carry −0.9, turn −0.4, deliver −0.3, damage −0.97, death −0.9 cm; all others 0 — inside
   the −1 cm rule.
5. Compatibility parts are exported for the existing code-driven rig; which route the integration task
   adopts (skeletal mesh + clips or static parts + code) is its decision (amendment: "preserve existing
   event and socket compatibility until adapter migration is tested"). **Constants conflict, recorded:** the
   parts are authored at the concept lengths (thigh 43.9, shin 41.8, ankle above the sole 13 cm; `build-manifest.json`
   → `compatibility_parts`) while `UpdateM01SurveyorRig` solves the knee with literal 42/46 and puts the ankle at
   sole +10 (`EchoesM01SurveyorRig.cpp:439,446-449`): under that IK the thigh part overshoots by 1.9 cm, the shin
   falls 4.2 cm short and the sole sits 3 cm off. The static-part route therefore requires those constants to
   change (or a second export at the runtime lengths); the manifest flags `requires_runtime_change: true` and
   `test_compatibility_parts_against_runtime_constants` keeps the flag honest.
6. Socket rotations are checked on the imported asset (skeletal report), not on the stills.
7. The context scene imports the Power Link review OBJ from the neighbouring package
   (`../../EBS-MER-BLD-002/review/SM_EBS_MER_BLD_002_assembly_connected_LOD0.obj`), which this receipt cannot
   inventory; its sha256 at render time is written into the receipt notes so a later re-render can be traced.
   The four context stills and `context_and_death.png` were re-rendered against the current neighbour OBJ.
8. Open: re-run the headless skeletal import on concept-v3 (the `import/` evidence is from blockout-v2);
   in-engine playback capture; emissive-share measurement on the textured asset (the 1.6% figure is a
   geometry proxy); textures (2048² per card); gate reviews; owner acceptance. Known minor items: the carry and
   damage clips bottom out at −0.9/−0.97 cm (inside the rule, no margin); the deliver squat splays the arms to
   y ±119 cm (wider than rest) because the shoulders yaw 10° outward to keep the folded tools clear of the knees.
