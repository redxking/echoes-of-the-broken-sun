---
title: EBS-KHA-UNT-003 Cairnback — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-CAIRNBACK
production_asset_id: EBS-KHA-UNT-003
production_maturity: BLOCKOUT
revision: ebs-kha-unt-003-concept-v1
canon_status: CANDIDATE (built to the cairnback candidate against canon SPEC-UNIT-007); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-UNT-003 Cairnback — production source

Single authoritative source record for the Kharuun assault screen. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
[`REL-FAC-025.KA.CAIRNBACK.ASSET`](../kharuun-asset-cards.md); maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Silhouette first, with the separation measured

The owner directed on 2026-09-07 that this build establish a contrasting silhouette against the
Riftstalker **before** detailing, and that the two stay apart in monochrome at tactical distance. The
shell profile was defined first and everything else fitted under it.

| Contrast measure | Riftstalker | Cairnback |
|---|---|---|
| leg share of standing height | 0.69 | **0.24** |
| body width ÷ body length | 0.26 | **0.74** |
| head | a prow ahead of the front feet | tucked behind the shell's front edge |
| gait | diagonal pairs, 410 cm/s | lateral sequence, 270 cm/s |

Both ratios are asserted by test. The monochrome separation is **measured**, not asserted, by
[../tools/ebs_silhouette.py](../tools/ebs_silhouette.py), which reduces two renders from the same camera
to occupancy masks, scales each to its own bounding box and compares shape:

| Monochrome pass | Intersection over union | Largest band difference | Fill difference |
|---|---|---|---|
| side | 0.469 | 0.198 | 0.380 |
| tactical | 0.540 | 0.292 | 0.217 |

A low intersection means two shapes occupying different areas once scaled alike. Reports are in
`renders/concept-compare/silhouette-side.json` and `silhouette-tactical.json`, with the paired images
beside them. **This is a measurement, not a verdict**: it says the shapes differ, and a human still has
to agree they read as two animals.

## 2. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-CAIRNBACK` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept input | `EBS-CON-KHA-UNT-003` |
| Selected candidate | `…/concept-discovery-20260906/cairnback-review/cairnback-candidate.png` (ASSAULT SCREEN, MINERAL COVER, STRATA DAMAGE) |
| Canon | `SPEC-UNIT-007`: a broad, low assault warform whose back is a slab of layered heat-holding strata like a vaultback's; thick forelimbs; head low and protected. Creating mineral cover is a heave that leaves a grown barrier behind it. Damage chips strata; destruction is a ceramic slump |
| Gameplay record | `units.json` `ka_cairnback`: assault screen, 245 HP, 270 cm/s, 800 cm sight, population 3; mineral cover at 450 cm, a 180 HP barrier lasting 300 ticks |
| Asset card | `REL-FAC-025.KA.CAIRNBACK.ASSET` — **provisional**, authored in this worktree, not in `Docs/Requirements.md` |
| Production ID | `EBS-KHA-UNT-003`, planned `SK_EBS_KHA_UNT_003` under `/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_003/` |

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Standing height | 228 cm | larger than the Riftstalker's 201, as a population-3 assault unit |
| Body length | 323 cm = 1.41 of the height | traced on the candidate (1.45) |
| Body width | 240 cm | broad: 0.74 of the length |
| Leg clearance | 54 cm = 0.24 of the height | traced (0.20) |
| Units / axes / pivot | cm; +X forward, +Y right, +Z up; pivot at the ground-contact centre between the feet | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-kha-unt-003-concept-v1`)

Generator: [build_cairnback.py](build_cairnback.py). One skinned mesh `SK_EBS_KHA_UNT_003`. Slots:
`MI_EBS_KHA_Strata`, `MI_EBS_KHA_Amber`. Rig, from the card: `root`, `body`, a two-bone back slab, a
short neck and head, and four limb chains — 22 bones, sixteen of them in the limbs. Sockets (5):
`Cover_Cast_Origin`, `Back_Slab_Center`, `Target_Anchor_Center`, `Foot_Contact_FL`, `Foot_Contact_FR`.
Clips: `idle`, `move`, `attack`, `heave`, `damage_chip`, `death`. Budgets: LOD0 908 and LOD1 288
against the card's 8,000 / 3,500. Amber is 10.7% of surface area against the `REL-ART-029` ceiling.

**Triangle counts far under the ceiling are headroom, not sufficiency** (owner ruling 2026-09-07). The
shell reads as stepped courses rather than the candidate's smoother overlapping dome, and that detail
judgement against the concept at gameplay distance has not been made.

**The mineral cover is a separate asset.** This package builds only the heave motion and the
`Cover_Cast_Origin` socket the runtime places a barrier from. A test asserts no component here is a
barrier or cover.

## 5. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-UNT-003/`. Renders: orthographic side,
front and top with a 180 cm reference figure, a three-quarter, a head close-up, a tactical pass, and a
dedicated ground-free monochrome silhouette scene. Seven comparison sheets and two silhouette reports
in `renders/concept-compare/`. Checks: 21 structural tests in
[test_cairnback_build.py](test_cairnback_build.py) — the two silhouette contrast ratios and their
separation from the Riftstalker, the head tucked behind the shell, four limbs on four distinct gait
phases, the shell stepping inward, the traced proportion, amber confined to the seams and under the
ceiling, the mineral cover absent, the chipped state breaking strata, the card's bone plan and limb
weighting, the attack not pitching the body, the heave moving the back slab, **no frame of any clip
passing through the ground**, foot planting being measured rather than guessed, frame alignment,
sockets, the provisional budget with its headroom note, LOD1 keeping the shell and four legs, and
determinism — all passing. Not yet: textures, in-engine capture, gates.

