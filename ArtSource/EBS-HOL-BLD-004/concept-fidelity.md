---
title: EBS-HOL-BLD-004 Phase Anchor — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Phase Anchor

## Traced before the card was written

Order followed, per the owner's ruling of 2026-09-07: trace the candidate, author
`REL-BLD-017.HC.ANCHOR.ASSET` from the trace, write this target, then build.

## Sources

| Role | File |
|---|---|
| Selected candidate (FIELD ACTIVE, FIELD LOST, REGISTER DETAIL) | `…/concept-discovery-20260906/phase-anchor-review/phase-anchor-candidate.png` |
| Field study | `…/phase-anchor-review/phase-anchor-magenta-field.png` |

Canon `SPEC-BLD-017.HC.ANCHOR`: the coherence optimizer. 480 health, 800 cm sight, 130 construction
ticks, 2×2 footprint, 120 Matter / 35 Dawn. It charges 5 Dawn every 600 ticks and projects a 700 cm
cost-reduction aura that drops other Choir structures' upkeep from 5 Dawn to 4. Fields do not stack.
Gameplay row `hc_phase_anchor` agrees on every shared field.

## Form (traced on the candidate's FIELD ACTIVE view, dark-pixel spans below luminance 120)

| Measured | Value |
|---|---|
| apex to plinth base | 682 px |
| spire alone | 562 px |
| plinth | 120 px |
| plinth ÷ total height | 0.176 |
| shaft width at its base | 121 px |
| slenderness (shaft base ÷ total height) | 0.177 |
| shaft width at the upper quarter | 50 px |
| field ring | an 878 × 276 px ellipse |

**Two of these are perspective reads and are not used as build ratios.** The plinth's ~480 px apparent
width is a hexagon's across-corners plus its ground shadow seen from a low camera; the ring's ellipse is
a circle in perspective. The two that *are* used are the slenderness (0.177) and the plinth's share of
the height (0.176), because both are measured on the near-vertical axis where foreshortening is least.

1. **A tapering hexagonal spire** from a pointed apex cap down to its base. No arms, no dish, no
   aperture: the Anchor is a marker, and its whole silhouette is the taper.
2. **A low stepped hexagonal plinth** beneath it, inside the 2×2 footprint.
3. **Magenta arris lines up the spire's edges.**
4. **A diamond register lattice on the spire's faces** — the read the REGISTER DETAIL panel calls out.
5. **The 700 cm aura ring is a light effect, not geometry.** The candidate draws it on the ground as a
   thin magenta circle. Its radius belongs to gameplay data; the mesh provides `Field_Ring_Origin` and
   must not imply any other radius.
6. **Two power states.** Field active: arrises and register lit, ring projected. Field lost: both dark,
   the spire inert stone.
7. **Palette.** Grey vitrified stone with Magenta Fracture. No Kharuun strata, no Meridian plates or
   conduits, no amber, no cyan.

## Forced adaptation: the plinth against the 2×2 footprint

The traced plinth-to-shaft proportion would put the plinth near 500 cm across — outside the 400 cm
footprint. The plinth is held to 300 cm across corners, inside the footprint with margin, and the
adaptation is recorded here rather than resolved by widening the placement envelope. This follows the
owner's Array Foundry ruling of 2026-09-07: adapt the proportion to the footprint, and do not change
navigation or placement rules to match a drawing's apparent proportion.

## Rig and tracks

Root, one bone for the spire and one for the apex, so the spire can drift out of phase with its own
tip — the faction's reality-bleed device, **not a canon motion clause for this building**. Tracks:
`field_active_idle`, `field_lost`, `restore`.

## Budgets and rules

Provisional card: LOD0 ≤4,000 / LOD1 ≤1,600. 2048² packed PBR. Magenta ≤12% of surface area as a
**ceiling**. Nanite off; cm, +X forward, +Y right, +Z up; pivot at the plinth's ground-contact centre.
Everything inside the 2×2 footprint. Being under the ceiling is headroom, not sufficiency.

## Fidelity checks

- [ ] Front orthographic: the taper and the plinth steps, against a 180 cm reference figure.
- [ ] Register detail: the diamond lattice reads on the shaft face.
- [ ] Field active beside field lost: the lit read differs in geometry, not brightness alone.
- [ ] Top view: everything inside the 2×2 square.
- [ ] Tactical framing: the Anchor is separable from the Chorus Loom's posts in monochrome.
