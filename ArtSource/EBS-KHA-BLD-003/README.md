---
title: EBS-KHA-BLD-003 Growth Basin — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-GROWTH-BASIN
production_asset_id: EBS-KHA-BLD-003
production_maturity: BLOCKOUT
revision: ebs-kha-bld-003-concept-v1
canon_status: CANDIDATE (built to the growth-basin candidate against canon SPEC-BLD-016); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-BLD-003 Growth Basin — production source

Single authoritative source record for the Kharuun production centre. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
[`REL-BLD-016.KA.BASIN.ASSET`](../kharuun-asset-cards.md); maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-GROWTH-BASIN` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Selected candidate | `…/concept-discovery-20260906/growth-basin-review/growth-basin-candidate.png` (GROWING, RESTING, MOLT NICHE) and `growth-basin-state-contrast.png` |
| Canon | `SPEC-BLD-016` Growth Basin: a shallow bowl of grown strata with an amber-lit matrix pool at its center, ringed by visible molt niches. The pool says *grows warforms*; niches say *adaptation choices*. Growing shows the pool brightening; a molting warform stands in a niche and visibly changes. Sounds: liquid mineral resonance; a crack-and-settle on molt completion |
| Gameplay record | `buildings.json` `ka_growth_basin`: production, 700 HP, 500 cm sight, 160 construction ticks, 4×4 footprint; adaptation within 600 cm, 80-tick molt at 150% damage taken |
| Asset card | `REL-BLD-016.KA.BASIN.ASSET` — **provisional**, authored in this worktree, not in `Docs/Requirements.md` |
| Production ID | `EBS-KHA-BLD-003`, planned `SK_EBS_KHA_BLD_003` under `/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_003/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Growth Basin trains every Assemblies combat unit and is where warforms molt. Two reads
must survive at gameplay distance: the pool says this place grows things, and the niches say adaptation
happens here. A third read matters tactically — which niches are occupied, because a molting warform
takes 150% damage for 80 ticks. What must be absent: machinery around the rim, vats, pipework.

**DETAIL.** Large scale: one shallow circular bowl, wider than anything else the faction builds at this
size, with a wide amber pool. Medium: a continuous coursed outer ring with six alcoves cut into it, each
alcove's back wall lower than the ring so a player can see into the pocket. Fine: mineral grain, pool
surface fracture, wear on the alcove floors — texture.

**ACTION.** Growing: the pool brightens. Molting: the occupied alcove's floor and wall lip light. On
completion, canon's crack-and-settle — the niche stone drops sharply, overshoots and settles back.
Sound (canon): liquid mineral resonance, and a crack-and-settle on completion — specified, not produced.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 4×4 tiles = 800 cm square; every part inside it in every state and pose | `buildings.json`, asserted by test |
| Basin diameter | 760 cm | sized to sit inside the square |
| Pool diameter | 570 cm = 0.75 of the basin | traced on the candidate's GROWING view |
| Height | 128 cm = 0.17 of the basin diameter | canon's "shallow bowl", asserted by test |
| Molt ticks | 80 = 4.0 s at 20 ticks per second | `ka_growth_basin.adaptation.molt_ticks` |
| Units / axes / pivot | cm; +X forward, +Y right, +Z up; pivot at the bowl's ground-contact centre | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-kha-bld-003-concept-v1`)

Generator: [build_basin.py](build_basin.py). One skinned mesh `SK_EBS_KHA_BLD_003`. Slots:
`MI_EBS_KHA_Strata`, `MI_EBS_KHA_Amber`. Rig, from the card: `root` plus one bone per niche, six in
all. Sockets (9): `Target_Anchor_Center`, `Rally_Default`, `Pool_Center` and `Molt_Niche_01`–`06`, each
on its own niche bone. Clips: `idle`, `growing`, `molt_start`, `molt_complete`, `restore`. Budgets:
LOD0 1,162 idle and 1,250 molting, LOD1 784, against the card's 8,000 / 3,500. Amber is 11.2% of
surface area against the `REL-ART-029` ceiling of 15%, and the pool is most of it, which the card allows
because canon makes the pool the asset's identity.

**The niches do not close.** The card's first draft said a niche closes around a molting warform. The
candidate's MOLT NICHE panel shows a warform standing in an open alcove, and canon describes only a
crack-and-settle on completion, so the closure was my invention. The card was corrected and each niche
bone now carries the settle instead. A test asserts no state contains a hood, shutter or lid.

