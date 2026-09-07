---
title: EBS-FWL-SYS-001 Future Well — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
package: EBS-PKG-EBS-FAM-FWL-001
production_asset_id: EBS-FWL-SYS-001
production_maturity: BLOCKOUT
revision: ebs-fwl-sys-001-concept-v3
canon_status: CANDIDATE (one-family direction selected under delegation; REL-ART-014 amendment prepared, not applied)
status: Isolated production source built to the concept-fidelity target; no Unreal integration authorization
---

# EBS-FWL-SYS-001 Future Well — production source

Single authoritative source record for the Future Well state family (the state-family pilot of the
owner's production handoff). Bounded by the shared [agent contract](../../AGENTS.md), the
[authority map](../../Docs/README.md) and the frozen preparation records; production-lane maturity is
in [../production-ledger.json](../production-ledger.json). Owner ruling 2026-09-06: the concept images
define what the asset looks like; the authoritative target is [concept-fidelity.md](concept-fidelity.md).
Gameplay rules bound the concept but never replace it; every forced deviation is in §8.

## 1. Source contract

| Binding | Value |
|---|---|
| Concept fidelity target | [concept-fidelity.md](concept-fidelity.md): candidate `future-well-candidate.png` (sha256 `b16e84cb…d60ba7`) is the reconciled family; states sheet `future-well-states.png` TL Dormant / TR Harvest gives the material and courtyard read; `target-render-vertical-slice.jpg` `[0.37, 0.29, 0.63, 0.68]` is composition only; `echoes-future-well-landmark.jpg` is retained history, not a reference |
| Package contract | `production_policy[EBS-PKG-EBS-FAM-FWL-001]`; GAP-01: amber Harvest telegraph and inward spire collapse, charcoal spent bowl permanent, Preserve cyan custody, Reshape magenta temporary terrain, Dormant one bowl/spire with doubled shadows ([gap-decisions.json](../../Docs/VisualAssetPipeline/motion/gap-decisions.json)) |
| Prepared amendment | `FWL-HARVEST` rewrites `REL-ART-014` (cyan geyser → amber rise and fold); status PREPARED_NOT_APPLIED; recorded authority conflict, not resolved here |
| Reserved production ID | `EBS-FWL-SYS-001`, planned `SM_EBS_FWL_SYS_001` (+ `_Spire`, `_Wall` parts) under `/Game/Echoes/Production/FWL/SYS/EBS_FWL_SYS_001/` |
| Family | `EBS-FAM-FWL-001` (six concept members; one stateful system; state authority `Content/Data/Source/future_wells.json`) |
| State board / motion | `motion/storyboards/EBS-PKG-EBS-FAM-FWL-001-GAP-01.svg`; motion package GAP-01 (six phases, reduced motion, cancel/restore rules) |
| Canon | Bible "Future Wells": Harvest 180-tick public telegraph then permanent collapse; Preserve intact (15 Dawn / 300 ticks, 1,400 cm intelligence); Reshape 120 Dawn, 180-tick telegraph, 1,800-tick authored possibility with warned expiry |
| Book | ¶243 "Amber light began to climb the core spire"; ¶245 "The spire folded into the bowl"; ¶246 the harmonic ends abruptly, a strip of fractured ground gives way |
| Requirements | `SPEC-WEL-001..004` (impassable, indestructible landmark; 420 cm capture zone), `SPEC-WELLP-001..003`, `REL-WEL-005/006/010/015/016` (600 cm scar radius), `REL-ART-014` (amendment pending), `REL-ART-016`/`REL-ART-030` (passable dressing ≤20 cm, never reads impassable), `SPEC-VISD-007` effects grammar |
| Simulation envelope | `Simulation.cpp`: Well footprint half extent `kFixedScale/2` (one tile), capture radius 4.2 tiles, scar radius 6 tiles; presentation tile 200 cm; the runtime shrinks the current 280 cm-radius placeholder dais by 0.35 to fit; the M01 map lists the Well with a 1-tile half-extent reservation |
| Units / import policy | cm; +X forward (Reshape trace direction), +Y right, +Z up; pivot at the dais ground centre; **Nanite off** on every part (`build-manifest.json` `units.nanite: false`); authored LOD0/LOD1, no auto LOD; UBX collision from the main part only |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** A Future Well is a neutral, indestructible landmark at which one committed protocol
decides a mission's future: Harvest (immediate Dawn, permanent collapse and scar), Preserve
(contestable custody and income) or Reshape (a temporary authored possibility). It sits on open
ground inside a 4.2-tile capture zone that workers must occupy; nothing about it is a weapon or a
structure to build. What must be absent: any faction identity, any reading that the courtyard blocks
the capture zone, any implied state the simulation has not committed.

**DETAIL.** Large scale (the candidate): a broad circular courtyard of dark vitrified basalt masonry,
a low ring wall of stacked blocks with a ruined rhythm and four taller broken segments, one faceted
four-sided shard on a small circular dais at the centre, a fractured apron crumbling into the ground
outside. Medium: two wall courses with fallen blocks and coping stones, three concentric paving
joint rings, four cardinal paving bands (states sheet), the dais step and rim and the round vitrified
socket recessed inside the rim that the shard stands in (the dark bowl centre), rubble around the
apron and fallen blocks at every gap. Fine: twelve radial fracture seams, six crack rays across the
socket, seam lights along the shard's edges, rung marks along the Reshape trace (a mask feature, not
geometry). Material logic:
vitrified basalt masonry and charcoal glass with quiet magenta micro-fracture (`vitrified_glass`),
doubled outline/shadow as a lighting cue, matte ≥0.85 on every ground-facing surface.

