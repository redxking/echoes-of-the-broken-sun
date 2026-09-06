---
title: EBS-FWL-SYS-001 Future Well — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
package: EBS-PKG-EBS-FAM-FWL-001
production_asset_id: EBS-FWL-SYS-001
production_maturity: BLOCKOUT
revision: ebs-fwl-sys-001-blockout-v1
canon_status: CANDIDATE (one-family direction selected under delegation; REL-ART-014 amendment prepared, not applied)
status: Isolated production source; no Unreal integration authorization
---

# EBS-FWL-SYS-001 Future Well — production source

Single authoritative source record for the Future Well state family (the state-family pilot of the
owner's production handoff). Bounded by the shared [agent contract](../../AGENTS.md), the
[authority map](../../Docs/README.md) and the frozen preparation records; production-lane maturity is
in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `production_policy[EBS-PKG-EBS-FAM-FWL-001]`; GAP-01: amber Harvest telegraph and inward spire collapse, charcoal spent bowl permanent, Preserve cyan custody, Reshape magenta temporary terrain, Dormant one bowl/spire with doubled shadows ([gap-decisions.json](../../Docs/VisualAssetPipeline/motion/gap-decisions.json)) |
| Prepared amendment | `FWL-HARVEST` rewrites `REL-ART-014` (cyan geyser → amber rise and fold); status PREPARED_NOT_APPLIED; recorded authority conflict, not resolved here |
| Reserved production ID | `EBS-FWL-SYS-001`, planned `SM_EBS_FWL_SYS_001` under `/Game/Echoes/Production/FWL/SYS/EBS_FWL_SYS_001/` |
| Family | `EBS-FAM-FWL-001` (six concept members; one stateful system; state authority `Content/Data/Source/future_wells.json`) |
| Selected candidate | `future-well-review/future-well-candidate.png`, sha256 `b16e84cb…d60ba7`; state board `motion/storyboards/EBS-PKG-EBS-FAM-FWL-001-GAP-01.svg`; motion package GAP-01 (six phases, reduced motion, cancel/restore rules) |
| Canon | Bible "Future Wells": Harvest 180-tick public telegraph then permanent collapse; Preserve intact (15 Dawn / 300 ticks, 1,400 cm intelligence); Reshape 120 Dawn, 180-tick telegraph, 1,800-tick authored possibility with warned expiry |
| Book | ¶243 "Amber light began to climb the core spire"; ¶245 "The spire folded into the bowl"; ¶246 the harmonic ends abruptly, a strip of fractured ground gives way |
| Requirements | `SPEC-WEL-001..004` (impassable, indestructible landmark; 420 cm capture zone), `SPEC-WELLP-001..003`, `REL-WEL-005/006/010/015/016` (600 cm scar radius), `REL-ART-014` (amendment pending), `REL-ART-016`/`REL-ART-030` (passable dressing ≤20 cm, never reads impassable), `SPEC-VISD-007` effects grammar |
| Simulation envelope | `Simulation.cpp`: Well footprint half extent `kFixedScale/2` (one tile), capture radius 4.2 tiles, scar radius 6 tiles; presentation tile 200 cm; the runtime shrinks the current 280 cm-radius placeholder dais by 0.35 to fit; the M01 map lists the Well with a 1-tile half-extent reservation |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** A Future Well is a neutral, indestructible landmark at which one committed protocol
decides a mission's future: Harvest (immediate Dawn, permanent collapse and scar), Preserve
(contestable custody and income) or Reshape (a temporary authored possibility). It sits on open
ground inside a 4.2-tile capture zone that workers must occupy; nothing about it is a weapon or a
structure to build. What must be absent: any faction identity, any reading that the apron blocks
movement, any implied state the simulation has not committed.

**DETAIL.** Large scale: a low charcoal masonry bowl one tile across with a slim four-petal spire,
set in a flat fractured apron three tiles across. Medium: sixteen rim courses of uneven height
(ruined rhythm), eight low apron ridges, the Preserve ring and the Reshape trace as shallow channels,
the core stub. Fine: coping stones, seam lights on the petals, rung marks along the trace. Material
logic: vitrified basalt masonry and charcoal glass with magenta micro-fracture (`vitrified_glass`),
matte ≥0.85 on every ground-facing surface.

**ACTION.** Thirteen state presentations (§5) driven only by authoritative protocol state: colour
lives in the state slot's masks (dormant veins, amber petal seams, cyan ring, magenta trace) and the
one moving element is the petal set, which tilts inward and sinks into the bowl on Harvest commit;
the spent bowl is permanent and non-interactive. Doubled shadows (Crownfall dual key/fill) are a
lighting/decal cue, not geometry. Sound intent from the motion package (dormant hum, Harvest rise and
hard cut, Preserve cadence, Reshape phase tone and expiry warning) is specified, not produced.
Reduced motion: discrete state shapes and steady masks, no rapid flash. Boundaries: the apron is
walkable dressing; the capture zone, intelligence radius, scar radius and the Reshape feature itself
are simulation- and map-owned.

**REVIEW.** Fields checked against §1 before geometry (2026-09-06). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Impassable mass | bowl radius 98 cm (< one-tile half extent 100) with a 196×196×72 cm collision box | simulation footprint half extent 0.5 tile |
| Apron | radius 300 cm, relief ≤16 cm, no blocking | REL-ART-016/030 walkable-dressing rules; sits inside the 840 cm capture zone |
| Spire | top at 180 cm above ground (petal 172 cm on an 8 cm floor) | PROVISIONAL; reads as a landmark beside 176–265 cm units without occluding the capture zone |
| Units / axes / pivot | cm; +X = Reshape trace direction; pivot at ground-contact centre | contract `import_policy`; the trace direction is a placement decision for the map |

## 4. Geometry (revision `ebs-fwl-sys-001-blockout-v1`)

Generator: [build_future_well.py](build_future_well.py). Main mesh `SM_EBS_FWL_SYS_001` (bowl floor,
16 rim blocks with coping stones, three dormant veins, core stub, apron disc, eight ridges, Preserve
ring, Reshape trace with rungs) plus the petal part `SM_EBS_FWL_SYS_001_Petal` instanced at sockets
`Petal_01..04` (yaw 0/90/180/270, hinge on the bowl floor). Sockets: `Target_Anchor_Center`,
`State_VFX_Origin`, `Petal_01..04`, `Reshape_Trace_End`. Three slots: `MI_EBS_FWL_Basalt`,
`MI_EBS_FWL_Vitrified`, `MI_EBS_FWL_State`. Budgets: LOD0 986 ≤ 8,000; LOD1 638 ≤ 3,500 assembled
(large reserve for ART_ALPHA masonry and fracture detail). Exact numbers, sockets and hashes:
[build-manifest.json](build-manifest.json).

## 5. State presentations

| Track | Petals | State mask | Authority read |
|---|---|---|---|
| dormant | up | veins 0.25, seams 0.15 (quiet magenta/amber) | `wellChoice == Dormant` |
| harvest_telegraph | up | petal seams ramp 0.15→1.0 amber with telegraph progress | telegraph progress (180 ticks) |
| harvest_cancel | up | seams return to dormant; no spent pose, no success cue | control broken |
| harvest_commit | tilt inward 28°, sink 150 cm over ~20 ticks | seams 1.0 then hard cut | committed |
| spent | sunk; tips break the floor | all masks 0; dark cracked bowl, permanent | `collapsed` |
| preserve_hold / preserve_loss | up | cyan ring 1.0 with slow custody pulse / 0 | Preserve and controlled / lost |
| reshape_telegraph / cancel / manifest / warning / expiry | up | magenta trace ramps / returns / 1.0 / slow warning cadence / 0 | Reshape telegraph, manifest (1,800 ticks), warning, expiry |
| restore | per state | per state | load/replay; never replays payout, collapse or terrain creation |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-FWL-SYS-001/` (`receipt.json`, `review/`
assemblies for dormant, mid-fold, spent and LOD1; `scenes/`; `renders/` for dormant, harvest telegraph,
mid-fold, spent, preserve, reshape and LOD1 at the game's orthographic framing, near and far, plus a
monochrome pass). Checks: 8 structural tests in [test_future_well_build.py](test_future_well_build.py)
(inventory and thirteen presentations, one-tile containment of every raised component and the
collision box, apron relief ≤20 cm, sockets, spire stands/collapses inside the tile, budgets and slots,
determinism), all passing; author's visual inspection of every state render. Not yet: import,
textures, in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-FWL-SYS-001"
python3 build_future_well.py --evidence-dir "<evidence root>/EBS-FWL-SYS-001"; python3 build_future_well.py --evidence-dir "<evidence root>/EBS-FWL-SYS-001" --check
python3 test_future_well_build.py
python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-FWL-SYS-001/scenes/dormant.json" --out "<evidence root>/EBS-FWL-SYS-001/renders/dormant"
```

## 8. Decisions and open items

1. The raised bowl is confined to the simulation's one-tile footprint; landmark presence comes from
   the flat apron (rings, trace, ridges ≤16 cm). This reconciles the painted candidate (a broad bowl)
   with `SPEC-WEL-004` and the passable-dressing rules; the current runtime's 0.35 basin shrink shows
   the same constraint.
2. Collapse = inward tilt + sink (book: "folded into the bowl"); an outward fold was tried and rejected
   because the petals sprawled across walkable apron.
3. The Reshape trace points +X; the map places the asset so the trace faces the authored feature.
4. Open: `REL-ART-014` amendment must be applied by the integration task before the amber Harvest is
   master-compliant; import inspection; textures (2048² per contract); the doubled-shadow decal; VFX
   for the amber climb, custody pulse and trace; emissive-share measurement; gates and owner review.
