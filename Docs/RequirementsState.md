# Echoes of the Broken Sun — Requirements state

**Author and owner:** Angelis Pseftis
**Standing:** the sole record of per-requirement engineering state, acceptance, and decision history.
**Created:** 2026-09-03.

Requirement bodies live in **[`Requirements.md`](Requirements.md)** and are never restated here.

## State vocabulary

Agent-assignable: `OPEN` → `IN PROGRESS` → `IMPLEMENTED` → `AGENT VERIFIED` → `EVIDENCE READY` →
`AWAITING HUMAN ACCEPTANCE`, plus `BLOCKED`. Owner-only: `HUMAN ACCEPTED`,
`HUMAN REJECTED — CHANGES REQUIRED`, `COMPLETE`. A parent stays open until every mandatory child is accepted. `IMPLEMENTED — NOT YET VERIFIED` is a
legacy alias of `IMPLEMENTED`, not a separate state. Test-output PASS means only that the named test
passed; an unconditional requirement PASS/COMPLETE needs recorded owner acceptance.

Read state at the exact requirement/build/evidence boundary. Dated entries override creation defaults
only for the IDs and scope they name. A family summary is a navigation aid, not proof that every child
has its required evidence. Preserve historical claims while recording any missing or conflicting support.

## Global verdicts and identities

* DEMO-GOV-001 stands: the current demo is `HUMAN REJECTED` (owner, 2026-09-02). Rejected
  candidate identity: package `BuildArtifacts/Packages/Mac-Development-20260902T011241Z-f0cf042b/`
  from clean `f0cf042bea800c474b1c3e08c557d0aae49ff744` (origin/main), macOS Apple Silicon.
* Authoritative source state at ledger creation: `origin/main = f0cf042`, tree clean.
* The original model-specific lane assignment is retired. Current task ownership follows `AGENTS.md`;
  this state record does not permanently assign a model to writing or review.


## Record defaults (apply to every requirement below unless its row states otherwise)

* Engineering state: `OPEN`. Human acceptance state: none (not yet offered). Acceptance
  date/notes: none. Commit/package identity: none yet (recorded when work starts).
* Dependencies: the milestone ordering in the directive §6; per-ID exceptions recorded inline.
* New evidence locations: `BuildArtifacts/Evidence/<gate>-<UTC>/` or the designated evidence root.
  Historical external paths below remain original receipts and must be located before reuse.
* Known limitations: none recorded yet.
* Verification-method classes (referenced per row): **PKG-PHYS** = packaged build, physical
  mouse/keyboard input; **PKG-REND** = packaged build, rendered/audible inspection; **PKG-AUTO**
  = packaged-build automation (bounded claims only); **EDT** = editor demonstration (never
  substitutes for PKG classes); **SRC** = source/test inspection; **HUM** = uncoached
  project-naive human sessions; **OWNER** = personal owner test/acceptance.
* Owner lanes: GOV=Coordinator+QA; JRN=Player+Campaign; NAR=Narrative+Visual+Audio;
  TUT=Campaign+Player; INP=Player; UI=Player+Visual; AUD=Audio; VIS=Visual+World;
  PERF=Performance+Build; AI=Opponent-AI; ACC=Player; VAL=QA+Build+Coordinator.


## Current registry coverage — 2026-09-04 reconciliation

The master currently registers **1,125 parent identities** (433 SPEC, 152 DEMO, 540 REL), including retained
retired identifiers, and 1,892 subordinate definitions. These are structural counts, not accepted behaviors.
`Scripts/check_requirement_registry.py` checks exact identity/index coverage. The historical family summaries
below are preserved as receipts; they are not the current inventory or blanket verification status.
New/restored/rebound records use the exact current entries at the end of this file. Session multiplayer is
now approved release scope, not dormant release scope; offline isolation remains mandatory.

| Family | Registered parents | Current evidence interpretation |
|---|---|---|
| `DEMO-ACC-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-AI-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-AUD-*` | 13 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-GOV-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-INP-*` | 15 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-JRN-*` | 7 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-NAR-*` | 11 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-PERF-*` | 15 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-TUT-*` | 22 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-UI-*` | 13 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-VAL-*` | 17 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `DEMO-VIS-*` | 13 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-ACC-*` | 22 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-AI-*` | 42 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-ART-*` | 33 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-AUD-*` | 23 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-BLD-*` | 20 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-CAM-*` | 38 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-CIN-*` | 8 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-CMB-*` | 32 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-DIST-*` | 17 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-ECO-*` | 17 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-EDT-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-FAC-*` | 29 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-FTU-*` | 12 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-GOV-*` | 15 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-LOC-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-MP-*` | 19 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-PERF-*` | 25 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-PORT-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-PUB-*` | 20 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-QA-*` | 36 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-QOL-*` | 16 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-SAV-*` | 14 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-SEC-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-SIM-*` | 19 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-STAB-*` | 5 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-UI-*` | 28 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `REL-WEL-*` | 18 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-ACC-*` | 5 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-AI-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-AIST-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-ARC-*` | 3 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-ART-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-AUD-*` | 3 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-AUDF-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-AUT-*` | 5 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-AUTH-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-BAL-*` | 8 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-BLD-*` | 13 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-BUD-*` | 8 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-CAM-*` | 11 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-CAN-*` | 2 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-CANON-*` | 16 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-CIN-*` | 2 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-CMB-*` | 12 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-CMD-*` | 15 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-CTL-*` | 19 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-DIF-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-DOC-*` | 5 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-ECO-*` | 7 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-END-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-EVID-*` | 8 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-FACID-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-FOG-*` | 3 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-HUD-*` | 7 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-INFO-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-LOC-*` | 2 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-LSN-*` | 11 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-MAP-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-MOD-*` | 7 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-MOV-*` | 13 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-MSN-*` | 15 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-OUT-*` | 7 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-PIL-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-PLAN-*` | 15 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-PLAT-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-PRD-*` | 10 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-RES-*` | 8 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-SAV-*` | 5 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-SCT-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-SIM-*` | 15 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-SKM-*` | 18 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-STANCE-*` | 5 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-STR-*` | 12 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-TEC-*` | 8 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-TECH-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-TER-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-TUT-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-UI-*` | 6 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-UNIT-*` | 12 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-VAL-*` | 3 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-VISD-*` | 8 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-WEL-*` | 4 | Use exact dated ID/build/class entries; this count grants no acceptance. |
| `SPEC-WELLP-*` | 3 | Use exact dated ID/build/class entries; this count grants no acceptance. |

## Historical `SPEC-*` family snapshot

The family table below retains previously recorded engineering summaries. The former blanket “all OPEN”
statement contradicted these rows and has been removed. Use exact dated per-ID entries and artifacts;
`AGENT VERIFIED` is bounded by the cited check and does not supply missing packaged or human evidence.
Identifier and family-count reconciliation remains open in the audit decision below.

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


The release family table retains prior summaries; the blanket all-OPEN label and aggregate count were
stale. Later entries record changes for individual IDs. Do not infer completion or a complete inventory
from these counts; reconcile against exact master definitions and the dated evidence.

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
| `REL-AI-*` | Skirmish & Opponent AI | 40 | `PKG-AUTO`, `SRC`, `PKG-REND` | 2 `AGENT VERIFIED` (`REL-AI-020`, `REL-AI-026`), 38 `OPEN` |
| `REL-QOL-*` | Replays & Quality-of-Life | 16 | `PKG-PHYS`, `PKG-AUTO` | `OPEN` |
| `REL-UI-*` | UMG/Slate Interface & HUD | 24 | `PKG-REND`, `PKG-PHYS` | `OPEN` |
| `REL-ART-*` | World Art & VFX Readability | 27 | `PKG-REND` | 1 `AGENT VERIFIED` (`REL-ART-026`), 26 `OPEN` |
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

Append-only. The original entries migrated from retired ledgers are preserved, followed by later records.
Historical roles, paths, and “current” claims apply only at their recorded boundary; use `AGENTS.md` for
current operations. Preserve original evidence and owner wording; append corrections and supersessions.

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
* 2026-09-04 — Gate 32 (Track F1: Skirmish Setup) completed and verified under ledger `SKIRMISH-F1-001`:
  * Advanced `REL-AI-020` and `REL-AI-026` from `OPEN` to `AGENT VERIFIED`:
    * `REL-AI-020` (Skirmish Mirror Matchup Support): Validated that all 9 matchup combinations across Meridian, Kharuun, and Hollow Choir (including mirror matchups MM, KK, CC) execute cleanly without assertion failure. Setup cycler steps legally into mirror states without skipping past identical factions. Covered by `SRC` + `PKG-AUTO` in `Echoes.Runtime.Gameplay.SkirmishSetup` inside clean 76/76 Unreal automation suite run.
    * `REL-AI-026` (Skirmish Contract): Skirmish setup exposes all 9 parameters in model and UI before deployment: Local Faction, Opponent Faction, Teams (1v1, FFA), Battlefield Map (Glass Scar, Crownfall Basin, Soryn Confluence), AI Profile (5 authored doctrines: Defensive, Raider, Economic, Expansionist, Adaptive), Difficulty (Assisted, Standard, Challenging, Sovereign), Starting Resources (250/400/700 Matter per `REL-ECO-002.AUTH`), Victory Conditions (Corefall, Well Control, Conquest), and Game Speed (0.75x, 1.0x, 1.5x). Unauthored `Balanced` AI doctrine is unreachable from selector. Disclosed Assisted handicap banner (`+50% reaction delay (1.5s), APM ceiling 30, -20% combat damage multiplier`) rendered in dedicated non-overlapping geometry; Standard AI enforces 100% fair information model (`SPEC-AI-001/002`). Covered by `SRC`, `PKG-AUTO`, and `PKG-REND` (1920×1080 captures `skirmish-setup-standard.png` and `skirmish-setup-assisted.png` in `BuildArtifacts/Evidence/release-gate32-skirmish-setup/`).
* 2026-09-04 — Gate 6 (Track A4: Environment Completion & Site Dressing Pass) completed and verified under ledger `WORLD-A4-002`:
  * Advanced `REL-ART-026` (Landscape Passability Truth) from `OPEN` to `AGENT VERIFIED`:
    * Passability Truth & Zero Simulation Touch (`SIM-002`, `REL-ART-026`): 100% of dressing records in `lume_reach_dressing_pack_v1.json` (39 records) and `glass_scar_dressing_pack_v1.json` (29 records) occupy cells strictly marked `Blocked` in underlying map contracts (`overlay_map_packs_v1.json`). Zero records placed on open or navigable paths.
    * Presentation-Only Invariant: In-engine instancing in `AEchoesTerrainView` guarantees presentation-only behavior: layers enforce `ECollisionEnabled::NoCollision`, `bGenerateOverlapEvents = false`, `CastShadow = false`, and `CanEverAffectNavigation() == false`.
    * Anti-Glint Specification (`REL-ART-003`): All civic dressing materials (Pale Ceramic plates `0.68, 0.66, 0.62`, Charcoal foundations) enforce a strict matte roughness floor of $\ge 0.85$ (measured 0.88 and 0.92) to eliminate specular glint noise. Broken-Sun Amber interior lighting (`0.92, 0.52, 0.06`) provides warm accent visibility without bloom leakage.
    * Scoped Fog of War & Reshape Truth: Exploratory visibility gating and dynamic Reshape open/close deactivation verified across both Glass Scar and Lume Reach profiles. Refusal telemetry (`[ECHOES_DRESSING_REFUSED]`) verified on passability violations.
    * Covered by `SRC` (`EchoesTerrainView.cpp`, `test_lume_reach_dressing.py`), `PKG-AUTO` (`Echoes.Runtime.Map.LumeReachDressing` and `Echoes.Runtime.Map.GlassScarDressing` passing cleanly in 77/77 Unreal automation suite), and `PKG-REND` (1920×1080 rendered review captures `LumeReachReview.png` and `LumeReachOverview.png` in `BuildArtifacts/ChoirAtLumeReach/` verifying zero bare collision floor).




