---
title: EBS-MER-UNT-002 Lancer — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Lancer

## Sources (read the images, not the notes about them)

| Role | File | sha256 |
|---|---|---|
| Selected candidate — the production direction | `…/Evidence/concept-discovery-20260906/lancer-review/lancer-candidate.png` (main braced view, braced side view, tactical silhouette) | `b5c85d32390c8f4c…` |
| Derived turnaround — front / left side / rear / top, plus the hip-mount and leg-attachment inset | `…/lancer-review/lancer-derived-turnaround.png` | `efa03f0490a17d8a…` |
| Superseded, retained history only (`EBS-CON-MER-UNT-002`, decision REPLACE) | `…/Project/site/assets/concepts/meridian-units.png` top-right `[0.5, 0, 1, 0.5]` | `427e60cd27bd78e9…` |

Review decision (verbatim intent, `EBS-CON-MER-UNT-002` REPLACE): replace the upright armored hero
silhouette. Retain pale ceramic, charcoal underframe and cyan rail technology in a new narrow
two-legged line-fire frame: low cowl, lance held low, exposed thin flanks, visible rear-leg recoil
strut. Owner note on the original: "Check out the book for the lancer make sure it makes sense we need
to rework this i think". Owner feedback on this candidate, recorded in the Relay Skiff generation
record: "Love that continue to the next" (positive on the candidate; not production acceptance).
Canon row `SPEC-UNIT-002` (Bible line 510) stays intact.

## Silhouette and proportion (measured on the candidate and the turnaround; H = 200 cm at the cowl/pod top)

Scale basis: canon "slightly taller than the Surveyor and narrow"; the Surveyor is authored at 176 cm,
so H = 200 cm is PROVISIONAL at the same authored scale (runtime `PresentationScale` is not part of the asset).

1. **Braced stance.** A narrow two-legged frame standing in a wide fore-and-aft brace, not a square
   stance: the leading leg is planted ahead and the trailing leg back, feet ≈ 1.0–1.2 H apart along +X
   and only ≈ 0.35 H apart across +Y. The body is carried low between the hips and pitched ~8° forward.
2. **Rail-lance.** ONE long lance carried at hip height on a hip mount at the body's front, axis parallel
   to the ground and along +X. Total lance length ≈ 1.05 H; ≈ 0.60 H of it projects ahead of the leading
   foot. The lance is a stack of two parallel rails over a charcoal body with a continuous cyan channel
   running its full length between them, a blocky muzzle head and three collar clamps along its length.
   Diameter of the whole assembly ≈ 0.10 H. It reads as the heaviest single element in the silhouette.
3. **Low sensor cowl.** A low wedge head set BETWEEN the shoulder pods with a horizontal cyan visor slot,
   no neck: the cowl top is level with or just under the pod tops, and it sits at ≈ 0.90 H.
4. **Shoulder pods.** Two pale ceramic boxy pods flanking the cowl at ≈ 0.55 H apart, tops at H, each
   ≈ 0.22 H long and ≈ 0.13 H tall, with a small cyan strip. They are the widest part of the upper body.
5. **Exposed flanks.** The body between the pods and the hips is open charcoal frame: struts and joint
   hubs are visible from the side, no ceramic skirt. This is the readable weak read the canon requires.
6. **Rear-leg recoil strut.** A thin charcoal strut running from the lower rear of the body diagonally
   down and back to the trailing ankle, with a visible slide/piston section at its middle. It is present
   in the side, rear and top views and must read as a separate slim member, not a leg plate.
7. **Legs.** Digitigrade: thigh forward-down ≈ 0.28 H, shin back-down ≈ 0.26 H, prominent knee hub
   (disc ≈ 0.08 H diameter), block foot ≈ 0.20 H long with a raised toe plate. Pale plate on the thigh
   and shin outer faces, charcoal hubs.
8. **Palette.** Pale ceramic plates, charcoal frame, brass-toned edge trim (texture, not geometry), cyan
   only on the lance channel, the visor and small strips. Nothing on it reads as a hero body.

## Rig and tracks

- 18 bones (`REL-ART-005.MC.LANCER` .ANIM_RIG), including `lance_yaw` (the card's `Turret_Y` sub-object)
  and `lance_barrel` (`Barrel_X`), the cowl, both legs (thigh/shin/foot), both pods, and the two-part
  recoil strut. No root motion; the runtime owns facing (`SPEC-MOV-010`).
- Sockets required by the card: `Muzzle_Flash_01` (lance muzzle), `Target_Anchor_Center` (body centre),
  `Left_Tread_Vector` (card name; this frame walks on legs, so it is placed at the left foot ground
  contact and the naming conflict is recorded as a deviation, exactly as for the Surveyor's tread wording).
- Tracks per canon: idle, move, turn, stop, **halt-plant-aim-fire-recover** (the card's line: never fires
  while moving; recoil returns through the mount and the strut slides), damage, death, cancel, restore.
  The firing track drives the lance back along its own axis and the strut's slide section; the muzzle is a
  clean cyan-white line, produced by effects, not geometry.

## Budgets and rules that bound the concept (do not replace it)

`REL-ART-005.MC.LANCER`: LOD0 ≤ 8,000 tris; LOD1 ceiling is printed as "3,3500" in Requirements.md — an
evident typo, bounded here at 3,500 and raised as an OWNER-QUESTION rather than silently chosen. 2048²
PBR stack; emissive ≤ 15% of surface with an un-bloomed amber/cyan blend; team colour by mask. Nanite
off; cm, +X forward (lance axis), +Y right, +Z up; root at ground-contact centre between the feet.
Where a rule forces a deviation from the concept, record it in the README §8 with the rule cited.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Side view: fore-and-aft brace with the lance level at hip height and the rear-leg strut visible.
- [ ] Front view: narrow body, two pods flanking a low cowl, thin exposed flanks.
- [ ] Top view: the lance runs down the centreline and projects well past the leading foot.
- [ ] Rear view: recoil strut and open frame; no rear armour.
- [ ] Tactical framing: reads as sustained ranged fire, forward-facing, and not as a hero body.
