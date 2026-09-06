# Candidate correction and motion handoff

**Author and owner:** Angelis Pseftis
**Scope:** isolated concept production preparation; owner authorized gap remediation, animations and additional reference work.
**Baseline:** `docs/concept-production-pipeline`, `f85123f`; primary checkout `1a60cb1` contained extensive unrelated dirty gameplay/audio work. No primary checkout writes, engine launch, import, build or runtime integration took place.

[Open the self-contained motion review](motion-review.html). It has eleven schematic animation studies and a searchable-by-selection library of all 21 candidate records. It works as a standalone HTML file without the local server. Play, pause, reset, scrubbing and reduced-motion poses are available. These diagrams are deliberately abstract mechanical/state studies; the concept paintings have not been animated, and there are no exported skeletons, animation clips or Unreal assets.

The authoritative machine-readable candidate audit is [candidate-review.json](candidate-review.json). It binds each current image and receipt by SHA-256, retains source motion passages from [reference-packages.json](../reference-packages.json), and records open gaps, track families, event binding, interruption, accessibility and audio requirements. Missing limitations in older receipts remain explicit review gaps; absence of a note never becomes acceptance. Future Well's older family receipt is explicitly mapped to its package ID. Candidate status does not supersede the concept register's canon classification or the owner's earlier image-specific feedback.

## Decisions and sequence

1. **Geometry before rigging.** Fix component counts, attachments, front/side/rear/top consistency, entrance/exit paths and packed clearances. Preserve Bulwark's owner-approved six centered panes, three per wing. Freeze an explicit component inventory before producing further poses. Generative views that repeatedly change anatomy are not modeling authority; use a dimensioned blockout in the later authorized production lane.
2. **Operational states before cosmetic detail.** Power Link must extinguish connection lights; Aegis must stop tracking and droop when offline; Foundry must stop its rail for research; Basin and Chorus Loom need distinct production/research/rest presentations. Shape or posture must supplement color. Keep critical damage, offline and insolvency separate states.
3. **Motion reference before clips.** Each unit needs idle, locomotion, turn/stop, action anticipation/execution/recovery, interruption, damage/death and selection treatment where the authoritative interface supports them. Workers additionally need gather/carry/deliver/build and authorized repair. Structures need build, operational/offline/recovery, role action, interruption and faction-specific destruction. Quoted creative briefs provide each asset's specific action language; common tracks are proposals until tied to actual adapter events.
4. **Complete the production package.** Establish approved scale and footprint, pivots, material zones, sockets, event/audio interfaces, collision/navigation boundaries, LOD/material/texture budget and rights evidence. No values are invented here. Required art, gameplay and technical gates remain open.

The initial eight studies cover Bulwark wing chains, Aegis power loss, Foundry job exclusivity and straight rail, Waystone root-before-movement ordering, Listening Spine sensor sequencing, Concordance pane registration, Growth Basin growth/molt and Chorus Loom production/research. Playback takes eight review seconds with a normalized progress slider. That is illustrative timing, never simulation timing. Future Well timing/color conflicts, Resonant weapon form, damage/destruction and remaining unit gaits are not covered by those initial eight studies.

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

Run `build_review.py --packages <reference-packages.json> --evidence <retained evidence root> --decisions <gap-decisions.json>`; it reads sources and writes JSON to stdout only. It rejects missing images, duplicate receipts, hash mismatches and unmatched coverage. The separate [gap-decisions.json](gap-decisions.json) owns persistent disposition history. CLI defaults to that file; regeneration preserves decisions. Missing/duplicate decisions, changed original findings and unbound correction evidence fail closed. Save output to the existing candidate-review.json. Run `build_page.py` to emit the standalone HTML to stdout and save to the existing motion-review.html. Template plus data are sources; HTML is a derived review export, not another prose authority.

`python3 test_review.py` now passes ten positive/negative audit and disposition tests. `node test_motion.cjs` now exercises eleven studies at six sample positions in normal and reduced-motion modes (132 samples), 21 library records, and four control checks using a mocked canvas. This proves source execution only. Browser visual inspection was blocked because the browser interface could not verify its security policy; no alternate browser route was used. Layout, actual playback rendering, assistive-technology behavior and visual QA remain unverified.

No audit gap is automatically closed because a schematic exists. Image receipt hashes are verified where present; records lacking an old hash receive a current binding without pretending it is historical integrity evidence. Local evidence files are not backed up merely because Git tracks their path. A portable media handoff and final rights assessment remain required before integration.

## Disposition pass after c646381

All 63 original items retain their stable IDs and original findings. Each now has a selected direction and specific next evidence. Two image defects have `CORRECTED_REFERENCE` evidence, eight have `DESIGN_DECIDED` directions, 25 require consistent reference geometry, seven require motion evidence, 19 require later integration evidence and two retain authority dependencies. Design decisions do not constitute canon approval; none of the 63 is marked owner-accepted. The 53 remaining evidence/dependency items are not reported as complete.