## 2026-09-04 — Agent guidance synchronization and explicit campaign direction

**Author and owner:** Angelis Pseftis
**Evidence boundary:** documentation/source inspection on `release/world-map-concept-pass`, base `fc05cdf`,
with pre-existing dirty code/assets. This entry records documentation changes and owner direction; no
runtime, package, physical-play, performance, or human-acceptance result is claimed.

Owner direction: “Each Mission 1-15 should have a unique map that plays into the game story.” The owner
also requires a strong storyline, strong characters/backstories, and a massive connected-world feeling
across related battlegrounds. The MMO comparison supplies the intended sense of adventure and place within
the RTS; no MMO/networked/shared-world scope was requested. The existing skirmish map set remains separate.

`SPEC-PRD-006` and `SPEC-CAM-001` are clarified in place. Added:

| ID | Engineering state | Implementation/evidence binding | Human acceptance |
|---|---|---|---|
| `SPEC-MAP-004` | OPEN | No result claimed; fifteen distinct story-driven campaign map contracts and presentation require qualification. | None |
| `SPEC-CAM-041` | OPEN | No result claimed; connected geography, campaign-map/transition delivery, and experience require qualification. | None |
| `SPEC-CAM-042` | OPEN | No result claimed; mission-to-character/backstory/place trace and in-game delivery require qualification. | None |

All new child records inherit OPEN. Authoring the requirements proves no map or story implementation.
Existing evidence for amended parents remains bounded by its original scope and does not discharge these
new obligations. Owner review is required for final acceptance; current authorized implementation may proceed.

**DOC-SYNC-001 — Shared operating rules.** `AGENTS.md` is the common agent contract. Client entry files and
all canonical skills refer to it. Current task/path ownership and resource coordination replace missing
external lane files and old model-specific assignments. `Requirements.md` owns bodies; this file owns
lifecycle, defects, evidence state, and owner decisions. `ProjectLedger.md` and the directive's gate matrix
retain historical evidence. Document maintenance does not require a game-release acceptance gate.

**TBR-DOC-001 — Map and story-reference reconciliation — OPEN.** The map references propose six skirmish
maps/additional formats against the master's three-map baseline; some M09–M12 descriptions conflict with
creative mission authority. The new direction resolves campaign quantity at fifteen unique maps. It does
not accept the conflicting skirmish formats or rewrite mission roles/objectives. Continue authorized
presentation work within its boundaries; before changing gameplay/canon, reconcile each site against its
master mission and creative source. Record any material alternatives and seek only the decision still missing.

**DOC-SYNC-002 — Identifier and state-index reconciliation — OPEN.** Source inspection found stale totals,
index/body mismatches, and reused IDs. Before this entry's three additions, the index contained 1,066 unique
base IDs (393 SPEC, 152 DEMO, 521 REL), which is an index count, not proof of complete definitions. Examples:
`SPEC-CANON-015/016`, `SPEC-RES-004..007`, and `SPEC-TEC-003..008` have bodies absent from the index;
`REL-AI-016` is reused for strategy-controller weights and the competitive-balance band; `REL-AUD-002`
is reused for dynamic vocal ducking and integrated loudness. Cite exact body titles while reconciling IDs. Family summaries
also contain stale totals and broad verification labels. Preserve every ID/body and historical result;
reconcile definitions and evidence before implementing or closing an affected requirement. Where two
bodies differ, neither “last one wins” nor a family PASS is an acceptable resolution. The next requirements
maintenance pass must enumerate the discrepancies, preserve intended obligations, and obtain an owner
ruling for semantic conflicts. This documentation audit does not certify the entire requirements corpus.


## 2026-09-04 — Contextual detail, functional readability, and coherent action

Owner direction: details throughout the maps, units, and buildings must fit their place in the story and
communicate what belongs there, what an object does, and how it is used. Both large and small details need
deliberate planning; visuals, movement, actions, and sound must feel smooth, purposeful, and coherent.

Added `SPEC-VISD-008` (contextual brief before production) and `SPEC-ART-004` (integrated craftsmanship and
role readability). Their parents and children are **OPEN** with no implementation/evidence binding or human
acceptance. Clarified `SPEC-VISD-006`, `SPEC-ART-002`, and `SPEC-AUDF-004` in place to connect world detail,
action transitions, and material sound. Prior evidence for those records does not automatically cover the
clarification. This is owner-directed design/quality scope, not proof that current assets meet it.

Planning uses the existing map/art/audio/character records in place. Internal preparation and qualification
may continue under current owner authorization; final player/owner experience review remains separate.


## 2026-09-04 — World production brief and canyon visibility correction

**Scope:** active world-map work on `release/world-map-concept-pass`, base `fc05cdf`, dirty checkout.
The current production section in `MapConcepts.md` now contains fifteen mission-to-place rows and shared
craftsmanship decisions. Internal source review checked all rows against the detailed mission contracts;
M01 character attribution, M02 ecology/objective boundaries and M07's singular Spine site were corrected.
This establishes a planning artifact, not fifteen implemented maps or stronger narrative delivery.

- `SPEC-CAM-042.TRACE`: **IN PROGRESS**. Fifteen-row story/character/place trace authored; runtime delivery,
  compiled mission-map identity and complete branch continuity qualification remain outstanding.
- `SPEC-VISD-008`: **IN PROGRESS**. Mission context, meaningful detail, applicable action/sound and exclusions
  are specified at planning level. Per-asset briefs and map source/binding work remain.
- `SPEC-MAP-004`: remains **OPEN** for completed unique map bindings/layouts and packaged comparison.
  Six reusable biome families and the present screenshots do not satisfy fifteen distinct battlefields.
- `SPEC-ART-004`: remains **OPEN** for integrated production quality and the prescribed rendered evidence.

Canyon visibility regression evidence: `BuildArtifacts/Evidence/world-map-concept-pass/build-fog-camera.log`
reports a successful editor build (with one existing font API deprecation warning); the retained Unreal
report `automation-before-perimeter.json` reports 80 successful tests, zero test warnings/failures/not-run
at `2026.09.04-19.52.56`. `unreal-fog-regression.log` records the wrapper result. Chasm meshes and lights were
checked through unexplored, visible, explored and reset states. This is editor automation, not packaged
fog qualification or a visual completion claim. Later perimeter/grass edits require fresh verification.
No owner acceptance or requirement completion is assigned.


## 2026-09-04 — Requirement identity, mission design and release-scope reconciliation

**Author and owner:** Angelis Pseftis. **Authority:** owner's “Proceed with that” authorizes the proposed
ID/index and mission-design reconciliation followed by representative qualification. The separate scope
answer below expands the release. Work is in the existing dirty `release/world-map-concept-pass` checkout
at base `fc05cdf`; concurrent production retains runtime/source/assets and heavy-run ownership.

**DOC-SYNC-002 — Registry structure corrected; semantic decisions remain separately tracked.** The
master now has one definition/index row per parent and unique subordinate identifiers. Nine historically
ambiguous base IDs are retained as tombstones with eighteen titled successor bindings. Repeated identical
authority, outcome and technology copies now point to a single canonical body. Scenario/card fields,
the M13 AUTH typo and movement VERIF/LANE typo are corrected with a retained crosswalk. Index titles derive
from the actual current body; a retired ID never displays one old meaning as if it won the collision.
The registry guard detects duplicate definitions, missing parents, index/body mismatch and truncated bodies.
It does not establish all semantic consistency or a full requirement-to-test acceptance matrix.

| Retired ambiguous ID | Current titled successors | Evidence disposition |
|---|---|---|
| `REL-AI-016` | `REL-AI-041` strategy/fair-fog; `REL-AI-042` Standard matchup balance | Old ID alone maps to neither; inspect title/clause/test/revision. |
| `REL-ART-024` | `REL-ART-028` Meridian forms; `REL-ART-031` cosmetic ragdolls | No automatic acceptance transfer. |
| `REL-ART-025` | `REL-ART-029` Kharuun forms; `REL-ART-032` scorch/vitrification | No automatic acceptance transfer. |
| `REL-ART-026` | `REL-ART-030` passability truth; `REL-ART-033` shield ripples | WORLD-A4-002 explicitly concerns passability, not shields; preserve the receipt and recheck its retained evidence before assigning current successor verification. |
| `REL-AUD-001` | `REL-AUD-019` combined loudness; `REL-AUD-020` submix routing | Loudness session clauses also carry the second former loudness body. |
| `REL-AUD-002` | `REL-AUD-019.SESSION` loudness session; `REL-AUD-022` critical-dialogue ducking | Ducking policy remains TBR-DOC-003 until owner selects it. |
| `REL-AUD-003` | `REL-AUD-021` spatial attenuation; `REL-AUD-023` general-dialogue ducking | Spatial attenuation is separate from ducking. |
| `REL-UI-017` | `REL-UI-025` command deck; `REL-UI-026` selection identity | No automatic acceptance transfer. |
| `REL-UI-018` | `REL-UI-027` UI atlas; `REL-UI-028` selection state | No automatic acceptance transfer. |

Successor parents start **OPEN** unless a later exact evidence entry establishes otherwise. Tombstone
retirement records identity repair; it is not game-requirement completion. `SPEC-CANON-015/016` are duplicate
aliases to restored `SPEC-CAN-001/002`. Full title/source crosswalk remains in the master. Existing `.SIG`,
`.ASSET` and `.SHEET` artifacts are supporting clauses, not a new evidence class or proof of completion.

**Fifty lost bodies restored.** The prior index referred to records whose bodies had disappeared. Exact
normative text was recovered from committed masters: `2ca9e059ce1cc138740077dcc4a7d3ffe8b59faf` for
`SPEC-CTL-016..019`; `67a44c3cc16d01a291ee55f150a9d2d11aa9ebac` for `SPEC-CMB-011/012`, `SPEC-AUT-005`,
`SPEC-CAN-001/002`, `SPEC-ECO-001..006`, `SPEC-OUT-001..007`, `SPEC-PIL-001..010`, `SPEC-STR-001..012`, and
`SPEC-TECH-001..006`. Original source line ranges and exact recovered text are retained in the audit evidence
receipt. This is recovery of previous master content, not a claim that each threshold was personally
approved or tested. Current restored-record engineering state is **OPEN pending evidence reconciliation**;
past state/log entries remain unchanged. Network acknowledgement now also includes the retained owner's
negotiated-delay instruction, bound to REL-MP-005. Steering and whole-tick budgets retain their separate
measurement scopes. Twelve structure tables and six technology rows preserve their stable IDs; newer
role/action descriptions do not erase their interaction, failure or numerical clauses.

**TBR-DOC-001 — Map-reference conflict resolved at design-document level.** The current fifteen-row
story/place brief is retained. Obsolete conflicting mission studies, invented causal geography, objective
coordinates, unmodeled casualties and six-map skirmish proposals are removed from active map references.
M06 is Talar/Meridian; M08 is Talar's Meridian proxies guided by Neme, with no playable Choir there. The
M08 plan is corrected to the detailed `SPEC-MSN-008` and creative canon. M09 is Mara's exactly-two-district
allocation; M10–M12 retain Oruun's contact/liability/public-readback contracts. Fifteen distinct campaign maps
and three named offline skirmish maps remain separate. New multiplayer formats follow the explicit scope
answer below, not the retired six-map lists. This resolves those conflicting reference instructions; it
does not qualify their game implementation.