**ACTION.** Thirteen state presentations (§5) driven only by authoritative protocol state: colour
lives in the state slot's masks (dormant seams, amber shard-edge seams, cyan ring, magenta trace) and
the one moving element is the shard part, which tilts about `Spire_Hinge` and sinks into the socket on
Harvest commit; the spent courtyard is permanent and non-interactive, its round dark cracked socket
the only trace of the shard. Doubled shadows (Crownfall dual key/fill) are a lighting/decal cue, not
geometry. Sound intent from the motion package is specified, not produced. Reduced motion: discrete
state shapes and steady masks, no rapid flash. Boundaries: the paving, apron and wall are dressing;
the capture zone, intelligence radius, scar radius and the Reshape crossing itself are simulation-
and map-owned.

**REVIEW.** Fields checked against §1 and the concept images before geometry (2026-09-06, concept-v1);
concept-v1 sheets inspected against the concept pixels and reworked into concept-v2 (§6, §8.11);
concept-v2 sheets measured in pixels against the candidate and reworked into concept-v3 (courtyard scale,
flush light-only channels, one wall peak, torn apron; §8.2, §8.6, §8.12–8.15). Open items in §8.

## 3. Scale basis (from the concept measurements)

S = shard height, 220 cm at presentation scale (concept-fidelity.md item 2). Measured on the candidate at
source pixels (`future-well-candidate.png`, 1536×1024; concept-v3 measurement, recorded in
concept-fidelity.md "Scale"): Dormant panel shard apparent height 88–95 px at ≈32° elevation (the
Preserve ring ellipse 410×220 px gives sin ≈ 0.53) → S ≈ 104–112 px; wall outer extremes 107..600 px
(R ≈ 246) and 117..575 px (R ≈ 229) → R_out ≈ 2.0–2.4 S (built 2.05 S = 450 cm); shard base ≈0.63 S;
low wall crest ≈0.26–0.35 S; one dominant back peak ≈0.85–0.95 S (apex (392,125) over a ≈0.35 S crest)
with the other broken segments ≈0.55–0.65 S; Preserve ring at 0.90 R_out, touching the wall; apron field
≈1.3 R_out with a broken +X tongue to ≈1.5 R_out. The fidelity file's earlier 1.6–1.8 S / 360 cm estimate
(built in concept-v1/v2) is superseded there (§8.2). The candidate spent inset shows the centre as a dark
round depression with cracks radiating from it; the Dormant shard-base crop shows the shard rising from a
small round mound with pink micro-fracture lines running out from its base.

