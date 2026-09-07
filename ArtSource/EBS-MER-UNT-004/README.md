---
title: EBS-MER-UNT-004 Relay Skiff — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
updated: 2026-09-07 (concept-v3: clip durations snapped to the 30 fps frame grid)
package: EBS-PKG-MC-RELAY-SKIFF
production_asset_id: EBS-MER-UNT-004
production_maturity: BLOCKOUT
revision: ebs-mer-unt-004-concept-v3
canon_status: CANDIDATE (selected replacement concept under delegation; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-UNT-004 Relay Skiff — production source

Single authoritative source record for the Relay Skiff production asset. Created in the isolated
production worktree; bounded by the shared [agent contract](../../AGENTS.md), the
[authority map](../../Docs/README.md) and the frozen preparation records under
[Docs/VisualAssetPipeline](../../Docs/VisualAssetPipeline/README.md), which are read, never edited.
Production-lane maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `production_policy[EBS-PKG-MC-RELAY-SKIFF]` in [gap-decisions.json](../../Docs/VisualAssetPipeline/motion/gap-decisions.json): component inventory hull / relay_mast / dish / archive_cradle / secondary_emitter; `bone_count_source` 8; sockets `Scouting_Sensor_Pod`, `Logistics_Relay_Beam`; LOD0 ≤ 5,000 / LOD1 ≤ 2,100; 2 material slots; emissive ≤ 8%; Nanite OFF; pivot "ground-contact center; flying hull uses ground-projected center and separate hover offset"; 14-track `required_track_inventory` |
| Reserved production ID | `EBS-MER-UNT-004`, planned `SK_EBS_MER_UNT_004` under `/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_004/` |
| Review decision | `EBS-CON-MER-UNT-004` **REPLACE** ([review-selections.json](../../Docs/VisualAssetPipeline/review-selections.json)): "replace the fighter-like pointed canopy and ornamental ring assembly with a thin-hulled utility skimmer, dominant mast/dish and an exposed strapped archive cradle. Retain the maintained ceramic equipment language. The book says it could not fight; game canon gives a small secondary weapon. Flag that conflict and retain game capability pending reconciliation." Owner note on the original: "Read the book make sure this make sense." |
| Selected candidate (the production direction) | `…/Evidence/concept-discovery-20260906/relay-skiff-review/relay-skiff-candidate.png` — three panels: main three-quarter, SIDE VIEW, TACTICAL SILHOUETTE |
| Style reference | `…/Evidence/concept-discovery-20260906/lancer-review/lancer-candidate.png` (pale ceramic plates over a charcoal frame, brass trim, small cyan strips, matte) |
| Superseded concept | `EBS-CON-MER-UNT-004` (`Project/site/assets/concepts/meridian-units.png` bottom-right, sha256 `427e60cd27bd78e9…`) — retained history only; its canopy and ring assembly are deliberately absent |
| Fidelity target | [concept-fidelity.md](concept-fidelity.md) (AUTHORITATIVE prose target, byte-identical to the blob the coordinator delivered, git `a7a5764`, sha256 `d4578b8825cf8e54…`; four prose lines are contradicted by the candidate's own pixels and are recorded in §8.2 rather than edited into the target) |
| Canon row | DevelopmentBible.md line 512 (`SPEC-UNIT-004`): "A light, fast, low-slung skimmer with a tall relay mast, a dish, and a small forward weapon that is clearly secondary. In Mission 01 it carries an **archive cradle**: a sealed pale-ceramic cassette rack lashed to its deck with dark straps. … Never reads as a hero body. Glides with a slight nose-down lean; the mast lights cyan when relaying temporary logistics. The archive rack never changes unless an authoritative event binds it." |
| Book | ¶139, ¶169, ¶1941 (attached to the review record; the "could not fight" line is the conflict carried in §8.4) |
| Gameplay record | `SPEC-UNIT-004` / `mc_relay_skiff`: 70 Matter + 20 Dawn, 75 HP, 500 cm/s, 1,500 cm sight, Logistics footprint 1, 80-tick production, 6 damage at 400 cm every 24 ticks, Extend Relay +4 Logistics for 400 ticks within 700 cm on an 800-tick cooldown |
| Requirement card | `REL-FAC-025.MC.SKIFF.ASSET`: LOD0 ≤ 5,000 / LOD1 ≤ 2,100 tris; 2048² PBR stack with copper Link conduits and a dynamic opacity map for the relay visualization; cyan conduit lines un-bloomed, low saturation, ≤ 8% of the visible surface; hovering uses ground raycasts and never alters simulation passability; 8-bone rig with a continuous harmonic hover bob; sockets `Scouting_Sensor_Pod` / `Logistics_Relay_Beam`; facing snaps under Reduced Motion; Extend Relay projects a cyan cone toward the nearest grid node within 700 cm |
| Simulation envelope | A unit's collision footprint is a 25 cm square (`Rules.footprintHalfExtentRaw = kFixedScale / 8` on a 100 cm tile, `EchoesContentSubsystem.cpp:363`, `EchoesGlassScarCompiledMapPack.h:21`). The visual envelope overhangs it; recorded as a deviation in §8.3 |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Relay Skiff is the Compact's eye and its temporary lifeline: a fast unarmoured skimmer
that extends vision across fog corridors and, standing within 700 cm of a connected grid node, lends the
network four points of Logistics for 400 ticks. In Mission 01 it is the archive carrier. Its silhouette
must say *sees and connects* and never *fights*: the mast and dish dominate, the weapon is a stub.

**DETAIL.** Large scale: a long, low, flat-decked hull floating clear of the ground, one tall segmented
mast at the rear quarter carrying a small dish and two whip antennas, and a sealed pale ceramic cassette
rack strapped to the deck centre. Medium: four charcoal lift pods slung under the hull edges with cyan
intake strips, an orthogonal pipe/rail framework around the deck and wrapping the squared tail, a blunt
chamfered prow with a recessed cyan-lit bay, a short emitter barrel under the nose. Fine (close camera):
strap buckles and deck tie-downs, mast collars and segment strips, the slack cable loop from the dish
back to the deck, the radiator keel vents. Material logic: pale ceramic plates bolted over a charcoal
serviceable frame, brass trim, matte, nothing polished (`lancer-candidate.png`).

**ACTION.** Tracks per the frozen package inventory (§5): idle hover, move, turn, stop, damage, death,
cancel, restore, the attack triple and the relay triple. The hover bob is continuous and small (≤ 4 cm);
travel adds a slight nose-down lean (canon); the relay clips key a fixed placeholder dish aim and the
node-facing rotation is a runtime look-at on the `dish` bone (§8.1), while the mast lights cyan as a
material state, not geometry; the archive rack never changes unless an authoritative event
binds it. Sound (Bible): a light turbine whine and a soft relay chirp — specified, not produced.
Accessibility: role reads from the silhouette (mast, dish, cradle, hover gap), state from posture and
luminance rather than hue; facing snaps instantly under Reduced Motion (the clip never yaws the frame).
Boundaries: no root motion, no simulation authority in animation, no collision primitive of its own.

**REVIEW.** Every field checked against the sources in §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

Authority: [concept-fidelity.md](concept-fidelity.md) and the candidate's pixels (owner ruling 2026-09-06:
the concept images define what the asset looks like; gameplay rules bound the concept, never replace it).
Every proportion is a ratio of **L = 360 cm**, the hull length. Pixel basis (re-traced for concept-v2 with
a per-column silhouette scan at luminance threshold 172): the craft is 533 px long in both panels
(SIDE VIEW x 951–1484, TACTICAL SILHOUETTE x 955–1487) and the side panel's ground shadow centre is at
y 503, so 1 px = L / 533 = 0.675 cm. Built values are in `build-manifest.json` → `concept_measurements`,
and `concept_measurements.measured_on_candidate` carries the concept numbers they are checked against.

| Item | Built | Basis |
|---|---|---|
| Hull length L | 360 cm | concept-fidelity.md scale basis (PROVISIONAL): a 500 cm/s skimmer carrying a cassette rack, small beside the 176 cm Surveyor |
| Hull plan | 120 cm wide = 0.333 L; stretched hexagon, nose face 0.134 L, tail face **0.278 L** (100 cm), chamfer from 0.35 L ahead of centre and from x −0.40 L aft | PIXELS: 176 px of 533 through the middle; the stern holds 146–150 px from x 968 to x 1008 (0.274–0.281 L). concept-fidelity.md item 1 says 0.42 L wide — correction §8.2. concept-v1 built a 0.234 L tail from a 124 px reading that only occurs 9 px from the stern tip |
| Hull section | underside 32.4 cm (0.09 L), deck top 68.4 cm (0.19 L), body thickness 36 cm (0.10 L), of which the **pale ceramic flank is 27 cm (0.075 L, up to 0.165 L)** and a charcoal deck band carries the rest | items 1–2; measured 0.077 / 0.205. The pale plate is 45.7% of the 59 cm band from the rail top to the underframe bottom; the candidate draws 42% (y 415–451 inside y 392–478). concept-v1 gave the pale plate 61% |
| Hover gap | lowest geometry (pod shoes) at 10.08 cm = 0.028 L above z = 0 | item 2; measured 0.024 L. The runtime hover offset is **additive** on this gap (§4) |
| Lift pods | **4 pods, two per side**, 50.4 cm long (0.14 L), Ø 22.3, at x −85.7 / +94.3, y ±47.9, hanging to 0.028 L | PIXELS: the SIDE VIEW underside drops to the pod line only over x 1027–1140 and x 1298–1395; between them it holds y 470–478 with no pod. Flat-run centres 0.738 L and 0.238 L from the bow. concept-fidelity.md item 3 says six — correction §8.2, OWNER-QUESTION 5 |
| Relay mast | column cap 198 cm (0.55 L) = 3.6 × the hull thickness above the deck, at x −86.4 (0.26 L ahead of the tail) | item 4; PIXELS put the mast at the REAR quarter — deviation §8.2 |
| Dish | Ø 46.9 cm (0.13 L) at (−80.3, 28.1, **199.4** = 0.554 L), rest aim forward, **10° to starboard and 28° up** | item 4; the panels disagree on the diameter (plan 0.149 L, side 0.111 L) and 0.13 L is the prose value and their mid-point. Hub height and aim are measured — §8.3. concept-v1 sat at 0.49 L, aimed 35°/16° |
| Whip antennas | tips at 255.6 cm (0.71 L) and 234 cm (0.65 L) — the tallest points of the asset | item 4; measured 0.71 L |
| Archive cradle | 151.2 × 72 × 46.8 cm (0.42 / 0.20 / 0.13 L) on the deck centre, two straps at x ∓37.8, deck tie-downs stay with the hull | item 5 |
| Secondary emitter | barrel 39.6 cm (0.11 L) under the nose, mount at x 140.4, tip at (180.0, 0, 29.5) on the nose face | item 6: "clearly secondary, no muzzle drama". concept-v1 quoted the 0.11 L constant but built 34.3 cm of tube |
| Overall envelope | 376.9 long × 141.5 wide × 255.6 tall; pivot at the ground point under the hull centre | bounds in the manifest; overhangs the 25 cm simulation footprint (§8.3) |
| Units / axes / pivot | cm; +X forward, +Y right, +Z up; root at the ground-contact centre; **Nanite OFF** | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-mer-unt-004-concept-v2`)

Generator: [build_relay_skiff.py](build_relay_skiff.py) on the mesh kit and the skeletal kit
([../tools/ebs_meshkit.py](../tools/ebs_meshkit.py), [../tools/ebs_skelkit.py](../tools/ebs_skelkit.py)).
Every concept proportion is a named constant in terms of L; the parts are authored in their own local
frames (pod, mast, dish, emitter, cradle) and placed by `assemble()`.

| Concept item (concept-fidelity.md) | Built |
|---|---|
| 1 thin hull, stretched hexagon | two prisms on the same 6-point plan outline: pale `hull_shell` (z 32.4–59.4) and charcoal `hull_deck_band` (z 59.4–68.4), so the flank reads as a plate on a deep dark frame as the candidate draws it; + `hull_prow`, a convex wedge from the full section at x 126 to a 48 cm nose face at x 180 that also chamfers up (z 40–64). Pale `hull_deck_plate_l/r` strips run the deck edges over the charcoal band, with `hull_deck_panel*` as the dark centre |
| 2 hover gap | nothing below z = 10.08; the deck reads at 0.19 L with the pods hanging under it |
| 3 lift pods (**four**, two per side) | `pod_l/r_01..02_*`: a charcoal cylinder (Ø 22.3 × 50.4) on a strut and shoe, with a cyan intake strip, at the two measured stations per side under the hull edges (§8.2) |
| 4 mast, dish, whips, cable | `mast_base` + `mast_base_block` ×2 on the deck, three tapering column segments with collars and cyan strips, `mast_head`, `mast_avionics`, two whips, a five-segment slack `mast_cable` hanging from just under the dish yoke down to the deck; `dish_face_01..03` (a stepped paraboloid) + `dish_hub` + cyan `dish_feed` on the yoke `mast_dish_arm`/`mast_dish_gimbal`, now at deck +131 cm so the disc centre sits inside the mast head block's z range (195.1–209.5) |
| 5 archive cradle | `deck_archive_case` with four corner blocks, two straps over the top and down both sides, a cyan status strip and two lid ribs; the deck-side tie-down blocks and plates are `hull_cradle_tiedown*` so the unloaded deck still reads as strapped |
| 6 secondary emitter | `emitter_mount` + `emitter_barrel` + `emitter_muzzle` with a cyan strip, slung under the prow so it reads from the front and below; `hull_nose_bay` is the recessed cyan-lit bay above it |
| 7 palette | two export slots: `MI_EBS_MER_UnitCeramic` (pale plates) and `MI_EBS_MER_UnitFrame` (charcoal structure); every cyan strip is the review-only `MI_EBS_MER_StatusCyan` pseudo slot, folded into the ceramic slot on export |
| card "orthogonal rail framework" | `hull_deck_rail_l/r` outboard of the hull plates on four posts, four `hull_cross_rail_*`, two `hull_mast_gantry_*` beams at the mast station, `hull_tail_hoop` and two `hull_tail_stub_*` behind the squared tail |
| card "anti-gravity radiator core" | `hull_keel` under the hull with cyan `hull_keel_vent_l/r` |

Rig (8 bones, identity rest orientation, `bone_count_source` 8):

| Bone | Parent | Head (cm) | Purpose |
|---|---|---|---|
| `root` | — | (0, 0, 0) | the **ground point under the hull centre**; never keyed, so the runtime hover offset is additive |
| `hull` | root | (0, 0, 50.4) | harmonic hover bob, nose-down lean, turn bank |
| `mast` | hull | (−86.4, 0, 68.4) | mast sway and the lit relay state; carries `Logistics_Relay_Beam` |
| `dish` | mast | (−80.3, 28.1, 199.4) | dish gimbal; carries `Scouting_Sensor_Pod`. The clips key a fixed placeholder aim — the node-facing rotation is a runtime look-at layered here (§8.1) |
| `pod_bank_l` / `pod_bank_r` | hull | (0, ∓47.9, 32.4) | the two pods of each side: rake on travel, counter the hull attitude, droop on death |
| `emitter` | hull | (140.4, 0, 29.5) | secondary weapon aim, recoil, recovery |
| `cradle` | hull | (0, 0, 68.4) | the archive cradle's deck contact; the group is hidden when nothing is bound |

Sockets: `Scouting_Sensor_Pod` on `dish` at (−72.8, 29.4, 203.5), 8.64 cm along the dish axis in front of
the face, so it swings with the dish — including during `relay_extend`, a logistics action;
`Logistics_Relay_Beam` on `mast` at (−86.4, 0, 198.0), the mast head. **`Logistics_Relay_Beam` is a fixed
origin with identity rotation, not an aim transform**: the card's cyan cone toward a grid node must be
aimed at runtime and must not be bound to this socket's rotation (§8.1). Triangles by bone (LOD0):
hull 740, mast 364, dish 252, pod banks 176 + 176, cradle 168, emitter 80.

Budgets: LOD0 **1,956** ≤ 5,000; LOD1 **1,340** ≤ 2,100 (headroom reserved for ART_ALPHA plate detail);
cyan share of the surface 2.07% ≤ 8%; two material slots (656 ceramic / 1,300 frame triangles at LOD0);
Nanite OFF; no collision primitive is authored (the hover must not alter simulation passability).
Rest bounds: x −196.2…180.7, y ±70.7, z 10.08…255.6.

States: `loaded` (the `deck_archive_*` group present) and `unloaded` (hidden — nothing else changes).
The cradle is also exported on its own as `SM_EBS_MER_UNT_004_ArchiveCradle_LOD0/1` with its pivot at the
deck contact centre, so the runtime may either hide the group or attach the part at the `cradle` bone.

## 5. Clips (keyframed, Unreal rotators; §4 rig)

Fourteen tracks, the frozen `required_track_inventory`. No clip keys `root`; no clip translates the frame
along the ground (the only in-frame translation is the emitter recoil along its own barrel); no clip yaws
the hull, so presentation facing stays runtime-owned and can snap under Reduced Motion. Every clip sampled
at 20 ms keeps the whole asset above z = 0 — a hovering unit never touches the ground — except the death
settle, which comes to rest 0.32 cm above it on its pod shoes. The tightest live clearance is the
`damage` flinch at 2.47 cm (§8.6).

| Track | Duration | Intent | Lowest vertex |
|---|---|---|---|
| idle | 2.0 s loop | continuous harmonic bob: hull ±1.7 cm (3.4 cm peak to peak, rule ≤ 4), 0.7° pitch and 0.35° roll, pods breathing, mast swaying 0.6° | 4.73 |
| move | 0.6 s loop | glide at 500 cm/s: −3° nose-down lean (canon) with a ±1 cm bob; the pod banks counter the lean so the cushion stays level; play rate follows authoritative velocity | 5.25 |
| turn | 0.6 s loop | banks 5.5° into the heading sweep the runtime owns, lifting 1.5 cm, pods differential 1.5°; no yaw on any bone | 6.50 |
| stop | 0.4 s | deceleration flare: nose lifts 3.2° and the frame settles back to the hover attitude | 10.08 |
| relay_extend | 0.7 s | **contract track `relay_activation`** (§8.1): the dish swings to a fixed unit-relative placeholder aim (−60° yaw, +14° pitch — the clip does not know where a node is; see §8.1), the mast leans 1.2° into it and the segments light cyan (material state); the hull holds station | 9.72 |
| relay_hold | 1.6 s loop | sustained relay: the dish holds the placeholder aim with a slow ±3° scan, the hull keeps station on a reduced bob | 8.35 |
| relay_expiry | 0.5 s | the dish returns to the forward rest aim and the lights fade; no simulation effect from the clip | 9.72 |
| attack_anticipation | 0.25 s | the emitter mount aims 6° off the nose, the hull noses up 1°; no muzzle drama | 8.55 |
| attack_execution | 0.2 s | 3.5 cm recoil along the barrel with a 2° hull kick; damage stays authoritative | 6.83 |
| attack_recovery | 0.3 s | emitter and hull return to rest | 8.55 |
| damage | 0.3 s | flinch: 5° pitch, 2.5° roll and a 3 cm drop on the cushion, then recovery; no displacement | 2.47 |
| death | 1.2 s | cushion failure: the hull sinks 7.8 cm onto its pods with a 1.2° pitch and 2.2° roll while the mast folds 26° to port and the dish droops; final pose held (cosmetic debris only, 200 ticks) | 0.32 |
| cancel | 0.3 s | order cancelled: the dish and emitter return to rest, the hull resumes the hover attitude; never replays a one-shot | 3.43 |
| restore | single frame | identity key on the seven non-root bones for reconstruction from saved state | 10.08 |

`build_relay_skiff.py --evidence-dir …` writes the skinned GLBs with these clips (`SK_EBS_MER_UNT_004_LOD0/1.glb`,
one skin, 8 joints, 14 animations, sockets as child nodes of their joints) on the verified skeletal kit.
[pose_review.py](pose_review.py) samples thirteen clip stills at the review fractions (linear interpolation,
as the glTF samplers do) and bakes posed review OBJs plus their scenes; the death and relay stills are also
baked with the cradle hidden.

## 6. Review evidence (concept-v2 blockout stage)

Evidence root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-004/`
(`receipt.json` inventories every file with SHA-256; `review/` rest and posed OBJs; `scenes/`; `renders/`).

Renders (29 scenes): orthographic front / right / rear / left / top / top_plan at authored scale with a
180 cm reference figure, loaded (`renders/rest/`) and unloaded (`renders/rest_unloaded/`); the game's
orthographic tactical framing (view blocks copied from `EBS-MER-UNT-001/scenes/rest_tactical.json`:
3800 cm arm, 55° FOV, −48° default and −60° gameplay tilt, plus a mono pass; `tactical_close` at 800 cm is
a review aid, not a game framing) for both states; **three `compare` scenes** (loaded, unloaded, LOD1) that
reproduce the candidate's panel layout — hull horizontal, bow to the image right, tight margin, 1280 × 900 —
so a panel and a render can be read side by side at one scale (§8.6 records why concept-v1's sheets could
not be); LOD1 turnaround and tactical; the cradle part on its own; thirteen posed stills
(`renders/pose_<clip>_<fraction>/`) with three tactical pose scenes; and a context scene with the loaded and
unloaded skiff beside the Power Link (`../EBS-MER-BLD-002`) and a Surveyor (`../EBS-MER-UNT-001`).

Comparison sheets (`renders/concept-compare/`, concept panel in the first cell; the exact crops and tile
orders are in [make_sheets.py](make_sheets.py) and §7):

| Sheet | Cells |
|---|---|
| `check1_side.png` | candidate SIDE VIEW (cut to the render aspect and object share) · `compare/side_plate` · `compare_unloaded/side_plate` · `compare_lod1/side_plate` — one scale, one orientation |
| `check2_top.png` | candidate TACTICAL SILHOUETTE (same treatment) · `compare/top_plate` · `compare_unloaded/top_plate` · `compare_lod1/top_plate` |
| `check1_side_turnaround.png` | rest right · rest left · move still · rest `top_plan` (the turnaround cells concept-v1 carried in check1/check2) |
| `check3_pods_and_emitter.png` | candidate nose crop · rest front · attack_execution front · rest right |
| `check4_loaded_vs_unloaded.png` | candidate three-quarter · rest right · unloaded right · unloaded top |
| `check4_loaded_vs_unloaded_tactical.png` | loaded/unloaded at `tactical_close` and at the gameplay framing |
| `check5_tactical.png` | candidate three-quarter · tactical_close · tactical_gameplay · tactical_mono |
| `mast_and_dish.png` | candidate mast crop · rest rear · relay_extend front · relay_extend right |
| `style_and_history.png` | lancer-candidate crop · tactical_front_quarter · superseded `EBS-CON-MER-UNT-004` · tactical_close |
| `pose_sheet_right.png`, `pose_sheet_tactical_near.png` | the twelve clip samples |
| `lod0_vs_lod1.png`, `context_and_states.png` | LOD1 beside LOD0; context and the death / relay reads |

Fidelity checks (concept-fidelity.md), decided by the author on the sheets above:

| Check | Result | Pixel evidence |
|---|---|---|
| Side view: long thin hull floating clear of the ground, mast and dish dominating, cradle on the deck | **MET** | `check1_side.png` cell 2 (`compare/side_plate.png`), beside the panel at one scale: the hull runs 360 cm at 36 cm thick with its underside 32.4 cm clear of the ground and the pods hanging to 10 cm; the pale flank is a 27 cm plate on the dark frame (42% of the band in the candidate, 45.7% built); the mast + dish + whips reach 255.6 cm — 3.7 × the deck height — and read as the tallest mass, with the disc centre level with the mast head as the candidate draws it; the cradle stands on the deck centre. `check1_side_turnaround.png` cell 3 (`pose_move_050`) holds the same read under the nose-down lean |
| Top view matches the tactical silhouette: stretched hexagon hull, mast at the front-left, cradle centred | **PARTIAL** | `check2_top.png` (panel and renders at one scale and one orientation): the plan is the candidate's stretched hexagon (parallel sides, blunt 48 cm nose face, squared 100 cm tail, pipe framework outboard of the pale plates), the deck reads as a charcoal centre band between pale edge strips with the pale cradle centred between the straps, and the relay disc reads as a **disc** at the stern quarter (top-down footprint 23.2 × 46.4 cm against the candidate's 33.1 × 53.6; concept-v1 presented 28.9 × 39.2 with only 477 cm² of projected area against the candidate's 1,393). **Three prose values are not matched, all because the candidate's own pixels say otherwise (§8.2):** the mast stands at the REAR quarter (0.26 L ahead of the squared tail, opposite the emitter nose) rather than the "front-left quarter" — the wording this check line still carries; the hull is 0.333 L wide (176 px of a 533 px length) rather than 0.42 L; and there are two lift pods per side, not three. The disc is smaller than the candidate's plan draws it, which is a hard limit of the 0.13 L diameter — §8.3 |
| Six lift pods visible under the hull edges; secondary emitter small and forward | **PARTIAL — built to the pixels, not to the prose** | `check3_pods_and_emitter.png` cell 4 and `check2_top.png`: **two** pods per side under the hull edges at the measured stations, each with its cyan intake strip, clear of the keel. The candidate shows two per side in all three panels and the prose says three; the pixels win under the owner ruling and the divergence is recorded in §8.2 with the trace and raised as OWNER-QUESTION 5. Cell 2 (`rest/front.png`): the emitter is a 39.6 cm barrel (0.11 L, tip on the nose face) under the prow with the cyan nose bay above it — 2.07% of the surface is cyan and nothing about it reads as a gunship |
| Cradle hidden variant renders with an empty strapped deck (loaded/unloaded read) | **MET** | `check4_loaded_vs_unloaded.png` cells 3–4 and `check4_loaded_vs_unloaded_tactical.png`: with `deck_archive_*` hidden the deck is empty, the four tie-down blocks and plates stay on the deck, and the mast, rails and pods are unchanged (1,956 → 1,788 triangles); the difference is legible at the gameplay framing |
| Tactical framing: reads as a light utility scout, never as a hero body or a gunship | **MET** | `check5_tactical.png` and `context_and_states.png`: at 3800 cm / −60° the skiff is a low pale wafer with one thin mast; beside the Surveyor and the Power Link in `context/tactical_near.png` it reads as a vehicle, not a walker or a weapon; the mono pass keeps the mast, cradle and hull band separable without hue |

Checks: 30 structural tests in [test_relay_skiff_build.py](test_relay_skiff_build.py) (contract inventory
and the candidate's own parts, **two pods per side at the measured concept stations with nothing in the
concept's pod-free stretch**, **the tail face and plan widths against the measured silhouette**, **the pale
flank's share of the hull band**, **the dish footprint in both panels and its height on the mast**, **the
emitter barrel at the full item 6 length**, the concept ratios of items 1–7, the mast station and the
cradle placement, the plan hexagon, the 8-bone hierarchy, every polygon bound, socket names / bones /
positions, **pivot at the ground point with the hover additive**, budgets, two export slots, no authored
collision, loaded/unloaded states and the separate cradle part, the contract track inventory with the
documented alias, ground clearance for every clip sampled at 20 ms, the ≤ 4 cm idle bob, the nose-down move
lean, the turn bank without yaw, the relay dish aim and its sockets, the secondary attack triple, the
cradle staying still outside death, the death settle and mast fold, determinism), all passing. One of them,
`test_readme_rig_table_and_emissive_figures_match_the_generator`, parses **this file** and asserts its rig
table and its three cyan-share figures against the generator's constants, so the source record cannot
silently desynchronise again. `--check` rebuilds into a temporary directory and reports no drift.

Not done here: textures, in-engine playback, the headless Unreal import (the coordinator runs imports
serially afterwards), gate reviews, owner acceptance.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-UNT-004"
R="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z"
python3 build_relay_skiff.py --evidence-dir "$R/EBS-MER-UNT-004"          # exports, review OBJs, scenes, build-manifest.json
python3 build_relay_skiff.py --evidence-dir "$R/EBS-MER-UNT-004" --check   # read-only: rebuild in a temp dir, compare hashes
python3 build_relay_skiff.py --check                                      # the same check without the evidence tree (skips only the review-file presence test)
python3 test_relay_skiff_build.py
python3 pose_review.py --evidence-dir "$R/EBS-MER-UNT-004"
for s in "$R/EBS-MER-UNT-004/scenes/"*.json; do python3 ../tools/ebs_render.py --scene "$s" --out "$R/EBS-MER-UNT-004/renders/$(basename "$s" .json)"; done
python3 make_sheets.py --evidence-root "$R"      # cuts the nine concept panels, then the thirteen sheet commands
python3 ../tools/make_evidence_receipt.py --evidence-dir "$R/EBS-MER-UNT-004" --package EBS-MER-UNT-004 \
  --manifest build-manifest.json --stage "BLOCKOUT (concept-v1)" --command ... --note ...   # exact strings in receipt.json
```

## 8. Decisions, deviations and open items

### 8.1 Naming and rig decisions

1. **Track naming conflict, unresolved by me.** The frozen package contract's `required_track_inventory`
   names the relay action `relay_activation`; [concept-fidelity.md](concept-fidelity.md) and the production
   handoff name the same track `relay_extend` (the ability in `SPEC-UNIT-004` is "Extend Relay"). One clip is
   authored, under the fidelity target's name, and the contract name is carried as a documented alias
   (`TRACK_ALIAS` in the generator, `track_contract.alias` in the manifest, asserted by
   `test_track_inventory_matches_the_frozen_contract`). See OWNER-QUESTION 1.
2. **Eight bones spend the budget on function, not decoration** (`bone_policy`: "no decorative animation
   bones"): root, hull, mast, dish, two pod banks, emitter, cradle. The whip antennas, the cable loop, the
   rails and the pods' cyan strips are rigid with their bone; the mast's relay light and the emitter muzzle
   are material and VFX states. The cradle earns its bone because it is the group the runtime shows, hides
   or attaches, and the death settle carries it with the hull without a second skin.
3. **The pivot is the ground point under the hull centre, not the hull** (contract `import_policy.pivot`).
   `root` sits at (0, 0, 0) and is never keyed, and the authored geometry starts 10.08 cm above it, so the
   runtime's hover offset adds to the authored gap instead of replacing it
   (`test_pivot_is_ground_point_hover_additive`).
4. **The dish does not aim at anything, and the relay socket is not an aim.** `relay_extend` /
   `relay_hold` / `relay_expiry` key a FIXED unit-relative placeholder aim on the `dish` bone
   (−60° yaw, +14° pitch, with a ±3° scan on the hold); no clip knows where a grid node is, so the
   node-facing rotation must be a runtime look-at layered on the `dish` bone. Consequences the
   integration task must not get wrong: **`Logistics_Relay_Beam` is a position with identity rotation**,
   so the card's `.VFX_POLY` cone "toward the nearest grid node within 700 cm" must be aimed at runtime
   and must not inherit the socket's rotation; and **`Scouting_Sensor_Pod` rides the `dish` bone**, so
   any scouting VFX bound to it swings 60° whenever the unit performs a logistics relay. This split is
   deliberate and tested (`test_relay_turns_the_dish_and_only_the_dish`) and is recorded in the manifest
   as `socket_semantics`.
5. **No collision primitive is authored.** Hovering "uses standard ground raycasts to float the view actor
   without altering simulation passability" (card .MAT_RULE) and the approved simulation envelope controls
   (`import_policy.collision`), so the asset ships no UBX boxes at all.

### 8.2 Corrections to the prose target, made on the candidate's pixels

The owner ruling is that the concept images define what the asset looks like. Where
[concept-fidelity.md](concept-fidelity.md) and `relay-skiff-candidate.png` disagree, the pixels win and the
prose line is recorded here rather than edited. The target file on disk is byte-identical to the blob the
coordinator delivered (git `a7a5764`, sha256 `d4578b8825cf8e54…`): concept-v1 amended item 4 in place on
2026-09-07 after writing this section, which made this paragraph false; concept-v2 restored the delivered
bytes and carries every correction here instead.

- **Mast end (item 4: "at the FRONT-LEFT quarter").** In the candidate the mast stands at the end
  **opposite** the chamfered prow: the SIDE VIEW puts the column centre at x 1082.5 of a hull running
  951–1484, i.e. 0.246 L ahead of the squared tail (built at 0.26 L), and the TACTICAL SILHOUETTE puts the dish at the same
  end while the blunt nose (with the recessed cyan-lit emitter bay under its lip, visible in the
  three-quarter crop) is at the far end. The build follows the pixels: mast at x −86.4, emitter at the nose.
  Reading the candidate's three-quarter panel as facing left would put the mast at the front and the
  weapon at the tail, which contradicts canon line 512 ("a small **forward** weapon"). Recorded, not
  silently reconciled; if the owner reads the panels the other way the fix is one sign on `MAST_X` and one
  on `EMITTER_X`, and every clip and socket follows the bones.
- **Hull width (item 1: "width ≈ 0.42 L").** The TACTICAL SILHOUETTE measures 176 px of plate width on a
  533 px length = **0.333 L**, with a maximum of 199 px (0.373 L) across the outboard pipe framework at the
  mast station. 0.42 L would be 224 px, which nothing in the panel spans. The build authors 0.333 L of hull
  plate and 0.393 L over the rails and strakes (141.5 cm total); the missing local widening at the mast is
  a known minor item (§8.6).
- **Lift pod count (item 3: "Six serviceable pods … three per side").** The candidate shows **two per
  side**. In the SIDE VIEW, per-column underside trace over the hull (x 951–1484, luminance threshold 172,
  y search 110–495): the outline reaches the pod line y 490 only over x 1027–1140 (flat run x 1072–1109)
  and x 1298–1395 (flat run x 1339–1375). Between them, x 1141–1296, it holds y 470–478 — 12 to 20 px
  (8–14 cm equivalent) higher — and a 7× crop of that stretch shows rails, a keel pipe and small fittings
  with no pod body, no end caps and no cyan intake. The TACTICAL SILHOUETTE shows two bumps per hull edge
  and the three-quarter panel two pod clusters on the near side. The flat-run centres are 0.738 L and
  0.238 L from the bow, so the build places four pods at x −85.7 and +94.3. concept-v1 built six, put the
  middle pair in the concept's pod-free stretch, and recorded no deviation. See OWNER-QUESTION 5.
- **Mast end again, in the fidelity-check list.** The same "front-left" reading appears a second time in
  concept-fidelity.md, in its own fidelity-check line ("Top view matches the tactical silhouette: … mast
  at the front-left …"). It is the same error as item 4 and the same correction applies; recorded here
  rather than edited, and the §6 check row is marked PARTIAL against the wording it carries. Both
  occurrences are covered by OWNER-QUESTION 3.
- **Tail face width (concept-v1's "124 px").** A 2 px per-column trace of the TACTICAL SILHOUETTE stern
  (y search 620–900) gives 146–150 px from x 968 to x 1008, then a step out to 167–172 px at x 1012.
  124 px occurs only at x 964, nine pixels from the stern tip. The tail face is therefore 0.278 L
  (100 cm), not the 0.234 L concept-v1 built, and the step to full width sits at x 1010 = x −0.395 L,
  which is where `TAIL_CHAMFER_X` already was. `test_tail_face_and_plan_widths_match_the_measured_silhouette`
  now holds the number.
- Item 4's "the tallest thing on the asset" describes the mast column (0.55 L); the same item's whip
  antennas rise above it, and the candidate draws them at 0.71 L, so the built tallest point is a whip tip
  at 255.6 cm. No conflict, recorded for the measurement table.

### 8.3 Deviations forced by a rule, each with the rule cited

- **Dish size and rest orientation — measured, and partly unreachable.** The two panels do not draw one
  consistent dish, so this is a compromise stated in numbers rather than in prose. Measured on the
  candidate at 1 px = L / 533: the TACTICAL SILHOUETTE disc is 48.8 × 79.1 px = **33.1 × 53.6 cm**
  (along hull × across hull) with a projected area of ~1,393 cm²; the SIDE VIEW disc is 29 × 59 px =
  **19.6 × 39.8 cm** with its major axis leaning ~66° from horizontal, top aft. Those imply diameters of
  0.149 L and 0.111 L respectively; item 4's 0.13 L is their mid-point and is what the build uses.
  * **Hard limit, not a choice.** A projection of a circle of diameter *d* and unit normal **n** has an
    extent *d*·√(1 − (**n**·**u**)²) along any image axis **u**, so a 46.9 cm dish can present at most
    46.9 cm across the hull in plan. The candidate's 53.6 cm is unreachable at 0.13 L, and its 1,393 cm²
    of plan area would need the disc tilted ~54° up — a dish pointing nearly at the sky, which neither
    panel draws. This is recorded, not forced.
  * **What was chosen.** Yaw 10° to starboard, 28° up. Built footprints: plan **23.2 × 46.4 cm**, side
    **23.2 × 41.4 cm**, side major axis 62° (candidate 66°), plan area **812 cm²** (58% of the
    candidate's). concept-v1's 35° / 16° gave plan 28.9 × 39.2, side 28.9 × 45.1 and 477 cm² (34%): the
    across-hull extent — the axis the plan panel actually shows — was 27% short and the disc read as a
    sliver from above. Its §8.3 claimed the face "reads … at the tactical top-down framing"; the geometry
    did not deliver that.
  * **Why not the reviewer's "keep the 35° starboard swing".** Holding 35° and raising the pitch to 33°
    gives plan 34.1 × 41.2 and side 34.1 × 39.4: the plan along-hull extent lands on the candidate's 33.1
    and the plan area rises to 942 cm² (68%), but the SIDE VIEW along-hull extent overshoots its measured
    19.6 cm by 74% and the plan across-hull extent stays 23% short of 53.6. Measured as RMS
    relative error over the four panel extents, that is 39% against concept-v1's 29% and 19% for the
    10° / 28° aim actually built — i.e. the reviewer's exact suggestion would have made the fit worse
    overall while fixing the area. The starboard swing is what costs the across-hull read, so it was
    reduced instead of held. `test_dish_footprint_and_height_against_both_candidate_panels` holds every
    number above.
  * **Caveat for integration.** At the shipped camera's fixed −45° yaw the disc's apparent roundness
    depends on the unit's heading, and the dish is a runtime-aimed bone (§8.1), so this is a rest read,
    measured against the candidate's pure top-down panel — not a promise about any particular gameplay frame.
- **Readability renders use the shipped camera, not the contract's citation.** `readability_test.camera`
  in gap-decisions.json and `Docs/Requirements.md:4398` (`REL-ART-002`) both say "3,800 uu arm /
  45-degree tilt". Every tactical view in this package renders at 3800 cm with pitch −48 (default) and
  −60 (gameplay), which is what `AEchoesRTSCameraPawn` actually sets
  (`Source/EchoesOfTheBrokenSun/Private/EchoesRTSCameraPawn.cpp:78` `FRotator(-48, -45, 0)` and
  `:86` `SetCameraFraming(3800.0f)`; `:197/:222/:245/:345/:362` `FRotator(-60, -45, 0)`). The renders
  follow the code because the code is what the player sees; the contract's 45° citation is stale.
  Recorded here so the integration task knows which number is authoritative and can reconcile
  `REL-ART-002` rather than discovering the difference.
- **Cyan strips, brass trim, plate seams and bolts are texture channels, not geometry** (2-slot rule
  `material_slots_provisional_max` = 2 and the LOD0 ≤ 5,000 budget). The review OBJs keep a third pseudo
  slot so the renders show the cyan; the export folds it into the ceramic slot. The 2.07% emissive share
  is therefore a geometry proxy for the card's ≤ 8% rule, not a measurement on a textured asset.
- **Visual envelope vs simulation footprint.** A unit's collision footprint is a 25 cm square
  (`kFixedScale / 8`, §1); the skiff's visual envelope is 377 × 141 cm. The concept width and length win
  (owner ruling: gameplay rules bound the concept, never replace it; the collision box is a pathing and
  hit quantity, not a visual bound) and the asset changes no simulation quantity. The integration task's
  `readability_test` must cover what this creates: skiffs overlapping neighbours at the gameplay framing,
  selection and hover hit-testing on the overhang, and the 700 cm relay cone originating 198 cm above the
  ground. Recorded as a deviation, not fabricated as an approved dimension.
- **Scale basis is PROVISIONAL.** L = 360 cm comes from the fidelity target, not from a measured in-engine
  envelope (`import_policy.scale`: "Physical dimensions remain unmeasured, not undecided gameplay scale").
  Every proportion is expressed as a ratio of L, so a scale decision is one constant.
- **Death settles rather than tumbles.** `destruction_policy` gives cosmetic debris only, and the ground
  rule keeps the wreck out of the terrain: the hull sinks 7.8 cm onto its pods (lowest vertex 0.32 cm) and
  the mast folds 26°, which is the readable death at the tactical camera without any simulation effect.
- **The plated, pipe-dense candidate is blocked as prisms, boxes and tubes.** Plate depth, the brass edge
  trim and the fine pipe runs are ART_ALPHA work under the reserved budget (LOD0 1,956 of 5,000).
- **The hull body is one thickness but two materials, and that needed extra geometry.** Item 1's 0.10 L
  is the hull BODY (underside 0.09 L to deck top 0.19 L) and is unchanged. Inside it the candidate's SIDE
  VIEW paints the pale ceramic flank over y 415–451 only, inside a band running y 392 (deck rail top) to
  y 478 (underframe bottom): 42% of the band, a thin plate riding on a deep dark frame. concept-v1 ran one
  pale prism through the whole body and gave the pale plate 61%, so the hull read as a thick pale slab.
  concept-v2 splits it into pale `hull_shell` (0.090–0.165 L) and charcoal `hull_deck_band`
  (0.165–0.19 L) — 45.7% pale, +20 triangles. That turned the deck's top surface charcoal, which the plan
  panel contradicts (a charcoal centre band between pale ceramic edge strips), so `hull_deck_plate_l/r`
  restore that strip at 0.058 L per side and `hull_deck_panel*` were narrowed to meet them without
  overlapping: another 24 triangles. Recorded because those two plates are geometry the concept implies
  rather than draws as a separate part.

### 8.4 Canon conflict carried, not resolved

The review decision records it explicitly: **the book says the skiff could not fight; game canon
(`SPEC-UNIT-004`, `REL-FAC-025.MC.SKIFF`) gives it 6 damage at 400 cm.** The build retains the game
capability — a 0.11 L barrel under the nose (39.6 cm of built tube, tip on the nose face) with no armoured
housing and no muzzle drama, and an attack triple whose recoil is 3.5 cm — so the asset reads as a scout that can defend itself, never as a gunship.
The conflict is flagged for reconciliation, not decided here. See OWNER-QUESTION 2.

### 8.5 OWNER-QUESTION blocks (for the coordinator to batch; the owner is not addressed directly)

> **OWNER-QUESTION 1 — relay track name.** The frozen package record calls the relay action
> `relay_activation`; the fidelity target and the handoff call it `relay_extend`. The clip is authored as
> `relay_extend` with `relay_activation` recorded as an alias. Which name should the AnimSequence carry
> when the integration task binds it? (One string in `build_clips`; no geometry or key changes.)

> **OWNER-QUESTION 2 — book vs canon on the weapon.** The book paragraph attached to this package says the
> skiff could not fight; the requirements give it a 6-damage 400 cm weapon, which this build models as a
> small forward emitter. Confirm the weapon stays (current build), or the emitter and the attack triple are
> removed and the requirement row is amended.

> **OWNER-QUESTION 3 — mast end.** concept-fidelity.md says the mast is at the "front-left quarter"; the
> candidate's own panels put it at the end opposite the nose and its emitter, and the build follows the
> pixels (§8.2). Confirm the mast reads at the REAR quarter, or the prose is the intent and the mast and
> emitter swap ends.

> **OWNER-QUESTION 4 — scale.** L = 360 cm is PROVISIONAL: a 3.6 m skimmer overhangs a 100 cm tile and is
> 2 × the Surveyor's height at the mast. Confirm 360 cm, or name the length the gameplay framing wants;
> every proportion follows L.

> **OWNER-QUESTION 5 — lift pod count.** concept-fidelity.md item 3 says six pods, three per side; all
> three panels of the candidate show two per side and nothing at all in the middle stretch of the
> underside (§8.2 carries the trace). The build follows the pixels and ships four. Confirm four, or the
> prose is the intent and a third station returns at x ≈ 0 — one tuple in `POD_STATIONS_X`, plus the two
> tests that hold the measured stations.

### 8.6 Open items

Textures (2048² per card, including the dynamic opacity map the relay visualization needs); the headless
Unreal skeletal import and its probe evidence (the coordinator runs imports serially after this task);
in-engine playback of the hover bob against the runtime's additive hover offset; the emissive-share
measurement on the textured asset; the `readability_test` cases in the contract; gate reviews; owner
acceptance.

Known minor items carried, not fixed:

- The `damage` flinch bottoms out at 2.47 cm of clearance (inside the rule, little margin).
- The cable loop and the whip antennas are single-segment tubes that will alias at far zoom until the
  texture pass gives them width.
- The candidate puts the dish hub ~21 cm forward of the mast axis (SIDE VIEW: disc centre x 1114.1 against
  a column centre of x 1082.5); the build's yoke reaches sideways only, so the hub sits 6.1 cm forward.
  Measured and left, because moving it means re-authoring the yoke arm for 15 cm of plan offset.
- The candidate's plan holds ~176 px of width through the middle and bulges to 199 px (0.373 L) only at
  the mast station; the build is 141.5 cm (0.393 L) over the rails for most of its length with no local
  widening. The hull plate itself is the measured 0.333 L; it is the outboard rail framework that is
  uniform. ART_ALPHA work, not a blockout change.
- The pods' cyan intake strips face down and outboard, so they do not read in the pure side view the way
  the candidate paints them; they read in the front, rear and tactical views.
- The comparison sheets still leave ~30% of the canvas empty: `ebs_sheet.py` fits every image into a
  SQUARE cell and both the concept panels and the `compare` renders are 1.42:1. The tool is read-only for
  this task, so the fix taken was to match aspect and object share between panel and render (§6) rather
  than to change the cell geometry.
- At the 3800 cm gameplay framing (`context_and_states.png`, bottom row) the death and relay reads rest
  almost entirely on the mast angle, because the hull itself is ~130 px wide there — the contract's
  `readability_test` ("recognition_seconds_max" 1, cases "far zoom" / "damage" / "role-specific state")
  has to confirm that the lit mast and the folded mast carry those states at that size, and the cyan
  relay cone is a VFX the asset does not own.

## Concept-v3 amendment — clip durations on the 30 fps frame grid (2026-09-07)

The concept-v2 exports imported into the sandbox project with **`attack_anticipation`** missing: the Interchange
skeletal import creates no AnimSequence for a clip whose duration is not an integer number of frames at
30 fps, logs no warning and still reports success. A dedicated probe (twelve clips of identical shape,
durations from 1 to 30 frames including four half-frame values) reproduced it exactly: every whole-frame
clip imported with its exact length, every half-frame clip vanished. Evidence:
`…/asset-production-20260906T221157Z/skeletal-clip-duration-probe/` (`probe-record.json`).

concept-v3 therefore snaps every authored duration up to the next whole frame and scales the key times
with it, so the pose at any normalized time — and every posed review still — is unchanged. The rule is
now enforced in the shared kit: `ebs_skelkit.write_skinned_glb` refuses to write an unaligned clip
(`ANIMATION_FPS`, `frame_aligned_duration()`, `retime_clip()`, `SKELETAL_ENCODING['clip_duration']`,
kit revision `ebs-skelkit-v2`), and this package's tests assert that every clip is frame-aligned.

Retimed here: `attack_anticipation` 0.25 s → 0.2667 s (8 frames). The import of the concept-v3 exports reports 0 errors with every clip present
(`import/import-report-ebs-mer-unt-004-concept-v3.json`, run 2 of `import/heavy-run-receipt.json`).
