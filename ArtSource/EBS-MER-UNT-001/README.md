---
title: EBS-MER-UNT-001 Surveyor — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
package: EBS-PKG-MC-SURVEYOR
production_asset_id: EBS-MER-UNT-001
production_maturity: BLOCKOUT
revision: ebs-mer-unt-001-blockout-v2
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
| Gameplay record | `SPEC-UNIT-001`: 360 cm/s, logistics footprint 1, work rate 10, cargo 10 Matter, unarmed; Network Repair channel within 200 cm |
| Requirement card | `REL-FAC-025.MC.SURVEYOR.ASSET`: LOD0 ≤4,500 / LOD1 ≤1,800 tris, 2048² stack, team colour by mask, emissive ≤5%, 12-bone kinematic rig, sockets `Harvest_Tether_Muzzle` / `Cargo_Drop_Anchor` / `Center_Hitbox_Socket`; `SPEC-ART-002` worker animation set; `SPEC-MOV-010` presentation facing (720°/s sweep, runtime-owned); `REL-ART-009` code-driven kinematic motion synchronized to authoritative velocity |
| Existing envelope | runtime placeholder `SM_Meridian_Surveyor` (~180 cm tall, ~100 cm across the arms) and the M01 articulated parts (`meridian_surveyor_body`, `m01_surveyor_upper/lower/foot`: thigh 42 cm, shin 46 cm, foot 32 cm, pivots at the origin along local +X) driven by `AEchoesEntityView::UpdateM01SurveyorRig`; workers draw at `PresentationScale` 1.5 |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Surveyor is the Compact's ordinary worker: it gathers Matter, builds every Compact
structure, operates Future Wells and repairs the network (SPEC-UNIT-001). The book calls it a walking
worker frame; its whole body language is service reach. It is present in every Compact base and in
M01's evacuation. Nothing on it may read as a weapon; the drill and welder are tools.

**DETAIL.** Large scale: a crouched biped about person height, wider than tall across the arms, boxy
ceramic torso over a charcoal frame, rear cradle with three canisters, small mast. Medium: two tool
arms with distinct ends (tapered drill right, two-finger gripper left), planted block feet, hip/knee/
elbow hubs, chest status band. Fine (close camera): fastener bosses, shoulder repair patch (¶1606),
canister windows, palm welder. Material logic: ceramic plates bolted over a serviceable frame.

**ACTION.** Tracks per contract: idle, move, turn, stop, damage, death, cancel, restore, gather,
carry, deliver, build, repair_when_authorized (§5). Drill spins only while gathering (material
effect gated by state); build extends both arms to the footprint; repair shows the welder arm only;
delivery empties the cradle only on the authoritative transfer. Sound (Bible): ceramic-on-stone
footfalls, dry rotary drill, soft cargo latch — specified, not produced. Accessibility: role reads from
silhouette (tools, cradle), state from posture and luminance, not hue. Boundaries: no root motion, no
simulation authority in animation; facing sweep stays runtime-owned (SPEC-MOV-010).

