---
title: Echoes of the Broken Sun Art Direction Reference
author: Angelis Pseftis
creator: Angelis Pseftis
status: Operational reference — subordinate to the Development Bible and Track A
created: 2026-09-01
updated: 2026-09-01
---

# Art Direction Reference

This is the single page every visual pass is checked against before it is called done. It does not
replace authority: creative intent lives in `Docs/Archive/DevelopmentBible.md` (Art and audio), the
gate program lives in `Docs/GameCompletionDirective.md` Track A, and acceptance evidence lives in
`Docs/Archive/ProjectLedger.md`. Where this page and those documents disagree, they win and this page
gets corrected. What this page adds is the operational layer: the accepted concrete values, the frame
hierarchy, and the binary composed-frame checklist established by the owner review protocol
(ledger ART-A4-002).

## The direction in one sentence

A dark vitrified world that quietly carries the memory of a shattered golden sun, under a gold/indigo
sky duality, where the terrain recedes, the actors own the light, and every strategic fact is readable
at combat speed without color.

## Master palette

Five notes, plus one temperature complement. Every surface, effect, and interface element must be
attributable to one of them. A color that cannot name its note does not ship.

| Note | Role | Accepted anchors |
|---|---|---|
| **Charcoal** | Terrain body, structure mass, UI panel ground | Ground body 0.02–0.07 linear; ground roughness floor 0.85; never lifted to grey by fill light |
| **Pale ceramic** | Civic/Compact-built surfaces, UI type | Panel tone ~0.86 with edge/load-point wear, roughness ~0.34, metallic 0.04 (`ceramic_civic`) |
| **Broken-sun amber/gold** | The world's warm accent: fracture veins, Kharuun mineral identity, key light, alert accent | Key light (1.0, 0.82, 0.62) at intensity 10; ground vein glow weighted (0.50, 0.22, 0.06) with gamma-2.2 falloff, ember-dim and matte, steady under reduced flashing |
| **Magenta-fracture** | Crownfall phenomena, possibility bleed, the Choir | Vitrified-glass micro-fracture veins tint red+blue over a charcoal body (`vitrified_glass`); rises across Act III per Track A |
| **Cyan** | Matter, Meridian engineered systems, interface confirmation | Unit/structure accent meshes, Matter deposits, order-confirmation and UI confirm states |
| *Indigo (complement)* | The sky's cool half: fill light and shadow temperature, not a surface color | Sky fill (0.48, 0.60, 0.88) at intensity 1.6 |

The gold key against the indigo fill is the Crownfall duality and the game's lighting signature.
Layer separation works by temperature as well as value: warm belongs to actors' faces and the world's
scars, cool belongs to the receding ground and shadow.

## The composed-frame hierarchy

Established as the standing review gate in ART-A4-002. Every layer has a budget; claiming another
layer's budget is a defect regardless of how good the element looks in isolation.

1. **Terrain** — lowest saturation, no emissive claim, matte (roughness ≥ 0.85 on ground). It may be
   beautiful only in ways that recede: seams, strata, quiet vein glow.
2. **World landmarks** (Future Well, routes, mission sites) — may carry emissive identity, held below
   any actor's claim; landmarks orient, they do not compete.
3. **Actors** (units and structures) — own the saturation and emissive budgets. Silhouette first;
   faction accent (cyan / amber / magenta) reads at gameplay camera height.
4. **Transient feedback** (order sigils, selection, destruction, telegraphs) — the brightest
   short-lived elements, shape-identifiable without color, gone in seconds.
5. **Interface** — the brightest stable layer: charcoal panels, pale-ceramic type, cyan confirm,
   amber alert. The battlefield stays visually primary; the UI frames it and never floods it.

## Lighting and exposure rules

- One authored rig per site, key direction motivated by the Crownfall sky. Glass Scar accepted rig:
  gold key (1.0, 0.82, 0.62) at 10.0; indigo skylight fill (0.48, 0.60, 0.88) at 1.6
  (`EchoesGameMode.cpp`, A1/ART-A4-002). New sites re-weight this duality; they do not invent new suns.
- Exposure is authored (`exposure-authored-v1`, EXPOSURE-001), never free-running auto-exposure.
- Accepted capture window: clipped highlights ≤ 0.005% of pixels, mean luma between roughly 50 and 70
  (accepted frames measure 55.9–66.1 via `measure_capture_exposure.py`). A frame outside this window
  fails regardless of intent.
- No terrain specular event larger than an actor silhouette at gameplay zoom. The matte pass exists
  precisely because ground glint competed with actors; do not reintroduce it.
