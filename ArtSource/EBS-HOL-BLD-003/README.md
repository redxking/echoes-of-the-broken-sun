---
title: EBS-HOL-BLD-003 Chorus Loom — production source
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
package: EBS-PKG-HC-CHORUS-LOOM
production_asset_id: EBS-HOL-BLD-003
production_maturity: BLOCKOUT
revision: ebs-hol-bld-003-concept-v1
canon_status: CANDIDATE (built to the chorus-loom candidate against canon SPEC-BLD-017.HC.CHORUS); not owner acceptance
status: Isolated production source; no Unreal integration authorization
---

# EBS-HOL-BLD-003 Chorus Loom — production source

Single authoritative source record for the Hollow Choir production centre. Bounded by the shared
[agent contract](../../AGENTS.md) and the [authority map](../../Docs/README.md); the visual target is
[concept-fidelity.md](concept-fidelity.md); the contract is the provisional card
`REL-BLD-017.HC.CHORUS.ASSET` in [../hollow-choir-asset-cards.md](../hollow-choir-asset-cards.md);
maturity is in [../production-ledger.json](../production-ledger.json).

## 1. Source contract

| Binding | Value |
|---|---|
| Package contract | `EBS-PKG-HC-CHORUS-LOOM` in [reference-packages.json](../../Docs/VisualAssetPipeline/reference-packages.json) |
| Selected candidate | `…/concept-discovery-20260906/chorus-loom-review/chorus-loom-candidate.png` |
| Canon | `SPEC-BLD-017.HC.CHORUS`: production centre, 680 HP, 550 cm sight, 170 construction ticks, 4×4 footprint; trains every Choir mobile combat unit and hosts research; charges 5 Dawn every 600 ticks, reduced to 4 inside a Phase Anchor field |
| Asset card | `REL-BLD-017.HC.CHORUS.ASSET` — **provisional**, authored after the trace, not in `Docs/Requirements.md` |
| Production ID | `EBS-HOL-BLD-003`, planned `SK_EBS_HOL_BLD_003` under `/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_003/` |

## 2. Geometry and rig (revision `ebs-hol-bld-003-concept-v1`)

Generator: [build_chorus_loom.py](build_chorus_loom.py). A low 760 × 620 × 26 cm platform carries two
slab posts 572 cm apart (34 × 128 × 300 cm) with magenta edge strips down their inner borders, a warp of
20 fine threads strung between them from 66 to 286 cm, and a 430 × 96 × 18 cm beam centred at 372 cm —
**hovering, touching nothing**, 37 cm clear of the post crowns and 77 cm clear of the top thread. Slots:
`MI_EBS_HOL_Vitrified`, `MI_EBS_HOL_ApronStone`, `MI_EBS_HOL_Magenta`. Rig: `root`, `post_l`, `post_r`,
`beam`. Sockets (5): `Target_Anchor_Center`, `Rally_Default`, `Weave_Center`, `Research_Beam`,
`Unit_Emergence`. Clips: `producing_idle`, `researching`, `restore`. Budgets: LOD0 292 and LOD1 232
against the card's 6,000 / 2,400; the worst state (researching, 444) is also inside it. Magenta is 7.1%
producing and 10.9% researching against the 12% ceiling.

**The state read is the asset's job, and it is geometry as well as material.** Producing lights 20
threads. Researching interleaves 19 further, finer threads between them and lights the beam's underside
line — 39 lit threads and 444 triangles against 20 and 292. Insolvent moves every thread and strip into
the vitrified slot, so nothing is lit. Brightness alone is a material parameter; relying on it would
have made two states identical in the mesh.

**The woven form is a light effect, not geometry.** The candidate draws a glowing polyhedral cage at
the centre of the warp. The mesh provides the `Weave_Center` socket and nothing else, and a test asserts
no `weave` or `lattice` component exists.

## 3. Review evidence (blockout stage)

Evidence root: `…/asset-production-20260906T221157Z/EBS-HOL-BLD-003/`. Renders: orthographic front and
top with a 180 cm reference figure, plus three-quarter, weave-detail and tactical passes, for each of
the four states and for LOD1, and one posed frame per looping clip. Comparison sheets under
`renders/concept-compare/`: the producing three-quarter beside the candidate, the three power states
front-on, and the same three at tactical framing — where producing reads as a fine striped band,
researching as a dense solid one, and insolvent as bare stone.

