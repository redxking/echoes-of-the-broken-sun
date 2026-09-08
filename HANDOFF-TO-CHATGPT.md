---
title: Echoes of the Broken Sun — art production handoff
author: Angelis Pseftis
created: 2026-09-08
purpose: Brief for another AI to take the visual assets from BLOCKOUT to production-ready 3D models.
---

# Handoff: take these assets from blockout to production

You are taking over the 3D art production for **Echoes of the Broken Sun**, a real-time strategy game
built in **Unreal Engine 5.8.2** on macOS (Apple M1 Pro, 16 GB). Twenty-one asset packages exist as
verified blockouts with full contract compliance. **None of them look like the concept art yet.** Your
job is to close that gap and deliver production-ready models. How you do it is your decision.

---

## 1. What the game is

Three factions, each with a strict visual grammar that is canon and must not be violated:

| Faction | Material language | Emissive accent | Ceiling |
|---|---|---|---|
| **Kharuun** | Grown mineral: faceted basalt/obsidian, layered strata, **zero organic smoothing** | Broken-Sun Amber, matte, ember-dim | ≤15% of surface area |
| **Meridian Compact** | Engineered: pale civic ceramic + machined charcoal metal | Cyan | ≤15% |
| **Hollow Choir** | Vitrified glass, reality-bleed, unsupported floating forms | Magenta Fracture | ≤12% |

Master palette (every colour must be attributable to one of these): **Charcoal** (terrain/structure body,
0.02–0.07 linear, roughness floor 0.85), **Pale ceramic** (~0.86 tone, roughness 0.34, metallic 0.04),
**Broken-Sun Amber/gold** (warm accent; key light 1.0/0.82/0.62; ember weighting 0.50/0.22/0.06),
**Magenta-Fracture**, **Cyan**, with **Indigo** as fill-light complement (0.48/0.60/0.88). The signature
is a gold key against an indigo fill.

Everything is authored in **centimetres, +X forward, +Y right, +Z up**, Nanite off, pivot at the
contract point (usually ground-contact centre).

---

## 2. What is already done — 21 packages at BLOCKOUT

Every package has: deterministic generator source, LOD0 + LOD1 exports (76 GLB/OBJ files), a traced
concept-fidelity target, a README, a structural test suite, evidence renders, and a **clean headless
Unreal import** with sockets, rig and material slots verified.

| ID | Subject | LOD0 / LOD1 tris |
|---|---|---|
| EBS-KHA-UNT-001 | Tender (worker) | 552 / 408 |
| EBS-KHA-UNT-002 | **Riftstalker** (skirmisher, furthest along) | 542 / 346 |
| EBS-KHA-UNT-003 | Cairnback (heavy) | 908 / 288 |
| EBS-KHA-UNT-004 | Resonant (support) | 736 / 540 |
| EBS-KHA-BLD-001 | Memory Hearth (HQ) | 1554 / 634 |
| EBS-KHA-BLD-002 | Waystone | 972 / 544 |
| EBS-KHA-BLD-003 | Growth Basin | 1162 / 784 |
| EBS-KHA-BLD-004 | Listening Spine | 602 / 292 |
| EBS-MER-UNT-001 | Surveyor (worker) | 1932 / 1116 |
| EBS-MER-UNT-002 | Lancer | 1972 / 1136 |
| EBS-MER-UNT-003 | Bulwark Team | 2898 / 1126 |
| EBS-MER-UNT-004 | Relay Skiff | 1956 / 1340 |
| EBS-MER-BLD-001 | Anchor (HQ) | 1840 / 960 |
| EBS-MER-BLD-002 | Power Link (textured pilot) | — |
| EBS-MER-BLD-003 | Array Foundry | 648 / 480 |
| EBS-MER-BLD-004 | Aegis Post | 496 / 352 |
| EBS-HOL-BLD-001 | Concordance | 1144 / 392 |
| EBS-HOL-BLD-002 | Interval Loom | 756 / 420 |
| EBS-HOL-BLD-003 | Chorus Loom | 292 / 232 |
| EBS-HOL-BLD-004 | Phase Anchor | 254 / 158 |
| EBS-FWL-SYS-001 | Future Well (4-state family) | — |

