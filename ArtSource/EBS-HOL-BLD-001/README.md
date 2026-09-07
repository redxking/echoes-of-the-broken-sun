---
title: EBS-HOL-BLD-001 Concordance — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-HC-CONCORDANCE
production_asset_id: EBS-HOL-BLD-001
production_maturity: BLOCKOUT
revision: ebs-hol-bld-001-concept-v1
canon_status: CANDIDATE (built to the concordance candidate against canon SPEC-BLD-017.HC.CONCORDANCE); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-HOL-BLD-001 Concordance — production source

Single authoritative source record for the Hollow Choir command core, and the first of the four
remaining packages. Bounded by the shared [agent contract](../../AGENTS.md) and the
[authority map](../../Docs/README.md); the visual target is [concept-fidelity.md](concept-fidelity.md);
the contract is the provisional card [`REL-BLD-017.HC.CONCORDANCE.ASSET`](../hollow-choir-asset-cards.md);
maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Traced before the card was written

The owner directed on 2026-09-07 that each remaining concept be traced **before** its provisional card
is authored, after three Kharuun cards written the other way round described the wrong asset. The order
here was: trace the candidate, write the card from the trace, write this target, then build.

| Traced on the candidate's WORKING view | Value | Built |
|---|---|---|
| ring across | 952 px | 940 cm |
| slab height ÷ ring diameter | 0.24 | 0.240 |
| slab pairs, counted on the RING / INTAKE view | 14 | 14 |

## 2. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-HC-CONCORDANCE` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Selected candidate | `…/concept-discovery-20260906/concordance-review/concordance-candidate.png` |
| Canon | `SPEC-BLD-017.HC.CONCORDANCE`: Choir Command Core and Matter drop-off. 1,250 HP, 900 cm sight, +12 logistics, 5×5 footprint, produces Threadkeepers, exempt from the non-Core coherence upkeep every other Choir structure pays |
| Gameplay record | `buildings.json` `hc_concordance` |
| Asset card | `REL-BLD-017.HC.CONCORDANCE.ASSET` — **provisional**, authored in this worktree after the trace. **No authoritative card covers this building**: the four `REL-FAC-027.HC.*` cards are unit cards |
| Production ID | `EBS-HOL-BLD-001`, planned `SK_EBS_HOL_BLD_001` under `/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_001/` |

## 3. Geometry and rig (revision `ebs-hol-bld-001-concept-v1`)

Generator: [build_concordance.py](build_concordance.py). One skinned mesh `SK_EBS_HOL_BLD_001`. Slots:
`MI_EBS_HOL_Vitrified`, `MI_EBS_HOL_ApronStone`, `MI_EBS_HOL_Magenta`. A low 940 cm disc carries
fourteen slab pairs, each a 226 cm vitrified slab with a duplicate offset 3 cm behind it, with magenta
fracture edging down both borders of every slab and a ground intake apron at the front. Rig: `root` plus
one bone per pair, fifteen in all. Sockets (5): `Target_Anchor_Center`, `Unit_Emergence`,
`Rally_Default`, `Matter_Dropoff`, `Choir_Thread_Center`. Clips: `working_idle`, `damaged`, `restore`.
Budgets: LOD0 1,144 and LOD1 392 against the card's 8,000 / 3,500. Magenta is 8.8% against the 12%
ceiling.

**The 3 cm offset and the 12% magenta ceiling are inferences, and the manifest says so.** Both come
from the four Hollow Choir *unit* cards, which specify an offset duplicate layer for reality bleed and
cap Magenta Fracture at 12%. No authoritative card places either on a Choir building. They are adopted
as faction language with their provenance recorded rather than implied.

**The centre filament is not geometry.** The candidate draws a faint thread rising inside the ring. It
is a light effect; this build provides the `Choir_Thread_Center` socket and nothing else, and a test
asserts no component pretends to be it.

## 4. States

| State | Read |
|---|---|
| working | every pair whole, edging lit, apron clear |
| damaged | pair 4 cracked and drifting further than the rest |
| destroyed | the ring broken: slabs down on a scarred disc |

## 5. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-HOL-BLD-001/`. Renders: orthographic front and
top with a 180 cm reference figure, a three-quarter, a close pair view, and a tactical pass, for each
state. Five comparison sheets in `renders/concept-compare/`. Checks: 20 structural tests in
[test_concordance_build.py](test_concordance_build.py) — the contract inventory, the traced proportion
and pair count, each pair being two slabs exactly 3 cm apart, the duplicate staying visible as a
duplicate, two fracture edges per slab that are not stacked, magenta being the only emissive and under
the ceiling, **no other faction's language present in any component or slot name**, the centre thread
absent from geometry, the states differing, the damaged pair drifting further, the drift phases being
real rather than in unison, footprint and ground containment, no clip frame passing through the ground,
sockets, the provisional card and its recorded provenance, LOD1 keeping the ring and apron, and
determinism — all passing.

## 6. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-HOL-BLD-001"
python3 build_concordance.py --evidence-dir "<evidence root>/EBS-HOL-BLD-001" --skinned
python3 build_concordance.py --evidence-dir "<evidence root>/EBS-HOL-BLD-001" --check
python3 test_concordance_build.py
```

## 7. Decisions and open items

1. **The fracture edges were briefly one bar, not two borders.** Both edge boxes were placed at the
   slab centre because the lateral offset was written and then discarded. The offset now follows the
   ring's tangent, and a test requires the two edges to sit more than half a slab width apart.
2. **The pair drift is a presentation device, not canon motion.** Canon gives this building no motion
   clause. The per-pair drift implements the faction's reality-bleed language, and both the card and the
   manifest label it as such so it cannot later be cited as a canon requirement.
3. **The articulated-component requirement stays pending.** The pair drift does not satisfy it.
4. Open: textures, a detail judgement at gameplay distance, in-engine capture, gate reviews,
   incorporation of the provisional card into the authoritative requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
