---
title: EBS-KHA-BLD-004 Listening Spine — concept fidelity target
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
status: AUTHORITATIVE production target (owner ruling 2026-09-06: the concepts define what every asset should be)
---

# Concept fidelity target — Listening Spine

## Sources (read the images, not the notes about them)

| Role | File |
|---|---|
| Selected candidate — the production direction (LISTENING, DETECTING, ROOTED SOCKET) | `…/concept-discovery-20260906/listening-spine-review/listening-spine-candidate.png` |
| Concept inputs | `EBS-CON-KHA-BLD-004` and `EBS-CON-KHA-BLD-008`, cropped to `…/concept-crops/all/EBS-PKG-KA-LISTENING-SPINE/` |

Canon row `SPEC-BLD-016` Listening Spine: a single tall rib of strata with amber sensor nodules climbing
it, set into a rooted socket. **A spine, not a weapon.** Detecting shows nodules lighting in sequence
toward the source direction. Sounds: a slow pulse that quickens with movement signatures.

## Form and proportion (traced on the candidate's LISTENING view)

Scale basis: `buildings.json` `ka_listening_spine.footprint_cells` = 2×2 tiles = 400 cm square.
A silhouette trace of dark-pixel row spans gives:

| Measured on the candidate | Value |
|---|---|
| root disc width | 404 px (the reference length) |
| total height ÷ root disc width | **1.87** |
| rib width at its base ÷ total height | **0.176** |
| tip lateral offset ÷ total height | **0.125** |

The rib is not vertical. It curves, and its tip finishes measurably off the axis of its base — that
curve is most of the silhouette's character.

1. **One tall rib, curved and tapering.** A single horn-like rib rising from the socket, widest just
   above the roots and narrowing to a point. Faceted, built of visible plate courses spiralling up it,
   with zero organic smoothing (`REL-ART-029`).
2. **Amber sensor nodules climbing the front face.** A single line of nodules up the rib's leading
   edge, largest low down and smaller toward the tip. Eleven, counted on the candidate. Each is its own
   component so the runtime can address them individually.
3. **A rooted socket at the base.** A stepped stone collar with roots splaying flat across the ground —
   the same rooted language as the Waystone, but a socket that never lifts.
4. **The sequence is the information.** Detecting lights the nodules in order toward the source. The
   sequence itself is a per-nodule material animation the runtime drives; the geometry's job is to make
   every nodule individually resolvable, and the rib leans toward the contact.
5. **States.** Listening, contact, damaged, destroyed.
6. **Palette.** Dark plated strata with warm amber nodules; matte. **No dish, antenna, radar or mast
   language** — canon says a spine, not a weapon, and nothing here may read as one.

## Rig and tracks

Per the provisional card `REL-BLD-016.KA.SPINE.ASSET`: `root`, `socket`, and a two-bone rib that can
lean toward a detected direction — four bones. It is **not** a turret rig: the rib leans, it does not
aim, and there is no yaw ring, no pitch trunnion and no muzzle.

Tracks: `idle_pulse` (a slow lean cycle, the visual partner of canon's slow pulse), `detect_sweep` (the
rib leans toward the source while the nodules run their sequence), `offline`, `restore`.

## Budgets and rules that bound the concept

Provisional card: LOD0 ≤3,000 / LOD1 ≤1,200 for the complete assembly including the rooted socket —
well under the faction default because canon specifies one rib in a socket, and detail no LOD could keep
would be wasted. 2048² packed PBR. Amber ≤15% of surface area as a **ceiling, not a target**. Nanite
off; cm, +X forward, +Y right, +Z up; pivot at the socket's ground-contact centre. Everything inside the
2×2 footprint, and the flat roots respect `REL-ART-030`'s 20 cm ceiling on decorative ground displacement.

## Fidelity checks (each shown in a render beside the concept crop)

- [ ] Listening three-quarter: one curved tapering rib, nodules dark, socket rooted.
- [ ] Contact: every nodule lit, the rib leaned toward the source.
- [ ] Front orthographic: height 1.87 of the root disc, tip offset 0.125 of the height.
- [ ] Socket detail: the stepped collar and flat roots, answering the candidate's ROOTED SOCKET panel.
- [ ] Top view: everything inside the 2×2 square.
- [ ] Tactical framing: it reads as a spine and never as a weapon.
