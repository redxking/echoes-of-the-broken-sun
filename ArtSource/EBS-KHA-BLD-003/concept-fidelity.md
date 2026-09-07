---
title: EBS-KHA-BLD-003 Growth Basin — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Growth Basin

## Sources (read the images, not the notes about them)

| Role | File |
|---|---|
| Selected candidate — the production direction (GROWING, RESTING, MOLT NICHE) | `…/concept-discovery-20260906/growth-basin-review/growth-basin-candidate.png` |
| State contrast | `…/growth-basin-review/growth-basin-state-contrast.png` |

Canon row `SPEC-BLD-016` Growth Basin: a shallow bowl of grown strata with an amber-lit matrix pool at
its center, ringed by visible molt niches. The pool says *grows warforms*; niches say *adaptation
choices*. Growing shows the pool brightening; a molting warform stands in a niche and visibly changes.
Sounds: liquid mineral resonance, and a crack-and-settle on molt completion.

## Form and proportion (traced on the candidate's GROWING view)

Scale basis: `buildings.json` `ka_growth_basin.footprint_cells` = 4×4 tiles = 800 cm square.

| Measured on the candidate | Value |
|---|---|
| bowl diameter | 843 px (the reference length) |
| pool diameter ÷ bowl diameter | **0.75** |
| plan shape | circular; the view's 0.53 height-to-width is isometric foreshortening, not a proportion |

1. **A shallow bowl of grown strata, circular in plan.** Low and wide. The rim is a stack of stone
   courses, faceted, with zero organic smoothing (`REL-ART-029`).
2. **An amber matrix pool at the centre**, three quarters of the bowl across. It is the asset's
   identity and is allowed to dominate the amber budget.
3. **A ring of OPEN curved molt niches around the rim.** Each niche is a curved outer wall standing
   proud of the rim with an open alcove floor inside it. The candidate's MOLT NICHE panel shows a
   warform standing in an open alcove — nothing closes over it.
4. **A per-niche amber floor.** With open alcoves, the only way occupancy can read is the niche floor
   lighting. Dark when free, lit when a warform is molting in it.
5. **States.** Idle, growing, molting, damaged, destroyed. Growing is the pool brightening, which is a
   material parameter and not geometry.
6. **Palette.** Dark coursed strata with warm amber in the pool and the occupied niche floors; matte.
   No machinery around the rim, no vats, no pipework, nothing Meridian.

## The niche count is not specified anywhere

The provisional card requires the niche count to match the authoritative adaptation contract. **No such
number exists.** `buildings.json` `ka_growth_basin.adaptation` carries `site_radius_cm`, `molt_ticks`,
`dawn_cost` and the stat deltas, and nothing that says how many warforms may molt at once. Six niches
are counted off the candidate's GROWING view and used here. This is recorded as **OWNER-QUESTION A**
and is not a gameplay claim.

## Rig and tracks

Per the provisional card: `root` plus one bone per niche. The niches do **not** close — that was wrong
in the card's first draft and is corrected there. Each niche bone carries canon's crack-and-settle at
molt completion: a small drop and recovery of the niche stone.

Tracks: `idle`, `growing`, `molt_start`, `molt_complete`, `restore`. Molt takes 80 ticks = 4.0 s at the
project's 20 ticks per second. `growing` holds the niche bones at rest, because in that state the change
is the pool's emissive; the clip exists so the runtime has a named state to play, and the manifest says
so rather than implying the geometry moves.

## Budgets and rules that bound the concept

Provisional card `REL-BLD-016.KA.BASIN.ASSET`: LOD0 ≤8,000 / LOD1 ≤3,500 for the complete assembly
including every niche. 2048² packed PBR. Amber ≤15% of surface area as a **ceiling, not a target**.
Nanite off; cm, +X forward, +Y right, +Z up; pivot at the bowl's ground-contact centre. Everything inside
the 4×4 footprint.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Growing three-quarter: shallow coursed bowl, wide amber pool, ring of open niches.
- [ ] Resting: the same bowl with the pool dimmed and every niche floor dark.
- [ ] Molting: only the occupied niches' floors are lit, per niche.
- [ ] Niche detail: an open curved alcove big enough for a warform, nothing closing over it.
- [ ] Top view: the bowl inside the 4×4 square, pool 0.75 of the bowl across, six niches evenly spaced.
- [ ] Tactical framing: pool and niches both legible at gameplay distance.