**These triangle counts are the problem, not an achievement.** The asset cards allow 4,000–8,500
triangles at LOD0. The blockouts use a fraction of that because they are boxes, prisms and tubes.
There is enormous unused budget.

**Concept art exists for all 21** as painted multi-panel sheets (front/side/state/detail views) in
`BuildArtifacts/Evidence/concept-discovery-20260906/<name>-review/<name>-candidate.png`. Each package's
`concept-fidelity.md` records measurements traced from its concept (proportions, component counts,
state reads) and any deviation forced by the gameplay footprint.

### Contracts each asset must satisfy
Per-asset "cards" specify LOD0/LOD1 triangle ceilings, a 2048² PBR texture stack (Albedo, Normal,
packed Roughness/Metallic, Emissive Mask), material rules, bone counts, **named sockets**, states
(working/damaged/destroyed, plus role-specific states), and readability requirements at the tactical
camera. Ten of these cards are **provisional** (authored during blockout, not yet in the authoritative
requirements document) and are bundled in `ArtSource/provisional-cards-reconciliation.md`.

---

## 3. The one package taken furthest: Riftstalker

Use it as the reference for what "further along" currently means, and as the pilot to beat.

- **Textures**: a full 2048² stack baked from a unique UV atlas (448 charts, 2.03 px/cm, 81% packed) —
  BaseColor, Normal, packed MRE, StateMask, plus a 512² molt-blend mask. Amber emissive measured on the
  baked mask at 3.8% of surface area against the ≤15% ceiling. Obsidian body measured at 0.029–0.065
  linear, inside the Charcoal anchor on all 311 obsidian charts.
- **Vertex ID channels**: `COLOR_0.R` = molt sweep order (graded nose-to-tail), `.G` = Carapace
  adaptation membership, `.B` = Striker membership, `.A` reserved. Verified end-to-end through export,
  Unreal import and material response for all three molt states. **Team ownership is deliberately in no
  vertex channel** — it rides a separate `TeamColor` parameter and a StateMask band, so a molting unit's
  owner is never ambiguous.
- **Geometry detail tier**: an in-Unreal pass (weld → remove hidden faces → polygroups → bevel →
  regenerate UVs) produced a **3,907-triangle chamfered mesh** inside the 7,500 ceiling, carrying the
  22-bone rig and the COLOR_0 channels through the topology change, with normal/AO/curvature baked from
  a 15,628-triangle high-poly.
- **Rendered in Unreal** offscreen at close, three-quarter and tactical framings.

**It still does not match its concept.** It reads as chamfered boxes with a good mineral surface, not as
the sculpted, layered, mineral-organic creature the concept shows.

---

## 4. Tooling that already exists (use it or replace it — your call)

All pure-Python 3 standard library unless noted. In `ArtSource/tools/`:

- `ebs_meshkit.py` — deterministic mesh construction, UV atlas packing, GLB/OBJ export, optional
  vertex colours. **Writes unindexed triangles** (weld before any topological operation).
- `ebs_skelkit.py` — skeletons, skinning, animation clips, skinned GLB export.
- `ebs_texbake.py` — procedural PBR baker driven by a per-chart bake manifest; surface families for
  each faction; measures emissive by baked area.
- `ebs_render.py` / `ebs_sheet.py` / `ebs_silhouette.py` — offline renderer, contact sheets, monochrome
  silhouette separability.
- `ue_import_inspect_skeletal.py` — headless Unreal import with full verification and regression checks.
- `ue_detail_bake.py`, `ue_build_detailed_mesh.py`, `ue_transfer_bake.py` — **in-Unreal Geometry Script**
  detail pass and high-to-low baking (bevel, tessellate, displace, remesh, simplify, UV regeneration,
  bone-weight and vertex-colour transfer, texture transfer between UV layouts).
- `ebs_composite.py` — numpy/Pillow measurement and compositing (runs in `~/.venvs/ebs-art`).