| Item | Value | Basis |
|---|---|---|
| Shard (S) | 220 cm tall, base 132 cm across the flats (0.60 S), base at z 14.2 (`Spire_Hinge`), apex at z 234.2 | concept-fidelity.md item 2; candidate base ≈0.63 S; the base corners (r 85–93 cm) rest on the dais rim (§8.3) |
| Dais | step radius 98 cm to z 10; rim annulus r 82–94 cm to z 14; round vitrified socket r 82 cm recessed 1.5 cm below the rim (top z 12.5); three fracture veins (six rays at 15/75/135°) 0.8 cm proud of the socket | the only raised impassable mass; one-tile footprint half extent 100 cm; spent inset (dark cracked round centre); book ¶245 "folded into the bowl" (§8.11) |
| Ring wall | outer radius 450 cm (2.05 S), thickness 40 cm, ~52 cm blocks; low crest 40–91 cm (mean 69 cm = 0.31 S); three broken stubs 138 / 124 / 142 cm (0.56–0.65 S) and one dominant peak 195 cm (0.89 S) on the far diagonal (307–321°); four cardinal gaps 178 / 152 / 152 / 152 cm chord (≥140) | concept-fidelity.md item 1 and "Build to the concept" as amended concept-v3; gaps are a forced deviation (§8.1) |
| Inner paving | radius 412 cm, top at 6 cm; flat vitrified joint rings at r 165/255/345 (+1 cm), cardinal bands 28 cm wide to r 360 (+5 cm), twelve flush seams (+1.5 cm, stopping short of the ring) | walkable, relief ≤20 cm (REL-ART-016/030) |
| Apron | torn field of 36 convex sectors (18 at LOD1) tucked under the paving, outer radius 506–610 cm (mean 569 = 1.27 R_out) with six bites 40–60 cm deep and stepped slab ends; +X tongue 60/42 cm half-widths to x 640 with six broken slabs to x 680 (1.51 R_out); top at 4 cm, rubble bedded on the ground 3–9 cm proud, fallen blocks ≤17 cm | candidate Dormant/Reshape panels (ragged field ≈1.3 R_out, tongue ≈1.5 R_out); walkable dressing; everything inside the 840 cm capture radius |
| Preserve ring | radius 404 cm (0.90 R_out), 10 cm wide, outer edge 1 cm off the wall's inner face, +1 cm proud | candidate 03: the ring touches the wall; pure light (§8.12) |
| Reshape trace | 8 cm wide flush line from the dais edge (x 94) over the +X band (top 12 = band +1), across the paving (top 7.5) and the apron/tongue (top 5.5) through `Wall_Gap_01` to x 600 (`Reshape_Trace_End`); no rung geometry | candidate 04: a thin line on the paving; candidates 01/03 and the spent inset show none (§8.12); crossing is map-owned |
| Harvest fold | pitch 24° about `Spire_Hinge` (apex toward −X) and sink 224 cm; the highest shard point ends at z −6.0 (below the paving) | book ¶245; fidelity item 5 (shard tilts and sinks, ≈20 ticks) |
| Assembled extent (dormant) | x −579.2 … +680.4 (1,259.7 cm incl. the tongue), y −607.1 … +572.1 (1,179.2 cm), z 0 … 234.2 | build-manifest `assembled_bounds_dormant` |
| Units / axes / pivot | cm; +X = Reshape trace direction; pivot at the dais centre on the ground; Nanite off | contract `import_policy`; manifest `units` |

## 4. Geometry (revision `ebs-fwl-sys-001-concept-v3`)

Generator: [build_future_well.py](build_future_well.py). Three parts (LOD0 / LOD1 triangles):

| Part | Content | LOD0 | LOD1 |
|---|---|---|---|
| `SM_EBS_FWL_SYS_001` (main) | `dais_step_01`, `dais_step_02` (rim annulus), `dais_socket` (round recessed vitrified socket), `dais_vein_01..03` (state slot, on the socket top), `paving_disc`, `paving_course_01..03`, `paving_spoke_01..04`, `paving_seam_01..12`, `apron_sector_01..36` (torn field), `apron_tongue` (+X causeway stub and six broken slabs), `apron_crumble_01..14`, `apron_fallen_01..08`, `preserve_ring` (flush), `reshape_trace` (three flush segments, no rungs); sockets `Target_Anchor_Center` (0,0,14), `State_VFX_Origin` (0,0,14.2), `Spire_Hinge` (0,0,14.2), `Reshape_Trace_End` (600,0,4), `Wall_Gap_01..04` (r 430 at 0/90/180/270°); one UBX box `dais` 196×196×120 cm centred on the tile | 2,042 | 1,018 |
| `SM_EBS_FWL_SYS_001_Spire` (part) | `spire_body` (14 facets: irregular base quad, bulged and twisted mid ring, offset apex), `spire_seam_01..04` (edge beads, state slot); origin at the base centre, instanced at `Spire_Hinge`; no collision | 78 | 46 |
| `SM_EBS_FWL_SYS_001_Wall` (part) | component group `wall_*`: `wall_block_01..60` (two staggered courses of ~52 cm blocks, some fallen), `wall_coping_01..06`, `wall_tall_01..04` (three tiers and a ridge cap; `wall_tall_04` is the 195 cm peak with a taller, narrower cap); no sockets, NO collision | 784 | 432 |
| Assembled (main + shard + wall) | budgets LOD0 ≤ 8,000 / LOD1 ≤ 3,500 | **2,904** | **1,496** |