Power Link's new maintenance sheet includes both removed panels 02/03 and an exposed loose connector. The generated edit also changes cable routing, so it resolves the missing-panel item only; coupling geometry remains open. Phase Anchor's ring is now visibly magenta. Its dimensions and final material are still unvalidated. Both exact outputs and hashes are bound to their gap decisions; originals are preserved in receipt histories.

Eight delegated design choices are now explicit: the Foundry assembly stays a placeholder until an existing unit is selected; offline Aegis emission is zero; Hearth furniture is optional dressing; Riftstalker and Cairnback use four-limb candidate anatomy; four Basin niches create no gameplay capacity; Concordance's held tone stays an abstract effect; and Chorus Loom's partial volume creates no new unit. A six-pair Concordance ring is selected as the next geometry target, not claimed to exist consistently in the current painting.

Three more procedural motion drafts cover Resonant fin activation, Interval Loom's conflicting shadows/upkeep dimming, and Phase Anchor field loss with one neighbor response. They remain unrendered browser drafts because the same browser security-policy verification failed again. Source execution passed; that is not visual validation. Do not clear motion gaps on the strength of mocked-canvas tests.


## Construction and state-reference evidence

[Five construction sheets](construction/component-geometry.json) project the same dimensionless component vertices into front, side and top views. They cover Power Link panels/couplings, the Foundry rail and end portals, Aegis tripod/bearing attachment, Concordance's candidate twelve-pane layout, and the Interval Loom's two grounded spans with crown clearance. These simplified component drawings constrain topology; they do not reproduce finished surface geometry, establish physical dimensions or validate a rig sweep. Seven topology tests check component identities, support placement, rail alignment and crown separation.

[Seven motion packages](motion-packages.json) bind each motion gap to source contract IDs, six discrete reference states, interruption/reload behavior, reduced-motion treatment, audio intent and later acceptance evidence. Their [state boards](storyboards/) were rendered as vector artifacts and visually inspected. They supplement the eleven interactive drafts; browser playback remains unverified. Audio is specified but neither generated nor auditioned. The board for Phase Anchor deliberately avoids choosing a numerical stacking rule.

The independent `preparation` field in [gap-decisions.json](gap-decisions.json) now distinguishes nine scoped artifacts ready for review, six partial construction references, 27 briefs ready, 19 integration deferrals and two original authority-blocked records. `ARTIFACT_READY` describes the stated evidence scope only; it does not close the compound finding. All 63 retain `NOT_ACCEPTED`, and the 53 original evidence/dependency items remain open. In particular, the 19 reference items with briefs still need source-faithful artwork or geometry; they are not reported as completed references. The two new Lancer/Surveyor turnarounds failed consistency review and remain preserved as rejected modeling references, with original candidates unchanged.

There are now **three recorded authority conflicts**: Future Well Harvest, Resonant combat, and Phase Anchor overlap. The third is newly identified: `SPEC-BLD-017.HC.ANCHOR` says fields do not stack while `REL-FAC-013` specifies an overlap floor of three. This is attached to the existing Phase Anchor handoff rather than changing the original 63-item census. Resolve the master authority before final overlap validation. No primary requirements or runtime files were changed.

Run `python3 construction/test_drawings.py` for the seven construction checks and `python3 test_review.py` for fifteen audit/decision checks. Prepared artifacts require a scope, remaining evidence, hash binding and unaccepted state. Missing or modified evidence and attempted acceptance promotion fail. [preparation-evidence.json](preparation-evidence.json) retains generator and render hashes. The static boards establish reviewed reference intent, not executed timing, physical motion, collision, material performance or owner acceptance.

## Portable preservation handoff

`export_handoff.py --repo <isolated worktree> --evidence <discovery evidence root> --output <new archive outside both input trees>` reads all 244 inventoried repository visual paths, the pipeline documents and the retained discovery/candidate evidence. It fails on source hash drift, missing files, symlinks or an existing output, and re-reads each input to detect changes during packaging. It never edits source files. `--verify <archive>` checks member coverage and every payload hash without extraction. The manifest maps historical absolute paths to portable archive members; original documents remain authoritative and historical provenance is retained.

The local archive is a preservation export, not off-device backup or publication. It contains the supplied book and concept history; do not publish it as a public download. Rights, licenses and owner acceptance remain independent gates. The next production lane must begin with the unresolved reference geometry and authority conflicts, then obtain stage-specific evidence for blockout, rigs, Unreal integration and the art/gameplay/technical gates. This planning lane has not crossed those stages.

Seven preservation tests exercise archive round-trip, inventory drift, overwrite refusal, output recursion, tampered payloads unmanifested members, and payload resolution from a separate read-only root. The local export is retained under `BuildArtifacts/Evidence/concept-handoff-20260906/concept-pipeline-handoff.zip`; its separately tracked receipt binds the exported bytes. Rebuilding an archive uses a new explicit output path; no source cleanup or replacement is performed.

The isolated worktree contains Git LFS pointers for 79 inventory paths. All 244 full original payloads were found in the primary checkout and matched the discovery hashes. Export with `--source-root <primary Project>` to read those payloads without modifying or hydrating either checkout; the default source root remains the isolated worktree. A hash mismatch fails rather than substituting a newer image.
