# StarCraft II Spatial Metrics Reference

**Author and owner:** Angelis Pseftis\
**Status:** Research reference with an isolated editor calibration fixture. Recommendations remain non-normative; fixture verification is recorded below and does not establish gameplay parity or owner acceptance.\
**Evidence boundary:** SC2Mapster's extracted SC2 5.0.16.97364 data snapshot at commit `7be894538d6eac77a9583acfeb5f721e273beee4` (2026-06-22), accessed 2026-09-05. The extraction is a practical inspection source, not a Blizzard-published release manifest. Re-extract the installed SC2 build before claiming exact parity with a particular live patch.

## Scope and conversion convention

This reference supports a StarCraft II-like spatial feel during Unreal blockout. SC2's abstract map units do not establish a real-world metre scale.

For the Echoes blockout convention, use **1 SC2 map/grid unit = 200 Unreal units (UU) = 200 cm = 2 m**. Under that convention, a camera distance of 34 SC2 units maps directly to a `TargetArmLength` of 6,800 UU. A 100-UU-per-unit convention is a valid alternate comparison scale; it halves every converted distance and area scales by one quarter. Validate screen coverage, unit readability, pathing, and travel time in Echoes; a numerical conversion alone cannot establish gameplay equivalence.

The 200-UU recommendation is consistent with the current presentation defaults in [EchoesTerrainView.h](../Source/EchoesOfTheBrokenSun/Public/EchoesTerrainView.h) (`WorldUnitsPerTile = 200.0f`) and [EchoesPlayerController.cpp](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp) (`NetworkTileWorldSize = 200.0f`), inspected 2026-09-05. Those values do not establish that an Echoes simulation tile already implements all SC2 placement or movement rules. [Requirements.md](Requirements.md) remains normative, including `SPEC-SIM-002` and `SPEC-MOV-006..013`; this document does not change their thresholds.

Conversion rule: `length_UU = length_SC2 × 200`. Apply the same factor to XY distances, Z differences, radii, ranges, camera distances, and speed expressed per the same time unit. Angles and durations remain unchanged. Matching movement time also requires matching the simulation/game-speed time basis.

## 1. Base grid and structures