Slots: `MI_EBS_FWL_Basalt`, `MI_EBS_FWL_Vitrified`, `MI_EBS_FWL_State` (the wall part uses basalt only;
the shard uses vitrified + state). Review assemblies (evidence `review/`): dormant, harvest_telegraph,
harvest_fold_mid (pose 0.5), harvest_commit (0.85), spent (1.0), preserve, reshape and all_channels at
LOD0 and dormant at LOD1; review assemblies carry review-only materials for the channels a state does
not light — `MI_EBS_FWL_State_Off` for unlit seams (dark fracture lines) and `MI_EBS_FWL_State_Hidden`
for the unlit ring and trace (coloured as the basalt: nothing shows; §8.9) — while the exports keep
three slots (in-engine the masks select channels). `all_channels` keeps every channel lit and exists
only for the emissive-area worst case. Exact numbers, sockets, measurements and hashes:
[build-manifest.json](build-manifest.json).

concept-v3 over concept-v2 (geometry): courtyard rescaled to the candidate pixels (wall 360 → 450 cm,
paving 322 → 412, courses 130/200/270 → 165/255/345, bands to 360, gap sockets at r 430, ten blocks per
arc); Preserve ring 292 → 404 cm (0.90 R_out) and flattened to +1 cm; Reshape trace rebuilt as three
flush segments 8 cm wide (band +1, paving +1.5, apron +1.5) ending at x 600, rung geometry dropped;
`wall_tall_04` 154 → 195 cm with a taller, narrower cap, the other three 138/124/142; apron rebuilt as
36 torn sectors (six bites, stepped ends) with the +X tongue and its slabs; rubble bedded on the ground
outside the 450 cm wall. concept-v2 over concept-v1: the square 140 cm vitrified plate on the dais top
became a round vitrified socket (r 82) recessed 1.5 cm inside a basalt rim (r 82–94) so the spent
centre reads as a dark cracked round centre; the dais fracture veins moved from inside the plate
(invisible) onto the socket top; the shard hinge dropped from z 17.2 to 14.2 (base on the rim).
Exports, review assemblies and manifest regenerated at every step. **Comparison baseline:** the only
committed, receipted revision is `ebs-fwl-sys-001-blockout-v1` (commit `5a59c855fb82`, main LOD0 glb
`538ca37c…`; manifest `comparison_baseline`); concept-v1 and concept-v2 were never committed, so their
hashes are not reproducible and are not cited. Every concept-v3 export hash differs from blockout-v1,
and the revision string is new (same string = silently reused assets).

## 5. State presentations

| Track | Shard | State mask | Authority read |
|---|---|---|---|
| dormant | standing | seams 0.25, shard seams 0.15 (quiet magenta/amber, low glow), ring 0, trace 0 | `wellChoice == Dormant` |
| harvest_telegraph | standing | shard-edge seams ramp 0.15→1.0 amber with telegraph progress | telegraph progress (180 ticks) |
| harvest_cancel | standing | seams return to dormant; no spent pose, no success cue | control broken |
| harvest_commit | tilts 0→24° about `Spire_Hinge`, sinks 224 cm into the socket over ~20 ticks | seams 1.0 then hard cut | committed |
| spent | gone below the paving; the round dark socket with its dark veins remains | all masks 0; dark cracked courtyard, permanent, nothing above the wall | `collapsed` |
| preserve_hold / preserve_loss | standing | cyan ring 1.0 with slow custody pulse / 0 | Preserve and controlled / lost |
| reshape_telegraph / cancel / manifest / warning / expiry | standing | magenta trace ramps / returns / 1.0 / slow warning cadence / 0 | Reshape telegraph, manifest (1,800 ticks), warning, expiry |
| restore | per state | per state | load/replay; never replays payout, collapse or terrain creation |