**M01 representative slice — IN PROGRESS (planning/source boundary only).** Existing map documents now
specify M01's scene, character stakes, carrier recovery22,18/extraction6,17, distinct campaign layout,
unit/building role details, contextual materials, motion/audio and review views. The technical blueprint
defines source binding, all three Well paths, failure/retry, ledger receipts, persistence, ordinary M02
continuation, integrated craft and human/owner evidence. Optional rewards are conditional. M01 requires its
own map binding; sharing a palette, source fixture or Glass Scar route graph is insufficient. None of the
new five world/craft parents is accepted or complete. Map/narrative runtime binding, authored voice/subtitle/
cinematic delivery, real gameplay captures/listening, package/physical paths and owner review remain.

**TBR-SCP-001 / expanded release scope — RESOLVED by owner.** The owner answered:
“Include those expansions in this release and reconcile the larger scope.” The question explicitly named
the separate 25-sector Conquest/roguelite, team battles and free-for-all alongside fifteen story missions
and three offline 1v1 maps. This activates `REL-CAM-033..038`, `REL-AI-037..040`, and applicable `REL-MP-*`
as release obligations. Their engineering state is **OPEN** except for separately supported exact evidence;
release-wide multiplayer dormancy is superseded. `SPEC-PRD-003/007`, replay/privacy/scope/portability/lobby
references and the delivery/public direction are aligned. New `SPEC-SKM-014..018` and `REL-MP-018/019`
(and children) start **OPEN** with no evidence or human acceptance.

The six-participant ceiling derives from the already required 3v3 mode; FFA retains its four-player
ceiling. It does not add six-player FFA, an MMO, a shared persistent world, a cooperative rewrite of the
fifteen operations, another platform, an account service, or monetized progression. Conquest's seed/run/
territory persistence is separate from the authored campaign ledger. Corefall remains the skirmish win
condition; Conquest is a separate mode, not a new skirmish victory selector. Historical claims of Well
Control/Conquest skirmish selectors are not a scope decision. Every format needs its actual spawn/map,
fairness, full-load performance/soak, session recovery, security, package and human evidence.

**TBR-DOC-003 — Dialogue ducking — OPEN, owner choice pending.** `REL-AUD-022` lowers Music/SFX by6dB
with300ms attack/500ms release for critical dialogue; `REL-AUD-023` lowers Music6dB/Ambience4dB within150ms,
keeps combat/interface cues and restores over500ms. Triggers overlap, so neither policy is implicitly
selected by position, title or an old AudioDirection recipe. A choice is pending from the owner. Both
policy records are **BLOCKED** for dependent mix implementation/qualification; other audio work may proceed.

**TBR-DOC-004 — Economy alternatives — OPEN, owner choice pending.** Restored `SPEC-ECO-002/004/005`
specify three workers, assigned/round-trip delivery and a200-tick exhausted marker. `SPEC-RES-003/005/006`
specify two workers, closest reachable automatic fallback and immediate exhausted-marker transition.
The owner is choosing the baseline; preserve both bodies and prevent dependent tuning/closure until then.
`SPEC-ECO-006` versus `SPEC-RES-007` also needs the reservation boundary explicit: whether previously
reserved production can finish during supply deficit. No throughput or reservation behavior was silently changed.

**TBR-NET-001 — Network design/service decisions — OPEN.** Before REL-MP-019 can be verified, define
participant authentication/key lifecycle, peer trust/hidden-state exposure, and the authenticated packet
envelope (tag/session/issuer/freshness fields and whether REL-MP-003’s32-byte limit includes them); exact team/FFA disconnect
outcome and remaining-unit disposition; reconnect versus host-election ordering; and rating identity/
persistence/trust. Choose the required relay deployment/availability, data retention, recurring cost and
failure/support contract before acquiring or deploying a service. The general mode approval supplies no
provider purchase, credential, service subscription or deployment authorization. Preserve existing timeout/
security targets. These choices block their dependent network implementation, not the M01/source-map work.

**Verification receipt.** Python M01 narrative63/63, compiled Glass Scar13/13 and overlay1/1 passed. The
initial Glass Scar source test failed1/10 on stale source matching and traced spawn relocations. The active
runtime owner repaired the test while retaining the frozen source snapshot and actual spawn/fairness
invariants; the focused recheck passes10/10. The failure and recheck logs are both retained under
`BuildArtifacts/Evidence/doc-reconciliation-20260904T201127Z/`, with command/exit/source identities. These
are source tests only. No Unreal, packaged, physical-play, sound, performance, or human result is inferred.
Document/registry checks and internal review are recorded in DocumentationAudit.md. No commit or push is
claimed by this task; concurrent production evidence remains separately bounded.


## 2026-09-04 — Hosting deferral and worker-economy assessment

**Owner direction:** “put multi play hosting to the back. that will be in the next game version after
this version is fully completed.” Hosting services, hosted relay deployment and service-spending decisions
are deferred until the next version. REL-MP-013/018/019 and the delivery references now express that
applicability. TBR-NET-001 hosting/provider/cost choices are deferred; current session authentication,
packet format, peer trust and failure/recovery choices remain applicable to shipped direct/LAN modes.
No purchase, service deployment or game implementation occurred. The targeted hosting deferral does not
silently withdraw the prior Conquest/team/FFA scope decision.

**TBR-DOC-004 — refined recommendation, not an approved rule or implementation.** The owner asked which
worker behavior fits the game's concept, strategy and factions. Recommend two simultaneous extraction
positions per standard deposit as a test baseline, not a hard cap of two assigned workers: additional
haulers can use freed positions while others deliver cargo. Useful assigned-worker saturation therefore
depends on harvest time, cargo, travel, delivery and route congestion. Compare two versus three extraction
positions before treating either as balanced. Keep the same extraction rules for all factions; differentiate
their economy through Meridian's networked drop-offs, Kharuun's relocatable rooted Waystones, and Choir's
Dawn-funded coherence upkeep. Give the player stable automatic delivery to an efficient known reachable
drop-off plus explicit assignment override; reroute on failure without hidden-information use or constant
route switching. Exhausted sites stop production immediately but remain visibly exhausted when observed
and remembered only under fog rules. Existing reserved production should finish during supply loss while
new starts wait. These are proposed resolutions of the old alternatives, with no thresholds, worker code,
map data or acceptance changed by this assessment. Delivery and saturation need full-cycle income,
expansion/harassment, route-failure and faction matchup checks, followed by player testing.

## Accepted worker economy resolution

**Owner direction:** “go with your recomment\dationw”, accepting the preceding worker recommendation.
**TBR-DOC-004 — RESOLVED as a design decision.** SPEC-RES-003..007 now govern two simultaneous
extraction positions, unlimited assignments subject to visible queueing, stable efficient known delivery
with explicit override and failure fallback, immediate depletion with persistent observed/remembered
terrain, and completion of previously reserved production during supply loss. SPEC-ECO-002..006 retain
their stable IDs as references to those bodies. REL-ECO-004/005/007/012/013 are aligned. Automatic deposit
retargeting and the 200-tick exhausted-marker alternative are superseded. Existing cargo is delivered
before the depleted assignment becomes idle. Faction infrastructure differences remain binding.

Affected implementation and qualification remain **OPEN**. No runtime change, balance result, packaged
verification, human acceptance or owner gameplay acceptance is claimed. Test extraction versus hauling
occupancy, queue order, full-cycle delivered income, explicit locks/fallback, route failure, retained cargo,
fog memory and production reservations. Compare two versus three positions under expansion, harassment
and faction matchup/player tests. Hosting remains deferred to the next version under the prior decision.

**TBR-ECO-001 — OPEN, inherited numeric consistency.** SPEC-UNIT-001/005/009 author different work/cargo
values (Surveyor 10/10, Tender 9/10, Threadkeeper 9/12), while REL-ECO-003 prescribes universal 10 Matter
over 20 ticks. The accepted shared extraction-position and logistics policy does not authorize silently
flattening faction stats or selecting a new throughput formula. Preserve source values pending a focused
rate/cargo reconciliation; harvesting-rate implementation and balance acceptance remain blocked on that
resolution. The 20-tick cadence is common; neither unequal rates nor universal throughput is claimed
validated by this document decision. TBR-DOC-003 dialogue-mix alternatives remain unresolved.

## 2026-09-04 — M01 editor-rendered surface evidence

For `SPEC-VISD-008`, `SPEC-ART-004` and the M01 representative production brief, the bounded
surface pass is recorded at `BuildArtifacts/Evidence/editor-visual-pass-20260904T235321Z/`.
It corrects missing instanced-material usage, introduces continuous service ceramic, exposes
apron markings and removes distracting basalt contour patterns. Six regenerated meshes retain
two LODs and zero simple collision; three existing source geometry checks passed. The current
PIE frame shows the owned carrier recovered/intact at the archive after an ordinary movement
command through the project's existing visual-preview path. Source and asset identities are
in `surface-pass-manifest.json`; provenance is in the AssetRegister's live editor entry.

This adds source-check and editor-rendered evidence only. No requirement is promoted to
`COMPLETE`, and no packaged journey, audio, sustained performance, physical-input verification
or owner acceptance is claimed. M01 composition and the wider map-delivery obligations remain
open. The owner requested that the editor stay open; live material iteration followed that
direction after the earlier relaunches.

## 2026-09-05 — M01 visual inventory and B1 correction state

`SPEC-ART-004`, `SPEC-VISD-008`, `REL-ART-030`, `SPEC-FOG-001` and `REL-ART-017`
remain **IN PROGRESS** for M01. The current evidence root is
`BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z/` on dirty
`fc05cdf08191649363fb774ec88ad19d96c37a37`. The WorldMapWorkLog records the unchanged
M01 terrain binding, source revisions and retained render classifications. Its
`visual-inventory.json` and `defects.json` distinguish inspected portions from open
construction, material, placement, state and package work. No parent is complete.

Bounded EDT reinspection shows a less machined retaining profile, an unlit unknown
shroud, remembered ground visible through the tint, removal of the v2 tint's false
grid and concealment of unobserved crossing details. The partially revealed ravine
bed and sealed backing were built and reinspected in8831; the basalt surface correction
loaded6484 still needs a matched wide-bank reinspection. Native tests, full object/state
coverage, packaged rendering, performance and human review remain open. Editor scripting failures and shader-warmup/tick mismatch are retained, not
counted as successful matched execution. Only Angelis may accept or assign COMPLETE.

**M01 capability dependencies — OPEN:** the actual Harvest/Reshape core implementation
activates immediately; the required180-tick public commitment/interrupt interval has
no authoritative state to render. Reshape has real1800-tick expiry/fallback logic. The
3138/0495 presentation now projects the authoritative countdown, final10-second warning
and expired state; the real-time0495 EDT clip shows these transitions and subsequent
evacuation. Occupied-cell fallback and all changed terrain cells remain uninspected.
The existing Well keyboard choices have no required
three-card comparison/confirmation presentation. Visual production must not fabricate
those missing states. Current M01 narrative consumers bind sequence/text IDs, while
authored voice delivery, cinematic timing, listening and audiovisual synchronization
remain unverified. Record these gaps through B3/B4 and keep their requirements open.

**TBR-M01-ROSTER-001 — OPEN:** `SPEC-PLAN-001` describes6 Surveyors/2 Lancers, while the
current M01 source and approved spatial brief preserve3 workers/3 line units. This
visual pass preserves the current deployment and does not silently rebalance the
mission. Resolve the authoritative starting-force discrepancy separately.

**B5 package qualification — OPEN:** the registered packager requires a clean detached
linked worktree at the exact pushed canonical main commit. The active source contains
substantial prior and current dirty work. No such integration, push, package execution
or provenance qualification is claimed by editor builds. Prepare the concrete M01
review state and retain the unresolved package boundary without weakening that gate.