Use **1 × 1 SC2 world unit** as the building-placement/level-design square. A 3 × 3 footprint spans three coordinate units on each axis. Placement masks, movement masks, and unit positions are separate concepts: a moving unit does not occupy a whole building square or move only in grid-sized steps. Blizzard exposes separate placement and pathing grids in its [raw client protocol](https://github.com/Blizzard/s2client-proto/blob/master/s2clientprotocol/raw.proto).

| Object | SC2 placement footprint | Unreal footprint at 200 UU/unit |
|---|---:|---:|
| One build square | 1 × 1 | 200 × 200 cm |
| Supply Depot | 2 × 2 | 400 × 400 cm |
| Barracks | 3 × 3 | 600 × 600 cm |
| Command Center | 5 × 5 | 1,000 × 1,000 cm |
| Tech Lab / Reactor body | 2 × 2 | 400 × 400 cm |

The building dimensions come from `PlacementFootprint` and related footprint definitions in the pinned [Liberty UnitData](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/liberty.sc2mod/base.sc2data/GameData/UnitData.xml) and [FootprintData](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/liberty.sc2mod/base.sc2data/GameData/FootprintData.xml). Depot and Barracks pair full placement footprints with `Contour` movement footprints. Thus a placement rectangle is not necessarily the exact blocked shape around a building. The Command Center's `Footprint5x5DropOff` also has resource/drop-off-specific placement rules.

Reserve the add-on on its authored side when laying out production rows, and keep a separate spawn/rally exit. For a simple Unreal square-placement model, use grid-line boundaries: even-width building centres fall on whole coordinates and odd-width centres on half coordinates. The origin choice is arbitrary; consistently aligning occupied cell boundaries is what matters.

## 2. Cliffs and ramps

The inspected Core `CliffHeights` array advances by **2 SC2 world units per normal adjacent upper tier**. It explicitly lists indices 2–15 as 2, 4, 6, …, 28; expansion catalogs contain special lowest-floor overrides. Use relative heights for blockout rather than treating an absolute terrain datum or a void floor as low ground. [Pinned Core CliffMeshData](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/core.sc2mod/base.sc2data/GameData/CliffMeshData.xml).

| Authored plateau | Relative SC2 Z | Relative Unreal Z |
|---|---:|---:|
| Low ground, chosen datum | 0 | 0 cm |
| One tier above low ground | +2 | +400 cm |
| Two tiers above low ground | +4 | +800 cm |

“Watchtower level” is not a universal elevation category. If a watchtower is placed on a third plateau, +800 cm is a suitable two-step conversion; the watchtower itself does not establish that terrain height. Custom terrain and maps can override default heights. Blizzard added support for 15 cliff levels in [patch 5.0.3](https://news.blizzard.com/en-us/article/23492556/starcraft-ii-5-0-3-patch-notes), so three gameplay tiers are a design choice rather than the current editor's maximum.

No universal SC2 narrow-ramp or main-ramp width was established by the reviewed primary sources. Width is authored per map. The following are **Unreal design recommendations**, measured as clear traversable width perpendicular to the route at its narrowest point:

| Ramp purpose | Recommended clear width in grid units | Unreal width |
|---|---:|---:|
| Deliberately restrictive choke | 3–4 | 600–800 cm |
| Main-base ramp starting point | 6–8 | 1,200–1,600 cm |
| Broad natural/army entrance | 10–14 | 2,000–2,800 cm |

A 2-cell-wide passage is a special tight-case test, not the default guarantee for a 2-cell-diameter unit. Diagonal ramp brush width and the axis-aligned bounding box do not directly equal usable clearance. Measure after movement blockers and agent clearance are applied; decorative ramp lips must not silently narrow navigation.

For the 400-cm climb, start with 800–1,200 cm of horizontal ramp run (4–6 cells): this gives approximately 26.6°–18.4° slope. These run/slope values are recommendations, not extracted SC2 ramp standards. In Echoes, simulation passability, elevation/vision rules, and the visual slope must agree; a mesh ramp alone does not establish gameplay traversal.

## 3. Units and pathing clearance

These are resolved, unmodified `CUnit.Radius` values from the inspected multiplayer catalog chain, not mesh bounds. Radius is a useful blockout reference, but it is not the complete SC2 movement algorithm or a guarantee of passage through a particular footprint.

| Unit | Radius in SC2 units | Diameter in SC2 units | Unreal radius | Unreal diameter |
|---|---:|---:|---:|---:|
| Marine | 0.375 | 0.750 | 75 cm | 150 cm |
| Zergling | 0.375 | 0.750 | 75 cm | 150 cm |
| Marauder | 0.5625 | 1.125 | 112.5 cm | 225 cm |
| Roach | 0.500 | 1.000 | 100 cm | 200 cm |
| Thor | 1.000 | 2.000 | 200 cm | 400 cm |
| Ultralisk | 1.000 | 2.000 | 200 cm | 400 cm |

Provenance: Marine, Zergling, Marauder, and Ultralisk are explicit in [Liberty UnitData](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/liberty.sc2mod/base.sc2data/GameData/UnitData.xml); Roach inherits the 0.5 default in [Core UnitData](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/core.sc2mod/base.sc2data/GameData/UnitData.xml). Thor's older values are superseded by later multiplayer/expansion values of 1.0, including [Void UnitData](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/void.sc2mod/base.sc2data/GameData/UnitData.xml). Core → Liberty → LibertyMulti → Swarm → SwarmMulti → Void → VoidMulti → BalanceMulti was inspected for these fields. This guards against quoting a campaign/base value as the final multiplayer value.

SC2 also stores `SeparationRadius` and `InnerRadius` independently. The shipped [editor field descriptions](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/core.sc2mod/enus.sc2data/LocalizedData/Editor/EditorCatalogStrings.txt) describe `Radius` as interaction with other units, `InnerRadius` as interaction with buildings, and `SeparationRadius` as spacing for air units in a squadron. The land-collision hint identifies `InnerRadius` as determining occupied space. Blizzard's [5.0.13 patch notes](https://news.blizzard.com/en-us/article/24078322/starcraft-ii-5-0-13-patch-notes) likewise identify inner radius with terrain/structure collision. Do not treat the Radius table as a universal hard movement collider.

| Unit | Explicit/resolved catalog InnerRadius | Unreal equivalent |
|---|---:|---:|
| Marine | 0.375 | 75 cm |
| Zergling | Not explicitly assigned in inspected XML; fallback unresolved | Requires editor/runtime query |
| Marauder | 0.375 | 75 cm |
| Roach | 0.625 | 125 cm |
| Thor | 1.000 | 200 cm |
| Ultralisk | 0.750 | 150 cm |

For example, Marauder's Radius is 0.5625 while its InnerRadius is 0.375; Ultralisk's corresponding values are 1.0 and 0.75. Use the static footprint contour and applicable InnerRadius when reconstructing SC2 clearance. The 200-cm massive-unit radius below is a conservative simplified Unreal choice that covers the Thor and exceeds the Ultralisk's explicit static radius. Mesh silhouette, selection bounds, range calculations, placement checks, and movement clearance should have explicit responsibilities.

The Ultralisk is version-sensitive: [5.0.11](https://news.blizzard.com/en-us/article/23893118/starcraft-ii-5-0-11-patch-notes) reduced its size by 12.5%; [5.0.14](https://news.blizzard.com/en-us/article/24162754/starcraft-ii-5-0-14-patch-notes) restored model size and added allied push priority. The table reports the inspected 5.0.16 catalog, rather than projecting a past percentage change onto an assumed current radius.

For a simplified Unreal ground-agent model with massive-unit radius `r = 200 cm`, let `m` be clearance reserved on each side. The geometric straight-corridor rule is `clear width >= 2r + 2m`. With `m = 25–50 cm`, the target is **450–500 cm** (2.25–2.5 cells). Reserve **3 clear cells / 600 cm** between blocked structure edges as a practical single-file blockout baseline. For two opposing massive units, start at **5–6 cells / 1,000–1,200 cm** and verify opposing flow.

A two-cell gap is exactly 400 cm and leaves no tolerance for a hard 200-cm-radius collider. It may work in particular SC2 building contours or navigation rules, but is not a portable guarantee. Measure edge-to-edge between actual blockers, not between building pivots or placement decals. Corner entry, turning geometry, spawn exits, mesh overhangs, and mixed-unit traffic still require traversal checks. If using Recast or another radius-eroded navigation representation, configure clearance consistently and do not apply the same radius twice. Echoes' deterministic simulation remains authoritative under `SPEC-SIM-002` and `SPEC-MOV-013`.

## 4. Camera findings

### Extracted default tactical camera

The current extracted Core camera catalog's default `CCamera` defines the following initial parameters:

| Parameter | Extracted value | Blockout interpretation at the proposed scale |
|---|---:|---:|
| Distance | 34.0 SC2 units | 6,800 UU / 68.0 m arm length |
| Pitch | 56° | 56° downward angle from the horizontal, pending in-editor visual confirmation of the engine convention |
| Yaw | 180° | Orientation only; do not use it as a world-design dimension |
| Field of view | 27.8° | Axis is not declared by this catalog entry; see FOV treatment below |
| Near clip | 0.1 SC2 units | 20 UU / 20 cm if direct-scaled; choose production clipping from actual terrain/selection requirements |
| Far clip | 600 SC2 units | 120,000 UU / 1,200 m if direct-scaled; rendering/culling policy is an Unreal decision |

The catalog supplies five player zoom stops: `(distance, pitch)` = `(34, 56°)`, `(30, 52°)`, `(26, 48°)`, `(22, 44°)`, and `(18, 40°)`. Observer zoom adds `(44, 56°)` and `(54, 56°)`. These are source values, not measured screen-space coverage.

If SC2's pitch is treated as the conventional down-from-horizontal angle, the default eye position relative to a ground focus point is approximately **5,637 UU above** and **3,803 UU horizontally behind** it: `height = 6800 × sin(56°)` and `horizontal offset = 6800 × cos(56°)`. This calculation is a UE placement aid, not a separate SC2 datum. Camera-to-target distance is not vertical camera height; add any focus-point height offset separately.

### FOV and aspect-ratio treatment

`CameraData.xml` gives `FieldOfView = 27.799999` but does **not** state whether that value is vertical or horizontal. Blizzard's Cutscene Editor instruction PDF documents a `Vertical FOV` option, which demonstrates that SC2 tooling can express FOV on either axis; it does not prove which axis the default gameplay camera uses. Therefore, do not record “SC2 uses 27.8° vertical FOV” as a verified fact from these sources alone.

For a 16:9 blockout, use this deliberately qualified starting point:

| UE setting | Starting value | Rationale and adjustment rule |
|---|---:|---|
| Camera FOV | **47.5° horizontal** | This is the 16:9 horizontal equivalent of a *hypothetical* 27.8° vertical FOV: `2 × atan(tan(27.8° / 2) × 16 / 9) = 47.4947°`. It is a composition hypothesis, not a confirmed SC2 axis value. |
| Aspect ratio | 16:9 for comparison captures | Lock it only for reference captures; allow the shipped game to support its intended display policy. |
| Alternate test | 27.8° horizontal | Compare against the first setup at the same arm length. Retain screenshots and select the framing that meets Echoes readability and map-coverage requirements. |

Unreal's camera documentation describes the Camera Component as the place to set field of view and aspect ratio. Its FOV is documented as horizontal. That means a chosen SC2 vertical-FOV hypothesis must be converted per target aspect ratio before assigning it to `UCameraComponent::FieldOfView`.

### Unreal SpringArm starting configuration

Create a camera pivot at the intended ground-focus point, attach a `USpringArmComponent`, and attach the `UCameraComponent` to `USpringArmComponent::SocketName`.

| Component/property | Recommended blockout start | Evidence boundary |
|---|---|---|
| `TargetArmLength` | 6,800 UU | Direct conversion of extracted default SC2 distance (34.0) under 1 unit = 200 UU. Epic defines this as the arm's natural no-collision length. Reconcile the active Echoes zoom constraints before implementation; this is a proposed preset, not an applied runtime setting. |
| Spring-arm relative pitch | -56° | UE's standard boom arrangement places the camera at the arm's end; visually confirm that this produces the intended elevated, downward view in the current pawn orientation. |
| `UCameraComponent::FieldOfView` | 47.5° horizontal at 16:9, with 27.8° horizontal as the controlled comparison | UE uses horizontal FOV; SC2 default's axis remains unverified here. |
| `bDoCollisionTest` | Start disabled for a bounded RTS tactical camera, or enable only with camera-safe collision and test terrain-edge behavior | Epic documents collision retraction. Retraction changes perceived scale and must not silently alter tactical framing. |
| Camera lag / rotation lag | Disabled at first | Establish framing and command precision before introducing smoothing; add it only if its response is playtested. |
| Zoom | Use the five converted arm/pitch pairs: `(6800,-56°)`, `(6000,-52°)`, `(5200,-48°)`, `(4400,-44°)`, `(3600,-40°)` | Direct transformed source table; interpolate only after validating discrete stops. |

`USpringArmComponent` maintains the child at its target distance and can retract it on collision; `TargetArmLength` is the natural, unblocked length. Epic's C++ quick-start demonstrates the standard attachment pattern and a negative relative pitch on a SpringArm. These APIs explain the Unreal implementation; they do not authenticate the SC2 values.

### Verification needed before adopting the preset

1. Create a 16:9 controlled test scene with the chosen 2 m grid and fixed reference objects.
2. Capture default, minimum, maximum, and both candidate FOV settings with the same focus point.
3. Check ground coverage, selection readability, cliff occlusion, building silhouettes, and cursor-to-ground targeting at every stop.
4. Retain the screenshot/configuration evidence and record the chosen preset against the applicable Echoes requirements. A blockout comparison is local test evidence, not SC2 parity or owner acceptance.

## Camera sources

1. [SC2Mapster extracted Core `CameraData.xml`, SC2 5.0.16.97364 snapshot](https://raw.githubusercontent.com/SC2Mapster/SC2GameData/7be894538d6eac77a9583acfeb5f721e273beee4/mods/core.sc2mod/base.sc2data/GameData/CameraData.xml) — inspected default `CCamera` values and zoom table. This community-maintained extraction is the source for the numerical SC2 camera values above.
2. [Blizzard Cutscene Editor Instructions](https://us.media.blizzard.com/starcraft2/downloads/tutorial/SCII-CutsceneEditorInstructions.pdf) — documents the `Vertical FOV` tooling option, supporting the explicit FOV-axis caveat.
3. [Epic: `USpringArmComponent` API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USpringArmComponent) — arm length, collision probing, offsets, and lag semantics.
4. [Epic: Player-Controlled Cameras C++ Quick Start](https://dev.epicgames.com/documentation/en-us/unreal-engine/quick-start-guide-to-player-controlled-cameras-in-unreal-engine-cpp) — camera-to-SpringArm attachment and negative-pitch example.
5. [Epic: `UCameraComponent::FieldOfView` API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/Camera/UCameraComponent/FieldOfView?application_version=5.5) — `UCameraComponent::FieldOfView` is a horizontal perspective FOV and can be converted under its aspect-ratio axis constraint.

## Verification record

Research and conversion review performed 2026-09-05. The authoritative physical reference is this Markdown file, edited in place. Authorship is Angelis Pseftis. Numeric conversions, catalog inheritance, local relative links, and table structure were checked. Source inspection establishes the reported catalog values; recommended clearances, ramp widths, and the FOV mapping remain proposals for blockout validation. The subsequent Unreal fixture work is recorded separately below. No SC2 runtime measurement, gameplay movement validation, or owner acceptance is claimed.

## Editor calibration implementation — 2026-09-05

The owner's “Proceed” authorizes implementing the reference as an inspectable blockout aid. The dedicated
level is `/Game/Developers/AngelisPseftis/SpatialCalibration/SC2SpatialCalibration`. It contains original
primitive measuring stations, not a campaign location: 200-cm grid; 2×2, 3×3, 4×4 and 5×5 footprints;
separate unit-interaction and terrain/structure-radius discs; 400-cm-rise ramps with 3/6/10-cell widths;
2/3/5-cell clearance lanes; five reference zoom cameras; the alternate FOV hypothesis; and an explicitly
labelled overview camera. Block heights are arbitrary 200-cm measuring blocks, not claimed SC2 building
heights. The Zergling's unresolved inner radius is labelled rather than fabricated.

The current runtime `SimToWorld` and `WorldToSim` conversion uses
`UEchoesSimulationSubsystem::TileWorldSize = 200.0f`, matching the reference grid in presentation space.
Historical 100-cm simulation-tile prose must not be used to introduce a factor-of-two conversion into this
fixture. Fixed-point precision (`kFixedScale = 1024`) is a separate quantity. Echoes' 4×4 production
footprint remains distinct from the SC2 Barracks' 3×3 reference.

The authoritative fixture source is [sc2_spatial_calibration_v1.json](../Content/World/Source/Authoring/sc2_spatial_calibration_v1.json).
[generate_spatial_calibration.py](../Scripts/generate_spatial_calibration.py) creates the Developer map
and reads actual primitive bounds back from Unreal. Regeneration replaces only tagged fixture actors
inside that map. All fixture actors are editor-only and non-colliding; the generator introduces no
campaign runtime binding. The ordinary M01 camera, simulation, unit data, maps, and passability remain
outside this implementation's write scope. Reference CameraActor transforms are calculated from the
equivalent boom length; this fixture does not replace the gameplay SpringArm implementation.

Run the lightweight geometry checks from the checkout:

```sh
/usr/bin/python3 Tests/World/test_spatial_calibration_source.py
/usr/bin/python3 Scripts/generate_spatial_calibration.py --check
```

For generation, use a separate clean Unreal editor process with
`-ExecutePythonScript=<absolute path to Scripts/generate_spatial_calibration.py>` and `-nullrhi` after
acquiring the shared heavy-run reservation. Do not execute generation inside the retained M01 PIE
session. Set `ECHOES_SPATIAL_EVIDENCE` to an evidence directory to retain the engine readback receipt.
For screenshots, launch the same script in a separate rendering editor process with
`ECHOES_SPATIAL_CAPTURE=1` and without `-nullrhi`; it loads the saved calibration map, captures six named
views at 1920×1080, and exits. The capture directory must be new so earlier evidence is preserved.
Capture uses editor view because game view excludes the editor-only fixture. Require the readiness
marker, complete receipt and image files, and inspect the images: Unreal can exit with code zero after
a Python exception, so its process exit code alone is insufficient evidence.

**Current implementation state:** the Developer level and six measuring materials are generated.
Six Python geometry tests pass. Unreal 5.8.2 reopened the saved level and verified all 122 primitive
bounds, persisted `NoCollision` profiles, disabled overlaps, material authorship, and label text.
The saved level contains 155 tagged fixture actors, including nine cameras; all nine camera transforms,
FOV/aspect settings and constrained aspect ratios were checked. Map Author and Creator metadata both
identify Angelis Pseftis, runtime binding is `none`, and source/generator hashes match the saved package.

All six 1920×1080 captures were inspected. The
[overview](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/capture-v2/Overview_NotGameplayZoom.png)
shows the full layout; the
[clearance view](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/capture-v2/Reference_Clearance.png)
shows all three lane widths. The
[default reference](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/capture-v2/Reference_Zoom_0.png),
[near zoom](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/capture-v2/Reference_Zoom_4.png),
[alternate FOV](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/capture-v2/Reference_AlternateHorizontalFOV.png)
and [ramp view](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/capture-v2/Reference_Ramps.png)
are closer comparisons, so outer stations are cropped. The overview is the whole-fixture coverage check.
Editor icons/grid remain visible, and overview text is too small for detailed reading; inspect the
station cameras or actor properties for measurements. These are measuring-fixture captures, not final
game art, gameplay navigation tests, camera-usability validation, or owner acceptance.

Generation evidence is retained in
[the calibration evidence directory](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/).
The final successful run is `capture-v2.log`, with generation, saved-map and capture receipts in
`capture-v2/`; [verification-summary.json](../BuildArtifacts/Evidence/sc2-spatial-calibration-20260905T113646Z/verification-summary.json)
records source/asset/image hashes and the inspection outcome. Earlier API, world-reload and initial
capture failures remain in the same evidence directory. The first reload exposed an unpersisted
collision setting, corrected by assigning the `NoCollision` profile; the first capture used game view
and was black, corrected by retaining editor view. These failed runs are not qualification evidence.
The generated map is stored under `Content/Developers/angelispseftis/SpatialCalibration/` on disk;
Unreal normalizes the developer-folder casing. Enable Developer Content in the Content Browser to
find it, then open the calibration level and pilot a named camera under `SpatialCalibration/Cameras`.
Use `Overview_NotGameplayZoom` to locate the stations; use the reference cameras for perspective
comparisons. Open it in a separate editor session if M01 PIE is being retained for another review.
