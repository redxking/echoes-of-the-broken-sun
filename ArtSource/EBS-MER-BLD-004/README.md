---
title: EBS-MER-BLD-004 Aegis Post — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-MC-AEGIS-POST
production_asset_id: EBS-MER-BLD-004
production_maturity: BLOCKOUT
revision: ebs-mer-bld-004-concept-v1
canon_status: CANDIDATE (two REWORK inputs reconciled by the aegis-post candidate; not owner acceptance)
status: Isolated production source; no Unreal integration authorization
---

# EBS-MER-BLD-004 Aegis Post — production source

Single authoritative source record for the Compact's static defence. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-MC-AEGIS-POST` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept inputs | `EBS-CON-MER-BLD-004` **REWORK** (retain the readable rotating gun and the obvious power connection; rework the single ornamental barrel and circular pedestal into the three-legged ceramic mount with a twin-emitter head; supply and offline status must be visible) and `EBS-CON-MER-BLD-008` **REWORK** (retain twin emitters and a readable turret head; replace the bunker and the decorative shield bubble with the canonical mount and a heavy supply coupling; no new shielding mechanic follows; make loss of power obvious) |
| Selected candidate | `…/concept-discovery-20260906/aegis-post-review/aegis-post-candidate.png` (POWERED, OFFLINE, REAR/SUPPLY) with `aegis-post-mount-study.png` |
| Canon | `SPEC-BLD-015.MC.AEGIS`, Bible line 516: a three-legged ceramic mount carrying a rotating twin-emitter head; it fires only while supplied, and severing power drops the head |
| Gameplay record | `buildings.json` `mc_aegis_post`: defence, footprint 2×2 tiles |
| Asset card | `REL-BLD-015.MC.AEGIS.ASSET`: LOD0 ≤4,000 / LOD1 ≤1,600, 2×2 footprint, 2048² PBR, a 2-bone pitch-and-yaw aiming rig, and loss of power darkens all status bands and drops the head within 1 tick |
| Production ID | `EBS-MER-BLD-004`, planned `SK_EBS_MER_BLD_004` under `/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_004/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Aegis Post is the defence that depends on the network. A player glancing at it must
read two things without selecting it: which way it can shoot, and whether it still has power. What must
be absent: a bunker, a pedestal, a fourth leg, a shield bubble, and any status colour other than the one
supply band.

**DETAIL.** Large scale: a tripod of three splayed legs under a compact, wide, flat head. Medium: a
turntable hub between them, twin emitter barrels on the head's front face, a heavy coupling block with
two cables on the hub's rear. Fine: plate seams, brass edge trim and hazard chevrons on the pads —
texture, not geometry.

**ACTION.** Powered: the head sits level and scans slowly, the band lit. Aiming and firing are runtime
angles on the two aiming bones, with a short recoil through the head. Offline: the head droops 34°
nose-down and holds, every status surface dark, nothing implying life.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 2×2 tiles = 400 cm square; every part inside it in every state and pose | `buildings.json`, asserted by test |
| Leg spread | 340 cm across the outer pad edges | traced on the candidate (§8.1) |
| Height | 298.5 cm = 0.87 of the leg spread | traced on the candidate |
| Head | 110 × 204 × 42 cm; 0.60 of the leg spread wide, 0.14 of the height tall | traced on the candidate |
| Hub top | 255 cm = 0.85 of the height | traced on the candidate |
| Units / axes / pivot | cm; +X is the head's rest facing, +Y right, +Z up; pivot at the ground-contact centre between the pads | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-mer-bld-004-concept-v1`)

Generator: [build_aegis.py](build_aegis.py). One skinned mesh `SK_EBS_MER_BLD_004`. Slots:
`MI_EBS_MER_CeramicCivic`, `MI_EBS_MER_CompactFrame`, `MI_EBS_MER_StatusCyan`. Rig: `root`,
`turret_yaw` on the hub and `turret_pitch` at the head trunnion — the two aiming bones the card names,
and nothing else. No root motion. Sockets (4): `Muzzle_Left`, `Muzzle_Right` (on `turret_pitch`),
`Target_Anchor_Center` (on `turret_yaw`), `Power_Coupling` (on `root`). Clips: `powered_idle`, `aim`,
`fire`, `offline`, `restore` — every duration a whole number of frames at 30 fps, which the shared kit
now refuses to violate. Budgets: LOD0 496 and LOD1 352 against the card's 4,000 / 1,600, recorded and
tested as whole-asset sums per the owner's ruling of 2026-09-07.

