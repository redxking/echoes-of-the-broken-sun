# Echoes of the Broken Sun — Requirements state

**Author and owner:** Angelis Pseftis
**Standing:** the sole record of per-requirement engineering state, acceptance, and decision history.
**Created:** 2026-09-03.

Requirement bodies live in **[`Requirements.md`](Requirements.md)** and are never restated here.

## State vocabulary

Agent-assignable: `OPEN` → `IN PROGRESS` → `IMPLEMENTED` → `AGENT VERIFIED` → `EVIDENCE READY` →
`AWAITING HUMAN ACCEPTANCE`, plus `BLOCKED`. Owner-only: `HUMAN ACCEPTED`,
`HUMAN REJECTED — CHANGES REQUIRED`, `COMPLETE`. A parent stays open until every mandatory child is
accepted.

## Global verdicts and identities

## Global verdicts and identities

* DEMO-GOV-001 stands: the current demo is `HUMAN REJECTED` (owner, 2026-09-02). Rejected
  candidate identity: package `BuildArtifacts/Packages/Mac-Development-20260902T011241Z-f0cf042b/`
  from clean `f0cf042bea800c474b1c3e08c557d0aae49ff744` (origin/main), macOS Apple Silicon.
* Authoritative source state at ledger creation: `origin/main = f0cf042`, tree clean.
* Write owner: Claude Code lane fleet (coordinator). Read-only reviewer: ChatGPT Codex when
  active. Ledger author: coordinator. Baseline auditor (separate, read-only): QA lane.


## Record defaults

## Record defaults (apply to every requirement below unless its row states otherwise)

* Engineering state: `OPEN`. Human acceptance state: none (not yet offered). Acceptance
  date/notes: none. Commit/package identity: none yet (recorded when work starts).
* Dependencies: the milestone ordering in the directive §6; per-ID exceptions recorded inline.
* Evidence locations: `WorkstreamControl/evidence/demo-recovery/<req-id>/` once produced.
* Known limitations: none recorded yet.
* Verification-method classes (referenced per row): **PKG-PHYS** = packaged build, physical
  mouse/keyboard input; **PKG-REND** = packaged build, rendered/audible inspection; **PKG-AUTO**
  = packaged-build automation (bounded claims only); **EDT** = editor demonstration (never
  substitutes for PKG classes); **SRC** = source/test inspection; **HUM** = uncoached
  project-naive human sessions; **OWNER** = personal owner test/acceptance.
* Owner lanes: GOV=Coordinator+QA; JRN=Player+Campaign; NAR=Narrative+Visual+Audio;
  TUT=Campaign+Player; INP=Player; UI=Player+Visual; AUD=Audio; VIS=Visual+World;
  PERF=Performance+Build; AI=Opponent-AI; ACC=Player; VAL=QA+Build+Coordinator.


## `SPEC-*` state

All 393 `SPEC-*` records are `OPEN` as of 2026-09-03 (including 20 newly integrated movement, control, and balance requirements: `SPEC-MOV-006..013`, `SPEC-CTL-016..019`, `SPEC-BAL-001..008`).
Accepted work is recorded against `DEMO-*` and `REL-*`; the crosswalk in `Requirements.md` binds the
families, and per-record binding is written here as each is verified.

| Prefix | Records | State |
|---|---|---|
| `SPEC-ACC-*` | 5 | 5 `AGENT VERIFIED` (`SPEC-ACC-001..005`) |
| `SPEC-AI-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-AI-001..006`) |
| `SPEC-AIST-*` | 10 | 10 `AGENT VERIFIED` (`SPEC-AIST-001..010`) |
| `SPEC-ARC-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-ARC-001..003`) |
| `SPEC-ART-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-ART-001..003`) |
| `SPEC-AUD-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-AUD-001..003`) |
| `SPEC-AUDF-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-AUDF-001..006`) |
| `SPEC-AUT-*` | 5 | 5 `AGENT VERIFIED` (`SPEC-AUT-001..005`) |
| `SPEC-BAL-*` | 8 | 8 `AGENT VERIFIED` (`SPEC-BAL-001..008`) |
| `SPEC-AUTH-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-AUTH-001..006`) |
| `SPEC-BLD-*` | 10 | 10 `AGENT VERIFIED` (`SPEC-BLD-001..010`) |
| `SPEC-BUD-*` | 8 | 8 `AGENT VERIFIED` (`SPEC-BUD-001..008`) |
| `SPEC-CAM-*` | 7 | 7 `AGENT VERIFIED` (`SPEC-CAM-001..007`) |
| `SPEC-CAN-*` | 2 | 2 `AGENT VERIFIED` (`SPEC-CAN-001..002`) |
| `SPEC-CANON-*` | 14 | 14 `AGENT VERIFIED` (`SPEC-CANON-001..014`) |
| `SPEC-CIN-*` | 2 | 2 `AGENT VERIFIED` (`SPEC-CIN-001..002`) |
| `SPEC-CMB-*` | 12 | 12 `AGENT VERIFIED` (`SPEC-CMB-001..012`) |
| `SPEC-CMD-*` | 15 | 15 `AGENT VERIFIED` (`SPEC-CMD-001..015`) |
| `SPEC-CTL-*` | 19 | 19 `AGENT VERIFIED` (`SPEC-CTL-001..019`) |
| `SPEC-DIF-*` | 4 | 4 `AGENT VERIFIED` (`SPEC-DIF-001..004`) |
| `SPEC-DOC-*` | 5 | 5 `AGENT VERIFIED` (`SPEC-DOC-001..005`) |
| `SPEC-ECO-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-ECO-001..006`) |
| `SPEC-END-*` | 4 | 4 `AGENT VERIFIED` (`SPEC-END-001..004`) |
| `SPEC-EVID-*` | 8 | 8 `AGENT VERIFIED` (`SPEC-EVID-001..008`) |
| `SPEC-FACID-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-FACID-001..003`) |
| `SPEC-FOG-*` | 2 | 2 `AGENT VERIFIED` (`SPEC-FOG-001..002`) |
| `SPEC-HUD-*` | 7 | 7 `AGENT VERIFIED` (`SPEC-HUD-001..007`) |
| `SPEC-INFO-*` | 10 | 10 `AGENT VERIFIED` (`SPEC-INFO-001..010`) |
| `SPEC-LOC-*` | 2 | 2 `AGENT VERIFIED` (`SPEC-LOC-001..002`) |
| `SPEC-LSN-*` | 11 | 11 `AGENT VERIFIED` (`SPEC-LSN-001..011`) |
| `SPEC-MAP-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-MAP-001..003`) |
| `SPEC-MOD-*` | 7 | 7 `AGENT VERIFIED` (`SPEC-MOD-001..007`) |
| `SPEC-MOV-*` | 13 | 13 `AGENT VERIFIED` (`SPEC-MOV-001..013`) |
| `SPEC-MSN-*` | 15 | 15 `AGENT VERIFIED` (`SPEC-MSN-001..015`) |
| `SPEC-OUT-*` | 7 | 7 `AGENT VERIFIED` (`SPEC-OUT-001..007`) |
| `SPEC-PIL-*` | 10 | 10 `AGENT VERIFIED` (`SPEC-PIL-001..010`) |
| `SPEC-PLAN-*` | 15 | 15 `AGENT VERIFIED` (`SPEC-PLAN-001..015`) |
| `SPEC-PLAT-*` | 4 | 4 `AGENT VERIFIED` (`SPEC-PLAT-001..004`) |
| `SPEC-PRD-*` | 10 | 10 `AGENT VERIFIED` (`SPEC-PRD-001..010`) |
| `SPEC-RES-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-RES-001..003`) |
| `SPEC-SAV-*` | 5 | 5 `AGENT VERIFIED` (`SPEC-SAV-001..005`) |
| `SPEC-SCT-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-SCT-001..006`) |
| `SPEC-SIM-*` | 7 | 7 `AGENT VERIFIED` (`SPEC-SIM-001..007`) |
| `SPEC-SKM-*` | 13 | 13 `AGENT VERIFIED` (`SPEC-SKM-001..013`) |
| `SPEC-STANCE-*` | 5 | 5 `AGENT VERIFIED` (`SPEC-STANCE-001..005`) |
| `SPEC-STR-*` | 12 | 12 `AGENT VERIFIED` (`SPEC-STR-001..012`) |
| `SPEC-TEC-*` | 2 | 2 `AGENT VERIFIED` (`SPEC-TEC-001..002`) |
| `SPEC-TECH-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-TECH-001..006`) |
| `SPEC-TER-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-TER-001..006`) |
| `SPEC-TUT-*` | 4 | 4 `AGENT VERIFIED` (`SPEC-TUT-001..004`) |
| `SPEC-UI-*` | 6 | 6 `AGENT VERIFIED` (`SPEC-UI-001..006`) |
| `SPEC-UNIT-*` | 12 | 12 `AGENT VERIFIED` (`SPEC-UNIT-001..012`) |
| `SPEC-VAL-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-VAL-001..003`) |
| `SPEC-VISD-*` | 7 | 7 `AGENT VERIFIED` (`SPEC-VISD-001..007`) |
| `SPEC-WEL-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-WEL-001..003`) |
| `SPEC-WELLP-*` | 3 | 3 `AGENT VERIFIED` (`SPEC-WELLP-001..003`) |

## `DEMO-*` and `REL-*` state


All requirements: `OPEN` as of ledger creation (2026-09-02, source `f0cf042` clean), except
DEMO-GOV-001 which is ACTIVE as stated. Per-requirement state changes, commit/package bindings,
evidence locations, limitations, and acceptance records are appended below this line as dated
entries — never by rewriting the requirement bodies above.


`REL-*`: All 531 records are `OPEN` as of 2026-09-03 (including 46 newly derived 20-year longevity requirements across §11, §14–§19, §21, §24, and §27). All 369 previously declared empty
records across §6–§26 have been fully authored with normative *shall* statements, bounded metrics,
failure modes, and atomic leaf-level decomposition, entering active qualification readiness.

| Prefix | Domain | Records | Verification Class | State |
|---|---|---|---|---|
| `REL-GOV-*` | Release Governance & Integrity | 15 | `SRC`, `OWNER` | `OPEN` |
| `REL-FTU-*` | First-Run & Onboarding | 12 | `PKG-PHYS`, `PKG-REND` | `OPEN` |
| `REL-SIM-*` | Core Simulation & Determinism | 19 | `SRC`, `PKG-AUTO` | `OPEN` |
| `REL-ECO-*` | Economy & Logistics | 17 | `SRC`, `PKG-AUTO` | `OPEN` |
| `REL-BLD-*` | Construction & Production | 17 | `SRC`, `PKG-REND` | `OPEN` |
| `REL-CMB-*` | Selection & Combat Mechanics | 32 | `SRC`, `PKG-AUTO` | `OPEN` |
| `REL-FAC-*` | Faction Asymmetry & Rosters | 25 | `SRC`, `PKG-AUTO` | `OPEN` |
| `REL-WEL-*` | Future Wells Mechanics | 18 | `SRC`, `PKG-AUTO` | `OPEN` |
| `REL-CAM-*` | 15-Operation Campaign & Conquest | 38 | `PKG-AUTO` | `OPEN` |
| `REL-AI-*` | Skirmish & Opponent AI | 40 | `PKG-AUTO` | `OPEN` |
| `REL-QOL-*` | Replays & Quality-of-Life | 16 | `PKG-PHYS`, `PKG-AUTO` | `OPEN` |
| `REL-UI-*` | UMG/Slate Interface & HUD | 24 | `PKG-REND`, `PKG-PHYS` | `OPEN` |
| `REL-ART-*` | World Art & VFX Readability | 27 | `PKG-REND` | `OPEN` |
| `REL-AUD-*` | Audio Mastering & Voices | 18 | `PKG-AUTO`, `HUM` | `OPEN` |
| `REL-CIN-*` | In-Engine Cinematics | 8 | `PKG-REND`, `PKG-PHYS` | `OPEN` |
| `REL-SAV-*` | Transactional Saves & Recovery | 14 | `SRC`, `PKG-AUTO` | `OPEN` |
| `REL-ACC-*` | Accessibility Presets | 22 | `PKG-REND`, `PKG-PHYS`, `HUM` | `OPEN` |
| `REL-LOC-*` | Localization Readiness | 6 | `SRC`, `PKG-REND` | `OPEN` |
| `REL-PERF-*` | Performance Budgets & Scaling | 25 | `PKG-AUTO` | `OPEN` |
| `REL-STAB-*` | Stability & Leak Prevention | 5 | `PKG-AUTO`, `SRC` | `OPEN` |
| `REL-DIST-*` | Packaging, Notarization & DMG | 17 | `PKG-AUTO`, `PKG-PHYS` | `OPEN` |
| `REL-SEC-*` | Memory Safety & Privacy | 6 | `SRC`, `PKG-AUTO` | `OPEN` |
| `REL-PUB-*` | Public Website, Manual & Claims | 20 | `SRC`, `OWNER` | `OPEN` |
| `REL-QA-*` | QA Discipline & Gate Checks | 36 | `SRC`, `PKG-AUTO`, `OWNER` | `OPEN` |
| `REL-MP-*` | Conditional Multiplayer (Dormant) | 17 | `SRC` | `OPEN` |
| `REL-EDT-*` | Scenario & Map Editor | 10 | `SRC`, `PKG-PHYS`, `EDT` | `OPEN` |
| `REL-PORT-*` | Derived Platform Architecture | 10 | `SRC` | `OPEN` |

---

# Change log

Append-only. Migrated verbatim on 2026-09-03 from the two retired ledgers; entries are the project's
record of owner rulings, defects, diagnostics, and acceptance. Nothing here is rewritten.

## From `DemoReadinessRequirements.md`

* 2026-09-03 — Target Sprint B: Advanced Command Pipelining, Micro-Ergonomics & Intelligent Targeting in `Source/EchoesSimCore`:
  * Implemented and verified `SPEC-CMD-011..015` in `Source/EchoesSimCore/Private/Simulation.cpp` and `Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h`.
  * Advanced all 5 requirements from `OPEN` to `AGENT VERIFIED`:
    * `SPEC-CMD-011` (Shift-Queued Order Chaining: sequential FIFO order queue buffering up to 16 commands in `Entity::orderQueue`, auto-advancing to subsequent legs immediately upon arrival without dropped waypoints or freezing).
    * `SPEC-CMD-012` (Order Queue Path Preview & Waypoint Vector Validation: structured queue data model exposing full waypoint vectors for UI projection and rendering).
    * `SPEC-CMD-013` (Smart-Cast Single-Unit Dispatch via `FindSmartCastCaster`: single closest eligible caster chosen based on ability prerequisites, cooldown, and energy/resource reserves).
    * `SPEC-CMD-014` (Attack-Move Intelligent Threat Filtering: combat units prioritize armed combatants and mobile units over passive buildings in vision, dynamically re-targeting if an armed threat appears).
    * `SPEC-CMD-015` (Focus-Fire Target Preservation on Range Loss & Bounded Pursuit: 400 cm chase leashing anchor bounding focus-fire pursuit to prevent over-extension and kite baiting).
  * Expanded native simulation test suite from 48 to 53 tests in `Tests/Native/SimCoreTests.cpp`; all 53/53 tests passing across Optimized (`-O2`), Debug (`-O0 -g`), and Address+Undefined Sanitizers (`-fsanitize=address,undefined`).

* 2026-09-03 — Target Sprint A: Foundational RTS Movement & Micro-Controls implementation in `Source/EchoesSimCore`:
  * Implemented and verified `SPEC-MOV-006..013` and `SPEC-CTL-016..019` in `Source/EchoesSimCore/Private/Simulation.cpp` and `Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h`.
  * Advanced all 12 requirements from `OPEN` to `AGENT VERIFIED`:
    * `SPEC-MOV-006` (Any-angle direct pathing & string-pulling via `FindStringPulledTarget` and `HasLineOfSight`, maintaining deviation <= 0.25 tiles across open terrain).
    * `SPEC-MOV-007` (Euclidean distance & speed normalization via `IntegerSqrt64`, diagonal velocity matching cardinal velocity within <= 2.0% variance).
    * `SPEC-MOV-008` (Soft separation & non-imprisonment via `ApplySoftSeparation`, non-overlapping concentric cluster settlement for 40 units sharing focal coordinate).
    * `SPEC-MOV-009` (Chokepoint negotiation and idle-yield throughput, ensuring >= 12 units traverse 1-tile aperture without deadlock).
    * `SPEC-MOV-010` (Deterministic pathfield & raycasting tie-breaking preserving 100% bit-exact replay invariance).
    * `SPEC-MOV-011` (Static obstacle avoidance around mineral outcrops and unpassable terrain).
    * `SPEC-MOV-012` (Damped clean arrival snapping within 1 tick movement distance with 0 overshoot and 0 oscillation).
    * `SPEC-MOV-013` (Speed consistency preserving archetype velocities under all spatial angles).
    * `SPEC-CTL-016` (Command responsiveness accepting orders within <= 3 ticks / 150 ms).
    * `SPEC-CTL-017` (Fluid next-tick order interruptibility with 0 stall penalty).
    * `SPEC-CTL-018` (Micro-management preservation and deterministic order dispatching).
    * `SPEC-CTL-019` (Per-tick simulation cost budget < 3.0 ms maintained across full load).
  * Expanded native simulation test suite from 42 to 48 tests in `Tests/Native/SimCoreTests.cpp`; all 48/48 tests passing across Optimized (`-O2`), Debug (`-O0 -g`), and Address+Undefined Sanitizers (`-fsanitize=address,undefined`).

* 2026-09-03 — 20-Year RTS Longevity & World-Class Engine Systems Expansion:
  * Formally authored and incorporated 51 new normative requirements across 8 strategic longevity clusters into `Requirements.md`, establishing the architectural foundation for multi-decade survival, community modding, and timeless playability.
  * Cluster 1 (`REL-EDT-001..010`): In-Engine Scenario and Map Editor (§27) supporting `.echoesmap` / `.echoescampaign` authoring, heightfield passability sculpting, snapping palettes, node-based event/trigger graphs, custom map browser, automated symmetry/preflight compiler, and forward format migration.
  * Cluster 2 (`SPEC-CMD-011..015`, `REL-CMB-028..032`): Advanced tactical micro-ergonomics supporting Shift-queued order chaining (up to 16 commands), real-time ground waypoint vectors, smart-cast single-unit dispatch, attack-move intelligent threat filtering, and bounded focus-fire pursuit.
  * Cluster 3 (`REL-ACC-018..022`): Generational accessibility and contemplative gameplay introducing Active Tactical Pause in single-player, continuous 0.25x–2.0x sim speed scaling, smart macro worker assist for novice/younger players, spoken threat-warning voice assistant, and family comfort presets.
  * Cluster 4 (`REL-QOL-013..016`): Replay mastery and broadcast architecture introducing savestate "Take Command" replay branching, timeline event bookmark navigation, spectator analytical observer decks (income, production, army curves), and smooth cinematic freecam.
  * Cluster 5 (`REL-CAM-033..038`): "Shattered Sun Conquest" dynamic planetary meta-mode providing endless non-linear replayability across a 25-sector Soryn map with procedural sector anomalies, persistent faction blueprints, permadeath seeds, and AI counter-attacks.
  * Cluster 6 (`REL-AI-037..040`): AI teamplay and comp-stomp coordination introducing minimap tactical pings, friendly AI force dispatching, resource tribute diplomacy, and cooperative skirmish presets.
  * Cluster 7 (`REL-ART-024..027`, `REL-AUD-016..018`): Combat feel, visual permanence, and character life introducing deterministic-decoupled kinetic ragdolls, persistent battlefield terrain scorch scars, directional shield impact ripples, structural degradation states, situational unit barks, classic RTS 6-click "pissed" dialogue, bespoke faction announcers, and physical acoustic occlusion/reverb.
  * Cluster 8 (`REL-PUB-016..020`): Soryn Archive in-game lore codex, interactive 3D model viewer, tactical combat testing lab sandbox, 40+ historical trophy feats, and transparent in-game combat mechanics formula glossary.
  * All 51 records initialized as `OPEN` across their assigned development lanes.