Checks: 19 structural tests in [test_chorus_loom_build.py](test_chorus_loom_build.py) — the contract
inventory, the beam hovering and touching nothing with no strut, brace or support component anywhere,
the woven form absent from geometry, the three lit states differing in triangle count as well as slot,
magenta under the ceiling in both lit states, **no other faction's language in any component or slot
name**, the destroyed state keeping its platform and dropping the beam, footprint and ground
containment, no clip frame passing through the ground, the beam hanging out of step with the posts and
the posts out of step with each other, sockets, frame alignment, the rig, the provisional card recorded
as traced-first, LOD1 keeping both posts, the warp and the beam, LOD1 carrying no collision, and
determinism — all passing.

Headless Unreal 5.8.2 import into the isolated sandbox: both meshes at 4 bones, 5 sockets, 3 material
slots and 3 clips, imported bounds 760 × 620 × 381 cm matching the source exactly, LOD1 at 504 vertices
against LOD0's 624, and no errors, warnings, pipeline property failures or fatals.

## 4. Reproduction

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline/ArtSource/EBS-HOL-BLD-003"
python3 build_chorus_loom.py --evidence-dir "<evidence root>/EBS-HOL-BLD-003" --skinned
python3 build_chorus_loom.py --evidence-dir "<evidence root>/EBS-HOL-BLD-003" --check
python3 test_chorus_loom_build.py
```

## 5. Decisions and open items

1. **The traced 0.616 height-to-width and the 80 px post width are isometric bounding measurements, not
   proportions.** The panel is drawn in three-quarter isometric, so the overall ratio mixes post height
   with the ground plan and each post's apparent width is its thickness plus its foreshortened depth.
   They are recorded as measurements; the build's 572 cm post span across a 760 cm platform and its
   34 × 128 cm slab section are stated as design choices read from the drawing.
2. **The researching state breached the magenta ceiling at 15.4%.** Doubling the warp at the base radius
   nearly doubled the emissive area, and the beam's full 72 cm-wide lit underside added the rest.
   Thinning the whole warp was the wrong fix — even at a 1.0 cm radius the state only reached 11.85%,
   with no headroom and thinner threads in every state. Instead the interleaved rows are finer than the
   base rows (0.9 cm against 1.6 cm), which is what the candidate's researching panel draws, and the
   beam's lit underside is a 14 cm line rather than a panel. The state now measures 10.9%.
3. **The beam sits higher than the candidate draws it.** In the candidate it is at roughly post-crown
   height, read as floating through the isometric depth cue. Orthographically that cue is gone, so the
   build lifts it 37 cm above the crowns to keep it unambiguously unsupported from the front. Recorded
   as a deviation forced by the loss of the drawing's projection.
4. **The platform does not fill the 4×4 footprint.** At 760 × 620 cm it leaves 20 cm on the long axis
   and 90 cm on each short side. The candidate draws a rectangular slab wider than deep; the card's
   "filling the footprint" is met on the long axis only. Flagged rather than forced.
5. **`Target_Anchor_Center` and `Weave_Center` coincide** at (0, 0, 176) — the mid-height centre is the
   right place for both the selection anchor and the weave VFX origin. Not a defect; noted so a later
   reader does not read it as a duplicate socket.
6. **The post and beam drift is a presentation device**, labelled as such in the card and the manifest
   so it cannot later be cited as canon motion. The articulated-component requirement stays pending.
7. **The import script takes its job through `EBS_IMPORT_JOB`, not an argument.** Passing the job path
   after the script name in `-ExecutePythonScript` crashed the editor with `KeyError: 'EBS_IMPORT_JOB'`
   under `-ScriptErrorsAreFatal`. My invocation error, not an engine defect.
8. Open: textures, a detail judgement at gameplay distance, in-engine capture, gate reviews,
   incorporation of the provisional card into the authoritative requirements, owner acceptance.

No OWNER-QUESTIONs are open for this package.