**B2–B4 bounded progress — IN PROGRESS:** 6484/3138 reuse registered 3D basalt on
M01 backing and reduce basin-body glare. Retained Preserve and Harvest route clips
show archive approach, Well interaction, withdrawal and results from controlled
ordinary-command fixtures. They do not cover every actor/action. The0495 Reshape clip
is120.04seconds at an exact1280×720 raster;2880 frames decoded, selected warning/expiry
frames inspected. HUD150% resources, selection summary/integrity and result text now
fit in the inspected views. The follow-up shared layout correction removes the old
1.35 geometry cap and adds native endpoint/input assertions; its native run and new
rendered sweep remain pending. No audio, physical-input, performance or package result
is inferred from these movies. Failed startup/capture attempts remain retained.

**TBR-M01-ANCHOR-002 — OPEN:** `SPEC-MSN-001` and the current mission/map source bind
extraction to6,17; legacy `REL-CAM-006.AUTH` still names42,18. This visual pass preserves
the controlling6,17 mission geometry and records the requirement conflict for Angelis.

**TBR-M01-RESHAPE-003 — OPEN:** current SimCore Reshape affects passability only in
the3×3 around the Well (31–33,31–33). All nine M01 source cells are already open.
Consequently this mission has no actual Reshape terrain opening, closing or occupied
blocked-cell fallback to render. Preserve the current causeway and mission geometry;
resolving the missing tactical terrain effect requires an authoritative map/rules
decision. The timed Well/HUD presentation can be corrected independently. The first
0495 expiry render retained active purple core/orbit motion; its M01-only inactive
presentation correction is under native build and needs reinspection.


M01 follow-up evidence (2026-09-05): the5140 region movie covers all four ordinary
camera edges and HUD80/100/120/150 at1280×720. Coverage exposed actual defects V022
(wrong-branch withdrawal narration), V023(exposed boundary/constant-width rock row),
and V024(80% objective overflow); their source corrections await matched reinspection.
The3019 actual-expiry clip and fresh native M01WellExpiry check verify the inactive
Well presentation within their evidence classes. The fresh23-check report has22PASS;
its one stale landmark expectation is corrected and passes a separate focused native
run. Save guards pass. This does not advance owner acceptance or any requirement to
COMPLETE. All-instance, all-action,1080p, packaged and performance gates remain open.


**2026-09-05 M01 follow-up — IN PROGRESS:** native focused contact5 and failure/pointer4 checks passed in isolated environments with unchanged real player saves. Module4065 ordinary reveal/freeze at tick364 exposed all28 registered landmark instances; E1/E2 retain inspected static views. V030 obstructed causeway manifold and V031 Well ornament-shadow spots remain pending correction reinspection. M01 Surveyor four-part derivatives generated under existing original provenance; articulated motion, other deployed walkers, Bulwark deployed form and supported work/production feedback remain open. No packaged, full interaction/performance or owner acceptance is established. Existing TBR anchor/roster/Reshape decisions remain open.


**2026-09-05 M01 B2/B4 reinspection — IN PROGRESS:** module2947 and native7PASS support the M01 Surveyor derivative rig, contact and geometry source checks. F2 closesV030/V031 in the matched dormant/static views. Ordinary gather F1 retains sharp-reversal uncertaintyV036; no universal locomotion acceptance is assigned. Actual1080p F3 exposes briefing/title/pause defectsV035/V037/V038; M01 source corrections require current-editor and ordinary-input reinspection. Four capture qualification receipts retain hashes, observed frames and temporal module association. All applicable PKG, performance, audio, human and owner gates remain open; no requirement is COMPLETE.


**2026-09-05 M01 interface and motion follow-up — IN PROGRESS:** hot3712 native1280×720/HUD150 high-contrast, reduced-motion and reduced-flashing views correct the sampled briefing and pause button overlaps (V035/V038). Bounded CUA keyboard U/Return/P observations are retained separately from editor setup. Mouse activation remains unresolved as V039; no production pointer change is justified yet. Surveyor native runs073434Z,075404Z and081339Z retain failed reversal checks; the fourth correction is under focused test and has no pass claim here. The Bulwark baseline confirms V032 and its M01-only derivative sources are prepared; generation and runtime state inspection remain open. No requirement is COMPLETE. Packaged, performance, audio, broader accessibility, human and owner acceptance remain outstanding.

### 2026-09-05 — M01 Bulwark state evidence and diagonal Surveyor regression

**Author and owner:** Angelis Pseftis. Applicable `SPEC-ART-004`, `SPEC-VISD-008`, `REL-ART-009..014`, `REL-ART-031/033`, `SPEC-UI-005` and accessibility records remain **IN PROGRESS**; this entry grants no COMPLETE or owner acceptance.

The retained M01 visual evidence root is `BuildArtifacts/Evidence/m01-visual-completion-20260905T024342Z`. Native084912Z passes three exact Bulwark derivative/pooling/deployment tests with zero warnings/errors and unchanged real player saves. Loaded8594 I1 sampled frames1–8 show attached folding wings, distinct packed/deployed silhouettes and consistent facing. This is SRC/GEN/native/EDT coverage; undercarriage motion and other actor states remain open.

Native084249Z Surveyor rig PASS is bounded by a later actual-route failure: I1 logs three emergency foot replants on diagonal gathering/delivery. V036 remains OPEN and the corrected initial-departure fixture awaits repeated native/render evidence. The earlier failed runs remain retained. I1 also substantiates V040 generic M01 producer labels. Return/Tab/Q works in that controlled editor journey; pointer clicks still resolve to unchanged cursor coordinates (V039). Neither is packaged or physical-player acceptance.

The inventory is being expanded in place to distinguish uninspected states from missing authoritative capabilities and the existing anchor, roster and Reshape decisions. Source checks and sampled corrections cannot close the full M01 inventory, packaged fog/persistence/interaction/performance gates, audiovisual bindings or Angelis's final review.

**2026-09-05 M01 camera and native follow-up — IN PROGRESS:** J2/K1 supported-camera attribution is withdrawn after actual rotation readback exposed unintended roll. K4 explicitly asserts the normal rotation, and all ten unobstructed6200-corner/edge and1400/6200-arrival samples show continuous current exterior backing. V042 bank material/depth refinement is source prepared and passes the native world-kit bounds/material/fog checks; rendered reinspection remains pending. Native101237Z has seven passes and five Surveyor assertions, with unchanged real player saves. The derived fixture's inherited initial position, duplicate target-edge sample and omitted0.5841second segment are corrected separately; V036 remains open pending test and actual motion. The build gate actually refused the dirty release checkout; no integrated package, performance, physical-input, audio or owner acceptance is claimed. Requirements remain IN PROGRESS.

## 2026-09-05 — Harvesting FSM guidance intake

**Author and owner:** Angelis Pseftis.
**Owner source:** “Here is some information to think about when this code gets worked on,” followed
by a proposed Worker / ResourceNode / TownHall harvesting FSM. The key constraints are “only ONE
worker can harvest at a single node” and “the physical travel distance and pathing must dictate the
income rate.” This entry retains that proposal for future implementation. It does not amend the
requirements master, adopt the example balance values, or claim implementation or gameplay acceptance.
StarCraft 2 is the owner's behavioral reference; exact equivalence has not been established.

**Proposed state contract from the owner input:**

| State | Requested behavior |
|---|---|
| Idle | Wait for a player command. |
| MovingToResource | Physically path to the assigned node. Workers assigned to that node must not block each other in transit. Recheck a depleted or removed target before extraction. |
| Harvesting | On arrival, wait immediately beside an occupied node in a visible micro-queue. Only the worker holding the node's exclusive extraction ownership may advance its harvest duration. On completion, remove the extracted amount from the node, attach a visible carried-resource flag, and release the node for the next waiting worker. |
| ReturningHome | Physically path to the nearest valid TownHall/drop-off while retaining the node assignment and extracted cargo. No resource income is credited during travel. |
| Delivering | On actual arrival, credit the owning player's resource pool once, clear cargo and its visual flag, and return to MovingToResource for the exact same node without another player command. |

The owner permits either idle or nearest-node fallback on depletion. The idle option is compatible
with SPEC-RES-006: deliver existing cargo first, then enter the idle registry and alert; a new player
order is needed to select another node. Empty workers whose target depletes while travelling or waiting
stop that assignment. No hidden node discovery is authorized by this input.

**TBR-ECO-002 — OPEN, reconciliation before changing the affected harvesting rules.**
The input refines the existing loop but also proposes changes to previously accepted behavior:

* **Extraction occupancy:** one exclusive extractor conflicts with the two-position baseline in
  SPEC-RES-003 and REL-ECO-004. A future decision must select one or retain two, then align both master
  records and the economy skill's baseline. Queue capacity and assigned hauler count are separate
  from active extraction capacity. Changing this limit changes saturation and delivered income;
  the earlier two-versus-three comparison does not qualify a one-position design.
* **Transit behavior:** the non-blocking intent aligns with SPEC-MOV-008's non-imprisonment rule.
  Literal worker phasing would need an explicit bounded exception to its clearance rule and
  SPEC-MOV-009's sequential chokepoint behavior. Preserve terrain passability and unit targetability;
  do not silently disable authoritative collision for every worker or unit.
* **Drop-off choice:** nearest TownHall differs from SPEC-RES-005 / REL-ECO-007's valid explicit
  assignment, stable destination and lowest predicted round-trip travel time. For Echoes, the
  candidate interpretation is nearest known, reachable, operational, faction-valid friendly drop-off
  by navigable travel cost, with deterministic ties. Decide whether explicit assignments still take
  precedence; keep faction infrastructure semantics instead of restricting delivery to Command Cores.
* **Timing and amount:** 2.0 seconds and 5 units are explicitly examples, not adopted constants.
  TBR-ECO-001 remains open for the inherited faction work/cargo versus universal-throughput conflict.
  The existing 20-tick cadence remains unchanged. Resolve completion-time extraction versus
  REL-ECO-003.AUTH's per-tick accumulation, including interruption and final partial loads, before
  changing the accounting model. Use authoritative simulation time for work duration and actual
  movement for both legs of the cycle; no periodic passive-income substitute is supported.

**Implementation considerations derived from the proposed loop, pending reconciliation:**
Use the existing Unreal/C++ architecture: Worker maps to the worker entity, ResourceNode to the
Matter deposit, and TownHall to the faction-valid delivery role. EchoesSimCore owns FSM state,
arrival validation, exclusive ownership, queue ordering, cargo and balance transactions; Unreal
presentation consumes those fields. The object-oriented entity roles must not introduce a second
gameplay authority in actor Tick functions or independent wall-clock timers.

Represent waiting as a Harvesting substate, distinguishable from active extraction. Order waiters
by arrival with a stable entity-ID tie-break for simultaneous arrivals, avoiding starvation. The
exclusive-ownership invariant holds throughout each simulation transition, including same-tick
handoffs; “any given millisecond” does not require changing the project's simulation frequency.
Release ownership and remove stale queue entries on cancellation, reassignment, death or node
removal. Preserve only legitimately extracted cargo across interruption and drop-off failure;
worker death follows REL-ECO-008. Revalidate the destination on arrival and retain cargo with the
existing failure feedback when no valid delivery route exists.

At extraction completion, take no more than the configured amount, remaining node stock and free
cargo capacity. Credit that actual carried amount at delivery, not an unconditional HarvestAmount;
a final 3-unit load must deliver 3, even if the configured example load is 5. No extraction while
queued, negative stock, duplicated delivery or cargo loss from a depleted assignment is valid.
Save/load and replay must retain or deterministically reconstruct assignment, progress, extraction
ownership and queue order without duplicate harvest or delivery.