**REVIEW.** Fields checked against the sources in §1 before geometry (2026-09-06). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Authored height | 176 cm (mast top) — PROVISIONAL | canon "about as tall as a person"; existing placeholder ~180 |
| Width across arms | ≈124 cm; torso 50 cm | canon "half again as wide at the shoulder" read against the existing envelope |
| Draw scale | ×1.5 at runtime (`PresentationScale` for workers) | `EchoesEntityView.cpp` gate-50 readability rule; not part of the asset |
| Footprint | logistics footprint 1 (one 200 cm tile) | `SPEC-UNIT-001` |
| Units / axes / pivot | cm; +X forward, +Y right (anatomical right), +Z up; root at ground-contact centre | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-mer-unt-001-blockout-v2`)

Generator: [build_surveyor.py](build_surveyor.py) on the mesh kit and the skeletal kit
([../tools/ebs_skelkit.py](../tools/ebs_skelkit.py)). Limb parts are authored in their local frames
(pivot at the joint, limb along +X) and placed into the crouched rest stance; the same parts are
exported unrotated as `SM_EBS_MER_UNT_001_UpperLeg/LowerLeg/Foot` for the runtime's existing
code-driven M01 rig, so both the skeletal route and the current adapter can consume the design.

| Contract component | Built |
|---|---|
| 2 legs, planted feet | thigh/shin/foot parts per side; soles on z = 0 in rest |
| 2 tool arms | right forearm carries the tapered drill and `Harvest_Tether_Muzzle`; left carries the two-finger gripper with a recessed palm welder |
| 3 rear canisters | one transverse cradle behind the torso, canisters 01–03 with cyan-white windows |
| 1 optical mast | rear-left of the torso top, sensor head with lens |
| 12 bones | root, body, r/l_thigh, r/l_shin, r/l_foot, r/l_shoulder, r/l_forearm; identity rest orientation, hinge pitch about +Y at each joint |
| Sockets | `Harvest_Tether_Muzzle` (r_forearm, drill tip), `Cargo_Drop_Anchor` (body, cradle centre), `Center_Hitbox_Socket` (body, torso centre) |

Budgets: LOD0 1,096 ≤ 4,500; LOD1 604 ≤ 1,800 (room reserved for ART_ALPHA detail). Materials: two
export slots (`MI_EBS_MER_UnitFrame`, `MI_EBS_MER_UnitCeramic`); the status band, canister windows,
lens, drill/welder status and the team band are texture channels of the ceramic slot (review OBJs
keep a third pseudo slot so renders show the cyan). Rest stance, bone heads and clip definitions are
in [build-manifest.json](build-manifest.json).

## 5. Clips (keyframed, Unreal rotators; §4 rig)

Every torso lean is keyed on `body` with the thighs countering it, so the soles stay on z = 0
(the legs hang from the body bone). No clip keys `root`; no clip moves the frame along the ground.

| Track | Duration | Intent (v2) |
|---|---|---|
| idle | 2.0 s loop | 1° breathing lean and a 3° shoulder rise, feet planted |
| move | 0.5 s loop | walk cycle authored for 360 cm/s (two 90 cm strides), ≤2 cm bob; play rate follows authoritative velocity (REL-ART-009) |
| carry | 0.5 s loop | loaded walk: torso leaned 4° forward, arms tucked up (upper arm +10°, forearm +20°); cargo visibility follows the authoritative load |
| turn | 0.5 s loop | stationary shuffle (alternate feet lift ≤4 cm, soles level) while the runtime sweeps heading; no root yaw |
| stop | 0.3 s | 3° forward settle and recover (Bible) |
| gather | 1.0 s loop | 8° forward lean, drill tip 80–88 cm ahead of the origin at 4–8 cm above ground; spin is a material effect gated by this state |
| deliver | 0.8 s | knees dip 12 cm and the torso tilts 8° back to present the rear cradle (the arms cannot reach it); cargo empties only on the authorized transfer |
| build | 1.0 s loop | 6° forward lean, both arms extended forward ~128 cm at ~1 m height; welder effect on the left palm |
| repair_when_authorized | 1.0 s loop | welder arm only (left, 34°/16°); drill at rest |
| damage | 0.3 s | torso jolts 8° back with a 2° roll, arms lift 12°; no displacement |
| death | 1.0 s | engineered collapse: knees fold (55°/−110°), body drops 54 cm and settles 35° back onto its cradle, tools splay with the drill tip on the ground ahead; top of the held pose at 104 cm |
| cancel | 0.3 s | tools return to rest from the work lean |
| restore | single frame | rest pose for reconstruction from saved state; never replays a one-shot |

`build_surveyor.py --skinned` writes the skinned GLBs with these clips (`SK_EBS_MER_UNT_001_LOD0/1.glb`,
one skin, 12 joints, 13 animations, sockets as child nodes of their joints) on the verified skeletal
kit encodings (probe evidence under `EBS-MER-UNT-001/import/skeletal-probe/`). [pose_review.py](pose_review.py)
samples the clips at review fractions (linear interpolation, as the glTF samplers do) and bakes posed
review OBJs plus their render scenes (posed sockets carry the composed position and rotator).

## 6. Review evidence (blockout stage)

Evidence root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-001/`
(`receipt.json` inventories files with SHA-256; `review/` rest-stance OBJs; `scenes/`; `renders/`).
Renders: orthographic front/right/rear/left/top at authored scale with a 180 cm reference figure;
tactical views at the game's orthographic framing with the unit at ×1.5; LOD1 sheets; a context scene
with five Surveyors beside the Power Link; posed stills (`renders/pose_<clip>_<fraction>/`: right, front
and near tactical views for twelve clip samples, plus ×1.5 gameplay-framing views for gather, build,
deliver and death). Checks: 14 structural tests in [test_surveyor_build.py](test_surveyor_build.py)
(inventory, 12-bone hierarchy and sides, every polygon bound, socket names/bones, grounded planted
stance, limb directions, budgets, two export slots, clip inventory and ordering, determinism, no vertex
below −1 cm in any clip sampled at 20 ms, soles within ±1.5 cm in every hold pose, collapse height and
drill placement), all passing; author's visual inspection of the renders.

