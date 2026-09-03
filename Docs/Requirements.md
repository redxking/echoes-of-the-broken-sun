# Echoes of the Broken Sun — Requirements

**Author and owner:** Angelis Pseftis
**Standing:** master requirements document. This file defines every requirement for the game.
**Created:** 2026-09-03, by owner instruction to consolidate the project's requirements into a single
authoritative document.

This file holds requirement **bodies**: identity, normative statement, and acceptance criteria. It holds no
engineering state and no evidence. Per-requirement state lives in
**[`RequirementsState.md`](RequirementsState.md)** and nowhere else. Those two files are the whole
requirements system; a third document that appears to define or accept a requirement is wrong by
construction.

## Authority

This document is the sole normative definition of what the game shall be. It replaces the authority
previously claimed by `Archive/Superseded/EchoesOfTheBrokenSun_CompleteGameRequirements.docx`, whose text it carries forward
and whose `AUTH-*` rules it retains as `SPEC-AUTH-*`. That `.docx` is retained as the origin record only
and no longer governs; where it and this file differ, this file wins.

Creative intent for world, factions, characters, and narrative remains with
`Archive/DevelopmentBible.md`. Sequencing, gates, and lane control remain with
`GameCompletionDirective.md`, `DeliveryPlan.md`, and `../WorkstreamControl/ACTIVE_LANES.md` — those
documents order the work and may not define, restate, or waive a requirement.

Carried forward from the build specification and still binding:

* **SPEC-AUTH-002 — Normative language.** *Shall* is mandatory. *May* is permitted. *Tuning baseline* is a
  binding starting value that may change only through documented balance review without changing the
  element's purpose or counterplay.
* **SPEC-AUTH-003 — No silent invention.** Where a required behavior is genuinely absent or contradictory,
  work stops at that decision boundary. The proposed change identifies affected requirements, player
  impact, dependencies, migration, tests, and owner approval.
* **SPEC-AUTH-004 — Traceable change.** Every approved change edits **this file** in place, preserves
  stable identifiers where meaning is unchanged, records rationale, and updates dependent content, UI, AI,
  saves, replays, tests, and player documentation. (Amended 2026-09-03: the file this rule governs is now
  `Docs/Requirements.md`, not the `.docx`.)

## Identifier namespaces

Identifiers are permanent. Nothing here is ever renumbered, reused, or deleted: 335 citations in
`../WorkstreamControl/` evidence logs, QA matrices, and lane handoffs resolve against these IDs, and
renumbering would break the evidence trail for work already accepted. A withdrawn requirement keeps its ID
and reads `SUPERSEDED BY <id>` or `WITHDRAWN — <authority, date>`.

| Namespace | Count | Origin | Defines |
|---|---|---|---|
| `SPEC-*` | 368 | the build specification | what the finished game shall be |
| `DEMO-*` | 152 | `DemoReadinessRequirements.md`, retired 2026-09-03 | what the demo must satisfy |
| `REL-*` | 475 | `InitialReleaseRequirements.md`, retired 2026-09-03 | what the initial release must satisfy |

The three namespaces are independent. Sixteen prefix names collide across them with unrelated numbering —
`SPEC-SIM-005` and `REL-SIM-005` are different requirements. Never treat a shared prefix as a match. The
crosswalk binding `DEMO-*` and `REL-*` records to the `SPEC-*` requirement each implements is built during
their migration and recorded in the *Crosswalk* section below.

Of the 368 `SPEC-*` records, **129** carry the identifiers the build specification already assigned
(prefix added, number untouched) and **239** were minted on 2026-09-03 for normative content that
previously had no identifier at all — including the entire twelve-unit roster, the twelve-structure set,
and all fifteen mission objective and failure contracts. Nothing could cite or test that content before.

## Record schema

Every requirement carries the acceptance-card fields the build specification defined in its §29:
requirement ID and title; player purpose; preconditions and inputs; authoritative result; presentation;
failures and degraded states; AI and campaign behavior; save and replay behavior; verification;
dependencies and change impact. A field that is genuinely inapplicable records `NOT APPLICABLE` with a
technical reason and owner acceptance — never silent omission.

Cards are populated as a requirement enters active work, hierarchically under its ID
(`SPEC-UNIT-001.SIG`). Records below carry the normative statement and its binding values; a record
without a populated card is not ready to leave `IN PROGRESS`.

## Verification classes

`SRC` source and schema inspection · `PKG-AUTO` packaged-build automation · `PKG-REND` packaged rendered
or audible inspection · `PKG-PHYS` packaged build driven by physical mouse and keyboard · `EDT` editor
demonstration, which never substitutes for a `PKG` class · `HUM` uncoached project-naive human sessions ·
`OWNER` personal owner acceptance. No class substitutes for another. Only Angelis Pseftis assigns
`HUMAN ACCEPTED` or `HUMAN REJECTED — CHANGES REQUIRED`.

## Migration status

| Source | Status |
|---|---|
| `Archive/Superseded/EchoesOfTheBrokenSun_CompleteGameRequirements.docx` §1–§31 | **Migrated in full**, 2026-09-03. Text verbatim, tables preserved, 239 identifiers minted. |
| `DemoReadinessRequirements.md` | **Migrated in full**, 2026-09-03. Bodies below; state and the append-only change log in `RequirementsState.md`. Retired to `Archive/Superseded/`. |
| `InitialReleaseRequirements.md` | **Migrated in full**, 2026-09-03. Bodies below; governance retained in Part III; change log in `RequirementsState.md`. Retired to `Archive/Superseded/`. |
| `MovementAndBalanceRequirements.md` | **Not merged.** Self-marked "proposed addendum… the master document wins until this is merged." Merging is an owner decision, not an agent's. |

Two requirements — `DEMO-NAR-010` and `DEMO-NAR-011` — were owner-added on 2026-09-02 through change-log
entries and never seated in a requirement section. Consolidation recovered both bodies and placed them at the
end of Part II section C. Their change-log entries are retained as history.

## Crosswalk

Each `DEMO-*` and `REL-*` family bound to the `SPEC-*` requirements it implements. Mapping is by family,
read from section intent; a family marked *spec gap* has no `SPEC-*` parent, which is a finding against the
specification, not a defect in the record. Never infer a match from a shared prefix name:
`REL-SIM-005` and `SPEC-SIM-005` are unrelated requirements.

| Records | Count | Implements |
|---|---|---|
| `DEMO-ACC-001..006` | 6 | SPEC-ACC-*, SPEC-LOC-* |
| `DEMO-AI-001..010` | 10 | SPEC-AI-*, SPEC-AIST-*, SPEC-DOC-*, SPEC-DIF-* |
| `DEMO-AUD-001..013` | 13 | SPEC-AUD-*, SPEC-AUDF-*, SPEC-CIN-* |
| `DEMO-GOV-001..010` | 10 | SPEC-AUTH-*, SPEC-VAL-*, SPEC-EVID-* |
| `DEMO-INP-001..015` | 15 | SPEC-CTL-*, SPEC-CMD-*, SPEC-UI-* |
| `DEMO-JRN-001..007` | 7 | SPEC-PIL-*, SPEC-PRD-*, SPEC-OUT-* |
| `DEMO-NAR-001..011` | 11 | SPEC-CANON-*, SPEC-CIN-*, SPEC-CAM-* |
| `DEMO-PERF-001..015` | 15 | SPEC-BUD-*, SPEC-PLAT-* |
| `DEMO-TUT-001..022` | 22 | SPEC-TUT-*, SPEC-LSN-* |
| `DEMO-UI-001..013` | 13 | SPEC-HUD-*, SPEC-UI-* |
| `DEMO-VAL-001..017` | 17 | SPEC-VAL-*, SPEC-EVID-*, SPEC-PLAT-* |
| `DEMO-VIS-001..013` | 13 | SPEC-ART-*, SPEC-VISD-* |
| `REL-ACC-001..017` | 17 | SPEC-ACC-* |
| `REL-AI-001..036` | 36 | SPEC-AI-*, SPEC-AIST-*, SPEC-DOC-*, SPEC-DIF-* |
| `REL-ART-001..023` | 23 | SPEC-ART-*, SPEC-VISD-* |
| `REL-AUD-001..015` | 15 | SPEC-AUD-*, SPEC-AUDF-* |
| `REL-BLD-001..020` | 20 | SPEC-BLD-*, SPEC-STR-* |
| `REL-CAM-001..032` | 32 | SPEC-CAM-*, SPEC-MSN-*, SPEC-PLAN-*, SPEC-END-* |
| `REL-CIN-001..008` | 8 | SPEC-CIN-* |
| `REL-CMB-001..027` | 27 | SPEC-CMB-*, SPEC-CMD-*, SPEC-STANCE-* |
| `REL-DIST-001..017` | 17 | SPEC-PLAT-*, SPEC-ARC-* |
| `REL-ECO-001..017` | 17 | SPEC-ECO-*, SPEC-RES-* |
| `REL-FAC-001..029` | 29 | SPEC-FACID-*, SPEC-UNIT-* |
| `REL-FTU-001..012` | 12 | SPEC-PIL-*, SPEC-TUT-* |
| `REL-GOV-001..015` | 15 | SPEC-AUTH-*, SPEC-VAL-* |
| `REL-LOC-001..006` | 6 | SPEC-LOC-* |
| `REL-MP-001..017` | 17 | SPEC-PRD-* (excluded scope) |
| `REL-PERF-001..025` | 25 | SPEC-BUD-*, SPEC-PLAT-* |
| `REL-PORT-001..010` | 10 | SPEC-PLAT-* |
| `REL-PUB-001..015` | 15 | — none (spec gap) |
| `REL-QA-001..036` | 36 | SPEC-VAL-*, SPEC-EVID-* |
| `REL-QOL-001..012` | 12 | SPEC-SAV-* |
| `REL-SAV-001..014` | 14 | SPEC-SAV-* |
| `REL-SEC-001..006` | 6 | SPEC-PLAT-*, SPEC-ARC-* |
| `REL-SIM-001..019` | 19 | SPEC-SIM-*, SPEC-OUT-* |
| `REL-STAB-001..005` | 5 | SPEC-BUD-*, SPEC-PLAT-* |
| `REL-UI-001..024` | 24 | SPEC-UI-*, SPEC-HUD-*, SPEC-CTL-* |
| `REL-WEL-001..018` | 18 | SPEC-WEL-*, SPEC-WELLP-* |

**Spec gaps.** `REL-PUB-*` (15 records) have no `SPEC-*` parent. The specification defines no public-website, manual, claims, or support
requirements, so those release records stand on their own authority until the specification is extended.


---

## Part I — Product Definition & Core Specifications (`SPEC-*`)

### 1. Authority, Interpretation, and Change Control
* **SPEC-AUTH-001 — Single Source of Truth:** This document defines the complete intended player experience, deterministic rules, content boundaries, and validation criteria. No team or agent may invent missing behaviors silently [10].
* **SPEC-AUTH-002 — Normative Language:** *Shall* implies a non-negotiable mandatory structural requirement. *May* implies an authorized design variation [10].
* **SPEC-AUTH-003 — No Silent Invention:** Where functionality or definitions are absent, work halts at that decision boundary until explicitly authorized [10].
* **SPEC-AUTH-004 — Traceable Change:** Every approved adjustment edits this master document inline and cascades changes directly into dependent code, data schemas, and tests [10].

### 2. Core Visual-Auditory Infrastructure
* **SPEC-ART-004 — Silhouette-First Visual Contrast Architecture:** Viewport rendering shall guarantee complete tactical legibility at default camera configurations (3800 uu arm length, isometric tilt). Faction-specific accent emissive regions (Meridian Cyan, Kharuun Amber, Choir Magenta) shall occupy no more than 15% of any asset's visible onscreen surface area [6]. Underlying terrain materials (`M_EchoesWorldSurface`) shall maintain a strict matte roughness floor of 0.85 and a luma wash of 152.6 ±5 linear points to eliminate specular glint competition with units [6].
* **SPEC-AUD-004 — Neural Performance Performance-Direction & Mastering:** Character audio lines shall be synthesized locally at 48 kHz PCM mono using the **Kokoro-82M** engine [3]. Dialog streams must normalize tightly to a target integrated loudness of −16 LUFS ±0.5 LU with true peaks capping at ≤ −1 dBTP [3]. The side-chain submix architecture must immediately duck active music by −6 dB and environment ambience by −4 dB within 150 ms of vocal line initialization [3].

---

## Part II — Release Specifications (`REL-*`)

### §9 Economy and Logistics
* **REL-ECO-015 — Automated Gather-Delivery Pathing Invariant:** Assigning an active worker unit (`Surveyor`, `Tender`, `Threadkeeper`) to a legal `Matter` deposit shall instantly initialize a continuous, closed-loop state machine: *Move to Deposit → Harvest Resource over 20 ticks → Calculate Optimal Dropoff → Travel Passable Ground → Deliver Cargo → Loop* [10]. No manual click-management shall be required to maintain basic economic throughput.
  * **REL-ECO-015.PRE:** Worker unit selected; valid target node containing available Matter; at least one operational friendly Drop-off structure exists [10].
  * **REL-ECO-015.ACT:** Worker targets Matter node using contextual right-click interface.
  * **REL-ECO-015.FAIL:** If the targeted drop-off or path is fully occluded by changing map geometry, the worker unit shall halt, flush its resource cargo reservation, cast an immediate spatial alert (`[DROP-OFF LOST]` or `[ROUTE SEVERED]`), and transfer to the local `Idle Worker` registry [10].
* **REL-ECO-016 — Dawn Ledger Reservation Invariant:** Transactions requiring the consumption of `Dawn` assets shall validate the player’s liquid asset balance before initiating cost commitment [10]. The simulation engine shall fail closed and reject any order attempting to drop net balances below zero.
  * **REL-ECO-016.FAIL:** Deficits instantly halt queues, return an error code string `[INSUFFICIENT DAWN]` to the HUD command card, and flash the missing balance requirements in high-contrast red type [10].
* **REL-ECO-017 — Resource Monitor Telemetry Schema:** The core user interface framework shall render a persistent, real-time data table measuring trailing 30-second and 60-second net resource generation curves [10]. It shall dynamically track individual worker unit states (`Gathering`, `Delivering`, `Constructing`, `Idle`) and output real-time deposit exhaustion countdown metrics based on historical consumption velocities [10].
  * **REL-ECO-017.ACC:** The telemetry layer shall support toggle mapping to an ultra-high-contrast view utilizing `Space Grotesk` fonts with a 30% spatial boundary safety margin for layout text expansion [6, 7].

---

### §11 Selection, Movement, Commands, and Combat
* **REL-CMB-019 — Contextual Command Click Deconfliction:** Contextual right-click actions targeted at active screens shall parse under a distinct two-layer trace protocol. Layer 1 (`ECC_GameTraceChannel1 / EchoesEntityPick`) shall project hit boxes matching the unit’s visual mesh profile; Layer 2 (`ECC_Visibility`) shall evaluate underlying ground tiles [5]. 
  * **REL-CMB-019.AUTH:** This algorithm removes the critical bug where clicking a tiny unit base cylinder misses the collider and mistakenly converts a targeted action into a generic terrain `Move` command [5].
  * **REL-CMB-019.FAIL:** If an entity click misses its primary boundaries but intercepts a background ground plane, it shall evaluate if an entity silhouette is within a 12-pixel selection radius. If found, it executes the target order; if empty, it falls back to standard terrain pathing with no silent order corruption [5].
* **REL-CMB-020 — Formation Geometry & Cohesion Thresholds:** Mobile units grouped under a single command array shall assume explicit bounding geometries (`Box`, `Line`, `Wedge`) [10]. Group speeds shall normalize dynamically to match the slowest active member inside the cluster [10].
  * **REL-CMB-020.FAIL:** If pathfinding constraints force a group through narrow chokes, the configuration shall break layout alignment into single-file tracking to prioritize continuous forward throughput. Units shall independently resume their primary formation layout within 40 ticks of reaching open space [10].

---

### §17 UI and Interaction
* **REL-UI-017 — Multi-Unit Subgroup Navigation Protocol:** Selection boxes encompassing mixed unit classes shall compile a compound array card [10]. The layout shall expose unit types sorted hierarchically by combat weight. Pressing `Tab` or `Shift+Tab` shall toggle active command card priority through sub-selections without wiping out the primary multi-unit selection group [10].
* **REL-UI-023 — Exclusivity Gating of Critical Keybind Inputs:** Keyboards maps shall enforce strict operational isolation between active state layers [5]. The `Space` shortcut key shall parse as a dual-mode context-switched route:
  1. If the keyboard-targeting reticle layer is active, `Space` confirms and executes the targeted action line [5].
  2. Under standard view modes, `Space` jumps the camera center to the screen position of the newest tactical alert [5].
  * **REL-UI-023.AUTH:** Removes duplicate key conflicts (e.g., `C` sharing `ChoosePreserve` and `ContinueCampaign`), ensuring clean single-key predictability [5].

---

## Part III — Atomic Acceptance Card Template Blueprint

Every requirement row in active phase development must populate this data-contract verification layout before moving from `IN PROGRESS` to `AGENT VERIFIED` [5]:

### [Acceptance Card: REL-CMB-019.SIG — Contextual Selection Pick]
* **.PRE (Preconditions):** Packaged `arm64` shipping configuration build running on baseline Mac local hardware [5, 7]. Active selection contains exactly 3 Meridian Lancer units [10]. Camera is tracking at standard RTS distance zoom heights [7].
* **.ACT (Actions):** Issue a right-click targeted command directly at the top 15% visual edge of a moving Kharuun Riftstalker silhouette [5, 10]. Intentionally mis-click 5 pixels off the visual mesh boundary into open vitrified ground [5].
* **.AUTH (Authoritative Results):** Layer 1 Pick trace intercept validates target entity pointer ID successfully; issues explicit combat order line [5]. Layer 2 click fallback algorithm catches the proximity vector; confirms target entity instead of triggering standard terrain Move [5].
* **.VIS (Visual Presentation):** A targeted cyan targeting ring wraps the enemy silhouette instantly upon order entry [5, 6].
* **.AUD (Audio Feedback):** Meridian Operations Annunciator fires the brief acknowledgment cue via the interface audio submix bus [3].
* **.FAIL (Failure Behavior):** Clicking empty ground outside the 12px fallback boundary safely executes a standard Move; tracking sigils display on the ground mesh surface [5].
* **.PERF (Performance):** Projection matrix math checks remain under ≤0.5ms on the core loop thread.
* **.AUTO (Automation Test):** Verification suite runs exact projection matrix math inside `EchoesPlayerControllerTest.cpp`; asserts zero thread leaks [5].
* **.PKG (Packaged Test):** Physical interaction testing verified clean across the full `REL-UI-013` screen resolution profile matrix [5].
* **.HUM (Human Usability):** Testing shows 5/5 naive users execute targeted focus-firing patterns on moving actors without click frustration [5, 8].


## 1. Authority, interpretation, and change control

* **SPEC-AUTH-001 —** Single source of truth. This document owns the complete intended player experience, game rules, content boundaries, system contracts, and acceptance criteria for the first commercial release.
* **SPEC-AUTH-002 —** Normative language. Shall means mandatory. May means permitted. Tuning baseline means a binding starting value that may change only through documented balance review without changing the element's purpose or counterplay.
* **SPEC-AUTH-003 —** No silent invention. When a required behavior is genuinely absent or contradictory, work stops at that decision boundary. The proposed change must identify affected requirements, player impact, dependencies, migration, tests, and owner approval.
* **SPEC-AUTH-004 —** Traceable change. Every approved design change updates this file in place, preserves stable requirement identifiers where meaning remains, records rationale, and updates dependent content, UI, AI, saves, replays, tests, and player documentation.
* **SPEC-AUTH-005 —** Purpose rule. Every accessible unit, building, technology, resource, terrain class, objective, command, alert, map interaction, and narrative beat must have a documented player purpose, cost or tradeoff, observable effect, failure behavior, and counterplay. Duplicates are differentiated, combined, or removed.
* **SPEC-AUTH-006 —** Truthful acceptance. A design requirement, automated test, visual inspection, physical play session, and player-comprehension result establish different facts. Release requires the full acceptance set named in sections 29–31.

| BUILD INSTRUCTION<br>Teams and AI agents build against this document from foundation to release. They do not treat source-code behavior, temporary assets, or convenient implementation shortcuts as authority when those conflict with the requirements. |
|---|


## 2. Product definition and release boundary


| ID | Decision | Binding definition |
|---|---|---|
| SPEC-PRD-001 | Genre | Premium 3D science-fantasy real-time strategy game. |
| SPEC-PRD-002 | Camera and control | Isometric battlefield camera; mouse-and-keyboard first; real-time unit, economy, base, terrain, and information control. |
| SPEC-PRD-003 | Primary experience | Single-player campaign and configurable 1v1 skirmish against fair AI. |
| SPEC-PRD-004 | Player fantasy | Command a civilization under consequence: build a force, interpret incomplete information, choose which future becomes real, and live with the spatial and human cost. |
| SPEC-PRD-005 | Release platform | macOS on Apple Silicon. The architecture remains portable, but other operating systems are not release deliverables. |
| SPEC-PRD-006 | Campaign | Fifteen operations across three acts with four earned endings. |
| SPEC-PRD-007 | Skirmish | Three maps; any of three factions versus any faction, including mirror matchups; five AI doctrines; four difficulty levels; Corefall victory. |
| SPEC-PRD-008 | Launch language | English voice and text; all player-facing strings externalized for later localization. |
| SPEC-PRD-009 | Expected match length | 20–35 minutes on Standard. Campaign operations target 20–45 minutes according to mission scale. |
| SPEC-PRD-010 | Business model | Complete premium game. No advertising, loot boxes, premium currency, battle pass, gacha, or pay-to-win progression. |


### 2.1 Included systems

* All three playable factions and the exact twelve-unit/twelve-structure launch roster defined here.
* Matter, Dawn, Logistics, construction, production, research, combat, fog, reconnaissance, terrain interaction, Future Wells, saves, replays, tutorial, accessibility, campaign persistence, and post-match reporting.
* Professional original environments, models, animation, effects, interface, music, ambience, sound effects, character voice, in-engine cinematics, subtitles, manual, support information, signing, notarization, and clean-machine installation.

### 2.2 Explicit exclusions

* Multiplayer, matchmaking, accounts, social features, teams, free-for-all, cooperative campaign, and live-service systems.
* Combat aircraft, air transports, air pathfinding, altitude combat, naval units, water combat, and amphibious systems.
* Unrestricted burrowing. Kharuun subsurface movement is limited to visible map-authored entrance/exit passages.
* Random weapon accuracy, random critical hits, hidden damage modifiers, and undisclosed AI information or resource advantages.
* Skirmish heroes, persistent hero leveling, garrisons, walls, gates, structure capture, unit conversion, and Command Core reconstruction.
* Cloud saves, achievements, controller support, user-generated content, mod tools, and alternate standard-match victory conditions.

## 3. Creative canon, setting, and lore

* **SPEC-CAN-001 —** Central theme. Echoes of the Broken Sun is about the cost of making one future real. Strategy, narrative, economy, interface, art, and sound must repeatedly express the tension between immediate survival and futures that become impossible.
* **SPEC-CAN-002 —** Tone. The tone is urgent, humane, and occasionally dry. No faction is a proxy for good or evil. Characters act from incomplete evidence and defensible needs. Painful choices remain understandable rather than becoming morality quizzes.

### 3.1 Soryn and the Crownfall


Soryn orbits a field of stellar fragments called the Crownfall. The breaking of the sun condensed unrealized causal branches into mineral-organic Dawnshards. A shard can power a city or expose a technology that never developed in the surviving timeline; consuming it also closes the possibility it contains. Future Wells are large deposits where several futures remain locally coherent.


Probability leakage appears as duplicated shadows, memories of streets never built, tools worn by absent hands, mineral-organic growth, and temporary contradictory geometry. The Hollow Choir consists of linked consciousness retained by erased branches. The Choir is not supernatural shorthand; it is a civilization struggling to remain coherent while several incompatible futures speak through it.


### 3.2 Historical frame


| ID | Era | Canon |
|---|---|---|
| SPEC-CANON-001 | Before Crownfall | Multiple city cultures and mineral-organic ecologies existed; later records are fragmentary and politically edited. |
| SPEC-CANON-002 | First Impact Generations | Ark-cities formed around surviving infrastructure; impact caverns became nurseries of early Kharuun assemblies. |
| SPEC-CANON-003 | Ledger Peace | Meridian city-states standardized Dawnshard accounting and mutual defense, preventing local collapse while concentrating extraction authority. |
| SPEC-CANON-004 | Quiet Omissions | Kharuun memory-bearers and Compact historians independently discover gaps: curated ancestral memories and census references to neighborhoods that no archive contains. |
| SPEC-CANON-005 | Present War | Unstable Wells threaten ark-city reserves and Kharuun birthing caverns. Both sides mobilize for survival; apparent anomalies become the first coherent Choir incursions. |


### 3.3 Cultures and language


| ID | Culture | Identity and language rules |
|---|---|---|
| SPEC-CANON-006 | Meridian Compact | A plural governance and logistics compact. Speech favors measured commitments: anchors, tolerances, ledgers, duty windows, and reserve margins. Civic ritual centers on maintaining inherited systems. |
| SPEC-CANON-007 | Kharuun Assemblies | A person is present consciousness, custodian of ancestral fragments, and participant in an imperfectly combined assembly. Names describe chosen relations. Humor arises from inherited certainty colliding with present evidence. |
| SPEC-CANON-008 | Hollow Choir | Members select stable speech from incompatible phrasings. Precision prevents one component future from dominating. Dialogue may overlap or resolve in more than one direction but must remain intelligible. |


### 3.4 Principal characters


| ID | Character | Role, motivation, and dramatic pressure |
|---|---|---|
| SPEC-CANON-009 | Commander Mara Vey | Meridian command authority and tutorial guide. She treats uncertainty as engineering debt that eventually kills people. Her strength is preparation; her danger is converting moral questions into control problems. |
| SPEC-CANON-010 | Talar Venn | Archive and civic witness whose immediate concern is the people and records that policy turns into abstractions. Talar challenges decisions with concrete human consequences. |
| SPEC-CANON-011 | Oruun-of-Seven-Stones | Kharuun memory-bearer carrying seven mutually correcting evacuation accounts. Oruun asks whether a society can remain accountable when continuity depends on curated forgetting. |
| SPEC-CANON-012 | Neme | Choir interlocutor and later command authority containing futures that disagree about coexistence. Neme's arc is negotiation among internal positions, not a secret dominant personality. |
| SPEC-CANON-013 | Chancellor Cael Rhyse | Meridian political architect who can point to cities saved by controlled extraction. His effort to restore one stable future is credible governance pursued to an existentially unacceptable conclusion. |
| SPEC-CANON-014 | Meridian Operations Annunciator | Operational system voice. It states class, location, urgency, and recovery information. It never comforts, moralizes, jokes, addresses the player as 'you,' or replaces character dialogue. |


### 3.5 Writing rules

* Characters speak from immediate needs, partial evidence, and distinct institutional histories.
* Exposition occurs through disagreement, action, evidence, or consequence; no villain explains the setting.
* No civilization speaks with one opinion, and strong belief does not make an interpretation automatically true.
* Humor comes from character and circumstance, never from undermining loss or turning lore into quips.
* Mission and result text states what happened and what remains unresolved. It does not invent population counts, consent, trust, moral correctness, or wider consequences that the game did not model.

## 4. Player experience pillars and gameplay loops


| ID | Pillar | Required player experience |
|---|---|---|
| SPEC-PIL-001 | Spatial economy | Resource choices change routes, exposure, drop-off value, vision, and timing. A resource is never only a number. |
| SPEC-PIL-002 | Asymmetric planning | Meridian establishes networks; Kharuun changes terrain and composition; Choir spends possibility and manages temporary coherence. |
| SPEC-PIL-003 | Readable consequence | The player can identify ownership, role, order, threat, cost, duration, terrain effect, and Well state at combat speed. |
| SPEC-PIL-004 | Fair uncertainty | Scouting matters because hidden information stays hidden from player and AI. Automation reduces repetition without becoming omniscience. |
| SPEC-PIL-005 | Recoverable command | Valid commands acknowledge immediately. Invalid commands explain why and how to recover. Context input never silently becomes a plausible wrong action. |
| SPEC-PIL-006 | Story through play | Objectives, terrain, resources, alerts, music, and consequences communicate the story while the player retains control. |


### 4.1 Time-scale loops


| ID | Scale | Loop | Player question |
|---|---|---|---|
| SPEC-PIL-007 | Seconds | Select → issue order → read acknowledgment → reposition/fire/ability → reassess. | What must move, fire, hold, retreat, or change state now? |
| SPEC-PIL-008 | Minutes | Gather → expand Logistics → scout → choose production/research → contest routes and Wells. | Where does the next advantage come from, and what does it cost elsewhere? |
| SPEC-PIL-009 | Match | Infer doctrine → counter visible composition → deny economy/information → create a timing → convert it into Core damage. | What is the opponent protecting, and when can that protection be broken? |
| SPEC-PIL-010 | Campaign | Learn a system → solve a distinct operation → record consequence → receive an authored later effect → qualify an ending. | Which future did this operation make possible, and which did it close? |


### 4.2 Strategic knowledge

* Which information is visible now, remembered from earlier, approximate, or unknown?
* Where can the economy safely grow, and which route or network link makes that growth possible?
* Should the Well be Harvested for tempo, Preserved for compounding control, or Reshaped for a temporary route?
* What visible composition and posture is the opponent using, and which soft counter can arrive in time?
* When is a retreat, raid, flank, defense, feint, or direct Corefall commitment stronger than another fight?
* How much Dawn remains after research, abilities, Well costs, and Choir charges already committed?

## 5. Match and operation outcomes

* **SPEC-OUT-001 —** Corefall victory. A player wins a standard skirmish when the opposing player has no surviving Command Core. Anchor, Memory Hearth, and Concordance are Command Cores. The match ends in the authoritative resolution window in which the final enemy Core is destroyed.
* **SPEC-OUT-002 —** Defeat. A player loses when the player's final Command Core is destroyed or the player confirms concession. Losing all workers, combat units, production, resources, or Wells is not defeat while the Core survives.
* **SPEC-OUT-003 —** Draw. If both final Command Cores are destroyed in the same 20 Hz resolution tick, the result is Draw. Score, resources, damage, command order, and elapsed time never break the tie.
* **SPEC-OUT-004 —** Campaign success. An operation is won only when every mandatory objective and required consequence commitment is complete. Destroying a hostile Core substitutes only where that operation explicitly names Corefall as an objective.
* **SPEC-OUT-005 —** Campaign failure. Each operation names failure predicates before play: required Core, character, civilian, witness, route, asset, timer, or irreversible contract. Ordinary unit loss is not a hidden failure.
* **SPEC-OUT-006 —** Result explanation. The result screen states the precise win/loss cause, optional objectives, rewards, irreversible record, elapsed time, resources, units, and Well decisions. A campaign replay cannot silently rewrite established history.
* **SPEC-OUT-007 —** Stalemate. At 45 minutes, skirmish warns that the match is prolonged but does not force a result. An AI with no recoverable production/economy/Core-defense path concedes. The player may continue or concede at any time.

## 6. Authoritative simulation and command rules

* **SPEC-SIM-001 —** Fixed time. Gameplay authority advances at 20 deterministic ticks per second. Presentation interpolates but never changes authoritative outcomes.
* **SPEC-SIM-002 —** Authority separation. Simulation owns entities, resources, commands, movement, terrain, fog, combat, objectives, AI inputs, outcomes, save state, and replay. Rendering, sound, animation, and UI consume authorized views and cannot create gameplay facts.
* **SPEC-SIM-003 —** Command validation. Every command is validated against ownership, visibility, target class, range, resources, Logistics, cooldown, state, footprint, path, and mode. Rejection returns a stable reason code and plain-language recovery.
* **SPEC-SIM-004 —** Determinism. The same initial state, content identifiers, player commands, and deterministic seed produce the same authoritative result across save/load and replay.
* **SPEC-SIM-005 —** Entity identity. Every authoritative entity has a stable identifier, faction, owner, class, role, health, position, footprint, movement domain, sight, orders, status effects, and lifecycle state.
* **SPEC-SIM-006 —** Limits. Entity and command limits fail visibly and safely. They never delete another entity, discard paid production, or corrupt a save to make room.
* **SPEC-SIM-007 —** Player time. UI expresses durations in seconds, with optional detailed tick values. One second equals 20 ticks.

### 6.1 Common commands


| ID | Command | Behavior | Failure examples |
|---|---|---|---|
| SPEC-CMD-001 | Move / Context | Move to ground or invoke the unambiguous legal action on a visible target. | No path; invalid terrain; target lost; ambiguous hit. |
| SPEC-CMD-002 | Direct Attack | Attack one visible valid hostile until destroyed, lost, invalid, or superseded. | Not hostile; not visible; cannot target class; no path/range. |
| SPEC-CMD-003 | Attack-move | Move toward a point, engage visible hostiles within stance leash, then resume. | No path; no attack-capable units. |
| SPEC-CMD-004 | Patrol | Repeat between waypoints and engage according to stance. | Invalid waypoint; no route. |
| SPEC-CMD-005 | Guard | Maintain radius around an owned or allied target and intercept legal threats. | Invalid target; guardee lost; no route. |
| SPEC-CMD-006 | Hold Position | Do not translate; acquire and fire on legal targets in range. | Unit cannot attack; state prevents firing. |
| SPEC-CMD-007 | Stop | Cancel reversible active orders and remain in place. Irreversible costs stay committed where stated. | Locked transition or non-cancelable consequence. |
| SPEC-CMD-008 | Ability | Preview and execute the selected named ability. | Cost, cooldown, target, state, range, terrain, connection, or mode invalid. |
| SPEC-CMD-009 | Rally | Set or clear the emergence destination for future units. | Destination permanently unreachable; producer inactive. |
| SPEC-CMD-010 | Interact | Use an authored mission/world interaction. | Wrong entity, phase, range, or prerequisite. |


## 7. Movement, pathfinding, formations, and terrain

* **SPEC-MOV-001 —** Ground-only domain. All controllable launch units use one surface-ground domain. There are no combat flying units, air transports, naval units, or unrestricted subterranean movement. Visual hovering does not grant different passability, collision, vision, or targeting; fixed subsurface passages are the sole exception and follow section 7.1.
* **SPEC-MOV-002 —** Pathfinding. Paths use deterministic destination-tile cost, known passability, unit footprint, reserved goals, dynamic obstruction, and bounded recalculation. Failure produces NO PATH, ROUTE BLOCKED, or DESTINATION OCCUPIED rather than wandering.
* **SPEC-MOV-003 —** Avoidance and body blocking. Enemy and neutral solid entities block movement. Allied mobile units use soft separation and yield rules; they may not permanently imprison one another. Large units reserve more clearance at chokes.
* **SPEC-MOV-004 —** Route change. When terrain changes under a route, units recalculate at the next tick. If no route remains, they stop at the last safe position, preserve their order as blocked, and alert the owner.
* **SPEC-MOV-005 —** Formations. Box provides compact travel, Line maximizes frontage perpendicular to movement, and Wedge points its apex toward travel. Units receive deterministic slots by footprint, speed, and stable identifier, regroup after obstacles, and abandon cohesion before accepting damage or blocking the path.

| ID | Terrain/object | Movement | Construction | Vision / fire | Gameplay meaning |
|---|---|---|---|---|---|
| SPEC-TER-001 | Open | 100% speed | Allowed on clear footprint | Does not block | Normal surface. |
| SPEC-TER-002 | Scarred | 85% speed | Not allowed until stabilized | Does not block | Visible Crownfall damage; route friction and lost build space. |
| SPEC-TER-003 | Blocked | Impassable | Not allowed | Blocks line of fire and sight only when marked as an occluding obstacle | Cliff, wall, deep fracture, or authored obstruction. |
| SPEC-TER-004 | Water / void | Impassable | Not allowed | Does not imply naval play | Presentation and map boundary. |
| SPEC-TER-005 | Mineral Cover | Impassable occupied tile | Temporary object | Blocks projectiles intersecting its volume; never grants hidden armor | Player-created temporary lane control. |
| SPEC-TER-006 | Subsurface Passage | Transit between fixed entrances | Entrance footprint fixed | No unit vision or attack in transit | Kharuun route shortcut with public counterplay. |


### 7.1 Subsurface passage contract

* Entrances and exits are visible, targetable map objects with public capacity and travel time.
* Eligible Kharuun units queue visibly; each entrance moves at most four units concurrently, with one unit entering every 10 ticks.
* Transit time is the authored passage length divided by 500 cm/s, rounded up to ticks, with a minimum of 60 ticks.
* Units have no attack, vision, ability, direction change, or targetability during transit; Resonants and Listening Spines show anonymous vibration along the passage.
* If the entrance is destroyed after entry, units continue to the exit. If the exit is destroyed or blocked, units wait up to 100 ticks, then return to the entrance; if both ends are gone, they emerge at the nearest authored fallback.

### 7.2 Environmental boundaries

* Elevation, weather, fog drift, pale tides, ambient flying creatures, distant vehicles, and decorative vegetation are presentation-only.
* Shivergrass is an information signal: it bends for a nearby possible footfall but never reveals identity or accepts a direct target.
* Vaultbacks are non-interactable ecology and cannot provide cover unless a specific mission promotes one to an authored objective object with full rules.
* Bridges and routes change only through a mission event or Future Well Reshape with telegraph, timer, fallback, fog, minimap, AI, and replay rules.
* Decorative rubble, cliffs, vegetation, shards, and architecture must match the authoritative collision and cover truth. Art cannot imply an interaction that rules do not provide.

## 8. Fog of war, intelligence, alerts, and reconnaissance


| ID | Information state | What the player may know |
|---|---|---|
| SPEC-INFO-001 | Unexplored | No terrain, resource, route, unit, structure, or event detail except public mission markers. |
| SPEC-INFO-002 | Explored | Remembered terrain and last observed permanent objects; no live unit or temporary terrain state. |
| SPEC-INFO-003 | Visible | Live authorized state of terrain and visible entities, limited to fields the rules expose. |
| SPEC-INFO-004 | Last known | Optional timestamped position and class from earlier direct vision; fades after 600 ticks and is never targetable. |
| SPEC-INFO-005 | Anonymous vibration | Approximate moving contact with age and uncertainty; no unit identity, exact location, ownership certainty, or direct target. |
| SPEC-INFO-006 | Public event | Telegraphed Well, mission, structure-collapse, or route event visible to all affected players as explicitly authored. |

* **SPEC-FOG-001 —** Single information boundary. World rendering, terrain, minimap, targeting, alerts, AI, audio, and effects consume the same player-scoped information view. No subsystem may disclose hidden live state by reading the full simulation.
* **SPEC-FOG-002 —** Alert anatomy. An alert has class, urgency, short text, world location when legitimate, timestamp, source, acknowledgment state, and recovery action. Critical alerts use sound, text, shape, and minimap pulse; rate limiting never suppresses the only warning of a terminal threat.

### 8.1 Automatic scouting


