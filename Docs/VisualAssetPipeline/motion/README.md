# Concept production decisions and evidence handoff

**Author and owner:** Angelis Pseftis

All **53 remaining design decisions are selected under the owner's explicit delegation** to use the book and concepts and decide the remaining work. There are **zero undecided items in this 53-item decision batch**. This does not mean 53 assets or tests are complete: their reference/model/animation/integration evidence remains pending. The original 63-item census, findings, previous decisions and evidence histories are preserved. No asset receives owner acceptance or a production-stage promotion.

The current machine-readable authority for this isolated preparation lane is [gap-decisions.json](gap-decisions.json). Each of the 53 resolutions records the selected direction, rationale, relevant book paragraphs, exact candidate image/hash and source canon text. The `production_policy` section supplies 21 component inventories, reserved production IDs, planned Unreal names/paths, material/LOD limits, socket purposes, motion tracks, and acceptance cases. `master_amendments` contains six exact coordinated amendment packages; they are **prepared, not applied** to the shared master or runtime.

## Selected direction

- **Future Wells:** one bowl/spire family. Amber Harvest rises through the 180-tick telegraph and folds inward into a permanent spent bowl; Preserve is cyan custody, Reshape is magenta temporary terrain, Dormant remains quiet charcoal with doubled shadows. The book paragraphs 244–247 and game Bible support the selected Harvest treatment. Mechanics retain their authoritative timing, costs, control and payout rules.
- **Resonant:** an **unarmed sensor scout**. Its fragile fins and listening posture carry its purpose. This deliberately changes the future design from the current attack record; removal of attack orders/data/UI requires the prepared master amendment, coordinated integration and balance testing. No attack capability was removed from the current game in this task.
- **Phase Anchor:** bounded overlap, 5/4/3 Dawn with a floor of 3. More than two qualifying fields cannot make presence free. Membership and charges remain simulation-owned; exact register remains the visual exception among Choir structures.
- **Meridian anatomy:** Surveyor and Lancer are bipeds; Aegis is a three-foot, twin-emitter tripod. Supersede tread/pillbox language in the identified older asset cards. Lancer's future recoil socket is `Rear_Recoil_Strut_Anchor`; any old adapter name requires an explicit compatibility migration.
- **Geometry limits:** use the tighter of family and specific-card limits. Meridian's 8,000/3,500 triangle ceiling applies to Bulwark and Anchor too. The malformed Lancer LOD1 number resolves to 3,500. Other tighter caps remain intact. Provisional limits for families without a specific card are clearly labeled as design targets, never measured performance.
- **Destruction:** clear gameplay authority/occupancy immediately at destruction resolution. Cosmetic, nonblocking debris may persist for 200 simulation ticks. The delayed-passability wording needs the prepared reconciliation; a visual effect cannot hold a gameplay footprint.

Component counts not established in prose are explicit delegated design choices: Hearth has seven crown spires/four worker hollows/one intake; Waystone has four carriage pads/eight root bundles; Resonant has twelve fins; Listening Spine has twelve nodules. They establish stable future modeling inventories, not claims that the novel specifies those counts. Bulwark's already approved six centered panes remain unchanged. Generated turnarounds that altered anatomy remain rejected modeling references and do not supersede selected candidates.

## Production contracts

All identifiers below are **reserved only**; no engine assets have been created. Physical dimensions must be measured against the existing authoritative gameplay envelope. This is a measurement task, not permission to use the spectacular printed sizes in old artwork. Original canon classifications and `NOT_STARTED` maturity remain unchanged.

