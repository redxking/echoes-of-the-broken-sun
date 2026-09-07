---
title: EBS-KHA-UNT-004 Resonant — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Resonant

## It is a quadruped, and my card said biped

The provisional card described a slender biped with 26 bones. The candidate's LISTENING and DETECTING
views both show **four long stilt legs**. The card was corrected against the concept, as the Riftstalker's
was. That is the third of my eight Kharuun cards to carry the wrong anatomy, all for the same reason:
I wrote them in one pass before tracing the candidates. The faction rules now say cards are written
after the trace, not before.

## Silhouette: separation from two quadrupeds, not one

This is the third Kharuun quadruped, so it must separate from both the others in monochrome at tactical
distance.

| Measure | Riftstalker | Cairnback | Resonant (target) |
|---|---|---|---|
| height ÷ max horizontal extent | 0.68 | 0.71 | **≥ 1.45** |
| dominant mass | four long legs | one domed shell | a dorsal fin array on stilts |

Neither of the others is taller than it is long. This one is, by half again, and that is the separation.
It is asserted by test and measured by [../tools/ebs_silhouette.py](../tools/ebs_silhouette.py) against
both.

## Sources

| Role | File |
|---|---|
| Selected candidate (LISTENING, DETECTING, SENSOR FINS) | `…/concept-discovery-20260906/resonant-review/resonant-candidate.png` |
| Concept input | `EBS-CON-KHA-UNT-004` |

Canon row `SPEC-UNIT-008`: a tall, thin scout with sensor-fins of translucent amber along the spine and
head, and a delicate frame. Fins and delicacy say *listens, does not fight*. Detecting shows the fins
brightening in sequence. Almost silent movement.

## Form and proportion (traced on the candidate's LISTENING view)

| Measured on the candidate | Value |
|---|---|
| standing height | 810 px |
| maximum horizontal extent | 506 px |
| **height ÷ extent** | **1.60** |
| body across the shoulders ÷ extent | ~0.55 |

Scale: 255 cm at the crown, the tallest Kharuun unit and the lightest — 85 health against the
Cairnback's 245. Body length 160 cm, so the traced ratio holds.

1. **Four long stilt legs**, thin and nearly straight, splaying outward to small feet.
2. **An arched spine** rising from the shoulders and curving forward to a small head carried high in
   `listening` and dropped low in `contact`.
3. **A dorsal fin array**: thirteen translucent amber fins along the spine, largest at mid-back and
   tapering fore and aft. Each is its own component so the runtime can light them in sequence.
4. **Delicacy is the read.** Limb radii stay small; nothing about the silhouette may promise a fight.
5. **States.** Passive with the fins dark, detecting with them lit.
6. **Palette.** Dark faceted plate with translucent amber fins; matte.

## Rig and tracks

Per the corrected card: a slender quadruped with a segmented spine carrying the fin array — 24 bones,
weighted toward the spine and neck rather than the limbs, the inverse of the Cairnback's weighting.

Tracks: `idle`, `move` (almost silent), `detect` (the spine lowers and sweeps toward the source while
the fins run their sequence), `attack`, `death`. The sequence itself is a per-fin material animation the
runtime drives; the geometry's job is to make every fin individually addressable.

## Budgets and rules

Provisional card: LOD0 ≤4,500 / LOD1 ≤1,800. 2048² packed PBR with a translucency mask on the fins.
Amber ≤15% of surface area as a **ceiling, not a target**. Counts under the ceiling are headroom, not
sufficiency. Nanite off; cm, +X forward, +Y right, +Z up; pivot at the ground-contact centre.

## Fidelity checks

- [ ] Side view: taller than long, stilt legs, arched spine, fin array.
- [ ] Detecting pose: spine lowered, fins lit.
- [ ] Fin detail beside the candidate's SENSOR FINS panel.
- [ ] Monochrome pair against the Riftstalker **and** the Cairnback.
- [ ] Tactical framing: it reads as something that listens, never as something that fights.