| ID | Order | Player defines | Completion |
|---|---|---|---|
| SPEC-INFO-007 | Explore Area | Area boundary and contact policy | All reachable frontier inside the area explored, or no safe route. |
| SPEC-INFO-008 | Find Matter | Search area and return point | First new Matter deposit reported, area exhausted, or route unsafe. |
| SPEC-INFO-009 | Locate Hostiles | Area, hostile classes, and observation distance | First qualifying contact reported or area exhausted. |
| SPEC-INFO-010 | Screen Route | Guarded force, lead distance, and contact response | Force reaches destination, route is blocked, or scout is lost. |

* **SPEC-SCT-001 —** Eligible scouts. Relay Skiff, Resonant, and Afterimage may use every reconnaissance order. Other mobile combat units may Explore Area and Screen Route but do not gain specialist threat routing.
* **SPEC-SCT-002 —** Legal routing. Automation searches reachable unexplored frontier from player-known terrain. It never queries hidden enemies, resources, structures, routes, or Well state to choose a path.
* **SPEC-SCT-003 —** Policies. CAUTIOUS reports and returns on contact; OBSERVE maintains the player-set distance without attacking; PERSIST continues until health reaches the player-set threshold or a manual order overrides it.
* **SPEC-SCT-004 —** Player control. Selection shows mission, boundary, planned route, progress, discoveries, response policy, health threshold, and return point. Manual commands interrupt immediately; resume or return is explicit.
* **SPEC-SCT-005 —** Outcomes. Matter, Well, route, hostile unit, hostile structure, damage, blocked path, and scout loss generate distinct fair alerts. An exhausted order reports SEARCH COMPLETE, NO UNEXPLORED AREA, or NO SAFE ROUTE as applicable instead of wandering indefinitely.
* **SPEC-SCT-006 —** Authority limit. Reconnaissance never selects a Well protocol, gathers a newly found resource, attacks unless stance permits, spends resources, changes identity/adaptation, or commits a campaign decision.

## 9. Economy, resources, Logistics, and forecasting


| ID | Resource | Source | Uses | Strategic pressure |
|---|---|---|---|---|
| SPEC-RES-001 | Matter | Finite visible strata; reclaimed mission-specific wreckage | Workers, units, structures, repair, and research | Route length, saturation, drop-off position, depletion, protection, and harassment. |
| SPEC-RES-002 | Dawn | Starting reserve, Harvest, controlled Preserve income, and authored campaign rewards | Advanced units, research, Well Reshape, faction abilities, adaptation, identity, and Choir coherence | Every spend closes another timing or possibility; commitments must be forecast. |
| SPEC-RES-003 | Logistics | Command Core and faction infrastructure; temporary Compact Relay | Capacity for completed and queued units | Infrastructure loss blocks new production; temporary capacity creates visible expiry risk. |

* **SPEC-ECO-001 —** Starting resources. Skirmish presets are Scarce 250 Matter/18 Dawn, Standard 400/30, and Abundant 700/60. Both players receive the same preset and begin with one Command Core and five workers.
* **SPEC-ECO-002 —** Matter deposits. A standard deposit contains 1,500 Matter and supports three workers without queue delay. Additional workers may be assigned but wait visibly. Workers gather work-rate units over 20 ticks and carry up to their cargo capacity.
* **SPEC-ECO-003 —** Automatic gather cycle. A Gather order repeats source → gather → valid drop-off → deliver until depletion, invalid route/drop-off, explicit danger policy, or player override. The worker does not require manual delivery legs.
* **SPEC-ECO-004 —** Drop-off choice. Workers choose the assigned operational drop-off; if none is assigned, they choose the reachable drop-off with lowest predicted round-trip time. The player may lock an assignment. Loss triggers DROP-OFF LOST and requests reassignment.
* **SPEC-ECO-005 —** Depletion. A depleted deposit remains as a visible exhausted marker for 200 ticks, then becomes non-interactable remembered terrain. Assigned workers seek no new hidden deposit; they become idle and alert.
* **SPEC-ECO-006 —** Logistics loss. Units already completed remain controllable when capacity falls below use. New units and queued units that have not reserved capacity cannot begin. Reserved production remains reserved until completion or cancellation.

### 9.1 Resource monitor

* Current Matter and Dawn balances; separate Matter income and Dawn income realized over the previous 30 and 60 seconds.
* Workers total, idle, gathering, delivering, traveling, blocked, constructing, and repairing.
* Assignments, capacity, saturation, known remaining amount, travel time, projected income, and estimated depletion time for each known deposit.
* Operational and lost drop-offs, severed routes, and worker reassignment controls.
* Logistics used, durable, temporary, reserved, blocked, and expiring, including the Relay Skiff countdown.
* Production, construction, research, ability, Preserve, and Choir commitments; projected Dawn after each upcoming charge.
* Alerts at 30, 15, and 5 seconds before a forecasted capacity expiry or Choir insolvency, plus immediate alerts for route loss, idle workers, depletion, and blocked spending.
* Forecasts use only owned and legitimately observed data, identify their time window, and round conservatively rather than promising unavailable resources.

## 10. Construction, production, repair, and research

* **SPEC-BLD-001 —** Placement. Construction preview shows faction-specific name, cost, footprint, rotation, buildability, worker route, expected completion, network/Logistics effect, and whether the finished structure will operate. Buildings rotate in 90-degree increments only; facing changes no footprint except an explicitly directional weapon.
* **SPEC-BLD-002 —** Payment. Construction and unit costs are paid when the order enters its active slot. A queued item shows its uncommitted cost but does not reserve resources until activation. Research cost is paid at start and is never refunded.
* **SPEC-BLD-003 —** Builders. One worker builds at 100% rate. A second and third assist at 60% and 40%. More than three may repair or wait but add no construction speed. If all builders leave or die, progress pauses without decay.
* **SPEC-BLD-004 —** Incomplete structures. An incomplete structure is targetable, has health proportional to completion with a 10% minimum, provides no production/Logistics/power/vision beyond its construction site, and can be repaired only up to its completion-limited health.
* **SPEC-BLD-005 —** Cancellation. Cancel construction or unit production before 50% progress for a 75% Matter/Dawn refund; at or after 50%, refund 50%. Research interruption refunds nothing. The confirmation preview states the exact refund.
* **SPEC-BLD-006 —** Queues. Each producer has a five-unit queue and one active slot. Units may be reordered except the active item. A production structure may research instead of producing; one research project may be active per player.
* **SPEC-BLD-007 —** Emergence. A completed unit appears at the nearest free legal tile to the producer, then follows its rally route. If no tile is free for 100 ticks, production pauses at complete, retains the unit safely, and alerts SPAWN BLOCKED.
* **SPEC-BLD-008 —** Rally. The player may set one destination or a Shift-queued route. Rally paths may target terrain, a friendly guard target, or a valid resource for workers. If blocked, the unit stops at the last safe point and alerts.
* **SPEC-BLD-009 —** Base rules. Players may build multiple production, supply, utility, and drop-off structures, but no additional Command Core. Structures cannot be sold, captured, garrisoned, abandoned, or converted. They may be destroyed or canceled while incomplete.
* **SPEC-BLD-010 —** Repair. All workers repair allied completed units and structures using the worker-specific repair language defined in unit cards. Repair cannot exceed maximum health, resurrect, rewind, remove a status effect, or operate without Matter.

### 10.1 Technology model


Launch research is intentionally compact: two sequential faction technologies. Each creates a visible timing tradeoff because it consumes Dawn, Matter, and the producer's active slot. Effects apply immediately to all living eligible units and future eligible units. Research cannot be reversed.


## 11. Combat resolution, stances, and counterplay

* **SPEC-CMB-001 —** Deterministic direct hit. Weapons have no random accuracy or critical hits. A valid attack deals the stated damage when its deterministic projectile or contact resolves.
* **SPEC-CMB-002 —** Damage model. Launch combat has one damage class and no armor-class multipliers. Health, range, cadence, movement, vision, cover, state, and positioning create soft counters.
* **SPEC-CMB-003 —** Projectiles. Ranged attacks create deterministic projectiles traveling 1,200 cm/s. Damage resolves on impact. A projectile striking valid mineral or Bulwark cover resolves against that protection. It does not retarget after launch.
* **SPEC-CMB-004 —** Line of fire. A ray from muzzle to target checks authored occluders, temporary cover, and protected Bulwark geometry. Units do not block allied fire. Decorative art cannot block or permit a shot independently of the simulation.
* **SPEC-CMB-005 —** Friendly fire. Ordinary weapons and launch abilities do not damage allies or the firing unit. Area damage does not exist unless a later named ability defines it explicitly.
* **SPEC-CMB-006 —** Fire and movement. All units except Riftstalker Slipfire stop to fire. They rotate within the wind-up, attack, then may resume movement after recovery.

CMB-006A — Range and facing. Launch weapons have zero minimum range. Facing never changes accuracy or damage. A unit turns toward its target during the stated wind-up; if it cannot complete facing before resolution, the attack waits rather than firing backward.

* **SPEC-CMB-007 —** Acquisition. Default target priority is immediate threat to self/guardee, then lowest predicted time-to-disable within range, then nearest visible target, resolved by stable entity identifier. The player may focus any legal visible target.
* **SPEC-CMB-008 —** Overkill. Units with no explicit focus order avoid launching damage already predicted to exceed a target's remaining health by more than one attack. Focus-fire obeys the player even when it overkills.
* **SPEC-CMB-009 —** Death. At zero health, an entity loses authority immediately, plays faction/role-readable destruction, blocks no commands, and leaves non-interactable remains for 200 ticks. Wreckage yields Matter only in missions that explicitly mark it salvageable.
* **SPEC-CMB-010 —** Retreat. Retreat is ordinary movement, not a hidden morale state. No disengagement penalty exists. Wounded units receive visible health alerts and may be included in player-defined automatic retreat policy.
* **SPEC-CMB-011 —** No hidden systems. There is no suppression, morale, stun, knockback, capture, stealth, camouflage, resurrection, or regeneration unless an explicit named ability in this document supplies the complete rule.

### 11.1 Stances


| ID | Stance | Acquire | Pursuit | Ability use |
|---|---|---|---|---|
| SPEC-STANCE-001 | Aggressive | Any visible legal hostile within sight | Up to 900 cm from order path/anchor | Off unless separately toggled |
| SPEC-STANCE-002 | Defensive (default) | Threats within weapon range or attacking self/guardee | Up to 400 cm | Off |
| SPEC-STANCE-003 | Hold Position | Legal hostile in weapon range | None | Off |
| SPEC-STANCE-004 | Return Fire | Only a unit that damages self/guardee | Up to 250 cm | Off |
| SPEC-STANCE-005 | Hold Fire | None | None | Off |

* **SPEC-CMB-012 —** Automation. Automatic ability use is disabled by default. A player may enable an ability-specific toggle where offered; the toggle shows allowed targets, resource floor, and cancellation. Automation never spends the last 20 Dawn unless the player lowers that reserve.

### 11.2 Unit and worker automation

* **SPEC-AUT-001 —** Player-owned policy. Every automation is opt-in, visible on the selection card, constrained by player-set area, target class, health threshold, and resource reserve, and canceled immediately by a manual command. Automation executes policy; it never chooses technology, Well protocol, adaptation, identity, campaign consequence, or another irreversible strategic commitment.
* **SPEC-AUT-002 —** Worker auto-repair. Disabled by default. When enabled for a worker or control group, the worker may repair the nearest damaged eligible allied target inside a player-set radius only while the projected Matter balance remains above the player-set floor. It returns to its prior assignment when the target is full or invalid and reports when no legal repair remains.
* **SPEC-AUT-003 —** Worker flee. Disabled by default. The player selects a health threshold and owned safe point or operational drop-off. On hostile damage below that threshold, the worker stops gathering, building, or repairing and moves to the safe point using known passability; NO SAFE ROUTE leaves it stopped and alerts. The flee policy never discovers a hidden route or threat.
* **SPEC-AUT-004 —** Idle workers. A worker with no valid order remains idle, receives the idle-worker marker and alert, and may be selected through the idle-worker control. It does not independently choose a resource, construction, repair, or risk policy.
* **SPEC-AUT-005 —** Reinforcement policy. Each production structure may optionally assign completed combat units to one control group and send them along its rally route. The option is explicit, survives save/load, stops when the group is full or invalid, and never replaces an existing unit order after the unit joins.

## 12. Factions and strategic identities


| ID | Faction | Plan | Strength | Failure pressure |
|---|---|---|---|---|
| SPEC-FACID-001 | Meridian Compact | Extend a connected network, establish reliable firing positions, and convert information into precise pressure. | Infrastructure reliability, range, directional defense, temporary supply. | Network severance, flanks, forced movement, slow expansion. |
| SPEC-FACID-002 | Kharuun Assemblies | Change route geometry and force composition through migration, cover, adaptation, and vibration intelligence. | Mobility, terrain control, scouting, post-contact adaptation. | Root/molt exposure, expensive adaptation, weak prolonged frontal exchanges. |
| SPEC-FACID-003 | Hollow Choir | Commit units to Manifest or Possible, create fair uncertainty, and sustain temporary infrastructure through Dawn. | Flexible timing, route pressure, control, temporary possibility. | Public transitions, recurring costs, Dawn insolvency, forced commitment. |


### 12.1 Meridian Compact units


#### SPEC-UNIT-001 — Surveyor


| Field | Binding value |
|---|---|
| Role | Worker |
| Cost | 50 Matter / 0 Dawn |
| Health / speed / sight | 90 / 360 cm/s / 900 cm |
| Logistics / production | 1 / 60 ticks (3.0 s) |
| Worker | Work rate 10; cargo 10; no attack |


| Player question | Answer |
|---|---|
| Why build it? | Build and sustain the Compact economy and network. |
| What can it do? | Move, Gather, Deliver, Build, Repair, Future Well protocol, Guard, Stop. |
| Signature rule | Network Repair: channel on an allied Compact unit or structure within 200 cm; restore 10 health per second at 1 Matter per 10 health. Up to three workers may repair one target at 100%, 60%, and 40% efficiency. Damage interrupts for 1 second. |
| How should it be used? | Keep a short protected Matter route, extend Power Links deliberately, repair severed infrastructure, and approach Wells only after the route is scouted. |
| When does it not help? | No attack, low health, and high strategic value. It cannot repair enemies, temporary cover, or a structure that is being dismantled. |
| How does the opponent answer it? | Raid cargo routes, force workers off construction, and sever the network faster than Surveyors can restore it. |


#### SPEC-UNIT-002 — Lancer


