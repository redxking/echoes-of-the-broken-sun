# Candidate correction and motion handoff

**Author and owner:** Angelis Pseftis
**Scope:** isolated concept production preparation; owner authorized gap remediation, animations and additional reference work.
**Baseline:** `docs/concept-production-pipeline`, `f85123f`; primary checkout `1a60cb1` contained extensive unrelated dirty gameplay/audio work. No primary checkout writes, engine launch, import, build or runtime integration took place.

[Open the self-contained motion review](motion-review.html). It has eight schematic animation studies and a searchable-by-selection library of all 21 candidate records. It works as a standalone HTML file without the local server. Play, pause, reset, scrubbing and reduced-motion poses are available. These diagrams are deliberately abstract mechanical/state studies; the concept paintings have not been animated, and there are no exported skeletons, animation clips or Unreal assets.

The authoritative machine-readable candidate audit is [candidate-review.json](candidate-review.json). It binds each current image and receipt by SHA-256, retains source motion passages from [reference-packages.json](../reference-packages.json), and records open gaps, track families, event binding, interruption, accessibility and audio requirements. Missing limitations in older receipts remain explicit review gaps; absence of a note never becomes acceptance. Future Well's older family receipt is explicitly mapped to its package ID. Candidate status does not supersede the concept register's canon classification or the owner's earlier image-specific feedback.

## Decisions and sequence

1. **Geometry before rigging.** Fix component counts, attachments, front/side/rear/top consistency, entrance/exit paths and packed clearances. Preserve Bulwark's owner-approved six centered panes, three per wing. Freeze an explicit component inventory before producing further poses. Generative views that repeatedly change anatomy are not modeling authority; use a dimensioned blockout in the later authorized production lane.
2. **Operational states before cosmetic detail.** Power Link must extinguish connection lights; Aegis must stop tracking and droop when offline; Foundry must stop its rail for research; Basin and Chorus Loom need distinct production/research/rest presentations. Shape or posture must supplement color. Keep critical damage, offline and insolvency separate states.
3. **Motion reference before clips.** Each unit needs idle, locomotion, turn/stop, action anticipation/execution/recovery, interruption, damage/death and selection treatment where the authoritative interface supports them. Workers additionally need gather/carry/deliver/build and authorized repair. Structures need build, operational/offline/recovery, role action, interruption and faction-specific destruction. Quoted creative briefs provide each asset's specific action language; common tracks are proposals until tied to actual adapter events.
4. **Complete the production package.** Establish approved scale and footprint, pivots, material zones, sockets, event/audio interfaces, collision/navigation boundaries, LOD/material/texture budget and rights evidence. No values are invented here. Required art, gameplay and technical gates remain open.

The eight studies cover Bulwark wing chains, Aegis power loss, Foundry job exclusivity and straight rail, Waystone root-before-movement ordering, Listening Spine sensor sequencing, Concordance pane registration, Growth Basin growth/molt and Chorus Loom production/research. Playback takes eight review seconds with a normalized progress slider. That is illustrative timing, never simulation timing. Future Well timing/color conflicts, Resonant weapon form, damage/destruction and remaining unit gaits are not covered by these eight studies.

A new Aegis mount study was retained at `BuildArtifacts/Evidence/concept-discovery-20260906/aegis-post-review/aegis-post-mount-study.png`. Its top-down inset shows three distinct radial supports beneath the bearing. The main view again obscures or omits the rear support; it is a supplemental partial correction, not a replacement approved rig reference. The original receipt now records the supplement, prompt, hash and limitation. Original candidate and earlier attempts remain preserved.

Growth Basin now has a darker resting pool in `growth-basin-state-contrast.png`; Chorus Loom now has a brighter research pane with subdued threads in `chorus-loom-state-contrast.png`. Both image-level improvements were visually inspected, retained alongside originals, and bound through the current receipts. Their gameplay-camera state-recognition gates remain open.

## Motion interfaces and unresolved authority