- Nanite and Virtual Shadow Maps stay off (M1 Pro baseline). Presentation lights cast no shadows
  unless a ledger entry deliberately accepts one.
- The Broken Sun itself enters the frame indirectly at RTS pitch — through the key's direction and
  warmth, vein emissives, and long value gradients — and directly only in low-pitch cinematic and
  title framing. The sky object (open A4 item) must satisfy both.

## Materials vocabulary

Deterministic families in `Scripts/echoes_texture_synth.py`, registered per revision
(current: textures `surface-textures-v7`, world master `world-surface-textured-v6`). Byte-idempotent
regeneration is part of a family's definition.

| Family | Character |
|---|---|
| `T_EchoesGlassScarGround` | Dark vitrified basalt, six long golden fracture arteries, 25 m world tiling, ember-dim matte veins |
| `T_EchoesVitrifiedGlass` | Deep charcoal glass, magenta micro-fracture, low roughness body |
| `T_EchoesCausewayAsh` | Basalt/ash strata with a foot-polished track — wear tells use |
| `T_EchoesCeramicCivic` | Pale beveled civic paneling, wear at edges and load points, never uniform grime |

Still owed under A3, same recipe discipline: Compact machined metal with status-band paint, Kharuun
grown mineral (banded strata, translucent amber nodules), Choir coherent-light surfaces
(view-shifting emissive within reduced-flashing limits), Matter deposit crystal, and route-specific
wear. Wear is history, not dirt: it appears where hands, feet, loads, and repairs have been.

## Faction form languages

Authoritative in the Bible; the short form for review:

- **Meridian Compact** — engineered load paths: orthogonal rails, plates, exposed connections,
  visible repair states, cyan accents, bracketed rectangular ownership marks.
- **Kharuun Assemblies** — grown mineral structure: facets, cones, nodules, layered strata, amber
  accents, inhabited and maintained, never primitive; paired faceted ownership marks.
- **Hollow Choir** — repeated luminous edges with deliberate contradiction: offset duplicates, spans
  with two valid shadows, maintained-possible rather than solid; magenta identity, offset concentric
  ownership marks.

A silhouette that needs its accent color to say which faction it is has failed the check.

## Effects grammar

- **Compact effects are engineered**: directional, clean edges, cyan-white.
- **Kharuun effects are material**: dust, strata shards, amber heat.
- **Choir effects are phase**: offset afterimages, magenta interference.
- The Future Well's three protocols must *feel* like their meaning: Harvest as windfall-and-permanent
  loss, Preserve as slow custody, Reshape as temporary impossible terrain — each with its public
  telegraph readable by both players.
- Invariants for every effect, no exceptions: presentation-only, spawned from authoritative state;
  no collision, overlap, navigation influence, or shadows; shape-first identification; explicit
  reduced-motion (transforms hold) and reduced-flashing (constant emission) branches; nothing enters
  saves, replays, fog authority, or checksums. Mesh-VFX by default; each Niagara system is a recorded
  exception with measured cost.

## Interface rules

One system across every screen (A8): charcoal panels, pale-ceramic type, cyan confirmation, amber
alert, the three non-color ownership marks everywhere ownership appears. Everything obeys HUD scale
and has a high-contrast variant that changes palette, never information. Development diagnostics live
behind a flag absent from packaged builds (A2); a placeholder leaves only when its registered
replacement lands. Typography remains an open decision under directive section 9 — a licensed or
original registered typeface; until it is registered, no decorative font enters the project.

## The composed-frame checklist

Run against real gameplay captures — gameplay camera height, 1920×1080 minimum, captured only after
the async compile queue drains (capture-integrity rule, ART-A3-001). A visual deliverable passes when
every line is true:

1. Exposure: clip ≤ 0.005%, mean luma 50–70, measured, not eyeballed.
2. Terrain saturation and brightness sit below every actor; no ground emissive or specular event
   competes with an actor silhouette.
3. Every faction and role on screen is identifiable in a grayscale copy of the capture.
4. Order state, selection, and telegraphs are shape-distinct without color.
5. High-contrast, reduced-motion, and reduced-flashing variants captured and behaving.
6. No debug or development string in a player-facing configuration.
7. Layer hierarchy holds: UI type is the brightest stable element; transient feedback may exceed it
   only briefly.
8. Nothing new collides, casts shadows, influences navigation, or touches simulation state.
9. The asset is registered (`AssetRegister.md`), regeneration is byte-idempotent, and the ledger
   entry states what the pass does *not* cover.
10. The owner's eye remains the final acceptance authority (ART-A4-002); this checklist qualifies a
    pass for that review, it does not replace it.