Review rendering of the masks (`scenes/*.json`): the renderer draws one flat unlit colour per
material, so each state scene sets `MI_EBS_FWL_State` to its identity colour — Dormant faint magenta
`[0.16, 0.09, 0.15]` (the 0.25/0.15 mask split is not shown separately), Harvest amber
`[1.0, 0.62, 0.2]`, Preserve cyan `[0.16, 0.86, 0.96]`, Reshape magenta `[0.9, 0.25, 0.75]`, spent
dark — and parks every channel the state does not light: unlit seams in `MI_EBS_FWL_State_Off`
`[0.025, 0.025, 0.03]`, darker than the basalt so they read as dark fracture lines in the charcoal
glass (the concept's Preserve/Reshape/spent panels keep the seams as dark lines), and the unlit ring
and trace in `MI_EBS_FWL_State_Hidden` `[0.06, 0.06, 0.065]` (= the basalt), so nothing shows where
they lie — the candidate's Dormant, Preserve (trace) and spent panels carry no ring or trace at all.
In-engine this is the state material's job: the mask channels for the ring and the trace must fall
back to the paving look, not to a dark seam, when they are 0 (texture/material stage, §8.10).

## 6. Review evidence (concept-v3 blockout)

Evidence root: `…/asset-production-20260906T221157Z/EBS-FWL-SYS-001/` (`receipt.json`; `review/`
nine assemblies; `scenes/` ten scene files; `renders/`):

- `renders/dormant/` front, right, top, tactical_default, tactical_gameplay, tactical_mono,
  tactical_near (arm 1,700 cm since concept-v3; the courtyard is 1,260 cm across), tactical_far,
  dais_near; the reference figure stands at (−540, 540), outside the apron; `renders/harvest_telegraph/` default, near;
  `renders/harvest_fold_mid/` front, near; `renders/harvest_commit/` front, near, dais_near;
  `renders/spent/` front, default, near, mono, dais_near, top; `renders/preserve/` default, gameplay,
  mono, near; `renders/reshape/` default, near; `renders/lod1/` front, far; `renders/parts/` (wall
  part alone with the shard part at the hinge height) near, front.
- `renders/area_check/emissive-area.json` (`measure-emissive.py`, unlit magenta-keyed render of the
  `all_channels` assembly, every state channel lit at full cyan at once): 3.3 % (tactical_default) /
  3.2 % (tactical_gameplay) of mesh area, limit 15 % (concept-fidelity.md); pinned by
  `test_evidence_emissive_area_within_limit`.
- `renders/concept-compare/` (built by `build-sheets.sh` with `ebs_sheet.py`; concept crop beside the
  matching render view): `01_tactical_courtyard.png`, `02_shard_dais_front_side.png`,
  `03_preserve_reshape.png`, `04_harvest_commit_spent.png`, `05_dormant_glow_game_framing.png`,
  `06_wall_part_gaps_top.png`, `07_spent_dais_centre.png` (spent inset vs spent dais_near and top
  centre, dormant top centre, concept shard base vs dormant dais_near, sheet Harvest vs commit
  dais_near). Fidelity checklist result (author's inspection of the pixels, concept-v3):

| Check (concept-fidelity.md) | Result | Evidence |
|---|---|---|
| Tactical: broad dark courtyard (wall 2.0–2.2 S), ring wall of uneven blocks, one dominant peak over three stubs, gaps, torn apron with the +X tongue | MET (four gaps instead of one, §8.1) | sheets 01, 05, 06: the shard spans ≈15 % of the courtyard width in both the candidate and the render (concept-v2: ≈30 %); ring wall with two courses, fallen blocks, the 195 cm peak at the back of the default frame, three lower stubs, gaps at the cardinals; the apron edge steps and bites, the tongue with its slabs at +X (sheet 06 top view) |
| Single faceted shard on a dais at the centre; radial/concentric paving | MET | sheet 02 (front/side: one 14-facet shard, no petals), 06 (top: three courses, four bands, twelve seams), 07 (round rim and socket under the shard) |
| Preserve: flat cyan ring hugging the wall; Reshape: thin magenta trace through the +X gap; neither shows in the other states | MET | sheet 03: cyan ring at r 404 touching the wall's inner face, flat on the paving, no trace; thin magenta trace from the dais out through the +X gap onto the tongue to x 600, no ring; sheets 01/04/05: no ring or trace in Dormant, Harvest or spent |
| Harvest commit and spent: shard sinks, bowl dark and cracked, nothing above the wall | MET for geometry (the amber climb/vortex is VFX; the fine "cracked" surface is texture) | sheet 04 (near and front: fold mid, commit, spent below the paving), sheet 07 (spent centre: round dark socket with six dark crack rays, wall unchanged) |
| Dormant reads as held potential (low glow); doubled shadow documented for lighting | MET (faint seams only; the ring and the trace are flush and hidden, §8.12; doubled shadow §8.7) | sheet 05 (game framing and mono: no dark bar along +X, unlike concept-v2), 01 (near), 07 (dormant dais_near: faint seams at the shard base like the concept crop) |

Checks: 20 structural tests in [test_future_well_build.py](test_future_well_build.py) (inventory and
thirteen presentations, revision string, one-tile dais and collision box, the recessed socket and its
veins, walkable relief, single faceted shard and its sink inside the tile, wall part collision-less
with `wall_*` group, wall proportions to S incl. the single dominant peak, four cardinal gaps ≥140 cm,
ring hugging the wall, trace flush with no rungs, torn apron and tongue inside the capture zone, review
parking (seams → Off, ring/trace → Hidden), sockets, budgets and slots, determinism, and two evidence
pins: `scenes/parts.json` instances the shard at `HINGE_Z` and `emissive-area.json` ≤ 15 %), all
passing; `--check` is read-only (builds into a temporary directory) and reproduces byte-identical
exports and review assemblies. Not yet: textures, in-engine import of this revision (the previous
blockout-v1 import evidence in `import/` is retained and superseded), in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-FWL-SYS-001"
python3 build_future_well.py --evidence-dir "<evidence root>/EBS-FWL-SYS-001"; python3 build_future_well.py --evidence-dir "<evidence root>/EBS-FWL-SYS-001" --check
python3 test_future_well_build.py
for s in dormant harvest_telegraph harvest_fold_mid harvest_commit spent preserve reshape lod1 parts area_check; do python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-FWL-SYS-001/scenes/$s.json" --out "<evidence root>/EBS-FWL-SYS-001/renders/$s"; done
(cd "<evidence root>/EBS-FWL-SYS-001/renders" && python3 measure-emissive.py)
(cd "<evidence root>/EBS-FWL-SYS-001/renders/concept-compare" && sh build-sheets.sh)
python3 ../tools/make_evidence_receipt.py --evidence-dir "<evidence root>/EBS-FWL-SYS-001" --package EBS-FWL-SYS-001 --manifest build-manifest.json --stage "BLOCKOUT (concept-v3) + retained blockout-v1 import evidence" --command "..." --note "..."
```

## 8. Decisions, deviations from the concept and open items

1. **Four cardinal wall gaps (concept: one break on +X).** Forced by `SPEC-WEL-004` (blocking footprint
   is one tile) with `REL-ART-016`/`REL-ART-030` (a 75–150 cm wall reads impassable, so a closed ring
   outside the tile would lie about the capture zone). concept-fidelity.md prescribes four gaps ≥140 cm
   so units enter from every side; the +X gap stays the widest (177 cm) as the concept's break, and
   fallen blocks flank every gap so the wall reads broken there.
2. **Wall outer radius 450 cm (2.05 S) — the fidelity file's 1.6–1.8 S / 360 cm estimate superseded.**
   concept-v1/v2 built the prescribed 360 (1.64 S) and the courtyard came out 20–25 % too small for the
   shard (concept-v2 sheet 01: the shard spanned ≈30 % of the courtyard width against ≈15 % in the
   candidate). The pixel measurement (§3; concept-fidelity.md "Scale", amended 2026-09-06 with the old
   text struck through) gives R_out ≈ 2.0–2.4 S. No rule forced 360: the collision-less wall at 450, the
   apron at ≈580 and the trace end at 600 all lie inside the 840 cm capture radius and the 1,200 cm scar
   radius, and the one-tile footprint conflict (item 4) is unchanged. Not a deviation any more.
3. **Dais radius 98 cm and shard base 0.60 S (candidate ≈0.63 S).** The dais is the impassable mass and
   must stay inside the one-tile footprint together with the UBX box (`SPEC-WEL-004`, `Simulation.cpp`
   half extent one tile); the shard base corners must stand on the dais rim.
4. **Wall as a separate collision-less part.** The concept is one structure; the split (own component
   group `wall_*` and part `SM_EBS_FWL_SYS_001_Wall`) keeps the collision box confined to the tile while
   the footprint conflict is open. The conflict, verbatim from concept-fidelity.md ("Scale and the
   simulation conflict (recorded, not resolved here)"):

   > - Simulation: impassable footprint = one tile (200 cm) centred on the spire (`Simulation.cpp`,
   >   half extent `kFixedScale/2`); capture radius 4.2 tiles (840 cm); scar radius 6 tiles.
   > - Concept: the courtyard wall outer radius is ≈ 2.0–2.2 S ≈ 440–480 cm [amended concept-v3 …; the
   >   earlier 1.6–1.8 S / 350–400 cm estimate is superseded], well outside the impassable tile, and a
   >   75–150 cm wall reads impassable (REL-ART-016 / REL-ART-030 forbid passable dressing that reads
   >   impassable).
   > - Build to the concept: wall outer radius 450 cm (2.05 S; amended concept-v3 …) with four cardinal gaps ≥ 140 cm wide (units can enter
   >   the courtyard from every side; the capture zone is reachable), the wall as its own component group
   >   and a separate collision-less part `SM_EBS_FWL_SYS_001_Wall`, the spire + dais + inner paving as the
   >   main mesh with the UBX collision box confined to the one-tile footprint. The OWNER-QUESTION on
   >   extending the Well's blocking footprint to the wall ring (a simulation change, integration task) is
   >   recorded in the README §8; until answered the wall part is a concept-faithful dressing with the
   >   conflict flagged, and the previous one-tile "bowl-only" build remains available in git history.

   **OWNER-QUESTION (recorded, not resolved; routed through the coordinator):** extend the Well's
   blocking footprint to the wall ring (radius 450 cm, a simulation change and an integration task), or
   keep the one-tile footprint and accept a collision-less wall that reads impassable where it stands?
   Until answered the wall is concept-faithful dressing, the four gaps keep the capture zone reachable,
   and the previous one-tile "bowl-only" build (`ebs-fwl-sys-001-blockout-v1`) remains in git history.
5. **Not built from the states sheet:** floating rings (Preserve = flat ring on the paving, Harvest =
   amber climb effect), floating shard fragments and the Harvest vortex (VFX at `State_VFX_Origin`), the
   four corner pylons (reworked into the four taller broken wall segments) — per the review decisions
   in concept-fidelity.md.
6. **Apron (concept-v3)** built as a torn field: 36 convex sectors (the kit fans caps, so the concave
   outline is assembled from convex pieces) tucked under the paving, each with its own outer radius
   (506–610 cm, six bites 40–60 cm deep, stepped slab ends), plus the candidate's broken +X tongue (a
   causeway stub to x 640 with six slabs sinking into the ground to x 680, 1.51 R_out) carrying the
   trace out of the courtyard, rubble bedded on the ground 3–9 cm proud, fallen blocks at the gaps.
   concept-v2's smooth convex disc read as a grey plate at the game framing. The fine rubble haze of
   the painting remains texture/scatter work, bounded by ≤20 cm relief (`REL-ART-016`/`REL-ART-030`).
7. **Doubled outline/shadow** (Dormant "held potential") is a lighting/decal task (Crownfall dual
   key/fill), not geometry; the fine "cracked" spent surface and the vitrified micro-fracture are texture
   stage (2048² per contract). The geometry-level crack read is the three socket veins and the twelve
   paving seams, dark when unlit.
8. Collapse = tilt toward −X + sink through the socket (book: "folded into the bowl"); the socket stays
   as the dark centre. The single dominant peak (`wall_tall_04`, 195 cm = 0.89 S, taller and narrower
   cap) sits on the far diagonal (+X, −Y) so the default tactical frame matches the candidate's rhythm
   (one peak at the back over three stubs of 0.56–0.65 S; concept-v2's four near-equal 0.58–0.70 S
   segments read as a crown, §8.13); the map places the asset by the +X trace direction.
9. Review assemblies carry two review-only materials so each state render shows only its own light:
   `MI_EBS_FWL_State_Off` (unlit seams, dark fracture) and, since concept-v3, `MI_EBS_FWL_State_Hidden`
   (unlit ring and trace, coloured as the basalt: nothing shows). The exports keep three slots; the
   review renderer has one colour per material, so the Dormant 0.25 seam / 0.15 shard-seam split renders
   as one faint colour. concept-v1's dormant review lit every channel; concept-v2 parked the ring and
   trace dark (still a visible dark line, §8.12); concept-v3 hides them. `all_channels` stays for the
   emissive worst case only.
10. Open: `REL-ART-014` amendment must be applied by the integration task before the amber Harvest is
    master-compliant; textures; the doubled-shadow decal; VFX for the amber climb, custody pulse and
    trace; headless import and in-engine capture of this revision (imports are run serially by the
    coordinator); gates and owner review.
11. **Recessed round socket (concept-v2).** The candidate Dormant crop shows the shard rising from a
    small round mound and the spent inset a dark round cracked depression; concept-v1's square plate read
    as a flat dark square once spent (concept-v1 sheet 04). The socket is round (16-gon, r 82), vitrified,
    recessed 1.5 cm inside a basalt rim, with three veins on its top — a reading of the two concept
    crops and book ¶245, not a rule-forced deviation. The recess is inside the dais (impassable mass),
    so walkable relief is unaffected.
12. **Flush light-only channels (concept-v3).** concept-v2's Reshape trace was a 12 cm box 7–8.5 cm proud
    of the paving with eight rungs: relief, not a mask, so it read as a dark rail from the dais through
    the +X gap in every state (sheets 01/03/04/05 of concept-v2), and in-engine it would shade and shadow
    regardless of the mask — an implied state the simulation has not committed (§2 CONTEXT), although
    within REL-ART-016/030. Now: three flush segments 8 cm wide, ≤1 cm proud of the +X band (top 12) and
    ≤1.5 cm proud of the paving (7.5) and the apron/tongue (5.5), no rung geometry (the rung fill is a
    mask/texture feature of the reshape_telegraph track), the ring flattened from +2 to +1 cm. The
    Preserve ring and the trace are pure light in the candidate, so the review hides them when unlit
    (§8.9); the in-engine state material must fall back to the paving look for these two channels at
    mask 0 (§8.10). Residual at the game framing: the +X band itself (5 cm proud, permanent, states
    sheet) with a 1 cm strip on it.
13. **One dominant wall peak (concept-v3).** The candidate Dormant panel has one far-side spike ≈0.85–0.95 S
    over a ≈0.35 S crest (apex (392,125)); concept-v2's 128/140/146/154 cm read near-equal. `wall_tall_04`
    is now 195 cm (0.89 S) with a 58 cm narrower ridge cap; the others 138/124/142 (0.56–0.65 S). The wall
    carries no collision and the spent "nothing above the wall" check still holds.
14. **Read-only `--check` and evidence pins (concept-v3).** `--check` used to rewrite `export/` and the
    review assemblies before comparing (and never compared the assemblies); it now builds into a
    temporary directory and compares every export and review-assembly hash with the manifest (and the
    revision string). `scenes/parts.json`'s shard translate and `emissive-area.json`'s fractions are pinned
    by tests (skipped when the evidence directory is absent; `EBS_FWL_EVIDENCE_DIR` overrides the root).
15. **Known minor items (recorded, not fixed here).** (a) The seams stop 19 cm short of the ring so no two
    state channels overlap; the candidate's seams run to the wall — texture stage. (b) The apron sectors'
    radial side walls are coincident planes between neighbours (hidden except at the steps); harmless in
    the review renderer, worth a weld at the texture stage. (c) The 1 cm trace strip on the +X band and
    the 1.5 cm seams show as sub-pixel edge lines in the review renderer at the near framing (zoom of
    `renders/dormant/tactical_near.png`); at the game framing they do not register.