| Subject | Reserved production ID | Fixed component inventory |
|---|---|---|
| Lancer | `EBS-MER-UNT-002` | 2 legs; 2 feet; 1 hip lance; 1 recoil brace |
| Relay Skiff | `EBS-MER-UNT-004` | 1 hull; 1 relay mast; 1 dish; 1 archive cradle; 1 secondary emitter |
| Future Well | `EBS-FWL-SYS-001` | 1 bowl; 1 core spire; 4 state family |
| Surveyor | `EBS-MER-UNT-001` | 2 legs; 2 tool arms; 3 rear canisters; 1 optical mast |
| Bulwark Team | `EBS-MER-UNT-003` | 2 operators; 1 chassis; 6 barrier panes; 2 wing chains; 1 central emitter |
| Tender | `EBS-KHA-UNT-001` | 2 legs; 2 arms; 1 forked staff; 1 woven sling; 1 work apron |
| Anchor | `EBS-MER-BLD-001` | 1 drum; 1 mast; 3 worker bays; 1 matter intake |
| Power Link | `EBS-MER-BLD-002` | 4 numbered panels; 1 collar assembly; 2 base couplings; 4 conduits |
| Array Foundry | `EBS-MER-BLD-003` | 1 hall; 1 intake; 1 output; 1 longitudinal rail; 1 research gantry |
| Aegis Post | `EBS-MER-BLD-004` | 3 stationary legs; 3 feet; 1 azimuth bearing; 1 elevation cradle; 2 emitters |
| Memory Hearth | `EBS-KHA-BLD-001` | 1 dome; 7 crown spires; 4 worker hollows; 1 intake cleft |
| Waystone | `EBS-KHA-BLD-002` | 1 monolith; 4 carriage pads; 8 root bundles |
| Riftstalker | `EBS-KHA-UNT-002` | 4 locomotor limbs; 1 left shoulder caster |
| Cairnback | `EBS-KHA-UNT-003` | 4 locomotor limbs; 1 protected head; existing mineral_cover detached cover entity |
| Resonant | `EBS-KHA-UNT-004` | 4 locomotor limbs; 12 sensor fins; 0 weapons |
| Growth Basin | `EBS-KHA-BLD-003` | 1 pool; 4 visual niches; parallel capacity unchanged by niche count |
| Listening Spine | `EBS-KHA-BLD-004` | 1 curved rib; 12 sensor nodules; 1 rooted socket |
| Concordance | `EBS-HOL-BLD-001` | 6 pane pairs; 12 panes; 1 broad intake gap; 5 narrow worker intervals |
| Interval Loom | `EBS-HOL-BLD-002` | 2 crossed spans; 1 drop pane; 0 physical extra shadow meshes |
| Chorus Loom | `EBS-HOL-BLD-003` | 2 pylons; 1 research pane; 1 ground platform |
| Phase Anchor | `EBS-HOL-BLD-004` | 1 registered spire; 1 magenta ring; 0 offset copies |

## The 53 decisions

These are design resolutions, not completed production tasks. Detailed source/hash bindings and the remaining execution evidence live beside each ID in the ledger.