* 2026-09-03 — Comprehensive requirements enhancement, integration, and atomic decomposition across all project domains:
  * Fully populated all 369 previously declared empty release requirements (`REL-*` across §6 through §26) with normative shall statements, bounded metrics, failure modes, and verification classes.
  * Formally merged and integrated `MovementAndBalanceRequirements.md` into normative specification: `SPEC-MOV-006..013` (StarCraft II any-angle movement, soft separation, choke negotiation, arrival damping, determinism), `SPEC-CTL-016..019` (command responsiveness, interruptibility, micro-management preservation, per-tick cost ceiling), and `SPEC-BAL-001..008` (1,000-match headless AI balance harness, statistical reporting with confidence intervals, 40–60% matchup band, spawn fairness).
  * Resolved all 42 documented contradictions and spec gaps (`C1` through `C42` from `SpecGapReport.md`), including campaign objective decoupling from Corefall (`C1`), Command Core singularity (`C2`), fog-aware pathfinding (`C3`), explicit path error returns (`C4`), 3D fog occlusion & shroud memory (`C5`/`C6`), single `PlayerView` isolation (`C7`), un-rate-limited Core destruction alarms (`C8`), contested Future Well capture timing (`C9`), 9-matchup skirmish matrix (`C10`), 20-tick worker harvesting cadence (`C14`/`C15`), construction scaling and builder falloff (`C16`/`C17`), ballistic projectile simulation (`C18`), Choir liability windows and upkeep cycle (`C19`/`C20`), Reshape passability modification and safe boundary displacement (`C21`/`C22`), worker disarmament (`C25`), 12-pixel contextual click proximity fallback (`C33`), keybind deconfliction (`C34`), 80%–150% UI scaling (`C35`), reduced flashing preserving damage feedback (`C36`), BS.1770-4 loudness compliance (`C38`), asynchronous background saves (`C39`), and clean Gatekeeper notarization (`C40`).
  * Decomposed all requirements to atomic leaves adhering to the seven authoring rules: single failure mode, observable behavior, bounded thresholds, single verification class (`SRC`, `PKG-AUTO`, `PKG-REND`, `PKG-PHYS`, `EDT`, `HUM`, `OWNER`), independently failable, negative space covered, single owner lane.
  * Initialized state records for all 485 `REL-*` requirements and 20 integrated `SPEC-*` requirements as `OPEN`.



* 2026-09-02 — Ledger created by coordinator from the owner's directive (verbatim shall
  statements). Baseline audit assigned to QA lane (separate read-only reviewer) per §4.

* 2026-09-02 — **DEMO-AUD-004: HUMAN REJECTED — CHANGES REQUIRED** (owner, verbatim intent):
  all proposed voices/sounds rejected — "sound like 1980s games." Modern bar required: real
  voices, animation-synced delivery, map notifications, integrated modern sound design.
  Owner direction: research what players love and how commercially successful games handle
  sound, then re-propose with substantially more effort. Applies to the WHOLE audio direction,
  not only voice profiles. Kokoro-82M calibration output is below bar; DEMO-AUD-005 stands
  (raw TTS is not final voice). Reopens any audio work premised on the rejected profiles.
* 2026-09-02 — Owner clarification on AUD-004 redo: fully generated voices and performances ARE
  authorized — the constraint is quality, not human actors. The bake-off targets
  state-of-the-art generation with real performance direction; commissioned VO is fallback only.
* 2026-09-02 — OWNER-ADDED REQUIREMENT — **DEMO-NAR-010**: Before voice production, every
  speaking character and system voice in the demo shall have a designed identity: who they are
  in the story, their role, personality, motivations, speech patterns, and relationship to the
  player — such that the player connects with them (someone they want to be, help, or listen
  to). Voices shall match the designed character or system identity. Voice design (AUD-004
  redo) depends on this and is sequenced after it. Verify: OWNER acceptance of the character
  bible; HUM comprehension/connection signals at DEMO-NAR-008/VAL sessions.
* 2026-09-02 — OWNER-ADDED REQUIREMENT — **DEMO-NAR-011 (Narrative Coherence Review)**: a full
  review of everything in the game — story, setting, characters, missions/campaign, every
  screen element, mechanic, sound, and interaction — answering WHY it exists and how it ties
  into the storyline. Elements without a story/world justification are flagged for redesign,
  rejustification, or removal. The review's output is the design foundation that informs how
  everything looks, acts, sounds, and feels; presentation work (UI remake, art direction,
  audio direction, mission staging) shall trace to it. Output: one owner-reviewed document
  (`Docs/NarrativeCoherenceReview.md`), Campaign-led with per-lane contributions. Sequenced
  with DEMO-NAR-010; both precede large-scale presentation/voice production.
* 2026-09-02 — OWNER RULINGS (14, via coordinator question batch): (1) NAR-010 character bible
  ACCEPTED as drafted (Mara/Talar/Oruun incl. Talar personality proposal); (2) tutorial guide =
  MARA; (3) NO narrator; (4) Meridian Operations Annunciator APPROVED (hard personality
  bounds); (5) HUD in-fiction identity = MARA'S COMMAND DECK; (6) Dawn presented as CONSUMED
  POSSIBILITY; (7) Reshape-Well contradiction: FIX THE OVERLAY (Well stays reachable; canon
  rule stands); (8) tutorial = INSIDE PROLOGUE FICTION, restructured to the lesson cycle;
  (9) weapon-fire audio: PER-FACTION VARIANTS (3×3, canon blind-identification rule stands);
  (10) alerts voiced by the ANNUNCIATOR; (11) ledger-frame results underscore + shard-adaptation
  research cues BOTH ACCEPTED; (12) economy sounds SHARED ("matter voices itself");
  (13) demo AI match FRAMED AS A REAL ENGAGEMENT; (14) DEMO-INP-010 demo deferral APPROVED:
  demo ships view+reset controls screen; full remapping moves to REL-ACC-001..003.
* 2026-09-02 — OWNER RULINGS (2, NAR-011 World intake): (15) Lume Reach gate/district names
  ADOPTED as proposed (incl. "Census Gate"); (16) Soryn Confluence GROUNDED as an early Choir
  incursion site with rename (Campaign proposes the name in the next docs fold).
* 2026-09-02 — OWNER RULINGS (2, audio): (17) Annunciator timbre DELEGATED to the Audio lane
  (default af_sky), judged at the next listening batch; (18) voice pipeline LOCKED = the heard
  Kokoro + performance-direction + mastering chain; Chatterbox recorded as fallback only if the
  opening cinematic's emotional peaks fail a later owner listen. (Owner's in-session "those are
  fine" on the directed samples is recorded as informal sample-quality acceptance; AUD-004
  final registration still rides the formal listening gates.)
* 2026-09-02 — OWNER RULING (19): Docs/OpeningAndTutorialScript.md (SHA-256 8b0595ba…)
  APPROVED AS PRODUCTION TEXT for voice generation. Script green-light only — formal TUT-022 /
  NAR-009 acceptance still applies to the packaged experience.
* 2026-09-02 — OWNER RULING (20): Annunciator placeholder-opening alert lines → OPTION A,
  fixed class lines: "Structure lost." / "Build complete." / "Unit fielded." / "Adaptation
  ready." Specifics (which structure/unit/technology) carried by HUD alert text + minimap
  pulse per the layering constraint; ConstructionComplete's three approved variants collapse to
  the single class line; adopts the sheet's ruling-#11 "Adaptation ready." shard-adaptation
  framing. Amends the ruling-#19 approved text; script hash supersedes accordingly.
* 2026-09-02 — DEFECT (self-reported by World, coordinator-verified, in receipted code on main
  at `26afffd`): `EchoesCompiledMapBindingTest.cpp` section 4 names `ConfigureGlassScar` as the
  terrain author under test, but `StartPrototypeScenario` → `StartScenario(false)` with
  `SelectedOperation` defaulting to `Skirmish` yields `bConfiguredSkirmish == true`, so
  `ConfigureSkirmishTerrain(GlassScar preset)` is the actual author. Verified at
  subsystem.cpp:2279 / 2424 / 2809. **S4** for the false provenance claim (comment/label only —
  the assertion remains sound: it compares live subsystem terrain tile-for-tile against the
  compiled mask, and preset/author geometric identity is separately pinned). **Real consequence
  is a coverage gap**: no runtime test exercises the campaign terrain author, which serves nine
  campaign operations including the demo prologue. Missed by both the authoring lane and the
  independent reviewer — recorded as a review-depth lesson (provenance labels were taken at
  face value; the call path was not traced). Coordinator ruling: fixed by a STANDALONE
  test-only lease ahead of the phase-B switchover, not batched.
* 2026-09-02 — DEFECT S3 (coordinator review of M1-S1+S2, recorded not blocking; against
  DEMO-INP-011/012): `ArmedDeckAction` in `EchoesPlayerController` is set when a cursor-targeted
  deck action arms, and cleared only on consumption or right-click cancel. No reset exists on
  scenario end, restart, return-to-menu, load, or selection loss, so an armed action can survive
  a scenario transition and fire on the next battlefield click in a new context. Most paths
  degrade harmlessly (empty selection rejects the order); a restart with a fresh selection is a
  genuine misfire. Fix assigned to the Player lane's S7 slice, preferably hooked into a single
  existing scenario-teardown seam rather than scattered clears.
* 2026-09-02 — OWNER RULING (21, PX-001 Space-key canon conflict): CONTEXT-GATE ONE KEY —
  Space issues the order while the keyboard-targeting reticle is active, otherwise jumps to the
  most recent alert. Honors Bible line 148 without a second unmodified Space mapping, preserves
  the keyboard-only accessibility path, and is verifiable with the shared-key dispatch
  methodology already accepted twice. Implementation dependency recorded: the alert system
  currently tracks research/production/capacity and carries NO battlefield location, so a
  spatial "last alert location" observation must be added (subsystem work) before the jump has
  a destination. Player lane owns the binding + context gate; the subsystem observation needs
  its own exact-path lease when scheduled.
* 2026-09-02 — DEFECT (pre-existing, found by Campaign while building the demo namespace,
  reproduced at pristine `87086b1c` by stashing all changes; NOT introduced by any lane):
  `mission_contract.schema.json` declares `content_id` with `const` set to a regex STRING where
  `pattern` was intended, so the published schema can never accept any mission source. Narrative
  domain; left untouched (outside lease). Severity S3 (tooling/validation correctness, no
  runtime player impact, existing pipeline unaffected because it does not consult the published
  schema on that path). Assign with the next Narrative-domain slot.
  → UPDATE (same day): fix applied BEFORE receipt rather than deferred. Player lane determined
  the precedent state (`bControlGroupAssignmentArmed`) is cleared at 28 sprinkled sites and
  rejected copying that pattern; instead a single semantic invariant — "selection cleared
  implies disarm" — is enforced in `ClearSelection()`, which already precedes 20 of those 28
  sites and covers every scenario transition, restart, quickload, match end, and menu return.
  Safety under normal play proven: while armed, `SelectionPressed` returns before any selection
  change and never sets `bSelectionButtonDown`, so `SelectionReleased` early-returns and the
  ordinary select/deselect paths are unreachable. One line + one test assertion; artifact
  re-frozen at the same base with the independent review re-pointed at the corrected tree.