**Read-only source findings and future verification boundary:**
The current Simulation.cpp ProcessGather / ProcessDeliver implementation already uses physical
MoveTowards pathing and retains an assignedResourceNode. State is implicit in orders, range,
cargo and harvestTicks rather than an explicit five-state FSM. In-range extraction arbitration
currently admits two workers by entity order; it is not the proposed exclusive node lock and
arrival-ordered micro-queue. Presentation receives cargo amounts, but the scan did not establish
a dedicated visible carried-resource marker. The source also still automatically retargets depleted
assignments in gather/delivery recovery, contrary to the accepted SPEC-RES-006 idle policy. This is
a source-inspection finding, not an executed regression result; retain it for the next economy fix.

Qualification should cover simultaneous arrivals, queue fairness and cancellation, partial final
loads, depletion during approach/wait/work/return, worker and drop-off loss, blocked terrain,
same-node continuity, actual cargo marker transitions and save/replay continuity. Compare otherwise
identical short and long navigable delivery routes and an impassable route: Matter credits must
follow real arrivals, and blocked travel must produce no delivered income. Use source/native tests
for accounting and determinism, followed by the required packaged interaction and rendered evidence
for movement, queue and cargo readability. These are future checks, not pass claims.

Affected SPEC-RES-003..006, REL-ECO-003..008 and SPEC-MOV-008..009 retain their existing lifecycle
records; this intake grants no IMPLEMENTED, AGENT VERIFIED or COMPLETE status. No runtime, balance,
asset or engine changes were made by this task. Source identity, dirty-state capture and document
checks are retained in BuildArtifacts/Evidence/harvesting-guidance-20260905T105254Z/.


### 2026-09-05 — M01 ordinary-route gait reinspection

**Author and owner:** Angelis Pseftis. SPEC-ART-004, REL-ART-009 and SPEC-MOV-010 remain **IN PROGRESS**.

Native run 104801Z passed nine focused checks; final native run 105653Z passed four after the M01 authored 720°/s
facing correction. The latter verifies exact angular progress, ninety authoritative seconds of repeated
gathering/delivery, planted support within 1 cm and no emergency/discontinuity reset. Real player save
guards passed. Loaded 3467 M1 retains 110.04 seconds of 1280×720 video and112 seconds of presentation telemetry:
zero emergency replants, zero discontinuity resets and no unexpected pose resets outside accessibility
transitions. Twelve sampled views were reviewed across normal, reduced and restored motion. This
corrects V036 only for the retained route and V043 within source/native evidence; weak lower-leg
contrast remains V044. The build, native reports, module identity and movie qualification are linked
from [WorldMapWorkLog.md](WorldMapWorkLog.md) and retained under BuildArtifacts/Evidence/m01-continuation-20260905T104625Z/.

This evidence does not qualify every route/action, full-map composition, audiovisual delivery,
packaged performance, physical-player interaction or owner acceptance. Existing roster, anchor,
Reshape and harvesting decisions remain unchanged. No requirement is marked COMPLETE.


## 2026-09-05 — RTS regression implementation and harvesting instruction adoption

**Author and owner:** Angelis Pseftis.
**Status:** IN PROGRESS; native and build gates passed, isolated Unreal automation underway. No packaged, rendered,
performance, balance or owner acceptance is established by this entry.

The owner authorized the architecture plan with “Proceed” and repeated the strict single-extractor
FSM in the active M01 task. That explicit instruction supersedes the older two-position baseline.
SPEC-RES-003, REL-ECO-004, REL-ECO-003.AUTH and the bounded worker-clearance exception in
SPEC-MOV-008.AUTH are reconciled in the master. The earlier intake above remains historical;
TBR-ECO-002 occupancy, transit and completion-time accounting are resolved by this instruction.
The example 2 seconds/5 units are not adopted numerical constants. TBR-ECO-001 remains open.

The implementation uses five authoritative HarvestState values, stable arrival tickets, one held
position, physical queue parking and pathing, completion-time cargo extraction, actual-cargo deposit,
same-node return and idle after the final depleted-node load. An explicit Return Cargo/Deliver
command retains the existing assignment for resumption; Stop, Move and a new Gather replace it.
Nearest fallback uses live operational friendly depots and the player's known navigable grid with
stable ties; exact weighted round-trip optimization and persistent depot-lock UI remain unqualified.
No failed route may credit income. The configured 10-load requires20 ticks; other existing capacities
use their existing work rates to derive completion duration without changing those source values.

Snapshot schema26 extends the prior layout with work state, assignment, arrival ticket, held slot,
queued orders, fractional construction progress, ballistic-mode configuration and projectiles in
flight. Legacy schemas20–25 cannot recover fields they never wrote: known Gather restarts its work
phase; a returning legacy load with unknown source delivers then idles. This is an explicit migration
limit, not reconstructed evidence of the former assignment. Historical replay checksums must not
be treated as current rules equivalence.

Save tests now use a launcher-created temporary root and a macOS deny policy for the normal project
save root and real home. The launcher does not inspect those protected paths. It validates deny
clauses and exercises access denial only on synthetic fixtures. The game module checks dedicated
routing before GameInstance creation; scoped save fixtures refuse ordinary editor launches.
`-UserDir` does not redirect every Unreal user path; whole-home denial remains enforced. Attempt3
reached engine initialization but blocked before automation in the Home Screen installation browser;
an early sandbox-owned startup setting now disables that UI for the next attempt. Campaign slot names encode spaces and
underscores distinctly; ambiguous legacy underscore filenames remain untouched and require explicit
migration rather than guessed ownership.

Contact scope adds a common pre-fire terrain gate and projectile flight/checkpoint corrections.
Explored-terrain movement admission now consumes remembered terrain instead of live hidden changes.
Terrain-occluded fog, transparent void classification and elevation mechanics remain the joint
Core/World/AI gate specified by owner ruling25; no arbitrary height threshold or damage bonus is
introduced here. Existing event-driven mission models remain the sole reducers; campaign regression
coverage is reused and the slot collision is fixed without creating another progression authority.

Evidence is retained in `BuildArtifacts/Evidence/rts-regression-implementation-20260905T112003Z/`.
The initial optimized suite reached94/98; the next reached96/98. The final native gate passed100/100
in optimized, debug and ASan/UBSan configurations. The controlled editor build passed; ten synthetic
launcher tests passed. These are separate from the Unreal automation result, which remains pending
after retained startup failures. See the architecture document and per-attempt evidence receipts.
No unrelated editor was closed by this task; unrelated map/lighting work remains preserved.

## 2026-09-05 — Owner-directed canon expansion: story causality, backstories, places, and visual description

**Author and owner:** Angelis Pseftis
**Evidence boundary:** documentation/source inspection on `main` at `b7adbb4` with pre-existing dirty paths preserved.
This entry records creative-canon authoring and its scope. No runtime, package, rendered, physical-play, or
human-acceptance result is claimed. `Scripts/check_agent_docs.py` passed after the edit (structural/link check only).

**Owner direction (2026-09-05):** polish and complete the storyline so lore, backstories, heroes/characters,
buildings, environments, maps, world, missions, and campaign align and the reader understands what caused what;
describe everything visual precisely enough that later production can build from it.

**What changed.** `Docs/Archive/DevelopmentBible.md` gained an appended part, *Expanded canon — world, history,
people, places, and the fifteen operations*, edited in place (frontmatter `updated` 2026-09-05). It adds: a
two-layer rule (authorial truth in design documents; only `SPEC-MSN` witnessed facts in player text); the sky,
Dawnshard, Well, and leakage physics; the five eras as one causal chain (Ledger Peace Harvests closed futures →
struck census entries and curated Kharuun memory → the Choir as the erased branches → Rhyse's single-future
program); faction cultures and construction languages; production descriptions for all twelve units and twelve
structures across three factions, including the previously unciteable Hollow Choir roster; Well state visuals;
institutions (ledger, doctrine, public interfaces, quarantine posture, accord/conduit); backstories, motivations,
arcs, appearance, and knowledge-by-mission for Mara, Talar, Oruun, Neme, Rhyse, and the Annunciator; fifteen place
descriptions plus the three skirmish maps; the doctrine-echo naming table (Ash/Held/Folded) for branch variants; the
campaign told as one story with the reason for every change of commander; and the four endings' conduits, light,
and sound. `Docs/MapConcepts.md` gained a pointer from the story-to-place trace to those sections.

