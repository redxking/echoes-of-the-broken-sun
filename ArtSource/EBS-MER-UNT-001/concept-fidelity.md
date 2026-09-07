---
title: EBS-MER-UNT-001 Surveyor — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Surveyor

## Sources (read the images, not the notes about them)

| Role | File | Region | sha256 |
|---|---|---|---|
| Concept, KEEP as DESIGN_IDENTITY (`EBS-CON-MER-UNT-001`) | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/site/assets/concepts/meridian-units.png` | top-left quadrant `[0, 0, 0.5, 0.5]` | `427e60cd27bd78e9…` |
| Derived reference (front, rear, gathering, delivery) | `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/surveyor-review/surveyor-reference.png` | whole | `98c3a0ef23001789…` |
| Rejected as modelling reference | `surveyor-derived-turnaround.png` in the same folder | — | canister-count drift, reversed side labels |

Review decision (verbatim intent): retain the compact worker frame, paired working arms and visible
cargo; drill/gripper and repaired ceramic shoulder; simplify the decorative mast panel; loaded/unloaded
cargo readable; a worker, not an armed robot. Canon row `SPEC-UNIT-001` (Bible line 509) stays intact.

## Silhouette and proportion (measured on the concept; authored height 176 cm at mast top, ×1.5 at runtime)

1. **Knuckle-walker stance.** The torso is a single dense block at the TOP of the figure, pitched
   forward ~10°. Both arms hang forward-down from shoulder hubs on the torso sides and reach nearly to
   the ground ahead of the feet (tool tips 5–15 cm above ground, 60–90 cm ahead of the hip line). The
   arms are as long as the legs: upper arm ≈ 0.28 H, forearm ≈ 0.26 H, tool ≈ 0.14 H (H = 176).
2. **Legs.** Two thick digitigrade legs at the REAR half of the torso: thigh ≈ 0.30 H angled forward-
   down, prominent knee hub (disc ≈ 0.09 H diameter), shin ≈ 0.28 H angled back-down, block foot
   ≈ 0.20 H long with a raised toe plate and heel block. Feet planted ≈ 0.45 H apart. Knees ahead of
   the hips, feet under or slightly behind the knees.
3. **Torso.** Width ≈ 0.38 H, height ≈ 0.30 H, depth ≈ 0.34 H; pale ceramic plates over a charcoal
   frame with brass-toned edge trim (trim is texture, not geometry). Front face carries ONE large cyan
   optical lens (Ø ≈ 0.07 H) on the upper-left third and a cyan status band below it. A shoulder
   plate with a visible repair patch (¶1606) on the upper-right plate.
4. **Cargo cradle.** THREE upright cylindrical canisters (each Ø ≈ 0.10 H, height ≈ 0.16 H) standing
   side by side in a low open cradle frame on the REAR-RIGHT top of the torso, tops above the torso top,
   each with a vertical cyan window strip. Loaded/unloaded must read: canisters are separate components
   (`body_canister_01..03`) so the runtime can hide them.
5. **Optical mast.** A short post (≈ 0.12 H) rising from the rear-LEFT top of the torso with a small
   boxy camera head (≈ 0.06 H) — the concept's tall holographic panel is simplified away per the review.
6. **Right arm: rotary drill.** Tapered conical drill (length ≈ 0.14 H) on a cylindrical housing with a
   cyan status strip; `Harvest_Tether_Muzzle` at the drill tip.
7. **Left arm: gripper/welder.** Two-finger (concept shows two long fingers plus a thumb nub) gripper,
   recessed palm welder; cyan strip on the forearm.
8. **Width.** Across the arms at rest ≈ 1.15–1.30 H; across the feet ≈ 0.45 H; the figure is wider
   than tall in the front view (canon: "half again as wide at the shoulder").

## Rig and tracks (unchanged contract, re-tuned to the new proportions)

- 12 bones: root, body, r/l_thigh, r/l_shin, r/l_foot, r/l_shoulder, r/l_forearm. Sockets
  `Harvest_Tether_Muzzle` (r_forearm, drill tip), `Cargo_Drop_Anchor` (body, cradle centre),
  `Center_Hitbox_Socket` (body, torso centre).
- Rest pose = the concept stance (item 1–2), not an upright humanoid.
- Clips per the README §5 intents. Gather = the derived reference's GATHERING panel (drill tip into the
  ground ahead, torso lowered). Deliver = the DELIVERY panel: the frame crouches beside the drop point;
  the cradle's canisters are runtime-hidden on the authorized transfer (no arm reach required).
  Death = knees fold, torso drops forward onto its arms (the long arms carry it), held. All clips keep
  every vertex above z = −1 cm (existing tests).

## Budgets and rules that bound the concept (do not replace it)

LOD0 ≤ 4,500 / LOD1 ≤ 1,800 tris; two material slots; emissive ≤ 5% of area at the game framing;
Nanite off; cm, +X forward, +Y right, +Z up; root at ground-contact centre; no root motion; footprint
1 tile. Where a rule forces a deviation from the concept, record it in the README §8 as a deviation
with the rule cited.

## Fidelity checks (each must be shown in a render next to the concept crop)

- [ ] Front view: torso on top, arms hanging forward-down to the ground, legs at the rear, wider than tall.
- [ ] Right view: knuckle-walker profile (torso pitched, arm reach ahead of the knees, digitigrade legs).
- [ ] Three upright canisters visible from the rear and top; mast on the opposite side.
- [ ] Eye and status band on the front face; drill right, gripper left (anatomical).
- [ ] Gather pose matches the GATHERING panel; deliver crouch matches DELIVERY.
