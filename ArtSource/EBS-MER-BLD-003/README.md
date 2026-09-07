---
title: EBS-MER-BLD-003 Array Foundry — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-MC-ARRAY-FOUNDRY
production_asset_id: EBS-MER-BLD-003
production_maturity: BLOCKOUT
revision: ebs-mer-bld-003-concept-v1
canon_status: CANDIDATE (a KEEP and a REWORK input reconciled by the array-foundry candidate; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-BLD-003 Array Foundry — production source

Single authoritative source record for the Compact's production centre. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-MC-ARRAY-FOUNDRY` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept inputs | `EBS-CON-MER-BLD-003` **KEEP** (retain the long open fabrication hall, structural rails and the visible work-to-output sequence) and `EBS-CON-MER-BLD-007` **REWORK** (keep overhead gantries and service access, reorganise the stacked workshop into the Bible's long intake/work/output hall with a separate readable roof research gantry, reduce sparks and tiny machinery) |
| Selected candidate | `…/concept-discovery-20260906/array-foundry-review/array-foundry-aligned-output-candidate.png` (PRODUCING, RESEARCHING, INTERRUPTED) |
| Canon | `SPEC-BLD-015.MC.FOUNDRY`, Bible line 515: a long rectangular hall with an intake ramp at one end, an open fabrication bay in the middle where frames are visibly assembled on a rail, an output door at the far end, and a research gantry with instruments on the roof. Producing: the rail carries a half-built frame toward the door. Researching: the roof gantry lights and the rail stops. Interrupted: gantry dims, no refund animation |
| Gameplay record | `buildings.json` `mc_array_foundry`: production, 760 HP, 500 cm sight, 160 construction ticks, footprint 4×4 tiles; hosts research |
| Asset card | `REL-BLD-015.MC.FOUNDRY`: LOD0 ≤8,000 / LOD1 ≤3,400, 4×4 footprint, gantries and crane rails, 2048² grit/grease stack, operational states reveal interior assembly lights, exterior research progress band, matte ≥0.85 |
| Production ID | `EBS-MER-BLD-003`, planned `SM_EBS_MER_BLD_003` under `/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_003/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Array Foundry makes every Compact combat unit and hosts research. The player must read
what it is doing from where the activity is: intake at one end, work in the middle, output at the other,
research on the roof. What must be absent: a stacked workshop, spark clutter that hides the production
path, and any suggestion that the ramps block movement.

**DETAIL.** Large scale: one long pale hall on a charcoal plinth, open across its middle third. Medium:
an arched intake hood and ramp at the −X end, two gantry rails carrying a frame carriage over the open
bay, a framed output door and ramp at the +X end, a research gantry on the roof over the output.
Fine: pilasters and conduit runs along the flanks, hazard striping on the ramps, grit and grease — texture.

**ACTION.** Producing: the carriage carries a half-built frame toward the door with its assembly seam
lit. Researching: the roof instruments light and the rail is empty. Interrupted: the gantry goes dark
and a partial frame sits stationary — no refund animation. Sound (canon): a rhythmic fabrication clank
and a rising research tone — specified, not produced.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 4×4 tiles = 800 cm square; every part inside it | `buildings.json`, asserted by test |
| Hall | 520 × 260 cm in plan (2:1), walls to 200 cm, roof deck 226 cm | the footprint has to hold a ramp at BOTH ends (see §8.1) |
| Open bay | the middle third of the roof, rails at 240 cm | canon's "open fabrication bay … on a rail" |
| Research gantry | platform over the output end, instruments to 358 cm (the asset's height) | canon's roof gantry |
| Units / axes / pivot | cm; +X runs intake → output, +Y right, +Z up; pivot at the hall's ground-contact centre | contract `import_policy` |

## 4. Geometry (revision `ebs-mer-bld-003-concept-v1`)

Generator: [build_foundry.py](build_foundry.py). One mesh `SM_EBS_MER_BLD_003`; no sub-objects at this
stage. Slots: `MI_EBS_MER_CeramicCivic`, `MI_EBS_MER_CompactFrame`, `MI_EBS_MER_StatusCyan`. Sockets (6):
`Target_Anchor_Center`, `Intake_Mouth` (facing −X, away from the output), `Output_Door`, `Rail_Start`,
`Rail_End`, `Research_Gantry`. Collision: two UBX boxes on the hall and the intake hood only, so neither
ramp reads as a blocker. Budgets: LOD0 648 and LOD1 480 against the card's 8,000 / 3,400, recorded and
tested as whole-asset sums per the owner's ruling of 2026-09-07.

## 5. States

| State | Read | Authority |
|---|---|---|
| producing | the carriage sits toward the output end with the frame's assembly seam lit; the roof gantry is dark | a unit is queued |
| researching | no carriage on the rail; the gantry instruments and the exterior progress band light | research occupies the slot |
| interrupted | the frame sits stationary toward the intake end, its seam unlit, the gantry dark | production or research interrupted; no refund animation |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-MER-BLD-003/` (`receipt.json`, `review/`
assemblies for the three states and LOD1, `scenes/`, `renders/`). Renders: orthographic
front/right/rear/top with a 180 cm reference figure, a close three-quarter matching the candidate's
framing, tactical gameplay and a monochrome pass, for each state. Six comparison sheets in
`renders/concept-compare/`. Checks: 15 structural tests in [test_foundry_build.py](test_foundry_build.py)
— contract inventory, footprint containment in every state and LOD, nothing below ground, intake and
output at opposite ends with their ramps beyond them, the hall's 2:1 plan, the middle bay open with
rails above the wall line, the three states differing where the concept says, the carriage moving toward
the door when producing, socket names and placement, collision clear of the ramps, the whole-asset
budget and determinism — all passing. Not yet: textures, in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-BLD-003"
python3 build_foundry.py --evidence-dir "<evidence root>/EBS-MER-BLD-003"
python3 build_foundry.py --evidence-dir "<evidence root>/EBS-MER-BLD-003" --check
python3 test_foundry_build.py
for s in producing researching interrupted lod1; do python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-MER-BLD-003/scenes/$s.json" --out "<evidence root>/EBS-MER-BLD-003/renders/$s"; done
```

## 8. Decisions and open items

1. **The hall is 2:1, not the candidate's longer read, and the footprint is why.** The candidate draws a
   ramp at BOTH ends of a long hall. Inside the 4×4 square the length is shared: hall half (260) +
   intake hood (70) + intake ramp (60) = 390 ≤ 400. A longer hall would push a ramp outside the
   footprint. The proportion is a consequence of the rule, not a design preference, and it is the
   largest divergence from the candidate. **Owner ruling 2026-09-07: the 2:1 hall is confirmed.** Both
   ramps stay inside the placement envelope; they are not to be extended onto walkable ground to chase
   the candidate's apparent 3:1. The long-axis production flow, the aligned entrance and exit, and the
   straight internal rail are preserved, and this is recorded as a footprint-driven adaptation.
   Navigation and placement rules are unchanged.
2. **The intake mouth and the output door are dark recesses with lit edges**, not glowing panels. A
   first pass made both large emissive slabs, which read as screens rather than doorways.
3. **The three states are geometry plus slot assignment**, not material promises: the carriage exists or
   not, its seam is lit or absent, and the gantry lamps sit in the status slot only while researching.
   A test asserts each difference so a state cannot silently collapse into another.
4. **Ramps carry no collision.** Both UBX boxes stop short of them, so the approaches never read as
   blockers (`REL-ART-016` / `REL-ART-030`).
5. Open: textures (2048² per the card, including the grit and grease layers and the hazard striping),
   the interior assembly lights the card calls for, in-engine capture, gate reviews, owner acceptance.

> **OWNER-QUESTION A — RESOLVED 2026-09-07.** Asked: confirm the 2:1 hall, or allow the ramps past the
> footprint so the hall could lengthen toward the candidate's 3:1. Owner ruling: the 2:1 hall is
> confirmed and both ramps stay inside the placement envelope. The proportion is recorded as a
> footprint-driven adaptation. This is production direction, not acceptance of the finished asset.
