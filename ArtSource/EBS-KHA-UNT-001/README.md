---
title: EBS-KHA-UNT-001 Tender — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-TENDER
production_asset_id: EBS-KHA-UNT-001
production_maturity: BLOCKOUT
revision: ebs-kha-unt-001-concept-v1
canon_status: CANDIDATE (the tender-candidate sheet replaces the original under a REPLACE decision; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-UNT-001 Tender — production source

Single authoritative source record for the Kharuun Assemblies worker. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-TENDER` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept input | `EBS-CON-KHA-UNT-001` **REPLACE**: "replace the spider-like machine with hose manipulators. The story shows an ordinary cultivator using a staff, carrying a sling, and working with broad forearms… Design living strata, amber wrist nodules, staff and carried Matter. Avoid turning an ordinary person into a generic insect worker." |
| Selected candidate | `…/concept-discovery-20260906/tender-review/tender-candidate.png` (front, REAR VIEW, GATHERING, CULTIVATING) |
| Canon | `SPEC-UNIT-005`, Bible line 551: a stocky Kharuun cultivator carrying a resonance staff and a woven mineral-fibre sling of carried matter across the back; forearms thickened with working strata; amber nodules at the wrists glow while growing. Gathering is a kneeling press of the staff into strata; growing a structure is a slow circling walk that leaves the organism's first ring |
| Gameplay record | `Content/Data/Source/units.json` `ka_tender`: worker, 100 HP, 390 cm/s, 920 cm sight, work rate 9, cargo 10 Matter, unarmed; Stabilize Scar channels 120 ticks |
| Form language | `REL-ART-007`: grown mineral-organic form — faceted basalt strata, hexagonal columns, translucent amber nodules, living root anchors. `REL-ART-007.FAIL`: machined metallic panels or industrial bolts fail the rules |
| Production ID | `EBS-KHA-UNT-001`, planned `SK_EBS_KHA_UNT_001` under `/Game/Echoes/Production/KHA/UNT/EBS_KHA_UNT_001/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Tender is the Assemblies' ordinary worker: it gathers Matter, grows structures,
operates Wells and stabilises scarred ground. It is a person, not a machine, and it is unarmed. What
must be absent: any machined panel, bolt or plate; any cyan (that is the Compact's colour); any insect
or spider reading.

**DETAIL.** Large scale: a stocky humanoid in faceted basalt strata, broad at the shoulder, with a
forked resonance staff and a woven sling of Matter across the back. Medium: thickened forearms with
amber nodules at the wrists, a wrapped kilt and belt, bare wide feet, a faceted crown. Fine (close
camera): scale facets, amber seam lines, the weave of the sling and kilt.

**ACTION.** Gathering is a kneeling press of the staff into the strata; growing is a slow circling walk
that leaves the first ring; the amber nodules read bright only while growing. Delivery presents the
sling and empties it only on the authorised transfer. Sound (canon): stone resonance, a soft crumble on
gather, a low sustained tone while growing — specified, not produced. Boundaries: no root motion; the
circling walk is authored in place and the runtime drives the path.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Crown height | 190 cm — PROVISIONAL | canon "stocky"; reads correctly beside the 176 cm Surveyor |
| Shoulder line | 52 cm (0.27 H); span across the arms 83 cm | measured on the candidate (≈0.30 H), both reported so neither stands in for the other |
| Staff | 180.5 cm (0.95 H), standing taller than the figure | the candidate's staff rises past the crown |
| Leg length | 92 cm (0.48 H); bare soles on z = 0 | candidate proportions |
| Units / axes / pivot | cm; +X forward, +Y right (anatomical right), +Z up; root at ground-contact centre | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-kha-unt-001-concept-v1`)

Generator: [build_tender.py](build_tender.py) on the mesh kit and the skeletal kit. One skinned mesh
`SK_EBS_KHA_UNT_001`. Slots: `MI_EBS_KHA_Strata`, `MI_EBS_KHA_Fibre`, `MI_EBS_KHA_Amber` (the only
emissive). 19 bones: root, pelvis, spine, chest, head, both arm chains (upper, forearm, hand), both leg
chains (thigh, shin, foot), plus `staff` parented to the working hand and `sling` on the chest so the
carried Matter can be shown or hidden. Sockets: `Harvest_Tether_Muzzle` (staff head),
`Cargo_Drop_Anchor` (sling centre), `Center_Hitbox_Socket` (chest) — the worker set the Surveyor uses.
Budgets: LOD0 552 ≤ 4,500, LOD1 408 ≤ 1,800; amber is 2.0% of surface area against the 5% cap.

## 5. Clips (keyframed, Unreal rotators)

| Track | Duration | Intent |
|---|---|---|
| idle | 2.0 s loop | standing cultivator, slow breath, staff planted |
| move | 0.6 s loop | walk authored for 390 cm/s; play rate follows authoritative velocity |
| turn | 0.6 s loop | stationary shuffle while the runtime sweeps heading |
| stop | 0.4 s | forward lean recovered, staff re-planted |
| gather | 1.2 s loop | kneeling press of the staff into the strata (the candidate's GATHERING panel) |
| grow | 1.6 s loop | slow circling walk that leaves the first ring (the CULTIVATING panel), authored in place |
| deliver | 0.8 s | the sling is presented; the load empties only on the authorised transfer |
| stabilize_scar | 1.6 s loop | continuous channel over a scarred tile (`SPEC-UNIT-005`) |
| damage | 0.4 s | flinch, no displacement |
| death | 1.2 s | folds to the ground and settles; held pose |
| cancel | 0.4 s | the staff returns to rest from any work pose |
| restore | single frame | rest pose for reconstruction from saved state |

No clip keys `root`. Every duration is a whole frame at 30 fps (the skeletal kit refuses anything else).
The kneeling `gather` and the `death` fold were solved by forward kinematics on the hip-knee-ankle chain
so the trailing knee rests on the ground and the leading sole is planted; every clip sampled at 50 ms
keeps all geometry above z = −1 cm.

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-UNT-001/` (`receipt.json`, `review/` rest,
empty-sling and twelve posed OBJs, `scenes/`, `renders/`). Renders: orthographic front/right/rear/top
with a 180 cm reference figure, tactical gameplay and near views, LOD1, the empty-sling variant, and a
render set per posed sample. Six comparison sheets in `renders/concept-compare/` put the candidate's own
panels beside the build: front, REAR VIEW (sling), GATHERING, CULTIVATING, loaded-versus-empty and the
tactical read. Checks: 16 structural tests in [test_tender_build.py](test_tender_build.py) — the
concept's parts, no panel or bolt, no cyan slot, heavier forearms, loaded/empty difference, proportions,
the staff standing taller than the figure, rig and sockets, clip inventory with no root track, whole-frame
durations, ground contact for every clip, the kneeling gather, the upright grow, budgets and the amber
area share, determinism — all passing. Not yet: textures, in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-KHA-UNT-001"
python3 build_tender.py --evidence-dir "<evidence root>/EBS-KHA-UNT-001" --skinned
python3 build_tender.py --evidence-dir "<evidence root>/EBS-KHA-UNT-001" --check
python3 test_tender_build.py
for f in "<evidence root>/EBS-KHA-UNT-001/scenes/"*.json; do python3 ../tools/ebs_render.py --scene "$f" --out "<evidence root>/EBS-KHA-UNT-001/renders/$(basename "$f" .json)"; done
```

## 8. Decisions and open items

1. **No Tender asset card exists.** `REL-FAC-026.KA.TENDER` records only gameplay metrics. The build is
   bounded by the equivalent worker card (`REL-FAC-025.MC.SURVEYOR.ASSET`: LOD0 ≤4,500, LOD1 ≤1,800,
   2048² stack, emissive ≤5%) and the bone count is chosen for a humanoid rather than given.
   **OWNER-QUESTION A.**
2. **Emissive is measured by surface area**, as the card states, not by triangle count. On a blockout the
   two differ sharply: amber is 2.0% of area but 20% of triangles, because the body is a few large boxes
   and the seams are many small ones. Both numbers are in the manifest.
3. **The kneeling gather and the death fold were solved numerically**, not eyeballed: forward kinematics
   on the hip-knee-ankle chain gives the trailing knee at ground level and the leading sole planted
   59 cm ahead. The death fall was re-timed so the pelvis drop lags the limb fold, because dropping in
   step with it drove a half-folded leg through the ground.
4. **Surface detail is texture-stage work.** The candidate's scale facets, weave and seam network are not
   modelled; the blockout carries the parts, proportions and stance only. This is the largest visible gap
   on the comparison sheets and it is expected at this stage.
5. Open: textures (2048² per the equivalent card), in-engine capture and playback, gate reviews, owner
   acceptance.

> **OWNER-QUESTION A — no Tender asset card.** The Kharuun roster record gives the Tender gameplay
> metrics but no visual asset manifest, so its triangle, texture, emissive and rig budgets are inherited
> from the Surveyor's card. Confirm that inheritance, or issue a Tender card.
