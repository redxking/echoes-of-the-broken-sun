---
title: EBS-KHA-BLD-001 Memory Hearth — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-MEMORY-HEARTH
production_asset_id: EBS-KHA-BLD-001
production_maturity: BLOCKOUT
revision: ebs-kha-bld-001-concept-v1
canon_status: CANDIDATE (a KEEP design-identity input and a REPLACE retained-history input, reconciled by the memory-hearth candidate); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-BLD-001 Memory Hearth — production source

Single authoritative source record for the Kharuun headquarters. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-MEMORY-HEARTH` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept inputs | `EBS-CON-KHA-BLD-005` **KEEP**, used as design identity (the grown mineral-organic dome with inhabited arched hollows, banded strata and a rooted adaptation crown) and `EBS-CON-KHA-BLD-001` **REPLACE**, retained history only (the armoured shrine-like headquarters is explicitly not the production direction; only "warm communal centre" is retained, and its geometry is excluded from the target constraints) |
| Selected candidate | `…/concept-discovery-20260906/memory-hearth-review/memory-hearth-candidate.png` (WORKING, INTAKE DETAIL, DAMAGED) |
| Canon | `SPEC-BLD-016.KA.HEARTH`, Bible line 555: a wide grown dome of banded strata with a warm amber glow from within, several arched worker hollows at the base, a matter-intake cleft, and a crown of rooted adaptation spires. Working: interior glow breathes slowly. Damaged: a spire dark, strata cracked. Destroyed: ceramic collapse inward |
| Gameplay record | `buildings.json` `ka_memory_hearth`: headquarters drop-off, 1,300 HP, 800 cm sight, 400 construction ticks, +12 logistics, footprint 5×5 tiles |
| Requirement | `REL-BLD-016.KA.HEARTH` (function) and `REL-ART-029` (Kharuun grown mineral architecture) |
| Asset card | `REL-BLD-016.KA.HEARTH.ASSET` — **provisional**, authored in this worktree at [../kharuun-asset-cards.md](../kharuun-asset-cards.md). Not yet in `Docs/Requirements.md`. See §8.1 |
| Production ID | `EBS-KHA-BLD-001`, planned `SM_EBS_KHA_BLD_001` under `/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_001/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Memory Hearth is the Kharuun command core, worker source and matter drop-off, and
losing it loses the match. A player must read three things from a distance: that it is inhabited, where
workers come out, and where matter goes in. What must be absent: anything volcanic — no lava, fire,
smoke or molten cracks — and any machined panel or bolt, which belong to the Meridian language.

**DETAIL.** Large scale: one wide banded dome, broader than it is tall, on open ground. Medium: five
arched worker hollows with tended thresholds around the front arc, a low wide intake cleft on the right
flank with delivered matter at its mouth, and a crown of seven tapered spires rooted in the shell.
Fine: micro-noise mineral grain, worn paths at the thresholds, faint veining in the spires — texture.

**ACTION.** Working: the interior glow breathes slowly. Damaged: one spire's vein goes dark and a
fracture steps down the front flank. Destroyed: the shell collapses inward and settles as a rubble
ring. Sound (canon): a deep communal hum below music tempo and a cleft settle on delivery — specified,
not produced.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 5×5 tiles = 1,000 cm square; every part inside it in every state | `buildings.json`, asserted by test |
| Dome base | 880 cm across | the thresholds, the cleft apron and the delivered matter have to fit inside the square too (§8.2) |
| Shell height | 387 cm = 0.44 of the base width | traced on the candidate |
| Total height | 607 cm = 0.69 of the base width | traced on the candidate |
| Spire spread | 0.55 of the base width | traced on the candidate |
| Units / axes / pivot | cm; +X forward, +Y right, +Z up; pivot at the dome's ground-contact centre | contract `import_policy` |

Canon says "breadth greater than shell height". The trace puts a number on it: the shell is 0.44 as
tall as the dome is wide, and a test asserts the relation rather than the wording.

## 4. Geometry (revision `ebs-kha-bld-001-concept-v1`)

Generator: [build_hearth.py](build_hearth.py). One static mesh `SM_EBS_KHA_BLD_001`. Slots:
`MI_EBS_KHA_Strata`, `MI_EBS_KHA_Fibre`, `MI_EBS_KHA_Amber`. The shell is eight stacked strata bands,
each a straight-sided prism sampled from an elliptical profile at its mid height — `REL-ART-029` forbids
organic smoothing, so a subdivided dome would breach the requirement. Sockets (5):
`Target_Anchor_Center`, `Unit_Emergence`, `Rally_Default`, `Matter_Dropoff`, `Adaptation_Crown`.
Collision: two UBX boxes on the shell only. Budgets: LOD0 1,554 and LOD1 634 against provisional
ceilings of 8,000 / 3,500. Amber is 10.5% of surface area against the `REL-ART-029` cap of 15%.

