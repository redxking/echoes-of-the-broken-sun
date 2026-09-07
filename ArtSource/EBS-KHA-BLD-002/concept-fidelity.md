---
title: EBS-KHA-BLD-002 Waystone — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Waystone

## Sources (read the images, not the notes about them)

| Role | File |
|---|---|
| Selected candidate — the production direction (ROOTED, MIGRATING, ROOT RETRACTION) | `…/concept-discovery-20260906/waystone-review/waystone-candidate.png` |
| Concept inputs | `EBS-CON-KHA-BLD-002` and `EBS-CON-KHA-BLD-006`, cropped to `…/concept-crops/all/EBS-PKG-KA-WAYSTONE/` |

Canon row `SPEC-BLD-016` Waystone: a tall faceted monolith of dark strata with amber seams that, rooted,
sinks a visible ring of root-strata into the ground; uprooted, it lifts on a grown carriage and moves
slowly. Rooted or moving is visible from the roots alone. Rooting has four beats: preparation, contact,
settling, release.

## Form and proportion (traced on the candidate's two main views)

Scale basis: `buildings.json` `ka_waystone.footprint_cells` = 2×2 tiles = 400 cm square. Silhouette
traces of dark-pixel row spans give:

| Measured on the candidate | Value |
|---|---|
| ROOTED root spread | 522 px (the reference length) |
| total height ÷ root spread | **1.09** |
| monolith shaft width ÷ root spread | **0.25** |
| shaft share of the total height (above where the root mass flares) | **0.56** |
| MIGRATING carriage-plate width ÷ rooted root spread | **0.92** |

The migrating footprint is measurably *narrower* than the rooted one. That is the retraction reading as
geometry, and it is the asset's primary job.

1. **A tall faceted monolith of stacked courses.** The shaft is built of visible stone courses, roughly
   square in plan and slightly tapered, each course a straight-sided prism. Zero organic smoothing
   (`REL-ART-029`).
2. **Amber seams, banded not scattered.** Horizontal amber seams ring the shaft between courses, with a
   few small nodules. Amber is the accent, never the mass.
3. **A stepped root plinth.** Two or three stacked courses wider than the shaft, where the roots meet it.
4. **Roots that splay when rooted and braid when mobile.** ROOTED: many roots radiate out and lie flat
   across the ground in a wide disc. MIGRATING: the same roots retract and braid into vertical bundles
   hugging the plinth. The ROOT RETRACTION detail panel shows exactly this pair.
5. **A carriage plate on four stubby legs.** Present only while migrating. Four legs, not three —
   counted on the candidate's MIGRATING view. The legs are stacked mineral courses, not machined feet.
6. **States.** Rooted, uprooted and mobile, damaged, destroyed.
7. **Palette.** Dark grey strata with warm amber seams; matte. No wheels, tracks, treads, pistons or any
   machined carriage; nothing Meridian.

## Rig and tracks

Per the provisional card `REL-BLD-016.KA.WAYSTONE.ASSET`: `root`, `ring_sink`, `carriage_lift` and four
feet. The roots' splay-versus-braid is a **state**, not a bone animation — the bones carry the sink and
the lift, and the state carries the root form, because a braid is not a pose of a splay.

Tracks: `rooted_idle` (loop), `uproot` (40 ticks = 2.0 s), `mobile_move` (loop), `root` (60 ticks =
3.0 s, in canon's four beats), `restore`. The project runs 20 ticks per second, taken from the
Waystone's own 100-tick / 5.0 s construction figure. No root motion; the runtime owns translation.

## Budgets and rules that bound the concept

Provisional card: LOD0 ≤5,000 / LOD1 ≤2,200 for the complete assembly including the root ring and the
carriage. 2048² packed PBR. Amber ≤15% of surface area as a **ceiling, not a target**. Nanite off; cm,
+X forward, +Y right, +Z up; pivot at the ground-contact centre under the monolith. Everything stays
inside the 2×2 footprint in every state, and the flat roots respect `REL-ART-030`'s 20 cm ceiling on
decorative ground displacement so they never alter passability.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Rooted three-quarter: tall coursed shaft, amber seam bands, roots splayed flat in a wide disc.
- [ ] Migrating three-quarter: roots braided up, carriage plate, four legs, body lifted clear.
- [ ] The two side by side: the migrating footprint is visibly narrower than the rooted one.
- [ ] Front orthographic: total height 1.09 of the root spread, shaft width 0.25 of it.
- [ ] Top view: everything inside the 2×2 square in both states.
- [ ] Tactical framing: rooted or moving is readable from the roots alone.
