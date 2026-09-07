---
title: EBS-MER-UNT-002 Lancer — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-MC-LANCER
production_asset_id: EBS-MER-UNT-002
production_maturity: BLOCKOUT
revision: ebs-mer-unt-002-concept-v4
canon_status: CANDIDATE (REPLACE direction under delegation; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-UNT-002 Lancer — production source

Single authoritative source record for the Lancer production asset. Edited in place inside the isolated
production worktree; bounded by the shared [agent contract](../../AGENTS.md), the
[authority map](../../Docs/README.md) and the frozen preparation records under
[Docs/VisualAssetPipeline](../../Docs/VisualAssetPipeline/README.md). Production-lane maturity is in
[../production-ledger.json](../production-ledger.json) (not written by this task).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `packages[EBS-PKG-MC-LANCER]` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json): `canon_production_brief` (Bible line 510), `gameplay_contract` (`mc_lancer`), `reference_items_required`, `blocking_dependencies` (seven PKG-GATE-* still open) |
| Review decision | `EBS-CON-MER-UNT-002` = **REPLACE**, use `RETAINED_HISTORY_ONLY` ([review-selections.json](../../Docs/VisualAssetPipeline/review-selections.json)). Owner note on the original: "Check out the book for the lancer make sure it makes sense we need to rework this i think". Delegated direction: replace the upright armored hero silhouette; keep pale ceramic, charcoal underframe and cyan rail technology in a narrow two-legged line-fire frame with a low cowl, the lance held low, exposed thin flanks and a visible rear-leg recoil strut |
| Reserved production ID | `EBS-MER-UNT-002`, planned `SK_EBS_MER_UNT_002` under `/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_002/` |
| Selected candidate | `…/Evidence/concept-discovery-20260906/lancer-review/lancer-candidate.png` (main braced view, braced side view, tactical silhouette) — **the production direction**. Owner feedback recorded in the Relay Skiff generation record: "Love that continue to the next" (positive on the candidate; not production acceptance) |
| Derived reference | `…/lancer-review/lancer-derived-turnaround.png` (front / left side / rear / top plus the hip-mount and leg-attachment inset). Its FRONT and REAR panels are **quarter views** — the lance leaves the centreline in both — so only the LEFT SIDE panel was used for measurement (§8.2) |
| Superseded concept | `EBS-CON-MER-UNT-002` (`meridian-units.png` top-right `[0.5, 0, 1, 0.5]`, sha256 `427e60cd27bd78e9…`), decision REPLACE. Retained history only; **not** a modelling target. Crop kept at `…/concept-crops/all/EBS-PKG-MC-LANCER/EBS-CON-MER-UNT-002.png` |
| Fidelity target | [concept-fidelity.md](concept-fidelity.md) — AUTHORITATIVE. Seven of its lines are corrected or added against the pixels across `concept-v1` and `concept-v2`, all marked in that file (§8.2, §8.7); its rig section and its budget line are rewritten to the owner ruling in `concept-v4` (§8.8) |
| Canon row | DevelopmentBible.md line 510 (`SPEC-UNIT-002`): "A two-legged line-fire frame, slightly taller than the Surveyor and narrow, with a long forward rail-lance carried at hip height, a recoil strut braced to the rear leg, and a low armored cowl. Ceramic plate over charcoal, cyan band along the lance… Halts, plants the strut, aims, fires with recoil returning through the mount, recovers. Never fires while moving." |
| Book | ¶178, ¶181, ¶1966, ¶3078 (frames and bracing; the book does not call for a human infantry redesign) |
| Gameplay record | `SPEC-UNIT-002` / `units.json` `mc_lancer`: 145 HP, 320 cm/s, 1,100 cm sight, population cost 2, 18 damage at 650 cm range, 30-tick cooldown, role `ranged_line`. `EchoesContentSubsystem.cpp:309-310` maps it to `EntityType::Soldier` → runtime `PresentationScale` 1.60 (`EchoesEntityView.cpp:1809-1812`) |
| Requirement card | `REL-ART-005.MC.LANCER` (Requirements.md:2382): LOD0 ≤ 8,000 tris, **LOD1 ≤ 3,500 (CONFIRMED, owner ruling 2026-09-07 citing `REL-ART-028`; the card's printed "3,3500" is a Requirements.md typo — §8.5)**, 2048² PBR stack, ceramic micro-noise detail normal, team colour by mask, emissive ≤ 15% with an un-bloomed amber/cyan blend, **18-bone** rig, **sub-object separation for `Turret_Y` and `Barrel_X`**, card socket names `Muzzle_Flash_01` / `Target_Anchor_Center` / `Left_Tread_Vector`; `SPEC-MOV-010` runtime-owned facing; `REL-ART-009` code-driven motion synchronized to authoritative velocity |
| Owner ruling | **2026-09-07**, two answers, implemented verbatim in `concept-v4` (§8.8): the LOD1 ceiling is **3,500** ("`3,3500` is malformed"), and the socket set is **`Rear_Recoil_Strut_Anchor`** with **`Left_Tread_Vector` retained temporarily as a compatibility alias**, left-foot ground contact carried by a separately named foot-contact socket, and the **two-legged concept preserved** |
| Existing envelope | No Lancer visual placeholder and no code-driven Lancer rig exist in the runtime (unlike the Surveyor's M01 parts), so this package has no compatibility-part constraint. The simulation collision envelope of any unit is a 25 cm square (`Rules.footprintHalfExtentRaw = kFixedScale / 8`, `EchoesContentSubsystem.cpp:363`, on a 100 cm tile) |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Lancer is the Compact's disciplined ranged line: it holds behind a Bulwark screen and
puts sustained fire down a lane at 650 cm. Its whole body language is *anchored aim* — it must halt,
plant and fire, and it must read as fragile in the flanks so the player understands why it needs a
screen. It is never a hero body; the canon calls the flanks "visibly thin".

**DETAIL.** Large scale: a narrow two-legged frame in a wide fore-and-aft brace with ONE long rail-lance
carried level at hip height on a hip mount at the body's front; the lance is the heaviest single element
in the silhouette and projects more than half a body-height past the leading foot. Medium: two pale
ceramic shoulder pods with antenna rods flanking a low sensor cowl with a horizontal cyan visor; an open
charcoal frame with visible struts and joint hubs between the pods and the hips (no ceramic skirt); a
thin recoil strut with a slide section running from the lower rear of the body down and back to the
trailing ankle; digitigrade legs with prominent knee discs and block feet with raised toe plates. Fine
(close camera): rail clamps, the cyan channel between the two rails, pod strips, plate seams and brass
edge trim (texture, not geometry). Material logic: pale ceramic plate bolted over a charcoal underframe;
cyan only on the lance channel, the visor and small strips.

**ACTION.** Tracks per the canon row and the fidelity target: idle, move, turn, stop,
**halt-plant-aim-fire-recover**, damage, death, cancel, restore (§5). The frame leaves the brace to walk
and returns to it to fire; the lance recoils along its own axis and the strut's slide section works on
the shot. It never fires while moving — the fire clip is stationary and the move clip carries no recoil
translation at all (asserted in the tests). The muzzle is a clean cyan-white line produced by effects,
not geometry; the impact is a small engineered flash. Sound (Bible): a sharp, dry crack with a short
metallic ring — specified, not produced. Accessibility: role reads from silhouette (the lance line and
the brace), state from posture and luminance, not hue. Boundaries: no root motion, no simulation
authority in animation; the facing sweep stays runtime-owned (`SPEC-MOV-010`).

**REVIEW.** Fields checked against the sources in §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

Authority: [concept-fidelity.md](concept-fidelity.md) (owner ruling 2026-09-06: the concept images define
what the asset looks like; gameplay rules bound the concept, never replace it). Every value below is
measured on the built rest stance (`build-manifest.json` → `concept_measurements`) and re-checked against
that file value by value: `concept-v1` printed three figures here (visor z, cowl top, strut length) that
disagreed with the manifest it names as its source, and they are corrected in `concept-v2`.

| Item | Value | Basis |
|---|---|---|
| Authored height H | **200.0 cm at the pod tops**; 213.1 cm overall to the antenna tips (1.065 H) | concept-fidelity.md: H defined at the cowl/pod top; canon "slightly taller than the Surveyor" (Surveyor authored at 176 cm) |
| Overall envelope | x −89.0 … +194.5 (283.5 cm long), y ±48.6 (97.2 cm wide), z 0 … 213.1 | built bounds; longer than tall — a line-fire frame, not a hero body |
| Brace | foot centres 128 cm apart along +X (0.64 H), 172 cm heel-to-toe (0.86 H), 70 cm apart across +Y (0.35 H); body pitched 8° forward | concept-fidelity.md item 1 **as corrected on the pixels** (§8.2) |
| Rail-lance | axis level at z 132 (0.66 H) on the centreline; 212.5 cm long (1.06 H); **clear shaft 19 cm deep (0.095 H), thickest collar 22 cm (0.110 H), muzzle head 19 cm (0.095 H)**, 20 cm across the cyan sleeve (0.10 H); muzzle 110 cm past the leading toe (0.55 H) and 132 cm past the foot centre (0.66 H) | concept-fidelity.md item 2 **as corrected on the pixels** (§8.2, §8.7) |
| Cowl and pods | visor at z **178.3** (0.89 H), cowl top **197.0** (**3.0** cm under the pod tops); pods 44 × 28 × 26 (0.22 / 0.13 H) with a 4 cm charcoal rear closure under a pale top cap, tops at H, centres 60 cm apart (0.30 H), 88 cm across the pair (0.44 H) | concept-fidelity.md items 3–4 **as corrected** (§8.2, §8.7) |
| Upper assembly | deck, both pods and the cowl carried **back over the hips**: spine origin at x −42, 0.80–1.00 H silhouette band centroid **−28.4 cm** spanning −75.0 … +18.0 (concept −30.4 / −30.2, −75.4 … +16.6) | concept-fidelity.md item 5 **as added** (§8.7) |
| Body | narrow charcoal core 34 cm wide between open flanks; hips at (−6, ±35, 112); `Target_Anchor_Center` at (−4, 0, 138) | concept-fidelity.md item 5 |
| Legs | thigh 60 cm (0.30 H), shin 50 cm (0.25 H), knee disc Ø 16 cm (0.08 H), ankle at z 27 (0.135 H), block foot 44 cm long (0.22 H) with a raised toe plate on its own hinge | concept-fidelity.md item 7 **as corrected** (§8.2) |
| Recoil strut | (−46, −20, 112) → (−63.7, −27.3, 69.4) → (−80, −34, 30); **89.9** cm long, Ø 6.8 cm (slim: < half the leg width), slide joint at 52% of its length, landing **10.5** cm from the trailing ankle over the heel, and **solved from the trailing leg in every clip** (§8.7) | concept-fidelity.md item 6 |
| Draw scale | ×1.60 at runtime (`PresentationScale`, `EntityType::Soldier`): 320 cm tall, 454 cm long, muzzle 311 cm ahead of the pivot | `EchoesEntityView.cpp:1809-1812`; **not part of the asset** |
| Footprint | `SPEC-UNIT-002` "Logistics Footprint 2" is the **population cost** (`units.json` `population_cost: 2`). The **simulation collision footprint** is a 25 cm square. The **visual envelope** (283.5 cm long, 454 cm at ×1.6) overhangs it far more than the Surveyor does; recorded as a deviation in §8.3 | `Requirements.md:682`, `EchoesContentSubsystem.cpp:361-363` |
| Units / axes / pivot | cm; +X forward (the lance axis), +Y right, +Z up; **root on the ground plane (z = 0) on the centreline** — the brace is asymmetric fore-and-aft, so the origin sits **2.0 cm ahead of the sole-contact midpoint** (`Foot_Contact_L` x −66, `Foot_Contact_R` x +62) and 6.0 cm ahead of the ankle midpoint (−70 / +58); **Nanite OFF** | contract `import_policy`; card; `build-manifest.json` → `units.pivot_offset_cm`, asserted by `test_rest_stance_is_grounded_and_forward_facing` |

## 4. Geometry and rig (revision `ebs-mer-unt-002-concept-v4`)

Generator: [build_lancer.py](build_lancer.py) on the mesh kit and the skeletal kit
([../tools/ebs_skelkit.py](../tools/ebs_skelkit.py)). The rest stance **is** the concept's braced firing
stance: the right (+Y) leg leads planted ahead, the left (−Y) leg trails and carries the recoil strut,
the lance is level on the centreline at hip height, and the upper assembly (deck, both pods, cowl) is
pitched 8° forward about the spine head, lifted by a solved correction so the pod tops land exactly on
H = 200. Limb parts are authored in their local frames (pivot at the joint, limb along +X) and placed by
pitch; both legs use the **same** part lengths and are mirrored about the hip line, so one rig drives the
brace. The two-bone leg solve is in the generator (`solve_leg`), so the stance targets — not hand-tuned
angles — are the constants.

| Concept item (concept-fidelity.md) | Built |
|---|---|
| 1 braced stance | hips (−6, ±35, 112); lead knee (40.2, +35, 73.7) → ankle (58, +35, 27); trail knee (−52.2, −35, 73.7) → ankle (−70, −35, 27); toes at ±(66 / −62, 8). Mirror brace: the lead knee bows forward of the hip–ankle line, the trailing knee back |
| 2 rail-lance | one assembly on `lance_barrel`: charcoal body, **two** parallel rails at z 132 ± 6.5 (19 cm envelope, 0.095 H), a **continuous cyan channel** between them at z 128.5–135.5 running 96% of its length, a 32 cm breech, **three** collar clamps at x 72 / 114 / 156 built as blocks **above and below** the cyan band (22 cm envelope, 0.110 H), an 18 cm muzzle head built the same way, a 4.5 cm muzzle plug and a bore ring; `Muzzle_Flash_01` on the muzzle face. Nothing forward of the breech crosses the channel, so the cyan reads as **one 140 cm stroke** from the side (§8.7) |
| 3 low sensor cowl | ceramic wedge between the pods (two stacked blocks plus a nose block), horizontal cyan visor slot across the front face at z 178.3, charcoal bezel and a charcoal rear closure over its lower three quarters, no neck (it sits on the deck) |
| 4 shoulder pods | two pods 44 × 28 × 26 with tops at H: a 40 cm ceramic shell, a 4 cm **charcoal rear closure** under a pale top cap (concept REAR panel), a charcoal cradle under each, a cyan strip on the **outer** face and a 13 cm antenna rod rising to 213.1 |
| 5 exposed flanks | no ceramic skirt anywhere: a transverse hip beam, exposed hip hubs, a narrow charcoal core, four thin flank struts, two flank braces and two large flank hubs (Ø 18) between the core and the deck, a 44 cm-wide rear tail beam carrying the strut anchor, and a belly pan — all charcoal, all visible from the side. The core column, the flank members and the shoulder yoke sit **aft with the deck they carry**; the lance mount arm stays on the hip line |
| 6 rear-leg recoil strut | `strut_upper` (rod + anchor + slide gland) and `strut_slide` (piston + foot joint) on two bones, Ø 6.8 cm, running behind the trailing shin at every shared height (asserted) down to the trailing heel, and **solved from the trailing leg in every clip** rather than keyed by hand (§8.7) |
| 7 legs | thigh strut + **a pair of ceramic outer-face plates** + hip hub; shin strut + a pair of ceramic outer-face plates + Ø 16 knee disc with ceramic rings; foot with an ankle hub, pastern, block sole, heel spur and a ceramic plate; a separate toe block with a **raised ceramic toe plate** and claws on its own bone. The leg ceramic is thinner than the charcoal strut in the limb’s own z, so nothing pale faces aft (§8.7) |
| 8 palette | two export slots — `MI_EBS_MER_UnitFrame` (charcoal) and `MI_EBS_MER_UnitCeramic` (pale). Cyan is a review-only pseudo slot folded into the ceramic slot on export; brass trim and plate seams are texture channels. Ceramic-bright share of the silhouette: front 0.265, **rear 0.107** (concept 0.204 / 0.116). The card’s **team-colour vertex mask is NOT authored** — deviation in §8.3, OWNER-QUESTION 3 |
| 18 bones | `root, body, spine, cowl, r_pod, l_pod, lance_yaw, lance_barrel, r_thigh, r_shin, r_foot, r_toe, l_thigh, l_shin, l_foot, l_toe, strut_upper, strut_slide` — identity rest orientation, hinge pitch about +Y at each joint |
| Card sub-objects | `Turret_Y` → `lance_yaw` (yaw ring at (30, 0, 132)); `Barrel_X` → `lance_barrel` (trunnion at (44, 0, 132)). Both are also exported as separate static parts (`SM_EBS_MER_UNT_002_LanceYoke`, `SM_EBS_MER_UNT_002_LanceBarrel`, LOD0/LOD1, GLB + OBJ) authored around their own pivots |
| Sockets (owner ruling 2026-09-07, §8.8) | six, all yaw 0: `Muzzle_Flash_01` (`lance_barrel`, (194, 0, 132)); `Target_Anchor_Center` (`body`, (−4, 0, 138)); **`Rear_Recoil_Strut_Anchor`** (`strut_upper`, (−46, −20, 112)) at the strut's UPPER anchor — the end bolted to the tail beam, where the recoil load passes between the strut and the frame (§8.1, measured); **`Left_Tread_Vector`** (`strut_upper`, (−46, −20, 112)) — **TEMPORARY COMPATIBILITY ALIAS** of that socket at the identical transform, to be dropped when the adapter migration is tested; **`Foot_Contact_L`** (`l_foot`, (−66, −35, 0)) and **`Foot_Contact_R`** (`r_foot`, (62, 35, 0)) at the two sole contact centres — the ground-contact function, which no longer rides on the alias |
| Upper-assembly placement | spine origin (−42, 0, 148 + solved lift): the deck, both pods and the cowl are carried back over the hips so the 0.80–1.00 H band centroid lands at −28.4 cm against the concept’s −30.4 / −30.2 (§8.7) |

Budgets, whole unit: the standalone `SM_…_LanceYoke` and `SM_…_LanceBarrel` parts **duplicate the lance
geometry that is already skinned into `SK_EBS_MER_UNT_002`** on the `lance_yaw` and `lance_barrel` bones
(96 + 244 tris at LOD0, 56 + 168 at LOD1), so they are **alternates** for a static sub-object integration
route, **not additions**: an integration that instantiates the skeletal mesh *and* both static parts draws
the lance twice. The whole unit is therefore **1,972 / 1,136** on the skeletal route and **2,312 / 1,360**
if all three parts are placed together — both inside `REL-ART-028`'s 8,000 / 3,500. Recorded in
`build-manifest.json` → `budgets.whole_unit_triangles` and asserted by
`test_whole_unit_triangle_sum_is_recorded`.

Budgets: LOD0 **1,972** ≤ 8,000; LOD1 **1,136** ≤ 3,500 (the card's LOD1 ceiling is printed "3,3500", an
evident typo; **`REL-ART-028` states the roster rule verbatim** — "≤8,000 triangles, transitioning smoothly
down to ≤3,500 triangles" (`Docs/Requirements.md:2377`) — so 3,500 is the rule, not a guess, and the card
typo is recorded for correction rather than raised as an owner decision, §8.5). Emissive share of the
surface (cyan pseudo slot as a geometry proxy) **6.1%** ≤ 15%. Nanite OFF. Large headroom is deliberately left for the ART_ALPHA
plating pass; the blockout spends triangles on the readable elements only. Triangles by bone, the rest
stance, the concept measurements and every clip definition are in
[build-manifest.json](build-manifest.json).

## 5. Clips (keyframed, Unreal rotators; §4 rig)

Every clip is authored through one poser: a body rotation is countered on both thighs so the planted
feet stay put, the per-leg keys come from the two-bone solve at a target ankle, the **recoil strut is
solved from the trailing leg** (`strut_keys`: the strut's foot joint holds its rest offset from the
trailing ankle, or the piston draws home when the clip stows it), and the body's z translation is
re-solved at **every emitted key** so the lowest sole sits exactly on the ground
(`write_pose` / `ground_drop`). Transitions that fold the legs are emitted densely (30–50 ms) because the
fold is non-linear and the runtime interpolates linearly between keys. No clip keys `root`; no clip moves
the frame along the ground.

| Track | Duration | Keys | Intent |
|---|---|---|---|
| idle | 2.4 s loop (72 frames) | 70 | braced firing stance breathing: a 1° settle on the hips, the cowl scanning ±2.5°, the lance held level on the centreline |
| move | 0.6 s loop (18 frames) | 350 | walk cycle authored for 320 cm/s **at authored scale** (two 96 cm strides per cycle): the frame **leaves the brace** into a travel gait, ankles sweeping ±48 cm about the hip line with a 14 cm swing lift, a ≤ 1.6 cm bob and an 8° toe roll-off before toe-off; the recoil strut is **stowed** (piston drawn 38 cm home) because the trailing leg swings forward past its anchor; the lance is carried level and locked, and the clip carries **no recoil translation at all**. **The unit draws at ×1.60, so at play rate 1.0 the soles travel 512 cm/s in world against a 320 cm/s actor — the integration task must set play rate 0.625** (`integration_play_rate` in the manifest, §8.3) |
| turn | 0.6 s loop (18 frames) | 60 | stationary shuffle inside the brace (alternate 5 cm foot lift, 4° body yaw countered on the lance yaw) while the runtime sweeps heading (`SPEC-MOV-010`) |
| stop | 0.4 s (12 frames) | 132 | settle out of travel into the brace: the feet spread fore-and-aft, the trailing knee rolls into the kickstand and the strut retracts |
| fire | 2.2 s (66 frames) | 675 | **halt** (travel stance, lance 3° nose-up, strut stowed) → **plant** at 0.35 s (into the brace; the strut swings down onto the trailing ankle and the piston runs out 38 cm) → **aim** at 0.75–0.95 s (yaw to 0, lance level on the target line) → **shot** at 1.00 s (the lance translates −26 cm **along its own +X axis**, the frame rocks 2.5° back, and the strut slide works: the body rock plus a 6.5 cm stroke) → **recover** by 1.45 s → back to the brace at 2.2 s. Stationary throughout: the canon line "never fires while moving" is a state rule, and the geometry of this clip cannot be played while `move` is |
| damage | 0.366667 s (11 frames) | 168 | flinch on authoritative damage: the frame rocks 6° back on the brace, the lance lifts 4° and the cowl snaps 3°; no displacement, both soles stay planted (≤ 1.5 cm), and the strut absorbs the rock through its slide because it is solved from the (stationary) trailing ankle |
| death | 1.4 s (42 frames) | 435 | engineered collapse: the braced legs fold (thigh +70 / shin −135 / foot +65), the frame drops and pitches 20° forward, the lance goes nose-down with its muzzle 10.6 cm above the ground, and the strut stays bolted to the folding trailing ankle; the whole frame ends **below** its standing height (193.9 cm against 213.1) and the final pose is held (cosmetic debris ≤ 200 ticks). The fold is deeper and the pitch shallower than `concept-v1` because, with the mass carried aft, a 26° nose-down pitch levered the pods **up** to 211.6 cm (§8.7) |
| cancel | 0.4 s (12 frames) | 126 | order cancelled mid-aim: the lance returns from an 8° yaw / 3° elevation to the carry angle, the strut slide retracts and the frame relaxes back onto the brace |
| restore | single frame (0 frames) | 17 | identity key on the seventeen non-root bones for reconstruction from saved state; never replays a one-shot |

`build_lancer.py` writes the skinned GLBs with these clips (`SK_EBS_MER_UNT_002_LOD0/1.glb`: one skin, 18
joints, 9 animations, the six sockets as child nodes of their joints — two of them, the strut anchor and its
alias, on `strut_upper`) on the verified skeletal-kit
encodings. [pose_review.py](pose_review.py) samples the clips at the seventeen review times, bakes posed
review OBJs and writes their render scenes; the fire samples are named for the canon phases.

## 6. Review evidence (concept-v4 blockout stage)

Evidence root:
`/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-MER-UNT-002/`
(`receipt.json` inventories every file with SHA-256; `review/` rest and posed OBJs plus `pose-manifest.json`;
`scenes/`; `renders/`).

Renders (27 scenes): orthographic front / right / rear / left / top at authored scale with a 180 cm
reference figure (`renders/rest/`); the game's orthographic RTS framing with the unit at ×1.6
(`renders/rest_tactical/`, view blocks copied from `EBS-MER-UNT-001/scenes`); LOD1 front / right / top;
a Surveyor scale scene (`renders/scale_vs_surveyor/`); a context scene with four Lancers, a Surveyor and
the Power Link (`renders/context/`); and seventeen posed stills (`renders/pose_<label>/`: right, front,
top and near-tactical, plus ×1.6 gameplay framing for `move_stance`, `fire_plant`, `fire_aim`, `fire_shot`
and `death_hold`).

All 27 scenes were re-rendered for `concept-v4` and all 13 sheets rebuilt: the 98 scene PNGs came back
byte-identical (the ruling changed sockets, not geometry), and two of the sheets changed because they were
**stale from `concept-v3`** — see §8.8.

Comparison sheets (`renders/concept-compare/`, concept panel cropped into the first cell; exact commands
in [make_sheets.py](make_sheets.py) and §7):

| Sheet | Cells |
|---|---|
| `candidate_side_vs_right.png` | candidate BRACED SIDE VIEW · rest right · fire_aim right · rest left |
| `candidate_main_vs_tactical.png` | candidate main braced view · tactical front quarter · tactical near · tactical default |
| `candidate_silhouette_vs_tactical.png` | candidate tactical silhouette · tactical mono near · tactical front quarter · context near |
| `turnaround_left_vs_right.png` | turnaround LEFT SIDE · rest right · rest left · idle right |
| `turnaround_front_vs_front.png` | turnaround FRONT · rest front · fire_aim front · Surveyor scale front |
| `turnaround_rear_vs_rear.png` | turnaround REAR · rest rear · rest left · fire_shot right |
| `turnaround_top_vs_top.png` | turnaround TOP · rest top · fire_aim top · move_stance top |
| `turnaround_inset_vs_mount.png` | turnaround HIP MOUNT / LEG ATTACHMENT inset · rest right · fire_shot right · fire_plant right |
| `fire_sequence_right.png` | halt · plant · aim · shot · recover · cancel |
| `pose_sheet_right.png`, `pose_sheet_tactical_near.png` | all seventeen posed samples |
| `lod0_vs_lod1.png`, `context_and_scale.png` | LOD1 beside LOD0 front/right; context near and gameplay, the Surveyor scale side, and the death hold at the gameplay framing |

Fidelity checks (concept-fidelity.md), decided on the sheets above by looking at them:

| Check | Result | Pixel evidence |
|---|---|---|
| Side view: fore-and-aft brace with the lance level at hip height and the rear-leg strut visible | **MET** | `candidate_side_vs_right.png` cell 2 / `rest/right.png`: the lance runs level at z 132 (0.66 H) from the hip mount to a muzzle 110 cm past the leading toe; the legs splay 128 cm centre-to-centre with the lead knee forward and the trailing knee back; `rest/left.png` (cell 4) shows the recoil strut as a separate slim diagonal from the tail beam down to the trailing heel, with its gland and piston visible clear of the shin |
| Side view: the upper assembly is carried BACK over the hips | **MET (`concept-v2`; NOT met in `concept-v1`)** | `turnaround_left_vs_right.png` and `candidate_side_vs_right.png`: the deck, both pods and the cowl overhang behind the hip line and the cowl's nose stops behind the lance yoke's cheeks. Measured on the 0.80–1.00 H silhouette band: centroid −28.4 cm spanning −75.0 … +18.0, against **−30.4 cm / −75.4 … +16.6** on the turnaround LEFT SIDE panel and **−30.2 cm / −72.6 … +13.4** on the candidate BRACED SIDE panel. `concept-v1` measured +7.7 cm — 0.18 H too far forward (§8.7) |
| Side view: the lance section and one continuous cyan stroke | **MET (`concept-v2`)** | `rest/right.png` and `rest_tactical/tactical_near.png`: clear shaft 19 cm (0.095 H) against the concept's 19.0 cm, thickest collar 22 cm (0.110 H) against 22.1 cm, muzzle head 19 cm against 17.5–19.0 cm. The collars and the muzzle head are blocks above and below the cyan band, so the channel reads as **one 140 cm stroke** at the tactical framing instead of `concept-v1`'s four dashes whose longest run was 34 cm (§8.7) |
| Front view: narrow body, two pods flanking a low cowl, thin exposed flanks | **MET** | `turnaround_front_vs_front.png` cell 2 / `rest/front.png`: 88 cm across the pods (0.44 H) over a 34 cm core; the cowl sits between the pods with the horizontal cyan visor as the only cyan on the face; the frame between the deck and the hips is open charcoal — four struts, two braces and two hubs with daylight between them, no skirt |
| Top view: the lance runs down the centreline and projects well past the leading foot | **MET** | `turnaround_top_vs_top.png` cell 2 / `rest/top.png`: the lance is exactly on y = 0 with the cyan channel visible either side of the rails, and it clears the leading toe by 110 cm — over half the whole silhouette length lies ahead of the frame, as in the concept's TOP panel |
| Rear view: recoil strut and open frame; no rear armour, and no pale plate facing aft | **MET (`concept-v2`; PARTLY REFUTED in `concept-v1`)** | `turnaround_rear_vs_rear.png` cell 2 / `rest/rear.png`: nothing closes the back of the frame — the hip beam, tail beam, core and flank struts read as separate members and the strut crosses the trailing thigh as a distinct diagonal; cell 4 (`fire_shot/right.png`) shows the slide section extended on the shot. The **ceramic now forms outer faces only**: ceramic-bright share of the silhouette (lum > 0.55 × background, the orange reference figure excluded) front 0.265 / **rear 0.107**, against the concept panels' 0.204 / **0.116**. `concept-v1` measured 0.372 / 0.323 by the same method, i.e. three times the concept's rear ceramic (§8.7) |
| Tactical framing: reads as sustained ranged fire, forward-facing, and not as a hero body | **MET** | `candidate_silhouette_vs_tactical.png` cells 2–3 and `context_and_scale.png`: at the near tactical framing the mono silhouette is a low braced wedge with one long horizontal line through it; the frame is 283.5 cm long against 213 cm tall, so the read is a weapon carried by a frame, not a torso with arms. In the context scene four Lancers at ×1.6 keep their lance lines separate at 300–400 cm spacing beside the Power Link and a Surveyor |
| Sockets (owner ruling 2026-09-07): the strut anchor at the frame-side end, the alias on the same transform, the foot contacts at the soles | **MET (`concept-v4`)** | `rest/left.png` and `candidate_side_vs_right.png` cell 4: the strut reads as one slim diagonal leaving the body's lower rear and landing over the trailing heel, so it has exactly two ends and only the upper one touches the frame. Measured on the concept pixels: the rod enters the body silhouette at z = 113.9 cm (0.57 H) on the turnaround LEFT SIDE panel and z ≈ 112 cm on the REAR panel; `Rear_Recoil_Strut_Anchor` is built at z = 112.0 cm (0.56 H), inside the `strut_anchor` clevis and on the `tail_beam`. `Left_Tread_Vector` is bit-identical to it; `Foot_Contact_L/R` sit on z = 0 inside each block foot, 50+ cm away (§8.1) |
| Canon: slightly taller than the Surveyor and narrow | **MET** | `scale_vs_surveyor/front.png` and `right.png`: 200 cm at the pod tops against the Surveyor's 176 cm mast cap (+13.6%), and 97 cm across against the Surveyor's 214.5 cm arm span |
| Palette: nothing on it reads as a hero body; cyan only on the lance channel, visor and small strips | **PARTIAL** | `rest/*.png`: geometry and slot assignment are correct (6.1% cyan by area, pale ceramic only on the pod shells and top caps, the cowl above its charcoal closure, the deck plate, the limb outer-face plates and the toe plates), but this is a **blockout with flat review colours**. Brass edge trim, plate seams and the ceramic micro-noise normal are texture channels that do not exist yet, so the "pale ceramic over charcoal" read cannot be judged finally until the 2048² stack is authored (§8.6). The card's **team-colour vertex mask is not authored at all** and cannot be from this package — deviation in §8.3, OWNER-QUESTION 3 |

Checks: **38 structural tests** in [test_lancer_build.py](test_lancer_build.py) (contract inventory
including every concept part; the corrected proportion bands for the brace, the lance, the head and pods,
and the legs; **the fore-aft placement of the upper assembly against the two concept side panels**; **the
lance's per-section depth against the concept's own shaft and collar**; **the cyan channel reading as one
stroke, with every collar and the muzzle head clear of the channel band**; **the ceramic forming outer
faces, not the rear**; the strut's slimness, run and clearance from the trailing shin, **plus its
attachment to the trailing ankle in every sampled frame of every clip and the exactness of the aim solve
at the rest pose**; **the walk speed recorded against the ×1.60 draw scale**; the 18-bone hierarchy and
sides; every polygon bound; **the six socket names, bones and positions with the alias contract asserted in
the socket purpose**; the card sub-objects and their pivots; budgets, emissive share and the two export
slots; clip inventory with no root track and an identity restore key; **every clip's duration asserted to
be a whole 30 fps frame with its exact frame count, and the §5 clip table's printed durations and key
counts bound to the built clips**; **the frozen `gap-decisions.json` eleven-track inventory recorded
against the built nine, with the three attack clips asserted to be owed rather than delivered**; **the
whole-unit triangle sum with and without the duplicated sub-object parts**; **the root's fore-aft offset
from the sole-contact midpoint**; the fire clip's recoil along the
lance axis and the strut slide along the strut axis, the level aim and the pulled-back muzzle on the
shot; the move clip leaving the brace and carrying no recoil; the death lance nose-down without breaking
the ground; no vertex below −1 cm in any clip sampled at **10 ms**; planted soles within ±1.5 cm through
every hold pose and through the whole braced section of the fire clip; determinism of both the static and
skinned exports), all passing. Seven of the thirty-eight are the `concept-v4` socket contract (§8.8): the
six socket names, bones and positions; the strut anchor at the **upper** end (on the `strut_upper` head,
inside the `strut_anchor` clevis and on the `tail_beam`); `Left_Tread_Vector` asserted to be an alias at
the **identical** transform and marked as one in its purpose; the foot-contact sockets asserted to be at
the soles (z = 0, inside each footprint, on the mesh's lowest surface) and **not** at the strut; the
anchor socket asserted to hold the clevis in every frame of every clip while the strut solve swings the
rod; **no tread, track, roller, wheel or bogie component in either LOD**; and the confirmed LOD1 bound
with its record wording. `--check` reports `ok` with no drift over **60 artefacts** — the 14
exports, the 2 rest review OBJs, the **17 posed review OBJs** and the **27 render scenes** (§7). Author's
inspection of every sheet.

**Not covered:** no Unreal import has been run for this package (the coordinator runs imports serially
afterwards); no textures; no in-engine playback; no gate or owner acceptance.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-UNT-002"
R="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z"
python3 build_lancer.py --evidence-dir "$R/EBS-MER-UNT-002"
python3 pose_review.py  --evidence-dir "$R/EBS-MER-UNT-002"
for s in "$R/EBS-MER-UNT-002/scenes/"*.json; do python3 ../tools/ebs_render.py --scene "$s" --out "$R/EBS-MER-UNT-002/renders/$(basename "$s" .json)"; done
python3 make_sheets.py --evidence-root "$R"          # runs the thirteen ebs_sheet.py commands, in this order
python3 build_lancer.py --evidence-dir "$R/EBS-MER-UNT-002" --check    # read-only: rebuilds into a temp dir and compares hashes
python3 test_lancer_build.py
python3 ../tools/ebs_make_import_job.py --manifest build-manifest.json --evidence-dir "$R/EBS-MER-UNT-002" \
    --destination /Game/Echoes/Production/MER/UNT/EBS_MER_UNT_002 \
    --out "$R/EBS-MER-UNT-002/import/import-job-ebs-mer-unt-002-concept-v4.json"   # the in-engine expectation for the coordinator's serial import
python3 ../tools/make_evidence_receipt.py --evidence-dir "$R/EBS-MER-UNT-002" --package EBS-MER-UNT-002 --manifest build-manifest.json --stage "BLOCKOUT (concept-v4: owner ruling of 2026-09-07 implemented)" --command ... --note ...   # exact strings in receipt.json
```

**The import job.** This package never runs `UnrealEditor-Cmd` — the coordinator runs the imports
serially afterwards — but it does ship the machine-readable expectation that run is checked against.
`import/import-job-ebs-mer-unt-002-concept-v4.json` is derived from `build-manifest.json`, so it carries
`revision: ebs-mer-unt-002-concept-v4`, the **six** sockets
(`Foot_Contact_L`, `Foot_Contact_R`, `Left_Tread_Vector`, `Muzzle_Flash_01`, `Rear_Recoil_Strut_Anchor`,
`Target_Anchor_Center`), the eighteen bones, `lod0_triangles: 1972` and every clip's frame-aligned
duration. `ue_import_inspect_skeletal.py` passes `expected` through to its report untouched, so running
the older `concept-v3` job — whose socket list is the pre-ruling three — would produce a report whose
expectation block silently contradicts the asset. Use the `concept-v4` job.

`make_sheets.py --dry-run --evidence-root "$R"` prints the thirteen `ebs_sheet.py` command lines
verbatim (tile order, `--cols`, `--cell` and the normalized `--crop` of each concept panel), so the sheet
hashes reproduce from this record alone.

**What `--check` covers.** It rebuilds every export, the two rest review OBJs, the five base render
scenes and — through `pose_review.bake` into the same temporary directory — the seventeen posed review
OBJs and the twenty-two pose scenes, and compares all sixty against what is on disk:
`{"compared": {"outputs": 14, "review": 2, "posed_review": 17, "scenes": 27}}`. `review/pose-manifest.json`
is deliberately **outside** the comparison because it stores absolute paths, which are a property of the
run rather than of the geometry. In `concept-v1` this gate compared only the 14 exports and the 2 rest
OBJs — 16 of the 185 inventoried artefacts — while §6 presented it as the reproducibility evidence for
the package; that gap is closed here. The renders and the sheets are still outside it: they reproduce
from the scenes and the command lines above, and every render directory carries its own
`render-manifest.json` with the scene hash and each PNG's hash.

## 8. Decisions, deviations and open items

### 8.1 The socket set (`Rear_Recoil_Strut_Anchor`, its alias, and the foot contacts)

**Owner ruling, 2026-09-07 — the decision:**

> "Correct the socket interpretation. Our recorded Lancer decision specifies `Rear_Recoil_Strut_Anchor`,
> with `Left_Tread_Vector` retained temporarily as a compatibility alias. Left-foot ground contact is a
> different function; it should have a separately named foot-contact socket if needed. Preserve the
> two-legged concept."

~~**Superseded, `concept-v1`–`concept-v3` (struck 2026-09-07 by the ruling above; kept for the record):**
The card names a socket `Left_Tread_Vector`, but this frame walks on legs — the concept has no treads and
the canon row calls it "a two-legged line-fire frame". The socket is kept **at the card's exact name** and
placed at the LEFT (trailing) foot's ground contact, (−66, −35, 0). The conflict is recorded here rather
than resolved by renaming, exactly as `EBS-MER-UNT-001` does for the Surveyor card's "rotates mechanical
treads" wording (its §8.1); the socket's `purpose` string carries the same note into the imported asset,
and a test asserts it. Renaming needs an owner or gate decision.~~ That reading was wrong on the ruling's
own terms: it put the card name on the foot, which silently merged two different functions — the recoil
anchor and left-foot ground contact — onto one socket.

**Built in `concept-v4`:**

| Socket | Bone | Position (cm) | Function |
|---|---|---|---|
| `Muzzle_Flash_01` | `lance_barrel` | (194, 0, 132) | rail-lance muzzle face; the canon cyan-white line effect origin |
| `Target_Anchor_Center` | `body` | (−4, 0, 138) | body centre; damage acknowledgement, selection, health reference |
| **`Rear_Recoil_Strut_Anchor`** | `strut_upper` | **(−46, −20, 112)** | the recoil strut's **frame-side (upper) anchor** — see the measurement below |
| **`Left_Tread_Vector`** | `strut_upper` | **(−46, −20, 112)** | **TEMPORARY COMPATIBILITY ALIAS** of the row above, identical bone / position / rotation; drop it when the adapter migration is tested. Marked as an alias in its `purpose` string, so the note travels into the imported asset |
| **`Foot_Contact_L`** | `l_foot` | (−66, −35, 0) | LEFT (trailing) sole ground-contact centre |
| **`Foot_Contact_R`** | `r_foot` | (62, 35, 0) | RIGHT (leading) sole ground-contact centre |

**Which end of the strut, and why — measured.** The concept's strut is one slim diagonal member with two
ends: an upper end bolted to the lower rear of the body, and a lower end landing on the trailing ankle.
Only the upper end touches the frame, so it is the end across which the recoil load passes between the
strut and the frame; the lower end passes that load to the ground through the trailing foot. The socket is
therefore at the **upper** end. Measured on the pixels:

- **Turnaround LEFT SIDE panel** (segmented at 0.545 cm/px, ground on the sole line, H = 200 cm at the pod
  top — the same scaling that reproduces this record's own 1.065 H antenna figure): the slim rod is a
  separable 10–11 px run (≈ 5.5–6 cm, against the built Ø 6.8 cm) from source row y = 295 downward; at
  y = 292 it is no longer separable from the body mass. The rod therefore enters the frame at
  y ≈ 293 → **z = 113.9 cm = 0.57 H**. Built anchor: **z = 112.0 cm = 0.56 H**, 1.9 cm (0.010 H) below the
  reading.
- **Turnaround REAR panel — a quarter view, and it does NOT corroborate the side panel.**
  ~~(0.509 cm/px by the same method): the strut's upper clevis reads at y ≈ 721 → **z ≈ 112 cm**,
  cross-checking the side panel.~~ *(struck 2026-09-07: the 0.509 figure was derived from the wrong
  landmark and the agreement with the built 112.0 cm was an artefact of that error.)* Re-measured
  row by row on the same file: the pale **pod top caps** first appear at source row **y = 571** (the
  row where two new outer runs open at x 423–427 and 542–549 beside the central block; confirmed at
  6× zoom of x 390–620, y 530–650), and the silhouette's last firm row is **y = 943** (rows
  944–946 are the soft contact shadow). That is **200 / (943 − 571) = 0.538 cm/px**, not 0.509 —
  0.509 is exactly 200 / (941 − 548), i.e. scaled from the top of the **central sensor-cowl block**
  at y = 548, which stands 23 px above the pod caps in this view. At the corrected scale the strut's
  upper clevis (its boss spans y ≈ 709–729, its pin at y ≈ 723) reads
  **z = (943 − 721) × 0.538 = 119.4 cm = 0.60 H** — **7.4 cm above the built 112.0 cm and 5.5 cm
  above the LEFT SIDE panel's 113.9 cm**. The panel also fails the record's own antenna calibration by a
  comparable margin: the antenna tips at y = 537 read (943 − 537) × 0.538 = **218 cm = 1.09 H** against
  the LEFT SIDE panel's 1.065 H (213.1 cm) — 5 cm, a 2.4% over-read, which is the size of the
  perspective error a quarter view carries. This is what §8.3 says about this panel — FRONT and
  REAR are quarter views and nothing orthographic can be measured from them — so the REAR reading is
  recorded here as a **quarter-view reading that disagrees by ≈ 7 cm**, not as a cross-check. The
  socket's z comes from the **LEFT SIDE panel alone**.
- The **other** end lands over the trailing heel at z = 30 cm (0.15 H), beside an ankle at 0.135 H — the
  ground end, 82 cm below the anchor.

**Why `strut_upper` and not `body`.** `strut_upper`'s head *is* the anchor, so a socket there sits at that
joint's own rotation centre: it holds the anchor point exactly while the per-clip strut solve (§8.7 fix 2)
swings the rod and runs the piston. `test_strut_anchor_socket_holds_the_anchor_through_every_clip` asserts
it stays within 0.5 cm of the `strut_anchor` clevis centroid at 50 ms across all nine clips, and that the
alias is bit-identical to it. Riding the strut bone also makes the alias's "same transform" exact rather
than approximate. Socket rotations are still checked on the imported asset, not on the stills (§8.4).

**Two-legged concept preserved.** No tread geometry was added and the braced stance is untouched: the
component inventory, every triangle count and every review OBJ are byte-identical to `concept-v3`
(§8.8). `test_two_legged_concept_is_preserved` asserts no component in either LOD carries a
tread/track/roller/wheel/bogie name.

### 8.2 Corrections to the fidelity target (measured, marked in `concept-fidelity.md`)

The target says its numbers were measured on the candidate and the turnaround. Re-measured on the
turnaround LEFT SIDE panel with the panel scaled so H = 200 cm at the pod top (0.545 cm per source pixel,
ground at the sole line), three of them do not match the pixels. Following the `EBS-MER-UNT-001`
precedent, the file is corrected in place with the correction marked, and the build follows the pixels:

- **Brace span.** Target: "feet ≈ 1.0–1.2 H apart along +X". Measured: **0.63 H between foot centres**
  and **0.87 H heel-to-toe** on the turnaround, 0.745 H / 0.88 H on the candidate's braced side view.
  Built 0.64 H / 0.86 H. A 1.0–1.2 H foot separation is also unreachable with the concept's own leg
  lengths (thigh + shin ≈ 0.55 H from a hip at 0.56 H): the legs would not close.
- **Pod separation.** Target: pods "≈ 0.55 H apart". Measured across the whole pod pair on both quarter
  panels: **0.41–0.45 H**, i.e. centres ≈ 0.30 H apart. Built centres 0.30 H apart, 0.44 H across.
- **Leg split.** Target: thigh 0.28 H / shin 0.26 H. Measured **0.31 H / 0.24 H** (the same 0.55 H total).
  Built 0.30 H / 0.25 H — inside the measurement error of both readings.
- **Added:** two antenna rods rise from the pods to 1.065 H in both concept sheets and were missing from
  the target's item list. They are built; H stays defined at the pod tops, and the overall height with
  the antennas is reported separately.
- **Clarified:** "≈ 0.60 H of the lance projects ahead of the leading foot" is ambiguous. Measured
  0.51 H ahead of the leading **toe** and 0.71 H ahead of the leading **ankle**; built 0.55 H / 0.66 H.
  *(`concept-v2` fix: this clarification and item 7's "≈ 0.20 H long" foot were listed here but never
  written into `concept-fidelity.md`, which §1 calls AUTHORITATIVE. Both are now marked in that file, so
  the authoritative record and this section agree.)*

`concept-v2` adds four more, all marked in `concept-fidelity.md` in the same way and all detailed in §8.7:
the **fore-aft placement of the upper assembly** (added to item 5 — the target never said where it sits);
the lance's **per-section depth and the continuity of the cyan stroke** (item 2: "≈ 0.10 H" is the
whole-assembly figure, and the concept's own sections are 0.095 H of shaft and 0.110 H of collar); the
**ceramic as outer faces only** (items 7 and 8, measured on the REAR panel); and the recoil strut as a
**per-clip constraint** rather than a rest-stance shape (rig section).

### 8.3 Deviations from the concept, each with the rule that forced it

- **The derived turnaround's FRONT and REAR panels are quarter views.** In both, the lance swings well off
  the centreline and the legs read as a left-right splay rather than a fore-and-aft brace. Nothing
  orthographic can be measured from them, so they were used only for the across-Y read and the built
  front/rear views deliberately do **not** reproduce their apparent leg spread. The candidate and the
  LEFT SIDE / TOP panels win (owner ruling: the selected concept is the direction). The one z figure
  §8.1 takes off the REAR panel is labelled there as a quarter-view reading that **disagrees** with
  the LEFT SIDE panel by ≈ 7 cm; it is recorded, not used.
- **The clip inventory is nine tracks, not the frozen record's eleven; three named attack clips are
  still owed.** `Docs/VisualAssetPipeline/motion/gap-decisions.json` —
  `production_policy[EBS-PKG-MC-LANCER].required_track_inventory`, the same record the owner ruling
  cites as authority for the socket name, the LOD caps and the legacy alias — asks for **eleven**
  tracks: idle, move, turn, stop, damage, death, cancel, restore and
  **`attack_anticipation` / `attack_execution` / `attack_recovery`**. This package builds **nine**,
  with one 2.2 s `fire` clip carrying all three attack phases as keyed sections (halt 0.00–0.35 s,
  plant 0.35–0.75 s and aim 0.75–0.95 s = anticipation; the shot at 1.00 s = execution; recover to
  1.45 s and settle to 2.20 s = recovery). Rule that forced it: the canon row `SPEC-UNIT-002`
  describes **one** continuous "halts, plants the strut, aims, fires… recovers" action whose plant and
  recovery blends are runtime-owned, and a blockout cannot specify those blend boundaries. **This is a
  substitution, not a delivery:** an integration that looks up `attack_anticipation`,
  `attack_execution` or `attack_recovery` by name will not find them, and the three-way split is owed
  at ART_ALPHA / integration. Recorded in `build-manifest.json` →
  `component_inventory.tracks.source_contract` with the eleven-name list, so the two records no longer
  disagree silently. The manifest's own `tracks.contract` is the **built** nine and is what
  `test_clip_inventory_matches_contract` checks; the eleven-name list is checked separately by
  `test_track_contract_records_the_frozen_record`.
- **The same record names the component `recoil_brace`; this package calls it the recoil strut.**
  `gap-decisions.json` `component_inventory.recoil_brace: 1`; the canon row, the requirement card and
  the owner ruling's socket name (`Rear_Recoil_Strut_Anchor`) all say *strut*. One component, two
  spellings in the records. The ruling's spelling wins here and the difference is recorded in
  `build-manifest.json` → `component_inventory.recoil_strut.source_record_name` rather than silently
  reconciled; renaming the frozen record is outside this package's write scope.
- **Brass edge trim, plate seams, bolts and the ceramic micro-noise normal are texture, not geometry.**
  Rule: `REL-ART-005.MC.LANCER` .TEX_MAPS (a 2048² Albedo / Normal / packed Roughness-Metallic / Emissive
  Mask stack) plus the two-slot family convention; the LOD0 budget is not the binding constraint here
  (1,972 of 8,000 used). **This bullet does not cover the team colour** — see the next one.
- **The card's team-colour vertex mask is NOT authored, and this package cannot author it.**
  `REL-ART-005.MC.LANCER` .MAT_RULE (`Docs/Requirements.md:2385`) says "Albedo channel masked by
  **TeamColor vertex data**", and `REL-ART-028` (`:2377`) says "Team color accent mapping uses **exclusive
  vertex ID masks**". That is a vertex-attribute decision, not a texture-stage one. Measured on the
  exports: `SK_EBS_MER_UNT_002_LOD0.glb` and `_LOD1.glb` primitives carry
  `['JOINTS_0','NORMAL','POSITION','TEXCOORD_0','TEXCOORD_1','WEIGHTS_0']` — no `COLOR_0` and no vertex-ID
  channel; `TEXCOORD_1` is the mesh kit's per-polygon lightmap grid (`ebs_meshkit.py:359-377`), not a mask.
  **Why it is not fixed here:** neither `ebs_meshkit.py` nor `ebs_skelkit.py` writes a vertex-colour
  accessor, and `ArtSource/tools/` is read-only for this package (the skeletal kit's `SKELETAL_ENCODING` is
  verified against UE 5.8.2, so it is not something to change from inside an asset package). Recorded here
  as a deviation and raised as **OWNER-QUESTION 3**. `concept-v1`'s §8.3 cited .MAT_RULE as the rule that
  *forced* the texture route — a misquotation, since .MAT_RULE requires the opposite; that citation is
  withdrawn. **This is roster-wide, not Lancer-only:** `EBS-MER-UNT-001` exports the same attribute set and
  carries the same wording (its README:108), so the coordinator should treat it as one roster item.
- **The walk is authored at authored scale, not at draw scale.** `WALK_STRIDE = 96 cm` over a 0.6 s cycle
  gives 320 cm/s, matching `units.json` `mc_lancer` `move_speed_cm_s` — but the unit draws at
  `PresentationScale` ×1.60, so at play rate 1.0 the soles travel **512 cm/s** in world while the actor
  advances 320: a 60% slide. The manifest now records
  `move_world_speed_at_draw_scale_cm_s: 512` and `integration_play_rate: 0.625`, and a test asserts both,
  so the integration task **sets** the play rate rather than discovering the slide. Rule: `REL-ART-009`
  (play rate follows the authoritative velocity) is satisfied only once the draw scale is divided out.
  Also roster-wide: `EBS-MER-UNT-001` uses the same convention (90 cm strides for 360 cm/s at ×1.5).
- **The concept's dense mechanical detail — hub stacks, cabling, layered plate edges, the multi-toe feet —
  is blocked as struts, plates and hubs.** Rule: this is the BLOCKOUT stage (`stage_boundary` in the
  manifest); plating depth is ART_ALPHA work under the reserved budget.
- **The cyan channel is modelled as a sleeve wider than the rails** so it reads from the side *and* from
  the top, matching both concept panels; the concept's channel is a lit gap between the rails. Rule:
  readability at the tactical framing (gate-50 rule the Surveyor cites) — a modelled gap would vanish at
  the game camera.
- **Visual envelope vs simulation footprint.** The simulation collision footprint of a unit is a 25 cm
  square (`kFixedScale / 8` half extent). The Lancer's visual envelope is 283.5 cm long — 454 cm at the
  runtime ×1.6 — with the muzzle 311 cm ahead of the pivot at draw scale. The concept width wins (owner
  ruling: gameplay rules bound the concept, never replace it; the collision box is a pathing/hit
  quantity, not a visual bound), but the overhang here is far larger than the Surveyor's and it creates
  cases the integration task's `readability_test` must cover: lances crossing neighbouring units' bodies
  in a firing line, selection and hover hit-testing on a 3 m overhang, and the muzzle entering a
  neighbour's tile. Recorded as a deviation, not fabricated as an approved dimension.
- **Fire range vs lance length.** At ×1.6 the muzzle sits 311 cm ahead of the pivot against a 650 cm
  authoritative range, so the projectile effect starts about half-way to the target. This is a
  presentation consequence of the concept's lance length, recorded so the VFX task can plan the beam
  origin rather than discover it.
- **The move clip leaves the mirror brace, and the strut stows with it.** The concept's stance is a firing
  brace with the trailing knee bowed backwards; a walk cannot keep it, because the legs swap roles every
  stride. `move` therefore re-poses both legs into a forward-knee gait and `stop` / `fire` roll the trailing
  knee back into the kickstand. The recoil strut is **bolted to the trailing ankle**, so it cannot follow a
  leg that swings forward past its own body anchor: at mid-stride the ankle reaches x = +42 while the
  anchor is at x = −46, and a strut tracking it would cut straight through the belly pan and the trailing
  thigh. In the travel states the piston therefore draws 38 cm home and the strut reads as a retracted
  member on the tail beam, which is also what the canon line describes ("Halts, **plants the strut**, aims,
  fires… Never fires while moving") — the strut is a firing brace, not a travel member. §8.7 has the
  measurements.
- **The `damage` flinch carries no roll.** A body roll about the centreline lifts one hip and would raise
  a sole 2.4 cm off the ground. Rule: the ground/planting rule for hold poses; the flinch is expressed as
  pitch plus a lance-yaw and cowl snap instead.
- **The lance's absolute elevation is keyed, not its bone pitch.** The fire clip's barrel key is
  `elevation − body pitch`, so the aim holds the lance level on the target line while the frame settles.
  Rule: concept item 2 (the lance axis is parallel to the ground) and the canon "the lance axis… says
  sustained ranged fire".

### 8.4 Rig and export decisions

- Eighteen bones spend the budget on what the concept animates: the two legs with toes (12 with the body
  and root), the lance's two card sub-objects, the two-part strut, the cowl, both pods and a spine that
  carries the pitched upper assembly. Nothing decorative is rigged; the muzzle flash is an effect.
- The recoil is a **translation** of `lance_barrel` along its own +X and the strut works as a translation
  of `strut_slide` along the strut axis, rather than extra bones or scale.
- Both lance sub-objects are also exported as standalone static parts around their own pivots, so the
  integration task can choose a skeletal-mesh route or a static sub-object route without re-authoring.
  Unlike the Surveyor, there is **no existing runtime rig** to stay compatible with, so there is no
  constants conflict to record here.
- Socket rotations will be checked on the imported asset (a skeletal report), not on the stills.

### 8.5 OWNER-QUESTION items (for the coordinator to batch — not addressed to the owner here)

**ANSWERED (owner ruling, 2026-09-07) — the LOD1 ceiling.** `concept-v1` raised the card's "3,3500" as
OWNER-QUESTION 1; `concept-v2` withdrew it to a Requirements.md typo. The owner has now settled it:

> "Confirm 3,500 triangles for Lancer LOD1. '3,3500' is malformed. Both `REL-ART-028` and our recorded
> production decision specify 8,000 LOD0 / 3,500 LOD1."

So **LOD1 ≤ 3,500 is the confirmed bound**, on `REL-ART-028` (`Docs/Requirements.md:2377`: "a maximum LOD0
cap of ≤8,000 triangles, transitioning smoothly down to **≤3,500 triangles** at distance/zoom thresholds")
and the owner ruling together. `REL-ART-005.MC.LANCER` .MESH_PROP's printed "≤3,3500 tris" stands as a
**Requirements.md typo to correct in the card** — a documentation fix outside this package's write scope.
Nothing in the geometry changed for this: the generator already used `LOD1_CAP = 3500` and the built LOD1
is **1,136** (32% of the bound), LOD0 **1,972** of 8,000. The confirmed wording is carried in
`build-manifest.json` → `budgets.lod1_cap_note`, in the receipt notes, in `concept-fidelity.md` and in
`test_lod1_ceiling_is_the_confirmed_bound`.

*Line numbers, corrected 2026-09-07:* every `REL-ART-028` citation in this package printed
`Docs/Requirements.md:2376` — off by one; 2376 is the `### §18.1` heading. `REL-ART-028` is at **2377**
in this worktree's copy and at **2379** in `Project/Docs/Requirements.md` (the two files differ), and
`REL-ART-005.MC.LANCER` .MAT_RULE is at **2385**, not 2384 (2384 is `.TEX_MAPS`). All citations in this
README, in `build_lancer.py` and in the manifest now use the worktree copy's numbering, which is what the
relative paths in this record resolve to. The requirement **IDs** are the stable reference; the line
numbers are a convenience that the two copies do not share.

> **OWNER-QUESTION 1 — visual overhang vs simulation footprint.** The lance puts the muzzle 311 cm ahead
> of the unit's pivot at the runtime ×1.6 draw scale, against a 25 cm collision square and a 100 cm tile.
> That is a much larger overhang than any unit shipped so far. Requested: whether the presentation may
> keep the concept's lance length (with the readability cases in §8.3 handled at integration), or whether
> the Lancer should draw at a lower `PresentationScale` than the other Soldiers.

**OWNER-QUESTION 2 — `Left_Tread_Vector` on a legged frame. ANSWERED (owner ruling, 2026-09-07).**
Asked in `concept-v1`–`concept-v3`: keep the card name on the left foot, or amend the card to a legged
name. The answer was neither:

> "Correct the socket interpretation. Our recorded Lancer decision specifies `Rear_Recoil_Strut_Anchor`,
> with `Left_Tread_Vector` retained temporarily as a compatibility alias. Left-foot ground contact is a
> different function; it should have a separately named foot-contact socket if needed. Preserve the
> two-legged concept."

Implemented verbatim in `concept-v4`: §8.1 has the six-socket table, the measured choice of the strut's
**upper** anchor end, and the alias contract; §8.8 has the change record. The item is closed **as a
question** — the owner has answered it and this package has implemented the answer.

**One coordinator action remains, and it is outside this package's write scope:**
[`ArtSource/production-ledger.json`](../production-ledger.json) still carries the question as open and
the package as it stood before the ruling. Its `EBS-PKG-MC-LANCER` entry records
`"current_revision": "ebs-mer-unt-002-concept-v3"`; a `next` field reading *"Owner ruling on the card's
printed LOD1 ceiling typo and the `Left_Tread_Vector` name on a legged frame; …"*; a stage receipt whose
socket list is the pre-ruling three (`Muzzle_Flash_01`, `Target_Anchor_Center`, `Left_Tread_Vector`) and
whose import line reads *"3 sockets"*; and triangle counts of **1,828 / 968**, which were already wrong
for `concept-v3` (`import/import-report-ebs-mer-unt-002-concept-v3.json` records `lod0_triangles` 1972,
and the `concept-v3` manifest 1,972 / 1,136). The ledger is the roster's production-lane record and this
package may not write it, so the update — revision to `ebs-mer-unt-002-concept-v4`, the six-socket list,
the corrected 1,972 / 1,136, and a `next` field with the ruling struck off — is handed to the coordinator
here and repeated in §8.8 and in the receipt notes.

> **OWNER-QUESTION 3 — team-colour vertex mask (roster-wide).** §8.3. `REL-ART-005.MC.LANCER` .MAT_RULE
> requires "Albedo channel masked by TeamColor **vertex data**" and `REL-ART-028` "exclusive **vertex ID**
> masks", but neither shared mesh kit writes a `COLOR_0` or vertex-ID accessor and `ArtSource/tools/` is
> read-only for asset packages, so no Meridian unit exported so far carries the mask — `EBS-MER-UNT-001`
> included. Requested: whether the shared kits should gain a vertex-colour channel (a tools change, outside
> an asset package's scope) so the mask can be authored as the cards require, or whether the cards should
> be amended to a texture-mask route. Until then the exports carry no team mask at all, and the deviation
> stands on both packages.

### 8.6 Open

**Corrected in `concept-v4` (2026-09-07):** the `concept-v3` amendment below states that `damage` was
retimed "0.35 s → 0.4 s (12 frames)". That is wrong and was never what was built: 0.35 s is 10.5 frames at
30 fps, so the next whole frame is **11**, and the built clip is **0.366667 s** (`build-manifest.json` →
`clips[damage].duration_s`; every other clip is a whole frame too — idle 72, move 18, turn 18, stop 12,
fire 66, death 42, cancel 12, restore 0). The amendment text is corrected in place below.

Re-run nothing here: **no import has been run for `concept-v4`** — the coordinator runs the Unreal
imports serially after this task, so the `import/` evidence in this directory is still the `concept-v3`
run (`import-report-ebs-mer-unt-002-concept-v3.json`, 0 errors, 18 AnimSequences, the pre-ruling three
sockets, plus the two `UnrealEditor-Cmd` logs and `heavy-run-receipt.json`). ~~there is no `import/`
evidence yet~~ *(struck 2026-09-07: that clause was false — the directory holds two full import runs, and
§8.8 and the receipt already said so.)* What is unverified in engine is the **`concept-v4` socket set**
(the report must come back with six sockets, two of them on `strut_upper`) together with socket
rotations, material-slot names and AnimSequence end poses. The machine-readable expectation for that run
is `import/import-job-ebs-mer-unt-002-concept-v4.json` (§7). Also open:
textures (2048² per the card) and therefore the final palette read (§6 PARTIAL); emissive-share
measurement on the textured asset (the 6.1% figure is a geometry proxy); in-engine playback capture; the
seven `PKG-GATE-*` blocking dependencies on the package; gate reviews; owner acceptance. Known minor
items: **at the 0.01 s sampling `test_clips_never_break_the_ground_plane` now enforces**, the `move` clip
bottoms out at **−0.377 cm**, `death` at **−0.355 cm** and `fire` at **−0.285 cm** (inside the −1 cm rule,
little margin; an independent 5 ms scan finds the true minima within 0.02 cm of those, −0.379 / −0.355 /
−0.297). `concept-v1` quoted −0.32 and −0.29 read off a 0.02 s sampling that stepped over the minima it
was meant to bound, so the figure quoted here is now the figure the test visits; the walk's 96 cm stride is long for a
110 cm leg and should be re-timed against a real velocity capture (and see the draw-scale play rate in
§8.3); the deck reads as a flatter slab than the concept's chunky wedge and the pods carry no ceramic
side-relief yet — plating depth is ART_ALPHA work under the reserved budget (6,028 triangles unused); the
context scene imports neighbouring packages' review OBJs, whose hashes at render time are recorded in the
receipt notes so a later re-render can be traced.

### 8.7 `concept-v2` corrections (2026-09-07)

Five defects confirmed against the concept pixels and the built geometry, all fixed here; the revision is
bumped to `ebs-mer-unt-002-concept-v2`, every export, review OBJ, scene, render and sheet is regenerated,
and each fix carries a test so it cannot regress.

1. **The upper assembly was 0.18 H too far forward.** `concept-v1` cantilevered the deck, both pods and
   the cowl out over the lance yoke (`cowl_cowl_visor` at x +47…+50, nosing past `yaw_yoke_cheek` at
   x 50); the concept carries them **back over the hips**. Both side panels were segmented, scaled so
   H = 200 cm at the pod top with the ground on the sole line, and anchored fore-and-aft on the trailing
   heel and the leading toe — an anchoring the legs validate (heel-to-toe 169.3 cm on the turnaround
   against 172 cm built, 1.6%). The 0.80–1.00 H silhouette band's fore-aft centroid measures **−30.4 cm**
   (turnaround LEFT SIDE, band −75.4 … +16.6) and **−30.2 cm** (candidate BRACED SIDE, band −72.6 … +13.4);
   `concept-v1` built **+7.7 cm**, band −40 … +56. The assembly's *length* was already right (86 cm on the
   candidate panel and 92 cm on the turnaround, 93 cm built), so only its position moved: `UPPER_PIVOT` x from −4 to **−42**, and the body's
   core column, flank struts, flank braces, flank hubs and shoulder yoke moved aft with the deck they
   carry, while the hip beam, the lance mount arm and both legs stayed on the hip line. The tail beam
   widened from 22 to 44 cm so the strut anchor at y −20 actually lands on it. Built: centroid **−28.4 cm**,
   band −75.0 … +18.0. Asserted by `test_upper_assembly_is_carried_back_over_the_hips` (centroid in
   [−34, −22], rear edge ≤ −68, front edge ≤ +24, cowl nose behind the yoke cheeks).

2. **The recoil strut hung in mid-air through every travel state.** The strut is bolted to the trailing
   ankle, but `concept-v1` keyed it in place while the trailing leg swung away: measured between the
   centroids of `strut_slide_strut_foot_joint` and `l_foot_ankle_hub` in the posed review OBJs, the gap
   reached **132.9 cm** in `pose_move_pass` (the strut's foot joint at x −89.4, z 46.6 — 47 cm above the
   ground and 131 cm behind the leg it braces), 86.1 cm in `move_swing`, 83.7 in `move_stance`, 52.0 in
   `stop_travel` and `fire_halt`. `move` is the most-seen state in play. The strut is now **solved from the
   trailing leg** (`strut_keys`): the target point is the trailing ankle in the body's own frame plus the
   strut's rest offset, `_aim_rotator` carries the rest strut direction onto it exactly (a closed-form
   two-angle solve, identity at the rest pose), and the slide translation is whatever length that needs.
   *Where the fix cannot be taken literally:* a walking leg swings **forward past the body anchor** — at
   mid-stride the trailing ankle reaches x = +42 against an anchor at x = −46 — and a strut tracking it
   there would run through the belly pan and the trailing thigh. So the travel states (`move` throughout,
   the first 0.2 s of `stop`, the `fire` halt) **stow** it instead: the piston draws 38 cm home and the rod
   keeps its rest angle, which is also what the canon line describes ("Halts, **plants the strut**, aims,
   fires"). `stop` and `fire` blend from stowed to bolted over the plant. Measured now: ≤ 13.7 cm from the
   ankle in every braced, planting and firing frame (rest offset 10.5 cm), and in the stowed frames the
   free end is ≤ 49.9 cm from its own anchor. Asserted by
   `test_strut_stays_bolted_to_the_trailing_ankle_or_is_drawn_home` at 10 ms across all nine clips, plus
   `test_strut_solver_is_exact_at_the_rest_pose`.

3. **The lance's collars and muzzle head were 20–31% deeper than the concept's thickest section.** Measured
   column by column on the candidate BRACED SIDE panel at 0.514 cm/px: clear shaft **19.0 cm = 0.095 H**,
   thickest collar **22.1 cm = 0.110 H**, muzzle end 17.5–19.0 cm. `concept-v1` built a 21 cm shaft
   (0.105 H, fine) but 26 cm collars and muzzle head (0.131 H) and a `lance_muzzle_status` strip sitting on
   top at z 145.9–147.3, taking the silhouette to 28.7 cm (**0.143 H**, 31% over the concept's own thickest
   section). Now: rails at z 132 ± 6.5 (**19 cm, 0.095 H**), collars at z 121–143 (**22 cm, 0.110 H**),
   muzzle head 18 cm long at z 122.5–141.5 (**19 cm, 0.095 H**), and `lance_muzzle_status` deleted. Only the
   breech/mount block still reaches 0.13–0.15 H, which is what the concept measures there too. Asserted by
   `test_lance_section_depth_matches_the_concept`, which also walks every lance component forward of the
   breech.

4. **The cyan channel read as three or four dashes, not one line.** Geometrically the channel ran 98% of
   the lance, but 26 cm-deep collars across an 8 cm band cut it up: at the tactical framing `concept-v1`
   showed 4 separate cyan runs whose longest was **34 cm**. The concept's collars sit **above and below**
   the cyan line, not across it (visible in the candidate's lance close-up). Each clamp and the muzzle head
   are now built as a pair of blocks clearing the channel band, the channel runs to a 4.5 cm muzzle plug,
   and the side profile shows **1 run, 140 cm long** (a raster of the +Y view at 2 cm; the rear of the
   channel is occluded by the body, as it is in the concept). Checked on the re-rendered
   `rest_tactical/tactical_near.png`, not only on the geometry fraction. Asserted by
   `test_cyan_channel_reads_as_one_stroke`.

5. **Ceramic faced aft where the concept has only charcoal.** A 2.2× read of the turnaround REAR panel
   shows hub stacks, struts and brass edges, with pale plate appearing only as a cap on the top quarter of
   each pod and a strip on the central block. `concept-v1` presented a 68 cm ceramic deck plate as the
   rearmost surface across the whole pod span, both pods' full 28 × 26 ceramic rear faces, and leg ceramic
   modelled as boxes wrapping the charcoal struts. Now: the charcoal `deck_frame` is full deck width and
   5.4 cm behind the ceramic `deck_plate`; each pod closes with a 4 cm charcoal back under a pale top cap;
   the cowl gains a charcoal rear closure over its lower three quarters; and the thigh and shin ceramic is
   a **pair of outer-face plates** thinner than the charcoal strut in the limb's own z, as concept item 7
   actually says. Measured on the renders by the same method that reproduces the reported `concept-v1`
   figures (ceramic-bright share of the silhouette, lum > 0.55 × background, the orange reference figure
   excluded): front 0.372 → **0.265**, rear 0.323 → **0.107**, against the concept panels' 0.204 / **0.116**.
   The rear now matches the concept almost exactly; the ratio (0.40) sits *below* the concept's 0.57
   because the flat blockout's front is still brighter than the painted concept's — that closes once the
   brass and seam textures exist (§8.6). Asserted by `test_ceramic_forms_outer_faces_not_the_rear`.

Also corrected in `concept-v2`, without geometry: the three §3 figures that disagreed with the manifest
they cite (visor z 177.7 → **178.3**, cowl top 196.4 → **197.0** and 3.6 → **3.0** cm under the pod tops,
strut length 81.9 → **89.9** cm — the last contradicted the endpoints printed on its own line, whose
length is √(34² + 14² + 82²) = 89.87 — and the strut landing 10.4 → **10.5** cm); the two §8.2 corrections
that were listed here but never written into the authoritative `concept-fidelity.md`; the ground-penetration
figures, now quoted at the sampling the test enforces (§8.6); the withdrawn OWNER-QUESTION on the LOD1
ceiling (§8.5); the un-recorded draw-scale play rate and the un-recorded team-colour vertex mask (§8.3);
and the scope of `--check`, which now covers 60 artefacts instead of 16 (§7).

The `death` clip was re-authored as a consequence of fix 1: with the mass carried aft, `concept-v1`'s
26° nose-down pitch levered the pods and antennas **up** to 211.6 cm — above the standing height, which is
not a collapse. The legs now fold deeper (70 / −135 / 65 against 45 / −92 / 47) and the frame pitches 20°,
so the hips drop instead: the muzzle still lands 10.6 cm above the ground and the frame ends at 193.9 cm.

### 8.8 `concept-v4` — the owner ruling of 2026-09-07, implemented (2026-09-07)

Two owner answers, implemented verbatim. The revision is bumped to `ebs-mer-unt-002-concept-v4`; every
export is rewritten, the manifest and `--check` are regenerated, every scene is re-rendered and every sheet
rebuilt, and each change carries a test that fails on the `concept-v3` contract.

**1. LOD1 ceiling — CONFIRMED at 3,500.** §8.5 has the quoted ruling. No geometry changed for it; the built
LOD1 is 1,136. The open question is marked ANSWERED here, in §8.5, in `concept-fidelity.md`, in
`build-manifest.json` → `budgets.lod1_cap_note` and in the receipt notes.

**2. The socket set — corrected.** §8.1 has the quoted ruling, the six-socket table, the measured choice of
the strut's frame-side (upper) anchor end and the reason `strut_upper` carries it. In short:

| | `concept-v3` | `concept-v4` |
|---|---|---|
| strut anchor | *(none)* | **`Rear_Recoil_Strut_Anchor`**, `strut_upper`, (−46, −20, 112) |
| `Left_Tread_Vector` | `l_foot`, (−66, −35, 0) — the left sole, carrying ground contact under the card's tread name | **temporary compatibility alias**, `strut_upper`, (−46, −20, 112), identical transform, marked as an alias in its purpose string |
| left-foot ground contact | rode on `Left_Tread_Vector` | **`Foot_Contact_L`**, `l_foot`, (−66, −35, 0) — the same point, its own name and its own function |
| right-foot ground contact | *(none)* | **`Foot_Contact_R`**, `r_foot`, (62, 35, 0) |

The two foot-contact sockets are at the heel-to-toe midpoint of each block foot on z = 0 — the mesh's
lowest surface, inside the sole and toe footprint in both x and y (asserted). The alias is not a
ground-contact socket any more and a test asserts it is more than 50 cm from either sole.

**Preserved: the two-legged concept.** No tread geometry, no stance change. Measured: the LOD0 and LOD1
triangle counts (1,972 / 1,136), the per-bone split, every component name and **both rest review OBJs plus
all seventeen posed review OBJs are byte-identical to `concept-v3`**, and all **98 scene render PNGs
re-rendered byte-for-byte identical**. Only the ten exports that embed the revision string changed (`SK_*_LOD0/1.glb`,
`_static.glb`, `.obj`, and the four sub-object GLBs); the four sub-object OBJs, whose headers carry no
revision, are unchanged too.

**Found while re-running the evidence — two stale pose sheets.** `renders/concept-compare/pose_sheet_right.png`
and `pose_sheet_tactical_near.png` on disk still carried the **`concept-v2` damage cell**: `concept-v3`
retimed the `damage` clip, re-baked `review/pose_damage.obj` and re-rendered `renders/pose_damage/`, but
those two sheets were not rebuilt, so they disagreed with the renders they cite. Every other sheet was
current (the eleven that do not include the damage still). Proven, not guessed: rebuilding the `concept-v2`
damage still (the same clip with its key times scaled back to a 0.35 s duration, sampled at 0.10 s) and
substituting only that one cell reproduces the two stale files' exact sha256 —
`21232475ee73970091a5024a0da669248f7a74e230dc34c06638daecd2d773f3` and
`8796c52c732158c22640b72c214029bf1438930795856e9df261171c41a5a58c`. Both sheets are rebuilt here and now
match `renders/pose_damage/`. A third record error surfaced with it and is corrected in §8.6: the
`concept-v3` amendment's "`damage` 0.35 s → 0.4 s (12 frames)" should read **0.366667 s (11 frames)**.

**Not changed by this ruling.** OWNER-QUESTION 1 (visual overhang vs simulation footprint) and
OWNER-QUESTION 3 (the roster-wide team-colour vertex mask) are untouched and stay open (§8.5). No import
has been run for `concept-v4`; the coordinator runs imports serially afterwards, so the `import/` evidence
in this directory is still the `concept-v3` run and the new socket nodes are unverified in engine — the
skeletal report will need to show six sockets, two of them on `strut_upper`. The expectation for that run
now ships as `import/import-job-ebs-mer-unt-002-concept-v4.json` (§7); the `concept-v3` job in the same
directory is superseded and must not be used, because its `expected.sockets` is the pre-ruling three.

**Handoff — what this package cannot write.** Two records outside this package's write scope still carry
the pre-ruling state and are named here so they are not discovered later:

| Record | What it still says | What it needs |
|---|---|---|
| [`ArtSource/production-ledger.json`](../production-ledger.json) | `current_revision: ebs-mer-unt-002-concept-v3`; a `next` field asking for the very ruling that has now been given ("Owner ruling on the card's printed LOD1 ceiling typo and the `Left_Tread_Vector` name on a legged frame"); a stage receipt listing the pre-ruling three sockets and "Import: … 3 sockets"; triangle counts **1,828 / 968**, which were already stale for `concept-v3` (the `concept-v3` import report records `lod0_triangles` 1972) | revision → `ebs-mer-unt-002-concept-v4`; the six-socket list with the alias marked; **1,972 / 1,136**; `next` with the ruling struck off and the remaining items (textures, emissive measurement, in-engine playback, gates) kept |
| `Docs/Requirements.md` — `REL-ART-005.MC.LANCER` .MESH_PROP (line 2383 in this worktree) and .ANIM_RIG | LOD1 ceiling printed "≤3,3500 tris"; the socket list is the card's three names, `Left_Tread_Vector` among them | the typo corrected to **≤3,500** per the ruling, and — when the adapter migration is tested — `Left_Tread_Vector` replaced by `Rear_Recoil_Strut_Anchor` |

Neither is a geometry change and neither belongs to an asset package; both are coordinator actions.

### 8.9 `concept-v4` record corrections (2026-09-07, second pass)

Two independent verifiers attacked the `concept-v4` implementation. **No geometry defect survived** — the
socket set, the strut anchor end, the stance and every triangle count stand — so the revision is **not**
bumped: every export is byte-identical to the first `concept-v4` build (`--check` re-verified, 60
artefacts), the 98 scene renders are untouched and the thirteen sheets are unchanged. What changed is the
**record**, and one missing deliverable. Each item is struck through and dated where it stood, never
deleted.

1. **The REAR-panel cross-check in §8.1 was derived from the wrong landmark.** The record scaled that
   panel at 0.509 cm/px, which is 200 / (941 − 548) — measured from the top of the **central sensor-cowl
   block**, not from the pod caps the method names. Re-measured (pod caps first appear at y = 571, last
   firm silhouette row y = 943) the scale is **0.538 cm/px** and the clevis reads **119.4 cm (0.60 H)**,
   **7.4 cm above** the built 112.0 cm and 5.5 cm above the LEFT SIDE panel — it never corroborated
   anything. The panel also over-reads its own antenna calibration by 2.4% (1.09 H against 1.065 H),
   which is exactly what §8.3 says about a quarter view. §8.1 now records it as a **disagreeing
   quarter-view reading**. The socket's z still comes from the LEFT SIDE panel alone, and the geometry
   did not move.
2. **§8.5 claimed nothing was outstanding for the coordinator.** `ArtSource/production-ledger.json` still
   records the answered question in its `next` field, `current_revision: ebs-mer-unt-002-concept-v3`, the
   pre-ruling three-socket stage receipt and stale 1,828 / 968 counts. The ledger is outside this
   package's write scope, so it is now named as the one remaining coordinator action in §8.5, in the
   §8.8 handoff table and in the receipt notes.
3. **No `concept-v4` import job existed.** The newest job in `import/` was the `concept-v3` one, whose
   `expected.sockets` is the pre-ruling three with `Left_Tread_Vector` at the left sole — so the ruling's
   own deliverable had no machine-readable in-engine expectation. `import-job-ebs-mer-unt-002-concept-v4.json`
   is generated here from the manifest (§7) and inventoried in the receipt.
4. **§8.6 said there was no `import/` evidence at all.** False: the directory holds two full import runs
   (1.2 MB of `concept-v3` report, three `UnrealEditor-Cmd` logs and a heavy-run receipt), and §8.8 and
   the receipt already said so. The clause is struck and rewritten to what is actually open — the
   `concept-v4` socket set is unimported.
5. **The §5 clip table still printed `damage` as 0.35 s** — the exact 10.5-frame value the `concept-v3`
   amendment exists to eliminate — while the built clip is 0.366667 s (11 frames), as §8.6 and the
   amendment both already said. Corrected, and every row now prints its frame count. The table is bound
   to the generator by `test_readme_clip_table_matches_the_built_clips`, and frame alignment itself is
   asserted by `test_clip_durations_are_whole_frames`; before this pass **no test in this package
   asserted either**, despite the amendment claiming they did.
6. **The clip inventory disagreed with the record the ruling leans on.** `gap-decisions.json`
   `required_track_inventory` lists **eleven** tracks with `attack_anticipation` / `attack_execution` /
   `attack_recovery`; this package builds nine with one `fire` clip. That substitution — and the fact
   that the three named clips are **still owed** — is now a deviation in §8.3 and a
   `component_inventory.tracks.source_contract` block in the manifest, asserted by
   `test_track_contract_records_the_frozen_record`. The same record's `recoil_brace` spelling against
   this package's `recoil_strut` is recorded beside it.
7. **`REL-ART-028` was cited one line short in six places** (`Docs/Requirements.md:2376` is the §18.1
   heading; the requirement is at **2377** here and 2379 in `Project/`), and `.MAT_RULE` at 2384 instead
   of **2385** (2384 is `.TEX_MAPS`). Fixed in the generator, the manifest, the receipt and the three
   README sites, with the two-copy offset recorded in §8.5 so the next reader does not re-derive it.
8. **The whole-unit triangle sum was nowhere on record**, and the two standalone lance parts duplicate
   geometry already skinned into the SK. Added as `budgets.whole_unit_triangles`
   (1,972 / 1,136 skeletal-only; 2,312 / 1,360 with both sub-objects) with the alternates-not-additions
   sentence in §4.
9. **The pivot was described as the ground-contact centre between the braced feet**, but the brace is
   asymmetric and the root sits 2.0 cm ahead of the sole-contact midpoint (6.0 cm ahead of the ankle
   midpoint). Restated precisely in §3 and in `units.pivot`, with `units.pivot_offset_cm` recorded and
   the offset now bounded by `test_rest_stance_is_grounded_and_forward_facing`.

Tests: **38** (34 before this pass), all passing. `--check` still reports
`{"check": "ok", "compared": {"outputs": 14, "review": 2, "posed_review": 17, "scenes": 27}}`.

## Concept-v3 amendment — clip durations on the 30 fps frame grid (2026-09-07)

The concept-v2 exports imported into the sandbox project with **`damage`** missing: the Interchange
skeletal import creates no AnimSequence for a clip whose duration is not an integer number of frames at
30 fps, logs no warning and still reports success. A dedicated probe (twelve clips of identical shape,
durations from 1 to 30 frames including four half-frame values) reproduced it exactly: every whole-frame
clip imported with its exact length, every half-frame clip vanished. Evidence:
`…/asset-production-20260906T221157Z/skeletal-clip-duration-probe/` (`probe-record.json`).

concept-v3 therefore snaps every authored duration up to the next whole frame and scales the key times
with it, so the pose at any normalized time — and every posed review still — is unchanged. The rule is
now enforced in the shared kit: `ebs_skelkit.write_skinned_glb` refuses to write an unaligned clip
(`ANIMATION_FPS`, `frame_aligned_duration()`, `retime_clip()`, `SKELETAL_ENCODING['clip_duration']`,
kit revision `ebs-skelkit-v2`), and — since 2026-09-07 — **this package's own tests assert it too**:
`test_clip_durations_are_whole_frames` checks `skel.is_frame_aligned()` and the exact expected frame
count of every clip (72 / 18 / 18 / 12 / 66 / 11 / 42 / 12 / 0) and
`test_readme_clip_table_matches_the_built_clips` binds the §5 table's printed durations and key counts to
the manifest, so neither the retime nor a printed figure can drift unnoticed. ~~and this package's tests
assert that every clip is frame-aligned~~ *(struck 2026-09-07: when that sentence was written no test
here asserted it — the property held only because `build_clips()` self-retimes and the shared kit refuses
an unaligned clip. It is asserted now.)*

Retimed here: `damage` 0.35 s → **0.366667 s (11 frames)** ~~0.4 s (12 frames)~~ *(figure corrected in
`concept-v4`, 2026-09-07: 0.35 s is 10.5 frames, so the next whole frame is 11, not 12; the built duration
has always been 0.366667 s — §8.6)*. The import of the concept-v3 exports reports 0 errors with every clip present
(`import/import-report-ebs-mer-unt-002-concept-v3.json`, run 2 of `import/heavy-run-receipt.json`).