## 5. States

| State | Read | Authority |
|---|---|---|
| powered | head level and scanning; the supply band and the muzzle cores lit | the post is supplied |
| offline | every status surface moves to the charcoal slot and the `offline` clip droops the head 34° nose-down | supply severed; the card requires both within 1 tick |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-MER-BLD-004/` (`receipt.json`, `review/`
assemblies for both states, LOD1 and four sampled poses, `scenes/`, `renders/`). Renders: orthographic
front/right/rear/top with a 180 cm reference figure, a three-quarter matching the candidate's framing,
a rear supply view and a tactical gameplay pass. Six comparison sheets in `renders/concept-compare/`.
Checks: 17 structural tests in [test_aegis_build.py](test_aegis_build.py) — three legs with two forward,
twin level parallel emitters, the status slot confined to the band and the muzzle cores, offline darkening
every one of them without changing the component set, the offline clip drooping nose-down, exactly two
aiming bones plus the root, the correct bone per component, socket names and sides, the coupling and its
two cables, the four traced proportions, footprint and ground containment in every state and every
sampled pose, frame alignment at 30 fps, the whole-asset budget and determinism — all passing. Not yet:
textures, in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-MER-BLD-004"
python3 build_aegis.py --evidence-dir "<evidence root>/EBS-MER-BLD-004" --skinned
python3 build_aegis.py --evidence-dir "<evidence root>/EBS-MER-BLD-004" --check
python3 test_aegis_build.py
for s in powered offline lod1 pose_powered_idle_050 pose_aim_100 pose_fire_025 pose_offline_100; do \
  python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-MER-BLD-004/scenes/$s.json" \
    --out "<evidence root>/EBS-MER-BLD-004/renders/$s"; done
```

## 8. Decisions and open items

1. **I corrected my own proportions mid-build, and the first ones were wrong.** My fidelity target
   first recorded height ≈ 1.02 of the leg spread, head width ≈ 0.74 and hub top at 0.62 of the height.
   I had measured a region of the candidate card that included its caption text and an inset view. A
   silhouette trace of the main POWERED view alone (669 × 577 px of dark pixels, row spans sampled at
   twentieths) gives 0.87, 0.60 and 0.85, with the head 0.14 of the height tall. The first build made
   from the wrong numbers read as a slab head on two stumps. The corrected build reads as a tripod. The
   struck-through originals stay in [concept-fidelity.md](concept-fidelity.md).
2. **The card's `.MESH_PROP` contradicts everything else in the record.** It describes "a vertical heavy
   turret barrel mounted onto an elevated orthogonal protective concrete pillbox carriage". The canon
   row, both review decisions and the selected candidate all call for a three-legged ceramic mount with
   twin emitters, and decision B explicitly replaces the bunker. This build follows canon and the
   concept. **Owner ruling 2026-09-07: the three-legged mount with twin emitters is confirmed**, and the
   conflicting pillbox wording is to be retired through a coordinated asset-card amendment. That
   amendment has not happened; the card still carries the pillbox line, so anyone reading it will still
   meet the contradiction until it is amended. No canon or concept-review reversal is needed.
3. **Offline is geometry plus slot assignment, not a material promise.** The state mesh moves the band
   and the muzzle cores into the charcoal slot; the `offline` clip supplies the 34° droop. A test asserts
   the component set is identical between states, so a state cannot silently add or drop parts.
4. **The muzzles are dark bezels with a small lit core**, not glowing caps. A first pass slotted the
   whole muzzle block as status, which read as two lamps rather than two emitters.
5. **Collision is one UBX box over the mount**, sized to the knee circle and stopping at the hub top, so
   the head and barrels never form a blocker as they sweep.
6. Open: textures (2048² per the card, including the brass trim and the pad chevrons), in-engine
   capture, gate reviews, owner acceptance.

> **OWNER-QUESTION A — RESOLVED 2026-09-07.** Asked: confirm the three-legged mount, or direct the
> card's pillbox instead. Owner ruling: the three-legged ceramic mount with twin emitters is confirmed,
> following the selected concept and the explicit decision to replace the bunker; the pillbox and
> vertical-barrel wording is to be retired through a coordinated asset-card amendment. The tripod
> silhouette and the twin-emitter aiming assembly are preserved. **The card is not yet amended** — that
> amendment is outside this worktree's authority and remains outstanding. This is production direction,
> not acceptance of the finished asset.
