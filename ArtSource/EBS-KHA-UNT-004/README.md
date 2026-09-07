---
title: EBS-KHA-UNT-004 Resonant — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-RESONANT
production_asset_id: EBS-KHA-UNT-004
production_maturity: BLOCKOUT
revision: ebs-kha-unt-004-concept-v1
canon_status: CANDIDATE (built to the resonant candidate against canon SPEC-UNIT-008); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-UNT-004 Resonant — production source

Single authoritative source record for the Kharuun scout, and the last of the eight Kharuun packages.
Bounded by the shared [agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md);
the visual target is [concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
[`REL-FAC-025.KA.RESONANT.ASSET`](../kharuun-asset-cards.md); maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Silhouette: separation from two quadrupeds

This is the third Kharuun quadruped, so it had to separate from both the others. Its separation is that
it is taller than it is long, which neither of them is.

| Measure | Riftstalker | Cairnback | Resonant |
|---|---|---|---|
| height ÷ max horizontal extent | 0.68 | 0.71 | **1.53** |
| dominant mass | four long legs | one domed shell | a dorsal fin array on stilts |

Measured by [../tools/ebs_silhouette.py](../tools/ebs_silhouette.py) on ground-free monochrome renders
from a shared camera:

| Monochrome tactical pair | Intersection over union | Largest band difference |
|---|---|---|
| Resonant vs Riftstalker | 0.392 | 0.469 |
| Resonant vs Cairnback | 0.344 | 0.406 |

Both are lower than the Riftstalker-Cairnback pair's 0.540, so this is the most separable of the three.
`renders/concept-compare/three_kharuun_quadrupeds.png` shows all three side by side in monochrome.

## 2. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-RESONANT` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept input | `EBS-CON-KHA-UNT-004` |
| Selected candidate | `…/concept-discovery-20260906/resonant-review/resonant-candidate.png` (LISTENING, DETECTING, SENSOR FINS) |
| Canon | `SPEC-UNIT-008`: a tall, thin scout with sensor-fins of translucent amber along the spine and head, and a delicate frame. Fins and delicacy say *listens, does not fight*. Detecting shows the fins brightening in sequence. Almost silent movement |
| Gameplay record | `units.json` `ka_resonant`: scout and counter-scout, 85 HP, 470 cm/s, 1,550 cm sight, population 1; vibration detection to 2,200 cm |
| Asset card | `REL-FAC-025.KA.RESONANT.ASSET` — **provisional**, authored in this worktree, not in `Docs/Requirements.md` |
| Production ID | `EBS-KHA-UNT-004`, planned `SK_EBS_KHA_UNT_004` under `/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_004/` |

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Standing height | 255 cm | the tallest Kharuun unit and the lightest, at 85 health |
| Body length | 167 cm | short, so the traced height-to-extent ratio holds |
| Height ÷ extent | 1.53 | traced on the candidate (1.60) |
| Limb radius | 7.5 cm | delicacy is the read |
| Fins | 13 | counted on the candidate |

## 4. Geometry and rig (revision `ebs-kha-unt-004-concept-v1`)

Generator: [build_resonant.py](build_resonant.py). One skinned mesh `SK_EBS_KHA_UNT_004`. Slots:
`MI_EBS_KHA_Strata`, `MI_EBS_KHA_Amber`. Rig, from the corrected card: `root`, `body`, a four-segment
spine, `neck`, `head` and four stilt chains — 24 bones. Sockets (4): `Fin_Array_Base`, `Fin_Array_Tip`,
`Target_Anchor_Center`, `Emitter_Muzzle`. Clips: `idle`, `move`, `detect`, `attack`, `death`. Budgets:
LOD0 736 and LOD1 540 against the card's 4,500 / 1,800.

**The emissive is the vein, not the whole fin.** Canon calls the fins translucent amber, and thirteen
fully amber blades measured 32% of surface area against `REL-ART-029`'s 15% ceiling. The translucency
belongs to the texture; the emissive is a narrow vein along each blade, which is also what the
candidate's SENSOR FINS panel actually draws glowing. Amber now measures 7.7% in the detecting state.

**The sequence is a material animation.** The `detect` clip carries the spine's lowering and sweep; the
ordered brightening along the array is driven per fin by the runtime, and every fin is its own
component so it can be.

## 5. States

| State | Read |
|---|---|
| passive | fins in the plate slot, head carried high |
| detecting | every fin's vein in the amber slot; the order toward the source is the runtime's animation |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-UNT-004/`. Renders: orthographic side,
front and top with a 180 cm reference figure, a three-quarter, a fin close-up answering the candidate's
own SENSOR FINS panel, a tactical pass, and a ground-free monochrome silhouette scene. Five comparison
sheets and two silhouette reports in `renders/concept-compare/`. Checks: 20 structural tests in
[test_resonant_build.py](test_resonant_build.py) — four legs, taller than long with measured separation
from both other quadrupeds, the fin array spanning the back, fins ordered and individually lit, the
emissive confined to the veins and under the ceiling, the spine arching, the frame staying delicate,
the rig weighted toward the spine, the detect clip lowering and sweeping, the gait phases being real,
**no frame of any clip passing through the ground**, foot planting measured, frame alignment, sockets,
the provisional budget, LOD1 keeping every fin, and determinism — all passing.

## 7. Decisions and open items

1. **Third anatomy correction in eight cards.** The card said biped; the candidate shows four stilt
   legs. All eight Kharuun cards were authored in one pass before their concepts were traced, and three
   described the wrong thing: this one, the Riftstalker's anatomy, and the Growth Basin's closing
   niches. The faction rules now require a card to be written after its candidate is traced. That rule
   arrived three errors late, which is the honest summary.
2. **A key-replacement helper deleted the keys it meant to insert.** `AnimationClip.key` appends **and
   sorts**, so the newly added key is not necessarily last. A helper that took the last element as "the
   one just added" removed the real new key whenever its time was not the largest, which emptied a body
   track down to one key and made the collapse interpolate from nothing. Fixed here and in the
   Cairnback, which carried the same helper.
3. **The fin array first merged into one lit crest.** The spine arc spanned 40 cm, so thirteen fins
   packed into less than one fin's length. The arc now runs the length of the back.
4. **The collapse is bounded by what the stilts support.** Measured on the rig: at thigh 60 and shin 40
   these legs hold an 82 cm settle; the first version's 26 / −34 fold supported almost none and put the
   feet 2.8 cm under. The body rides 17.3 cm higher through that clip to keep the feet planted, which
   is recorded in the manifest rather than hidden.
5. Open: textures including the fins' translucency mask, a detail judgement at gameplay distance,
   in-engine capture, gate reviews, incorporation of the provisional card into the authoritative
   requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
