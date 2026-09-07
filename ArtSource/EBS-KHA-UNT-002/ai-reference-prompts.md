---
title: EBS-KHA-UNT-002 Riftstalker — image-generation prompts for the concept-fidelity pilot
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: Prompts for the owner to run in Gemini / ChatGPT. Outputs are REFERENCE INPUTS to the pipeline, not assets; the registered concept stays the authority.
---

# How to use these

Attach **`BuildArtifacts/Evidence/concept-discovery-20260906/riftstalker-review/riftstalker-candidate.png`**
to every prompt. Run prompts 1–4 in order; each later one may also attach the result of prompt 1.
Save outputs as PNG into `BuildArtifacts/Evidence/concept-discovery-20260906/riftstalker-review/ai-reference/`
with the file names given. If a result changes the design (adds a head, a neck, a fifth leg, softens the
facets, moves the caster), discard it and re-run — the concept is the authority, not the generator.

Rules that apply to every prompt (paste them; they are the contract in words):
- Four long three-segment legs, knees high and outboard, small pointed feet. No neck, no head: the body
  tapers forward into a prow. Body length ÷ standing height = 1.46 (292 cm long, 200 cm tall at the
  carapace top). A lean skirmisher that visibly lacks the mass for a frontal fight.
- Five overlapping faceted carapace shells down the back, charcoal, hard-edged, zero organic smoothing.
  Amber seams between shells only. A shoulder-mounted shard-caster: an amber-lit slot in the shoulder
  carapace aimed forward past the prow, part of the carapace, not a bolted-on gun.
- Palette: charcoal body (very dark, 0.02–0.07 linear); Broken-Sun Amber for seams and the caster slot
  (warm gold, ember-dim, matte, never white); no cyan, no magenta, no metallic sheen.
- Neutral flat grey background, even studio light, no dramatic lighting, no text, no labels, no props,
  no ground shadow, no motion blur, no projectiles.

---

## Prompt 1 — orthographic turnaround sheet  → `turnaround_baseline.png`

> Using the attached concept as the only design authority, draw a single orthographic turnaround sheet
> of this creature at the same scale in every view: FRONT, LEFT SIDE, BACK, TOP, arranged left to right
> in one row, feet on one common baseline, each view centred in an equal-width cell. True orthographic
> projection with no perspective. Keep every proportion of the concept's side view: body length 1.46
> times the standing height, four long three-segment legs with knees high and outboard, small pointed
> feet, no head and no neck, the body tapering forward into a prow. Five overlapping faceted charcoal
> carapace shells down the back with thin amber seams between them; a shoulder-mounted shard-caster slot
> lit amber, aimed forward past the prow, integrated into the carapace. Hard-edged mineral facets
> everywhere, no smooth organic surfaces. Charcoal body, amber seams only, matte. Neutral flat grey
> background, even studio lighting, no shadows on the ground, no text, no labels. Square image,
> maximum resolution.

If the sheet comes back with perspective, or the views at different scales, re-run with: *"All four
views must share one scale and one baseline; measure the standing height identically in each."*

## Prompt 2a — carapace molt variant  → `turnaround_carapace_molt.png`

> Attached: the concept and the turnaround sheet. Produce the SAME turnaround sheet, same four views,
> same scale, same baseline, with exactly one change: the creature has undergone a carapace molt. Each
> of the five back shells now carries an additional thicker faceted plate grown over it, in the same
> charcoal mineral, slightly lighter and less fractured than the old shells (new growth), with the
> amber seams still visible between shells. Nothing else changes: same legs, same prow, same caster,
> same silhouette length and height. No new colours.

## Prompt 2b — striker molt variant  → `turnaround_striker_molt.png`

> Attached: the concept and the turnaround sheet. Produce the SAME turnaround sheet, same four views,
> same scale, same baseline, with exactly one change: the creature has undergone a striker molt. Two
> faceted mineral vanes have grown from the shoulder carapace, one each side of the shard-caster slot,
> sweeping back along the flanks, in the same charcoal mineral with a single amber seam along each
> vane's root. The caster slot itself is unchanged. Nothing else changes. No new colours.

## Prompt 3 — surface tiles  → `tile_obsidian.png`, `tile_fresh_growth.png`, `tile_amber_seam.png`, `tile_foot_wear.png`

Run four times, one line each, same preamble:

> Produce a single seamless, tileable, square texture at maximum resolution, flat even lighting, no
> shadows, no vignette, no text, that reads as the surface of the attached creature's carapace:
> - **tile_obsidian**: charcoal volcanic obsidian, hard faceted cells about a hand's width across with
>   thin fractured borders, faint warped strata banding, fine mineral grit; very dark overall; the crack
>   bottoms carry an ember-dim warm tint, matte, not glowing.
> - **tile_fresh_growth**: the same obsidian as new growth after a molt: fewer fractures, slightly
>   lighter, smoother, cleaner facets.
> - **tile_amber_seam**: a horizontal seam of Broken-Sun Amber between two charcoal plates: an
>   ember-dark amber base with a brighter warm-gold core line along its centre, matte, never white.
> - **tile_foot_wear**: the obsidian of a foot and lower leg: dust and abrasion, pale grit lodged in
>   the crack bottoms, facets rubbed dull at their edges.

## Prompt 4 — detail callouts  → `detail_shoulder_caster.png`, `detail_seam_profile.png`, `detail_leg_joint.png`

> Attached: the concept and the turnaround sheet. Draw three close-up orthographic detail studies on
> one sheet, each in its own cell on a neutral grey background, same faceted charcoal mineral and amber
> palette, no text: (1) the shoulder shard-caster slot from the side and from the front, showing how
> the amber-lit slot sits inside the carapace facets; (2) a cross-section profile through two
> overlapping back shells showing the amber seam between them and the facet bevels; (3) a hind-leg
> knee joint from the outside, showing the three leg segments, the joint facets and the small pointed
> foot.

---

# What happens to the outputs

- Turnarounds → the alignment authority for the Blender pass (silhouette fitting, per-view) and the
  input to a 3D generator if one is authorised.
- Tiles → authored layers for the texture baker in place of procedural noise; they are baked into the
  existing 2048² atlas with the same emissive-by-area check (≤15%).
- Callouts → the geometry recipes for the caster housing, seam bevels and leg joints.

Every output is checked against `concept-fidelity.md` before it is used; anything that contradicts the
registered concept is rejected, and the rejection is recorded.