## 5. States

| State | Read | Authority |
|---|---|---|
| working | five lit hollows, amber growth seams between the bands, seven spires with lit veins | canon: the interior glow breathes slowly |
| damaged | one spire's vein dark and a fracture stepping down the front flank; nothing else changes | canon: a spire dark, strata cracked |
| destroyed | the upper bands fall inward and settle; hollows, cleft, seams and spires gone; a rubble ring remains | canon: ceramic collapse inward |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-BLD-001/` (`receipt.json`, `review/`
assemblies for the three states and LOD1, `scenes/`, `renders/`). Renders: orthographic
front/right/rear/top with a 180 cm reference figure, a three-quarter matching the candidate's framing,
a cleft-flank view and a tactical gameplay pass, for each state. Six comparison sheets in
`renders/concept-compare/`. Checks: 18 structural tests in [test_hearth_build.py](test_hearth_build.py)
— the contract inventory, the shell stepping in band by band, breadth over shell height, the three
traced proportions, footprint containment and ground contact in every state, the glow sitting behind
the jamb faces, the cleft being wider and lower than an arch and on the opposite flank from the
hollows, amber being the only emissive and under the Kharuun cap, each state differing where canon
says, socket names and sides, collision confined to the shell, the provisional budget and determinism —
all passing. Not yet: textures, in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-KHA-BLD-001"
python3 build_hearth.py --evidence-dir "<evidence root>/EBS-KHA-BLD-001"
python3 build_hearth.py --evidence-dir "<evidence root>/EBS-KHA-BLD-001" --check
python3 test_hearth_build.py
for s in working damaged destroyed lod1; do \
  python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-KHA-BLD-001/scenes/$s.json" \
    --out "<evidence root>/EBS-KHA-BLD-001/renders/$s"; done
```

## 8. Decisions and open items

1. **The asset card is provisional and lives here, not in the requirements.** `Docs/Requirements.md`
   §18.2 still carries eight asset cards, every one of them Meridian. On 2026-09-07 the owner confirmed
   8,000 / 3,500 as this asset's provisional ceilings and directed that a dedicated Kharuun card be
   authored; `REL-BLD-016.KA.HEARTH.ASSET` is in [../kharuun-asset-cards.json](../kharuun-asset-cards.json),
   rendered to [../kharuun-asset-cards.md](../kharuun-asset-cards.md). The owner was explicit that these
   match the preparation pipeline's selected limits and are **not existing authoritative per-asset
   requirements**. `acceptance.technical` is therefore **PENDING**, not compliant: it waits on the card
   being incorporated into the authoritative requirements and its checks passing.
2. **The dome is 880 cm across, not the full 1,000.** The thresholds, the cleft apron and the delivered
   matter all sit outside the shell and all have to stay inside the placement envelope, which is the
   same rule the owner confirmed for the Array Foundry on 2026-09-07. The shell was sized so the whole
   asset, protrusions included, reaches 495.6 cm from centre against the 500 cm limit.
3. **The shell is banded prisms, not a subdivided dome.** A first pass sampled the profile at each
   band's top, which made every band a tall cylinder and the stack read as a stepped cake. Sampling at
   the band's mid height reads as a dome while staying faceted, which is what `REL-ART-029` requires.
4. **The hollows are jambs, a lintel and an arch head with the glow set behind them.** A first pass
   used one solid frame block that enclosed the glow panel, so the hollows read as lit plates stuck on
   the wall. A test now asserts the glow's centre sits at a smaller radius than the jamb's.
5. **The damage read is weak at gameplay distance.** One dark spire vein and one fracture block are
   legible up close and hard to see in the tactical pass. That is a blockout limitation to fix with
   texture and a larger fracture, not a contract change; it is recorded here rather than left implied.
6. **Static mesh, with the articulated-component requirement still pending.** Canon's only motion is a
   glow breath and a cleft settle, neither of which is geometry. Per the owner's Anchor ruling of
   2026-09-07 that is acceptable for a blockout but does not close the requirement, which stays
   recorded in the manifest under `pending_requirements`.
7. Open: textures (2048² packed PBR with micro-noise normals per `REL-ART-029`), in-engine capture,
   gate reviews, owner acceptance.

> **OWNER-QUESTION A — RESOLVED 2026-09-07.** Asked: confirm the provisional ceilings, give different
> ones, or direct that a Kharuun card be authored. Owner ruling: 8,000 / 3,500 confirmed as this asset's
> provisional ceilings, and a dedicated Kharuun card authored — together with cards for every remaining
> Kharuun package, the Tender keeping its separately selected 4,500 / 1,800. The cards are in
> [../kharuun-asset-cards.md](../kharuun-asset-cards.md). They are production contracts, not authoritative
> requirements: final technical acceptance stays pending until they are incorporated and their checks pass.
