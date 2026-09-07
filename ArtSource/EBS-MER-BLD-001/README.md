---
title: EBS-MER-BLD-001 Anchor — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-MC-ANCHOR
production_asset_id: EBS-MER-BLD-001
production_maturity: BLOCKOUT
revision: ebs-mer-bld-001-concept-v1
canon_status: CANDIDATE (two REWORK inputs reconciled by the anchor-candidate sheet; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-BLD-001 Anchor — production source

Single authoritative source record for the Meridian Compact headquarters. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); production-lane maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-MC-ANCHOR` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept inputs | `EBS-CON-MER-BLD-001` REWORK (radial network-root organisation, delivery approaches, a field station rather than a monumental reactor spire) and `EBS-CON-MER-BLD-005` REWORK (repairable civic shell, ramp and status bands; lower the bunker dome; separate worker intake from exit) |
| Selected candidate | `…/concept-discovery-20260906/anchor-review/anchor-candidate.png` (main three-quarter, TOP VIEW, DAMAGED STATE) |
| Canon | `SPEC-BLD-015.MC.ANCHOR`, Bible line 513: a squat, wide ceramic drum on a charcoal plinth with a tall central mast, three visible worker bays at ground level, a Matter intake chute, thick conduit roots running out to the network, cyan bands ringing the drum. Working: bands lit, mast steady. Damaged: a band dark, panel plates visibly cracked |
| Gameplay record | `Content/Data/Source/buildings.json` `mc_anchor`: headquarters drop-off, 1,400 HP, 800 cm sight, 400 construction ticks, +12 logistics capacity, footprint 5×5 tiles |
| Asset card | `REL-BLD-015.MC.CORE`: LOD0 ≤12,000 / LOD1 ≤4,500, 4096² stack, cyan under the 15% area rule, 4-bone rig for vanes and data-grid extensions, panel deformation below 30% health |
| Form language | `REL-ART-006` (engineered load paths: orthogonal frames, heavy plates, structural rails, exposed conduits, status bands); `REL-ART-028` (Meridian roster: LOD0 ≤8,000 / LOD1 ≤3,500) |
| Production ID | `EBS-MER-BLD-001`, planned `SM_EBS_MER_BLD_001` under `/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_001/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Anchor is the Compact's headquarters: the root of the power network, the source of
workers and the Matter drop-off. Losing it loses the match, so it must read as the thing worth
defending without reading as a fortress. What must be absent: a monumental reactor spire, a bunker
dome, and any suggestion that the ramps or conduit roots block movement.

**DETAIL.** Large scale: a squat, wide, many-sided pale ceramic drum in two tiers on a charcoal
plinth, ringed by cyan bands, with a slim sensor mast on the apex. Medium: three numbered worker bays
with lit interiors and their own ramps on the front arc, a separate Matter intake chute to their right,
eight short conduit roots reaching terminal nodes at the footprint edge, pilasters between the panel
bays. Fine (close camera): panel seams, hazard striping on the ramp edges, bay numerals — all texture.

**ACTION.** Working: bands and bay interiors lit, mast steady. Damaged: the lower band goes dark and a
panel section of the upper tier is gone, its frame ribs exposed. Destroyed (engineered collapse, drum
sagging on the plinth) is canon but is NOT built at this stage. Sound intent from canon (a deep steady
transformer tone, a chute clatter on delivery) is specified, not produced. Boundaries: passability,
construction and production are simulation-owned; nothing here changes them.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 5×5 tiles = 1,000 cm square; every part stays inside it | `buildings.json` `mc_anchor.footprint_cells`, asserted by test |
| Drum | 720 cm across (0.72 of the footprint), mass 432 cm tall → 0.60 height/width | measured on the candidate: a big drum with stub arms, and canon's "squat, wide" |
| Mast | tip at 660 cm; the mast is 0.345 of the total height | measured on the candidate's main view silhouette (0.345) |
| Conduit roots | stubs from r 396 to nodes at r 468 (outer 498), inside the 500 cm half-width | the footprint is the outer bound; the candidate draws short arms |
| Units / axes / pivot | cm; +X forward (the bay arc), +Y right, +Z up; pivot at ground-contact centre | contract `import_policy` |

## 4. Geometry (revision `ebs-mer-bld-001-concept-v1`)

Generator: [build_anchor.py](build_anchor.py) on the mesh kit. One mesh `SM_EBS_MER_BLD_001`; no
sub-objects at this stage. Two 16-sided tiers (8-sided at LOD1) with pilasters, a two-step charcoal
plinth, three bay portals standing proud of the wall with lintels, jambs, lit interiors and ramps, the
chute housing with its angled throat and lit slot, eight paired conduit tubes with lamp-topped nodes,
and a four-column mast lattice with rungs, a sensor head and two whips.