| Item | Selected direction |
|---|---|
| `EBS-PKG-MC-LANCER-GAP-01` | Two legs and two feet. Mount the lance at the anatomical right hip; connect its recoil brace to the rearward leg above the ankle. All four views use one neutral braced pose and the same weapon axis. |
| `EBS-PKG-MC-RELAY-SKIFF-GAP-01` | Keep the sealed strapped archive cradle and tall mast/dish. Place the existing secondary emitter on the hull front centerline below the deck, separate from lift pods. Relay light belongs to the mast. |
| `EBS-PKG-EBS-FAM-FWL-001-GAP-01` | Select amber Harvest telegraph and inward spire collapse; charcoal spent bowl is permanent. Preserve is cyan custody; Reshape is magenta temporary terrain. Dormant retains one bowl/spire with doubled shadows. |
| `EBS-PKG-MC-SURVEYOR-GAP-01` | Exactly three rear canisters in one transverse cradle. Anatomical right arm is drill; left is gripper with recessed palm welder. Two planted feet; stop tool motion on travel/cancel. Delivery empties cargo only on an authorized transfer. |
| `EBS-PKG-MC-BULWARK-TEAM-GAP-01` | Six persistent panes centered around the operator axis: 01–03 left, 04–06 right; center seam between 03/04. Each wing folds as a linked three-pane chain along its own chassis side. Disable field before packing. |
| `EBS-PKG-KA-TENDER-GAP-01` | Use the main candidate as attire source: two legs, two arms, forked staff in anatomical right hand, crossed woven sling and complete work apron. Kneel to gather, rise before travel, circle only within the authorized build presentation. |
| `EBS-PKG-MC-ANCHOR-GAP-01` | Freeze three worker bays across the front arc, separate intake on the front-right quadrant, central mast and squat drum. Keep bay emergence paths distinct from the delivery path; conduit sockets on rear/side plinth. |
| `EBS-PKG-MC-POWER-LINK-GAP-01` | Number panels 01 at top through 04 at bottom. One collar assembly, two base couplings and two conduits per coupling. Panels 02/03 remove toward the service face; connector remains physically traceable to its coupling. |
| `EBS-PKG-MC-POWER-LINK-GAP-03` | Use short physical conduit stubs and a separate cosmetic span component to the authorized neighbor endpoints. Cosmetic length carries no grid radius, collision or network authority. Connection state controls collar and pulse. |
| `EBS-PKG-MC-ARRAY-FOUNDRY-GAP-01` | Three independent masks: passive service marks, active fabrication rail, research gantry. Production lights rail and stops research; research lights gantry and stops rail; interruption stops both active masks and loops. |
| `EBS-PKG-MC-ARRAY-FOUNDRY-GAP-03` | Use the existing 4x4 footprint. Keep the hall and collision-free approach dressing within its approved presentation envelope; dimension openings against the largest authorized produced unit before detailing. |
| `EBS-PKG-MC-ARRAY-FOUNDRY-GAP-04` | Release only after separate art fidelity, gameplay readability and technical gates. Use source-matched hall/rail, default RTS and crowded views, then target-device material/LOD/performance capture. |
| `EBS-PKG-MC-ARRAY-FOUNDRY-GAP-05` | Intake and output are opposite short ends on one straight longitudinal rail. No side door is a production exit. Reserve an uninterrupted unit clearance corridor through the hall. |
| `EBS-PKG-MC-AEGIS-POST-GAP-01` | Exactly three radial supports at 120 degrees with three separate feet. Keep all three in the component inventory even where the camera hides one. Twin emitters remain on the head. |
| `EBS-PKG-MC-AEGIS-POST-GAP-02` | Stationary legs attach below the azimuth bearing. Head rotates above it; elevation cradle carries both emitters. Route flexible power through the bearing center and keep cables out of the emitter sweep. |
| `EBS-PKG-MC-AEGIS-POST-GAP-04` | Keep 2x2 occupancy and cosmetic feet within its approved art envelope. Use simulation weapon direction for tracking; disable emission/fire/audio on power loss before droop finishes. |
| `EBS-PKG-KA-MEMORY-HEARTH-GAP-02` | Freeze seven crown spires, four front worker hollows and one separate intake cleft from the main composition as the selected modeling layout. Healthy/damaged views share the entire shell and one fixed full-frame camera. |
| `EBS-PKG-KA-MEMORY-HEARTH-GAP-03` | Damage the front-right secondary crown spire: break its tip, extinguish that seam and fracture only adjacent upper strata. Preserve other six spires and all surviving hollows. |
| `EBS-PKG-KA-MEMORY-HEARTH-GAP-04` | Fit dome, hollows and cleft to 5x5. Construction grows base/hollows then crown; destruction slumps inward into basalt fragments, never metallic wreckage. Keep decorative interiors optional and collision-free. |
| `EBS-PKG-KA-WAYSTONE-GAP-01` | Choose four broad carriage pads and eight persistent root bundles. Retract roots sequentially before translation; walk with one pad swinging and three supporting, keeping the body center over support. |
| `EBS-PKG-KA-WAYSTONE-GAP-02` | Stow all eight root tips above the lowest carriage contact plane. Use a minimum candidate clearance of one root-tip thickness; check the actual terrain envelope before production acceptance. |
| `EBS-PKG-KA-WAYSTONE-GAP-03` | Rooted 2x2 state alone exposes drop-off/logistics. Uproot uses 40 ticks and root 60. Animation consumes mobile/rooted state and authoritative translation; no grounded root remains during travel. |
| `EBS-PKG-KA-WAYSTONE-GAP-04` | Freeze the selected component inventory now; production progression still requires accepted scale, geometry, rig, state-bound playback and three-gate evidence. Do not use this decision to promote NOT_STARTED. |
| `EBS-PKG-KA-RIFTSTALKER-GAP-02` | Use one grown caster saddle on anatomical left shoulder, attached by mineral tendon to the shoulder plate. Four locomotor limbs; no mirrored second caster or mechanical rotating turret. |
| `EBS-PKG-KA-RIFTSTALKER-GAP-03` | Keep a visible notch between head/brow and caster mouth from front-quarter and tactical view. Enlarge that negative space before adding small detail if the silhouette merges at zoom. |
| `EBS-PKG-KA-RIFTSTALKER-GAP-04` | Damage opens the left saddle seam only. Carapace adaptation thickens existing dorsal plates; striker adaptation extends existing caster plates, with four limbs retained. Firing sidestep is in-place pose sway and never changes the simulation location. |
| `EBS-PKG-KA-CAIRNBACK-GAP-02` | Brace all four feet, compress the front shoulders, heave a separate mineral barrier behind the body, release and recover. Keep visible separation between barrier and body after the authoritative creation event. |
| `EBS-PKG-KA-CAIRNBACK-GAP-03` | Use the existing mineral_cover entity geometry, placement and lifetime as the barrier constraint. Shape its surface as layered grown mineral; do not size its collision from the painting. |
| `EBS-PKG-KA-CAIRNBACK-GAP-04` | Four short load-bearing limbs, protected low head and layered vaultback. Show weight through compression and foot timing, not slower simulation. Death is a ceramic slump with detached chips under the common cosmetic debris lifecycle. |
| `EBS-PKG-KA-RESONANT-GAP-01` | Choose four delicate grounded limbs and twelve persistent fins along the head-to-tail dorsal line, numbered 01–12. Preserve a small inclined head; reuse this inventory across idle, sensing, travel and damage. |
| `EBS-PKG-KA-RESONANT-GAP-02` | Sense with one localized fin wave in fixed anatomical order, then a subdued active group. Reduced motion holds a stable group and sensor marker. Clear only from authoritative signature removal/expiry. |
| `EBS-PKG-KA-RESONANT-GAP-03` | Select UNARMED sensor-scout direction: no cannon, weapon socket, attack animation or direct-damage effect. Prepare explicit removal of current attack clauses for later authorized integration; current gameplay remains unchanged. |
| `EBS-PKG-KA-RESONANT-GAP-04` | Use thin amber fin surfaces with controlled translucent area and an authored opaque/masked distant LOD. Disable refraction and individual fin lights; prioritize readable edge shape over glass spectacle. |
| `EBS-PKG-KA-GROWTH-BASIN-GAP-01` | Rest is a dark open pool; production has a visible forming volume; molt shows one occupied niche and progress marker. Use occupancy and pose alongside amber brightness so color is not the only cue. |
| `EBS-PKG-KA-GROWTH-BASIN-GAP-02` | Keep the matrix liquid with a continuous surface, slow low-amplitude ripples and subsurface lattice. Freeze ripples under reduced motion; cancel settles without a success beat; reload reconstructs current active or idle state. |
| `EBS-PKG-KA-GROWTH-BASIN-GAP-04` | Use the same four-limb Riftstalker source in all niche poses. Outer dorsal plates loosen and new plates settle on existing attachment points; keep head, feet and left caster stable across the 80-tick molt. |
| `EBS-PKG-KA-GROWTH-BASIN-GAP-05` | Four visual niches surround a central pool inside 4x4; niche count grants no parallel capacity. Keep one clear ingress/egress route to the active niche and separate it from the emergence route. |
| `EBS-PKG-KA-LISTENING-SPINE-GAP-01` | Use a local sequence on twelve nodule identities. Generic cue is direction-neutral; display coarse bearing only if the authorized quantized signature supplies it. Stop/reconstruct from signature state on interruption/reload. |
| `EBS-PKG-KA-LISTENING-SPINE-GAP-02` | Freeze one curved rib, one rooted socket and twelve nodules numbered from base to tip. Position every nodule on the same centerline parameterization; do not add nodules for more contacts. |
| `EBS-PKG-KA-LISTENING-SPINE-GAP-03` | Fit socket and root dressing to 2x2; preserve the curved rib at distant LOD, merging micro-strata before shrinking silhouette. Construction rises from the root, damage dims a bounded nodule group, death breaks the rib into mineral fragments. |
| `EBS-PKG-HC-CONCORDANCE-GAP-01` | Six offset pane pairs / twelve panes, one broad intake gap at the front and five narrower emergence intervals. Keep all twelve identities across every view, state and LOD representation. |
| `EBS-PKG-HC-CONCORDANCE-GAP-03` | Local damage darkens one pane and aligns its partner. Destruction aligns all six pairs then extinguishes into interference; no sudden pane deletion. Core destruction/outcome remains immediate authoritative logic with the final-team-Core condition. |
| `EBS-PKG-HC-CONCORDANCE-GAP-04` | Fit the ring to 5x5 and show coherence through existing HUD/state markers plus pane behavior. Worker/intake routes remain open; a held tone is an effect, never a new central orb or gameplay entity. |
| `EBS-PKG-HC-INTERVAL-LOOM-GAP-01` | Exactly two physical spans with two fixed conflicting shadow solutions in the same lighting setup. Author the second shadow as a bounded cosmetic effect; use no cloned solid span or flickering extra geometry. |
| `EBS-PKG-HC-INTERVAL-LOOM-GAP-02` | Use two perpendicular grounded spans with a small positive crown clearance; keep the drop-pane accessible below and fit all physical geometry to 2x2. Preserve separate span identities. |
| `EBS-PKG-HC-INTERVAL-LOOM-GAP-03` | Payment triggers one brief brighten/dim. Deficit tiers, recovery and collapse use REL-FAC-011.SIG event state; reduced flashing uses a steady magenta outline and tier text. Stop warning hum on recovery. |
| `EBS-PKG-HC-INTERVAL-LOOM-GAP-04` | Require separate art, gameplay and technical acceptance, including fixed-light contradictory-shadow capture and upkeep/recovery/reload tests. Use authored LODs with stable spans; no Nanite or runtime collision from shadow effects. |
| `EBS-PKG-HC-CHORUS-LOOM-GAP-02` | Production draws a growing silhouette with moving threads and subdued research pane; research stills threads and brightens the raised pane. Interruption stills both; accompany states with existing HUD markers. |
| `EBS-PKG-HC-CHORUS-LOOM-GAP-03` | Two pylons, one research pane, one complete ground platform. Use an unclipped common camera framing for front/side/top; platform boundary derives from the pylon source, not a separate painted crop. |
| `EBS-PKG-HC-CHORUS-LOOM-GAP-04` | Fit two pylons and formation space within 4x4. Threads are cosmetic job-progress VFX; choose the existing selected roster unit only at final formation, with no new placeholder species becoming canon. |
| `EBS-PKG-HC-PHASE-ANCHOR-GAP-01` | A single exactly registered spire projects the authoritative 700 cm magenta ring. Membership uses entity positions and simulation state; the ring never computes membership or selection. |
| `EBS-PKG-HC-PHASE-ANCHOR-GAP-03` | Select bounded overlap: 5 Dawn outside, 4 in one field, 3 in two or more, never below 3. On loss remove ring and give affected neighbors one state-driven response; reduced flashing uses markers. |
| `EBS-PKG-HC-PHASE-ANCHOR-GAP-04` | Keep the single spire exactly in register in construction, operation, damage and collapse, within 2x2. Use local glass fissures for damage and light-interference dissolution for death; no offset duplicate mesh. |