| Field | Binding value |
|---|---|
| Role | Ranged Line |
| Cost | 85 Matter / 20 Dawn |
| Health / speed / sight | 145 / 320 cm/s / 1100 cm |
| Logistics / production | 2 / 100 ticks (5.0 s) |
| Attack | 18 damage, 650 cm, 30 ticks (1.50 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Deliver reliable ranged line damage. |
| What can it do? | All common combat orders and stances. |
| Signature rule | No activated ability. Its value comes from range, positioning, target choice, and protected firing time. |
| How should it be used? | Fight behind Bulwarks, use Skiff vision, focus exposed targets, and retreat before fast units close the distance. |
| When does it not help? | Moderate health and no close-combat escape. It must stop to fire. |
| How does the opponent answer it? | Flank it, break its screen, force repeated repositioning, or attack from more than one route. |


#### SPEC-UNIT-003 — Bulwark Team


| Field | Binding value |
|---|---|
| Role | Heavy Screen |
| Cost | 130 Matter / 25 Dawn |
| Health / speed / sight | 260 / 230 cm/s / 850 cm |
| Logistics / production | 3 / 140 ticks (7.0 s) |
| Attack | 10 damage, 300 cm, 24 ticks (1.20 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Create directional protection for a Compact firing line. |
| What can it do? | Common combat orders; Deploy Facing; Pack. |
| Signature rule | Deploy: after a 20-tick setup, project a 350 cm deep by 500 cm wide directional cover zone and reduce qualifying incoming damage by 40%. Movement falls to 35%. Packing takes 15 ticks. |
| How should it be used? | Anchor a choke, cover Lancers or a retreat, and rotate before the enemy completes a flank. |
| When does it not help? | Slow, short-ranged, and vulnerable from the sides and rear. Directional protection never becomes universal armor. |
| How does the opponent answer it? | Bypass, split pressure, displace, attack from behind, or force it to pack and redeploy. |


#### SPEC-UNIT-004 — Relay Skiff


| Field | Binding value |
|---|---|
| Role | Scout Support |
| Cost | 70 Matter / 20 Dawn |
| Health / speed / sight | 75 / 500 cm/s / 1500 cm |
| Logistics / production | 1 / 80 ticks (4.0 s) |
| Attack | 6 damage, 400 cm, 24 ticks (1.20 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Scout, screen routes, extend vision, and provide temporary Logistics. |
| What can it do? | Common combat orders; all reconnaissance orders; Extend Relay. |
| Signature rule | Extend Relay: while within 700 cm of a connected Compact node, grant 4 temporary Logistics for 400 ticks; 800-tick cooldown. Expiration blocks new production but never kills fielded units. |
| How should it be used? | Find expansions and enemy approaches, screen a moving force, and bridge short production timing windows. |
| When does it not help? | Fragile and weak in combat. It visually hovers but uses the ground movement layer and cannot cross blocked terrain or water. |
| How does the opponent answer it? | Destroy or drive it away, sever its network connection, and time pressure around the visible capacity-expiration warning. |


### 12.2 Kharuun Assemblies units


#### SPEC-UNIT-005 — Tender


| Field | Binding value |
|---|---|
| Role | Worker |
| Cost | 50 Matter / 0 Dawn |
| Health / speed / sight | 100 / 390 cm/s / 920 cm |
| Logistics / production | 1 / 60 ticks (3.0 s) |
| Worker | Work rate 9; cargo 10; no attack |


| Player question | Answer |
|---|---|
| Why build it? | Grow Kharuun economy, structures, and usable terrain. |
| What can it do? | Move, Gather, Deliver, Build, Repair, Stabilize Scar, Future Well protocol, Guard, Stop. |
| Signature rule | Stabilize Scar: spend 15 Dawn and channel for 120 ticks on one Scarred tile within 300 cm, converting it to Open. The tile must be empty and outside an active Well manifestation. Interruption refunds no Dawn after channel start. |
| How should it be used? | Move drop-off capacity with Waystones, convert an important build tile, and support post-contact migration. |
| When does it not help? | No attack. Stabilization is slow, visible, and too expensive for indiscriminate terrain clearing. |
| How does the opponent answer it? | Contest the channel, deny the intended footprint, and punish exposed Tenders during migration. |


#### SPEC-UNIT-006 — Riftstalker


| Field | Binding value |
|---|---|
| Role | Mobile Skirmisher |
| Cost | 75 Matter / 30 Dawn |
| Health / speed / sight | 125 / 410 cm/s / 1050 cm |
| Logistics / production | 2 / 100 ticks (5.0 s) |
| Attack | 14 damage, 500 cm, 22 ticks (1.10 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Take short mobile trades and pressure exposed routes. |
| What can it do? | All common combat orders and stances; eligible fixed-passage transit. |
| Signature rule | Slipfire: may fire while moving at 75% normal damage and a 125% cooldown. Stopping restores normal weapon performance. |
| How should it be used? | Probe, kite slower units, raid workers, escort a Resonant, and disengage before a frontal fight becomes prolonged. |
| When does it not help? | Low staying power and reduced moving-fire efficiency. |
| How does the opponent answer it? | Corner it with route control, use long-range fire, screen workers, or force it to fight inside a defended position. |


#### SPEC-UNIT-007 — Cairnback


| Field | Binding value |
|---|---|
| Role | Assault Screen |
| Cost | 120 Matter / 30 Dawn |
| Health / speed / sight | 245 / 270 cm/s / 800 cm |
| Logistics / production | 3 / 140 ticks (7.0 s) |
| Attack | 16 damage, 200 cm, 28 ticks (1.40 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Absorb pressure and reshape a firing lane with temporary mineral cover. |
| What can it do? | All common combat orders and stances; Raise Mineral Cover; Warform Adaptation. |
| Signature rule | Raise Mineral Cover: 15 Dawn, 450 cm range, 180 health, 300-tick duration, 600-tick cooldown. The cover occupies one tile, blocks ground movement and line of fire, and can be destroyed by either player. |
| How should it be used? | Break a firing lane, protect a retreat, divide a choke, or give skirmishers time to reposition. |
| When does it not help? | Slow, short-ranged, and vulnerable to being bypassed. Cover can obstruct allies. |
| How does the opponent answer it? | Destroy the cover, attack another lane, wait out the duration, or force the Cairnback to spend Dawn defensively. |


#### SPEC-UNIT-008 — Resonant


| Field | Binding value |
|---|---|
| Role | Scout Counter Scout |
| Cost | 80 Matter / 25 Dawn |
| Health / speed / sight | 85 / 470 cm/s / 1550 cm |
| Logistics / production | 1 / 80 ticks (4.0 s) |
| Attack | 8 damage, 380 cm, 20 ticks (1.00 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Scout and detect moving threats without granting perfect information. |
| What can it do? | Common combat orders; all reconnaissance orders; eligible fixed-passage transit; Warform Adaptation. |
| Signature rule | Passive vibration sense: moving enemy signatures within 2,200 cm appear for 40 ticks at 200 cm positional resolution. Contacts have no identity and cannot be directly targeted. |
| How should it be used? | Watch approaches, identify route pressure, verify safe migration, and distinguish movement from silence. |
| When does it not help? | Fragile; stationary enemies produce no vibration contact; approximate contacts are not vision. |
| How does the opponent answer it? | Stop outside direct vision, use decoys or split movement, destroy the Resonant, or approach through multiple intervals. |


### 12.3 Hollow Choir units


#### SPEC-UNIT-009 — Threadkeeper


| Field | Binding value |
|---|---|
| Role | Worker |
| Cost | 55 Matter / 5 Dawn |
| Health / speed / sight | 80 / 380 cm/s / 1000 cm |
| Logistics / production | 1 / 65 ticks (3.2 s) |
| Worker | Work rate 9; cargo 12; no attack |


| Player question | Answer |
|---|---|
| Why build it? | Build the Choir economy while forecasting coherence obligations. |
| What can it do? | Move, Gather, Deliver, Build, Repair, Future Well protocol, Guard, Stop. |
| Signature rule | Reconcile Structure: standard worker repair rules, but the command surface also shows the target's next coherence charge and projected Dawn after repair. |
| How should it be used? | Maintain a compact affordable structure field and avoid creating more recurring obligations than Dawn can sustain. |
| When does it not help? | No attack, includes a Dawn production cost, and cannot waive coherence charges. |
| How does the opponent answer it? | Pressure several Choir structures before the next charge, attack Threadkeepers, and force a choice between repair and upkeep. |


#### SPEC-UNIT-010 — Intervalist


| Field | Binding value |
|---|---|
| Role | Phase Skirmisher |
| Cost | 80 Matter / 35 Dawn |
| Health / speed / sight | 115 / 350 cm/s / 1150 cm |
| Logistics / production | 2 / 100 ticks (5.0 s) |
| Attack | 16 damage, 550 cm, 25 ticks (1.25 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Serve as the Choir's flexible line skirmisher. |
| What can it do? | All common combat orders and stances; Reconcile Identity. |
| Signature rule | Reconcile Identity: 20 Dawn; transition for 160 ticks; then Manifest grants 130% damage or Possible grants 130% movement and 125% vision. A new change waits through the transition and a 400-tick cooldown. |
| How should it be used? | Select Manifest for a committed damage window or Possible for scouting, pursuit, and escape. Change before the engagement, not after the outcome is obvious. |
| When does it not help? | The transition is public and expensive. One state cannot retain the other's bonus. |
| How does the opponent answer it? | Disengage from Manifest, pressure during transition, trap Possible units away from damage support, and tax Dawn. |


#### SPEC-UNIT-011 — Lacuna Warden


| Field | Binding value |
|---|---|
| Role | Recovery Controller |
| Cost | 140 Matter / 45 Dawn |
| Health / speed / sight | 230 / 260 cm/s / 900 cm |
| Logistics / production | 3 / 150 ticks (7.5 s) |
| Attack | 15 damage, 400 cm, 30 ticks (1.50 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Hold the Choir center and control an enemy's retreat or pursuit. |
| What can it do? | All common combat orders and stances; Reconcile Identity; Bind Interval. |
| Signature rule | Bind Interval: 25 Dawn, 500 cm range, 80-tick duration, 500-tick cooldown. One visible enemy is slowed by 35% and cannot activate abilities. The effect breaks if the Warden dies, moves more than 700 cm from the target, or loses vision for 20 ticks. |
| How should it be used? | Protect a retreat, prevent a key ability, or hold a target inside a Manifest damage window. |
| When does it not help? | Slow and expensive; no healing, resurrection, rewind, or hidden-information recovery. |
| How does the opponent answer it? | Break vision, focus the Warden, move beyond its tether, or bait the cooldown with a lower-value target. |


#### SPEC-UNIT-012 — Afterimage


| Field | Binding value |
|---|---|
| Role | Misdirection Support |
| Cost | 75 Matter / 35 Dawn |
| Health / speed / sight | 70 / 520 cm/s / 1600 cm |
| Logistics / production | 1 / 85 ticks (4.2 s) |
| Attack | 7 damage, 420 cm, 22 ticks (1.10 s) cooldown |


| Player question | Answer |
|---|---|
| Why build it? | Scout, pressure routes, and create fair tactical misdirection. |
| What can it do? | Common combat orders; all reconnaissance orders; Reconcile Identity; Forked Trace. |
| Signature rule | Forked Trace: 15 Dawn, 120-tick duration, 500-tick cooldown. Create two player-directed projection paths. Each projection has 1 health, no attack, no collision, and no vision. It appears as an uncertain contact under fog and is labeled PROJECTION when directly seen. |
| How should it be used? | Test reactions, disguise the direction of a real scout, split sensor attention, or screen a retreat. |
| When does it not help? | A projection cannot capture, scout, deal damage, block movement, or preserve hidden information. |
| How does the opponent answer it? | Use direct vision, destroy the 1-health projection, wait for expiry, and verify with multiple information sources. |


### 12.4 Kharuun Warform Adaptation


An eligible Riftstalker, Cairnback, or Resonant within 600 cm of a completed Growth Basin may spend 25 Dawn and molt for 80 ticks while taking 150% damage. Carapace sets health to 135% and movement to 80%; Striker sets damage to 125% and cooldown to 85%. Adaptation replaces any prior adaptation. It is public, interruptible by death, and never automatic.


### 12.5 Choir Identity Reconciliation


Intervalist, Lacuna Warden, and Afterimage may spend 20 Dawn to enter a 160-tick public transition. Manifest grants 130% damage. Possible grants 130% movement and 125% vision. The next change becomes legal only after transition plus a 400-tick cooldown. Transition does not teleport, duplicate, heal, rewind, or restore hidden information.


## 13. Buildings and base-management actions


### 13.1 Meridian Compact structures


#### SPEC-STR-001 — Anchor


| Field | Binding value |
|---|---|
| Role | Headquarters Dropoff |
| Cost | 0 Matter / 0 Dawn |
| Health / sight | 1400 / 800 cm |
| Construction / footprint | 400 ticks (20.0 s) / 5×5 tiles |
| Logistics | 12 |
| Special | None |


| Player question | Answer |
|---|---|
| Purpose | Compact Command Core, worker producer, network root, and Matter drop-off. |
| Selection options | Produce Surveyor; set rally; inspect economy/network; repair. |
| How to use | Protect it because its loss is defeat. It cannot be rebuilt or replaced in standard play. |
| Counterplay | Attack from several routes, sever outward links, and force the Compact to defend its center. |


#### SPEC-STR-002 — Power Link


| Field | Binding value |
|---|---|
| Role | Supply Node |
| Cost | 90 Matter / 10 Dawn |
| Health / sight | 450 / 500 cm |
| Construction / footprint | 100 ticks (5.0 s) / 2×2 tiles |
| Logistics | 6 |
| Special | None |


| Player question | Answer |
|---|---|
| Purpose | Extend the Compact network, Matter drop-off coverage, and Logistics. |
| Selection options | Inspect connection; set worker drop-off priority; dismantle is not available. |
| How to use | Place links with overlap and defensible spacing; one link should not expose the whole chain. |
| Counterplay | Sever narrow links and attack the isolated systems they supported. |


#### SPEC-STR-003 — Array Foundry


| Field | Binding value |
|---|---|
| Role | Production |
| Cost | 180 Matter / 30 Dawn |
| Health / sight | 760 / 500 cm |
| Construction / footprint | 160 ticks (8.0 s) / 4×4 tiles |
| Logistics | 0 |
| Special | None |


| Player question | Answer |
|---|---|
| Purpose | Produce all Compact combat units and research Compact technology. |
| Selection options | Queue up to five units; research one technology; reorder/cancel units; set rally. |
| How to use | Add production only when resources and Logistics can sustain it; research consumes its active slot. |
| Counterplay | Raid it during research, block emergence, or force production away from the needed counter. |


#### SPEC-STR-004 — Aegis Post


| Field | Binding value |
|---|---|
| Role | Defense |
| Cost | 130 Matter / 30 Dawn |
| Health / sight | 520 / 700 cm |
| Construction / footprint | 120 ticks (6.0 s) / 2×2 tiles |
| Logistics | 0 |
| Special | Powered attack: 28 damage, 900 cm, 20 ticks; connection 800 cm. |


| Player question | Answer |
|---|---|
| Purpose | Provide powered automatic area defense. |
| Selection options | Inspect network source, power state, range, target priority, and hold-fire toggle. |
| How to use | Cover an economy route or network junction; combine with units rather than relying on it alone. |
| Counterplay | Sever power, attack beyond its arc/range, overwhelm another route, or focus it with a screened force. |


### 13.2 Kharuun Assemblies structures


#### SPEC-STR-005 — Memory Hearth


| Field | Binding value |
|---|---|
| Role | Headquarters Dropoff |
| Cost | 0 Matter / 0 Dawn |
| Health / sight | 1300 / 800 cm |
| Construction / footprint | 400 ticks (20.0 s) / 5×5 tiles |
| Logistics | 12 |
| Special | None |


| Player question | Answer |
|---|---|
| Purpose | Kharuun Command Core, worker producer, adaptation root, and Matter drop-off. |
| Selection options | Produce Tender; set rally; inspect economy and migration network. |
| How to use | Protect it because its loss is defeat. It cannot be rebuilt in standard play. |
| Counterplay | Draw mobile forces away, deny rooted Waystones, then pressure the Hearth. |


#### SPEC-STR-006 — Waystone


| Field | Binding value |
|---|---|
| Role | Mobile Supply Node |
| Cost | 80 Matter / 20 Dawn |
| Health / sight | 390 / 500 cm |
| Construction / footprint | 100 ticks (5.0 s) / 2×2 tiles |
| Logistics | 5 |
| Special | Uproot 40 ticks; move 120 cm/s at 125% damage; root 60 ticks. |


| Player question | Answer |
|---|---|
| Purpose | Mobile Logistics and Matter drop-off node. |
| Selection options | Root, Uproot, move while uprooted, set drop-off priority, inspect affected capacity. |
| How to use | Root after scouting, then migrate when the value of a new route exceeds the exposure window. |
| Counterplay | Attack during uproot/root, force capacity loss, occupy the intended footprint, or harass the new route. |


#### SPEC-STR-007 — Growth Basin


| Field | Binding value |
|---|---|
| Role | Production |
| Cost | 165 Matter / 35 Dawn |
| Health / sight | 700 / 500 cm |
| Construction / footprint | 160 ticks (8.0 s) / 4×4 tiles |
| Logistics | 0 |
| Special | Adaptation site within 600 cm; see Warform Adaptation. |


| Player question | Answer |
|---|---|
| Purpose | Produce Kharuun combat units, research, and enable Warform Adaptation. |
| Selection options | Queue/reorder/cancel units; research; set rally; inspect nearby eligible units and molt risk. |
| How to use | Place where returning units can adapt without exposing the entire economy. |
| Counterplay | Attack during research or molt, deny the 600 cm site, and force repeated expensive adaptations. |


#### SPEC-STR-008 — Listening Spine


| Field | Binding value |
|---|---|
| Role | Detection |
| Cost | 115 Matter / 25 Dawn |
| Health / sight | 440 / 900 cm |
| Construction / footprint | 120 ticks (6.0 s) / 2×2 tiles |
| Logistics | 0 |
| Special | Moving signatures within 2600 cm; 200 cm resolution; 40 ticks. |


| Player question | Answer |
|---|---|
| Purpose | Provide broad anonymous vibration detection. |
| Selection options | Inspect coverage and contact history; toggle alert threshold. |
| How to use | Cover routes that direct vision cannot safely hold and combine with Resonants for confirmation. |
| Counterplay | Approach stationary, split routes, use projections, or destroy the Spine. |


### 13.3 Hollow Choir structures


#### SPEC-STR-009 — Concordance


| Field | Binding value |
|---|---|
| Role | Headquarters Dropoff |
| Cost | 0 Matter / 0 Dawn |
| Health / sight | 1250 / 900 cm |
| Construction / footprint | 400 ticks (20.0 s) / 5×5 tiles |
| Logistics | 12 |
| Special | None |


| Player question | Answer |
|---|---|
| Purpose | Choir Command Core, worker producer, Matter drop-off, and coherence summary. |
| Selection options | Produce Threadkeeper; set rally; inspect all upcoming structure charges. |
| How to use | Anchor the economy and keep a Dawn reserve. It has no ordinary coherence charge and cannot be rebuilt. |
| Counterplay | Pressure Dawn and structures simultaneously, then attack the Core when the field contracts. |


#### SPEC-STR-010 — Interval Loom


| Field | Binding value |
|---|---|
| Role | Supply Node |
| Cost | 85 Matter / 25 Dawn |
| Health / sight | 400 / 600 cm |
| Construction / footprint | 110 ticks (5.5 s) / 2×2 tiles |
| Logistics | 6 |
| Special | Coherence: 5 Dawn every 600 ticks; 4 within one Phase Anchor field. |


| Player question | Answer |
|---|---|
| Purpose | Provide Logistics and a Matter drop-off while adding a recurring coherence obligation. |
| Selection options | Inspect next charge, route assignment, capacity, and projected insolvency. |
| How to use | Build only when its route and added capacity generate more value than its 5-Dawn charge every 600 ticks. |
| Counterplay | Force the Choir to choose between the Loom, production, abilities, and Well control. |


#### SPEC-STR-011 — Chorus Loom


| Field | Binding value |
|---|---|
| Role | Production |
| Cost | 175 Matter / 40 Dawn |
| Health / sight | 680 / 550 cm |
| Construction / footprint | 170 ticks (8.5 s) / 4×4 tiles |
| Logistics | 0 |
| Special | Coherence: 5 Dawn every 600 ticks; 4 within one Phase Anchor field. |


| Player question | Answer |
|---|---|
| Purpose | Produce Choir combat units and research Choir technology. |
| Selection options | Queue/reorder/cancel units; research; set rally; inspect production plus coherence forecast. |
| How to use | Synchronize production with Dawn income and identity windows. |
| Counterplay | Attack before a charge, interrupt a key unit, and force the Choir to overspend. |


#### SPEC-STR-012 — Phase Anchor


| Field | Binding value |
|---|---|
| Role | Coherence |
| Cost | 120 Matter / 35 Dawn |
| Health / sight | 480 / 800 cm |
| Construction / footprint | 130 ticks (6.5 s) / 2×2 tiles |
| Logistics | 0 |
| Special | Coherence: 5 Dawn every 600 ticks; 4 within one Phase Anchor field. |


| Player question | Answer |
|---|---|
| Purpose | Reduce the recurring cost of a compact Choir position. |
| Selection options | Inspect coherence field, protected structures, next charges, and hold-fire. |
| How to use | Within 700 cm, each eligible non-Core Choir structure pays 4 rather than 5 Dawn per charge. Fields do not stack. |
| Counterplay | Destroy or bypass it; when its field ends, the next full charges remain due and must be forecast. |


## 14. Technology and strategic progression


| ID | Faction | Technology | Cost | Time | Requires | Effect | Why choose it |
|---|---|---|---|---|---|---|---|
| SPEC-TECH-001 | Meridian Compact | Prismatic Targeting | 120 M / 40 D | 180 ticks (9.0s) | None | Damage 115%; vision 100% | Create a decisive Lancer/Bulwark damage timing. |
| SPEC-TECH-002 | Meridian Compact | Horizon Lattice | 90 M / 55 D | 220 ticks (11.0s) | mc_prismatic_targeting | Damage 100%; vision 120% | Extend safe acquisition and network-supported pressure. |
| SPEC-TECH-003 | Kharuun Assemblies | Echo Cartography | 100 M / 45 D | 180 ticks (9.0s) | None | Damage 100%; vision 120% | Improve scouting, route control, and pre-contact adaptation. |
| SPEC-TECH-004 | Kharuun Assemblies | Ancestral Edge | 110 M / 50 D | 220 ticks (11.0s) | ka_echo_cartography | Damage 115%; vision 100% | Convert successful approach and adaptation into damage. |
| SPEC-TECH-005 | Hollow Choir | Held Alternatives | 105 M / 50 D | 190 ticks (9.5s) | None | Damage 110%; vision 110% | Increase both present combat value and information before the final commitment. |
| SPEC-TECH-006 | Hollow Choir | Shared Resolution | 115 M / 60 D | 230 ticks (11.5s) | hc_held_alternatives | Damage 100%; vision 120% | Extend vision for coordinated identity and coherence play. |

* **SPEC-TEC-001 —** Research visibility. The archive shows exact cost, time, effects, prerequisite, affected units, producer contention, queue state, and no-refund interruption before confirmation.
* **SPEC-TEC-002 —** Strategic sufficiency. These two steps are the complete launch tree. Their purpose is a readable timing choice rather than breadth. Campaign rewards are separate and never appear in skirmish.

## 15. Future Wells


A Future Well is a contested strategic site, not a victory point. One eligible worker captures within a 420 cm radius over 300 uncontested ticks. Enemy contest pauses progress. Leaving the radius reverses uncaptured progress at one tick per tick. Control can change hands until Harvest permanently collapses the Well.


| ID | Protocol | Commitment | Effect | When it helps | Cost and counterplay |
|---|---|---|---|---|---|
| SPEC-WELLP-001 | Harvest | 180-tick public telegraph | Gain 500 Dawn; permanently collapse the Well; execute the map's named permanent terrain consequence. | An emergency reserve, decisive production timing, evacuation power, or final attack. | Loses future income and alternatives. Break control before completion. |
| SPEC-WELLP-002 | Preserve | Control remains contestable | Controller gains 15 Dawn every 300 ticks and faction-appropriate intelligence within 1,400 cm. | A defensible long game, recurring Dawn, and information advantage. | Requires continued defense; capture transfers rather than duplicates benefits. |
| SPEC-WELLP-003 | Reshape | Pay 120 Dawn; 180-tick public telegraph | Manifest one map-authored route, bridge, cover, cavern, or evacuation possibility for 1,800 ticks. | A temporary flank, reinforcement route, retreat, denial, or rescue window. | Both players may exploit it; expiry warns at 300/100/20 ticks and uses authored fallback displacement. |

* **SPEC-WEL-001 —** Confirmation. Before commitment, three comparable cards show immediate benefit, resource cost, telegraph, duration, permanence, exact map consequence, interruption, and known campaign consequence. The player confirms the selected protocol.
* **SPEC-WEL-002 —** Strategic neutrality. No protocol changes a hidden morality score or directly wins. Each is situationally rational and creates visible opponent counterplay.
* **SPEC-WEL-003 —** Fog and replay. Capture, contest, telegraph, protocol, terrain change, timers, income, control transfer, interruption, save/load, minimap, AI, and replay obey the common information and determinism rules.

## 16. Single-player opponent AI

* **SPEC-AI-001 —** Fair information. AI reads the same player-scoped terrain, vision, public events, sensor contacts, and aged memory available to a human player. It never reads hidden units, resources, queues, routes, or Well choices.
* **SPEC-AI-002 —** Equal rules. AI pays the same costs, waits the same times, obeys the same Logistics, pathing, fog, range, cooldown, formation, Well, and coherence rules, and receives no hidden income or damage bonuses on any difficulty.
* **SPEC-AI-003 —** Layered control. A strategic controller selects states; economy, production, scouting, tactical groups, and abilities execute bounded plans; campaign director events remain a separate authored system.

| ID | Strategic state | Entry question | Exit condition |
|---|---|---|---|
| SPEC-AIST-001 | ESTABLISH ECONOMY | Can required workers, drop-offs, and Logistics sustain first production? | Opening economy targets met or immediate defense overrides. |
| SPEC-AIST-002 | SCOUT | What route, resource, Well, or threat is unknown? | Priority discovery, loss, unsafe route, or budget expiry. |
| SPEC-AIST-003 | EXPAND | Is a known resource worth route and defense cost? | Operational drop-off established or threat cancels. |
| SPEC-AIST-004 | DEFEND | Which Core, route, network, or producer is threatened? | Threat defeated, disengaged, or base evacuation/recovery required. |
| SPEC-AIST-005 | ASSEMBLE | Does observed composition justify a timed force? | Threshold and rally achieved or counter-evidence changes plan. |
| SPEC-AIST-006 | ATTACK | Can this force convert advantage into economy, Well, or Core damage? | Objective achieved, retreat threshold reached, or defense override. |
| SPEC-AIST-007 | RAID | Is an exposed route or production site worth a bounded force? | Time/damage budget reached, target lost, or defender response. |
| SPEC-AIST-008 | CONTEST WELL | Is the visible protocol or site worth commitment? | Control/protocol resolved or opportunity cost exceeds value. |
| SPEC-AIST-009 | RETREAT | Can force value be preserved? | Safe rally reached, pursuit broken, or no path remains. |
| SPEC-AIST-010 | RECOVER | Which stalled economy, supply, production, power, coherence, or scouting failure is recoverable? | Named recovery succeeds or concession assessment begins. |


### 16.1 AI doctrines


| ID | Doctrine | Economy and scouting | Combat posture | Well preference |
|---|---|---|---|---|
| SPEC-DOC-001 | Warden | Conservative expansion; strong approach vision | Layered defense, short pursuit, counterattack after defense | Preserve unless Harvest prevents defeat |
| SPEC-DOC-002 | Raider | Lean economy; two-scout route search | Frequent worker/route raids; avoids static defense | Reshape for access; Harvest for tempo |
| SPEC-DOC-003 | Steward | Worker and income priority; protects saturation | Defends economy and attacks after production advantage | Preserve |
| SPEC-DOC-004 | Expansionist | Earlier secondary drop-off and Logistics | Multi-route map pressure; accepts wider front | Preserve or Reshape to secure expansion |
| SPEC-DOC-005 | Adaptive | Moderate opening; evidence-driven scouting | Composition and posture change from observed evidence | Chooses from observed timing and route value |


### 16.2 Difficulty


| ID | Difficulty | Reaction delay | Strategic review | Group command ceiling | Planning behavior |
|---|---|---|---|---|---|
| SPEC-DIF-001 | Story | 60 ticks | Every 200 ticks | 4 commands/s | Obvious plans, generous retreat, delayed counters |
| SPEC-DIF-002 | Standard | 30 ticks | Every 100 ticks | 7 commands/s | Fair baseline; one-step counters; moderate coordination |
| SPEC-DIF-003 | Veteran | 18 ticks | Every 60 ticks | 10 commands/s | Multi-step planning, stronger focus and retreat discipline |
| SPEC-DIF-004 | Sovereign | 10 ticks | Every 40 ticks | 12 commands/s | Longer planning horizon, feints, synchronized routes; still equal information/resources |

* **SPEC-AI-004 —** Perceived intelligence. AI scouts, protects workers, retreats damaged forces, regroups, expands to known resources, contests or concedes Wells intentionally, exploits observed weak routes, defends its Core, and converts advantage into Corefall pressure.
* **SPEC-AI-005 —** Recovery and concession. AI diagnoses stalled workers, blocked spawn, lost drop-off, capacity loss, disconnected power, failed molt/identity, insolvency, destroyed scouts, and failed attacks. It concedes only when no meaningful Core-defense, production, or economic recovery path remains.
* **SPEC-AI-006 —** Mission director. Scripted waves, dialogue, reinforcements, hazards, and cinematic beats have authored triggers, telegraphs, save/load idempotence, and event-ledger labels. They never masquerade as units bought by the opponent economy.

## 17. Skirmish configuration and maps


| ID | Setting | Options |
|---|---|---|
| SPEC-SKM-001 | Player faction | Meridian Compact, Kharuun Assemblies, Hollow Choir |
| SPEC-SKM-002 | Opponent faction | Any faction, including mirror |
| SPEC-SKM-003 | Map | Glass Scar, Crownfall Basin, The Confluence Ring |
| SPEC-SKM-004 | AI doctrine | Warden, Raider, Steward, Expansionist, Adaptive |
| SPEC-SKM-005 | Difficulty | Story, Standard, Veteran, Sovereign |
| SPEC-SKM-006 | Starting resources | Scarce, Standard, Abundant |
| SPEC-SKM-007 | Game speed | 0.75×, 1.0×, 1.25×; 1.0× default |
| SPEC-SKM-008 | Map reveal | Standard fog only; no full-reveal launch option |
| SPEC-SKM-009 | Victory | Corefall; fixed and shown before deployment |
| SPEC-SKM-010 | Pause | Freezes simulation; menus remain available; battlefield orders cannot be issued while paused |


| ID | Map | Geometry and strategic identity | Resource/Well contract |
|---|---|---|---|
| SPEC-SKM-011 | Glass Scar | 64×64 tiles at 100 cm simulation scale. Opposed basins separated by Ash Cut, Buried Causeway, and Folded Verge. Central ridge and route commitment reward scouting and controlled conversion. | One central Future Well; two safe inefficient home deposits, two contested side deposits, and two central deposits. Scout-to-Well target 45–60 seconds; army 65–90 seconds. |
| SPEC-SKM-012 | Crownfall Basin | 64×64. Twin vertical ridges with three gates plus north/south shelf walls. The player chooses which gate becomes economy route, defense line, or flank. | One Well offset north of center; two home, two near-expansion, and four contested deposits. |
| SPEC-SKM-013 | The Confluence Ring | 64×64. Central walled ring with four cardinal gates, west/east shards, and repeated Choir geometry. Control of entrances matters more than raw distance. | One central Well inside the ring; two home deposits and six distributed contested deposits. |

* **SPEC-MAP-001 —** Spawn fairness. Each map supports two fixed mirrored-distance spawn regions with equivalent starting build area, resource travel time within 5%, Well approach time within 5%, and no sightline into the opposing start.
* **SPEC-MAP-002 —** Map truth. Every map ships with a machine-readable contract for grid, starts, terrain, resources, Well, passages, objectives, camera bounds, Reshape outcomes, fallbacks, and deterministic hash.
* **SPEC-MAP-003 —** Post-match. Results show outcome cause, duration, resource curves, unit production/losses, damage, scouting coverage, idle-worker time, Well control/protocol, Logistics blocks, and AI doctrine/difficulty. Rematch preserves settings; Restart uses the same seed; New Match returns to setup.

## 18. Campaign structure, persistence, and progression

* **SPEC-CAM-001 —** Structure. The campaign contains fifteen operations: five in Necessary Fires, five in The Cost of One Future, and five in Crownfall. All operations remain replayable after first completion.
* **SPEC-CAM-002 —** Feature introduction. Every operation has a capability manifest marking each unit, building, command, ability, resource, terrain interaction, and UI system as introduced, practiced, assessed, retained, or locked. A low-pressure lesson precedes required mastery.
* **SPEC-CAM-003 —** Persistence. Mission completion, Well records, district allocation, public evidence, optional objectives, campaign rewards, unlocked roster, and ending eligibility persist. Units, unit damage, ordinary resources, constructed bases, and skirmish research reset between operations. There is no unit permadeath or persistent army.
* **SPEC-CAM-004 —** Rewards. Rewards change a later tactical or strategic decision and show exact effects. They never create an unknowable trap, purchase power outside the campaign, or replace the consequence of the original choice.
* **SPEC-CAM-005 —** Retry. Autosave occurs before deployment, at authored checkpoints, and after successful result commitment. Retry restores the last checkpoint. Failure writes no campaign consequence. Replaying a completed operation shows alternate outcomes but does not rewrite the established ledger.
* **SPEC-CAM-006 —** Branch clarity. The campaign map shows completed operations, known consequences, available next operations, and the decision classes affecting the future. It does not reveal unearned narrative information or use a hidden morality score.

### 18.1 Mission plan


| ID | ID | Scale | Duration | Command | Starting package | System focus | Primary purpose | Optional/reward or consequence |
|---|---|---|---|---|---|---|---|---|
| SPEC-PLAN-001 | M01 | MICRO | 20–25 min | Meridian | Anchor; 6 Surveyors; 2 Lancers; 1 Bulwark; 1 Relay Skiff | Selection, camera, gathering, construction, combat, Future Wells | Recover the archive carrier and evacuate after one Well decision. | Optional: save both outer reserve stations; reward: +1 starting Surveyor in Meridian-led operations. |
| SPEC-PLAN-002 | M02 | MICRO | 20–25 min | Kharuun | Memory Hearth; 6 Tenders; 2 Riftstalkers; 1 Resonant | Vibration contacts, patrol, fixed subsurface passages | Defend a migration route while reconciling conflicting terrain memories. | Optional: verify all seven memory markers; reward: longer vibration-contact linger in campaign only. |
| SPEC-PLAN-003 | M03 | HYBRID | 25–30 min | Meridian | Anchor; 7 Surveyors; 3 Lancers; 1 Bulwark; 1 Skiff | Power links, multi-site defense, Logistics | Stabilize three ark-city grid sites without losing the reserve chain. | Optional: keep every link operational; reward: Power Links build 10% faster in campaign. |
| SPEC-PLAN-004 | M04 | HYBRID | 25–30 min | Kharuun | Hearth; 7 Tenders; 3 Riftstalkers; 1 Cairnback; 1 Resonant | Waystone migration, Mineral Cover, adaptation | Move infrastructure through a contested road and recover a memory shard. | Optional: complete without losing a rooted Waystone; reward: first adaptation in each later Kharuun mission costs 10 less Dawn. |
| SPEC-PLAN-005 | M05 | HYBRID | 25–30 min | Meridian/Kharuun alliance fiction; Meridian command | Anchor detachment; mixed allied NPC column | Guard, escort, ceasefire rules, scripted-event fairness | Hold a ceasefire corridor and preserve witnesses while pressure reveals a wider conflict. | Optional: prevent damage to both networks; reward: additional branch evidence and later dialogue, not a combat bonus. |
| SPEC-PLAN-006 | M06 | MICRO | 20–25 min | Meridian | Small mobile force; no initial production | Low-force reconnaissance, evidence protection, avoidance | Trace erased census records and extract civilians and evidence. | Optional: recover every census fragment; reward: unlock the Archive district as a later power candidate. |
| SPEC-PLAN-007 | M07 | HYBRID | 25–30 min | Kharuun | Hearth; scouting and mobile-infrastructure force | Paired witnesses, detection triangulation | Prove a correspondence between erased census entries and curated communal-memory omissions. | Optional: leave neutral records untouched; reward: additional ending eligibility context. |
| SPEC-PLAN-008 | M08 | MICRO | 20–25 min | Choir contact through bounded proxy command | Intervalists and Afterimages; no full base | Choir identity, projections, reciprocal contact | Establish actionable contact with Neme across overlapping possible states. | Optional: make contact without destroying a neutral defense; reward: one additional M15 dialogue resolution path. |
| SPEC-PLAN-009 | M09 | MACRO | 30–35 min | Meridian | Full Compact base and roster | Resource triage, multi-base defense, irreversible allocation | Power exactly two of Life Support, Transit, and Archive while defending the city. | The unpowered district becomes an explicit deferred liability and changes M15 hold duration. |
| SPEC-PLAN-010 | M10 | HYBRID | 25–30 min | Kharuun | Full Kharuun base and roster | Three-faction contact, sequential Listening Spines, Well commitment | Establish branch-specific contact at Lume Reach and resolve one new Well protocol. | The chosen Lume protocol becomes a final-ending eligibility axis. |
| SPEC-PLAN-011 | M11 | MACRO | 30–35 min | Kharuun | Full Kharuun base | Ledger readback, multi-site coordination, route inheritance | Assemble public coalition evidence using the exact prior campaign record. | Optional: hold all public interfaces simultaneously; reward: shorter M12 activation hold. |
| SPEC-PLAN-012 | M12 | HYBRID | 25–30 min | Kharuun | Full base; Oruun plus verifier | Independent readback, district-link construction, timed receipt | Test Rhyse's claimed future against public Meridian and Kharuun evidence. | Optional: preserve both district links; reward: clearer M13 approach information. |
| SPEC-PLAN-013 | M13 | HYBRID | 25–30 min | Kharuun | Mobile force with construction support | Crownfall linkage, paired observation, protected witnesses | Assemble the missing through public readback without assigning unsupported authorship. | Optional: avoid destroying public apparatus; reward: reduced first Phase Anchor cost in M15. |
| SPEC-PLAN-014 | M14 | MICRO | 20–25 min | Hollow Choir | Concordance; Threadkeepers; limited Choir force | Full Choir economy, identity, coherence, command authority | Reconcile several internal voices into one bounded command without erasing disagreement. | Optional: finish with every temporary structure solvent; reward: +10 starting Dawn in M15. |
| SPEC-PLAN-015 | M15 | MACRO | 35–45 min | Hollow Choir | Full Choir base; Mara, Oruun, Talar, and Neme protected | All systems; accord; ending eligibility; final hold | Secure the Crownfall approach, assemble three witnessed accords, confirm an earned resolution twice, raise its conduit, and hold the contract. | Ending availability and hold time derive only from visible recorded choices. |


### 18.2 Four endings


| ID | Ending | Eligibility | Final meaning | Hold modifier |
|---|---|---|---|---|
| SPEC-END-001 | Restoration | Preserve at Lume Reach and Life Support powered | Return held futures slowly and under guard to the ground from which they were taken. | +80 ticks |
| SPEC-END-002 | Controlled Stabilization | Always eligible | Hold the Crownfall as a managed wound: lower immediate disruption, unresolved long-term cost. | +0 ticks |
| SPEC-END-003 | Extinguishment | Harvest doctrine or Lume protocol | Spend remaining held futures to close the Crownfall and silence what argues through it. | +40 ticks |
| SPEC-END-004 | Open Evolution | Reshape doctrine or Lume protocol | Release the Crownfall to continue becoming without central control or guaranteed outcome. | +120 ticks |

* **SPEC-CAM-007 —** Ending confirmation. The player sees the earned subset, eligibility reasons, known tradeoff, protected witnesses, conduit location, and hold duration; one ending must be confirmed twice before construction. No ending is labeled canonical, correct, or morally superior.

## 19. Mission objective and failure contracts


These contracts bind narrative purpose, player authority, objective language, failure causes, and consequence boundaries. Level design may tune encounter placement and composition only while preserving these contracts.


### What the Ledger Keeps


Mara Vey commands a Meridian evacuation, recovers Talar Venn's displaced archive convoy, makes the first irreversible Future Well decision, and withdraws the archive carrier to Lume Reach.


#### SPEC-MSN-001 — What the Ledger Keeps

| Contract field | Binding content |
|---|---|
| Player authority | mara vey command authority |
| Command faction | meridian compact |
| Briefing | Recover the displaced archive convoy at tile 22,18, commit one visible Future Well protocol while the carrier holds the site, then evacuate the carrier to Lume Reach at tile 6,17. |
| Objectives | Bring the Meridian scout carrying the archive to tile 22,18.<br>Hold the carrier at the recovery site while a worker commits Harvest, Preserve, or Reshape at the Future Well.<br>After the protocol commits, bring the surviving carrier to Lume Reach at tile 6,17. |
| Failure causes | The local Command Core is absent or has no hit points.<br>The archive carrier is absent or has no hit points.<br>A committed Future Well is controlled by a nonlocal player.<br>The skirmish outcome is no longer ongoing before mission completion.<br>A terminal failure condition occurs without a presentation-safe reason code. |
| Canonical facts | The city's failing reserve creates a credible reason to Harvest without making Harvest the canonical answer.<br>Talar asks for time to recover a displaced archive convoy.<br>Oruun opposes collapse propagation into a birthing cavern rather than seeking to seize Lume Reach.<br>The Meridian scout is the archive carrier; Mara is the player's command authority.<br>A worker must discover and commit Harvest, Preserve, or Reshape while the surviving carrier holds tile 22,18.<br>Success requires the carrier to reach tile 6,17 after the Well protocol commits.<br>Failure appends no campaign record; a successful authoritative completion may add, verify, conflict with, or fail to save a record. |
| Prohibited claims | Mara is physically represented by the archive-carrier unit.<br>Talar and Oruun are not physically represented as controllable field units in this operation.<br>Destroying the hostile Command Core completes the mission without evacuation.<br>Harvest, Preserve, or Reshape is labeled good, bad, canonical, or morally scored.<br>This contract alone does not establish mission-specific dialogue, subtitles, voice, or cinematics.<br>Mission 01 establishes modeled downstream civilian, relationship, or faction-trust outcomes. |
| Branch tradeoffs | Harvest: The protocol requires a public 180-tick commitment before irreversible collapse.<br>Preserve: The protocol pays its stated cadence only while control is held; delayed value and changing ownership remain explicit to the player.<br>Reshape: The protocol requires a public 180-tick commitment and a pre-expiry warning before deterministic fallback displacement. |


### Seven Accounts of Rain


Oruun-of-Seven-Stones defends a Kharuun migration route whose inherited accounts disagree about the terrain, re-roots the Waystone at the anchor the founding Well decision selected, and brings the memory bearer to the account site to make the recall.


#### SPEC-MSN-002 — Seven Accounts of Rain

| Contract field | Binding content |
|---|---|
| Player authority | oruun command authority |
| Command faction | kharuun assemblies |
| Briefing | The migration moves today, and the seven inherited accounts disagree about the ground. Hold the route, root the Waystone at the anchor the founding decision left, and bring the memory bearer to the account site. |
| Objectives | Uproot the Waystone and root it at the inherited route anchor.<br>Bring the memory bearer to the account site while the route holds. |
| Failure causes | The local command core is destroyed while the operation is active.<br>The memory bearer is destroyed before the recall is made.<br>The Waystone is destroyed before or after rooting.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision and selects one of three authored routes from it.<br>The Harvest founding decision selects the fractured western account, with the Waystone anchor at tile 20,42 and the account site at tile 23,44.<br>The Preserve founding decision selects the archive-verified central account, with the Waystone anchor at tile 35,40 and the account site at tile 38,43.<br>The Reshape founding decision selects the manifested eastern account, with the Waystone anchor at tile 40,42 and the account site at tile 43,44.<br>The operation requires the Waystone rooted at the selected anchor before the recall phase can begin.<br>Completion requires the memory bearer standing at the selected account site while the local core, the bearer, and the Waystone survive.<br>Losing the local core, the memory bearer, the Waystone, or reaching a terminal engagement outcome fails the attempt irreversibly. |
| Prohibited claims | No inherited account is established as true or infallible by playing this mission.<br>The Kharuun memory dispute is not resolved by the recall.<br>Completion does not assert the migration herds survive beyond the held route.<br>No claim of consent, trust, or agreement between the seven accounts is established.<br>The recall does not establish archival authenticity of any account.<br>No wider Kharuun political outcome follows from this single held route. |
| Branch tradeoffs |   |


### A City on Reserve


Mara Vey stabilizes an ark-city reserve grid by reconnecting the Life Support, Transit, and Archive districts in the priority order the founding Well decision selected, while every district structure and the local core must survive.


#### SPEC-MSN-003 — A City on Reserve

| Contract field | Binding content |
|---|---|
| Player authority | mara vey command authority |
| Command faction | meridian compact |
| Briefing | The ark-city grid is on reserve and failing. The founding decision fixed the order the districts come back: reconnect each district interface in that order and keep every structure standing. The order is inherited; the work is now. |
| Objectives | Power the first district interface in the inherited priority order.<br>Power the second district interface while the first holds.<br>Power the final district interface and keep all three standing. |
| Failure causes | The local command core is destroyed while the operation is active.<br>A Life Support, Transit, or Archive district structure is destroyed.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision and selects one of three authored reserve plans from it.<br>The Harvest founding decision selects the emergency load-shed plan, stabilizing Life Support, then Transit, then Archive.<br>The Preserve founding decision selects the continuity reserve plan, stabilizing Archive, then Life Support, then Transit.<br>The Reshape founding decision selects the manifested transit-weave plan, stabilizing Transit, then Life Support, then Archive.<br>The Life Support district interface stands at tile 24,10, Transit at tile 10,24, and Archive at tile 20,20.<br>Each district must be powered in the inherited order; a later district cannot be stabilized before an earlier one.<br>Losing the local core, any district structure, or reaching a terminal engagement outcome fails the attempt irreversibly. |
| Prohibited claims | The wider city is not fully recovered by stabilizing three districts.<br>No claim is made about reserve consequences beyond the three reconnected interfaces.<br>Completion does not assert civilian survival or wellbeing inside any district.<br>The inherited priority order is not established as the correct or moral order.<br>No consent of district populations to the chosen order is established.<br>Grid stability beyond the held session is not established. |
| Branch tradeoffs |   |


### The Unburied Road


Oruun-of-Seven-Stones takes mobile Kharuun infrastructure down the route the founding Well decision selected, roots the Waystone at the roadhead, raises a Listening Spine, and brings the memory bearer to the missing shard site to recover it.


#### SPEC-MSN-004 — The Unburied Road

| Contract field | Binding content |
|---|---|
| Player authority | oruun command authority |
| Command faction | kharuun assemblies |
| Briefing | A memory shard is missing from the record, and the road to it runs subsurface. Take the road the founding decision selected: root the Waystone at the roadhead, raise a Listening Spine over the vaults, and walk the bearer to the shard. |
| Objectives | Uproot the Waystone and root it at the inherited roadhead.<br>Raise a Listening Spine at the survey site while the road holds.<br>Bring the memory bearer to the shard site to make the recovery. |
| Failure causes | The local command core is destroyed while the operation is active.<br>The memory bearer is destroyed before the shard is recovered.<br>The Waystone is destroyed before or after rooting.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision and selects one of three authored roads from it.<br>The Harvest founding decision selects the Ash Cut, with the roadhead at tile 14,28, the Listening Spine site at tile 14,37, and the shard site at tile 20,43.<br>The Preserve founding decision selects the Buried Causeway, with the roadhead at tile 32,28, the Listening Spine site at tile 32,37, and the shard site at tile 38,43.<br>The Reshape founding decision selects the Folded Verge, with the roadhead at tile 50,28, the Listening Spine site at tile 49,35, and the shard site at tile 44,40.<br>The Waystone must be rooted at the roadhead before the Listening Spine phase can begin.<br>The Listening Spine must be complete before the shard recovery phase can begin, and completion requires the memory bearer at the shard site.<br>Losing the local core, the memory bearer, the Waystone, or reaching a terminal engagement outcome fails the attempt irreversibly. |
| Prohibited claims | The recovered shard does not resolve the Assembly memory dispute.<br>No complete ancestral record is restored by recovering one shard.<br>The shard's content is not established as authentic or infallible.<br>Completion does not assert anything about who buried the shard or why.<br>No claim of subsurface territory control beyond the walked road is established.<br>The three roads' names describe terrain, not moral standing. |
| Branch tradeoffs |   |


### Terms of Continuance


Mara Vey, as the Meridian treaty authority, synchronizes the Meridian relay with the Kharuun spine so drafted ceasefire terms can be read across both networks, holds the continuance window uncompromised for its authored span, and takes both treaty witnesses off the field alive. Canon continuity extension recorded 2026-09-01 with owner approval: Mara Vey alone carries Mission 05's spoken lines, consistent with her Act I Meridian command arc; the Bible's treaty and witness proxies remain unnamed.


#### SPEC-MSN-005 — Terms of Continuance

| Contract field | Binding content |
|---|---|
| Player authority | mara command authority |
| Command faction | meridian compact |
| Briefing | Drafted ceasefire terms are ready to read, and nobody has signed them. Synchronize our relay with the Kharuun spine, hold the continuance window for its full span while the terms read out, and take both witnesses off the field alive. |
| Objectives | Synchronize the Meridian relay and the Kharuun spine.<br>Hold the continuance window uncompromised for its full span.<br>Extract both witnesses from the field alive. |
| Failure causes | The local command core is destroyed while the operation is active.<br>The Meridian relay is destroyed before extraction completes.<br>The Kharuun spine is destroyed before extraction completes.<br>Either witness is destroyed before extraction completes.<br>The continuance window is compromised before completing its span.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision and selects one of three authored accord clauses from it.<br>The Harvest founding decision selects the Iron Clause, with the Meridian relay at tile 14,27, the Kharuun spine at tile 14,39, and the witness extraction at tile 20,47.<br>The Preserve founding decision selects the Witness Clause, with the Meridian relay at tile 32,27, the Kharuun spine at tile 32,39, and the witness extraction at tile 32,47.<br>The Reshape founding decision selects the Folded Clause, with the Meridian relay at tile 50,27, the Kharuun spine at tile 44,38, and the witness extraction at tile 44,47.<br>Both the Meridian relay and the Kharuun spine must be synchronized before the continuance window phase can begin.<br>The continuance window spans authored ticks 300 through 900 and must be held uncompromised for its full span.<br>Completion requires both witnesses extracted alive after the window has held.<br>Losing the local core, the Meridian relay, the Kharuun spine, or either witness, or compromising the window, fails the attempt irreversibly. |
| Prohibited claims | No party signs or ratifies the ceasefire in this operation.<br>The source of the pressure on the field is not identified, and no Hollow Choir involvement is established.<br>The extracted witnesses' account is not established as accepted by either command.<br>Holding the window does not establish that the war paused anywhere beyond this field.<br>Mixed-faction command is not established; command authority in this operation is Meridian only.<br>The three clauses' names describe drafting variants and terrain, not legal force. |
| Branch tradeoffs |   |


### Names Without Births


Talar Venn traces an erased census record along the geometry the founding Well decision selected, powers the archive that holds it, shelters the two civilian proxies the trace exposes, and stands at the extraction site to take the evidence out intact.


#### SPEC-MSN-006 — Names Without Births

| Contract field | Binding content |
|---|---|
| Player authority | talar venn command authority |
| Command faction | meridian compact |
| Briefing | Somewhere in this district, names were recorded that no birth explains, and then the record of them was erased. Follow the trace the founding decision left reachable: locate the census, power the archive, shelter who the search exposes, and take the evidence out whole. |
| Objectives | Locate the census evidence along the inherited trace.<br>Power the archive that holds the record.<br>Shelter both exposed civilian proxies.<br>Bring Talar to the extraction site with the evidence intact. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Talar Venn is destroyed while the operation is active.<br>The census archive structure is destroyed.<br>Either exposed civilian proxy is destroyed.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision and selects one of three authored census geometries from it.<br>The Harvest founding decision selects the Foundry Roll trace, with the census at tile 16,22, the power link at 16,16, the shelter at 14,48, and the extraction at 22,44.<br>The Preserve founding decision selects the Missing Quarter trace, with the census at tile 32,22, the power link at 28,16, the shelter at 32,48, and the extraction at 32,44.<br>The Reshape founding decision selects the Folded Register trace, with the census at tile 48,22, the power link at 45,16, the shelter at 39,37, and the extraction at 46,37.<br>The census evidence must be located before the archive can be stabilized, and the archive powered before the civilians can be sheltered.<br>Both civilian proxies must be sheltered before the extraction phase, and completion requires Talar at the extraction site.<br>Losing the local core, Talar, the archive, or either civilian proxy, or reaching a terminal engagement outcome, fails the attempt irreversibly. |
| Prohibited claims | No cause or author of the erased census is established by the trace.<br>Civilian outcomes beyond the two bounded proxies are not modeled and may not be claimed.<br>The recovered evidence is not established as complete or authentic.<br>Sheltering the proxies does not assert their consent or their trust.<br>The missing names are not established as recoverable.<br>No wider Compact records policy follows from one extracted trace. |
| Branch tradeoffs |   |


### The Shape of Silence


Oruun-of-Seven-Stones follows the correspondence between an erased census and curated omissions in Kharuun communal memory: root the Waystone at the hollow's anchor, raise a Listening Spine, position two memory witnesses, and reach the confluence himself.


#### SPEC-MSN-007 — The Shape of Silence

| Contract field | Binding content |
|---|---|
| Player authority | oruun command authority |
| Command faction | kharuun assemblies |
| Briefing | The Compact lost a census. Our communal memory lost a neighborhood, and lost the memory of losing it. The two silences correspond. Root the Waystone, raise the Spine, set two witnesses where the accounts thin, and Oruun walks to the confluence to hear the shape. |
| Objectives | Root the Waystone at the hollow's inherited anchor.<br>Raise a Listening Spine over the thinned accounts.<br>Position both memory witnesses at their sites.<br>Bring Oruun to the confluence while the geometry holds. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Oruun-of-Seven-Stones is destroyed while the operation is active.<br>The Waystone is destroyed before or after rooting.<br>Either memory witness is destroyed.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision and selects one of three authored memory hollows from it.<br>The Harvest founding decision selects the Cinder Hollow geometry, anchored at tile 14,28 with its confluence at tile 14,50.<br>The Preserve founding decision selects the Held Hollow geometry, anchored at tile 32,28 with its confluence at tile 32,50.<br>The Reshape founding decision selects the Folded Hollow geometry, anchored at tile 48,20 with its confluence at tile 25,50.<br>The Waystone must be rooted at the anchor before the Listening Spine, and the Spine raised before the witnesses can be positioned.<br>Both memory witnesses must stand at their sites before the confluence phase, and completion requires Oruun himself at the confluence.<br>Losing the local core, Oruun, the Waystone, or either witness, or reaching a terminal engagement outcome, fails the attempt irreversibly. |
| Prohibited claims | The correspondence between the erased census and the communal omissions is bounded; no cause is established.<br>No hidden author of either erasure is identified.<br>No Choir identity is established by this mission.<br>The Assembly confrontation the correspondence gestures toward is not completed here.<br>The witnesses' accounts are not established as true, only as positioned and heard.<br>No claim about what the silence contains is established, only its shape. |
| Branch tradeoffs |   |


### The Shape Beside Us


Talar Venn commands Meridian proxies through the overlap geometry the founding Well decision selected while Neme guides the traversal: reach the first echo, raise an echo relay, traverse both paired states with the state witnesses, and bring Talar to the convergence.


#### SPEC-MSN-008 — The Shape Beside Us

| Contract field | Binding content |
|---|---|
| Player authority | talar venn command authority |
| Command faction | meridian compact |
| Briefing | Something walks beside our proxies through ground that exists twice. Neme offers guidance through the overlap; the offer is the first reciprocal contact we can act on. Reach the first echo, raise the relay, traverse both paired states, and Talar walks to the convergence. |
| Objectives | Reach and observe the first echo along the inherited geometry.<br>Raise the echo relay at its site.<br>Traverse both paired states with the state witnesses.<br>Bring Talar to the convergence while the overlap holds. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Talar Venn is destroyed while the operation is active.<br>Either paired-state witness is destroyed.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision and selects one of three authored overlap geometries from it.<br>The Harvest founding decision selects the Exhausted Echo, from first echo at tile 14,28 to convergence at tile 14,50.<br>The Preserve founding decision selects the Held Echo, from first echo at tile 32,28 to convergence at tile 32,50.<br>The Reshape founding decision selects the Folded Echo, from first echo at tile 46,20 to convergence at tile 44,6.<br>The first echo must be observed before the relay, and the relay raised before the paired states can be traversed.<br>Both paired states must be traversed by the state witnesses before the convergence phase, and completion requires Talar at the convergence.<br>Losing the local core, Talar, or either state witness, or reaching a terminal engagement outcome, fails the attempt irreversibly. |
| Prohibited claims | Contact through Neme does not establish a unified Choir identity.<br>No hidden authorship of the echoes or the earlier erasures is established.<br>No causation between the overlap and the census silences is established, only proximity.<br>The traversal does not assert that the paired states are safe, stable, or repeatable outside this geometry.<br>Neme's guidance is not established as complete or disinterested.<br>No Choir faction becomes playable or commandable through this contact. |
| Branch tradeoffs |   |


### Reserve Authority


Mara Vey secures branch-specific authority over a failing reserve grid where the inherited doctrine is advisory: ordinary workers power exactly two of Life Support, Transit, and Archive, and Mara herself confirms the intact deferred district. The allocation is irreversible and local.


#### SPEC-MSN-009 — Reserve Authority

| Contract field | Binding content |
|---|---|
| Player authority | mara vey command authority |
| Command faction | meridian compact |
| Briefing | The reserve cannot carry three districts. The doctrine we inherited recommends an order; the choice is ours, and it will not be reversible. Secure the authority site, power exactly two districts, and Mara confirms the one we defer, standing in it. |
| Objectives | Secure the branch authority site.<br>Power the first district relay of your allocation.<br>Power the second district relay of your allocation.<br>Bring Mara to the deferred district and confirm it intact. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Mara Vey is destroyed while the operation is active.<br>A Life Support, Transit, or Archive district structure is destroyed.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision as an advisory doctrine, not a binding order.<br>The Harvest doctrine recommends Life Support first from the authority site at tile 15,16; Preserve recommends Archive first from tile 15,15; Reshape recommends Transit first from tile 16,15.<br>Mara must secure the branch authority site before any allocation can begin.<br>Ordinary workers power exactly two district relays: Life Support at tile 17,12, Transit at tile 12,17, or Archive at tile 16,14.<br>The third district is deferred by the player's own allocation, and completion requires Mara standing at the deferred district while it is intact.<br>The allocation is recorded as one irreversible local decision.<br>Losing the local core, Mara, or any district structure, or reaching a terminal engagement outcome, fails the attempt irreversibly. |
| Prohibited claims | No wider-city recovery follows from powering two districts.<br>Civilian survival inside any district, powered or deferred, is not modeled and may not be claimed.<br>The advisory doctrine is not established as correct; following or ignoring it carries no hidden score.<br>Deferring a district does not assert its population consented to deferral.<br>The deferred district's intact confirmation is a structural fact, not a welfare claim.<br>No claim about future reserve capacity beyond this session is established. |
| Branch tradeoffs |   |


### The Choir at Lume Reach


Oruun-of-Seven-Stones commands a Kharuun force at Lume Reach with Mara Vey as an off-map liaison: establish branch-specific contact, re-root the Waystone at the Mission 09 deferred district's liability, raise two Listening Spine anchors in sequence, commit a new Lume Well protocol from all three choices, and reach that protocol's public resolution.


#### SPEC-MSN-010 — The Choir at Lume Reach

| Contract field | Binding content |
|---|---|
| Player authority | oruun command authority |
| Command faction | kharuun assemblies |
| Briefing | Lume Reach holds a Well, a Choir presence that answers, and the liability our Mission 09 deferral left standing. Establish contact on the inherited approach, resolve the liability, raise both anchors, and commit a new Well protocol — then stand at its public resolution. |
| Objectives | Establish contact at the inherited approach site.<br>Re-root the Waystone at the deferred district's liability.<br>Raise the first Listening Spine anchor.<br>Raise the second Listening Spine anchor in sequence.<br>Commit a new Lume Well protocol from the three offered choices.<br>Reach the committed protocol's public resolution site. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Oruun-of-Seven-Stones is destroyed while the operation is active.<br>The Waystone is destroyed before or after the liability is resolved.<br>The Lume Well is destroyed before the protocol is resolved.<br>The Reshape protocol's resolution window expires before the public resolution.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the Mission 01 founding Well decision for its approach and the Mission 09 allocation for its liability site.<br>The Harvest founding decision selects the Ashward approach with contact at tile 18,20; Preserve the Held-Vault approach at 32,20; Reshape the Foldward approach at 46,20.<br>The deferred district's liability stands at tile 18,34 for Life Support, 32,33 for Transit, or 46,34 for Archive.<br>The two Listening Spine anchors rise in sequence at tiles 28,39 and 36,39, and the Lume Well stands at tile 32,43.<br>The new Well protocol is committed from all three offered choices, and each protocol's public resolution site differs: Harvest at 18,50, Preserve at 32,50, Reshape at 46,50.<br>The Reshape protocol's resolution window can expire, failing the attempt.<br>Losing the local core, Oruun, the Waystone, or the Well, or reaching a terminal engagement outcome, fails the attempt irreversibly, and success records a separate tenth irreversible Well decision. |
| Prohibited claims | The local Choir remains a public contact and infrastructure presence, never playable or commandable.<br>The mechanically opposing Meridian units are quarantine proxies; their presence does not establish Mara's involvement or Compact-wide action.<br>No Choir identity, intent, or unified voice is established by the contact.<br>The committed protocol is not established as the right choice; each carries its own recorded consequence.<br>No claim about Lume Reach's civilian population follows from the protocol resolution.<br>The tenth Well decision does not resolve or supersede the founding decision. |
| Branch tradeoffs | Harvest: Design target: the Reach's held futures become immediate yield, and the settlement's Well weight is spent rather than kept.<br>Preserve: Design target: the Reach's futures stay held and unspent, and the Well keeps its weight against later need.<br>Reshape: Design target: the Reach's ground opens where it was closed, for a bounded time whose ending is part of the commitment. |


### No Neutral Ledger


Oruun-of-Seven-Stones turns the exact ten-record campaign ledger into one of twenty-seven explicit plans: secure the founding doctrine's inherited route, integrate the two powered districts' contributions, attest both neutral evidence channels, apply only the recorded Lume protocol, and rally the coalition at its public site.


#### SPEC-MSN-011 — No Neutral Ledger

| Contract field | Binding content |
|---|---|
| Player authority | oruun command authority |
| Command faction | kharuun assemblies |
| Briefing | Ten records, no neutral reading. The doctrine picks our route, the allocation our contributors, the Lume protocol itself. Walk it exactly: secure, integrate, attest both neutral interfaces, apply only what was recorded, and rally the coalition where the protocol says. |
| Objectives | Secure the founding doctrine's inherited route.<br>Integrate both powered districts' contributions.<br>Attest both neutral evidence interfaces with distinct witnesses.<br>Apply only the recorded Lume protocol at the Well.<br>Rally the coalition at the protocol's public site. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Oruun-of-Seven-Stones is destroyed while the operation is active.<br>The Waystone is destroyed while the operation is active.<br>The distinct Kharuun ledger witness is destroyed.<br>The Future Well is destroyed before the recorded protocol is applied.<br>Either neutral public evidence interface is destroyed.<br>A protocol conflicting with the Mission 10 record is applied at the Well.<br>The Reshape rally window expires before the coalition rallies.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the exact ten-record ledger: the founding doctrine selects the route, the Mission 09 allocation selects the two contributing districts, and the Mission 10 protocol is the only one that may be applied.<br>The twenty-seven plans are explicit combinations of doctrine, districts, and protocol; no hidden trust score or survivor roster exists.<br>The inherited route stands at tile 18,30 under Harvest doctrine, 32,30 under Preserve, or 46,30 under Reshape.<br>District contributions integrate at tile 18,35 for Life Support, 32,33 for Transit, and 46,35 for Archive; only the two powered districts contribute.<br>The neutral Meridian evidence interface stands at tile 26,43 and the Kharuun interface at tile 38,43; both must be attested by Oruun and a distinct Kharuun witness.<br>Applying a protocol that conflicts with the Mission 10 record fails the attempt, and the Reshape rally window can expire.<br>Losing the local core, Oruun, the Waystone, the ledger witness, the Well, or a public interface, or reaching a terminal engagement outcome, fails the attempt irreversibly. |
| Prohibited claims | No mixed-faction command is established; the coalition is a bounded local assembly.<br>The Hollow Choir does not become playable and takes no commanded part.<br>No numeric faction trust exists or may be implied.<br>The coalition's composition beyond the assembled units is not modeled; no optional survivor roster exists.<br>Attestation records that evidence was witnessed, not that it is true.<br>The applied protocol carries the Mission 10 record's authority, not a fresh judgment of its rightness. |
| Branch tradeoffs |   |


### The Future That Won


Oruun-of-Seven-Stones and a distinct Kharuun verifier establish independent readback at the neutral public interfaces, verify the two recorded district inputs, bind only the recorded protocol at the separate Future Well, hold its activation stable for the fixed window, and observe both district readbacks — while Chancellor Rhyse is present only as attributable public demonstrator apparatus.


#### SPEC-MSN-012 — The Future That Won

| Contract field | Binding content |
|---|---|
| Player authority | oruun command authority |
| Command faction | kharuun assemblies |
| Briefing | The Chancellor's apparatus demonstrates a future in public, and the record it points at is ours. Establish independent readback at both interfaces, verify the recorded inputs, bind only the recorded protocol, hold the fixed window, and watch both districts read it back. |
| Objectives | Establish independent readback at both neutral interfaces.<br>Verify both recorded district inputs.<br>Bind only the recorded protocol at the separate Well.<br>Hold the activation receipt for the fixed window.<br>Observe both district readbacks with the two scouts. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Oruun-of-Seven-Stones is destroyed while the operation is active.<br>The distinct Kharuun verifier is destroyed.<br>The separate Future Well is destroyed before the readbacks are observed.<br>Either neutral public interface is destroyed.<br>A protocol conflicting with the Mission 10 record is bound at the Well.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the exact eleven-record ledger; its plan is derived from the Mission 11 plan, inheriting doctrine, districts, and the recorded protocol.<br>Independent readback is established by Oruun and a distinct Kharuun verifier at the neutral Meridian interface at tile 26,43 and the Kharuun interface at tile 38,43.<br>The two recorded district inputs are the exact sites the Mission 09 allocation powered, carried through the Mission 11 plan.<br>The separate Future Well accepts only the Mission 10 recorded protocol, and binding a conflicting protocol fails the attempt.<br>The activation receipt must hold for exactly 300 fixed ticks.<br>Completion requires both district readbacks observed by the two scouts after the window holds.<br>Losing the local core, Oruun, the verifier, the Well, or a public interface, or reaching a terminal engagement outcome, fails the attempt irreversibly. |
| Prohibited claims | Chancellor Rhyse is represented only by attributable neutral public demonstrator apparatus; no personal presence, consent, or command is asserted.<br>No population restoration, civilian count, or survival claim follows from the demonstrator or the readbacks.<br>The held window establishes local protocol stability for 300 ticks, not permanence.<br>No trust, consent, or ethical justification between factions is established.<br>The demonstrator's public claims are its own; observation of them does not make them true.<br>No mixed-faction command is established. |
| Branch tradeoffs |   |


### Assembly of the Missing


Oruun-of-Seven-Stones and a distinct Kharuun verifier establish paired readback at the neutral Meridian and Kharuun public-record interfaces, an ordinary worker raises a Listening Spine within reach of the neutral Crownfall public index, and both then complete one public observation receipt from separate assembly witness sites.


#### SPEC-MSN-013 — Assembly of the Missing

| Contract field | Binding content |
|---|---|
| Player authority | oruun command authority |
| Command faction | kharuun assemblies |
| Briefing | The missing have an assembly, and the assembly has no record. Establish paired readback at both public-record interfaces, raise a Listening Spine within reach of the Crownfall index, and complete one public observation receipt from separate witness sites. |
| Objectives | Establish paired readback at both public-record interfaces.<br>Raise a Listening Spine within three tiles of the Crownfall index.<br>Complete one public observation receipt from separate witness sites. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Oruun-of-Seven-Stones is destroyed while the operation is active.<br>The distinct Kharuun verifier is destroyed.<br>Either neutral public-record interface is destroyed.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the exact twelve-record ledger; its sites derive from the Mission 11 and Mission 12 plans.<br>The public-record readback interfaces are the neutral Meridian and Kharuun sites carried from the Mission 12 plan.<br>The Crownfall public index stands at the Mission 11 coalition rally site, and the Listening Spine must rise within three tiles of it.<br>Command authority stays bounded to Oruun; the verifier is a distinct Kharuun unit, and readback is established by both at separate interfaces.<br>After the durable link exists, Oruun and the verifier move to separate assembly witness sites and complete one public observation receipt.<br>The result records public readback, Crownfall linkage, and paired observation only.<br>Losing the local core, Oruun, the verifier, or a public interface, or reaching a terminal engagement outcome, fails the attempt irreversibly. |
| Prohibited claims | No authorship or responsibility for the missing is assigned by the assembly.<br>No consent or trust between factions or with the missing is established.<br>Civilian or survivor state beyond the bounded receipt is not modeled and may not be claimed.<br>The observation receipt is not cryptographic proof of anything beyond its own completion.<br>No mixed-faction command is created; the Hollow Choir is not made playable.<br>The assembly does not resolve the Kharuun memory dispute or the census erasures. |
| Branch tradeoffs |   |


### Several Voices, One Command


Neme holds one bounded Hollow Choir force under local authority: a protected Soldier voice researches Held Alternatives and resolves as Possible at its inherited site, a protected Heavy voice remains Manifest at a separate site, Shared Resolution is researched over both, a worker raises a Phase Anchor at the Crownfall crisis site, and the whole contract holds for 160 fixed ticks.


#### SPEC-MSN-014 — Several Voices, One Command

| Contract field | Binding content |
|---|---|
| Player authority | neme command authority |
| Command faction | hollow choir |
| Briefing | The Choir asks to be commanded, once, precisely: a Soldier voice resolving Possible, a Heavy voice holding Manifest, Neme at the command site, and a Phase Anchor at the crisis. Research both looms, place every voice exactly, and hold the contract for one hundred sixty fixed ticks. |
| Objectives | Research Held Alternatives on the protected Soldier voice.<br>Resolve Possible and Manifest at their separate inherited sites.<br>Research Shared Resolution over the incompatible identities.<br>Raise the Phase Anchor at the Crownfall crisis site.<br>Hold every protected element for the fixed crisis window. |
| Failure causes | The local command core is destroyed while the operation is active.<br>The protected Possible or Manifest voice is destroyed.<br>Neme is destroyed or leaves the command site under contract.<br>The research loom is destroyed.<br>Any protected element or required site is breached after the hold begins; the failure is irreversible even if repaired.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the exact thirteen-record ledger, and its sites derive from the prior plans.<br>Command is granted only over a bounded Hollow Choir force; no other faction is commandable.<br>The protected Soldier voice must research Held Alternatives and resolve as Possible at its inherited site.<br>The protected Heavy voice must remain Manifest at its separate inherited site while the Soldier resolves as Possible.<br>Shared Resolution is unavailable until the incompatible identities and placements are simultaneously authoritative.<br>A worker raises a Phase Anchor at the Crownfall crisis site, and the crisis window holds for exactly 160 fixed ticks.<br>Any breach after the hold begins — a protected voice, Neme, the research loom, the anchor, the local core, or a required site — fails the operation irreversibly, even if the visible state is repaired. |
| Prohibited claims | Completion does not decide the Choir's final fate.<br>No unified permanent Choir identity is established; the incompatibility is held, not resolved away.<br>The crisis contract's completion does not prove broad faction balance.<br>Neme's command is local and bounded; no Choir-wide authority is asserted.<br>The held window does not substitute for Mission 15's endings.<br>No claim about what the voices experience is established, only where they stood and what they resolved. |
| Branch tradeoffs |   |


### The Broken Sun


Neme commands the local Hollow Choir through the Crownfall's final contract while Mara, Oruun, and Talar stand protected as witnesses: secure the approach with an Approach Anchor, assemble the three-faction accord, confirm one eligible ending twice, raise that ending's distinct Resolution Conduit, and hold the exact contract for its route-dependent duration.


#### SPEC-MSN-015 — The Broken Sun

| Contract field | Binding content |
|---|---|
| Player authority | neme command authority |
| Command faction | hollow choir |
| Briefing | The Crownfall is the last contract. Secure the approach, assemble all three accords with the witnesses protected, confirm the earned ending twice — it cannot be unsaid — raise its Resolution Conduit, and hold everything the contract names until the sky believes you. |
| Objectives | Secure the Crownfall approach with an Approach Anchor.<br>Assemble the Meridian, Kharuun, and Choir accords at their witnessed sites.<br>Confirm one eligible ending twice.<br>Raise the selected ending's Resolution Conduit.<br>Hold the final contract for its full route-dependent duration. |
| Failure causes | The local command core is destroyed while the operation is active.<br>Mara, Oruun, Talar, or Neme is destroyed while protected.<br>The bounded Choir command force is broken.<br>The approach anchor, accord, eligibility, or conduit contract is breached after becoming irreversible.<br>The underlying engagement reaches a terminal outcome while objectives remain.<br>Any other failure surface without a bound stable reason. |
| Canonical facts | The mission consumes the exact fourteen-record ledger and retains twenty-seven plans from the doctrine, powered-district, and Lume-protocol axes.<br>The earned endings are an explicit subset: Controlled Stabilization is always available; Restoration requires the Preserve protocol with Life Support powered; Extinguishment requires Harvest doctrine or protocol; Open Evolution requires Reshape doctrine or protocol.<br>The accord sites, approach anchor, and final convergence derive from the prior plans, and the ordered objectives can never be satisfied by one structure.<br>The selected ending must be confirmed twice before its Resolution Conduit may rise, and each ending's conduit stands at a distinct convergence offset.<br>The hold duration is route-dependent: a base of 320, 280, or 240 fixed ticks by deferred district, plus 80 for Restoration, 0 for Controlled Stabilization, 40 for Extinguishment, or 120 for Open Evolution.<br>The force, all three witnesses, the local core, and the nonterminal world are protected from the first fixed step; the anchor, accord, eligibility, and conduit each become irreversible in turn.<br>Any applicable breach fails irreversibly, and success records only the selected ending and its availability context. |
| Prohibited claims | The unchosen endings do not occur and their consequences remain unresolved.<br>No ending is the canonical or correct resolution; the ledger records which was earned and selected, not which was right.<br>Wider social consequences of the selected ending are not modeled and may not be claimed.<br>The witnesses' presence does not assert their factions' consent to the selected ending.<br>The Choir's fate beyond the selected ending's recorded contract is not established.<br>Completion does not turn the campaign's chain of bounded records into a claim of moral resolution. |
| Branch tradeoffs | Restoration: Design target: the held futures return to the ground they were taken from, slowly and guarded, at the longest cost but one.<br>ControlledStabilization: Design target: the Crownfall is held at its current pitch, a managed wound that asks the least and promises the same.<br>Extinguishment: Design target: the remaining held futures are spent to close the Crownfall permanently, silencing what argued through it.<br>OpenEvolution: Design target: the Crownfall is released to continue becoming, unmanaged, with the longest hold of the four and no guarantee attached. |


## 20. Interface, selection, controls, and player feedback

* **SPEC-UI-001 —** Selection answer. Every selection answers: what is it, what is it doing, what can I order, what will that cost or require, why would I choose it, when does it fail, and how can the opponent answer it?
* **SPEC-UI-002 —** Selection fields. Show faction, name, role, owner, health, order, stance, target/route, cargo, control group, status, ability cost/cooldown, and every applicable queue, network, adaptation, identity, coherence, or Well field.
* **SPEC-UI-003 —** Mixed selection. Show composition and deterministic subgroups. The command deck displays only commands legal for at least one selected unit and reports how many will execute.
* **SPEC-UI-004 —** Action feedback. Accepted actions use distinct non-color ground markers and audio. Rejections identify the reason and recovery. Target hit regions match visible silhouettes; a miss never silently becomes Move when a target action was plausible.
* **SPEC-UI-005 —** HUD fiction. The field HUD is Mara's command deck: a Compact operations instrument using ledger entries, duty windows, reserve margins, status bands, and factual annunciator alerts. Faction selections retain their own visual language inside that instrument.

| ID | Area | Required information |
|---|---|---|
| SPEC-HUD-001 | Top resource bar | Matter, Dawn, Logistics, commitments, income, and alerts; opens resource monitor. |
| SPEC-HUD-002 | Objective panel | Primary, optional, protected assets, timers, phase, completion/failure state. |
| SPEC-HUD-003 | Selection panel | Identity, health/state, purpose, strong use, limitation, counterplay, details. |
| SPEC-HUD-004 | Command deck | Context-valid commands, hotkeys, costs, prerequisites, targets, preview, disabled reasons. |
| SPEC-HUD-005 | Production/research | Queues, progress, emergence, rally, reservations, cancellation refund, producer contention. |
| SPEC-HUD-006 | Minimap | Fog states, owned units, visible hostiles, fair contacts, resources, Wells, objectives, alerts, camera. |
| SPEC-HUD-007 | Alert history | Filterable recent alerts with location, time, severity, source, and recovery action. |


### 20.1 Default controls


| ID | Input | Default |
|---|---|---|
| SPEC-CTL-001 | Left click / drag | Select; box-select owned visible units |
| SPEC-CTL-002 | Shift + selection | Add or remove |
| SPEC-CTL-003 | Double click | Select matching visible units in view |
| SPEC-CTL-004 | Right click | Unambiguous context action |
| SPEC-CTL-005 | A / S / H / P / G | Attack-move / Stop / Hold / Patrol / Guard |
| SPEC-CTL-006 | B / R | Worker build menu / Repair |
| SPEC-CTL-007 | Q / W / E | Contextual ability grid; on a Well: Harvest / Preserve / Reshape |
| SPEC-CTL-008 | F | Cycle Aggressive, Defensive, Return Fire, Hold Fire |
| SPEC-CTL-009 | C | Cycle Box, Line, Wedge |
| SPEC-CTL-010 | Ctrl + 1–0 | Assign control group; Shift + number adds selection; number recalls; double-tap centers |
| SPEC-CTL-011 | Tab / Shift+Tab | Next/previous subgroup |
| SPEC-CTL-012 | WASD / edge pan / middle drag / wheel | Camera pan and zoom |
| SPEC-CTL-013 | Space | Jump to most recent spatial alert |
| SPEC-CTL-014 | F1 / F2 / F3 | Idle worker / production structures / combat force |
| SPEC-CTL-015 | Escape / Pause | Menu / pause |

* **SPEC-UI-006 —** Remapping. Every gameplay command is remappable; collisions are rejected before save. Prompts, tutorial, help, and tooltips resolve the active binding. Contextual Q/W/E reuse is allowed only where selection states cannot overlap.

## 21. Onboarding, tutorial, manual, and learning


| ID | Lesson | Player proves |
|---|---|---|
| SPEC-LSN-001 | Survey | Pan, zoom, center, identify own Core and objective. |
| SPEC-LSN-002 | Roster | Select a unit and explain purpose, health, order, and command deck. |
| SPEC-LSN-003 | Section muster | Box-select, modify selection, subgroup, and assign/recall a control group. |
| SPEC-LSN-004 | Route check | Move, context action, stop, patrol, guard, and read a rejection. |
| SPEC-LSN-005 | Reserve | Start and inspect a continuous Matter route and resource monitor. |
| SPEC-LSN-006 | Link restoration | Place, construct, assist, repair, and read operational/network state. |
| SPEC-LSN-007 | Foundry | Produce, queue, cancel, set rally, and understand Logistics reservation. |
| SPEC-LSN-008 | Perimeter probe | Attack-move, focus, stance, cover, retreat, and loss feedback. |
| SPEC-LSN-009 | The board | Use objectives, minimap, alerts, fog states, and last-known information. |
| SPEC-LSN-010 | The Well | Compare, confirm, protect, interrupt, and understand all three protocols. |
| SPEC-LSN-011 | Readiness gate | Complete an independent mini-operation without step-by-step commands. |

* **SPEC-TUT-001 —** Fiction. Mara conducts an operational readiness check inside the prologue. There is no narrator. The tutorial never breaks world tone or uses unexplained development vocabulary.
* **SPEC-TUT-002 —** Hint ladder. After no relevant state change, first highlight the relevant UI, then restate the goal, then demonstrate the input without completing it. Hints adapt to remapped controls and may be disabled.
* **SPEC-TUT-003 —** Mastery. A lesson completes only from authoritative player action, not elapsed time or opening a tooltip. It can be replayed individually from Help.
* **SPEC-TUT-004 —** Reference. A searchable in-game archive defines units, buildings, resources, terrain, technologies, Wells, controls, objectives, status effects, counters, and campaign records using the same terms as the HUD.

## 22. Accessibility and localization

* **SPEC-ACC-001 —** Non-color communication. Ownership, state, command, alert, danger, route, and protocol use shape, text, motion, and sound redundancy; color is never the sole channel.
* **SPEC-ACC-002 —** Visual settings. HUD scale 80–150%; text scale; high contrast; color-vision presets; reduced motion; reduced flashing; subtitle size/background; cursor scale; camera shake; effect density.
* **SPEC-ACC-003 —** Audio settings. Separate master, music, effects, dialogue, interface, and ambience controls; reduced dynamic range; mono compatibility; directional indicators for critical cues.
* **SPEC-ACC-004 —** Input. Full keyboard-only menu and field path, complete remapping, edge-pan toggle, camera-speed/zoom controls, hold/toggle options, and no timing-sensitive double press without adjustable window.
* **SPEC-ACC-005 —** Cognition. Pause, tutorial replay, glossary, persistent objective history, clear confirmations, stable error language, and optional simplified alert density.
* **SPEC-LOC-001 —** Externalized text. No player-facing string is hard-coded. Every string has an identifier, context, owner, length budget, plural/gender notes where relevant, and 30% layout expansion allowance.
* **SPEC-LOC-002 —** English launch. English is the release language. Fonts, layout, input, saves, and content pipelines accept later localization without changing gameplay authority.

## 23. Visual direction, animation, and effects


| ID | Discipline | Binding direction |
|---|---|---|
| SPEC-VISD-001 | Style | Stylized realism: believable material and load, simplified value structure, deliberate silhouettes, restrained surface noise at RTS distance. |
| SPEC-VISD-002 | Palette | Charcoal and vitrified black ground; pale ceramic; broken-sun amber; magenta fracture; cyan-white Matter. Combat readability controls saturation and contrast. |
| SPEC-VISD-003 | Meridian form | Orthogonal frames, plates, rails, exposed load paths, redundant conduits, modular hardpoints, and readable status bands. |
| SPEC-VISD-004 | Kharuun form | Grown mineral-organic facets, strata, cavities, and maintained living surfaces without primitive or tribal coding. |
| SPEC-VISD-005 | Choir form | Repeated near-identical geometry, offset silhouettes, luminous edges, and locally contradictory alignment that remains target-readable. |
| SPEC-VISD-006 | World | Terrain silhouette and route shape communicate gameplay first. Decorative dressing never obscures units, markers, footprints, projectiles, or construction validity. |
| SPEC-VISD-007 | Effects | Every effect identifies source, faction, target, radius/facing, start, active state, expiry, and result. Reduced-motion/flashing variants preserve timing and information. |

* **SPEC-ART-001 —** Unit readability. At normal gameplay camera, faction, role, ownership, selection, health band, order, and high-impact ability state are identifiable within one second under combat load.
* **SPEC-ART-002 —** Animation set. Each unit has idle, locomotion, turn, acquire, wind-up, attack, recovery, hit, ability, state transition, death, and selection acknowledgment as applicable. Workers add gather, carry, deliver, construct, and repair. Buildings add construction, operational, offline, produce/research, damaged, and destruction states.
* **SPEC-ART-003 —** Camera framing. Cinematics may use closer detail, but all gameplay assets must be authored first for tactical camera legibility. No cinematic model or effect may imply a player ability absent from gameplay.

## 24. Audio, music, voice, and cinematics


| ID | Family | Direction |
|---|---|---|
| SPEC-AUDF-001 | Meridian music | Measured pulse, prepared piano, restrained brass, and mechanical resonance. |
| SPEC-AUDF-002 | Kharuun music | Interlocking rhythms and resonant stone/ceramic timbres without generic tribal coding. |
| SPEC-AUDF-003 | Choir music | Harmony resolves in more than one direction before committing; temporal displacement remains rhythmic and intelligible. |
| SPEC-AUDF-004 | Effects | Material truth: engineered collapse, mineral fracture, temporary coherence, Matter handling, weapon roles, routes, and Well states are audible and faction-readable. |
| SPEC-AUDF-005 | Interface | Brief modern confirmation and alert grammar; no retro bleeps; rate limiting preserves critical information. |
| SPEC-AUDF-006 | Voice | Character performances follow identity, motivation, relationship, pace, and emotional beat. Mara teaches; the Annunciator reports state; no narrator. |

* **SPEC-AUD-001 —** Mix. Target integrated loudness is −16 LUFS ±1 with true peak no higher than −1 dBTP. Dialogue remains intelligible over combat; side-chain reduction is subtle and state-driven.
* **SPEC-AUD-002 —** Cue completeness. Every command, rejection, production, research, construction, resource, Logistics, alert, combat role, ability, terrain transformation, Well protocol, objective, result, and menu transition has a defined cue or an explicit intentional-silence requirement.
* **SPEC-AUD-003 —** Accessibility. Critical audio always has simultaneous visual/text information. Subtitle timing follows performance, identifies speaker, supports size/background settings, and never exposes hidden information.
* **SPEC-CIN-001 —** Cinematic set. The game includes a title sequence, campaign opening, act transitions, mission-critical in-engine scenes, and one ending cinematic per resolution. Cinematics are skippable after first start, pausable, subtitled, and replayable.
* **SPEC-CIN-002 —** Control handoff. A cinematic states when control is removed, returns camera and selection safely, never advances combat invisibly, and resumes at a deterministic authored state.

## 25. Saves, campaign records, and replays

* **SPEC-SAV-001 —** Transactional save. Manual save, autosave, checkpoint, settings, campaign ledger, and replay write atomically through temporary file, validation, and replace. A failed write preserves the prior valid generation.
* **SPEC-SAV-002 —** Slots. Campaign provides three named journeys plus autosave and one prior validated generation. Skirmish provides quicksave and manual saves. Delete requires confirmation and recovery information.
* **SPEC-SAV-003 —** Compatibility. A save records schema, content identifiers, map, mission, seed, difficulty, campaign ledger, entities, orders, queues, timers, AI state, fog, alerts, and deterministic checksum. Unsupported saves fail without mutation and explain why.
* **SPEC-SAV-004 —** Replay. Every match and completed operation may produce a deterministic command replay with setup, content hash, seed, inputs, periodic checksums, outcome, and branch context. Replays are local and contain no observer or multiplayer authority.
* **SPEC-SAV-005 —** Replay browser. Filter by mode, operation/map, faction, result, date, and duration; inspect version-independent content identifiers; play, pause, seek from checkpoints, change camera, and exit safely.

## 26. Technical architecture and content contracts


| ID | Module | Responsibility and boundary |
|---|---|---|
| SPEC-MOD-001 | Simulation core | Platform-independent deterministic rules; no rendering, audio, wall-clock, filesystem, network, or engine-object authority. |
| SPEC-MOD-002 | Game adapter | Translates validated input into commands and authorized views into Unreal actors, animation, effects, audio, UI, and camera. |
| SPEC-MOD-003 | Content compiler | Validates authored JSON schemas, stable IDs, references, units, buildings, technologies, factions, maps, missions, dialogue, and assets; emits deterministic compiled data and hashes. |
| SPEC-MOD-004 | AI | Consumes player-scoped observations and issues ordinary commands. Strategic/tactical state is serializable and replay-safe. |
| SPEC-MOD-005 | Mission director | Consumes mission contract and public simulation events; issues labeled authored events with idempotent trigger records. |
| SPEC-MOD-006 | Save/replay | Serializes authoritative state and command history transactionally; validates schema/content before load. |
| SPEC-MOD-007 | Presentation | Never mutates authority; supports quality scaling, accessibility, fair fog, pooling, and graceful missing-asset failure. |

* **SPEC-ARC-001 —** Data ownership. Faction, unit, building, technology, Well, map, mission, dialogue, localization, and asset metadata live in validated source files. Generated or compiled outputs are never hand-edited.
* **SPEC-ARC-002 —** Trust boundaries. Save files, replay files, settings, and content packs are untrusted input. Validate size, schema, identifiers, numeric bounds, references, paths, and checksums before allocation or mutation.
* **SPEC-ARC-003 —** Failure behavior. Missing or invalid optional presentation assets use a registered fallback and visible diagnostics outside release UI. Missing authoritative content prevents start with a stable error; it never substitutes different gameplay silently.

## 27. Performance, stability, and platform


| ID | Budget | Acceptance target |
|---|---|---|
| SPEC-BUD-001 | Frame target | 60 fps at 1920×1080 Medium on Apple M1 Pro 16 GB; 30 fps at 1280×720 Low on base M1 8 GB. |
| SPEC-BUD-002 | Frame time | p95 ≤16.67 ms at baseline; game thread ≤4.0 ms; render plus GPU ≤11.0 ms. |
| SPEC-BUD-003 | Fog | ≤1.5 ms baseline p95. |
| SPEC-BUD-004 | Path burst | ≤6.0 ms for accepted stress burst without sustained hitching. |
| SPEC-BUD-005 | Memory | Resident memory ≤10 GB on baseline and ≤6.5 GB on minimum profile. |
| SPEC-BUD-006 | Scale | 400-unit/four-force stress scene remains controllable; launch 1v1 design targets 200 simultaneous controllable units. |
| SPEC-BUD-007 | Save | Ordinary save initiation ≤250 ms without corrupting active play; background completion reports success/failure. |
| SPEC-BUD-008 | Stability | 60-minute rendered match, multi-hour AI soak, repeated save/load/restart, and clean exit with no crash, hang, leak trend, or deterministic divergence. |

* **SPEC-PLAT-001 —** Display matrix. 1280×720, 1440×900, 1600×900, 1920×1080, 2560×1440, native Retina, windowed, fullscreen, and live resize remain readable and operable at every supported HUD scale.
* **SPEC-PLAT-002 —** Graphics scaling. Low, Medium, High, and Auto alter texture, shadow, effects, foliage, post-processing, and resolution scale without changing visibility authority, collision, targetability, terrain truth, or simulation.
* **SPEC-PLAT-003 —** Distribution. Ship a Release configuration app signed with Developer ID, notarized, stapled, installable by an ordinary user, removable without orphaned privileged components, and qualified on a clean supported Mac.
* **SPEC-PLAT-004 —** Privacy. The game operates offline, creates no account, sends no telemetry by default, and stores saves, settings, logs, and replays locally. Any future diagnostic upload requires explicit user selection and preview.

## 28. Team ownership and development sequence


| Discipline | Primary ownership |
|---|---|
| Creative/game director | Document authority, product scope, pillars, gameplay decisions, acceptance and change control. |
| Systems design | Economy, combat, units, buildings, technology, Wells, terrain, balance, rules and tuning. |
| Narrative/campaign | Canon, characters, mission contracts, branches, persistence, dialogue, cinematics, ending causality. |
| Simulation engineering | Deterministic rules, commands, pathing, combat, fog, economy, saves, replay and validation. |
| AI engineering | Fair observation, doctrines, strategy, tactics, scouting, recovery, difficulty and mission integration. |
| UI/UX and accessibility | Information architecture, controls, feedback, onboarding, settings, localization and usability. |
| Level/world design | Map contracts, resources, routes, terrain, passages, Well outcomes, objectives and encounter pacing. |
| Art/animation/VFX | Visual language, assets, readability, animation states, effects, cinematics and performance LODs. |
| Audio/voice/music | Sonic identity, cue matrix, mix, character performance, subtitles and accessibility. |
| QA/release | Traceability, test automation, packaged physical play, compatibility, performance, security, distribution and evidence. |


### 28.1 Build order


| Phase | Exit condition |
|---|---|
| Foundation | Content schemas, deterministic simulation, commands, save/replay skeleton, test harness, player-scoped view. |
| Core RTS | Selection, input, camera, movement, terrain, fog, economy, construction, production, combat, outcomes. |
| Faction slice | Meridian and Kharuun rosters, abilities, structures, AI, Glass Scar, Well protocols, tutorial path. |
| Strategic breadth | Choir, remaining maps, technologies, reconnaissance, resource monitor, full AI doctrines/difficulty. |
| Campaign | Persistent ledger, all mission contracts, rewards, branches, dialogue, cinematics, four endings. |
| Presentation | Production art, animation, VFX, UI, audio, voice, music, accessibility, localization readiness. |
| Qualification | Balance, performance, soak, save migration, adversarial testing, clean-machine package, documentation. |
| Release | Frozen candidate, signed/notarized distribution, support materials, owner authorization. |


## 29. Requirement acceptance-card template


| Field | Required content |
|---|---|
| Requirement ID and title | Stable identity and one testable behavior. |
| Player purpose | Why the behavior exists and what decision it supports. |
| Preconditions and inputs | Ownership, resources, state, targets, map, bindings, and setup. |
| Authoritative result | Exact state transition, timing, calculation, persistence, and deterministic result. |
| Presentation | UI, animation, VFX, audio, text, minimap, accessibility, and localization behavior. |
| Failures and degraded states | Every rejection, interruption, loss, blocked state, recovery, and attribution. |
| AI and campaign behavior | Observation boundary, decision use, director interaction, and difficulty implications. |
| Save/replay | Serialized fields, load restoration, replay equivalence, migration, and corruption behavior. |
| Verification | Unit/system tests, adversarial cases, performance, packaged physical interaction, rendered inspection, human comprehension, owner acceptance. |
| Dependencies and change impact | Upstream/downstream requirements, content, interfaces, risks, rollback, and documentation. |


## 30. Verification and validation strategy


| ID | Evidence class | What it establishes |
|---|---|---|
| SPEC-EVID-001 | Static/schema | IDs, references, ranges, ownership, content completeness, and prohibited dependencies. |
| SPEC-EVID-002 | Deterministic unit/system | Authoritative calculations, transitions, failure reasons, fog, save/replay, and seeded equivalence. |
| SPEC-EVID-003 | Adversarial | Hidden-information leakage, invalid input, corrupted files, blocked routes, interruption, concurrency, and exploit resistance. |
| SPEC-EVID-004 | Packaged physical play | Real input, selection, command, camera, tutorial, saves, missions, results, and installer behavior. |
| SPEC-EVID-005 | Rendered/audio inspection | Readability, clipping, animation/VFX truth, mix, dialogue, subtitles, accessibility, and quality modes. |
| SPEC-EVID-006 | Uncoached player testing | Comprehension, strategic usefulness, onboarding, frustration, recovery, and ability to explain win/loss and roster purpose. |
| SPEC-EVID-007 | Balance | Map/start fairness, faction/doctrine matchup distribution, duration, openings, counterplay, and no dominant choice. |
| SPEC-EVID-008 | Owner acceptance | Creative quality, scope, consequence, presentation, and authorization of the same frozen candidate. |

* **SPEC-VAL-001 —** Comprehension floor. At least four of five uncoached representative players correctly explain each release roster element's purpose, best use, limitation, counterplay, and primary action. All five identify Corefall win and loss before the first skirmish.
* **SPEC-VAL-002 —** Balance floor. Across the approved automated-plus-human test set, no non-mirror Standard faction matchup lies outside 40–60% without an accepted design reason, and no spawn position changes win rate by more than five percentage points. Report sample size and uncertainty; do not treat this as the only balance evidence.
* **SPEC-VAL-003 —** No purposeless content. Every accessible element passes AUTH-005. A fiction-only verb, dead control, unexplained state, decorative false affordance, or mechanic with no counterplay blocks release.

## 31. Final definition of done


| RELEASE CONDITION<br>The game is complete only when every mandatory requirement in this document is implemented, integrated, tested at its authoritative and presentation layers, playable through all fifteen operations and every ending, balanced across the approved matrix, accessible, stable, packaged, signed, notarized, documented, human-accepted on the same frozen candidate, and explicitly authorized for release by Angelis Pseftis. |
|---|

* No placeholder, debug surface, development language, dead control, missing cue, unstyled state, or unowned requirement remains player-accessible.
* Every unit, building, ability, technology, resource, terrain rule, Well choice, objective, alert, and map interaction has complete purpose, presentation, failure, AI, save/replay, and counterplay coverage.
* Campaign consequences and endings derive from visible recorded choices; no hidden moral score or unsupported narrative claim appears.
* Performance, stability, compatibility, privacy, security, rights, provenance, clean-machine installation, recovery, support, and truthful public documentation have passed their acceptance cards.
* Any change after candidate freeze reopens every acceptance card whose inputs, outputs, assets, data, timing, UI, AI, or evidence are affected.


---

# Part II — Demo readiness (`DEMO-*`)

Authority: `Docs/DemoRecoveryDirective.md` (owner directive, 2026-09-02). Owner and final acceptance
authority: Angelis Pseftis. Bodies migrated verbatim from `DemoReadinessRequirements.md` (retired
2026-09-03), which was the sole `DEMO-*` requirements and acceptance ledger. Global verdicts, record
defaults, per-requirement state, and the change log live in `RequirementsState.md`.

## A. Scope, integrity, and traceability (owner: Coordinator+QA; verify: SRC + ledger audit)

* DEMO-GOV-001 — The current demo shall remain classified as `HUMAN REJECTED` until I accept a later identified packaged build.
* DEMO-GOV-002 — Every implementation task, commit, test, capture, and defect shall map to one or more requirement IDs.
* DEMO-GOV-003 — Every evidence claim shall identify the exact commit, dirty or clean tree state, package, operating system, hardware, resolution, and graphics preset.
* DEMO-GOV-004 — Source code, tests, editor demonstrations, screenshots, and packaged human play shall remain separate evidence classes.
* DEMO-GOV-005 — Existing automated or headless campaign evidence shall not be represented as rendered human-play evidence.
* DEMO-GOV-006 — Every player-visible asset shall have recorded authorship, generation, licensing, and integration provenance.
* DEMO-GOV-007 — No unfinished option shall be presented as available. Incomplete functionality shall be completed, clearly identified as unavailable, or removed from the demo path with my approval.
* DEMO-GOV-008 — The demo shall contain no visible debug commands, prototype instructions, default engine assets, placeholder geometry, temporary icons, or knowingly dead controls.
* DEMO-GOV-009 — Genre references shall guide interaction quality and design discipline without copying protected expression.
* DEMO-GOV-010 — A requirement shall not be called complete until its evidence is ready and I explicitly accept it.

## B. Complete player journey (owner: Player+Campaign; verify: PKG-PHYS + HUM + OWNER)

* DEMO-JRN-001 — A clean first-time profile shall complete the entire golden path without a terminal, editor, developer console, cheat, state injection, or developer coaching.
* DEMO-JRN-002 — Every required menu and transition shall be usable with a physical mouse.
* DEMO-JRN-003 — First-time players shall complete the tutorial before the full AI demo unlocks.
* DEMO-JRN-004 — After tutorial completion, replay and approved skip behavior may become available for later sessions.
* DEMO-JRN-005 — Loading, failure, cancellation, back-navigation, restart, and return-to-menu paths shall have clear visible states and shall not dead-end.
* DEMO-JRN-006 — The demo shall end with a complete victory or defeat result and understandable replay, restart, and exit choices.
* DEMO-JRN-007 — The player shall never need an external manual, developer explanation, or hidden control to complete the intended demo journey.

## C. Opening story and player orientation (owner: Narrative+Visual+Audio; verify: PKG-REND + HUM + OWNER)

* DEMO-NAR-001 — The first launch shall present a polished title and opening sequence before normal gameplay.
* DEMO-NAR-002 — The opening shall establish the broken world of Soryn, the Crownfall, and the immediate situation without contradicting the Development Bible.
* DEMO-NAR-003 — The opening shall tell the player who they are, what role they occupy, what immediate problem they face, what they must do next, and why it matters.
* DEMO-NAR-004 — The opening shall use authored in-engine visuals, motion, lighting, voice-over, exact subtitles, music, ambience, and deliberate transitions. A silent flyover, static text card, storyboard, or lore dump does not pass.
* DEMO-NAR-005 — The opening should remain focused enough to preserve player attention; the proposed maximum is 90 seconds unless I approve another duration.
* DEMO-NAR-006 — The sequence shall support pause, accessible subtitle controls, replay, and approved skip behavior without losing required gameplay information.
* DEMO-NAR-007 — The transition from cinematic to playable tutorial shall be coherent and shall immediately connect the story problem to the player's first action.
* DEMO-NAR-008 — At least four of five uncoached, project-naive testers shall be able to explain the player's identity, immediate situation, first objective, and why it matters. (Verify: HUM)
* DEMO-NAR-009 — I shall personally accept the opening's story clarity, emotional tone, pacing, visual direction, and ability to create interest in continuing. (Verify: OWNER)

* **DEMO-NAR-010 —** Before voice production, every speaking character and system voice in the demo shall have a designed identity: who they are in the story, their role, personality, motivations, speech patterns, and relationship to the player — such that the player connects with them (someone they want to be, help, or listen to). Voices shall match the designed character or system identity. Voice design (AUD-004 redo) depends on this and is sequenced after it. Verify: OWNER acceptance of the character bible; HUM comprehension/connection signals at DEMO-NAR-008/VAL sessions. *(Owner-added 2026-09-02; body recovered from the change log during consolidation, where it had never been seated in a section.)*
* **DEMO-NAR-011 —** a full review of everything in the game — story, setting, characters, missions/campaign, every screen element, mechanic, sound, and interaction — answering WHY it exists and how it ties into the storyline. Elements without a story/world justification are flagged for redesign, rejustification, or removal. The review's output is the design foundation that informs how everything looks, acts, sounds, and feels; presentation work (UI remake, art direction, audio direction, mission staging) shall trace to it. Output: one owner-reviewed document (`Docs/NarrativeCoherenceReview.md`), Campaign-led with per-lane contributions. Sequenced with DEMO-NAR-010; both precede large-scale presentation/voice production. *(Owner-added 2026-09-02; body recovered from the change log during consolidation, where it had never been seated in a section.)*

## D. Progressive tutorial and demonstrated learning (owner: Campaign+Player; verify: PKG-PHYS + HUM + OWNER)

Lesson cycle (binding for every lesson): Explain → highlight or demonstrate → allow the player to act → verify the real game state → acknowledge success → explain why it mattered → unlock the next lesson.

* DEMO-TUT-001 — The tutorial shall assume no prior RTS knowledge.
* DEMO-TUT-002 — It shall begin in a safe, low-pressure situation and introduce one coherent concept at a time.
* DEMO-TUT-003 — It shall teach camera movement, zoom, recentering, and navigation.
* DEMO-TUT-004 — It shall teach left-click selection, deselection, selection feedback, and how to identify the selected entity.
* DEMO-TUT-005 — It shall teach drag-box and multi-selection before requiring management of multiple units.
* DEMO-TUT-006 — It shall identify every introduced unit, building, resource, objective, and interface area by name, function, available action, and tactical reason for mattering.
* DEMO-TUT-007 — It shall teach move, contextual right-click commands, attack, attack-move, stop, and other commands required by the demo.
* DEMO-TUT-008 — It shall teach resource identification, gathering, delivery, current totals, and what the resources enable.
* DEMO-TUT-009 — It shall teach valid and invalid building placement, construction, building purpose, and completion feedback.
* DEMO-TUT-010 — It shall teach unit production, costs, prerequisites, queues, rally behavior, and unit roles.
* DEMO-TUT-011 — It shall teach basic force composition and combat against a controlled initial threat.
* DEMO-TUT-012 — It shall teach objectives, minimap use, alerts, and navigation to important events.
* DEMO-TUT-013 — It shall introduce the Future Well mechanic and clearly explain the available choice, immediate effect, long-term consequence, and strategic reason it matters.
* DEMO-TUT-014 — Tutorial instructions shall be presented through synchronized voice and readable text using the player's current bindings.
* DEMO-TUT-015 — A step shall advance only after the game verifies that the player performed the required action and achieved the required state. Timers, dismissed text, scripted automation, or entering a trigger volume alone do not demonstrate learning.
* DEMO-TUT-016 — Incorrect actions shall produce understandable feedback without punishing a new player unfairly.
* DEMO-TUT-017 — Contextual hints shall escalate after hesitation or repeated failure without completing the action for the player.
* DEMO-TUT-018 — Every step shall have recovery, retry, reset, save, and resume behavior that prevents a soft lock.
* DEMO-TUT-019 — The tutorial shall not introduce unexplained controls or mechanics immediately after claiming the fundamentals are learned.
* DEMO-TUT-020 — Tutorial completion shall transition naturally into the full AI portion of the demo.
* DEMO-TUT-021 — At least four of five project-naive testers shall complete the tutorial without verbal coaching. (Verify: HUM)
* DEMO-TUT-022 — I shall personally accept the tutorial's pacing, clarity, instructional quality, and mastery threshold. (Verify: OWNER)

## E. Mouse, keyboard, and interaction behavior (owner: Player; verify: PKG-PHYS + OWNER)

* DEMO-INP-001 — Every visible title, menu, settings, pause, tutorial, gameplay, results, confirmation, and error-dialog control shall work with mouse hover and click.
* DEMO-INP-002 — Left click shall select valid units and buildings; clicking empty terrain shall clear selection when appropriate.
* DEMO-INP-003 — Dragging shall create a predictable selection box with visible feedback.
* DEMO-INP-004 — Shift modification, double-click selection, and multi-selection shall behave consistently where supported.
* DEMO-INP-005 — Right click shall issue the correct contextual command, including move, attack, gather, repair, enter, or interact when applicable.
* DEMO-INP-006 — Clickable command-card actions shall perform the same real action as their displayed keyboard shortcuts.
* DEMO-INP-007 — The cursor and target indicators shall communicate valid commands, invalid targets, placement state, interaction state, and cancellation.
* DEMO-INP-008 — Mouse-wheel zoom, edge or configured mouse pan, keyboard camera movement, and recentering shall be smooth and configurable.
* DEMO-INP-009 — Attack-move, stop, hold, control groups, queued commands, and other displayed RTS shortcuts shall function consistently.
* DEMO-INP-010 — All required actions shall be remappable, conflict-checked, resettable, persisted, and immediately reflected in tutorial prompts and tooltips.
* DEMO-INP-011 — Escape, cancel, pause, back-navigation, window focus changes, and input-mode transitions shall behave predictably.
* DEMO-INP-012 — Accepted and rejected commands shall receive immediate visual and audible acknowledgment.
* DEMO-INP-013 — No normal player path shall depend on a keyboard-only fallback because mouse interaction is broken.
* DEMO-INP-014 — A packaged-build interaction matrix shall verify every control at all supported window modes and target resolutions. Calling event handlers directly does not satisfy this requirement.
* DEMO-INP-015 — I shall physically test and accept the mouse, keyboard, menu, camera, selection, command, and remapping behavior. (Verify: OWNER)

## F. Menu, HUD, and UX redesign (owner: Player+Visual; verify: PKG-REND + PKG-PHYS + OWNER)

* DEMO-UI-001 — The existing prototype-like UI shall be replaced by one coherent, original RTS interface system, not merely recolored.
* DEMO-UI-002 — The front door shall clearly present the guided demo, continue when valid, skirmish, options, accessibility, credits, and exit behavior appropriate to the accepted demo scope.
* DEMO-UI-003 — Every menu option shall provide a concise plain-language explanation on hover and keyboard focus.
* DEMO-UI-004 — Hovered, focused, pressed, selected, disabled, loading, warning, error, and confirmed states shall be visually distinct. Disabled controls shall explain why.
* DEMO-UI-005 — The HUD shall clearly present resources, capacity, objectives, alerts, selected entities, health, status, production, abilities, and available commands.
* DEMO-UI-006 — Unit and building panels shall explain identity, role, cost, prerequisites, current state, available actions, and why the entity matters.
* DEMO-UI-007 — Tooltips shall include the action, consequence, current hotkey, cost, prerequisites, and reason an unavailable action cannot be used.
* DEMO-UI-008 — The minimap shall clearly distinguish terrain, ownership, allies, enemies, objectives, alerts, and the current camera location using color and non-color cues.
* DEMO-UI-009 — Tutorial prompts and objectives shall remain readable without obscuring the play area or competing with other critical information.
* DEMO-UI-010 — Results screens shall clearly explain the outcome and provide working replay, restart, and return choices.
* DEMO-UI-011 — The interface shall be inspected at 1280×720, 1440×900, 1600×900, 1920×1080, and 2560×1440, with no clipped, overlapping, unreadable, or unreachable controls.
* DEMO-UI-012 — Debug overlays, prototype instructions, engine-default styling, and internal validation text shall not appear in the public demo path.
* DEMO-UI-013 — I shall personally accept the UI's appearance, hierarchy, readability, discoverability, responsiveness, and consistency with the game universe. (Verify: OWNER)

## G. Audio, voice, and cinematic sound (owner: Audio; verify: PKG-REND listened + OWNER)

* DEMO-AUD-001 — No player-facing scene or required action shall be unintentionally silent.
* DEMO-AUD-002 — The title, menus, opening, tutorial, gameplay, combat, results, victory, and defeat shall have appropriate original music and ambience.
* DEMO-AUD-003 — The opening and tutorial shall contain directed, final-demo-quality voice performances with synchronized subtitles.
* DEMO-AUD-004 — Proposed character and narrator voice profiles shall receive my listening approval before large-scale generation or final integration. (OWNER decision gate)
* DEMO-AUD-005 — Locally generated voice may satisfy the requirement only when its model, license, profile, performance, pronunciation, artifacts, mix, and final in-game result are accepted. Raw or unreviewed TTS is not final voice.
* DEMO-AUD-006 — Hover, selection, confirmation, rejection, menu transitions, alerts, and objective updates shall provide suitable interface feedback.
* DEMO-AUD-007 — Movement orders, attacks, impacts, damage, destruction, gathering, construction, production, abilities, and Future Well interactions shall have functioning audio appropriate to their material and faction.
* DEMO-AUD-008 — Music and ambience shall respond coherently to cinematic, exploration, tension, combat, victory, and defeat states without abrupt or broken transitions.
* DEMO-AUD-009 — Voice shall remain intelligible over music, ambience, and combat. Mixing shall meet the project's approved loudness, peak, and ducking requirements.
* DEMO-AUD-010 — Music, voice, effects, interface, and ambience volumes shall be independently adjustable and persistent. Voice-off shall preserve all required information through text.
* DEMO-AUD-011 — Subtitle text shall match the spoken meaning and support accepted size and background controls.
* DEMO-AUD-012 — Audio files merely existing in the project shall not count; their correct triggering, routing, spatial behavior, transitions, and mix shall be verified in the packaged build.
* DEMO-AUD-013 — I shall listen to and accept the opening, tutorial, menu, representative gameplay, and result-state audio from the candidate package. (Verify: OWNER)

## H. Art, animation, and battlefield readability (owner: Visual+World; verify: PKG-REND at gameplay camera + HUM + OWNER)

* DEMO-VIS-001 — The demo path shall use one coherent original visual language derived from Soryn, its factions, and the Development Bible.
* DEMO-VIS-002 — Terrain shall use sufficiently detailed materials, landmarks, elevation cues, boundaries, and environmental dressing to communicate place and gameplay function.
* DEMO-VIS-003 — Friendly units, enemies, unit classes, and factions shall have distinct silhouettes, scale, materials, motion, and non-color identity cues.
* DEMO-VIS-004 — Buildings shall communicate faction, purpose, operational state, construction state, damage, and production activity at ordinary gameplay distance.
* DEMO-VIS-005 — Resources, objectives, Future Wells, pathable areas, blocked areas, hazards, and interactive locations shall be immediately distinguishable.
* DEMO-VIS-006 — Selection, ownership, health, command, target, damage, and threat states shall remain readable during representative combat.
* DEMO-VIS-007 — Introduced units and buildings shall have credible idle, movement, work, attack, construction, production, damage, and destruction presentation as applicable.
* DEMO-VIS-008 — Effects, lighting, fog, shadows, and atmosphere shall improve the scene without hiding tactical information.
* DEMO-VIS-009 — Visual acceptance shall be judged at the normal RTS camera height and during motion, not only through close-up screenshots or isolated asset previews.
* DEMO-VIS-010 — No placeholder cubes, primitive stand-ins, default materials, temporary icons, missing portraits, or visually unintegrated assets shall remain on the accepted demo path.
* DEMO-VIS-011 — Greater geometric or texture detail alone shall not pass if new players still cannot distinguish entities and gameplay states.
* DEMO-VIS-012 — At least four of five project-naive testers shall correctly identify representative allies, enemies, buildings, resources, objectives, and interactable locations. (Verify: HUM)
* DEMO-VIS-013 — I shall personally accept the terrain, units, buildings, animation, effects, visual hierarchy, and overall presentation. (Verify: OWNER)

## I. Automatic graphics calibration and performance (owner: Performance+Build; verify: PKG-REND measured + OWNER)

* DEMO-PERF-001 — The implementation shall identify the exact CPU, GPU, memory, display, operating system, and relevant rendering capabilities without assuming the developer's exact M1 model.
* DEMO-PERF-002 — On first run, Auto quality shall execute a representative rendering benchmark or calibration rather than selecting a preset solely from a device-name table.
* DEMO-PERF-003 — The game shall provide understandable Auto, Low, Medium, High, and highest-supported presets with clear descriptions of performance and visual consequences.
* DEMO-PERF-004 — Auto shall choose a conservative stable starting configuration based on measured performance and shall record why that configuration was selected.
* DEMO-PERF-005 — Manual overrides shall work, persist, reset correctly, and not be silently replaced by Auto.
* DEMO-PERF-006 — Stronger supported hardware shall receive materially higher fidelity through appropriate resolution, textures, effects, shadows, view distance, density, or related settings.
* DEMO-PERF-007 — Lower settings shall preserve every tactically important cue and remain visually acceptable rather than removing information required to play.
* DEMO-PERF-008 — On the developer's verified M1-class MacBook Pro, the proposed target is p95 frame time at or below 16.67 ms at the Auto-selected resolution and preset. A lower target requires my approval.
* DEMO-PERF-009 — Performance evidence shall report frame-time distributions, game thread, render thread, GPU, memory, resolution, preset, thermals where available, and representative unit/combat load. Menu-only or average-FPS results do not pass.
* DEMO-PERF-010 — The candidate shall survive at least a 30-minute rendered demo session without crash, progressive memory growth, thermal collapse, lost audio, severe stutter, input failure, or visual corruption.
* DEMO-PERF-011 — Shader compilation, asset streaming, resolution changes, fullscreen changes, and settings application shall not create an unrecoverable state.
* DEMO-PERF-012 — A safe graphics fallback shall recover after a settings-related crash or failed launch.
* DEMO-PERF-013 — Higher-end qualification requires execution on an actual materially stronger supported computer. Simulated settings or theoretical scalability do not prove hardware support. (External hardware — owner-gated)
* DEMO-PERF-014 — The current platform boundary remains macOS Apple Silicon unless I approve expansion. Do not claim Windows, Linux, or discrete-GPU qualification without a package and direct evidence from that platform.
* DEMO-PERF-015 — I shall accept the visual-quality/performance tradeoff on the baseline Mac and any higher-capability system used for demo qualification. (Verify: OWNER)

## J. AI skirmish and complete match lifecycle (owner: Opponent-AI; verify: PKG-PHYS + HUM + OWNER)

* DEMO-AI-001 — First-time players shall enter the AI skirmish only after completing the required tutorial mastery gates.
* DEMO-AI-002 — The skirmish setup shall clearly explain map, faction, AI personality or difficulty, starting conditions, victory conditions, and game speed.
* DEMO-AI-003 — Every option presented as selectable shall change the match as described and shall be operable by mouse and keyboard.
* DEMO-AI-004 — The AI shall gather resources, construct, produce units, respond to threats, expand or reposition where appropriate, attack, defend, and reach victory or defeat through actual gameplay systems.
* DEMO-AI-005 — Standard AI shall use only authorized game information and shall not receive hidden resources or knowledge unless an assisted difficulty clearly discloses the exact modifier.
* DEMO-AI-006 — The introductory opponent shall be beatable by a new player who successfully learned and applies the tutorial lessons.
* DEMO-AI-007 — The skirmish shall use the same mechanics and controls taught in the tutorial. Unexplained new requirements shall not be introduced at the transition.
* DEMO-AI-008 — Victory, defeat, pause, restart, rematch, and return-to-menu behavior shall work without debug intervention.
* DEMO-AI-009 — At least one complete unassisted victory and one complete defeat or controlled defeat-path validation shall be recorded from the packaged build.
* DEMO-AI-010 — I shall play and accept the AI experience, difficulty, pacing, clarity, and match lifecycle. (Verify: OWNER)

## K. Accessibility and learning support (owner: Player; verify: PKG-PHYS/REND + OWNER)

* DEMO-ACC-001 — Subtitle size, subtitle background, UI scale, high-contrast mode, and color-vision-safe/non-color markers shall change actual packaged behavior.
* DEMO-ACC-002 — Reduced motion, reduced flashing, adjustable camera motion, and reduced dynamic range shall operate across the opening, tutorial, UI, and gameplay.
* DEMO-ACC-003 — Keyboard navigation shall remain available throughout menus even though mouse interaction is mandatory.
* DEMO-ACC-004 — Tutorial voice, text, hints, pacing, pause, replay, and recovery shall support players who require more time without automatically performing the lesson.
* DEMO-ACC-005 — Remapped controls and accessibility settings shall persist and remain reflected accurately in every prompt and tooltip.
* DEMO-ACC-006 — I shall verify and accept the accessibility behaviors included in the demo. (Verify: OWNER)

## L. Packaging, human testing, and final acceptance (owner: QA+Build+Coordinator; verify: per row)

* DEMO-VAL-001 — All acceptance evidence shall come from one clearly identified candidate package built from the recorded source state.
* DEMO-VAL-002 — A clean profile shall travel continuously from cold launch through opening, tutorial, AI match, result, and return to menu.
* DEMO-VAL-003 — The end-to-end evidence shall use physical mouse and keyboard input. Editor play, headless automation, scripted controllers, state injection, and stitched unrelated clips do not pass.
* DEMO-VAL-004 — Existing automated suites shall remain green, but their claims shall remain limited to the behavior they actually exercise.
* DEMO-VAL-005 — Every retained screenshot shall be opened and inspected. Every required audio segment shall be listened to. File existence alone is not usable evidence.
* DEMO-VAL-006 — The golden path shall contain zero crashes, progression blockers, broken visible controls, save corruption events, audio-loss failures, or known high-severity defects.
* DEMO-VAL-007 — Reproducing a crash followed by a successful relaunch shall remain a failed run until the cause is understood and corrected.
* DEMO-VAL-008 — Human testing shall use five project-naive participants where reasonably available, including players who cannot be assumed to know RTS conventions. (Human participation — owner-gated recruitment)
* DEMO-VAL-009 — Participants shall receive no verbal coaching about where to click or how to complete a lesson.
* DEMO-VAL-010 — The test record shall capture time to begin, first selection, misclicks, ignored clicks, stalled lessons, hint use, tutorial completion time, story comprehension, visual recognition, economy establishment, combat behavior, result, crashes, and every observed confusion.
* DEMO-VAL-011 — At least four of five participants shall complete the tutorial, explain the immediate story and objective, begin the AI match, and execute the taught economy and combat loop.
* DEMO-VAL-012 — At least four of five participants shall state that the demo is clear and that they would voluntarily continue playing. This is a bounded internal usability signal, not proof of market success.
* DEMO-VAL-013 — Failures found in human sessions shall become tracked defects and, where technically appropriate, regression tests before retesting.
* DEMO-VAL-014 — I shall receive the same candidate package, a short review path, evidence summary, known limitations, and exact requirement IDs being offered for acceptance.
* DEMO-VAL-015 — I shall personally play the candidate and explicitly accept or reject each review batch. (Verify: OWNER)
* DEMO-VAL-016 — The aggregate demo may be called `COMPLETE` or `DEMO-READY` only after every mandatory requirement is `HUMAN ACCEPTED` for the same candidate build.
* DEMO-VAL-017 — My final acceptance is a demo decision only. It does not by itself establish full-game completion, public release readiness, notarization, broad hardware compatibility, or market acceptance.



---

# Part III — Initial release (`REL-*`)

Governance and bodies migrated verbatim from `InitialReleaseRequirements.md` on 2026-09-03.


Authority: owner order of 2026-09-02 ("Initial-Release Requirements Expansion and Agent
Synchronization Order"), received verbatim by the Claude Code coordinator. Owner and final
acceptance authority: Angelis Pseftis. This file is the sole normative ledger for `REL-*`
requirements (REL-GOV-001). It references and never duplicates `DEMO-*` bodies
(`Docs/DemoReadinessRequirements.md`); all `DEMO-*` requirements are prerequisites to release.
Orchestration/gates: `Docs/GameCompletionDirective.md` (bidirectional gate mapping per
REL-GOV-002 is an open ledger task). Roles: Claude Code coordinator + lane fleet = write owner;
ChatGPT Codex = read-only requirements/evidence reviewer (audits, challenges; never edits or
commits; Claude verifies Codex findings against the live checkout and integrates accepted ones
through its own write lane).

## Record schema and states

Every REL-* record carries: ID; normative shall statement; player outcome; preconditions;
dependencies; acceptance threshold; failure/recovery behavior; accessibility behavior;
automated verification; packaged verification; human-play requirement; owner acceptance
requirement; evidence location; exact source and package identity; engineering state; human
acceptance state; reopening conditions. Baseline default for all records below: state `OPEN`,
no bindings, evidence under `WorkstreamControl/evidence/release/<req-id>/` when produced.
Agent states: `OPEN → IN PROGRESS → IMPLEMENTED → AGENT VERIFIED → EVIDENCE READY → AWAITING
HUMAN ACCEPTANCE`. Owner-only: `HUMAN ACCEPTED`, `HUMAN REJECTED — CHANGES REQUIRED`,
`COMPLETE`. Batch acceptance permitted; parents stay open until every mandatory child is
accepted.

## Acceptance-card rule (applies to every DEMO-* and REL-* requirement)

Before any requirement moves beyond `IN PROGRESS` it receives an acceptance card with child
checks `.PRE .ACT .AUTH .VIS .AUD .FAIL .REC .ACC .PERF .AUTO .PKG .HUM .OWNER .EVID .REOPEN`
(hierarchical IDs, e.g. `DEMO-INP-002.AUTH`). A genuinely inapplicable child records
`NOT APPLICABLE` + technical reason + reviewer concurrence + owner acceptance — never silent
omission. Cards are populated incrementally as requirements enter active work. The owner's
worked example for `DEMO-INP-002` (packaged build, cursor mapping through window/viewport/DPI,
hover identification, exact-entity selection, authoritative selection, ring/panel/sound, replace
vs modifier, empty-terrain clear rule, enemy-info bounds, no hidden/dead/occluded selection, no
UI click-through, correctness across scale/resolution/window/camera/preset, rejected-click
reasons, no stale selection across save/load/cinematic/pause/focus/transition, handler tests
never substitute for physical input, owner physical acceptance) sets the required specificity
bar for all cards.

## Quality target (§2 of the order — binding interpretation)

"Built with the care of a major gaming company" = major-studio discipline in every
player-visible and operational surface (complete coherent journeys; responsive controls;
tactical readability; strong art direction; finished animation/effects; professional
audio/voice; cinematic story delivery; understandable systems; strategic depth; reliable AI;
accessible interaction; stable performance; hardware scalability; safe saving/recovery;
professional packaging; accurate public claims; release/support readiness; rigorous
playtesting; no visible seams). It does not authorize copying another game, false
staffing/budget claims, or content-volume parity claims without evidence. Every component
must be: (1) designed against the authoritative game; (2) implemented; (3) integrated into the
actual player journey; (4) tested at its technical boundary; (5) exercised in the packaged
build; (6) seen/heard/operated by a human where applicable; (7) accepted by Angelis for the
exact candidate build.

## Scope boundary and TBR decisions (§4)

1.0 scope per `GameCompletionDirective.md` (preserved until the owner changes it): macOS Apple
Silicon; single-player; fifteen campaign operations; four reachable endings; Glass Scar PvAI
skirmish; Meridian Compact, Kharuun Assemblies, Hollow Choir selectable in skirmish; five AI
personalities; full professional art/audio/voice/cinematics/UI/accessibility/saves; Developer
ID signed and notarized distribution; coherent website/trailer/manual/rights/known-limitations;
no multiplayer; no Windows/Linux; no maps beyond approved. One consolidated scope-sufficiency
decision packet SHALL be prepared (recommendation, cost, schedule, dependencies, playable
consequences per item) and presented together:

* TBR-SCP-001 — Keep multiplayer out of 1.0 or add human-versus-human multiplayer.
* TBR-SCP-002 — Keep one skirmish map or expand the launch map set. Proposed professional breadth: at least six fully finished skirmish maps unless human testing supports an intentionally compact alternative.
* TBR-SCP-003 — Keep the currently observed approximate roster of four units, four buildings, and two technologies per faction or expand strategic breadth. Proposed review target: at least eight fieldable unit roles, six constructed building roles, and ten meaningful technology or upgrade decisions per faction, unless a smaller roster demonstrates equivalent strategic depth.
* TBR-SCP-004 — Direct-download release, Steam release, or both.
* TBR-SCP-005 — English-only launch or additional launch languages.
* TBR-SCP-006 — Local saves only or platform cloud synchronization.
* TBR-SCP-007 — Required replay browser, observer tools, achievements, and platform integration.
* TBR-SCP-008 — Mouse-and-keyboard only or optional controller support.
* TBR-SCP-009 — Final campaign and skirmish difficulty tiers.
* TBR-SCP-010 — Minimum and recommended supported Apple Silicon hardware and macOS versions.
* TBR-SCP-011 — How much portability and graphics-scalability enforcement lands in the initial release:
  automated guards inside the ordinary suite (REL-PORT-008) and a second-toolchain, second-architecture
  determinism run (REL-PORT-002), versus a recorded release-time audit only. Low-cost candidate to price:
  an `x86_64` build of `EchoesSimCore` run under Rosetta 2 on the existing Mac for the second architecture,
  and a second compiler for the second toolchain — neither needs a second machine. The packet states cost,
  schedule, and the consequence of deferring each until the first Linux build.

## Requirement bodies

The complete normative shall statements for the following sections were delivered verbatim in
the owner's 2026-09-02 order and are transcribed in the section files of this ledger BELOW —
this ledger is authoritative once the transcription audit (QA lane) confirms fidelity.

### §6 Release governance and integrity — REL-GOV-001..015
### §7 First-run, front door, onboarding — REL-FTU-001..012
### §8 Core simulation, time, player authority — REL-SIM-001..012
### §9 Economy and logistics — REL-ECO-001..014 (Matter, Dawn, Logistics)
### §10 Construction, production, research — REL-BLD-001..014
### §11 Selection, movement, commands, combat — REL-CMB-001..018
### §12 Factions, rosters, strategic depth — REL-FAC-001..013
### §13 Future Wells — REL-WEL-001..012 (canonical values: Harvest 180-tick telegraph/500 Dawn; Preserve 15 Dawn per 300 ticks/1,400 cm radius; Reshape 120 Dawn/180-tick telegraph/1,800-tick manifestation — changes require owner approval)
### §14 Campaign and narrative — REL-CAM-001..021
### §15 Skirmish, AI, difficulty, balance — REL-AI-001..021 (balance target REL-AI-016: no non-mirror Standard matchup outside 40–60% and no start-position advantage >5 points over the approved test set, absent owner-accepted design reason)
### §16 Replays and QoL — REL-QOL-001..012 (unless owner excludes at scope approval)
### §17 UI and interaction — REL-UI-001..016 (resolution matrix REL-UI-013: 1280×720, 1440×900, 1600×900, 1920×1080, 2560×1440, baseline native, windowed, fullscreen, live resize)
### §18 World art, units, structures, animation, VFX — REL-ART-001..020 (asset completion cards mandatory per family)
### §19 Audio, voice, music, cinematics — REL-AUD-001..015, REL-CIN-001..008 (mix standard REL-AUD-010: −16 LUFS ±1 integrated, true peak ≤ −1 dBTP unless owner revises)
### §20 Saves, profiles, progression, recovery — REL-SAV-001..014
### §21 Accessibility and localization readiness — REL-ACC-001..017, REL-LOC-001..006
### §22 Graphics scalability, performance, stability — REL-PERF-001..018, REL-STAB-001..005 (budgets REL-PERF-007: p95 ≤16.67 ms, game thread ≤4.0 ms, render+GPU ≤11.0 ms, fog ≤1.5 ms, path burst ≤6.0 ms, resident ≤10 GB, save ≤250 ms; REL-PERF-010: 400-unit/four-team stress; REL-PERF-011: 600 s preflight + 60-min rendered session; REL-PERF-012: multi-hour AI soak)
### §23 Security, privacy, packaging, distribution — REL-DIST-001..017, REL-SEC-001..006
### §24 Public website, manual, claims, support — REL-PUB-001..015
### §25 QA, human validation, release blockers — REL-QA-001..032; severity ladder S0 (release prohibited) / S1 (release prohibited) / S2 (zero known on release-critical path absent owner waiver) / S3 (correct or accept+disclose) / S4 (record and disposition)
### §26 Conditional multiplayer module — REL-MP-001..016 (DORMANT; activates only if the owner changes the multiplayer scope decision)

> TRANSCRIPTION STATUS: the section headers above carry the load-bearing numeric standards
> inline; the full per-requirement shall statements are being transcribed verbatim from the
> owner's order into this file as each section's requirements first enter work (same
> incremental rule as acceptance cards), with the owner's received order as the source of
> truth held by the coordinator. QA audit item: verify each transcription verbatim on entry.

## Player-purpose, victory, defeat, and strategic-system requirements

Authority for this section: direct owner instruction of 2026-09-02 to define what every unit,
building, and game element does from the player's perspective; define how a player wins and
loses; define the strategic problems the player must solve; and place those definitions in this
requirements ledger before further game construction. These records extend the ranges summarized
above. They do not claim that the current prototype satisfies them.

Unless a child record states otherwise, acceptance requires: source inspection and focused
automation; deterministic save/load and replay equivalence where stateful; packaged physical
mouse and keyboard use; rendered inspection at the REL-UI-013 resolution matrix; uncoached human
comprehension; and owner acceptance on the same identified candidate. Player-facing time shall be
shown in seconds; deterministic ticks may also appear in detailed information and evidence.

### Core match and operation outcomes

* **REL-SIM-013 — Corefall standard-match victory.** Standard skirmish and competitive play shall
  use Corefall: a player or team wins when every opposing team has no surviving Command Core.
  Anchor, Memory Hearth, and Concordance are faction-specific Command Cores. Destroying the final
  opposing Core shall end the match immediately and reject later commands. Future Well control,
  score, kills, resources, army size, and destruction of non-Core assets shall not independently
  award victory.
* **REL-SIM-014 — Corefall defeat and concession.** A player loses when its final Command Core is
  destroyed or the player explicitly confirms concession. Zero workers, zero combat units, zero
  production structures, or zero resources shall not automatically defeat a player while its Core
  survives. Concession shall be deliberate, recoverable before confirmation, and attributable in
  results and replay metadata.
* **REL-SIM-015 — Team elimination.** A team shall remain active while any team-owned Command Core
  survives. The pre-match rules shall state whether an individually eliminated ally observes,
  shares vision, or transfers control; no such authority shall be inferred silently.
* **REL-SIM-016 — Draw.** If all remaining Command Cores are destroyed within the same
  authoritative resolution window, the result shall be Draw. No hidden tiebreak based on score,
  damage, resources, or command order shall replace it.
* **REL-SIM-017 — Outcome comprehension.** Before deployment and throughout a standard match, the
  UI shall state `WIN: Destroy the enemy Command Core` and `LOSE: Your Command Core is destroyed`.
  It shall identify the Future Well as an optional strategic objective rather than a victory point.
  Own-Core integrity shall remain visible; enemy-Core information shall obey fair fog; critical
  thresholds shall generate distinguishable alerts. At least four of five uncoached testers shall
  correctly explain both victory and defeat before normal play begins.
* **REL-SIM-018 — Initial-release outcome scope.** Corefall shall be the only standard-match victory
  condition required for initial release. An alternate condition shall remain unavailable unless
  its rules, setup disclosure, UI, AI behavior, map support, save/replay behavior, tests, packaged
  play, counterplay, balance evidence, and owner acceptance are complete.
* **REL-SIM-019 — Visibility-scoped world presentation.** Terrain, temporary cover, Reshape changes,
  minimap state, alerts, and presentation systems shall consume the same player-scoped visibility
  authority as unit targeting. A change under explored shroud shall not disclose current hidden
  state merely because the full simulation knows it. Tests shall cover appearance, change, expiry,
  destruction, save/load, and replay both inside and outside current vision.
* **REL-CAM-022 — Objective-based operation victory.** A campaign operation shall be won only by
  completing its authored primary objective sequence. The briefing shall identify the primary
  objective, protected people/assets, optional objectives, and irreversible choices. Enemy-Core
  destruction shall not substitute for escort, recovery, hold, witness, Well, route, withdrawal,
  or other authored objectives unless the operation explicitly says it does.
* **REL-CAM-023 — Objective-based operation defeat.** Each operation shall define its terminal
  failures: required Core loss, mission-critical unit/structure loss, expired opportunity,
  invalid irreversible commitment, impossible escort/evacuation, or force state that makes the
  primary objective impossible. Ordinary unit loss shall not silently cause defeat.
* **REL-CAM-024 — Result causality.** Results shall state why the player won or lost, optional
  outcomes, the exact irreversible campaign record written, and material states not changed.
  Restart and alternate replay shall not silently rewrite established campaign history.
* **REL-CAM-025 — Progressive capability introduction.** The campaign shall own a machine-readable
  unlock manifest stating, for every operation, which units, structures, commands, abilities,
  technologies, resources, objectives, and interface systems are introduced, practiced, assessed,
  retained, or intentionally unavailable. A capability shall be introduced in a low-pressure,
  narratively justified situation before success depends on mastery. Locked controls shall explain
  their campaign requirement and shall not look broken or available.
* **REL-CAM-026 — Objective portfolio.** The fifteen-operation campaign shall not reduce to repeated
  Core destruction. Its accepted mission matrix shall include evacuation or escort, survival or
  timed pressure, hold/defense, low-force reconnaissance or avoidance, economy/logistics, Future
  Well commitment, terrain/route transformation, multi-site coordination, and decisive macro battle.
  No more than one-third of operations may use enemy-Core destruction as the primary objective
  without an explicit owner-accepted narrative and pacing reason. No two consecutive operations
  shall present materially identical primary problems.
* **REL-CAM-027 — Mission pacing.** Every operation shall be classified by scale (`MICRO`, `HYBRID`,
  or `MACRO`), intensity, expected duration, player-attention burden, and new-system load. The
  sequence shall alternate pressure, recovery, tactical focus, strategic expansion, revelation, and
  consequence. More than two consecutive high-intensity macro operations requires owner acceptance.
  Dialogue, cinematics, and result transitions shall provide recovery without withholding required
  control during an active threat.
* **REL-CAM-028 — Environmental storytelling.** Map layout, architecture, damage, abandoned routes,
  resource placement, ecology, objective sites, lighting, sound, and visible Well consequences shall
  communicate the operation's history and present problem. Scripted triggers and mid-operation
  dialogue shall respond to authoritative events and player choices. Exposition shall not require
  the player to stop controlling the game or read an unrelated lore dump.
* **REL-CAM-029 — Persistent progression contract.** The campaign shall define exactly what persists
  between operations: completed objectives, irreversible Well records, unlocked units/structures,
  research or doctrine choices, unit modifications, faction perks, optional rewards, difficulty,
  and any surviving-force state. Each category shall state whether it carries, resets, branches, or
  is mission-local. Save migration, replay conflict, new-campaign reset, restore, and rollback
  behavior shall be transactional and visible.
* **REL-CAM-030 — Meaningful campaign rewards.** Completing required and optional objectives shall
  grant authored rewards that change a later player decision rather than only increase a score.
  Each reward shall state availability, exclusivity, affected units/systems, campaign duration,
  previewed effect, and downstream consequence. Reward choices shall not create an unknowable trap;
  the player shall see the decision class even when the story consequence remains uncertain.
* **REL-CAM-031 — Feature teaching gate.** A unit, building, ability, automation, resource, terrain
  interaction, or technology shall not enter normal campaign difficulty until its tutorial beat,
  selection explanation, failure feedback, AI use, save/replay behavior, accessibility path, and
  focused tests are complete. Introduction in a cinematic or tooltip alone does not pass.
* **REL-CAM-032 — Scripted-event fairness.** Authored waves, reinforcements, hazards, route changes,
  and story events shall have an explicit source, trigger, telegraph, counterplay, and repeat/load
  policy. A director-spawned mission force shall be labeled separately from forces produced by the
  opponent economy. Save/load or checkpoint restore shall not duplicate, omit, or reorder an event.

### Strategic decision model

* **REL-FAC-014 — Strategic loop.** The standard match shall support and teach the connected loop:
  gather; scout; choose production; secure Logistics; contest territory and the Future Well; select
  or avoid an engagement; convert an advantage into Command-Core damage; defend the counterattack.
  The sequence shall permit viable raiding, defensive, economic, and objective-first variations.
* **REL-FAC-015 — Economic allocation.** Worker growth, army production, structures, research,
  faction abilities, and Dawn reserve shall create observable opportunity costs. No economy choice
  shall be dominant without a timing, safety, Logistics, production, or territorial tradeoff.
* **REL-FAC-016 — Route economy.** Worker cargo routes shall make distance, drop-off placement,
  congestion, protection, and harassment materially affect realized Matter income. The UI shall
  expose cargo, destination, order, and failed-delivery reason.
* **REL-FAC-017 — Logistics strategy.** Logistics shall cap committed population and shall come
  from completed operational headquarters and faction supply infrastructure. The HUD shall show
  used, durable, temporary, and expiring capacity; production shall fail explicitly with
  `LOGISTICS FULL`. Destroying, disconnecting, migrating, or defunding a supply element shall
  produce the authored faction-specific consequence without an unexplained state change.
* **REL-FAC-018 — Information strategy.** Visible, Explored, Unexplored, and anonymous approximate
  contact states shall remain distinct. Scouting shall affect safe investment, engagement timing,
  route choice, and retreat. Hidden enemies shall not be directly targetable or retained as perfect
  contacts.
* **REL-FAC-019 — Soft-counter strategy.** The initial roster shall use observable range, speed,
  health, vision, facing, cover, state, timing, resource, and position differences rather than
  secret unit bonuses. If armor, damage types, or target modifiers are later introduced, production
  and selection UI shall disclose them before competitive use.
* **REL-FAC-020 — Engagement choice.** Hold, flank, delay, raid, focus fire, guard, patrol, and
  retreat shall be viable decisions. Time-to-kill and command response shall permit threat
  recognition and withdrawal while preserving the value of focus fire.
* **REL-FAC-021 — Production tempo.** Unit production, additional producers, and research shall
  compete for resources and producer time. A player shall be able to understand the timing window
  created by each choice and the risk of research during an attack.
* **REL-FAC-022 — Territory and conversion.** Crossings, ridges, economy routes, fallback points,
  and Well approaches shall create different reinforcement, vision, formation, and exposure
  advantages. Gaining an advantage shall not itself score a win; the player shall have to convert it
  into the operation objective or enemy-Core damage.
* **REL-FAC-023 — Attention strategy.** Control groups, guard, patrol, hold, alerts, overview, and
  formations shall let the player distribute attention without automating tactical judgment. The
  opponent shall be able to create meaningful multi-route pressure and feints.
* **REL-FAC-024 — Strategic knowledge boundary.** The game shall teach rules, purposes, costs,
  prerequisites, failures, and end conditions. Players shall discover timings, combinations,
  efficient greed, feints, and map use through play rather than guess undisclosed mechanics.
* **REL-ECO-015 — Continuous worker economy.** A valid Gather order shall cycle an eligible worker
  between the assigned Matter source and an operational drop-off until the source depletes, the
  route or drop-off becomes invalid, danger policy interrupts if later authored, or the player
  issues another order. The worker shall not require the player to repeat every gather and delivery
  leg. Idle, cargo-full, route-blocked, drop-off-lost, and source-depleted states shall be distinct.
* **REL-ECO-016 — Dawn sources and obligations.** The HUD and relevant selections shall identify
  current Dawn, known income sources, Preserve interval, pending expenditures, Relay/ability costs,
  Choir structure charges, and the next affordability failure. Starting resources, Harvest, and
  Preserve shall not be mistaken for ordinary field gathering.
* **REL-ECO-017 — Resource monitor.** The persistent economy surface shall show current Matter and
  Dawn; realized 30- and 60-second income; total, idle, gathering, delivering, traveling, and blocked
  workers; assignments and saturation per known Matter source; known remaining deposit amount and
  estimated depletion; operational drop-offs and route time; production/research commitments;
  Logistics used, permanent, temporary, reserved, and expiring; the next Preserve return; Relay
  expiry; and each upcoming Choir coherence charge. Forecasts shall use only owned or legitimately
  observed information, identify their window and rounding, survive UI scaling, and never expose a
  hidden enemy economy or undiscovered resource.

### Universal selection and command contract

* **REL-UI-017 — Selection identity.** Selecting any owned entity shall show faction marker,
  faction-specific name, role, owner, health, current order/state, and selected count. Grouped and
  mixed selections shall expose composition and deterministic subgroup navigation.
* **REL-UI-018 — Selection state.** Where applicable, selection shall expose cargo; target and
  route; construction, production, and research progress; queue; Logistics; power connection;
  deployment facing; rooting/migration; adaptation; Choir identity; upkeep; ability cost, duration,
  and cooldown; Well control and protocol state.
* **REL-UI-019 — Purpose and counterplay.** Every selectable gameplay element shall state one
  primary purpose, its strongest use, its meaningful limitation, and the opponent's available
  counterplay. Lore text shall not substitute for mechanical explanation.
* **REL-UI-020 — Action availability.** Every enabled control shall invoke a supported authoritative
  action. A disabled control that teaches progression shall identify the exact missing resource,
  prerequisite, target, range, connection, state, cooldown, producer, footprint, or mode. An
  unimplemented option shall be completed, clearly unavailable, or removed.
* **REL-UI-021 — Action feedback.** Accepted Move, Direct Attack, Attack-move, Patrol, Guard, Build,
  Gather/Deliver, and Interact actions shall use distinct non-color markers and appropriate audio.
  Rejections shall return stable reasons. A context click that misses an intended visible target
  shall not silently become a plausible but wrong Move order.
* **REL-UI-022 — Input parity.** Pointer and keyboard paths shall expose the same gameplay authority,
  costs, previews, confirmations, and feedback. Remapping shall update prompts and tutorial text.
* **REL-UI-023 — Binding exclusivity.** One active gameplay context shall not bind the same key to
  two actions whose conditions can overlap or whose result is ambiguous. Known conflicts involving
  Preserve/Continue Campaign, Formation/Online, and Warform adaptation/camera speed shall be removed
  or separated by an explicit, visible, non-overlapping mode. Automated collision checks and
  packaged physical input shall verify the final binding set.
* **REL-UI-024 — Tactical overview truth.** The minimap shall distinguish Unexplored, Explored, and
  currently Visible terrain; owned forces; currently visible hostiles; fair last-known contacts only
  if authored; resources; objective markers; camera bounds; anonymous vibration contacts; and each
  Future Well protocol/state without relying on color alone. It shall not expose hidden Scarred,
  cover, Reshape, movement, or structure state.
* **REL-CMB-019 — Common mobile commands.** Eligible mobile selections shall support Move/Context,
  Direct Attack, Attack-move, Patrol, Guard, Hold, Stop, and Box/Line/Wedge formations. Direct Attack
  requires a fair visible hostile. Hold prevents translation but permits valid fire. Stop cancels the
  current order and shall disclose any irreversible cancellation consequence.
* **REL-CMB-020 — Formation meaning.** Box shall provide compact mixed-force movement; Line shall
  maximize centered frontage perpendicular to travel; Wedge shall place its apex toward the
  destination. Formation UI shall not claim path deconfliction, obstacle cohesion, or combat bonuses
  unless those behaviors are implemented and verified.
* **REL-CMB-021 — Control groups.** Assignment and recall shall preserve deterministic living
  membership, remove destroyed members, show group association, and remain separate from automation.
* **REL-CMB-022 — Worker authority.** Only eligible workers shall Gather Matter, Deliver cargo,
  construct faction structures, and commit a Future Well protocol. Worker selection shall show cargo
  and route state. Workers shall not contribute attack damage unless a later visible rule says so.
* **REL-CMB-023 — Repair resolution.** The project shall either implement Repair with eligible
  targets, Matter/time cost, range, channel/interruption, feedback, AI, save/replay, and tests, or
  remove all player-facing claims that Surveyors and Tenders repair. The current command model does
  not satisfy this requirement.
* **REL-CMB-024 — Rally resolution.** Production structures shall support a visible set/clear rally
  point with valid-destination, emergence, formation, fog, save/replay, and failure behavior, or all
  release requirements and teaching that promise rally behavior shall be revised with owner approval.
* **REL-CMB-025 — Player-issued reconnaissance.** Eligible scout units shall support `Explore Area`,
  `Find Matter`, `Locate Hostiles`, and `Screen Route` missions. The player shall select the search
  area, route, or guarded force and one response policy: `CAUTIOUS` (report and withdraw on contact),
  `OBSERVE` (report and maintain safe observation), or `PERSIST` (continue until damaged or directly
  overridden). Reconnaissance shall use only player-known terrain and information acquired through
  normal vision or faction sensors; it shall never query hidden enemy or resource state.
* **REL-CMB-026 — Reconnaissance lifecycle.** The selected scout shall show mission, search boundary,
  route/frontier, discoveries, response policy, health threshold, and return point. It shall issue
  separate fair alerts for Matter, Future Wells, routes, hostiles, structures, damage, blocked paths,
  and completed search. `NO UNEXPLORED AREA`, `NO SAFE ROUTE`, `SEARCH COMPLETE`, and `SCOUT LOST`
  shall be explicit outcomes. A manual command shall override immediately; resume/return behavior
  shall be chosen rather than inferred. Reconnaissance shall persist deterministically through
  save/load and replay.
* **REL-CMB-027 — Automation authority boundary.** Player automation may execute a declared policy
  for movement, gathering, reconnaissance, repair if supported, stance, rally, or production
  reinforcement. It shall not select a Future Well protocol, spend a campaign reward, choose
  research, change warform/Choir identity, concede, or initiate another irreversible strategic
  commitment without a direct player command. Automation state and cancellation shall remain visible.

### Exact faction roster requirements

The numeric baselines below bind content-hydrated play at 20 simulation ticks per second. Balance
changes require traceable rationale, updated UI/tooltips/tests, regression play, and owner acceptance.

* **REL-FAC-025.MC.SURVEYOR — Surveyor.** Cost 50 Matter; 90 health; 360 cm/s; 900 cm sight;
  1 Logistics; 60-tick production; work rate 10; cargo 10. It shall gather, deliver, build Array
  Foundry, Power Link, and Aegis Post, operate Future Wells, and repair only if REL-CMB-023 is met.
  Its purpose is economy and network expansion; it shall have no attack.
* **REL-FAC-025.MC.LANCER — Lancer.** Cost 85 Matter/20 Dawn; 145 health; 320 cm/s; 1,100 cm sight;
  2 Logistics; 100-tick production; 18 damage at 650 cm every 30 ticks. It shall be sustained ranged
  line damage, strongest behind vision and screening and vulnerable to flanking and close pressure.
* **REL-FAC-025.MC.BULWARK — Bulwark Team.** Cost 130/25; 260 health; 230 cm/s; 850 cm sight;
  3 Logistics; 140-tick production; 10 damage at 300 cm every 24 ticks. Deploy/Pack shall show facing
  and protected geometry, grant 40% directional damage reduction, and reduce movement to 35% while
  deployed. Flanking, bypass, forced rotation, and separation from ranged support shall remain valid
  counterplay.
* **REL-FAC-025.MC.SKIFF — Relay Skiff.** Cost 70/20; 75 health; 500 cm/s; 1,500 cm sight;
  1 Logistics; 80-tick production; 6 damage at 400 cm every 24 ticks. A connected Skiff within
  700 cm of valid Compact infrastructure may grant +4 temporary Logistics for 400 ticks with an
  800-tick cooldown. UI shall disclose connection, duration, expiry risk, and cooldown. It is a
  fragile scout/support, not a line combatant.
* **REL-FAC-026.KA.TENDER — Tender.** Cost 50 Matter; 100 health; 390 cm/s; 920 cm sight;
  1 Logistics; 60-tick production; work rate 9; cargo 10. It shall gather, deliver, grow Growth Basin,
  Waystone, and Listening Spine, operate Wells, and have no attack. Terrain stabilization shall be
  implemented with a visible effect/cost/counterplay or removed from its role description.
* **REL-FAC-026.KA.RIFTSTALKER — Riftstalker.** Cost 75/30; 125 health; 410 cm/s; 1,050 cm sight;
  2 Logistics; 100-tick production; 14 damage at 500 cm every 22 ticks. It shall create value through
  short mobile trades and repositioning; no stealth or ambush action shall be implied until authored.
* **REL-FAC-026.KA.CAIRNBACK — Cairnback.** Cost 120/30; 245 health; 270 cm/s; 800 cm sight;
  3 Logistics; 140-tick production; 16 damage at 200 cm every 28 ticks. Raise Mineral Cover shall cost
  15 Dawn, target within 450 cm, create 180-health destructible cover for 300 ticks, and use a
  600-tick cooldown. Invalid overlap/terrain and molt state shall reject clearly. Cover shall affect
  attacks only through authoritative geometry and may not imply universal armor.
* **REL-FAC-026.KA.RESONANT — Resonant.** Cost 80/25; 85 health; 470 cm/s; 1,550 cm sight;
  1 Logistics; 80-tick production; 8 damage at 380 cm every 20 ticks. Passive vibration detection
  shall report moving signatures within 2,200 cm at 200 cm resolution for 40 ticks without identity,
  exact targeting, or stationary detection.
* **REL-FAC-026.KA.ADAPT — Warform adaptation.** Eligible Kharuun combat units within 600 cm of a
  completed friendly Growth Basin may spend 25 Dawn and molt for 80 ticks while taking 150% damage.
  Carapace shall set health to 135% and movement to 80%; Striker shall set damage to 125% and attack
  cooldown to 85%. The choice, resulting stats, vulnerability, invalid reasons, and interruption
  shall be visible. Adaptation shall reward scouting, not blind universal upgrading.
* **REL-FAC-027.HC.THREADKEEPER — Threadkeeper.** Cost 55 Matter/5 Dawn; 80 health; 380 cm/s;
  1,000 cm sight; 1 Logistics; 65-tick production; work rate 9; cargo 12. It shall perform common
  worker actions and expose the Choir's projected coherence obligations. It shall have no attack.
* **REL-FAC-027.HC.INTERVALIST — Intervalist.** Cost 80/35; 115 health; 350 cm/s; 1,150 cm sight;
  2 Logistics; 100-tick production; 16 damage at 550 cm every 25 ticks. It shall be the phase
  skirmisher whose public identity choice trades damage against mobility/vision.
* **REL-FAC-027.HC.WARDEN — Lacuna Warden.** Cost 140/45; 230 health; 260 cm/s; 900 cm sight;
  3 Logistics; 150-tick production; 15 damage at 400 cm every 30 ticks. It shall be a durable Choir
  center. `Recovery controller` shall gain a bounded, visible recovery/control mechanic with cost,
  target, failure, and counterplay, or the role shall be renamed to actual behavior. No healing,
  revive, rewind, or repair shall be implied meanwhile.
* **REL-FAC-027.HC.AFTERIMAGE — Afterimage.** Cost 75/35; 70 health; 520 cm/s; 1,600 cm sight;
  1 Logistics; 85-tick production; 7 damage at 420 cm every 22 ticks. It shall be fast scouting and
  route pressure. `Misdirection support` shall gain a fair public-duration counterplay mechanic or
  be renamed; no decoy, disguise, duplicate, or displacement shall be implied meanwhile.
* **REL-FAC-027.HC.IDENTITY — Choir reconciliation.** Each Choir line/heavy/scout unit may spend
  20 Dawn to reconcile. Manifest shall grant 130% damage; Possible shall grant 130% movement and
  125% vision. The 160-tick transition carries both declared effects; the next change is unavailable
  until the transition plus 400-tick cooldown completes. State, preview, cost, transition, cooldown,
  and opponent telegraph shall be visible. The mechanic shall not rewind, teleport, duplicate an
  army, or restore hidden information.

### Exact structure requirements

* **REL-BLD-015.MC.ANCHOR — Anchor.** Faction Command Core and Matter drop-off; 1,400 health;
  800 cm sight; +12 Logistics; 5x5 footprint. It shall produce Surveyors and expose own-Core health,
  economy, capacity, queue, rally when implemented, and loss warning. It is not a complete defense.
* **REL-BLD-015.MC.LINK — Power Link.** Cost 90 Matter/10 Dawn; 450 health; 500 cm sight;
  100-tick construction; +6 Logistics; 2x2 footprint. It shall be an operational Matter drop-off and
  chained Compact network node unless the drop-off mapping is deliberately changed and accepted.
  Connection and supported Aegis Posts shall be visible. It shall not attack or produce.
* **REL-BLD-015.MC.FOUNDRY — Array Foundry.** Cost 180/30; 760 health; 500 cm sight;
  160-tick construction; 4x4 footprint. It shall produce Lancer, Bulwark Team, and Relay Skiff and
  host Compact research. Production/research contention, progress, costs, Logistics, emergence,
  rally, cancellation, and interruption shall be visible.
* **REL-BLD-015.MC.AEGIS — Aegis Post.** Cost 130/30; 520 health; 700 cm sight; 120-tick
  construction; 2x2 footprint. A completed Post connected through an operational Compact network
  within 800 cm shall automatically attack valid visible enemies for 28 damage at 900 cm every
  20 ticks. Powered/offline state, source, range, and severance shall be visible. It shall remain
  inert when unpowered and shall not replace a field army.
* **REL-BLD-016.KA.HEARTH — Memory Hearth.** Kharuun Command Core and Matter drop-off;
  1,300 health; 800 cm sight; +12 Logistics; 5x5 footprint. It shall produce Tenders and expose the
  common Core, economy, capacity, queue, and loss information.
* **REL-BLD-016.KA.WAYSTONE — Waystone.** Cost 80/20; 390 health; 500 cm sight; 100-tick
  construction; +5 Logistics while rooted; 2x2 footprint. Uproot shall take 40 ticks, mobile movement
  shall be 120 cm/s with 125% incoming damage, and Root shall take 60 ticks on a clear footprint.
  Non-rooted Waystones shall not accept Matter or provide Logistics. State, progress, footprint,
  exposure, and affected capacity shall be visible.
* **REL-BLD-016.KA.BASIN — Growth Basin.** Cost 165/35; 700 health; 500 cm sight; 160-tick
  construction; 4x4 footprint. It shall produce all Kharuun combat units, host research, and serve
  as the 600 cm adaptation site. It shall show eligible warforms, molt choice/cost/risk, queues,
  contention, interruption, and rally.
* **REL-BLD-016.KA.SPINE — Listening Spine.** Cost 115/25; 440 health; 900 cm sight; 120-tick
  construction; 2x2 footprint. Passive detection shall report moving signatures within 2,600 cm at
  200 cm resolution for 40 ticks. Coverage and anonymous contacts shall be visible but not directly
  targetable. It shall not attack or identify contacts.
* **REL-BLD-017.HC.CONCORDANCE — Concordance.** Choir Command Core and Matter drop-off;
  1,250 health; 900 cm sight; +12 Logistics; 5x5 footprint. It shall produce Threadkeepers and remain
  excluded from ordinary non-Core coherence upkeep. It shall show total projected obligations.
* **REL-BLD-017.HC.INTERVAL — Interval Loom.** Cost 85/25; 400 health; 600 cm sight; 110-tick
  construction; +6 Logistics; 2x2 footprint. It shall be a Matter drop-off and Logistics node and
  shall visibly charge 5 Dawn each 600 ticks while coherent.
* **REL-BLD-017.HC.CHORUS — Chorus Loom.** Cost 175/40; 680 health; 550 cm sight; 170-tick
  construction; 4x4 footprint. It shall produce all Choir combat units, host research, and visibly
  charge 5 Dawn each 600 ticks. Production, research, upkeep, reserve, interruption, and rally shall
  be shown together so the player can judge solvency.
* **REL-BLD-017.HC.ANCHOR — Phase Anchor.** Cost 120/35; 480 health; 800 cm sight; 130-tick
  construction; 2x2 footprint; 5 Dawn per 600-tick coherence charge. Before ordinary skirmish build
  access, it shall receive a defined positional coherence benefit with coverage, cost, failure,
  opponent counterplay, UI, AI use, and evidence. Campaign-specific use shall not be generalized into
  an unsupported skirmish claim.
* **REL-BLD-017.HC.UPKEEP — Choir structural coherence.** Each completed living Choir Drop-off,
  Production, and Utility structure shall independently charge 5 Dawn every 600 ticks. Failure to
  pay may destroy that structure only after persistent resource forecast, next-charge timer, low
  reserve warning, final warning, attributable loss feedback, and tutorial coverage are present.

### Production and technology requirements

* **REL-BLD-018 — Faction-specific construction language.** Worker controls shall name Array
  Foundry/Power Link/Aegis Post, Growth Basin/Waystone/Listening Spine, or Chorus Loom/Interval
  Loom/Phase Anchor, not only generic `Barracks`, `Drop-off`, and `Utility`. Placement shall preview
  cost, footprint, validity, worker route, completion, and whether the result will be operational.
* **REL-BLD-019 — Producer inspection.** A producer shall show supported units with cost, Logistics,
  time, role, prerequisite, queue/progress, emergence and rally. `Producer busy`, `insufficient
  resources`, `Logistics full`, and entity-cap failures shall remain distinct.
* **REL-BLD-020 — Cancellation policy.** Research shall remain no-refund when stopped or its producer
  is destroyed and shall warn before commitment/cancellation. Unit-production and construction
  cancellation/refund rules shall be explicitly decided, displayed, tested, and owner-accepted; no
  interface shall imply a refund before that decision.
* **REL-FAC-028.MC.TECH — Compact technology.** Array Foundry research shall provide Prismatic
  Targeting (120 Matter/40 Dawn, 180 ticks, 115% combat damage) followed by Horizon Lattice (90/55,
  220 ticks, requires Tier 1, 120% combat vision).
* **REL-FAC-028.KA.TECH — Kharuun technology.** Growth Basin research shall provide Echo Cartography
  (100/45, 180 ticks, 120% combat vision) followed by Ancestral Edge (110/50, 220 ticks, requires
  Tier 1, 115% combat damage).
* **REL-FAC-028.HC.TECH — Choir technology.** Chorus Loom research shall provide Held Alternatives
  (105/50, 190 ticks, 110% combat damage and vision) followed by Shared Resolution (115/60,
  230 ticks, requires Tier 1, 120% combat vision). UI shall show every applied effect.
* **REL-FAC-029 — Technology purpose.** The archive shall explain the problem each technology solves,
  show exact cost/time/effect/prerequisite/resource/progress/interruption, and disclose that research
  occupies the producer. Linear percentage research shall be expanded, replaced, or explicitly
  owner-accepted as sufficient under TBR-SCP-003 before release.

### Future Well and world-element requirements

* **REL-WEL-013 — Well selection.** A visible Future Well shall show control owner, capture progress,
  contest state, protocol, telegraph, affected terrain/route, and three comparable protocol cards.
  A confirmation shall state immediate gain, resource cost, duration, irreversibility, opponent
  interruption, and known campaign consequence before commitment.
* **REL-WEL-014 — Well capture.** Control shall use the authored 420 cm radius and 300-tick capture
  requirement. Contention, interruption, worker eligibility, loss of control, and recapture shall be
  authoritative, visible, deterministic, AI-usable, saved, replayed, and tested. Source data values
  that are not currently loaded or enforced do not satisfy this requirement.
* **REL-WEL-015 — Harvest.** Harvest shall publicly telegraph for 180 ticks, grant 500 Dawn only on
  successful completion, permanently collapse the Well, and apply the exact named map consequence.
  Breaking control during the telegraph shall interrupt it. The UI shall identify Harvest as
  immediate tempo purchased with loss of recurring income and future options.
* **REL-WEL-016 — Preserve.** Preserve shall leave the Well contestable, grant 15 Dawn every
  300 ticks only to its current controller, and grant the authored faction-appropriate intelligence
  within 1,400 cm. Ownership changes shall transfer—not duplicate—benefits. It shall be presented as
  compounding value that requires defense, not a permanent uncontestable selection.
* **REL-WEL-017 — Reshape.** Reshape shall cost 120 Dawn, publicly telegraph for 180 ticks, manifest
  one exact map-authored possibility for 1,800 ticks, show expiration, and warn affected units.
  Expiration shall use authored fallback displacement and never geometry-kill a unit. Both players
  shall see and be able to exploit the changed route. A random unused variant or fixed unexplained
  opening does not satisfy this requirement.
* **REL-WEL-018 — Strategic neutrality.** No Well protocol shall be a moral-score or automatic-win
  button. Harvest shall serve emergency/decisive tempo; Preserve sustained control/intelligence;
  Reshape a temporary route, cover, denial, evacuation, reinforcement, or escape window. Maps,
  objectives, and AI shall make each choice situationally rational and counterable.
* **REL-ART-021 — Gameplay-truthful terrain.** Ash Cut shall read as constrained/irregular, Buried
  Causeway as broad/direct, and Folded Verge as temporary/zigzag. Open, Blocked, and Scarred effects
  shall be disclosed where mechanically different. Decorative geometry shall not falsely imply
  collision, cover, pathing, vision blocking, power, or interaction.
* **REL-ART-022 — Cover truth.** Bulwark protection and Cairnback Mineral Cover shall show exact
  facing, footprint, duration or state, affected attacks, damage/integrity, and counterplay. Neither
  shall look like universal armor or indestructible world geometry.
* **REL-ART-023 — Ecological signal boundary.** Shivergrass may indicate possible footfall without
  unit identity. Vaultbacks, pale tides, and other ecology shall remain non-interactable worldbuilding
  until rules, telegraphs, effects, AI response, and counterplay are authored and accepted.

### Faction strategy acceptance

* **REL-AI-022 — Meridian strategy.** Human and Standard AI play shall demonstrate that Relay
  scouting, connected logistics/power, correctly faced Bulwarks, Lancer fire, and powered Aegis
  defense form a coherent but counterable strategy. Flanking, link severance, Skiff loss, bypass,
  and forced redeployment shall create observable counterplay.
* **REL-AI-023 — Kharuun strategy.** Play shall demonstrate that vibration information, mobile
  skirmishing, temporary cover, adaptation, and post-contact Waystone migration form a coherent but
  counterable strategy. Molt, rooting, migration exposure, stationary deception, and prolonged
  frontal combat shall create counterplay.
* **REL-AI-024 — Choir strategy.** Play shall demonstrate that Manifest/Possible timing and Dawn
  allocation among units, research, and recurring structure coherence form a coherent but
  counterable strategy. Overspending, public transitions, cooldowns, and upkeep deadlines shall
  create counterplay. Undefined Warden, Afterimage, and Phase Anchor roles shall be resolved before
  this requirement may pass.
* **REL-AI-025 — Advantage conversion.** Standard AI and tutorial scenarios shall demonstrate how
  income, vision, Well, or combat advantage becomes Core pressure and how a defender trades space,
  repairs/rebuilds if supported, attacks reinforcements, or counterattacks. Passive accumulation
  without an endgame plan shall not be the only viable behavior.
* **REL-AI-026 — Skirmish contract.** Release skirmish setup shall show the selected factions, map,
  AI doctrine and difficulty, starting resources, Corefall victory/defeat, and any owner-approved
  speed or team options before deployment. A `Balanced` AI option shall be removed or receive a
  distinct authored doctrine and verified behavior. The setup shall not imply unsupported mirror,
  team, handicap, alternate-victory, or multiplayer capability.
* **REL-AI-027 — Layered AI architecture.** Single-player opponents shall separate a fair strategic
  controller, tactical controllers, economy/production management, reconnaissance, and a
  mission-specific director. The strategic controller shall expose deterministic states including
  `ESTABLISH ECONOMY`, `SCOUT`, `EXPAND`, `DEFEND`, `ASSEMBLE`, `ATTACK`, `RAID`, `CONTEST WELL`,
  `RETREAT`, and `RECOVER`. Authored transition inputs, thresholds, minimum dwell time, cooldowns,
  and fallback states shall prevent oscillation and deadlock.
* **REL-AI-028 — Mission director separation.** Scripted waves, dialogue beats, reinforcements,
  environmental changes, and cinematic events shall be controlled by the mission director rather
  than represented as ordinary AI production. The event ledger shall identify which units were
  produced through the economy and which were director-authored. The opponent shall not receive
  unbounded reinforcements under the appearance of normal production.
* **REL-AI-029 — Fair information model.** Standard AI shall make decisions only from its
  player-scoped view, completed scouting, anonymous faction sensor contacts, authored public events,
  and remembered observations with explicit age/confidence. It shall pay the same costs, wait the
  same construction/production/research/ability times, obey the same Logistics, pathing, fog,
  cooldown, range, and Well-control rules, and receive no hidden income. Any campaign-specific
  exception shall be a disclosed director rule, not concealed as opponent intelligence.
* **REL-AI-030 — Dynamic threat assessment.** AI shall assess only observed or reasonably remembered
  enemy composition, location, damage potential, mobility, production evidence, exposed economy,
  route pressure, and objective threat. It shall record observation age and confidence, choose
  faction-supported responses, and revise the assessment after contrary evidence. It shall not build
  a counter to an unobserved unit, know a hidden base location, or infer a player queue directly.
* **REL-AI-031 — Perceived intelligence behaviors.** Each supported doctrine and faction shall scout,
  protect workers and critical infrastructure, retreat damaged or outmatched forces, regroup,
  reinforce, expand to known resources, contest or concede the Well intentionally, exploit an
  observed weak route, defend a threatened Core, and convert advantage into objective/Core pressure.
  These behaviors shall arise from the AI's fair state rather than cinematic animation alone.
* **REL-AI-032 — Doctrine differentiation.** Warden/Defensive, Raider, Steward/Economic,
  Expansionist, and Adaptive shall each have an accepted decision table covering worker targets,
  production mix, expansion threshold, scout budget, defense radius, attack threshold, retreat
  threshold, Well preference, research timing, faction-ability use, and recovery. `Balanced` remains
  excluded unless it receives a sixth distinct table and owner acceptance.
* **REL-AI-033 — Scalable difficulty.** Difficulty shall scale declared planning horizon, reaction
  delay, threat-estimate accuracy from observed evidence, coordination, command cadence, retreat
  discipline, target selection, and tolerance for mistakes. Command cadence shall remain within an
  owner-accepted human-reasonable ceiling; higher difficulty shall not gain unlimited APM. Standard
  shall use equal resources and information. Any assisted resource, build-time, vision, or damage
  modifier shall be named numerically before play and recorded in save/replay/result metadata.
* **REL-AI-034 — AI reconnaissance parity.** AI scouts shall obey the same reachable terrain, fog,
  sensor, threat-response, and discovery rules as player-issued reconnaissance. AI may select search
  priorities from doctrine and known objectives but shall not receive undiscovered resource, Well,
  route, or enemy locations. Automated tests shall challenge it with decoys, stationary enemies,
  destroyed routes, moved economies, and stale contacts.
* **REL-AI-035 — AI recovery and surrender.** AI shall detect stalled workers, blocked production,
  lost drop-offs, capacity loss, disconnected power, interrupted migration/molt/reconciliation,
  destroyed scouts, and failed attacks and transition to a bounded recovery plan. It shall concede
  only when an authored recoverability assessment shows no meaningful path to protect or replace a
  Core and the mode permits concession; it shall not prolong a decided match through inert hiding.
* **REL-AI-036 — AI acceptance.** Each faction, doctrine, difficulty, map, and campaign-use case shall
  have deterministic scenario tests, adversarial hidden-information tests, long-run stall/soak tests,
  packaged observation, and human play. Acceptance shall measure rule compliance, scouting provenance,
  state transitions, recovery, tactical readability, challenge, frustration, and strategic variety;
  win rate alone shall not establish intelligence or fairness.
* **REL-MP-017 — Development network boundary.** The current localhost fixed-rules direct-connect
  path is a development validation surface, not release multiplayer. It shall remain inaccessible in
  release-facing UI unless the conditional multiplayer scope is activated and REL-MP-001..016 plus
  security, reachability, lobby, configuration, reconnect, abuse, packaging, and human requirements
  are satisfied.

### Purpose-map closure gate

* **REL-QA-033 — No purposeless element.** Every release-accessible unit, structure, technology,
  resource, terrain type, objective, command, alert, and map interaction shall have one documented
  primary player purpose, a meaningful tradeoff, a selection/inspection presentation, an accepted
  action or passive rule, an observable effect, an exact failure state, and opponent counterplay.
  Duplicate elements that create the same decision in the same way shall be differentiated,
  combined, or removed with owner approval.
* **REL-QA-034 — No fiction-only verbs.** Tooltip verbs such as repair, stabilize, recover,
  misdirect, cohere, power, detect, adapt, preserve, and reshape shall correspond to observable
  authoritative rules. A role name, lore line, visual effect, or campaign fixture alone does not
  satisfy a gameplay requirement.
* **REL-QA-035 — Purpose comprehension.** For every roster element used in the initial release, at
  least four of five uncoached representative players shall correctly identify what it does, when it
  helps, one circumstance where it does not help, and how to issue its primary action. For Corefall,
  every tester shall identify win and loss conditions before the first match.
* **REL-QA-036 — Evidence separation.** Automated tests shall prove authoritative rules and failure
  behavior; rendered inspection shall prove presentation; physical play shall prove operability;
  uncoached human play shall prove comprehension and strategic usefulness. No class substitutes for
  another, and all final evidence shall bind the exact source and packaged candidate.

## Platform-expansion and graphics-scalability derived requirements

Authority for this section: direct owner instruction of 2026-09-03 — the game is being built for macOS, a
Linux and a Windows version follow after it is finished, and discrete graphics cards with expanded graphics
options are to be supported later. Combined with `CLAUDE.md` (macOS ships first; Linux/SteamOS then Windows
in later releases), `Docs/GameCompletionDirective.md` Track I4 and gate 45, and the numeric, serialization,
and preset clauses of `Docs/Archive/TechnicalArchitecture.md`.

These are **derived** records: each states what an already-binding commitment implies for work performed
**now**, on the macOS initial release. Most make an existing architectural intent testable rather than adding
scope; where a record does add cost it says so and defers to `TBR-SCP-011`. None of them is a Linux, SteamOS,
Windows, or graphics-hardware commitment, and none may support a claim about any platform or device the
project has not built, run, and measured. All records are `OPEN`.

### Platform portability constraints — REL-PORT-001..010

* **REL-PORT-001 — Portability constraint authority.** *(DERIVED FROM owner instruction 2026-09-03,
  `CLAUDE.md` platform roadmap, `GameCompletionDirective.md` Track I4.)* The platform roadmap after the
  initial release shall be Linux/SteamOS, then Windows. REL-PORT-002..010 shall bind macOS initial-release
  work so that those releases are ports rather than rewrites. They shall not be read as a platform
  commitment, schedule, build, support statement, or addition to the 1.0 scope recorded above.
* **REL-PORT-002 — Toolchain and architecture determinism.** *(DERIVED FROM REL-SIM-001..012,
  `TechnicalArchitecture.md` §Numeric model and §Deterministic simulation contract.)* `EchoesSimCore` shall
  produce identical tick state, canonical snapshot bytes, and checksums for identical input under every
  roadmap toolchain and architecture, not only Apple Clang on `arm64`. The signed Q22.10 fixed-point and
  integer model shall carry explicit width, signedness, shift, rounding, and overflow behavior with no
  implementation-defined or undefined-behavior dependence, and no dependence on `std::size_t` width, pointer
  values, address-ordered iteration, unordered-container iteration order, locale, or wall clock. Acceptance:
  the deterministic core suite passes under a second toolchain and a second architecture on the same input
  set with byte-identical snapshots and checksums. Verification: `SRC` + `PKG-AUTO`. Cost and method:
  `TBR-SCP-011`.
* **REL-PORT-003 — Asset and content path portability.** *(DERIVED FROM REL-GOV-*, REL-ART-*, REL-DIST-*;
  no existing record covers this.)* Content source, compiled catalogs, packaged asset references, save
  containers, and script inputs shall resolve identically on a case-sensitive filesystem and under both
  Unicode normalization forms. No two files reachable by the build or the runtime shall differ only by case;
  every path reference shall match on-disk case exactly; filenames shall exclude characters and reserved
  names invalid on the roadmap platforms (`\ : * ? " < > |`, trailing dot or space, `CON`, `PRN`, `AUX`,
  `NUL`, `COM1`–`COM9`, `LPT1`–`LPT9`); paths shall be forward-slashed with no drive letter and no absolute
  host path. Enforcement runs in content compilation and packaging and fails closed. Verification: `SRC` +
  `PKG-AUTO`. Rationale: the macOS development filesystem is case-insensitive and normalizes to NFD, so every
  violation of this record is invisible until the first Linux build, when all of them surface at once.
* **REL-PORT-004 — Container encoding portability.** *(DERIVED FROM REL-SAV-*, REL-QOL-*,
  `TechnicalArchitecture.md` §Canonical serialization.)* Snapshots, replays, campaign progress, saves,
  profiles, settings, and the canonical rules pack shall use the encoding discipline that architecture
  already requires — fixed-width little-endian integers, declared byte widths for booleans and enums, counted
  and canonically ordered vectors, UTF-8 strings with byte limits — and shall contain no padding, native
  struct dump, RTTI name, pointer, `UObject` path, locale formatting, or compiler-dependent container layout.
  A container written on one roadmap platform shall load on another to an identical checksum, or fail closed
  with an explicit reason. Verification: `SRC` + `PKG-AUTO`.
* **REL-PORT-005 — Input and binding portability.** *(DERIVED FROM REL-UI-*, DEMO-INP-*.)* Bindings and
  modifier semantics shall be data, so per-platform defaults (Command on macOS, Control on Linux and Windows)
  can differ without a code change, a settings-schema break, or a change in accepted binding behavior. No
  requirement shall specify a physical input in terms of a macOS-only API or a macOS-only key name.
  Verification: `SRC`; the 1.0 behavior itself remains `PKG-PHYS` on macOS.
* **REL-PORT-006 — Dependency portability register.** *(DERIVED FROM `GameCompletionDirective.md` Track I4
  and gate 45.)* Every third-party dependency, engine plugin, engine feature, and platform API in the release
  shall carry a recorded Linux and Windows availability status, the named replacement where it is
  unavailable, and the accepted blocker where none exists. The register shall be updated when a dependency is
  added, not reconstructed once at release; the Track I4 audit shall read this register. Verification: `SRC`.
* **REL-PORT-007 — Generator and compiler platform behavior.** *(DERIVED FROM `CLAUDE.md` rule 3,
  REL-ART-*, REL-AUD-*.)* The registered asset, audio, and content generators required to be byte-idempotent
  under their recorded revision shall either produce byte-identical registered output on every roadmap
  platform, or record the exact platform dependence, its cause, and its accepted disposition in
  `Docs/Archive/AssetRegister.md`. A provenance claim that holds only on macOS shall say so. Verification:
  `SRC` + `PKG-AUTO`.
* **REL-PORT-008 — Portability guard runs continuously.** *(DERIVED FROM REL-PORT-003, -004, -007.)* Those
  constraints shall be enforced by automated checks inside the ordinary test suite that fail on violation. A
  violation found by a release-time audit has already been paid for; the guard exists to fail on the commit
  that introduced it. Verification: `PKG-AUTO`. Scope and cost: `TBR-SCP-011`.
* **REL-PORT-009 — No unsupported platform claim.** *(DERIVED FROM DEMO-PERF-014, REL-PUB-*, REL-GOV-*.)*
  Nothing in this section authorizes a Linux, SteamOS, or Windows claim in the game, store text, website,
  manual, support material, or evidence. Only a package built, run, and human-accepted on that platform may
  support such a claim. Verification: `SRC` + `OWNER`.
* **REL-PORT-010 — Roadmap modules are dormant.** *(DERIVED FROM the REL-MP-001..016 dormant-module
  pattern.)* The Linux/SteamOS and Windows release modules are `DORMANT`. Activating either requires its own
  complete requirement set — build host, toolchain, engine target, packaging, signing and store, input, GPU
  and driver matrix, per-platform performance budgets, clean-machine qualification, support, and human
  acceptance. Satisfying REL-PORT-001..009 shall never be represented as partial completion of either module.

### Graphics-hardware and options expansion — REL-PERF-019..025

* **REL-PERF-019 — Quality tiers are data, not code.** *(DERIVED FROM owner instruction 2026-09-03,
  `CLAUDE.md` M1 Pro baseline with Nanite and Virtual Shadow Maps off, `TechnicalArchitecture.md` §Scalable
  presets.)* Every graphics quality tier and individual option shall be defined in versioned data carrying
  its scalability values and its supported-hardware predicate, not hardcoded against the M1 Pro baseline.
  Adding a tier, an option, or a device class shall require no code change, no settings-schema break, and no
  save migration. Verification: `SRC` + `PKG-AUTO`.
* **REL-PERF-020 — Settings forward and backward compatibility.** *(DERIVED FROM REL-SAV-*,
  `TechnicalArchitecture.md` §Settings model.)* The versioned settings model shall carry an explicit schema
  version, preserve unknown keys it did not write, supply the accepted default for an absent key, and never
  silently reset or discard a profile written by a different build. It shall fail closed only where a value
  cannot be safely interpreted, and shall report which value and why. Verification: `SRC` + `PKG-AUTO`.
* **REL-PERF-021 — Presentation cannot reach authority.** *(DERIVED FROM `CLAUDE.md` rule 2,
  `TechnicalArchitecture.md` §Graphics preset boundary, REL-SIM-019.)* No quality tier, graphics option,
  renderer path, GPU capability, feature level, resolution, or display mode shall change simulation state,
  fog authority, command validity or result, checksums, snapshots, replays, or AI information. Acceptance:
  the same replay executed at the lowest and the highest available configuration produces identical per-tick
  checksums. Verification: `PKG-AUTO`.
* **REL-PERF-022 — Budgets bind to a measured device class.** *(DERIVED FROM REL-PERF-007, `TBR-SCP-010`.)*
  The REL-PERF-007 budgets are bound to the M1 Pro baseline at the accepted preset, resolution, and scene.
  Every additional device class, GPU, or tier shall carry its own measured budget set and its own evidence. A
  budget met on one class shall never be reported as met on another, and an unmeasured class has no budget
  status at all. Verification: `SRC` ledger discipline plus the applicable profile run.
* **REL-PERF-023 — Capability detection and fallback.** *(DERIVED FROM `TechnicalArchitecture.md` renderer
  fallback clause; implied by leaving a single known SoC.)* The renderer shall determine actual device
  capability at runtime and select a supported configuration. A requested tier, option, or feature the device
  cannot provide shall fall back to a supported, readable, playable configuration, shall state the fallback
  in the settings UI and in the log, and shall never present a black screen, a crash, a hang, or a silently
  incorrect render. Verification: `PKG-REND` + `PKG-AUTO`.
* **REL-PERF-024 — Readability floor across every tier.** *(DERIVED FROM `CLAUDE.md` rule 6, REL-ACC-*,
  REL-UI-*.)* No tier or option shall remove a gameplay-relevant signal: team ownership marker, non-color
  ownership mark, selection state, fog and shroud boundary, alert, ability telegraph, construction or damage
  state, or health threshold. A configuration that removes one is not a valid configuration regardless of its
  frame time. Verification: `PKG-REND` + `HUM`.
* **REL-PERF-025 — Graphics expansion is dormant.** *(DERIVED FROM owner instruction 2026-09-03,
  `TBR-SCP-010`.)* Discrete-GPU support and the expanded graphics-option set are post-initial-release.
  REL-PERF-019..024 constrain the initial release so that expansion stays possible; they qualify no GPU,
  vendor, driver, graphics API, or option. No hardware claim shall be made without measured evidence on that
  hardware, and `TBR-SCP-010` remains the controlling scope decision for supported hardware.

## Release gates (§27)

R0 Demo accepted (all mandatory DEMO-*) → R1 Scope/content depth locked (all TBR resolved) →
R2 Core RTS complete → R3 Campaign complete (15 operations, 4 endings) → R4 Presentation
complete → R5 Accessibility/quality complete → R6 Release candidate qualified (frozen Shipping
candidate; clean-machine; signing; notarization; adversarial review) → R7 Public front door
accepted → R8 Owner release authorization. Every working goal names: parent gate, exact
requirement IDs, dependencies, files owned, evidence to produce, pass threshold, human
decision if any, reopening conditions. Vague goals ("improve graphics") are not executable.

## Final definition of initial-release done (§29)

Done only when: demo human-accepted; scope frozen and accurately represented; complete campaign
and all four endings through physical human play; all approved content complete; every
player-facing surface final; saves/recovery protect progress; graphics scale across verified
hardware; performance/stability budgets met; Shipping artifact signed, notarized, stapled,
installable, clean-machine qualified; rights/provenance/privacy/security/docs/website/support/
claims complete; zero S0/S1; all evidence on the same frozen candidate; Codex adversarial
review complete and every finding dispositioned; every mandatory requirement `HUMAN ACCEPTED`;
and Angelis explicitly authorizes public release. Until then the precise state vocabulary is:
prototype / implementation / integrated slice / demo candidate / release candidate /
evidence-ready / awaiting human acceptance — never "finished game."


## Declared without text — `REL-*`

The retired ledger declared **386** `REL-*` requirements across its §6–§26 section headers but
transcribed only 106 bodies. The **369** identifiers below were declared as ranges and have no
requirement text anywhere in the repository. The retired ledger stated that their statements were being
transcribed incrementally "with the owner's received order as the source of truth held by the
coordinator" — that source is not a file in this project.

They are listed individually so they can be counted, assigned, and closed. Each is `DECLARED — NO TEXT`:
not a requirement yet, and not evidence of one. A range string in a heading hid this; a list does not.

* **REL-ACC-001** — `DECLARED — NO TEXT`
* **REL-ACC-002** — `DECLARED — NO TEXT`
* **REL-ACC-003** — `DECLARED — NO TEXT`
* **REL-ACC-004** — `DECLARED — NO TEXT`
* **REL-ACC-005** — `DECLARED — NO TEXT`
* **REL-ACC-006** — `DECLARED — NO TEXT`
* **REL-ACC-007** — `DECLARED — NO TEXT`
* **REL-ACC-008** — `DECLARED — NO TEXT`
* **REL-ACC-009** — `DECLARED — NO TEXT`
* **REL-ACC-010** — `DECLARED — NO TEXT`
* **REL-ACC-011** — `DECLARED — NO TEXT`
* **REL-ACC-012** — `DECLARED — NO TEXT`
* **REL-ACC-013** — `DECLARED — NO TEXT`
* **REL-ACC-014** — `DECLARED — NO TEXT`
* **REL-ACC-015** — `DECLARED — NO TEXT`
* **REL-ACC-016** — `DECLARED — NO TEXT`
* **REL-ACC-017** — `DECLARED — NO TEXT`
* **REL-AI-001** — `DECLARED — NO TEXT`
* **REL-AI-002** — `DECLARED — NO TEXT`
* **REL-AI-003** — `DECLARED — NO TEXT`
* **REL-AI-004** — `DECLARED — NO TEXT`
* **REL-AI-005** — `DECLARED — NO TEXT`
* **REL-AI-006** — `DECLARED — NO TEXT`
* **REL-AI-007** — `DECLARED — NO TEXT`
* **REL-AI-008** — `DECLARED — NO TEXT`
* **REL-AI-009** — `DECLARED — NO TEXT`
* **REL-AI-010** — `DECLARED — NO TEXT`
* **REL-AI-011** — `DECLARED — NO TEXT`
* **REL-AI-012** — `DECLARED — NO TEXT`
* **REL-AI-013** — `DECLARED — NO TEXT`
* **REL-AI-014** — `DECLARED — NO TEXT`
* **REL-AI-015** — `DECLARED — NO TEXT`
* **REL-AI-016** — `DECLARED — NO TEXT`
* **REL-AI-017** — `DECLARED — NO TEXT`
* **REL-AI-018** — `DECLARED — NO TEXT`
* **REL-AI-019** — `DECLARED — NO TEXT`
* **REL-AI-020** — `DECLARED — NO TEXT`
* **REL-AI-021** — `DECLARED — NO TEXT`
* **REL-ART-001** — `DECLARED — NO TEXT`
* **REL-ART-002** — `DECLARED — NO TEXT`
* **REL-ART-003** — `DECLARED — NO TEXT`
* **REL-ART-004** — `DECLARED — NO TEXT`
* **REL-ART-005** — `DECLARED — NO TEXT`
* **REL-ART-006** — `DECLARED — NO TEXT`
* **REL-ART-007** — `DECLARED — NO TEXT`
* **REL-ART-008** — `DECLARED — NO TEXT`
* **REL-ART-009** — `DECLARED — NO TEXT`
* **REL-ART-010** — `DECLARED — NO TEXT`
* **REL-ART-011** — `DECLARED — NO TEXT`
* **REL-ART-012** — `DECLARED — NO TEXT`
* **REL-ART-013** — `DECLARED — NO TEXT`
* **REL-ART-014** — `DECLARED — NO TEXT`
* **REL-ART-015** — `DECLARED — NO TEXT`
* **REL-ART-016** — `DECLARED — NO TEXT`
* **REL-ART-017** — `DECLARED — NO TEXT`
* **REL-ART-018** — `DECLARED — NO TEXT`
* **REL-ART-019** — `DECLARED — NO TEXT`
* **REL-ART-020** — `DECLARED — NO TEXT`
* **REL-AUD-001** — `DECLARED — NO TEXT`
* **REL-AUD-002** — `DECLARED — NO TEXT`
* **REL-AUD-003** — `DECLARED — NO TEXT`
* **REL-AUD-004** — `DECLARED — NO TEXT`
* **REL-AUD-005** — `DECLARED — NO TEXT`
* **REL-AUD-006** — `DECLARED — NO TEXT`
* **REL-AUD-007** — `DECLARED — NO TEXT`
* **REL-AUD-008** — `DECLARED — NO TEXT`
* **REL-AUD-009** — `DECLARED — NO TEXT`
* **REL-AUD-010** — `DECLARED — NO TEXT`
* **REL-AUD-011** — `DECLARED — NO TEXT`
* **REL-AUD-012** — `DECLARED — NO TEXT`
* **REL-AUD-013** — `DECLARED — NO TEXT`
* **REL-AUD-014** — `DECLARED — NO TEXT`
* **REL-AUD-015** — `DECLARED — NO TEXT`
* **REL-BLD-001** — `DECLARED — NO TEXT`
* **REL-BLD-002** — `DECLARED — NO TEXT`
* **REL-BLD-003** — `DECLARED — NO TEXT`
* **REL-BLD-004** — `DECLARED — NO TEXT`
* **REL-BLD-005** — `DECLARED — NO TEXT`
* **REL-BLD-006** — `DECLARED — NO TEXT`
* **REL-BLD-007** — `DECLARED — NO TEXT`
* **REL-BLD-008** — `DECLARED — NO TEXT`
* **REL-BLD-009** — `DECLARED — NO TEXT`
* **REL-BLD-010** — `DECLARED — NO TEXT`
* **REL-BLD-011** — `DECLARED — NO TEXT`
* **REL-BLD-012** — `DECLARED — NO TEXT`
* **REL-BLD-013** — `DECLARED — NO TEXT`
* **REL-BLD-014** — `DECLARED — NO TEXT`
* **REL-CAM-001** — `DECLARED — NO TEXT`
* **REL-CAM-002** — `DECLARED — NO TEXT`
* **REL-CAM-003** — `DECLARED — NO TEXT`
* **REL-CAM-004** — `DECLARED — NO TEXT`
* **REL-CAM-005** — `DECLARED — NO TEXT`
* **REL-CAM-006** — `DECLARED — NO TEXT`
* **REL-CAM-007** — `DECLARED — NO TEXT`
* **REL-CAM-008** — `DECLARED — NO TEXT`
* **REL-CAM-009** — `DECLARED — NO TEXT`
* **REL-CAM-010** — `DECLARED — NO TEXT`
* **REL-CAM-011** — `DECLARED — NO TEXT`
* **REL-CAM-012** — `DECLARED — NO TEXT`
* **REL-CAM-013** — `DECLARED — NO TEXT`
* **REL-CAM-014** — `DECLARED — NO TEXT`
* **REL-CAM-015** — `DECLARED — NO TEXT`
* **REL-CAM-016** — `DECLARED — NO TEXT`
* **REL-CAM-017** — `DECLARED — NO TEXT`
* **REL-CAM-018** — `DECLARED — NO TEXT`
* **REL-CAM-019** — `DECLARED — NO TEXT`
* **REL-CAM-020** — `DECLARED — NO TEXT`
* **REL-CAM-021** — `DECLARED — NO TEXT`
* **REL-CIN-001** — `DECLARED — NO TEXT`
* **REL-CIN-002** — `DECLARED — NO TEXT`
* **REL-CIN-003** — `DECLARED — NO TEXT`
* **REL-CIN-004** — `DECLARED — NO TEXT`
* **REL-CIN-005** — `DECLARED — NO TEXT`
* **REL-CIN-006** — `DECLARED — NO TEXT`
* **REL-CIN-007** — `DECLARED — NO TEXT`
* **REL-CIN-008** — `DECLARED — NO TEXT`
* **REL-CMB-001** — `DECLARED — NO TEXT`
* **REL-CMB-002** — `DECLARED — NO TEXT`
* **REL-CMB-003** — `DECLARED — NO TEXT`
* **REL-CMB-004** — `DECLARED — NO TEXT`
* **REL-CMB-005** — `DECLARED — NO TEXT`
* **REL-CMB-006** — `DECLARED — NO TEXT`
* **REL-CMB-007** — `DECLARED — NO TEXT`
* **REL-CMB-008** — `DECLARED — NO TEXT`
* **REL-CMB-009** — `DECLARED — NO TEXT`
* **REL-CMB-010** — `DECLARED — NO TEXT`
* **REL-CMB-011** — `DECLARED — NO TEXT`
* **REL-CMB-012** — `DECLARED — NO TEXT`
* **REL-CMB-013** — `DECLARED — NO TEXT`
* **REL-CMB-014** — `DECLARED — NO TEXT`
* **REL-CMB-015** — `DECLARED — NO TEXT`
* **REL-CMB-016** — `DECLARED — NO TEXT`
* **REL-CMB-017** — `DECLARED — NO TEXT`
* **REL-CMB-018** — `DECLARED — NO TEXT`
* **REL-DIST-001** — `DECLARED — NO TEXT`
* **REL-DIST-002** — `DECLARED — NO TEXT`
* **REL-DIST-003** — `DECLARED — NO TEXT`
* **REL-DIST-004** — `DECLARED — NO TEXT`
* **REL-DIST-005** — `DECLARED — NO TEXT`
* **REL-DIST-006** — `DECLARED — NO TEXT`
* **REL-DIST-007** — `DECLARED — NO TEXT`
* **REL-DIST-008** — `DECLARED — NO TEXT`
* **REL-DIST-009** — `DECLARED — NO TEXT`
* **REL-DIST-010** — `DECLARED — NO TEXT`
* **REL-DIST-011** — `DECLARED — NO TEXT`
* **REL-DIST-012** — `DECLARED — NO TEXT`
* **REL-DIST-013** — `DECLARED — NO TEXT`
* **REL-DIST-014** — `DECLARED — NO TEXT`
* **REL-DIST-015** — `DECLARED — NO TEXT`
* **REL-DIST-016** — `DECLARED — NO TEXT`
* **REL-DIST-017** — `DECLARED — NO TEXT`
* **REL-ECO-001** — `DECLARED — NO TEXT`
* **REL-ECO-002** — `DECLARED — NO TEXT`
* **REL-ECO-003** — `DECLARED — NO TEXT`
* **REL-ECO-004** — `DECLARED — NO TEXT`
* **REL-ECO-005** — `DECLARED — NO TEXT`
* **REL-ECO-006** — `DECLARED — NO TEXT`
* **REL-ECO-007** — `DECLARED — NO TEXT`
* **REL-ECO-008** — `DECLARED — NO TEXT`
* **REL-ECO-009** — `DECLARED — NO TEXT`
* **REL-ECO-010** — `DECLARED — NO TEXT`
* **REL-ECO-011** — `DECLARED — NO TEXT`
* **REL-ECO-012** — `DECLARED — NO TEXT`
* **REL-ECO-013** — `DECLARED — NO TEXT`
* **REL-ECO-014** — `DECLARED — NO TEXT`
* **REL-FAC-001** — `DECLARED — NO TEXT`
* **REL-FAC-002** — `DECLARED — NO TEXT`
* **REL-FAC-003** — `DECLARED — NO TEXT`
* **REL-FAC-004** — `DECLARED — NO TEXT`
* **REL-FAC-005** — `DECLARED — NO TEXT`
* **REL-FAC-006** — `DECLARED — NO TEXT`
* **REL-FAC-007** — `DECLARED — NO TEXT`
* **REL-FAC-008** — `DECLARED — NO TEXT`
* **REL-FAC-009** — `DECLARED — NO TEXT`
* **REL-FAC-010** — `DECLARED — NO TEXT`
* **REL-FAC-011** — `DECLARED — NO TEXT`
* **REL-FAC-012** — `DECLARED — NO TEXT`
* **REL-FAC-013** — `DECLARED — NO TEXT`
* **REL-FTU-001** — `DECLARED — NO TEXT`
* **REL-FTU-002** — `DECLARED — NO TEXT`
* **REL-FTU-003** — `DECLARED — NO TEXT`
* **REL-FTU-004** — `DECLARED — NO TEXT`
* **REL-FTU-005** — `DECLARED — NO TEXT`
* **REL-FTU-006** — `DECLARED — NO TEXT`
* **REL-FTU-007** — `DECLARED — NO TEXT`
* **REL-FTU-008** — `DECLARED — NO TEXT`
* **REL-FTU-009** — `DECLARED — NO TEXT`
* **REL-FTU-010** — `DECLARED — NO TEXT`
* **REL-FTU-011** — `DECLARED — NO TEXT`
* **REL-FTU-012** — `DECLARED — NO TEXT`
* **REL-GOV-001** — `DECLARED — NO TEXT`
* **REL-GOV-002** — `DECLARED — NO TEXT`
* **REL-GOV-003** — `DECLARED — NO TEXT`
* **REL-GOV-004** — `DECLARED — NO TEXT`
* **REL-GOV-005** — `DECLARED — NO TEXT`
* **REL-GOV-006** — `DECLARED — NO TEXT`
* **REL-GOV-007** — `DECLARED — NO TEXT`
* **REL-GOV-008** — `DECLARED — NO TEXT`
* **REL-GOV-009** — `DECLARED — NO TEXT`
* **REL-GOV-010** — `DECLARED — NO TEXT`
* **REL-GOV-011** — `DECLARED — NO TEXT`
* **REL-GOV-012** — `DECLARED — NO TEXT`
* **REL-GOV-013** — `DECLARED — NO TEXT`
* **REL-GOV-014** — `DECLARED — NO TEXT`
* **REL-GOV-015** — `DECLARED — NO TEXT`
* **REL-LOC-001** — `DECLARED — NO TEXT`
* **REL-LOC-002** — `DECLARED — NO TEXT`
* **REL-LOC-003** — `DECLARED — NO TEXT`
* **REL-LOC-004** — `DECLARED — NO TEXT`
* **REL-LOC-005** — `DECLARED — NO TEXT`
* **REL-LOC-006** — `DECLARED — NO TEXT`
* **REL-MP-001** — `DECLARED — NO TEXT`
* **REL-MP-002** — `DECLARED — NO TEXT`
* **REL-MP-003** — `DECLARED — NO TEXT`
* **REL-MP-004** — `DECLARED — NO TEXT`
* **REL-MP-005** — `DECLARED — NO TEXT`
* **REL-MP-006** — `DECLARED — NO TEXT`
* **REL-MP-007** — `DECLARED — NO TEXT`
* **REL-MP-008** — `DECLARED — NO TEXT`
* **REL-MP-009** — `DECLARED — NO TEXT`
* **REL-MP-010** — `DECLARED — NO TEXT`
* **REL-MP-011** — `DECLARED — NO TEXT`
* **REL-MP-012** — `DECLARED — NO TEXT`
* **REL-MP-013** — `DECLARED — NO TEXT`
* **REL-MP-014** — `DECLARED — NO TEXT`
* **REL-MP-015** — `DECLARED — NO TEXT`
* **REL-MP-016** — `DECLARED — NO TEXT`
* **REL-PERF-001** — `DECLARED — NO TEXT`
* **REL-PERF-002** — `DECLARED — NO TEXT`
* **REL-PERF-003** — `DECLARED — NO TEXT`
* **REL-PERF-004** — `DECLARED — NO TEXT`
* **REL-PERF-005** — `DECLARED — NO TEXT`
* **REL-PERF-006** — `DECLARED — NO TEXT`
* **REL-PERF-007** — `DECLARED — NO TEXT`
* **REL-PERF-008** — `DECLARED — NO TEXT`
* **REL-PERF-009** — `DECLARED — NO TEXT`
* **REL-PERF-010** — `DECLARED — NO TEXT`
* **REL-PERF-011** — `DECLARED — NO TEXT`
* **REL-PERF-012** — `DECLARED — NO TEXT`
* **REL-PERF-013** — `DECLARED — NO TEXT`
* **REL-PERF-014** — `DECLARED — NO TEXT`
* **REL-PERF-015** — `DECLARED — NO TEXT`
* **REL-PERF-016** — `DECLARED — NO TEXT`
* **REL-PERF-017** — `DECLARED — NO TEXT`
* **REL-PERF-018** — `DECLARED — NO TEXT`
* **REL-PUB-001** — `DECLARED — NO TEXT`
* **REL-PUB-002** — `DECLARED — NO TEXT`
* **REL-PUB-003** — `DECLARED — NO TEXT`
* **REL-PUB-004** — `DECLARED — NO TEXT`
* **REL-PUB-005** — `DECLARED — NO TEXT`
* **REL-PUB-006** — `DECLARED — NO TEXT`
* **REL-PUB-007** — `DECLARED — NO TEXT`
* **REL-PUB-008** — `DECLARED — NO TEXT`
* **REL-PUB-009** — `DECLARED — NO TEXT`
* **REL-PUB-010** — `DECLARED — NO TEXT`
* **REL-PUB-011** — `DECLARED — NO TEXT`
* **REL-PUB-012** — `DECLARED — NO TEXT`
* **REL-PUB-013** — `DECLARED — NO TEXT`
* **REL-PUB-014** — `DECLARED — NO TEXT`
* **REL-PUB-015** — `DECLARED — NO TEXT`
* **REL-QA-001** — `DECLARED — NO TEXT`
* **REL-QA-002** — `DECLARED — NO TEXT`
* **REL-QA-003** — `DECLARED — NO TEXT`
* **REL-QA-004** — `DECLARED — NO TEXT`
* **REL-QA-005** — `DECLARED — NO TEXT`
* **REL-QA-006** — `DECLARED — NO TEXT`
* **REL-QA-007** — `DECLARED — NO TEXT`
* **REL-QA-008** — `DECLARED — NO TEXT`
* **REL-QA-009** — `DECLARED — NO TEXT`
* **REL-QA-010** — `DECLARED — NO TEXT`
* **REL-QA-011** — `DECLARED — NO TEXT`
* **REL-QA-012** — `DECLARED — NO TEXT`
* **REL-QA-013** — `DECLARED — NO TEXT`
* **REL-QA-014** — `DECLARED — NO TEXT`
* **REL-QA-015** — `DECLARED — NO TEXT`
* **REL-QA-016** — `DECLARED — NO TEXT`
* **REL-QA-017** — `DECLARED — NO TEXT`
* **REL-QA-018** — `DECLARED — NO TEXT`
* **REL-QA-019** — `DECLARED — NO TEXT`
* **REL-QA-020** — `DECLARED — NO TEXT`
* **REL-QA-021** — `DECLARED — NO TEXT`
* **REL-QA-022** — `DECLARED — NO TEXT`
* **REL-QA-023** — `DECLARED — NO TEXT`
* **REL-QA-024** — `DECLARED — NO TEXT`
* **REL-QA-025** — `DECLARED — NO TEXT`
* **REL-QA-026** — `DECLARED — NO TEXT`
* **REL-QA-027** — `DECLARED — NO TEXT`
* **REL-QA-028** — `DECLARED — NO TEXT`
* **REL-QA-029** — `DECLARED — NO TEXT`
* **REL-QA-030** — `DECLARED — NO TEXT`
* **REL-QA-031** — `DECLARED — NO TEXT`
* **REL-QA-032** — `DECLARED — NO TEXT`
* **REL-QOL-001** — `DECLARED — NO TEXT`
* **REL-QOL-002** — `DECLARED — NO TEXT`
* **REL-QOL-003** — `DECLARED — NO TEXT`
* **REL-QOL-004** — `DECLARED — NO TEXT`
* **REL-QOL-005** — `DECLARED — NO TEXT`
* **REL-QOL-006** — `DECLARED — NO TEXT`
* **REL-QOL-007** — `DECLARED — NO TEXT`
* **REL-QOL-008** — `DECLARED — NO TEXT`
* **REL-QOL-009** — `DECLARED — NO TEXT`
* **REL-QOL-010** — `DECLARED — NO TEXT`
* **REL-QOL-011** — `DECLARED — NO TEXT`
* **REL-QOL-012** — `DECLARED — NO TEXT`
* **REL-SAV-001** — `DECLARED — NO TEXT`
* **REL-SAV-002** — `DECLARED — NO TEXT`
* **REL-SAV-003** — `DECLARED — NO TEXT`
* **REL-SAV-004** — `DECLARED — NO TEXT`
* **REL-SAV-005** — `DECLARED — NO TEXT`
* **REL-SAV-006** — `DECLARED — NO TEXT`
* **REL-SAV-007** — `DECLARED — NO TEXT`
* **REL-SAV-008** — `DECLARED — NO TEXT`
* **REL-SAV-009** — `DECLARED — NO TEXT`
* **REL-SAV-010** — `DECLARED — NO TEXT`
* **REL-SAV-011** — `DECLARED — NO TEXT`
* **REL-SAV-012** — `DECLARED — NO TEXT`
* **REL-SAV-013** — `DECLARED — NO TEXT`
* **REL-SAV-014** — `DECLARED — NO TEXT`
* **REL-SEC-001** — `DECLARED — NO TEXT`
* **REL-SEC-002** — `DECLARED — NO TEXT`
* **REL-SEC-003** — `DECLARED — NO TEXT`
* **REL-SEC-004** — `DECLARED — NO TEXT`
* **REL-SEC-005** — `DECLARED — NO TEXT`
* **REL-SEC-006** — `DECLARED — NO TEXT`
* **REL-SIM-001** — `DECLARED — NO TEXT`
* **REL-SIM-002** — `DECLARED — NO TEXT`
* **REL-SIM-003** — `DECLARED — NO TEXT`
* **REL-SIM-004** — `DECLARED — NO TEXT`
* **REL-SIM-005** — `DECLARED — NO TEXT`
* **REL-SIM-006** — `DECLARED — NO TEXT`
* **REL-SIM-007** — `DECLARED — NO TEXT`
* **REL-SIM-008** — `DECLARED — NO TEXT`
* **REL-SIM-009** — `DECLARED — NO TEXT`
* **REL-SIM-010** — `DECLARED — NO TEXT`
* **REL-SIM-011** — `DECLARED — NO TEXT`
* **REL-SIM-012** — `DECLARED — NO TEXT`
* **REL-STAB-001** — `DECLARED — NO TEXT`
* **REL-STAB-002** — `DECLARED — NO TEXT`
* **REL-STAB-003** — `DECLARED — NO TEXT`
* **REL-STAB-004** — `DECLARED — NO TEXT`
* **REL-STAB-005** — `DECLARED — NO TEXT`
* **REL-UI-001** — `DECLARED — NO TEXT`
* **REL-UI-002** — `DECLARED — NO TEXT`
* **REL-UI-003** — `DECLARED — NO TEXT`
* **REL-UI-004** — `DECLARED — NO TEXT`
* **REL-UI-005** — `DECLARED — NO TEXT`
* **REL-UI-006** — `DECLARED — NO TEXT`
* **REL-UI-007** — `DECLARED — NO TEXT`
* **REL-UI-008** — `DECLARED — NO TEXT`
* **REL-UI-009** — `DECLARED — NO TEXT`
* **REL-UI-010** — `DECLARED — NO TEXT`
* **REL-UI-011** — `DECLARED — NO TEXT`
* **REL-UI-012** — `DECLARED — NO TEXT`
* **REL-UI-013** — `DECLARED — NO TEXT`
* **REL-UI-014** — `DECLARED — NO TEXT`
* **REL-UI-015** — `DECLARED — NO TEXT`
* **REL-UI-016** — `DECLARED — NO TEXT`
* **REL-WEL-001** — `DECLARED — NO TEXT`
* **REL-WEL-002** — `DECLARED — NO TEXT`
* **REL-WEL-003** — `DECLARED — NO TEXT`
* **REL-WEL-004** — `DECLARED — NO TEXT`
* **REL-WEL-005** — `DECLARED — NO TEXT`
* **REL-WEL-006** — `DECLARED — NO TEXT`
* **REL-WEL-007** — `DECLARED — NO TEXT`
* **REL-WEL-008** — `DECLARED — NO TEXT`
* **REL-WEL-009** — `DECLARED — NO TEXT`
* **REL-WEL-010** — `DECLARED — NO TEXT`
* **REL-WEL-011** — `DECLARED — NO TEXT`
* **REL-WEL-012** — `DECLARED — NO TEXT`


---

# Identifier index

All 995 identifiers in this document.

| ID | Requirement | Section |
|---|---|---|
| `SPEC-AUTH-001` | Single source of truth. This document owns the complete intended player experien | 1. Authority, interpretation, and change con |
| `SPEC-AUTH-002` | Normative language. Shall means mandatory. May means permitted. Tuning baseline  | 1. Authority, interpretation, and change con |
| `SPEC-AUTH-003` | No silent invention. When a required behavior is genuinely absent or contradicto | 1. Authority, interpretation, and change con |
| `SPEC-AUTH-004` | Traceable change. Every approved design change updates this file in place, prese | 1. Authority, interpretation, and change con |
| `SPEC-AUTH-005` | Purpose rule. Every accessible unit, building, technology, resource, terrain cla | 1. Authority, interpretation, and change con |
| `SPEC-AUTH-006` | Truthful acceptance. A design requirement, automated test, visual inspection, ph | 1. Authority, interpretation, and change con |
| `SPEC-PRD-001` | Genre | 2. Product definition and release boundary |
| `SPEC-PRD-002` | Camera and control | 2. Product definition and release boundary |
| `SPEC-PRD-003` | Primary experience | 2. Product definition and release boundary |
| `SPEC-PRD-004` | Player fantasy | 2. Product definition and release boundary |
| `SPEC-PRD-005` | Release platform | 2. Product definition and release boundary |
| `SPEC-PRD-006` | Campaign | 2. Product definition and release boundary |
| `SPEC-PRD-007` | Skirmish | 2. Product definition and release boundary |
| `SPEC-PRD-008` | Launch language | 2. Product definition and release boundary |
| `SPEC-PRD-009` | Expected match length | 2. Product definition and release boundary |
| `SPEC-PRD-010` | Business model | 2. Product definition and release boundary |
| `SPEC-CAN-001` | Central theme. Echoes of the Broken Sun is about the cost of making one future r | 3. Creative canon, setting, and lore |
| `SPEC-CAN-002` | Tone. The tone is urgent, humane, and occasionally dry. No faction is a proxy fo | 3. Creative canon, setting, and lore |
| `SPEC-CANON-001` | Before Crownfall | 3. Creative canon, setting, and lore |
| `SPEC-CANON-002` | First Impact Generations | 3. Creative canon, setting, and lore |
| `SPEC-CANON-003` | Ledger Peace | 3. Creative canon, setting, and lore |
| `SPEC-CANON-004` | Quiet Omissions | 3. Creative canon, setting, and lore |
| `SPEC-CANON-005` | Present War | 3. Creative canon, setting, and lore |
| `SPEC-CANON-006` | Meridian Compact | 3. Creative canon, setting, and lore |
| `SPEC-CANON-007` | Kharuun Assemblies | 3. Creative canon, setting, and lore |
| `SPEC-CANON-008` | Hollow Choir | 3. Creative canon, setting, and lore |
| `SPEC-CANON-009` | Commander Mara Vey | 3. Creative canon, setting, and lore |
| `SPEC-CANON-010` | Talar Venn | 3. Creative canon, setting, and lore |
| `SPEC-CANON-011` | Oruun-of-Seven-Stones | 3. Creative canon, setting, and lore |
| `SPEC-CANON-012` | Neme | 3. Creative canon, setting, and lore |
| `SPEC-CANON-013` | Chancellor Cael Rhyse | 3. Creative canon, setting, and lore |
| `SPEC-CANON-014` | Meridian Operations Annunciator | 3. Creative canon, setting, and lore |
| `SPEC-PIL-001` | Spatial economy | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-002` | Asymmetric planning | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-003` | Readable consequence | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-004` | Fair uncertainty | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-005` | Recoverable command | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-006` | Story through play | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-007` | Seconds | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-008` | Minutes | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-009` | Match | 4. Player experience pillars and gameplay lo |
| `SPEC-PIL-010` | Campaign | 4. Player experience pillars and gameplay lo |
| `SPEC-OUT-001` | Corefall victory. A player wins a standard skirmish when the opposing player has | 5. Match and operation outcomes |
| `SPEC-OUT-002` | Defeat. A player loses when the player's final Command Core is destroyed or the  | 5. Match and operation outcomes |
| `SPEC-OUT-003` | Draw. If both final Command Cores are destroyed in the same 20 Hz resolution tic | 5. Match and operation outcomes |
| `SPEC-OUT-004` | Campaign success. An operation is won only when every mandatory objective and re | 5. Match and operation outcomes |
| `SPEC-OUT-005` | Campaign failure. Each operation names failure predicates before play: required  | 5. Match and operation outcomes |
| `SPEC-OUT-006` | Result explanation. The result screen states the precise win/loss cause, optiona | 5. Match and operation outcomes |
| `SPEC-OUT-007` | Stalemate. At 45 minutes, skirmish warns that the match is prolonged but does no | 5. Match and operation outcomes |
| `SPEC-SIM-001` | Fixed time. Gameplay authority advances at 20 deterministic ticks per second. Pr | 6. Authoritative simulation and command rule |
| `SPEC-SIM-002` | Authority separation. Simulation owns entities, resources, commands, movement, t | 6. Authoritative simulation and command rule |
| `SPEC-SIM-003` | Command validation. Every command is validated against ownership, visibility, ta | 6. Authoritative simulation and command rule |
| `SPEC-SIM-004` | Determinism. The same initial state, content identifiers, player commands, and d | 6. Authoritative simulation and command rule |
| `SPEC-SIM-005` | Entity identity. Every authoritative entity has a stable identifier, faction, ow | 6. Authoritative simulation and command rule |
| `SPEC-SIM-006` | Limits. Entity and command limits fail visibly and safely. They never delete ano | 6. Authoritative simulation and command rule |
| `SPEC-SIM-007` | Player time. UI expresses durations in seconds, with optional detailed tick valu | 6. Authoritative simulation and command rule |
| `SPEC-CMD-001` | Move / Context | 6. Authoritative simulation and command rule |
| `SPEC-CMD-002` | Direct Attack | 6. Authoritative simulation and command rule |
| `SPEC-CMD-003` | Attack-move | 6. Authoritative simulation and command rule |
| `SPEC-CMD-004` | Patrol | 6. Authoritative simulation and command rule |
| `SPEC-CMD-005` | Guard | 6. Authoritative simulation and command rule |
| `SPEC-CMD-006` | Hold Position | 6. Authoritative simulation and command rule |
| `SPEC-CMD-007` | Stop | 6. Authoritative simulation and command rule |
| `SPEC-CMD-008` | Ability | 6. Authoritative simulation and command rule |
| `SPEC-CMD-009` | Rally | 6. Authoritative simulation and command rule |
| `SPEC-CMD-010` | Interact | 6. Authoritative simulation and command rule |
| `SPEC-MOV-001` | Ground-only domain. All controllable launch units use one surface-ground domain. | 7. Movement, pathfinding, formations, and te |
| `SPEC-MOV-002` | Pathfinding. Paths use deterministic destination-tile cost, known passability, u | 7. Movement, pathfinding, formations, and te |
| `SPEC-MOV-003` | Avoidance and body blocking. Enemy and neutral solid entities block movement. Al | 7. Movement, pathfinding, formations, and te |
| `SPEC-MOV-004` | Route change. When terrain changes under a route, units recalculate at the next  | 7. Movement, pathfinding, formations, and te |
| `SPEC-MOV-005` | Formations. Box provides compact travel, Line maximizes frontage perpendicular t | 7. Movement, pathfinding, formations, and te |
| `SPEC-TER-001` | Open | 7. Movement, pathfinding, formations, and te |
| `SPEC-TER-002` | Scarred | 7. Movement, pathfinding, formations, and te |
| `SPEC-TER-003` | Blocked | 7. Movement, pathfinding, formations, and te |
| `SPEC-TER-004` | Water / void | 7. Movement, pathfinding, formations, and te |
| `SPEC-TER-005` | Mineral Cover | 7. Movement, pathfinding, formations, and te |
| `SPEC-TER-006` | Subsurface Passage | 7. Movement, pathfinding, formations, and te |
| `SPEC-INFO-001` | Unexplored | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-002` | Explored | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-003` | Visible | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-004` | Last known | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-005` | Anonymous vibration | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-006` | Public event | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-FOG-001` | Single information boundary. World rendering, terrain, minimap, targeting, alert | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-FOG-002` | Alert anatomy. An alert has class, urgency, short text, world location when legi | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-007` | Explore Area | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-008` | Find Matter | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-009` | Locate Hostiles | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-INFO-010` | Screen Route | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-SCT-001` | Eligible scouts. Relay Skiff, Resonant, and Afterimage may use every reconnaissa | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-SCT-002` | Legal routing. Automation searches reachable unexplored frontier from player-kno | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-SCT-003` | Policies. CAUTIOUS reports and returns on contact; OBSERVE maintains the player- | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-SCT-004` | Player control. Selection shows mission, boundary, planned route, progress, disc | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-SCT-005` | Outcomes. Matter, Well, route, hostile unit, hostile structure, damage, blocked  | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-SCT-006` | Authority limit. Reconnaissance never selects a Well protocol, gathers a newly f | 8. Fog of war, intelligence, alerts, and rec |
| `SPEC-RES-001` | Matter | 9. Economy, resources, Logistics, and foreca |
| `SPEC-RES-002` | Dawn | 9. Economy, resources, Logistics, and foreca |
| `SPEC-RES-003` | Logistics | 9. Economy, resources, Logistics, and foreca |
| `SPEC-ECO-001` | Starting resources. Skirmish presets are Scarce 250 Matter/18 Dawn, Standard 400 | 9. Economy, resources, Logistics, and foreca |
| `SPEC-ECO-002` | Matter deposits. A standard deposit contains 1,500 Matter and supports three wor | 9. Economy, resources, Logistics, and foreca |
| `SPEC-ECO-003` | Automatic gather cycle. A Gather order repeats source → gather → valid drop-off  | 9. Economy, resources, Logistics, and foreca |
| `SPEC-ECO-004` | Drop-off choice. Workers choose the assigned operational drop-off; if none is as | 9. Economy, resources, Logistics, and foreca |
| `SPEC-ECO-005` | Depletion. A depleted deposit remains as a visible exhausted marker for 200 tick | 9. Economy, resources, Logistics, and foreca |
| `SPEC-ECO-006` | Logistics loss. Units already completed remain controllable when capacity falls  | 9. Economy, resources, Logistics, and foreca |
| `SPEC-BLD-001` | Placement. Construction preview shows faction-specific name, cost, footprint, ro | 10. Construction, production, repair, and re |
| `SPEC-BLD-002` | Payment. Construction and unit costs are paid when the order enters its active s | 10. Construction, production, repair, and re |
| `SPEC-BLD-003` | Builders. One worker builds at 100% rate. A second and third assist at 60% and 4 | 10. Construction, production, repair, and re |
| `SPEC-BLD-004` | Incomplete structures. An incomplete structure is targetable, has health proport | 10. Construction, production, repair, and re |
| `SPEC-BLD-005` | Cancellation. Cancel construction or unit production before 50% progress for a 7 | 10. Construction, production, repair, and re |
| `SPEC-BLD-006` | Queues. Each producer has a five-unit queue and one active slot. Units may be re | 10. Construction, production, repair, and re |
| `SPEC-BLD-007` | Emergence. A completed unit appears at the nearest free legal tile to the produc | 10. Construction, production, repair, and re |
| `SPEC-BLD-008` | Rally. The player may set one destination or a Shift-queued route. Rally paths m | 10. Construction, production, repair, and re |
| `SPEC-BLD-009` | Base rules. Players may build multiple production, supply, utility, and drop-off | 10. Construction, production, repair, and re |
| `SPEC-BLD-010` | Repair. All workers repair allied completed units and structures using the worke | 10. Construction, production, repair, and re |
| `SPEC-CMB-001` | Deterministic direct hit. Weapons have no random accuracy or critical hits. A va | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-002` | Damage model. Launch combat has one damage class and no armor-class multipliers. | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-003` | Projectiles. Ranged attacks create deterministic projectiles traveling 1,200 cm/ | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-004` | Line of fire. A ray from muzzle to target checks authored occluders, temporary c | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-005` | Friendly fire. Ordinary weapons and launch abilities do not damage allies or the | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-006` | Fire and movement. All units except Riftstalker Slipfire stop to fire. They rota | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-007` | Acquisition. Default target priority is immediate threat to self/guardee, then l | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-008` | Overkill. Units with no explicit focus order avoid launching damage already pred | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-009` | Death. At zero health, an entity loses authority immediately, plays faction/role | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-010` | Retreat. Retreat is ordinary movement, not a hidden morale state. No disengageme | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-011` | No hidden systems. There is no suppression, morale, stun, knockback, capture, st | 11. Combat resolution, stances, and counterp |
| `SPEC-STANCE-001` | Aggressive | 11. Combat resolution, stances, and counterp |
| `SPEC-STANCE-002` | Defensive (default) | 11. Combat resolution, stances, and counterp |
| `SPEC-STANCE-003` | Hold Position | 11. Combat resolution, stances, and counterp |
| `SPEC-STANCE-004` | Return Fire | 11. Combat resolution, stances, and counterp |
| `SPEC-STANCE-005` | Hold Fire | 11. Combat resolution, stances, and counterp |
| `SPEC-CMB-012` | Automation. Automatic ability use is disabled by default. A player may enable an | 11. Combat resolution, stances, and counterp |
| `SPEC-AUT-001` | Player-owned policy. Every automation is opt-in, visible on the selection card,  | 11. Combat resolution, stances, and counterp |
| `SPEC-AUT-002` | Worker auto-repair. Disabled by default. When enabled for a worker or control gr | 11. Combat resolution, stances, and counterp |
| `SPEC-AUT-003` | Worker flee. Disabled by default. The player selects a health threshold and owne | 11. Combat resolution, stances, and counterp |
| `SPEC-AUT-004` | Idle workers. A worker with no valid order remains idle, receives the idle-worke | 11. Combat resolution, stances, and counterp |
| `SPEC-AUT-005` | Reinforcement policy. Each production structure may optionally assign completed  | 11. Combat resolution, stances, and counterp |
| `SPEC-FACID-001` | Meridian Compact | 12. Factions and strategic identities |
| `SPEC-FACID-002` | Kharuun Assemblies | 12. Factions and strategic identities |
| `SPEC-FACID-003` | Hollow Choir | 12. Factions and strategic identities |
| `SPEC-UNIT-001` | Surveyor (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-002` | Lancer (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-003` | Bulwark Team (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-004` | Relay Skiff (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-005` | Tender (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-006` | Riftstalker (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-007` | Cairnback (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-008` | Resonant (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-009` | Threadkeeper (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-010` | Intervalist (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-011` | Lacuna Warden (unit) | 12. Factions and strategic identities |
| `SPEC-UNIT-012` | Afterimage (unit) | 12. Factions and strategic identities |
| `SPEC-STR-001` | Anchor (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-002` | Power Link (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-003` | Array Foundry (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-004` | Aegis Post (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-005` | Memory Hearth (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-006` | Waystone (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-007` | Growth Basin (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-008` | Listening Spine (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-009` | Concordance (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-010` | Interval Loom (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-011` | Chorus Loom (structure) | 13. Buildings and base-management actions |
| `SPEC-STR-012` | Phase Anchor (structure) | 13. Buildings and base-management actions |
| `SPEC-TECH-001` | Meridian Compact | 14. Technology and strategic progression |
| `SPEC-TECH-002` | Meridian Compact | 14. Technology and strategic progression |
| `SPEC-TECH-003` | Kharuun Assemblies | 14. Technology and strategic progression |
| `SPEC-TECH-004` | Kharuun Assemblies | 14. Technology and strategic progression |
| `SPEC-TECH-005` | Hollow Choir | 14. Technology and strategic progression |
| `SPEC-TECH-006` | Hollow Choir | 14. Technology and strategic progression |
| `SPEC-TEC-001` | Research visibility. The archive shows exact cost, time, effects, prerequisite,  | 14. Technology and strategic progression |
| `SPEC-TEC-002` | Strategic sufficiency. These two steps are the complete launch tree. Their purpo | 14. Technology and strategic progression |
| `SPEC-WELLP-001` | Harvest | 15. Future Wells |
| `SPEC-WELLP-002` | Preserve | 15. Future Wells |
| `SPEC-WELLP-003` | Reshape | 15. Future Wells |
| `SPEC-WEL-001` | Confirmation. Before commitment, three comparable cards show immediate benefit,  | 15. Future Wells |
| `SPEC-WEL-002` | Strategic neutrality. No protocol changes a hidden morality score or directly wi | 15. Future Wells |
| `SPEC-WEL-003` | Fog and replay. Capture, contest, telegraph, protocol, terrain change, timers, i | 15. Future Wells |
| `SPEC-AI-001` | Fair information. AI reads the same player-scoped terrain, vision, public events | 16. Single-player opponent AI |
| `SPEC-AI-002` | Equal rules. AI pays the same costs, waits the same times, obeys the same Logist | 16. Single-player opponent AI |
| `SPEC-AI-003` | Layered control. A strategic controller selects states; economy, production, sco | 16. Single-player opponent AI |
| `SPEC-AIST-001` | ESTABLISH ECONOMY | 16. Single-player opponent AI |
| `SPEC-AIST-002` | SCOUT | 16. Single-player opponent AI |
| `SPEC-AIST-003` | EXPAND | 16. Single-player opponent AI |
| `SPEC-AIST-004` | DEFEND | 16. Single-player opponent AI |
| `SPEC-AIST-005` | ASSEMBLE | 16. Single-player opponent AI |
| `SPEC-AIST-006` | ATTACK | 16. Single-player opponent AI |
| `SPEC-AIST-007` | RAID | 16. Single-player opponent AI |
| `SPEC-AIST-008` | CONTEST WELL | 16. Single-player opponent AI |
| `SPEC-AIST-009` | RETREAT | 16. Single-player opponent AI |
| `SPEC-AIST-010` | RECOVER | 16. Single-player opponent AI |
| `SPEC-DOC-001` | Warden | 16. Single-player opponent AI |
| `SPEC-DOC-002` | Raider | 16. Single-player opponent AI |
| `SPEC-DOC-003` | Steward | 16. Single-player opponent AI |
| `SPEC-DOC-004` | Expansionist | 16. Single-player opponent AI |
| `SPEC-DOC-005` | Adaptive | 16. Single-player opponent AI |
| `SPEC-DIF-001` | Story | 16. Single-player opponent AI |
| `SPEC-DIF-002` | Standard | 16. Single-player opponent AI |
| `SPEC-DIF-003` | Veteran | 16. Single-player opponent AI |
| `SPEC-DIF-004` | Sovereign | 16. Single-player opponent AI |
| `SPEC-AI-004` | Perceived intelligence. AI scouts, protects workers, retreats damaged forces, re | 16. Single-player opponent AI |
| `SPEC-AI-005` | Recovery and concession. AI diagnoses stalled workers, blocked spawn, lost drop- | 16. Single-player opponent AI |
| `SPEC-AI-006` | Mission director. Scripted waves, dialogue, reinforcements, hazards, and cinemat | 16. Single-player opponent AI |
| `SPEC-SKM-001` | Player faction | 17. Skirmish configuration and maps |
| `SPEC-SKM-002` | Opponent faction | 17. Skirmish configuration and maps |
| `SPEC-SKM-003` | Map | 17. Skirmish configuration and maps |
| `SPEC-SKM-004` | AI doctrine | 17. Skirmish configuration and maps |
| `SPEC-SKM-005` | Difficulty | 17. Skirmish configuration and maps |
| `SPEC-SKM-006` | Starting resources | 17. Skirmish configuration and maps |
| `SPEC-SKM-007` | Game speed | 17. Skirmish configuration and maps |
| `SPEC-SKM-008` | Map reveal | 17. Skirmish configuration and maps |
| `SPEC-SKM-009` | Victory | 17. Skirmish configuration and maps |
| `SPEC-SKM-010` | Pause | 17. Skirmish configuration and maps |
| `SPEC-SKM-011` | Glass Scar | 17. Skirmish configuration and maps |
| `SPEC-SKM-012` | Crownfall Basin | 17. Skirmish configuration and maps |
| `SPEC-SKM-013` | The Confluence Ring | 17. Skirmish configuration and maps |
| `SPEC-MAP-001` | Spawn fairness. Each map supports two fixed mirrored-distance spawn regions with | 17. Skirmish configuration and maps |
| `SPEC-MAP-002` | Map truth. Every map ships with a machine-readable contract for grid, starts, te | 17. Skirmish configuration and maps |
| `SPEC-MAP-003` | Post-match. Results show outcome cause, duration, resource curves, unit producti | 17. Skirmish configuration and maps |
| `SPEC-CAM-001` | Structure. The campaign contains fifteen operations: five in Necessary Fires, fi | 18. Campaign structure, persistence, and pro |
| `SPEC-CAM-002` | Feature introduction. Every operation has a capability manifest marking each uni | 18. Campaign structure, persistence, and pro |
| `SPEC-CAM-003` | Persistence. Mission completion, Well records, district allocation, public evide | 18. Campaign structure, persistence, and pro |
| `SPEC-CAM-004` | Rewards. Rewards change a later tactical or strategic decision and show exact ef | 18. Campaign structure, persistence, and pro |
| `SPEC-CAM-005` | Retry. Autosave occurs before deployment, at authored checkpoints, and after suc | 18. Campaign structure, persistence, and pro |
| `SPEC-CAM-006` | Branch clarity. The campaign map shows completed operations, known consequences, | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-001` | M01 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-002` | M02 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-003` | M03 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-004` | M04 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-005` | M05 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-006` | M06 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-007` | M07 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-008` | M08 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-009` | M09 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-010` | M10 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-011` | M11 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-012` | M12 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-013` | M13 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-014` | M14 | 18. Campaign structure, persistence, and pro |
| `SPEC-PLAN-015` | M15 | 18. Campaign structure, persistence, and pro |
| `SPEC-END-001` | Restoration | 18. Campaign structure, persistence, and pro |
| `SPEC-END-002` | Controlled Stabilization | 18. Campaign structure, persistence, and pro |
| `SPEC-END-003` | Extinguishment | 18. Campaign structure, persistence, and pro |
| `SPEC-END-004` | Open Evolution | 18. Campaign structure, persistence, and pro |
| `SPEC-CAM-007` | Ending confirmation. The player sees the earned subset, eligibility reasons, kno | 18. Campaign structure, persistence, and pro |
| `SPEC-MSN-001` | Operation: What the Ledger Keeps | 19. Mission objective and failure contracts |
| `SPEC-MSN-002` | Operation: Seven Accounts of Rain | 19. Mission objective and failure contracts |
| `SPEC-MSN-003` | Operation: A City on Reserve | 19. Mission objective and failure contracts |
| `SPEC-MSN-004` | Operation: The Unburied Road | 19. Mission objective and failure contracts |
| `SPEC-MSN-005` | Operation: Terms of Continuance | 19. Mission objective and failure contracts |
| `SPEC-MSN-006` | Operation: Names Without Births | 19. Mission objective and failure contracts |
| `SPEC-MSN-007` | Operation: The Shape of Silence | 19. Mission objective and failure contracts |
| `SPEC-MSN-008` | Operation: The Shape Beside Us | 19. Mission objective and failure contracts |
| `SPEC-MSN-009` | Operation: Reserve Authority | 19. Mission objective and failure contracts |
| `SPEC-MSN-010` | Operation: The Choir at Lume Reach | 19. Mission objective and failure contracts |
| `SPEC-MSN-011` | Operation: No Neutral Ledger | 19. Mission objective and failure contracts |
| `SPEC-MSN-012` | Operation: The Future That Won | 19. Mission objective and failure contracts |
| `SPEC-MSN-013` | Operation: Assembly of the Missing | 19. Mission objective and failure contracts |
| `SPEC-MSN-014` | Operation: Several Voices, One Command | 19. Mission objective and failure contracts |
| `SPEC-MSN-015` | Operation: The Broken Sun | 19. Mission objective and failure contracts |
| `SPEC-UI-001` | Selection answer. Every selection answers: what is it, what is it doing, what ca | 20. Interface, selection, controls, and play |
| `SPEC-UI-002` | Selection fields. Show faction, name, role, owner, health, order, stance, target | 20. Interface, selection, controls, and play |
| `SPEC-UI-003` | Mixed selection. Show composition and deterministic subgroups. The command deck  | 20. Interface, selection, controls, and play |
| `SPEC-UI-004` | Action feedback. Accepted actions use distinct non-color ground markers and audi | 20. Interface, selection, controls, and play |
| `SPEC-UI-005` | HUD fiction. The field HUD is Mara's command deck: a Compact operations instrume | 20. Interface, selection, controls, and play |
| `SPEC-HUD-001` | Top resource bar | 20. Interface, selection, controls, and play |
| `SPEC-HUD-002` | Objective panel | 20. Interface, selection, controls, and play |
| `SPEC-HUD-003` | Selection panel | 20. Interface, selection, controls, and play |
| `SPEC-HUD-004` | Command deck | 20. Interface, selection, controls, and play |
| `SPEC-HUD-005` | Production/research | 20. Interface, selection, controls, and play |
| `SPEC-HUD-006` | Minimap | 20. Interface, selection, controls, and play |
| `SPEC-HUD-007` | Alert history | 20. Interface, selection, controls, and play |
| `SPEC-CTL-001` | Left click / drag | 20. Interface, selection, controls, and play |
| `SPEC-CTL-002` | Shift + selection | 20. Interface, selection, controls, and play |
| `SPEC-CTL-003` | Double click | 20. Interface, selection, controls, and play |
| `SPEC-CTL-004` | Right click | 20. Interface, selection, controls, and play |
| `SPEC-CTL-005` | A / S / H / P / G | 20. Interface, selection, controls, and play |
| `SPEC-CTL-006` | B / R | 20. Interface, selection, controls, and play |
| `SPEC-CTL-007` | Q / W / E | 20. Interface, selection, controls, and play |
| `SPEC-CTL-008` | F | 20. Interface, selection, controls, and play |
| `SPEC-CTL-009` | C | 20. Interface, selection, controls, and play |
| `SPEC-CTL-010` | Ctrl + 1–0 | 20. Interface, selection, controls, and play |
| `SPEC-CTL-011` | Tab / Shift+Tab | 20. Interface, selection, controls, and play |
| `SPEC-CTL-012` | WASD / edge pan / middle drag / wheel | 20. Interface, selection, controls, and play |
| `SPEC-CTL-013` | Space | 20. Interface, selection, controls, and play |
| `SPEC-CTL-014` | F1 / F2 / F3 | 20. Interface, selection, controls, and play |
| `SPEC-CTL-015` | Escape / Pause | 20. Interface, selection, controls, and play |
| `SPEC-UI-006` | Remapping. Every gameplay command is remappable; collisions are rejected before  | 20. Interface, selection, controls, and play |
| `SPEC-LSN-001` | Survey | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-002` | Roster | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-003` | Section muster | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-004` | Route check | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-005` | Reserve | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-006` | Link restoration | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-007` | Foundry | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-008` | Perimeter probe | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-009` | The board | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-010` | The Well | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-LSN-011` | Readiness gate | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-TUT-001` | Fiction. Mara conducts an operational readiness check inside the prologue. There | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-TUT-002` | Hint ladder. After no relevant state change, first highlight the relevant UI, th | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-TUT-003` | Mastery. A lesson completes only from authoritative player action, not elapsed t | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-TUT-004` | Reference. A searchable in-game archive defines units, buildings, resources, ter | 21. Onboarding, tutorial, manual, and learni |
| `SPEC-ACC-001` | Non-color communication. Ownership, state, command, alert, danger, route, and pr | 22. Accessibility and localization |
| `SPEC-ACC-002` | Visual settings. HUD scale 80–150%; text scale; high contrast; color-vision pres | 22. Accessibility and localization |
| `SPEC-ACC-003` | Audio settings. Separate master, music, effects, dialogue, interface, and ambien | 22. Accessibility and localization |
| `SPEC-ACC-004` | Input. Full keyboard-only menu and field path, complete remapping, edge-pan togg | 22. Accessibility and localization |
| `SPEC-ACC-005` | Cognition. Pause, tutorial replay, glossary, persistent objective history, clear | 22. Accessibility and localization |
| `SPEC-LOC-001` | Externalized text. No player-facing string is hard-coded. Every string has an id | 22. Accessibility and localization |
| `SPEC-LOC-002` | English launch. English is the release language. Fonts, layout, input, saves, an | 22. Accessibility and localization |
| `SPEC-VISD-001` | Style | 23. Visual direction, animation, and effects |
| `SPEC-VISD-002` | Palette | 23. Visual direction, animation, and effects |
| `SPEC-VISD-003` | Meridian form | 23. Visual direction, animation, and effects |
| `SPEC-VISD-004` | Kharuun form | 23. Visual direction, animation, and effects |
| `SPEC-VISD-005` | Choir form | 23. Visual direction, animation, and effects |
| `SPEC-VISD-006` | World | 23. Visual direction, animation, and effects |
| `SPEC-VISD-007` | Effects | 23. Visual direction, animation, and effects |
| `SPEC-ART-001` | Unit readability. At normal gameplay camera, faction, role, ownership, selection | 23. Visual direction, animation, and effects |
| `SPEC-ART-002` | Animation set. Each unit has idle, locomotion, turn, acquire, wind-up, attack, r | 23. Visual direction, animation, and effects |
| `SPEC-ART-003` | Camera framing. Cinematics may use closer detail, but all gameplay assets must b | 23. Visual direction, animation, and effects |
| `SPEC-AUDF-001` | Meridian music | 24. Audio, music, voice, and cinematics |
| `SPEC-AUDF-002` | Kharuun music | 24. Audio, music, voice, and cinematics |
| `SPEC-AUDF-003` | Choir music | 24. Audio, music, voice, and cinematics |
| `SPEC-AUDF-004` | Effects | 24. Audio, music, voice, and cinematics |
| `SPEC-AUDF-005` | Interface | 24. Audio, music, voice, and cinematics |
| `SPEC-AUDF-006` | Voice | 24. Audio, music, voice, and cinematics |
| `SPEC-AUD-001` | Mix. Target integrated loudness is −16 LUFS ±1 with true peak no higher than −1  | 24. Audio, music, voice, and cinematics |
| `SPEC-AUD-002` | Cue completeness. Every command, rejection, production, research, construction,  | 24. Audio, music, voice, and cinematics |
| `SPEC-AUD-003` | Accessibility. Critical audio always has simultaneous visual/text information. S | 24. Audio, music, voice, and cinematics |
| `SPEC-CIN-001` | Cinematic set. The game includes a title sequence, campaign opening, act transit | 24. Audio, music, voice, and cinematics |
| `SPEC-CIN-002` | Control handoff. A cinematic states when control is removed, returns camera and  | 24. Audio, music, voice, and cinematics |
| `SPEC-SAV-001` | Transactional save. Manual save, autosave, checkpoint, settings, campaign ledger | 25. Saves, campaign records, and replays |
| `SPEC-SAV-002` | Slots. Campaign provides three named journeys plus autosave and one prior valida | 25. Saves, campaign records, and replays |
| `SPEC-SAV-003` | Compatibility. A save records schema, content identifiers, map, mission, seed, d | 25. Saves, campaign records, and replays |
| `SPEC-SAV-004` | Replay. Every match and completed operation may produce a deterministic command  | 25. Saves, campaign records, and replays |
| `SPEC-SAV-005` | Replay browser. Filter by mode, operation/map, faction, result, date, and durati | 25. Saves, campaign records, and replays |
| `SPEC-MOD-001` | Simulation core | 26. Technical architecture and content contr |
| `SPEC-MOD-002` | Game adapter | 26. Technical architecture and content contr |
| `SPEC-MOD-003` | Content compiler | 26. Technical architecture and content contr |
| `SPEC-MOD-004` | AI | 26. Technical architecture and content contr |
| `SPEC-MOD-005` | Mission director | 26. Technical architecture and content contr |
| `SPEC-MOD-006` | Save/replay | 26. Technical architecture and content contr |
| `SPEC-MOD-007` | Presentation | 26. Technical architecture and content contr |
| `SPEC-ARC-001` | Data ownership. Faction, unit, building, technology, Well, map, mission, dialogu | 26. Technical architecture and content contr |
| `SPEC-ARC-002` | Trust boundaries. Save files, replay files, settings, and content packs are untr | 26. Technical architecture and content contr |
| `SPEC-ARC-003` | Failure behavior. Missing or invalid optional presentation assets use a register | 26. Technical architecture and content contr |
| `SPEC-BUD-001` | Frame target | 27. Performance, stability, and platform |
| `SPEC-BUD-002` | Frame time | 27. Performance, stability, and platform |
| `SPEC-BUD-003` | Fog | 27. Performance, stability, and platform |
| `SPEC-BUD-004` | Path burst | 27. Performance, stability, and platform |
| `SPEC-BUD-005` | Memory | 27. Performance, stability, and platform |
| `SPEC-BUD-006` | Scale | 27. Performance, stability, and platform |
| `SPEC-BUD-007` | Save | 27. Performance, stability, and platform |
| `SPEC-BUD-008` | Stability | 27. Performance, stability, and platform |
| `SPEC-PLAT-001` | Display matrix. 1280×720, 1440×900, 1600×900, 1920×1080, 2560×1440, native Retin | 27. Performance, stability, and platform |
| `SPEC-PLAT-002` | Graphics scaling. Low, Medium, High, and Auto alter texture, shadow, effects, fo | 27. Performance, stability, and platform |
| `SPEC-PLAT-003` | Distribution. Ship a Release configuration app signed with Developer ID, notariz | 27. Performance, stability, and platform |
| `SPEC-PLAT-004` | Privacy. The game operates offline, creates no account, sends no telemetry by de | 27. Performance, stability, and platform |
| `SPEC-EVID-001` | Static/schema | 30. Verification and validation strategy |
| `SPEC-EVID-002` | Deterministic unit/system | 30. Verification and validation strategy |
| `SPEC-EVID-003` | Adversarial | 30. Verification and validation strategy |
| `SPEC-EVID-004` | Packaged physical play | 30. Verification and validation strategy |
| `SPEC-EVID-005` | Rendered/audio inspection | 30. Verification and validation strategy |
| `SPEC-EVID-006` | Uncoached player testing | 30. Verification and validation strategy |
| `SPEC-EVID-007` | Balance | 30. Verification and validation strategy |
| `SPEC-EVID-008` | Owner acceptance | 30. Verification and validation strategy |
| `SPEC-VAL-001` | Comprehension floor. At least four of five uncoached representative players corr | 30. Verification and validation strategy |
| `SPEC-VAL-002` | Balance floor. Across the approved automated-plus-human test set, no non-mirror  | 30. Verification and validation strategy |
| `SPEC-VAL-003` | No purposeless content. Every accessible element passes AUTH-005. A fiction-only | 30. Verification and validation strategy |
| `DEMO-ACC-001` | see Part II | Demo readiness |
| `DEMO-ACC-002` | see Part II | Demo readiness |
| `DEMO-ACC-003` | see Part II | Demo readiness |
| `DEMO-ACC-004` | see Part II | Demo readiness |
| `DEMO-ACC-005` | see Part II | Demo readiness |
| `DEMO-ACC-006` | see Part II | Demo readiness |
| `DEMO-AI-001` | see Part II | Demo readiness |
| `DEMO-AI-002` | see Part II | Demo readiness |
| `DEMO-AI-003` | see Part II | Demo readiness |
| `DEMO-AI-004` | see Part II | Demo readiness |
| `DEMO-AI-005` | see Part II | Demo readiness |
| `DEMO-AI-006` | see Part II | Demo readiness |
| `DEMO-AI-007` | see Part II | Demo readiness |
| `DEMO-AI-008` | see Part II | Demo readiness |
| `DEMO-AI-009` | see Part II | Demo readiness |
| `DEMO-AI-010` | see Part II | Demo readiness |
| `DEMO-AUD-001` | see Part II | Demo readiness |
| `DEMO-AUD-002` | see Part II | Demo readiness |
| `DEMO-AUD-003` | see Part II | Demo readiness |
| `DEMO-AUD-004` | see Part II | Demo readiness |
| `DEMO-AUD-005` | see Part II | Demo readiness |
| `DEMO-AUD-006` | see Part II | Demo readiness |
| `DEMO-AUD-007` | see Part II | Demo readiness |
| `DEMO-AUD-008` | see Part II | Demo readiness |
| `DEMO-AUD-009` | see Part II | Demo readiness |
| `DEMO-AUD-010` | see Part II | Demo readiness |
| `DEMO-AUD-011` | see Part II | Demo readiness |
| `DEMO-AUD-012` | see Part II | Demo readiness |
| `DEMO-AUD-013` | see Part II | Demo readiness |
| `DEMO-GOV-001` | see Part II | Demo readiness |
| `DEMO-GOV-002` | see Part II | Demo readiness |
| `DEMO-GOV-003` | see Part II | Demo readiness |
| `DEMO-GOV-004` | see Part II | Demo readiness |
| `DEMO-GOV-005` | see Part II | Demo readiness |
| `DEMO-GOV-006` | see Part II | Demo readiness |
| `DEMO-GOV-007` | see Part II | Demo readiness |
| `DEMO-GOV-008` | see Part II | Demo readiness |
| `DEMO-GOV-009` | see Part II | Demo readiness |
| `DEMO-GOV-010` | see Part II | Demo readiness |
| `DEMO-INP-001` | see Part II | Demo readiness |
| `DEMO-INP-002` | see Part II | Demo readiness |
| `DEMO-INP-003` | see Part II | Demo readiness |
| `DEMO-INP-004` | see Part II | Demo readiness |
| `DEMO-INP-005` | see Part II | Demo readiness |
| `DEMO-INP-006` | see Part II | Demo readiness |
| `DEMO-INP-007` | see Part II | Demo readiness |
| `DEMO-INP-008` | see Part II | Demo readiness |
| `DEMO-INP-009` | see Part II | Demo readiness |
| `DEMO-INP-010` | see Part II | Demo readiness |
| `DEMO-INP-011` | see Part II | Demo readiness |
| `DEMO-INP-012` | see Part II | Demo readiness |
| `DEMO-INP-013` | see Part II | Demo readiness |
| `DEMO-INP-014` | see Part II | Demo readiness |
| `DEMO-INP-015` | see Part II | Demo readiness |
| `DEMO-JRN-001` | see Part II | Demo readiness |
| `DEMO-JRN-002` | see Part II | Demo readiness |
| `DEMO-JRN-003` | see Part II | Demo readiness |
| `DEMO-JRN-004` | see Part II | Demo readiness |
| `DEMO-JRN-005` | see Part II | Demo readiness |
| `DEMO-JRN-006` | see Part II | Demo readiness |
| `DEMO-JRN-007` | see Part II | Demo readiness |
| `DEMO-NAR-001` | see Part II | Demo readiness |
| `DEMO-NAR-002` | see Part II | Demo readiness |
| `DEMO-NAR-003` | see Part II | Demo readiness |
| `DEMO-NAR-004` | see Part II | Demo readiness |
| `DEMO-NAR-005` | see Part II | Demo readiness |
| `DEMO-NAR-006` | see Part II | Demo readiness |
| `DEMO-NAR-007` | see Part II | Demo readiness |
| `DEMO-NAR-008` | see Part II | Demo readiness |
| `DEMO-NAR-009` | see Part II | Demo readiness |
| `DEMO-NAR-010` | see Part II | Demo readiness |
| `DEMO-NAR-011` | see Part II | Demo readiness |
| `DEMO-PERF-001` | see Part II | Demo readiness |
| `DEMO-PERF-002` | see Part II | Demo readiness |
| `DEMO-PERF-003` | see Part II | Demo readiness |
| `DEMO-PERF-004` | see Part II | Demo readiness |
| `DEMO-PERF-005` | see Part II | Demo readiness |
| `DEMO-PERF-006` | see Part II | Demo readiness |
| `DEMO-PERF-007` | see Part II | Demo readiness |
| `DEMO-PERF-008` | see Part II | Demo readiness |
| `DEMO-PERF-009` | see Part II | Demo readiness |
| `DEMO-PERF-010` | see Part II | Demo readiness |
| `DEMO-PERF-011` | see Part II | Demo readiness |
| `DEMO-PERF-012` | see Part II | Demo readiness |
| `DEMO-PERF-013` | see Part II | Demo readiness |
| `DEMO-PERF-014` | see Part II | Demo readiness |
| `DEMO-PERF-015` | see Part II | Demo readiness |
| `DEMO-TUT-001` | see Part II | Demo readiness |
| `DEMO-TUT-002` | see Part II | Demo readiness |
| `DEMO-TUT-003` | see Part II | Demo readiness |
| `DEMO-TUT-004` | see Part II | Demo readiness |
| `DEMO-TUT-005` | see Part II | Demo readiness |
| `DEMO-TUT-006` | see Part II | Demo readiness |
| `DEMO-TUT-007` | see Part II | Demo readiness |
| `DEMO-TUT-008` | see Part II | Demo readiness |
| `DEMO-TUT-009` | see Part II | Demo readiness |
| `DEMO-TUT-010` | see Part II | Demo readiness |
| `DEMO-TUT-011` | see Part II | Demo readiness |
| `DEMO-TUT-012` | see Part II | Demo readiness |
| `DEMO-TUT-013` | see Part II | Demo readiness |
| `DEMO-TUT-014` | see Part II | Demo readiness |
| `DEMO-TUT-015` | see Part II | Demo readiness |
| `DEMO-TUT-016` | see Part II | Demo readiness |
| `DEMO-TUT-017` | see Part II | Demo readiness |
| `DEMO-TUT-018` | see Part II | Demo readiness |
| `DEMO-TUT-019` | see Part II | Demo readiness |
| `DEMO-TUT-020` | see Part II | Demo readiness |
| `DEMO-TUT-021` | see Part II | Demo readiness |
| `DEMO-TUT-022` | see Part II | Demo readiness |
| `DEMO-UI-001` | see Part II | Demo readiness |
| `DEMO-UI-002` | see Part II | Demo readiness |
| `DEMO-UI-003` | see Part II | Demo readiness |
| `DEMO-UI-004` | see Part II | Demo readiness |
| `DEMO-UI-005` | see Part II | Demo readiness |
| `DEMO-UI-006` | see Part II | Demo readiness |
| `DEMO-UI-007` | see Part II | Demo readiness |
| `DEMO-UI-008` | see Part II | Demo readiness |
| `DEMO-UI-009` | see Part II | Demo readiness |
| `DEMO-UI-010` | see Part II | Demo readiness |
| `DEMO-UI-011` | see Part II | Demo readiness |
| `DEMO-UI-012` | see Part II | Demo readiness |
| `DEMO-UI-013` | see Part II | Demo readiness |
| `DEMO-VAL-001` | see Part II | Demo readiness |
| `DEMO-VAL-002` | see Part II | Demo readiness |
| `DEMO-VAL-003` | see Part II | Demo readiness |
| `DEMO-VAL-004` | see Part II | Demo readiness |
| `DEMO-VAL-005` | see Part II | Demo readiness |
| `DEMO-VAL-006` | see Part II | Demo readiness |
| `DEMO-VAL-007` | see Part II | Demo readiness |
| `DEMO-VAL-008` | see Part II | Demo readiness |
| `DEMO-VAL-009` | see Part II | Demo readiness |
| `DEMO-VAL-010` | see Part II | Demo readiness |
| `DEMO-VAL-011` | see Part II | Demo readiness |
| `DEMO-VAL-012` | see Part II | Demo readiness |
| `DEMO-VAL-013` | see Part II | Demo readiness |
| `DEMO-VAL-014` | see Part II | Demo readiness |
| `DEMO-VAL-015` | see Part II | Demo readiness |
| `DEMO-VAL-016` | see Part II | Demo readiness |
| `DEMO-VAL-017` | see Part II | Demo readiness |
| `DEMO-VIS-001` | see Part II | Demo readiness |
| `DEMO-VIS-002` | see Part II | Demo readiness |
| `DEMO-VIS-003` | see Part II | Demo readiness |
| `DEMO-VIS-004` | see Part II | Demo readiness |
| `DEMO-VIS-005` | see Part II | Demo readiness |
| `DEMO-VIS-006` | see Part II | Demo readiness |
| `DEMO-VIS-007` | see Part II | Demo readiness |
| `DEMO-VIS-008` | see Part II | Demo readiness |
| `DEMO-VIS-009` | see Part II | Demo readiness |
| `DEMO-VIS-010` | see Part II | Demo readiness |
| `DEMO-VIS-011` | see Part II | Demo readiness |
| `DEMO-VIS-012` | see Part II | Demo readiness |
| `DEMO-VIS-013` | see Part II | Demo readiness |
| `REL-ACC-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-ACC-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-018` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-019` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-020` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-021` | **DECLARED — NO TEXT** | Initial release |
| `REL-AI-022` | see Part III | Initial release |
| `REL-AI-023` | see Part III | Initial release |
| `REL-AI-024` | see Part III | Initial release |
| `REL-AI-025` | see Part III | Initial release |
| `REL-AI-026` | see Part III | Initial release |
| `REL-AI-027` | see Part III | Initial release |
| `REL-AI-028` | see Part III | Initial release |
| `REL-AI-029` | see Part III | Initial release |
| `REL-AI-030` | see Part III | Initial release |
| `REL-AI-031` | see Part III | Initial release |
| `REL-AI-032` | see Part III | Initial release |
| `REL-AI-033` | see Part III | Initial release |
| `REL-AI-034` | see Part III | Initial release |
| `REL-AI-035` | see Part III | Initial release |
| `REL-AI-036` | see Part III | Initial release |
| `REL-ART-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-018` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-019` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-020` | **DECLARED — NO TEXT** | Initial release |
| `REL-ART-021` | see Part III | Initial release |
| `REL-ART-022` | see Part III | Initial release |
| `REL-ART-023` | see Part III | Initial release |
| `REL-AUD-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-AUD-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-BLD-015` | see Part III | Initial release |
| `REL-BLD-016` | see Part III | Initial release |
| `REL-BLD-017` | see Part III | Initial release |
| `REL-BLD-018` | see Part III | Initial release |
| `REL-BLD-019` | see Part III | Initial release |
| `REL-BLD-020` | see Part III | Initial release |
| `REL-CAM-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-018` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-019` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-020` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-021` | **DECLARED — NO TEXT** | Initial release |
| `REL-CAM-022` | see Part III | Initial release |
| `REL-CAM-023` | see Part III | Initial release |
| `REL-CAM-024` | see Part III | Initial release |
| `REL-CAM-025` | see Part III | Initial release |
| `REL-CAM-026` | see Part III | Initial release |
| `REL-CAM-027` | see Part III | Initial release |
| `REL-CAM-028` | see Part III | Initial release |
| `REL-CAM-029` | see Part III | Initial release |
| `REL-CAM-030` | see Part III | Initial release |
| `REL-CAM-031` | see Part III | Initial release |
| `REL-CAM-032` | see Part III | Initial release |
| `REL-CIN-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-CIN-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-CIN-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-CIN-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-CIN-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-CIN-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-CIN-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-CIN-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-018` | **DECLARED — NO TEXT** | Initial release |
| `REL-CMB-019` | see Part III | Initial release |
| `REL-CMB-020` | see Part III | Initial release |
| `REL-CMB-021` | see Part III | Initial release |
| `REL-CMB-022` | see Part III | Initial release |
| `REL-CMB-023` | see Part III | Initial release |
| `REL-CMB-024` | see Part III | Initial release |
| `REL-CMB-025` | see Part III | Initial release |
| `REL-CMB-026` | see Part III | Initial release |
| `REL-CMB-027` | see Part III | Initial release |
| `REL-DIST-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-DIST-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-ECO-015` | see Part III | Initial release |
| `REL-ECO-016` | see Part III | Initial release |
| `REL-ECO-017` | see Part III | Initial release |
| `REL-FAC-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-FAC-014` | see Part III | Initial release |
| `REL-FAC-015` | see Part III | Initial release |
| `REL-FAC-016` | see Part III | Initial release |
| `REL-FAC-017` | see Part III | Initial release |
| `REL-FAC-018` | see Part III | Initial release |
| `REL-FAC-019` | see Part III | Initial release |
| `REL-FAC-020` | see Part III | Initial release |
| `REL-FAC-021` | see Part III | Initial release |
| `REL-FAC-022` | see Part III | Initial release |
| `REL-FAC-023` | see Part III | Initial release |
| `REL-FAC-024` | see Part III | Initial release |
| `REL-FAC-025` | see Part III | Initial release |
| `REL-FAC-026` | see Part III | Initial release |
| `REL-FAC-027` | see Part III | Initial release |
| `REL-FAC-028` | see Part III | Initial release |
| `REL-FAC-029` | see Part III | Initial release |
| `REL-FTU-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-FTU-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-GOV-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-LOC-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-LOC-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-LOC-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-LOC-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-LOC-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-LOC-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-MP-017` | see Part III | Initial release |
| `REL-PERF-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-018` | **DECLARED — NO TEXT** | Initial release |
| `REL-PERF-019` | see Part III | Initial release |
| `REL-PERF-020` | see Part III | Initial release |
| `REL-PERF-021` | see Part III | Initial release |
| `REL-PERF-022` | see Part III | Initial release |
| `REL-PERF-023` | see Part III | Initial release |
| `REL-PERF-024` | see Part III | Initial release |
| `REL-PERF-025` | see Part III | Initial release |
| `REL-PORT-001` | see Part III | Initial release |
| `REL-PORT-002` | see Part III | Initial release |
| `REL-PORT-003` | see Part III | Initial release |
| `REL-PORT-004` | see Part III | Initial release |
| `REL-PORT-005` | see Part III | Initial release |
| `REL-PORT-006` | see Part III | Initial release |
| `REL-PORT-007` | see Part III | Initial release |
| `REL-PORT-008` | see Part III | Initial release |
| `REL-PORT-009` | see Part III | Initial release |
| `REL-PORT-010` | see Part III | Initial release |
| `REL-PUB-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-PUB-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-017` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-018` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-019` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-020` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-021` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-022` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-023` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-024` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-025` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-026` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-027` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-028` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-029` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-030` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-031` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-032` | **DECLARED — NO TEXT** | Initial release |
| `REL-QA-033` | see Part III | Initial release |
| `REL-QA-034` | see Part III | Initial release |
| `REL-QA-035` | see Part III | Initial release |
| `REL-QA-036` | see Part III | Initial release |
| `REL-QOL-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-QOL-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-SAV-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-SEC-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-SEC-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-SEC-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-SEC-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-SEC-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-SEC-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-SIM-013` | see Part III | Initial release |
| `REL-SIM-014` | see Part III | Initial release |
| `REL-SIM-015` | see Part III | Initial release |
| `REL-SIM-016` | see Part III | Initial release |
| `REL-SIM-017` | see Part III | Initial release |
| `REL-SIM-018` | see Part III | Initial release |
| `REL-SIM-019` | see Part III | Initial release |
| `REL-STAB-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-STAB-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-STAB-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-STAB-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-STAB-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-013` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-014` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-015` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-016` | **DECLARED — NO TEXT** | Initial release |
| `REL-UI-017` | see Part III | Initial release |
| `REL-UI-018` | see Part III | Initial release |
| `REL-UI-019` | see Part III | Initial release |
| `REL-UI-020` | see Part III | Initial release |
| `REL-UI-021` | see Part III | Initial release |
| `REL-UI-022` | see Part III | Initial release |
| `REL-UI-023` | see Part III | Initial release |
| `REL-UI-024` | see Part III | Initial release |
| `REL-WEL-001` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-002` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-003` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-004` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-005` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-006` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-007` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-008` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-009` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-010` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-011` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-012` | **DECLARED — NO TEXT** | Initial release |
| `REL-WEL-013` | see Part III | Initial release |
| `REL-WEL-014` | see Part III | Initial release |
| `REL-WEL-015` | see Part III | Initial release |
| `REL-WEL-016` | see Part III | Initial release |
| `REL-WEL-017` | see Part III | Initial release |
| `REL-WEL-018` | see Part III | Initial release |
