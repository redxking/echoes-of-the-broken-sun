---
title: EBS-KHA-BLD-001 Memory Hearth — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Memory Hearth

## Sources (read the images, not the notes about them)

| Role | File |
|---|---|
| Selected candidate — the production direction (WORKING, INTAKE DETAIL, DAMAGED) | `…/concept-discovery-20260906/memory-hearth-review/memory-hearth-candidate.png` |
| Concept B, KEEP — design identity (`EBS-CON-KHA-BLD-005`) | `…/site/assets/concepts/echoes-kharuun-structures.jpg`, left large dome, `[0, 0.12, 0.4, …]` |
| Concept A, REPLACE — retained history only (`EBS-CON-KHA-BLD-001`) | `…/site/assets/concepts/kharuun-structures.png` top-left `[0, 0, 0.5, 0.5]` |

Review decisions (verbatim intent): A — **REPLACE**. The armoured shrine-like headquarters is not the
production direction; retain the warm communal centre only. Its geometry is retained history and is
explicitly excluded from the target constraints. B — **KEEP** as design identity: the grown
mineral-organic dome with inhabited arched hollows, banded strata and a rooted adaptation crown. Canon
row `SPEC-BLD-016.KA.HEARTH` (Bible line 555) stays intact.

## Form and proportion (traced on the candidate's main WORKING view)

Scale basis: `buildings.json` `ka_memory_hearth.footprint_cells` = 5×5 tiles = 1,000 cm square. A
silhouette trace of the main view (dark-pixel row spans, figure 952 × 660 px) gives:

| Measured on the candidate | Value |
|---|---|
| dome base width | 952 px (the reference length) |
| shell height ÷ base width (crown of the shell, spires excluded) | **0.44** |
| total height ÷ base width (tallest spire tip) | **0.69** |
| spire cluster width ÷ base width | **0.55** |
| spires ÷ total height | **0.36** |

This confirms the canon phrase "breadth greater than shell height" numerically: the shell is less than
half as tall as it is wide, and everything above that is spire.

1. **A wide grown dome of banded strata.** The shell is a stack of horizontal strata bands, each band
   stepping in as it rises — faceted, with zero organic smoothing (`REL-ART-029`). Not a smooth
   hemisphere and not a cone.
2. **Several arched worker hollows at the base.** Five legible arched openings around the front arc,
   each with a walkable threshold. Warm amber interior light is visible *through* the arch; the arch
   itself is dark stone. They are the unit emergence points.
3. **A matter-intake cleft, distinct in silhouette from the hollows.** On the near right flank: a low
   receiving lip leading into a mineral-lined recess, wider and lower than an arch, with raw matter at
   the mouth. A player must not confuse it with a worker entrance — that is its whole job.
4. **A crown of rooted adaptation spires.** Seven tapered spires emerging *out of* the shell strata,
   not mounted on top of it: one tallest at the centre, the rest shorter and spread across 0.55 of the
   base width. Restrained and graceful, integrated into the shell.
5. **Warm amber from within.** Light in the hollows, in narrow growth seams up the shell, and faint at
   the spire veins. Amber is the only emissive and it reads as interior life, not as machinery.
6. **States.** Working: the interior glow breathes slowly. Damaged: one spire dark and the strata
   cracked. Destroyed: ceramic collapse inward.
7. **Palette.** Dark grey banded mineral-ceramic with warm amber within; matte. No lava, fire, smoke,
   molten cracks, or anything volcanic — the candidate prompt excludes them and so does this target.

## Rig and tracks

Canon specifies the Hearth's only motion as a slow interior glow breath and a cleft settle on delivery.
Neither is geometric articulation, so this is a **static mesh**. Per the owner's Anchor ruling of
2026-09-07, that is acceptable for a blockout but is **not closure of the pipeline's articulated-component
requirement**; the requirement stays pending and is recorded in the manifest rather than silently waived.

## Budgets and rules that bound the concept

**There is no Kharuun asset card.** `Docs/Requirements.md` §18.2 carries Meridian cards only, so no
`REL-BLD-016.KA.HEARTH.ASSET` exists to govern this asset. What does bind:

- `REL-ART-029` (Kharuun Roster Grown Mineral Architecture): faceted basalt silhouette with zero organic
  smoothing, packed 2048² PBR stacks with micro-noise normals, amber emissive clamped to ≤15.0% of mesh
  surface area.
- `REL-BLD-016.KA.HEARTH`: 1,300 health, 800 cm sight, +12 logistics, 5×5 footprint, produces Tenders.
- Nanite off; cm, +X forward, +Y right, +Z up; pivot at the ground-contact centre.

Triangle ceilings are **provisional and mine**, pending an owner ruling: LOD0 ≤8,000 / LOD1 ≤3,500, the
largest structure budget already in use in this pipeline. Raised as **OWNER-QUESTION A**.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Three-quarter working view: wide banded dome, five arched hollows, spire crown, amber within.
- [ ] Right flank: the intake cleft reads as a cleft, not as another arch.
- [ ] Front orthographic: shell height 0.44 of the base width, total 0.69, spires 0.36 of the height.
- [ ] Damaged view: one spire dark, the strata cracked, everything else unchanged.
- [ ] Top view: the dome inside the 5×5 square, spires spread across 0.55 of the base width.
- [ ] Tactical framing: the hollows and the cleft are both legible at gameplay distance.
