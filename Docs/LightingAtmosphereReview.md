---
title: M01 lighting and atmosphere review
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-05
updated: 2026-09-05
status: In progress; camera comparison complete, sun correction awaiting compilation and fresh load
---

# M01 lighting and atmosphere review

This is the authoritative record for the Phase 5 lighting pass authorized on
2026-09-05. It follows [Requirements.md](Requirements.md),
[RequirementsState.md](RequirementsState.md), [ArtDirection.md](ArtDirection.md),
and the existing M01 story/place brief. It does not accept a requirement or certify
a complete map. Earlier conversational setting suggestions were proposals, not
observed implementation.

The gameplay purpose is to keep unit bodies, lower limbs, ramp mouths, and route
edges distinguishable at the normal 48-degree RTS camera, including the supported
1400–6200 cm arm range. M01 is an evacuation through the civic/Glass Scar setting:
charcoal terrain, pale ceramic equipment and infrastructure, warm Crownfall key,
cool ambient fill, and readable cyan ownership and activity cues. Lighting should
make the existing authored forms easier to read. This pass creates no new lore,
geometry, animation, audio, simulation state, or visibility information.

Applicable contracts are `REL-ART-019`, `SPEC-VISD-006`, `SPEC-VISD-008`,
`SPEC-ART-004`, `SPEC-BUD-001..003`, and the fog/visibility presentation boundaries.
The target remains 1920×1080 Medium on M1 Pro, p95 frame time ≤16.67 ms,
render plus GPU ≤11.0 ms, and fog ≤1.5 ms. A short editor comparison cannot qualify
those budgets or substitute for packaged and owner evidence.

## Inspected starting state

Source base: `fc05cdf08191649363fb774ec88ad19d96c37a37`, with substantial inherited
dirty work. The phase evidence root contains before snapshots, exact scoped
patches, hashes, and the checkout status:

`/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/BuildArtifacts/Evidence/phase5-lighting-20260905T113659Z/`

Read-only editor inventory at 2026-09-05T11:38Z found the paused
`/Engine/Maps/UEDPIE_0_Entry.Entry` instance in editor process 21554. Its loaded
module is separate from the candidate source patch. The inventory reports raw
camera properties and override flags; unoverridden property values alone do not
prove a particular postprocess pass is active.

| Item | Observed state |
|---|---|
| Camera | Pitch −48°, yaw −45°, FOV 55°, arm 3800 cm |
| Exposure | Histogram, min 1.0 / max 1.2, bias 0; extended EV100 range off |
| Postprocess | Bloom override 0.3; AO properties 0.5 / radius 200 / view space and vignette 0.4 without camera overrides |
| Sun | One stationary directional light, intensity 10, shadows enabled; live rotation differs from source and is recorded in the inventory |
| Shadow setup | Stationary cascade distance 0, movable distance 40000 cm, four configured cascades; live shadow scalability Epic |
| Sky fill | One movable captured-scene Sky Light, intensity 8, emissive-only capture, 30000 cm threshold, real-time capture off |
| Ambient bounce | Fixed lower hemisphere RGB (0.035, 0.045, 0.065) |
| Fog | Existing height fog, density 0.008 and height falloff 0.15; source disables volumetric fog |
| Local lights | Five bounded point lights; no spot lights; all five disable ordinary and volumetric shadows |
| GI/reflections | Dynamic GI and reflection methods both 0 in current project/live console settings |
| Additional actors | No Sphere/Box Reflection Captures, Cull Distance Volumes, Sky Atmosphere, or Volumetric Cloud actors in this runtime instance |

M01 unit views use authored materials, faction accents, silhouettes, selection,
and ownership cues without per-unit light components. Visible-view construction
and release follow the scoped player observation. The reported V044 concern is
dark Surveyor lower legs and soles merging with ground shadow; its prior motion
receipt is corroborating historical evidence, not a result of this pass.

These units are articulated static-mesh assemblies. `EchoesEntityView.cpp` creates
the static body; `EchoesM01SurveyorRig.cpp` creates six static leg parts, and
`EchoesM01BulwarkParts.cpp` creates two static wings. Their motion uses component
transforms. The inspected implementation has no skeletal-mesh component or
physics asset for capsule shadows, so capsule shadows are not a setting-only
option for these units.

## Implemented treatment and rendered comparison

`EchoesRTSCameraPawn.cpp::ApplyAuthoredPostProcess` holds histogram exposure at
1.0/1.0, sets AO to 0.3 with an 80 cm world-space radius, and explicitly zeros
motion blur, lens flare, chromatic aberration, film grain, and vignette. Existing
film response and bloom 0.3 remain. This is `exposure-authored-v2`.

