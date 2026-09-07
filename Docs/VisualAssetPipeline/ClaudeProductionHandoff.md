---
title: Claude handoff — Echoes asset production
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: Owner handoff prompt; isolated production, no shared-game integration
---

# Echoes of the Broken Sun — Begin asset production

You are continuing an established concept-to-Unreal production pipeline for `redxking/echoes-of-the-broken-sun`. Begin creating actual models, textures, rigs, animations and effects from the retained concepts and selected production contracts. This is an execution task, not a request for another general plan or a new concept inventory.

Make routine implementation decisions yourself within the established contracts. Preserve the book's visual intent, the selected concepts, and the game's authoritative requirements. Do not reinterpret missing production evidence as unanswered design choices: all 53 remaining design directions were decided under my delegation. Actual production and acceptance evidence is still outstanding.

## Locate the work and protect current development

Workspace: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun`

Shared game checkout: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project`

Prepared pipeline checkout: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Worktrees/concept-production-pipeline`

Pipeline branch: `docs/concept-production-pipeline`. Relevant commits: `156a846` (53 decisions and contracts), `7468fba` (archive receipt), `f58fae0` (Unreal references). These are retrieval anchors; verify current HEAD and later changes before starting.

Read the workspace and checkout `AGENTS.md`, `Project/Docs/README.md`, and `Docs/AgentSkillRouting.md`. Inspect branches, worktrees, dirty/staged files, active tasks and editor/GPU ownership. Apply the relevant session-control, modeling/material/animation/VFX/provenance and heavy-run skills that actually exist in the repository.

Establish a separate production branch/worktree or an existing explicitly assigned production workspace. Keep one writer per checkout. Do not reset, clean, overwrite or commit another agent's work. Do not use the active game checkout as a scratch project.

This handoff authorizes isolated asset authoring and isolated Unreal import/preview where resources are available. It does not authorize modifications to the shared game's Content, Source, Config, maps, gameplay data, build/CI, or active authoritative documents. Do not merge or replace current assets without a separate coordinated integration task. If shared Editor/GPU ownership is unavailable, continue independent source preparation rather than interrupting another task.

## Read the existing production sources

The following paths are relative to the prepared pipeline checkout:

- `Docs/VisualAssetPipeline/README.md`: workflow, identity standards, reference-package specification and acceptance gates.
- `Docs/VisualAssetPipeline/concept-register.json`: stable concept objects and classifications.
- `Docs/VisualAssetPipeline/sources.json`: image-source provenance. This is not the Unreal documentation catalog.
- `Docs/VisualAssetPipeline/reference-packages.json`: candidate/reference packages and their evidence locations.
- `Docs/VisualAssetPipeline/review-selections.json`, `visual-use-assessment.json`, and `book-based-review.json`: retained selections, placeholder assessments and book-based reasoning.
- `Docs/VisualAssetPipeline/motion/README.md`: current production decisions; older sections are explicitly historical.
- `Docs/VisualAssetPipeline/motion/gap-decisions.json`: the full 53 resolutions, source hashes, book paragraph references, 21 `production_policy` contracts, and six `master_amendments` prepared but unapplied. Read the complete applicable records, not just summaries.
- `Docs/VisualAssetPipeline/motion/candidate-review.json`, `motion-packages.json`, `preparation-evidence.json`, `construction/`, and `storyboards/`: supporting references. Schematics and state boards are not completed rigs or clips.
- `Docs/VisualAssetPipeline/UnrealReferences.md` and `unreal-references.json`: 58 Epic documentation references, eight installed-engine file references, and mappings for all 21 packages.

The retained book is `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/book-source.docx`. Expected SHA-256: `994e7df5d37a6ef1532c65a7b742d71e81b4ea7752311f5e5acc2e04c817ef83`. Use the exact candidate paths and hashes in the package/decision records. If a path is missing, locate the retained artifact through `motion/handoff-receipt.json` and the archive manifest; never silently substitute another image or an LFS pointer.

Read the live shared game's `Docs/Requirements.md` for behavior and numbers, `Docs/RequirementsState.md` for lifecycle and owner acceptance, `Docs/Archive/DevelopmentBible.md` for creative canon, and applicable art/audio direction and provenance records. Record the exact versions read. The novel and Epic examples do not override game authority. The six selected amendments govern the planned direction but remain unapplied to current gameplay; preserve that distinction and prepare compatibility work for later integration.

## Preserve the selected designs

The 21 contracts cover:

- Meridian: Surveyor, Lancer, Bulwark Team, Relay Skiff, Anchor, Power Link, Array Foundry, Aegis Post.
- Kharuun: Tender, Riftstalker, Cairnback, Resonant, Memory Hearth, Waystone, Growth Basin, Listening Spine.
- Hollow Choir: Concordance, Interval Loom, Chorus Loom, Phase Anchor.
- World system: Future Well, one stateful family.

Use each contract's reserved production ID, component inventory, planned names/paths, material limits, required tracks and sockets. In particular:

- Surveyor and Lancer are bipeds. Surveyor has three rear canisters; anatomical right drill, left gripper/welder. Lancer carries the lance at the right hip with a rear-leg recoil brace.
- Bulwark has exactly six centered panes: three from the left, three from the right, seam between 03/04. Preserve both linked three-pane chains.
- Power Link is distinct from Meridian Anchor: four numbered panels, one collar assembly, two base couplings, four conduits. Aegis has three stationary supports and two emitters. Foundry has opposite-end intake/output on one longitudinal rail.
- Memory Hearth has seven crown spires, four worker hollows and a separate intake. Waystone has four carriage pads and eight root bundles. Riftstalker, Cairnback and Resonant retain four locomotor limbs. Resonant has twelve fins and no weapon or attack clip. Listening Spine has twelve nodules. Four Growth Basin niches grant no additional gameplay capacity.
- Concordance has twelve panes in six pairs. Interval Loom has two physical spans; conflicting shadows are cosmetic, not extra solid structures. Chorus Loom has two pylons, one research pane and a complete platform. Phase Anchor has one exactly registered spire and magenta ring, with no offset copies; its selected upkeep direction is 5/4/3 Dawn, floor 3.
- Future Wells remain indestructible. Dormant, amber Harvest, cyan Preserve and magenta Reshape share the same family. Successful Harvest folds the spire inward into a persistent spent bowl; it is not combat destruction. Use authoritative timings and protocol records.

These highlights do not replace the detailed ledger. Preserve rejected/placeholder imagery as history; do not use rejected turnarounds as modeling authority or regenerate approved concepts unnecessarily. Supplemental views must be labeled derived references. Distinguish delegated design selection from owner acceptance of a finished asset.

## Start producing, then expand

Start with **Power Link** as the rigid-geometry/material/state pilot. Next complete a **Surveyor** rig/animation pilot, then the **Future Well** state-family pilot. This sequence is a practical production order, not a change to gameplay priorities. Use lessons from those pilots to progress the remaining packages in dependency order.

For each package:

1. Confirm source identity, component counts, canon status, source rights, silhouette, footprint and intended states. Resolve scale from the authoritative gameplay envelope. If an exact value is still unknown, label any blockout estimate; do not fabricate an approved dimension.
2. Create editable source geometry in the sanctioned source location of the isolated production workspace. If none exists, establish and document an isolated `ArtSource/<production-ID>/` location. Retain native DCC source, export settings and reproducible generation steps. Create consistent front/side/rear/top and tactical-camera views from the same geometry.
3. Build and inspect the blockout before costly detail. Check openings, exits, contacts, moving-part clearance, component count and functional silhouette against the concept. Continue independent packages while a consequential review is pending.
4. Produce suitable topology, UVs, material masks and PBR textures; apply scale and orientation deliberately. Use centimeters, +X forward, +Y right, +Z up and the contract pivot. Apply the tighter relevant geometry cap; Meridian's general ceiling is 8,000/3,500 triangles for LOD0/LOD1, with tighter asset caps retained. Nanite remains OFF. Read texture/material budgets per asset; do not invent a universal replacement budget.
5. Rig only what requires deformation or articulated motion. Preserve anatomy and required sockets across LODs. Author the full role track inventory, including construction, work/production, locomotion, attack where authorized, power changes, sensing, adaptation, interruption, damage and restoration. Include Waystone root/migration transitions and the Kharuun 80-tick adaptation presentation where specified. Do not supply idle-only rigs as complete animation packages.
6. Author state-driven materials, Niagara effects and audio interface bindings. Use faction grammar and reduced-motion/non-color alternatives. Do not substitute lights and particle spectacle for readable geometry.
7. Import and preview in an isolated compatible Unreal project using recorded settings. The last verified local engine was `/Users/Shared/Epic Games/UE_5.8`, build 5.8.2; verify it live. Consult the mapped Epic references and flag 5.7 documentation compatibility checks. Do not upgrade the shared project or enable its plugins casually.
8. Record actual evidence, defects and next work. Commit small, explicit source changes with appropriate binary handling. Maintain one authoritative document per deliverable with author/creator metadata `Angelis Pseftis`, while preserving legitimate third-party attribution and rights.

If required authoring tools are unavailable, identify the exact missing capability and continue useful independent work. A rendered drawing, generated script, storyboard, or proposed file path is not an executed model, animation, or imported asset. Report only artifacts that actually exist and have been inspected.

## Integration and acceptance boundaries

`EchoesSimCore` owns movement, combat, resources, construction, state transitions, navigation decisions and save/replay authority. Animation Blueprints, root motion, notifies, materials, Niagara and cosmetic physics must not create competing gameplay logic. Notifies may synchronize presentation; they must not apply damage or award resources. Cosmetic cables, shadows and debris cannot block input or navigation. On destruction, gameplay clearance is immediate; cosmetic debris may remain for 200 simulation ticks. The Future Well exception remains indestructible.

Evaluate each asset through three independent gates:

- **Art:** concept fidelity, exact components, proportions, material language, faction/world consistency and coherent motion.
- **Gameplay:** representative RTS camera and crowded scenes; function, faction, team, weapon, construction/damage/Well-state recognition; selection, footprint, occlusion and navigation. Use the actual package camera/recognition targets, not cinematic close-ups alone.
- **Technical:** import/reimport, names, paths, units, pivot, skeleton/sockets, collision, LODs, texture/material budgets, animation/event behavior, dependencies, provenance and target-device performance. Retain measured results, not inferred performance from polygon counts.

Track canon and maturity independently. Use the established maturity sequence: NOT_STARTED → REFERENCE_READY → BLOCKOUT → GAMEPLAY_PROXY → ART_ALPHA → ART_BETA → PRODUCTION → UE_INTEGRATED → VERIFIED. Advance only when evidence supports the stage; isolated preview is not integration into the game. Only Angelis records owner acceptance.

At each delivery report production IDs, source and export paths, reference bindings, component/triangle/material/texture/rig/clip counts where measured, images or playback captures, checks actually performed, failures, remaining integration dependencies and commits. Update isolated production records without disturbing shared authority files. Keep original evidence and selection history.

Begin now: verify the workspace and sources, establish isolated ownership, load the Power Link contract and actual candidate, and create its first source-faithful editable blockout with review views. Continue authorized production work; do not stop after presenting a plan.