Every cue must read an existing authoritative event/state. Animation must not advance production, deduct resources, apply hits, reveal targets, move entities, change collision, or cause save/replay differences. Adapter symbol names remain TBD. On interruption or visibility loss, stop inappropriate loops/effects; on restore, reconstruct from current state rather than replaying stale attacks. Prevent duplicate audio when scrubbing or restoring. Root motion cannot become a competing movement authority. Reduced motion retains meaningful discrete poses, status labels and non-color distinctions.

Current primary requirements were inspected read-only alongside the package's creative-source excerpts. They are dirty in another workstream; retained source identity is recorded with this handoff's verification receipt. Numbers below are source observations, not new animation timings or accepted implementation.

| Contract | Binding required in future production |
|---|---|
| `SPEC-ART-001..003`, `SPEC-ACC-001..002`, `REL-ART-009` | Gameplay-camera motion/readability, required state families, non-color redundancy and reduced motion; authoritative velocity drives presentation. |
| `REL-FAC-005` | Bulwark deploy 20 ticks, pack 15, directional frontal condition. Six physical cells remain invariant. |
| `REL-FAC-002..004` | Aegis inert when unpowered; grid loss disables it within one tick. Do not let droop animation delay logical disable. |
| `REL-BLD-011..012` | Production/research exclusivity; interruption does not create a refund animation. |
| `REL-FAC-007` | Waystone uproot 40 ticks and root 60; mobile state offers no logistics/drop-off. Study motion does not prove support physics. |
| `REL-FAC-008` | Listening Spine detects anonymous moving signatures; no identity/target knowledge may leak through directional effects. |
| `REL-FAC-009` | Molt 80 ticks, 25 Dawn, 150% incoming damage; cancel without refund. Progress and vulnerability are state-driven. |
| `REL-FAC-011.SIG` | Choir payment/warnings/collapse follow the exact authoritative upkeep schedule and restricted-flash presentation. Simple dimming diagrams are not the full insolvency sequence. |
| `REL-FAC-013` | Phase Anchor 700 cm coverage; 5→4 Dawn reduction, overlap floor 3. Ring and neighbor flicker visualize state, never compute upkeep. |

Open conflicts are kept here rather than editing another task's requirements/state files:

- **Future Well:** Bible amber Harvest collapse and RequirementsState's adopted palette conflict with normative `REL-ART-014` cyan geyser wording. Retain the amber study as a candidate; do not silently resolve the master. `REL-WEL-*` acceptance remains open.
- **Resonant:** Bible “listens, does not fight” conflicts with authored attack/anti-scout records (`SPEC-UNIT-008`, `REL-FAC-026.KA.RESONANT`). Keep scout anatomy; a supported attack interface and its visual form still require resolution. Do not remove the attack or invent a cannon.
- **Critical damage versus offline:** `REL-ART-027` generic smoke/sparks below 30% must be reconciled with faction/offline direction. Offline is not damage; the Aegis offline study is silent and inactive. Do not copy generic smoke into all states.
- **Destruction:** `REL-BLD-014` debris lifecycle (200 ticks, clear at 201) remains separate from initial faction collapse motion. `REL-ART-013` material outcomes govern debris interpretation. No destruction clip or cleanup binding was implemented.

## Reproduction and checks

Run `build_review.py --packages <reference-packages.json> --evidence <retained evidence root>`; it reads sources and writes JSON to stdout only. It rejects missing images, duplicate receipts, hash mismatches and unmatched coverage. Save output to the existing candidate-review.json. Run `build_page.py` to emit the standalone HTML to stdout and save to the existing motion-review.html. Template plus data are sources; HTML is a derived review export, not another prose authority.

`python3 test_review.py` passed five positive/negative audit tests. `node test_motion.cjs` exercised eight studies at six sample positions in normal and reduced-motion modes (96 samples), 21 library records, and four control checks using a mocked canvas. This proves source execution only. Browser visual inspection was blocked because the browser interface could not verify its security policy; no alternate browser route was used. Layout, actual playback rendering, assistive-technology behavior and visual QA remain unverified.

No audit gap is automatically closed because a schematic exists. Image receipt hashes are verified where present; records lacking an old hash receive a current binding without pretending it is historical integrity evidence. Local evidence files are not backed up merely because Git tracks their path. A portable media handoff and final rights assessment remain required before integration.