Slots: `MI_EBS_MER_CeramicCivic`, `MI_EBS_MER_CompactFrame`, `MI_EBS_MER_StatusCyan`.
Sockets (14): `Target_Anchor_Center`, `Worker_Bay_01..03`, `Matter_Intake_Chute`,
`Conduit_Node_01..08`, `Mast_Top`. Collision: two UBX boxes on the plinth and the drum mass only, so
the ramps, conduit stubs and nodes never read as blockers. Budgets: LOD0 1,660 ≤ 3,500 (LOD1 932),
against the tighter `REL-ART-028` bound rather than the card's 12,000. Exact numbers and hashes:
[build-manifest.json](build-manifest.json).

## 5. States

| State | Read | Authority |
|---|---|---|
| working | both bands and the bay interiors lit, mast steady | operational |
| damaged | the lower band goes dark and one upper-tier panel section is missing, frame ribs exposed | below the card's 30% health threshold |
| destroyed | NOT BUILT at this stage; canon calls for an engineered collapse with the drum sagging on the plinth | — |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-MER-BLD-001/` (`receipt.json`, `review/`
assemblies for both states and LOD1, `scenes/`, `renders/`). Renders: orthographic front/right/rear/top
with a 180 cm reference figure, and tactical views at the game's framing (default, gameplay, near and a
monochrome pass) for each state. Checks: 19 structural tests in
[test_anchor_build.py](test_anchor_build.py) — contract inventory, footprint containment at both LODs
and in the damaged state, nothing below ground, the squat-drum and mast-fraction ratios, bay portals
reading from outside the wall, each bay's ramp reaching the ground, the chute separate from the bays,
socket names and placements, collision covering the mass only, budgets, the damaged-state difference,
and determinism — all passing. Not yet: comparison sheets against the candidate crop, textures,
in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-BLD-001"
python3 build_anchor.py --evidence-dir "<evidence root>/EBS-MER-BLD-001"
python3 build_anchor.py --evidence-dir "<evidence root>/EBS-MER-BLD-001" --check
python3 test_anchor_build.py
for s in working damaged lod1; do python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-MER-BLD-001/scenes/$s.json" --out "<evidence root>/EBS-MER-BLD-001/renders/$s"; done
```

## 8. Decisions and open items

1. **Drum diameter, not the footprint.** `concept-fidelity.md` said "D = 1,000 cm drum diameter on the
   5×5 tile footprint". Both readings cannot hold: the candidate's conduit arms and bay ramps extend
   past the drum, and everything must stay inside the 1,000 cm square. The drum is built at 720 cm
   (0.72 of the footprint) and the footprint is the outer bound. The fidelity file is amended in place.
2. **Proportions came from a pixel trace, not an estimate.** A first pass built a 600 cm drum with long
   arms and a 478 cm mast; the candidate's silhouette measures a drum that dominates the width with
   stub arms and a mast at 0.345 of the total height, and the constants now follow that.
3. **Height/width divergence, recorded.** The candidate's whole silhouette measures 0.97 height over
   width; this build measures 0.70. The width is pinned by the 5×5 footprint and canon fixes the drum
   as "squat, wide", so matching 0.97 would need a drum nearly as tall as it is wide. Canon wins and
   the divergence is recorded rather than split. **OWNER-QUESTION A.**
4. **Budget bound.** `REL-BLD-015.MC.CORE` allows 12,000 / 4,500 for this structure while `REL-ART-028`
   bounds the whole Meridian roster at 8,000 / 3,500. The build uses the tighter pair so both hold; the
   conflict between the two records is reported, not resolved here. **OWNER-QUESTION B.**
5. **Destroyed state not built.** Canon calls for an engineered collapse; this stage ships working and
   damaged only.
6. **The card's 4-bone rig is not built.** `REL-BLD-015.MC.CORE` names a 4-bone rig for core exhaust
   vanes and data-grid extensions. This blockout is a static mesh; the vanes and extensions are not
   modelled. **OWNER-QUESTION C.**
7. Open: comparison sheets against the candidate crop, textures (4096² per the card), in-engine
   capture, gate reviews, owner acceptance.

> **OWNER-QUESTION A — silhouette proportion.** Canon's "squat, wide" drum and the candidate's 0.97
> height/width cannot both hold inside a 5×5 footprint. The build follows canon (0.70). Confirm, or the
> drum grows taller toward the candidate and the canon wording is amended.

> **OWNER-QUESTION B — which budget governs.** `REL-BLD-015.MC.CORE` (12,000 / 4,500) against
> `REL-ART-028` (8,000 / 3,500) for the same asset. The build uses the tighter pair.

> **OWNER-QUESTION C — the card's 4-bone rig.** Core exhaust vanes and data-grid extensions are named
> by the card but are not in the concept. Confirm the Anchor ships as a static mesh at this stage, or
> the vanes and extensions are designed and rigged.
