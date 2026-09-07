---
title: EBS-MER-BLD-001 Anchor — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Anchor

## Sources (read the images, not the notes about them)

| Role | File | sha256 |
|---|---|---|
| Selected candidate — the production direction (main three-quarter, TOP VIEW, DAMAGED STATE) | `…/Evidence/concept-discovery-20260906/anchor-review/anchor-candidate.png` | (recorded in the package README §1 from the generation record) |
| Concept A, REWORK input (`EBS-CON-MER-BLD-001`) | `…/Project/site/assets/concepts/meridian-structures.png` top-left `[0, 0, 0.5, 0.5]` | `5b64820bbd2bbc28…` |
| Concept B, REWORK input (`EBS-CON-MER-BLD-005`) | `…/Project/site/assets/concepts/echoes-meridian-structures.jpg` top-left `[0, 0, 0.5, 0.5]` ("ANCHOR [Command Headquarters]") | `b6396728d4fd2a49…` |

Review decisions (verbatim intent): A — retain radial network-root organization and delivery approaches,
reduce the monumental reactor-spire emphasis; make a field station with serviceable rails, redundant
conduits, work entrances and a modest command/sensor core. B — use the repairable civic shell, ramp and
status bands as the stronger architectural direction; lower the bunker-like dome, expose useful load
paths and separate worker intake from exit; combine its service clarity with A's network-root
organization; do not adopt its printed scale or doorway proportions as approved dimensions.
Canon row `SPEC-BLD-015.MC.ANCHOR` (Bible line 513) stays intact: a squat wide ceramic drum on a
charcoal plinth, tall central mast, three visible worker bays at ground level, a Matter intake chute,
thick conduit roots to the network, cyan bands ringing the drum.

## Form and proportion (amended 2026-09-07 from a pixel trace of the candidate; the 5×5 footprint is the outer bound)

Scale basis: `SPEC-BLD-015.MC.ANCHOR` fixes a 5×5 tile footprint = 1,000 cm across at the 200 cm
presentation tile. ~~The drum fills it: D = 1,000 cm, drum height ≈ 0.34 D (340 cm) to the upper rim,
mast top ≈ 0.85 D (850 cm).~~ **[amended 2026-09-07]** The drum cannot fill the footprint AND leave the
ramps and conduit arms outside it, and the candidate draws both. Measured on the candidate's main view
(silhouette traced against the paper, title block excluded): the drum dominates the width with stub
arms, and the mast above the mass is **0.345** of the total silhouette height. Built to that:
**drum 720 cm across (0.72 F), mass 432 cm tall (0.60 height/width — squat and wide per canon), mast
tip 660 cm**, conduit nodes at r 468 (outer 498) inside the 500 cm half-width. Recorded divergence: the
candidate's whole silhouette measures 0.97 height/width against this build's 0.70, because the width is
pinned by the footprint while canon fixes the drum as squat (README §8.3, OWNER-QUESTION A).

1. **Squat ceramic drum.** A wide, low, many-sided ceramic drum (the candidate reads as a 16-sided
   rotunda) rising in two tiers: a broad lower tier of vertical panel bays and a stepped upper tier that
   domes gently toward the mast collar. Pale ceramic panels over a charcoal frame; the dome is LOW, per
   review decision B ("lower the bunker-like dome").
2. **Charcoal plinth.** A low charcoal plinth ring under the drum, ≈ 0.04 D tall, with a chamfered skirt
   that the ramps and conduit arms spring from.
3. **Three worker bays.** THREE numbered bays at ground level (01, 02, 03) on the front arc, each a
   recessed doorway with a lit cyan interior and its own ramp running down to the ground. The bays are
   the production read; their numerals are texture, the recess and ramp are geometry. Worker intake is
   separated from exit (decision B): the Matter intake chute is its own opening, not a bay.
4. **Matter intake chute.** A separate low housing on the right front of the drum with a wide angled
   throat and a lit slot — visibly a delivery mouth, not a door. It is where the chute clatter happens.
5. **Conduit roots.** EIGHT thick conduit arms leaving the plinth radially, each running out to a small
   charcoal terminal node at the footprint edge; the arms are segmented pipe pairs with brass collars.
   This is the "network root" organization retained from decision A.
6. **Central mast.** ONE slim lattice mast on the drum's apex collar with a small sensor head, two whip
   antennas and a dish bracket — a modest command/sensor core, NOT a reactor spire (decision A).
7. **States.** Working: cyan bands around both tiers lit, mast steady, bay interiors lit. Damaged: one
   band dark, panel plates visibly cracked and one upper-tier panel section replaced by exposed dark
   frame ribs (as the DAMAGED STATE panel shows). Destroyed: engineered collapse, the drum sagging on
   the plinth (authored as a separate assembly, cosmetic debris ≤ 200 ticks).
8. **Palette.** Pale ceramic, charcoal frame, brass trim and yellow-black hazard striping on the ramp
   edges (texture); cyan only in the bands, bay interiors, chute slot and mast head.

## Budgets and rules that bound the concept (do not replace it)

`REL-BLD-015.MC.CORE`: LOD0 ≤ 12,000 tris; LOD1 ≤ 4,500; 4096² PBR stack; desaturated hull albedo;
cyan emissive clamped under 15% of mesh area; 4-bone rig for core exhaust vanes and data-grid extensions;
panel deformation states below 30% health. `REL-ART-006` (engineered load paths: orthogonal frames,
heavy machined plates, structural rails, exposed conduits, functional status bands). Nanite off; cm,
+X forward (the bay arc faces +X), +Y right, +Z up; pivot at ground-contact centre; UBX collision on the
drum and plinth only — ramps and conduit arms must not block the bay approaches. Record every forced
deviation in README §8 with the rule cited.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Three-quarter view: squat two-tier ceramic drum on a charcoal plinth, low dome, slim mast.
- [ ] Front arc: three numbered bays with ramps, plus the separate intake chute housing.
- [ ] Top view matches the candidate: radial symmetry, eight conduit arms to edge nodes, mast centred.
- [ ] Damaged state: one dark band, cracked plates, one panel section open to the frame.
- [ ] Tactical framing: reads as the network root and worker source at 5×5 tiles, not as a fortress.