## What is finished and what remains

The delegated decision pass, 21 production contracts and six master-amendment packages are prepared. Eight static state boards and five component construction sheets are available alongside eleven interactive schematic drafts. State-board rendering is reference QA; interactive browser playback remains unverified because browser policy verification was unavailable. No alternative browser route was used.

The remaining execution covers source-faithful modeling references, physical scale and clearance, actual rigs/clips/materials/audio, adapter binding, source/master reconciliation, and art/gameplay/technical acceptance. Those are now assigned directions and test requirements, rather than unanswered design questions. No model, clip, audio file, Unreal import, runtime behavior or owner acceptance is claimed from this document. The existing non-interference boundary still protects active gameplay work.

Run `python3 validate_decisions.py` for the 53-decision/21-contract audit and `python3 test_decisions.py` for its negative tests. The validator rejects missing decisions, image/book/canon hash and quoted-line drift, duplicate reserved IDs, unarmed attack tracks, combat destruction of indestructible Wells, missing amendments, and claims that planned amendments or production stages have been applied. Existing audit, topology, archive and schematic execution checks still apply. [handoff-receipt.json](handoff-receipt.json) identifies the current verified preservation export; prior exports remain historical evidence.

Role animation contracts also include relay activation/expiry, mineral cover, the 80-tick adaptation lifecycle, power loss/droop/recovery, worker production/emergence and Matter delivery, connection gain/loss, and rooted Waystone production. These tracks consume authoritative events; they do not create gameplay effects. Future Wells are indestructible and excluded from combat destruction; successful Harvest alone produces their persistent spent presentation.

