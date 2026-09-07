---
title: EBS-FWL-SYS-001 Future Well — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Future Well family

## Sources (read the images)

| Role | File | Region | sha256 |
|---|---|---|---|
| Concept states sheet, REWORK inputs (`EBS-CON-FWL-SYS-001..004`) | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/site/assets/concepts/future-well-states.png` | TL Dormant, TR Harvest, BL Preserve, BR Reshape | `f95912742d42bd75…` |
| Derived candidate (one family, four states + spent) | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/future-well-review/future-well-candidate.png` | whole | `b16e84cb4465c3c3…` |
| Composition reference only (`EBS-CON-FWL-SYS-007`) | `.../site/assets/concepts/target-render-vertical-slice.jpg` | central rings and upright shard `[0.37, 0.29, 0.63, 0.68]` | `04ec9daa608ee6d5…` |
| Retained history only, NOT a reference (`EBS-CON-FWL-SYS-005`) | `echoes-future-well-landmark.jpg` | — | giant tornado, rejected |

Review decisions (verbatim intent): retain the courtyard-scale bowl and central spire as the family
base; rework the ceremonial stone machinery into vitrified charcoal glass with quiet fracture seams and
doubled outline/shadows; lower the gold glow and floating spectacle so Dormant reads as held potential;
the floating rings are a design proposal, not established engineering. Harvest: telegraph climbs the
spire, the spire folds into the bowl, light cuts off, a dark cracked spent site remains (amber per
book/Bible; REL-ART-014 cyan conflict recorded, not resolved). Preserve: intact core, one slow custody
ring around the bowl. Reshape: magenta identity pointing to an actual temporary crossing with expiry.

## Form (the candidate is the reconciled family; the sheet gives the material and courtyard read)

1. **Courtyard bowl.** A broad circular courtyard of dark vitrified basalt masonry: a low ring wall of
   stacked blocks (height ≈ 0.35 S where S = spire height) with a "ruined rhythm" — four to six taller
   broken segments rising to ≈ 0.7 S — and at least one clear gap in the wall (the candidate shows the
   break on the +X side, where the Reshape trace leaves). Inside: flat radial paving with concentric
   courses and fine magenta/amber fracture seams. Outside: a fractured paving apron that crumbles into
   the ground.
2. **Central spire.** ONE faceted dark shard/pyramid (four-sided, slightly irregular, S ≈ 220 cm at
   presentation scale) standing on a small circular dais at the centre; fine seam light along its edges.
   NOT a four-petal opening flower.
3. **Corner pylons (sheet).** The sheet's four standing pylons are the "ceremonial machinery" the review
   reworks: keep their presence as the four taller broken wall segments of item 1, not as separate
   monoliths.
4. **Floating rings (sheet).** Design proposal only: Preserve's custody ring is a flat cyan light ring on
   the paving inside the wall (candidate 03), not a floating hoop; Harvest's rings are the amber climb
   effect (candidate 02), not geometry.
5. **States.** Dormant: dark bowl, faint seams, doubled outline/shadow cue. Harvest telegraph: amber
   climbs the spire; commit: the spire folds/sinks into the dais (single shard tilting and sinking, ≈ 20
   ticks), then spent: dark cracked bowl, spire gone below the paving, permanent (candidate inset).
   Preserve: cyan ring on the paving. Reshape: magenta trace from the centre through the wall gap toward
   +X, ending at the footprint edge (the crossing itself is map-owned).

## Scale and the simulation conflict (recorded, not resolved here)

- Simulation: impassable footprint = one tile (200 cm) centred on the spire (`Simulation.cpp`,
  half extent `kFixedScale/2`); capture radius 4.2 tiles (840 cm); scar radius 6 tiles.
- Concept: the courtyard wall radius is ≈ 1.6–1.8 S ≈ 350–400 cm, well outside the impassable tile,
  and a 75–150 cm wall reads impassable (REL-ART-016 / REL-ART-030 forbid passable dressing that reads
  impassable).
- Build to the concept: wall outer radius 360 cm with four cardinal gaps ≥ 140 cm wide (units can enter
  the courtyard from every side; the capture zone is reachable), the wall as its own component group
  and a separate collision-less part `SM_EBS_FWL_SYS_001_Wall`, the spire + dais + inner paving as the
  main mesh with the UBX collision box confined to the one-tile footprint. The OWNER-QUESTION on
  extending the Well's blocking footprint to the wall ring (a simulation change, integration task) is
  recorded in the README §8; until answered the wall part is a concept-faithful dressing with the
  conflict flagged, and the previous one-tile "bowl-only" build remains available in git history.

## Budgets and rules that bound the concept

LOD0 ≤ 8,000 / LOD1 ≤ 3,500 assembled; three slots (basalt, vitrified, state); Nanite off; cm, +X =
Reshape trace direction; pivot at the spire's ground centre; sockets `Target_Anchor_Center`,
`State_VFX_Origin`, `Spire_Hinge`, `Reshape_Trace_End`, `Wall_Gap_01..04`; emissive ≤ 15% of area at
the game framing; walkable inner paving relief ≤ 20 cm.

## Fidelity checks (render next to the concept crop)

- [ ] Tactical view: broad dark courtyard with a ring wall of uneven blocks, four taller segments, gaps.
- [ ] Single faceted shard spire on a dais at the centre; radial/concentric paving.
- [ ] Preserve: flat cyan ring inside the wall. Reshape: magenta trace leaving through the +X gap.
- [ ] Harvest commit and spent: shard sinks, bowl dark and cracked, nothing rising above the wall.
- [ ] Dormant reads as held potential (low glow), doubled shadow cue documented for lighting.
