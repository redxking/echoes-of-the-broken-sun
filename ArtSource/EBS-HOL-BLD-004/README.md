---
title: EBS-HOL-BLD-004 Phase Anchor — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-HC-PHASE-ANCHOR
production_asset_id: EBS-HOL-BLD-004
production_maturity: BLOCKOUT
revision: ebs-hol-bld-004-concept-v1
canon_status: CANDIDATE (built to the phase-anchor candidate against canon SPEC-BLD-017.HC.ANCHOR); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-HOL-BLD-004 Phase Anchor — production source

Single authoritative source record for the Hollow Choir coherence optimizer. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
`REL-BLD-017.HC.ANCHOR.ASSET` in [../hollow-choir-asset-cards.md](../hollow-choir-asset-cards.md);
maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-HC-PHASE-ANCHOR` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Selected candidate | `…/concept-discovery-20260906/phase-anchor-review/phase-anchor-candidate.png` |
| Canon | `SPEC-BLD-017.HC.ANCHOR`: coherence optimizer, 480 HP, 800 cm sight, 130 construction ticks, 2×2 footprint, 120 Matter / 35 Dawn; charges 5 Dawn every 600 ticks; projects a 700 cm cost-reduction aura. Gameplay row `hc_phase_anchor` agrees on every shared field |
| Asset card | `REL-BLD-017.HC.ANCHOR.ASSET` — **provisional**, authored after the trace, not in `Docs/Requirements.md` |
| Production ID | `EBS-HOL-BLD-004`, planned `SK_EBS_HOL_BLD_004` under `/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_004/` |

## 2. Geometry and rig (revision `ebs-hol-bld-004-concept-v1`)

Generator: [build_phase_anchor.py](build_phase_anchor.py). A three-step hexagonal plinth 300 cm across
corners and 112 cm tall carries a tapering hexagonal spire from a 56 cm base radius to 23 cm at 488 cm,
closing in a pointed apex cap at 640 cm. Magenta arris lines run up all six edges, and a diamond
register zig-zags up the −Y face between 168 and 452 cm. Slots: `MI_EBS_HOL_Vitrified`,
`MI_EBS_HOL_ApronStone`, `MI_EBS_HOL_Magenta`. Rig: `root`, `spire`, `apex`. Sockets (4):
`Target_Anchor_Center`, `Field_Ring_Origin`, `Apex_Beacon`, `Register_Center`. Clips:
`field_active_idle`, `field_lost`, `restore`. Budgets: LOD0 254 and LOD1 158 against the card's
4,000 / 1,600. Magenta is 5.6% against the 12% ceiling.

**The proportions come from the trace.** Slenderness (shaft base ÷ height) is 0.175 against a traced
0.177; the plinth is 0.175 of the height against a traced 0.176; the shaft narrows to 0.41 of its base
at the cap, which is what the candidate measures. Two traced figures were rejected as perspective reads
rather than proportions: the plinth's ~480 px apparent width and the field ring's ellipse.

**The 700 cm aura is a light effect, not geometry.** The candidate draws it as a magenta ground circle.
Its radius belongs to gameplay data; the mesh provides `Field_Ring_Origin` and a test asserts the mesh's
own extent is nowhere near 700 cm, so it implies no radius of its own.

**The lit read is a slot change, not a brightness parameter.** Field active and field lost share their
geometry exactly — the same 254 triangles — and differ only in whether the fourteen arris and register
lines sit in the magenta slot or the vitrified one. LOD1 keeps the two silhouette arrises so the read
survives at distance.

## 3. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-HOL-BLD-004/`. Renders: orthographic front and
top with a 180 cm reference figure, plus three-quarter, register-detail and tactical passes, for each of
the three states and for LOD1, and one posed frame per looping clip. Comparison sheets under
`renders/concept-compare/`: the field-active three-quarter beside the candidate, the register detail in
both power states beside the destroyed state, and the two power states at tactical framing.