## Earlier preparation record

The material below is the retained history from before the current delegated closure. Its references to open design/authority choices and earlier counts are historical; the selected directions and decision summary above supersede them. Its preserved test, artifact and provenance limits remain applicable unless explicitly updated above.

**Scope:** isolated concept production preparation; owner authorized gap remediation, animations and additional reference work.
**Baseline:** `docs/concept-production-pipeline`, `f85123f`; primary checkout `1a60cb1` contained extensive unrelated dirty gameplay/audio work. No primary checkout writes, engine launch, import, build or runtime integration took place.

[Open the self-contained motion review](motion-review.html). It has eleven schematic animation studies and a searchable-by-selection library of all 21 candidate records. It works as a standalone HTML file without the local server. Play, pause, reset, scrubbing and reduced-motion poses are available. These diagrams are deliberately abstract mechanical/state studies; the concept paintings have not been animated, and there are no exported skeletons, animation clips or Unreal assets.

The authoritative machine-readable candidate audit is [candidate-review.json](candidate-review.json). It binds each current image and receipt by SHA-256, retains source motion passages from [reference-packages.json](../reference-packages.json), and records open gaps, track families, event binding, interruption, accessibility and audio requirements. Missing limitations in older receipts remain explicit review gaps; absence of a note never becomes acceptance. Future Well's older family receipt is explicitly mapped to its package ID. Candidate status does not supersede the concept register's canon classification or the owner's earlier image-specific feedback.

