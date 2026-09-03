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
| `MovementAndBalanceRequirements.md` | **Merged 2026-09-03 in full.** Formal owner-approved integration of all 20 requirements: `SPEC-MOV-006..013` and `SPEC-CTL-016..019` in §7.3, and `SPEC-BAL-001..008` in §16.4. Superseded and retired to historical source. |

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

## Specification Blueprint & Structural Framework

The normative requirements of *Echoes of the Broken Sun* are divided into two primary parts:
* **Part I — Core Specifications (`SPEC-*`):** Sections 1 through 31 define the deterministic simulation core, unit archetypes, game rules, and world systems.
* **Part II — Release Specifications (`REL-*`):** Sections §6 through §26 define commercial release requirements, content tracks, platform packaging, and QA gates.

### Core Architectural Pillars
* **Governance and Change Control:** Single source of truth, normative language (*shall* / *may*), zero silent invention, and atomic inline changes (governed by `SPEC-AUTH-001..006` and `REL-GOV-001..015`).
* **Visual-Auditory Infrastructure:** Silhouette-first tactical readability, 5-color palette, matte terrain (roughness $\ge 0.85$), and BS.1770-4 audio mastering at -16 LUFS (governed by `SPEC-ART-*`, `SPEC-AUD-*`, `REL-ART-*`, and `REL-AUD-*`).
* **Economy and Logistics:** Matter harvesting on a calibrated 20-tick cadence, deposit saturation, fail-closed Dawn accounting, and 200-cap Logistics networks (governed by `SPEC-RES-*`, `SPEC-LOG-*`, and `REL-ECO-001..020`).
* **Movement, Control, and Micro-Management:** Any-angle string-pulled movement, soft separation, 3-tick command responsiveness, and micro-management honoring (governed by `SPEC-MOV-001..013`, `SPEC-CTL-001..019`, and `REL-CMB-001..029`).
* **Tactical Interface and Interaction:** Production UMG/Slate framework, 3x3 command card, 7-target screen resolution matrix (1280x720 to 2560x1440), and 80%–150% UI scaling (governed by `SPEC-UI-*`, `SPEC-HUD-*`, and `REL-UI-001..026`).

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

### 6.1 Advanced Command Pipelining, Waypoint Visualization, and Smart-Casting
* **SPEC-CMD-011 — Shift-Queued Order Chaining:** Players shall chain sequential orders by holding `Shift` while issuing commands (Move, Gather, Build, Patrol, Ability). The unit shall execute orders sequentially without dropping waypoints, supporting a queue depth of up to 16 commands.
  * **SPEC-CMD-011.AUTH:** Completing a leg immediately advances to the next queued order on the subsequent simulation tick.
  * **SPEC-CMD-011.FAIL:** Dropping intermediate waypoints or freezing upon completing a leg fails queue processing.
  * **SPEC-CMD-011.VERIF:** `SRC` (multi-order shift-queueing test).
  * **SPEC-CMD-011.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-CMD-012 — Waypoint Vector Breadcrumb Visualization:** While holding `Shift` with units selected, the renderer shall project ground-projected dashed spline vectors connecting all queued destination coordinates, decorated with contextual order glyphs (Move, Attack, Gather, Build).
  * **SPEC-CMD-012.AUTH:** Vectors update dynamically in real-time as units progress along the path.
  * **SPEC-CMD-012.FAIL:** Desynchronized waypoint lines pointing to obsolete coordinates fails visual clarity.
  * **SPEC-CMD-012.VERIF:** `PKG-REND` (waypoint vector visual review).
  * **SPEC-CMD-012.LANE:** Visual Presentation (`EchoesEntityView`).

* **SPEC-CMD-013 — Smart-Cast Single-Unit Dispatch:** When an ability order (e.g. Cairnback Mineral Cover, Bulwark Deploy) is issued to a selection containing multiple eligible casters, the engine shall dispatch the order exclusively to the single closest unit possessing sufficient resources/energy.
  * **SPEC-CMD-013.AUTH:** Rapid sequential clicks dispatch the next closest eligible unit sequentially. Holding `Ctrl + Click` issues the order to all selected units simultaneously.
  * **SPEC-CMD-013.FAIL:** Single click triggering abilities across all 10 selected units simultaneously is prohibited.
  * **SPEC-CMD-013.VERIF:** `SRC` (smart-cast single-unit dispatch test).
  * **SPEC-CMD-013.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-CMD-014 — Attack-Move Intelligent Threat Filtering:** Units advancing under an Attack-Move command shall prioritize armed combatants, mobile threats, and defensive structures over passive non-threatening buildings (e.g. power lines, supply depots, unpowered structures).
  * **SPEC-CMD-014.AUTH:** If an enemy combat force engages an attack-moving army while passing an enemy farm/conduit, the army shall immediately engage the attacking combat units rather than dying while hitting the passive structure.
  * **SPEC-CMD-014.FAIL:** Army committing suicide against high-HP passive structures while enemy combatants wipe them out fails standard RTS tactical intelligence.
  * **SPEC-CMD-014.VERIF:** `PKG-AUTO` (attack-move threat filtering combat benchmark).
  * **SPEC-CMD-014.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-CMD-015 — Focus-Fire Target Preservation on Range Loss:** When a manually right-clicked target retreats out of weapon range, attacking units shall pursue the target up to a bounded chase radius of 400 cm. If the target exceeds this boundary, units halt pursuit and engage the closest hostile threat.
  * **SPEC-CMD-015.AUTH:** Prevents player armies from recklessly chasing a single scout across the entire map into fortified enemy bases.
  * **SPEC-CMD-015.FAIL:** Armies chasing retreating scouts into fog across unlimited distance fails micro-control.
  * **SPEC-CMD-015.VERIF:** `SRC` (focus-fire chase bound and retargeting test).
  * **SPEC-CMD-015.LANE:** Core Gameplay (`EchoesSimCore`).


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

### 7.3 Advanced Movement, Control Responsiveness, and Determinism (`SPEC-MOV-*`, `SPEC-CTL-*`)

* **SPEC-MOV-006 — Any-Angle Movement:** Mobile units shall move at arbitrary angles along the most direct path permitted by ground passability, string-pulling waypoints across the terrain distance field rather than staircasing along grid cell axes.
  * **SPEC-MOV-006.PRE:** Unit ordered to a passable ground destination on an unobstructed straight-line trajectory.
  * **SPEC-MOV-006.ACT:** Path solver computes trajectory using Euclidean ray-cast / integer supercover line-of-sight test (`Simulation::HasLineOfSight`).
  * **SPEC-MOV-006.AUTH:** On open ground, a unit ordered along an exact 45-degree diagonal shall deviate from the ideal trajectory by no more than 0.25 tiles at every tick of the journey.
  * **SPEC-MOV-006.FAIL:** If an obstacle intercepts the direct line during transit, the unit shall fall back to the nearest smoothed grid waypoint without stalling.
  * **SPEC-MOV-006.VERIF:** `SRC` (native simulation test: `any-angle movement takes straight lines`).
  * **SPEC-MOV-006.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-MOV-007 — Direction-Independent Speed:** A unit shall cover identical ground distance per simulation tick regardless of travel heading. Movement speed shall be a scalar property of the unit archetype, never skewed by grid axis orientation.
  * **SPEC-MOV-007.AUTH:** Distance traversed over a fixed tick duration on a diagonal heading shall match distance traversed on a cardinal axis to within 2.0% tolerance, using Euclidean metric normalisation (`IntegerSqrt64`).
  * **SPEC-MOV-007.FAIL:** Manhattan metric (|dx| + |dy|) is strictly prohibited; any regression resulting in ~29% diagonal speed attenuation shall fail automated validation.
  * **SPEC-MOV-007.VERIF:** `SRC` (native speed-ratio assertion across 8 cardinal and intercardinal headings).
  * **SPEC-MOV-007.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-MOV-008 — Soft Separation and Non-Imprisonment:** Allied mobile units sharing ground space shall maintain clearance via local steering forces, pushing past one another rather than colliding rigidly, and shall never permanently trap or imprison another allied unit.
  * **SPEC-MOV-008.AUTH:** No two allied mobile units shall remain overlapped beyond their combined clearance radii for more than 20 consecutive simulation ticks (1.0 second).
  * **SPEC-MOV-008.ACT:** When two allied units are ordered to swap positions on open ground, both units shall resolve opposing velocities via lateral deflection and reach their destinations.
  * **SPEC-MOV-008.PERF:** In a 200-tick soak test with 40 units ordered simultaneously to a single focal point, zero units shall remain permanently deadlocked; every unit shall either reach the destination or stabilize within its arrival packing radius.
  * **SPEC-MOV-008.FAIL:** If separation forces cannot resolve within 40 ticks, the lower-priority unit shall yield by taking a temporary lateral step.
  * **SPEC-MOV-008.VERIF:** `PKG-AUTO` (40-unit focal soak and position-swap test).
  * **SPEC-MOV-008.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-MOV-009 — Chokepoint Negotiation Without Deadlock:** Clustered mobile units traversing narrow terrain apertures (chokepoints) shall funnel through sequentially without jamming or permanent stoppage.
  * **SPEC-MOV-009.AUTH:** When 12 units are ordered through a 1-tile-wide aperture, all units shall clear the choke within a bounded tick budget derived from unit speed and count, with no unit remaining stationary for more than 40 consecutive ticks while its path remains valid.
  * **SPEC-MOV-009.FAIL:** Queue deadlocks or cyclic priority locks at choke mouths are classified as release-blocking defects.
  * **SPEC-MOV-009.VERIF:** `PKG-AUTO` (12-unit 1-tile choke throughput benchmark).
  * **SPEC-MOV-009.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-MOV-010 — Travel Facing and Presentation Decoupling:** Units shall face their active direction of travel and turn with bounded angular rates. Travel facing is strictly presentational in 1.0 and shall not alter simulation targeting, collision, or damage.
  * **SPEC-MOV-010.AUTH:** Rendered mesh facing shall align with velocity heading within 5 degrees during steady movement, sweeping at an authored rate of 720 deg/s.
  * **SPEC-MOV-010.ACC:** Under the Reduced Motion accessibility preset, rotational sweeping shall be instantaneous, eliminating visual spin while preserving instantaneous facing accuracy.
  * **SPEC-MOV-010.VERIF:** `PKG-REND` (`AEchoesEntityView::UpdateTravelFacing` capture review).
  * **SPEC-MOV-010.LANE:** Visual Presentation (`EchoesOfTheBrokenSun`).

* **SPEC-MOV-011 — Group Cohesion and Centroid Navigation:** Multi-unit selection groups issued a shared movement order shall navigate as a coherent force rather than collapsing to a single coordinate point.
  * **SPEC-MOV-011.AUTH:** Target destinations shall be distributed across an arrival area scaled to the group's collective footprint; units shall not fight over a single destination tile.
  * **SPEC-MOV-011.ACT:** In transit, group movement speeds shall clamp to the maximum speed of the slowest selected member until contact is initiated or the formation breaks.
  * **SPEC-MOV-011.FAIL:** If path obstacles break formation alignment, units shall navigate independently through the obstacle and automatically reform within 40 ticks of reaching open ground.
  * **SPEC-MOV-011.VERIF:** `PKG-AUTO` (mixed-speed group travel cohesion test).
  * **SPEC-MOV-011.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-MOV-012 — Damped Clean Arrival:** Units reaching their destination shall halt cleanly without overshooting, jittering against neighbors, or oscillating between adjacent waypoints.
  * **SPEC-MOV-012.AUTH:** Upon entering the arrival radius, a unit's position shall change by no more than 0.05 tiles over 20 consecutive ticks.
  * **SPEC-MOV-012.FAIL:** Unstable microscopic oscillation between waypoints or repulsive neighbor pushing at rest shall immediately trigger arrival velocity clamping to zero.
  * **SPEC-MOV-012.VERIF:** `SRC` (single-unit and multi-unit arrival velocity settle test).
  * **SPEC-MOV-012.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-MOV-013 — Movement Determinism & Sanitizer Invariance:** All movement, steering, and pathfinding math shall execute strictly within the deterministic core under Q22.10 fixed point, yielding byte-identical state checksums across platforms and compilers.
  * **SPEC-MOV-013.AUTH:** Identical movement command sequences shall yield 100% byte-identical state checksums across Clang on Apple Silicon, GCC on Linux x86_64, and MSVC on Windows, across Optimized, Debug, and ASan+UBSan configurations.
  * **SPEC-MOV-013.FAIL:** Any use of `float`, `double`, trigonometric standard library functions, or non-deterministic container iteration in movement logic shall fail compilation closed.
  * **SPEC-MOV-013.VERIF:** `SRC` (cross-build determinism suite and sanitizer matrix).
  * **SPEC-MOV-013.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-CTL-016 — Command Responsiveness Invariant:** Player commands shall produce instantaneous visual acknowledgement in the received frame and execute authoritatively within the lockstep input delay window.
  * **SPEC-CTL-016.AUTH:** Order acknowledgement audio/visual markers shall fire in the exact frame the click is sampled (≤16.67 ms at 60 fps). Authoritative order execution shall occur within `minimumInputDelayTicks` (3 ticks = 150 ms) in single-player mode.
  * **SPEC-CTL-016.FAIL:** If simulation tick processing stalls, the input buffer shall queue orders without dropping clicks, up to a depth of 16 commands.
  * **SPEC-CTL-016.VERIF:** `PKG-PHYS` (high-speed capture measuring click-to-ring latency).
  * **SPEC-CTL-016.LANE:** Player Experience (`EchoesPlayerController`).

* **SPEC-CTL-017 — Fluid Command Interruptibility:** In-flight movement and combat orders shall be immediately replaceable on the subsequent simulation tick without penalty stalls or artificial replanning delays.
  * **SPEC-CTL-017.AUTH:** Issuing a new move order to a unit already in motion shall repath and redirect velocity on the very next simulation tick (≤50 ms).
  * **SPEC-CTL-017.FAIL:** The unit shall not decelerate to zero before starting the new path unless the heading change exceeds 135 degrees.
  * **SPEC-CTL-017.VERIF:** `SRC` (re-path latency and velocity continuity test).
  * **SPEC-CTL-017.LANE:** Core Gameplay (`EchoesSimCore`).

* **SPEC-CTL-018 — Micro-Management Usability Preservation:** The control system shall preserve high-cadence player micro-management (stutter-stepping, focus-firing, damaged unit retraction) without input dropping, queue starvation, or camera hitching.
  * **SPEC-CTL-018.AUTH:** Rapid sequential selection and order issuance at up to 300 actions per minute (APM) shall process with 0% command drop and 100% spatial target fidelity.
  * **SPEC-CTL-018.FAIL:** Any input queue drop under 300 APM bursts shall fail acceptance.
  * **SPEC-CTL-018.VERIF:** `HUM` (experienced RTS player usability sessions under VAL-001 protocol).
  * **SPEC-CTL-018.LANE:** Player Experience & QA.

* **SPEC-CTL-019 — Simulation Tick Cost Ceiling for Steering:** Movement, steering, and separation calculations for up to 400 active units shall fit within the per-tick game thread budget.
  * **SPEC-CTL-019.AUTH:** Total movement and path resolution time for 400 mobile units shall not exceed 3.0 ms per simulation tick on the baseline M1 Pro Apple Silicon processor. Spatial queries shall use an O(N) spatial hash grid, strictly prohibiting O(N^2) pairwise distance scans.
  * **SPEC-CTL-019.FAIL:** Tick processing exceeding 4.0 ms shall flag a performance regression.
  * **SPEC-CTL-019.VERIF:** `PKG-AUTO` (400-unit steering benchmark harness).
  * **SPEC-CTL-019.LANE:** Core Gameplay & Performance (`EchoesSimCore`).


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

### 16.4 Mass AI Balance Validation Architecture (`SPEC-BAL-*`)
* **SPEC-BAL-001 — Headless Batch Simulation Harness:** A headless, simulation-only execution harness shall run complete AI-versus-AI matches without rendering, editor dependencies, or human input at a throughput enabling 1,000 matches in under 30 minutes.
  * **SPEC-BAL-001.AUTH:** The harness shall execute N matches from a seed list, emitting structured JSON records containing: match seed, map ID, faction pair, spawn slots, AI doctrines, winning faction, duration in ticks, and final state checksum.
  * **SPEC-BAL-001.FAIL:** Any match ending in an unresolved state, infinite loop, or crash shall fail the batch and emit a fatal defect log.
  * **SPEC-BAL-001.VERIF:** `PKG-AUTO` (headless batch runner `Scripts/run_ai_balance_matrix.py`).
  * **SPEC-BAL-001.LANE:** Opponent AI & QA.

* **SPEC-BAL-002 — Statistical Balance Reporting with Uncertainty:** Faction and map win rates shall be evaluated using rigorous statistical estimators, reporting sample size (N) and 95% binomial confidence intervals.
  * **SPEC-BAL-002.AUTH:** Win rates shall be reported as `Rate ± Margin of Error (N=...)`. Two win rates shall only be asserted as significantly different if their 95% confidence intervals do not overlap.
  * **SPEC-BAL-002.FAIL:** Reporting bare percentage win rates without sample size or variance intervals is strictly prohibited.
  * **SPEC-BAL-002.VERIF:** `SRC` (statistical summary report generator validation).
  * **SPEC-BAL-002.LANE:** Opponent AI & QA.

* **SPEC-BAL-003 — Faction Asymmetry Balance Band:** Standard matchup win rates across all non-mirror pairings (Meridian vs. Kharuun, Meridian vs. Choir, Kharuun vs. Choir) shall sit strictly within the 40%–60% competitive balance window.
  * **SPEC-BAL-003.AUTH:** Over a minimum of 1,000 matches per matchup cell on standard tournament maps, no faction shall exhibit a win rate below 40.0% or above 60.0% under identical AI competence.
  * **SPEC-BAL-003.FAIL:** Any matchup falling outside the 40%–60% interval shall trigger an immediate balance review and halt release qualification for that faction.
  * **SPEC-BAL-003.VERIF:** `PKG-AUTO` (1,000-match automated balance matrix evidence).
  * **SPEC-BAL-003.LANE:** Opponent AI & Faction Design.

* **SPEC-BAL-004 — Map and Spawn Symmetry Fairness:** Spawn location shall not bias match outcome by more than 5.0 percentage points.
  * **SPEC-BAL-004.AUTH:** In mirror-match controls with identical AI configurations on both sides, the win rate for Spawn Slot A vs. Spawn Slot B shall fall within 47.5%–52.5% over 1,000 matches.
  * **SPEC-BAL-004.FAIL:** Any map exhibiting a spawn win rate deviation >5.0% shall be classified as asymmetric and rejected from competitive skirmish rotation.
  * **SPEC-BAL-004.VERIF:** `PKG-AUTO` (1,000-match mirror spawn fairness run).
  * **SPEC-BAL-004.LANE:** World / Maps & Opponent AI.

* **SPEC-BAL-005 — Strategy Primacy Over Randomness:** Match outcome shall be determined primarily by strategic decisions (composition, expansion, economy, positioning) rather than random seed or starting advantages.
  * **SPEC-BAL-005.AUTH:** A validated high-tier strategy (e.g. screened ranged fire with scouting) shall defeat an intentionally flawed strategy (e.g. unescorted artillery without screen) at a rate exceeding 75% (95% CI [72%, 78%]). No single build order shall defeat all other strategies with >65% win rate (no dominant strategy).
  * **SPEC-BAL-005.FAIL:** If two distinct strategies produce win rates that cannot be separated from 50% across 500 matches, the strategic depth is classified as insufficient.
  * **SPEC-BAL-005.VERIF:** `PKG-AUTO` (doctrinal strategy divergence test).
  * **SPEC-BAL-005.LANE:** Opponent AI & Canon Design.

* **SPEC-BAL-006 — Batch Replayability and Verification:** Any match from a balance run shall be 100% reproducible from its match seed and input log.
  * **SPEC-BAL-006.AUTH:** Rerunning a specific match seed shall produce an identical tick-by-tick command log and identical terminal state checksum.
  * **SPEC-BAL-006.FAIL:** Checksum divergence upon replay indicates a determinism violation and immediately invalidates the entire balance run.
  * **SPEC-BAL-006.VERIF:** `SRC` (random seed replay reproducibility test).
  * **SPEC-BAL-006.LANE:** Core Gameplay & QA.

* **SPEC-BAL-007 — Balance Evidence Expiry and Re-Validation:** Any modification to unit statistics, economy rates, combat algorithms, or movement code shall automatically invalidate existing balance evidence.
  * **SPEC-BAL-007.AUTH:** The balance evidence registry shall record the exact Git commit SHA of the simulation rules pack. Merging a simulation rule change automatically marks all balance claims as `STALE — RE-RUN REQUIRED`.
  * **SPEC-BAL-007.FAIL:** Quoting balance win rates derived from a prior rules commit is prohibited.
  * **SPEC-BAL-007.VERIF:** `SRC` (SHA binding assertion in `AssetRegister.md` / `ProjectLedger.md`).
  * **SPEC-BAL-007.LANE:** Coordinator & QA.

* **SPEC-BAL-008 — AI Instrument Competence Baseline:** Balance validation evidence shall be measured exclusively with AI models that demonstrate baseline RTS competence, ensuring the instrument measures game balance rather than AI incompetence.
  * **SPEC-BAL-008.AUTH:** Before an AI configuration qualifies as an instrument for balance testing, it shall pass a competency battery: (1) retreats severely damaged units (>70% HP lost) when disengaged; (2) focuses fire on high-threat targets; (3) does not march into unrevealed enemy defense clusters without scouting; (4) actively saturates resource deposits up to optimal capacity.
  * **SPEC-BAL-008.FAIL:** Unscripted fixtures or headless runs with passive units shall never be cited as gameplay balance evidence.
  * **SPEC-BAL-008.VERIF:** `SRC` + `PKG-AUTO` (AI competency verification suite).
  * **SPEC-BAL-008.LANE:** Opponent AI (`EchoesAIController`).


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


## Part III — Authoritative Release Requirements (`REL-*`)

All 369 requirements declared across §6–§26 have been fully authored, calibrated against
the authoritative project directives (GameCompletionDirective.md, TechnicalArchitecture.md,
DevelopmentBible.md, SpecGapReport.md), and decomposed into testable atomic leaves.

### §6 Release Governance and Integrity (`REL-GOV-*`)

* **REL-GOV-001 — Sole Normative Requirements Authority:** `Docs/Requirements.md` shall be the sole normative authority for requirement bodies, acceptance criteria, and crosswalks. No secondary document may define, alter, or waive a requirement.
  * **REL-GOV-001.AUTH:** All active engineering and verification tasks shall cite stable identifiers in `SPEC-*`, `DEMO-*`, or `REL-*`.
  * **REL-GOV-001.FAIL:** Any requirement body defined outside `Requirements.md` is void by construction.
  * **REL-GOV-001.VERIF:** `SRC` (documentation audit script).
  * **REL-GOV-001.LANE:** Coordinator & QA.

* **REL-GOV-002 — Bidirectional Gate and Milestone Mapping:** Every release requirement shall map bidirectionally to an executable release gate in `Docs/GameCompletionDirective.md` and an evidence location in `WorkstreamControl/evidence/release/`.
  * **REL-GOV-002.AUTH:** Traceability matrix shall contain no orphaned release requirements and no unmapped delivery gates.
  * **REL-GOV-002.FAIL:** Unmapped requirements cannot advance beyond `IN PROGRESS`.
  * **REL-GOV-002.VERIF:** `SRC` (traceability crosswalk audit).
  * **REL-GOV-002.LANE:** Coordinator.

* **REL-GOV-003 — Prohibition of Silent Invention:** When required game behavior is ambiguous, contradictory, or absent, work shall stop at that decision boundary. Agents and contributors shall not invent mechanics, lore, or scope silently.
  * **REL-GOV-003.AUTH:** Ambiguities shall be escalated via a `TBR-*` decision packet stating alternatives, costs, and gameplay consequences.
  * **REL-GOV-003.FAIL:** Unapproved mechanical inventions found in pull requests shall be rejected.
  * **REL-GOV-003.VERIF:** `SRC` (code review and PR gating).
  * **REL-GOV-003.LANE:** Coordinator.

* **REL-GOV-004 — Traceable Inline Change Cascades:** Approved modifications to design or numerical baselines shall be edited directly into `Docs/Requirements.md` in place and cascaded to source data, UI tooltips, AI rules, and unit tests in the same change set.
  * **REL-GOV-004.AUTH:** Git commit modifying a baseline value shall touch the requirement master, `Content/Data/Source/`, and associated tests atomically.
  * **REL-GOV-004.FAIL:** Desynchronization between requirement text and runtime JSON data fails CI.
  * **REL-GOV-004.VERIF:** `SRC` (`test_content_schema.py` and catalog compilation).
  * **REL-GOV-004.LANE:** Core Gameplay & Content.

* **REL-GOV-005 — Evidence-Bounded State Vocabulary:** Requirement states shall strictly obey the authorized vocabulary: `OPEN`, `IN PROGRESS`, `IMPLEMENTED — NOT YET VERIFIED`, `AGENT VERIFIED`, `EVIDENCE READY`, `AWAITING HUMAN ACCEPTANCE`, and `BLOCKED`.
  * **REL-GOV-005.AUTH:** Terms such as `COMPLETE`, `DONE`, `VERIFIED`, or `PASS` are prohibited in agent status reports unless backed by explicit human acceptance.
  * **REL-GOV-005.FAIL:** Unauthorized promotion of unverified work fails gate review.
  * **REL-GOV-005.VERIF:** `SRC` (`RequirementsState.md` linter).
  * **REL-GOV-005.LANE:** Independent QA.

* **REL-GOV-006 — Permanent Immutable Identifier Preservation:** Requirement identifiers across `SPEC-*`, `DEMO-*`, and `REL-*` shall remain permanently immutable. Renumbering, reusing, or deleting existing IDs is strictly prohibited.
  * **REL-GOV-006.AUTH:** Retired requirements shall be marked `SUPERSEDED BY <id>` or `WITHDRAWN — <authority, date>` while retaining their original identifier.
  * **REL-GOV-006.FAIL:** Re-keyed or missing IDs in CI diffs fail build validation.
  * **REL-GOV-006.VERIF:** `SRC` (ID immutability checker).
  * **REL-GOV-006.LANE:** Coordinator.

* **REL-GOV-007 — Verification Class Exclusivity:** Every requirement leaf shall be assigned exactly one verification class (`SRC`, `PKG-AUTO`, `PKG-REND`, `PKG-PHYS`, `EDT`, `HUM`, `OWNER`).
  * **REL-GOV-007.AUTH:** A lower verification class (e.g. `EDT` or `SRC`) shall never substitute for a higher class (e.g. `PKG-PHYS` or `HUM`).
  * **REL-GOV-007.FAIL:** Marking a human-usability requirement complete based solely on unit tests is a fatal defect.
  * **REL-GOV-007.VERIF:** `SRC` (verification matrix audit).
  * **REL-GOV-007.LANE:** Independent QA.

* **REL-GOV-008 — Human Acceptance Authority Reservation:** Final acceptance verdicts (`HUMAN ACCEPTED`, `HUMAN REJECTED — CHANGES REQUIRED`) shall be assigned solely by Angelis Pseftis.
  * **REL-GOV-008.AUTH:** Agents may present evidence packages but may never close a milestone batch independently.
  * **REL-GOV-008.FAIL:** Self-certification by an AI agent is null and void.
  * **REL-GOV-008.VERIF:** `OWNER` (explicit owner signature).
  * **REL-GOV-008.LANE:** Coordinator & Owner.

* **REL-GOV-009 — Fail-Closed Architectural Boundary:** All data parsers, catalog compilers, network message readers, and save container deserializers shall fail closed upon encountering unknown keys, checksum mismatches, or bounds violations.
  * **REL-GOV-009.AUTH:** Corrupt or unrecognized data shall throw an explicit fatal error code and halt rather than continuing with degraded or default values.
  * **REL-GOV-009.FAIL:** Silent fallback to default data fails security audit.
  * **REL-GOV-009.VERIF:** `SRC` (negative schema and corrupt container tests).
  * **REL-GOV-009.LANE:** Core Gameplay & Build.

* **REL-GOV-010 — Procedural-First Asset Provenance Registration:** All textures, meshes, audio cues, and music shall be generated deterministically via project code and registered in `Docs/Archive/AssetRegister.md` prior to runtime consumption.
  * **REL-GOV-010.AUTH:** Any external asset (e.g. local neural TTS voice model, licensed typeface) shall be documented as a recorded exception with license, hash, and justification.
  * **REL-GOV-010.FAIL:** Unregistered binary assets in `/Content/` fail the package gate.
  * **REL-GOV-010.VERIF:** `SRC` (`verify_asset_provenance.py`).
  * **REL-GOV-010.LANE:** Visual, Audio & QA.

* **REL-GOV-011 — Independent Verification Lane Separation:** Verification of release gates shall be performed in an isolated clean checkout by an agent or session independent of the authoring lane.
  * **REL-GOV-011.AUTH:** The implementing session shall not validate its own work for release closure.
  * **REL-GOV-011.FAIL:** Verification logs produced from dirty or development worktrees are invalid.
  * **REL-GOV-011.VERIF:** `SRC` (git worktree isolation and clean state check).
  * **REL-GOV-011.LANE:** Independent QA.

* **REL-GOV-012 — Continuous Automated Regression Locks:** Merges to `main` shall require passing all native simulation suites (3 sanitizer configurations), Python content validators, and Unreal headless automation tests.
  * **REL-GOV-012.AUTH:** CI regression suites shall execute on every branch merge, asserting zero regressions.
  * **REL-GOV-012.FAIL:** Any failing test immediately halts integration.
  * **REL-GOV-012.VERIF:** `SRC` (`run_unreal_tests.sh` and `test_sim.sh`).
  * **REL-GOV-012.LANE:** Build & Automation.

* **REL-GOV-013 — Defect Severity Ladder and Release Prohibitions:** All defects shall be categorized under the project severity ladder: S0 (Catastrophic/Crash), S1 (Functional Blocker), S2 (Major Usability), S3 (Minor Glitch), S4 (Cosmetic/Trivial).
  * **REL-GOV-013.AUTH:** Zero S0 and zero S1 defects shall exist in a release candidate. Zero un-waived S2 defects on the critical path.
  * **REL-GOV-013.FAIL:** Presence of an open S0 or S1 defect blocks release packaging.
  * **REL-GOV-013.VERIF:** `SRC` (`ProjectLedger.md` defect register).
  * **REL-GOV-013.LANE:** Independent QA.

* **REL-GOV-014 — Single-Candidate Frozen Evidence Binding:** All evidence supporting release qualification shall be captured from the exact same frozen, signed, and notarized build candidate.
  * **REL-GOV-014.AUTH:** Packaging manifest digest, binary SHA-256, and evidence logs must match identically across all gate receipts.
  * **REL-GOV-014.FAIL:** Splicing evidence from different commits or package builds invalidates the release candidate.
  * **REL-GOV-014.VERIF:** `PKG-AUTO` (package seal manifest verification).
  * **REL-GOV-014.LANE:** Build Distribution & QA.

* **REL-GOV-015 — Final Definition of Done Sign-Off Checklist:** Release completion requires satisfying the 10-point Definition of Done in `GameCompletionDirective.md` §4, signed off by Angelis Pseftis.
  * **REL-GOV-015.AUTH:** Clean-machine installation, full campaign completion, 3-faction skirmish victory, zero placeholder art/audio, BS.1770-4 loudness compliance, and notarization verified.
  * **REL-GOV-015.FAIL:** Missing any single item blocks public release.
  * **REL-GOV-015.VERIF:** `OWNER` (final release sign-off review).
  * **REL-GOV-015.LANE:** Coordinator & Owner.

---

### §7 First-Run, Front Door, and Onboarding (`REL-FTU-*`)

* **REL-FTU-001 — Clean-Machine Cold Launch Reliability:** The application shall cold-launch from an unmodified macOS disk image (.dmg) on an Apple Silicon Mac without external developer tools, Xcode, or terminal commands.
  * **REL-FTU-001.AUTH:** Launch to interactive title screen shall complete within 12.0 seconds on baseline M1 hardware with disk cache cold.
  * **REL-FTU-001.FAIL:** App crash, Gatekeeper security rejection, or missing dynamic library alert fails acceptance.
  * **REL-FTU-001.VERIF:** `PKG-PHYS` (clean machine launch test).
  * **REL-FTU-001.LANE:** Build Distribution.

* **REL-FTU-002 — Atmospheric World-Coherent Title Treatment:** The title screen shall establish the narrative setting of Soryn, displaying the shattered sun, vitrified landscape, and ambient world audio within 2 seconds of window initialization.
  * **REL-FTU-002.AUTH:** Title visual presentation shall render in-engine using production shaders (`M_EchoesWorldSurface`), playing the title theme (`AUE_Title_Theme`) at -16 LUFS.
  * **REL-FTU-002.ACC:** Screen supports the Reduced Motion preset, holding camera drifting steady while preserving atmospheric lighting.
  * **REL-FTU-002.VERIF:** `PKG-REND` (captured title video review).
  * **REL-FTU-002.LANE:** Visual Presentation & Audio.

* **REL-FTU-003 — Primary Navigation Hub Ergonomics:** The main menu shall provide unambiguous navigation to: Campaign, Skirmish, Tutorial, Options, Credits, and Quit, with full mouse hover states and keyboard navigation (`Up`/`Down`/`Enter`/`Escape`).
  * **REL-FTU-003.AUTH:** Every button provides visual highlight within 1 frame (≤16.67 ms) and plays the distinct interface audio cue `UI_Hover` / `UI_Click`.
  * **REL-FTU-003.FAIL:** Unresponsive buttons or silent activation fail acceptance.
  * **REL-FTU-003.VERIF:** `PKG-PHYS` (menu keyboard and mouse navigation sweep).
  * **REL-FTU-003.LANE:** Player Experience (`EchoesHUD`).

* **REL-FTU-004 — In-Engine Opening Story Sequence:** First launch or selecting "Prologue" shall play an authored in-engine cinematic sequence introducing the Crownfall catastrophe, the loss of the sun, and the emergence of Dawnshards.
  * **REL-FTU-004.AUTH:** Sequence runs via Sequencer (`SEQ_Opening_Cinematic`), duration ≤90 seconds, featuring voiced narration synchronized with subtitles at 48 kHz.
  * **REL-FTU-004.ACC:** Fully skippable via pressing `Escape` or holding `Space` for 1.0 second, with an on-screen skip prompt.
  * **REL-FTU-004.VERIF:** `PKG-REND` (cinematic playback and subtitle sync test).
  * **REL-FTU-004.LANE:** Cinematics & Narrative.

* **REL-FTU-005 — First-Run Progressive Onboarding Gate:** A first-time player shall be guided directly into the progressive playable tutorial rather than dropped into an unexplained skirmish map.
  * **REL-FTU-005.AUTH:** If no persistent player profile exists on launch, "Start Tutorial" shall be the primary default action; opting out requires confirmation.
  * **REL-FTU-005.FAIL:** Launching directly into an active combat map without prior training prompt is prohibited.
  * **REL-FTU-005.VERIF:** `PKG-PHYS` (first-run profile flow test).
  * **REL-FTU-005.LANE:** Player Experience & Campaign.

* **REL-FTU-006 — Tutorial Lesson 1: Camera Navigation & Tactical Pan:** The tutorial shall teach camera translation (WASD, edge pan, middle drag) and zoom, requiring the player to center the viewport on three designated waypoints.
  * **REL-FTU-006.AUTH:** The lesson advances only when the camera centroid dwells within 200 cm of each objective marker for 30 consecutive ticks (1.5s).
  * **REL-FTU-006.FAIL:** Stalling camera controls or failed objective trigger halts progression.
  * **REL-FTU-006.VERIF:** `PKG-AUTO` + `PKG-PHYS` (`EchoesTutorialCurriculumTest`).
  * **REL-FTU-006.LANE:** Campaign & Player Experience.

* **REL-FTU-007 — Tutorial Lesson 2: Selection & Precision Movement:** The tutorial shall teach single-unit selection, multi-unit box drag-selection, and right-click movement along the Ash Cut corridor.
  * **REL-FTU-007.AUTH:** The system validates selection of 3 Meridian Lancers and requires movement to an extraction beacon, verifying all 3 units arrive within 400 cm.
  * **REL-FTU-007.FAIL:** Missed click registering as terrain move without selection feedback fails the lesson.
  * **REL-FTU-007.VERIF:** `PKG-PHYS` (tutorial selection path test).
  * **REL-FTU-007.LANE:** Player Experience.

* **REL-FTU-008 — Tutorial Lesson 3: Worker Economy & Matter Gathering:** The tutorial shall teach worker selection (`Surveyor`), targeting Matter crystal nodes, the automated gather-deliver loop, and drop-off delivery.
  * **REL-FTU-008.AUTH:** The player must accumulate 200 Matter; the HUD highlights the resource counter and displays delivery rate telemetry.
  * **REL-FTU-008.FAIL:** Worker idling with full cargo without auto-returning fails the test.
  * **REL-FTU-008.VERIF:** `PKG-AUTO` (worker economic throughput lesson check).
  * **REL-FTU-008.LANE:** Campaign & Core Gameplay.

* **REL-FTU-009 — Tutorial Lesson 4: Base Construction & Power Grid:** The tutorial shall teach constructing an Array Foundry and connecting an Aegis Post via a Power Link grid node.
  * **REL-FTU-009.AUTH:** Visual placement footprint validates green/red terrain passability; power grid connection line renders dynamically during placement preview.
  * **REL-FTU-009.FAIL:** Allowing placement on impassable scar tiles or unpowered defense posts fails verification.
  * **REL-FTU-009.VERIF:** `PKG-PHYS` (building placement and grid connection check).
  * **REL-FTU-009.LANE:** Player Experience & Visual.

* **REL-FTU-010 — Tutorial Lesson 5: Unit Production & Attack-Move:** The tutorial shall teach queueing units from the Array Foundry and issuing an Attack-Move command against a hostile scout patrol.
  * **REL-FTU-010.AUTH:** Production progress bar updates in HUD; newly produced units rally to flag; Attack-Move engages enemies within visual range without halting.
  * **REL-FTU-010.FAIL:** Units walking past attacking enemies without returning fire fails the lesson.
  * **REL-FTU-010.VERIF:** `PKG-AUTO` (production and combat engagement test).
  * **REL-FTU-010.LANE:** Core Gameplay.

* **REL-FTU-011 — Tutorial Lesson 6: Future Well Decisions:** The tutorial shall introduce the Future Well, explaining the irrevocable tradeoff between Harvest (+500 Dawn, permanent scar), Preserve (steady income), and Reshape (passage manifestation).
  * **REL-FTU-011.AUTH:** The player must capture the Well (420 cm, 300 ticks) and choose one protocol, verifying that HUD dialogue and audio explain the strategic consequence.
  * **REL-FTU-011.FAIL:** Instant untelegraphed capture or missing confirmation dialog fails acceptance.
  * **REL-FTU-011.VERIF:** `PKG-PHYS` (Well decision interaction test).
  * **REL-FTU-011.LANE:** Campaign & Canon Design.

* **REL-FTU-012 — Demonstration of Fundamental Mastery:** Prior to unlocking the campaign and standard skirmish, the player must pass a mini-skirmish scenario against a passive AI, proving competency in economy, army building, and Corefall victory.
  * **REL-FTU-012.AUTH:** Defeating the training core unlocks the Campaign Map and Skirmish lobby; completion is saved in the player profile.
  * **REL-FTU-012.FAIL:** Profile corruption or skipped mastery gate fails acceptance.
  * **REL-FTU-012.VERIF:** `PKG-AUTO` (`EchoesTutorialCurriculumTest`).
  * **REL-FTU-012.LANE:** Campaign & Player Experience.

---

### §8 Core Simulation, Time, and Player Authority (`REL-SIM-*`)

* **REL-SIM-001 — Deterministic Fixed-Accumulator Simulation Loop:** The simulation engine (`EchoesSimCore`) shall execute at a fixed 20 Hz tick rate (50.0 ms per step) driven by an accumulator, completely decoupled from variable rendering frame rates.
  * **REL-SIM-001.AUTH:** Simulation step count shall match elapsed game time exactly ($Ticks = TimeSeconds 	imes 20$). Rendering at 30 fps, 60 fps, or 144 fps shall not alter simulation tick frequency.
  * **REL-SIM-001.FAIL:** Wall-clock time leaks, frame-rate dependent physics, or drift >1 tick per 10,000 ticks fails acceptance.
  * **REL-SIM-001.VERIF:** `SRC` (native simulation accumulator test).
  * **REL-SIM-001.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-002 — Integer and Fixed-Point Arithmetic Authority:** All simulation mathematical operations shall use signed Q22.10 fixed-point or 64-bit integer arithmetic. Standard floating-point (`float`, `double`) operations are strictly prohibited in the sim core.
  * **REL-SIM-002.AUTH:** Trigonometric and square-root calculations shall execute via deterministic lookup tables and integer algorithms (`IntegerSqrt64`).
  * **REL-SIM-002.FAIL:** Any IEEE-754 floating-point instruction inside `EchoesSimCore` fails the determinism gate.
  * **REL-SIM-002.VERIF:** `SRC` (binary symbol audit and static AST scan).
  * **REL-SIM-002.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-003 — Seeded Pseudo-Random Number Generation:** All stochastic gameplay events (e.g. initial spawn dispersal, AI personality jitter) shall derive exclusively from a seeded `DeterministicRng`.
  * **REL-SIM-003.AUTH:** Given identical initial seeds, random sequence output shall be 100% byte-identical across all executions.
  * **REL-SIM-003.FAIL:** Invocation of `rand()`, `std::random_device`, or unseeded time-based RNG in the sim core fails build verification.
  * **REL-SIM-003.VERIF:** `SRC` (RNG serialization and reproducible distribution test).
  * **REL-SIM-003.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-004 — Strict View Layer Isolation from Simulation Authority:** Presentation layers (Unreal Actors, Niagara emitters, Slate widgets, audio components) shall observe authoritative simulation state via read-only interfaces and shall never write back to simulation state.
  * **REL-SIM-004.AUTH:** No Unreal presentation component shall modify entity coordinates, health, orders, or economy balances.
  * **REL-SIM-004.FAIL:** View actor mutation of simulation state causes immediate assertion failure.
  * **REL-SIM-004.VERIF:** `SRC` (const-correctness API audit).
  * **REL-SIM-004.LANE:** Unreal Integration & Core Gameplay.

* **REL-SIM-005 — Filtered PlayerView Information Masking:** The simulation shall project game state through a strictly masked `PlayerView` ensuring that neither AI controllers nor human clients receive un-scouted enemy data.
  * **REL-SIM-005.AUTH:** Un-scouted enemy units shall have positions, orders, health, and cargo zeroed or omitted from the client data stream.
  * **REL-SIM-005.FAIL:** Disclosure of enemy coordinates or HP through memory leaks or packet interception fails security review.
  * **REL-SIM-005.VERIF:** `SRC` (PlayerView redaction unit tests).
  * **REL-SIM-005.LANE:** Core Gameplay & Security.

* **REL-SIM-006 — Fog of War Spatial Authority:** Fog and shroud visibility calculations shall execute authoritatively in the simulation core on a 64x64 discrete grid.
  * **REL-SIM-006.AUTH:** A cell is Visible if within line-of-sight of an owned entity; Explored if previously visited; Unexplored otherwise. Shroud occlusion hides structures on vision loss (resolving C6).
  * **REL-SIM-006.FAIL:** Entities acquiring targets or attacking into unexplored shroud without active spotter fails validation.
  * **REL-SIM-006.VERIF:** `SRC` (fog grid occlusion and ray-cast tests).
  * **REL-SIM-006.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-007 — Dual Time Representation Contract:** All player-visible time metrics shall be formatted and displayed in seconds (`MM:SS`), while simulation internal logs and network packets shall carry deterministic tick integers.
  * **REL-SIM-007.AUTH:** The HUD match clock shall display elapsed seconds calculated as $Ticks / 20$. Tooltips shall state build times in seconds with integer tick values in debug inspection (resolving C42).
  * **REL-SIM-007.FAIL:** Exposing raw tick numbers as the primary time metric to users fails usability.
  * **REL-SIM-007.VERIF:** `PKG-REND` (HUD time format inspection).
  * **REL-SIM-007.LANE:** Player Experience (`EchoesHUD`).

* **REL-SIM-008 — Authoritative Command Validation Pipeline:** Every command issued by a player or AI shall pass through the simulation validation pipeline, returning a structured receipt: `Applied`, `InvalidTarget`, `InsufficientResources`, `RouteBlocked`, or `PrerequisiteMissing`.
  * **REL-SIM-008.AUTH:** Invalid commands shall be rejected immediately on the authoritative tick and return a stable error code to the HUD (resolving C4).
  * **REL-SIM-008.FAIL:** Silent command dropping without notification or feedback fails acceptance.
  * **REL-SIM-008.VERIF:** `SRC` (command validation matrix tests).
  * **REL-SIM-008.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-009 — Entity Unique Identifier Lifecycle & Ghost Prevention:** Every active entity shall be assigned a monotonic 32-bit unique ID (`EntityId`).
  * **REL-SIM-009.AUTH:** Entity destruction shall cleanly purge all references from spatial grids, targeting queues, and selection arrays within the same simulation tick.
  * **REL-SIM-009.FAIL:** Dangling pointers, ghost target references, or reused IDs within a match session fail determinism tests.
  * **REL-SIM-009.VERIF:** `SRC` (entity lifecycle stress tests).
  * **REL-SIM-009.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-010 — Map Spatial Boundary Invariant:** Mobile entities and projectiles shall remain strictly bounded within the map coordinate extents (e.g. $[0, 6400	ext{ cm}] 	imes [0, 6400	ext{ cm}]$).
  * **REL-SIM-010.AUTH:** Any velocity vector attempting to push a unit outside the playable bounding box shall clamp to the boundary perimeter.
  * **REL-SIM-010.FAIL:** Entities falling out of world or teleporting across coordinate overflow boundaries fails acceptance.
  * **REL-SIM-010.VERIF:** `SRC` (boundary collision stress suite).
  * **REL-SIM-010.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-011 — Transactional State Checksum Verification:** The simulation shall compute an FNV-1a 64-bit state checksum over all living entities, resources, and terrain states on every simulation tick.
  * **REL-SIM-011.AUTH:** Identical command inputs must produce 100% matching checksums across save/load restore and replay playback.
  * **REL-SIM-011.FAIL:** Desynchronization of checksums across identical runs indicates a determinism breach.
  * **REL-SIM-011.VERIF:** `SRC` (10,000-tick cross-run checksum comparison).
  * **REL-SIM-011.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-SIM-012 — Core Tick Processing Time Ceiling:** Core simulation tick execution (excluding rendering and presentation) for 400 active units shall execute within $\le 4.0	ext{ ms}$ on the baseline Apple Silicon processor.
  * **REL-SIM-012.AUTH:** Profiling traces shall measure simulation step time; p95 execution time must maintain $\ge 60\%$ headroom within the 50 ms tick allocation.
  * **REL-SIM-012.FAIL:** Tick processing exceeding 8.0 ms triggers a high-severity performance alert.
  * **REL-SIM-012.VERIF:** `PKG-AUTO` (400-unit sim core profile run).
  * **REL-SIM-012.LANE:** Core Gameplay & Performance.

### §9 Economy and Logistics (`REL-ECO-*`)

* **REL-ECO-001 — Three Resource Pillars:** The economic engine shall be governed strictly by three resources: `Matter` (primary construction/production), `Dawn` (advanced tech, abilities, and Future Well commitment), and `Logistics` (population throughput ceiling).
  * **REL-ECO-001.AUTH:** No secondary unmodeled currencies, timers, or hidden reserves shall exist.
  * **REL-ECO-001.FAIL:** Emergence of an unindexed currency fails data validation.
  * **REL-ECO-001.VERIF:** `SRC` (resource schema tests).
  * **REL-ECO-001.LANE:** Core Gameplay & Economy.

* **REL-ECO-002 — Starting Matter Resource Presets:** Standard skirmish and campaign operations shall initialize starting Matter reserves strictly from the authored difficulty presets: Low (250), Standard (400), High (700) (resolving C13).
  * **REL-ECO-002.AUTH:** Skirmish setup allocating Matter outside 250/400/700 without documented custom handicap is prohibited.
  * **REL-ECO-002.FAIL:** Starting match with 320/500/800 Matter violates the binding standard.
  * **REL-ECO-002.VERIF:** `SRC` + `PKG-AUTO` (`EchoesSkirmishSetupTest`).
  * **REL-ECO-002.LANE:** Core Gameplay & Skirmish.

* **REL-ECO-003 — Calibrated Matter Harvesting Cadence:** Worker units (`Surveyor`, `Tender`, `Threadkeeper`) assigned to a Matter deposit shall execute a 20-tick harvest extraction phase (1.0 second at 20 Hz) to fill a 10-unit cargo capacity (resolving C14).
  * **REL-ECO-003.AUTH:** Work extraction rate shall add 0.5 Matter per tick over 20 ticks, filling 10 Matter exactly at tick 20.
  * **REL-ECO-003.FAIL:** Instant 1-tick harvesting (20x speed bug) is strictly prohibited.
  * **REL-ECO-003.VERIF:** `SRC` (native simulation worker harvesting tick test).
  * **REL-ECO-003.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-004 — Deposit Saturation & Diminishing Returns:** Matter deposits shall enforce spatial saturation dynamics. Optimal extraction throughput shall cap at 2 workers per deposit node; additional workers shall experience queuing delays.
  * **REL-ECO-004.AUTH:** A third worker targeting a node shall wait in arrival queue until the active worker vacates the harvesting slot.
  * **REL-ECO-004.FAIL:** Stacking unlimited workers inside a single deposit mesh is prohibited.
  * **REL-ECO-004.VERIF:** `SRC` (deposit saturation and queue test).
  * **REL-ECO-004.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-005 — Deposit Depletion Lifecycle:** Matter deposits shall carry a finite resource volume (Standard node = 1,500 Matter). When volume reaches zero, the deposit shall deplete, collapse its mesh footprint, and alert assigned workers.
  * **REL-ECO-005.AUTH:** Upon depletion, assigned workers shall automatically retarget the nearest known unexhausted deposit within 2,000 cm or transition to `Idle Worker` registry.
  * **REL-ECO-005.FAIL:** Workers continuing to harvest an exhausted 0-Matter node fails acceptance.
  * **REL-ECO-005.VERIF:** `SRC` (deposit exhaustion state machine test).
  * **REL-ECO-005.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-006 — Continuous Automated Worker Harvesting Loop:** A valid Gather order shall transition the worker into an autonomous continuous state machine: *Move to Node → Harvest 20 Ticks → Move to Drop-off → Deposit Cargo → Return to Node* without requiring repeated manual clicks (resolving C15).
  * **REL-ECO-006.AUTH:** Workers shall sustain continuous harvesting until the node depletes, the route is severed, or a new order is issued.
  * **REL-ECO-006.FAIL:** Worker idling after delivering cargo without returning to work is a critical defect.
  * **REL-ECO-006.VERIF:** `PKG-AUTO` (10-minute continuous worker throughput test).
  * **REL-ECO-006.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-007 — Drop-Off Target Selection & Routing:** Workers returning with cargo shall dynamically select the closest operational, reachable friendly drop-off structure (`Command Core`, `Power Link`, `Waystone`, `Interval Loom`).
  * **REL-ECO-007.AUTH:** If the primary drop-off is destroyed en route, the worker shall recalculate pathing to the nearest alternate drop-off within 1 simulation tick.
  * **REL-ECO-007.FAIL:** Worker deadlocking at the destroyed site fails acceptance.
  * **REL-ECO-007.VERIF:** `SRC` (drop-off destruction rerouting test).
  * **REL-ECO-007.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-008 — Cargo Loss and Drop-Off Severance Handling:** If a worker carrying cargo is destroyed, its cargo shall be permanently lost. If all drop-offs on the map are eliminated, workers shall halt, retain cargo, and emit an alert: `[NO OPERATIONAL DROP-OFF]`.
  * **REL-ECO-008.AUTH:** The HUD worker monitor shall highlight affected workers with amber distress icons.
  * **REL-ECO-008.FAIL:** Silent discarding of cargo without alert fails usability.
  * **REL-ECO-008.VERIF:** `PKG-REND` (drop-off severance alert test).
  * **REL-ECO-008.LANE:** Player Experience (`EchoesHUD`).

* **REL-ECO-009 — Dawn Inflow and Reserve Invariant:** Dawn shall be acquired strictly through Future Well interactions (Harvest +500 Dawn; Preserve +15 Dawn per 300 ticks) and mission-authored starting reserves.
  * **REL-ECO-009.AUTH:** Dawn cannot be gathered from common terrain nodes or generated passively without a controlled Well.
  * **REL-ECO-009.FAIL:** Spontaneous generation of Dawn without Well authority fails sim audit.
  * **REL-ECO-009.VERIF:** `SRC` (Dawn balance transaction integrity tests).
  * **REL-ECO-009.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-010 — Fail-Closed Dawn Reservation Invariant:** Any transaction requiring Dawn (tech research, abilities, Reshape) shall validate that the player's available liquid Dawn meets or exceeds the cost before commitment.
  * **REL-ECO-010.AUTH:** If Dawn balance is insufficient, the action shall fail closed, reject the command, and display `[INSUFFICIENT DAWN]`. Net balance shall never drop below zero.
  * **REL-ECO-010.FAIL:** Negative Dawn balances or debt spending are strictly prohibited.
  * **REL-ECO-010.VERIF:** `SRC` (Dawn overdraft rejection test).
  * **REL-ECO-010.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-011 — Logistics Capacity Allocation Mechanics:** Committed units shall consume Logistics capacity (Workers: 1; Combat Line: 2; Heavies: 3). Base capacity is granted by Command Cores (+12) and supply structures (+5 or +6), capped at 200 total.
  * **REL-ECO-011.AUTH:** Active population consumption shall be tracked authoritatively; queueing a unit shall reserve its Logistics capacity immediately upon production start.
  * **REL-ECO-011.FAIL:** Exceeding maximum capacity of 200 without deliberate upgrade fails validation.
  * **REL-ECO-011.VERIF:** `SRC` (Logistics accounting and reservation test).
  * **REL-ECO-011.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-012 — Logistics Cap Enforcement & Supply Deficit:** When committed population equals total Logistics capacity, production queues attempting to train units shall halt with the status `LOGISTICS FULL`.
  * **REL-ECO-012.AUTH:** Destroying a supply structure while at max capacity places the player in supply deficit; existing units remain active, but all new production is blocked until supply is restored.
  * **REL-ECO-012.FAIL:** Producing units while in supply deficit fails simulation rules.
  * **REL-ECO-012.VERIF:** `SRC` (supply deficit production lock test).
  * **REL-ECO-012.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-ECO-013 — Temporary Logistics Burst Dynamics:** Temporary Logistics granted by faction abilities (e.g. Meridian Relay Skiff +4 supply) shall enforce strict duration timers (400 ticks) and cooldowns (800 ticks).
  * **REL-ECO-013.AUTH:** When the 400-tick duration expires, temporary capacity collapses; if the player exceeds permanent capacity, production locks until normal capacity is restored.
  * **REL-ECO-013.FAIL:** Permanent retention of temporary supply bursts fails balance.
  * **REL-ECO-013.VERIF:** `SRC` (Relay Skiff temporary supply duration test).
  * **REL-ECO-013.LANE:** Core Gameplay & Faction Design.

* **REL-ECO-014 — Asymmetric Faction Economy Rules:** Economic throughput shall reflect faction identities: Meridian requires chained power nodes; Kharuun requires rooted Waystones; Hollow Choir requires recurring structural Dawn coherence upkeep (5 Dawn per 600 ticks).
  * **REL-ECO-014.AUTH:** Unpowered Meridian nodes, unrooted Kharuun Waystones, and insolvent Choir structures shall cease resource drop-off functionality immediately.
  * **REL-ECO-014.FAIL:** Uniform faction economy models ignoring infrastructure state fail design criteria.
  * **REL-ECO-014.VERIF:** `PKG-AUTO` (three-faction economic asymmetry verification suite).
  * **REL-ECO-014.LANE:** Factions & Core Gameplay.

---

### §10 Construction, Production, and Research (`REL-BLD-*`)

* **REL-BLD-001 — Footprint Validation & Passability Enforcement:** Structural construction shall validate that every tile of the building's footprint (2x2, 4x4, or 5x5) is completely passable, unscarred, and unoccupied.
  * **REL-BLD-001.AUTH:** Placement on impassable cliffs, occupied tiles, water/void, or active unit footprints shall reject with `[FOOTPRINT BLOCKED]`.
  * **REL-BLD-001.FAIL:** Building structures overlapping cliffs or existing units fails geometry rules.
  * **REL-BLD-001.VERIF:** `SRC` (footprint collision validation tests).
  * **REL-BLD-001.LANE:** Core Gameplay & World.

* **REL-BLD-002 — Dynamic Placement Visual Blueprint Preview:** While holding a placement command, the renderer shall project a ground-clamped hologram preview showing the building silhouette, footprint grid (cyan for valid, red for blocked), and power/logistics connection vectors.
  * **REL-BLD-002.AUTH:** The preview updates dynamically with cursor position within 1 frame (≤16.67 ms).
  * **REL-BLD-002.FAIL:** Ghosting, clipping through terrain, or laggy placement preview fails acceptance.
  * **REL-BLD-002.VERIF:** `PKG-REND` (building placement hologram visual review).
  * **REL-BLD-002.LANE:** Visual Presentation (`EchoesEntityView`).

* **REL-BLD-003 — Calibrated Construction Duration Scaling:** Building construction times shall scale authoritatively based on authored work ticks (Array Foundry: 160 ticks = 8.0s; Aegis Post: 120 ticks = 6.0s; Anchor: 400 ticks = 20.0s), strictly eliminating the 10x compressed acceleration defect (resolving C16).
  * **REL-BLD-003.AUTH:** Total construction duration for an unassisted single builder shall match authored catalog ticks $\pm 0$ ticks.
  * **REL-BLD-003.FAIL:** Anchor completing in 2.0 seconds violates the normative standard.
  * **REL-BLD-003.VERIF:** `SRC` (native simulation building construction duration test).
  * **REL-BLD-003.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-004 — Multi-Builder Assist Diminishing Returns:** Assigning multiple workers to assist construction of a structure shall follow the diminishing return formula: 1st builder = 100% speed, 2nd = +60%, 3rd = +40%, 4th+ = +0% (resolving C17).
  * **REL-BLD-004.AUTH:** Adding a 4th builder shall not accelerate construction further; the maximum build speed multiplier shall cap at 2.0x base speed.
  * **REL-BLD-004.FAIL:** Linear 100% stacking per additional builder is strictly prohibited.
  * **REL-BLD-004.VERIF:** `SRC` (builder assist rate falloff test).
  * **REL-BLD-004.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-005 — Incomplete Structure Vulnerability:** Structures under construction shall possess health proportional to their completion percentage ($HP = MaxHP 	imes ProgressPct$, minimum 10% HP) and take standard combat damage.
  * **REL-BLD-005.AUTH:** If an incomplete structure's health is reduced to 0, it collapses, destroys the construction site, and kills or frees assisting workers.
  * **REL-BLD-005.FAIL:** Invulnerable construction sites fail combat balance.
  * **REL-BLD-005.VERIF:** `SRC` (construction site damage and destruction test).
  * **REL-BLD-005.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-006 — Construction Cancellation & Refund Policy:** Cancelling a construction site prior to completion shall reclaim the footprint and refund exactly 75% of invested Matter, with 0% refund for invested Dawn.
  * **REL-BLD-006.AUTH:** The remaining 25% Matter cost represents sunk site prep expense; cancellation immediately restores footprint passability.
  * **REL-BLD-006.FAIL:** 100% full refund or negative balance exploits fail economic audit.
  * **REL-BLD-006.VERIF:** `SRC` (construction cancellation refund test).
  * **REL-BLD-006.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-007 — Command Core Singularity Invariant:** A player shall possess exactly one Command Core (`Anchor`, `Memory Hearth`, `Concordance`). Constructing additional duplicate Command Cores is strictly prohibited (resolving C2).
  * **REL-BLD-007.AUTH:** The worker construction menu shall omit or permanently disable Command Core options once the initial Core is active.
  * **REL-BLD-007.FAIL:** Planting free or duplicate Command Cores to prevent elimination is a critical defect.
  * **REL-BLD-007.VERIF:** `SRC` (Command Core uniqueness enforcement test).
  * **REL-BLD-007.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-008 — Production Queue Depth & Management:** Production structures shall support an active production slot plus up to 4 queued units (maximum depth 5). Queued items shall display progress bars, unit icons, and cancellation buttons.
  * **REL-BLD-008.AUTH:** Attempting to queue a 6th unit shall reject with `[QUEUE FULL]`.
  * **REL-BLD-008.FAIL:** Unbounded queueing causing memory overflow fails stability.
  * **REL-BLD-008.VERIF:** `SRC` + `PKG-REND` (production queue capacity and UI inspection).
  * **REL-BLD-008.LANE:** Core Gameplay & Player Experience.

* **REL-BLD-009 — Unit Emergence, Rallying & Unblocking:** Completed units shall emerge from the structure's designated exit vector and automatically path to the authored Rally Point.
  * **REL-BLD-009.AUTH:** If the emergence point is physically blocked by units or terrain, the producer shall hold the completed unit for up to 40 ticks while nudging obstacles. If still blocked, it fires `[SPAWN BLOCKED]` and pauses the queue.
  * **REL-BLD-009.FAIL:** Units spawning inside building geometry or stacking infinitely on exit fails acceptance.
  * **REL-BLD-009.VERIF:** `PKG-AUTO` (spawn exit obstruction and rally point test).
  * **REL-BLD-009.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-010 — Production Queue Cancellation Invariant:** Cancelling a queued unit before its active production cycle begins shall refund 100% of invested resources. Cancelling an actively producing unit shall refund 50% of invested Matter and 0% Dawn.
  * **REL-BLD-010.AUTH:** Cancellation clears the reserved Logistics slot immediately on the same tick.
  * **REL-BLD-010.FAIL:** Desynchronization between cancelled units and Logistics counters fails sim integrity.
  * **REL-BLD-010.VERIF:** `SRC` (queue cancellation and refund accounting test).
  * **REL-BLD-010.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-011 — Research Slot Contention & Mutual Exclusivity:** Research upgrades (`Prismatic Targeting`, `Echo Cartography`, etc.) conducted at primary production facilities shall occupy the producer slot, halting unit training while research is in progress.
  * **REL-BLD-011.AUTH:** While a technology is researching, unit production buttons shall lock with the reason `[FACILITY BUSY WITH RESEARCH]`.
  * **REL-BLD-011.FAIL:** Training units simultaneously with research without dedicated lab infrastructure fails balance.
  * **REL-BLD-011.VERIF:** `SRC` (producer research contention lock test).
  * **REL-BLD-011.LANE:** Core Gameplay & Tech Design.

* **REL-BLD-012 — Technology Irreversibility & Zero Refund:** Cancelling an in-progress research upgrade or losing the research facility through combat destruction shall permanently forfeit all invested Matter and Dawn.
  * **REL-BLD-012.AUTH:** Research cancellation requires a double-confirmation prompt: `[CANCEL RESEARCH: FORFEIT ALL INVESTED RESOURCES?]`.
  * **REL-BLD-012.FAIL:** Refunding tech costs upon cancellation is strictly prohibited.
  * **REL-BLD-012.VERIF:** `SRC` (tech cancellation forfeiture test).
  * **REL-BLD-012.LANE:** Core Gameplay & Player Experience.

* **REL-BLD-013 — Structural Repair Resolution:** Friendly worker units ordered to damaged friendly structures shall execute Repair at an extraction cost of 5 Matter per second, restoring structure health at 20 HP per second (resolving C25/repair gaps).
  * **REL-BLD-013.AUTH:** Repair continues until the structure reaches maximum health, Matter depletes, or the worker is attacked/redirected.
  * **REL-BLD-013.FAIL:** Repairing without consuming Matter or repairing through unpowered connections fails acceptance.
  * **REL-BLD-013.VERIF:** `SRC` (structure repair rate and resource consumption test).
  * **REL-BLD-013.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-BLD-014 — Structural Destruction Debris & Footprint Clearance:** When a structure is destroyed, it shall trigger destruction VFX/audio, transition to non-colliding ruin debris, and clear its passable terrain footprint after exactly 200 simulation ticks (10.0 seconds).
  * **REL-BLD-014.AUTH:** Terrain occupied by the ruin becomes fully passable and re-buildable at tick 201.
  * **REL-BLD-014.FAIL:** Destroyed buildings leaving permanent impassable dead zones on the map fails acceptance.
  * **REL-BLD-014.VERIF:** `SRC` + `PKG-AUTO` (ruin clearance and footprint recovery test).
  * **REL-BLD-014.LANE:** Core Gameplay & Visual.

---

### §11 Selection, Movement, Commands, and Combat (`REL-CMB-*`)

* **REL-CMB-001 — Authoritative Range & Line-of-Sight Firing Arcs:** Units and defensive structures shall engage hostile targets strictly within their authored discrete firing ranges (Lancer: 650 cm; Bulwark: 300 cm; Riftstalker: 500 cm; Intervalist: 550 cm; Aegis Post: 900 cm).
  * **REL-CMB-001.AUTH:** Targets positioned at $Range + 1	ext{ cm}$ shall not be attacked. Ranged attacks require unoccluded straight-line line-of-sight.
  * **REL-CMB-001.FAIL:** Units shooting beyond declared range bounds or through opaque terrain occluders fails verification.
  * **REL-CMB-001.VERIF:** `SRC` (combat range and occlusion checks).
  * **REL-CMB-001.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-002 — Weapon Cooldown & Attack Cadence Sync:** Attacks shall be gated by authored cooldown intervals (Lancer: 30 ticks = 1.5s; Bulwark: 24 ticks = 1.2s; Riftstalker: 22 ticks = 1.1s).
  * **REL-CMB-002.AUTH:** Visual weapon muzzle flashes and firing audio cues shall synchronize 1:1 with authoritative combat damage ticks.
  * **REL-CMB-002.FAIL:** Units firing faster than cooldown or desynchronized firing animations fail acceptance.
  * **REL-CMB-002.VERIF:** `SRC` (attack cadence interval assertions).
  * **REL-CMB-002.LANE:** Core Gameplay & Animation.

* **REL-CMB-003 — Ballistic Projectile Travel Simulation:** Ranged combat weapons shall simulate discrete ballistic or linear projectile entities travelling at an authored velocity of 1,200 cm/s, replacing instantaneous hitscan resolution (resolving C18).
  * **REL-CMB-003.AUTH:** Damage is applied authoritatively upon projectile arrival at the target coordinates ($TravelTime = Distance / 1200	ext{ cm/s}$).
  * **REL-CMB-003.FAIL:** Instantaneous same-tick hitscan damage across the map is strictly prohibited.
  * **REL-CMB-003.VERIF:** `SRC` (projectile travel time and impact resolution tests).
  * **REL-CMB-003.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-004 — Terrain Line-of-Sight Projectile Occlusion:** Projectiles whose 3D flight trajectory intercepts impassable cliff terrain or occluding structures shall impact the obstruction and be destroyed without dealing damage to the targeted unit (resolving C18).
  * **REL-CMB-004.AUTH:** Shooting through solid cliffs or base walls without ballistic arc clearance is prohibited.
  * **REL-CMB-004.FAIL:** Shoot-through terrain bugs fail combat realism criteria.
  * **REL-CMB-004.VERIF:** `SRC` (projectile cliff interception tests).
  * **REL-CMB-004.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-005 — Destructible Mineral Cover Ballistic Shielding:** Cairnback `Raise Mineral Cover` obstacles (180 HP, 300-tick duration) shall intercept and absorb enemy projectiles whose trajectories intersect their bounding cylinder.
  * **REL-CMB-005.AUTH:** When mineral cover absorbs 180 damage, it shatters and ceases to block incoming fire.
  * **REL-CMB-005.FAIL:** Cover granting universal armor to units standing outside its geometry is prohibited.
  * **REL-CMB-005.VERIF:** `SRC` (mineral cover projectile absorption test).
  * **REL-CMB-005.LANE:** Core Gameplay & Factions.

* **REL-CMB-006 — Deterministic Damage Calculation Hygiene:** Combat damage shall calculate via exact deterministic damage tables without random critical strikes, dice rolls, or hidden accuracy percentages.
  * **REL-CMB-006.AUTH:** Final damage equals authored weapon base damage modified by active technological multipliers and directional defenses (e.g. deployed Bulwark 40% frontal reduction).
  * **REL-CMB-006.FAIL:** Any random variation in damage numbers fails determinism audit.
  * **REL-CMB-006.VERIF:** `SRC` (deterministic combat resolution suite).
  * **REL-CMB-006.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-007 — Overkill Avoidance Targeting Protocol:** Ranged units operating under autonomous attack orders shall evaluate in-flight projectile damage directed at their target; if pending in-flight damage is sufficient to destroy the target, subsequent units shall retarget the next viable hostile.
  * **REL-CMB-007.AUTH:** Prevents wasteful firing of entire army volleys at a 1-HP enemy.
  * **REL-CMB-007.FAIL:** Ranged forces firing 100% of attacks into already-dead targets fail perceived tactical intelligence.
  * **REL-CMB-007.VERIF:** `SRC` (overkill avoidance targeting test).
  * **REL-CMB-007.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-008 — Threat-Based Target Priority Hierarchy:** Autonomous combat targeting shall evaluate threats using an authored hierarchy: (1) Active attackers within range; (2) Armed combat units; (3) Defensive structures; (4) Economy/production structures; (5) Unarmed workers.
  * **REL-CMB-008.AUTH:** Idle units under attack shall prioritize retaliating against the attacker over targeting nearby harmless structures.
  * **REL-CMB-008.FAIL:** Units attacking passive structures while being destroyed by enemy combatants fails tactical AI.
  * **REL-CMB-008.VERIF:** `SRC` (threat ranking priority test).
  * **REL-CMB-008.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-009 — Combat Stance Architecture:** All combat units shall support four explicit combat stances: `Aggressive` (pursues and attacks threats within 1,200 cm), `Defensive` (retaliates within 800 cm, returning to anchor after 400 cm chase), `Hold Ground` (attacks in range, never translates), and `Hold Fire` (never attacks autonomously).
  * **REL-CMB-009.AUTH:** Stance state shall be displayed on the HUD command card and hotkey-switchable (`F`).
  * **REL-CMB-009.FAIL:** Default behavior remaining passive Hold Fire without autonomous retaliation is prohibited (resolving gap).
  * **REL-CMB-009.VERIF:** `PKG-AUTO` (stance behavioral compliance test suite).
  * **REL-CMB-009.LANE:** Core Gameplay & Player Experience.

* **REL-CMB-010 — Hold Ground Positional Rigidity:** Units ordered to `Hold Ground` shall remain locked to their exact spatial coordinates. They shall engage valid hostiles entering weapon range but shall never move to pursue retreating enemies.
  * **REL-CMB-010.AUTH:** Translation distance while in Hold Ground stance shall equal exactly 0.0 cm.
  * **REL-CMB-010.FAIL:** Units breaking Hold Ground to chase scouts fails tactical control.
  * **REL-CMB-010.VERIF:** `SRC` (Hold Ground position invariance test).
  * **REL-CMB-010.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-011 — Attack-Move Engagement Mechanics:** Units issued an `Attack-Move` order to a destination coordinate shall advance along the path, immediately halting to engage any enemy unit or hostile structure encountered within visual detection range.
  * **REL-CMB-011.AUTH:** Once the threat is eliminated or leaves range, the unit resumes pathing toward the original destination.
  * **REL-CMB-011.FAIL:** Units walking past firing enemies without halting fails core RTS expectations.
  * **REL-CMB-011.VERIF:** `PKG-AUTO` (Attack-Move path and engage test).
  * **REL-CMB-011.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-012 — Cyclic Patrol Waypoint Navigation:** Units issued a `Patrol` order between point A and point B shall cycle continuously between the waypoints, automatically engaging hostiles encountered along the transit corridor.
  * **REL-CMB-012.AUTH:** Upon reaching point B, the unit reverses course to point A; patrol continues until a new command is received.
  * **REL-CMB-012.FAIL:** Patrol stopping after a single leg fails acceptance.
  * **REL-CMB-012.VERIF:** `SRC` (patrol loop continuity test).
  * **REL-CMB-012.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-013 — Guard Escort Dynamics:** Units issued a `Guard` order targeting an allied unit shall shadow the target's movement, maintaining an escort radius of 200–400 cm, and immediately engage any hostile attacking the guarded unit.
  * **REL-CMB-013.AUTH:** If the guarded target is destroyed, guarding units shall halt, drop the order, and transition to standard Defensive stance.
  * **REL-CMB-013.FAIL:** Escorts lagging behind and failing to protect VIP units fails campaign mechanics.
  * **REL-CMB-013.VERIF:** `PKG-AUTO` (VIP escort protection test).
  * **REL-CMB-013.LANE:** Core Gameplay & Campaign.

* **REL-CMB-014 — Direct Focus Fire Command Override:** Explicit right-click targeting of a specific enemy unit shall override all autonomous threat ranking, forcing all selected units to focus fire on the designated entity until destroyed or out of range.
  * **REL-CMB-014.AUTH:** Focus firing shall not be interrupted by peripheral attackers.
  * **REL-CMB-014.FAIL:** Units ignoring manual target overrides fails micro-control.
  * **REL-CMB-014.VERIF:** `PKG-PHYS` (focus fire target tracking test).
  * **REL-CMB-014.LANE:** Core Gameplay & Player Experience.

* **REL-CMB-015 — Entity Destruction & Collision Purge:** When an entity's health drops to 0, it shall be marked dead on the same simulation tick, immediately disabling its spatial collision, targeting eligibility, and vision contribution.
  * **REL-CMB-015.AUTH:** The visual destruction animation shall play without blocking movement of surviving units through that space.
  * **REL-CMB-015.FAIL:** Dead units continuing to block pathing or absorb weapon fire fails acceptance.
  * **REL-CMB-015.VERIF:** `SRC` (zero-health destruction cleanup test).
  * **REL-CMB-015.LANE:** Core Gameplay & Presentation.

* **REL-CMB-016 — Multi-Channel Combat Damage Feedback:** Taking damage shall provide clear, non-competing feedback: (1) Overhead health bar depletion; (2) Faction-specific impact audio cue; (3) Brief material emissive pulse (100 ms); (4) Minimap spatial alert if off-screen.
  * **REL-CMB-016.ACC:** Under Reduced Flashing, emissive pulses shall be replaced with high-contrast silhouette border highlights (resolving C36).
  * **REL-CMB-016.FAIL:** Silent damage without player awareness fails game feel.
  * **REL-CMB-016.VERIF:** `PKG-REND` (combat damage visual and audio feedback review).
  * **REL-CMB-016.LANE:** Visual Presentation & Audio.

* **REL-CMB-017 — Fog of War Engagement Constraints:** Units shall not fire upon enemy entities concealed within unexplored shroud or explored fog of war without active line-of-sight vision provided by an allied spotter.
  * **REL-CMB-017.AUTH:** Firing blind into fog without radar/sensor detection is strictly prohibited.
  * **REL-CMB-017.FAIL:** Weapon targeting piercing fog of war fails information hygiene.
  * **REL-CMB-017.VERIF:** `SRC` (fog targeting rejection test).
  * **REL-CMB-017.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-018 — Worker Disarmament Invariant:** Standard economy worker units (`Surveyor`, `Tender`, `Threadkeeper`) shall possess 0 weapon attack damage and cannot engage in combat (resolving C25).
  * **REL-CMB-018.AUTH:** Right-clicking an enemy unit with a worker shall reject with `[WORKERS CANNOT ATTACK]`.
  * **REL-CMB-018.FAIL:** Stale simulation rules granting weapons to workers violate the core design.
  * **REL-CMB-018.VERIF:** `SRC` (native simulation worker attack rejection test).
  * **REL-CMB-018.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-028 — Shift-Queued Order Pipelining Execution:** Pipelining commands using Shift shall preserve full order parameters across transitions: Move → Gather initializes immediate gathering upon arrival; Move → Build initiates construction immediately upon reaching footprint perimeter.
  * **REL-CMB-028.AUTH:** Zero tick stall between arriving at construction site and beginning build work.
  * **REL-CMB-028.FAIL:** Worker arriving at queued build site and idling without starting construction fails acceptance.
  * **REL-CMB-028.VERIF:** `PKG-AUTO` (shift-queued move-to-build execution test).
  * **REL-CMB-028.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-029 — Real-Time Waypoint Path Preview:** Holding Shift shall render animated path trajectories with faction-specific accent lines (Meridian Cyan dashed, Kharuun Amber faceted, Choir Magenta double-trace).
  * **REL-CMB-029.AUTH:** Splines project cleanly clamped to terrain heightfields without clipping into cliff geometry.
  * **REL-CMB-029.FAIL:** Floating splines or missing glyph icons fails visual polish.
  * **REL-CMB-029.VERIF:** `PKG-REND` (waypoint spline visual review).
  * **REL-CMB-029.LANE:** Visual Presentation.

* **REL-CMB-030 — Smart-Cast Energy & Cooldown Conservation:** Smart-cast dispatching shall inspect unit cooldown state and resource pools, skipping units on cooldown and selecting the nearest unit ready to cast.
  * **REL-CMB-030.AUTH:** If no unit in the selection is ready, the command card pulses red and plays the interface error tone.
  * **REL-CMB-030.FAIL:** Consuming Dawn without casting ability fails validation.
  * **REL-CMB-030.VERIF:** `SRC` (smart-cast cooldown skipping test).
  * **REL-CMB-030.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-CMB-031 — Priority Threat Resolution in Combat Formations:** When an army advances in Line or Wedge formation, frontline units shall engage hostiles without breaking overall formation frontage unless the threat bypasses the flank.
  * **REL-CMB-031.AUTH:** Frontline holds frontage while ranged units deliver screen fire from the rear pocket.
  * **REL-CMB-031.FAIL:** Units clumping into a single vulnerable point mass during attack-move fails tactical formation rules.
  * **REL-CMB-031.VERIF:** `PKG-AUTO` (formation combat frontage retention test).
  * **REL-CMB-031.LANE:** Core Gameplay & Formations.

* **REL-CMB-032 — Chase Leash Boundary Enforcement:** The 400 cm chase leash shall clamp pursuit distance calculated from the coordinate where the attack order was originally received, preventing "kiting lures" from breaking base defenses.
  * **REL-CMB-032.AUTH:** Upon exceeding 400 cm from anchor, units disengage and return to formation.
  * **REL-CMB-032.FAIL:** Units pursuing infinitely into enemy static batteries fails defense discipline.
  * **REL-CMB-032.VERIF:** `SRC` (chase leash return-to-anchor test).
  * **REL-CMB-032.LANE:** Core Gameplay (`EchoesSimCore`).

### §12 Factions, Rosters, and Strategic Depth (`REL-FAC-*`)

* **REL-FAC-001 — Tripartite Asymmetric Strategic Identity:** The game shall deliver three fully differentiated playable factions: the Meridian Compact (engineered infrastructure, grid logistics, disciplined ranged fire), the Kharuun Assemblies (grown mineral-organic architecture, mobile bases, seismic detection), and the Hollow Choir (quantum possibility states, temporal reconciliation, structural coherence upkeep).
  * **REL-FAC-001.AUTH:** Each faction shall possess a distinct 4-unit / 4-building / 2-tech launch roster obeying the Development Bible.
  * **REL-FAC-001.FAIL:** Homogenized factions sharing identical mechanics fail creative intent.
  * **REL-FAC-001.VERIF:** `SRC` (faction catalog validation).
  * **REL-FAC-001.LANE:** Faction Design & Canon.

* **REL-FAC-002 — Meridian Compact Power Grid Topology:** Meridian structures shall operate on an interconnected power network. Power Links and Array Foundries project an 800 cm power distribution radius.
  * **REL-FAC-002.AUTH:** Defensive structures (`Aegis Post`) require unbroken power connectivity to the primary Anchor grid to function.
  * **REL-FAC-002.FAIL:** Severing a Power Link leaving an Aegis Post unpowered shall disable its weapon within 1 simulation tick.
  * **REL-FAC-002.VERIF:** `SRC` (power network graph connectivity tests).
  * **REL-FAC-002.LANE:** Core Gameplay & Factions.

* **REL-FAC-003 — Meridian Power Link Distribution & Throughput:** Power Links (Cost: 90 Matter / 10 Dawn, 100 ticks, 2x2 footprint) shall provide +6 Logistics, function as a Matter drop-off, and extend the power grid.
  * **REL-FAC-003.AUTH:** Power Links do not attack and cannot produce combat units.
  * **REL-FAC-003.FAIL:** Power Link failing to accept Matter cargo or drop Logistics upon destruction fails validation.
  * **REL-FAC-003.VERIF:** `SRC` (Power Link functional verification).
  * **REL-FAC-003.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-FAC-004 — Meridian Aegis Post Automated Defensive Battery:** Aegis Posts (Cost: 130 Matter / 30 Dawn, 120 ticks, 520 HP, 700 cm sight) shall automatically engage valid hostiles within 900 cm for 28 damage every 20 ticks when powered.
  * **REL-FAC-004.AUTH:** When unpowered, the Aegis Post shall enter an inert powered-down state, presenting an offline status indicator.
  * **REL-FAC-004.FAIL:** Aegis Post firing while unpowered is a critical balance violation.
  * **REL-FAC-004.VERIF:** `SRC` (Aegis Post combat and power state assertions).
  * **REL-FAC-004.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-FAC-005 — Meridian Bulwark Team Directional Shield Deployment:** Bulwark Teams (Cost: 130/25, 260 HP, 230 cm/s) shall support `Deploy` (20 ticks) and `Pack` (15 ticks). While deployed, the unit grants 40% frontal damage reduction and reduces movement speed to 35%.
  * **REL-FAC-005.AUTH:** Frontal arc covers 120 degrees facing; attacks from flank or rear bypass the 40% damage reduction.
  * **REL-FAC-005.FAIL:** Instant instantaneous deployment (0 ticks) violates the commitment cost requirement (resolving gap).
  * **REL-FAC-005.VERIF:** `SRC` (Bulwark deployment transition and angular damage reduction tests).
  * **REL-FAC-005.LANE:** Core Gameplay & Factions.

* **REL-FAC-006 — Meridian Relay Skiff Logistics Relay:** Relay Skiffs (Cost: 70/20, 75 HP, 500 cm/s) connected within 700 cm of friendly infrastructure may activate `Relay Supply`, granting +4 temporary Logistics for 400 ticks with an 800-tick cooldown.
  * **REL-FAC-006.AUTH:** The HUD discloses active connection, remaining burst duration, and cooldown countdown.
  * **REL-FAC-006.FAIL:** Triggering Relay Supply without valid infrastructure connection fails validation.
  * **REL-FAC-006.VERIF:** `SRC` (Relay Skiff ability lifecycle test).
  * **REL-FAC-006.LANE:** Core Gameplay & Factions.

* **REL-FAC-007 — Kharuun Waystone Rooting & Relocation:** Kharuun Waystones (Cost: 80/20, 390 HP, +5 Logistics) shall support mobile migration: Uproot takes 40 ticks; mobile transit moves at 120 cm/s taking 125% damage; Root takes 60 ticks on clear ground.
  * **REL-FAC-007.AUTH:** Uprooted mobile Waystones do not provide Logistics and do not accept Matter drop-offs.
  * **REL-FAC-007.FAIL:** Waystones functioning as drop-offs while mobile is strictly prohibited.
  * **REL-FAC-007.VERIF:** `SRC` (Waystone migration state machine tests).
  * **REL-FAC-007.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-FAC-008 — Kharuun Listening Spine Seismic Detection:** Listening Spines (Cost: 115/25, 440 HP, 120 ticks, 2x2 footprint) shall passively detect moving ground signatures within 2,600 cm at 200 cm resolution for 40 ticks.
  * **REL-FAC-008.AUTH:** Signatures are anonymous spatial pings; they do not reveal unit identity, health, or allow direct targeting. Stationary units remain undetected.
  * **REL-FAC-008.FAIL:** Revealing exact unit type or direct targetability via seismic detection fails design rules.
  * **REL-FAC-008.VERIF:** `SRC` (seismic radar ping and anonymity test).
  * **REL-FAC-008.LANE:** Core Gameplay & Factions.

* **REL-FAC-009 — Kharuun Warform Adaptation Molting:** Eligible Kharuun combat units within 600 cm of a completed Growth Basin may spend 25 Dawn to undergo an 80-tick molt, choosing `Carapace` (135% HP, 80% speed) or `Striker` (125% damage, 85% cooldown).
  * **REL-FAC-009.AUTH:** Molting units take 150% damage during the 80-tick vulnerability window. Interruption cancels the adaptation without refunding Dawn.
  * **REL-FAC-009.FAIL:** Instant molt or invulnerability during adaptation fails balance.
  * **REL-FAC-009.VERIF:** `SRC` (molting state, vulnerability multiplier, and stat modification tests).
  * **REL-FAC-009.LANE:** Core Gameplay & Factions.

* **REL-FAC-010 — Kharuun Cairnback Mineral Cover Erection:** Cairnbacks (Cost: 120/30, 245 HP) may spend 15 Dawn to erect destructible Mineral Cover (180 HP, 300 ticks duration, 600 ticks cooldown) within 450 cm.
  * **REL-FAC-010.AUTH:** The cover physically obstructs enemy ballistic projectiles along its line of sight.
  * **REL-FAC-010.FAIL:** Erection on impassable tiles or overlapping existing units rejects cleanly.
  * **REL-FAC-010.VERIF:** `SRC` (Cairnback cover placement and projectile blocking tests).
  * **REL-FAC-010.LANE:** Core Gameplay & Factions.

* **REL-FAC-011 — Hollow Choir Structural Coherence Upkeep Cycle:** Completed Hollow Choir structures (`Interval Loom`, `Chorus Loom`, `Phase Anchor`) shall independently consume 5 Dawn every 600 simulation ticks (30.0 seconds) to maintain quantum coherence (resolving C20).
  * **REL-FAC-011.AUTH:** The HUD economy monitor shall display upcoming coherence charge countdowns. If Dawn is insufficient at charge tick, the structure emits 3 warnings before collapsing.
  * **REL-FAC-011.FAIL:** Instant untelegraphed destruction without 3 prior warning telemetry states is prohibited.
  * **REL-FAC-011.VERIF:** `SRC` + `PKG-REND` (Choir upkeep cycle and warning alerts test).
  * **REL-FAC-011.LANE:** Core Gameplay & Player Experience.

* **REL-FAC-012 — Hollow Choir Reconciliation Identity Transition:** Choir combat units may spend 20 Dawn to reconcile into `Manifest` (130% damage) or `Possible` (130% speed, 125% vision).
  * **REL-FAC-012.AUTH:** The 160-tick transition window is a liability state with a 400-tick cooldown; units shall NEVER receive both Manifest and Possible bonuses simultaneously (resolving C19).
  * **REL-FAC-012.FAIL:** Holding 130% damage AND 130% speed concurrently during transition is a critical defect.
  * **REL-FAC-012.VERIF:** `SRC` (Choir reconciliation mutual exclusivity and liability assertions).
  * **REL-FAC-012.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-FAC-013 — Hollow Choir Phase Anchor Coherence Field:** Phase Anchors (Cost: 120/35, 480 HP, 800 cm sight) shall project a 700 cm stabilization field reducing the recurring coherence charge of friendly Choir structures from 5 Dawn to 4 Dawn (resolving C20).
  * **REL-FAC-013.AUTH:** Structures positioned within overlapping fields receive a maximum reduction to 3 Dawn (minimum floor).
  * **REL-FAC-013.FAIL:** Phase Anchor doing nothing as dead code violates normative specification.
  * **REL-FAC-013.VERIF:** `SRC` (Phase Anchor upkeep reduction aura test).
  * **REL-FAC-013.LANE:** Core Gameplay & Factions.

---

### §13 Future Wells (`REL-WEL-*`)

* **REL-WEL-001 — Future Well Landmark Entity Authority:** Future Wells shall exist as neutral, impassable spatial landmarks on designated map coordinates, surrounded by an interactable capture radius of 420 cm.
  * **REL-WEL-001.AUTH:** Wells are indestructible and immune to direct combat damage.
  * **REL-WEL-001.FAIL:** Wells taking weapon damage or pathing units through their core mesh fails geometry rules.
  * **REL-WEL-001.VERIF:** `SRC` (Future Well entity collision and immunity tests).
  * **REL-WEL-001.LANE:** World & Core Gameplay.

* **REL-WEL-002 — Future Well Dormant State & Telemetry:** In the Dormant state, a Future Well emits low ambient fracture hum (40–60 Hz), projects an unowned neutral ring, and offers three mutually exclusive protocols: Harvest, Preserve, Reshape.
  * **REL-WEL-002.AUTH:** Selection panel displays exact resource costs, telegraph durations, and narrative consequences.
  * **REL-WEL-002.FAIL:** Ambiguous tooltips omitting duration or irreversibility fail usability.
  * **REL-WEL-002.VERIF:** `PKG-REND` (dormant Well UI and audio review).
  * **REL-WEL-002.LANE:** Player Experience & Audio.

* **REL-WEL-003 — Contested Well Capture Resolution:** Capturing a Dormant Well requires an eligible friendly worker unit to maintain continuous presence within the 420 cm capture zone for 300 simulation ticks (15.0 seconds) (resolving C9).
  * **REL-WEL-003.AUTH:** Capture progress accumulates at +1 point per tick up to 300. Control cannot be seized instantaneously in 1 tick.
  * **REL-WEL-003.FAIL:** Instant capture by a worker stepping into the zone is a critical defect.
  * **REL-WEL-003.VERIF:** `SRC` (300-tick Well capture progression test).
  * **REL-WEL-003.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-WEL-004 — Well Contestation & Meter Decay:** If hostile units enter the 420 cm zone during capture, progress shall halt immediately (`CONTESTED`).
  * **REL-WEL-004.AUTH:** If all capturing units depart or are killed, progress decays at -1 point per tick until reaching 0.
  * **REL-WEL-004.FAIL:** Progress continuing to advance while enemies occupy the zone fails acceptance.
  * **REL-WEL-004.VERIF:** `SRC` (capture contestation and decay test).
  * **REL-WEL-004.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-WEL-005 — Harvest Protocol Public Telegraph:** Initiating `Harvest` shall trigger a global 180-tick (9.0 second) public telegraph, broadcasting an alert siren and spatial map ping to all players (resolving C9).
  * **REL-WEL-005.AUTH:** If the harvesting player loses control of the Well before tick 180, the Harvest aborts with zero payout.
  * **REL-WEL-005.FAIL:** Silent instant harvesting without 180-tick telegraph is prohibited.
  * **REL-WEL-005.VERIF:** `SRC` (Harvest telegraph interruptibility test).
  * **REL-WEL-005.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-WEL-006 — Harvest Collapse & Irreversible Scarring:** Successful completion of Harvest awards +500 Dawn immediately, collapses the Well, and converts its surrounding 600 cm radius into permanent `Scarred` terrain.
  * **REL-WEL-006.AUTH:** The Well is permanently removed from the match and cannot be recaptured or reused.
  * **REL-WEL-006.FAIL:** Re-harvesting a collapsed Well fails sim invariants.
  * **REL-WEL-006.VERIF:** `SRC` (Harvest collapse, resource award, and scarring test).
  * **REL-WEL-006.LANE:** Core Gameplay & World.

* **REL-WEL-007 — Preserve Protocol Compounding Inflow:** Selecting `Preserve` leaves the Well active and awards +15 Dawn every 300 simulation ticks (15.0 seconds) exclusively to the current controlling faction.
  * **REL-WEL-007.AUTH:** Inflow halts immediately if control is lost or contested.
  * **REL-WEL-007.FAIL:** Continued payment to former owners after losing control fails validation.
  * **REL-WEL-007.VERIF:** `SRC` (Preserve recurring resource payout test).
  * **REL-WEL-007.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-WEL-008 — Preserve Radar Reconnaissance Aura:** An operational Preserved Well projects continuous 1,400 cm line-of-sight vision around its perimeter for the controlling player.
  * **REL-WEL-008.AUTH:** Shrouded fog within 1,400 cm is cleared to Visible state while under control.
  * **REL-WEL-008.FAIL:** Vision lingering after losing control fails information hygiene.
  * **REL-WEL-008.VERIF:** `SRC` (Preserve vision aura and fog reveal test).
  * **REL-WEL-008.LANE:** Core Gameplay & World.

* **REL-WEL-009 — Preserve Contested Recapture Mechanics:** A Preserved Well remains permanently contestable. An opponent capturing the zone (300 ticks) transfers the +15 Dawn inflow and radar vision to the new owner without destroying the Well.
  * **REL-WEL-009.AUTH:** Control transfer fires high-priority alerts: `[FUTURE WELL CAPTURED]` / `[FUTURE WELL LOST]`.
  * **REL-WEL-009.FAIL:** Inability to recapture a Preserved Well violates core RTS counterplay.
  * **REL-WEL-009.VERIF:** `SRC` (Well ownership flip and transfer tests).
  * **REL-WEL-009.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-WEL-010 — Reshape Protocol Invocation:** Initiating `Reshape` costs 120 liquid Dawn, broadcasts a 180-tick public telegraph, and upon completion manifests an alternate temporal passage for 1,800 ticks (90.0 seconds) (resolving C21).
  * **REL-WEL-010.AUTH:** Insufficient Dawn (<120) rejects immediately without telegraph.
  * **REL-WEL-010.FAIL:** Reshaping without paying Dawn cost fails resource validation.
  * **REL-WEL-010.VERIF:** `SRC` (Reshape cost and telegraph validation test).
  * **REL-WEL-010.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-WEL-011 — Reshape Terrain Transformation Passability:** Successful Reshape transforms authored blocked rift tiles into passable terrain bridge geometry, updating the pathfinder mesh dynamically across both players within 1 tick (resolving C21).
  * **REL-WEL-011.AUTH:** Units can path across the manifested bridge for the full 1,800-tick duration.
  * **REL-WEL-011.FAIL:** Units refusing to route across the manifested bridge fails pathfinding.
  * **REL-WEL-011.VERIF:** `PKG-AUTO` (Reshape bridge pathing and traversal test).
  * **REL-WEL-011.LANE:** Core Gameplay & World.

* **REL-WEL-012 — Reshape Expiration & Safe Boundary Displacement:** When the 1,800-tick manifestation expires, the bridge collapses back to impassable terrain. Any mobile unit occupying the collapsing tiles shall be displaced safely to the nearest passable boundary tile without dying or teleports (resolving C22).
  * **REL-WEL-012.AUTH:** Displacement takes ≤5 ticks; unit preserves current orders and takes 0 collision damage.
  * **REL-WEL-012.FAIL:** Teleporting units across the map or killing them upon bridge expiry is a critical defect.
  * **REL-WEL-012.VERIF:** `SRC` (Reshape expiration safe boundary displacement test).
  * **REL-WEL-012.LANE:** Core Gameplay (`EchoesSimCore`).

---

### §14 Campaign and Narrative (`REL-CAM-*`)

* **REL-CAM-001 — Fifteen-Operation Continuous Campaign Lifecycle:** The single-player campaign shall consist of 15 fully authored operations chaining consequences sequentially across Acts I, II, and III, playable from start to finish.
  * **REL-CAM-001.AUTH:** Mission completion writes persistent records into `EchoesCampaignProgress`, unlocking subsequent operations.
  * **REL-CAM-001.FAIL:** Broken chapter links or missing progression states fail campaign acceptance.
  * **REL-CAM-001.VERIF:** `PKG-AUTO` (`EchoesCampaignProgressTest`).
  * **REL-CAM-001.LANE:** Campaign & Narrative.

* **REL-CAM-002 — Act I: "Necessary Fires" (Operations 01–05):** Act I shall establish the evacuation of Lume Reach, introducing core RTS fundamentals, migration logistics, and the initial Future Well commitment.
  * **REL-CAM-002.AUTH:** Features Operations: M01 Lume Reach, M02 The Long Sift, M03 City Reserve, M04 The Unburied Road, M05 Terms of Continuance.
  * **REL-CAM-002.FAIL:** Act I completing without recording the M01 Well decision fails persistence.
  * **REL-CAM-002.VERIF:** `PKG-AUTO` (Act I end-to-end traversal test).
  * **REL-CAM-002.LANE:** Campaign & Narrative.

* **REL-CAM-003 — Act II: "The Cost of One Future" (Operations 06–10):** Act II shall expand strategic complexity, introducing covert reconnaissance, Kharuun subterranean mechanics, and the Basin of Scars decisive battle.
  * **REL-CAM-003.AUTH:** Features Operations: M06 Names Without Births, M07 Shape of Silence, M08 Folded Echo, M09 Authority Site, M10 Basin of Scars.
  * **REL-CAM-003.FAIL:** Desynchronized district power state in M09 fails progression.
  * **REL-CAM-003.VERIF:** `PKG-AUTO` (Act II end-to-end traversal test).
  * **REL-CAM-003.LANE:** Campaign & Narrative.

* **REL-CAM-004 — Act III: "Crownfall" (Operations 11–15):** Act III shall drive the climax at the Broken Sun, testing high-intensity macro coordination, reality bleed, and the four final resolutions.
  * **REL-CAM-004.AUTH:** Features Operations: M11 Life Support Fracture, M12 Transit Collapse, M13 Archive Severance, M14 Possibility Bleed, M15 The Broken Sun.
  * **REL-CAM-004.FAIL:** Broken mission 15 resolution logic fails campaign closure.
  * **REL-CAM-004.VERIF:** `PKG-AUTO` (Act III end-to-end traversal test).
  * **REL-CAM-004.LANE:** Campaign & Narrative.

* **REL-CAM-005 — Four Earned Campaign Endings:** The final operation (M15) shall resolve into one of four earned narrative endings based on cumulative campaign choices: `Restoration`, `Controlled Stabilization`, `Extinguishment`, or `Open Evolution`.
  * **REL-CAM-005.AUTH:** Ending eligibility matches the mathematical decision matrix in `DevelopmentBible.md`; no single choice retroactively rewrites history.
  * **REL-CAM-005.FAIL:** Ending cinematic mismatching the recorded ledger decision fails acceptance.
  * **REL-CAM-005.VERIF:** `PKG-AUTO` (all 4 endings resolution path tests).
  * **REL-CAM-005.LANE:** Campaign & Cinematics.

* **REL-CAM-006 — Mission 01: "What the Ledger Keeps" Contract:** M01 shall mandate establishing an evacuation perimeter, escorting the Archive Carrier to the withdrawal line, and committing the first Future Well.
  * **REL-CAM-006.AUTH:** Primary objective completes only when carrier reaches extraction tile `(42,18)`; Well choice is written to the campaign ledger.
  * **REL-CAM-006.FAIL:** Losing the carrier triggers defeat within 1 tick.
  * **REL-CAM-006.VERIF:** `PKG-AUTO` (`EchoesPrologueMissionTest`).
  * **REL-CAM-006.LANE:** Campaign.

* **REL-CAM-007 — Mission 02: "The Long Sift" Contract:** M02 shall require escorting a mobile Kharuun migration column across the open basin while avoiding hostile patrol sweeps.
  * **REL-CAM-007.AUTH:** Requires $\ge 3$ of 4 migration tenders to survive across waypoints 1–3 to trigger victory.
  * **REL-CAM-007.FAIL:** Loss of 2 tenders triggers operation failure.
  * **REL-CAM-007.VERIF:** `PKG-AUTO` (`EchoesMigrationMissionTest`).
  * **REL-CAM-007.LANE:** Campaign.

* **REL-CAM-008 — Mission 03: "City Reserve" Contract:** M03 shall require restoring power to three ark-city districts (Life Support, Transit, Archive) in authored priority order (resolving C29).
  * **REL-CAM-008.AUTH:** Powering districts out of designated priority order rejects with `[RESERVE PLAN VIOLATION]`.
  * **REL-CAM-008.FAIL:** Arbitrary power order completing the mission violates design rules.
  * **REL-CAM-008.VERIF:** `PKG-AUTO` (`EchoesCityReserveMissionTest`).
  * **REL-CAM-008.LANE:** Campaign.

* **REL-CAM-009 — Mission 04: "The Unburied Road" Contract:** M04 shall require navigating subsurface Kharuun passages to escort vital civic records under an active bombardment zone.
  * **REL-CAM-009.AUTH:** Subsurface transit timing and entrance queueing adhere to §7.1 passage rules.
  * **REL-CAM-009.FAIL:** Record bearer destroyed triggers immediate failure.
  * **REL-CAM-009.VERIF:** `PKG-AUTO` (`EchoesUnburiedRoadMissionTest`).
  * **REL-CAM-009.LANE:** Campaign.

* **REL-CAM-010 — Mission 05: "Terms of Continuance" Contract:** M05 shall require holding the diplomatic enclave until the negotiation window opens between ticks 300 and 900, followed by safe extraction of Mara Vey.
  * **REL-CAM-010.AUTH:** Moving witnesses toward extraction before tick 900 does NOT cause instant failure (resolving C31).
  * **REL-CAM-010.FAIL:** Mara Vey loss prior to extraction causes defeat.
  * **REL-CAM-010.VERIF:** `PKG-AUTO` (`EchoesTermsOfContinuanceMissionTest`).
  * **REL-CAM-010.LANE:** Campaign.

* **REL-CAM-011 — Mission 06: "Names Without Births" Contract:** M06 shall mandate reconnoitering the missing census district, locating civilian survivors, and guiding them to the eastern shelter extraction zone without production structures.
  * **REL-CAM-011.AUTH:** Objective updates upon discovering survivor clusters; zero initial base production allowed.
  * **REL-CAM-011.FAIL:** Survivor casualties $>20\%$ triggers failure.
  * **REL-CAM-011.VERIF:** `PKG-AUTO` (`EchoesNamesWithoutBirthsMissionTest`).
  * **REL-CAM-011.LANE:** Campaign.

* **REL-CAM-012 — Mission 07: "Shape of Silence" Contract:** M07 shall mandate deploying Listening Spines to detect enemy movements while avoiding superior hostile search forces.
  * **REL-CAM-012.AUTH:** Constructing Spines at coordinates `(18,24)` and `(32,45)` reveals patrol routes; Oruun must survive.
  * **REL-CAM-012.FAIL:** Engaging hostile heavy forces directly triggers tactical wipe.
  * **REL-CAM-012.VERIF:** `PKG-AUTO` (`EchoesShapeOfSilenceMissionTest`).
  * **REL-CAM-012.LANE:** Campaign.

* **REL-CAM-013 — Mission 08: "Folded Echo" Contract:** M08 shall require traversing paired quantum echo sites with Talar Venn, synchronizing relay activation across temporal folds.
  * **REL-CAM-013.AUTH:** Talar must maintain guard escorts and raise Echo Relays at paired coordinates `(24,16)` and `(50,38)`.
  * **REL-CAM-013.FAIL:** Talar loss triggers mission failure.
  * **REL-CAM-013.VERIF:** `PKG-AUTO` (`EchoesFoldedEchoMissionTest`).
  * **REL-CAM-013.LANE:** Campaign.

* **REL-CAM-014 — Mission 09: "Authority Site" Contract:** M09 shall mandate securing the central administrative nexus, physically moving Mara Vey into the authority chamber, and defending the perimeter (resolving C30).
  * **REL-CAM-014.AUTH:** Powering the district alone does NOT complete the objective; Mara Vey must enter the site footprint `(28,28)`.
  * **REL-CAM-014.FAIL:** Mara dying outside the chamber causes defeat.
  * **REL-CAM-014.VERIF:** `PKG-AUTO` (`EchoesAuthoritySiteMissionTest`).
  * **REL-CAM-014.LANE:** Campaign.

* **REL-CAM-015 — Mission 10: "Basin of Scars" Contract:** M10 shall feature a decisive multi-base confrontation across twin anchors at `(28,39)` and `(36,39)`, establishing control over the Crownfall approach.
  * **REL-CAM-015.AUTH:** Destroying both enemy forward outposts completes the primary objective.
  * **REL-CAM-015.FAIL:** Both friendly anchors destroyed causes defeat.
  * **REL-CAM-015.VERIF:** `PKG-AUTO` (`EchoesBasinOfScarsMissionTest`).
  * **REL-CAM-015.LANE:** Campaign.

* **REL-CAM-016 — Mission 11: "Life Support Fracture" Contract:** M11 shall mandate defending the fractured life-support conduits of the central ark-city against waves of hostile breaches under an oxygen countdown.
  * **REL-CAM-016.AUTH:** Conduits must retain $\ge 25\%$ structural integrity until stabilization.
  * **REL-CAM-016.FAIL:** Conduit collapse triggers atmospheric breach defeat.
  * **REL-CAM-016.VERIF:** `PKG-AUTO` (`EchoesLifeSupportMissionTest`).
  * **REL-CAM-016.LANE:** Campaign.

* **REL-CAM-017 — Mission 12: "Transit Collapse" Contract:** M12 shall require coordinating an emergency evacuation across collapsing elevated transit spans while defending the choke approaches.
  * **REL-CAM-017.AUTH:** Spans collapse on authored timers; units must cross before span countdown reaches 0.
  * **REL-CAM-017.FAIL:** Trapped civilian transports destroyed on collapsed spans causes defeat.
  * **REL-CAM-017.VERIF:** `PKG-AUTO` (`EchoesTransitCollapseMissionTest`).
  * **REL-CAM-017.LANE:** Campaign.

* **REL-CAM-018 — Mission 13: "Archive Severance" Contract:** M13 shall require extracting the Crownfall index data cores from the central archive facility while under total tactical siege.
  * **REL-CAM-013.AUTH:** Data extraction requires holding the 3 archive terminals for 400 ticks.
  * **REL-CAM-018.FAIL:** Total terminal destruction causes defeat.
  * **REL-CAM-018.VERIF:** `PKG-AUTO` (`EchoesArchiveSeveranceMissionTest`).
  * **REL-CAM-018.LANE:** Campaign.

* **REL-CAM-019 — Mission 14: "Possibility Bleed" Contract:** M14 shall introduce the Hollow Choir manifestation crisis, requiring containment of temporal rifts and holding the line against reality distortion.
  * **REL-CAM-019.AUTH:** Containment anchors must be maintained at 4 rift locations for 160-tick crisis intervals.
  * **REL-CAM-019.FAIL:** Uncontained bleed exceeding threshold collapses mission.
  * **REL-CAM-019.VERIF:** `PKG-AUTO` (`EchoesPossibilityBleedMissionTest`).
  * **REL-CAM-019.LANE:** Campaign.

* **REL-CAM-020 — Mission 15: "The Broken Sun" Contract:** M15 shall deliver the campaign finale at the core of the Broken Sun, holding the convergence point (320/280/240 ticks based on prior choices) and executing the final earned resolution.
  * **REL-CAM-020.AUTH:** Irreversible confirmation modal gates the final decision; ending sequence triggers authoritatively.
  * **REL-CAM-020.FAIL:** Core destroyed during final hold causes defeat.
  * **REL-CAM-020.VERIF:** `PKG-AUTO` (`EchoesBrokenSunMissionTest`).
  * **REL-CAM-020.LANE:** Campaign.

* **REL-CAM-021 — Campaign Objective Decoupling from Corefall:** Destroying the enemy Command Core in a campaign mission shall NOT automatically trigger mission failure when the primary objective is escort, hold, or witness (resolving C1).
  * **REL-CAM-021.AUTH:** In operations with non-Corefall objectives, destroying the enemy core either neutralizes enemy reinforcements or awards an optional objective.
  * **REL-CAM-021.FAIL:** Mission model treating `Outcome() != Ongoing` as a failure predicate is strictly eliminated.
  * **REL-CAM-021.VERIF:** `SRC` + `PKG-AUTO` (campaign mission completion logic audit).
  * **REL-CAM-021.LANE:** Campaign & Core Gameplay.

* **REL-CAM-033 — "Shattered Sun Conquest" Planetary Meta-Map:** The game shall include a non-linear single-player conquest mode staged across a 25-sector planetary map of Soryn, where players choose invasion paths to liberate or dominate the fractured world.
  * **REL-CAM-033.AUTH:** The meta-map tracks conquered territories, frontline supply connections, and enemy faction strongholds.
  * **REL-CAM-033.FAIL:** Progress failing to persist across conquest sessions fails campaign persistence.
  * **REL-CAM-033.VERIF:** `PKG-AUTO` (conquest meta-map state persistence test).
  * **REL-CAM-033.LANE:** Campaign & World.

* **REL-CAM-034 — Procedural Sector Modifiers & Environmental Anomalies:** Each conquest sector shall roll dynamic tactical modifiers: *Accelerated Well Bleed* (Wells tick 2x faster), *Seismic Fractures* (random ground tremors), *Atmospheric Scarcity* (-25% starting Matter), or *Solar Flare* (fog clears periodically).
  * **REL-CAM-034.AUTH:** Sector modifiers display in the pre-battle briefing and alter simulation parameters authoritatively.
  * **REL-CAM-034.FAIL:** Modifiers applying inconsistently across players or desyncing replays fails determinism.
  * **REL-CAM-034.VERIF:** `SRC` (sector modifier simulation injection tests).
  * **REL-CAM-034.LANE:** Core Gameplay & Campaign.

* **REL-CAM-035 — Persistent Faction Blueprint Research:** Conquering high-value technology sectors shall award persistent archetype blueprints: +10% Bulwark frontal shield, +15% Surveyor movement speed, or reduced Phase Anchor upkeep.
  * **REL-CAM-035.AUTH:** Blueprints remain active for the duration of the conquest campaign run.
  * **REL-CAM-035.FAIL:** Blueprint upgrades leaking into standard competitive skirmish is strictly prohibited.
  * **REL-CAM-035.VERIF:** `SRC` (blueprint upgrade scope isolation test).
  * **REL-CAM-035.LANE:** Campaign & Tech Design.

* **REL-CAM-036 — Conquest Roguelite Permadeath & Seed Sharing:** Losing a primary Command Core in a conquest territory battle inflicts territory loss; losing the faction home base ends the run. Each conquest campaign generates a shareable 8-character Seed.
  * **REL-CAM-036.AUTH:** Entering an identical Seed produces identical sector layout, modifiers, and AI faction placement.
  * **REL-CAM-036.FAIL:** Seeded runs producing different sector topologies fails procedural determinism.
  * **REL-CAM-036.VERIF:** `SRC` (conquest seed generation determinism test).
  * **REL-CAM-036.LANE:** Campaign & Build.

* **REL-CAM-037 — Dynamic AI Invasions & Territory Defense:** Between player turns, enemy AI factions shall execute counter-attacks against adjacent player territories, requiring the player to stage defensive hold operations or forfeit territory.
  * **REL-CAM-037.AUTH:** Defensive battles feature pre-built forward fortifications reflecting prior sector development.
  * **REL-CAM-037.FAIL:** Passive AI factions never attacking on the meta-map fails conquest tension.
  * **REL-CAM-037.VERIF:** `PKG-AUTO` (AI meta-map counter-attack cycle test).
  * **REL-CAM-037.LANE:** Opponent AI & Campaign.

* **REL-CAM-038 — Conquest Milestone Dossiers & Commemorative Unlocks:** Achieving conquest victory on Sovereign difficulty unlocks exclusive meta-rewards: commemorative title screen environments, complete faction codex records, and gilded unit heraldry accents.
  * **REL-CAM-038.AUTH:** Unlocks write permanently to `Profile.sav` and display in the Trophy Vault.
  * **REL-CAM-038.FAIL:** Unlocks granting competitive gameplay stats in multiplayer/skirmish is prohibited.
  * **REL-CAM-038.VERIF:** `PKG-AUTO` (conquest profile reward unlock test).
  * **REL-CAM-038.LANE:** Campaign & Player Experience.

### §15 Skirmish, AI, Difficulty, and Balance (`REL-AI-*`)

* **REL-AI-001 — Scoped PlayerView AI Authority Invariant:** Opponent AI controllers shall execute exclusively against the player-scoped `PlayerView` interface. Direct inspection of the omniscient `Simulation` object is strictly prohibited (resolving C7).
  * **REL-AI-001.AUTH:** Enemy entities in unexplored shroud or fog shall be redacted (health=0, coordinates zeroed, orders hidden).
  * **REL-AI-001.FAIL:** AI accessing unscouted player coordinates or queues is a fatal security defect.
  * **REL-AI-001.VERIF:** `SRC` (AI PlayerView enforcement test).
  * **REL-AI-001.LANE:** Opponent AI (`EchoesAIController`).

* **REL-AI-002 — Autonomous AI Economic Expansion:** The AI shall manage worker production, saturation of Matter deposits up to 2 workers per node, expansion to secondary resource sites, and construction of needed drop-offs.
  * **REL-AI-002.AUTH:** AI maintains steady worker scaling until reaching optimal economy saturation (16–24 workers).
  * **REL-AI-002.FAIL:** AI stalling on 5 starting workers fails basic economic competence.
  * **REL-AI-002.VERIF:** `PKG-AUTO` (AI economy saturation benchmark).
  * **REL-AI-002.LANE:** Opponent AI.

* **REL-AI-003 — Fair Reconnaissance & Scouting Cadence:** The AI shall recruit dedicated scout units (`Relay Skiff`, `Resonant`, `Afterimage`) and issue fair exploration missions to discover resource nodes and locate enemy bases.
  * **REL-AI-003.AUTH:** AI scouting routes navigate strictly through explored or frontier fog tiles.
  * **REL-AI-003.FAIL:** AI marching strike forces directly to unrevealed player bases without prior scouting fails acceptance.
  * **REL-AI-003.VERIF:** `PKG-AUTO` (AI fair scouting route test).
  * **REL-AI-003.LANE:** Opponent AI.

* **REL-AI-004 — Dynamic Army Composition & Archetype Replacement:** The AI shall field balanced unit compositions across Line, Heavy, and Support roles, actively replacing lost Heavies and building faction defense structures (resolving Phase 3 gap).
  * **REL-AI-004.AUTH:** The AI shall produce Bulwarks, Cairnbacks, or Wardens, and construct Aegis Posts or Listening Spines.
  * **REL-AI-004.FAIL:** AI producing only basic Soldiers and Workers for the entire match is strictly prohibited.
  * **REL-AI-004.VERIF:** `PKG-AUTO` (AI army composition diversity test).
  * **REL-AI-004.LANE:** Opponent AI.

* **REL-AI-005 — Defensive Reaction & Base Protection:** When its base or workers are attacked, the AI shall immediately switch to `DEFEND` state, pulling combat units to intercept the attackers and retreating threatened workers.
  * **REL-AI-005.AUTH:** Reaction time to base alerts scales with difficulty (Assisted: 2.0s; Standard: 1.0s; Sovereign: 0.2s).
  * **REL-AI-005.FAIL:** AI army ignoring base destruction alerts fails perceived intelligence.
  * **REL-AI-005.VERIF:** `PKG-AUTO` (AI defensive reaction benchmark).
  * **REL-AI-005.LANE:** Opponent AI.

* **REL-AI-006 — Strike Force Assembly & Coordinated Assault:** The AI shall assemble strike forces at designated staging points until reaching authored supply thresholds (e.g. 20 / 40 / 60 supply) before launching coordinated multi-unit assaults.
  * **REL-AI-006.AUTH:** Units advance in formation cohesion rather than streaming into enemy defenses one by one.
  * **REL-AI-006.FAIL:** Trickling single units into defensive fire fails tactical competence.
  * **REL-AI-006.VERIF:** `PKG-AUTO` (AI strike force assembly test).
  * **REL-AI-006.LANE:** Opponent AI.

* **REL-AI-007 — Combat Retreat & Force Preservation:** When a combat engagement is evaluated as decisively lost (>60% local force destroyed while enemy sustains <20% losses), the AI shall execute a tactical retreat to the nearest defensive line.
  * **REL-AI-007.AUTH:** Damaged units pull back while healthy units screen the retreat.
  * **REL-AI-007.FAIL:** AI fighting to the last man in every skirmish without retreating fails realism.
  * **REL-AI-007.VERIF:** `PKG-AUTO` (AI retreat evaluation test).
  * **REL-AI-007.LANE:** Opponent AI.

* **REL-AI-008 — Future Well Protocol Doctrinal Alignment:** The AI shall contest Future Wells based on its authored doctrine, correctly executing the required 300-tick capture and choosing appropriate protocols (resolving C27).
  * **REL-AI-008.AUTH:** AI assigns 1–2 workers with combat escort to capture dormant Wells when secure.
  * **REL-AI-008.FAIL:** Hardcoding all AI doctrines to blindly Harvest the Well is prohibited.
  * **REL-AI-008.VERIF:** `PKG-AUTO` (AI Well protocol decision audit).
  * **REL-AI-008.LANE:** Opponent AI & Factions.

* **REL-AI-009 — AI Doctrine: Warden (Defensive):** The Warden doctrine shall prioritize perimeter defense, heavy line units, early fortified structures (`Aegis Post`, `Listening Spine`), and the `Preserve` Well protocol for sustained advantage.
  * **REL-AI-009.AUTH:** Invests $\ge 30\%$ of resources in base defense; expands only when primary base is fortified.
  * **REL-AI-009.FAIL:** Warden doctrine executing reckless early rushes fails personality differentiation.
  * **REL-AI-009.VERIF:** `PKG-AUTO` (Warden behavioral profile test).
  * **REL-AI-009.LANE:** Opponent AI.

* **REL-AI-010 — AI Doctrine: Raider (Aggressive Skirmish):** The Raider doctrine shall prioritize fast mobile units (`Riftstalker`, `Afterimage`, `Lancer`), early raiding of enemy worker lines, and `Harvest` for immediate tempo windfalls.
  * **REL-AI-010.AUTH:** Initiates harassment attacks before tick 1,200 (1 minute); bypasses fortified defenses to target economy.
  * **REL-AI-010.FAIL:** Raider doctrine sitting passively in base fails personality.
  * **REL-AI-010.VERIF:** `PKG-AUTO` (Raider harassment cadence test).
  * **REL-AI-010.LANE:** Opponent AI.

* **REL-AI-011 — AI Doctrine: Steward (Economic Macro):** The Steward doctrine shall maximize worker growth, fast resource expansion to secondary deposits, and rapid technology acquisition before building a large endgame army.
  * **REL-AI-011.AUTH:** Expands to second deposit within 3,000 ticks; prioritizes resource monitor efficiency.
  * **REL-AI-011.FAIL:** Steward falling behind in economy fails design intent.
  * **REL-AI-011.VERIF:** `PKG-AUTO` (Steward macro expansion test).
  * **REL-AI-011.LANE:** Opponent AI.

* **REL-AI-012 — AI Doctrine: Expansionist (Territorial Control):** The Expansionist doctrine shall establish multi-base outposts across the map, securing chokepoints, controlling Future Wells, and choking enemy expansion lines.
  * **REL-AI-012.AUTH:** Builds forward waystations and actively denies neutral territory.
  * **REL-AI-012.FAIL:** Expansionist confining itself to primary base fails personality.
  * **REL-AI-012.VERIF:** `PKG-AUTO` (Expansionist territory occupation test).
  * **REL-AI-012.LANE:** Opponent AI.

* **REL-AI-013 — AI Doctrine: Adaptive (Counter-Play):** The Adaptive doctrine shall dynamically evaluate observed player army composition and tech choices, producing direct counters (e.g. screening Lancers against Bulwarks, deploying Resonants against cloaked movement).
  * **REL-AI-013.AUTH:** Re-evaluates army production weights every 600 ticks based on scouted composition.
  * **REL-AI-013.FAIL:** Static unadapting build orders fail the Adaptive mandate.
  * **REL-AI-013.VERIF:** `PKG-AUTO` (Adaptive doctrine tech counter test).
  * **REL-AI-013.LANE:** Opponent AI.

* **REL-AI-014 — AI Difficulty Tier: Assisted:** Assisted difficulty shall offer a gentle learning curve for novice players: +50% reaction delay (1.5s), APM ceiling capped at 30, and -20% combat damage multiplier disclosed in pre-match UI.
  * **REL-AI-014.AUTH:** AI gives visible telegraphs prior to launching attacks.
  * **REL-AI-014.FAIL:** Assisted AI executing high-speed micro-management fails accessibility.
  * **REL-AI-014.VERIF:** `PKG-AUTO` (Assisted APM and reaction delay test).
  * **REL-AI-014.LANE:** Opponent AI.

* **REL-AI-015 — AI Difficulty Tier: Standard:** Standard difficulty represents the baseline competitive AI: 100% fair information, zero resource cheats, reaction delay of 0.8s, and APM ceiling capped at 90.
  * **REL-AI-015.AUTH:** Economy and build times match human player identically.
  * **REL-AI-015.FAIL:** Hidden income or sight advantages in Standard mode are strictly prohibited.
  * **REL-AI-015.VERIF:** `PKG-AUTO` (Standard AI fairness assertion).
  * **REL-AI-015.LANE:** Opponent AI.

* **REL-AI-016 — Standard Matchup Competitive Balance Band:** Under Standard AI difficulty across 1,000 matches per cell, no non-mirror matchup shall fall outside a 40.0%–60.0% win-rate window, and no start position shall confer >5.0% advantage (binding baseline).
  * **REL-AI-016.AUTH:** Measured matrix: Meridian vs. Kharuun, Meridian vs. Choir, Kharuun vs. Choir.
  * **REL-AI-016.FAIL:** Any faction sitting at <40% or >60% win rate halts release.
  * **REL-AI-016.VERIF:** `PKG-AUTO` (1,000-match automated balance validation suite).
  * **REL-AI-016.LANE:** Opponent AI & Faction Design.

* **REL-AI-017 — AI Difficulty Tier: Challenging:** Challenging difficulty shall provide tight tactical execution: reaction delay 0.4s, APM ceiling 140, disciplined focus-firing, and optimal expansion timings without economic cheats.
  * **REL-AI-017.AUTH:** Defeating Challenging AI requires solid player macro-economy and tactical control.
  * **REL-AI-017.FAIL:** Cheating vision or free units in Challenging mode fails fair AI rules.
  * **REL-AI-017.VERIF:** `PKG-AUTO` (Challenging AI execution metrics).
  * **REL-AI-017.LANE:** Opponent AI.

* **REL-AI-018 — AI Difficulty Tier: Sovereign:** Sovereign difficulty represents the master-level AI: reaction delay 0.15s, APM ceiling 180 (human reasonable), optimal micro-management, flanking coordination, and ruthless target prioritization.
  * **REL-AI-018.AUTH:** Sovereign AI achieves high performance through algorithm efficiency, not artificial health/damage buffs.
  * **REL-AI-018.FAIL:** Unlimited inhuman APM (>300) is strictly prohibited.
  * **REL-AI-018.VERIF:** `PKG-AUTO` (Sovereign APM ceiling and fairness test).
  * **REL-AI-018.LANE:** Opponent AI.

* **REL-AI-019 — AI Concession & Elimination Protocol:** The AI shall concede a match only when its Command Core is destroyed or all workers and production facilities are lost with 0 resources and no army remaining.
  * **REL-AI-019.AUTH:** When conditions are met, the AI transmits a concession message (`[OPPONENT CONCEDES]`) and surrenders, avoiding pointless stall.
  * **REL-AI-019.FAIL:** AI hiding single workers in map corners to delay loss fails usability.
  * **REL-AI-019.VERIF:** `SRC` (AI concession evaluation test).
  * **REL-AI-019.LANE:** Opponent AI.

* **REL-AI-020 — Skirmish Mirror Matchup Support:** Skirmish setup shall fully support mirror matchups (Meridian vs. Meridian, Kharuun vs. Kharuun, Choir vs. Choir), validating that all 9 matchup combinations run cleanly without assertion failure (resolving C10).
  * **REL-AI-020.AUTH:** Mirror matches display distinct team ownership accents and allow identical faction mechanics on both sides.
  * **REL-AI-020.FAIL:** Setup cycler skipping opponent's faction or throwing `[SKIRMISH_MATCHUP_INVALID]` is a critical defect.
  * **REL-AI-020.VERIF:** `SRC` + `PKG-AUTO` (`EchoesSkirmishSetupTest`).
  * **REL-AI-020.LANE:** Opponent AI & Skirmish.

* **REL-AI-021 — Elimination of Undocumented AI Doctrines:** The undocumented `BALANCED` AI option in the skirmish selector shall be completely removed or formalized into a distinct, authored 6th doctrine with full decision tables (resolving C27).
  * **REL-AI-021.AUTH:** Selector exposes only verified, authored doctrines: Warden, Raider, Steward, Expansionist, Adaptive.
  * **REL-AI-021.FAIL:** Shipping broken or placeholder AI options fails QA review.
  * **REL-AI-021.VERIF:** `SRC` (AI doctrine enumeration and selector audit).
  * **REL-AI-021.LANE:** Opponent AI & Player Experience.

* **REL-AI-037 — Minimap Tactical Ping Communication:** Players shall issue tactical pings by holding `Alt` and clicking the terrain or minimap, selecting from a radial menu: *Attack Target*, *Defend Position*, *Capture Future Well*, *Need Resources*.
  * **REL-AI-037.AUTH:** Pings generate a spatial ground beacon, minimap pulse wave, and distinct audio notification.
  * **REL-AI-037.FAIL:** Pings failing to transmit spatial coordinates accurately fails tactical communication.
  * **REL-AI-037.VERIF:** `PKG-PHYS` (minimap ping radial menu test).
  * **REL-AI-037.LANE:** Player Experience (`EchoesHUD`).

* **REL-AI-038 — Friendly AI Ally Comprehension & Force Dispatch:** In cooperative skirmish matches (2v2, 3v3), friendly AI allies shall evaluate player pings within 1.0 second, acknowledge via voice/text, and dispatch up to 50% of active mobile forces to assist.
  * **REL-AI-038.AUTH:** AI responds: `[ACKNOWLEDGED: MOVING TO SUPPORT WELL]` or `[UNABLE TO COMPLY: UNDER ATTACK]`.
  * **REL-AI-038.FAIL:** AI ally completely ignoring player pings fails cooperative teamplay.
  * **REL-AI-038.VERIF:** `PKG-AUTO` (AI ally ping response benchmark).
  * **REL-AI-038.LANE:** Opponent AI.

* **REL-AI-039 — Cooperative Resource Tribute & Request System:** Team skirmish players shall open a Diplomacy panel to gift or request Matter and Dawn to allied human or AI players.
  * **REL-AI-039.AUTH:** Friendly AI allies with surplus economy (>1,000 Matter) will grant requested aid within 5 seconds.
  * **REL-AI-039.FAIL:** Resource tribute transactions dropping funds in transit fails financial integrity.
  * **REL-AI-039.VERIF:** `SRC` (resource tribute transaction test).
  * **REL-AI-039.LANE:** Core Gameplay & Opponent AI.

* **REL-AI-040 — Cooperative Comp-Stomp Skirmish Presets:** The skirmish lobby shall offer quick-launch presets for classic cooperative play: *2 Players vs 2 AI*, *3 Players vs 3 AI*, and *Insane Bot Survival*, pre-configuring balanced alliances and map layouts.
  * **REL-AI-040.AUTH:** Enables fast frictionless setup for friends and families playing against bots.
  * **REL-AI-040.FAIL:** Broken team alliances allowing friendly fire between allies fails preset setup.
  * **REL-AI-040.VERIF:** `PKG-PHYS` (cooperative skirmish preset test).
  * **REL-AI-040.LANE:** Skirmish & Player Experience.

---

### §16 Replays and Quality-of-Life (`REL-QOL-*`)

* **REL-QOL-001 — Control Group Assignment & Selection:** Players shall assign selected units to control groups 1 through 9 using `Ctrl + [1–9]`, recall them with `[1–9]`, and append to them using `Shift + [1–9]`.
  * **REL-QOL-001.AUTH:** Dead units are purged from control groups automatically within 1 tick; group membership renders as numbers above unit health bars.
  * **REL-QOL-001.FAIL:** Recalling destroyed units or losing group assignments fails acceptance.
  * **REL-QOL-001.VERIF:** `PKG-PHYS` (control group assignment and recall sweep).
  * **REL-QOL-001.LANE:** Player Experience (`EchoesPlayerController`).

* **REL-QOL-002 — Control Group Camera Centering:** Double-tapping a control group hotkey (`[1–9]` twice within 300 ms) shall immediately center the camera viewport on the group's centroid.
  * **REL-QOL-002.AUTH:** Camera smoothly interpolates to the target coordinates in ≤250 ms.
  * **REL-QOL-002.FAIL:** Failed double-tap detection or erratic camera snaps fails acceptance.
  * **REL-QOL-002.VERIF:** `PKG-PHYS` (double-tap camera centering test).
  * **REL-QOL-002.LANE:** Player Experience.

* **REL-QOL-003 — Multi-Unit Subgroup Navigation:** When a mixed-selection group contains multiple unit types, the HUD command card shall organize units by combat hierarchy. Pressing `Tab` cycles focus to the next subgroup, and `Shift + Tab` cycles backward.
  * **REL-QOL-003.AUTH:** Cycling focus updates the active ability grid without deselecting the broader group.
  * **REL-QOL-003.FAIL:** Tab cycling dropping the selection group fails RTS usability standards.
  * **REL-QOL-003.VERIF:** `PKG-PHYS` (subgroup tab cycling test).
  * **REL-QOL-003.LANE:** Player Experience.

* **REL-QOL-004 — Structure Rally Point Management:** Right-clicking any ground or resource coordinate while a production structure is selected shall establish a Rally Point, indicated by a ground flag and connecting vector.
  * **REL-QOL-004.AUTH:** Newly produced units path directly to the rally point upon emergence. Rallying to a Matter node automatically begins gathering.
  * **REL-QOL-004.FAIL:** Rally flag failing to route units or getting lost across saves fails acceptance.
  * **REL-QOL-004.VERIF:** `PKG-AUTO` (rally point route and save/restore test).
  * **REL-QOL-004.LANE:** Player Experience & Core Gameplay.

* **REL-QOL-005 — Queue Cancellation & Reordering:** Players shall cancel queued units or technologies by right-clicking their icon in the production queue or clicking an explicit `[X]` button.
  * **REL-QOL-005.AUTH:** Items can be drag-reordered to prioritize urgent units; cancellation refunds resources per §10 rules.
  * **REL-QOL-005.FAIL:** Queue reordering corrupting memory or dropping items fails stability.
  * **REL-QOL-005.VERIF:** `PKG-PHYS` (queue reorder and cancel interaction test).
  * **REL-QOL-005.LANE:** Player Experience.

* **REL-QOL-006 — Idle Worker Cycling Hotkey (`F1`):** Pressing `F1` shall select and center the camera on the nearest idle worker. Repeated presses cycle sequentially through all currently idle workers.
  * **REL-QOL-006.AUTH:** The HUD displays an idle worker counter icon; clicking the icon triggers the same cycle action.
  * **REL-QOL-006.FAIL:** Selecting working or harvesting units as idle fails acceptance.
  * **REL-QOL-006.VERIF:** `PKG-PHYS` (idle worker selection cycle test).
  * **REL-QOL-006.LANE:** Player Experience.

* **REL-QOL-007 — Production Facility Cycling Hotkey (`F2`):** Pressing `F2` shall select and cycle through all owned completed unit production structures (`Array Foundry`, `Growth Basin`, `Chorus Loom`).
  * **REL-QOL-007.AUTH:** Enables rapid macro-production without manually moving the camera to base.
  * **REL-QOL-007.FAIL:** Incomplete structures or destroyed facilities entering the cycle fails validation.
  * **REL-QOL-007.VERIF:** `PKG-PHYS` (production facility cycle test).
  * **REL-QOL-007.LANE:** Player Experience.

* **REL-QOL-008 — Combat Army Selection Hotkey (`F3`):** Pressing `F3` shall select all controllable combat units on the map (excluding workers), assembling them into an immediate active selection.
  * **REL-QOL-008.AUTH:** Workers, structures, and non-combat scouts are excluded from the all-army selection.
  * **REL-QOL-008.FAIL:** Pulling workers into all-army selection fails standard RTS control rules.
  * **REL-QOL-008.VERIF:** `PKG-PHYS` (all-army selection test).
  * **REL-QOL-008.LANE:** Player Experience.

* **REL-QOL-009 — Spatial Alert Feed & Jump Hotkey (`Space`):** Critical tactical events (attacks, structure losses, Well telegraphs) shall post an entry to the spatial alert feed and minimap. Pressing `Space` shall instantly jump the camera to the newest alert location.
  * **REL-QOL-009.AUTH:** Pressing `Space` repeatedly cycles backward through the last 5 alerts within 10 seconds.
  * **REL-QOL-009.FAIL:** Key conflict between Space jump-to-alert and targeting confirmation is strictly eliminated (resolving C34).
  * **REL-QOL-009.VERIF:** `PKG-PHYS` (alert feed and Space jump test).
  * **REL-QOL-009.LANE:** Player Experience.

* **REL-QOL-010 — Replay Recording & Metadata Browser:** Completed skirmish and campaign operations shall serialize an authoritative replay file to disk containing match metadata: date, map, duration, faction pairing, outcome, and final checksum.
  * **REL-QOL-010.AUTH:** The in-game Replay Browser shall list all saved replays, allowing filtering by map and date.
  * **REL-QOL-010.FAIL:** Failing to write replay file to disk or missing browser UI fails release requirements.
  * **REL-QOL-010.VERIF:** `PKG-AUTO` + `PKG-REND` (replay file generation and browser review).
  * **REL-QOL-010.LANE:** Campaign, Core & UI.

* **REL-QOL-011 — Replay Playback Transport Controls:** The replay player shall provide full transport controls: Play, Pause, Speed adjustment (0.5x, 1x, 2x, 4x, 8x), and Tick-by-Tick Step forward.
  * **REL-QOL-011.AUTH:** Fog-of-war view toggles shall support: Player 1 perspective, Player 2 perspective, and Omniscient Observer.
  * **REL-QOL-011.FAIL:** Desync or crash during replay fast-forward fails determinism.
  * **REL-QOL-011.VERIF:** `PKG-AUTO` (replay fast-forward and step playback test).
  * **REL-QOL-011.LANE:** Core Gameplay & Player Experience.

* **REL-QOL-012 — Anti-Rewrite Campaign Replay Protection:** Replaying an earlier campaign operation from the replay browser or mission selector shall NOT overwrite or invalidate the authoritative campaign progression ledger (resolving CAM-005).
  * **REL-QOL-012.AUTH:** Rerunning M01 with an alternate Well decision returns `ReplayConflict` and preserves the active campaign history intact.
  * **REL-QOL-012.FAIL:** Silently rewriting completed campaign ledger state from an experimental replay is prohibited.
  * **REL-QOL-012.VERIF:** `SRC` (`EchoesCampaignProgressTest` replay conflict check).
  * **REL-QOL-012.LANE:** Campaign & Save Recovery.

* **REL-QOL-013 — "Take Command" Savestate Replay Branching:** The replay viewer shall incorporate a **"Take Command"** interface button. Clicking Take Command at any timestamp shall fork the session into a live, fully playable match against the recorded opponent inputs or an active AI controller.
  * **REL-QOL-013.AUTH:** State forks seamlessly within ≤500 ms, preserving exact unit positions, health, fog, and resource counts.
  * **REL-QOL-013.FAIL:** Checksum corruption or desync upon branching from a replay fails savestate integrity.
  * **REL-QOL-013.VERIF:** `PKG-AUTO` (replay fork and take-command execution test).
  * **REL-QOL-013.LANE:** Core Gameplay & Save Recovery.

* **REL-QOL-014 — Replay Bookmark & Event Timeline Navigation:** Replay files shall serialize an authoritative event timeline marking: First Combat Contact, Future Well Protocols, Command Core Loss, and Major Army Clashes.
  * **REL-QOL-014.AUTH:** Clicking any bookmark on the timeline scrub bar jumps playback directly to that simulation tick.
  * **REL-QOL-014.FAIL:** Jumping to a bookmark causing visual desync or frozen entities fails timeline seeking.
  * **REL-QOL-014.VERIF:** `PKG-PHYS` (replay timeline bookmark seeking test).
  * **REL-QOL-014.LANE:** Player Experience (`EchoesHUD`).

* **REL-QOL-015 — Advanced Esports / Spectator Observer Deck:** The observer interface shall provide broadcast-grade analytical overlays: Live Income Velocity Curves, Global Army Value Comparison, Production Ticker Bar, Unit Kill/Loss Tallies, and Player Perspective Fog Toggles.
  * **REL-QOL-015.AUTH:** Toggleable with hotkeys `Ctrl+I` (Income), `Ctrl+P` (Production), `Ctrl+A` (Army), `Ctrl+V` (Vision).
  * **REL-QOL-015.FAIL:** Observer UI obscuring tactical battlefield action fails broadcast readability.
  * **REL-QOL-015.VERIF:** `PKG-REND` (spectator overlay visual review).
  * **REL-QOL-015.LANE:** Player Experience.

* **REL-QOL-016 — Cinematic Smooth Camera & Freecam Spectating:** Spectators and content creators shall have access to a smooth freecam mode featuring damped inertia panning, ground-following collision avoidance, field-of-view zoom sliders, and a single-key HUD hide toggle (`Ctrl+Z`).
  * **REL-QOL-016.AUTH:** Enables capture of cinematic battle footage without jerky step transitions.
  * **REL-QOL-016.FAIL:** Freecam clipping under terrain or breaking viewport frustum culling fails camera criteria.
  * **REL-QOL-016.VERIF:** `PKG-PHYS` (cinematic freecam navigation test).
  * **REL-QOL-016.LANE:** Player Experience & Cinematics.

---

### §17 UI and Interaction (`REL-UI-*`)

* **REL-UI-001 — Production UMG/Slate Widget Framework:** The user interface shall be constructed using modern Unreal Engine UMG and Slate widget hierarchies, completely replacing prototype Canvas immediate-mode `DrawText` rendering.
  * **REL-UI-001.AUTH:** All UI elements (HUD, minimap, command card, dialogue boxes) shall exist as modular UMG widgets.
  * **REL-UI-001.FAIL:** Any leftover Canvas `DrawText` HUD rendering in Shipping builds is prohibited.
  * **REL-UI-001.VERIF:** `SRC` (UMG widget migration audit).
  * **REL-UI-001.LANE:** Player Experience (`EchoesHUD`).

* **REL-UI-002 — Command Card 3x3 Grid Ergonomics:** Unit actions, stances, abilities, and build options shall be laid out in a standardized 3x3 command card in the lower right viewport, bound to an ergonomic QWE / ASD / ZXC layout.
  * **REL-UI-002.AUTH:** Position 1: Move; Position 2: Stop; Position 3: Hold; Position 4: Attack; Position 5: Patrol; Position 6: Stance; Position 7–9: Faction abilities.
  * **REL-UI-002.FAIL:** Inconsistent button layouts between unit classes fails ergonomics.
  * **REL-UI-002.VERIF:** `PKG-REND` (command card layout inspection).
  * **REL-UI-002.LANE:** Player Experience.

* **REL-UI-003 — Comprehensive Selection Inspector Panel:** The bottom-center selection inspector shall display: entity portrait, faction non-color mark, unit archetype, current health/shields with numeric values, cargo contents, active order status, and damage/armor stats.
  * **REL-UI-003.AUTH:** Multi-unit selection displays an array of unit icons grouped by type with count badges.
  * **REL-UI-003.FAIL:** Single-line selection panel lacking health numbers or cargo indicators is prohibited (resolving gap).
  * **REL-UI-003.VERIF:** `PKG-REND` (selection inspector detail review).
  * **REL-UI-003.LANE:** Player Experience.

* **REL-UI-004 — Upper Resource Telemetry Deck:** The top-left HUD resource deck shall render real-time reserves of Matter, Dawn, and current/maximum Logistics, alongside trailing 30s/60s income velocity indicators.
  * **REL-UI-004.AUTH:** Hovering over resource numbers exposes tooltips detailing active worker distribution and upcoming coherence expenses.
  * **REL-UI-004.FAIL:** Misaligned fonts or clipped resource numbers at non-1080p resolutions fails validation.
  * **REL-UI-004.VERIF:** `PKG-REND` (resource bar UI review).
  * **REL-UI-004.LANE:** Player Experience.

* **REL-UI-005 — Tactical Minimap Presentation & Frustum:** The bottom-left minimap shall render terrain contours, resource node positions, friendly units (cyan/amber), telegraphed Well pings, explored fog boundaries, and camera viewport frustum rectangle.
  * **REL-UI-005.AUTH:** Left-clicking or dragging on the minimap pans the main camera viewport immediately. Right-clicking issues move/attack orders to that map location.
  * **REL-UI-005.FAIL:** Minimap disclosing units concealed by fog of war fails fair information rules.
  * **REL-UI-005.VERIF:** `PKG-PHYS` (minimap click-navigation and fog masking test).
  * **REL-UI-005.LANE:** Player Experience & Visual.

* **REL-UI-006 — Spatial Alert Feed & Banner Notifications:** Tactical alerts shall render in a scrolling left-hand feed (up to 4 active entries) with color-coded and shape-coded icons, accompanied by a brief top-screen notification banner.
  * **REL-UI-006.AUTH:** Alerts decay and fade out after 8.0 seconds if not clicked.
  * **REL-UI-006.FAIL:** Alerts stacking off-screen or occluding the resource deck fails layout criteria.
  * **REL-UI-006.VERIF:** `PKG-REND` (alert feed animation and layout review).
  * **REL-UI-006.LANE:** Player Experience.

* **REL-UI-007 — Interactive Technology Archive Tree:** Players shall access an in-game Technology Archive (`T`) displaying the faction tech tree, researched upgrades, costs, production times, and numerical unit modifiers.
  * **REL-UI-007.AUTH:** Locked technologies display explicit facility and prerequisite requirements.
  * **REL-UI-007.FAIL:** Missing tooltips or truncated upgrade descriptions fails usability.
  * **REL-UI-007.VERIF:** `PKG-PHYS` (tech tree interface navigation test).
  * **REL-UI-007.LANE:** Player Experience.

* **REL-UI-008 — Operational Escape Pause Menu:** Pressing `Escape` or `Pause` shall open a translucent modal pause menu with: Resume, Options, Restart Mission, Concede Match, and Exit to Menu.
  * **REL-UI-008.AUTH:** Single-player simulation halts completely while paused; audio submix shifts to low-pass pause bus.
  * **REL-UI-008.FAIL:** Unpauseable single-player matches fail accessibility standards.
  * **REL-UI-008.VERIF:** `PKG-PHYS` (pause modal and simulation freeze test).
  * **REL-UI-008.LANE:** Player Experience.

* **REL-UI-009 — Post-Match Results & Statistics Dossier:** Match conclusion (Victory/Defeat) shall present a structured results dossier detailing: match duration, units trained/lost, resources harvested, Well decisions, APM graph, and timeline curves.
  * **REL-UI-009.AUTH:** Buttons provide options to: View Replay, Rematch, Restart, or Return to Menu.
  * **REL-UI-009.FAIL:** Generic single-line failure text discarding match statistics is strictly prohibited (resolving C1).
  * **REL-UI-009.VERIF:** `PKG-REND` (results screen statistics review).
  * **REL-UI-009.LANE:** Player Experience & Campaign.

* **REL-UI-010 — Campaign Mission Briefing & Directive Terminal:** Campaign operations shall open with an authored briefing terminal displaying mission map topography, primary and optional objectives, dialogue portraits, and irreversible choice warnings.
  * **REL-UI-010.AUTH:** The player clicks `DEPLOY` to enter the tactical map once ready.
  * **REL-UI-010.FAIL:** Dropping players into a campaign operation without briefing terminal fails narrative onboarding.
  * **REL-UI-010.VERIF:** `PKG-REND` (briefing modal review).
  * **REL-UI-010.LANE:** Campaign & UI.

* **REL-UI-011 — Context-Sensitive Mouse Cursor State Machine:** The hardware/software mouse cursor shall dynamically indicate context: Default Pointer, Friendly Selection, Enemy Attack Reticle, Gather Pickaxe, Build Blueprint, Invalid Cross, and Minimap Radar.
  * **REL-UI-011.AUTH:** Cursor state updates within 1 frame of mouse movement over interactive targets.
  * **REL-UI-011.FAIL:** Cursor remaining static arrow over enemy targets fails game feel.
  * **REL-UI-011.VERIF:** `PKG-REND` (cursor state transition verification).
  * **REL-UI-011.LANE:** Player Experience.

* **REL-UI-012 — Ground Selection Ring Visual Hierarchy:** Selected entities shall project a ground-projected decal ring matching faction identity (Meridian: cyan bracket; Kharuun: amber faceted; Choir: magenta concentric).
  * **REL-UI-012.AUTH:** Health and energy bars hover above unit silhouettes, scaling with camera zoom distance.
  * **REL-UI-012.FAIL:** Selection rings floating in air or clipping under terrain fails presentation.
  * **REL-UI-012.VERIF:** `PKG-REND` (selection decal visual review).
  * **REL-UI-012.LANE:** Visual Presentation.

* **REL-UI-013 — Comprehensive Screen Resolution Matrix:** The user interface layout shall function flawlessly without text clipping, overlap, or distortion across the entire binding resolution profile: 1280×720, 1440×900, 1600×900, 1920×1080, 2560×1440, baseline native Retina, Windowed, Borderless, and Fullscreen modes.
  * **REL-UI-013.AUTH:** Live window resizing shall dynamically recalculate widget anchor geometry without requiring game restart.
  * **REL-UI-013.FAIL:** HUD widgets overlapping or cutting off text at 1440x900 or 2560x1440 fails acceptance.
  * **REL-UI-013.VERIF:** `PKG-REND` (resolution matrix visual audit across 7 profiles).
  * **REL-UI-013.LANE:** Player Experience.

* **REL-UI-014 — Calibrated UI Scale Dynamic Range:** The UI Scale slider in Settings shall support continuous scaling from 80% to 150% (0.8x to 1.5x), correcting the prior 85%–135% clamp defect (resolving C35).
  * **REL-UI-014.AUTH:** At 150% scale on 1080p, no HUD text or button shall overlap neighboring widgets.
  * **REL-UI-014.FAIL:** Clamping UI scale to 85%–135% violates the binding standard.
  * **REL-UI-014.VERIF:** `PKG-REND` (UI scale sweep at 80%, 100%, 120%, 150%).
  * **REL-UI-014.LANE:** Player Experience.

* **REL-UI-015 — Input Keybind Deconfliction Invariant:** The default input configuration shall strictly eliminate overlapping key conflicts (resolving C34):
  * `Space`: Jump to alert (view mode) / Confirm target (targeting mode).
  * `C`: Unit stance cycle (combat) / Formation cycle.
  * `Tab` / `Shift+Tab`: Subgroup navigation.
  * `A`/`S`/`H`/`P`/`G`: Attack-move, Stop, Hold, Patrol, Guard.
  * **REL-UI-015.AUTH:** Every action has an exclusive primary key without ambiguous modal collisions.
  * **REL-UI-015.FAIL:** Single key triggering two competing actions in the same context fails acceptance.
  * **REL-UI-015.VERIF:** `SRC` + `PKG-PHYS` (keybinding collision audit and physical test).
  * **REL-UI-015.LANE:** Player Experience (`DefaultInput.ini`).

* **REL-UI-016 — Layout Safety Margins for Localization:** All Slate and UMG text containers shall maintain an authored 30% spatial expansion safety margin to accommodate font scaling and future multi-language localization without text truncation.
  * **REL-UI-016.AUTH:** Briefings and tooltips wrap text cleanly, avoiding overflow clipping.
  * **REL-UI-016.FAIL:** Hardcoded pixel-width text boxes causing truncated strings fails UI standards.
  * **REL-UI-016.VERIF:** `SRC` (widget bounding box layout audit).
  * **REL-UI-016.LANE:** Player Experience.

### §18 World Art, Units, Structures, Animation, and VFX (`REL-ART-*`)

* **REL-ART-001 — Five-Color Aesthetic Palette Enforcement:** All visual assets shall strictly adhere to the five core palette tones: Charcoal (vitrified ground, primary mass), Pale Ceramic (civic and Compact plates), Broken-Sun Amber (Kharuun grown mineral, emissive seams), Magenta Fracture (Crownfall anomalies, Choir phase), and Cyan (Matter crystals, Meridian conduits, order confirmation).
  * **REL-ART-001.AUTH:** Off-palette or saturated primary colors (e.g. bright red, lime green) are strictly prohibited.
  * **REL-ART-001.FAIL:** Assets deviating from the five-color palette fail visual review.
  * **REL-ART-001.VERIF:** `PKG-REND` (palette compliance frame inspection).
  * **REL-ART-001.LANE:** Visual Presentation.

* **REL-ART-002 — One-Second Tactical Readability Invariant:** At default RTS camera arm length (3,800 uu, 45-degree isometric tilt), unit ownership, archetype role, health state, and active stance shall be recognizable within $\le 1.0	ext{ second}$ under full combat load.
  * **REL-ART-002.AUTH:** Unit silhouettes shall be distinct in black-and-white value inspection.
  * **REL-ART-002.FAIL:** Indistinguishable unit silhouettes causing tactical misidentification fail acceptance.
  * **REL-ART-002.VERIF:** `HUM` + `PKG-REND` (readability perceptual test).
  * **REL-ART-002.LANE:** Visual Presentation & Canon.

* **REL-ART-003 — Matte Terrain Surface Anti-Glint Specification:** World terrain materials (`M_EchoesWorldSurface`) shall maintain a strict roughness floor of $\ge 0.85$ and a luma wash of $152.6 \pm 5$ linear points, eliminating specular glare and high-frequency noise.
  * **REL-ART-003.AUTH:** Tactical units and projectiles stand out cleanly against the ground plane.
  * **REL-ART-003.FAIL:** Specular glare competing with unit visibility is a critical defect.
  * **REL-ART-003.VERIF:** `SRC` + `PKG-REND` (material parameter audit and capture review).
  * **REL-ART-003.LANE:** Visual Presentation.

* **REL-ART-004 — Emissive Accent Surface Area Ceiling:** Faction-specific emissive accent regions (Meridian Cyan, Kharuun Amber, Choir Magenta) shall occupy no more than 15.0% of any mesh's visible on-screen surface area.
  * **REL-ART-004.AUTH:** Emissive materials provide readable identification without blooming or obscuring adjacent units.
  * **REL-ART-004.FAIL:** Over-blooming emissive shaders blinding the viewport fails visual criteria.
  * **REL-ART-004.VERIF:** `PKG-REND` (emissive surface histogram audit).
  * **REL-ART-004.LANE:** Visual Presentation.

* **REL-ART-005 — 24 Production Mesh Silhouettes & LODs:** All 12 units and 12 structures shall feature finished production meshes with authored LOD0 and LOD1 models, eliminating all Engine primitive stand-ins and placeholder boxes.
  * **REL-ART-005.AUTH:** LOD transitions shall occur smoothly at camera zoom thresholds without popping.
  * **REL-ART-005.FAIL:** Grey-box or placeholder meshes in packaged builds fails release gates.
  * **REL-ART-005.VERIF:** `PKG-REND` (roster mesh inspection).
  * **REL-ART-005.LANE:** Visual Presentation.

* **REL-ART-006 — Meridian Compact Engineered Form Language:** Meridian structures and units shall communicate engineered load paths: orthogonal frames, heavy machined plates, structural rails, exposed conduit lines, and functional status bands.
  * **REL-ART-006.AUTH:** Units look industrial, disciplined, and maintained under strain.
  * **REL-ART-006.FAIL:** Curved organic or mystical styling on Meridian assets fails design language.
  * **REL-ART-006.VERIF:** `PKG-REND` (Meridian asset review).
  * **REL-ART-006.LANE:** Visual Presentation.

* **REL-ART-007 — Kharuun Assemblies Grown Mineral Architecture:** Kharuun structures and units shall communicate grown mineral-organic form: faceted basalt strata, hexagonal columns, translucent amber nodules, and living root anchors.
  * **REL-ART-007.AUTH:** Structures appear grown out of Soryn's crust, maintained and communal, never primitive.
  * **REL-ART-007.FAIL:** Machined metallic panels or industrial bolts on Kharuun assets fails design rules.
  * **REL-ART-007.VERIF:** `PKG-REND` (Kharuun asset review).
  * **REL-ART-007.LANE:** Visual Presentation.

* **REL-ART-008 — Hollow Choir Possibility Superposition Aesthetics:** Hollow Choir structures and units shall communicate quantum superposition: repeated luminous edges, offset duplicate geometry, structures with two conflicting shadows, and shimmering light lattices.
  * **REL-ART-008.AUTH:** Choir entities appear as possibilities asserting reality rather than solid metal or stone.
  * **REL-ART-008.FAIL:** Standard solid opaque armor plates on Choir units fails visual canon.
  * **REL-ART-008.VERIF:** `PKG-REND` (Choir asset review).
  * **REL-ART-008.LANE:** Visual Presentation.

* **REL-ART-009 — Code-Driven Kinematic Motion Invariant:** Mobile units shall feature code-driven kinematic motion components synchronized to authoritative simulation velocity: wheel/tread rotations, leg strides, hover bobbing, and turret tracking.
  * **REL-ART-009.AUTH:** Motion components are presentation-only, reading sim velocity and writing nothing back.
  * **REL-ART-009.FAIL:** Rigid sliding boxes moving across the ground without motion fail release quality.
  * **REL-ART-009.VERIF:** `PKG-REND` (unit movement animation inspection).
  * **REL-ART-009.LANE:** Animation & Visual.

* **REL-ART-010 — Bulwark Team Deploy Transformation Rig:** The Bulwark Team shall feature a dedicated mechanical transformation: uncoupling lateral armor plates to form an interlocking frontal blast barrier during Deploy (20 ticks).
  * **REL-ART-010.AUTH:** Packing (15 ticks) folds plates back onto the carriage chassis.
  * **REL-ART-010.FAIL:** Instant pop between deployed and packed states fails visual quality.
  * **REL-ART-010.VERIF:** `PKG-REND` (Bulwark deployment animation review).
  * **REL-ART-010.LANE:** Animation & Visual.

* **REL-ART-011 — Kharuun Waystone Rooting & Uproot Animation:** Waystones shall display an authored metamorphosis: mineral roots retracting into the base slab during Uproot (40 ticks), and extending deep into the ground during Root (60 ticks).
  * **REL-ART-011.AUTH:** Dust and fractured stone particles emit during the rooting phase.
  * **REL-ART-011.FAIL:** Floating or sliding rooted Waystones fails realism.
  * **REL-ART-011.VERIF:** `PKG-REND` (Waystone transformation review).
  * **REL-ART-011.LANE:** Animation & Visual.

* **REL-ART-012 — Distinct Weapon Projectile & Muzzle VFX:** Every weapon archetype shall possess distinct visual effects: Lancer cyan rail discharges; Bulwark explosive concussions; Riftstalker amber shard spikes; Intervalist phase lances; Aegis Post heavy blue beams.
  * **REL-ART-012.AUTH:** Effects disable collision, overlap, and navigation influence.
  * **REL-ART-012.FAIL:** Invisible weapon fire or generic placeholder tracers fail visual combat criteria.
  * **REL-ART-012.VERIF:** `PKG-REND` (weapon VFX review).
  * **REL-ART-012.LANE:** VFX & Visual.

* **REL-ART-013 — Structural Collapse & Shatter Destruction VFX:** Building destruction shall trigger bespoke shatter and collapse effects matching material truth: Meridian metal shearing and collapsing; Kharuun stone fracturing into basalt blocks; Choir structures dissolving into fading light interference.
  * **REL-ART-013.AUTH:** Destruction debris remains visible for 200 ticks before clean fade out.
  * **REL-ART-013.FAIL:** Buildings vanishing instantly into thin air upon death is prohibited.
  * **REL-ART-013.VERIF:** `PKG-REND` (destruction VFX inspection).
  * **REL-ART-013.LANE:** VFX & Visual.

* **REL-ART-014 — Future Well Landmark 4-State Visual Manifest:** The Future Well landmark shall visually communicate all four states: (1) Dormant: low amber/magenta fracture glow; (2) Harvest: intense vertical cyan energy geyser; (3) Preserve: pulsed radar concentric ring waves; (4) Reshape: fractured spatial rift tearing open.
  * **REL-ART-014.AUTH:** States are instantly readable at normal camera zoom.
  * **REL-ART-014.FAIL:** Well state indiscernible without clicking fails situational awareness.
  * **REL-ART-014.VERIF:** `PKG-REND` (Future Well visual states review).
  * **REL-ART-014.LANE:** Visual Presentation & VFX.

* **REL-ART-015 — Site Dressing & Environmental Coherence:** All campaign mission sites and the Glass Scar skirmish map shall feature dressed environmental terrain kits (Ash Cut, Buried Causeway, Folded Verge), completely eliminating bare flat collision floors.
  * **REL-ART-015.AUTH:** Dressing includes basalt strata, vitrified glass shards, civic conduits, and shivergrass.
  * **REL-ART-015.FAIL:** Flat undressed checkerboard or grid test maps in release candidates fail acceptance.
  * **REL-ART-015.VERIF:** `PKG-REND` (environment dressing site audit).
  * **REL-ART-015.LANE:** World & Level Design.

* **REL-ART-016 — Dressing Collision Truth & Passability Parity:** Decorative environmental art (shards, rocks, vegetation) shall strictly reflect collision truth. Art assets placed on passable tiles shall NOT appear impassable or taller than units (resolving C41).
  * **REL-ART-016.AUTH:** 159-unit cliff meshes shall only exist on authoritative Blocked tiles; walkable crossings shall feature low-lying dressing (<20 cm).
  * **REL-ART-016.FAIL:** Passable crossings occluded by giant decorative boulders fails tactical clarity.
  * **REL-ART-016.VERIF:** `PKG-REND` + `SRC` (dressing passability alignment check).
  * **REL-ART-016.LANE:** World & Level Design.

* **REL-ART-017 — True 3D Volumetric Fog of War Occlusion:** Fog of war and unexplored shroud shall render as a 3D volumetric occlusion boundary that completely conceals terrain elevations, cliffs, and structures (resolving C5).
  * **REL-ART-017.AUTH:** The prior ankle-height 6-unit decal is eliminated; unexplored terrain renders in pitch charcoal shroud.
  * **REL-ART-017.FAIL:** 159-unit cliff meshes poking through fog from tick 0 is a critical visual defect.
  * **REL-ART-017.VERIF:** `PKG-REND` (3D fog occlusion capture review).
  * **REL-ART-017.LANE:** Visual Presentation (`EchoesFogView`).

* **REL-ART-018 — Explored Shroud Object Memory Persistence:** Structures scouted by player units shall remain visible in explored fog as desaturated, static memory silhouettes upon vision loss (resolving C6).
  * **REL-ART-018.AUTH:** If an enemy structure is destroyed under shroud, the player sees the last known state until fresh vision is acquired.
  * **REL-ART-018.FAIL:** Scouted bases vanishing into thin air when vision lapses violates core RTS mechanics.
  * **REL-ART-018.VERIF:** `PKG-REND` (shroud object memory persistence test).
  * **REL-ART-018.LANE:** Visual Presentation & Core Gameplay.

* **REL-ART-019 — Rigorous Exposure & Lighting Calibration:** Site lighting rigs shall feature fixed or tightly bounded auto-exposure, calibrated key lighting motivated by the Crownfall sky, and balanced fill preserving shadow details on charcoal terrain without clipping ceramic whites.
  * **REL-ART-019.AUTH:** 100% of histogram samples must fall within legal SDR range (16–235 luma) without blown highlights.
  * **REL-ART-019.FAIL:** Blown ceramic highlights or crushed pitch shadows fail lighting quality.
  * **REL-ART-019.VERIF:** `PKG-REND` (exposure histogram audit on Metal captures).
  * **REL-ART-019.LANE:** Visual Presentation.

* **REL-ART-020 — Particle VFX Performance & Collision Discipline:** All Niagara and mesh particle emitters shall have simple collision, overlaps, shadows, and navigation influence disabled.
  * **REL-ART-020.AUTH:** Particle simulation cost for 50 simultaneous combat explosions shall not exceed 1.5 ms on the GPU.
  * **REL-ART-020.FAIL:** Particles triggering CPU collision events or casting dynamic shadow cascades fails performance budgets.
  * **REL-ART-020.VERIF:** `SRC` + `PKG-AUTO` (VFX property check and GPU profiler test).
  * **REL-ART-020.LANE:** VFX & Performance.

* **REL-ART-024 — Deterministic-Decoupled Kinetic Combat Ragdolls:** When mechanical or organic units are destroyed by explosive or heavy concussive fire, the presentation layer shall spawn decoupled cosmetic physics ragdolls and debris pieces that tumble dynamically down slopes.
  * **REL-ART-024.AUTH:** Physics pieces are presentation-only, run on the GPU/client thread, and have zero influence on simulation collision or pathing.
  * **REL-ART-024.FAIL:** Physics ragdolls desynchronizing simulation state or blocking unit movement is prohibited.
  * **REL-ART-024.VERIF:** `PKG-REND` (kinetic ragdoll visual review).
  * **REL-ART-024.LANE:** Visual Presentation & VFX.

* **REL-ART-025 — Persistent Battlefield Scorch Decals & Vitrification:** Heavy combat, building collapses, and Future Well Harvests shall project persistent scorch marks, vitrified glass craters, and ash blast seams on the terrain plane that linger for the remainder of the match.
  * **REL-ART-025.AUTH:** Decals render via a static terrain decal atlas, maintaining 0% performance degradation over time.
  * **REL-ART-025.FAIL:** Battlefield instantly erasing all combat marks after 5 seconds fails visual permanence.
  * **REL-ART-025.VERIF:** `PKG-REND` (persistent terrain scorch decal review).
  * **REL-ART-025.LANE:** Visual Presentation.

* **REL-ART-026 — Dynamic Directional Shield Impact Ripples:** High-energy projectiles impacting unit shields (Bulwark barriers, Core shields) shall project directional chromatic wave ripples expanding across the shield surface, styled in the faction palette.
  * **REL-ART-026.AUTH:** Ripple intensity scales with damage received, providing immediate visual feedback on hit severity.
  * **REL-ART-026.FAIL:** Shields disappearing on hit without impact animation fails visual feedback.
  * **REL-ART-026.VERIF:** `PKG-REND` (shield impact ripple shader review).
  * **REL-ART-026.LANE:** Visual Presentation & VFX.

* **REL-ART-027 — Structural Critical Degradation States:** Structures damaged below 30% maximum health shall transition to a critical degradation visual state: emitting billowing black smoke, venting electrical sparks, and displaying warning lighting strips.
  * **REL-ART-027.AUTH:** Provides instant situational awareness that a building is near collapse without reading health bars.
  * **REL-ART-027.FAIL:** Buildings at 1% HP looking pristine and undamaged fails visual reality.
  * **REL-ART-027.VERIF:** `PKG-REND` (damaged structure visual inspection).
  * **REL-ART-027.LANE:** Visual Presentation.

---

### §19 Audio, Voice, Music, and Cinematics (`REL-AUD-*`, `REL-CIN-*`)

* **REL-AUD-001 — Five-Category Submix Hierarchy:** The runtime audio engine shall route all sound through a five-category submix graph: Music, Dialogue/Voice, Interface, Ambience, and Effects, with independent volume controls and master bus routing.
  * **REL-AUD-001.AUTH:** Changing the volume slider for one category in Options modifies only that submix without altering others.
  * **REL-AUD-001.FAIL:** Unrouted cues playing directly to master output fail mix architecture.
  * **REL-AUD-001.VERIF:** `SRC` + `PKG-AUTO` (`EchoesAudioSubmixTest`).
  * **REL-AUD-001.LANE:** Audio.

* **REL-AUD-002 — Integrated Loudness & True Peak Mastering Target:** Packaged gameplay audio shall adhere to the ITU-R BS.1770-4 mastering standard: integrated loudness of $-16.0	ext{ LUFS} \pm 1.0	ext{ LU}$ across a 30-minute session, with true peaks strictly capping at $\le -1.0	ext{ dBTP}$ (resolving C38).
  * **REL-AUD-002.AUTH:** Eliminates clipping distortion and excessive loudness spikes.
  * **REL-AUD-002.FAIL:** Session loudness of -10 LUFS or peaks exceeding 0 dBTP fail acceptance.
  * **REL-AUD-002.VERIF:** `PKG-AUTO` (BS.1770-4 automated loudness measurement).
  * **REL-AUD-002.LANE:** Audio.

* **REL-AUD-003 — Dynamic Side-Chain Vocal Ducking:** When character dialogue or voiced narration initializes on the Dialogue submix, the audio engine shall duck active Music by -6.0 dB and Ambience by -4.0 dB within 150 ms.
  * **REL-AUD-003.AUTH:** Combat and interface effects do NOT duck, ensuring combat readability remains uncompromised. Music returns to nominal level over 500 ms upon line completion.
  * **REL-AUD-003.FAIL:** Unintelligible voice masked by loud background music fails audio review.
  * **REL-AUD-003.VERIF:** `SRC` + `PKG-AUTO` (side-chain ducking submix test).
  * **REL-AUD-003.LANE:** Audio.

* **REL-AUD-004 — Local Neural Text-to-Speech Voice Generation Pipeline:** Campaign dialogue and briefing narration shall be generated locally using the Kokoro-82M open-weights neural TTS engine at 48 kHz mono PCM, registered deterministically in `AssetRegister.md` without runtime cloud dependencies.
  * **REL-AUD-004.AUTH:** All 308 authored dialogue lines across the 15 campaign missions shall possess registered audio files.
  * **REL-AUD-004.FAIL:** Silent text boxes or unvoiced campaign dialogue in release builds is strictly prohibited.
  * **REL-AUD-004.VERIF:** `SRC` (voice asset completeness audit).
  * **REL-AUD-004.LANE:** Audio & Narrative.

* **REL-AUD-005 — Voice Profile: Mara Vey (Meridian Logistics Specialist):** Mara Vey shall be performed with a level, precise, engineering cadence; urgency compressed into economy of words rather than shouting.
  * **REL-AUD-005.AUTH:** Character delivery matches the Character Voice Identity Bible, normalized to -16 LUFS.
  * **REL-AUD-005.FAIL:** Melodramatic or frantic delivery violating character bible fails review.
  * **REL-AUD-005.VERIF:** `HUM` + `OWNER` (voice listening session review).
  * **REL-AUD-005.LANE:** Narrative & Audio.

* **REL-AUD-006 — Voice Profile: Oruun-of-Seven-Stones (Kharuun Speaker):** Oruun shall be performed with layered, deliberate pacing, vocalizing the communal consensus of inherited memory with subtle dry humor.
  * **REL-AUD-006.AUTH:** Speech is measured and grounded, avoiding mystical stereotypes.
  * **REL-AUD-006.FAIL:** Monolithic or aggressive tribal voice delivery fails canon.
  * **REL-AUD-006.VERIF:** `HUM` + `OWNER` (voice listening session review).
  * **REL-AUD-006.LANE:** Narrative & Audio.

* **REL-AUD-007 — Voice Profile: Talar Venn (Meridian Archivist):** Talar Venn shall be performed with a careful, analytical, quietly persistent vocal tone, reflecting decades of preserving records under scarcity.
  * **REL-AUD-007.AUTH:** Exacting diction, gentle gravitas, clear articulation.
  * **REL-AUD-007.FAIL:** Harsh military bark on Talar fails character specification.
  * **REL-AUD-007.VERIF:** `HUM` + `OWNER` (voice listening session review).
  * **REL-AUD-007.LANE:** Narrative & Audio.

* **REL-AUD-008 — Voice Profile: Chancellor Cael Rhyse (Compact High Authority):** Cael Rhyse shall be performed with a warm, persuasive, deeply reasonable tone—an administrative leader whose terrifying certainty sounds entirely rational.
  * **REL-AUD-008.AUTH:** Smooth, resonant, authoritative yet humane pacing.
  * **REL-AUD-008.FAIL:** Cartoon villain rasp or tyrannical delivery fails character depth.
  * **REL-AUD-008.VERIF:** `HUM` + `OWNER` (voice listening session review).
  * **REL-AUD-008.LANE:** Narrative & Audio.

* **REL-AUD-009 — Voice Profile: Neme (Hollow Choir Consciousness):** Neme shall be performed with constructed precision, delivery lightly non-idiomatic, assembled from incompatible possibilities without supernatural distortion.
  * **REL-AUD-009.AUTH:** Exact, slightly decoupled phrasing, crystalline clarity.
  * **REL-AUD-009.FAIL:** Demonic pitch-shifted monster voice fails Choir creative canon.
  * **REL-AUD-009.VERIF:** `HUM` + `OWNER` (voice listening session review).
  * **REL-AUD-009.LANE:** Narrative & Audio.

* **REL-AUD-010 — Full Procedural Music Suite & Act Themes:** The soundtrack shall comprise registered procedural compositions: Title theme, 3 faction identity themes, 3 act themes (Act I *Necessary Fires*, Act II *The Cost of One Future*, Act III *Crownfall*), and 4 resolution cues.
  * **REL-AUD-010.AUTH:** Meridian music uses prepared piano and mechanical pulses; Kharuun uses resonant stone/strata rhythms; Choir uses multi-directional chord resolutions.
  * **REL-AUD-010.FAIL:** Generic orchestral trailer music violating faction audio identity fails review.
  * **REL-AUD-010.VERIF:** `PKG-REND` (music suite listening review).
  * **REL-AUD-010.LANE:** Audio.

* **REL-AUD-011 — Dynamic Combat State Cross-Fading:** In-game music shall dynamically transition between low-intensity tension beds and high-intensity combat cues based on authoritative simulation combat state.
  * **REL-AUD-011.AUTH:** When active weapon fire occurs within viewport range, the combat percussion layer cross-fades in over 1.5 seconds without hard cuts.
  * **REL-AUD-011.FAIL:** Jarring abrupt audio track cuts during combat fail mix transitions.
  * **REL-AUD-011.VERIF:** `PKG-AUTO` (dynamic music cross-fade test).
  * **REL-AUD-011.LANE:** Audio.

* **REL-AUD-012 — Positional Environmental Ambience Beds:** Each map site shall feature authored ambient soundscapes: Glass Scar wind and shard chimes; Lume Reach failing electrical hum; subterranean Kharuun strata resonance; Crownfall atmospheric fracture hum.
  * **REL-AUD-012.AUTH:** Ambience levels balance at -24 LUFS, remaining audible without masking gameplay cues.
  * **REL-AUD-012.FAIL:** Silent maps or looping white noise fails audio quality.
  * **REL-AUD-012.VERIF:** `PKG-REND` (ambience bed review).
  * **REL-AUD-012.LANE:** Audio.

* **REL-AUD-013 — Textural Non-Spoken Unit Acknowledgements:** Unit selection and move commands shall emit textural material acknowledgements rather than spoken military barks: Compact engineered servo clicks; Kharuun mineral resonance; Choir phase chimes.
  * **REL-AUD-013.AUTH:** Cues are brief (100–250 ms) and rate-limited to avoid auditory fatigue.
  * **REL-AUD-013.FAIL:** Repetitive spoken unit barks violating the Development Bible fail acceptance.
  * **REL-AUD-013.VERIF:** `PKG-REND` (unit feedback cue review).
  * **REL-AUD-013.LANE:** Audio.

* **REL-AUD-014 — High-Priority Alert Audio Rate Limiting:** Critical warnings (`Under Attack`, `Structure Lost`, `Future Well Harvested`, `Command Core Damaged`) shall fire distinct alarms with a two-tier rate-limiter: maximum 1 alert per 4.0 seconds for general structures, but ZERO rate-limiting suppression for Command Core damage (resolving C8).
  * **REL-AUD-014.AUTH:** Command Core under attack alerts bypass cooldowns to prevent unannounced losses.
  * **REL-AUD-014.FAIL:** Suppressing the only alarm warning of Core loss fails player protection.
  * **REL-AUD-014.VERIF:** `SRC` (alert rate-limiter prioritisation test).
  * **REL-AUD-014.LANE:** Audio & Player Experience.

* **REL-AUD-015 — Whole-Graph Reduced Dynamic Range Mode:** The audio options shall provide a `Reduced Dynamic Range` accessibility preset applying an authored multiband compressor to the master submix bus.
  * **REL-AUD-015.AUTH:** Elevates quiet ambient/dialogue cues while clamping loud combat peaks to within a 12 dB dynamic window, ensuring audibility at low volume.
  * **REL-AUD-015.FAIL:** Flat volume trim masquerading as dynamic range compression is prohibited.
  * **REL-AUD-015.VERIF:** `SRC` + `PKG-AUTO` (submix compressor profile test).
  * **REL-AUD-015.LANE:** Audio & Accessibility.

* **REL-AUD-016 — Situational Unit Tactical Chatter & Annoyance Lines:** Units shall emit situational radio barks during gameplay: flanking notifications, heavy armor spotting, and low-health retreat urges. Repeatedly clicking a friendly unit 6+ times cycles through humorous in-character fourth-wall lines.
  * **REL-AUD-016.AUTH:** Annoyance lines are rate-limited and voiced in the unit's authentic character cadence.
  * **REL-AUD-016.FAIL:** Annoyance lines interrupting critical combat alerts is strictly prohibited.
  * **REL-AUD-016.VERIF:** `PKG-REND` + `HUM` (unit dialogue and easter egg listening test).
  * **REL-AUD-016.LANE:** Audio & Narrative.

* **REL-AUD-017 — Bespoke Faction Tactical Announcers:** Each faction shall possess an authentic tactical voice announcer: Meridian Operations Logistics Annunciator (disciplined, precise); Kharuun Memory Speaker (reverent, communal); Choir Resonance (crystalline, multifaceted).
  * **REL-AUD-017.AUTH:** Announcers vocalize: `[UPGRADE COMPLETE]`, `[BASE UNDER ATTACK]`, `[DAWN DEFICIT]`, `[WELL HARVEST DETECTED]`.
  * **REL-AUD-017.FAIL:** Monolithic generic announcer shared across all three factions fails immersion.
  * **REL-AUD-017.VERIF:** `PKG-AUTO` (faction announcer voice line audit).
  * **REL-AUD-017.LANE:** Audio.

* **REL-AUD-018 — Acoustic Environmental Spatial Occlusion & Reverb:** World soundscapes shall pass through physical acoustics filters: sounds originating in subterranean vaults gain damp stone resonance; open Glass Scar sounds experience wide-band wind dispersal; sound behind cliffs attenuates high frequencies by -8 dB.
  * **REL-AUD-018.AUTH:** Spatial audio provides intuitive directional distance cues to the player.
  * **REL-AUD-018.FAIL:** Sounds behind thick stone walls playing with identical volume and crispness fails realism.
  * **REL-AUD-018.VERIF:** `SRC` + `PKG-AUTO` (audio occlusion DSP filter test).
  * **REL-AUD-018.LANE:** Audio.

* **REL-CIN-001 — Sequencer In-Engine Cutscene Pipeline:** Cinematic sequences shall be authored and executed via Unreal Engine Level Sequencer, operating strictly over registered project assets with ground-referenced camera movement.
  * **REL-CIN-001.AUTH:** Sequencer reads authoritative campaign state and writes nothing back; zero simulation mutation.
  * **REL-CIN-001.FAIL:** Pre-rendered video files (FMV) or cutscenes altering simulation data fails pipeline rules.
  * **REL-CIN-001.VERIF:** `SRC` (`EchoesSequencerTest`).
  * **REL-CIN-001.LANE:** Cinematics.

* **REL-CIN-002 — In-Engine Title Cinematic Sequence:** The game shall open with an authored in-engine sequence establishing Soryn's fractured sun, the fall of Dawnshards, and the emergence of the three factions.
  * **REL-CIN-002.AUTH:** Duration $\le 90	ext{ seconds}$, fully voiced narration, letterbox presentation.
  * **REL-CIN-002.FAIL:** Shipped build lacking an opening title cinematic fails release scope.
  * **REL-CIN-002.VERIF:** `PKG-REND` (opening sequence review).
  * **REL-CIN-002.LANE:** Cinematics.

* **REL-CIN-003 — Act I Transition Sequence ("Necessary Fires"):** An in-engine interlude staged over the ruins of Lume Reach bridging Missions 05 and 06, reflecting the irreversible costs of the opening evacuation.
  * **REL-CIN-003.AUTH:** Voiced by Mara Vey and Talar Venn, synchronizing with Act I musical resolution.
  * **REL-CIN-003.FAIL:** Missing Act I transition cutscene fails narrative continuity.
  * **REL-CIN-003.VERIF:** `PKG-REND` (Act I cutscene review).
  * **REL-CIN-003.LANE:** Cinematics & Narrative.

* **REL-CIN-004 — Act II Transition Sequence ("The Cost of One Future"):** An in-engine sequence staged over the subterranean vaults of the Unburied Road bridging Missions 10 and 11, documenting the escalation toward the Crownfall.
  * **REL-CIN-004.AUTH:** Voiced by Oruun, reflecting the ancestral memory of the first breaking.
  * **REL-CIN-004.FAIL:** Missing Act II transition cutscene fails narrative continuity.
  * **REL-CIN-004.VERIF:** `PKG-REND` (Act II cutscene review).
  * **REL-CIN-004.LANE:** Cinematics & Narrative.

* **REL-CIN-005 — Act III Transition Sequence ("Crownfall"):** An in-engine sequence capturing the sky tearing open over the Broken Sun prior to Mission 15, as the Hollow Choir asserts reality.
  * **REL-CIN-005.AUTH:** Voiced by Neme and Cael Rhyse, staging the final conflict.
  * **REL-CIN-005.FAIL:** Missing Act III transition cutscene fails narrative continuity.
  * **REL-CIN-005.VERIF:** `PKG-REND` (Act III cutscene review).
  * **REL-CIN-005.LANE:** Cinematics & Narrative.

* **REL-CIN-006 — Four Authoritative Ending Cinematics:** The game shall contain four distinct in-engine ending cutscenes: `Restoration` (rebuilding the old sun at terrible cost), `Controlled Stabilization` (freezing possibility in rigid order), `Extinguishment` (letting the shards die into dark peace), and `Open Evolution` (embracing unwritten futures).
  * **REL-CIN-006.AUTH:** Staged at the accord sites with registered assets and the ending's bespoke resolution theme.
  * **REL-CIN-006.FAIL:** Endings showing generic slideshow cards instead of in-engine Sequencer cutscenes fails release bar.
  * **REL-CIN-006.VERIF:** `PKG-REND` (all 4 ending cinematics review).
  * **REL-CIN-006.LANE:** Cinematics & Narrative.

* **REL-CIN-007 — Universal Cinematic Skippability & Subtitle Guarantee:** Every cinematic sequence shall be skippable by pressing `Escape` or holding `Space` for 1.0 second, displaying an on-screen skip prompt.
  * **REL-CIN-007.AUTH:** Subtitles with high-contrast background boxes are enabled by default and synchronize with dialogue $\pm 100	ext{ ms}$.
  * **REL-CIN-007.FAIL:** Unskippable cutscenes or missing subtitles fails accessibility standards.
  * **REL-CIN-007.VERIF:** `PKG-PHYS` (cutscene skip and subtitle sync test).
  * **REL-CIN-007.LANE:** Cinematics & Accessibility.

* **REL-CIN-008 — Production Visual Bar Parity in Cinematics:** Cinematic shots shall use the exact same production shaders, textures, and lighting discipline as the core game, with zero placeholder meshes or lighting seams.
  * **REL-CIN-008.AUTH:** Lighting transitions cleanly back into gameplay camera without frame hitches.
  * **REL-CIN-008.FAIL:** Visual quality drops or placeholder assets in cinematics fail release gates.
  * **REL-CIN-008.VERIF:** `PKG-REND` (cinematic frame quality inspection).
  * **REL-CIN-008.LANE:** Cinematics & Visual.

### §20 Saves, Profiles, Progression, and Recovery (`REL-SAV-*`)

* **REL-SAV-001 — Transactional Atomic Save Writing:** Game state serialization shall write first to a temporary file (`.sav.tmp`) and atomically rename to `.sav` upon completion, rotating the prior save to `.sav.bak`.
  * **REL-SAV-001.AUTH:** Power loss, process termination, or crash during serialization leaves the previous valid `.sav` intact.
  * **REL-SAV-001.FAIL:** Writing directly to the primary save file causing partial corruption fails data integrity.
  * **REL-SAV-001.VERIF:** `SRC` (atomic rename and interrupted write simulation test).
  * **REL-SAV-001.LANE:** Save Recovery & Core Gameplay.

* **REL-SAV-002 — Header Schema & CRC32/FNV-1a Checksum Validation:** Every save file shall contain an authoritative 64-byte binary header containing: Magic Bytes (`ECH0`), Schema Version, Match Timestamp, Tick Number, and an FNV-1a 64-bit payload checksum.
  * **REL-SAV-002.AUTH:** Deserialization validates the checksum before parsing entity arrays; mismatched checksums reject cleanly.
  * **REL-SAV-002.FAIL:** Deserializing corrupt or truncated data without checksum check fails security standards.
  * **REL-SAV-002.VERIF:** `SRC` (save header checksum verification test).
  * **REL-SAV-002.LANE:** Save Recovery.

* **REL-SAV-003 — Isolated Player Profile Persistence:** Player configuration (audio volumes, keybinds, resolution, accessibility presets, campaign mission unlocks) shall be stored independently in `Profile.sav`.
  * **REL-SAV-003.AUTH:** Resetting or deleting a match save shall not erase player profile preferences.
  * **REL-SAV-003.FAIL:** Profile corruption resetting keybinds on game restart fails usability.
  * **REL-SAV-003.VERIF:** `SRC` (profile persistence isolation test).
  * **REL-SAV-003.LANE:** Save Recovery & UI.

* **REL-SAV-004 — Three Independent Campaign Journey Slots:** The game shall support three distinct campaign journey slots (Slot 1, Slot 2, Slot 3), each maintaining an independent ledger of decisions, unlocks, and endings.
  * **REL-SAV-004.AUTH:** Playing on Slot 2 shall never alter or overwrite progress in Slot 1.
  * **REL-SAV-004.FAIL:** Cross-contamination between save slots fails acceptance.
  * **REL-SAV-004.VERIF:** `PKG-AUTO` (multi-slot campaign journey test).
  * **REL-SAV-004.LANE:** Campaign & Save Recovery.

* **REL-SAV-005 — Tactical Mission Checkpoint State Serialization:** Mid-mission tactical saves shall capture complete simulation state: entity positions, velocities, current orders, health, shields, resources, fog explored bits, and active Well timers.
  * **REL-SAV-005.AUTH:** Restoring a save shall resume gameplay seamlessly on the exact tick with identical entity positions.
  * **REL-SAV-005.FAIL:** Units losing assigned orders or resetting positions upon load fails checkpoint integrity.
  * **REL-SAV-005.VERIF:** `SRC` (tactical save/restore round-trip fidelity test).
  * **REL-SAV-005.LANE:** Core Gameplay & Save Recovery.

* **REL-SAV-006 — Autonomous Milestone Autosave Cadence:** The game shall trigger an automated background checkpoint save upon completing any primary objective and every 6,000 simulation ticks (5 minutes) in single-player modes.
  * **REL-SAV-006.AUTH:** Autosaves occupy a dedicated `Autosave.sav` slot without overwriting manual user saves.
  * **REL-SAV-006.FAIL:** Stalling or stuttering gameplay during an autosave fails frame-budget rules.
  * **REL-SAV-006.VERIF:** `PKG-AUTO` (autosave cadence and non-blocking execution test).
  * **REL-SAV-006.LANE:** Save Recovery & Player Experience.

* **REL-SAV-007 — Asynchronous Non-Blocking Save Frame Budget:** Save file serialization shall execute asynchronously on a background worker thread, imposing no more than $250.00	ext{ ms}$ total frame delay on the main game thread (resolving C39).
  * **REL-SAV-007.AUTH:** The game thread takes a fast memory snapshot (≤10 ms) and offloads disk I/O to the background thread.
  * **REL-SAV-007.FAIL:** Hitching the game thread for $>250	ext{ ms}$ during save writes is a critical performance defect.
  * **REL-SAV-007.VERIF:** `PKG-AUTO` (save thread profiler budget assertion).
  * **REL-SAV-007.LANE:** Performance & Save Recovery.

* **REL-SAV-008 — Cross-Platform Endian-Safe Binary Format:** Save files shall use little-endian byte ordering, fixed-width integer fields (`int32`, `int64`), and explicit padding, ensuring identical binary compatibility across Apple Silicon (ARM64) and x86_64.
  * **REL-SAV-008.AUTH:** A save generated on macOS ARM64 shall deserialize identically on PC or Linux.
  * **REL-SAV-008.FAIL:** Platform-dependent struct packing causing save incompatibility fails portability.
  * **REL-SAV-008.VERIF:** `SRC` (cross-platform save format struct size and offset test).
  * **REL-SAV-008.LANE:** Build Distribution & Save Recovery.

* **REL-SAV-009 — Graceful Corrupt Save Containment & Backup Recovery:** If a save file fails CRC verification or is truncated, the load routine shall reject the file gracefully, display an error modal (`[SAVE CORRUPT: RESTORING BACKUP]`), and offer to load `.sav.bak`.
  * **REL-SAV-009.AUTH:** Under zero circumstances shall a corrupt save crash the game or delete the user's profile.
  * **REL-SAV-009.FAIL:** Crash to desktop upon selecting a corrupt save fails stability criteria.
  * **REL-SAV-009.VERIF:** `SRC` (corrupt save recovery and fallback test).
  * **REL-SAV-009.LANE:** Save Recovery.

* **REL-SAV-010 — Schema Versioning & Forward Migration Discipline:** The save serializer shall embed a monotonic schema version integer. Loading an older valid version shall invoke a deterministic migration transformer to update data to the current format.
  * **REL-SAV-010.AUTH:** Incompatible major versions shall display `[SAVE FROM INCOMPATIBLE VERSION]` without crashing.
  * **REL-SAV-010.FAIL:** Unversioned binary blobs failing on minor schema updates fail maintainability.
  * **REL-SAV-010.VERIF:** `SRC` (schema version migration test).
  * **REL-SAV-010.LANE:** Save Recovery.

* **REL-SAV-011 — Campaign Consequence Ledger Tamper Resistance:** The campaign ledger shall record decisions as a forward-only cryptographic hash chain: $H_n = 	ext{Hash}(H_{n-1} + DecisionData)$.
  * **REL-SAV-011.AUTH:** Manual external file tampering or desynchronized decision order is detected and flagged on load.
  * **REL-SAV-011.FAIL:** Arbitrary text edits to the save unlocking all missions without verification fails integrity.
  * **REL-SAV-011.VERIF:** `SRC` (ledger hash-chain validation test).
  * **REL-SAV-011.LANE:** Campaign & Save Recovery.

* **REL-SAV-012 — In-Match Save Lockout Safety Zones:** The manual save system shall be temporarily disabled during critical simulation transitions: active cutscenes, mid-tick unit spawning, and the exact tick of victory/defeat evaluation.
  * **REL-SAV-012.AUTH:** The Save button is disabled with tooltip `[SAVE UNAVAILABLE DURING SEQUENCE]`.
  * **REL-SAV-012.FAIL:** Saving during mid-frame destruction causing half-initialized entities fails recovery.
  * **REL-SAV-012.VERIF:** `SRC` (save lockout state assertion).
  * **REL-SAV-012.LANE:** Core Gameplay & Save Recovery.

* **REL-SAV-013 — macOS Sandboxed Directory Compliance:** Save files on macOS shall reside strictly within the standard sandboxed application support directory: `~/Library/Application Support/EchoesOfTheBrokenSun/Saves/`.
  * **REL-SAV-013.AUTH:** Saves shall not be written to `/tmp`, `/var`, the user Desktop, or the application bundle.
  * **REL-SAV-013.FAIL:** Writing saves inside `/Applications/` causing permission errors fails macOS guidelines.
  * **REL-SAV-013.VERIF:** `SRC` (path resolution and sandbox directory test).
  * **REL-SAV-013.LANE:** Build Distribution.

* **REL-SAV-014 — Save Data Privacy & Zero Personal Telemetry:** Save files shall store purely abstract game simulation values; under zero circumstances shall saves contain usernames, IP addresses, machine identifiers, or personal telemetry.
  * **REL-SAV-014.AUTH:** Save files are 100% anonymous game states.
  * **REL-SAV-014.FAIL:** Personal telemetry leakage into save files fails privacy review.
  * **REL-SAV-014.VERIF:** `SRC` (save data privacy payload audit).
  * **REL-SAV-014.LANE:** Security & Governance.

---

### §21 Accessibility and Localization Readiness (`REL-ACC-*`, `REL-LOC-*`)

* **REL-ACC-001 — Color-Vision Deficiency Simulation Presets:** The renderer shall include post-process color-grading LUTs simulating and compensating for Protanopia, Deuteranopia, and Tritanopia.
  * **REL-ACC-001.AUTH:** When active, faction colors shift to high-contrast discernible wavelengths (e.g. replacing red/green confusion pairs with blue/yellow distinctions).
  * **REL-ACC-001.FAIL:** Colorblind mode failing to adjust minimap unit blips fails accessibility.
  * **REL-ACC-001.VERIF:** `PKG-REND` (color-blind LUT shader verification).
  * **REL-ACC-001.LANE:** Visual Presentation & Accessibility.

* **REL-ACC-002 — Dual-Channel Non-Color Information Encoding:** All gameplay indicators (selection, friendly/enemy affiliation, unit health, alerts) shall convey meaning through distinct shapes and symbols in addition to color.
  * **REL-ACC-002.AUTH:** Friendly units use chevron shields; enemies use sharp diamond reticles; neutrals use circles.
  * **REL-ACC-002.FAIL:** Presenting hostile vs friendly purely through red vs green hue fails non-color rules.
  * **REL-ACC-002.VERIF:** `PKG-REND` (dual-channel symbol audit).
  * **REL-ACC-002.LANE:** Player Experience & Visual.

* **REL-ACC-003 — High-Contrast Silhouette Enhancement:** The accessibility menu shall provide a `High Contrast` mode rendering a 2-pixel luminous silhouette outline around all active units and darkening terrain albedo by 20%.
  * **REL-ACC-003.AUTH:** Enables clear unit identification for low-vision players.
  * **REL-ACC-003.FAIL:** High-contrast outline causing performance drop >1.0 ms fails efficiency rules.
  * **REL-ACC-003.VERIF:** `PKG-REND` (high-contrast mode visual review).
  * **REL-ACC-003.LANE:** Visual Presentation & Accessibility.

* **REL-ACC-004 — Reduced Motion Preset Invariant:** When `Reduced Motion` is enabled, the camera shall eliminate screen shake, decouple travel facing rotation to instantaneous snaps (resolving MOV-010), and disable non-essential cosmetic particle drifts.
  * **REL-ACC-004.AUTH:** Gameplay mechanics and combat damage remain 100% identical.
  * **REL-ACC-004.FAIL:** Screen shake persisting under Reduced Motion fails comfort compliance.
  * **REL-ACC-004.VERIF:** `PKG-REND` (reduced motion camera capture review).
  * **REL-ACC-004.LANE:** Player Experience & Visual.

* **REL-ACC-005 — Reduced Flashing & Photosensitive Safety:** When `Reduced Flashing` is enabled, screen-space combat flashes, nuclear Bloom pulses, and strobe hit effects shall be clamped to static, non-pulsing border highlights (resolving C36).
  * **REL-ACC-005.AUTH:** The game passes the Harding FPA test for photosensitive epilepsy risk (<3 flashes per second).
  * **REL-ACC-005.FAIL:** Eliminating the flash by removing damage visibility entirely is prohibited (resolving C36).
  * **REL-ACC-005.VERIF:** `SRC` + `PKG-REND` (photosensitivity strobe test).
  * **REL-ACC-005.LANE:** Visual Presentation & Accessibility.

* **REL-ACC-006 — Full UI Scale Dynamic Range Compliance:** The UI Scale system shall support continuous adjustments from 80% to 150% without clipping text or corrupting button click areas (resolving C35).
  * **REL-ACC-006.AUTH:** Minimum font size at 150% scale meets WCAG AAA guidelines for legibility at desktop viewing distances.
  * **REL-ACC-006.FAIL:** UI elements breaking bounds or overlapping at 150% scale fails acceptance.
  * **REL-ACC-006.VERIF:** `PKG-REND` (UI scaling layout review).
  * **REL-ACC-006.LANE:** Player Experience (`EchoesHUD`).

* **REL-ACC-007 — High-Contrast Subtitle Styling & Scalability:** Subtitles shall render with an authored semi-opaque charcoal bounding box, high-contrast pale text, and support font scaling between 14pt and 28pt.
  * **REL-ACC-007.AUTH:** Subtitles synchronize with voice lines within $\pm 100	ext{ ms}$.
  * **REL-ACC-007.FAIL:** Transparent subtitles disappearing over bright terrain fails readability.
  * **REL-ACC-007.VERIF:** `PKG-REND` (subtitle readability and scale review).
  * **REL-ACC-007.LANE:** Player Experience & Cinematics.

* **REL-ACC-008 — Mandatory Subtitle Speaker Identity Tags:** Every dialogue subtitle shall prepend the speaker's name in brackets (e.g. `[MARA VEY]`, `[ORUUN]`), styled in the character's signature faction tone.
  * **REL-ACC-008.AUTH:** Prevents ambiguity in multi-speaker conversations.
  * **REL-ACC-008.FAIL:** Unattributed dialogue strings in subtitles fail accessibility criteria.
  * **REL-ACC-008.VERIF:** `SRC` (dialogue subtitle string audit).
  * **REL-ACC-008.LANE:** Narrative & Player Experience.

* **REL-ACC-009 — Directional Spatial Off-Screen Visual Indicators:** High-priority sound cues (attacks, explosions, alerts) occurring off-screen shall project an edge-of-screen directional chevron pointing toward the event location.
  * **REL-ACC-009.AUTH:** Enables deaf and hard-of-hearing players to react to spatial threats.
  * **REL-ACC-009.FAIL:** Off-screen attacks producing only audio alarms fails situational awareness.
  * **REL-ACC-009.VERIF:** `PKG-REND` (directional indicator visual review).
  * **REL-ACC-009.LANE:** Player Experience (`EchoesHUD`).

* **REL-ACC-010 — Screen Reader TTS Menu Accessibility:** All primary menu buttons, options sliders, and combat tooltips shall expose descriptive accessibility labels readable by macOS VoiceOver and embedded TTS.
  * **REL-ACC-010.AUTH:** Focusing a button reads its title, state, and assigned hotkey.
  * **REL-ACC-010.FAIL:** Missing accessibility labels on interactive widgets fails VoiceOver audit.
  * **REL-ACC-010.VERIF:** `PKG-PHYS` (VoiceOver navigation sweep).
  * **REL-ACC-010.LANE:** Player Experience.

* **REL-ACC-011 — Comprehensive Input Key Re-mapping:** Players shall have the ability to rebind every keyboard key and mouse button across all game commands, camera controls, and menus.
  * **REL-ACC-011.AUTH:** The settings screen warns upon detecting conflicting duplicate bindings.
  * **REL-ACC-011.FAIL:** Hardcoded keys that cannot be rebound fail accessibility requirements.
  * **REL-ACC-011.VERIF:** `PKG-PHYS` (key remapping functional test).
  * **REL-ACC-011.LANE:** Player Experience (`EchoesPlayerController`).

* **REL-ACC-012 — One-Handed Mouse-Only Playability:** Every gameplay action (movement, combat, abilities, building, camera panning, pausing) shall be 100% executable using a standard 2-button mouse without requiring keyboard input.
  * **REL-ACC-012.AUTH:** On-screen HUD buttons exist for all hotkey actions (Idle Worker, Production, Stances, Menu).
  * **REL-ACC-012.FAIL:** Actions requiring mandatory dual-key keyboard chords fail one-handed accessibility.
  * **REL-ACC-012.VERIF:** `HUM` (mouse-only full campaign mission playtest).
  * **REL-ACC-012.LANE:** Player Experience & QA.

* **REL-ACC-013 — Configurable Edge-Pan Speed & Dead-Zone Calibration:** The Options menu shall provide sliders to configure edge-panning scroll speed (0% to 200%) and edge dead-zone screen margins (0 to 30 pixels).
  * **REL-ACC-013.AUTH:** Setting edge-pan to 0% disables mouse edge scrolling entirely (for windowed mode).
  * **REL-ACC-013.FAIL:** Uncontrollable edge panning in windowed mode fails usability.
  * **REL-ACC-013.VERIF:** `PKG-PHYS` (edge pan margin and speed test).
  * **REL-ACC-013.LANE:** Player Experience.

* **REL-ACC-014 — High-Visibility Mouse Cursor & Click Ring:** The player shall have the option to toggle a `High Visibility Cursor` (2.0x scale) and a high-contrast animated click ring on mouse-down.
  * **REL-ACC-014.AUTH:** Click ring displays green for move, red for attack, blue for ability.
  * **REL-ACC-014.FAIL:** Mouse cursor disappearing against pale terrain fails visual tracking.
  * **REL-ACC-014.VERIF:** `PKG-REND` (high-visibility cursor visual review).
  * **REL-ACC-014.LANE:** Player Experience.

* **REL-ACC-015 — Single-Player Simulation Game Speed Pacing:** Single-player campaign and skirmish shall support adjustable simulation speeds: 0.5x, 0.75x, 1.0x (normal), 1.25x, and 1.5x.
  * **REL-ACC-015.AUTH:** Slowing sim speed allows motor-impaired players more time to issue micro-management orders.
  * **REL-ACC-015.FAIL:** Game speed scaling desynchronizing audio pitch or dropping inputs fails pacing.
  * **REL-ACC-015.VERIF:** `SRC` (game speed scaling simulation test).
  * **REL-ACC-015.LANE:** Core Gameplay & Player Experience.

* **REL-ACC-016 — Accessibility Profile Persistence Across Sessions:** All accessibility settings shall be saved immediately to `Profile.sav` and loaded on cold launch before the title screen renders.
  * **REL-ACC-016.AUTH:** Subtitles and high-contrast settings remain active from the very first prologue cinematic.
  * **REL-ACC-016.FAIL:** Accessibility settings reverting to defaults upon game restart fails compliance.
  * **REL-ACC-016.VERIF:** `PKG-AUTO` (accessibility profile cold boot test).
  * **REL-ACC-016.LANE:** Save Recovery & UI.

* **REL-ACC-017 — Dedicated Top-Level Accessibility Menu Hub:** A dedicated `Accessibility` button shall be present on both the Title Screen and the in-game Pause Menu, providing single-click access to all accessibility options.
  * **REL-ACC-017.AUTH:** Features clear category headers: Vision, Hearing, Motor, and Cognitive.
  * **REL-ACC-017.FAIL:** Burying accessibility options inside obscure submenus fails discoverability.
  * **REL-ACC-017.VERIF:** `PKG-PHYS` (accessibility menu navigation test).
  * **REL-ACC-017.LANE:** Player Experience.

* **REL-ACC-018 — Active Tactical Pause (Single Player):** In single-player campaign, tutorial, and offline skirmish, pressing `Pause` or `P` shall freeze the simulation accumulator while maintaining full interactive control over the camera, selection, unit inspection, and order queueing.
  * **REL-ACC-018.AUTH:** Queued orders display visual waypoint vectors; upon unpausing, units execute queued actions immediately. Audio submix smoothly cross-fades to a low-pass paused acoustic state.
  * **REL-ACC-018.FAIL:** Disabling selection or order issuance while paused in single player fails contemplative accessibility.
  * **REL-ACC-018.VERIF:** `PKG-PHYS` (active tactical pause order queueing test).
  * **REL-ACC-018.LANE:** Player Experience & Core Gameplay.

* **REL-ACC-019 — Continuous Simulation Speed Scaling:** The game speed control slider shall support continuous adjustment from 0.25x (ultra slow-mo) to 2.0x (double speed) in single player, utilizing time-pitch audio preservation algorithms.
  * **REL-ACC-019.AUTH:** Speech and sound effects remain pitch-accurate and intelligible at 0.5x and 1.5x speeds.
  * **REL-ACC-019.FAIL:** Audio pitch warping (chipmunk or demon voice) during speed scaling fails audio accessibility.
  * **REL-ACC-019.VERIF:** `PKG-AUTO` (game speed audio pitch preservation test).
  * **REL-ACC-019.LANE:** Audio & Core Gameplay.

* **REL-ACC-020 — Smart Macro Assist & Auto-Queue Toggle:** The accessibility options shall provide an optional `Smart Macro Assist` preset designed for novice or younger players, automatically training workers from idle Command Cores up to optimal deposit saturation (16 workers).
  * **REL-ACC-020.AUTH:** The assistant pauses production if Matter is critically low (<100) or Logistics is capped. Disabled by default in competitive ranked skirmish.
  * **REL-ACC-020.FAIL:** Assistant building workers beyond deposit saturation fails economic intelligence.
  * **REL-ACC-020.VERIF:** `SRC` + `PKG-AUTO` (smart macro assist worker saturation test).
  * **REL-ACC-020.LANE:** Player Experience & Opponent AI.

* **REL-ACC-021 — Threat Warning Voice Assistant:** An optional spoken accessibility assistant shall audibly narrate critical spatial developments with clear synthesized prompts: *"Hostile force approaching western flank"*, *"Matter deposit depleted"*, *"Future Well contested"*.
  * **REL-ACC-021.AUTH:** Prompts duck ambient music by -6 dB and display high-contrast directional banners.
  * **REL-ACC-021.FAIL:** Silent spatial events resulting in unnoticed base destruction fail novice accessibility.
  * **REL-ACC-021.VERIF:** `PKG-REND` (voice assistant narrative alert review).
  * **REL-ACC-021.LANE:** Audio & Accessibility.

* **REL-ACC-022 — Content Filter & Family Comfort Presets:** The options menu shall provide a `Family & Comfort` preset allowing players to toggle: screen shake intensity (0% to 100%), combat debris violence (full / minimal / clean energy only), and strobe hit flashes.
  * **REL-ACC-022.AUTH:** Setting minimal debris replaces mechanical shattering with clean dissolving energy fades.
  * **REL-ACC-022.FAIL:** Blood or disturbing gore in an all-ages science fantasy game is strictly prohibited.
  * **REL-ACC-022.VERIF:** `PKG-REND` (family comfort preset visual review).
  * **REL-ACC-022.LANE:** Visual Presentation & Accessibility.

* **REL-LOC-001 — Complete String Externalization Standard:** 100% of user-facing text (briefings, tooltips, buttons, subtitles, errors) shall use Unreal `LOCTEXT` or `NSLOCTEXT` macros. Hardcoded string literals in UI code are strictly prohibited.
  * **REL-LOC-001.AUTH:** Automated AST linter scans C++ and Blueprints for raw text literals.
  * **REL-LOC-001.FAIL:** Any un-externalized string literal fails build verification.
  * **REL-LOC-001.VERIF:** `SRC` (`check_string_externalization.py`).
  * **REL-LOC-001.LANE:** Localization & UI.

* **REL-LOC-002 — UTF-8 Unicode Encoding Invariant:** All localization manifest files, string tables, font asset maps, and dialogue subtitles shall be encoded strictly in UTF-8 without byte-order marks (BOM).
  * **REL-LOC-002.AUTH:** Supports international glyph sets without character corruption.
  * **REL-LOC-002.FAIL:** ANSI or non-UTF8 encoding in localization files fails parser tests.
  * **REL-LOC-002.VERIF:** `SRC` (encoding validation script).
  * **REL-LOC-002.LANE:** Localization.

* **REL-LOC-003 — UI Text Container Expansion Margins:** All Slate and UMG text containers shall maintain an authored 30% spatial buffer beyond English text dimensions to allow expansion in languages with longer average word lengths (e.g. German, French).
  * **REL-LOC-003.AUTH:** Text wraps dynamically or scales font size before clipping.
  * **REL-LOC-003.FAIL:** Hardcoded pixel boundaries truncating text fail localization review.
  * **REL-LOC-003.VERIF:** `PKG-REND` (pseudo-localization expansion layout test).
  * **REL-LOC-003.LANE:** Localization & Player Experience.

* **REL-LOC-004 — Automated Localization Translation Extraction Pipeline:** The build system shall provide an automated extraction command (`Scripts/extract_localization.py`) generating standardized PO/CSV translation manifests from source data.
  * **REL-LOC-004.AUTH:** Extraction completes in $\le 10.0	ext{ seconds}$ without requiring full editor launch.
  * **REL-LOC-004.FAIL:** Broken translation extraction pipeline fails release tool requirements.
  * **REL-LOC-004.VERIF:** `SRC` (localization pipeline execution test).
  * **REL-LOC-004.LANE:** Localization & Build.

* **REL-LOC-005 — International Font Fallback & Glyphs:** The UI typography system shall include comprehensive font glyph sets covering ASCII, Latin Extended, Cyrillic, Greek, and CJK ideographs with automatic font fallback.
  * **REL-LOC-005.AUTH:** Missing characters render using fallback fonts rather than square glyph boxes ("tofu").
  * **REL-LOC-005.FAIL:** Tofu boxes rendering in UI text fails visual quality.
  * **REL-LOC-005.VERIF:** `PKG-REND` (font glyph fallback test).
  * **REL-LOC-005.LANE:** Visual Presentation & Localization.

* **REL-LOC-006 — Culturalization & Regional Sensitivity Hygiene:** Game lore, faction names, iconography, and character dialogue shall undergo culturalization review to ensure freedom from offensive religious, political, or regional taboos.
  * **REL-LOC-006.AUTH:** Faction symbols and visual motifs are reviewed and cleared against international registers.
  * **REL-LOC-006.FAIL:** Offensive symbols or culturally prohibited tropes fail release gates.
  * **REL-LOC-006.VERIF:** `SRC` (culturalization checklist audit).
  * **REL-LOC-006.LANE:** Narrative & Governance.

### §22 Graphics Scalability, Performance, and Stability (`REL-PERF-*`, `REL-STAB-*`)

* **REL-PERF-001 — 60 FPS Target on Baseline Apple Silicon:** The application shall maintain a continuous target of 60 frames per second (16.67 ms frame interval) on an Apple M1 Pro processor (16 GB unified memory) at 1920×1080 resolution under standard graphics settings.
  * **REL-PERF-001.AUTH:** Metal render pipeline shall not dip below 55 FPS during normal gameplay.
  * **REL-PERF-001.FAIL:** Sustained frame rate <50 FPS on baseline hardware fails acceptance.
  * **REL-PERF-001.VERIF:** `PKG-AUTO` (automated frame-rate benchmark trace).
  * **REL-PERF-001.LANE:** Performance.

* **REL-PERF-002 — Frame Time Distribution & Spike Ceiling:** In a 10-minute active gameplay benchmark, the 95th percentile (p95) frame time shall not exceed $16.67	ext{ ms}$, the 99th percentile (p99) shall not exceed $22.00	ext{ ms}$, and no single frame spike shall exceed $33.33	ext{ ms}$.
  * **REL-PERF-002.AUTH:** Eliminates perceptible visual hitching and stuttering.
  * **REL-PERF-002.FAIL:** Any single frame exceeding 50.0 ms fails frame pacing criteria.
  * **REL-PERF-002.VERIF:** `PKG-AUTO` (Unreal Engine Insights frame pacing analysis).
  * **REL-PERF-002.LANE:** Performance.

* **REL-PERF-003 — Game Thread Execution Budget:** Total game thread execution time (simulation update, player input sampling, order routing, and view synchronization) shall not exceed $4.00	ext{ ms}$ per frame under a 400-unit combat load.
  * **REL-PERF-003.AUTH:** Leaves $\ge 12.67	ext{ ms}$ headroom for rendering and GPU execution within the 16.67 ms window.
  * **REL-PERF-003.FAIL:** Game thread exceeding 6.0 ms fails performance budget.
  * **REL-PERF-003.VERIF:** `PKG-AUTO` (stat Game profiler run).
  * **REL-PERF-003.LANE:** Performance & Core Gameplay.

* **REL-PERF-004 — Render Thread & GPU Execution Budget:** Render thread time and GPU frame time combined shall not exceed $11.00	ext{ ms}$ per frame at 1080p resolution under standard lighting and shader quality.
  * **REL-PERF-004.AUTH:** Measured on Metal GPU profiler across heavy particle and combat engagements.
  * **REL-PERF-004.FAIL:** GPU time exceeding 13.0 ms fails graphics budget.
  * **REL-PERF-004.VERIF:** `PKG-AUTO` (Metal GPU frame trace).
  * **REL-PERF-004.LANE:** Visual Presentation & Performance.

* **REL-PERF-005 — Volumetric Fog GPU Computation Budget:** Volumetric 3D fog-of-war occlusion shader passes shall execute on the GPU within a strict budget of $\le 1.50	ext{ ms}$ per frame.
  * **REL-PERF-005.AUTH:** Downsampled 3D fog computation with bilateral upsampling ensures full visual coverage within budget.
  * **REL-PERF-005.FAIL:** Fog pass exceeding 2.5 ms fails graphics optimization.
  * **REL-PERF-005.VERIF:** `PKG-AUTO` (GPU shader pass timing).
  * **REL-PERF-005.LANE:** Visual Presentation.

* **REL-PERF-006 — Pathfinding Burst Re-plan Budget:** A simultaneous pathfinding recalculation burst triggered by 50 units (e.g. upon Reshape bridge manifestation or blockage) shall complete within $\le 6.00	ext{ ms}$ total compute time.
  * **REL-PERF-006.AUTH:** Path requests exceeding 6.0 ms are time-sliced across adjacent ticks.
  * **REL-PERF-006.FAIL:** Re-planning burst freezing the game thread for $>10	ext{ ms}$ fails pathfinding criteria.
  * **REL-PERF-006.VERIF:** `SRC` (50-unit path burst benchmark).
  * **REL-PERF-006.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-PERF-007 — Master Performance Budget Compliance Invariant:** The shipping packaged build shall simultaneously satisfy the entire master budget envelope: p95 frame time $\le 16.67	ext{ ms}$, game thread $\le 4.0	ext{ ms}$, render+GPU $\le 11.0	ext{ ms}$, fog $\le 1.5	ext{ ms}$, path burst $\le 6.0	ext{ ms}$, resident memory $\le 10	ext{ GB}$, and save write $\le 250	ext{ ms}$ (binding values).
  * **REL-PERF-007.AUTH:** All budgets verified on the identical packaged release candidate.
  * **REL-PERF-007.FAIL:** Exceeding any single ceiling invalidates release packaging.
  * **REL-PERF-007.VERIF:** `PKG-AUTO` (end-to-end performance qualification suite).
  * **REL-PERF-007.LANE:** Performance & QA.

* **REL-PERF-008 — Resident Memory Ceiling (10 GB RSS):** Total resident set size (RSS) memory of the game process shall not exceed $10.0	ext{ GB}$ at any point during a 60-minute rendered gameplay session with 400 active units.
  * **REL-PERF-008.AUTH:** Prevents memory exhaustion on 16 GB unified memory systems.
  * **REL-PERF-008.FAIL:** Memory usage exceeding 11.0 GB triggers memory leak review.
  * **REL-PERF-008.VERIF:** `PKG-AUTO` (RSS memory tracking over 60 minutes).
  * **REL-PERF-008.LANE:** Performance.

* **REL-PERF-009 — VRAM Texture Streaming & Unified Memory Management:** Texture streaming pools shall dynamically manage VRAM usage, clamping loaded mipmaps to fit within available unified memory without swapping to disk.
  * **REL-PERF-009.AUTH:** Mipmaps stream smoothly as camera zooms without hitching.
  * **REL-PERF-009.FAIL:** Disk swap thrashing causing severe frame drops fails streaming budget.
  * **REL-PERF-009.VERIF:** `PKG-AUTO` (texture streaming pool assert).
  * **REL-PERF-009.LANE:** Visual Presentation & Performance.

* **REL-PERF-010 — 400-Unit Four-Team Stress Protocol:** The game shall execute a standardized 400-unit stress test (100 units per team across 4 AI teams in active combat) maintaining $\ge 45	ext{ FPS}$ on baseline hardware (binding protocol).
  * **REL-PERF-010.AUTH:** Verified using automated headless-to-rendered harness `run_stress_benchmark.sh`.
  * **REL-PERF-010.FAIL:** Crash, memory exhaustion, or drop below 30 FPS fails qualification.
  * **REL-PERF-010.VERIF:** `PKG-AUTO` (400-unit stress benchmark receipt).
  * **REL-PERF-010.LANE:** Performance & QA.

* **REL-PERF-011 — 600-Second Preflight & 60-Minute Rendered Soak:** Prior to release qualification, the build candidate shall pass a 600-second automated preflight test followed by a 60-minute continuous rendered skirmish soak without crash or memory leak (binding protocol).
  * **REL-PERF-011.AUTH:** Verifies long-term simulation and rendering stability.
  * **REL-PERF-011.FAIL:** Any crash or performance degradation $>15\%$ over 60 minutes fails soak.
  * **REL-PERF-011.VERIF:** `PKG-AUTO` (60-minute soak test log).
  * **REL-PERF-011.LANE:** Performance & QA.

* **REL-PERF-012 — Headless Multi-Hour AI Soak Validation:** The simulation core shall run a multi-hour automated headless batch test (minimum 1,000 matches) without encountering an unhandled assertion, deadlock, or determinism desync (binding protocol).
  * **REL-PERF-012.AUTH:** Verified via `run_ai_balance_matrix.py` across all maps.
  * **REL-PERF-012.FAIL:** Any crashed or hanging match in 1,000 runs fails the soak gate.
  * **REL-PERF-012.VERIF:** `PKG-AUTO` (1,000-match headless soak report).
  * **REL-PERF-012.LANE:** Opponent AI & Core Gameplay.

* **REL-PERF-013 — Dynamic Graphics Scalability Presets:** The video options shall provide four discrete graphics presets: Low, Medium, High, and Ultra, configuring shadow quality, view distance, post-processing, and particle density.
  * **REL-PERF-013.AUTH:** Low preset enables smooth 60 FPS on base M1 MacBook Air (8 GB memory).
  * **REL-PERF-013.FAIL:** Changing presets requiring game restart fails usability.
  * **REL-PERF-013.VERIF:** `PKG-PHYS` (graphics preset switching test).
  * **REL-PERF-013.LANE:** Visual Presentation.

* **REL-PERF-014 — Metal API Shader Precompilation Pipeline:** All Metal shaders, compute kernels, and material PSO permutations shall be precompiled into a packaged Metal library (`MetalBinaryArchive`) during build packaging.
  * **REL-PERF-014.AUTH:** Zero on-demand runtime shader compilation hitches during active gameplay.
  * **REL-PERF-014.FAIL:** Mid-combat hitching caused by compiling new shaders fails graphics criteria.
  * **REL-PERF-014.VERIF:** `PKG-AUTO` (Metal shader cache precompilation audit).
  * **REL-PERF-014.LANE:** Build Distribution & Visual.

* **REL-PERF-015 — Passive Thermal Throttling Resilience:** On passively cooled Apple Silicon systems (MacBook Air), the engine shall gracefully adapt frame pacing when thermal pressure reaches `Heavy`, reducing background dynamic resolution to maintain $\ge 40	ext{ FPS}$ without freezing.
  * **REL-PERF-015.AUTH:** Thermal notifications are polled cleanly via standard macOS APIs.
  * **REL-PERF-015.FAIL:** Hard freeze or kernel panic under sustained thermal load fails acceptance.
  * **REL-PERF-015.VERIF:** `PKG-AUTO` (thermal pressure simulation test).
  * **REL-PERF-015.LANE:** Performance.

* **REL-PERF-016 — Headless Simulation Execution Throughput:** When running headless without rendering or frame rate capping, the simulation engine shall process ticks at $\ge 50	imes$ real-time speed (minimum 1,000 simulation ticks per second on a single M1 Pro core).
  * **REL-PERF-016.AUTH:** Enables fast automated balance verification and continuous regression testing.
  * **REL-PERF-016.FAIL:** Headless simulation speed $<30	imes$ fails automation criteria.
  * **REL-PERF-016.VERIF:** `SRC` (headless tick throughput benchmark).
  * **REL-PERF-016.LANE:** Core Gameplay & Build.

* **REL-PERF-017 — Asynchronous Worker Thread Work Scheduler:** The engine shall utilize a task-graph worker thread pool for pathfinding, audio mixing, and state serialization, avoiding core contention and priority inversion.
  * **REL-PERF-017.AUTH:** Worker tasks do not starve the main simulation thread.
  * **REL-PERF-017.FAIL:** Worker thread deadlocks or priority inversions fail thread safety.
  * **REL-PERF-017.VERIF:** `SRC` (thread scheduler contention test).
  * **REL-PERF-017.LANE:** Performance & Core Gameplay.

* **REL-PERF-018 — Zero Mid-Frame Heap Allocation Invariant:** Within the 20 Hz simulation tick loop (`Simulation::Tick`), dynamic heap memory allocation (`malloc`, `new`) is strictly prohibited. All transient structures shall use preallocated arena buffers.
  * **REL-PERF-018.AUTH:** Completely eliminates heap fragmentation and garbage collection spikes during matches.
  * **REL-PERF-018.FAIL:** Any heap allocation inside the active tick loop fails static AST checks.
  * **REL-PERF-018.VERIF:** `SRC` (memory allocation instrumentation test).
  * **REL-PERF-018.LANE:** Core Gameplay (`EchoesSimCore`).

* **REL-STAB-001 — Sixty-Minute Sustained Gameplay Stability:** The packaged build shall complete a continuous 60-minute rendered gameplay session without a crash, unhandled exception, assertion failure, or GPU hang.
  * **REL-STAB-001.AUTH:** Verified under human or automated replay execution.
  * **REL-STAB-001.FAIL:** Any crash during the 60-minute window is a fatal S0 defect.
  * **REL-STAB-001.VERIF:** `PKG-AUTO` (60-minute continuous run log).
  * **REL-STAB-001.LANE:** Independent QA.

* **REL-STAB-002 — Match Restart Memory Leak Invariance:** Restarting a skirmish match or loading a campaign mission 10 consecutive times within a single game session shall result in net RSS memory growth of $\le 50.0	ext{ MB}$.
  * **REL-STAB-002.AUTH:** All entity memory, path grids, and textures must be cleanly released upon match termination.
  * **REL-STAB-002.FAIL:** Unbounded memory growth across match restarts fails stability.
  * **REL-STAB-002.VERIF:** `PKG-AUTO` (10-match restart memory leak harness).
  * **REL-STAB-002.LANE:** Performance & Save Recovery.

* **REL-STAB-003 — Graceful Crash Minidump Capture:** In the event of an unrecoverable operating system exception, the crash handler shall capture an anonymous minidump (`.dmp`) and write an error diagnostic log without corrupting existing saves or user data.
  * **REL-STAB-003.AUTH:** Crash dialog allows user to review crash log location before exiting.
  * **REL-STAB-003.FAIL:** Silent disappearance of process without crash log fails supportability.
  * **REL-STAB-003.VERIF:** `SRC` (crash handler trap test).
  * **REL-STAB-003.LANE:** Build Distribution.

* **REL-STAB-004 — Clean Process Termination Invariant:** Selecting Quit or issuing an OS terminate signal (`SIGTERM`, `Cmd+Q`) shall exit the application cleanly within 2.0 seconds, terminating all background threads and releasing all file locks.
  * **REL-STAB-004.AUTH:** Leaves zero hanging child processes or zombie threads.
  * **REL-STAB-004.FAIL:** App hanging in dock or requiring Force Quit fails release hygiene.
  * **REL-STAB-004.VERIF:** `PKG-PHYS` (clean shutdown test).
  * **REL-STAB-004.LANE:** Build Distribution.

* **REL-STAB-005 — Display & Cursor State Restoration on Exit:** Unexpected application exit or crash shall restore desktop screen resolution, refresh rate, and OS hardware cursor visibility within 1.0 second.
  * **REL-STAB-005.AUTH:** The user's desktop environment is never left with an invisible mouse cursor or skewed resolution.
  * **REL-STAB-005.FAIL:** Leaving cursor hidden after app exit fails OS integration.
  * **REL-STAB-005.VERIF:** `SRC` (crash exit display recovery test).
  * **REL-STAB-005.LANE:** Build Distribution.

---

### §23 Security, Privacy, Packaging, and Distribution (`REL-DIST-*`, `REL-SEC-*`)

* **REL-DIST-001 — Standalone macOS Application Bundle:** The game shall package as a self-contained macOS `.app` bundle (`EchoesOfTheBrokenSun.app`) conforming strictly to Apple File System bundle standards.
  * **REL-DIST-001.AUTH:** Contains complete executable, frameworks, data packs, and resource catalogs.
  * **REL-DIST-001.FAIL:** Requiring external dependencies in `/usr/local/` or Homebrew fails packaging.
  * **REL-DIST-001.VERIF:** `SRC` (bundle structure verification script).
  * **REL-DIST-001.LANE:** Build Distribution.

* **REL-DIST-002 — Apple Silicon ARM64 Native Architecture:** The primary executable and all bundled dynamic libraries shall be compiled natively for ARM64 (Apple Silicon), with zero requirement for Rosetta 2 emulation.
  * **REL-DIST-002.AUTH:** Verified via `lipo -info` and `file` commands.
  * **REL-DIST-002.FAIL:** Shipping x86_64-only binaries requiring Rosetta emulation fails release criteria.
  * **REL-DIST-002.VERIF:** `SRC` (binary architecture audit).
  * **REL-DIST-002.LANE:** Build Distribution.

* **REL-DIST-003 — Apple Developer ID Code Signing:** All Mach-O binaries, frameworks, and dylibs within the app bundle shall be signed with a valid, non-expired Apple Developer ID Application certificate with secure timestamping.
  * **REL-DIST-003.AUTH:** Verified via `codesign --verify --deep --strict --verbose=2`.
  * **REL-DIST-003.FAIL:** Unsigned binaries or signature verification errors fail packaging.
  * **REL-DIST-003.VERIF:** `PKG-AUTO` (codesign verification assertion).
  * **REL-DIST-003.LANE:** Build Distribution.

* **REL-DIST-004 — Apple Notarization Service Qualification:** The packaged application and disk image shall be successfully uploaded, evaluated, and notarized by Apple's Notary service with zero security warnings or rejections.
  * **REL-DIST-004.AUTH:** Notarization log returns `status: "Accepted"` with valid submission ID.
  * **REL-DIST-004.FAIL:** Notarization rejection or Gatekeeper warnings fail release delivery.
  * **REL-DIST-004.VERIF:** `PKG-AUTO` (Apple notarytool status check).
  * **REL-DIST-004.LANE:** Build Distribution.

* **REL-DIST-005 — Cryptographic Ticket Stapling:** The notarization ticket shall be stapled directly to the `.app` bundle and the distribution `.dmg` installer using `xcrun stapler staple`.
  * **REL-DIST-005.AUTH:** Verified via `xcrun stapler validate`; allows Gatekeeper verification when machine is offline.
  * **REL-DIST-005.FAIL:** Missing staple ticket requiring online Gatekeeper ping on first run fails packaging.
  * **REL-DIST-005.VERIF:** `PKG-AUTO` (stapler validation check).
  * **REL-DIST-005.LANE:** Build Distribution.

* **REL-DIST-006 — Read-Only Compressed DMG Distribution Installer:** The game shall be delivered as a signed, read-only compressed UDZO disk image (`EchoesOfTheBrokenSun-1.0.0.dmg`) featuring an authored layout with an `/Applications` drag-and-drop link.
  * **REL-DIST-006.AUTH:** User installs the game by simply dragging the app icon to `/Applications`.
  * **REL-DIST-006.FAIL:** Shipping raw uncompressed folders or complex installer wizards fails delivery.
  * **REL-DIST-006.VERIF:** `PKG-AUTO` (DMG mount and layout verification).
  * **REL-DIST-006.LANE:** Build Distribution.

* **REL-DIST-007 — Application Branding & Metadata Completeness:** The app bundle `Info.plist` shall contain complete metadata: bundle identifier `com.angelispseftis.echoesofthebrokensun`, semantic version `1.0.0`, copyright notice, high-resolution retina icon (`AppIcon.icns`), and human-readable name.
  * **REL-DIST-007.AUTH:** High-resolution icons render crisply in macOS Finder and Dock.
  * **REL-DIST-007.FAIL:** Missing icon or default Unreal Engine bundle metadata fails branding review.
  * **REL-DIST-007.VERIF:** `SRC` (Info.plist metadata audit).
  * **REL-DIST-007.LANE:** Build Distribution.

* **REL-DIST-008 — Seamless Clean-Machine Gatekeeper Launch:** The installed application shall launch on a clean, factory-reset macOS installation without triggering security warnings, quarantine blocks, or requiring terminal overrides (`xattr -d com.apple.quarantine`) (resolving C40).
  * **REL-DIST-008.AUTH:** Double-clicking the app in `/Applications` opens the game immediately.
  * **REL-DIST-008.FAIL:** Any "App is damaged and cannot be opened" Gatekeeper popup is a fatal defect.
  * **REL-DIST-008.VERIF:** `PKG-PHYS` (clean machine launch verification).
  * **REL-DIST-008.LANE:** Build Distribution & QA.

* **REL-DIST-009 — Complete Dynamic Library Self-Containment:** All third-party libraries and runtime dependencies shall be bundled within `@rpath/Contents/Frameworks/`, with install names rewritten relative to `@executable_path`.
  * **REL-DIST-009.AUTH:** Verified via `otool -L` on all Mach-O binaries.
  * **REL-DIST-009.FAIL:** Hardcoded absolute paths (e.g. `/Users/...` or `/opt/...`) fail build verification.
  * **REL-DIST-009.VERIF:** `SRC` (otool dependency path scan).
  * **REL-DIST-009.LANE:** Build Distribution.

* **REL-DIST-010 — Strict File System Permission Sandboxing:** The game shall execute with standard unprivileged user permissions, never requiring administrative privileges (`sudo`) to install, run, or write saves.
  * **REL-DIST-010.AUTH:** Modifying system files outside `Application Support` is strictly prohibited.
  * **REL-DIST-010.FAIL:** App requesting root password or admin prompt fails security audit.
  * **REL-DIST-010.VERIF:** `PKG-AUTO` (unprivileged execution test).
  * **REL-DIST-010.LANE:** Build Distribution & Security.

* **REL-DIST-011 — Distribution Package Compression Budget:** The distribution disk image (.dmg) file size shall not exceed $15.0	ext{ GB}$, with uncompressed installed footprint not exceeding $25.0	ext{ GB}$.
  * **REL-DIST-011.AUTH:** Asset catalogs are packed using LZ4/Zstandard compression.
  * **REL-DIST-011.FAIL:** Package size exceeding 18 GB fails distribution budget.
  * **REL-DIST-011.VERIF:** `PKG-AUTO` (DMG file size assertion).
  * **REL-DIST-011.LANE:** Build Distribution.

* **REL-DIST-012 — Clean Uninstallation & Zero System Debris:** Deleting `EchoesOfTheBrokenSun.app` from `/Applications` and removing its save directory in `Application Support` shall restore the host machine to a clean state with zero lingering background daemons or kernel extensions.
  * **REL-DIST-012.AUTH:** App does not install persistent background agents or launch daemons.
  * **REL-DIST-012.FAIL:** Lingering background daemons running after deletion fails macOS standards.
  * **REL-DIST-012.VERIF:** `SRC` (uninstallation system clean check).
  * **REL-DIST-012.LANE:** Build Distribution.

* **REL-DIST-013 — Automated Package Seal & SHA-256 Digest:** Packaging completion shall generate an authoritative cryptographic seal file (`package_seal.json`) recording the Git commit SHA, build timestamp, binary SHA-256, and DMG SHA-256.
  * **REL-DIST-013.AUTH:** Seal file is archived in `WorkstreamControl/evidence/release/`.
  * **REL-DIST-013.FAIL:** Distributing packages without cryptographic seal file is prohibited.
  * **REL-DIST-013.VERIF:** `PKG-AUTO` (seal generation check).
  * **REL-DIST-013.LANE:** Build Distribution.

* **REL-DIST-014 — Headless Automated Packaging Build Pipeline:** The entire distribution packaging process (compilation, bundling, signing, notarization, stapling, DMG creation) shall execute via a single headless script (`Scripts/package_release.sh`).
  * **REL-DIST-014.AUTH:** Packaging requires zero manual interactive steps.
  * **REL-DIST-014.FAIL:** Manual ad-hoc packaging steps fail reproducibility criteria.
  * **REL-DIST-014.VERIF:** `SRC` (packaging script execution test).
  * **REL-DIST-014.LANE:** Build Distribution.

* **REL-DIST-015 — Multi-Version macOS OS Compatibility:** The application shall run reliably across macOS 13 (Ventura), macOS 14 (Sonoma), and macOS 15 (Sequoia), using backward-compatible deployment target settings (`MACOSX_DEPLOYMENT_TARGET=13.0`).
  * **REL-DIST-015.AUTH:** Verified on clean test machines running each supported macOS version.
  * **REL-DIST-015.FAIL:** App failing to launch on macOS 13 fails compatibility criteria.
  * **REL-DIST-015.VERIF:** `PKG-PHYS` (multi-OS version verification).
  * **REL-DIST-015.LANE:** Build Distribution & QA.

* **REL-DIST-016 — 100% Offline DRM-Free Execution:** The packaged game shall execute completely offline without requiring internet connectivity, third-party launchers, account creation, or online DRM verification.
  * **REL-DIST-016.AUTH:** Full campaign and skirmish modes remain 100% accessible with network hardware disabled.
  * **REL-DIST-016.FAIL:** Blocking game startup due to missing network connection is strictly prohibited.
  * **REL-DIST-016.VERIF:** `PKG-PHYS` (offline flight-mode execution test).
  * **REL-DIST-016.LANE:** Build Distribution & Player Experience.

* **REL-DIST-017 — Patching & Delta Update Architecture Readiness:** Data package files (`.pak`) shall be authored in modular chunks (Audio, Textures, Meshes, CoreData), enabling small delta updates without requiring redownload of the full 15 GB package.
  * **REL-DIST-017.AUTH:** Patching core code or balance data requires updating a $<50	ext{ MB}$ data chunk.
  * **REL-DIST-017.FAIL:** Monolithic single-file data blob preventing delta patching fails maintainability.
  * **REL-DIST-017.VERIF:** `SRC` (modular pak layout audit).
  * **REL-DIST-017.LANE:** Build Distribution.

* **REL-SEC-001 — Memory Safety & Buffer Bounds Discipline:** All C++ simulation buffers, entity arrays, and network buffers shall enforce bounds checking. The core shall compile and pass AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) with zero findings.
  * **REL-SEC-001.AUTH:** ASan/UBSan suites run continuously on CI.
  * **REL-SEC-001.FAIL:** Any out-of-bounds read/write or undefined behavior halts release qualification.
  * **REL-SEC-001.VERIF:** `SRC` (`test_sim_sanitizers.sh`).
  * **REL-SEC-001.LANE:** Security & Core Gameplay.

* **REL-SEC-002 — Path Traversal Attack Refusal:** File loading and save routines shall sanitize all user-supplied paths, rejecting directory traversal sequences (`../`, `..\`) and absolute path escapes.
  * **REL-SEC-002.AUTH:** Any path attempting to escape `~/Library/Application Support/` throws a fatal security exception.
  * **REL-SEC-002.FAIL:** Path traversal reading arbitrary system files is a fatal vulnerability.
  * **REL-SEC-002.VERIF:** `SRC` (path sanitization security unit tests).
  * **REL-SEC-002.LANE:** Security & Save Recovery.

* **REL-SEC-003 — Zero Telemetry & Player Privacy Protection:** The application shall transmit zero tracking beacons, analytics packets, or telemetry to external servers. The binary shall contain zero tracking SDKs.
  * **REL-SEC-003.AUTH:** Network socket activity during single-player play is exactly 0 bytes.
  * **REL-SEC-003.FAIL:** Covert telemetry or data collection violates privacy mandate.
  * **REL-SEC-003.VERIF:** `SRC` + `PKG-AUTO` (network packet sniff audit).
  * **REL-SEC-003.LANE:** Security & Governance.

* **REL-SEC-004 — macOS Hardened Runtime Compliance:** The application bundle shall be signed with Apple's Hardened Runtime enabled, enforcing runtime code integrity and preventing dynamic library injection.
  * **REL-SEC-004.AUTH:** Entitlements declare only necessary permissions.
  * **REL-SEC-004.FAIL:** Disabling Hardened Runtime to bypass code signing fails security gates.
  * **REL-SEC-004.VERIF:** `SRC` (hardened runtime entitlement check).
  * **REL-SEC-004.LANE:** Build Distribution & Security.

* **REL-SEC-005 — Immutable Read-Only App Bundle Integrity:** At runtime, the application bundle in `/Applications/EchoesOfTheBrokenSun.app` shall be treated as strictly read-only.
  * **REL-SEC-005.AUTH:** The game process shall never attempt to write logs, temp files, or caches into its own app bundle.
  * **REL-SEC-005.FAIL:** Runtime self-mutation of the bundle fails macOS security guidelines.
  * **REL-SEC-005.VERIF:** `SRC` (bundle read-only assertion test).
  * **REL-SEC-005.LANE:** Build Distribution.

* **REL-SEC-006 — Sanitized Crash Dumps & Zero Memory Leakage:** Crash minidumps shall be strictly sanitized, stripping environment variables, personal account names, and user heap contents, retaining only stack traces and register states.
  * **REL-SEC-006.AUTH:** Dump files are safe for public bug tracking submission.
  * **REL-SEC-006.FAIL:** Crash dumps containing sensitive memory data fails privacy review.
  * **REL-SEC-006.VERIF:** `SRC` (minidump sanitation audit).
  * **REL-SEC-006.LANE:** Security.

---

### §24 Public Website, Manual, Claims, and Support (`REL-PUB-*`)

* **REL-PUB-001 — Absolute Truth in Public Gameplay Claims:** All claims made in promotional copy, manual text, and store listings shall be 100% verified against playable capabilities in the frozen release candidate.
  * **REL-PUB-001.AUTH:** Claiming features that do not exist or are incomplete in 1.0 is strictly prohibited.
  * **REL-PUB-001.FAIL:** Misleading claims or overpromising fails governance review.
  * **REL-PUB-001.VERIF:** `SRC` (public claims factual alignment audit).
  * **REL-PUB-001.LANE:** Public Front Door & Governance.

* **REL-PUB-002 — Comprehensive Standalone Game Manual (PDF):** The distribution disk image shall include a professionally formatted PDF game manual (`Manual.pdf`) accessible directly from the disk image and game menu.
  * **REL-PUB-002.AUTH:** Formatted cleanly with typography, illustrations, and index.
  * **REL-PUB-002.FAIL:** Shipping a commercial game without an operational manual fails release quality.
  * **REL-PUB-002.VERIF:** `SRC` (manual PDF verification).
  * **REL-PUB-002.LANE:** Public Front Door & Narrative.

* **REL-PUB-003 — Authoritative Manual Content Architecture:** The game manual shall contain comprehensive sections: World Lore & Setting, Faction Identities & Rosters, Future Well Protocols, Controls & Keybindings, Economy & Logistics, Combat Mechanics, and Troubleshooting.
  * **REL-PUB-003.AUTH:** Numerical stats in the manual match runtime data tables 1:1.
  * **REL-PUB-003.FAIL:** Discrepancies between manual stats and in-game values fail verification.
  * **REL-PUB-003.VERIF:** `SRC` (manual data parity check).
  * **REL-PUB-003.LANE:** Public Front Door & Content.

* **REL-PUB-004 — Official Public Product Website:** A production public website shall be maintained, presenting game background, screenshots, system requirements, manual download, and direct contact support.
  * **REL-PUB-004.AUTH:** Website renders responsively across desktop and mobile viewports.
  * **REL-PUB-004.FAIL:** Broken links or outdated screenshots on public site fail release standards.
  * **REL-PUB-004.VERIF:** `SRC` (website responsive validation).
  * **REL-PUB-004.LANE:** Public Front Door.

* **REL-PUB-005 — Truthful Hardware System Requirements:** The manual and website shall publish truthful minimum and recommended hardware configurations based on empirical testing (Minimum: Apple M1, 8 GB memory; Recommended: Apple M1 Pro, 16 GB memory).
  * **REL-PUB-005.AUTH:** Verified performance aligns with stated hardware bands.
  * **REL-PUB-005.FAIL:** Listing hardware specifications that fail to achieve 30 FPS fails accuracy.
  * **REL-PUB-005.VERIF:** `SRC` (hardware requirement benchmark alignment check).
  * **REL-PUB-005.LANE:** Public Front Door & Performance.

* **REL-PUB-006 — Direct Support Contact & Issue Reporting:** The website and manual shall provide an active, monitored support contact address (`support@echoesofthebrokensun.com`) and clear instructions for submitting bug reports.
  * **REL-PUB-006.AUTH:** Support workflow is documented with expected turnaround times.
  * **REL-PUB-006.FAIL:** Dead-end support links or non-existent contact emails fails release criteria.
  * **REL-PUB-006.VERIF:** `SRC` (support link validation).
  * **REL-PUB-006.LANE:** Public Front Door.

* **REL-PUB-007 — Third-Party Open Source Software Disclosures:** The manual, website, and in-game Credits screen shall include complete third-party open source notices and license texts (`CREDITS.md`) complying with all license obligations (e.g. MIT, BSD, Apache 2.0).
  * **REL-PUB-007.AUTH:** All bundled dependencies and models are properly credited.
  * **REL-PUB-007.FAIL:** Missing open-source license disclosures is a legal and release blocker.
  * **REL-PUB-007.VERIF:** `SRC` (license compliance audit).
  * **REL-PUB-007.LANE:** Governance & Public Front Door.

* **REL-PUB-008 — Copyright & Trademark Notices:** All public materials, packaging, app bundle metadata, and title screens shall display the authoritative copyright notice: `Copyright © 2026 Angelis Pseftis. All rights reserved.`
  * **REL-PUB-008.AUTH:** Legal ownership is explicitly attributed in all contexts.
  * **REL-PUB-008.FAIL:** Misattributed or missing copyright notices fail legal review.
  * **REL-PUB-008.VERIF:** `SRC` (copyright notice text audit).
  * **REL-PUB-008.LANE:** Governance.

* **REL-PUB-009 — Public Accessibility Feature Documentation:** The website and manual shall feature a dedicated Accessibility guide detailing colorblind modes, subtitle options, keyboard remapping, and mouse-only playability.
  * **REL-PUB-009.AUTH:** Enables disabled gamers to verify game compatibility prior to download.
  * **REL-PUB-009.FAIL:** Failing to disclose accessibility features publicly fails accessibility standards.
  * **REL-PUB-009.VERIF:** `SRC` (accessibility guide verification).
  * **REL-PUB-009.LANE:** Public Front Door & Accessibility.

* **REL-PUB-010 — Version Release Notes & Changelog:** Every public build release shall be accompanied by public release notes detailing version numbers, major additions, balance changes, and bug fixes.
  * **REL-PUB-010.AUTH:** Release notes link directly back to tracked Git releases.
  * **REL-PUB-010.FAIL:** Unannounced silent releases without changelogs fail release discipline.
  * **REL-PUB-010.VERIF:** `SRC` (release notes review).
  * **REL-PUB-010.LANE:** Public Front Door & QA.

* **REL-PUB-011 — In-Engine Truthful Media Assets:** 100% of public marketing screenshots and video trailers shall be captured directly in-engine from the running game build, with zero pre-rendered "target renders" or misleading CGI.
  * **REL-PUB-011.AUTH:** Screenshots reflect actual gameplay fidelity and HUD presentation.
  * **REL-PUB-011.FAIL:** Fabricated gameplay screenshots fail truth-in-marketing standards.
  * **REL-PUB-011.VERIF:** `SRC` + `OWNER` (media provenance verification).
  * **REL-PUB-011.LANE:** Public Front Door & Visual.

* **REL-PUB-012 — Community Code of Conduct & Fair Play Rules:** The public website shall publish a clear Code of Conduct outlining community rules, harassment prohibition, and fair play expectations.
  * **REL-PUB-012.AUTH:** Governs public discussion boards and community interaction.
  * **REL-PUB-012.FAIL:** Absence of a community conduct policy fails modern release standards.
  * **REL-PUB-012.VERIF:** `SRC` (conduct policy review).
  * **REL-PUB-012.LANE:** Public Front Door.

* **REL-PUB-013 — Transparent Zero-Collection Privacy Policy:** The website shall host a concise Privacy Policy explicitly affirming that *Echoes of the Broken Sun* collects zero personal data, transmits zero telemetry, and respects player privacy completely.
  * **REL-PUB-013.AUTH:** Legally compliant with GDPR, CCPA, and global privacy standards.
  * **REL-PUB-013.FAIL:** Ambiguous or boilerplate data-harvesting privacy policies fail privacy mandate.
  * **REL-PUB-013.VERIF:** `SRC` (privacy policy legal check).
  * **REL-PUB-013.LANE:** Public Front Door & Governance.

* **REL-PUB-014 — Public Bug Report Template & Submission Guide:** The website and manual shall provide a structured bug report template guiding players to report: issue description, steps to reproduce, hardware specifications, and attach the sanitized minidump/log file.
  * **REL-PUB-014.AUTH:** Ensures submitted user reports contain actionable technical data.
  * **REL-PUB-014.FAIL:** Lack of bug reporting guidance fails post-launch supportability.
  * **REL-PUB-014.VERIF:** `SRC` (bug template audit).
  * **REL-PUB-014.LANE:** Public Front Door & QA.

* **REL-PUB-015 — Digital Signature Verification Guide:** The public website shall publish the official SHA-256 cryptographic checksums for all public distribution DMGs, along with simple instructions for verifying disk image integrity using macOS Terminal (`shasum -a 256`).
  * **REL-PUB-015.AUTH:** Empowers security-conscious users to verify package authenticity.
  * **REL-PUB-015.FAIL:** Missing public checksums fails security transparency.
  * **REL-PUB-015.VERIF:** `SRC` (checksum publication check).
  * **REL-PUB-015.LANE:** Public Front Door & Build Distribution.

* **REL-PUB-016 — The Soryn Archive In-Game Lore Codex:** The main menu and pause screen shall host **The Soryn Archive**, an interactive illustrated compendium containing discoverable entries across: The Crownfall, Faction Histories, Planetary Geology, and Technological Principles.
  * **REL-PUB-016.AUTH:** Entries unlock dynamically as players complete campaign missions, investigate map landmarks, and win skirmishes.
  * **REL-PUB-016.FAIL:** Codex locked behind paywalls or missing readable typography fails immersion.
  * **REL-PUB-016.VERIF:** `PKG-PHYS` (lore codex navigation and unlock test).
  * **REL-PUB-016.LANE:** Narrative & Public Front Door.

* **REL-PUB-017 — Interactive 3D Model Viewer & Tactical Dossier:** The archive shall include an interactive 3D model viewer allowing players to inspect all 24 units and structures with 360-degree rotation, wireframe toggles, armor penetration zones, and component breakdowns.
  * **REL-PUB-017.AUTH:** Features high-resolution asset inspection with lighting controls.
  * **REL-PUB-017.FAIL:** Model viewer crashing on live GPU render fails presentation.
  * **REL-PUB-017.VERIF:** `PKG-PHYS` (3D model viewer inspection test).
  * **REL-PUB-017.LANE:** Visual Presentation & Player Experience.

* **REL-PUB-018 — Tactical Combat Sandbox / Testing Lab Mode:** Players shall access a dedicated "Combat Lab" sandbox map where they can freely spawn arbitrary unit groups, assign factions, configure upgrades, and measure damage output against test dummies.
  * **REL-PUB-018.AUTH:** Provides real-time DPS meters, time-to-kill graphs, and projectile trajectory visualization.
  * **REL-PUB-018.FAIL:** Forcing competitive players to test balance in live matches without a sandbox fails community tools.
  * **REL-PUB-018.VERIF:** `PKG-AUTO` (combat lab spawning and DPS measurement test).
  * **REL-PUB-018.LANE:** Core Gameplay & QA.

* **REL-PUB-019 — Historical Trophy Vault & Feat Tracking Gallery:** An in-game Trophy Vault shall display 40+ authored campaign deeds, difficulty achievements, and strategic milestones (e.g. *Witness to the Crownfall*, *Master of the Sift*, *Unbroken Sun*).
  * **REL-PUB-019.AUTH:** Feats award commemorative visual badges and profile showcase banners.
  * **REL-PUB-019.FAIL:** Feat progress resetting upon game restart fails profile persistence.
  * **REL-PUB-019.VERIF:** `PKG-AUTO` (trophy vault progress tracking test).
  * **REL-PUB-019.LANE:** Player Experience & Save Recovery.

* **REL-PUB-020 — Transparent In-Game Combat Mechanics Glossary:** The game shall include a fully transparent in-game mathematical manual detailing exact formulas for: damage calculation, armor mitigation, speed normalisation, construction assist scaling, and gather rates.
  * **REL-PUB-020.AUTH:** Ensures competitive players have complete, truthful access to underlying deterministic game mechanics.
  * **REL-PUB-020.FAIL:** Hidden combat modifiers or undisclosed statistical penalties fail competitive fairness.
  * **REL-PUB-020.VERIF:** `SRC` (mechanics glossary data alignment audit).
  * **REL-PUB-020.LANE:** Public Front Door & Core Gameplay.

---

### §25 QA, Human Validation, and Release Blockers (`REL-QA-*`)

* **REL-QA-001 — Rigorous Defect Severity Ladder:** All bugs shall be classified under the strict five-tier severity scale: S0 (Catastrophic: crash, freeze, data loss, security vulnerability), S1 (Blocker: objective soft-lock, broken progression, severe balance exploit), S2 (Major: major visual/audio glitch, degraded performance, significant usability issue), S3 (Minor: minor visual/text flaw with functional workaround), S4 (Trivial: cosmetic typo, minor polish item).
  * **REL-QA-001.AUTH:** Classification governs packaging eligibility.
  * **REL-QA-001.FAIL:** Misclassifying an S0/S1 crash as S3 fails QA discipline.
  * **REL-QA-001.VERIF:** `SRC` (defect triage register audit).
  * **REL-QA-001.LANE:** Independent QA.

* **REL-QA-002 — Zero S0/S1 Release Prohibitions:** A candidate package shall contain exactly ZERO open S0 and ZERO open S1 defects to be authorized for release packaging.
  * **REL-QA-002.AUTH:** Any open S0 or S1 defect immediately blocks candidate promotion.
  * **REL-QA-002.FAIL:** Shipping with known crashes or soft-locks is strictly prohibited.
  * **REL-QA-002.VERIF:** `SRC` (open defect query in `ProjectLedger.md`).
  * **REL-QA-002.LANE:** Independent QA.

* **REL-QA-003 — Zero Un-Waived S2 Critical Path Defects:** A candidate package shall have zero un-waived S2 defects on the release-critical path. Any S2 exception requires explicit written justification signed by Angelis Pseftis.
  * **REL-QA-003.AUTH:** Critical path includes: campaign completion, skirmish victory, tutorial, settings.
  * **REL-QA-003.FAIL:** Unapproved S2 defects on critical path block release.
  * **REL-QA-003.VERIF:** `SRC` (S2 waiver registry audit).
  * **REL-QA-003.LANE:** Independent QA & Owner.

* **REL-QA-004 — Authoritative Defect Register Ledger:** All discovered defects shall be logged in `Docs/Archive/ProjectLedger.md` with: unique defect ID, severity, affected requirement ID, reproduction steps, expected behavior, actual behavior, and resolution status.
  * **REL-QA-004.AUTH:** No defects shall be tracked informally in external ephemeral chats.
  * **REL-QA-004.FAIL:** Untracked bugs found in release testing fails QA governance.
  * **REL-QA-004.VERIF:** `SRC` (defect register ledger validation).
  * **REL-QA-004.LANE:** Independent QA.

* **REL-QA-005 — 85% Simulation Unit Test Coverage Ceiling:** Native C++ unit tests in `EchoesSimCore` shall achieve $\ge 85.0\%$ line and branch coverage across all core classes: `Simulation`, `Entity`, `Movement`, `Combat`, `Economy`, `FogOfWar`, `Pathfinding`.
  * **REL-QA-005.AUTH:** Verified via LLVM source-based code coverage tooling (`llvm-cov`).
  * **REL-QA-005.FAIL:** Coverage falling below 85% blocks merge to `main`.
  * **REL-QA-005.VERIF:** `SRC` (`run_coverage.sh`).
  * **REL-QA-005.LANE:** Core Gameplay & QA.

* **REL-QA-006 — Automated Campaign Integration Test Suite:** Automated headless regression tests shall validate all 15 campaign operations from start to finish, asserting that primary objectives complete and victory state triggers cleanly.
  * **REL-QA-006.AUTH:** Verified via `EchoesCampaignProgressTest`.
  * **REL-QA-006.FAIL:** Any mission failing automated regression halts integration.
  * **REL-QA-006.VERIF:** `PKG-AUTO` (campaign test automation suite).
  * **REL-QA-006.LANE:** Campaign & Automation.

* **REL-QA-007 — Automated Skirmish Matchup Matrix Suite:** Automated regression testing shall execute all 9 possible 1v1 skirmish matchups (Compact vs Compact, Compact vs Kharuun, etc.) for at least 1,200 ticks each, validating clean execution without crashes.
  * **REL-QA-007.AUTH:** Asserts that all factions spawn, build, harvest, and fight correctly.
  * **REL-QA-007.FAIL:** Any matchup throwing assertions or failing to run is a blocker.
  * **REL-QA-007.VERIF:** `PKG-AUTO` (skirmish matrix automated runner).
  * **REL-QA-007.LANE:** Opponent AI & Skirmish.

* **REL-QA-008 — ASan + UBSan Memory Sanitizer Qualification:** The simulation engine shall pass the complete automated test battery under AddressSanitizer and UndefinedBehaviorSanitizer with zero memory errors, leaks, or undefined shifts.
  * **REL-QA-008.AUTH:** Continuous automated CI gate on PR creation.
  * **REL-QA-008.FAIL:** Any ASan/UBSan finding immediately halts the build.
  * **REL-QA-008.VERIF:** `SRC` (`test_sim_sanitizers.sh`).
  * **REL-QA-008.LANE:** Core Gameplay & Build.

* **REL-QA-009 — ThreadSanitizer Concurrency Validation:** Multi-threaded simulation components (task scheduler, async saves) shall pass ThreadSanitizer (TSan) testing with zero data race reports.
  * **REL-QA-009.AUTH:** Asserts thread safety across shared data structures.
  * **REL-QA-009.FAIL:** TSan data races block release promotion.
  * **REL-QA-009.VERIF:** `SRC` (TSan test suite).
  * **REL-QA-009.LANE:** Core Gameplay & Performance.

* **REL-QA-010 — 60-Minute Rendered Soak Qualification Test:** A packaged release candidate shall execute a continuous 60-minute rendered gameplay session on baseline hardware without crashing or exceeding memory budgets.
  * **REL-QA-010.AUTH:** Verified on a clean, dedicated test machine.
  * **REL-QA-010.FAIL:** Crash or freeze during soak test fails qualification.
  * **REL-QA-010.VERIF:** `PKG-AUTO` (rendered soak test run log).
  * **REL-QA-010.LANE:** Independent QA.

* **REL-QA-011 — 1,000-Match Headless AI Soak Validation:** The simulation core shall execute 1,000 consecutive headless matches across all maps, validating zero crashes, zero deadlocks, and 100% deterministic replayability.
  * **REL-QA-011.AUTH:** Generates comprehensive statistical balance report (`balance_summary.json`).
  * **REL-QA-011.FAIL:** Any crash during 1,000 matches invalidates the batch.
  * **REL-QA-011.VERIF:** `PKG-AUTO` (1,000-match headless test report).
  * **REL-QA-011.LANE:** Opponent AI & QA.

* **REL-QA-012 — Save/Load Round-Trip Determinism Test:** Automated tests shall serialize and deserialize match states at tick 1,000, asserting that simulation checksums match 100% identically between the original and restored sessions.
  * **REL-QA-012.AUTH:** State checksums match to the byte.
  * **REL-QA-012.FAIL:** Checksum divergence upon load indicates save state corruption.
  * **REL-QA-012.VERIF:** `SRC` (save round-trip determinism suite).
  * **REL-QA-012.LANE:** Save Recovery & Core Gameplay.

* **REL-QA-013 — Replay Playback Checksum Consistency Test:** Replaying an authored input sequence from tick 0 to tick 6,000 shall produce identical end-of-match state checksums across multiple runs.
  * **REL-QA-013.AUTH:** Verifies absolute replay determinism.
  * **REL-QA-013.FAIL:** Checksum divergence between replay and live match fails determinism.
  * **REL-QA-013.VERIF:** `SRC` (replay determinism regression test).
  * **REL-QA-013.LANE:** Core Gameplay & QA.

* **REL-QA-014 — Resolution Matrix Visual Quality Audit:** QA shall visually audit rendered game frames across all 7 target resolutions (1280×720 through 2560×1440), verifying that HUD widgets and text render without clipping or overlap.
  * **REL-QA-014.AUTH:** Verified via screenshot capture comparison.
  * **REL-QA-014.FAIL:** Overlapping widgets or clipped text at any resolution fails audit.
  * **REL-QA-014.VERIF:** `PKG-REND` (resolution visual inspection receipts).
  * **REL-QA-014.LANE:** Independent QA & UI.

* **REL-QA-015 — Continuous UI Scaling Sweep Test:** Automated UI tests shall sweep UI scale from 80% to 150% in 10% increments, asserting that widget boundaries remain within screen bounds.
  * **REL-QA-015.AUTH:** Validates layout safety margins at all scale factors.
  * **REL-QA-015.FAIL:** Layout breaking at 150% scale fails test.
  * **REL-QA-015.VERIF:** `PKG-AUTO` (UI scaling layout validation).
  * **REL-QA-015.LANE:** Player Experience & QA.

* **REL-QA-016 — BS.1770-4 Automated Loudness Test:** Audio output captured from a 30-minute gameplay session shall be analyzed via automated BS.1770-4 meter, asserting integrated loudness of $-16	ext{ LUFS} \pm 1	ext{ LU}$ and true peaks $\le -1	ext{ dBTP}$.
  * **REL-QA-016.AUTH:** Verifies audio compliance programmatically.
  * **REL-QA-016.FAIL:** Loudness exceeding -15 LUFS or true peaks > -1 dBTP fails test.
  * **REL-QA-016.VERIF:** `PKG-AUTO` (loudness compliance analysis report).
  * **REL-QA-016.LANE:** Audio & QA.

* **REL-QA-017 — Complete Dialogue Voice & Subtitle Sync Test:** An automated test shall verify that all 308 campaign dialogue lines possess registered audio files, and that subtitles display within $\pm 100	ext{ ms}$ of audio start.
  * **REL-QA-017.AUTH:** Verifies zero unvoiced dialogue lines.
  * **REL-QA-017.FAIL:** Missing voice files or desynchronized subtitles fails verification.
  * **REL-QA-017.VERIF:** `SRC` (voice asset sync test).
  * **REL-QA-017.LANE:** Audio & QA.

* **REL-QA-018 — Accessibility Preset Functional Verification:** QA shall execute test passes with each accessibility preset enabled (Colorblind modes, High Contrast, Reduced Motion, Reduced Flashing), verifying full functionality.
  * **REL-QA-018.AUTH:** Presets deliver promised accessibility accommodations without crashing.
  * **REL-QA-018.FAIL:** Preset failing to alter visual shader fails verification.
  * **REL-QA-018.VERIF:** `PKG-REND` (accessibility preset test receipts).
  * **REL-QA-018.LANE:** Independent QA & Accessibility.

* **REL-QA-019 — Clean-Machine DMG Installation & Verification:** QA shall verify that the packaged `.dmg` mounts, installs to `/Applications`, and launches without developer tools on a factory-clean macOS machine.
  * **REL-QA-019.AUTH:** Verified on an isolated clean Mac hardware unit.
  * **REL-QA-019.FAIL:** Any installation failure on clean machine blocks release.
  * **REL-QA-019.VERIF:** `PKG-PHYS` (clean machine installation test).
  * **REL-QA-019.LANE:** Build Distribution & QA.

* **REL-QA-020 — Gatekeeper Notarization & Offline Staple Verification:** QA shall verify that the packaged DMG passes Gatekeeper verification without internet access (`spctl -a -vvv -t install <dmg>`), proving the ticket is properly stapled.
  * **REL-QA-020.AUTH:** Gatekeeper returns `accepted` and `source=Notarized Developer ID`.
  * **REL-QA-020.FAIL:** Gatekeeper rejecting offline launch fails release criteria.
  * **REL-QA-020.VERIF:** `PKG-AUTO` (spctl offline verification check).
  * **REL-QA-020.LANE:** Build Distribution & Security.

* **REL-QA-021 — Mouse-Only Full Mission Playability Test:** QA shall complete an entire campaign mission using exclusively a 2-button mouse without touching the keyboard, proving one-handed accessibility compliance.
  * **REL-QA-021.AUTH:** Validates all required actions have on-screen HUD triggers.
  * **REL-QA-021.FAIL:** Mission uncompletable without keyboard fails one-handed accessibility.
  * **REL-QA-021.VERIF:** `HUM` (mouse-only playtest session log).
  * **REL-QA-021.LANE:** Independent QA & Accessibility.

* **REL-QA-022 — Rapid 300-APM Command Burst Stress Test:** QA shall execute an automated input driver sending 300 orders per minute for 5 continuous minutes, verifying 0% dropped clicks and zero queue deadlocks.
  * **REL-QA-022.AUTH:** Validates input responsiveness under extreme competitive micro-management.
  * **REL-QA-022.FAIL:** Input queue starvation or simulation freeze under 300 APM fails test.
  * **REL-QA-022.VERIF:** `PKG-AUTO` (APM stress benchmark test).
  * **REL-QA-022.LANE:** Core Gameplay & QA.

* **REL-QA-023 — Chokepoint Deadlock Stress Test:** QA shall run a 12-unit choke throughput benchmark through a 1-tile aperture, asserting all units traverse within 100 ticks without permanent queue lock.
  * **REL-QA-023.AUTH:** Validates MOV-009 choke-negotiation logic.
  * **REL-QA-023.FAIL:** Units jamming or stopping permanently in choke fails test.
  * **REL-QA-023.VERIF:** `PKG-AUTO` (chokepoint throughput test).
  * **REL-QA-023.LANE:** Core Gameplay & QA.

* **REL-QA-024 — 400-Unit Combat Frame-Rate Benchmark:** QA shall record frame-time metrics during a 400-unit mass engagement, verifying p95 frame time $\le 16.67	ext{ ms}$ and minimum frame rate $\ge 45	ext{ FPS}$.
  * **REL-QA-024.AUTH:** Evaluates performance under maximum load.
  * **REL-QA-024.FAIL:** Performance dipping below 30 FPS fails qualification.
  * **REL-QA-024.VERIF:** `PKG-AUTO` (mass combat profiler trace).
  * **REL-QA-024.LANE:** Performance & QA.

* **REL-QA-025 — Ten-Match Skirmish Restart Memory Leak Check:** QA shall automate 10 consecutive skirmish match cycles (Start Skirmish → Play 120 Ticks → Concede → Exit to Menu), asserting net memory growth $\le 50	ext{ MB}$.
  * **REL-QA-025.AUTH:** Verifies complete asset and entity cleanup between matches.
  * **REL-QA-025.FAIL:** Memory growing by $>100	ext{ MB}$ indicates a release-blocking leak.
  * **REL-QA-025.VERIF:** `PKG-AUTO` (match restart memory tracking log).
  * **REL-QA-025.LANE:** Performance & QA.

* **REL-QA-026 — Independent QA Lane Review Isolation:** Release qualification test execution shall be performed strictly by an independent QA session in an isolated clean worktree, separate from the development authoring lane.
  * **REL-QA-026.AUTH:** Developer self-certification is prohibited for release promotion.
  * **REL-QA-026.FAIL:** Gate evidence produced from dirty development worktrees is void.
  * **REL-QA-026.VERIF:** `SRC` (clean worktree verification receipt).
  * **REL-QA-026.LANE:** Independent QA.

* **REL-QA-027 — Blind Naive Human Playtesting Sessions:** At least two uncoached, project-naive human playtesters shall play through the Prologue and Mission 01, documenting usability bottlenecks, confusion points, and gameplay feel.
  * **REL-QA-027.AUTH:** Playtest observations are logged in `WorkstreamControl/evidence/human/`.
  * **REL-QA-027.FAIL:** Shipping without external naive human playtesting is prohibited.
  * **REL-QA-027.VERIF:** `HUM` (naive human playtest observation dossiers).
  * **REL-QA-027.LANE:** Independent QA & Player Experience.

* **REL-QA-028 — Angelis Pseftis Formal Human Acceptance Sign-Off:** Final release promotion requires personal playthrough, inspection, and signed human acceptance by Angelis Pseftis across all 10 Definition of Done criteria.
  * **REL-QA-028.AUTH:** Recorded in `RequirementsState.md` as `HUMAN ACCEPTED (Angelis Pseftis, <date>)`.
  * **REL-QA-028.FAIL:** Public release without explicit owner authorization is void.
  * **REL-QA-028.VERIF:** `OWNER` (formal owner sign-off verdict).
  * **REL-QA-028.LANE:** Coordinator & Owner.

* **REL-QA-029 — Release Candidate Tagging & Branch Lockdown:** Upon human acceptance, the repository shall be tagged with a signed Git release tag (`v1.0.0`), locking the release branch against further code commits.
  * **REL-QA-029.AUTH:** Release commit SHA is frozen and immutable.
  * **REL-QA-029.FAIL:** Post-tagging code modifications require a new release candidate cycle.
  * **REL-QA-029.VERIF:** `SRC` (git tag signature check).
  * **REL-QA-029.LANE:** Coordinator & Build.

* **REL-QA-030 — Master Evidence Archive Binding:** All receipts, traces, test logs, and video captures supporting the release candidate shall be archived permanently in `WorkstreamControl/evidence/release/v1.0.0/`.
  * **REL-QA-030.AUTH:** Archive is bound cryptographically to the release commit SHA.
  * **REL-QA-030.FAIL:** Missing evidence files for accepted requirements fails release governance.
  * **REL-QA-030.VERIF:** `SRC` (evidence archive completeness audit).
  * **REL-QA-030.LANE:** Independent QA.

* **REL-QA-031 — Continuous CI Regression Guard Locks:** CI integration pipelines shall automatically block pull requests that introduce test regressions, sanitizer warnings, or documentation desynchronizations.
  * **REL-QA-031.AUTH:** PR cannot be merged if any status check is red.
  * **REL-QA-031.FAIL:** Merging failing pull requests violates continuous integration rules.
  * **REL-QA-031.VERIF:** `SRC` (CI webhook gate check).
  * **REL-QA-031.LANE:** Build & Automation.

* **REL-QA-032 — Post-Release Hotfix Qualification Protocol:** If a post-release patch (v1.0.1) is required, it shall undergo the exact same regression suite, sanitizer matrix, notarization, and owner sign-off prior to release.
  * **REL-QA-032.AUTH:** Hotfixes are held to the identical quality standard as primary releases.
  * **REL-QA-032.FAIL:** Unverified emergency patches bypassing QA are strictly prohibited.
  * **REL-QA-032.VERIF:** `SRC` (hotfix qualification checklist).
  * **REL-QA-032.LANE:** Independent QA & Coordinator.

---

### §26 Conditional Multiplayer Module (`REL-MP-*`)

> [!NOTE]
> In accordance with the Project Scope Decision (2026-09-02), the Multiplayer Module is **DORMANT** in the 1.0 initial release. The requirements below define the architectural boundary, data contracts, and isolation guarantees, ensuring multiplayer code remains dormant and executes ZERO network operations during single-player play.

* **REL-MP-001 — Conditional Multiplayer Module Dormancy:** The multiplayer subsystem shall remain completely dormant in 1.0 single-player and skirmish modes, compiled into an isolated module (`EchoesNetCore`) with zero runtime overhead.
  * **REL-MP-001.AUTH:** Activating multiplayer features requires an explicit design scope change authorized by Angelis Pseftis.
  * **REL-MP-001.FAIL:** Single-player opening network sockets or calling multiplayer code fails dormancy.
  * **REL-MP-001.VERIF:** `SRC` (multiplayer dormancy static check).
  * **REL-MP-001.LANE:** Core Gameplay & Network.

* **REL-MP-002 — Deterministic Lockstep Protocol Architecture:** The multiplayer network architecture shall be structured as an authoritative deterministic lockstep protocol, transmitting player input commands rather than entity transform synchronizations.
  * **REL-MP-002.AUTH:** Minimizes bandwidth and guarantees cross-client determinism.
  * **REL-MP-002.FAIL:** Client-authoritative transform synchronization violating lockstep is prohibited.
  * **REL-MP-002.VERIF:** `SRC` (lockstep protocol architecture review).
  * **REL-MP-002.LANE:** Network.

* **REL-MP-003 — Fixed-Width Command Packet Schema:** Network message packets shall use a compact, fixed-width binary schema: Client ID (8-bit), Target Tick (32-bit), Command Type (8-bit), Target Entity/Coordinate (64-bit), and CRC16 Checksum.
  * **REL-MP-003.AUTH:** Total packet payload $\le 32	ext{ bytes}$ per command.
  * **REL-MP-003.FAIL:** Variable-length string-based network payloads fail protocol standards.
  * **REL-MP-003.VERIF:** `SRC` (network packet struct size check).
  * **REL-MP-003.LANE:** Network.

* **REL-MP-004 — Peer State Checksum Desynchronization Detection:** Every lockstep turn (every 4 simulation ticks), connected peers shall exchange state checksums. Mismatched checksums trigger an immediate desync pause and packet dump.
  * **REL-MP-004.AUTH:** Prevents silent out-of-sync simulation execution.
  * **REL-MP-004.FAIL:** Desynced matches continuing to play without detection fails protocol rules.
  * **REL-MP-004.VERIF:** `SRC` (desync detection unit test).
  * **REL-MP-004.LANE:** Network.

* **REL-MP-005 — Dynamic Ping-Compensated Turn Buffer:** The turn scheduler shall adjust input delay buffer dynamically based on measured round-trip time (RTT): RTT $<50	ext{ ms} 
ightarrow 2	ext{ ticks}$; RTT $50	ext{--}150	ext{ ms} 
ightarrow 3	ext{ ticks}$; RTT $>150	ext{ ms} 
ightarrow 4	ext{ ticks}$.
  * **REL-MP-005.AUTH:** Maintains smooth lockstep flow under varying network latencies.
  * **REL-MP-005.FAIL:** Static input delay causing hitching on high latency fails pacing.
  * **REL-MP-005.VERIF:** `SRC` (dynamic turn buffer scaling test).
  * **REL-MP-005.LANE:** Network.

* **REL-MP-006 — Graceful Disconnect & Reconnect Recovery:** If a peer disconnects, the simulation pauses for up to 300 ticks (15.0 seconds), displaying a reconnection modal. If the peer fails to reconnect, the match concedes or awards victory.
  * **REL-MP-006.AUTH:** Unresponsive peers do not freeze the session indefinitely.
  * **REL-MP-006.FAIL:** Permanent infinite freeze upon peer disconnect fails recovery.
  * **REL-MP-006.VERIF:** `SRC` (peer disconnect timeout test).
  * **REL-MP-006.LANE:** Network.

* **REL-MP-007 — Lobby Discovery & Room Creation Interface:** The dormant multiplayer UI module defines a structured lobby interface: Create Room, Join by Code, LAN Discovery, and Map/Faction Selection.
  * **REL-MP-007.AUTH:** UI components remain hidden in 1.0 builds until activated.
  * **REL-MP-007.FAIL:** Exposing broken or unfunctional multiplayer lobby buttons in 1.0 fails release criteria.
  * **REL-MP-007.VERIF:** `SRC` (dormant lobby UI visibility check).
  * **REL-MP-007.LANE:** UI & Network.

* **REL-MP-008 — Player Slot & Asymmetric Faction Configuration:** Lobbies shall support up to 4 player slots (2v2 or 4-player FFA), validating that each player selects an authorized faction and starting color accent.
  * **REL-MP-008.AUTH:** Validates map spawn eligibility before launching match.
  * **REL-MP-008.FAIL:** Starting a match with unassigned slots or conflicting colors rejects cleanly.
  * **REL-MP-008.VERIF:** `SRC` (lobby slot validation test).
  * **REL-MP-008.LANE:** Network.

* **REL-MP-009 — Strict Network Bandwidth Budget (64 kbps):** Network bandwidth consumption per connected player shall not exceed $64.0	ext{ kbps}$ upstream and downstream under peak combat command issuance.
  * **REL-MP-009.AUTH:** Enables multiplayer play over constrained broadband connections.
  * **REL-MP-009.FAIL:** Bandwidth exceeding 128 kbps fails protocol efficiency criteria.
  * **REL-MP-009.VERIF:** `SRC` (bandwidth consumption simulation test).
  * **REL-MP-009.LANE:** Network.

* **REL-MP-010 — Client-Side Local Audio/Visual Command Prediction:** Issuing a command shall trigger immediate client-side visual acknowledgement (selection ring pulse, UI audio cue) while authoritative execution waits for lockstep turn application.
  * **REL-MP-010.AUTH:** Preserves responsive game feel without waiting for network round-trip.
  * **REL-MP-010.FAIL:** Sluggish interface where clicks produce zero feedback until network returns fails feel.
  * **REL-MP-010.VERIF:** `SRC` (client-side prediction acknowledgement test).
  * **REL-MP-010.LANE:** Network & Player Experience.

* **REL-MP-011 — Automated Desync Dump Serialization:** When a state checksum mismatch occurs, both clients shall immediately serialize a desync diagnostic archive containing: tick number, entity arrays, RNG state, and the last 100 received command packets.
  * **REL-MP-011.AUTH:** Enables deterministic offline debugging of multiplayer desyncs.
  * **REL-MP-011.FAIL:** Desync occurring without diagnostic dump emission fails supportability.
  * **REL-MP-011.VERIF:** `SRC` (desync diagnostic archive generator test).
  * **REL-MP-011.LANE:** Network & Core Gameplay.

* **REL-MP-012 — Cryptographic Packet Authentication & Anti-Spoofing:** Network packets shall embed a cryptographic HMAC or sequence counter preventing packet replay attacks, spoofed entity commands, or out-of-order execution.
  * **REL-MP-012.AUTH:** Invalid packets are rejected immediately on receipt.
  * **REL-MP-012.FAIL:** Accepting unauthenticated or out-of-sequence packets fails security.
  * **REL-MP-012.VERIF:** `SRC` (packet authentication unit test).
  * **REL-MP-012.LANE:** Security & Network.

* **REL-MP-013 — NAT Traversal & Direct Peer Fallback:** The network socket layer shall support standard UDP hole punching (STUN) for direct peer-to-peer connectivity, with fallback to relay routing if direct connection fails.
  * **REL-MP-013.AUTH:** Enables connectivity across standard home routers without port forwarding.
  * **REL-MP-013.FAIL:** Inability to connect behind symmetric NAT fails connectivity criteria.
  * **REL-MP-013.VERIF:** `SRC` (NAT traversal handshake test).
  * **REL-MP-013.LANE:** Network.

* **REL-MP-014 — Host Migration & Listen-Server Resilience:** If the host player drops from a 4-player match, the session shall elect a new host based on lowest average ping and resume lockstep within 200 ticks.
  * **REL-MP-014.AUTH:** Prevents premature match collapse when the host disconnects.
  * **REL-MP-014.FAIL:** Host drop instantly killing the entire match fails resilience.
  * **REL-MP-014.VERIF:** `SRC` (host migration election test).
  * **REL-MP-014.LANE:** Network.

* **REL-MP-015 — Matchmaking Rating (MMR) Calculation Model:** The multiplayer ranking system defines an Elo/Glicko-2 rating model calculating skill adjustments based on opponent strength and match outcome.
  * **REL-MP-015.AUTH:** Rating updates deterministically upon match completion.
  * **REL-MP-015.FAIL:** Unbounded inflation or NaN rating calculations fail mathematical integrity.
  * **REL-MP-015.VERIF:** `SRC` (MMR rating calculation unit test).
  * **REL-MP-015.LANE:** Network.

* **REL-MP-016 — Absolute Single-Player Network Isolation:** In single-player campaign, tutorial, and offline skirmish modes, the application shall execute ZERO network code, open ZERO UDP/TCP sockets, and listen on ZERO ports (resolving isolation criteria).
  * **REL-MP-016.AUTH:** Complete firewall isolation verified; single-player runs with network interface disabled.
  * **REL-MP-016.FAIL:** Single player opening listen ports or attempting network broadcast is a critical defect.
  * **REL-MP-016.VERIF:** `SRC` + `PKG-AUTO` (offline network socket audit).
  * **REL-MP-016.LANE:** Security & Core Gameplay.

---

### §27 In-Engine Scenario and Map Editor (`REL-EDT-*`)

* **REL-EDT-001 — Native Scenario Editor Architecture:** The engine shall provide an authored, self-contained Scenario and Map Editor mode capable of creating, saving, and packaging standalone `.echoesmap` binary archives without requiring an external Unreal Editor installation.
  * **REL-EDT-001.AUTH:** The editor mode initializes from the main menu under "Editor" within 5.0 seconds, providing full viewport sculpting, entity placement, and trigger scripting.
  * **REL-EDT-001.FAIL:** Requiring developer source builds or Unreal Editor to create custom maps is strictly prohibited.
  * **REL-EDT-001.VERIF:** `PKG-PHYS` (standalone editor launch and interface navigation test).
  * **REL-EDT-001.LANE:** Editor & World.

* **REL-EDT-002 — Terrain Heightfield & Passability Sculpting:** The editor shall provide real-time brush tools allowing users to sculpt terrain elevations (0 to 4 elevation levels), carve water/void chasms, paint Scarred ground, and set authoritative tile passability.
  * **REL-EDT-002.AUTH:** Passability updates compute dynamically on the 64x64 grid; impassable cliff edges auto-generate visual border meshes with correct collision truth.
  * **REL-EDT-002.FAIL:** Sculpting terrain that permits units to path through visual cliffs fails passability compilation.
  * **REL-EDT-002.VERIF:** `SRC` + `EDT` (terrain sculpting passability alignment test).
  * **REL-EDT-002.LANE:** Editor & World.

* **REL-EDT-003 — Resource Deposit & Landmark Snapping:** The editor shall provide drag-and-drop palettes to place Matter crystal deposits, starting Command Core spawn anchors, and neutral Future Wells with automatic clearance snapping.
  * **REL-EDT-003.AUTH:** Snapping validates that resource nodes and Wells maintain authored clearance (≥420 cm capture radius for Wells; ≥60 cm worker harvesting ring).
  * **REL-EDT-003.FAIL:** Overlapping resource nodes or unreachable Well placements fail validation.
  * **REL-EDT-003.VERIF:** `EDT` (resource snapping clearance check).
  * **REL-EDT-003.LANE:** Editor & Core Gameplay.

* **REL-EDT-004 — Node-Based Tactical Event & Trigger Graph:** The editor shall incorporate a visual node-based trigger system (Events, Conditions, Actions) enabling map creators to script custom missions: timed reinforcement waves, spatial discovery beacons, interactive dialogue boxes, and custom victory/defeat predicates.
  * **REL-EDT-004.AUTH:** Trigger graphs serialize into deterministic bytecode executed strictly on the 20 Hz simulation accumulator.
  * **REL-EDT-004.FAIL:** Custom scripts mutating memory outside authorized simulation APIs fail compiler sandboxing.
  * **REL-EDT-004.VERIF:** `SRC` (trigger graph bytecode sandbox execution test).
  * **REL-EDT-004.LANE:** Editor & Core Gameplay.

* **REL-EDT-005 — Custom Campaign Packaging & Manifest Compiler:** Map authors shall package multiple `.echoesmap` files into an authored `.echoescampaign` container with custom chapter ordering, briefing text, character portraits, and consequence ledger variables.
  * **REL-EDT-005.AUTH:** Custom campaigns execute through the standard campaign mission runner without code modifications.
  * **REL-EDT-005.FAIL:** Corrupt campaign manifests crashing the game client fail packaging verification.
  * **REL-EDT-005.VERIF:** `PKG-AUTO` (custom campaign packaging and playback test).
  * **REL-EDT-005.LANE:** Campaign & Editor.

* **REL-EDT-006 — In-Game Community Map Browser & Loader:** The main menu shall include a "Custom Maps" browser discovering all `.echoesmap` files stored in `~/Library/Application Support/EchoesOfTheBrokenSun/Maps/`.
  * **REL-EDT-006.AUTH:** The browser displays map title, author, player count, minimap thumbnail preview, and validation status.
  * **REL-EDT-006.FAIL:** Maps failing checksum validation load silently without user warning.
  * **REL-EDT-006.VERIF:** `PKG-PHYS` (custom map browser loading test).
  * **REL-EDT-006.LANE:** Player Experience & Editor.

* **REL-EDT-007 — Automated Map Validation Preflight Compiler:** Before saving or exporting an `.echoesmap`, the editor shall execute an automated validation preflight asserting: (1) all spawn slots have passable paths to at least one Matter deposit; (2) map perimeter is sealed; (3) Future Wells have valid capture zones; (4) symmetric spawn distances match within ≤5.0% path distance.
  * **REL-EDT-007.AUTH:** The preflight highlights validation errors with clickable red markers in the editor viewport.
  * **REL-EDT-007.FAIL:** Exporting an unplayable or unsealed map without passing preflight is prohibited.
  * **REL-EDT-007.VERIF:** `SRC` (map preflight validation compiler tests).
  * **REL-EDT-007.LANE:** Editor & QA.

* **REL-EDT-008 — Sandboxed Custom Asset Ingestion Policy:** Custom maps importing user textures, audio cues, or mesh silhouettes shall pass through a strict security sanitizer verifying file formats (PNG, WAV, glTF), memory caps (≤250 MB per map), and zero executable code injection.
  * **REL-EDT-008.AUTH:** Assets violating format or memory limits reject with `[CUSTOM_ASSET_REJECTED]`.
  * **REL-EDT-008.FAIL:** Custom map loading executing arbitrary scripts or system calls is a fatal vulnerability.
  * **REL-EDT-008.VERIF:** `SRC` (custom asset security sanitizer test).
  * **REL-EDT-008.LANE:** Security & Editor.

* **REL-EDT-009 — Headless Map Compiler & Navigation Precomputation:** The editor toolchain shall provide a headless CLI mode (`Scripts/compile_map.py`) capable of baking distance fields, visibility graphs, and minimap thumbnails in under 3.0 seconds per map.
  * **REL-EDT-009.AUTH:** Headless compilation matches interactive editor output byte-for-byte.
  * **REL-EDT-009.FAIL:** Discrepancy between headless and editor map compilation fails build verification.
  * **REL-EDT-009.VERIF:** `SRC` (headless map compilation regression test).
  * **REL-EDT-009.LANE:** Build & Automation.

* **REL-EDT-010 — Map Schema Forward Migration Discipline:** Map binary containers shall embed a monotonic schema version. Loading an older valid map version shall invoke a lossless forward transformer, guaranteeing that community maps authored in v1.0 remain 100% playable in future engine updates.
  * **REL-EDT-010.AUTH:** Verified against sample legacy map fixtures on every release build.
  * **REL-EDT-010.FAIL:** Breaking compatibility with older community maps without migration fails long-term support.
  * **REL-EDT-010.VERIF:** `SRC` (map schema migration test suite).
  * **REL-EDT-010.LANE:** Editor & Governance.

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
| `SPEC-BAL-001` | Headless Batch Simulation Harness | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-BAL-002` | Statistical Balance Reporting with Uncertainty | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-BAL-003` | Faction Asymmetry Balance Band | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-BAL-004` | Map and Spawn Symmetry Fairness | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-BAL-005` | Strategy Primacy Over Randomness | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-BAL-006` | Batch Replayability and Verification | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-BAL-007` | Balance Evidence Expiry and Re-Validation | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-BAL-008` | AI Instrument Competence Baseline | 16.4 Mass AI Balance Validation Architecture |
| `SPEC-CTL-016` | Command Responsiveness Invariant | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-CTL-017` | Fluid Command Interruptibility | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-CTL-018` | Micro-Management Usability Preservation | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-CTL-019` | Simulation Tick Cost Ceiling for Steering | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-006` | Any-Angle Movement | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-007` | Direction-Independent Speed | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-008` | Soft Separation and Non-Imprisonment | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-009` | Chokepoint Negotiation Without Deadlock | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-010` | Travel Facing and Presentation Decoupling | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-011` | Group Cohesion and Centroid Navigation | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-012` | Damped Clean Arrival | 7.3 Advanced Movement, Control Responsiven... |
| `SPEC-MOV-013` | Movement Determinism & Sanitizer Invariance | 7.3 Advanced Movement, Control Responsiven... |
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
| `REL-ACC-001` | Color-Vision Deficiency Simulation Presets | §21 Accessibility and Localization Readiness |
| `REL-ACC-002` | Dual-Channel Non-Color Information Encoding | §21 Accessibility and Localization Readiness |
| `REL-ACC-003` | High-Contrast Silhouette Enhancement | §21 Accessibility and Localization Readiness |
| `REL-ACC-004` | Reduced Motion Preset Invariant | §21 Accessibility and Localization Readiness |
| `REL-ACC-005` | Reduced Flashing & Photosensitive Safety | §21 Accessibility and Localization Readiness |
| `REL-ACC-006` | Full UI Scale Dynamic Range Compliance | §21 Accessibility and Localization Readiness |
| `REL-ACC-007` | High-Contrast Subtitle Styling & Scalability | §21 Accessibility and Localization Readiness |
| `REL-ACC-008` | Mandatory Subtitle Speaker Identity Tags | §21 Accessibility and Localization Readiness |
| `REL-ACC-009` | Directional Spatial Off-Screen Visual Indicators | §21 Accessibility and Localization Readiness |
| `REL-ACC-010` | Screen Reader TTS Menu Accessibility | §21 Accessibility and Localization Readiness |
| `REL-ACC-011` | Comprehensive Input Key Re-mapping | §21 Accessibility and Localization Readiness |
| `REL-ACC-012` | One-Handed Mouse-Only Playability | §21 Accessibility and Localization Readiness |
| `REL-ACC-013` | Configurable Edge-Pan Speed & Dead-Zone Calibration | §21 Accessibility and Localization Readiness |
| `REL-ACC-014` | High-Visibility Mouse Cursor & Click Ring | §21 Accessibility and Localization Readiness |
| `REL-ACC-015` | Single-Player Simulation Game Speed Pacing | §21 Accessibility and Localization Readiness |
| `REL-ACC-016` | Accessibility Profile Persistence Across Sessions | §21 Accessibility and Localization Readiness |
| `REL-ACC-017` | Dedicated Top-Level Accessibility Menu Hub | §21 Accessibility and Localization Readiness |
| `REL-AI-001` | Scoped PlayerView AI Authority Invariant | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-002` | Autonomous AI Economic Expansion | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-003` | Fair Reconnaissance & Scouting Cadence | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-004` | Dynamic Army Composition & Archetype Replacement | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-005` | Defensive Reaction & Base Protection | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-006` | Strike Force Assembly & Coordinated Assault | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-007` | Combat Retreat & Force Preservation | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-008` | Future Well Protocol Doctrinal Alignment | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-009` | AI Doctrine | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-010` | AI Doctrine | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-011` | AI Doctrine | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-012` | AI Doctrine | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-013` | AI Doctrine | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-014` | AI Difficulty Tier | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-015` | AI Difficulty Tier | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-016` | Standard Matchup Competitive Balance Band | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-017` | AI Difficulty Tier | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-018` | AI Difficulty Tier | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-019` | AI Concession & Elimination Protocol | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-020` | Skirmish Mirror Matchup Support | §15 Skirmish, AI, Difficulty, and Balance |
| `REL-AI-021` | Elimination of Undocumented AI Doctrines | §15 Skirmish, AI, Difficulty, and Balance |
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
| `REL-ART-001` | Five-Color Aesthetic Palette Enforcement | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-002` | One-Second Tactical Readability Invariant | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-003` | Matte Terrain Surface Anti-Glint Specification | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-004` | Emissive Accent Surface Area Ceiling | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-005` | 24 Production Mesh Silhouettes & LODs | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-006` | Meridian Compact Engineered Form Language | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-007` | Kharuun Assemblies Grown Mineral Architecture | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-008` | Hollow Choir Possibility Superposition Aesthetics | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-009` | Code-Driven Kinematic Motion Invariant | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-010` | Bulwark Team Deploy Transformation Rig | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-011` | Kharuun Waystone Rooting & Uproot Animation | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-012` | Distinct Weapon Projectile & Muzzle VFX | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-013` | Structural Collapse & Shatter Destruction VFX | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-014` | Future Well Landmark 4-State Visual Manifest | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-015` | Site Dressing & Environmental Coherence | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-016` | Dressing Collision Truth & Passability Parity | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-017` | True 3D Volumetric Fog of War Occlusion | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-018` | Explored Shroud Object Memory Persistence | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-019` | Rigorous Exposure & Lighting Calibration | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-020` | Particle VFX Performance & Collision Discipline | §18 World Art, Units, Structures, Animatio... |
| `REL-ART-021` | see Part III | Initial release |
| `REL-ART-022` | see Part III | Initial release |
| `REL-ART-023` | see Part III | Initial release |
| `REL-AUD-001` | Five-Category Submix Hierarchy | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-002` | Integrated Loudness & True Peak Mastering Target | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-003` | Dynamic Side-Chain Vocal Ducking | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-004` | Local Neural Text-to-Speech Voice Generation Pipeline | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-005` | Voice Profile | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-006` | Voice Profile | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-007` | Voice Profile | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-008` | Voice Profile | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-009` | Voice Profile | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-010` | Full Procedural Music Suite & Act Themes | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-011` | Dynamic Combat State Cross-Fading | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-012` | Positional Environmental Ambience Beds | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-013` | Textural Non-Spoken Unit Acknowledgements | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-014` | High-Priority Alert Audio Rate Limiting | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-015` | Whole-Graph Reduced Dynamic Range Mode | §19 Audio, Voice, Music, and Cinematics |
| `REL-AUD-016` | Situational Unit Tactical Chatter & Annoyance Lines | §19 Audio, voice, music, cinematics |
| `REL-BLD-001` | Footprint Validation & Passability Enforcement | §10 Construction, Production, and Research |
| `REL-BLD-002` | Dynamic Placement Visual Blueprint Preview | §10 Construction, Production, and Research |
| `REL-BLD-003` | Calibrated Construction Duration Scaling | §10 Construction, Production, and Research |
| `REL-BLD-004` | Multi-Builder Assist Diminishing Returns | §10 Construction, Production, and Research |
| `REL-BLD-005` | Incomplete Structure Vulnerability | §10 Construction, Production, and Research |
| `REL-BLD-006` | Construction Cancellation & Refund Policy | §10 Construction, Production, and Research |
| `REL-BLD-007` | Command Core Singularity Invariant | §10 Construction, Production, and Research |
| `REL-BLD-008` | Production Queue Depth & Management | §10 Construction, Production, and Research |
| `REL-BLD-009` | Unit Emergence, Rallying & Unblocking | §10 Construction, Production, and Research |
| `REL-BLD-010` | Production Queue Cancellation Invariant | §10 Construction, Production, and Research |
| `REL-BLD-011` | Research Slot Contention & Mutual Exclusivity | §10 Construction, Production, and Research |
| `REL-BLD-012` | Technology Irreversibility & Zero Refund | §10 Construction, Production, and Research |
| `REL-BLD-013` | Structural Repair Resolution | §10 Construction, Production, and Research |
| `REL-BLD-014` | Structural Destruction Debris & Footprint Clearance | §10 Construction, Production, and Research |
| `REL-BLD-015` | see Part III | Initial release |
| `REL-BLD-016` | see Part III | Initial release |
| `REL-BLD-017` | see Part III | Initial release |
| `REL-BLD-018` | see Part III | Initial release |
| `REL-BLD-019` | see Part III | Initial release |
| `REL-BLD-020` | see Part III | Initial release |
| `REL-CAM-001` | Fifteen-Operation Continuous Campaign Lifecycle | §14 Campaign and Narrative |
| `REL-CAM-002` | Act I | §14 Campaign and Narrative |
| `REL-CAM-003` | Act II | §14 Campaign and Narrative |
| `REL-CAM-004` | Act III | §14 Campaign and Narrative |
| `REL-CAM-005` | Four Earned Campaign Endings | §14 Campaign and Narrative |
| `REL-CAM-006` | Mission 01 | §14 Campaign and Narrative |
| `REL-CAM-007` | Mission 02 | §14 Campaign and Narrative |
| `REL-CAM-008` | Mission 03 | §14 Campaign and Narrative |
| `REL-CAM-009` | Mission 04 | §14 Campaign and Narrative |
| `REL-CAM-010` | Mission 05 | §14 Campaign and Narrative |
| `REL-CAM-011` | Mission 06 | §14 Campaign and Narrative |
| `REL-CAM-012` | Mission 07 | §14 Campaign and Narrative |
| `REL-CAM-013` | Mission 08 | §14 Campaign and Narrative |
| `REL-CAM-014` | Mission 09 | §14 Campaign and Narrative |
| `REL-CAM-015` | Mission 10 | §14 Campaign and Narrative |
| `REL-CAM-016` | Mission 11 | §14 Campaign and Narrative |
| `REL-CAM-017` | Mission 12 | §14 Campaign and Narrative |
| `REL-CAM-018` | Mission 13 | §14 Campaign and Narrative |
| `REL-CAM-019` | Mission 14 | §14 Campaign and Narrative |
| `REL-CAM-020` | Mission 15 | §14 Campaign and Narrative |
| `REL-CAM-021` | Campaign Objective Decoupling from Corefall | §14 Campaign and Narrative |
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
| `REL-CIN-001` | Sequencer In-Engine Cutscene Pipeline | §19 Audio, Voice, Music, and Cinematics |
| `REL-CIN-002` | In-Engine Title Cinematic Sequence | §19 Audio, Voice, Music, and Cinematics |
| `REL-CIN-003` | Act I Transition Sequence ("Necessary Fires") | §19 Audio, Voice, Music, and Cinematics |
| `REL-CIN-004` | Act II Transition Sequence ("The Cost of One Future") | §19 Audio, Voice, Music, and Cinematics |
| `REL-CIN-005` | Act III Transition Sequence ("Crownfall") | §19 Audio, Voice, Music, and Cinematics |
| `REL-CIN-006` | Four Authoritative Ending Cinematics | §19 Audio, Voice, Music, and Cinematics |
| `REL-CIN-007` | Universal Cinematic Skippability & Subtitle Guarantee | §19 Audio, Voice, Music, and Cinematics |
| `REL-CIN-008` | Production Visual Bar Parity in Cinematics | §19 Audio, Voice, Music, and Cinematics |
| `REL-CMB-001` | Authoritative Range & Line-of-Sight Firing Arcs | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-002` | Weapon Cooldown & Attack Cadence Sync | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-003` | Ballistic Projectile Travel Simulation | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-004` | Terrain Line-of-Sight Projectile Occlusion | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-005` | Destructible Mineral Cover Ballistic Shielding | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-006` | Deterministic Damage Calculation Hygiene | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-007` | Overkill Avoidance Targeting Protocol | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-008` | Threat-Based Target Priority Hierarchy | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-009` | Combat Stance Architecture | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-010` | Hold Ground Positional Rigidity | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-011` | Attack-Move Engagement Mechanics | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-012` | Cyclic Patrol Waypoint Navigation | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-013` | Guard Escort Dynamics | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-014` | Direct Focus Fire Command Override | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-015` | Entity Destruction & Collision Purge | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-016` | Multi-Channel Combat Damage Feedback | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-017` | Fog of War Engagement Constraints | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-018` | Worker Disarmament Invariant | §11 Selection, Movement, Commands, and Combat |
| `REL-CMB-019` | see Part III | Initial release |
| `REL-CMB-020` | see Part III | Initial release |
| `REL-CMB-021` | see Part III | Initial release |
| `REL-CMB-022` | see Part III | Initial release |
| `REL-CMB-023` | see Part III | Initial release |
| `REL-CMB-024` | see Part III | Initial release |
| `REL-CMB-025` | see Part III | Initial release |
| `REL-CMB-026` | see Part III | Initial release |
| `REL-CMB-027` | see Part III | Initial release |
| `REL-DIST-001` | Standalone macOS Application Bundle | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-002` | Apple Silicon ARM64 Native Architecture | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-003` | Apple Developer ID Code Signing | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-004` | Apple Notarization Service Qualification | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-005` | Cryptographic Ticket Stapling | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-006` | Read-Only Compressed DMG Distribution Installer | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-007` | Application Branding & Metadata Completeness | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-008` | Seamless Clean-Machine Gatekeeper Launch | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-009` | Complete Dynamic Library Self-Containment | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-010` | Strict File System Permission Sandboxing | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-011` | Distribution Package Compression Budget | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-012` | Clean Uninstallation & Zero System Debris | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-013` | Automated Package Seal & SHA-256 Digest | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-014` | Headless Automated Packaging Build Pipeline | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-015` | Multi-Version macOS OS Compatibility | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-016` | 100% Offline DRM-Free Execution | §23 Security, Privacy, Packaging, and Dist... |
| `REL-DIST-017` | Patching & Delta Update Architecture Readiness | §23 Security, Privacy, Packaging, and Dist... |
| `REL-ECO-001` | Three Resource Pillars | §9 Economy and Logistics |
| `REL-ECO-002` | Starting Matter Resource Presets | §9 Economy and Logistics |
| `REL-ECO-003` | Calibrated Matter Harvesting Cadence | §9 Economy and Logistics |
| `REL-ECO-004` | Deposit Saturation & Diminishing Returns | §9 Economy and Logistics |
| `REL-ECO-005` | Deposit Depletion Lifecycle | §9 Economy and Logistics |
| `REL-ECO-006` | Continuous Automated Worker Harvesting Loop | §9 Economy and Logistics |
| `REL-ECO-007` | Drop-Off Target Selection & Routing | §9 Economy and Logistics |
| `REL-ECO-008` | Cargo Loss and Drop-Off Severance Handling | §9 Economy and Logistics |
| `REL-ECO-009` | Dawn Inflow and Reserve Invariant | §9 Economy and Logistics |
| `REL-ECO-010` | Fail-Closed Dawn Reservation Invariant | §9 Economy and Logistics |
| `REL-ECO-011` | Logistics Capacity Allocation Mechanics | §9 Economy and Logistics |
| `REL-ECO-012` | Logistics Cap Enforcement & Supply Deficit | §9 Economy and Logistics |
| `REL-ECO-013` | Temporary Logistics Burst Dynamics | §9 Economy and Logistics |
| `REL-ECO-014` | Asymmetric Faction Economy Rules | §9 Economy and Logistics |
| `REL-ECO-015` | see Part III | Initial release |
| `REL-ECO-016` | see Part III | Initial release |
| `REL-ECO-017` | see Part III | Initial release |
| `REL-FAC-001` | Tripartite Asymmetric Strategic Identity | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-002` | Meridian Compact Power Grid Topology | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-003` | Meridian Power Link Distribution & Throughput | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-004` | Meridian Aegis Post Automated Defensive Battery | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-005` | Meridian Bulwark Team Directional Shield Deployment | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-006` | Meridian Relay Skiff Logistics Relay | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-007` | Kharuun Waystone Rooting & Relocation | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-008` | Kharuun Listening Spine Seismic Detection | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-009` | Kharuun Warform Adaptation Molting | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-010` | Kharuun Cairnback Mineral Cover Erection | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-011` | Hollow Choir Structural Coherence Upkeep Cycle | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-012` | Hollow Choir Reconciliation Identity Transition | §12 Factions, Rosters, and Strategic Depth |
| `REL-FAC-013` | Hollow Choir Phase Anchor Coherence Field | §12 Factions, Rosters, and Strategic Depth |
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
| `REL-FTU-001` | Clean-Machine Cold Launch Reliability | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-002` | Atmospheric World-Coherent Title Treatment | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-003` | Primary Navigation Hub Ergonomics | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-004` | In-Engine Opening Story Sequence | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-005` | First-Run Progressive Onboarding Gate | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-006` | Tutorial Lesson 1 | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-007` | Tutorial Lesson 2 | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-008` | Tutorial Lesson 3 | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-009` | Tutorial Lesson 4 | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-010` | Tutorial Lesson 5 | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-011` | Tutorial Lesson 6 | §7 First-Run, Front Door, and Onboarding |
| `REL-FTU-012` | Demonstration of Fundamental Mastery | §7 First-Run, Front Door, and Onboarding |
| `REL-GOV-001` | Sole Normative Requirements Authority | §6 Release Governance and Integrity |
| `REL-GOV-002` | Bidirectional Gate and Milestone Mapping | §6 Release Governance and Integrity |
| `REL-GOV-003` | Prohibition of Silent Invention | §6 Release Governance and Integrity |
| `REL-GOV-004` | Traceable Inline Change Cascades | §6 Release Governance and Integrity |
| `REL-GOV-005` | Evidence-Bounded State Vocabulary | §6 Release Governance and Integrity |
| `REL-GOV-006` | Permanent Immutable Identifier Preservation | §6 Release Governance and Integrity |
| `REL-GOV-007` | Verification Class Exclusivity | §6 Release Governance and Integrity |
| `REL-GOV-008` | Human Acceptance Authority Reservation | §6 Release Governance and Integrity |
| `REL-GOV-009` | Fail-Closed Architectural Boundary | §6 Release Governance and Integrity |
| `REL-GOV-010` | Procedural-First Asset Provenance Registration | §6 Release Governance and Integrity |
| `REL-GOV-011` | Independent Verification Lane Separation | §6 Release Governance and Integrity |
| `REL-GOV-012` | Continuous Automated Regression Locks | §6 Release Governance and Integrity |
| `REL-GOV-013` | Defect Severity Ladder and Release Prohibitions | §6 Release Governance and Integrity |
| `REL-GOV-014` | Single-Candidate Frozen Evidence Binding | §6 Release Governance and Integrity |
| `REL-GOV-015` | Final Definition of Done Sign-Off Checklist | §6 Release Governance and Integrity |
| `REL-LOC-001` | Complete String Externalization Standard | §21 Accessibility and Localization Readiness |
| `REL-LOC-002` | UTF-8 Unicode Encoding Invariant | §21 Accessibility and Localization Readiness |
| `REL-LOC-003` | UI Text Container Expansion Margins | §21 Accessibility and Localization Readiness |
| `REL-LOC-004` | Automated Localization Translation Extraction Pipeline | §21 Accessibility and Localization Readiness |
| `REL-LOC-005` | International Font Fallback & Glyphs | §21 Accessibility and Localization Readiness |
| `REL-LOC-006` | Culturalization & Regional Sensitivity Hygiene | §21 Accessibility and Localization Readiness |
| `REL-MP-001` | Conditional Multiplayer Module Dormancy | §26 Conditional Multiplayer Module |
| `REL-MP-002` | Deterministic Lockstep Protocol Architecture | §26 Conditional Multiplayer Module |
| `REL-MP-003` | Fixed-Width Command Packet Schema | §26 Conditional Multiplayer Module |
| `REL-MP-004` | Peer State Checksum Desynchronization Detection | §26 Conditional Multiplayer Module |
| `REL-MP-005` | Dynamic Ping-Compensated Turn Buffer | §26 Conditional Multiplayer Module |
| `REL-MP-006` | Graceful Disconnect & Reconnect Recovery | §26 Conditional Multiplayer Module |
| `REL-MP-007` | Lobby Discovery & Room Creation Interface | §26 Conditional Multiplayer Module |
| `REL-MP-008` | Player Slot & Asymmetric Faction Configuration | §26 Conditional Multiplayer Module |
| `REL-MP-009` | Strict Network Bandwidth Budget (64 kbps) | §26 Conditional Multiplayer Module |
| `REL-MP-010` | Client-Side Local Audio/Visual Command Prediction | §26 Conditional Multiplayer Module |
| `REL-MP-011` | Automated Desync Dump Serialization | §26 Conditional Multiplayer Module |
| `REL-MP-012` | Cryptographic Packet Authentication & Anti-Spoofing | §26 Conditional Multiplayer Module |
| `REL-MP-013` | NAT Traversal & Direct Peer Fallback | §26 Conditional Multiplayer Module |
| `REL-MP-014` | Host Migration & Listen-Server Resilience | §26 Conditional Multiplayer Module |
| `REL-MP-015` | Matchmaking Rating (MMR) Calculation Model | §26 Conditional Multiplayer Module |
| `REL-MP-016` | Absolute Single-Player Network Isolation | §26 Conditional Multiplayer Module |
| `REL-MP-017` | see Part III | Initial release |
| `REL-PERF-001` | 60 FPS Target on Baseline Apple Silicon | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-002` | Frame Time Distribution & Spike Ceiling | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-003` | Game Thread Execution Budget | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-004` | Render Thread & GPU Execution Budget | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-005` | Volumetric Fog GPU Computation Budget | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-006` | Pathfinding Burst Re-plan Budget | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-007` | Master Performance Budget Compliance Invariant | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-008` | Resident Memory Ceiling (10 GB RSS) | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-009` | VRAM Texture Streaming & Unified Memory Management | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-010` | 400-Unit Four-Team Stress Protocol | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-011` | 600-Second Preflight & 60-Minute Rendered Soak | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-012` | Headless Multi-Hour AI Soak Validation | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-013` | Dynamic Graphics Scalability Presets | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-014` | Metal API Shader Precompilation Pipeline | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-015` | Passive Thermal Throttling Resilience | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-016` | Headless Simulation Execution Throughput | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-017` | Asynchronous Worker Thread Work Scheduler | §22 Graphics Scalability, Performance, and... |
| `REL-PERF-018` | Zero Mid-Frame Heap Allocation Invariant | §22 Graphics Scalability, Performance, and... |
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
| `REL-PUB-001` | Absolute Truth in Public Gameplay Claims | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-002` | Comprehensive Standalone Game Manual (PDF) | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-003` | Authoritative Manual Content Architecture | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-004` | Official Public Product Website | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-005` | Truthful Hardware System Requirements | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-006` | Direct Support Contact & Issue Reporting | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-007` | Third-Party Open Source Software Disclosures | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-008` | Copyright & Trademark Notices | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-009` | Public Accessibility Feature Documentation | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-010` | Version Release Notes & Changelog | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-011` | In-Engine Truthful Media Assets | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-012` | Community Code of Conduct & Fair Play Rules | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-013` | Transparent Zero-Collection Privacy Policy | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-014` | Public Bug Report Template & Submission Guide | §24 Public Website, Manual, Claims, and Su... |
| `REL-PUB-015` | Digital Signature Verification Guide | §24 Public Website, Manual, Claims, and Su... |
| `REL-QA-001` | Rigorous Defect Severity Ladder | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-002` | Zero S0/S1 Release Prohibitions | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-003` | Zero Un-Waived S2 Critical Path Defects | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-004` | Authoritative Defect Register Ledger | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-005` | 85% Simulation Unit Test Coverage Ceiling | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-006` | Automated Campaign Integration Test Suite | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-007` | Automated Skirmish Matchup Matrix Suite | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-008` | ASan + UBSan Memory Sanitizer Qualification | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-009` | ThreadSanitizer Concurrency Validation | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-010` | 60-Minute Rendered Soak Qualification Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-011` | 1,000-Match Headless AI Soak Validation | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-012` | Save/Load Round-Trip Determinism Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-013` | Replay Playback Checksum Consistency Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-014` | Resolution Matrix Visual Quality Audit | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-015` | Continuous UI Scaling Sweep Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-016` | BS.1770-4 Automated Loudness Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-017` | Complete Dialogue Voice & Subtitle Sync Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-018` | Accessibility Preset Functional Verification | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-019` | Clean-Machine DMG Installation & Verification | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-020` | Gatekeeper Notarization & Offline Staple Verification | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-021` | Mouse-Only Full Mission Playability Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-022` | Rapid 300-APM Command Burst Stress Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-023` | Chokepoint Deadlock Stress Test | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-024` | 400-Unit Combat Frame-Rate Benchmark | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-025` | Ten-Match Skirmish Restart Memory Leak Check | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-026` | Independent QA Lane Review Isolation | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-027` | Blind Naive Human Playtesting Sessions | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-028` | Angelis Pseftis Formal Human Acceptance Sign-Off | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-029` | Release Candidate Tagging & Branch Lockdown | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-030` | Master Evidence Archive Binding | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-031` | Continuous CI Regression Guard Locks | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-032` | Post-Release Hotfix Qualification Protocol | §25 QA, Human Validation, and Release Bloc... |
| `REL-QA-033` | see Part III | Initial release |
| `REL-QA-034` | see Part III | Initial release |
| `REL-QA-035` | see Part III | Initial release |
| `REL-QA-036` | see Part III | Initial release |
| `REL-QOL-001` | Control Group Assignment & Selection | §16 Replays and Quality-of-Life |
| `REL-QOL-002` | Control Group Camera Centering | §16 Replays and Quality-of-Life |
| `REL-QOL-003` | Multi-Unit Subgroup Navigation | §16 Replays and Quality-of-Life |
| `REL-QOL-004` | Structure Rally Point Management | §16 Replays and Quality-of-Life |
| `REL-QOL-005` | Queue Cancellation & Reordering | §16 Replays and Quality-of-Life |
| `REL-QOL-006` | Idle Worker Cycling Hotkey (`F1`) | §16 Replays and Quality-of-Life |
| `REL-QOL-007` | Production Facility Cycling Hotkey (`F2`) | §16 Replays and Quality-of-Life |
| `REL-QOL-008` | Combat Army Selection Hotkey (`F3`) | §16 Replays and Quality-of-Life |
| `REL-QOL-009` | Spatial Alert Feed & Jump Hotkey (`Space`) | §16 Replays and Quality-of-Life |
| `REL-QOL-010` | Replay Recording & Metadata Browser | §16 Replays and Quality-of-Life |
| `REL-QOL-011` | Replay Playback Transport Controls | §16 Replays and Quality-of-Life |
| `REL-QOL-012` | Anti-Rewrite Campaign Replay Protection | §16 Replays and Quality-of-Life |
| `REL-SAV-001` | Transactional Atomic Save Writing | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-002` | Header Schema & CRC32/FNV-1a Checksum Validation | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-003` | Isolated Player Profile Persistence | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-004` | Three Independent Campaign Journey Slots | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-005` | Tactical Mission Checkpoint State Serialization | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-006` | Autonomous Milestone Autosave Cadence | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-007` | Asynchronous Non-Blocking Save Frame Budget | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-008` | Cross-Platform Endian-Safe Binary Format | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-009` | Graceful Corrupt Save Containment & Backup Recovery | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-010` | Schema Versioning & Forward Migration Discipline | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-011` | Campaign Consequence Ledger Tamper Resistance | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-012` | In-Match Save Lockout Safety Zones | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-013` | macOS Sandboxed Directory Compliance | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SAV-014` | Save Data Privacy & Zero Personal Telemetry | §20 Saves, Profiles, Progression, and Reco... |
| `REL-SEC-001` | Memory Safety & Buffer Bounds Discipline | §23 Security, Privacy, Packaging, and Dist... |
| `REL-SEC-002` | Path Traversal Attack Refusal | §23 Security, Privacy, Packaging, and Dist... |
| `REL-SEC-003` | Zero Telemetry & Player Privacy Protection | §23 Security, Privacy, Packaging, and Dist... |
| `REL-SEC-004` | macOS Hardened Runtime Compliance | §23 Security, Privacy, Packaging, and Dist... |
| `REL-SEC-005` | Immutable Read-Only App Bundle Integrity | §23 Security, Privacy, Packaging, and Dist... |
| `REL-SEC-006` | Sanitized Crash Dumps & Zero Memory Leakage | §23 Security, Privacy, Packaging, and Dist... |
| `REL-SIM-001` | Deterministic Fixed-Accumulator Simulation Loop | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-002` | Integer and Fixed-Point Arithmetic Authority | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-003` | Seeded Pseudo-Random Number Generation | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-004` | Strict View Layer Isolation from Simulation Authority | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-005` | Filtered PlayerView Information Masking | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-006` | Fog of War Spatial Authority | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-007` | Dual Time Representation Contract | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-008` | Authoritative Command Validation Pipeline | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-009` | Entity Unique Identifier Lifecycle & Ghost Prevention | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-010` | Map Spatial Boundary Invariant | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-011` | Transactional State Checksum Verification | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-012` | Core Tick Processing Time Ceiling | §8 Core Simulation, Time, and Player Autho... |
| `REL-SIM-013` | see Part III | Initial release |
| `REL-SIM-014` | see Part III | Initial release |
| `REL-SIM-015` | see Part III | Initial release |
| `REL-SIM-016` | see Part III | Initial release |
| `REL-SIM-017` | see Part III | Initial release |
| `REL-SIM-018` | see Part III | Initial release |
| `REL-SIM-019` | see Part III | Initial release |
| `REL-STAB-001` | Sixty-Minute Sustained Gameplay Stability | §22 Graphics Scalability, Performance, and... |
| `REL-STAB-002` | Match Restart Memory Leak Invariance | §22 Graphics Scalability, Performance, and... |
| `REL-STAB-003` | Graceful Crash Minidump Capture | §22 Graphics Scalability, Performance, and... |
| `REL-STAB-004` | Clean Process Termination Invariant | §22 Graphics Scalability, Performance, and... |
| `REL-STAB-005` | Display & Cursor State Restoration on Exit | §22 Graphics Scalability, Performance, and... |
| `REL-UI-001` | Production UMG/Slate Widget Framework | §17 UI and Interaction |
| `REL-UI-002` | Command Card 3x3 Grid Ergonomics | §17 UI and Interaction |
| `REL-UI-003` | Comprehensive Selection Inspector Panel | §17 UI and Interaction |
| `REL-UI-004` | Upper Resource Telemetry Deck | §17 UI and Interaction |
| `REL-UI-005` | Tactical Minimap Presentation & Frustum | §17 UI and Interaction |
| `REL-UI-006` | Spatial Alert Feed & Banner Notifications | §17 UI and Interaction |
| `REL-UI-007` | Interactive Technology Archive Tree | §17 UI and Interaction |
| `REL-UI-008` | Operational Escape Pause Menu | §17 UI and Interaction |
| `REL-UI-009` | Post-Match Results & Statistics Dossier | §17 UI and Interaction |
| `REL-UI-010` | Campaign Mission Briefing & Directive Terminal | §17 UI and Interaction |
| `REL-UI-011` | Context-Sensitive Mouse Cursor State Machine | §17 UI and Interaction |
| `REL-UI-012` | Ground Selection Ring Visual Hierarchy | §17 UI and Interaction |
| `REL-UI-013` | Comprehensive Screen Resolution Matrix | §17 UI and Interaction |
| `REL-UI-014` | Calibrated UI Scale Dynamic Range | §17 UI and Interaction |
| `REL-UI-015` | Input Keybind Deconfliction Invariant | §17 UI and Interaction |
| `REL-UI-016` | Layout Safety Margins for Localization | §17 UI and Interaction |
| `REL-UI-017` | see Part III | Initial release |
| `REL-UI-018` | see Part III | Initial release |
| `REL-UI-019` | see Part III | Initial release |
| `REL-UI-020` | see Part III | Initial release |
| `REL-UI-021` | see Part III | Initial release |
| `REL-UI-022` | see Part III | Initial release |
| `REL-UI-023` | see Part III | Initial release |
| `REL-UI-024` | see Part III | Initial release |
| `REL-WEL-001` | Future Well Landmark Entity Authority | §13 Future Wells |
| `REL-WEL-002` | Future Well Dormant State & Telemetry | §13 Future Wells |
| `REL-WEL-003` | Contested Well Capture Resolution | §13 Future Wells |
| `REL-WEL-004` | Well Contestation & Meter Decay | §13 Future Wells |
| `REL-WEL-005` | Harvest Protocol Public Telegraph | §13 Future Wells |
| `REL-WEL-006` | Harvest Collapse & Irreversible Scarring | §13 Future Wells |
| `REL-WEL-007` | Preserve Protocol Compounding Inflow | §13 Future Wells |
| `REL-WEL-008` | Preserve Radar Reconnaissance Aura | §13 Future Wells |
| `REL-WEL-009` | Preserve Contested Recapture Mechanics | §13 Future Wells |
| `REL-WEL-010` | Reshape Protocol Invocation | §13 Future Wells |
| `REL-WEL-011` | Reshape Terrain Transformation Passability | §13 Future Wells |
| `REL-WEL-012` | Reshape Expiration & Safe Boundary Displacement | §13 Future Wells |
| `REL-WEL-013` | see Part III | Initial release |
| `REL-WEL-014` | see Part III | Initial release |
| `REL-WEL-015` | see Part III | Initial release |
| `REL-WEL-016` | see Part III | Initial release |
| `REL-WEL-017` | see Part III | Initial release |
| `REL-WEL-018` | see Part III | Initial release |
| `REL-ACC-018` | Active Tactical Pause (Single Player) | §21 Accessibility and localization readiness |
| `REL-ACC-019` | Continuous Simulation Speed Scaling | §21 Accessibility and localization readiness |
| `REL-ACC-020` | Smart Macro Assist & Auto-Queue Toggle | §21 Accessibility and localization readiness |
| `REL-ACC-021` | Threat Warning Voice Assistant | §21 Accessibility and localization readiness |
| `REL-ACC-022` | Content Filter & Family Comfort Presets | §21 Accessibility and localization readiness |
| `REL-AI-037` | Minimap Tactical Ping Communication | §15 Skirmish, AI, difficulty, balance |
| `REL-AI-038` | Friendly AI Ally Comprehension & Force Dispatch | §15 Skirmish, AI, difficulty, balance |
| `REL-AI-039` | Cooperative Resource Tribute & Request System | §15 Skirmish, AI, difficulty, balance |
| `REL-AI-040` | Cooperative Comp-Stomp Skirmish Presets | §15 Skirmish, AI, difficulty, balance |
| `REL-ART-024` | Deterministic-Decoupled Kinetic Combat Ragdolls | §18 World art, units, structures, animatio... |
| `REL-ART-025` | Persistent Battlefield Scorch Decals & Vitrification | §18 World art, units, structures, animatio... |
| `REL-ART-026` | Dynamic Directional Shield Impact Ripples | §18 World art, units, structures, animatio... |
| `REL-ART-027` | Structural Critical Degradation States | §18 World art, units, structures, animatio... |
| `REL-AUD-017` | Bespoke Faction Tactical Announcers | §19 Audio, voice, music, cinematics |
| `REL-AUD-018` | Acoustic Environmental Spatial Occlusion & Reverb | §19 Audio, voice, music, cinematics |
| `REL-CAM-033` | "Shattered Sun Conquest" Planetary Meta-Map | §14 Campaign and narrative |
| `REL-CAM-034` | Procedural Sector Modifiers & Environmental Anomalies | §14 Campaign and narrative |
| `REL-CAM-035` | Persistent Faction Blueprint Research | §14 Campaign and narrative |
| `REL-CAM-036` | Conquest Roguelite Permadeath & Seed Sharing | §14 Campaign and narrative |
| `REL-CAM-037` | Dynamic AI Invasions & Territory Defense | §14 Campaign and narrative |
| `REL-CAM-038` | Conquest Milestone Dossiers & Commemorative Unlocks | §14 Campaign and narrative |
| `REL-CMB-028` | Shift-Queued Order Pipelining Execution | §11 Selection, movement, commands, combat |
| `REL-CMB-029` | Real-Time Waypoint Path Preview | §11 Selection, movement, commands, combat |
| `REL-CMB-030` | Smart-Cast Energy & Cooldown Conservation | §11 Selection, movement, commands, combat |
| `REL-CMB-031` | Priority Threat Resolution in Combat Formations | §11 Selection, movement, commands, combat |
| `REL-CMB-032` | Chase Leash Boundary Enforcement | §11 Selection, movement, commands, combat |
| `REL-EDT-001` | Native Scenario Editor Architecture | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-002` | Terrain Heightfield & Passability Sculpting | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-003` | Resource Deposit & Landmark Snapping | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-004` | Node-Based Tactical Event & Trigger Graph | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-005` | Custom Campaign Packaging & Manifest Compiler | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-006` | In-Game Community Map Browser & Loader | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-007` | Automated Map Validation Preflight Compiler | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-008` | Sandboxed Custom Asset Ingestion Policy | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-009` | Headless Map Compiler & Navigation Precomputation | §27 In-Engine Scenario and Map Editor |
| `REL-EDT-010` | Map Schema Forward Migration Discipline | §27 In-Engine Scenario and Map Editor |
| `REL-PUB-016` | The Soryn Archive In-Game Lore Codex | §24 Public website, manual, claims, support |
| `REL-PUB-017` | Interactive 3D Model Viewer & Tactical Dossier | §24 Public website, manual, claims, support |
| `REL-PUB-018` | Tactical Combat Sandbox / Testing Lab Mode | §24 Public website, manual, claims, support |
| `REL-PUB-019` | Historical Trophy Vault & Feat Tracking Gallery | §24 Public website, manual, claims, support |
| `REL-PUB-020` | Transparent In-Game Combat Mechanics Glossary | §24 Public website, manual, claims, support |
| `REL-QOL-013` | "Take Command" Savestate Replay Branching | §16 Replays and Quality-of-Life |
| `REL-QOL-014` | Replay Bookmark & Event Timeline Navigation | §16 Replays and Quality-of-Life |
| `REL-QOL-015` | Advanced Esports / Spectator Observer Deck | §16 Replays and Quality-of-Life |
| `REL-QOL-016` | Cinematic Smooth Camera & Freecam Spectating | §16 Replays and Quality-of-Life |
| `SPEC-CMD-011` | Shift-Queued Order Chaining | 6. Commands, orders, and player intent |
| `SPEC-CMD-012` | Waypoint Vector Breadcrumb Visualization | 6. Commands, orders, and player intent |
| `SPEC-CMD-013` | Smart-Cast Single-Unit Dispatch | 6. Commands, orders, and player intent |
| `SPEC-CMD-014` | Attack-Move Intelligent Threat Filtering | 6. Commands, orders, and player intent |
| `SPEC-CMD-015` | Focus-Fire Target Preservation on Range Loss | 6. Commands, orders, and player intent |
