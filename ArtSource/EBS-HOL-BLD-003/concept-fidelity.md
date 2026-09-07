---
title: EBS-HOL-BLD-003 Chorus Loom — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Chorus Loom

## Traced before the card was written

Order followed, per the owner's ruling of 2026-09-07: trace the candidate, author
`REL-BLD-017.HC.CHORUS.ASSET` from the trace, write this target, then build.

## Sources

| Role | File |
|---|---|
| Selected candidate (PRODUCING, RESEARCHING, WEAVING DETAIL) | `…/concept-discovery-20260906/chorus-loom-review/chorus-loom-candidate.png` |

Canon row `SPEC-BLD-017.HC.CHORUS`: the production centre. 680 health, 550 cm sight, 170 construction
ticks, 4×4 footprint. It trains every Choir mobile combat unit and hosts research, and carries a
coherence charge of 5 Dawn every 600 ticks, reduced to 4 inside a Phase Anchor field.

## Form (traced on the candidate's PRODUCING view, dark-pixel spans below luminance 110)

| Measured | Value |
|---|---|
| overall figure | 959 × 591 px |
| bounding height ÷ width | 0.616 |
| post slabs | two, 80 px and 81 px apparent width |
| left post height (uncontaminated by the beam) | 572 px |
| clear span between the posts' inner faces | 607 px |
| post centre-to-centre span | 686 px |
| centre span ÷ overall width | 0.716 |
| beam band | 13 px thick, spanning x 299 → ~730 |

**The 0.616 and the 80 px post width are isometric bounding measurements, not proportions.** The panel
is drawn in three-quarter isometric, so the overall height-to-width ratio mixes post height with the
ground plan, and each post's apparent width is its thickness plus its foreshortened depth. They are
recorded as measurements and not used as build ratios. The build's own choices — a 572 cm post centre
span across a 760 cm platform (0.753), and posts 34 cm thick by 128 cm deep — come from the drawing's
read of thin slabs standing well inboard of the platform edges.

1. **A low rectangular platform** filling the footprint. Not a building: no walls, no roof.
2. **Two upright slab posts**, one near each end of the platform, thin in the span direction and deep
   across it, with **magenta edge strips down their inner borders** and nowhere else on the stone.
3. **A warp of fine horizontal threads** strung between the posts across the full open span.
4. **A beam hovering above the warp, touching nothing.** In the candidate it sits at roughly post-crown
   height with clear air beneath it. The build lifts it 37 cm above the crowns so that it still reads as
   unsupported in an orthographic front view, where the isometric depth cue is gone. **Nothing braces
   it; adding a strut destroys the faction read.**
5. **The woven form at the centre of the warp is a light effect, not geometry.** The candidate draws it
   as a glowing polyhedral cage. The mesh provides the `Weave_Center` socket and nothing else.
6. **Three power states, which are the asset's job.** Producing: the warp is strung and lit and the
   weave centre occupied. Researching: the warp doubles and the hovering beam's underside lights.
   Insolvent: warp, strips and beam dark.
7. **Palette.** Grey vitrified stone with Magenta Fracture. No Kharuun strata, no Meridian plates or
   conduits, no amber, no cyan.

## Rig and tracks

Root, one bone per post, and one for the hovering beam so it can hang out of step with them — the
faction's reality-bleed device, **not a canon motion clause for this building**. Tracks:
`producing_idle`, `researching`, `restore`.

## Budgets and rules

Provisional card: LOD0 ≤6,000 / LOD1 ≤2,400. 2048² packed PBR. Magenta ≤12% of surface area as a
**ceiling**. Nanite off; cm, +X forward, +Y right, +Z up; pivot at the platform's ground-contact centre.
Everything inside the 4×4 footprint. Being under the ceiling is headroom, not sufficiency.

## Fidelity checks

- [ ] Three-quarter: two slab posts, the warp between them, the beam floating clear.
- [ ] Front orthographic: nothing touches the beam.
- [ ] Producing and researching side by side: the warp density and the beam differ in geometry.
- [ ] Insolvent beside both: every thread and strip dark.
- [ ] Top view: everything inside the 4×4 square.
- [ ] Tactical framing: producing, researching and insolvent are separable at gameplay distance.
