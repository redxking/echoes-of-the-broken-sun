---
title: EBS-HOL-BLD-001 Concordance — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Concordance

## Traced before the card was written

The owner directed on 2026-09-07 that each remaining concept be traced **before** its provisional card
is authored, after three Kharuun cards written the other way round described the wrong asset. This
target and the card `REL-BLD-017.HC.CONCORDANCE.ASSET` were both written from the trace below.

## Sources

| Role | File |
|---|---|
| Selected candidate (WORKING, RING / INTAKE, DAMAGED PAIR) | `…/concept-discovery-20260906/concordance-review/concordance-candidate.png` |

Canon row `SPEC-BLD-017.HC.CONCORDANCE`: the Choir Command Core and Matter drop-off. 1,250 health,
900 cm sight, +12 logistics, 5×5 footprint, produces Threadkeepers, and is exempt from the ordinary
non-Core coherence upkeep every other Choir structure pays.

## Form and proportion (traced on the candidate's WORKING view)

| Measured on the candidate | Value |
|---|---|
| ring across, disc included | 952 px (the reference length) |
| slab height ÷ ring diameter | **0.24** |
| slab pairs counted on the RING / INTAKE top view | **14** |

Scale: `buildings.json` `hc_concordance` 5×5 tiles = 1,000 cm square, so the disc is 940 cm across and
the slabs stand 226 cm.

1. **A low circular disc**, barely raised, carrying everything.
2. **Fourteen slab pairs around the ring.** Each pair is a main vitrified slab with a second slab
   offset 3 cm behind it. The pair reads as one object that has not settled on where it is — the
   reality-bleed device the Hollow Choir unit cards specify for that faction, adopted here as faction
   language rather than as a requirement any authoritative card places on a building.
3. **A ground intake apron** at the front of the ring, with delivered matter on it. This is where
   workers bring Matter, and it must read as that.
4. **Magenta fracture edging** on the slab borders, and nowhere else.
5. **A thread at the centre.** The candidate draws a faint vertical filament rising inside the ring. It
   is a light effect, not geometry, and this build provides only a socket for it.
6. **States.** Working, damaged with one cracked pair, destroyed with the ring broken.
7. **Palette.** Grey vitrified stone with magenta edges. **No Kharuun strata banding, no Meridian plate
   seams or conduits, no amber, no cyan.**

## Rig and tracks

Per the provisional card: static primary structure, with root plus one bone per slab pair so pairs can
drift independently. That drift is the faction's reality-bleed device, not a canon motion clause for
this building, and the manifest says so. The pipeline's articulated-component requirement stays pending.

Tracks: `working_idle`, `damaged`, `restore`.

## Budgets and rules

Provisional card: LOD0 ≤8,000 / LOD1 ≤3,500. 2048² packed PBR. Magenta ≤12% of surface area as a
**ceiling, not a target**, taken from the Hollow Choir unit cards. Counts under the ceiling are
headroom, not sufficiency. Nanite off; cm, +X forward, +Y right, +Z up; pivot at the disc's
ground-contact centre. Everything inside the 5×5 footprint.

## Fidelity checks

- [ ] Three-quarter: a ring of paired slabs on a low disc, intake apron at the front.
- [ ] Top view: fourteen pairs evenly spaced, everything inside the 5×5 square.
- [ ] Pair detail: the offset duplicate visibly separate from its parent slab.
- [ ] Damaged: one pair cracked, matching the candidate's DAMAGED PAIR panel.
- [ ] Tactical framing: the ring reads as a ring and the apron as an intake.