The actual sun did not match the intended source angle. Unreal 5.8's
`ADirectionalLight` constructor gives its root a −46° rotation
(`Engine/Private/Light.cpp:245`). Ordinary `SpawnActor` composed that with the
requested −55° pitch, producing the observed equivalent orientation
(−79°, 145°, 180°). Registered stationary components also reject ordinary runtime
movement; a later `SetActorRotation` is not a reliable initialization fix.

The reviewed source correction uses `SpawnActorAbsolute`, which excludes the CDO
root transform, choosing (−55°, −35°, 0°) for normal presentation and the existing
(−24°, −137°, 0°) intent for the vertical-slice fixture before registration. The
light retains its constructor-set stationary mobility. The later rotation attempt
was removed. `[ECHOES_SUN_AUTHORED]` logs requested and actual angles and mobility.
The convenience overload sets the light's editor icon/root scale to 1 rather than
the constructor's 2.5; it does not scale directional illumination. This is
`sun-angle-v1` and still awaits the next compilation and fresh-load check.

An earlier movable-sun source candidate was withdrawn because its 40000 cm/four-
cascade default introduced an unmeasured workload. That trial is distinct from
the final stationary absolute-spawn correction. Terrain and cliff components
continue to disable shadow casting under the fair-fog presentation contract;
changing sun orientation does not create cast shadows from those meshes.

### Camera comparison

The corrected comparison completed at 12:35:08Z in scoped editor PID 29449. Eight
1920×1080 PNGs passed file-completion, dimensions, nonblack-content, and hash checks.
All six ordinary-zoom captures had the same actual camera location. Wide and close
camera travel matched the requested arm changes. The prior camera settings were
reconstructed in this fresh world; this was not a replay of the earlier scene.
The original settings and override flags, fog, arm length, and sun mobility were
restored with matching readbacks.

| Profile | Mean luma, 0–255 | Fully white clipped pixels |
|---|---:|---:|
| Reconstructed previous camera | 60.07 | 0 |
| Fixed 1.0 camera candidate | 66.00 | 0 |
| Fixed 1.2 alternative | 59.56 | 0 |
| Small palette-grade trial | 66.04 | 0 |
| Volumetric-fog trial | 65.93 | 0 |
| Candidate in grayscale | 66.25 | 0 |
| Candidate, 6200 cm arm | 66.53 | 0 |
| Candidate, 1400 cm arm | 68.95 | 0 |

All means fall within the narrower ArtDirection capture guide of 50–70. This
measurement includes the HUD and is not the master SDR legal-range gate or a
whole-map exposure qualification. Shadow, postprocess, and effects scalability
were Epic (3), not the required Medium performance profile. No frame-cost result
is inferred from these stills.

The view had drifted before this run because simulation freeze does not stop
camera edge panning. It stayed stationary during the six ordinary comparisons,
but placed units near the right edge and outside the close view. These captures
therefore support the camera/exposure comparison, not a centered lower-limb or
whole-map readability claim. The grade and volumetric trial showed no compelling
benefit in this view; both were restored and remain unadopted.

### Centered sun-angle comparison

A second bounded trial completed five 1920×1080 captures at 12:48:13Z. It centered
on visible Surveyor `EchoesEntityView_3` at (−4800, −3800), disabled camera-pawn
actor ticking during the fixture, and checked the actual center and camera-to-
center distance. Scenario simulation remained frozen. Each captured light used
stationary mobility and intensity 10.

| Sun profile | Arm | Mean luma | Fully white clipped pixels |
|---|---:|---:|---:|
| Original composed orientation | 3800 cm | 66.46 | 0 |
| Explicit −55° / −35° | 3800 cm | 59.58 | 0 |
| Explicit −65° / −35° alternative | 3800 cm | 62.55 | 0 |
| Explicit −55° / −35°, close | 1400 cm | 55.45 | 0 |
| Explicit −55° / −35°, grayscale | 3800 cm | 59.78 | 0 |

Visual judgment: the −55° treatment lights camera-facing leg panels and foot
edges, separates them from the cast shadow, and preserves the pale/cyan equipment
against darker terrain. The close and grayscale views retain those form cues.
The −65° alternative also helps; the preference for −55° is modest and follows
the original authored angle with slightly stronger lower-body definition. This
does not brighten the whole unit: upper masts, torsos, and parts of the weapons
are darker than the reference and somewhat darker than at −65°. The close view
shows longer, dense turret shadows; it has no matching close reference or −65°
comparison. An independent visual review found no blocking regression at the
tested standard zoom. Moving gait states, all facings, crowded battles, path
overlap, hostile team colors, and alternate accessibility modes remain untested.