Headless skeletal import (`import/`, UE 5.8.2, `-nullrhi`, sandbox project, Interchange with the
generic assets pipeline override): `SK_EBS_MER_UNT_001` and the LOD1 source import as skeletal meshes
with 12 bones in the manifest order and parents, reference pose translations equal to the authored bone
heads (identity rest rotations), sockets `Harvest_Tether_Muzzle` (r_forearm), `Cargo_Drop_Anchor` and
`Center_Hitbox_Socket` (body) at their authored local offsets, two material slots by name, LOD0 2,064
vertices, and 13 AnimSequences per mesh whose evaluated end poses match the clip keys (for example death:
body 35°/10° roll at −54 cm... see `import/skeletal-report.json`). Not yet: in-engine playback capture,
textures, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-UNT-001"
python3 build_surveyor.py --evidence-dir "<evidence root>/EBS-MER-UNT-001" --skinned
python3 build_surveyor.py --evidence-dir "<evidence root>/EBS-MER-UNT-001" --check
python3 test_surveyor_build.py
python3 pose_review.py --evidence-dir "<evidence root>/EBS-MER-UNT-001"
python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-MER-UNT-001/scenes/rest.json" --out "<evidence root>/EBS-MER-UNT-001/renders/rest"
for s in "<evidence root>/EBS-MER-UNT-001/scenes/pose_"*.json; do python3 ../tools/ebs_render.py --scene "$s" --out "<evidence root>/EBS-MER-UNT-001/renders/$(basename "$s" .json)"; done
EBS_IMPORT_JOB="<evidence root>/EBS-MER-UNT-001/import/skeletal-job.json" "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "<evidence root>/IsolatedPreview/EBSPreview/EBSPreview.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None -ExecutePythonScript="<worktree>/ArtSource/tools/ue_import_inspect_skeletal.py"
```

## 8. Decisions and open items

1. Twelve bones spend the budget on planted feet and elbows; the mast and cradle are rigid with the
   body and the drill spin is a material effect. The card's "rotates mechanical treads" is superseded by
   the prepared MERIDIAN-ANATOMY amendment (legs).
2. The crouched rest stance (thigh 40° forward, knee ahead of the hip, foot under it) follows the
   candidate and the existing paired-leg placeholder; upright variants are one constant away.
3. Compatibility parts are exported for the existing code-driven rig; which route the integration task
   adopts (skeletal mesh + clips or static parts + code) is its decision (amendment: "preserve existing
   event and socket compatibility until adapter migration is tested").
4. v2 clip pass (2026-09-06): the v1 clips pitched the whole leg chain with the torso and let the feet
   and tools break the ground plane (death drill tip 42 cm under); v2 keys every lean with countering
   thighs, drives the deliver track as a knee dip that presents the cradle (the arms are too short to reach
   it, measured: best gripper reach 50 cm short), and lands the collapse as a backward settle onto the
   cradle with the drill on the ground ahead. A forward kneel was tried and rejected: no arm setting kept
   the tools above ground with the body that low.
5. Socket rotations are checked on the imported asset (skeletal report), not on the stills.
6. Open: in-engine playback capture, emissive-share measurement (≤5%), textures (2048² per card), gate
   reviews.
