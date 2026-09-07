---
title: EBS-KHA-UNT-001 Tender — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Tender

## Sources (read the images, not the notes about them)

| Role | File | sha256 |
|---|---|---|
| Selected candidate — the production direction (front, rear with the sling, GATHERING, CULTIVATING) | `…/Evidence/concept-discovery-20260906/tender-review/tender-candidate.png` | (recorded in the package README §1 from the generation record) |
| Superseded, retained history only (`EBS-CON-KHA-UNT-001`, decision REPLACE) | `…/Project/site/assets/concepts/kharuun-units.png` top-left `[0, 0, 0.5, 0.5]` (crop at `…/concept-crops/all/EBS-PKG-KA-TENDER/EBS-CON-KHA-UNT-001.png`) | `88f1a44ffa34739d…` |

Review decision (verbatim intent, `EBS-CON-KHA-UNT-001` REPLACE): replace the spider-like machine with
hose manipulators. The story shows an ordinary cultivator using a staff, carrying a sling, and working
with broad forearms; expanded canon identifies a stocky humanoid Kharuun. Design living strata, amber
wrist nodules, staff and carried Matter. Avoid turning an ordinary person into a generic insect worker.
Canon row `SPEC-UNIT-005` (Bible line 551) stays intact.

## Silhouette and proportion (measured on the candidate; H = 190 cm at the crown)

Scale basis: PROVISIONAL. A stocky humanoid cultivator, heavier than the 176 cm Surveyor frame and
noticeably broad; H = 190 cm with a shoulder width ≈ 0.30 H reads correctly beside the Meridian units.

1. **Stocky humanoid.** An upright two-legged figure with a broad chest and heavy shoulders, thick
   forearms, short neck and a bald faceted crown. It stands square, weight even, not a crouched machine.
   Proportions from the candidate: shoulder width ≈ 0.30 H, hip width ≈ 0.22 H, head ≈ 0.13 H,
   arm span ≈ 1.0 H, leg length ≈ 0.48 H. Bare wide feet, no boots.
2. **Living strata skin.** The body reads as overlapping faceted basalt scales, darkest on the back and
   outer limbs, with thin amber seam lines between plates. No machined panels and no bolts anywhere
   (`REL-ART-007.FAIL` makes those a design failure on Kharuun assets); every edge is a grown facet.
3. **Thickened forearms.** Both forearms are visibly heavier than the upper arms (working strata), with
   a banded cuff and a cluster of amber nodules at the wrist. The nodules are the growth tell: they read
   bright only while the unit is growing a structure, dim otherwise.
4. **Resonance staff.** A staff carried in one hand, height ≈ 0.95 H, a plain shaft with a two-pronged
   forked head and an amber bead in the fork; the lower end is worn. It is a tool, never a weapon:
   the unit is unarmed (`SPEC-UNIT-005`, 0 damage).
5. **Woven sling.** A wide woven mineral-fibre sling worn diagonally across the back, holding a heap of
   dark rounded Matter nodules that sit proud of the weave. The sling and its load are a SEPARATE
   component group so the runtime can show loaded and empty (cargo capacity 10).
6. **Wrapped kilt.** A layered wrap at the hips with a woven geometric pattern and a plain belt with two
   pouches; the legs below it are bare strata.
7. **Palette.** Charcoal and slate strata with warm amber seams and nodules; matte throughout. No cyan
   anywhere — cyan is the Meridian Compact's colour.

## Rig and tracks

- Humanoid rig sized to the work: root, pelvis, spine, chest, neck, head, both clavicle/shoulder/
  forearm/hand chains and both thigh/shin/foot chains, plus a staff bone parented to the working hand
  and a sling bone on the chest. Bone count follows the Surveyor's precedent of spending the budget on
  the readable joints; the exact count is recorded in the README because no Tender asset card exists.
- Sockets: `Harvest_Tether_Muzzle` (staff head, the gather/effect origin), `Cargo_Drop_Anchor` (sling
  centre), `Center_Hitbox_Socket` (chest centre) — the worker socket set already used by the Surveyor.
- Tracks (canon, Bible line 551): idle, move, turn, stop, **gather** (a KNEELING press of the staff into
  the strata, as the GATHERING panel shows: one knee down, both hands on the staff, sparks at the
  contact point), **grow** (a slow circling walk that leaves the organism's first ring — the CULTIVATING
  panel: upright walk, staff trailing, amber ring rising behind), deliver, stabilize_scar (continuous
  channel, 120 ticks), damage, death, cancel, restore. No root motion: the circling walk of `grow` is
  authored in place and the runtime drives the path.

## Budgets and rules that bound the concept (do not replace it)

No Tender asset card exists in Requirements.md (`REL-FAC-026.KA.TENDER` records only the gameplay
metrics). Bound the build by the equivalent worker card (`REL-FAC-025.MC.SURVEYOR.ASSET`: LOD0 ≤ 4,500,
LOD1 ≤ 1,800, 2048² stack, emissive ≤ 5% of area) and record the missing card as an OWNER-QUESTION.
`REL-ART-007` governs the form language: faceted basalt strata, hexagonal columns, translucent amber
nodules, living root anchors; machined panels or industrial bolts are a failure. Nanite off; cm, +X
forward, +Y right, +Z up; root at ground-contact centre. Record forced deviations in README §8.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Front view: stocky upright humanoid, broad shoulders, thick forearms, staff in hand, bare feet.
- [ ] Rear view: woven sling across the back with the Matter load proud of the weave.
- [ ] Gather pose matches the GATHERING panel: kneeling, staff pressed into the ground ahead.
- [ ] Grow pose matches the CULTIVATING panel: upright circling walk with the ring behind.
- [ ] Empty-sling variant renders (loaded/unloaded read); amber nodules bright only while growing.
- [ ] No machined panel, bolt or cyan anywhere in any view.
