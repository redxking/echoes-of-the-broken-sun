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

## 8. Decisions and open items

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
   | 14-bone kinematic rig | **conflict**: 22 bones, 16 of them limb bones alone. OWNER-QUESTION A |
   | 3 vertex ID channels for molt phases | **not implemented** |
   | 512² translucent core blend for molts | pending at the texture stage |

> **OWNER-QUESTION A — the card's 14-bone rig against a four-legged animal.** `REL-ART-005.KA.RIFTSTALKER`
> specifies a 14-bone kinematic layout. The selected concept is a quadruped with three-segment limbs,
> which costs 16 bones in the limbs alone before a body, prow or caster mount. The card's figure is
> consistent with the biped my provisional card first described and inconsistent with the concept the
> owner's standing ruling makes authoritative. Confirm the quadruped and amend the card's bone count,
> or direct a 14-bone rig — which means dropping a limb segment or the caster's independent aim.
