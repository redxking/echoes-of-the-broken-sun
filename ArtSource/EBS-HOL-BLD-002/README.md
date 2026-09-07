---
title: EBS-HOL-BLD-002 Interval Loom — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-HC-INTERVAL-LOOM
production_asset_id: EBS-HOL-BLD-002
production_maturity: BLOCKOUT
revision: ebs-hol-bld-002-concept-v1
canon_status: CANDIDATE (built to the interval-loom candidate against canon SPEC-BLD-017.HC.INTERVAL); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-HOL-BLD-002 Interval Loom — production source

Single authoritative source record for the Hollow Choir supply node. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
[`REL-BLD-017.HC.INTERVAL.ASSET`](../hollow-choir-asset-cards.md); maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-HC-INTERVAL-LOOM` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Selected candidate | `…/concept-discovery-20260906/interval-loom-review/interval-loom-candidate.png` |
| Canon | `SPEC-BLD-017.HC.INTERVAL`: supply node, 400 HP, 600 cm sight, +6 logistics, 2×2 footprint; charges 5 Dawn every 600 ticks, reduced to 4 inside a Phase Anchor field |
| Asset card | `REL-BLD-017.HC.INTERVAL.ASSET` — **provisional**, authored after the trace. **`REL-FAC-027.HC.INTERVALIST.ASSET` is the Intervalist UNIT and does not govern this building**, despite the shared word; the manifest and a test both say so |
| Production ID | `EBS-HOL-BLD-002`, planned `SK_EBS_HOL_BLD_002` under `/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_002/` |

## 2. Geometry and rig (revision `ebs-hol-bld-002-concept-v1`)

Generator: [build_interval_loom.py](build_interval_loom.py). Two ribbon arches cross diagonally over
the bare 2×2 footprint, springing from four flat foot plates, with a drop-off pad and delivered matter
beside them and magenta fracture edge lines down both borders of every arch. Slots:
`MI_EBS_HOL_Vitrified`, `MI_EBS_HOL_ApronStone`, `MI_EBS_HOL_Magenta`. Rig: `root` plus one bone per
arch. Sockets (4): `Target_Anchor_Center`, `Matter_Dropoff`, `Arch_Crossing_Center`, `Upkeep_Signal`.
Clips: `supplied_idle`, `upkeep`, `restore`. Budgets: LOD0 756 and LOD1 420 against the card's
3,000 / 1,200. Magenta is 5.4% against the 12% ceiling.

**The solvency read is the asset's job, and it is geometry as well as material.** Supplied lights 36
edge segments; insolvent lights none, because the edges move into the vitrified slot; the upkeep tick
adds a surge band across both crowns that no other state carries. Brightness alone is a material
parameter, so relying on it would have made two states identical in the mesh.

## 3. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-HOL-BLD-002/`. Renders: orthographic front and
top with a 180 cm reference figure, a three-quarter and a tactical pass, for each of the four states.
Checks: 18 structural tests in [test_interval_loom_build.py](test_interval_loom_build.py) — the
contract inventory, the arches crossing on opposite diagonals over the centre, the arch springing from
its foot plate, the arch being broad rather than pointed, the three power states differing in
geometry, magenta being the only emissive and under the ceiling in both lit states, **no other
faction's language in any component or slot name**, the destroyed state keeping its feet and pad,
footprint and ground containment, no clip frame passing through the ground, the two arches drifting out
of step, sockets, frame alignment, the provisional card with its Intervalist disclaimer, LOD1 keeping
both arches and the pad, and determinism — all passing.

## 4. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-HOL-BLD-002"
python3 build_interval_loom.py --evidence-dir "<evidence root>/EBS-HOL-BLD-002" --skinned
python3 build_interval_loom.py --evidence-dir "<evidence root>/EBS-HOL-BLD-002" --check
python3 test_interval_loom_build.py
```

## 5. Decisions and open items

1. **The traced 0.648 is a bounding box, not an arch proportion.** The candidate is drawn
   isometrically, so its overall height-to-width ratio mixes the arch rise with the ground plan. It is
   recorded as a bounding measurement, and the arch's own rise against its span (0.64) is stated as a
   design choice from the drawing rather than dressed up as a measurement.
2. **The kit has no pitched box, so the arches are built from oriented quads.** `kit.Mesh.box` takes a
   yaw only; boxes alone would have laid the arches flat. A local helper places each ribbon segment's
   eight corners from its own axis, side and up vectors.
3. **The edge lines were 27% of the surface area at first.** At 3 × 13 cm they breached the 12% magenta
   ceiling more than twice over. At 1.6 × 1.2 cm they measure 5.4% and still read as lines.
4. **The arch springs from the top of its foot plate.** Starting the ribbon at ground level put its
   underside 2.4 cm below the floor, because a ribbon has thickness.
5. **The arch drift is a presentation device**, labelled as such in the card and the manifest so it
   cannot later be cited as canon motion. The articulated-component requirement stays pending.
6. Open: textures, a detail judgement at gameplay distance, in-engine capture, gate reviews,
   incorporation of the provisional card into the authoritative requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