## 5. States

| State | Read | Authority |
|---|---|---|
| idle | the pool holds a low glow; every niche dark | |
| growing | the pool brightens — this is the pool's emissive, a material parameter, not geometry | canon |
| molting | the occupied niches' floors and wall lips light; the rest stay dark | the 80-tick molt window |
| damaged | the bowl cracked and the pool out of the amber slot | |
| destroyed | the bowl broken, the pool dark, a rubble ring where the rim came down | |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-BLD-003/` (`receipt.json`, `review/`
assemblies for the five states, LOD1 and three sampled poses, `scenes/`, `renders/`). Renders:
orthographic front and top with a 180 cm reference figure, a three-quarter matching the candidate's
framing, a close niche view answering the candidate's own MOLT NICHE panel, and a tactical pass, for
each state. Six comparison sheets in `renders/concept-compare/`. Checks: 19 structural tests in
[test_basin_build.py](test_basin_build.py) — the contract inventory, the niches never closing in any
state, the traced pool ratio and canon's shallow bowl, amber being the only emissive and under the
ceiling, per-niche occupancy, growing changing nothing geometric, the crack-and-settle's drop-overshoot-
settle shape with unoccupied niches held still, clip durations against the canon molt ticks, the rig
being root plus one bone per niche, the plinth bound to root so the settle cannot push the alcove
underground, footprint and ground containment in every state and pose, sockets, state differences, the
provisional budget, LOD1 keeping the pool and every niche, and determinism — all passing. Not yet:
textures, in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-KHA-BLD-003"
python3 build_basin.py --evidence-dir "<evidence root>/EBS-KHA-BLD-003" --skinned
python3 build_basin.py --evidence-dir "<evidence root>/EBS-KHA-BLD-003" --check
python3 test_basin_build.py
for s in idle growing molting damaged destroyed lod1 pose_molt_start_100 \
         pose_molt_complete_015 pose_molt_complete_050; do \
  python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-KHA-BLD-003/scenes/$s.json" \
    --out "<evidence root>/EBS-KHA-BLD-003/renders/$s"; done
```

## 8. Decisions and open items

1. **Nothing states how many niches this building has.** The provisional card requires the niche count
   to match the authoritative adaptation contract. There is no such number: `ka_growth_basin.adaptation`
   carries `site_radius_cm`, `molt_ticks`, `dawn_cost` and the stat deltas, and nothing about how many
   warforms may molt at once. Six niches are **counted off the candidate's GROWING view** and used
   here. That is a reading of a concept image, not a gameplay claim, and the manifest records it as an
   unresolved contract gap. **OWNER-QUESTION A.**
2. **The niches are open alcoves; I corrected my own card, not the model.** Recorded in §4 and on the
   card itself.
3. **The occupancy tell needed a second surface.** With open alcoves the only tell is the lit niche
   floor, and the floor sits down inside the pocket where the ring spurs hide it at gameplay distance.
   An occupied niche now also lights the lip of its low back wall, which reads over the ring from the
   tactical camera. It is legible but subtle at maximum zoom-out; texture and a brighter emissive
   should carry it further, and that is recorded rather than assumed.
4. **The outer ring is continuous stone with alcoves cut into it.** A first pass built the niche walls
   as free-standing curved slabs outside the rim, which read as debris rather than architecture, and
   also pushed the asset 53 cm outside its footprint. The ring is now solid between the alcoves, and
   each alcove's back wall is lower than the spurs so the pocket reads.
5. **The alcove plinth is bound to `root`, not to its niche bone.** The crack-and-settle drops the niche
   by 11 cm; with the plinth on the same bone the alcove went through the ground.
6. **`growing` moves nothing.** In that state the change is the pool's emissive, which is a material
   parameter. The clip exists so the runtime has a named state to play, holds every bone at rest, and
   the manifest says so rather than implying the geometry moves.
7. Open: textures (2048² packed PBR with micro-noise normals), a stronger occupancy read, in-engine
   capture, gate reviews, incorporation of the provisional card into the authoritative requirements,
   owner acceptance.

> **OWNER-QUESTION A — the niche count has no authoritative source.** The card says the count shall
> match the adaptation contract, and that contract does not carry one. I built six, counted off the
> candidate. Confirm six, give a number, or direct that a concurrent-molt capacity be added to
> `ka_growth_basin.adaptation` — which is a gameplay data change and outside this worktree's authority.