### Decisions and sequence

1. **Geometry before rigging.** Fix component counts, attachments, front/side/rear/top consistency, entrance/exit paths and packed clearances. Preserve Bulwark's owner-approved six centered panes, three per wing. Freeze an explicit component inventory before producing further poses. Generative views that repeatedly change anatomy are not modeling authority; use a dimensioned blockout in the later authorized production lane.
2. **Operational states before cosmetic detail.** Power Link must extinguish connection lights; Aegis must stop tracking and droop when offline; Foundry must stop its rail for research; Basin and Chorus Loom need distinct production/research/rest presentations. Shape or posture must supplement color. Keep critical damage, offline and insolvency separate states.
3. **Motion reference before clips.** Each unit needs idle, locomotion, turn/stop, action anticipation/execution/recovery, interruption, damage/death and selection treatment where the authoritative interface supports them. Workers additionally need gather/carry/deliver/build and authorized repair. Structures need build, operational/offline/recovery, role action, interruption and faction-specific destruction. Quoted creative briefs provide each asset's specific action language; common tracks are proposals until tied to actual adapter events.
4. **Complete the production package.** Establish approved scale and footprint, pivots, material zones, sockets, event/audio interfaces, collision/navigation boundaries, LOD/material/texture budget and rights evidence. No values are invented here. Required art, gameplay and technical gates remain open.

The initial eight studies cover Bulwark wing chains, Aegis power loss, Foundry job exclusivity and straight rail, Waystone root-before-movement ordering, Listening Spine sensor sequencing, Concordance pane registration, Growth Basin growth/molt and Chorus Loom production/research. Playback takes eight review seconds with a normalized progress slider. That is illustrative timing, never simulation timing. Future Well timing/color conflicts, Resonant weapon form, damage/destruction and remaining unit gaits are not covered by those initial eight studies.

A new Aegis mount study was retained at `BuildArtifacts/Evidence/concept-discovery-20260906/aegis-post-review/aegis-post-mount-study.png`. Its top-down inset shows three distinct radial supports beneath the bearing. The main view again obscures or omits the rear support; it is a supplemental partial correction, not a replacement approved rig reference. The original receipt now records the supplement, prompt, hash and limitation. Original candidate and earlier attempts remain preserved.

Growth Basin now has a darker resting pool in `growth-basin-state-contrast.png`; Chorus Loom now has a brighter research pane with subdued threads in `chorus-loom-state-contrast.png`. Both image-level improvements were visually inspected, retained alongside originals, and bound through the current receipts. Their gameplay-camera state-recognition gates remain open.

### Motion interfaces and unresolved authority

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

### Reproduction and checks