## 6. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-KHA-UNT-003"
python3 build_cairnback.py --evidence-dir "<evidence root>/EBS-KHA-UNT-003" --skinned
python3 build_cairnback.py --evidence-dir "<evidence root>/EBS-KHA-UNT-003" --check
python3 test_cairnback_build.py
python3 ../tools/ebs_silhouette.py \
  --a "<evidence root>/EBS-KHA-UNT-002/renders/silhouette/sil_tactical.png" \
  --b "<evidence root>/EBS-KHA-UNT-003/renders/silhouette/sil_tactical.png" \
  --label-a riftstalker --label-b cairnback
```

## 7. Decisions and open items

1. **Building the gait exposed a defect in the already-committed Riftstalker.** Testing that these two
   animals walk differently showed that neither did: in both generators the key *time* and the swing
   *phase* were derived from the same value, so every limb received an identical curve and all four
   legs moved in unison. The Riftstalker's "diagonal gait" was a hop. Both generators are fixed in this
   commit, and the Riftstalker's package is rebuilt, re-tested and re-imported.
2. **Appended keys are invisible to the sampler.** `AnimationClip.key` appends, and a second key at the
   same time is never read, so the foot-planting corrections silently did nothing until they replaced
   keys instead of adding them. That cost two rounds of confusing measurements.
3. **Foot planting is measured, not dialled in.** Stubby vertical legs on flat plates put a corner
   underground on any rotation — six degrees costs about six centimetres. Each foot is counter-rotated
   by the negation of everything above it, and the residual is removed by raising the body. The lifts
   are in the manifest: every clip needs none except `move`, which needs 6.33 cm.
4. **The feet float up to 2.6 cm during the walk.** That is the price of the constant lift above. It is
   measured and recorded rather than left for someone to notice.
5. **The stride is deliberately small.** A six-degree stride dropped the body eleven centimetres and
   the constant lift needed to fix it would pop against the idle pose on transition. Two and a half
   degrees costs 6.33 cm instead.
6. Open: the shell's stepped read against the candidate's smoother dome, the walk's floating feet,
   textures, a detail judgement at gameplay distance, in-engine capture, gate reviews, incorporation of
   the provisional card into the authoritative requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
