---
title: EBS-KHA-BLD-004 Listening Spine — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-LISTENING-SPINE
production_asset_id: EBS-KHA-BLD-004
production_maturity: BLOCKOUT
revision: ebs-kha-bld-004-concept-v1
canon_status: CANDIDATE (built to the listening-spine candidate against canon SPEC-BLD-016); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-BLD-004 Listening Spine — production source

Single authoritative source record for the Kharuun seismic detector. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
[`REL-BLD-016.KA.SPINE.ASSET`](../kharuun-asset-cards.md); maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-LISTENING-SPINE` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept inputs | `EBS-CON-KHA-BLD-004` and `EBS-CON-KHA-BLD-008` |
| Selected candidate | `…/concept-discovery-20260906/listening-spine-review/listening-spine-candidate.png` (LISTENING, DETECTING, ROOTED SOCKET) |
| Canon | `SPEC-BLD-016` Listening Spine: a single tall rib of strata with amber sensor nodules climbing it, set into a rooted socket. **A spine, not a weapon.** Detecting shows nodules lighting in sequence toward the source direction. Sounds: a slow pulse that quickens with movement signatures |
| Gameplay record | `buildings.json` `ka_listening_spine`: detection, 440 HP, 900 cm sight, 120 construction ticks, 2×2 footprint; detects moving signatures within 2,600 cm at 200 cm resolution, lingering 40 ticks |
| Asset card | `REL-BLD-016.KA.SPINE.ASSET` — **provisional**, authored in this worktree, not in `Docs/Requirements.md` |
| Production ID | `EBS-KHA-BLD-004`, planned `SK_EBS_KHA_BLD_004` under `/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_004/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Listening Spine sees further than anything else the faction builds and cannot defend
itself. A player must read that instantly: this thing listens, it does not shoot. What must be absent is
the whole vocabulary of a weapon — no dish, no antenna, no radar, no mast, no turret ring, no barrel.

**DETAIL.** Large scale: one curved tapering rib rising seven metres from a rooted socket, leaning off
its own base axis. Medium: plate courses spiralling up the rib, a line of eleven amber nodules on the
concave leading edge, a stepped collar with roots splayed flat. Fine: plate chipping, nodule seating,
ground wear at the roots — texture.

**ACTION.** Listening: a slow lean cycle, the visual partner of canon's slow pulse. Contact: the rib
leans toward the source while the nodules run their sequence. Offline: the rib slumps off its lean.
Sound (canon): a slow pulse that quickens with signatures — specified, not produced.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 2×2 tiles = 400 cm square; every part inside it in every state and pose | `buildings.json`, asserted by test |
| Root disc | 380 cm | sized to sit inside the square |
| Total height | 711 cm = 1.87 of the root disc | traced on the candidate's LISTENING view |
| Rib base width | 125 cm = 0.176 of the height | traced |
| Tip lateral offset | 89 cm = 0.125 of the height | traced; the rib finishes measurably off its base axis |
| Nodules | 11 | counted on the candidate's DETECTING view |
| Units / axes / pivot | cm; +X forward, +Y right, +Z up; pivot at the socket's ground-contact centre | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-kha-bld-004-concept-v1`)

Generator: [build_spine.py](build_spine.py). One skinned mesh `SK_EBS_KHA_BLD_004`. Slots:
`MI_EBS_KHA_Strata`, `MI_EBS_KHA_Amber`. Rig, from the card: `root`, `socket`, `spine_lower`,
`spine_upper` — four bones, and **not a turret rig**. There is no yaw ring, no pitch trunnion and no
muzzle; the rib leans, it does not aim, and a test asserts the bone names contain none of those words.
Sockets (4): `Target_Anchor_Center`, `Nodule_Base`, `Nodule_Tip`, `Socket_Root`. Clips: `idle_pulse`,
`detect_sweep`, `offline`, `restore`. Budgets: LOD0 602 and LOD1 292 against the card's 3,000 / 1,200.
Amber is 8.4% of surface area in the fully lit contact state, against the `REL-ART-029` ceiling of 15%.

**The sequence is a material animation, not geometry.** Canon's read is nodules lighting *in order*
toward a source. The geometry's job is to make that possible: each nodule is its own component, ordered
base to tip, so the runtime can address them individually. The state assemblies show all-dark and
all-lit only, and the manifest says so rather than implying the ordering lives in the mesh.

## 5. States

| State | Read | Authority |
|---|---|---|
| listening | nodules in the strata slot, rib at rest | |
| contact | every nodule in the amber slot; the ordering toward the source is the runtime's material animation | canon |
| damaged | chips out of the rib's trailing edge | |
| destroyed | the rib snapped at the socket: the socket and roots remain, the rib lies broken beside them | the card's `.STATES` |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-BLD-004/` (`receipt.json`, `review/`
assemblies for the four states, LOD1 and three sampled poses, `scenes/`, `renders/`). Renders:
orthographic front, side and top with a 180 cm reference figure, a three-quarter, a socket close-up
answering the candidate's own ROOTED SOCKET panel, and a tactical pass, for each state. Six comparison
sheets in `renders/concept-compare/`. Checks: 21 structural tests in
[test_spine_build.py](test_spine_build.py) — the contract inventory, no weapon vocabulary in any state
or in the rig, exactly one rib, the three traced proportions, the rib narrowing and leaning further as
it rises, every nodule on the concave leading edge, the nodules ordered base to tip and individually
lit, amber being the only emissive and under the ceiling, the sweep being a lean and never a
translation with the upper rib leading, frame alignment, the socket and roots on the socket bone, the
ground-clutter ceiling, footprint and ground containment in every state and pose, sockets, the
destroyed state snapping at the socket, the provisional budget, LOD1 keeping every nodule resolvable,
and determinism — all passing. Not yet: textures, in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-KHA-BLD-004"
python3 build_spine.py --evidence-dir "<evidence root>/EBS-KHA-BLD-004" --skinned
python3 build_spine.py --evidence-dir "<evidence root>/EBS-KHA-BLD-004" --check
python3 test_spine_build.py
for s in listening contact damaged destroyed lod1 pose_detect_sweep_040 \
         pose_detect_sweep_100 pose_offline_100; do \
  python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-KHA-BLD-004/scenes/$s.json" \
    --out "<evidence root>/EBS-KHA-BLD-004/renders/$s"; done
```

## 8. Decisions and open items

1. **The nodules run up the concave side, and my first pass had them on the convex side.** The
   candidate draws the nodule line on the inside of the curve. I built it on the outside, which read as
   a spine wearing its sensors on its back. A test now checks every nodule against the rib's centre at
   its own height, so the side cannot silently flip.
2. **The rib leans; it does not aim.** Canon says a spine, not a weapon, so the two rib bones carry a
   bend and nothing else. `detect_sweep` has no translation, no roll and no pitch — only the lean, with
   the upper rib leading the lower. A separate test scans every state and the bone names for weapon
   vocabulary.
3. **The sequence is not in the mesh.** Recorded in §4; the manifest carries the same statement under
   `detection_readout` rather than leaving it implied.
4. **The broken rib had to be pulled inside the footprint.** The destroyed state first scattered pieces
   to 389 cm from centre, nearly twice the placement envelope. They now land inside it, like every
   other part in every other state.
5. Open: textures (2048² packed PBR with micro-noise normals), in-engine capture, gate reviews,
   incorporation of the provisional card into the authoritative requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
