---
title: EBS-MER-UNT-003 Bulwark Team — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Bulwark Team

## Sources (read the images, not the notes about them)

| Role | File | sha256 |
|---|---|---|
| Concept, KEEP as design identity (`EBS-CON-MER-UNT-003`) | `…/Project/site/assets/concepts/meridian-units.png` bottom-left `[0, 0.5, 0.5, 1]` (crop at `…/concept-crops/all/EBS-PKG-MC-BULWARK-TEAM/EBS-CON-MER-UNT-003.png`) | `427e60cd27bd78e9…` |
| Derived reference, owner-corrected — deployed wall with six centred cells, packed travel, packed rear | `…/Evidence/concept-discovery-20260906/bulwark-review/bulwark-centered-wings-reference.png` | `6a8c5cef6697f84f…` |
| Earlier derived step (six numbered cells, wall off-centre) | `…/bulwark-review/bulwark-six-panel-reference.png` | `7574643acfde08f0…` |
| Superseded derived step with an open defect (five visible panes) | `…/bulwark-review/bulwark-reference.png` | `55e15667ecf2c6a9…` |

Review decision (verbatim intent, `EBS-CON-MER-UNT-003` KEEP): retain the broad two-operator shield
chassis and articulated barrier wings; production must show one directional front, a vulnerable rear and
a packed state; the luminous panels cannot imply all-round invulnerability. Owner correction on the
derived reference: "center the 6 panes so it looks like 3 come from left 3 come from right" — the
centred version is the target. Canon row `SPEC-UNIT-003` (Bible line 511) stays intact.

## Silhouette and proportion (measured on the derived reference; W = 500 cm deployed wall width)

Scale basis: the deployment record fixes `cover_half_width_cm` 250 and `cover_depth_cm` 350, so the
deployed face is authored at W = 500 cm across to match the cover it grants. Chassis width ≈ 0.44 W
(220 cm), chassis+legs height ≈ 0.40 W (200 cm) to the operator tops, deployed wall height ≈ 0.56 W
(280 cm) — taller than the operators, as canon requires.

1. **Chassis.** A wide, low, two-operator box: pale ceramic plates over a heavy charcoal frame, deeper
   than tall, with the rear left open (exposed rear access, visible in the packed rear view). It reads
   as a heavy crawler when packed.
2. **Twin operator stations.** TWO operator torsos side by side on the chassis top, each with a rounded
   pale cowl and a horizontal cyan visor, ≈ 0.10 W apart, tops at the chassis height. They are the
   centreline reference: the deployed wall's central seam falls between them.
3. **Legs.** FOUR block legs (two front, two rear) with pale plated thighs and charcoal hubs, feet
   ≈ 0.11 W long. The stance is wide and low; deployed movement is a slow drag.
4. **Six barrier cells.** Six framed cells numbered 01–06, three carried on each hinged wing. Deployed
   they form ONE flat directional face: cells in a row across the front, the 03/04 seam on the chassis
   centreline, each cell ≈ 0.16 W wide × 0.50 W tall in a charcoal frame with a cyan edge line and a
   translucent blue field. The face is flat and frontal — not wrapped, not domed.
5. **Central emitter.** One small emitter with a cyan lens low on the chassis centreline, under the
   03/04 seam, at the base of the deployed face.
6. **Packed state.** The wings fold back along the chassis sides: the six cells stack as open frames
   flanking the chassis (three per side), the face disappears from the front read, and the silhouette
   becomes a compact crawler wider than tall. The rear stays open in both states.
7. **Palette.** Pale ceramic plates, charcoal frame and hinges, brass edge trim (texture), cyan only on
   the visors, the emitter lens and the cell edges. Matte roughness ≥ 0.85 per the card.

## Rig and tracks

- 18 bones (`REL-FAC-025.MC.BULWARK.ASSET` .ANIM_RIG): root, chassis, two operator bones, four leg bones,
  two wing-root bones and six cell bones (three per wing). Sub-object separation is required for
  `Left_Shield_Panel` and `Right_Shield_Panel`, so each wing is also exported as its own part.
- Tracks: packed idle, move (packed), deploy (20 ticks: anticipation, wing swing, contact, settle),
  deployed idle, deployed drag (slow), pack (15 ticks, reverse), impact ripple (material, not geometry),
  damage, death, cancel, restore. No root motion; the runtime owns facing and the anchor.
- Sockets: `Target_Anchor_Center` (chassis centre), `Emitter_Muzzle` (central emitter lens),
  `Shield_Face_Center` (deployed face centre, for the impact ripple and the cover volume), plus
  `Cell_01..06` at the cell hinges.

## Budgets and rules that bound the concept (do not replace it)

`REL-FAC-025.MC.BULWARK.ASSET`: LOD0 ≤ 8,200 tris; LOD1 ≤ 3,600; 2048² PBR; matte roughness floor ≥ 0.85;
the deployed state projects a 120° directional gradient with impact ripples (material work, not geometry).
Nanite off; cm, +X forward (the shield faces +X), +Y right, +Z up; pivot at ground-contact centre. The
deployed face must never read as all-round protection, and the rear must stay visibly open. Record forced
deviations in the README §8 with the rule cited.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Deployed front: one flat face of six cells, three per side, seam on the centreline between the operators.
- [ ] Deployed side: face forward of the chassis, chassis and legs behind it, wall taller than the operators.
- [ ] Packed three-quarter: cells folded as open frames along the chassis sides, compact crawler read.
- [ ] Rear view in both states: open rear access, no shield, no cyan face.
- [ ] Tactical framing: facing and deployed footprint unmistakable; the rear says "flank me".
