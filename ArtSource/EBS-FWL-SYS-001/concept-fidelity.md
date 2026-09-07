---
title: EBS-FWL-SYS-001 Future Well — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
amended: 2026-09-06 concept-v3 (pixel measurement of the candidate; Scale section and Form item 1 — the superseded text is kept struck through)
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
   stacked blocks (height ≈ 0.35 S where S = spire height) with a "ruined rhythm" — ~~four to six taller
   broken segments rising to ≈ 0.7 S~~ four taller broken segments: ONE dominant peak on the far side
   (≈ 0.85–0.95 S; candidate Dormant panel apex at source px (392,125) over a ≈ 0.35 S crest) and three
   lower stubs (≈ 0.55–0.65 S) [amended concept-v3 by pixel measurement] — and at least one clear gap in
   the wall (the candidate shows the
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
- Concept: ~~the courtyard wall radius is ≈ 1.6–1.8 S ≈ 350–400 cm~~ the courtyard wall outer radius is
  ≈ 2.0–2.2 S ≈ 440–480 cm [amended concept-v3, 2026-09-06: measured on the candidate at source pixels —
  Dormant panel shard apparent height 88–95 px at ≈ 32° elevation (Preserve ring ellipse 410 × 220 px)
  → S ≈ 104–112 px; wall outer extremes 107..600 px (R ≈ 246) and 117..575 px (R ≈ 229) → R_out/S ≈
  2.0–2.4; the Preserve ring sits at 0.90 R_out; the apron field reaches ≈ 1.3 R_out with a +X tongue to
  ≈ 1.5 R_out; the earlier 1.6–1.8 S / 350–400 cm estimate is superseded], well outside the impassable
  tile, and a 75–150 cm wall reads impassable (REL-ART-016 / REL-ART-030 forbid passable dressing that
  reads impassable).
- Build to the concept: wall outer radius ~~360 cm~~ 450 cm (2.05 S; amended concept-v3 — a collision-less
  wall at 450 cm, the apron at ≈ 580 cm and the trace end at 600 cm all stay inside the 840 cm capture
  radius and the 1,200 cm scar radius, so the rules never forced 360) with four cardinal gaps ≥ 140 cm wide (units can enter
  the courtyard from every side; the capture zone is reachable), the wall as its own component group
  and a separate collision-less part `SM_EBS_FWL_SYS_001_Wall`, the spire + dais + inner paving as the
  main mesh with the UBX collision box confined to the one-tile footprint. The OWNER-QUESTION on
  extending the Well's blocking footprint to the wall ring (a simulation change, integration task) is
  recorded in the README §8; until answered the wall part is a concept-faithful dressing with the
  conflict flagged, and the previous one-tile "bowl-only" build remains available in git history.
- Light-only channels (concept-v3): the Preserve ring and the Reshape trace are pure light in the
  candidate (no ring or trace shows in panels 01/03-trace/spent); they are built flush (≤ 1.5 cm proud of
  the paving, the +X band or the apron) so that nothing implies a state the simulation has not committed.

## Budgets and rules that bound the concept

LOD0 ≤ 8,000 / LOD1 ≤ 3,500 assembled; three slots (basalt, vitrified, state); Nanite off; cm, +X =
Reshape trace direction; pivot at the spire's ground centre; sockets `Target_Anchor_Center`,
`State_VFX_Origin`, `Spire_Hinge`, `Reshape_Trace_End`, `Wall_Gap_01..04`; emissive ≤ 15% of area at
the game framing; walkable inner paving relief ≤ 20 cm.

## Fidelity checks (render next to the concept crop)

- [ ] Tactical view: broad dark courtyard (wall ≈ 2.0–2.2 S; the shard spans ≈ 15 % of the courtyard
      width) with a ring wall of uneven blocks, one dominant far-side peak over three stubs, gaps, a torn
      apron with a +X tongue.
- [ ] Single faceted shard spire on a dais at the centre; radial/concentric paving.
- [ ] Preserve: flat cyan ring hugging the wall's inner face (0.90 R_out). Reshape: thin magenta trace
      leaving through the +X gap onto the tongue; neither shows in the other states.
- [ ] Harvest commit and spent: shard sinks, bowl dark and cracked, nothing rising above the wall.
- [ ] Dormant reads as held potential (low glow), doubled shadow cue documented for lighting.
