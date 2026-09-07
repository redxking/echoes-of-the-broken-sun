---
title: EBS-KHA-UNT-003 Cairnback — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Cairnback

## Silhouette first

The owner directed on 2026-09-07 that this build **establish a contrasting silhouette against the
Riftstalker before it is detailed**, and that the two remain distinguishable in **monochrome at
tactical distance**, where colour, emissive and texture count for nothing.

The candidate makes that easy, because the two animals are built the opposite way round. Both are
Kharuun quadrupeds of roughly the same length-to-height ratio, and that is where the similarity stops:

| Measure | Riftstalker (built) | Cairnback (target) |
|---|---|---|
| leg share of standing height | 0.69 | **≤ 0.24** |
| body width ÷ body length | 0.26 | **≥ 0.70** |
| dominant mass | four long legs | one domed shell |
| head | a forward prow ahead of the front feet | low, tucked, behind the shell's front edge |
| gait | fast diagonal, 410 cm/s | slow and heavy, 270 cm/s |

Those first two numbers are the separation, and both are asserted by test. A monochrome silhouette
comparison against the Riftstalker's rendered baseline is produced as evidence rather than asserted.

## Sources (read the images, not the notes about them)

| Role | File |
|---|---|
| Selected candidate (ASSAULT SCREEN, MINERAL COVER, STRATA DAMAGE) | `…/concept-discovery-20260906/cairnback-review/cairnback-candidate.png` |
| Concept input | `EBS-CON-KHA-UNT-003` |

Canon row `SPEC-UNIT-007`: a broad, low assault warform whose back is a slab of layered heat-holding
strata like a vaultback's; thick forelimbs; head low and protected. The strata back says *absorbs fire,
becomes cover*. Creating mineral cover is a heave that leaves a grown barrier behind it. Damage chips
strata; destruction is a ceramic slump.

## Form and proportion (traced on the candidate's ASSAULT SCREEN view)

| Measured on the candidate | Value |
|---|---|
| body length | 772 px |
| standing height | 534 px |
| length ÷ height | **1.45** |
| ground to the shell's lower edge (leg clearance) | ~105 px = **0.20 of the height** |

Scale: 228 cm standing and 330 cm long — larger than the Riftstalker's 201 × 295, as a population-3
assault unit against a population-2 skirmisher.

1. **A domed shell of layered strata courses**, stacked in visible rings that overhang the body. It is
   most of the animal's height and nearly all of its visual mass.
2. **Four short thick legs**, barely clearing the ground, set well inside the shell's overhang.
3. **A low protected head**, tucked under the shell's front edge — never ahead of it, which is the
   opposite of the Riftstalker's prow.
4. **Amber only in the strata seams.** The back is armour, not a lamp.
5. **States.** Intact and chipped, per canon's "damage chips strata".
6. **The mineral cover it creates is a SEPARATE asset** with its own budget, per the card. It is not
   built here and is not counted in this asset's triangles.

## Rig and tracks

Per the provisional card `REL-FAC-025.KA.CAIRNBACK.ASSET`: a heavy low-slung plan with four thick limb
chains, a short protected neck, and a back-slab chain that flexes on the heave — 22 bones, weighted
toward the limbs and the back rather than the extremities. That weighting is the inverse of the
Resonant's and deliberately unlike the Riftstalker's long-limbed plan.

Tracks: `idle`, `move` (heavy stone footfalls), `attack`, `heave` (leaves a grown barrier behind it),
`damage_chip`, `death` (a ceramic slump).

## Budgets and rules that bound the concept

Provisional card: LOD0 ≤8,000 / LOD1 ≤3,500 for the complete assembly, the mineral cover excluded as a
separate asset. 2048² packed PBR. Amber ≤15% of surface area as a **ceiling, not a target**. Nanite
off; cm, +X forward, +Y right, +Z up; pivot at the ground-contact centre between the feet. Triangle
counts below the ceiling are headroom, not sufficiency (owner ruling 2026-09-07).

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Side view: length 1.45 of the height, leg clearance 0.20 of it, shell dominating.
- [ ] Three-quarter: layered shell courses overhanging four short thick legs.
- [ ] Head view: tucked behind the shell's front edge, not ahead of it.
- [ ] Chipped state: strata visibly broken, per canon.
- [ ] Heave pose: the cast origin agrees with where a barrier would appear.
- [ ] **Monochrome tactical pass beside the Riftstalker's baseline**: unmistakably two animals.
