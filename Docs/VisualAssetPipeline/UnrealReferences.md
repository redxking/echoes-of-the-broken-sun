---
title: Unreal production reference library
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-06
status: Technical reference; no production or integration acceptance
---

# Unreal production reference library

This is the technical source library for the concept-production work in this conversation. It contains **58 official Epic documentation references**, **8 installed-engine file references**, and mappings for **21 production packages**. The machine-readable index is [unreal-references.json](unreal-references.json).

The project association is **5.8**. The installed build reports **5.8.2, changelist 56702186**. Documentation versions below are the labels returned by Epic on 6 September 2026; they are not a claim that each feature was exercised in this installation. The Static Mesh Editor UI and two Niagara pages returned 5.7 labels; attempts to retrieve pinned 5.8 variants were unavailable. Recheck those controls against the installed editor before production.

Epic owns the linked documentation and engine sources. This library supplies original project-use annotations, not copied manuals or a rights grant. The game requirements and canon control what we build; these sources explain how Unreal tools work. Follow [the pipeline](README.md), [reference packages](reference-packages.json), and [the selected production contracts](motion/README.md). The six prepared amendments remain unapplied.

## Use before each production package

1. Retrieve the package mapping from the JSON index and read its concept, locator, component inventory and motion contract.
2. Establish centimeters, axis conversion, pivot, footprint and static-versus-skeletal route before modeling.
3. Follow geometry/UV and import references; record DCC/exporter version, source hash, importer, settings and reimport behavior.
4. Use skeleton/rig/sequence references where required. Preserve the selected anatomy, component counts, contacts and socket semantics.
5. Bind materials, audio and Niagara to authorized presentation events; exercise start, hold, cancel, interruption, recovery and restored-state behavior.
6. Compare against Art, Gameplay and Technical Gates at the prescribed RTS cameras and crowd density. Retain actual import, visual, audio and performance evidence.

CORE means a relevant reading prerequisite for that workflow, not a mandate to use every system. CONDITIONAL means evaluate only if the selected asset needs it. Static buildings do not automatically need a Skeleton or Control Rig. No plugins were enabled, samples downloaded, editor session started, or runtime assets changed during this research.

Critical boundaries: animation notifies do not award resources or apply damage; root motion does not own unit translation; cosmetic debris does not hold gameplay occupancy; Future Wells remain indestructible; Nanite remains OFF under current contracts. Retargeting never grants rights to source motion. Shader-complexity views aid diagnosis but do not prove frame-time budgets.

## Official documentation

