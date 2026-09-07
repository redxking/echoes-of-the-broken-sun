---
title: EBS-MER-UNT-002 Lancer — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
amended: concept-v1 2026-09-07 (brace span, pod separation, leg split, antennas, lance projection); concept-v2 2026-09-07 (fore-aft placement of the upper assembly, per-section lance depth and cyan continuity, ceramic on outer faces only, the strut as a per-clip constraint)
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
   stance: the leading leg is planted ahead and the trailing leg back, ~~feet ≈ 1.0–1.2 H apart along +X~~
   and only ≈ 0.35 H apart across +Y. The body is carried low between the hips and pitched ~8° forward.
   **[corrected concept-v1, 2026-09-07, measured on the turnaround LEFT SIDE panel at 0.545 cm/px]** the
   fore-and-aft brace measures **0.63 H between the foot centres and 0.87 H heel-to-toe**, not 1.0–1.2 H;
   the same measurement on the candidate's braced side view gives 0.745 H / 0.88 H. Built: 0.64 H / 0.86 H.
   The brace is a **mirror brace**: the leading knee bows forward of the hip–ankle line and the trailing
   knee bows back, so the trailing leg reads as a kickstand under recoil (measured, both panels).
2. **Rail-lance.** ONE long lance carried at hip height on a hip mount at the body's front, axis parallel
   to the ground and along +X. Total lance length ≈ 1.05 H; ~~≈ 0.60 H of it projects ahead of the leading
   foot~~. The lance is a stack of two parallel rails over a charcoal body with a continuous cyan channel
   running its full length between them, a blocky muzzle head and three collar clamps along its length.
   Diameter of the whole assembly ≈ 0.10 H. It reads as the heaviest single element in the silhouette.
   **[clarified concept-v1, 2026-09-07: "ahead of the leading foot" is ambiguous. Measured 0.51 H ahead of
   the leading TOE and 0.71 H ahead of the leading ANKLE; built 0.55 H / 0.66 H.]**
   **[corrected concept-v2, 2026-09-07, measured column by column on the candidate BRACED SIDE panel at
   0.514 cm/px: "≈ 0.10 H" is the whole-assembly figure, but the lance is NOT one constant section. The
   clear shaft measures 19.0 cm = **0.095 H**, the thickest point (a collar) 22.1 cm = **0.110 H**, and the
   muzzle end 17.5-19.0 cm; only the breech/mount block reaches 0.13-0.15 H. Built 0.095 / 0.110 / 0.095 H.
   The collars sit ABOVE and BELOW the cyan band, not across it, so the channel survives as ONE stroke in
   the LEFT SIDE and TOP panels — that continuity is part of the target, not just the geometric length.]**
3. **Low sensor cowl.** A low wedge head set BETWEEN the shoulder pods with a horizontal cyan visor slot,
   no neck: the cowl top is level with or just under the pod tops, and it sits at ≈ 0.90 H.
4. **Shoulder pods.** Two pale ceramic boxy pods flanking the cowl at ~~≈ 0.55 H apart~~, tops at H, each
   ≈ 0.22 H long and ≈ 0.13 H tall, with a small cyan strip. They are the widest part of the upper body.
   **[corrected concept-v1: the front and rear quarter panels measure ≈ 0.41–0.45 H across the whole pod
   pair, so the pod CENTRES are ≈ 0.30 H apart, not 0.55 H. Built: centres 0.30 H apart, 0.44 H across.]**
   **[added concept-v1: two thin antenna rods rise from the pods, tips at 1.065 H — they are in both the
   candidate and the turnaround and were missing from this list. H stays defined at the pod tops.]**
5. **Exposed flanks.** The body between the pods and the hips is open charcoal frame: struts and joint
   hubs are visible from the side, no ceramic skirt. This is the readable weak read the canon requires.
   **[added concept-v2, 2026-09-07: the list never said WHERE the upper assembly sits fore-and-aft, and the
   concept-v1 build cantilevered it forward over the lance yoke. Both side panels, segmented and scaled so
   H = 200 cm with the ground on the sole line and the fore-aft axis anchored on the trailing heel and the
   leading toe, put the 0.80–1.00 H silhouette band's centroid at **−30.4 cm** (turnaround LEFT SIDE) and
   **−30.2 cm** (candidate BRACED SIDE), with its rear edge at −75.4 / −72.6 cm and its front edge at
   +16.6 / +13.4 cm. The frame is hunched and weight-back: the deck, both pods and the cowl are carried
   BACK over the hips, and the cowl's front face stays behind the lance yoke. Built: centroid −28.4 cm,
   band −75.0 … +18.0 cm.]**
