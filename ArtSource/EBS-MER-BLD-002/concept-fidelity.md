---
title: EBS-MER-BLD-002 Power Link — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
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

1. **Tower.** Shaft width W; total height H = 4.6–5.0 W (candidate 4.6, B ~6, A ~5). Footprint 2×2
   tiles = 400 cm: plinth 360 cm across → W = 180 cm → H ≈ 850 cm (cap top). This replaces the v3
   construction-reference height of 1,238 cm at 150 cm shaft (8:1) — that slenderness was NOT the concept.
2. **Section.** Square with heavy chamfers reading as an octagon (candidate) / hexagonal in B: use an
   octagonal section with four wide faces and four narrow chamfer faces; the service face is a wide face.
3. **Panels.** Four removable numbered ceramic panels 01–04 stacked on the service face: 01 above the
   collar, 02–04 below it, each ≈ 0.19 H tall with fastener bosses at the corners and a numeral cell.
   Panel 02 is the one shown removed in maintenance (leaning against the plinth) with the conduit bundle
   visible in the open bay behind it.
4. **Load collar.** ONE broad octagonal collar at 0.70–0.74 H (between panels 01 and 02), ≈ 1.35 W wide,
   ≈ 0.07 H tall, with four cyan strips on the wide faces (the "conductor collars" / "grid spreaders").
   Concept A's three stacked rings become the collar's three horizontal courses (grooves), not three rings.
5. **Cap.** Low octagonal cap plate with a small square fastener plate; NO crystal, NO antenna beyond a
   short stub.
6. **Base.** Two-step plinth (lower step 360 cm, upper step ≈ 250 cm, total ≈ 0.10 H tall) with a
   chamfered skirt; two coupling blocks (≈ 0.9 W × 0.5 W × 0.5 W) at the front-left and front-right of
   the plinth where the paired conduits leave; short conduit stubs (segmented, Ø ≈ 0.09 W) run outward
   and end at the footprint edge (the runtime draws the link between neighbours). Charcoal frame at
   the base, pale ceramic shaft.
7. **States.** Connected: collar strips lit cyan, conduit pulse. Disconnected: dark. Maintenance/damage:
   panel 02 removed, bay open, one conduit hanging from its port (existing hanging-stub part), a collar
   strip dark. Team colour by mask on a narrow band under the collar.

## Budgets and rules that bound the concept

LOD0 ≤ 3,500 / LOD1 ≤ 1,200 tris (`REL-BLD-015.MC.LINK.ASSET`); Nanite off; cm, +X forward (service
face), +Y right, +Z up; pivot at ground-contact centre; UBX collision on the plinth and shaft; the
socket set already contracted in the README §1/§4 (conduit ports, collar state, panels, target anchor);
emissive ≤ 15% of area at the game framing (`REL-ART-004`). Height 850 cm is a concept measurement, not
construction reasoning; record any budget-forced simplification in the README §8.

## Fidelity checks (render next to the concept crop)

- [ ] Front-quarter view: octagonal ceramic tower ≈ 4.6–5 W tall on a two-step plinth with two couplings.
- [ ] Service face: panels 01 / collar / 02 / 03 / 04 in that order with numeral cells.
- [ ] Collar reads as the single bright band at ~0.72 H; no crystal cap.
- [ ] Maintenance: panel 02 off and leaning, bay open, conduit hanging.
- [ ] Tactical game framing: the pylon reads "cables, not turret"; conduits leave the base.