The trial temporarily made the sun movable only during synchronous rotation
edits, then restored stationary mobility before each three-second settle and
capture. This causes transient component re-registration and is not a performance
measurement. The source fix uses absolute spawning and needs no such transition.

The original sun, camera position, zoom, actor tick state, saturation, and override
flag were restored. The first checker flagged roll +180° versus −180°; a separate
readback established equivalent orientation (wrapped error below 0.00000003°) and
exact matches for the other fields. The original mismatch receipt is retained
alongside `unit-sun-angle/restoration-verification.json`.

### Evidence and build association

The first seven timed screenshots were black because an automation helper targeted
an empty editor viewport. They remain failed evidence. Its restore ended 39 seconds
after that reservation; the coordinator was informed. The corrected scripts use
PIE-routed `HighResShot`, completed PNGs, per-frame settings, hashes, actual camera
positions, bounded deadlines, and restoration readbacks. The corrected runs both
returned control within their reservations.

Camera source SHA-256:
`da7b63bbc410e86151070946261531293e3c5921cedf5fb7cb1e0c89296bbacf`.
Sun-corrected GameMode SHA-256:
`bbb954d298af771406cb8909c41a0842d086b00565b6c18809a371308a5611a3`.
Scoped changes are retained in `final-camera.patch` and `final-sun-angle.patch`.

The controlled Mac Development Editor build passed for the camera patch; the
latest pre-sun-fix normal game module hash was
`73393ed641f7128192d3e1a715cf5fd758fa55a5f800417194c2e8cd1a6775f4`.
PID 29449 actually loaded suffix module 4651, hash
`677366a27def88e8d3bc886dde6df7736fd808934bd70d2c51742339be49805b`,
with normal SimCore. Its retained build log records a successful link/metadata-only
build. This distinction is explicit in
`BuildArtifacts/Evidence/m01-terrain-environment-20260905T115824Z/loaded-identity.json`.
These renders are that loaded module plus recorded runtime overrides, not proof
that the normal build-3 binary or the subsequent sun source fix was loaded.

The shared automation run reported 81 of 90 passing, with nine mission/schema/
skirmish failures being repaired by its owner. This is not a full regression pass.
Sun-fix compilation, fresh-load readback, and the joint regression rerun are pending.

## Remaining roadmap decisions

| Area | Current decision and evidence needed |
|---|---|
| Unit lighting | Centered −55° sun and restrained camera treatment improve visible front leg/foot separation. Preserve the existing component assemblies; no per-unit light or capsule proxy was added. Moving and crowded views remain to be qualified. |
| Lighting cost | Existing local lights are bounded and unshadowed. Final sun mobility remains stationary; no extra lights were added. No frame-cost improvement or budget compliance is claimed. |
| Visual culling | Preserve visibility-driven instancing. A generic distance volume can hide tactically visible units or required landmarks; no blanket culling change is justified by this audit. |
| Reflections | Retain the public sky capture. Local reflection probes require a specific visible material defect and a capture basis that cannot expose hidden state. No such defect has been established here. |
| Sky/clouds | Retain the authored fractured-sun sky mesh and captured ambient rig. Earth-like atmosphere/cloud actors are not prerequisites for this world's existing sky treatment. |
| Color grading | Retain the authored palette; the small grade trial did not establish a useful improvement. No LUT imported. |
| Height/volumetric fog | Retain existing height fog (0.008 density / 0.15 falloff) with volumetrics disabled. The trial did not establish a benefit that justifies adoption; canyon-specific appearance and cost remain unmeasured. |
| Baking | Runtime-generated movable/procedural terrain has no applicable Lightmass bake in the entry map. Conventional direct light and sky fill remain the current path; this is not bounced GI. |
| Final validation | Bounded still comparisons and restoration are complete. Sun-fix compilation/fresh load, whole-map motion, packaged performance, stress, accessibility, and owner acceptance remain open. |

## Source references

- [Epic: postprocess and grading](https://dev.epicgames.com/documentation/en-us/unreal-engine/post-process-effects-in-unreal-engine)
- [Epic: volumetric fog](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-fog-in-unreal-engine)
- [Epic: movable light mobility](https://dev.epicgames.com/documentation/en-us/unreal-engine/movable-light-mobility-in-unreal-engine)
- [Epic: capsule shadows](https://dev.epicgames.com/documentation/en-us/unreal-engine/capsule-shadows-in-unreal-engine)
- [Epic: visibility and culling](https://dev.epicgames.com/documentation/en-us/unreal-engine/visibility-and-occlusion-culling-reference-in-unreal-engine)

Documentation was checked on 2026-09-05; installed engine headers were used to
resolve Python property names where UI display names differ. No files were
published or submitted, and no owner acceptance is recorded.