**Decisions adopted under the 2026-09-05 direction (recorded for the owner's review packet):**

| Item | Decision | Source of prior status |
|---|---|---|
| Future Well choice colours (NarrativeCoherenceReview V3, open item 1) | Adopted as canon design: Harvest broken-sun amber, Preserve cyan-held, Reshape magenta-fracture, Dormant unlit charcoal; consistent with the ArtDirection master palette. | Awaited owner adoption |
| Hollow Choir roster in the Bible (NarrativeCoherenceReview open item 3) | Threadkeeper, Intervalist, Lacuna Warden, Afterimage, Concordance, Interval Loom, Chorus Loom, Phase Anchor authored into the Bible with the data-registered names and `SPEC-UNIT-009..012` / `SPEC-BLD-017` roles. | Existed only in data |
| New canon names | Solar Fall (highlands) and Solar Fall Dais; Understone (the birthing cavern under the Glass Scar — never shown or located); Line of Parity and Sector 9 (already registered world-source display names); Authority Exchange (M09), Lume Well court (M10), Census Forecourt (M11), Demonstrator Spine at Reserve Gate (M12); Ash/Held/Folded doctrine names; the Cisterns for Life Support. Ration/Census/Reserve Gate names were already owner-adopted 2026-09-02. | New |
| Character backstory events | Mara's Transit-block span failure; Talar's grandmother's struck register entry; Oruun's seven accounts of the Understone evacuation; Rhyse's Reserve Gate famine-winter Harvest; Neme's naming at the Confluence. These are authorial-layer facts; no mission asserts them. | New |

**Requirement effect.** `SPEC-CAM-041.CONNECTIONS` and `SPEC-CAM-042.TRACE` now have authored source support for
region/site, story stake, character/backstory link, and preceding/next relationship for all fifteen operations;
their states remain **OPEN** because in-game delivery (`.MAP`, `.DELIVERY`, `.RESULT_PATHS`), human experience
(`.EXPERIENCE`), and owner review (`.OWNER`) are unqualified. `SPEC-CANON-001..014`, `SPEC-CAN-001..002`,
`SPEC-MSN-001..015`, `SPEC-PLAN-*`, and `SPEC-END-*` bodies are unchanged; nothing here alters an objective,
coordinate, threshold, or consequence. `Content/Narrative/Source/campaign_canon_continuity.json` remains a
structured projection and was not edited; its next regeneration/validation should be checked against the expanded
part. Mission narrative JSON was not edited; no line was re-authored.

**OWNER-QUESTION (non-blocking, batch with the next review):**
1. Oruun's pronoun: `SPEC-MSN-007` canonical facts say "Oruun himself"; the Character & Voice Identity Bible says
   "who they are". The expanded canon avoids pronouns for Oruun. Which form should all text use?
2. NarrativeCoherenceReview open item 4 (Bible command set names `repair` and `rally`, absent from `CommandType`)
   remains open; the expanded canon describes Surveyor repair only as "when authorized". Redesign or rejustify?

## 2026-09-05 — Adaptive execution policy and P0 native baseline repair

**Author and owner:** Angelis Pseftis
**Source identity:** `release/world-map-concept-pass` at `b7adbb4b00add12980812decdb72a44ab4a8e544`,
with pre-existing staged and unstaged work preserved. The receipt binds the tested dirty inputs by hash.

The owner directed automatic task-appropriate model/effort selection and complete prompts for delegated
work. The shared contract now routes every package through the selection and handoff procedure in
[GameDevelopmentWorkflow.md](Prompts/GameDevelopmentWorkflow.md#select-model-effort-and-work-ownership).
Local model/role configuration was checked; no global setting or active parent model was changed.
For this P0 slice, a `gpt-5.6-sol`/`high` worker owned the three native test files, the integration owner
owned `Simulation.cpp`, and an `expert_reviewer` (`gpt-5.6-sol`/`high`) reviewed the scoped repair.
These routes are recorded choices, not evidence that one model is universally optimal.

**Implemented and native-verified scope:** corrected the schema 27-to-26-to-20 fixture chain, with
bounds-checked traversal and explicit legacy-state limits. Corrected snapshot validation so valid
Harvest countdowns and contested capture at zero progress can restore while malformed lifecycle states
remain rejected. Corrected abandoned capture to retain its claimant while progress decays one point
per tick; it clears at zero, resumes for the same claimant and restarts for a different claimant.
No snapshot schema or serialization layout changed in this repair.

The first run compiled and passed 95/100 optimized tests, exposing the decay defect and remaining
outdated timing/checksum fixtures. After repair, `bash Scripts/test_sim.sh` passed **100/100 optimized,
100/100 debug and 100/100 address/undefined-behavior sanitizer tests** on 2026-09-05,
15:02:35–15:03:25 UTC, exit 0. Inputs remained unchanged during that run. Coverage includes schema
migration, capture/Harvest boundaries, cancellation, abandonment and reacquisition, save/replay/checksum
equivalence, and malformed lifecycle fields. The scoped review reported no material defect.

**Requirement effect:** this adds `SRC` evidence for the bounded lifecycle and persistence behavior in
`SPEC-WEL-003`, `SPEC-WEL-004`, `SPEC-SAV-003`, `REL-SAV-005` and `REL-SAV-010`. These parent contracts
remain **IN PROGRESS**; this entry does not establish their broader UI, visibility, compatibility or
player-journey acceptance. P0 remains in progress pending current integrated editor/Unreal and generated
source checks. No current package, physical play, rendered/audio qualification or human acceptance was
produced by this slice, and no commit or push was performed.

**Open defect:** `REL-WEL-010` / `SPEC-WELLP-003` still lack Reshape's required 180-tick public telegraph.
`CompleteFutureWellCapture` currently charges and activates Reshape at capture completion. This repair's
test setup accounts for capture time but explicitly leaves the telegraph unqualified. Complete the
protocol state, cancellation/expiry/save/replay cases and public feedback under P0/P3; the separate M01
authored-route geometry decision remains open. Do not promote immediate activation into an accepted rule.

**Retained evidence:**
`/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/BuildArtifacts/Evidence/adaptive-routing-p0-20260905T144613Z/`.
`session.json` and `dispatch.json` identify ownership and routing; `p0-scoped.patch` separates this repair
from the pre-existing native changes; `native-attempt-1*` preserves the failure; `native-attempt-2-result.json`
and its log bind the successful run to input hashes. These local artifacts are not a backed-up release.


## 2026-09-05 — P0 integrated continuation and Reshape warning repair

**Author and owner:** Angelis Pseftis
**Execution state:** P0 automated integration gate passed locally; requirement and owner acceptance remain open.
**Source:** clean `main` at `15008d55378323bb1731193213d70ab586da49c0` at entry, followed by
scoped dirty changes. The external evidence root is
`/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/BuildArtifacts/Evidence/p0-integrated-20260905T155600Z/`.
`session.json`, build input manifests, logs and scoped diffs bind individual runs; a commit alone does
not identify subsequent edits. Root owns integration; read-only evidence/review specialists and a
`gpt-5.6-sol`/`high` worker with five disjoint sandbox paths supported this package.

The earlier native repair's seven recorded inputs matched the starting checkout exactly. Current
generated-source checks passed. A new build exposed the committed project descriptor's UTF-16 encoding,
which UnrealBuildTool rejected; conversion to UTF-8 preserved every JSON value. The test wrapper now
requires the two existing camera/Harvest-collapse tests it previously omitted (92 registered tests).

`REL-WEL-010`, `SPEC-WELLP-003`, `SPEC-WEL-003`, `SPEC-SAV-003` and `REL-SAV-005/010` now have a
bounded Reshape warning implementation: capture commits once at 120 Dawn; a separate pending state
broadcasts 180 ticks without opening terrain or advancing mission activation; manifestation lasts
1,800 ticks. Affordability is checked on authoritative command execution and again at capture completion.
Invalid mixed lifecycle snapshots fail closed; schema 27's layout and prior valid Harvest/Reshape states
remain supported. Offline player views receive only the public warning fields, and AI avoids already
committed Wells. Content requesting a different pinned warning duration is rejected. Harvest presentation
now follows the core's emitted countdown state instead of prematurely displaying collapse.

**Policy interpretation:** `SPEC-WEL-004` permits transfer until protocol commitment; Harvest has the
explicit warning-period interruption exception in `REL-WEL-005`. Reshape therefore retains contest/decay
before commitment and has no post-commit cancellation/refund. An interruption/refund extension needs an
owner decision. This does not resolve the separate M01 authored-route geometry decision.

Native optimized/debug/ASan+UBSan passed 100/100 each after the repair and again after review corrections
(`native-reshape-1.log`, `native-reshape-2.log`). The first post-repair editor build succeeded. The first
full isolated Unreal launch stopped before tests because its external DDC path exceeded Unreal's
119-character limit; denial probe and cleanup passed. The launcher and bootstrap now use the shorter
fixed project route `BuildArtifacts/TestIO/EAT.<run>/DDC`; 15 focused Python tests passed, including
static symlink/home/player-save refusal, real sandbox denial and overlong-path refusal/cleanup. Concurrent
same-user mutation of the checkout during launcher setup remains outside that local routing boundary.

The next editor build succeeded (`editor-build-3.log`, `candidate-3-inputs.json`). The first complete
isolated run executed all 92 tests: **80 succeeded and 12 failed** (`unreal-2/index.json`). The save
sandbox denial probe, policy checks, empty-save-root check and cleanup all passed. Failures remain
retained. Several native-schema assertions still expected 26, and the Unreal migration helper omitted
the schema-27 lifecycle count from its measured tail and did not validate a separate 27-to-26 step.
The test repair now bounds and validates each conversion through the real loader. Other fixture repairs
wait for actual Harvest payout before troop purchases and assert the emitted Harvest/Reshape visual state.

The next compiled candidate (`editor-build-4.log`, `candidate-4-inputs.json`) improved the full
isolated result to **85/92** (`unreal-3/index.json`); save isolation, denial probe and cleanup passed.
Seven failures remain in that retained run: M05, two M12 journeys, two M15 paths, complete skirmish,
and audio concurrency. M05 diagnostics identify loss of the Kharuun spine; the fixture now sends existing
combat units to Guard it without changing the fixed T300–T900 branch window. M12 fixtures no longer
abort solely because a replaceable escort dies; named protected losses still fail. The skirmish fixture
now gathers and produces reinforcements through ordinary commands. Audio assertions count the current
world policy's voices rather than unrelated voices using the same sound. These changes await Unreal validation.

M15 exposed a gameplay defect: neutral protected witnesses were eligible combat targets under the
core's default free-for-all hostility. Schema 28 now persists a validated symmetric four-player hostility
matrix. M15 authors `02 0d 02 02`; other operations retain default FFA. Combat, AI threat selection and
Well contests use this explicit relation. Command ownership and fog visibility remain separate. Legacy
M15 saves receive the authored fallback and remove current incompatible attack/Well orders and projectiles;
queued orders are rejected on execution. Current saves with a valid but wrong operation matrix are refused
without live mutation. Schema-27 replay verification retains its historical typed-checksum payload shape.
This does not close the broader team, shared-vision or allied-Guard contracts.

Native validation first caught a test-only optional-value compile error, then an incorrect test oracle
that confused snapshot FNV integrity with the typed state checksum. Both failed attempts are retained.
The historical schema-27 core at `15008d55` was compiled separately with the exact replay setup to obtain
checksum `7947105480651690908`; its source, fixture and result are retained in `legacy27-oracle/`.
The corrected 101-test suite passed optimized, debug and sanitizer configurations (`native-hostility-3.log`).
Additional command-ownership, fog-isolation and immediate legacy attack/projectile sanitation negatives
also passed in all three configurations (`native-hostility-4.log`).

The schema-28 editor build succeeded (`editor-build-5.log`, `candidate-5-inputs.json`). Its full isolated
run passed **86/92** (`unreal-4/index.json`), including audio policy isolation and M15 matrix migration
checks. Save protections, denial probe and cleanup passed. M15 neutral witnesses survive; Neme still
suffers direct enemy focus fire while his escorts trail behind. M12 loses Oruun and his two defenders
while the verifier and its two defenders remain unharmed. M05's accepted Guard orders do not intercept
the attackers before spine loss. The skirmish fixture incorrectly compared PlayerView's redacted resource
presence sentinel with the authoritative deposit amount. Candidate fixture repairs use a fair-information
resource selection with a separate test oracle, concentrate M12 escorts, scout/intercept M05 pressure,
and clear M15's approach using ordinary AttackMove commands. These tactical repairs are not yet validated.

Two further editor builds succeeded (`editor-build-8.log`, `editor-build-7.log`). Both full isolated
reports passed **88/92** (`unreal-7/index.json`, `unreal-6/index.json`) with save protections and cleanup
passing. M05 now completes its unchanged T300–T900 defense after ordinary scout/interception orders.
The complete skirmish reaches its real victory, results, save/recovery and restart assertions using
ordinary gathering and reinforcements. M12 and M15 remain open. Concurrent worker staging, corrected
escort regrouping and withdrawal allow M12 to reach protocol admission and preserve Oruun, but the
verifier left at the old readback is lost. M15's confined Move/Hold defense avoids pursuit into the enemy
base and reaches the final hold; Neme remains vulnerable as the forwardmost witness. Plan 25 completes,
while plans 7 and 17 retain protected-witness failures. Current corrections withdraw both M12 witnesses
and use a lawful rear holding position inside M15's unchanged accord radius. No combat stat, AI policy,
mission duration, loss predicate or acceptance criterion is relaxed.

Further retained runs (`unreal-7/index.json`, `unreal-8/index.json`) remain at **88/92**. New diagnostics
identify M12 worker loss at 253–258 of the unchanged 300 capture ticks; a second ordinary worker now
supports continuity without stacking capture speed. The witness withdrawals preserve both scouts through
the initial capture attempt. M15's rear stand preserves Neme, but the exact research feedback exposes
an adapter defect: research schedules at the current offline tick while movement/Guard schedules at the
next tick, causing valid increasing command sequences to be rejected across execution ticks. Research
now uses the same next-tick offline schedule; network scheduling remains three ticks. A focused mixed
movement/research regression and full qualification are pending. A test-only private-helper compile
failure is retained in `editor-build-9.log`; `editor-build-10.log` succeeded after using observable capture
progress instead. No core ordering check was weakened.

The command scheduling build succeeded (`editor-build-11.log`, `candidate-11-inputs.json`). The full
isolated run improved to **90/92** (`unreal-9/index.json`), with save protections and cleanup passing.
Both M15 playthrough tests now complete, and the focused same-frame Move/Research regression verifies
consecutive sequences, next-tick Applied receipts, actual movement, exact research cost/progress and
replay equality. FreshJourney now completes M12 and reaches M13, where unescorted Oruun is lost while
the Crownfall link is 105/120 complete. The focused M12 scenario still loses its two capturers before
binding. The ordinary reserve worker is being included in the unchanged fixed-rate capture, and the
M13 journey is receiving guarded witness movement. These last fixture corrections remain unverified.

The next full runs reached **91/92** (`unreal-11/index.json`, `unreal-12/index.json`) after current
successful editor builds. The focused M12 test passes its unchanged capture, activation, stability,
recovery and completion checks. The first FreshJourney route completes M12–M14; M13 required a fixture correction to
recognize actual completion within the authored three-tile observation radius after automatic pause.
Its current first failure is M15's older unescorted Neme route; later branch journeys have not yet
run past that point. The already qualified M15 positioning/defense
helpers are being shared with FreshJourney to remove the separate stale tactic implementation.

The shared M15 tactics compiled successfully (`editor-build-15.log`, `candidate-15-inputs.json`,
including the new header and its retained patch). Independent read-only review found no material
semantic defect. The full protected `unreal-13` run remains **91/92**: FreshJourney now completes its
first M01–M15 route, reset/restore and conflicting M15 replay checks, then reaches the second route's
M12 (plan 17, founding Harvest / Lume Reshape). That route loses all capture workers and combat escorts
before binding (tick 1031, capture progress 43). Save denial, protected-path policy, empty scoped save
root and cleanup all pass. Later branch completion remains unverified.

The next two screen candidates built successfully but retained **91/92** (`unreal-14`, `unreal-15`).
The first withdrew witnesses before protocol admission, so the reducer correctly refused the command;
the second preserved admission but lost Oruun at tick 624 while waiting for clearance. The correction
removes that extra pre-capture wait: accept the recorded protocol, immediately withdraw witnesses, and
maintain a bounded visible-threat screen during central Reshape capture. Preserve/Harvest retain their
previous worker-escort path. These failures are retained and are not runtime-rule relaxations.

The concurrent screen advanced capture to 256/300 with all three workers still at full health
(`unreal-16`), but the fixture incorrectly aborted when its last replaceable escort died. Removing that
extra loss predicate allowed the complete second route, including M12 and M15, to pass its checks in
`unreal-17` after successful `editor-build-19`. FreshJourney then reached the third route (founding
Reshape) and failed M05 at tick 307: its protected spine was destroyed. Read-only diagnosis found that
Fresh M05 never used the standalone M05 Guard/visible-contact interception routine. That qualified
routine is being shared between the two fixtures; no mission timing or loss criterion changes.

The shared M05 defense integration built successfully (`editor-build-20.log`, 24.57 seconds) and the
full protected run passed **92/92, zero failed, zero skipped** (`unreal-18/index.json`). FreshJourney
passes all four defined fresh routes, the conflicting-ending replay, and reset/restore checks. Both
new shared test headers are included in `candidate-20-inputs.json` and `candidate-20.patch`. All 356
candidate inputs matched at qualification; all seven native inputs still match the **101/101** optimized,
debug and ASan+UBSan receipt in `native-hostility-4.log`. The sandbox denial probe, protected-path policy,
empty scoped save directory and cleanup all pass. `p0-qualification.json` records this final comparison.
Generated-source checks remain applicable: registered content and generated packs are unchanged.

**P0 evidence gate:** the automated integrated baseline is now qualified on this dirty local source.
No requirement is marked COMPLETE and no owner acceptance is inferred. The latest owner instruction
limits this continuation to P0; no P1/P2 implementation is started. Packaged execution, physical input,
complete rendered/listening review and human acceptance remain separate unperformed gates. Network
transport of public warnings, structured resource-failure receipts, M01 geometry and later player-journey
gates remain open. Changes and evidence are retained locally; no commit, push or release was performed.

## 2026-09-05 — P1 shell and persistence implementation

**Author and owner:** Angelis Pseftis
**State:** IN PROGRESS. Owner authorized proceeding to P1 after the P0 automated gate passed.
**Evidence:** `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/BuildArtifacts/Evidence/p1-shell-20260905/`.
Root owns controller, flow state, subsystem integration and evidence. Disjoint workers own the new UMG
widget and profile files. P0 changes and retained evidence remain intact on `15008d55` plus dirty inputs.

The current increment binds `REL-UI-001/008/010/013/014`, `REL-SAV-003/004/009`, and `REL-FTU-003/005`:
a single base-screen state and overlay return stack, native UMG shell screens and modal controls,
transactional `Profile.sav`, three runtime journey slots, and slot-scoped checkpoints/recovery.
Slot 1 deliberately retains the existing campaign path and checkpoint names in place; Slots 2/3 use
separate named ledgers and checkpoint directories. This preserves existing saves without automatic
copying or deletion. Tutorial opt-out is confirmed and stored separately from verified mastery; UI
acknowledgement never grants a tutorial lesson. The actual tutorial curriculum integration remains P3.

The local editor build succeeds (`editor-build-8.log`) and the protected Unreal suite passes **95/95**
(`unreal-7/index.json`). Added tests cover transactional profile persistence and recovery, corrupt-profile
archive/reset with byte-preserving rollback, failed opt-out writes, invalid/damaged journey selection,
stale cross-slot recovery/dismissal, checkpoint isolation, modal cancellation, pause freeze, concession,
UMG cold attachment, stable Slate root lifetime, and focus/disabled controls at 80% and 150% UI scale.
The first rendered editor inspection exposed an invisible initial shell: the UMG root was created after
Slate attachment. The widget now constructs its root before Slate caches it, with a cold-attachment
regression test. Rendered inspection subsequently verified the title, Options, 80%/150% scale,
and high contrast; settings were restored to 100%/standard contrast. It also exposed child-widget
navigation competing with the shell and premature label wrapping. The final source owns preview-key
navigation, separates hover from keyboard focus, and fills the available button-label width. On that
build, reverse navigation scrolled to Back and the opt-out prompt visibly defaulted to Cancel.
Native Slate button dispatch reached Options; CUA pointer activation did not establish a reliable pass.
A Metal compiler failure splitting the spaced external TMPDIR was isolated in `editor-inspection-3.log`;
the corrected launch uses `/tmp/echoes-p1-metal` as a symlink to external `LocalCache/Temp`.
The corrected-cache session rendered mission materials without the prior Metal path errors, exercised
deploy/pause/restart/exit with keyboard input, and was returned to a paused title for review.
`p1-qualification.json`, `title-candidate-8.png` and `pause-candidate-8.png` retain this bounded result.

All 366 candidate inputs are bound in `candidate-8-inputs.json`. The seven P0 native inputs remain
unchanged; the retained P0 optimized/debug/sanitizer evidence carries forward only for those inputs.
The editor inspection uses explicit external `EditorFixture` save/user directories. It is development
inspection, not packaged physical-input or ordinary-player evidence.

P1 remains IN PROGRESS until the rendered focus/activation/display matrix is qualified. Full Shipping
HUD migration, complete results telemetry/replay, tutorial fact collection, authored briefing presentation,
and packaged/physical/listening/human/owner gates retain their DeliveryPlan dependencies. Existing camera
and display settings persist in the profile but do not yet have complete controls in the new options view.
No requirement is COMPLETE; no commit, push or release was performed.

## 2026-09-05 — P1/P2 continuation and AI difficulty decision

**Author and owner:** Angelis Pseftis
**State:** IN PROGRESS. Owner authorizes continuing through P1 and P2 and coordinating independent work.
**Decision:** Angelis selected: “Story / Standard / Veteran / Sovereign; equal combat rules, reaction delays 3.0 / 1.5 / 0.9 / 0.5 seconds.” This resolves the contradiction in favor of `SPEC-DIF-001..004` and `SPEC-SKM-005`. The existing `REL-AI-005/014/015/017/018` bodies now reference that policy, retaining their identifiers. The conflicting Assisted/Challenging names, reaction schedules, 30/90/140/180 APM caps and Assisted damage handicap are superseded by the selected contract. Group commands per second remain distinct from measured player APM. This is a requirements decision, not acceptance of implementation or balance.

Current work retains the prior P0/P1 dirty baseline and evidence. The continuation receipt is
`BuildArtifacts/Evidence/p1-p2-completion-20260905/session.json`. Root owns integration, UI/controller and
heavy runs; separate workers own asynchronous checkpoint work and deterministic replay/reporting.
No requirement, package or owner gate is closed by this entry.

### P1/P2 integration evidence — current continuation

The native core passes **102/102** in optimized, debug and Address/UndefinedBehavior Sanitizer
configurations (`native-current-5.log` and its input/result receipts), including cooperative
replay, snapshot-parser and visibility-rebuild cancellation and eager-reference differential coverage
for the streaming replay command schedule, dense-tick report outcome retention, 64-bit APM and final
cancellation publication gates. Input hashes stayed unchanged throughout all three configurations.
This qualifies the bounded native suite, not the Unreal adapters or rendered player journey.
Requirement registry, agent-document structure and build-identity checks passed earlier in this continuation.

The integration history below retains each failure at its original candidate. The latest runtime
qualification is candidate 20, recorded at the end of this entry; rendered exit checks remain open.
Candidate 4 passed 95/98 Unreal tests. Candidate 5
compiled, then exposed a replay-prefix field-order defect that rejected checkpoint writes; AutosaveRecovery
subsequently indexed an absent generation and crashed. The format mismatch and test preconditions are
corrected in source. Candidate 6 was intentionally interrupted during content preflight to integrate the
archive shutdown hook. Candidate 7 compiled and ran all 105 tests: 100 clean passes, one success with two
world-teardown warnings, and four failures. The failures concern a synchronous save wrapper competing
with an already queued save, an invalid legacy-geometry replay baseline in M05, hidden-mover command
frontier setup, and an unrelated expansion assertion after a real Standard Corefall loss. Corrections
are integrated in source; the failures are not recorded as passes. Candidate 8 then failed UHT on
widget parameter shadowing; candidate 9 passed UHT but failed C++ compilation on one fixture visibility
call and three widget/test type errors. Those source errors are corrected. Static integrity review also
identified staged recovery, dismissal, failed-autosave retry and replay-admission defects. Repairs and
negative regressions are integrated; runtime verification remains pending. Every failed or interrupted
run and its input receipt is retained under `BuildArtifacts/Evidence/p1-p2-completion-20260905/`.

Candidate 10 passed content preflight and UHT, then failed compilation on two const-access errors in
AutosaveRecovery. Candidate 11 corrected that fixture and compiled successfully. Its protected full
suite ran all 111 tests: **104 clean passes, three warning-only successes, four failures**, with no
input drift. The sandbox denial/protection and cleanup gates passed. The failures cover M05 legacy
baseline normalization, the replay-browser fixture's missing ready scenario, widget refresh/map-node
semantics, and display-config persistence suppressed by Unreal's `-Multiprocess` flag. Source repairs
are underway; this failed run remains retained as `candidate-11-unreal-result.json`.

Candidate 11 also exposed long-history checkpoint latency after a normal Corefall loss at tick 52,764.
The eight queued cadence saves drained before the terminal manual save, taking about 176 seconds.
A retained process sample and call-path inspection identified eager replay-command queue scans and
repeated semantic reconstruction of byte-identical generated/readback files. The streaming core repair
passes the current native suite; bounded checkpoint worker repair and Unreal rerun remain pending.

The scoped terrain repair exposed legacy Lume Reach dressing bound to an older overlay: 34 of its
39 record cells are open in the current M10 topology across all three doctrine variants. Current source
rejects that decoration layer against authored topology before unexplored sentinels can admit it; normal
knowledge updates still use only scoped terrain/passability. The legacy diagnostic fixture remains
available. Re-authoring that optional legacy decoration for current M10 is a P5 campaign-art gap, not
P1/P2 visual completion or a reason to draw incompatible scenery.

Candidate 13 compiled, then logged 106 completed successes and two fixture failures (the later M05
missing-seed replay baseline and a nonadjacent replay-filter action). A new LocalPlayer ownership test
then used the wrong UObject Outer and triggered an ensure, ending the editor before final report export.
The launcher retained production deny protection and removed its temporary tree, but the per-test
empty-save-directory gate did not pass after the interruption. This is not a successful suite or final
isolation qualification. Candidate 14 corrects only those three fixture defects and compiles successfully;
its protected full suite ran all 111 tests: **109 clean passes, one warning-only success and one
failure** in 145.25 seconds. All 501 captured runtime inputs remained unchanged, and the save sandbox
protection/cleanup gates passed (`candidate-14-runtime-result.json`). The remaining UI failure exposed
three issues: a test INI redirection that Unreal replaces during SaveSettings, a scanner that omits
interrupted skirmish saves, and a fresh-controller fixture initialized before presenting the paused title.
The browser's warning-only result requires explicit scenario teardown while its world context exists.
Candidate 15 includes the repairs but failed compilation because a free recovery helper called a private
subsystem parser. Candidate 16 keeps both the helper and its inspection state private inside the subsystem
and compiled in 136.44 seconds. Its full suite completed **110 clean passes and one failure**, with no
warnings and all input/isolation gates intact. The shell fixture stopped at its INI isolation guard: UE5.8
known-config globals are cache keys, while `FConfigBranch::IniPath` supplies the physical file path.
Candidate 17 separates those values and compiled in 55.28 seconds. Its protected targeted shell test
passed cleanly in 17.10 seconds, including display disk persistence and the new recovery/mastery routes.
The full integrated suite then completed 110 clean passes and one failed shell test: the skirmish and
M01 fixture saves both committed in the same second (01:51:36 UTC), while UE's Mac file timestamp API
retains only whole seconds. Candidate 18 explicitly ages the earlier fixture and compiled in 30.49 seconds;
its full suite passed **111/111 with zero warnings/errors** in 145.65 seconds. All 501 input hashes stayed
unchanged; the protected-data denial probe, exact sandbox policy and cleanup gates passed
(`candidate-18-runtime-result.json`). At that boundary, rendered mouse/keyboard qualification was
still open and production inputs were unchanged from candidate 16. Candidate 18 subsequently passed
Shipping compilation; candidate 20 below qualifies the later input repairs. The preceding failures remain
retained and are not passing qualification.

The completed candidate-13 budget fixture measured 40 µs capture, 475 µs initiation and 7,369 µs total
completion for 401 entities / 396 commands / 336,606 bytes. The tick-52,764 normal-loss manual checkpoint
measured 172 µs capture, 623 µs initiation and 3,087,314 µs completion for 823,885 bytes. Its preceding
fast-forward cadence backlog drained in about 27.5 seconds; ordinary asynchronous UI initiation did not
use that synchronous compatibility-wrapper drain. These measurements come from completed fixtures in
a later-interrupted development suite (`candidate-13-save-budget.json`), not packaged performance or
full-suite qualification. The equivalent candidate-11 first manual worker took about 23.4 seconds.

The rendered candidate-7 title appeared at 1280×754. The first pointer movement into the game exposed
an array self-reference assertion in custom cursor painting. Source now copies the first point before
closing a stroke, and the regression fixture paints all seven cursor states through Slate. Its real
Slate-paint regression passed in candidate 11. Candidate 19 later rendered the cursor during M01 without
that assertion; the full rendered cursor/placement matrix is still open. The field, campaign-map and online panels now use an immutable
field view model, modular UMG/Slate widgets and controller routes. The retired Canvas HUD is removed;
legacy coordinate handlers are restricted to development test fixtures. Compilation, rendered behavior
and the Shipping source audit still gate `REL-UI-001`; source migration alone does not close it.

The bounded Shipping preflight found no new compile hazard in the P1/P2 widget/replay sources.
Candidates 18 and 20 subsequently compiled and linked the Mac Shipping game target successfully. Both legacy and sustained stress entry flags are now disabled in
Shipping. Engine on-screen debug messages are already compiled out by UE5.8 in Shipping/Test, as verified
in the local engine implementation. Existing network smoke and packet-fault command-line fixtures remain
a P6 Shipping network qualification follow-up; this offline package does not qualify those network routes.

Current source includes the selected equal-rules AI difficulty policy, separate tactical pause, cursor and
placement feedback, minimap/camera controls, full options controls, asynchronous checkpoint captures,
result metrics/curves and detached replay browsing/transport. Replay schemas distinguish recorded-content
integrity from rules/build compatibility; asynchronous archive results are bound to their result generation
and captured storage directory. UI retry retains an exact replay time/perspective, and player perspectives
consume scoped information. The explicit observer perspective reads only the detached recording.

The new save budget fixture measures a 401-entity scenario with 396 admitted replay commands and checks
capture, full game-thread initiation, committed bytes and replay-prefix fidelity. It passed in candidate 7:
36 µs capture, 531 µs full game-thread initiation, 2,546 µs encoding and 14,420 µs total completion for
336,606 bytes (`candidate-7-save-budget.json`). This is an Unreal development fixture on this host;
packaged baseline-hardware performance remains a separate gate. Candidate 7's three Standard AI
long-run scenarios passed, reaching actual Corefall at ticks 6,434, 48,908 and 57,289 and preserving
explicit economy, Well and combat restoration across the runs. The normal winning fixture passed;
the normal losing fixture reached Corefall at tick 52,764 and passed terminal save/load but still failed
its expansion assertion. These observations do not establish balance or human usability.

Current P1 source also enforces the stored mastery predicate before player-owned Campaign/Skirmish
lobby, deployment, restart/rematch, resume and checkpoint recovery routes. Confirmed opt-out remains
persisted prompt consent, returns to Title, and keeps Tutorial primary; it cannot unlock play. M01 is
available through the explicit Tutorial route or an M01 learning checkpoint. Recovery preflights and
reuses the exact candidate operation. Local players lazily load profiles and fail closed; low-level
controllers without a LocalPlayer or initialized profile remain runtime fixtures, with a regression
that attaching a LocalPlayer activates denial. Shipping excludes unattended/command-line auto-start
and holds initial simulation paused even if no controller exists. No production writer yet awards
tutorial facts; that remains P3. Fully mastered profile fixtures are explicitly seeded controller/storage
tests, not evidence of a player completing training. Candidate 12 compiled all 82 editor actions after
full content/tool preflight, with no source drift. Its runtime run was deferred to candidate 13, which
separates checkpoint preflight from successful M01 authorization and strengthens deployment/recovery
regressions. The later candidate-20 receipts below supersede those pending runtime and compile checks;
rendered qualification remains separate.

The requested Gemini See Loop captured editor frames. Analysis was rejected by Google's API with
`API_KEY_INVALID` after correcting local session initialization and trusted-CA configuration; no Gemini
vision assessment is claimed. No P1/P2 package exit, requirement COMPLETE state, packaged Shipping
qualification or owner acceptance is inferred from that probe.


#### Current qualification — candidate 20, 2026-09-06 UTC

The implemented P1/P2 changes pass the current automated checks at this source boundary. Their delivery
exit remains **IN PROGRESS** because stable rendered mouse/keyboard, display and HUD-scale endpoint
checks are unfinished. The development fixture results do not close physical-input, uncoached-player,
packaged-performance or owner-acceptance gates.

Candidate 20 is bound to commit `bc051467d52ee3e6001ab42931a974449a5d6245` plus the archived dirty inputs
in [candidate-20-inputs.json](../BuildArtifacts/Evidence/p1-p2-completion-20260905/candidate-20-inputs.json).
All 501 hashes remained unchanged through the 46.42-second editor build, protected **111/111** Unreal
suite (144.515 seconds, zero test warnings/errors), exact registered-inventory check, and 58.46-second
Mac Shipping build. Its post-build sync ad-hoc signed the app for local execution and registered it
with LaunchServices; this does not establish a fresh cook/package, distribution signing, notarization
or clean-machine installation. The save-denial probe, protected policy, empty fixture storage and
launcher cleanup passed. Shipping binary SHA-256 is `2a5fe65f92affc6a676cfbad99f2bf0db56c321e83454e328ad8c22e74fc3b5c`.
The separate native receipt still covers the unchanged core: **102/102** in optimized, debug and
Address/UndefinedBehavior Sanitizer configurations. Retained current receipts are
[candidate-20-runtime-result.json](../BuildArtifacts/Evidence/p1-p2-completion-20260905/candidate-20-runtime-result.json),
[candidate-20-shipping-result.json](../BuildArtifacts/Evidence/p1-p2-completion-20260905/candidate-20-shipping-result.json)
and [native-current-5-result.json](../BuildArtifacts/Evidence/p1-p2-completion-20260905/native-current-5-result.json).

The final input repairs give modal UI ownership of the system cursor, cancel armed build placement
before modal return, and consume right/middle mouse presses without activating shell actions. The
existing shell-widget fixture now paints real child geometry and dispatches right, middle and left
pointer events through its SObjectWidget wrapper. Non-primary events preserve action focus; left click
reaches the pointed action. This test passed in the complete candidate-20 suite.

Current save measurements were 44 µs capture, 471 µs main-thread initiation and 20.768 ms total completion
for 401 entities / 396 commands / 336,606 bytes. The normal Corefall loss at tick 52,764 measured 181 µs,
653 µs and 3.139 seconds respectively for 823,885 bytes. Capture and initiation are the relevant bounded
foreground operations; background completion is reported separately. The normal victory/defeat,
restoration, three Standard AI terminal runs, replay fidelity and asynchronous browser fixtures all
passed. These are local development measurements and automated mechanics, not balance or human-play
validation. See [candidate-20-save-budget.json](../BuildArtifacts/Evidence/p1-p2-completion-20260905/candidate-20-save-budget.json).

Candidate 19 rendered readable Options at 80% and 150%, toggled high contrast, reached display Revert
by keyboard/pointer, followed Start tutorial → M01 briefing → Deploy without injected mastery, and
queued/completed Bulwark Team production through the visible command card. Native Slate event dispatch
worked; CUA and OS synthetic pointer probes were inconsistent. Camera movement followed some probes,
but competing input prevents attributing a control-specific pass. Escape after HUD focus was intercepted
by PIE, so those checks require a standalone game window. The probe record retains the precise evidence
classes: [editor-gui-19-result.json](../BuildArtifacts/Evidence/p1-p2-completion-20260905/editor-gui-19-result.json).

Candidate 20 launched an uncooked standalone Development game using isolated save/profile paths.
The actual window became 2560×1440 under the fresh profile's borderless setting, despite the 1280×720
launch request; startup arguments therefore do not prove a display endpoint. A click intended for Options
reached Quit, Escape cancelled it, and the log later recorded Quit plus Confirm while the coordinator was
only reading metadata. This indicates competing input; its source was not established. The game exited
normally. Live input is paused pending an idle computer window. The empty editor automatically opened by
CUA discovery after that exit was closed; no editor or heavy run remains active.

Next executable work is a coordinated standalone mouse/keyboard pass: display Apply/Keep/Revert and
actual viewport sizes; both HUD-scale endpoints; normal selection/orders and placement refusal/confirm;
minimap and middle-drag; tactical pause and modal focus recovery; and visible result/replay return paths.
Retain actual screenshots and event receipts, fix any reproducible game defect, and rerun affected checks
against a new source receipt only if code changes. The See Loop capture path works, but Gemini still needs
a valid locally configured API key; no key value is stored in the evidence. P3 curriculum/mastery awards,
P4 packaged journey/owner play and P7 distribution remain their own packages.