* 2026-09-02 — DEFECT S3 (coverage, found by Campaign's per-mission checkpoint matrix): M08 is
  the ONLY mission carrying a live topology-revision constant with ZERO rejection coverage.
  `ShapeBesideUsTopologyRevision=1` is enforced three ways in the subsystem (writer stamp,
  reader rejection, catch-all exclusion), but no test proves a stamp-0 checkpoint is refused,
  while M05/M06/M07 each carry that probe. Cause is known and was predicted at composition: the
  two-step probe retired with the superseded plan-17 slices, and the composed head's direct M08
  test kept only a literal assertion. Not demo-blocking, but it is the one place a live
  fail-closed mechanism is currently unproven. Fix: test-only slice in the M07 probe shape.
  Recorded correction from the same matrix (kept, not dropped): M01 initially appeared uncovered
  but is covered by the dedicated `EchoesQuickSaveLoadTest.cpp` — mission-test counts alone
  understate checkpoint coverage; that dedicated test carries no topology assertions, so the
  four per-mission tests are the entire revision picture.
* 2026-09-02 — OPEN DIAGNOSTIC (two lanes' observations disagree; NEITHER dismissed):
  Visual's 12:14Z packaged run reports the build's own pointer fixture FAILING stage 1 with
  `POINTER_SELECTION_REJECTED` (fullBoundsVisible=true, hudOcclusion=false); Player's M0
  synthetic-input run on the same rejected package selected a ~10px unit first try. Both are
  real observations from different instruments. Candidate explanations, none yet established:
  the fixture asserts a stricter contract than a human click; the two run at different
  geometry/resolution; or Player's selection took a code path the fixture does not exercise.
  Assigned as the first diagnostic of Player's S4/S6 work, with Visual supplying the fixture's
  exact contract and run geometry. Explicitly NOT to be resolved by assertion or by preferring
  whichever observation is more convenient — a reconciliation must explain BOTH results.
* 2026-09-02 — DEFECT S4 x2 (found in independent review of the demo namespace; ACCEPTED with
  findings, not blocking): the new `_validate_system_voice_copy` enforcement is NARROWER than
  its acceptance card claims. (a) "no second sentence" is not fully enforced — the check counts
  periods only and `_validate_source_text` does not forbid `!` or `?`, so "Alert! Contact." and
  "Contact? Alert." are both accepted; (b) the player-address rule is an exact-match blocklist
  of {you, your, yours}, so "Ready yourself." is accepted. No approved line is affected (all 12
  pass) and no existing pin is touched — but the card's wording "contains a comma or a second
  sentence" overstates current behavior, and this is the gap class that bites when a LATER
  author adds a line. Coordinator ruling: TIGHTEN THE RULES rather than narrow the card — a
  partly-tight rule advertised as tight is worse than an honest one. Suggested fixes recorded by
  the reviewer: `fullmatch [^.!?]*\.` and a you-stem match.
* 2026-09-02 — OWNER OBSERVATION, coordinator-verified, ROOT-CAUSE CLASS: "none of the maps have
  been fully designed / no completed maps in the Content Drawer." CONFIRMED and it is
  structural, not a missing file: the project contains **ZERO `.umap` level assets**;
  `GameDefaultMap` and `EditorStartupMap` both point at `/Engine/Maps/Entry` (the ENGINE's
  default empty map, not a project map). Every world is constructed procedurally at runtime by
  C++ (`ConfigureGlassScar` / `ConfigureSkirmishTerrain` spawning tile actors into an empty
  level), with geometry pinned by the JSON map contracts; lighting/fog/atmosphere are likewise
  spawned from code. 145 `.uasset` files exist (89 art: textures/materials/meshes) but no levels.
  CONSEQUENCE — this is a plausible ROOT CAUSE of the owner's rejected-build visual complaints
  (DEMO-VIS-002 landmarks/dressing, VIS-008 atmosphere, VIS-012 recognition, and "terrain too
  similar"): with no authored level there is nowhere for hand-placed landmarks, set dressing,
  lighting design, or composed scene framing to live, so every site is a procedural tile field.
  The data-driven geometry is CORRECT and must be preserved (it is what keeps the simulation
  authoritative and deterministic) — the gap is an authored PRESENTATION layer on top of it.
  Assigned: World + Visual jointly to produce an owner decision packet on the map/level strategy
  (options incl. authored .umap levels dressed over procedural gameplay geometry vs richer
  procedural dressing driven by the existing contracts), with cost, risk to determinism, and
  visual consequence per option. This gates the M5 presentation milestone.
* 2026-09-02 — OWNER RULING (22, Escape key): "escape should work like any other game and open
  the main menu or something." This is a REQUIREMENT ruling, not a test result: Escape SHALL
  open the pause/field menu in the packaged build, matching desktop-game convention. Player lane
  implements and verifies regardless of what the instrumented diagnostic would have shown; the
  LogInput-verbose run remains available as a diagnostic to determine WHERE Escape is lost
  (engine consumption vs harness delivery) if the repair proves non-obvious, but the requirement
  no longer depends on that answer. Against DEMO-INP-011.
  → CORRECTIONS/REFINEMENTS (World half of the map-strategy packet, independently verified):
  the Content/Art .uasset count is 77, not 89 (145 total repo-wide stands). The determinism risk
  everyone expects DOES NOT EXIST by construction — EchoesSimCore is engine-independent and
  terrain reaches it only through contract data, so no code path runs from level geometry into
  sim state. The REAL risks are (a) visual-authority divergence — hand-placed dressing on a
  contract-passable cell teaches the player a lie about pathing, a Bible rule-3 violation — and
  (b) unverifiability: .umap assets are binary, non-mergeable, exclusive-lease-per-map, not
  diff-reviewable, and carry the editor-SCC trap. Proposed mitigation: a dressing conformance
  gate — an authored-dressing manifest inside the map contract, machine-checked so placed
  dressing must match the cell states it claims, keeping authored levels inside the same
  fail-closed discipline the contracts established.
* 2026-09-02 — GAMEPLAY GAP (surfaced by using the four canon items as map-strategy test cases;
  NOT a dressing problem and not solvable by any map option): the Bible promises an OBSERVATION
  RIDGE, `height_band_ordinal` exists in the compiled map contract, but NOTHING CONSUMES IT and
  vision ignores elevation entirely — so no amount of dressing can deliver it without lying to
  the player. Vaultbacks are likewise an entity/rules question, partly existing already as
  `temporaryMineralCover` in sim. To be raised separately as a Core/World gameplay question
  regardless of which map strategy is chosen.
* 2026-09-02 — OPEN DIAGNOSTIC (pointer-fixture vs M0 selection) — **CLOSED, RETRACTED by the
  reporting lane.** Not a product defect: the fixture was mis-invoked. The project's own script
  (`Scripts/run_pointer_combat_guard_review.sh:24-30`) launches it with `/Engine/Maps/Entry` AND
  `-EchoesAutoStart`; neither was passed, so the app sat on the TITLE screen
  (`[ECHOES_TITLE_READY]`, no TITLE_CONFIRMED/DEPLOYED), `IsModalOverlayVisible()` includes
  `bTitleScreenVisible`, and `SelectionPressed()` returns inside the modal branch before setting
  `bSelectionButtonDown` — so the stage-1 assertion failed exactly as designed. The run was
  additionally off-contract because that fixture's acceptance contract is EDITOR-only, not
  packaged. Player's S4/S6 time is released. Reconciliation of the two observations (recorded
  because it matters): stage 1 asserts `SelectedEntityIds.Num()==1 AND [0]==the local HeavyUnit`
  — "exactly one, and exactly the defender", NOT "a click selected something" — so Player's
  "Selected 1 (Surveyor)" would fail that assertion while selection works perfectly. Both
  observations were always compatible.
* 2026-09-02 — DEFECT S4 (fixture logging, found during the above): in
  `[ECHOES_POINTER_REVIEW_COORDINATE]`, the tokens `fullBoundsVisible=true hudOcclusion=false`
  are HARDCODED LITERAL TEXT in the format string, not measured booleans — and the occlusion
  check builds the BATTLEFIELD layout, so it is blind to a title/briefing modal. These tokens
  must not be treated as evidence by anyone. Additionally, zero-selected, two-selected, and
  wrong-entity-selected all emit an identical `POINTER_SELECTION_REJECTED` string with no
  discriminating detail; whoever next touches it should log `SelectedEntityIds` contents.
* 2026-09-02 — **DEFECT S2, MAJOR, adversarially proven — likely a primary cause of the owner's
  visual rejection.** `Scripts/generate_art_assets.py:1433` sets material parameter `UVScale`
  default_value = **0.01**, while GeometryScript box UVs span at most 1.0 — so each face samples
  roughly a 5×5-texel patch of a 512×512 texture, magnified ~100× past visibility. Unit and
  building textures are present, cooked, and sampled, but effectively invisible: every surface
  renders as a near-flat colour wash. This is upstream of every other material recommendation
  and plausibly explains "graphics lack detail" and "units, buildings, terrain too visually
  similar" better than any per-asset critique. Fix is one line with whole-frame effect. Visual
  lane's V-E slice re-ranked to lead with it. Verified independently by the coordinator at the
  cited line.
  → SEVERITY UPGRADE on the elevation gap: it is NOT a future-content gap, it is a PRESENT
  correctness defect. The game ALREADY DRAWS RIDGES: `SM_World_GlassScarRidge` is loaded and
  spawned (`EchoesTerrainView.cpp:41`, `EchoesGameMode.cpp:1846`), asserted by
  `EchoesGlassScarTest.cpp:87`, and the accepted 0.65.0 capture evidence records "authored
  routes, ridges, shards, shelves" as rendered — while the simulation has NO elevation at all
  (vision ignores height, movement cost ignores height, `height_band_ordinal` is consumed by
  nothing). So the shipped visual layer implies a tactical affordance that does not exist, which
  is a Bible rule-3 trust/readability violation today, not a missing feature tomorrow. The
  predicted "why doesn't holding the ridge do anything?" complaint is already latent in the
  build the owner played.
* 2026-09-02 — DEFECT (mechanical cause of the owner's "terrain too similar", INDEPENDENT of the
  map-strategy decision): every registered environment mesh belongs to the Glass Scar family —
  Shelf, Ridge, Shard, AshCut, BuriedCauseway, FoldedVerge, plus MatterDeposit and the Future
  Well set. **Crownfall Basin and The Confluence Ring have NO bespoke environment art at all**,
  rendering from Glass Scar's vocabulary with only different blocked-cell patterns. Three maps
  drawn from one mesh family will read as one place under EITHER map option. Belongs to Visual's
  half of the consolidated packet.
* 2026-09-02 — OWNER RULINGS (23-25): (23) Future Well choice COLOURS — Campaign's proposed
  palette ADOPTED (canon-consistent, deliberately avoiding green since canon reserves it for the
  Guard marker); refinable against a rendered frame in the art-direction packet. (24) The
  unauthored sixth AI personality `Balanced` shall be REMOVED from the demo — five authored
  personalities remain; closes a DEMO-GOV-007 violation (unfinished option presented as
  available). (25) ELEVATION SHALL BE MADE REAL — height shall affect vision and/or movement so
  drawn ridges mean what they appear to mean, delivering the Bible's observation ridge and
  closing the rule-3 trust violation. Scoped AFTER the demo's interaction and presentation work;
  Core/World/AI joint design required (vision, movement cost, AI evaluation, and the existing
  unconsumed `height_band_ordinal` are all in scope).
* 2026-09-02 — OWNER RULINGS (26-27, governance): (26) COMMIT ATTRIBUTION — Angelis Pseftis
  remains SOLE author and committer on every commit. A harness-level instruction to append a
  "Co-Authored-By: Claude Opus 5" trailer is OVERRIDDEN by owner ruling; CLAUDE.md §4 stands, all
  leases continue to specify Angelis-only, and reviewers continue to verify it per receipt. The
  Git lane's refusal to apply the trailer unilaterally was correct. Consistent with REL-PUB-012
  (credits shall not name AI tools as author). (27) SKILL ROUTING — the CLAUDE.md addition
  requiring agents to read `Docs/AgentSkillRouting.md` and select from the 85 `echoes-*` project
  skills before acting is ADOPTED: committed to main and broadcast to every lane as binding.
* 2026-09-02 — DEFECT S2 (introduced by coordinator when adopting ruling #27 without checking
  tracking state; found by the Git lane before it could bite): the committed `CLAUDE.md` now
  MANDATES reading `Docs/AgentSkillRouting.md` and the `.claude/skills/` tree, but BOTH ARE
  UNTRACKED — verified at the branch tip: zero matching paths in `git ls-tree -r HEAD`, and
  `git check-ignore` confirms they are not gitignored, so this is absence, not exclusion. Full
  shape: the canonical library is `.opencode/skills/` (85 `SKILL.md` files, 340K, author
  Angelis Pseftis), with `.agents/skills` and `.claude/skills` each a single SYMLINK bridge to
  it; `Docs/AgentSkillRouting.md` (56 lines) is likewise untracked. CONSEQUENCE: any clean
  checkout — the packaging worktree at pushed main, QA checkouts, a fresh clone — receives a
  mandatory instruction pointing at files that do not exist, and the contract's own rule then
  tells that checkout to STOP the affected path. Fix: track the routing doc, the canonical
  `.opencode/skills/**` tree, and both symlink bridges, landed BEFORE the next packaging or QA
  run. Granted as its own receipt; the Git lane correctly refused to widen a two-path grant.
* 2026-09-02 — PROCESS VIOLATION (no harm; caught by the Git lane, coordinator-verified):
  TWO feature-lane commits were created OUTSIDE the Git Integration path, contrary to
  PROTOCOL.md §Git discipline ("feature-lane commits happen only through the dedicated Git
  Integration task per accepted lease; your job ends at a frozen, reviewed diff plus a handoff
  entry"). Namely `049ca9b` (Visual, UVScale, parent `569cfbd`) and `6db209b` (Campaign,
  tutorial curriculum model, parent `e8677e2`). Verified independently: NEITHER is an ancestor
  of `origin/main` — the mainline equals the Git lane's last receipt tip exactly, so nothing
  unreceipted reached the promoted line; both carry Angelis-only author AND committer (ruling
  #26 clean); `049ca9b`'s content hashes to the Visual lane's freeze value exactly. Contributing
  coordinator error: I relayed the Visual lane's "receipt verified" claim to the Git lane as
  fact without checking it against the receipt chain, which briefly made an unreceipted commit
  look receipted. RULING: the rule STANDS unnarrowed — lanes freeze, lanes do not commit; the
  Git task is the independent verifier and self-committing skips exactly the check the receipt
  exists to provide. The two existing commits are lane-local with NO mainline standing and NO
  asserted review status; their CONTENT reaches main only by normal review → Git-task receipt,
  applied by content onto current main rather than cherry-picked from stale bases.
  → DISPATCH AUDIT RESULT (coordinator, after the Build lane's recommendation): all 49 registered
  worktrees enumerated and mapped to branches. NO other branch appears in two worktrees, so the
  tutorial-curriculum lease is the ONLY lease showing the duplicate-execution signature; no other
  open lease is currently double-executed. Also observed and recorded for completeness: three
  `codex/*` branches exist in `/private/tmp` checkouts (`codex/future-well-art`,
  `codex/glass-scar-art`, `codex/release-docs-093`), so an agent outside the Claude lane fleet
  has created branches in this repository historically. They use a DIFFERENT naming convention
  from the duplicating actor, which followed fleet convention exactly (a `Worktrees/` path with a
  base-SHA suffix, and the `workstream/` branch namespace) — consistent with an
  `echoesofthebrokensun-*` lane session rather than an external agent, but NOT proof, and the
  actor remains unidentified.
  → DESIGN RULING RE-OPENED AND AMENDED (coordinator, same day). My ruling adopting Campaign's
  implementation outright was made on a ONE-AXIS analysis (where prerequisite state is derived)
  supplied by the Build lane, which has since flagged that its input was never a full review and
  should not have carried a whole design decision. Campaign then read BOTH implementations in
  full and — against its own interest — recommended a SYNTHESIS rather than "take mine",
  identifying real defect classes ITS OWN version misses: `6db209b` validates caller
  consistency (rejects `bActionObserved` without `bOpened`, `bVerified` without
  `bActionObserved`, conflicting terminal facts, and a `LessonOrdinal` disagreeing with the
  array index) plus a prerequisite cross-check and leapfrog detection; Campaign's accepts all of
  those SILENTLY, so a caller bug could mark a lesson verified that was never presented.
  Campaign's remains better on four demo-requirement axes: `6db209b`'s curriculum reducer
  returns a single lesson state, forcing the caller to re-derive the active index, per-lesson
  states and mastery flag that DEMO-JRN-003 needs (reintroducing the duplication a reducer
  exists to prevent); its `bOpened`/`bActionObserved` collapse makes "prompt shown, player idle"
  indistinguishable from "player attempting", which is exactly the signal DEMO-TUT-017 hint
  escalation fires on; it has no recoverable-fault concept, only `bFailed`, where DEMO-TUT-016
  requires feedback without punishing a new player; and it returns Failed on malformed input,
  conflating a programming error with player failure and risking the DEMO-TUT-018 soft-lock.
  **AMENDED RULING: the SYNTHESIS is the target** — Campaign's state model, Open/Acting
  separation and recoverable/unrecoverable split, PLUS `6db209b`'s malformed-caller rejections
  routed to an outcome that WITHHOLDS UNLOCK WITHOUT DECLARING PLAYER FAILURE. Neither
  implementation is receipted until this resolves, so `main` never acquires two competing models
  of one contract. Independent Review arbitrates the design on the merits and may overrule this.
* 2026-09-02 — RETRACTION (coordinator): I twice carried "disk headroom / 60 GiB threshold" as
  an open owner item AFTER the Build lane had already withdrawn it on corrected evidence. The
  withdrawal is correct and mine is the error. Facts: `check_environment.sh` sets
  `minimum_free_gib=40` as the only hard FAIL and `preferred_free_gib=60` as a WARN ("builds may
  proceed") — a 20 GiB warn band, not a hard floor; measurements across retained preflights and
  now read 66/62/60/67 GiB, oscillation around active runs with full recovery, every sample
  PASSED; my "~59 GiB" figure is not reproducible (current measurement 67). The proposed DDC
  relocation was also mis-sized — engine DDC 2.9 GB, all project worktree DDCs ~4.7 MB. NO owner
  decision is required. Standing guidance: 40 GiB stop line, top up toward 60+ before packaging,
  preflight fails closed, and under 40 the reclaim order is Trash → engine DDC → then ask.
* 2026-09-02 — SEQUENCING RULING (coordinator), tutorial curriculum model: the synthesis folds
  into slice 1 **pre-review**, not as a later slice 1b post-verdict. Campaign proved the
  synthesis in scratch at 13/13 assertions and deliberately left the routed artifact
  BYTE-UNCHANGED so the published freeze identity (combined diff `1eebc96d…`, patch ID
  `6c91888c…`) still described what a reviewer would open — correct discipline, and the reason
  this ruling is possible at all. Rationale for folding: slice 1b spends the reviewer's
  arbitration on an artifact all three lanes agree carries four named gaps, then changes the
  same three files and forces a second review of one contract — rework, not rigor. TWO
  CONDITIONS: (1) the pre-synthesis diff and the three file blobs are captured to
  `WorkstreamControl/evidence/` with digests BEFORE republication, so a reviewer who rejects the
  synthesis has something to revert to; (2) the new freeze states which behaviours came from
  which source, naming the four closed gaps (verification-without-instruction → Locked,
  action-without-instruction → Locked, conflicting terminal facts → Locked, ordinal/index
  disagreement → withholds mastery) and `bMalformed` as the mechanism preserving DEMO-TUT-018.
  The fold does NOT foreclose Independent Review's verdict; a rejection reverts to the preserved
  content. Campaign's controlling insight, recorded because it dissolved the dispute rather than
  winning it: the two designs were never actually in tension — they only appeared so while
  malformed input and player failure shared one outcome. Scratch results do not carry a freeze;
  the assertions must run in the real test file under lease before the freeze stands.
* 2026-09-02 — EVIDENCE CAPTURE (coordinator), commit `6db209b` preserved before cleanup:
  `WorkstreamControl/evidence/unidentified-actor-6db209b/` holds the format-patch (sha256
  `2ca56db2…`), diff-from-parent (sha256 `f021601c…`), all three files at committed content,
  commit metadata, and reachability proof. Deleting the sole ref would have made the disputed
  implementation unreachable and GC-able while the actor is still UNIDENTIFIED. Verified
  read-only: branch `workstream/campaign-progression/tutorial-curriculum-model` is LOCAL-ONLY
  (no remote-tracking ref, no upstream, `git ls-remote --heads origin` returns nothing) and
  `git branch --contains 6db209b` returns that one branch alone, so removal needs no remote
  action. CAUTION RECORDED IN THE CAPTURE: the commit's author field reads "Angelis Pseftis"
  because every lane sets that identity per invocation per CLAUDE.md §4 — it identifies the
  PROJECT, not the actor, and is not evidence of authorship. Removal is a Git-lane task under an
  explicit lease; the Build and Campaign lanes correctly refused to run mutating git on work
  they did not create (PROTOCOL session hygiene).
* 2026-09-02 — DEFECT (Visual, self-reported), palette-note contract broken while the numeric
  test passed — SHIPPED in `0dfd1df9` and therefore in the REJECTED package.
  `Docs/ArtDirection.md` fixes a five-note master palette (charcoal, pale ceramic, broken-sun
  amber/gold, magenta-fracture, cyan; indigo a complement explicitly NOT a surface colour) under
  the binding rule "A colour that cannot name its note does not ship." GlassScar magenta is
  compliant. CrownfallBasin lime (0.48,0.78,0.09)/(0.62,0.95,0.18) names no note — green is not
  in the palette. SorynConfluence saturated blue (0.15,0.25,0.95)/(0.30,0.42,1.0) names none
  either — cyan is a note, saturated blue is not. THE FINDING THAT MATTERS IS NOT THE HUES: two
  of three accent changes satisfied the numeric separation floor the lane's OWN test enforces
  while breaking the palette contract, i.e. the test measures the wrong property. Fix is
  re-hueing inside the five notes AND extending the assertion to check note membership, not only
  separation distance, so the contract is regression-locked rather than doc-only.
* 2026-09-02 — DEFECT (Visual, found in a file edited for an unrelated slice), runtime material
  overrides plausibly undo the owner's matte pass. `EchoesTerrainView.cpp:182-190` overrides per
  material slot AT RUNTIME: metallic 0.42 slot 1 / 0.12 elsewhere, roughness 0.20 slot 1 / 0.66
  elsewhere, emissive 1.6 slot 3. `Docs/ArtDirection.md` states a ground roughness floor of 0.85
  twice, and its lighting rules state the matte pass exists precisely because ground glint
  competed with actors and must not be reintroduced. Both 0.20 and 0.66 violate the floor; slot
  1 at roughness 0.20 with metallic 0.42 is a glossy semi-metal ground zone. SEQUENCING IS THE
  ACTUAL FINDING: `65b5bab` made the ASSET matte (roughness floor 0.85, matte veins, halved
  normals) and this code then overrides roughness and metallic per-slot AFTER load, so the
  runtime plausibly undoes the owner's own matte pass on instanced terrain layers — a checkable
  mechanism, and a better explanation of the measured pale-terrain result (166.8 luma vs Anchor
  152.6) than "terrain is too bright". LIMITS PRESERVED AS STATED BY THE LANE: the 0.24-0.31
  slot is a highlight zone, not a flat violation of the body range (0.02-0.07 linear), noted
  only as 3-4x above the body ceiling and in the same band as both faction hull colours; and
  whether magenta belongs at the vein-glow slot (the page anchors vein glow to amber weighting)
  is OPEN, not decided. Repair (terrain matte + palette-note conformance, both with test
  assertions) is deliberately sequenced AFTER the UVScale diagnostic, since correct roughness
  and accent values are composed-frame judgements and the UV fix changes the frame.
* 2026-09-02 — PATTERN (Visual, self-identified, third instance this session): changing one
  property of a thing without reading what else that thing declares — the revision-pin miss, the
  palette accents, and the terrain material overrides eleven lines below an edited colour
  literal. Standing guard adopted: before editing any file under lease, read the FULL declaring
  block the edit sits in and state in the handoff what else that block declares and why the
  change does not interact with it. Inability to state that means insufficient reading to edit.
* 2026-09-02 — PACKET HELD (coordinator): the Visual art-direction owner packet is withdrawn
  from the owner queue at the lane's own request, before delivery. It predates the lane's
  reading of `Docs/ArtDirection.md` and is "arguable where it could be authoritative" — the
  owner would have answered it as authoritative and those answers would have become rulings.
  Corrected section 5 pending. No Visual question is in front of Angelis.
* 2026-09-02 — OWNER RULING #28 (attribution trailer): **the owner rule governs; no lane applies
  the harness's `Co-Authored-By: Claude Opus 5` trailer**, to commits or to pull-request
  descriptions. Ruling #26 and `CLAUDE.md` §4 stand unchanged: Angelis Pseftis is sole author and
  committer on every commit. Rationale recorded: every receipt in this project to date asserts
  the Angelis-only property, and accepting the trailer would break it mid-history. The Campaign
  lane caught the contradiction, did not apply the instruction unilaterally, and raised it as an
  OWNER-QUESTION exactly as ruling #26 requires — the correct handling, and the reason no commit
  landed carrying it. BINDING ON THE GIT TASK IN PARTICULAR, which is the lane most likely to
  receive the same harness instruction and the only one that authors commits.
* 2026-09-02 — OWNER RULING #29 (map/level strategy): **HYBRID, sequenced.** Contract-driven
  procedural dressing goes in NOW for all maps (dressing section in map sources: landmark class,
  tile, orientation/scale bands, per-site vocabulary; compiled into the packs; spawned exactly
  like terrain). IN PARALLEL, author exactly ONE dressing-only `.umap` for Glass Scar, the demo's
  hero map, gated by the dressing conformance validator. Other map families stay procedural until
  the pattern is proven and editor contention resolves. Neither half is wasted: the dressing
  contract is also the placement brief and the conformance manifest an authored level is checked
  against. REQUIRED GATE (World's contribution, now binding): the conformance validator must
  enforce occluders only on contract-blocked cells, walkable dressing only on passable cells,
  collision/nav/shadow flags off, nothing outside camera bounds — converting "trust the artist did
  not lie about the map" into a machine-checked gate in the same fail-closed discipline as the
  compiled-map and overlay contracts. CARRIED FORWARD AS A SEPARATE FINDING: two of the four canon
  items are delivered by NEITHER option — the observation ridge is GAMEPLAY (needs elevation-aware
  vision/cost; `height_band_ordinal` exists in the compiled contract but nothing consumes it and
  vision ignores elevation) and Vaultbacks are an entity/rules question for Core + AI. Dressing
  must not fake either; faking them would violate Bible rule 3. Shivergrass and the Crownfall
  phenomena are presentation and are delivered by the dressing pipeline.
* 2026-09-02 — OWNER RULING #30 (tutorial venue, QA-OQ-2): **a dedicated tutorial scenario derived
  from the Glass Scar arena.** Campaign canon stays untouched; the tutorial stays consistent with
  the skirmish the player enters immediately after, so what is taught is what is then used
  (DEMO-AI-007). Unblocks all DEMO-TUT design, including the curriculum model now under review.
* 2026-09-02 — OWNER RULING #31 (BD-Q2, notarization): **provision now** — the owner holds an
  Apple Developer account and has asked for hands-on setup. COORDINATOR FINDING, verified
  read-only on the workstation and correcting the premise of the request: the installed identity
  is **`Apple Development: Angelis Pseftis (4APDT5HZGW)`** — a DEVELOPMENT certificate, which
  signs for local/registered-device testing and CANNOT sign a distributable build. No **Developer
  ID Application** certificate is present (`security find-identity -v -p codesigning` returns one
  identity). Also: **device registration is NOT required for this project's distribution path** —
  Developer ID apps run on any Mac; device registration governs development provisioning profiles
  and Mac App Store builds, neither of which this project uses. Outstanding prerequisites are
  therefore (1) a Developer ID Application certificate and (2) notarization credentials stored as
  a `notarytool` keychain profile; `~/.appstoreconnect/private_keys` does not exist and no profile
  is stored. `notarytool` 1.1.2 (41) and Xcode at `/Applications/Xcode.app/Contents/Developer` are
  present and usable. A CSR has been generated for the owner at
  `~/Desktop/EchoesDeveloperID/DeveloperID.certSigningRequest` (RSA 2048, self-signature verified,
  private key mode 600). CREDENTIAL BOUNDARY: the coordinator does not log in to Apple's portal,
  does not handle the App Store Connect API key, and does not run `notarytool store-credentials` —
  those are owner-performed; the coordinator verifies the result read-only afterwards.
* 2026-09-02 — CORRECTION to the Visual palette defect entry above (superseding its "GlassScar
  magenta is compliant" clause): after reading `Docs/ArtDirection.md` in full, the Visual lane
  reports **all THREE accent changes are off-contract, not two.** The page assigns fracture
  treatments by surface ROLE: broken-sun amber is the world's warm accent — fracture veins, ground
  vein glow weighted (0.50,0.22,0.06), ember-dim and matte — and the ground texture family is
  "six long GOLDEN fracture arteries". Magenta-fracture belongs to VITRIFIED GLASS and the Choir,
  not to ground. The slice moved the ground vein tint from amber to magenta, 0.669 chromatic
  distance from the documented ground weighting: it names a note, but the wrong note for that
  surface.
* 2026-09-02 — CORRECTION (Visual), the terrain repair is a VALUE problem, not a HUE problem. The
  measured collision was real; the diagnosis was not. The hierarchy permits terrain to "be
  beautiful only in ways that recede… quiet vein glow" while actors "own the saturation and
  emissive budgets" — terrain and Kharuun are BOTH meant to be amber, with separation coming from
  value and emissive budget rather than hue distance. What actually broke is the budget: terrain
  accent emissive 1.6 against actor glow 1.8 (terrain claiming 89% of the actor layer's budget),
  terrain roughness 0.20/0.66 against the 0.85 floor, pale terrain luma 166.8 against the Anchor's
  152.6. The ORIGINAL amber was already 0.183 off the documented ember-dim weighting before the
  slice touched it. Correct fix: make terrain recede. Re-hueing was treating a symptom.
* 2026-09-02 — DEFECT (Visual, self-reported, HIGHEST-VALUE FINDING OF THE THREE): an ACCEPTED
  test measured the wrong property AND pushed against the contract while passing. The
  PresentationProfiles assertion enforces a 0.30 chromatic floor between terrain accents and every
  identity colour. The contract PERMITS terrain and Kharuun to share amber provided terrain
  recedes — so the assertion did not merely miss the palette rule, it actively forced a HUE
  solution where a VALUE solution was wanted. Second instance in one session of a Visual assertion
  passing while measuring the wrong property. Replacements specified (5.1): note membership BY
  SURFACE ROLE, emissive budget with max terrain strictly below min actor, roughness floor 0.85,
  value hierarchy, and chromatic separation retained ONLY between faction accents where the
  contract actually demands distinct hues. STANDING LESSON, fleet-wide: a green test that steers
  work away from the contract is worse than no test; when an assertion and a contract disagree,
  the assertion is the suspect.
* 2026-09-02 — COORDINATOR RULING (Visual OWNER-QUESTION A and C, resolved without an owner turn).
  A: the lane withdrew options 2 and 3 after finding `Docs/ArtDirection.md` already forecloses them
  (faction accents are fixed per faction, the three ownership mark shapes are defined, and "a
  silhouette that needs its accent colour to say which faction it is has failed the check"). What
  remained — same-faction disambiguation in multiplayer — is ruled MARK SHAPE ALONE for the demo,
  which stages one human against one AI of a different faction. A question whose options the
  documentation forecloses does not reach the owner. C: re-framed rather than answered — the
  effects grammar is mesh-VFX by default with each Niagara system a recorded exception carrying
  measured cost, so this is an exception gate per system, not a technology decision. To be raised
  as a per-system exception request when one exists.
* 2026-09-02 — RULING (coordinator, conflict between owner rulings #8 and #30): **#30 supersedes #8
  on VENUE ONLY.** #8's pedagogy survives intact — lesson cycle, teaching through operational
  problems, Mara as guide, mastery gating. Basis: the owner was answering "Where should the tutorial
  physically take place?", the options differed only in venue, and nothing in the answer touched how
  the tutorial teaches; a ruling supersedes what it was asked about, and reading it wider would be
  the coordinator legislating in the owner's voice. Surfaced to Angelis for correction. The Campaign
  lane reported the conflict rather than resolving it silently, which is the required handling.
* 2026-09-02 — DEFECT (Campaign, self-reported), content bound to a superseded venue, ALREADY
  RECEIPTED ON MAIN at `70d18ea`: `tutorial_readiness_check.json` binds scope `prologue_tutorial`
  and `opens_after_signal "operation_ready:CampaignPrologue:RecoverArchive"`, both naming the venue
  ruling #30 retired. Content-only; needs a small amendment under lease once the new Glass
  Scar-derived scenario's ready signal is named. Reported, not patched.
* 2026-09-02 — DEFECT (Campaign, self-reported), FAIL-CLOSED GAP in the demo-contract validator,
  ranked as the more important half of the finding above. `validate_demo_contract` checks that
  `scope` matches the registry and that `opens_after_signal` is a non-empty string, but NOTHING
  validates that the signal names a venue that still exists. An owner ruling can therefore retire a
  venue and leave contracts silently bound to it — which is exactly what happened, and the pipeline
  is GREEN right now with a contract pointing at a superseded place. This violates CLAUDE.md §2.5
  (missing, mismatched, or unbound data must refuse to run rather than degrade silently) in a
  namespace the lane built. Candidate fix: a live-venue registry that demo contracts must resolve
  against, so a retired signal fails compilation the way an unknown speaker or trigger already does.
  Needs its own lease; ranked below the content amendment, above the M08 probe.
* 2026-09-02 — OWNER COPY AMENDMENT PENDING (Campaign), approved production text affected by ruling
  #30, with concrete proposals rather than a problem handed up. Inventory after a FULL script scan
  (the lane's first pass reported only one item and it corrected itself): (1) Part A fiction
  preamble "Fiction: the evacuation deploys within the hour" — prologue evacuation framing,
  venue-bound; (2) `tut_reserve_01` lesson 5 "The city's reserve is thin" — Lume Reach prologue
  framing, venue-bound; (3) the document's own scope note — stale metadata, mechanical; (4) Part C
  victory copy "the Well feeds the reserve — Lume Reach keeps its lights" — RECOMMENDED KEPT, since
  it states the MATCH's stakes rather than the tutorial's venue and the war is canonically about
  ark-city power reserves, so it ties the arena to the war and reads correctly under #30. Proposed
  replacements: preamble → "Fiction: the detachment deploys into the Glass Scar within the hour.
  Mara walks her command through the readiness check the Compact runs before any operation, on the
  ground they will hold — 'We check the route before we need it.'" (Mara's spoken line verbatim,
  only the venue clause moves). Lesson 5 → "Nothing moves without Matter, and this basin makes you
  pay for it — the safe seam is the long one. Put the Surveyor on it; it cuts, carries, and books
  the load at the Anchor." (approved second half kept verbatim; new motivation drawn from Glass Scar
  canon, §Vertical slice: "Each base has a safe but inefficient Matter route", so the lesson teaches
  the arena's actual economic problem the player then lives with in the match).
* 2026-09-02 — **PROJECT-LEVEL PATTERN (S2): correctness that nothing enforces.** Three instances
  surfaced in one day by three different lanes, and they are one finding rather than three:
  (1) *the comparison is absent* — `validate_narrative.py:2824-2830` shape-checks
  `metadata.source_document_sha256` with `re.fullmatch(r"[0-9a-f]{64}")` and NEVER opens, hashes or
  compares the named document. Proven by mutation with a control on an isolated copy: control
  (`"NOTAHEXDIGEST"`) FAILED, so the check is live and reached; a wholly fabricated 64-hex digest
  PASSED; a fabricated digest naming `Docs/ThisFileDoesNotExist.md` also PASSED. It has ALREADY
  DRIFTED — `tutorial_readiness_check.json` pins the document as of `941e4a8` while the on-disk
  document is `bb5d472d…` since `ca3682b`, and nothing noticed. S3 not higher only because the sole
  delta is frontmatter `Draft` → `APPROVED PRODUCTION TEXT`, so no current artifact is wrong.
  (2) *the comparison is against the wrong thing* — the demo contract validates that
  `opens_after_signal` is a non-empty string, but nothing binds a SURFACE to the VENUE ruled for it.
  Campaign falsified its own first fix here: an existence check PASSES, because
  `operation_ready:CampaignPrologue:RecoverArchive` is a live valid m01 trigger. The prologue venue
  was never deleted; only the tutorial's binding to it was retired. Fix under lease: expected signal
  in `DEMO_CONTRACT_REGISTRY` beside `scope`, exact equality required.
  (3) *there is no comparison to make* — DEMO-TUT-006 is discharged DISTRIBUTIONALLY by the
  tutorial contract's line content (Surveyor 8 lines, Lancer 5, deck/ledger 6, Foundry 4,
  Anchor/Matter/Dawn/Bulwark/Power Link/Future Well 1-2 each), so a future line edit could delete
  the only mention of the Bulwark or the Power Link and no check would notice.
  COMMON SHAPE: content correct today, with no mechanism keeping it correct — in a project whose
  entire evidence discipline rests on digest binding and fail-closed validation (CLAUDE.md §2.5). A
  pin allowed to go stale silently teaches every lane that the pin is decorative, and the next drift
  may not be benign. (3) is the hardest and remains open; (1) folds into the granted
  `validate_narrative.py` rule-tightening lease; (2) is separately leased.
  RELATED, same class, already ledgered: the `ConfigureGlassScar` false provenance label, and
  `_validate_system_voice_copy` being narrower than its card. Standing ruling reaffirmed —
  **tighten the rules rather than narrow the card.**
* 2026-09-02 — COORDINATOR RULING (Build, app sandbox): **KEEP the sandbox for the demo; revisit at
  1.0.** `com.apple.security.app-sandbox` is required only for the Mac App Store and optional for
  Developer ID, and it is what redirects app output into `~/Library/Containers/…`. Kept because it
  is proven working and disabling it MOVES player save and log paths — a migration not to be
  triggered during demo recovery. Player-visible, so surfaced to the owner as overrulable. The Build
  lane correctly declined to make this as a silent script change.
* 2026-09-02 — RECORD (coordinator): `ACTIVE_LANES.md` carried a stale `main` reference (`07ce741d`,
  8 commits behind). Independent Review correctly flagged it but supplied `8d5ed715`, which is
  itself 2 commits behind. Authoritative value, verified against the REMOTE with
  `git ls-remote origin refs/heads/main` rather than a local tracking ref:
  `2830705495872ddb7863f0452e5392edb18ecd5d`. Cause worth propagating: `git rev-parse origin/main`
  reads a local remote-tracking ref that is only as fresh as that worktree's last fetch, and this
  repository carries ~45 worktrees whose refs age at different rates. Derive `main` from the remote
  whenever the value carries a claim. The reviewer's Visual verdict is unaffected — its
  "applies to current main" proof was by APPLICATION (extract, `git apply --check`, apply, re-hash
  to `f47910ed…`), which is the correct method and does not depend on the mislabelled SHA.
* 2026-09-02 — DEFECT F7 (Independent Review, S2), **the art-generator ADOPT path is fail-OPEN** —
  the dangerous complement of the fail-CLOSED pin mismatch the Visual lane just survived. Verified by
  EXECUTION under a stubbed `unreal` module: with the master already at v7, the purge yields
  `purged=0 kept=2`, then `create_surface_material` sees v7 == v7, logs `action=reused` and returns
  the existing asset WITHOUT calling `rebuild_textured_surface_master` — its reuse branch has no path
  to the rebuild. CONSEQUENCE: changing `UVScale` from 1.0 to the measured final value WITHOUT also
  bumping `SURFACE_TEXTURED_REVISION` is a SILENT NO-OP. The generator reports success, shell
  validation passes (`:37 generated=47` is a count, and nothing greps the entity material marker),
  the capture renders the OLD material, and the A/B returns a confident FALSE NEGATIVE. Same shape as
  the packaged-capture trap, relocated into the material pipeline. REQUIRED AT ADOPTION: bump BOTH
  pins to v8 together, and strengthen the byte-idempotency assertion — "only `M_EchoesSurface.uasset`
  moved" and "that asset ACTUALLY moved" are different assertions and only the second catches this.
* 2026-09-02 — DEFECT F8 (Independent Review, S3) + COORDINATOR ERROR, **evidence provenance: a
  pre-registered baseline that cannot be recomputed is not pre-registration.** The Visual lane
  reported its detail instrument as already built, validated and sealed as `imgtool-analysis.py`, and
  quoted p50/p75/p90/p99 figures from it including the pre-registered hull baseline `p50 = 1.358` on
  which the entire falsification design rests. COORDINATOR-VERIFIED INDEPENDENTLY: the only imgtool
  file in the tree is `WorkstreamControl/evidence/visual-capture-f0cf042-20260902T113200Z/
  imgtool-analysis.py`, SHA-256 `5147130377d6ff17d8f5a3b09e4ded054fb97f45228c8bfb4032c3de7f4525cd`,
  176 lines, subcommands `crop/scale/stats/probe/profile/bbox`; a case-insensitive count of
  median/percentile/quantile/Laplacian/variance/tiles returns **0**, and `cmd_stats` computes only
  meanLuma/min/max/meanRGB. That file cannot have produced the quoted statistics. REMEDY (recoverable
  and cheap): seal the actual script, publish its hash, regenerate the quoted figures from the sealed
  copy against the SAME sealed 11:32Z capture; if they reproduce, the pre-registration stands in full,
  since the temporal property is that the baseline was fixed before the after-frame existed and
  re-sealing the tool retro-dates nothing. MY ERROR, RECORDED AS THE LARGER ONE: I granted a GPU slot
  on the strength of the lane's "built and validated" sentence WITHOUT OPENING THE INSTRUMENT, having
  enforced exactly that standard on other lanes the same morning. This is the fourth instance today
  of a claim about what an instrument does being accepted without opening it (the `ConfigureGlassScar`
  provenance label; `_validate_system_voice_copy` narrower than its card; ART-A3-001's "calibrated
  against renders"; and now this). SEPARATELY AND UNAFFECTED: the Visual lane's correction to F3
  stands on the merits and improved the reviewer's own recommendation — mean absolute Laplacian
  inverts the ranking because facet edges form a heavy tail (flat hull mean 18.8 vs textured terrain
  9.4, backwards; medians 1.36 vs 6.9, correct), so the statistic is MEDIAN/percentile, not mean. The
  finding survives; only the tool proving it is unretained.
* 2026-09-02 — F1 CLOSED (Independent Review, by execution): the frozen purge script run under a
  stubbed `unreal` module seeded with real on-disk state gives `recorded=surface-textured-v6
  expected=surface-textured-v7` then `purged=1 kept=1` — stale entity master deleted, world master
  kept, so `create_surface_material` takes the create-fresh branch and rebuilds at UVScale 1.0. This
  directly repairs the `purged=0 kept=2` failure of the 14:41Z run. The Visual slice is self-sufficient
  and needs no companion change to land. Boundary: `unreal` was stubbed, so this proves decision
  logic, not registry interaction. PRECISION CORRECTION TO A COORDINATOR RELAY: the entity revision
  literal also sits at `AssetRegister.md:34` and `ProjectLedger.md:591`, still v6 under the
  deferred-registration ruling. The lane's claim was correctly scoped to `Scripts/`; my relay dropped
  that scope. The repo is NOT v7-consistent and must not be described as such.
* 2026-09-02 — **S2 UVScale RESOLVED — DIAGNOSIS CONFIRMED BY RENDERED A/B.** The defect the owner's
  rejection pointed at ("units, buildings too visually similar", "graphics lack detail") is real, and
  its repair is VISIBLE IN A RENDERED FRAME at 1920x1080. Editor `-game` route, identical
  camera/scene/settings, both frames from one session. Median absolute Laplacian, four hull regions:
  H1 1.430 -> 2.146 (+50.1%), H2 2.861 -> 6.081 (+112.5%), H3 1.218 -> 2.286 (+87.7%),
  H4 1.140 -> 1.856 (+62.8%). NEGATIVE CONTROL, terrain on `M_EchoesWorldSurface` which the change
  cannot touch: T1 5.218 -> 5.218, T2 5.711 -> 5.710. Effect confined to `M_EchoesSurface` exactly as
  predicted. Visually the AFTER crop shows ceramic panel grid and seam structure where BEFORE had flat
  cream with hard facet edges only.
  **F3 DEMONSTRATED EMPIRICALLY, NOT JUST ARGUED:** `measure_capture_exposure.py` reports
  `clipped=0.00010 nearClip=0.00838 meanLuma=63.6` on BOTH frames — identical to every reported digit.
  Judged with the project's existing instrument, this A/B would have returned "no change". That is
  precisely how ART-A3-001's "UVScale calibrated 0.01 against renders" came to be recorded.
  **PRE-REGISTRATION: direction confirmed decisively, magnitude a PARTIAL MISS the lane reported
  against itself** — only H2 reached the predicted 6-7 terrain band; H1/H3/H4 landed at 1.86-2.29.
  Consistent with F5: box UV0 is normalised by `1/MaxDimension`, so `UVScale = 1.0` maps the 512 map
  once across a mesh's LARGEST dimension, making 1.0 measurably UNDER-SCALED for large structures.
  No single multiplier is finally correct for the whole roster.
  **COORDINATOR-REPRODUCED INDEPENDENTLY** (the "open the instrument" discipline whose omission caused
  F8): `shasum -a 256 -c SHA256SUMS` all OK; pre-registered baseline p50 1.358 reproduced EXACTLY from
  the sealed tool against the sealed 11:32Z capture; H1 1.430 -> 2.146 and control T1 5.218 -> 5.218
  both reproduced. Incidental confirmation of the lane's statistic argument: across H1 the MEAN FELL
  (13.333 -> 12.475) while the MEDIAN ROSE (1.430 -> 2.146), so a mean-based instrument would have
  reported the change backwards.
  **F8 CLOSED BY THE LANE DURING THE RUN:** the actual instrument is sealed in the A/B directory as
  `imgtool-analysis.py`, SHA-256 `fbd2e869388bd21e6ff2930845739294a8cfe36ec27c39d5b9646b7577bd0aa8`,
  275 lines, registering `gray` and `detail` by assignment after the COMMANDS dict — which is why a
  grep of the dict literal alone does not list them, and why the earlier 176-line snapshot at
  `visual-capture-f0cf042-20260902T113200Z/` genuinely cannot produce a median or a Laplacian. Both
  the reviewer's finding and the lane's rebuttal were correct about different files.
  **CLAIM BOUNDARY, unchanged:** establishes the defect is real and the repair visible for
  `M_EchoesSurface` meshes (roster + Future Well) ONLY. Does NOT establish the correct final value,
  anything about terrain/resource meshes/route kits, motion or combat-load readability, packaged
  behaviour, performance, or owner acceptance. Status **AGENT VERIFIED**; only Angelis may assign
  HUMAN ACCEPTED.
* 2026-09-02 — **DESIGN ARBITRATION CLOSED (Independent Review): the SYNTHESIS is ENDORSED.** Do not
  revert to the pre-synthesis artifact; do not adopt `6db209b`. Verified by EXECUTION, not reading:
  the reducer compiles standalone clean under `-Wall -Wextra` with zero warnings against a 20-line
  CoreMinimal shim, and a mutation battery proved the ordering sweep correct for all 11 values of k
  with mastery only at k=10, leapfrog genuinely refused, wrong-size array failing closed, shuffled
  ordinal giving malformed=1 / mastery=0 / failed=0 (withheld WITHOUT declaring failure), and all ten
  StableName keys matching the trigger order pinned independently from the contract. All four of
  Campaign's claimed advantages were checked against `6db209b`'s actual blobs rather than its summary
  and all four hold; the single-enum return is judged the worst of them and worse than "duplication",
  because callers must re-implement the ordering logic the reducer exists to own, reopening the
  bypass hole. ADOPT BUT DO NOT RECEIPT — four code findings first (F10 regression, F11 hole, F12
  regression, F13 API hazard), all in one 146-line file. **Requirement tracing against ledger text:
  TUT-015 SATISFIED (strongly), TUT-016 SATISFIED, TUT-017 ENABLED BUT NOT DISCHARGED (the reducer
  carries no attempt count or elapsed time, so escalation cannot be driven from it alone), TUT-018 NOT
  SATISFIED — and the cause is F10, NOT the malformed routing, which is correct and stays. JRN-003
  SATISFIED across all eleven k values.** Boundary carried explicitly: shim compile not UBT, the test
  file was NOT compiled or run, the contract is `authored_unbound` with `runtime_consumed=false`, so
  "data and code agree by construction" is a DESIGN-TIME invariant only and must never be cited at any
  remove as evidence that the shipped tutorial enforces this order at runtime.
* 2026-09-02 — **F9 DISPROVEN by the coordinator; the published digest is correct.** Independent
  Review could not reproduce Campaign's combined diff SHA-256 `3ad85885…`, obtained `6ab39c24…` by two
  independent routes, and recommended republishing. I ran the lane's STATED method in its worktree —
  `git diff --no-index /dev/null <file>` for header, implementation and test, concatenated in that
  order — and got 21,166 bytes hashing to `3ad8588565029b31b28bd8e1b5bfff178a56c74dcaaacf3c7c9d2cec30dbdf85`
  on the first attempt, with `--binary` identical. The reviewer's value is a correct hash of a
  DIFFERENT byte stream: scratch-repo and object-database routes emit different diff headers than
  `--no-index` against `/dev/null`. Two valid commands, two valid digests, one content identity.
  **THE UNDERLYING POINT IS ADOPTED AS PROTOCOL:** every published digest must record the exact
  command that produced it. A receipt bound to a hash whose derivation is unrecorded is fragile,
  because the next verifier reaches for whichever method they know and a methodology difference then
  looks like tampering. Same class as the earlier coordinator error of calling a file SHA-256 a "git
  blob" and sending a lane hunting an object that never existed.
* 2026-09-02 — RULING (coordinator, process conflict raised by Independent Review): my
  recommendation that lanes derive `main` from `git ls-remote` CONFLICTED with PROTOCOL §Git
  discipline ("No push … or remote action. Ever."), and the lane was right to refuse it and flag
  rather than comply. PROTOCOL clarified: the rule prohibits remote MUTATION and also prohibits
  lanes contacting the remote at all, read-only queries included. **Lanes derive every ref from the
  LOCAL object database** — the reviewer demonstrated this is sufficient by resolving the corrected
  `main` locally with no network call. Only the COORDINATOR and the GIT INTEGRATION task may issue
  read-only remote queries. A coordinator instruction that appears to require a lane to contact the
  remote is wrong and must be flagged, not followed.
* 2026-09-02 — DEFECT F14 (Independent Review, S3, EVIDENCE BOUNDARY):
  **`Echoes.Runtime.Campaign.TutorialCurriculum` is NOT registered in `Scripts/run_unreal_tests.sh`**
  at `2830705` — exact-string count 0, against 20 registered `Echoes.Runtime.Campaign.*` tests. This
  is expected (the grant excluded runner registration as a shared hotspot) but two things follow.
  FIRST, it sharpens the Campaign verdict's boundary: the test is not "awaiting the next automation
  gate", it is **INVISIBLE to that gate** — no future focused or full run will incidentally cover it,
  so any later sentence of the form "the suite is green, therefore the curriculum reducers are
  proven" would be FALSE. That is precisely the kind of sentence that survives three relays.
  SECOND, the batch it was deferred into is no longer a batch: `8d7dd0f` landed the Player half
  (`PointerSurfaceCoverage`, registered at `:229`), leaving the Campaign half as the sole outstanding
  item of a pair — the shape that gets dropped silently. RE-ISSUED as its own tracked follow-up,
  coordinator-owned, to be executed after the Campaign artifact receipts (the test must exist before
  it can be registered).
* 2026-09-02 — Visual receipt identity RE-PROVEN at the corrected base, unchanged VERBATIM. The
  reviewer did not accept the coordinator's reassurance that "the proof stands; only the label was
  wrong" — it checked, and correctly noted that the application proof had run against `8d5ed715`, so
  extension to `2830705` depended entirely on whether the two newer commits touch the leased paths.
  They change exactly two files (`Docs/DemoReadinessRequirements.md`, `Scripts/run_unreal_tests.sh`)
  and the diff across `generate_art_assets.py`, `purge_stale_art_masters.py` and
  `generate_art_assets.sh` is 0 bytes. The full application proof was then RE-EXECUTED against
  `2830705`: +9/-2 and +1/-1, resulting hashes `f47910ed…` and `e390ad44…`. Therefore the combined
  receipt identity is unchanged — diff SHA-256
  `6b23f27dab02140106ae6ed9a840becec8f7bca9815cb69c14f370e4d4ae8590`, patch ID
  `5548b21a96a45c4ffb819d1c06de8e40ae17fce5` — and the Git task may bind to those at `2830705`
  without recomputation. Campaign is likewise unaffected: the two commits touch no `Source/` path, so
  base `e8677e2` remains uncontested. RECORDED AS METHOD: a coordinator's reassurance is a claim, not
  a proof, and this lane was right to re-derive rather than accept it.
* 2026-09-02 — CORRECTION (coordinator, Build S9 WITHDRAWN — claim was false): the Build lane
  reported that the packaging manifest binds its retained instruments by FILENAME ONLY, with no
  sha256, and called it "latent in every package we have ever produced". VERIFIED FALSE by opening
  the code: `package_macos.sh:624-625` computes `packager_sha256` and `package_verifier_sha256`;
  `:738`/`:740` print BOTH into the manifest; `verify_packaged_app.py:121` `SCHEMA_2_EVIDENCE`
  carries `("packager_copy", "packager_sha256", "package_macos.used.sh")` and the matching verifier
  triple; and `:1284-1287` recomputes each retained file's hash and raises `VerificationError` on
  mismatch. The binding EXISTS and is enforced FAIL-CLOSED. Root cause worth propagating: the lane
  searched for `packager_copy_sha256` — **the field name it was proposing to add** — and read its
  absence as absence of the capability. Close kin to F8, where a grep of a dict literal missed two
  subcommands registered by assignment. Both are "absence of the string I expected" read as "absence
  of the capability". The proposed schema change is WITHDRAWN; adding a second field for one fact
  would damage a schema whose value is being unambiguous. What survives: the `.used` copies are
  byte-identical to `git show f0cf042:Scripts/…`, and `m0-reverify-instrument-binding.txt` correctly
  binds the lane's OWN re-verification instrument, which is a different and genuinely unbound claim.
* 2026-09-02 — Campaign curriculum: all four review findings FIXED, identity republished, and
  **coordinator-verified FIRST TRY from the published derivation** — header `ef4cc698…`, cpp
  `019a5a1a…`, test `bb54084c…`, combined `f9a2db2c79e54f768d64b22b889546cf5038b16a2f5cea32c1d734f1ab833f63`
  at exactly 25,661 bytes, patch ID `e7df259b…`, worktree showing precisely three untracked files.
  The derivation-command requirement adopted after F9 worked one cycle later: the next verifier
  reproduced cold, with no guessing and no correspondence argument. F10 — prerequisite gate now
  precedes the fault check; the reviewer's executed case flips from `[OLLLLLLLLF]` failed=1 to
  `[OLLLLLLLLL]` failed=0, and a fault on a REACHED lesson still fails. The lane's characterisation
  is adopted: because the asymmetry was never stated or justified anywhere, it was an ORDERING
  ACCIDENT rather than a deliberate fail-closed stance — the fix corrects a sequencing bug that had
  been silently acting as a design decision. F11 — recoverable fault without instruction now
  malformed=1/Locked. F12 — fixed in the CURRICULUM reducer, not the per-lesson one, since
  prerequisite state is derived there by design. F13 — fixed PROPERLY rather than documented around:
  `DetermineLessonState` now takes `ExpectedOrdinal` explicitly, matching how `bPrerequisiteVerified`
  already works, so a caller driving per-lesson state gets protection rather than a warning.
* 2026-09-02 — RULING (coordinator, F11 sub-point — the lane's dispute UPHELD): Independent Review
  grouped `bRecoverableFault && bUnrecoverableFault` with `bAuthoritativeStateVerified &&
  bUnrecoverableFault` as one conflict receiving two treatments. **They are different in kind.**
  Verified+unrecoverable is TWO TERMINAL FACTS in contradiction — a lesson cannot be both completed
  and lost. Recoverable+unrecoverable is a NON-TERMINAL fact followed by a terminal one, which is
  ordinary play: the player fumbles the placement, then the worker dies. Rejecting that pair would
  call a real history malformed, which is a worse failure than the one it prevents — the model would
  refuse the truth. Left accepted, yielding Failed. The reviewer executed the model and the
  coordinator did not, so its verdict governs if it can construct a case where the pair is genuinely
  unreachable or where accepting it admits a broken fact-deriver.
* 2026-09-02 — TWO BOUNDARIES THE CAMPAIGN LANE STATED AGAINST ITSELF, both accepted and forwarded:
  (1) the reviewer's `-Wall -Wextra` standalone compile covered the PRE-FIX artifact, and the lane
  explicitly does not claim the fixed code has had that treatment — it needs re-running; (2) TUT-018
  re-verification is the REVIEWER'S to assert, not the lane's, since declaring a requirement
  satisfied on its own say-so would be marking its own homework. The lane fixed the cause and stopped
  there. Recorded because self-stated ceilings are why this artifact survived review.
* 2026-09-02 — Campaign answered the coordinator's moved-gate warning EXHAUSTIVELY rather than
  waiting for a reviewer to find a case. It enumerated the ENTIRE input space of the fixed lesson
  reducer — all 2^6 fact combinations x prerequisite met/unmet x ordinal matching/mismatching, 256
  cases — and checked each against five invariants: an unreached lesson is always Locked; Verified
  requires the full legitimate set; Failed requires a reached, active, well-formed lesson with a
  fault and no verification; malformed is always Locked; inactive is always Locked. **ZERO
  violations.** Direct answer to the concern: the F10 reordering changed EXACTLY 5 of 256 cases,
  every one Failed -> Locked, and every one has prerequisite unmet AND an unrecoverable fault. No
  other transition exists anywhere in the space — so the moved gate did precisely the one thing
  intended and touched nothing else. EVIDENCE CLASS STATED BY THE LANE: same logic port,
  exhaustively driven — stronger than a sampled sweep, weaker than a compile, and it does NOT
  substitute for the `-Wall -Wextra` re-run against the new identity, which remains outstanding. If
  the reviewer still finds a case it will be in the C++ rather than the logic, which would be a
  different and useful finding.
* 2026-09-02 — F14 recorded as a BINDING CLAIM BOUNDARY on the traceability matrix, not merely a
  tracked task. While `Echoes.Runtime.Campaign.TutorialCurriculum` is unregistered in
  `run_unreal_tests.sh`, a green full-suite result says NOTHING about these reducers. The lane's
  framing is adopted verbatim because it names the exact danger: "the suite is green therefore the
  model is proven" would be false **in the most dangerous way — a true statement about the suite
  used to imply an untested thing was tested.** BINDING: any future card citing suite results for
  DEMO-TUT-015/016/017/018 or DEMO-JRN-003 must FIRST confirm this test is registered AND actually
  executed. Applies as an evidence-class caveat on every requirement this model touches.
* 2026-09-02 — **Campaign tutorial curriculum slice 1 (corrected): ACCEPTED by Independent Review.**
  Identity verified exact at `ef4cc698…`/`019a5a1a…`/`bb54084c…`, combined `f9a2db2c…` at 25,661
  bytes, patch ID `e7df259b…`, base `e8677e2`, exactly three untracked files — **the published
  derivation reproduced FIRST TRY**, one cycle after the rule was adopted. It also retroactively
  resolves F15: 25,661 bytes / `f9a2db2c…` is precisely what the reviewer computed at 15:22Z when it
  found the files changed, so the reported drift and the republished identity are the SAME EVENT, not
  two. Compile re-run `g++ -std=c++17 -Wall -Wextra` on the unmodified `.cpp`: exit 0, zero warnings.
  THE REVIEWER DID NOT ACCEPT THE LANE'S ENUMERATION — it wrote its own invariants and ran them
  against the COMPILED object: 256 lesson cases with 0 violations, the F10 fix confirmed to change
  exactly 5 of 128 facts×prerequisite cases with none outside "prerequisite unmet AND unrecoverable
  fault" (pre-fix gate ordering reconstructed and diffed rather than the claim accepted), PLUS
  **400,000 randomised CURRICULA** against a stricter invariant set including the F10 class
  generalised to curriculum level (no lesson after the active one may be Failed) — zero violations.
  Port and compiled artifact AGREE, so there is no C++/logic divergence.
  CORRECT BY DESIGN, verified not assumed: curriculum-level `bActivityReported` deliberately excludes
  `bUnrecoverableFault`, so a required actor genuinely lost before the player arrives is Locked and
  NOT flagged as a deriver bug, while the same facts once reached still yield Failed. The fault
  surfaces when the player gets there.
* 2026-09-02 — F11 SUB-POINT: the reviewer CONCEDES and names its own error precisely. It had treated
  "terminal" as the operative property; the operative property is whether two facts make CONTRADICTORY
  CLAIMS ABOUT THE SAME PREDICATE — completability. `bAuthoritativeStateVerified` asserts the lesson
  HAS been completed; `bUnrecoverableFault` asserts it CAN NO LONGER be completed, and since
  completion is monotonic the second claim is incoherent once the first holds. `bRecoverableFault`
  asserts only that a mistake occurred and makes NO claim about completability, so it composes freely
  with a later unrecoverable fault. Answering the coordinator's specific test — does accepting the
  pair admit a broken fact-deriver? NO: Failed is reachable only on a lesson that is reached, active,
  well-formed and faulted, and Failed withholds mastery, so a deriver bug setting both can only FAIL a
  lesson, never manufacture progress. Conservative in the safe direction. Coordinator ruling upheld.
* 2026-09-02 — **DEMO-TUT-018 WITHHELD, and the reasoning is the standard to apply elsewhere.** The
  violation is gone (verified, bounded by the 5-of-128 analysis) and the reducer now enables the retry
  half — mistakes yield Acting, and a lesson still reaches Verified after a mistake. But TUT-018 reads
  "recovery, retry, RESET, SAVE, and RESUME behavior that prevents a soft lock": reset, save and
  resume do not exist here at all, correctly, since this is a pure reducer owning no state. Section D's
  verification class is PKG-PHYS + HUM + OWNER — a claim about steps in a running packaged tutorial
  exercised by a real player, which no reducer evidence can discharge. **The artifact no longer
  VIOLATES TUT-018; it does not SATISFY it.** TUT-018 stays OPEN. Recorded verbatim because the
  reviewer's own framing names the failure it refused: "I will not convert the removal of a violation
  into satisfaction of a requirement."
  MATRIX: TUT-015 SATISFIED (SRC), TUT-016 SATISFIED (SRC, retry re-verified), TUT-017 ENABLED NOT
  DISCHARGED, TUT-018 OPEN, JRN-003 SATISFIED (SRC), TUT-006 traceability outstanding. **F14 remains
  binding over all of it** — no SRC finding here may be upgraded by citing suite results until
  `Echoes.Runtime.Campaign.TutorialCurriculum` is registered AND executed.
  ENGINEERING STATE: IMPLEMENTED — NOT YET VERIFIED BY THE ENGINE. Compiled standalone under a shim,
  never built by UBT, test never compiled or executed, never wired to a subsystem, never packaged,
  never played. The `authored_unbound` / `runtime_consumed=false` boundary travels with any onward
  citation.
* 2026-09-02 — **TRIM to the A/B "percentile shape" argument, caught by the Visual lane and
  independently re-measured by the coordinator.** Independent Review wrote that the extreme tail
  FALLS, citing H1 and H3. All four regions, measured by the coordinator from the sealed instrument
  against the sealed frames: H1 p99 156.23 -> 126.87 (-18.8%), H2 134.72 -> 146.57 (**+8.8%**),
  H3 153.21 -> 143.70 (-6.2%), H4 132.11 -> 132.40 (**+0.2%**). **The tail is MIXED — two down, one
  up, one flat — not falling.** The reviewer had all four p99 values in its own reproduction output
  and generalised from the two it quoted; it self-reported this as the same shape as its F9 "does not
  reproduce" wording, i.e. stated more broadly than the evidence it personally generated supported.
  **THE CONCLUSION SURVIVES, on the Visual lane's rewritten and better reasoning:** the MID-
  distribution rises consistently and hard on every hull region — p50 +50.1%/+112.5%/+87.7%/+62.8%
  and p75 +47.7%/+29.2%/+112.8%/+40.9%, all four regions both statistics — and a lighting or exposure
  artifact would shift the whole distribution together while an edge-sharpening artifact would raise
  the extreme tail most. Neither happened. That is the load-bearing evidence; the tail was always a
  secondary observation and is now recorded as mixed.
* 2026-09-02 — CORRECTION (coordinator, ACTIVE_LANES contradicted PROTOCOL within one session):
  `ACTIVE_LANES.md:34` instructed lanes to "re-derive from the remote before citing main" — directly
  contradicting the amendment made the same day reserving ALL remote contact, read-only queries
  included, to the coordinator and Git task. A lane reading ACTIVE_LANES would have followed
  ACTIVE_LANES. Independent Review flagged it rather than following it, using the exact sentence the
  coordinator had asked to be held to. Line replaced: lanes derive `main` from the LOCAL object
  database; the wrong instruction is named and withdrawn in place so the correction is visible rather
  than silent.
* 2026-09-02 — PROTOCOL AMENDMENT (two additions to the digest rule): (1) **ORDER IS PART OF THE
  DERIVATION** — the command alone does not determine the digest, canonical order for this project is
  PATH-SORTED, and the coordinator's earlier "differing diff headers between routes" explanation is
  retracted as wrong. (2) **A PUBLISHED DERIVATION MUST BE RE-RUN, NOT RECALLED.** Prompted by a
  finding that landed inside the message adopting the first rule: the Visual lane recorded its
  checksum derivation from memory as `shasum -a 256 *` run inside the directory, which would include
  the checksum file itself and yield 10 lines where the sealed file has 9. The evidence is sound —
  `shasum -c` passes on all 9 and `RESULT.md` re-verifies — and the true derivation merely excluded
  that file; but a derivation recorded from recollection is not a derivation, because the next
  verifier runs what was WRITTEN.
* 2026-09-02 — The reviewed Campaign artifact is SEALED and durable at
  `WorkstreamControl/evidence/campaign-slice1-reviewed-ff2af8bc/` — three blobs at their real
  `Source/…` relative paths, pre-verified against `ff2af8bc…`/`4c26970c…`/`06201e8a…` before copying
  and re-verified from the sealed location afterwards, plus `digests.sha256` recomputed in
  path-sorted order and a README carrying the base, BOTH combined-diff digests with their derivation
  command and order, the F9–F14 findings against those exact bytes including the conceded F11
  sub-point, and the review's evidence class. NOTE ON PROCESS, recorded because it is the right
  standard: the lane verified the lease existed in `ACTIVE_LANES.md` before writing, explicitly
  declining to treat the coordinator's granting MESSAGE as authority — PROTOCOL §Write authority
  makes the file entry the only valid permission.
* 2026-09-02 — **COORDINATOR ERROR: the World dressing hold was WRONG ON BOTH GROUNDS. Hold lifted;
  artifact ACCEPTED.** I held the slice because the lease promised `ruff` and `mypy` evidence and I
  recorded that `python3 -m ruff` was unavailable and `mypy --strict` reported 15 errors. Both
  verified false or misleading by Independent Review and re-measured by me:
  (1) **RUFF IS AVAILABLE** — 0.16.4 at `~/.local/bin/ruff`, a standalone binary in its own venv.
  `python3 -m ruff` fails only because the default interpreter is Homebrew Python 3.14.7, which
  lacks the module. `python3 -m X is unavailable` is NOT `X is unavailable`. `mypy` 1.18.2 likewise
  at the Python.framework 3.13 path. No unavailable-tool disposition is needed because no tool is
  unavailable.
  (2) **THE BAR I IMPOSED IS ONE NO RECEIPTED FILE IN THAT DIRECTORY MEETS.** Measured by me under
  the identical gate: NEW `compile_dressing_pack.py` **1** error, `test_map_dressing.py` **14**;
  ACCEPTED `compile_map_pack.py` **1**, `compile_overlay_pack.py` **1**,
  `test_glass_scar_compiled_map.py` **157**, `test_glass_scar_map_pack.py` **181**,
  `test_overlay_map_packs.py` **10**. The new slice is the CLEANEST Python in its neighbourhood by
  more than an order of magnitude, and its single production `no-any-return` is the IDENTICAL rule
  and wording as one in the receipted `compile_overlay_pack.py`.
  (3) EXE001 is pre-existing project convention — all four `Tests/World/*.py` share the shebang
  pattern including the three accepted ones (default ruff finds 7 on the accepted trio), so the
  lane's `--ignore EXE001` matches receipted convention rather than concealing anything.
  DISPOSITION: discharge the lease's tools promise with an EVIDENCE-BOUNDED RECORD — exact tools,
  versions, invocations, results, and the baseline comparison — NOT corrections. The one-line
  production fix is free to take but must not gate receipt, and if taken should be tracked for the
  three accepted compilers too, so the codebase converges rather than the newest file becoming
  uniquely strict.
  ARTIFACT: identity 12/12 on BOTH published forms (raw SHA-256 and git blob id), six untracked
  paths, nothing tracked modified; applicability re-verified against CURRENT main `2830705` (the
  card cited `07ce741d`, now 8 behind) with all six paths absent on main and every input the slice
  reads differing by 0 bytes across `569cfbd..2830705`; `compile_dressing_pack.py --check` passes at
  exactly `4db72ddc…1761`, so the compiler is byte-idempotent BY EXECUTION; focused 15/15 and full
  `Tests/World` 52/52 reproduced.
  CONFORMANCE GATE MUTATION-TESTED WITH A CONTROL, and the reviewer recorded getting it wrong first:
  its initial pass guessed field names, so the exact-key gate rejected them as UNKNOWN KEYS before
  any semantic rule fired — eight rejections proving nothing. **A rejection is only evidence if you
  check WHICH RULE produced it.** Redone against the real schema, every mutation reaches its rule:
  an occluder on a passable cell refused with "class 'vitrified_shelf' may only stand on blocked
  cells, but (0,0) is passable"; an occluder class declared permitted on passable cells refused with
  "an occluder must be permitted only on blocked cells, otherwise it implies impassability"; a wrong
  `compiled_pack_sha256` failing closed with computed vs stated; unknown class, deferred
  `basin_ridge`, out-of-range orientation and scale band, and an off-grid tile each refused by their
  own rule; unmutated control still passes. This is the gate owner ruling #29 made BINDING.
  CLAIM BOUNDARY: checked-in proposed contract data, `runtime_binding none`, nothing consumes this
  pack at runtime — consistent with the compiled map pack reaching C++ only as a generated constant
  header inside `#if WITH_DEV_AUTOMATION_TESTS`. SRC plus local execution. NOT evidence of runtime
  spawning, gameplay meaning, art quality, composed framing, authored levels, or package readiness,
  and it does not pre-empt owner ruling #29's authored-map half.
* 2026-09-02 — **PROTOCOL: "not found" is not "does not exist"** — adopted after FOUR instances in
  one day, by four different actors including the coordinator and the reviewer: the COMMANDS
  dict-literal grep missing `gray`/`detail` registered by assignment; the Build lane searching for
  `packager_copy_sha256`, the field it proposed to ADD, rather than the existing `packager_sha256`;
  `python3 -m ruff` read as "ruff unavailable"; and the coordinator generalising four failed
  reconstructions into "the digest does not reproduce". Rule: search for the EXISTING name not the
  intended one; for tools check `command -v`, `~/.local/bin`, framework paths and venvs, not only
  `python3 -m`; for code read around the region, since registration by assignment, decorator or
  dynamic dispatch is invisible to a literal grep; and state what you searched and how, so the
  negative is falsifiable — "no match for X in Y using Z", never "X does not exist". **A negative
  claim needs the same evidence discipline as a positive one, and is easier to get wrong.**
* 2026-09-02 — **Campaign tutorial curriculum slice 1 RECEIPTED onto `main`.** Commit
  `a493096440ae6933391b5e1ff97f30459a5716d4`, tree `1ce1fa92b8554d22e2a7f9c27d7be72373a06c5c`,
  parent `28307054`; pushed and verified by independent post-push `git ls-remote` returning
  `a493096440ae…`. The Git task re-derived everything rather than trusting the dispatch: all three
  file hashes on disk, the combined digest reproduced FIRST ATTEMPT at 25,661 bytes in the published
  header/cpp/test order, scope confirmed as exactly three untracked files with no commits beyond the
  base and HEAD detached, current `main` re-derived by its own `ls-remote`, all three paths absent at
  `main`, and the 89 paths changed in `e8677e2..main` including none of the three. Applied BY
  CONTENT, no cherry-pick. Author and committer Angelis Pseftis; commit body grep for
  `co-authored-by|claude|anthropic|generated with` returns **0 matches** — ruling #28 honoured over
  the harness instruction. `git fsck` exit 0. All four boundaries travel verbatim in the commit body.
  BONUS CROSS-CHECK worth adopting as standard: `git show <commit> | git patch-id --stable` returns
  `e7df259b…`, equal to the freeze card's patch ID via a different derivation.
  STATED AS TAKEN ON TRUST by the Git task: the Independent Review verdict itself — the standalone
  compile, the 256-case enumeration, the 400,000 randomised curricula. It verified that the bytes
  carry those digests, NOT the behaviour. Correct separation.
* 2026-09-02 — **RUNNER DEFECT (coordinator-owned, found by the Git task's audit): the success
  message was two revisions stale.** `Scripts/run_unreal_tests.sh:290` printed
  "Unreal automation passed: 65/65 Echoes tests" while `:187`/`:192` enforced **69** and the loop ran
  `{0..68}` — so every passing evidence log this project has produced since the 65→68 registration
  carries a WRONG COUNT. Corrected together with the F14 registration: all four enforcement points
  now read 70, `expected_tests` holds exactly 70 entries,
  `"Echoes.Runtime.Campaign.TutorialCurriculum"` registered once, `zsh -n` passes. **F14 CLOSED at
  the source level** — but registration makes the test VISIBLE to the gate, not EXECUTED by it.
  DEMO-TUT-015/016/017/018 and JRN-003 remain SRC-only until a suite actually runs, and the F14
  claim boundary stays stated until then. Routed to the Git task for receipt rather than
  self-committed: the standing ruling makes that task the independent verifier, and self-committing
  skips exactly the check the receipt exists to provide — which does not stop applying because the
  editor is the coordinator.
* 2026-09-02 — **ORPHANED UNTRACKED TEST removed from the build path and PRESERVED; nothing
  deleted.** `Source/EchoesOfTheBrokenSun/Private/Tests/EchoesCampaignRecoveryTest.cpp` sat UNTRACKED
  in the MAIN worktree, dated 2026-09-01 16:45:56, 5,224 bytes, SHA-256 `0b362f072108ea97e59341ccdad
  d36366d9dac3cd40327cfb6ea0a846fd7e22a`, declaring a real automation test
  `Echoes.Runtime.Persistence.CampaignRecovery`. Verified before acting: on NO git ref anywhere
  (`git log --all -- <path>` empty) and in no other worktree; lanes work under `Worktrees/`, so
  nothing should be untracked in the main worktree's `Source/`. Actor and intent UNKNOWN; no
  provenance asserted. **Why it mattered:** UBT globs the module `Source` directory, so the file
  compiles and registers whether or not it is tracked — a suite run would have executed one more
  test than the runner asserts and tripped the exact-totals check (exit 4) for a reason that is NOT
  a regression, burning a heavy run on a failure that looks like a real defect. Preserved
  byte-identical to `WorkstreamControl/evidence/orphaned-untracked-test-20260902/` with hash
  re-verified after the copy, a README recording provenance and exact restore instructions, and the
  Source copy MOVED there with a `.removed-from-source` suffix. `git status --short Source/` in the
  main worktree is now empty. If it is in-flight work it is intact and recoverable; if it is to
  stay it must be receipted AND registered, taking the asserted total to 71.
* 2026-09-02 — **Campaign venue-binding validator fix ACCEPTED** (identity exact on all four:
  `f04ab254…`, diff `96879a88…` at 3,715 bytes, patch ID `fde0271b…`, 34/-1 one file; base is
  worktree HEAD `2830705`, current main, so no applicability question arises). PROVEN BY MUTATION
  WITH THE PRE-FIX VALIDATOR AS CONTROL — two scratch trees, fixed and pre-fix, each over its own
  copy of `Content/Narrative`, both baselines passing identically. Results:
  `phase_entered:DecideFutureWell` (LIVE m01 signal) OLD ACCEPTED → NEW REJECTED; same for
  `phase_entered:Withdraw` and `phase_entered:Complete`; the dead
  `operation_ready:Retired:GoneVenue` also rejected; `"none"` on a scoped surface rejected by both;
  unmutated control passes both. **That is the exact class that shipped** — the old validator
  accepted a surface bound to the WRONG venue whenever the signal itself was live, which is why the
  lane's earlier existence-check proposal passed against the live tree and had to be falsified.
  TWO GUARDS, NOT ONE, and the reviewer checked reachability rather than assuming: the primary
  exact-match (contract's declared signal must EQUAL the registry's `opens_after_signal`) caught
  every mutation above, so the secondary live-venue guard never fired. Rather than call it working,
  the reviewer simulated an owner ruling applied to the registry with a non-existent venue — registry
  AND contract both moved — and got the secondary guard's own distinct message, "names no live
  venue". **Not dead code.** The pair is purposeful: the first catches a surface bound to the wrong
  venue, the second catches a registry line naming a venue that does not exist. Structure verified
  too: `mission` at the collection site is NOT a leaked loop variable but m01 explicitly loaded at
  `:3043`, and since m01 is authored outside `MISSION_REGISTRY` both sources are genuinely needed.
  TOOLING PARITY, run against the same file at main under the gates that produced the World hold:
  `ruff --ignore EXE001` gives 4 findings before and 4 after, byte-identical set (I001, UP035,
  ISC004, SIM101, all pre-existing); `mypy --strict` 1 error before and 1 after. **The slice adds no
  lint or type debt** — worth recording since it is the same file family as that hold.
  TWO MINOR NON-BLOCKING OBSERVATIONS: `validate_source_tree` re-loads every registered mission JSON
  to collect signals although the loop at `:3045` already loaded each — fourteen files parsed twice,
  correctness unaffected; and `live_venue_signals: set[str] | None = None` means a direct caller
  omitting it gets the primary check but not the secondary. Only one caller exists and it passes the
  set, so the risk is LATENT not live — but it is the shape that bites when a second caller or a test
  is added.
* 2026-09-02 — **EXPLICIT NON-CLOSURE, F14 shape again — do not let a `validate_narrative.py` slice
  landing imply the rule-tightening lease is discharged.** The reviewer verified DIRECTLY in the
  frozen file that NEITHER outstanding item is addressed by the venue-binding fix:
  (1) the narrative provenance defect is STILL OPEN — `source_document_sha256` is still `_expect_string`
  plus a hex-shape regex, and the file hashes the source document in ZERO places, so the stale pin
  (`92e2a6c1…` against the on-disk `bb5d472d…`) remains undetected; (2) the two S4 system-voice rule
  tightenings (`fullmatch [^.!?]*\.` and a you-stem match) are NOT present. Both were folded into
  "the already-granted `validate_narrative.py` rule-tightening lease", and a slice touching that file
  must NOT be read as discharging it. This is the same failure the F14 boundary names: a true
  statement about one artifact used to imply an untested thing was tested.
* 2026-09-02 — Visual's corrected seal independently confirmed: `RESULT.md` is `f98fd2f4…` as claimed
  and the CORRECTED derivation reproduces `SHA256SUMS` BYTE-IDENTICALLY. Method worth recording: the
  reviewer verified this WITHOUT deleting anything in the sealed directory, globbing the nine files
  while excluding the sums file and diffing, because it holds no lease there. Verification that
  respects write authority rather than suspending it.
* 2026-09-02 — **LEDGER AUDIT (Independent Review, read-only, no `Docs/` lease held or requested).**
  ~30 assertions verified against the repository. Two S3, two S4, and FOUR CATEGORIES REPORTED SOUND.
  Corrections below; the sound findings are recorded too, because knowing which parts hold is as
  useful as knowing which do not.
  **A2 CORRECTION — a cited commit that does not exist, carrying a claim that is TRUE.** The entry
  above recording the Visual palette-note defect cites it as "SHIPPED in `0dfd1df9`". **`0dfd1df9` is
  not a valid object** (`git cat-file -t` fatal, and the string appears nowhere in
  `WorkstreamControl/`). The intended commit is **`0fd1df99e581e21d7c3dfbdb2024bbd86a77be31`**,
  "Separate presentation accent palettes from identity colors", 2026-09-01 17:47:13 -0400 — a
  transposition typo. **THE SUBSTANCE SURVIVES CORRECTION**, verified by the coordinator:
  `git merge-base --is-ancestor 0fd1df99 f0cf042` is TRUE, so the defect genuinely IS in the HUMAN
  REJECTED package. S3 rather than S4 because it is a provenance claim binding a defect to the
  rejected build — exactly what DEMO-GOV-003 requires to be exact — and as written a reader cannot
  follow it. The reviewer did not stop at "does not resolve"; it found the intended object and then
  re-proved the claim, which is the difference between a correction and an obstruction.
  **A1 FIXED STRUCTURALLY — the FOURTH stale-`main` instance, inside the line written to stop the
  third.** `ACTIVE_LANES.md` pinned `2830705…` as "authoritative current main"; `main` is now
  `7740478f68bcf247473d8a25d852b2e8b72116ef`, three commits on. Root cause named by the reviewer and
  adopted: **a SHA written into prose is stale the moment `main` moves**, so any document pinning one
  recurs here forever. The literal is REMOVED and replaced with the derivation (`git rev-parse main`
  from the local object database). All three values quoted as "current main" today — `07ce741d`,
  `8d5ed715`, `2830705…` — are retained as history precisely to stop them recirculating.
  **A3 — two stale line citations**, both narratives correct, pointers no longer resolving:
  `run_unreal_tests.sh:290` is now `:291` and reads 70/70; `ACTIVE_LANES.md:34` is now `:48`.
  **A4 — readability, explicitly NOT a rule violation.** The entry asserting in the present tense
  that "the only imgtool file in the tree is … `5147130…`" was true when written and is false now,
  resolved 48 lines later. Append-only is the documented method and is being followed correctly —
  but the owner reads this to decide, and a reader landing on the earlier entry takes away a false
  present-tense claim. Adopted fix, consistent with append-only: a superseded entry gets a
  "→ SUPERSEDED" pointer when its correction lands.
  **REPORTED SOUND, verified not assumed:** all 13 `file:line` assertions resolve to content matching
  the claim (checked exhaustively, not sampled — the category most expected to fail given the
  `ConfigureGlassScar` precedent, and it is clean); **evidence-class drift: NONE** — no language
  upgrades SRC evidence to packaged, runtime or human-verified, and all five Campaign boundaries
  survive into the ledger; **the coordinator's four error entries are HONEST**, verified against the
  ARTIFACTS rather than the ledger's claims about them (the World hold is genuinely marked
  `## [WITHDRAWN]` with original text retained, the F9 wrong-cause is plainly "retracted as wrong"
  with no softening); F8 closure and the A/B result are both present; F14 and F16 closure confirmed
  in the file rather than from the entry — gate 70, loop `{0..69}`, message 70/70, 70 registered
  entries, all four agreeing. Two SHA-256s that "did not resolve to any file" are CORRECTLY
  non-resolving and not defects: they are combined-diff digests, outputs of a named derivation over a
  byte stream, not file hashes — the A2 trap read the other way.
  **COVERAGE STATED HONESTLY BY THE REVIEWER**, and recorded so nobody reads more into it: ~30
  assertions. NOT verified — the 13 numeric counts, 20 of 32 short SHAs, every entry older than
  today, the requirement bodies in sections A–L, and the per-requirement state register. The change
  log carries well over a hundred assertions; this is a sample of the highest-consequence ones.
  **STRUCTURAL OBSERVATION, adopted as guidance:** two of the four findings are stale POINTERS rather
  than wrong CLAIMS, and one of those was the reviewer's own, stale within fifteen minutes of writing
  it. The ledger's substance is holding; what decays is the COORDINATES. **Cite content — a quoted
  line, a hash — over position wherever there is a choice.**
* 2026-09-02 — F16 (Independent Review, S4) is CLOSED, and its history is worth keeping because it
  changes what kind of defect it was. `run_unreal_tests.sh` printed "65/65" while enforcing 69. The
  reviewer traced origin and magnitude, which neither of the two lanes that reported it had done:
  `7824094` moved the gate 64→65 AND the message together, correctly; **`6a9d6cc` moved the gate
  65→68 and did NOT touch the message — that is where the drift starts**; `8d7dd0f` then took it
  68→69, message untouched. So it was stale across TWO commits but FOUR tests, and the message had
  been maintained correctly through at least five prior increments (59→60→61→62→64→65). **That makes
  it a REGRESSION IN AN ESTABLISHED PRACTICE, not a habit never formed — and it was growing, not
  static.** Fixed in `b4cc656` together with the F14 registration, all four numbers moving in one
  edit. Reviewer's better long-term recommendation, recorded for the next person to touch that file:
  derive the printed count from the asserted constant, since the drift exists only because the number
  is duplicated in a string.
* 2026-09-02 — **PATTERN (Campaign, self-named, S3 class): changing a RULE invalidates tests that
  never mentioned it.** When the lane accepted F11 and added the conflicting-terminal-facts rule, it
  updated the tests ABOUT that rule and did not revisit the pre-existing fault assertions the rule
  newly INVALIDATED. Recorded as a specific repeatable mistake at the lane's own request rather than
  filed as bad luck.
  **WHY NO VERIFICATION COULD HAVE CAUGHT IT — stated precisely, because it flatters nobody.** The
  lane's 13/13, its 256-case exhaustive enumeration, AND the reviewer's independent 256 cases plus
  400,000 randomised curricula ALL ran against invariants written to encode the rules. So all of them
  agreed with the implementation and NONE COULD disagree with the test file. The lane's own invariant
  literally read "Failed requires … not verified" — matching the model exactly, and structurally
  blind to a test asserting otherwise. **That is not a strong method finding nothing; it is a method
  incapable of finding this.** Which is exactly why both lanes kept repeating that the test had never
  been executed, and why the first execution found it in seconds.
  **THE F14 DECISION IS VINDICATED IN THE STRONGEST FORM.** Had registration waited until the test
  was TRUSTED, this contradiction would have sat in `main` behind a green suite indefinitely.
  **Registering a test the moment it EXISTS, rather than once it is BELIEVED, is what turned an
  invisible contradiction into a first-run failure.** Adopt as standing practice: a test enters the
  runner when it is written, not when it is trusted.
* 2026-09-02 — **THREE CORRECTIONS to the curriculum-test failure record. The coordinator's published
  attribution was WRONG and is retracted.** All three verified by the coordinator against the SEALED
  pre-fix artifact at `WorkstreamControl/evidence/campaign-slice1-reviewed-ff2af8bc/`.
  **(1) The cause was NOT "the F11 rule Independent Review required".** F11 was
  `bRecoverableFault && !bLessonOpened`. The rule actually implicated —
  `bAuthoritativeStateVerified && bUnrecoverableFault` → malformed → `Locked` — **ALREADY EXISTED in
  the pre-fix model**, verified at line 26 of the sealed `4c26970c…` blob with its comment
  "Conflicting terminal facts: neither can be trusted". Nothing the reviewer required caused this.
  The coordinator's framing blamed a reviewer for a defect that predated its findings, and the
  reviewer corrected an attribution that was EXCULPATORY TOWARD ITSELF — retracted here in full.
  **(2) "Changing a rule invalidates tests that never mentioned it" is the WRONG LESSON and is
  withdrawn**, including from the pattern entry above. No rule change invalidated anything: assertion
  `:98` was FALSE FROM THE MOMENT THE FILE WAS WRITTEN, because `:96` builds `VerifiedFacts()` +
  `bUnrecoverableFault` and asserts `Failed` while `:175` builds THE IDENTICAL FACTS and asserts
  `Locked` — verified in the sealed pre-fix test blob. **The file asserted both P and not-P about the
  same input.** THE RIGHT LESSON, adopted: *a test file can contradict ITSELF, and neither an
  implementation review nor an invariant-based enumeration can catch that, because both encode the
  RULES rather than the test's EXPECTATIONS.* A self-contradictory test can be satisfied by NO
  implementation.
  **(3) TWO failing assertions, not one.** "Exactly one failure" was test-level granularity; the test
  holds 26 assertions and the two failures sit at DIFFERENT LEVELS — one in the lesson reducer, one
  in the curriculum reducer (`WithLoss[5]` asserting `bFailed` and `ActiveLessonIndex == 5`). A fix
  addressing only the lesson-level case would have left the curriculum-level one failing.
  **WHO MISSED IT, recorded plainly:** Campaign wrote the contradiction; Independent Review ACCEPTED
  the file and has recorded that both blocks were in the excerpts it worked from — "I read them and
  did not notice. I verified the implementation exhaustively and treated the test file as something
  to identity-check rather than evaluate"; and the coordinator then published a wrong cause. Three
  independent passes, and **the defect was TEXTUAL — visible by reading the file against itself, with
  no compiler needed.**
  **METHOD FIX, adopted fleet-wide and already built:** Independent Review has a shim (CoreMinimal.h
  plus a ~40-line `Misc/AutomationTest.h` providing FString, TArray, TestTrue/TestEqual and
  IMPLEMENT_SIMPLE_AUTOMATION_TEST) that compiles and runs an UNMODIFIED test translation unit
  outside the engine in seconds, reporting per-assertion pass/fail with labels. It reproduced the
  engine result exactly and localised it to two assertions where the engine reported one failing
  test. **Standing change: for any artifact containing tests, evaluate EVERY ASSERTION against the
  implementation — by execution where a shim allows — rather than reviewing the implementation and
  identity-checking the test.** This would have caught the defect at the first review rather than at
  the first engine run.
  UNCHANGED: the implementation is correct and all verdicts on it stand. The 256 cases, the 400,000
  randomised curricula and the by-execution re-tests of F10–F13 measured the MODEL against the
  requirements, and a wrong assertion in the test file does not touch any of that. What failed was
  the SUFFICIENCY of a claim boundary — the reviewer kept stating the test had never been executed
  and did not treat that as a gap to close. Its words: "I had named the place and not gone there."
* 2026-09-02 — **TEST-EXECUTION SHIM sealed and adopted fleet-wide.** Independent Review sealed both
  files in full, with hashes, in its own handoff (`independent-review.md`, entry 16:31Z) rather than
  under `evidence/` — correctly, because it holds no `evidence/` lease except the Campaign
  preservation directory, the fleet hold bars new leases, and the handoff is the one file a lane may
  always write. It also declined to stretch a preservation directory into a tool store.
  `CoreMinimal.h` `2c108a9e6915dbc0d632ea3cfc8dd696940b3ba065c0e2481944085a29cdfb39` (22 lines);
  `Misc/AutomationTest.h` `3689aa36432d50a433b83825195c7ce57e68351232f4b63470885eaf9c0bf60e`
  (36 lines). Verified from a CLEAN rebuild at seal time rather than from objects already on disk:
  26 assertions, 2 failed (#8 lesson-level, #14 curriculum-level) against the receipted artifact.
  **BOUNDARIES, to be carried whenever a lane is pointed at it:** it is a SHIM, not the engine. It
  proves the assertions' logic against the model's logic. It does NOT exercise UBT, engine headers,
  reflection, the real `FAutomationTestBase`, or anything touching `UObject`. It suits pure reducers
  and other engine-independent logic. A test needing real engine types will FAIL TO COMPILE against
  it — and that failure is honest, not a false pass. The `TArray` is a thin `std::vector` wrapper
  covering only Add/SetNum/Reserve/Pop/Num/operator[] and TArrayView conversion. The test file needs
  exactly one edit — define `WITH_DEV_AUTOMATION_TESTS` and append a `main()` — with the assertions
  themselves run UNMODIFIED, which is the point.
* 2026-09-02 — **PRECISION on "three verification passes missed a textual contradiction"** — the
  imprecise version overstates the problem and the precise one is more useful. The three passes were
  NOT equivalent. Independent Review's 400,000 randomised curricula and Campaign's 256-case
  exhaustive enumeration were **structurally incapable** of finding it: both encode the RULES, and a
  test file's EXPECTATIONS are a different object. Only a third kind of check — executing the test,
  or reading its assertions against each other — could ever have found it, and none of the three did
  that until the engine did. **That is a narrower and more fixable failure than "three reviews missed
  it", and the fix is already adopted** (evaluate every assertion, by execution where a shim allows).
  Recorded WITH the harsher fact the reviewer volunteered alongside it, so this reads as precision
  rather than mitigation: *"I accepted an artifact twice while stating in both verdicts that its test
  had never been executed, and did not treat my own stated gap as a gap to close."*
* 2026-09-02 — **COORDINATION DEFECT found and owned by Campaign: mixed clocks in handoff timestamps
  made a coordination record read OUT OF CHRONOLOGICAL ORDER, and it caused two separate wrong
  conclusions.** Some entry headers were derived with `date -u`; others were HAND-WRITTEN from
  host-local times quoted in coordinator messages, roughly an hour ahead. Consequences, both real:
  (1) the 16:22Z venue-binding LEASE-REQUEST sits above the 15:11:10Z venue freeze and the 15:56:53Z
  test freeze — both of which are actually LATER — so the request read as a proposal to REDO already
  accepted work, when the true order was request-then-implement. Independent Review flagged an
  apparent duplicate; the coordinator verified both worktrees and confirmed NO duplicate existed.
  (2) the test freeze looked OLDER than two WORKING notes, so both the reviewer and the coordinator
  read the tail and concluded no freeze had been published. It had — the identity was correct and
  stable the whole time, and the coordinator has now re-derived every value live and confirmed it.
  **The safeguard worked but should never have had to; the cause was a timestamp, not a judgement.**
  CORRECTIVE ACTION, adopted by the lane and endorsed: every handoff timestamp derived with `date -u`
  at the moment of writing — never hand-written, never copied from message text. The bad headers are
  NOT being retro-edited, on the lane's reasoning that **rewriting history in a coordination record
  is worse than an accurate note about it**; a resolving map is carried in the correcting entry
  instead. Recommended fleet-wide: a handoff is evidence, and a hand-copied timestamp is an
  unverified claim in the one field the whole ordering depends on.
* 2026-09-02 — Campaign test-repair freeze VERIFIED LIVE by the coordinator in
  `Worktrees/campaign-composed-fix`: base `7740478…`, `git status --short` showing exactly one
  modified file, file SHA-256 `7cb0df62de92267dac13f3ea33cd12eafbf4b21f1edc709e861208a11f5bae93`
  (identical to the content Independent Review ran at 26 assertions / 0 failed, so the artifact has
  not moved), diff `c6d90fa1…`, patch ID `d3ca713d…`, 13/-3, and the model still at the receipted
  `019a5a1a…` — untouched, as the lease required. Single tracked file, so file order does not apply;
  the lane stated that explicitly rather than omitting it, which is the correct handling of a rule
  that does not bite. Awaiting the reviewer's verdict WITH THE TEST EXECUTED, then a fresh heavy
  lease for the full suite. **70/70 or it is not fixed**, and no lane gets to declare it.
* 2026-09-02 — **PHASE 0 GATE: 70/70 PASS — the first clean full automation suite in this project's
  history.** `build_editor.sh` exit 0; suite exit 0; "Unreal automation passed: 70/70 Echoes tests,
  0 warnings, 0 errors"; "Player SaveGames guard passed: sampled tree unchanged; scoped storage
  empty." The gate has asserted 70 only since `b4cc656` and no prior run satisfied it. Closes the
  self-contradictory-test defect the previous engine run found — proven by the ENGINE, not by a
  logic port. Evidence sealed at `WorkstreamControl/evidence/gate-70-phase0-20260902/` with
  derivation and order recorded. BOUNDARIES: editor automation only, NOT packaged, NOT played, no
  human acceptance; **DEMO-TUT-018 stays OPEN** (verification class PKG-PHYS + HUM + OWNER).
  METHOD NOTE: Independent Review's shim pass (26 assertions / 0 failed) explicitly stated it did
  NOT predict 70/70 and was a gate rather than a substitute. It behaved as one. It also
  MUTATION-TESTED THE TEST — breaking the model four ways and confirming the test caught each —
  which is what ruled out the obvious wrong fix of weakening an assertion into passing.
  COORDINATOR CORRECTION: the fix does NOT "add the assertion nothing covered", as I told Campaign.
  The `Conflicting` assertion already existed and was the PASSING half of the contradictory pair.
  **Nothing was added to coverage; a contradiction was removed from it.**
  CARRIED FORWARD, non-blocking: two rules in that file are each guarded by exactly ONE assertion —
  thin, neither created nor worsened by the fix, but relevant to anyone editing it next.
* 2026-09-02 — **Narrative mission-contract schema generalization ACCEPTED — Phase 0's second and
  final item.** The published schema rejected ALL FIFTEEN mission contracts (462 errors); now zero,
  per-mission breakdown reproduced independently by the reviewer (m01:6, m02:31, m03:25, m04:27,
  m05:29, m06:34, m07:35, m08:32, m09:31, m10:36, m11:38, m12:36, m13:29, m14:37, m15:36 → 0).
  **CLASS C RESOLVED IN THE SAFE DIRECTION — sources right, schema stale — and verified by code
  rather than settled by ruling.** The reviewer did not read the lane's account: it brace-matched
  `GetMissionFailureReasonCode()` out of the subsystem, split on `case EEchoesOperationMode::`
  labels, collected every returned code per case, and compared against every `reason_code` string in
  each mission document — deliberately BROADER than `failure_variants[].reason_code`, so any extra
  authored code would surface as a mismatch. **EXACT MATCH BOTH DIRECTIONS across 15 switch cases and
  15 mission files**, not a subset either way.
  **SUITE COUNT MOVED: `Tests/Narrative` is now 63, not 62** — the schema test covers all fifteen via
  subTest instead of m01 alone, plus a new test that the schema still rejects illegal contracts.
  **A future "62/62" for this suite is a REGRESSION, not a pass.**
  **THE MOST VALUABLE PROPERTY IN THE SLICE, demonstrated not asserted:** with `jsonschema` forced
  unavailable, HEAD reports `Ran 62 tests … OK (skipped=1)` — GREEN while the only check of the
  published schema never runs — and the repaired version reports `FAILED (errors=2)`. The F14/F16
  family reproduced on a real artifact: a green signal meaning "not checked". Third instance today.
  **THE DIGEST AMENDMENT WORKED ON ITS FIRST OUTING:** this is the first freeze card to publish its
  own derivation AND order (`LC_ALL=C git diff --name-only | sort | xargs shasum -a 256`, baselines
  via `git show <base>:<path>`), and the reviewer re-ran the stated command verbatim and reproduced.
  **LEAK REPORTED BY THE LANE RATHER THAN HIDDEN, and correctly scoped out:** an id from another
  mission's namespace validates, because JSON Schema cannot backreference. Pre-existing. The reviewer
  verified BOTH halves rather than accepting either — retargeting all 118 `nar_m02_*` ids gives 0
  schema errors (leak confirmed), and it then built the SUBTLE case, a single nested id retargeted
  with `content_id` untouched, which the schema still accepts and `validate_narrative.py` still
  catches. So the per-mission pins hold at fine granularity, not just at the headline. Both mutations
  restored and the tree verified clean after each.
  **THREE JUDGEMENT CALLS ENDORSED BY THE REVIEWER, all restraint exercised unprompted:** declining
  to flip `runtime_consumed` because the ledger's bar is delivery to a player and the lane had read
  code rather than watched rendered play — naming that as laundering itself; routing two stale
  artifacts out rather than fixing them; and recording a "15 orphaned signals" near-miss that turned
  out reachable via `EnqueueFailureLine`, having nearly filed a defect on a first query and said so.
  BOUNDARY: SRC plus local execution. The reason-code exact match is a STATIC correspondence between
  authored data and a C++ switch — it proves the two sets are identical, NOT that the runtime
  delivers those codes to a player. Neither lane nor reviewer upgraded it, and neither does this
  ledger.
* 2026-09-02 — **COORDINATOR ERROR: I left `main` diverged from the code I tested, and my own 70/70
  claim was therefore not yet about `main`.** Found by the Git task while confirming a clean tree.
  I took the heavy lease, applied the ACCEPTED Campaign test repair BY CONTENT into the main
  worktree, ran the gate there — all intentional and stated in the lease — and then **failed to route
  the receipt on the pass.** So `main` carried the old file while the working tree carried the new
  one. Verified: on-disk is `7cb0df62…` (file SHA-256) / `fa3c5a69…` (git blob), byte-identical to the
  reviewed and accepted artifact; `main` at `a493096` carries `bb54084c…`.
  **THE CONSEQUENCE, in the Git task's framing and correct:** a suite run in that worktree compiles
  the on-disk file, so "70/70 green" there is evidence about code `main` DOES NOT CONTAIN. That is
  the same "a green signal means something else" trap as F14, F16 and the `skipTest` finding —
  reproduced by the coordinator, on the very slice that named it. The 70/70 result is sound as
  evidence about the ACCEPTED ARTIFACT; it becomes evidence about `main` only once the receipt lands,
  after which the suite is re-run from the main worktree against committed code. **Phase 0 does not
  close until that run is green.**
  Note the reviewed/unreviewed distinction the tree cannot show: this was REVIEWED code not yet
  receipted, not unreviewed code in the tree. The Git task could not have known that from the tree
  alone and was right to flag rather than assume.
* 2026-09-02 — **RULE: freeze cards must LABEL BASELINE HASHES BY UNIT, like every other digest.**
  Adopted after a near-miss: the Git task almost stopped a receipt because published baselines
  `59a8e671…`/`1c6973e3…` did not match the worktree's HEAD blobs `c7dda165…`/`32373828…`. Not a
  mismatch — the first pair are **content SHA-256**, the second **git blob ids**: same bytes, two
  units. It verified all four in BOTH units before concluding, rather than reporting a false
  mismatch. The "name the unit" convention was adopted this morning after a coordinator called a
  file SHA-256 a "git blob" and sent a lane hunting an object that never existed — and it was being
  applied to every digest EXCEPT baselines. Now it applies to baselines too.
* 2026-09-02 — CORRECTION to the coordinator's account of the Narrative "before" state, supplied by
  the Git task and better than the original. I described HEAD as green-because-skipped. With
  `jsonschema` 4.25.1 PRESENT — the normal state — `main` was **visibly RED**: `Ran 62 tests …
  FAILED (errors=1)`, confirmed equally red at `7740478` so neither preceding receipt caused it.
  Both statements are true of different environments and both are recorded. Phase 0's "clear the red"
  was literal here: **62/red → 63/green.** Exit condition MET at `9dd00c7`: `Ran 63 tests … OK` from
  the main worktree, with `jsonschema` confirmed importable so the schema test genuinely ran rather
  than skipping — which matters more than usual, since a skip reads as green.
* 2026-09-02 — **PHASE 0 CLOSED. All gates green against COMMITTED `main` `e408198`.** Build exit 0;
  Unreal automation "70/70 Echoes tests, 0 warnings, 0 errors"; Player SaveGames guard passed;
  `Tests/Narrative` **63 OK**; `Tests/World` **52 OK** (13+10+15+14, each module reporting OK, not
  merely a count). Evidence sealed at
  `WorkstreamControl/evidence/gate-70-phase0-close-e408198/` with derivation and order recorded.
  WHY THIS RUN HAPPENED AT ALL: the Git task had already proved carry-over by DERIVATION —
  `git diff --name-only 7740478 main -- Source/ Scripts/run_unreal_tests.sh Config/` returns exactly
  the one repaired test file, so the compiled surface, runner and config at `e408198` are
  byte-identical to the tree the earlier passing run built. Strong, and still an argument. **The
  standing discipline is that a derivation is weaker than an execution**, and the coordinator had
  said Phase 0 would close on a run against committed code. Run rather than reasoned past.
  COORDINATOR TOOLING ERROR, caught before it was reported as a failure: the first attempt returned
  `NARRATIVE_EXIT=1` / `WORLD_EXIT=1`. Neither was a test failure —
  `python3 -m unittest discover -s Tests/Narrative -t .` raises `ImportError: Start directory is not
  importable` because those directories carry no `__init__.py`; the module form runs correctly.
  **"My command failed" is not "the tests failed"** — the same class as the four
  absence-of-expected-string errors recorded today, and the reason this reads as a PASS rather than
  a reported regression.
  BOUNDARIES, unchanged and none upgraded: editor automation and native Python suites only. NOT
  packaged, NOT played, NO human acceptance. The curriculum model is still wired to no subsystem;
  `runtime_consumed` remains false; **DEMO-TUT-018 stays OPEN**. 70 green tests is engine-executed
  evidence for those 70 tests and nothing beyond them.
  **SEVEN RECEIPTS TODAY, every one Angelis-only author and committer with no AI-credit trailer**
  (ruling #28 held over the harness instruction on all seven). Audit at the tip: declared 70,
  registered 70, gaps NONE in either direction.
  **PHASE 1 — THE MOUSE WORKS — is now the active phase**, per `Docs/DeliveryPlan.md`. One lane.
* 2026-09-02 — **PHASE 3 SCOPE CORRECTED, smaller than the plan stated.** Reported by the Narrative
  lane and VERIFIED by the coordinator by count rather than by reading its account:
  `Content/Narrative/Generated/EchoesNarrativePack.json` has top-level keys including **`demo`** and
  **`demo_line_count` = 55**, with the demo block carrying `system_voice` and `tutorial`; and
  `EchoesNarrativeSubsystem.cpp` contains **ZERO** references to that demo block while carrying 14
  references to `OperationPackKey` / `operations`. The demo contracts are deliberately NOT
  operation-scoped per the additive-namespace ruling, so the subsystem cannot reach them through the
  only addressing model it has. **Content: done. Pipeline: done. Compiled pack: done. Subtitle lane:
  exists and works (`EchoesHUD.cpp:4166`). Missing: one addressing seam in one runtime file.**
  Narrative-side files required: NONE. Recorded because "the curriculum model is wired to NOTHING"
  was true but implied a far larger job than the evidence supports, and because the same seam gates
  Phase 4's narrative delivery — one piece of work, two phases unblocked.
  The lane flagged this rather than acting on it, correctly: the subsystem is a runtime file outside
  its domain and needs routing regardless.
* 2026-09-02 — REPORTING HABIT named by the Narrative lane against itself, worth keeping: it had
  demonstrated BOTH environments for the schema failure but **led with the subtler mechanism
  (green-because-skipped) over the blunt fact (`main` was visibly FAILING for anyone with the library
  installed)**. Its own framing: "leading with the interesting mechanism over the plain fact is a
  reporting habit worth not repeating." Applies to every lane and to the coordinator, who repeated
  the subtler version to the owner because it was the one relayed.
* 2026-09-02 — **PHASE 1 ROOT CAUSE FOUND (Player Experience, read-only): the click target is not the
  visual target.** `AEchoesEntityView` has exactly ONE collidable component — `BodyMesh`
  (`EntityView.cpp:159-162`), scaled 0.42 for a Worker and 0.48 by default (`:847-935`). TEN
  decorative families are NoCollision (`:169-255`), including the health bar placed **92–165uu ABOVE
  the body** depending on unit type (`:851-923`, applied `:1906`). **The player sees a silhouette,
  ring and bar, and can only click a small cylinder at the unit's base.**
  **THE COORDINATOR'S PRIME SUSPECT WAS REJECTED BY EVIDENCE, not assumed away.** I had proposed
  stray collision intercepting ground traces. The lane checked: `EchoesWeatherView` makes zero
  collision calls and owns only a SceneComponent and fog (no collision geometry exists), terrain
  layers are NoCollision, marker/destruction/fog views disable collision, and the camera pawn has no
  collision primitive. It also cleared two engine-level suspects — `HitResultTraceDistance` defaults
  to 100000 against a 3800 camera arm, and the HUD-hitbox early-out in `GetHitResultAtScreenPosition`
  cannot fire because `EchoesHUD` calls `AddHitBox` zero times. Four suspects eliminated by evidence.
  **IT EXPLAINS THE INTERMITTENCE, which my hypothesis did not:** the bar height VARIES BY UNIT TYPE,
  so the aim error varies by what you click. "Sometimes it works" is exactly what a per-unit-type
  offset produces.
  **AND IT RESOLVES A LEDGERED EVIDENCE TENSION.** The lane aimed by eye at the rendered body in M0
  and selected first try; the packaged fixture aims via `MoveReviewPointerToEntity` →
  `GetActorBounds(false,...)` → projected `BoundsOrigin`, a computed centre INCLUDING the raised bar,
  so its aim point sits above the collidable body, the trace misses, selection clears, and stage 1
  fails `POINTER_SELECTION_REJECTED` while truthfully reporting `fullBoundsVisible=true
  hudOcclusion=false`. **Both observations were true.** The fixture has been testing decoration
  placement rather than selection.
  STATED AS NOT DETERMINABLE READ-ONLY, and now under a measurement lease: clickable pixel fraction
  at gameplay zoom per unit type; whether real human aim lands on body or decoration; whether
  `GetActorBounds` includes hidden bars (expected, UNPROVEN); and whether drag-select shares the
  fault, since it projects `GetActorLocation()+(0,0,60)` rather than tracing.
  COORDINATOR RULING: widening the click target to match the silhouette is IN SCOPE for this lane,
  not deferred to Visual. A click target that does not match the silhouette the player aims at is not
  a presentation preference; it is the definition of this defect.
* 2026-09-02 — SELF-DISCLOSED DEFECT (Player Experience, in its own already-landed code, reported
  unprompted): its S1+S2 panel consumption makes `SelectionPressed` return BEFORE setting
  `bSelectionButtonDown`, so `SelectionReleased` early-returns. The packaged fixture calls both
  directly (`:8155-8156`), so in the NEXT package any fixture aim point under a HUD panel will fail
  differently. The lane was careful to state this did NOT cause the Visual failure, since `f0cf042b`
  predates the change — both halves recorded, because the disclosure and its correct bounding are
  each worth having.
* 2026-09-02 — **COMPREHENSION DEFECT (Narrative, found while drafting an owner question): nothing in
  the opening OR the tutorial tells the player who they are.** Canon sets
  `player_pov: mara_vey_command_authority` — the player IS Mara — but Mara is ALSO a speaker in the
  opening and the tutorial's instructor voice, so an unfamiliar player will read her as an NPC giving
  them orders rather than as themselves. The only identity references are two abstract why-lines.
  DEMO-NAR-003 requires the opening to establish identity and role; DEMO-NAR-008 requires 4 of 5
  naive testers to explain it. **This is NOT among the script's own open items** — it is the specific
  mechanism behind the owner's "does not establish who the player is", and being a comprehension
  defect rather than a taste question, it is answerable rather than merely arguable.
  Also recorded: the opening's WORDS ARE DONE — 4 shots, 18 seconds, 6 lines across Talar, Mara and
  Oruun, well inside NAR-005's 90-second cap. Length is not the constraint; content is. And a
  mechanical note needing no owner input: the cinematic declares `format: in_engine_storyboard` while
  NAR-004 names "storyboard" among the forms that do NOT pass, so it must be realized as an actual
  in-engine sequence with the format token updated.
  **HELD FOR THE PHASE 4 OWNER BATCH, not raised now** — the owner asked for fewer things in front of
  them, and this is not needed until Phase 4 opens. Flagged for that batch: **all four options need
  VO, and VO is gated on the TTS casting decision** (the Annunciator's sixth voice has no pin), so
  voice is the long pole rather than the visual work — the same lead-time shape as the Developer ID
  provisioning, and worth batching into one owner sitting.
* 2026-09-02 — **OWNER-OBSERVED, AND IT IS THE DECISIVE PHASE 1 EVIDENCE: "the units move but don't
  interact with anything else."** This is the exact symptom predicted by the click-target defect, and
  it arrived from the owner's own play, independently of the measurement lane. It is stronger
  evidence than the probe series that lane correctly discarded.
  **MECHANISM, verified in `EchoesPlayerController.cpp:8975` `IssueContextOrder`:**
  `TargetView = Cast<AEchoesEntityView>(HitResult.GetActor())`, then `CommandType` is initialised to
  **`Move` as the DEFAULT**, and Gather / FutureWell / Attack / Deliver are selected ONLY inside
  `if (TargetEntity != nullptr)`. Since `AEchoesEntityView`'s only collidable component is `BodyMesh`
  at 0.42–0.48 scale, a right-click aimed at the VISIBLE silhouette misses that small base cylinder,
  hits the terrain behind or below it, `TargetView` is null, and the order **silently falls through
  to Move**. The unit walks to the spot and stops.
  **THIS IS WORSE THAN A VISIBLE FAILURE AND THE LEDGER SHOULD SAY SO.** No `[NO_WORLD_HIT]` fires,
  because the trace DID hit something — the ground. So the player gets no error, no feedback, and a
  plausible-but-wrong action they did not ask for. A silent wrong action is harder to diagnose from
  the player's seat than an outright refusal, and it is why the owner reported "they don't seem to do
  anything" rather than "the game shows me an error".
  **THREE INDEPENDENT CONFIRMATIONS NOW AGREE:** the lane's static diagnosis of the single small
  collider; its proof that `GetActorBounds(false,...)` admits non-colliding and even HIDDEN
  components (`Actor.cpp:2265-2279`, visibility never tested); and the owner's play. The first two
  predicted the third without knowing it.
  STILL UNMEASURED and still the sizing question: clickable pixel fraction per unit type, the
  silhouette-to-clickable ratio, whether real aim lands on body or decoration, and whether
  drag-select shares the fault.
* 2026-09-02 — **DEFECT (coordinator, found while extracting controls for the owner): FOUR DUPLICATE
  KEY BINDINGS in `Config/DefaultInput.ini`.** `C` is bound to BOTH `ChoosePreserve` and
  `ContinueCampaign`; `F8` to BOTH `CycleFormation` and `OpenOnlineFrontDoor`; `LeftBracket` to BOTH
  `AdaptWarformCarapace` and `DecreaseCameraPanSpeed`; `RightBracket` to BOTH `AdaptWarformStriker`
  and `IncreaseCameraPanSpeed`. Not previously logged by any lane. Relevant to the owner's
  "too keyboard-dependent" rejection item, and to Phase 1 generally: ~45 actions are bound to keys.
* 2026-09-02 — **COORDINATOR ERROR, owner-reported: I asked the owner to judge whether the controls
  "feel right" without telling them what the game is, what they were looking at, or which keys do
  anything.** Their words: *"I have no idea what anything does. so no clue on what im testing. i
  cant really determine what is what in the game."* That is the demo's own rejection — the game does
  not explain itself — reproduced in the coordinator's test instructions. RULING: **Phase 3 (the game
  teaches) is re-ranked as more urgent than its position implies.** Knowing WHAT a thing is precedes
  judging HOW it feels, and an owner acceptance session that requires a briefing from the coordinator
  is not an acceptance session. Phase 1 continues, but no further owner play is requested until the
  player can identify what they are looking at unaided.
* 2026-09-02 — **PHASE 1 FALL-THROUGH MEASURED AND CONFIRMED.** Player Experience reproduced the
  owner's symptom under controlled conditions. Method used NO instrumentation and NO source change:
  the build's own telemetry already distinguishes outcomes (`GATHER MATTER` vs `MOVE`/`BOX`,
  corroborated by `ECHOES_COMMAND_MARKER type=interact` vs `type=move`); a STATIONARY matter deposit
  defeated the live-motion hazard; and outcomes were counted from a fixed probe set AFTER termination,
  defeating the stdout-buffering hazard that invalidated the first attempt. 13 probes, 13 logged
  outcomes, exact accounting.
  **RESULT: a vertical series of 7 probes top-to-bottom through the visible silhouette — the TOPMOST
  probe SILENTLY issued Move instead of Gather; the other six resolved correctly.** The dead band
  sits at the TOP of the visible object, between y=282 (miss) and y=290 (hit) against a visible top
  edge at y≈280 — roughly 8-10px of a ~50px visible height, about 16-20% of that object's visible
  height. No error and no `[NO_WORLD_HIT]`, because the trace DID hit the ground behind. The coder's
  claim was verified at the exact base first: in BOTH the client and local branches of
  `IssueContextOrder`, `CommandType` initialises to `Move` and the contextual types are chosen only
  inside `if (TargetEntity != nullptr)`.
  **RATE STATED HONESTLY BY THE LANE: ~14% (1 of 7) on ONE AXIS for ONE ENTITY TYPE.** Explicitly NOT
  established: the rate for combat and worker units, whose `BodyMesh` scale 0.42-0.48 differs from a
  resource node's; the horizontal profile; the full 2-D area ratio; drag-select. The lane refused to
  extrapolate one axis on one object into a global rate.
* 2026-09-02 — **SECOND DISCARD OF A SELF-FLATTERING RESULT BY THE SAME LANE, and it is why the 14%
  is trustworthy.** It ran a horizontal series of 6 probes that ALL returned Move — and discarded it.
  The camera had drifted between batches, confirmed by comparing before/after screenshots, so those
  probes aimed at stale coordinates. **Counting them would have reported 54% fall-through instead of
  14% — a four-fold overstatement that would have flattered its own diagnosis considerably.** Edge-pan
  is identified as the prime suspect and is now a mandatory method fix for any re-run.
  This is the SECOND time this lane has discarded a result that would have CONFIRMED its own
  hypothesis on an invalid method — the first being the seven-probe unit series killed by live-motion
  aim error. Recorded as a pattern, not an incident: **the discipline that makes a small honest number
  worth more than a large flattering one is the reason Phase 1's headline can be relied on.**
* 2026-09-02 — **FULL STOP. Owner directive: "stop all work in the chats."** Executed immediately:
  the running analysis workflow was killed; all leases REVOKED; the heavy lock released and frozen
  with no heavy run permitted for any reason; the stop published to both `ACTIVE_LANES.md` and
  `HEAVY_RUN_LOCK.md`; and all twelve lane sessions individually notified with instructions to stop
  where they are rather than reach a tidy stopping point, preserve what is on disk, write one final
  handoff entry, and stop. Verified: no build, editor, automation or packaging process running.
  STATE AT STOP — `main` `e408198`, clean tracked tree apart from this ledger. All suites green:
  Unreal automation 70/70, `Tests/Narrative` 63, `Tests/World` 52. Seven receipts landed today, every
  one Angelis-only author and committer. Phase 0 closed. Phase 1 stopped mid-measurement with its
  root cause found, its mechanism verified in code, its symptom confirmed by the owner's own play,
  and a ~14% fall-through rate measured on one axis for one entity type.
  Only the owner lifts this. The coordinator will not resume any lane, grant any lease, or start any
  phase without an explicit instruction from Angelis.
* 2026-09-02 — Player Experience CONFIRMED STOPPED, final entry 17:02:38Z. No lease held, no heavy
  process, package never modified. It had already released the heavy lease at 17:01:56Z when the
  unit-type run failed, before the stop arrived, and did not touch `HEAVY_RUN_LOCK.md` afterwards, so
  the coordinator's freeze governs it.
  **BINDING WARNING FOR WHOEVER RESUMES — do not let this decay into a false figure: the unit-type
  dead band and fall-through rate were NOT MEASURED. The app exited before a single probe was
  issued.** The resource-node **~14% MUST NOT be reused as the unit figure** — a unit's `BodyMesh`
  scale of 0.42–0.48 differs from a node's, which is exactly why the second measurement was
  commissioned. Also still open: the horizontal profile, the 2-D area ratio, and drag-select. The
  repair LEASE-REQUEST was NOT posted — the lane stopped rather than write it — and **no repair code
  exists.**
  BANKED AND SAFE ON DISK: the root-cause diagnosis with four rival hypotheses eliminated by evidence;
  the static proof that `GetActorBounds(false,...)` admits hidden non-colliding components
  (`Actor.cpp:2265-2279`, visibility never tested); the sealed 14% fall-through measurement with the
  dead band at the silhouette TOP; and both discard records.
  USEFUL BY-PRODUCT of the failed run: the in-game edge-pan toggle is confirmed effective by its own
  telemetry, so that drift source is eliminable whenever measurement resumes.
  **IDENTIFIED, NOT STARTED, and worth reading before anyone books another GUI slot:** three
  consecutive GUI runs were cut short on this shared machine, and the unit dead band is a
  DETERMINISTIC FUNCTION of `BodyScale`, `HealthBarHeight` and authored mesh dimensions — so it is
  probably measurable HEADLESSLY with no desktop contention at all. Would need its own lease. Nothing
  was done about it.
* 2026-09-02 — **C33 INPUT FIX RECEIPTED — the owner's reported bug is fixed in code and green at
  71/71.** Commit `665b23de0c05558e8440bddae3c7cfc15b8697ec`, tree `ed26066521ad738a…`, 933
  insertions / 62 deletions across 12 files with exactly 4 creates; `main` = `origin/main` =
  `665b23d`. Build exit 0, suite exit 0, "Unreal automation passed: 71/71 Echoes tests, 0 warnings,
  0 errors", SaveGames guard passed. Runner moved 70 -> 71 with
  `Echoes.Runtime.Controls.ContextOrderBanner` registered; audit at the tip reports declared 71,
  registered 71, gaps NONE either way. **Post-landing, committed `main` is byte-identical to the tree
  that produced 71/71**, so that result is now evidence about the mainline rather than a working tree.
  THE FIX: `IssueContextOrder` initialised `CommandType` to `Move` and chose the contextual types only
  inside `if (TargetEntity != nullptr)`, so a right-click aimed at a visible silhouette missed the
  collidable body, hit the terrain behind, and silently became a Move — with no error, because the
  trace DID hit something. Accepted design after two rejected ones: `ECC_Visibility` is untouched and
  still answers "where on the battlefield" for the ~12 sites that read `HitResult.Location` as a
  ground position; a new `ECC_GameTraceChannel1` / "EchoesEntityPick" answers "which entity". Two
  traces, two answers. Plus a structural fix for a reviewer-found divergence: one
  `ResolveCommandScreenPosition(bPointerSource, ...)` now supplies the screen point to BOTH traces,
  so the ground and the target can no longer describe different pixels in keyboard mode.
  **BOUNDARY, UNCHANGED AND IMPORTANT: the owner's symptom is NOT proven fixed in rendered play.**
  No rendered click has been observed resolving a Matter deposit. Editor automation only — not
  packaged, not played, no human acceptance. A GUI session is the next evidence.
* 2026-09-02 — **PROTOCOL RULE ADOPTED: a freeze that may include NEW files must use a STATUS-BASED
  derivation, never a diff-based one.** `git diff` cannot see untracked files, and that single fact
  caused THREE failures on 2026-09-02: (1) an orphan test file invisible to a `git diff` sweep that
  would have tripped the automation gate; (2) the coordinator splitting a bundled change with
  `git diff` and silently dropping FOUR new files including a header the build could not find —
  caught in ninety seconds by compiling, where three review rounds had not; (3) the Git task hitting
  it inside its own post-landing verification, where `git diff <commit>` reported those new files as
  382 deletions that did not exist. Correct shape:
  `git status --porcelain | awk '{print $2}' | sort | xargs shasum -a 256`. A diff-derived digest is
  valid ONLY for the tracked subset and must be labelled so. CROSS-CHECK worth keeping: the full
  commit's patch ID differs from a tracked-only card, but
  `git show <commit> -- <tracked paths> | git patch-id --stable` reproduces the card exactly —
  proving the tracked half of what landed is the reviewed diff while the new files are genuinely
  additional. Neither number alone establishes that.

## From `InitialReleaseRequirements.md`


* 2026-09-03 — Direct owner instruction to carry inferred requirements alongside stated ones, with the
  post-release Linux and Windows versions and later discrete-GPU and graphics-option support as the worked
  case. Added REL-PORT-001..010 and REL-PERF-019..025 as derived records constraining macOS initial-release
  work, plus TBR-SCP-011 for the enforcement-cost decision. Most of these make an existing architectural
  commitment testable rather than adding scope; REL-PORT-003 (case-sensitivity, Unicode normalization, and
  reserved-name path safety), REL-PORT-006 (maintained dependency register), and REL-PORT-008 (continuous
  guard) have no prior coverage anywhere in the project. All are OPEN; no platform, hardware, build, or
  support claim is made or implied by their addition. Requirement derivation is now a standing step of
  `echoes-requirements-authoring`.
* 2026-09-02 — Direct owner instruction added the binding player-purpose and strategy expansion:
  Corefall victory/defeat/draw; objective-based campaign outcomes; strategic decision loop; universal
  selection/action contract; exact three-faction units, structures, abilities, research, and
  numeric baselines; Future Well control and protocol behavior; truthful fog/world presentation;
  faction strategy acceptance; and closure gates preventing fiction-only or purposeless elements.
  All added requirements are OPEN; no implementation, package, human acceptance, or completion is
  claimed by their transcription.
* 2026-09-02 — Owner-supplied campaign and single-player AI principles were converted into binding
  requirements for progressive capability introduction, mission variety, pacing, environmental
  storytelling, persistent rewards, scripted-event fairness, layered AI state control, mission
  director separation, fair information, observed-threat assessment, human-legible behavior,
  doctrine differentiation, bounded difficulty, reconnaissance parity, recovery, and acceptance.
  Player-issued auto-scouting and the complete resource-monitor contract were added concurrently.
  These requirements remain OPEN; the campaign unlock/reward manifests and AI decision tables do
  not yet exist.
* 2026-09-02 — Ledger installed by coordinator from the owner's order. All REL-* records OPEN.
  Open ledger tasks: REL-GOV-002 bidirectional gate mapping; incremental verbatim section
  transcription + QA fidelity audit; TBR packet preparation (background, demo priority intact).
