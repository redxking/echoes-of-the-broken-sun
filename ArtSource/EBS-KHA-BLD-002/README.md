---
title: EBS-KHA-BLD-002 Waystone — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-KA-WAYSTONE
production_asset_id: EBS-KHA-BLD-002
production_maturity: BLOCKOUT
revision: ebs-kha-bld-002-concept-v1
canon_status: CANDIDATE (built to the waystone candidate against canon SPEC-BLD-016); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-KHA-BLD-002 Waystone — production source

Single authoritative source record for the Kharuun mobile supply node. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
[`REL-BLD-016.KA.WAYSTONE.ASSET`](../kharuun-asset-cards.md); maturity is in
[../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-KA-WAYSTONE` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Concept inputs | `EBS-CON-KHA-BLD-002` and `EBS-CON-KHA-BLD-006` |
| Selected candidate | `…/concept-discovery-20260906/waystone-review/waystone-candidate.png` (ROOTED, MIGRATING, ROOT RETRACTION) |
| Canon | `SPEC-BLD-016` Waystone: a tall faceted monolith of dark strata with amber seams that, rooted, sinks a visible ring of root-strata into the ground; uprooted, it lifts on a grown carriage and moves slowly. Rooted or moving is visible from the roots alone. Rooting: preparation, contact, settling, release |
| Gameplay record | `buildings.json` `ka_waystone`: mobile supply node, 390 HP, 500 cm sight, +5 logistics while rooted, 2×2 footprint; uproot 40 ticks, root 60 ticks, 120 cm/s mobile at 125% damage taken |
| Asset card | `REL-BLD-016.KA.WAYSTONE.ASSET` — **provisional**, authored in this worktree, not in `Docs/Requirements.md` |
| Production ID | `EBS-KHA-BLD-002`, planned `SK_EBS_KHA_BLD_002` under `/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_002/` |

## 2. Contextual brief (`SPEC-VISD-008`)

**CONTEXT.** The Waystone is logistics that walks. A player must read one thing above all: whether it is
rooted and supplying, or uprooted and vulnerable. Canon puts that read in the roots, not in a colour.
What must be absent: wheels, tracks, treads, pistons and any machined carriage.

**DETAIL.** Large scale: a tall coursed monolith over a wide root disc. Medium: amber seam bands between
courses, a stepped plinth, roots that either splay across the ground or braid up the shaft, and a
carriage plate on four stubby legs that exists only while migrating. Fine: course chipping, ground-contact
wear on the roots — texture.

**ACTION.** Uproot over 40 ticks: the ring pulls free and the body rises onto the carriage. Migrating at
120 cm/s: the four feet take the load in turn. Root over 60 ticks in canon's four beats — preparation,
contact, settling, release — with the settle overshooting below rest before releasing to it. Sound
(canon): a grinding root-set and a low tone while rooted — specified, not produced.

**REVIEW.** Fields checked against §1 before geometry (2026-09-07). Open items in §8.

## 3. Scale basis

| Item | Value | Basis |
|---|---|---|
| Footprint | 2×2 tiles = 400 cm square; every part inside it in every state and pose | `buildings.json`, asserted by test |
| Rooted root spread | 381 cm | traced on the candidate, sized to sit inside the square |
| Total height | 414 cm = 1.09 of the root spread | traced |
| Shaft width | 95 cm = 0.25 of the root spread | traced |
| Migrating span | 0.92 of the rooted spread | traced; the retraction is a measurable narrowing |
| Tick rate | 20 ticks per second | the Waystone's own 100-tick / 5.0 s construction figure |
| Units / axes / pivot | cm; +X forward, +Y right, +Z up; pivot at the ground-contact centre under the monolith | contract `import_policy` |

## 4. Geometry and rig (revision `ebs-kha-bld-002-concept-v1`)

Generator: [build_waystone.py](build_waystone.py). One skinned mesh `SK_EBS_KHA_BLD_002`. Slots:
`MI_EBS_KHA_Strata`, `MI_EBS_KHA_Amber`. Rig, from the card: `root`, `ring_sink`, `carriage_lift` and
four feet parented to the lift. Sockets (4): `Target_Anchor_Center`, `Matter_Dropoff`,
`Root_Ring_Center`, `Carriage_Front`. Clips: `rooted_idle`, `uproot` (40 ticks), `mobile_move`, `root`
(60 ticks), `restore`, every duration a whole number of frames at 30 fps. Budgets: LOD0 972 rooted and
1,096 in the mobile state, LOD1 544, against the card's 5,000 / 2,200. Amber is 8.7% of surface area
against the `REL-ART-029` ceiling of 15%.

**The roots' splay-versus-braid is a state, not a pose.** A braid is not a rotation of a splay, so the
bones carry the ring sink and the carriage lift while the state carries the root form. Trying to animate
one into the other would have produced roots that stretch rather than retract.

## 5. States

| State | Read | Authority |
|---|---|---|
| rooted | the ring is sunk, the roots splay flat across the ground, no carriage | supplying; +5 logistics |
| uprooted_mobile | the roots retract into braided bundles, the carriage plate and four legs carry the body clear | migrating at 125% damage taken |
| damaged | seams broken and courses chipped | |
| destroyed | the monolith falls and canon's ring is left in the ground | |

## 6. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-KHA-BLD-002/` (`receipt.json`, `review/`
assemblies for the four states, LOD1 and four sampled poses, `scenes/`, `renders/`). Renders:
orthographic front/right/top with a 180 cm reference figure, a three-quarter matching the candidate's
framing, a close root-detail view answering the candidate's own ROOT RETRACTION panel, and a tactical
pass. Seven comparison sheets in `renders/concept-compare/`. Checks: 19 structural tests in
[test_waystone_build.py](test_waystone_build.py) — the contract inventory, four carriage legs present
only while migrating, rooted versus mobile being distinguishable from the roots alone, the migrating
footprint measurably narrower, the four traced proportions, amber being the only emissive and under the
ceiling, the shaft continuous from the plinth, the flat roots under the ground-clutter ceiling, footprint
and ground containment in every state and every sampled pose, the card's own bone plan, clip durations
matching the canon tick counts, the root clip's four beats including the settle overshoot, sockets,
the provisional budget, LOD1 keeping the root ring, and determinism — all passing. Not yet: textures,
in-engine capture, gates.

## 7. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-KHA-BLD-002"
python3 build_waystone.py --evidence-dir "<evidence root>/EBS-KHA-BLD-002" --skinned
python3 build_waystone.py --evidence-dir "<evidence root>/EBS-KHA-BLD-002" --check
python3 test_waystone_build.py
for s in rooted uprooted_mobile damaged destroyed lod1 pose_uproot_050 pose_uproot_100 \
         pose_mobile_move_025 pose_root_060; do \
  python3 ../tools/ebs_render.py --scene "<evidence root>/EBS-KHA-BLD-002/scenes/$s.json" \
    --out "<evidence root>/EBS-KHA-BLD-002/renders/$s"; done
```

## 8. Decisions and open items

1. **I corrected my own card, not the model.** The provisional Waystone card first specified three
   carriage feet. I had written that before tracing the candidate. The candidate's MIGRATING view shows
   **four** legs under the carriage plate, and the concept governs the form, so the card was corrected
   to four and the correction is recorded on the card itself rather than hidden. The build has four.
2. **The rooted and migrating footprints differ, and that is the point.** Canon says rooted or moving is
   visible from the roots alone. The traced candidate makes the migrating span 0.92 of the rooted one, so
   the retraction is a measurable narrowing, not just a re-pose. A test asserts the inequality.
3. **The flat roots are bounded by `REL-ART-030`.** Decorative ground assets on open paths are clamped
   to 20 cm of vertical displacement. The flat roots span exactly z 0 to 20. A first pass gave them a
   15 cm radius on a 6.5 cm centre, which put them 8 cm underground; the fix was the radius, not the
   assertion.
4. **The root clip's feet translate upward as the body descends.** They are children of `carriage_lift`,
   so keying them downward drove the legs through the ground. Keying them up by the same amount keeps
   them planted while the plate lowers onto them.
5. **The shaft needed a flared root mass.** A first pass left a 50 cm gap between the plinth top and the
   first course, so the monolith floated. Two flare tiers now grip the shaft, which is also what the
   candidate shows.
6. Open: textures (2048² packed PBR with micro-noise normals and ground-contact wear on the roots),
   in-engine capture, gate reviews, incorporation of the provisional card into the authoritative
   requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