Run `build_review.py --packages <reference-packages.json> --evidence <retained evidence root> --decisions <gap-decisions.json>`; it reads sources and writes JSON to stdout only. It rejects missing images, duplicate receipts, hash mismatches and unmatched coverage. The separate [gap-decisions.json](gap-decisions.json) owns persistent disposition history. CLI defaults to that file; regeneration preserves decisions. Missing/duplicate decisions, changed original findings and unbound correction evidence fail closed. Save output to the existing candidate-review.json. Run `build_page.py` to emit the standalone HTML to stdout and save to the existing motion-review.html. Template plus data are sources; HTML is a derived review export, not another prose authority.

`python3 test_review.py` now passes ten positive/negative audit and disposition tests. `node test_motion.cjs` now exercises eleven studies at six sample positions in normal and reduced-motion modes (132 samples), 21 library records, and four control checks using a mocked canvas. This proves source execution only. Browser visual inspection was blocked because the browser interface could not verify its security policy; no alternate browser route was used. Layout, actual playback rendering, assistive-technology behavior and visual QA remain unverified.

No audit gap is automatically closed because a schematic exists. Image receipt hashes are verified where present; records lacking an old hash receive a current binding without pretending it is historical integrity evidence. Local evidence files are not backed up merely because Git tracks their path. A portable media handoff and final rights assessment remain required before integration.

### Disposition pass after c646381

All 63 original items retain their stable IDs and original findings. Each now has a selected direction and specific next evidence. Two image defects have `CORRECTED_REFERENCE` evidence, eight have `DESIGN_DECIDED` directions, 25 require consistent reference geometry, seven require motion evidence, 19 require later integration evidence and two retain authority dependencies. Design decisions do not constitute canon approval; none of the 63 is marked owner-accepted. The 53 remaining evidence/dependency items are not reported as complete.

Power Link's new maintenance sheet includes both removed panels 02/03 and an exposed loose connector. The generated edit also changes cable routing, so it resolves the missing-panel item only; coupling geometry remains open. Phase Anchor's ring is now visibly magenta. Its dimensions and final material are still unvalidated. Both exact outputs and hashes are bound to their gap decisions; originals are preserved in receipt histories.

Eight delegated design choices are now explicit: the Foundry assembly stays a placeholder until an existing unit is selected; offline Aegis emission is zero; Hearth furniture is optional dressing; Riftstalker and Cairnback use four-limb candidate anatomy; four Basin niches create no gameplay capacity; Concordance's held tone stays an abstract effect; and Chorus Loom's partial volume creates no new unit. A six-pair Concordance ring is selected as the next geometry target, not claimed to exist consistently in the current painting.

Three more procedural motion drafts cover Resonant fin activation, Interval Loom's conflicting shadows/upkeep dimming, and Phase Anchor field loss with one neighbor response. They remain unrendered browser drafts because the same browser security-policy verification failed again. Source execution passed; that is not visual validation. Do not clear motion gaps on the strength of mocked-canvas tests.


### Construction and state-reference evidence

[Five construction sheets](construction/component-geometry.json) project the same dimensionless component vertices into front, side and top views. They cover Power Link panels/couplings, the Foundry rail and end portals, Aegis tripod/bearing attachment, Concordance's candidate twelve-pane layout, and the Interval Loom's two grounded spans with crown clearance. These simplified component drawings constrain topology; they do not reproduce finished surface geometry, establish physical dimensions or validate a rig sweep. Seven topology tests check component identities, support placement, rail alignment and crown separation.

[Seven motion packages](motion-packages.json) bind each motion gap to source contract IDs, six discrete reference states, interruption/reload behavior, reduced-motion treatment, audio intent and later acceptance evidence. Their [state boards](storyboards/) were rendered as vector artifacts and visually inspected. They supplement the eleven interactive drafts; browser playback remains unverified. Audio is specified but neither generated nor auditioned. The board for Phase Anchor deliberately avoids choosing a numerical stacking rule.

The independent `preparation` field in [gap-decisions.json](gap-decisions.json) now distinguishes nine scoped artifacts ready for review, six partial construction references, 27 briefs ready, 19 integration deferrals and two original authority-blocked records. `ARTIFACT_READY` describes the stated evidence scope only; it does not close the compound finding. All 63 retain `NOT_ACCEPTED`, and the 53 original evidence/dependency items remain open. In particular, the 19 reference items with briefs still need source-faithful artwork or geometry; they are not reported as completed references. The two new Lancer/Surveyor turnarounds failed consistency review and remain preserved as rejected modeling references, with original candidates unchanged.