### Foundations

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-001 — [Recommended Asset Naming Conventions](https://dev.epicgames.com/documentation/unreal-engine/recommended-asset-naming-conventions-in-unreal-engine-projects) | 5.8 / CORE | Use type prefixes with reserved EBS IDs; Epic examples do not rename our stable concept IDs. |
| EBS-UE-002 — [Coordinate System and Spaces](https://dev.epicgames.com/documentation/en-us/unreal-engine/coordinate-system-and-spaces-in-unreal-engine) | 5.8 / CORE | Check handedness, axes and export transforms against the planned +X forward, +Y right, +Z up contract. |
| EBS-UE-003 — [Units of Measurement](https://dev.epicgames.com/documentation/en-us/unreal-engine/units-of-measurement-in-unreal-engine) | 5.8 / CORE | Bind centimeters and import scale to the authoritative gameplay footprint, not artwork labels. |
| EBS-UE-004 — [Unreal Engine 5.8 Release Notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes) | 5.8 / CORE | Recheck import, rendering and animation changes before choosing tools for the installed 5.8.2 build. |

### Modeling and import

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-005 — [Modeling Mode Overview](https://dev.epicgames.com/documentation/unreal-engine/modeling-mode-in-unreal-engine?lang=en-US) | 5.8 / CORE | Blockout, topology, material assignments and baking. The page marks Modeling Mode Beta; validate the chosen workflow. |
| EBS-UE-006 — [UV Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/uv-editor-in-unreal-engine) | 5.8 / CORE | Unwrap and inspect UV layout for ceramic panels, mineral surfaces and Choir panes before texturing. |
| EBS-UE-007 — [FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine?lang=en-US) | 5.8 / CORE | Export pivots, triangulation, UVs, collision and LODs. Epic specifies FBX 2020.2; retain exporter settings and test the actual importer. |
| EBS-UE-008 — [Importing Assets Using Interchange](https://dev.epicgames.com/documentation/unreal-engine/importing-assets-using-interchange-in-unreal-engine?lang=en-US) | 5.8 / CORE | Capture the selected import pipeline and reimport options. Do not assume the runtime Blueprint example supports skeletal meshes or animations. |
| EBS-UE-009 — [Static Mesh Editor UI](https://dev.epicgames.com/documentation/unreal-engine/static-mesh-editor-ui-in-unreal-engine?application_version=5.7) | 5.7 / CORE | Inspect bounds, UVs, materials, collision and LOD previews; verify UI differences in 5.8.2. |
| EBS-UE-010 — [Geometry Scripting User Guide](https://dev.epicgames.com/documentation/unreal-engine/geometry-scripting-users-guide-in-unreal-engine) | 5.8 / CONDITIONAL | Evaluate reproducible mesh construction utilities for panel counts and roots; preserve source-first generation and do not execute import/mutation examples during audit. |

### Collision and attachments

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-011 — [Setting Up Collisions With Static Meshes](https://dev.epicgames.com/documentation/unreal-engine/setting-up-collisions-with-static-meshes-in-unreal-engine) | 5.8 / CORE | Separate simple gameplay collision from cosmetic geometry; verify footprints and selection traces at integration. |
| EBS-UE-012 — [Using Sockets With Static Meshes](https://dev.epicgames.com/documentation/unreal-engine/using-sockets-with-static-meshes-in-unreal-engine?lang=en-US) | 5.8 / CORE | Author building attachment transforms for production exits, power connections, effects and moving subassemblies. |
| EBS-UE-013 — [Navigation Components](https://dev.epicgames.com/documentation/en-us/unreal-engine/navigation-components-in-unreal-engine) | 5.8 / CONDITIONAL | Inspect possible nav effects of imported components; Unreal navigation is not a replacement for EchoesSimCore movement authority. |

### Materials and textures

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-014 — [Physically Based Materials](https://dev.epicgames.com/documentation/unreal-engine/physically-based-materials-in-unreal-engine?lang=en-US) | 5.8 / CORE | Define base color, metallic and roughness intent for Meridian ceramic/metal, Kharuun mineral-organic surfaces and Choir forms. |
| EBS-UE-015 — [Creating and Using Material Instances](https://dev.epicgames.com/documentation/unreal-engine/creating-and-using-material-instances-in-unreal-engine?lang=en-US) | 5.8 / CORE | Expose faction, team, damage and state parameters while retaining shared material parents and bounded permutations. |
| EBS-UE-016 — [Texture Asset Editor](https://dev.epicgames.com/documentation/unreal-engine/texture-asset-editor-in-unreal-engine?lang=en-US) | 5.8 / CORE | Verify channels, color-space choices, compression, mip behavior and texture dimensions in the actual target platform. |
| EBS-UE-017 — [Texture Streaming Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-overview-for-unreal-engine) | 5.8 / CORE | Plan mip residency across RTS camera distances and dense armies; a high-resolution source is not a runtime budget. |
| EBS-UE-018 — [Texture Streaming Metrics](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-metrics-in-unreal-engine) | 5.8 / CORE | Retain streaming pool and wanted-mip measurements from representative gameplay scenes. |
| EBS-UE-019 — [Using Transparency in Materials](https://dev.epicgames.com/documentation/unreal-engine/using-transparency-in-unreal-engine-materials) | 5.8 / CORE | Review overlapping Bulwark panes and Choir effects for overdraw and state visibility; do not assume translucency is required. |
| EBS-UE-020 — [Decal Materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/decal-materials-in-unreal-engine) | 5.8 / CONDITIONAL | Evaluate selection, footprint, damage and spent-Well surface marks; confirm receiving material, fade and platform behavior. |

### VFX and destruction

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-021 — [Creating Visual Effects in Niagara](https://dev.epicgames.com/documentation/unreal-engine/creating-visual-effects-in-niagara-for-unreal-engine) | 5.8 / CORE | Entry point for construction, weapons, sensing, Well telegraphs and state effects driven by authorized simulation events. |
| EBS-UE-022 — [Niagara Overview](https://dev.epicgames.com/documentation/unreal-engine/overview-of-niagara-effects-for-unreal-engine?lang=en-US) | 5.7 / CORE | Understand systems, emitters, modules and parameters; confirm version-specific controls in 5.8.2. |
| EBS-UE-023 — [Scalability and Best Practices for Niagara](https://dev.epicgames.com/documentation/unreal-engine/scalability-and-best-practices-for-niagara?lang=en-US) | 5.7 / CORE | Use effect types, culling and instance budgets for many simultaneous RTS effects; important state communication must survive lower quality tiers. |
| EBS-UE-024 — [Measuring Performance in Niagara](https://dev.epicgames.com/documentation/unreal-engine/measuring-performance-in-niagara?lang=en-US) | 5.8 / CORE | Measure simulation and rendering overhead plus overlapping-particle overdraw in crowded combat. |
| EBS-UE-025 — [Destruction Overview](https://dev.epicgames.com/documentation/unreal-engine/destruction-overview?lang=en-US) | 5.8 / CONDITIONAL | Evaluate cosmetic fragments/caches only where justified. Clear gameplay occupancy immediately; Future Wells are indestructible and Harvest is a protocol transition. |

### Audio interfaces

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-026 — [Audio Engine Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/audio-engine-overview-in-unreal-engine) | 5.8 / CORE | Map mechanical, mineral and Choir sound intent to the existing audio pipeline; visual timing alone does not establish audio acceptance. |
| EBS-UE-027 — [Sound Attenuation](https://dev.epicgames.com/documentation/en-us/unreal-engine/sound-attenuation-in-unreal-engine) | 5.8 / CORE | Check listener placement and distance behavior at RTS camera heights for sockets and state loops. |
| EBS-UE-028 — [MetaSounds Quick Start](https://dev.epicgames.com/documentation/en-us/unreal-engine/metasounds-quick-start) | 5.8 / CONDITIONAL | Evaluate parameter-driven machinery and resonance loops without replacing existing audio authority. The retrieved page carries a Beta notice. |

### Performance and acceptance

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-029 — [Static Mesh Automatic LOD Generation](https://dev.epicgames.com/documentation/unreal-engine/static-mesh-automatic-lod-generation-in-unreal-engine?lang=en-US) | 5.8 / CORE | Build reductions against per-asset triangle caps, preserving silhouettes, panes, emitters and functional detail. |
| EBS-UE-030 — [Nanite Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-technical-details) | 5.8 / CONDITIONAL | Capability reference only: current production contracts keep Nanite OFF; adoption requires a separate measured platform decision. |
| EBS-UE-031 — [macOS Development Requirements](https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine?lang=en-US) | 5.8 / CORE | Check platform feature constraints before rendering choices. Epic lists Nanite/VSM M2+ beta and no hardware-ray-traced Lumen support on this page. |
| EBS-UE-032 — [Instanced Static Mesh Component](https://dev.epicgames.com/documentation/unreal-engine/instanced-static-mesh-component-in-unreal-engine?lang=en-US) | 5.8 / CONDITIONAL | Evaluate repeated props and environment modules; grouping must preserve required selection, state and visibility behavior. |
| EBS-UE-033 — [Viewport Modes](https://dev.epicgames.com/documentation/unreal-engine/viewport-modes-in-unreal-engine?lang=en-US) | 5.8 / CORE | Inspect wireframe, lighting and shader complexity at representative cameras; shader instruction visualization is not measured frame time. |
| EBS-UE-034 — [Introduction to Performance Profiling and Configuration](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine) | 5.8 / CORE | Identify CPU/GPU/memory bottlenecks before optimization; do not transfer Windows graphics-debugger examples to Mac without checking support. |
| EBS-UE-035 — [Unreal Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-in-unreal-engine) | 5.8 / CORE | Retain trace evidence with build/device/camera/crowd identity for later Technical Gate acceptance. |

### Environment

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-036 — [Landscape Technical Guide](https://dev.epicgames.com/documentation/unreal-engine/landscape-technical-guide-in-unreal-engine?lang=en-US) | 5.8 / CONDITIONAL | Reference for campaign terrain dimensions, components and collision; no authorization to reshape current maps. |
| EBS-UE-037 — [Importing and Exporting Landscape Heightmaps](https://dev.epicgames.com/documentation/unreal-engine/importing-and-exporting-landscape-heightmaps-in-unreal-engine) | 5.8 / CONDITIONAL | Preserve heightmap source, precision and world scale for later environment packages. |

### Audit and provenance

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-038 — [Scripting the Unreal Editor Using Python](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python) | 5.8 / CORE | Reference for later editor automation. The page marks the feature Experimental; isolate read-only audits from mutating asset scripts. |
| EBS-UE-039 — [Data Validation](https://dev.epicgames.com/documentation/unreal-engine/data-validation-in-unreal-engine) | 5.8 / CORE | Define asset naming, metadata and budget validators; built-in command-line validation runs C++ rules by default, so register other rule types explicitly. |
| EBS-UE-040 — [Reference Viewer](https://dev.epicgames.com/documentation/unreal-engine/reference-viewer-in-unreal-engine?lang=en-US) | 5.8 / CORE | Inspect asset dependency graphs at integration; engine dependencies supplement rather than replace concept provenance. |

### Animation and rigging

| ID / source | Version / use | Echoes application |
|---|---|---|
| EBS-UE-041 — [FBX Skeletal Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-skeletal-mesh-pipeline-in-unreal-engine) | 5.8 / CORE | Bind skin, animation, morph, UV and LOD exports; verify FBX 2020.2 and actual import settings. |
| EBS-UE-042 — [Importing Skeletal Meshes Using FBX](https://dev.epicgames.com/documentation/unreal-engine/importing-skeletal-meshes-using-fbx-in-unreal-engine?lang=en-US) | 5.8 / CORE | Follow the import and mesh-inspection workflow; only reuse a Skeleton after compatibility review. |
| EBS-UE-043 — [Skeletons](https://dev.epicgames.com/documentation/en-us/unreal-engine/skeletons-in-unreal-engine) | 5.8 / CORE | Preserve anatomy-specific hierarchies; do not force bipeds, quadrupeds and moving structures onto one skeleton. |
| EBS-UE-044 — [Skeletal Mesh Sockets](https://dev.epicgames.com/documentation/en-us/unreal-engine/skeletal-mesh-sockets-in-unreal-engine) | 5.8 / CORE | Map planned attachments to bones or mesh sockets and verify required sockets survive bone LOD reduction. |
| EBS-UE-045 — [Rigging with Control Rig](https://dev.epicgames.com/documentation/en-us/unreal-engine/rigging-with-control-rig-in-unreal-engine) | 5.8 / CONDITIONAL | Evaluate authoring controls for legs, recoil braces, roots and pane chains; no rig exists merely because a contract lists controls. |
| EBS-UE-046 — [IK Rig Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-in-unreal-engine?lang=en-US) | 5.8 / CONDITIONAL | Evaluate terrain contact and appendage placement per anatomy without altering authoritative movement. |
| EBS-UE-047 — [IK Rig Animation Retargeting](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine) | 5.8 / CONDITIONAL | Reuse compatible motion only after chain/pose mapping and contact/sliding review; preserve source animation rights. |
| EBS-UE-048 — [Animation Sequences](https://dev.epicgames.com/documentation/unreal-engine/animation-sequences-in-unreal-engine?lang=en-US) | 5.8 / CORE | Produce and verify each planned motion track, including start/hold/cancel/recovery and load restoration where applicable. |
| EBS-UE-049 — [Animation Blueprints](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprints-in-unreal-engine) | 5.8 / CORE | Consume deterministic simulation state for pose presentation; never create competing gameplay authority. |
| EBS-UE-050 — [Blend Spaces in Animation Blueprints](https://dev.epicgames.com/documentation/unreal-engine/blend-spaces-in-animation-blueprints-in-unreal-engine) | 5.8 / CONDITIONAL | Blend locomotion/turn poses from authorized movement where useful; not every asset needs a blend space. |
| EBS-UE-051 — [State Machines](https://dev.epicgames.com/documentation/en-us/unreal-engine/state-machines-in-unreal-engine) | 5.8 / CORE | Map visible construction, damage and operational transitions; authoritative simulation events still determine state. |
| EBS-UE-052 — [Animation Notifies](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine) | 5.8 / CORE | Synchronize cosmetic audio/VFX, never resource awards, damage resolution, Well protocols or authoritative orders. |
| EBS-UE-053 — [Root Motion](https://dev.epicgames.com/documentation/unreal-engine/root-motion-in-unreal-engine?lang=en-US) | 5.8 / CONDITIONAL | Keep authoritative RTS translation simulation-owned; assess root motion only for approved isolated visual/cinematic uses and thread cost. |
| EBS-UE-054 — [Physics Asset Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-asset-editor-in-unreal-engine) | 5.8 / CONDITIONAL | Evaluate bodies/constraints for cosmetic response; separate them from gameplay collision and navigation. |
| EBS-UE-055 — [Skeletal Mesh LODs](https://dev.epicgames.com/documentation/unreal-engine/skeletal-mesh-lods-in-unreal-engine?lang=en-US) | 5.8 / CORE | Reduce geometry and bones within contract budgets while preserving feet, silhouettes and required socket transforms. |
| EBS-UE-056 — [Animation Optimization](https://dev.epicgames.com/documentation/unreal-engine/animation-optimization-in-unreal-engine) | 5.8 / CORE | Profile crowded RTS animation cost and game-thread work before choosing optimizations. |
| EBS-UE-057 — [Animation Budget Allocator](https://dev.epicgames.com/documentation/unreal-engine/animation-budget-allocator-in-unreal-engine) | 5.8 / CONDITIONAL | Evaluate quality/tick throttling only after plugin and platform checks; essential state visibility must remain reliable. |
| EBS-UE-058 — [Skeletal Mesh Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/skeletal-mesh-editor-in-unreal-engine) | 5.8 / CORE | Inspect imported mesh, skeleton, skinning, materials, bounds and LOD presentation before integration. |

## Installed engine references

These are verified local file locators with retained hashes, not a completed API behavior review. They provide the exact installed declarations when web documentation and local behavior differ. Do not edit or redistribute engine sources through this library.

- EBS-UE-LOCAL-001: [Build.version](</Users/Shared/Epic Games/UE_5.8/Engine/Build/Build.version>) — installed Epic engine version evidence.
- EBS-UE-LOCAL-002: [EditorAssetSubsystem.h](</Users/Shared/Epic Games/UE_5.8/Engine/Source/Editor/UnrealEd/Public/Subsystems/EditorAssetSubsystem.h>) — installed Epic API declaration locator.
- EBS-UE-LOCAL-003: [AssetImportTask.h](</Users/Shared/Epic Games/UE_5.8/Engine/Source/Editor/UnrealEd/Public/AssetImportTask.h>) — installed Epic API declaration locator.
- EBS-UE-LOCAL-004: [AssetRegistryModule.h](</Users/Shared/Epic Games/UE_5.8/Engine/Source/Runtime/AssetRegistry/Public/AssetRegistry/AssetRegistryModule.h>) — installed Epic API declaration locator.
- EBS-UE-LOCAL-005: [MaterialInstanceDynamic.h](</Users/Shared/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Public/Materials/MaterialInstanceDynamic.h>) — installed Epic API declaration locator.
- EBS-UE-LOCAL-006: [SkeletalMesh.h](</Users/Shared/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/Engine/SkeletalMesh.h>) — installed Epic API declaration locator.
- EBS-UE-LOCAL-007: [StaticMesh.h](</Users/Shared/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/Engine/StaticMesh.h>) — installed Epic API declaration locator.
- EBS-UE-LOCAL-008: [AnimSequence.h](</Users/Shared/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/Animation/AnimSequence.h>) — installed Epic API declaration locator.

## Package coverage

All 21 current packages have explicit source-ID mappings in the JSON index. Foundations, import, collision, materials, VFX, audio, validation and performance sources cover every family. Mobile units additionally point to conditional IK, retargeting, blending and animation-budget guidance. Destruction guidance is excluded from the Future Well package mapping. Environment references cover later world/terrain work without authorizing changes to current maps.

## Retrieval and evidence limits

The documentation was verified through Epic search results with relevant extracted content and version labels. This is a researched reference library, not an exhaustive review of every linked manual or an engine qualification run. Recheck the relevant page, installed API and platform at the start of implementation. New production needs may require additional sources.

No callable interface was available to alter a native chat Sources list. This catalog is stored with the pipeline and linked in this conversation; it is the maintained technical reference entry point for future work here. The earlier preservation ZIP remains the historical decision-batch export and does not yet contain this new catalog.
