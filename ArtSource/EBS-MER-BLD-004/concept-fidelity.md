---
title: EBS-MER-BLD-004 Aegis Post — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Aegis Post

## Sources (read the images, not the notes about them)

| Role | File | sha256 |
|---|---|---|
| Selected candidate — the production direction (POWERED, OFFLINE, REAR/SUPPLY) | `…/concept-discovery-20260906/aegis-post-review/aegis-post-candidate.png` | (recorded in the README §1 from the generation record) |
| Mount study | `…/aegis-post-review/aegis-post-mount-study.png` | (same record) |
| Concept A, REWORK (`EBS-CON-MER-BLD-004`) | `…/site/assets/concepts/meridian-structures.png` bottom-right `[0.5, 0.5, 1, 1]` | `5b64820bbd2bbc28…` |
| Concept B, REWORK (`EBS-CON-MER-BLD-008`) | `…/site/assets/concepts/echoes-meridian-structures.jpg` bottom-right `[0.5, 0.5, 1, 1]` | `b6396728d4fd2a49…` |

Review decisions (verbatim intent): A — retain the readable rotating gun and the obvious power
connection; rework the large single ornamental barrel and circular pedestal into the Bible's
three-legged ceramic mount and twin-emitter head; supply and offline status must be visible. B — retain
twin emitters and a readable turret head; replace the massive bunker and decorative shield bubble with
the canonical three-legged mount and heavy supply coupling; no new shielding mechanic follows from that
painting; make loss of power obvious. Canon row `SPEC-BLD-015.MC.AEGIS` (Bible line 516) stays intact.

## Form and proportion (measured on the candidate's main view; F = 400 cm, the 2×2 footprint)

Scale basis: `buildings.json` `mc_aegis_post.footprint_cells` = 2×2 tiles = 400 cm square.

**Correction, 2026-09-07 (mine).** My first pass wrote ~~height ≈ 1.02 of the leg spread, head width
≈ 0.74 of the leg spread, hub top at ≈ 0.62 of the height~~. Those numbers were wrong: I had measured a
region of the card that included its caption text and an inset view. A silhouette trace of the main
POWERED view alone (dark-pixel row spans, figure 669 × 577 px) gives:

| Measured on the candidate | Value |
|---|---|
| height ÷ leg spread (outer pad edges) | **0.87** |
| head width ÷ leg spread | **0.60** |
| head height ÷ total height | **0.14** |
| hub top ÷ total height | **0.85** |

The post is therefore long-legged and low-headed: three splayed legs carry 0.85 of the height, and a
compact, wide, flat head sits on top. The build follows the traced numbers, not my first pass.

1. **Three-legged ceramic mount.** THREE legs, not four and not a pedestal: each a pale ceramic upper
   thigh from the hub, a charcoal knee joint, a lower shin, and a wide flat ground pad. Two legs forward
   (±60° from +X) and one to the rear, so the stance reads as a tripod from every angle. Leg spread
   ≈ 340 cm across the pads, inside the 400 cm square; hub top at ≈ 0.62 of the total height.
2. **Rotating head.** ONE boxy head on the hub, wider than tall, carrying TWO emitter barrels side by
   side on its front face. The head is ≈ 0.74 of the leg spread wide (measured), ≈ 0.30 of it tall, and
   it rotates in yaw on the hub and pitches at its trunnion.
3. **Twin emitters.** Two short hexagonal barrels with recessed dark muzzles, side by side and level —
   not one large ornamental barrel. Their axes are parallel and horizontal when powered.
4. **Cyan supply band.** ONE band circling the head below the barrels, lit only while the post is
   supplied. It is the single status tell: no other cyan on the asset except the muzzle interiors.
5. **Heavy power coupling.** A charcoal coupling block low on the hub's rear with two thick cables that
   run down the rear leg and out to the ground — the "defense that needs the network" read. Visible from
   the rear as the candidate's REAR/SUPPLY panel shows.
6. **States.** POWERED: head level, aiming, band lit. OFFLINE: the head droops nose-down and the band
   and muzzles go dark, with no smoke or motion implying life. The droop is geometry driven by the pitch
   bone, not a material change.
7. **Palette.** Pale ceramic plates over charcoal structure with brass edge trim (texture), yellow
   hazard chevrons on the leg pads (texture); matte throughout.

## Rig and tracks

- The card fixes a **2-bone pitch and yaw turret aiming rig**, so the asset is skeletal: `root`,
  `turret_yaw` on the hub and `turret_pitch` at the head trunnion (the two aiming bones the card names).
  Yaw sweeps at 360°/s to track authoritative targets and snaps instantly under Reduced Motion.
- Sockets: `Muzzle_Left`, `Muzzle_Right` (emitter mouths, the tracer origins), `Target_Anchor_Center`
  (hub centre) and `Power_Coupling` (the cable landing on the rear of the hub).
- Tracks: `powered_idle` (head level, slow scan), `aim` (yaw and pitch to a target), `fire` (a short
  recoil through the head), `offline` (the head droops and holds), `restore` (single-frame powered rest).
  No root motion; the post never moves.

## Budgets and rules that bound the concept

`REL-BLD-015.MC.AEGIS.ASSET`: LOD0 ≤4,000 / LOD1 ≤1,600; 2×2 footprint; 2048² PBR; the 2-bone aiming
rig; severing power darkens all status bands and drops the head into the offline pose within 1 tick.
`REL-ART-028` (8,000 / 3,500) is looser here, so the card governs. Owner ruling 2026-09-07: ceilings
apply to the complete asset including articulated components. Nanite off; cm, +X forward (the head's
rest facing), +Y right, +Z up; pivot at the ground-contact centre between the pads.

**Recorded card conflict, not resolved here.** The same card's `.MESH_PROP` line describes "a vertical
heavy turret barrel mounted onto an elevated orthogonal protective concrete pillbox carriage". That
contradicts the canon row, both review decisions and the selected candidate, all of which call for a
three-legged ceramic mount with twin emitters — and decision B explicitly replaces the bunker. The build
follows canon and the concept; the card's pillbox wording is raised as an OWNER-QUESTION.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Three-quarter powered view: tripod stance, boxy head, twin barrels, lit band, cables to the ground.
- [ ] Offline view: head drooped nose-down, band and muzzles dark, nothing else changed.
- [ ] Rear view: the power coupling and its two cables read as the supply connection.
- [ ] Top view: three legs at 120° with the pads inside the 2×2 square.
- [ ] Tactical framing: firing direction and the power connection are both readable.