**Installed and available**: Unreal 5.8.2 with Geometry Script enabled, Blender 5.2.1 LTS (scriptable
headless via `bpy`), numpy 2.5.3, Pillow 12.3.0. No paid DCC, no 3D-generation service account.

---

## 5. What is left to reach production-ready

This is the actual work. Ordered by how much it matters to the look.

1. **Concept fidelity — the core gap.** The blockouts have correct proportions, silhouette,
   footprint and component counts, but every form is a box, prism or tube. The concepts show sculpted
   plates with irregular jagged seams, tapering organic limbs, layered overlapping carapace, and
   surfaces with authored variation. Closing this needs real modelling — sculpted or generated geometry,
   retopologised to the card budgets, with the rig and sockets preserved. There is 4–8× unused triangle
   budget available on every asset.
2. **Textures for the other 20 packages.** Only the Riftstalker has a full stack. Every other package
   ships untextured with material slots only.
3. **Full animation sets.** Each unit has 3–6 clips. The requirement (`SPEC-ART-002`) is idle,
   locomotion, turn, acquire, wind-up, attack, recovery, hit, ability, state transition, death and
   selection acknowledgement; workers add gather/carry/deliver/construct/repair; buildings add
   construction, operational, offline, produce/research, damaged and destruction states.
4. **Production materials.** Only per-capture debug materials exist. Needed: a master material per
   faction with instances, driving state (damaged/destroyed/powered/producing), team colour, and the
   molt system for Kharuun units.
5. **Collision and physics assets.** Simple collision boxes are authored; skeletal meshes need physics
   assets. Navigation/footprint rules must not change.
6. **LOD chains.** LOD0 and LOD1 are authored separately; no generated reduction chain exists, and
   nothing has been verified through engine LOD reduction.
7. **VFX bindings.** Sockets exist and are named per contract; no Niagara systems are bound.
8. **Team colour system.** Defined (a `TeamColor` parameter plus a mask band) but not built; the
   captures use a cyan placeholder that is not a real Kharuun team colour.
9. **Contract reconciliation.** Ten provisional asset cards and one rig amendment (Riftstalker: 22 bones
   against a card that says 14 — the owner selected the 22-bone rig) must be reconciled with the
   authoritative requirements before final technical qualification.
10. **Integration.** Everything so far lives in an isolated sandbox Unreal project. Nothing is
    integrated into the game project, and no gate review (art / gameplay / technical) has been passed.

---

## 6. Rules you must not break

- **The concept art is the authority** for what each asset should look like. Where the gameplay
  footprint forces a deviation, record it explicitly rather than silently absorbing it.
- **Canon material grammar is binding**: no Kharuun strata on Meridian assets, no organic smoothing on
  Kharuun (`REL-ART-029`), no amber on Hollow Choir, no cyan on Kharuun.
- **Emissive ceilings are ceilings, not targets**, and are measured by surface area.
- **Triangle ceilings are per-card**; being under one is headroom, not sufficiency.
- **Sockets and their exact names are contract** and must survive every transformation and LOD.
- **Gameplay data is authoritative.** Art must not imply a capability the simulation does not have, and
  must not change footprints, navigation or placement rules to suit a drawing.
- **Evidence over assertion.** Every claim about an asset should be backed by a measurement, a render or
  an import report — not by "it should work."

---

## 7. Your task

**Take all 21 packages from blockout to production-ready 3D models that match their concept art.**

Decide your own method. You may sculpt, generate, procedurally refine, or direct a human — whatever you
judge best. Produce whatever you think the job needs: models, textures, scripts, tool code, step-by-step
instructions for someone with machine access, or a staged plan you then execute.

Two things to be honest about as you go: say plainly when something you produce is a proposal rather
than a finished asset, and say plainly when a target is not reachable by the method you chose — a
partial result that is labelled accurately is worth more here than a confident overstatement.

Start with the **Riftstalker** (`EBS-KHA-UNT-002`): it is the furthest along, it has the richest
requirements (three molt states, vertex ID channels, a 22-bone rig, an amber emissive ceiling), and if
your approach works there it will generalise. Show what a finished Riftstalker looks like, then scale
the method across the remaining twenty.
