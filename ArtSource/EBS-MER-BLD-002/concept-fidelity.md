---
title: EBS-MER-BLD-002 Power Link — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be); §Proportion items 1, 3 and 6 amended 2026-09-07 from the candidate pixels with the measurement recorded inline (the owner ruling puts the pixels above the doc's earlier numbers; the amendment is raised as an OWNER-QUESTION through the coordinator)
---

# Concept fidelity target — Power Link

## Sources (read the images)

| Role | File | Region | sha256 |
|---|---|---|---|
| Concept A, REWORK input (`EBS-CON-MER-BLD-002`) | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/site/assets/concepts/meridian-structures.png` | top-right `[0.5, 0, 1, 0.5]` | `5b64820bbd2bbc28…` |
| Concept B, REWORK input (`EBS-CON-MER-BLD-006`) | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/site/assets/concepts/echoes-meridian-structures.jpg` | top-right `[0.5, 0, 1, 0.5]` ("POWER LINK [Conduit Pylon]") | `b6396728d4fd2a49…` |
| Derived candidate reconciling A+B with the review notes | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/power-link-review/power-link-candidate.png` | whole (Connected, Disconnected, Maintenance/Damage) | `bb8ffbe50e6683a2…` |
| Maintenance detail | `power-link-maintenance-complete.png` in the same folder | whole | `b3816b9e6ec6f566…` |

Review decisions (verbatim intent): A — retain the vertical pylon and physical connection points; slim
its oversized reactor base; remove exposed trophy-crystal emphasis; show removable numbered ceramic
panels, load collar and accessible conduit couplings; maintained distribution equipment. B — retain the
slender ceramic pylon; replace airborne lightning webs with physically legible conduits and restrained
status illumination; numbered panels and a service collar the crew can remove. Canon row
`SPEC-BLD-015.MC.LINK` (Bible line 514): slim ceramic pylon, charcoal base, ring of cyan conductor
collars, paired conduits leaving the base toward neighbours; connected = collars lit and conduits
pulsing; disconnected = dark; damaged = a collar dark, a conduit hanging.

## Proportion (measured on the candidate, cross-checked with A and B)

1. **Tower.** Shaft width W (across the flats); total height H = **3.8–4.0 W** (amended 2026-09-07 —
   measured band 3.5–4.2 W; B ~6, A ~5 are not the selected candidate). Measurement on
   `power-link-candidate.png` (1536 × 1024): main view cap top y ≈ 120, plinth ground contact y ≈ 745
   → 625 px; W from the service face (114 px) plus two chamfers (44 px each, ×1/√2) at ≈ 15° yaw → 176 px
   (the octagon silhouette at that yaw is 178 px ≈ 1.0 W); H/W = 3.55 raw. Disconnected view: cap top
   y ≈ 40, ground y ≈ 406 → 366 px; W 99–107 px → 3.4–3.7 raw. Vertical foreshortening at the painted
   15–25° elevation adds 6–10 % and the front plinth corner subtracts ≈ 0.1 → 3.5–4.2 W. The earlier
   4.6–5.0 W band was not reproducible from the pixels. Plinth 360 cm = 1.8 W (main view 2.0 W,
   disconnected 1.5 W) → W = 200 cm → H = 3.9 W = 780 cm (cap top). This replaces the concept-v2
   850 cm / 180 cm and the v3 construction-reference height of 1,238 cm at 150 cm shaft (8:1) — that
   slenderness was NOT the concept.
2. **Section.** Square with heavy chamfers reading as an octagon (candidate) / hexagonal in B: use an
   octagonal section with four wide faces and four narrow chamfer faces; the service face is a wide face.
3. **Panels.** Four removable numbered ceramic panels 01–04 stacked on the service face: 01 above the
   collar, 02–04 below it, each ≈ 0.17–0.19 H tall (plates read near-square, ≈ 0.62 W wide × ≈ 0.55 W
   tall in the main view; the four plates fill the shaft with thin seams) with fastener bosses at the
   corners and a numeral cell.
   Panel 02 is the one shown removed in maintenance (leaning against the plinth) with the conduit bundle
   visible in the open bay behind it.
4. **Load collar.** ONE broad octagonal collar at 0.70–0.74 H (between panels 01 and 02), ≈ 1.35 W wide,
   ≈ 0.07 H tall, with four cyan strips on the wide faces (the "conductor collars" / "grid spreaders").
   Concept A's three stacked rings become the collar's three horizontal courses (grooves), not three rings.
5. **Cap.** Low octagonal cap plate with a small square fastener plate; NO crystal, NO antenna beyond a
   short stub.
6. **Base.** Two-step plinth (lower step 360 cm, upper step ≈ 250 cm, total ≈ 0.10 H tall) with a
   chamfered skirt; two coupling blocks (≈ 0.9 W × 0.5 W × 0.5 W, standing outboard at the corners —
   the sideways protrusion past the plinth is bounded by the footprint, README §8.1 D8) at the
   front-left and front-right of the plinth where the paired conduits leave; short conduit stubs
   (segmented, **Ø ≈ 0.16 W**, measured band 0.15–0.18 W — amended 2026-09-07: jackets 28–31 px on the
   main view (columns x 60–180, rows 673–763, the pair leaving the left coupling) against the
   across-flats W = 176 px of item 1, ± 10 % perspective; the earlier ~0.09 W was not a measurement) run
   outward and end at the footprint edge (the runtime draws the link between neighbours). Charcoal frame
   at the base, pale ceramic shaft, pale ceramic insulator rings on the conduits (B: "pale ceramic
   insulators").
7. **States.** Connected: collar strips lit cyan, conduit pulse. Disconnected: dark. Maintenance/damage:
   panel 02 removed, bay open, one conduit hanging from its port (existing hanging-stub part), a collar
   strip dark. Team colour by mask on a narrow band under the collar.

## Budgets and rules that bound the concept

LOD0 ≤ 3,500 / LOD1 ≤ 1,200 tris (`REL-BLD-015.MC.LINK.ASSET`); Nanite off; cm, +X forward (service
face), +Y right, +Z up; pivot at ground-contact centre; UBX collision on the plinth and shaft; the
socket set already contracted in the README §1/§4 (conduit ports, collar state, panels, target anchor);
emissive ≤ 15% of area at the game framing (`REL-ART-004`). Height 780 cm is a concept measurement, not
construction reasoning; record any budget-forced simplification in the README §8.

## Fidelity checks (render next to the concept crop)

- [ ] Front-quarter view: octagonal ceramic tower ≈ 3.8–4 W tall (measured band 3.5–4.2) on a two-step plinth with two couplings.
- [ ] Service face: panels 01 / collar / 02 / 03 / 04 in that order with numeral cells.
- [ ] Collar reads as the single bright band at ~0.72 H; no crystal cap.
- [ ] Maintenance: panel 02 off and leaning, bay open, conduit hanging.
- [ ] Tactical game framing: the pylon reads "cables, not turret"; conduits leave the base.