**Monochrome separability from the Chorus Loom**, measured on ground-free tactical renders:
intersection over union 0.514, largest band difference 0.490, aspect difference 0.481. The card requires
the Anchor to stay separable from the Loom's posts; this is the measurement, not a verdict.

Checks: 21 structural tests in [test_phase_anchor_build.py](test_phase_anchor_build.py) — the contract
inventory, the taper matching the traced slenderness and plinth fraction and narrowing monotonically,
the aura absent from geometry, the register lying on a face rather than across an edge, the two states
sharing geometry while the lit read changes slot, magenta under the ceiling, **no other faction's
language in any component or slot name**, the destroyed state snapping the spire rather than toppling it
whole, footprint and ground containment in every state, the plinth recorded as a footprint adaptation,
no clip frame passing through the ground, the apex drifting out of phase with the shaft, sockets, frame
alignment, the rig, the provisional card recorded as traced-first, LOD1 keeping the taper and an arris
read, no collision in the skinned GLB, and determinism — all passing.

Headless Unreal 5.8.2 import into the isolated sandbox: both meshes at 3 bones, 4 sockets, 3 material
slots and 3 clips, imported bounds 300 × 260 × 640 cm matching the source, LOD1 at 300 vertices against
LOD0's 516, and no errors, warnings, pipeline property failures or fatals.

## 4. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-HOL-BLD-004"
python3 build_phase_anchor.py --evidence-dir "<evidence root>/EBS-HOL-BLD-004" --skinned
python3 build_phase_anchor.py --evidence-dir "<evidence root>/EBS-HOL-BLD-004" --check
python3 test_phase_anchor_build.py
```

## 5. Decisions and open items

1. **The plinth is a footprint-driven adaptation.** The traced plinth-to-shaft proportion would put it
   near 500 cm across, outside the 400 cm footprint. It is held to 300 cm across corners and the
   adaptation is recorded in the manifest, the fidelity target and a test, under the owner's Array
   Foundry ruling of 2026-09-07: adapt the proportion to the footprint, and do not change navigation or
   placement rules to match a drawing's apparent proportion.
2. **The register was invisible until the hexagon was re-phased.** With a vertex on the −Y axis the
   front face was an edge, so the diamond register straddled it and never showed in the front
   orthographic; the arris tube sat on top of what little was visible. Rotating the hexagon so a face
   faces −Y put the register on the face the front camera looks at, which is how the candidate's
   REGISTER DETAIL panel frames it.
3. **A camera at yaw 0 looks at the −X side, not the +X face.** `perspective_camera` puts the camera at
   `target − arm × forward`, so yaw 0 sits at −X looking toward +X. An earlier register camera was
   therefore pointed at the back of the spire. Diagnosed by reading the camera function rather than
   guessing at the render.
4. **The destroyed state snaps the spire rather than toppling it whole.** A 528 cm shaft laid down
   reaches 300 cm outside the 400 cm footprint — it cannot lie inside its own placement envelope
   unbroken. It falls as a stump and two sections, all inside the footprint. The candidate has no
   destroyed panel, so this is invention constrained by the footprint, and it is labelled as such.
5. **Collision does not travel in a skinned GLB here.** This package was written with
   `include_collision=(lod == 0)`, copied from an older static-mesh generator; every other skeletal
   package in this pipeline exports without collision. Interchange imported the UBX box as a **second
   SkeletalMesh with its own skeleton**, and the inspection failed on 7 imported objects against an
   expected 5. My deviation from the established convention, not a new engine defect — but the
   behaviour is worth knowing and is recorded as a ledger finding.
6. **The spire and apex drift is a presentation device**, labelled as such in the card and the manifest
   so it cannot later be cited as canon motion. The articulated-component requirement stays pending.
7. Open: textures, a detail judgement at gameplay distance, in-engine capture, gate reviews,
   incorporation of the provisional card into the authoritative requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
