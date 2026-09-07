---
title: EBS-HOL-BLD-002 Interval Loom — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Interval Loom

## Traced before the card was written

Order followed, per the owner's ruling of 2026-09-07: trace the candidate, author
`REL-BLD-017.HC.INTERVAL.ASSET` from the trace, write this target, then build.

## Sources

| Role | File |
|---|---|
| Selected candidate (SUPPLIED, UPKEEP TICK, INSOLVENT) | `…/concept-discovery-20260906/interval-loom-review/interval-loom-candidate.png` |

Canon row `SPEC-BLD-017.HC.INTERVAL`: a supply node. 400 health, 600 cm sight, +6 logistics, 2×2
footprint, and a coherence charge of 5 Dawn every 600 ticks, reduced to 4 inside a Phase Anchor field.

## Form (traced on the candidate's SUPPLIED view)

| Measured | Value |
|---|---|
| overall figure | 849 × 550 px |
| bounding height ÷ width | 0.648 |
| arches | two, crossing |
| feet | four flat plates, one per arch end |
| drop-off pad | one, separate from the arches, on the ground |

**The 0.648 is an isometric bounding box, not an arch proportion**, and is recorded as such rather than
used as one. The arch's own rise against its span is a design choice from the drawing's proportions:
the candidate's arches are broad and low, so the build uses 0.64.

1. **Two ribbon arches crossing diagonally** over a bare footprint. No walls and no roof — the Loom is
   a frame, not a building.
2. **Four flat foot plates**, one at each arch end, near the corners of the square.
3. **A flat drop-off pad** on the ground beside the arches with delivered matter on it.
4. **Magenta fracture edge lines** along both borders of each arch, and nowhere else.
5. **Three power states, which are the asset's whole job.** Supplied: edges lit. Upkeep tick: a surge
   across the crowns as the 600-tick charge falls due. Insolvent: every edge dark.
6. **Palette.** Grey vitrified stone with magenta edges. No Kharuun strata, no Meridian plates or
   conduits, no amber, no cyan.

## Rig and tracks

Root plus one bone per arch, so the two drift independently — the faction's reality-bleed device, not a
canon motion clause. Tracks: `supplied_idle`, `upkeep`, `restore`.

## Budgets and rules

Provisional card: LOD0 ≤3,000 / LOD1 ≤1,200. 2048² packed PBR. Magenta ≤12% of surface area as a
**ceiling**. Nanite off; cm, +X forward, +Y right, +Z up; pivot at the ground-contact centre between the
feet. Everything inside the 2×2 footprint.

## Fidelity checks

- [ ] Three-quarter: two crossed arches, four feet, pad with matter, edge lines lit.
- [ ] The three power states side by side: supplied, ticking, insolvent.
- [ ] Top view: everything inside the 2×2 square.
- [ ] Tactical framing: solvency is readable at gameplay distance.
