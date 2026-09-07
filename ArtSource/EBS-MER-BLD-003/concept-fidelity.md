---
title: EBS-MER-BLD-003 Array Foundry — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Array Foundry

## Sources (read the images, not the notes about them)

| Role | File | sha256 |
|---|---|---|
| Selected candidate — the production direction (PRODUCING, RESEARCHING, INTERRUPTED) | `…/concept-discovery-20260906/array-foundry-review/array-foundry-aligned-output-candidate.png` | (recorded in the README §1 from the generation record) |
| Concept A, KEEP (`EBS-CON-MER-BLD-003`) | `…/site/assets/concepts/meridian-structures.png` bottom-left `[0, 0.5, 0.5, 1]` | `5b64820bbd2bbc28…` |
| Concept B, REWORK (`EBS-CON-MER-BLD-007`) | `…/site/assets/concepts/echoes-meridian-structures.jpg` bottom-left `[0, 0.5, 0.5, 1]` | `b6396728d4fd2a49…` |

Review decisions (verbatim intent): A **KEEP** — retain the long open fabrication hall, structural rails
and the visible work-to-output sequence; production still needs intelligible intake/output and research
gantry states. B **REWORK** — retain overhead gantries, service access and visible fabrication work, but
reorganise the stacked workshop into the Bible's long intake/work/output hall with a separate readable
roof research gantry, and reduce the sparks and tiny machinery that conceal the principal unit-production
path. Canon row `SPEC-BLD-015.MC.FOUNDRY` (Bible line 515) stays intact.

## Form and proportion (the candidate is the reconciled direction; the 4×4 footprint is the outer bound)

Scale basis: `buildings.json` `mc_array_foundry.footprint_cells` = 4×4 tiles = 800 cm square at the
200 cm presentation tile. The candidate is a LONG hall with a ramp at each end, so the hall itself is
shorter than the footprint and the ramps use the remaining length; nothing crosses the square.

1. **Long rectangular hall.** One straight hall running along +X: length ≈ 0.80 of the footprint
   (640 cm), width ≈ 0.38 (300 cm), wall height ≈ 0.26 (210 cm) with the roof deck above it. Pale
   ceramic panels over a charcoal frame, pilasters and conduit runs along both flanks, cyan strip lights
   at deck level. The hall is the dominant mass; nothing else competes with it.
2. **Intake hood and ramp (−X end).** An arched pale hood over the intake mouth, wider and taller than
   the hall wall, with a dark recessed opening; a ramp runs down from it to the ground and ends inside
   the footprint. This is where workers and Matter go in.
3. **Open fabrication bay (middle).** The middle third of the roof is OPEN: two heavy longitudinal rails
   on gantry legs run the length of the bay, and a half-built frame hangs from a carriage on the rail.
   The open bay and the frame on the rail are the whole point of the building — the player reads
   production from it.
4. **Output door (+X end).** A tall rectangular door in a framed portal at the far end with its own
   ramp; cyan edge lighting. Intake and output are at OPPOSITE ends, so the work sequence reads left to
   right.
5. **Roof research gantry.** A raised platform over the output end carrying a cluster of slim
   instruments (a mast, dishes, probes). It is separate from the fabrication rail and it is what lights
   during research.
6. **States.** PRODUCING: the carriage carries a half-built frame along the rail toward the door; the
   roof gantry is dark. RESEARCHING: the gantry instruments light cyan and the rail is empty and
   stopped. INTERRUPTED: the gantry dims and a partial frame sits stationary on the rail — no refund
   animation.
7. **Palette.** Pale ceramic, charcoal frame, brass trim and yellow hazard striping on the ramp edges
   (texture); cyan only in the strip lights, the door edge, the bay interior and the research gantry.

## Budgets and rules that bound the concept

`REL-BLD-015.MC.FOUNDRY`: LOD0 ≤8,000 / LOD1 ≤3,400; 4×4 footprint; 2048² PBR with grit and grease
layers; operational states reveal interior assembly lights; a research progress band on the exterior;
matte roughness floor ≥0.85. `REL-ART-028` bounds the Meridian roster at 8,000 / 3,500, which agrees
with the card at LOD0 and is looser at LOD1, so the card's 3,400 governs. Owner ruling 2026-09-07: the
ceilings apply to the complete asset including any articulated components. `REL-ART-006` engineered form
language. Nanite off; cm, +X from intake to output, +Y right, +Z up; pivot at the ground-contact centre
of the hall. Collision on the hall mass only — the ramps are approaches, not blockers.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Three-quarter view: one long hall with an arched intake hood at one end and a framed output door
      at the other, an open middle bay with rails, and a research gantry on the roof over the output end.
- [ ] Side view: the rail runs the length of the open bay with a frame carriage on it.
- [ ] Top view: intake ramp, hall, output ramp on one axis; everything inside the 4×4 square.
- [ ] The three states differ where the concept says: rail carriage, gantry light, stationary frame.
- [ ] Tactical framing: the work-to-output direction is readable at the game camera.
