---
title: EBS-MER-UNT-004 Relay Skiff — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Relay Skiff

## Sources (read the images, not the notes about them)

| Role | File | sha256 |
|---|---|---|
| Selected candidate — the production direction (main three-quarter, side view, tactical top silhouette) | `…/Evidence/concept-discovery-20260906/relay-skiff-review/relay-skiff-candidate.png` | `6ba89f6907b01dea…` |
| Faction/style reference used to make it | `…/lancer-review/lancer-candidate.png` | `b5c85d32390c8f4c…` |
| Superseded, retained history only (`EBS-CON-MER-UNT-004`, decision REPLACE) | `…/Project/site/assets/concepts/meridian-units.png` bottom-right `[0.5, 0.5, 1, 1]` | `427e60cd27bd78e9…` |

Review decision (verbatim intent, `EBS-CON-MER-UNT-004` REPLACE): replace the fighter-like pointed canopy
and ornamental ring assembly with a thin-hulled utility skimmer, dominant mast/dish and an exposed
strapped archive cradle; retain the maintained ceramic equipment language. Owner note on the original:
"Read the book make sure this make sense." Recorded conflict, not resolved here: the book says it could
not fight, game canon gives a small secondary weapon; the weapon is retained per canon and the conflict
is carried in the README. Canon row `SPEC-UNIT-004` (Bible line 512) stays intact.

## Silhouette and proportion (measured on the candidate; L = 360 cm hull length)

Scale basis: PROVISIONAL. Logistics footprint 1 and a 500 cm/s skimmer that carries a cassette rack;
L = 360 cm reads as a small vehicle beside the 176 cm Surveyor without covering a whole 200 cm tile.

1. **Thin hull.** A long, low, flat-decked hull: length L, width ≈ 0.42 L, hull thickness ≈ 0.10 L. The
   plan silhouette is a stretched hexagon — parallel sides with a blunt chamfered nose and a squared
   tail. The deck is flat and open, not a canopy; nothing on it reads as a cockpit.
2. **Hover gap.** The hull floats clear of the ground: underside at ≈ 0.09 L, deck top at ≈ 0.19 L. The
   gap is part of the read (the runtime hovers the view actor; passability is unchanged).
3. **Lift pods.** Six serviceable pods slung under the hull edges (three per side), each a charcoal
   cylinder/box ≈ 0.14 L long with a cyan intake strip, hanging below the hull line so they read as
   removable service items.
4. **Relay mast and dish.** ONE tall mast rising from the deck at the FRONT-LEFT quarter: a segmented
   charcoal column to ≈ 0.55 L above the ground (the tallest thing on the asset, roughly 3× the hull
   thickness above the deck), a small parabolic dish ≈ 0.13 L across mounted on its side facing forward,
   two thin whip antennas above it and a slack cable loop from the dish back to the deck. Mast and dish
   dominate the silhouette: this is the "sees and connects" read.
5. **Archive cradle.** A sealed pale ceramic cassette rack lashed to the deck centre with two dark straps:
   a box ≈ 0.42 L long, ≈ 0.20 L wide, ≈ 0.13 L tall with rounded corner blocks, a small cyan status
   strip and visible strap hardware. It is a SEPARATE component (`deck_archive_*`) so the runtime can
   show or hide it; it never changes unless an authoritative event binds it (M01 only).
6. **Secondary emitter.** A small forward weapon under the nose, clearly secondary: a short charcoal
   barrel ≈ 0.11 L long in a small mount, no muzzle drama, no armoured housing.
7. **Palette.** Pale ceramic hull plates over charcoal structure, brass edge trim (texture), small cyan
   strips on the pods, mast, cradle and nose. Matte; nothing polished.

## Rig and tracks

- 8 bones (`REL-FAC-025.MC.SKIFF.ASSET` .ANIM_RIG) carrying a continuous harmonic hover bob: root, hull,
  mast, dish, and four pod bones (or two pod banks plus the emitter — the card fixes the count, not the
  names). No root motion; facing snaps instantly under Reduced Motion.
- Sockets required by the card: `Scouting_Sensor_Pod` (dish face) and `Logistics_Relay_Beam` (mast head,
  the origin of the Extend Relay cone toward a grid node within 700 cm).
- Tracks: idle hover (bob ≤ 4 cm), move (slight nose-down lean per canon), turn, stop, relay_extend (mast
  lights cyan and the dish turns toward the node), damage, death, cancel, restore.

## Budgets and rules that bound the concept (do not replace it)

`REL-FAC-025.MC.SKIFF.ASSET`: LOD0 ≤ 5,000 tris; LOD1 ≤ 2,100; 2048² PBR stack; cyan link conduit lines
≤ 8% of visible surface, un-bloomed and low saturation; hovering never alters simulation passability.
Nanite off; cm, +X forward, +Y right, +Z up; pivot at the ground point under the hull centre (NOT at the
hull, so the runtime's hover offset is additive). Record forced deviations in the README §8 with the rule.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Side view: long thin hull floating clear of the ground, mast and dish dominating, cradle on the deck.
- [ ] Top view matches the tactical silhouette: stretched hexagon hull, mast at the front-left, cradle centred.
- [ ] Six lift pods visible under the hull edges; secondary emitter small and forward.
- [ ] Cradle hidden variant renders with an empty strapped deck (loaded/unloaded read).
- [ ] Tactical framing: reads as a light utility scout, never as a hero body or a gunship.