6. **Rear-leg recoil strut.** A thin charcoal strut running from the lower rear of the body diagonally
   down and back to the trailing ankle, with a visible slide/piston section at its middle. It is present
   in the side, rear and top views and must read as a separate slim member, not a leg plate.
7. **Legs.** Digitigrade: ~~thigh forward-down ≈ 0.28 H, shin back-down ≈ 0.26 H~~, prominent knee hub
   (disc ≈ 0.08 H diameter), block foot ~~≈ 0.20 H long~~ with a raised toe plate. Pale plate on the thigh
   and shin outer faces ONLY — the concept's REAR panel shows no ceramic on the rear-facing leg surfaces —
   charcoal hubs. **[corrected concept-v1: the LEFT SIDE panel measures thigh
   ≈ 0.31 H and shin ≈ 0.24 H (total 0.55 H, which the 0.28/0.26 split also gives); built 0.30 H / 0.25 H.
   The ankle sits ≈ 0.135 H above the ground on a chunky pastern, and the foot measures 0.22 H long.]**
8. **Palette.** Pale ceramic plates, charcoal frame, brass-toned edge trim (texture, not geometry), cyan
   only on the lance channel, the visor and small strips. Nothing on it reads as a hero body.
   **[added concept-v2, 2026-09-07: the ceramic is a set of OUTER FACES, not a wrapping. A 2.2x read of the
   turnaround REAR panel shows hub stacks, struts and brass edges only — the pale plate appears from behind
   solely as a cap on the top quarter of each pod and a strip on the central block. Measured on the panels
   at lum > 0.55 x background: ceramic-bright share of the silhouette FRONT 0.204, REAR 0.116 (rear/front
   0.57). concept-v1 built 0.372 / 0.323 (0.87); concept-v2 builds 0.265 / 0.107 (0.40) — the rear now
   matches the concept, the front stays high because the flat blockout has no brass or seam texture yet.]**

## Rig and tracks

- 18 bones (`REL-ART-005.MC.LANCER` .ANIM_RIG), including `lance_yaw` (the card's `Turret_Y` sub-object)
  and `lance_barrel` (`Barrel_X`), the cowl, both legs (thigh/shin/foot), both pods, and the two-part
  recoil strut. No root motion; the runtime owns facing (`SPEC-MOV-010`).
- Sockets required by the card: `Muzzle_Flash_01` (lance muzzle), `Target_Anchor_Center` (body centre),
  `Left_Tread_Vector` (card name; this frame walks on legs, so it is placed at the left foot ground
  contact and the naming conflict is recorded as a deviation, exactly as for the Surveyor's tread wording).
- Tracks per canon: idle, move, turn, stop, **halt-plant-aim-fire-recover** (the card's line: never fires
  while moving; recoil returns through the mount and the strut slides), damage, death, cancel, restore.
  **[added concept-v2, 2026-09-07: item 6's strut is BOLTED to the trailing ankle, so it is a constraint on
  every clip, not just on the rest stance. Its lower end must follow the trailing foot in every braced,
  planting and firing state; where a walking leg swings forward past the body anchor no strut can both stay
  bolted and stay clear of the frame, so the piston draws home instead (see README §8.7).]**
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
- [ ] Side view: the deck, pods and cowl sit BACK over the hips (0.80–1.00 H band centroid ≈ −30 cm), not
      cantilevered forward over the lance yoke.
- [ ] Side view: the cyan channel reads as ONE stroke past every collar, and the lance's clear shaft is
      0.095 H with its thickest collar at 0.110 H.
- [ ] Every clip: the recoil strut's lower end is on the trailing ankle, or the piston is drawn home.
- [ ] Front view: narrow body, two pods flanking a low cowl, thin exposed flanks.
- [ ] Top view: the lance runs down the centreline and projects well past the leading foot.
- [ ] Rear view: recoil strut and open frame; no rear armour.
- [ ] Tactical framing: reads as sustained ranged fire, forward-facing, and not as a hero body.
