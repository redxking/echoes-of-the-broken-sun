---
title: EBS-KHA-UNT-002 Riftstalker — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Riftstalker

## Sources (read the images, not the notes about them)

| Role | File |
|---|---|
| Selected candidate — the production direction (SKIRMISHER, MOVING FIRE, SHOULDER DETAIL) | `…/concept-discovery-20260906/riftstalker-review/riftstalker-candidate.png` |
| Concept input | `EBS-CON-KHA-UNT-002`, cropped to `…/concept-crops/all/EBS-PKG-KA-RIFTSTALKER/` |

Canon row `SPEC-UNIT-006`: a lean, long-limbed warform with a low forward posture, a faceted carapace in
charcoal with amber seams, and a shoulder-mounted shard-caster that fires while it moves. The forward
lean and light frame say *skirmisher that keeps moving*; it visibly lacks the mass for a frontal fight.
Fires on the move with a short sidestep after each shot. Molt at a Growth Basin shows a visible carapace
or striker change.

## It is a quadruped

Both the SKIRMISHER and MOVING FIRE views show **four long legs and no neck**. My provisional card first
described a biped with a long neck; that was written before the candidate was traced, and the card has
been corrected to match the concept. The body tapers forward into a prow rather than carrying a head on
a neck, and the shard-caster fires past that prow from a slot in the shoulder carapace.

## Form and proportion (traced on the candidate's MOVING FIRE side view)

| Measured on the candidate | Value |
|---|---|
| body length, prow tip to rear (projectile excluded) | 508 px |
| standing height, carapace top to ground | 348 px |
| **body length ÷ standing height** | **1.46** |

Scale: standing height 200 cm, sitting between the Tender's 190 cm crown and the Lancer's 213 cm, so
the roster reads consistently. Body length is therefore 292 cm.

1. **Four long three-segment legs.** Knees carried high and outboard, feet small and pointed. The legs
   are the unit's mass budget; the body is comparatively slight.
2. **A low forward posture built into the rest pose**, not only into the animation. The carapace top
   slopes down toward the prow.
3. **A layered faceted carapace.** Overlapping plate shells in charcoal, with amber seams between them.
   Zero organic smoothing (`REL-ART-029`).
4. **A forward-tapering prow.** The body narrows to a point ahead of the front legs; nothing sits on a
   neck.
5. **A shoulder-mounted shard-caster.** An amber-lit slot in the shoulder carapace, aimed forward past
   the prow. The SHOULDER DETAIL panel shows it as part of the carapace, not a bolted-on weapon.
6. **Molt variants.** A visible carapace change and a visible striker change, separately controllable.
7. **Palette.** Charcoal plate with amber seams; matte. **It must visibly lack the mass for a frontal
   fight**, and must not be mistakable for the Cairnback, which is the faction's heavy quadruped.

## Rig and tracks

Per the corrected provisional card `REL-FAC-025.KA.RIFTSTALKER.ASSET`: root, body, a two-bone prow, a
two-bone caster mount that aims independently of the gait, and four three-segment legs each with a foot
— 22 bones. Not the Tender's humanoid rig and not the Cairnback's heavy plan.

Tracks: `idle`, `move`, `fire_on_the_move`, `sidestep` (canon's short sidestep after each shot), `molt`,
`death`. No root motion; the runtime owns translation.

## Budgets and rules that bound the concept

Provisional card: LOD0 ≤6,000 / LOD1 ≤2,600 for the complete assembly including the caster and every
molt-variant part carried on the same asset — below the faction default because a light frame has to
read as light. 2048² packed PBR. Amber ≤15% of surface area as a **ceiling, not a target**. Nanite off;
cm, +X forward, +Y right, +Z up; pivot at the ground-contact centre between the feet.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Side view: body length 1.46 of the standing height, carapace sloping down toward the prow.
- [ ] Three-quarter: four legs, knees high and outboard, layered carapace, amber seams.
- [ ] Firing pose: the caster's line clears the prow and does not fight the gait.
- [ ] Molt states: carapace and striker changes visible on the unit itself.
- [ ] Top view: four legs at their stance, the prow ahead of the front pair.
- [ ] Beside the Cairnback when that package exists: the two must not be confusable.