There are now **three recorded authority conflicts**: Future Well Harvest, Resonant combat, and Phase Anchor overlap. The third is newly identified: `SPEC-BLD-017.HC.ANCHOR` says fields do not stack while `REL-FAC-013` specifies an overlap floor of three. This is attached to the existing Phase Anchor handoff rather than changing the original 63-item census. Resolve the master authority before final overlap validation. No primary requirements or runtime files were changed.

Run `python3 construction/test_drawings.py` for the seven construction checks and `python3 test_review.py` for fifteen audit/decision checks. Prepared artifacts require a scope, remaining evidence, hash binding and unaccepted state. Missing or modified evidence and attempted acceptance promotion fail. [preparation-evidence.json](preparation-evidence.json) retains generator and render hashes. The static boards establish reviewed reference intent, not executed timing, physical motion, collision, material performance or owner acceptance.

### Portable preservation handoff

`export_handoff.py --repo <isolated worktree> --evidence <discovery evidence root> --output <new archive outside both input trees>` reads all 244 inventoried repository visual paths, the pipeline documents and the retained discovery/candidate evidence. It fails on source hash drift, missing files, symlinks or an existing output, and re-reads each input to detect changes during packaging. It never edits source files. `--verify <archive>` checks member coverage and every payload hash without extraction. The manifest maps historical absolute paths to portable archive members; original documents remain authoritative and historical provenance is retained.

The local archive is a preservation export, not off-device backup or publication. It contains the supplied book and concept history; do not publish it as a public download. Rights, licenses and owner acceptance remain independent gates. The next production lane must begin with the unresolved reference geometry and authority conflicts, then obtain stage-specific evidence for blockout, rigs, Unreal integration and the art/gameplay/technical gates. This planning lane has not crossed those stages.

Eight preservation tests exercise archive round-trip, inventory drift, overwrite refusal, output recursion, tampered payloads unmanifested members, and payload resolution from a separate read-only root, and partial-output cleanup on a failed write. The local export is retained under `BuildArtifacts/Evidence/concept-handoff-20260906/concept-pipeline-handoff-reviewed.zip`; its separately tracked receipt binds the exported bytes. Rebuilding an archive uses a new explicit output path; no source cleanup or replacement is performed.

The isolated worktree contains Git LFS pointers for 79 inventory paths. All 244 full original payloads were found in the primary checkout and matched the discovery hashes. Export with `--source-root <primary Project>` to read those payloads without modifying or hydrating either checkout; the default source root remains the isolated worktree. A hash mismatch fails rather than substituting a newer image.

Final state-board QA corrected the Basin reload example to an active authoritative snapshot and clarified Concordance's conditional match outcome: losing one Core does not end team play while a required team Core survives. Resonant expiry wording now reads only authoritative signature state, never hidden-unit velocity. The first preservation archive remains a historical pre-review export; the reviewed archive and current receipt identify the corrected delivery.

The export manifest records dirty-path observations for both the isolated documentation checkout and the visual payload checkout. Selected payload hashes prove image identity even when unrelated primary work is dirty. On a failed archive write or verification, only that attempt's newly created partial output is removed; existing outputs and all inputs are preserved.

The exact inspected Requirements and Development Bible payloads are retained under `motion-preparation/authority-evidence/` as `EVIDENCE_COPY_NOT_AUTHORITY`, with full-file hashes and original paths. This matters because the inspected primary Requirements file was dirty and differs from the isolated checkout. Those evidence copies preserve the basis of the motion decisions; the primary documents remain the sole design authority.

Evidence limit: the earlier dirty RequirementsState payload was not captured before another workstream changed it. Its historical hash is retained, but the archive cannot reconstruct that exact state document. A separately labeled current observation is preserved without claiming equivalence. The exact Requirements and Bible payloads underlying the motion packages were successfully retained with matching hashes.
## Unreal technical sources

[UnrealReferences.md](../UnrealReferences.md) is the maintained Epic documentation and installed-engine reference library for future model, animation, material, VFX, audio and validation work. It maps references to all 21 production packages and records version and evidence limits.
