# Delivery plan — coherent, owner-visible progress

**Author and owner:** Angelis Pseftis
**Adopted:** 2026-09-02
**Maintained:** 2026-09-11
**Standing:** sequencing reference under [AGENTS.md](../AGENTS.md) and the [authority map](README.md).

The owner rejected a technically demonstrated prototype that did not yet provide the expected player
experience. Keep the recovery sequence focused on usable outcomes. [Requirements.md](Requirements.md)
defines those outcomes; [RequirementsState.md](RequirementsState.md) records evidence and acceptance.
The active state below records sequencing; requirement completion remains in RequirementsState.

The [complete game design and production plan](#complete-game-design-and-production-plan--2026-09-10)
is the active execution sequence. Dated audits and P0–P7 receipts are preserved in the
[historical ledger](Archive/ProjectLedger.md#delivery-plan-historical-execution-record--archived-2026-09-10).

Use the current owner's task scope and accepted decisions to choose the next slice. Preserve the active
work rather than restarting from a dated phase note. One write owner per checkout is the default;
bounded independent read-only review and explicitly disjoint work are allowed under the shared contract.
The retired lane-fleet roster and missing lock files do not govern current work.

## Active execution state

**Execution: RESUMED by owner (2026-09-10).**

| Field | Current continuation state |
|---|---|
| First unfinished package | **D2 — Establish integrated foundation** awaits the owner's replay of the five repaired findings (engineering exit met 2026-09-11); **D3 — Complete Meridian slice** started the same day under the standing mandate (first slice landed; see Last retained evidence). |
| Completed indexing | D0 preservation (`f8bfd47`), D1 binding (`23e7230`), retained main work (`bdd2d8a`), first D2 code (`120b60c`: 30-cap count, 4×4 Foundry, resource-depletion and unit-abilities salvage integrated). `b0108c1` ("D3") is a deck-card change that was reverted in this continuation; D3 has not started. |
| Saved baseline | `main` = `origin/main` after the schema 31 movement fix (code, tests and records `a8607f8`: masked-ground escape in the route field and the move-order gate, replay schema 31, native corridor tests, D2 review driver hardening, record correction; this baseline row follows it). Earlier today: `af611e6`/`afe96c6`/`899f55a` (D2 exit review), `9adc348`, `2cb36a9`, `d2992b7`/`68653ab`, `a37bacd`/`c924e72`, `ec1cca5`/`951433d`. |
| Ownership | Owner granted checkout ownership. On 2026-09-11 the owner ordered the push: main was fast-forwarded to `68653ab` and pushed to `origin/main` (862d7b2..68653ab); the checkout now works on `main`, single writer. `integration/d0-reconciliation` is folded in and retained only as a label. |
| Preservation boundary | Main dirty files in `BuildArtifacts/Evidence/resume-preservation-20260910`; Antigravity's scratch scripts, `tests.log` and walkthrough in `BuildArtifacts/Evidence/d2-foundation-20260911T0050Z/antigravity-scratch/`. Salvage worktrees untouched. |
| Last retained evidence | D3 third slice (2026-09-11, RequirementsState "D3 Meridian slice — third slice"): `BuildArtifacts/Evidence/d3-meridian-20260911T161144Z` — `readiness-review-8` PASSED 33/33 (lessons six to ten each committed in a rendered practice run via `Scripts/run_readiness_review.sh`; 33 captures), `automation-5/index.json` 139/139; practice-mode gate, practice staging and probe defects repaired. Before it: D3 second slice (2026-09-11, RequirementsState "D3 Meridian slice — second slice"): `BuildArtifacts/Evidence/d3-meridian-20260911T161144Z` — `automation-4/index.json` 139/139, 0 warnings; readiness lessons eight (Probe, scripted replay-safe contact), nine (Board, new `JumpToLatestAlert` F1) and ten (Well) are wired; `EchoesTutorialLessonCount` 10. Before it: D3 first slice (2026-09-11, RequirementsState "D3 Meridian slice — first slice"): `BuildArtifacts/Evidence/d3-meridian-20260911T161144Z` — `automation-3/index.json` 139/139, 0 warnings; research archive names the affected roster with before/after values and the fighter card explains researched damage; a completed unpowered Foundry is drawn dark; tutorial lessons six (Link) and seven (Foundry) are wired and earnable (`EchoesTutorialLessonCount` 7), narrative pack recompiled. Before it: Owner play-test findings 1–5 repaired (2026-09-11, RequirementsState "D2 owner play-test findings repaired"): `BuildArtifacts/Evidence/build-owner-findings-20260911T150015Z` — `test_sim-C.log` 143/143 native ×3 configurations (new "Meridian Foundry produces only while powered", "player view production answers match simulation"), `build-4.log` Result Succeeded, `automation-C/index.json` 139/139 with 0 warnings, rendered D2 exit review `review-1280x720` PASSED 15/15 (victory at tick 4939, deck shows names, prices and letter bindings; capture 07/09). Replay schema is now 32 (`kPoweredProductionReplayVersion`). Before it: Field console rebuilt as a non-scrolling instrument (2026-09-11, RequirementsState "Field console rebuilt as a non-scrolling instrument"): `BuildArtifacts/Evidence/hud-console-20260911T135529Z` — `build-09.log` Result Succeeded on the combined tree, `automation-04/index.json` 138/139 (the one failure is the concurrent play-test-findings lane's new `Presentation.FieldHudAuthority`), rendered D2 exit review `review-1280x720-e` PASSED 15/15 with the 3×3 deck, ledger and selection card all in frame without scrolling, and `review-2560x1440-b` for the REL-UI-013 matrix. Before it: D2 exit chain as an agent-driven in-process rendered review on Glass Scar: `BuildArtifacts/Evidence/d2-exit-review-20260911T1120Z-schema31` (result PASSED 15/15 on the schema 31 build, victory at tick 5416, 195 s, no stall, no assist, no stuck unit), and before it four consecutive full passes `…T0945Z`, `…T1005Z`, `…T1020Z`, `…T1035Z` plus the diagnostic `…T1050Z-naive-builder`; runs 1–10 are retained as the attempts that shaped the driver, each explained in RequirementsState. Simulation fix at replay schema 31 (masked-ground escape, SPEC-MOV-006/008): `d2-foundation-20260911T0050Z` `test_sim-12-fixoff.log` 139/141 with the change off (reproduction), `test_sim-13.log` 141/141 ×3 configurations with it on, `build-25.log` Result Succeeded, `automation-07/index.json` 138/138 (wrapper exit 0). Earlier today: `test_sim-09.log` 139/139, `automation-05` and `automation-06` 138/138. Evidence classes: automation and agent-driven rendered review only; no physical input, no package, no owner acceptance. |
| Next exact action after resume | Owner: replay the D2 chain on Glass Scar (five repaired findings, rebuilt console) and rule on SPEC-UI-007 / REL-UI-025, REL-FAC-002.PROD, SPEC-RES-006.INSPECT; play the ten-lesson readiness check (all ten lessons are now proven by the readiness review driver); rule on TBR-UX-001 (recommendation recorded: command-first QWE/ASD/ZXC grid, WASD as a camera preset); authorize the push of main so the post-D3 package can be produced (`package_macos.sh` requires a detached worktree at pushed origin/main). Agent: continue D3 — the researched-fighter silhouette cue (§4.1), REL-AI-022 measured Meridian strategy runs, then the D3 packaged complete match once the push is authorized. Carry the open decisions: SPEC-MOV-003 route-field body blindness, REL-AI-024, REL-AI-031; TBR-SCP-012 is decided (option B, D7). |
| Next dependency | D2 exit requires the affected integration suite green and a player able to gather, build, train, move, fight, repair and recover under 30 on an ordinary map; D3 (Meridian slice, REL-UI-025 3×3 deck, practice lessons) waits on it. |
| Open evidence | Owner acceptance of D2 is not given: the 2026-09-11 owner play test FAILED on five player-facing points, all five now repaired and awaiting the owner's replay (see RequirementsState "D2 owner play-test findings repaired"). Agent-driven rendered review and automation are green but are not human evidence. Current game reproduction and packaged verification remain open. Retained replays containing a Foundry predate the 4×4 correction and no longer reproduce (recorded in RequirementsState). |

This table is the sole current continuation summary. Update it before each handoff or interruption with
owned paths, exact source/dirty/package identity, last relevant result, unresolved failure and next action.
Link retained receipts rather than copying their logs. RequirementsState alone owns per-ID lifecycle and
acceptance; a receipt records an attempt and cannot independently advance this summary.

## Package context and state transitions

1. Read the shared contract and authority map, this active state, and the active package’s
   [exit gate](#11-execution-packages-dependencies-and-exit-gates).
2. Load only that package’s design sections below, exact affected master/state records, applicable skills,
   and source interfaces. Follow a dependency into another section when the actual change crosses it.
3. Consult historical receipts only for a named prior repair, provenance, failure or dependency. Check
   their source identity before reuse; a dated result is not proof on the current candidate.
4. Execute a bounded outcome only within current authorization. Record source checks, rendered/input
   evidence and owner acceptance separately. A failed or interrupted attempt stays in the same package.
5. Advance only after all package exit obligations have supporting evidence and unresolved dependencies
   have explicit dispositions. Keep pending human/owner gates visible; only Angelis grants acceptance.

| Package | Design context to load (section numbers below) |
|---|---|
| D0 | 2, 10, 10.2; preservation paths and exact candidate records. |
| D1 | 1–9.1, 10.1, 12.1 as affected by the design record being synchronized; exact master IDs and decisions. |
| D2 | 3, 5–7, 12; affected command/economy/population/save contracts. |
| D3 | 4–8, 9, 10.1, 12.1; Meridian object records and slice map. |
| D4 | 4–8, 9, 10.1, 12.1; Kharuun/Choir records and faction interfaces. |
| D5 | 7–9.1, 12.1; affected screens, settings and offline maps. |
| D6 | 9–10.1, 12; M01 and tutorial contracts plus campaign journey reference. |
| D7 | 9–9.1, 12; current mission contracts, narrative and cinematic references. |
| D8 | 2.1, 9, 12; affected Conquest, multiplayer or editor contracts. |
| D9 | 12–12.1; exact affected balance, performance, accessibility and release gates. |
| D10 | Release requirements/state and applicable packaging/distribution procedures from the authority map. |

The full design remains in this authoritative plan for targeted lookup. Dispatch briefs use the existing
[handoff contract](Prompts/GameDevelopmentWorkflow.md#delegation-and-additional-task-handoff), identifying
the active-state revision, package dependencies, exact writable scope, checks and next handback point.
Do not duplicate operating rules or requirement status in a separate global prompt or task database.

## Complete game design and production plan — 2026-09-10

**Author and owner: Angelis Pseftis. Status: planning complete for execution sequencing; implementation
paused by owner until an explicit resume after the weekly usage reset.** This section replaces the
narrow completion-audit execution sequence as the active plan. The archived historical packages remain
references, not competing starting points. No further code changes, builds, merges, game launches or
scheduled execution are part of this planning session.

### 1. The game we are building

An original, polished, ground-only science-fantasy RTS with the responsiveness, command clarity,
battlefield readability, functional base management and audiovisual feedback Angelis expects from
StarCraft II, expressed through Echoes' own story, factions, models, materials, environments and sound.
This is a quality and interaction target, not a commitment to copy another game's assets or numeric
balance. The game must make gathering, infrastructure, composition, scouting, position, timing,
retreat and Future Well decisions visibly matter in ordinary play.

**Owner-approved army limit, 2026-09-10:** at most **30 controllable mobile entities per player,
including workers and command characters; buildings excluded.** The earlier Unity-specific preference
is now explicitly approved for this game plan. A 200-unit army is not the design target. The existing
200 Logistics ceiling is a different quantity, not permission to field 200 entities. Package D1 must
synchronize the master and dependent records with this direction before implementing population changes.
Existing Logistics weights and capacity progression require review for usefulness under the 30-entity
ceiling; do not simply replace every occurrence of 200 with 30.

The full deliverable includes the current required twelve-unit/twelve-building baseline, its required abilities,
base and resource systems, research, all three factions, three offline skirmish maps, fifteen distinct
campaign maps and their endings, optional training, Conquest, supported multiplayer formats, complete
menus, accessibility, production art/audio and release qualification. The in-engine scenario/map editor
and other bound release features remain in the requirement coverage review. Hosted services retain their
existing deferral. Developer ID/notarization waits for publishing, not local play.

**First finished slice:** a fully functional Meridian base and force on a purposeful map, an opponent
that uses its real economy, complete command/HUD feedback, and a match to a truthful result. Extend that
slice through M01's story and choices. This is the first delivery milestone, not a reduction of the full
roster, ability, campaign or mode obligations. No engine rewrite or project discard is authorized by
this plan; retain useful work unless a bounded assessment proves it unsuitable.

### 2. What must be reconciled before production resumes

The September 9 [completion audit](https://claude.ai/code/artifact/f5682236-6875-4d35-bd8c-2a593f029ad5)
and its [179-item inventory](../BuildArtifacts/Evidence/completion-deep-dive-20260909T232000Z/domain-audits.json)
are intake, not the entire product specification or a current defect ledger. The user additionally
reports incomplete skills/menus, power and collection logic, health/damage feedback, text overflow and
unmerged work. Those concerns are explicitly covered below; the reported runtime defects still need
current reproductions.

Read-only inspection at `af3475e` found:

* A September 10 package at `7b7050f1` exists with retained cook/startup evidence; it predates later
  source changes and is not full player-journey evidence. The audit's September 4-only package claim is stale.
* AI gathering, structure occupancy, destination legality and M01 Reshape have newer repairs. The
  five-lesson tutorial mask is repaired, but the approved seven-chapter teaching experience is absent.
* Pending order/subsystem/input/layout changes and an order-queue test must be reviewed and preserved.
  This task's earlier build-preflight repair is also uncommitted; do not reimplement or revert it blindly.
* Ten worktrees and 88 local branches exist. Dirty work includes backend, HUD, resource depletion,
  unit abilities, M01 environment and concept-pipeline candidates. Salvage refs exist, but a salvage
  snapshot does not establish integration. Locally cached remote refs were not refreshed during planning.
* The component catalog contains names and footprints conflicting with the master—for example its
  Anchor is described as 6×6, while the stable master structure record is 5×5. Its claimed authority must
  be reconciled. Keep useful visual direction without adopting obsolete roster names or dimensions.
* The evidence checker reports 368 IDs without individual support among 393 historical family-row
  verification claims. That is documentation debt, not proof those features work or are absent.

Angelis granted this task ownership of the checkout. Do not ask again for that authorization. On resume,
check only for newly active conflicting writers and resources. Current owner direction and the master
control; old lane briefs cannot revive a retired restriction or override the 30-entity decision.

### 2.1 Remaining capability gaps and the StarCraft II comparison boundary

**2026-09-10 planning review:** a complete RTS experience cannot be guaranteed by the current feature
headings or by saying "all StarCraft II features." Retain the owner's quality/functionality ambition,
but name every equivalent capability, approved Echoes replacement and intentional exclusion. The
[162-item SC2 intake and 32-event checklist](SC2GameplayGapAudit.md) already provide the detailed
crosswalk; their dated implementation labels are not current truth. Reconcile them by package with
current source and player evidence, rather than launching another full audit at each restart.

Blizzard's [control guide](https://news.blizzard.com/en-us/article/6640645/game-guide-simplified-controls)
identifies type/idle-worker selection, subgroup cycling, order queues, eligible autocast, camera
locations and warning/base cycling, alongside chat and pings. These are concrete comparison surfaces,
not instructions to copy hotkeys that conflict with Echoes. Blizzard also documents
[ranked and Co-op modes](https://news.blizzard.com/en-us/article/21183638/starcraft-ii-4-0-patch-notes).
Those services/modes cannot be inferred from a working local match or a generic Multiplayer button.
Sources checked for feature examples on 2026-09-10, not for pricing or exhaustive current service parity.

| Gap that broad headings can conceal | Required disposition / owner package |
|---|---|
| Roster and technology breadth | D1 resolves `TBR-SCP-003`: the current four units/four buildings/two technologies per faction are a baseline, not proven sufficient depth. Evaluate materially different composition, opening, tech and recovery choices. Expand only through a coherent approved role design if the compact roster fails. Thirty entities limits fielded force, not the number of unit types in the game. |
| Tactical tools and counter coverage | D1 maps scouting, harassment, screening, fortified-position breaking, area control, sustain, disruption and deception to affordable available roles for every faction. Assess whether absent siege, area effects, transport or detection mechanics leave a strategic hole; do not add them just because another game has them. Respect ground-only and existing information rules. |
| Complete control ergonomics | D2/D5 explicitly close same-type selection, worker/army/producer cycling, control-group management, subgroup command routing, camera bookmarks, alert/outpost jumps, queue visibility/editing and all required rally target types. Preserve existing functionality rather than rewriting from the old absence list. |
| Ability and automation policy | D1/D4 define single-caster versus group-cast selection, invalid targets, range previews, cancel-without-spend, cooldown and interruption. Explicitly decide which abilities support autocast and its visible on/off policy. Never automate irreversible Well decisions or spending outside authorized policy. |
| Learning and reference tools | D1/D3/D4/D6 supply each unit/building/technology/ability with introduction, practice, counter-example and searchable help. Complete the Soryn Archive, mechanics glossary/model inspection and configurable combat lab where bound. Knowing a hotkey is not understanding a unit. |
| After-action learning | D5/D8 complete replay bookmarks/perspectives, analytical economy/production/loss overlays, camera controls and `REL-QOL-013` Take Command. Define safe branching and recorded-input versus AI behavior; never mutate the original replay or campaign ledger. Results explain damage, scouting, idle workers, supply blocks and decisive timings, not just victory/APM. |
| Team player experience | D8 includes ready/loading states, allied/all chat, pings, authorized vision/control/tribute, disconnect/desync/host recovery and spectator permissions. Define observer delay/fog/command restrictions before exposing live information. Chat needs usable channels/history and volume/mute controls as appropriate. |
| Accessibility behavior conflicts | D1 resolves ordinary pause versus optional tactical pause, gameplay assistance and input conflicts against current owner decisions. A settings checkbox is incomplete without correct battlefield behavior, persistence and feedback. |
| Content/editor lifecycle | D8/D10 cover creating, validating, saving, loading and playing supported custom content, compatibility errors and controlled import/export. A local editor is not an Arcade publishing/mod-hosting platform. |
| Whole-product parity boundary | D1 lists public accounts/friends, ranked matchmaking/rating services, hosted custom-game discovery/mod distribution and separate commander Co-op as service/mode questions. They are not silently added or claimed delivered: existing exclusions and hosting deferral remain until explicitly changed. |

Air combat/transports, unrestricted burrowing, a larger fielded army and gameplay elevation bonuses
are deliberate differences or current exclusions, not bugs to copy into Echoes. Campaign co-op and
hosted social/matchmaking infrastructure likewise need explicit scope changes. The completion claim
must be "the agreed Echoes capability set is complete," with the comparison crosswalk attached, not
unqualified literal StarCraft II parity. No review can prove that every possible feature has been
anticipated; acceptance requires the enumerated coverage and actual whole-game play.

### 3. Small-army strategy: design commitments and proof

The cap alone does not produce strategy. Establish the following relationships before broad tuning.
Use current binding stats as the starting baseline; amend them coherently through D1 where the new army
size requires it. Numbers below are not substitute balance tables.

| Design concern | Required design response | How to determine whether it works |
|---|---|---|
| Worker versus army commitment | Workers consume the same 30-entity allowance. More income trades against immediate fighting/scouting strength. One extractor per deposit, cargo, journey time and drop-off placement determine useful staffing. | Compare actual delivered income and viable army composition across worker allocations. More workers must not be automatically optimal or useless. |
| Population and Logistics | Track completed mobile entities plus production/spawn reservations separately from weighted Logistics. Reserve before a start; cancellation/death/replacement releases exactly once. Apply equally to AI. | Concurrent queues cannot exceed 30; save/load, capacity loss, cancellation and temporary supply cannot bypass either constraint. Show which limit is binding. |
| Special entities | Commandable characters and player-directed mobile projections require an explicit count/reservation rule. Morphing an existing unit does not create another slot. Buildings remain excluded; classify mobile buildings consistently as buildings. D1 records every entity type explicitly, including migrating Waystones and directed projections, before enforcement. | List every entity-producing action. No uncounted combat summons, free duplicate army, or ambiguous cap accounting. A capped ability gives a clear reason before spending. |
| Force composition | Each worker, line fighter, heavy/controller and scout/support must have a useful job, weakness and cooperation pattern. More copies of the strongest unit cannot dominate every map and matchup. | Test mixed forces, mono-role forces and alternative economic openings at comparable investment; retain counterexamples and explain wins through decisions. |
| Survival and reinforcement | Set HP, damage, attack timing, repair costs, production time and travel distance together. A player must have time to recognize threat and attempt retreat, without endless invulnerable repair loops. | Measure focused-fire time-to-kill, retreat survival, replacement delay, repair expenditure and recovery after a lost fight. |
| Territory and objectives | Small forces choose what to defend, raid or scout. Routes and Well states create commitments and alternatives. Avoid both an empty oversized map and a single compulsory choke. | Compare flank travel time to defender response and reinforcement time; verify opportunities to trade territory for an objective. |
| Bases and defenses | Building placement protects production and gathering but creates targetable dependencies. Static defenses must have affordable counterplay within the approved roster. | Break a defended position through composition, power denial, terrain, timing or another valid approach; do not require a nonexistent siege unit or 200-unit overwhelm. |
| Technology and abilities | Research, adaptations, identities and abilities consume resources/time that could support infrastructure or replacements. Their windows must be readable and exploitable. | Demonstrate distinct viable timing choices, not mandatory upgrades or ability spam that removes meaningful response. |
| Intelligence | A scout uses a valuable slot but enables better commitments. Faction sensors reveal only their contracted information. | Compare informed versus blind engagements; test deception, vision loss, anonymous pings and scout interception. |
| Winning | Resources, unit count and execution remain relevant, but positioning, information, counterplay and timing can overturn a numerical advantage. | Use paired scenarios changing one decision at a time. Do not script a smaller army to win or claim that numbers should never matter. |

D1 resolves roster sufficiency under TBR-SCP-003, then produces one consistent tuning baseline in registered content: costs, HP, damage, range, cooldowns,
movement, production, repair, resource income, Logistics and map travel times. Measure all units against
those actual rules. No balance claim may use a different default ruleset or a synthetic map presented as
a shipping map. Retain existing quantitative performance/match-length criteria unless explicitly revised.

### 4. Complete roster coverage

These tables define required coverage, not current completion. The master owns precise values and
conditions. Every row needs its full data, command, state, animation, UI, sound, failure and counterplay
coverage before that faction is called finished. Missing signature abilities remain full-game work.

| Unit / master ID | Strategic purpose and complete functional coverage |
|---|---|
| Surveyor / SPEC-UNIT-001 | Gather/carry/deposit, construction, repair, Well interaction; Network Repair costs, interruption, stacked-worker efficiency and recovery. |
| Lancer / SPEC-UNIT-002 | Disciplined ranged fire, targeting, wind-up, attack cadence, focus fire, facing and retreat. No invented activated ability to fill its command card. |
| Bulwark Team / SPEC-UNIT-003 | Deploy/pack barrier, frontal arc, damage reduction, flank vulnerability, movement constraints and visible transition/impact states. |
| Relay Skiff / SPEC-UNIT-004 | Ground-pathing scout; Extend Relay eligibility, connected source, temporary Logistics, cooldown, expiry and disconnection feedback. |
| Tender / SPEC-UNIT-005 | Gathering/building/Well work; Stabilize Scar targeting, cost, channel, interruption, terrain change and failure feedback. |
| Riftstalker / SPEC-UNIT-006 | Mobile harassment; Slipfire versus stationary damage/cadence, movement-fire animation and eligible Warform Adaptation. |
| Cairnback / SPEC-UNIT-007 | Heavy lane control; Raise Mineral Cover placement, HP, collision/projectile blocking, duration, expiry, cooldown and adaptation. |
| Resonant / SPEC-UNIT-008 | Scout/anti-scout; moving-target vibration pings, anonymous identity, positional limits, expiry, stationary-target refusal and adaptation. |
| Threadkeeper / SPEC-UNIT-009 | Gathering/building/repair and Well work; Reconcile Structure and truthful upkeep/solvency feedback. |
| Intervalist / SPEC-UNIT-010 | Flexible line fighter; Manifest/Possible reconciliation, vulnerable transition, exclusive modifiers, cooldown and readable identity. |
| Lacuna Warden / SPEC-UNIT-011 | Heavy controller; Bind Interval cost, valid visible target, slow/ability disable, range/line-of-sight break, expiry and identity interaction. |
| Afterimage / SPEC-UNIT-012 | Scout/misdirection; Forked Trace duration, player direction, count policy, anonymous sensing, non-collision/non-attack and cleanup; identity interaction. |

| Building / stable master ID | Complete operational and selection-menu coverage |
|---|---|
| Anchor / SPEC-STR-001 | Core defeat consequence, worker queue, rally/spawn access, drop-off, network root, Logistics and repair. |
| Power Link / SPEC-STR-002 | Valid connection chain, drop-off eligibility, supplied capacity, disconnected/reconnected state, affected dependents and range display. |
| Array Foundry / SPEC-STR-003 | Full Meridian roster queue, research prerequisites/slot use, reorder/cancel, costs/reservations, rally and blocked exit handling. |
| Aegis Post / SPEC-STR-004 | Powered fire, valid source, target/range/hold-fire, power-loss shutdown and restoration, damage and repair. |
| Memory Hearth / SPEC-STR-005 | Core defeat, Tender queue, rally/drop-off, Logistics and migration-network feedback. |
| Waystone / SPEC-STR-006 | Root/uproot/migrate/reroot, timing and exposure, footprint refusal, operational drop-off/capacity changes and worker reassignment. |
| Growth Basin / SPEC-STR-007 | Kharuun queue/research/rally; adaptation eligibility/site range, cost, vulnerable molt, replacement and interruption/death. |
| Listening Spine / SPEC-STR-008 | Anonymous movement sensing, coverage/contact history, alert threshold, information limits, disabled/destroyed feedback; do not invent an attack. |
| Concordance / SPEC-STR-009 | Core defeat, Threadkeeper queue, drop-off, Logistics and coherence-root information. |
| Interval Loom / SPEC-STR-010 | Capacity, operational dependencies, recurring upkeep, solvency warning, loss/recovery and exact master function. |
| Chorus Loom / SPEC-STR-011 | Choir queue/research/rally, recurring debt, production limitations and upcoming coherence obligations. |
| Phase Anchor / SPEC-STR-012 | Stabilization field, non-stacking upkeep reduction, protected structures, next-charge forecasts and loss of coverage; reconcile any ambiguous hold-fire text before implying a weapon. |

Include all six stable research records `SPEC-TECH-001..006`: Prismatic Targeting, Horizon Lattice,
Echo Cartography, Ancestral Edge, Held Alternatives and Shared Resolution. Show prerequisites, costs,
facility occupancy, progress, completion, cancellation policy and affected stats. Reconcile duplicate
technology identifiers before editing values. Do not invent new units/tech to hide an unfinished role;
resolve roster-sufficiency decisions against the compact roster and demonstrated strategic depth.

### 4.1 Research changes, appearance and player knowledge

Research is a visible strategic commitment, not an invisible modifier. For each technology, D1 defines
exact affected unit/building IDs, changed attributes or actions, prerequisite, cost, facility/time,
stacking/order of application, effects on existing versus newly produced units, maximum-health changes
if any, cancellation/destruction behavior and save/replay restoration. Never imply that every upgrade
changes every unit. Keep faction research separate from Kharuun adaptation, Choir identity and temporary
ability buffs; the inspector explains the source and remaining duration of each modifier.

The following are **presentation proposals for the existing six technologies**, to be implemented with
approved faction art direction. Numeric descriptions summarize the current stable master records;
D1 verifies affected entities and stacking before finalizing before/after values.

| Technology | Current intended mechanical benefit | Proposed persistent visual cue and tactical meaning |
|---|---|---|
| Prismatic Targeting | Damage at 115% of the applicable baseline. | A readable targeting-optic/weapon-module detail and distinct restrained firing cue on eligible weapons. Do not imply increased range or a shield. |
| Horizon Lattice | Vision at 120% of the applicable baseline. | Sensor/lattice detail on eligible entities; selected friendly vision-range preview. Distinguish sight range from weapon range. |
| Echo Cartography | Vision at 120% of the applicable baseline. | Kharuun sensor/strata detail and an updated friendly sight preview; no invented attack upgrade or hidden enemy revelation. |
| Ancestral Edge | Damage at 115% of the applicable baseline. | Distinct mineral weapon-edge detail on eligible fighters; attack presentation reflects stronger damage without inventing armor or a new attack type. |
| Held Alternatives | Damage and vision at 110% of applicable baselines. | A stable Choir geometry/edge motif distinguishable from temporary identity transitions; both attribute changes appear in the inspector. |
| Shared Resolution | Vision at 120% of the applicable baseline. | A distinct resolved sensor motif and sight preview; D1 determines composition with the earlier tier rather than guessing a net percentage. |

At research selection, show exact cost/time/prerequisite and the affected roster, with before/after
attributes and a short reason to choose it. During research, show facility occupancy/progress and what
cannot be produced concurrently. At completion, update eligible existing and future entities according
to the defined rule, play a bounded owner-only notification, mark the research completed and expose the
new capability immediately. No upgrade is accepted if its icon changes but simulation does not, or if
simulation changes without any usable player feedback.

The selection inspector shows completed technology icons/names, effective stats and a breakdown such
as base damage + permanent research + adaptation/identity + temporary effect. Multi-selection indicates
mixed states rather than presenting one unit's upgrades as everyone's. The faction research/archive
screen lists owned, researching, available and prerequisite-locked technology; tooltips remain usable
at all supported sizes. Only display stats applicable to that entity.

A player should recognize a meaningful upgrade at normal gameplay zoom without reading tiny cosmetic
detail, while exact numbers remain inspectable. Use a persistent silhouette/material detail plus the
inspector, not a mandatory full-model replacement for every tier or color alone. Enemy appearance is
revealed only through permitted observation; never announce hidden enemy research or show current
upgrades/HP through fog. Last-seen information remains explicitly stale. Test healthy/damaged variants,
identity/adaptation combinations, save/load and replay so upgrade presentation cannot disappear or lie.

### 5. One completion record per object, not a model-presence checklist

Populate the existing component/catalog and requirement records in place during D1–D4. Each named
unit, building, ability, technology and interactable needs these linked fields:

* Identity, faction, mode, campaign introduction, player purpose, tradeoff and counterplay.
* Costs, 30-entity accounting, Logistics, HP/max HP, authorized protection or damage-reduction rules, damage, range, cadence,
  movement/turning, footprint, build time, prerequisites and power/upkeep dependencies.
* Every command: target rules, queued/immediate behavior, validation, acceptance/refusal, cancellation,
  cooldown, interruption, resource/reservation effects and persistence/replay behavior.
* Complete state diagram and authoritative events feeding presentation; selection card and command
  menu; visible/aural success, denied, waiting, damaged, disabled and recovery feedback.
* Registered model/material/animation/effect/sound sources, rights/provenance and runtime bindings.
* Exact reproduction, required tests, package identity, observed player result, unresolved issue and
  acceptance boundary. One passing cell never completes the row.

Use separate lifecycle dimensions: designed, implemented, integrated, source-tested, packaged,
visually/aurally reviewed, physically exercised and owner accepted. Do not convert a family count,
source test or concept image into a finished unit/building claim.

### 6. Economy, power and base logic must be visible systems

| System | Required flow and failure cases | What the player must understand |
|---|---|---|
| Matter | Move → wait for extractor → work → carry → choose valid operational drop-off → credit → return. Include congestion, cancellation, destroyed/unpowered/migrating drop-off, retained cargo, empty deposit and idle registration. | Which workers are earning, waiting, carrying or stranded; deposit remaining; where cargo goes; why income stopped and how to fix it. |
| Dawn / Wells | Actual acquisition and spending; Harvest/Preserve/Reshape costs, timing, public commitment, interruption, control transfer, terrain effect and persistence. | Immediate gain, recurring value, irreversible consequence, known route change and competing expenditure. |
| Population / Logistics | Separate live count, reserved slots, weighted usage/capacity, temporary supply and waiting production. | “Army limit reached” differs from insufficient Logistics, Matter, Dawn, prerequisites or power. |
| Meridian grid | Root → valid connected links → dependents. Handle severed bridge, alternate path, loss of supplied behavior and reconnection; no visual-only cable or false powered turret. | Source connection, operational range, downstream exposure and exact reason a system is offline. |
| Kharuun migration | Rooted supply/drop-off → uproot → move → validate site → reroot; gather-route fallback and vulnerable transition. | What stops while migrating, where workers deliver, when capacity returns and why a site is illegal. |
| Choir coherence | Structure-specific debt, due times, Phase Anchor reductions, insolvency transitions and restoration. | Cost of the next commitment, affected structures, warning lead time and what restores operation. |
| Construction / repair | Placement and access, spend/reserve, foundation/progress, assist/cancel, completion, damage/repair, destruction and blocked production exits. | Building intent, blocked reason, progress, workers assisting, repair cost, lost capability and recovery action. |

Verify these with ordinary commands and the AI on the same rules. Source accounting, visible animations,
menu numbers and actual delivered resource totals must agree. No worker appears to harvest while its
extraction timer is continually reset; no disabled building looks operational.

### 7. Health, attack, hit and destruction presentation

HP is authoritative game state, not merely a number above a model. Each unit/building needs correct
max/current HP, damage application, authorized protection/reduction rules, repair/clamping, ownership/fog visibility,
selection readout and destruction consequences. Save/load restores the corresponding presentation.

| State or event | Required visible/audible treatment and consistency check |
|---|---|
| Healthy idle | Recognizable faction/role silhouette and restrained operational motion; no damage effects on a pristine asset. |
| Movement / work | Ground contact/gait or ground-constrained hover; turning; distinct waiting, extraction, cargo, deposit, build and repair behavior. |
| Attack | Acquire/aim, wind-up, firing/recoil or strike, projectile/beam, cooldown and recovery. Attack timing follows authoritative events. |
| Incoming fire / miss | Legible direction and projectile path; missed or blocked shots do not play a successful damage reaction on the target. |
| Shield / reduced hit | Separate directional shield/cover response from body damage; magnitude reflects actual resolved damage. Include rear/flank counterplay. |
| Damaged | Readable health change, hit reaction and persistent faction-appropriate material/geometry/effect state. A transient flash alone cannot communicate sustained damage. |
| Critical structure | Honor `REL-ART-027` below 30% HP; smoke, sparks and warning states as contracted, with faction-consistent treatment. Reconcile older catalog damage thresholds rather than silently replacing either. |
| Critical unit | Define readable unit-specific severity bands in its art card; damage motion must not falsely imply a speed/weapon penalty absent from simulation. |
| Repair / restored | Repair activity is distinct from attack impacts; effects and degradation clear at the defined recovery thresholds; health cannot exceed current maximum. |
| Power/coherence loss | Visibly different from damaged health; show the unavailable function and recovery reason without implying a dead building. |
| Ability / adaptation | Tell anticipation, active effect, vulnerability, cancellation and cooldown apart; preserve role/ownership readability. |
| Death / collapse | End attacks/work/orders exactly once, release reservations/capacity as contracted, play faction-appropriate destruction and retained aftermath. Cosmetic debris never blocks simulation or steals clicks. |

Review at actual RTS zoom, in motion, under fog, against every terrain family and with HUD visible.
Include simultaneous effects and low-quality settings; dense particles must not hide targets, hit points,
telegraphs or selection. Use `SPEC-ART-001..004`, `REL-ART-013/027/031..033` and the master visual budgets.
Production requires complete functional motion, not a particular animation technology chosen in advance.

### 8. Finished menus, HUD and text containment

Every screen gets a layout, navigation/focus graph, action wiring and state coverage. Cover title/main
menu, campaign/world map, mission briefing, optional training/chapter selection, faction/skirmish setup,
multiplayer lobby/connect/disconnect, Conquest map/run summary, gameplay HUD, pause, options, key rebinding,
save/load/recovery, technology/archive/help, results, replay browser/player and scenario/map editor.
Include loading, empty, disabled, error, confirmation, cancel/back and return-to-game states.

The tactical HUD must coherently expose resources/income, 30-entity count and reservations, Logistics,
selected-unit/building HP and attributes, subgroup/command card, abilities and cooldowns, production and
research queues, minimap, objectives, alerts/history, chat where applicable, tooltips and placement/Wells.
All icons either execute a supported action or communicate a real prerequisite/refusal. No dummy action,
unexplained disabled button, unfinished submenu or contradictory hotkey label is release-complete.

**Overflow is a release defect, not cosmetic cleanup.** For each text-bearing box define wrap, maximum
extent, scrolling/truncation policy and full-text access. Exercise long chat/log messages, long unbroken
strings, speaker/object names, multiple alerts, changing numeric widths and the longest authored refusal.
Text stays inside its intended region without clipping essential instructions or covering buttons.
Messages remain retrievable; scroll position and new-message indication behave deliberately.

Run the master resolution/UI-scale matrix, resize and display recovery, maximum supported text scale,
keyboard/pointer focus and remapped controls. Verify hit regions match what is drawn. Selection cards,
production actions, minimap routes and attack alerts must remain legible during a real battle. Finish
functional command panels alongside their units/buildings; do not postpone all UI to final polish.

### 9. Factions and maps must make the strategy real

| Faction | Strategic strengths to express | Vulnerability and map opportunity |
|---|---|---|
| Meridian | Prepared connected infrastructure, screened ranged fire, directional cover and temporary reach. | Flank a deployed line, sever an exposed link, force a choice between grid expansion and a mobile response. Routes need defendable junctions and alternatives. |
| Kharuun | Mobile infrastructure, harassment, sensing, terrain cover and adaptations chosen against contact. | Catch a migration/molt, deny a root site, force prolonged frontal combat. Maps need usable alternate paths and resource positions worth relocating toward. |
| Choir | Information ambiguity, identity timing and resource-funded control under coherence obligations. | Read public transitions, break a tether/position, pressure solvency and punish a mistimed commitment. Maps need meaningful reconnaissance and timing windows without hidden-information cheats. |

Before terrain dressing, map briefs bind story/location, commander, objectives, available force, worker
routes, buildable base area, chokes and flank access, visibility, reinforcement times, sensor value,
Future Well alternatives, fallback/retreat routes, AI plan and faction counterplay. Measure actual
clearance and travel time; do not scale SC2 map dimensions or camera values mechanically.

Design the connected fifteen-location world and its campaign progression together before finalizing
individual missions or tutorial placement. Physically build and qualify a representative slice before
mass-producing the remaining maps. A map is not distinct merely because its color or landmark changes.

* **Glass Scar:** qualify the Ash Cut/Buried Causeway/Folded Verge commitments and central Well with a
  small army; retain safe versus contested resource decisions and reachable Reshape fallbacks.
* **Crownfall Basin:** distinguish gate defense, expansion routes and flanking around its ridges; do not
  let the best static position cover every useful resource and objective.
* **The Confluence Ring:** make entrance control, information and rotation matter; verify a defending
  force can be challenged without permanent single-gate deadlock.
* **M01–M15:** use each `SPEC-MSN-001..015` contract and the existing
  [mission binding table](MapTechnicalBlueprint.md#campaign-map-contracts);
  the controlling mission records remain authoritative. Each map's capability manifest identifies
  what is introduced, practiced, assessed, retained and locked. Preserve actual command factions and
  roles; no invented mixed-faction army or replacement storyline.
* **Conquest and team/FFA:** author their own sector/participant/spawn/resource contracts. Six-player
  3v3 means up to 180 controllable mobile entities in total under the new per-player limit, plus buildings
  and other simulated entities; do not mistake 30 per player for a whole-session performance budget.

Prove 30-entity mission feasibility including workers and characters. Objective concurrency cannot
silently require more independent forces than the cap supports. Fix sequencing/layout/available assets
coherently with the mission contract, rather than adding hidden cap exceptions or gifting a huge army.

### 9.1 Larger maps with usable strategic features

**Owner direction, 2026-09-10: build larger maps with more strategically useful places and interactions.**
D1 reviews and revises each affected map's dimensions, layout and pacing contract in place before world
production. Existing 64×64 presets are not a permanent product limit. Select larger dimensions from
route timings, build space, objective separation, exploration and the 30-entity force—not from an
arbitrary multiplier. Preserve or explicitly revise existing travel-time/fairness requirements together
with geometry. Empty walking distance is not strategic depth.

Each map brief must answer: **what must I accomplish, what routes/resources/information can help me,
what do I give up by choosing them, and how can an opponent interfere?** Supply an annotated gameplay
layout showing starts, base expansion space, worker routes, objective chain, direct and alternative
approaches, defensive positions, retreat/reinforcement routes, Wells and their terrain variants, sensors,
passage endpoints and the camera/minimap boundary. In a small-army game, separated commitments must be
reachable and contestable without requiring a garrison at every decorative landmark.

| Strategic map element | Use toward an objective | Opponent response / required tradeoff |
|---|---|---|
| Direct road and longer flank | Reach an objective quickly or bypass a prepared front. | Scout/intercept the flank or exploit the attacker's longer reinforcement route. |
| Choke plus viable alternate access | Hold a route with a small screen while investing elsewhere. | Flank, deny the supply route, contest a different objective or use an authorized terrain ability; no unavoidable permanent stalemate. |
| Safe and exposed resource locations | Choose secure slower development or income supporting a timing attack. | Harass workers/drop-offs or force a costly defensive commitment. Test actual collection journey times. |
| Grid junction / rooted site | Extend production, defense, gathering or logistics toward the next objective. | Sever the junction or strike during relocation; the world and HUD identify affected services. |
| Scouting position / sensor corridor | Learn an approach before committing the limited force. | Clear the scout, remain outside coverage or use authorized misdirection. Information access obeys faction/fog rules. |
| Authored subsurface passage | Eligible Kharuun forces change approach through a known, bounded route. | Watch exits, exploit transit capacity/time and threaten endpoints under their contract. No free burrowing. |
| Temporary mineral cover | Screen retreat, divide firing lines or protect a crossing. | Reposition, destroy cover or wait out expiry; blocked paths and duration are legible. |
| Future Well transformation | Buy resources, sustained information/income or a temporary objective route. | Contest commitment, exploit the same opened route or pressure another location during the investment. Preview expiry and safe fallback. |
| Mission-specific interaction site | Recover records, power an objective, escort a carrier or complete the authored operation. | Use only threats and failure conditions supported by that mission; show objective dependency and progress. |

These use existing authorized systems. New gates, destructible bridges, capturable outposts, environmental
hazards or bonuses require a full interaction/counterplay contract in D1 before inclusion. A visually
prominent object is either clearly decorative or has a discoverable legal interaction. Elevation and
weather currently remain presentation-only; do not imply high-ground damage/vision bonuses, weather
penalties or ecological cover unless the master is explicitly amended. Shivergrass information and
passage behavior retain their precise existing boundaries.

For M01, evaluate how the archive/carrier route, controlled Well choice and evacuation route work as a
connected tactical decision. Show how a safe recovery approach, a faster exposed return and the authored
Reshape route change the player's available plan without inventing another mission objective. Across
M02–M15, repeat the method with each mission's own story and objective dependencies, not the same layout.

Every consequential terrain change updates movement/placement/projectile rules only as authorized,
world presentation, minimap, selected route preview and affected-unit recovery coherently. Test the AI
on the enlarged map for expansion, scouting, path choice, response time and retreat; increase path/fog/
save/performance coverage with map size. More terrain and decoration alone cannot pass this package.

### 10. Recover and integrate useful code before replacing it

D0 inventories every registered worktree, local branch, relevant remote branch and dirty/untracked
candidate. Record exact HEAD, merge base, unique committed changes, patch-equivalent changes, dirty
paths, generated assets and evidence identity. Refresh remote refs only when execution resumes; cached
refs and timestamps are not evidence that a branch is current or better.

Classify each coherent change as already present, useful missing work, conflicting implementation,
obsolete/superseded, or uncertain pending focused verification. Map it to requirements and the packages
below. Inspect both source and registered data: an unmerged ability, menu or environment must not be
rewritten merely because it is absent from main. A newer commit may be worse; a large ahead count may
represent different ancestry rather than hundreds of useful changes.

Preserve patches/untracked files and salvage references before integration. Integrate one bounded
change at a time, resolving semantic conflicts against the design, then compile/test its real interfaces.
Use at most one controlled integration worktree if needed to protect the dirty checkout, with a named
owner and a definite integration/retention handoff. Do not strand another fleet of candidates. No blind
whole-branch merge, broad reset/clean, history rewrite or deletion of unreviewed work. An archived candidate
retains its rationale and recoverable content. Old evidence retains its original identity.

### 10.1 Tutorial and progressive learning: teach decisions, not camera exercises

**Status:** the replacement is required by `SPEC-TUT-007/008` and on this roadmap; it is not implemented
or accepted. Fixing the old five-lesson completion mask did not implement the new curriculum. Mandatory
zoom/pan/centering/waypoint drills are retired. Camera assistance appears only when needed to perform
a meaningful action and never awards chapter mastery.

The introduction explains who commands, where the player is, what must be accomplished and why the
available tools matter. Begin with the Anchor and nearby Matter seam, without a prebuilt mobile army.
Every new object follows **identify → explain purpose/tradeoff → demonstrate → player uses it → observe
its result → apply it in a slightly different situation**. Demonstrations and automatic selections
cannot earn mastery. Use actual bound names, icons and remapped controls rather than generic jargon.

| Chapter | What the player learns | Required action and evidence of understanding |
|---|---|---|
| 1 — Establish a foothold | Anchor produces workers; Surveyors turn a Matter seam into delivered income; workers consume part of the 30-entity allowance. | Produce two Surveyors, assign gathering and observe credited deliveries. Explain working/carrying/waiting states. Clicking a building or moving the camera is insufficient. |
| 2 — Build a useful outpost | Power Link purpose, connection/drop-off/Logistics, placement tradeoffs, construction and operational state. | Place and finish useful connected infrastructure, observe its benefit and resolve a readable invalid-placement or disconnected condition. Teach why this location helps. |
| 3 — Prepare a first force | Foundry purpose, costs, queues, prerequisites, rally and the Lancer's role. | Construct the producer and genuinely produce three Lancers, establish a useful rally and identify their strengths/limits. Research is introduced through a focused follow-on exercise showing actual before/after benefit. |
| 4 — Protect the position | Recognize threat/health loss, defend gathering, distinguish movement from attack orders, focus/retreat and repair. | Respond to a bounded threat, preserve a functioning economy and restore damage. Explain what failed and how to recover rather than demanding a flawless win. |
| 5 — Scout and interpret | Scout role, fog/current versus old information, sensor limits, route choice and reinforcement exposure. | Discover a relevant approach/threat and choose a response using what was actually seen. Merely moving to a marker does not prove interpretation. |
| 6 — Choose a Future Well protocol | Capture/contest, compare Harvest/Preserve/Reshape, costs, public timing, irreversible effects and opponent response. | Secure and choose a protocol, then use its actual benefit toward a training objective. Offer replayable practice for the other choices without changing the campaign ledger. |
| 7 — Independent command | Combine income, infrastructure, production, information, composition, abilities and objectives into a plan. | Complete an unguided bounded operation under the cap. Loss offers useful diagnosis and direct retry; no forced repetition of the entire economy tutorial. |

A seven-chapter introduction does **not** teach the whole game by itself. Provide replayable
specializations for Meridian support/defense, all Kharuun and Choir mechanics, every signature ability,
research/adaptation/identity changes, efficient command, counterplay, all Well protocols and mode
orientation. Every required roster/tech item has a linked archive entry and a practical exercise; no
new faction is treated as a recolor requiring no teaching.

Each campaign mission's capability manifest marks tools introduced, practiced, assessed, retained or
locked. Before a newly required mechanic decides success or failure, introduce its name/appearance,
purpose, activation, cost, visible result, limitation and counter in a low-pressure context. Revisit it
later with fewer prompts and a different tactical problem. Do not force a camera lesson into that slot,
or demand an unexplained ability simply because its icon is now unlocked. Skipping remains available
and never fabricates mastery or blocks otherwise valid campaign/skirmish access.

D1 authors the curriculum and per-object learning links. D3/D4 implement and test the matching practice
when each faction system becomes usable. D6 assembles and qualifies the full first-time journey and
chapter recovery; **D6 is not the first time teaching is designed or built**. D7 integrates progressive
teaching with mission introductions. This prevents a finished roster followed by a rushed tutorial.

Each step specifies scenario/prerequisites, concise purpose, actual target, demonstration, permitted
learned actions, authoritative success predicate, acknowledgment, optional deeper explanation, wrong-
action response, hint escalation, skip, interruption, save/reload and successor. Ensure the student can
recover from spending resources, losing a required entity or blocked construction without becoming
soft-locked. Independent assessment remains distinct from assisted practice.

Acceptance includes a new player demonstrating **what the thing is, why to use it, how to use it and
when to choose an alternative** through play and focused comprehension checks. Time spent, completed
camera movement, dismissed text or a filled lesson bitmask cannot establish understanding. Retain
uncoached player observation separately from automated step-predicate tests and owner acceptance.

### 10.2 Phase D0 canonical inventory and preservation ledger

**Project inventory and data preservation ledger successfully recorded under Phase D0.**
**Execution remains paused.** This entry records the previously inspected snapshot of **10 registered
worktrees, 7 dirty locations and 23 local branches not merged into main by ancestry**. It is an indexing
baseline, not D0 reconciliation completion, proof of active writers, or a newly created backup.
No remote fetches, branch merges, worktree lifecycle operations, source edits or active-writer checks
were performed for this indexing update. Only this plan was edited; the existing salvage files were read.

Workspace root: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun`.
Canonical checkout: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project`.
Recorded main commit: `af3475eb096e2b95c264f9562cd88b88c32cf9d7`.
All locations in the tables below are relative to that workspace root. Recheck them when execution is
explicitly resumed; the inventory is not a continuing assertion that the working trees stayed unchanged.

#### Target preservation paths — seven dirty checkouts

| Location | Recorded branch / HEAD | Preservation scope |
|---|---|---|
| `Project` | `main`, `af3475eb` | Pending orders/subsystem/tactical input/HUD layout changes and order-queue test; this task's plan, document-link and build-preflight changes. |
| `Worktrees/backend-actions-20260909` | Detached, `6b559889` | Backend and integration source plus untracked content. |
| `Worktrees/m01-environment-completion` | Detached, `6b559889` | M01 environment/source and untracked content. |
| `Worktrees/resource-depletion-20260909` | Detached, `6b559889` | Resource/replay changes and untracked content. |
| `Worktrees/unit-abilities-20260909` | Detached, `6b559889` | Ability/checkpoint/simulation candidate and untracked content. |
| `Worktrees/concept-production-pipeline` | `docs/concept-production-pipeline`, `dbe4ddbb` | Pipeline changes and untracked content. |
| `Worktrees/hud-concept-completion` | `ui/concept-console-completion`, `6b559889` | HUD/content/runtime changes and untracked content. |

Branch association is not a lock or a live ownership claim. Detached HEAD alone does not mean dirty;
these seven locations were separately observed to contain changes. Preserve each independently before
any later integration, cleaning or reset.

#### Clean registered worktrees — committed-change review remains

* `Worktrees/art-concept-fidelity-production` — `art/concept-fidelity-production`, `7e1e25b4`.
* `Worktrees/map-environment-concepts` — `docs/map-environment-concepts`, `b628040b`.
* `Worktrees/verify-main-862d7b2` — detached, `862d7b21`.

A clean worktree does not establish that its committed content is integrated, correct or obsolete.

#### Unmerged branch matrix — 23 local references

These are ancestry results, not 23 independently missing features. Assess patch equivalence,
cherry-picks, duplication and superseding implementations before integration.

| Domain | Local references |
|---|---|
| Salvage and integrity baselines (6) | `salvage/backend-actions-20260909-20260910T000500Z`<br>`salvage/concept-production-pipeline-20260910T000500Z`<br>`salvage/hud-concept-completion-20260910T000500Z`<br>`salvage/m01-environment-completion-20260910T000500Z`<br>`salvage/resource-depletion-20260909-20260910T000500Z`<br>`salvage/unit-abilities-20260909-20260910T000500Z` |
| Gameplay systems and content (5) | `workstream/ai-skirmish-balance/guard-escort-semantics`<br>`workstream/campaign-progression/plan17-playability-repair`<br>`workstream/campaign-progression/tutorial-curriculum-model`<br>`workstream/player-experience/shared-arrow-dispatch-test`<br>`workstream/world/glass-scar-compiled-map-contract` |
| Network and online hardening (6) | `workstream/network-online/5b-log-throttle`<br>`workstream/network-online/5b-ratelimit-log-throttle`<br>`workstream/network-online/nsec1-smoke-exit-gating`<br>`workstream/network-online/nsec2-csprng-credential`<br>`workstream/network-online/nsec4-smoke-rpc-hardening`<br>`workstream/network-online/nsec6-protocol-fuzzers` |
| Presentation, audio and integration (6) | `art/concept-fidelity-production`<br>`docs/concept-production-pipeline`<br>`integration/composed-7824094`<br>`workstream/audio/sound-enabled-enforcement`<br>`workstream/visual-presentation/palette-separation`<br>`workstream/visual-presentation/uvscale-diagnostic` |

#### Existing salvage format — inspected, not regenerated

Directory: `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/BuildArtifacts/Evidence/worktree-salvage-20260910T000500Z`.
The existing [salvage README](../BuildArtifacts/Evidence/worktree-salvage-20260910T000500Z/README.md)
provides a consolidated inventory table. Each of its six worktree subdirectories contains:

* `identity.json` — recorded HEAD, branch and counts.
* `status.txt` — captured working-tree status.
* `tracked.patch` — pre-generated per-worktree unified/binary diff; the README records capture with
  `git diff HEAD --binary`. The inspected backend patch has normal Git unified-diff headers.
* `untracked-list.txt` and `untracked.tar.gz` — separate untracked-file inventory/archive.

**There is no single combined cross-worktree diff manifest in this inspected directory.** It already
has per-worktree diffs and identities; use them as the historical preservation baseline. The six salvage
refs are recorded as preserving tracked/index content, not untracked files. The evidence directory is
Git-ignored according to its README, so branch preservation alone does not back up those archives.
The main checkout is not one of these six snapshots and needs its own preservation on resume.

This inspection confirmed file presence and format, not archive integrity, restore success, current
worktree equality or the README's historical verification claims. Its statements about pruning are
not authority to operate on worktrees; no prune or deletion is part of this update.

**Comparison method on explicit resume:** retain the existing snapshots unchanged; verify their
identities/archive readability and preservation destination, then independently compare each current
dirty worktree with its recorded HEAD and captured patch/untracked inventory. Separately compare its
committed branch work against canonical main, accounting for patch equivalence and semantic overlap.
Do not treat an old patch as a current main-relative delta or apply it blindly. Preserve any newer
changes and the main checkout before selecting the first bounded integration.

**Next D0 action:** perform that preservation/comparison and disposition work after owner resumption.
The indexing substep is recorded; reconciliation, merge readiness and D0 exit gates remain open.

### 11. Execution packages, dependencies and exit gates

This is the sole active order. One package may contain several tightly related fixes; one integrated
player outcome is the unit of progress. Design and tests accompany production, rather than becoming
unbounded separate audits. Each row is planned work, not a completion claim.

| Package | Concrete work | Exit before advancing |
|---|---|---|
| D0 — Reconcile the project | Preserve all candidates, classify overlapping work, identify one canonical source baseline and reusable fixes; establish ownership and continuation record. | Every active candidate has a disposition/owner; no potentially useful dirty work is lost or silently ignored; exact next integration identified. No code rewrite during inventory. |
| D1 — Bind the game design | Synchronize the approved 30-entity rule; resolve roster/footprint/control/document conflicts; populate object/state/menu coverage; define faction dependencies, research appearance/stat feedback, a screen-event inventory, coherent tuning baseline and larger-map strategic briefs. | One consistent actionable design across master, component cards and registered-source targets. No invented completion and no unresolved decision hidden inside code. Resume here after D0, not in a generic bug sweep. |
| D2 — Establish integrated foundation | Integrate retained repairs; close command/pathing/fog, HP/damage, population reservation, production/economy/power and save/replay failures against D1. Use an ordinary playable map with diagnostic scenarios. | Focused failures repaired; affected integration suite passes; a real player can gather, build, train, move, fight, repair and recover under 30. Tests use shipped rules. |
| D3 — Complete Meridian slice | Finish all four units, four buildings, research, abilities, operational menus, faction motion/sound and power dependencies; build their matching practice lessons and finish baseline opponent behavior. | A packaged complete match with readable health/damage, functioning commands/economy, counterplay, truthful outcome, replay and recovery. No unfinished Meridian command/menu surfaces. |
| D4 — Complete Kharuun and Choir | Implement or integrate every roster/structure/tech ability and its UI/art/audio, migration/adaptation, sensing, identities/tethers/projections and coherence. Build the matching faction practice lessons; apply cap to AI plans and all spawning paths. | Each faction is independently playable; all object records exercised; faction mirrors and cross-faction counterplay demonstrated. Missing signatures cannot remain “deferred” in full-game status. |
| D5 — Complete the shared player experience | Finish all applicable menus/HUD/settings/rebind/save/replay surfaces, text containment, feedback, world/material/animation/effect/audio consistency. Validate each offline map's strategy with the full roster. | No reproduced overflow or dead menu paths; polished battle readability at required settings; complete three-map offline mode. Mode-specific unfinished screens remain explicit D8 obligations. |
| D6 — Finish M01 and optional teaching | Use the coherent world/roster to author the opening and seven optional chapters; teach actual production, infrastructure, combat, scouting and Well decisions. Finish M01's three branches, failure/retry and M02 continuation. | Packaged fresh-profile route and all chapter/mission recovery paths; visible consequences and intelligible story/audio. Uncoached understanding and owner review remain distinct. |
| D7 — Produce the full campaign | Complete M02–M15 environments, mission-specific encounters, introduced capabilities, characters/voices/cinematics, branch consequences and four endings. | Every mission and supported branch is playable on its own authored map, with correct persistence and presentation; full campaign routes succeed. No generic map substitution. |
| D8 — Complete remaining modes/tools | Conquest sectors/run state, direct/LAN formats and map variants, their menus/recovery/security, and bound scenario/map-editor workflows. | Complete runs and supported participant configurations; no dead selectable modes, save/schema regressions or unsupported connectivity claims. |
| D9 — Tune and qualify the whole game | Consolidate ongoing balance work across three maps, factions, doctrines and difficulties; performance, stress, stability, accessibility, save compatibility, content/provenance and final defect closure. | All mandatory coverage accounted for; no unresolved critical/high-impact player defects; explicit disposition of every audit item and bound requirement. |
| D10 — Deliver and accept | Package identified Shipping candidate, installation/recovery/manual/support and distribution requirements when authorized; final representative and complete-product owner review. | Required release evidence and explicit Angelis acceptance. A local ad-hoc package remains a local package. |

D2–D9 include continuous balance and performance checks appropriate to the changes; D9 consolidates them,
it does not discover fundamental fun or performance problems for the first time. Present a useful
packaged milestone after D3, D5, D6, D7 and D8. Do not rebuild/package for every cosmetic source edit or
stop delivering builds for weeks while polishing isolated components.

### 12. Verification that tests the design rather than just code existence

For each integrated feature, demonstrate player action → valid authoritative response → correct
visual/audio/UI feedback → counterplay/failure → recovery/persistence. A model, ability method, asset
import or passing unit test alone never completes that chain.

Maintain a targeted scenario set: gather through depletion and lost drop-off; sever/restore power;
migrate a Waystone under harassment; pay/miss/restore coherence; simultaneous production at the 30 cap;
train/research/cancel; each ability valid/invalid/interrupted; healthy/damaged/critical/repaired/dead
unit and building; frontal/flank/fire-blocked engagements; scouting/deception under fog; long-text UI;
base assault/retreat/reinforcement; M01's Well alternatives; save/load/replay and outcome transitions.
Reuse scenarios; do not manufacture a new verification framework for each failure.

For strategy, cover faction mirrors and all cross-faction pairings on the three offline maps with
representative seeds/openings, then broaden on a found imbalance. AI obeys the cap, resource costs,
vision, timing and cooldowns. Evaluate worker/force allocations, rush/defense/expansion choices, scouting,
ability use and static-defense counterplay. Combine measured runs with actual human play; automated
win rates cannot accept enjoyment or strategic depth. Retain losses, stalemates and exploit cases.

For visuals, review matched in-game views and motion at representative zoom/settings, followed by
real battles. Source metrics alone cannot establish recognizable HP damage, satisfying weapon feedback,
readable text, faction identity or sound intelligibility. Do not impose unsupported exact balance or
visual score thresholds merely to fill a table; derive them from the master or explicitly decided tuning.

### 12.1 Screen-wide event and feedback coverage

D1 inventories every player-visible screen, widget, world-space marker and effect in the existing
component records. For each, bind the authoritative event/state, permitted observer, world/UI location,
priority, start/end timing, stacking/replacement rule, sound, accessibility variant, performance budget
and recovery behavior. All effects must explain something useful; decorative spectacle cannot conceal
targets or pretend an absent mechanic exists.

| Event family | Required coordinated feedback |
|---|---|
| Selection / order / refusal | Selection/target marker, command acknowledgment, route or queue preview and concise actionable refusal; accepted order and failed action must look different. |
| Gather / deposit / depletion | Worker action/cargo state, real credited resource change, deposit exhaustion, idle/stranded indication and a recoverable location alert. |
| Build / production / research | Blueprint validity, progress, queue/reservation, cancellation/refund policy, completion and updated commands/attributes; no completion cue before authority confirms it. |
| Upgrade / adaptation / identity | Clear permanent-versus-temporary indicator, changed appearance/stat source, public transition where contracted, expiry/cooldown and interrupted state. |
| Fire / hit / shield / damage / death | Source and target, direction, resolved damage/protection, health change, persistent damage state and correct destruction/aftermath; no phantom hits. |
| Power / Logistics / coherence | Operational/offline appearance, affected connection or coverage, correct resource/capacity warning and specific restoration action. |
| Scouting / contact / under attack | Observed entity versus anonymous/stale contact, directional/location cue, minimap/alert history and bounded sound; no hidden-state disclosure. |
| Well / terrain / objective | Capture/contest/commitment, previewed spatial effect, progress/timer, interruption/expiry, route change and objective consequence. |
| Save / load / network / results | Progress/success/failure/recovery, pending versus durable state, interruption/disconnection and actual win/loss cause; never imply success before completion. |

Define information priority under simultaneous events: critical actionable threats and blocked essential
operations remain readable; repeated low-priority acknowledgments coalesce. Preserve a retrievable alert
history, avoid overlapping voice lines, keep subtitles legible and respect reduced flashing/motion and
volume settings. A stress scene combining a raid, damaged building, power loss, research completion,
resource depletion and objective update tests the composition of the whole screen—not just each widget
alone. Every visible control/indicator/effect has a coverage row; unbound or unexplained effects remain
unfinished work. D3–D8 prove their features, D5 checks cross-screen composition, and D9 closes omissions.

### 13. Continuation, authority and rules against repeated rework

Use the [active execution state](#active-execution-state) and
[package context procedure](#package-context-and-state-transitions) for each continuation. Preserve
existing design and source repairs; the historical ledger is a lookup reference. Update the current
summary before handoff and retain detailed receipts with their exact evidence/source identity.

Reopen a repaired issue only for a new reproduction, changed dependency, invalidated evidence or owner
instruction; record which. After two failed attempts under the same hypothesis, change the investigation
or obtain one bounded specialist review. Do not repeatedly run an unchanged suite, raise bookkeeping
volume, weaken tests or equate commit count with progress. One writer owns shared gameplay files; one
owner holds the build/editor resources. Specialists earn their cost through independent bounded work.

Ask only for a real owner-level design/rights/publishing decision, required human participation or a new
unresolvable live conflict. Continue unaffected authorized work. When usage/time is running short, retain
the exact continuation state rather than start another broad task. At each milestone report what a player
can now do, which gaps remain and the next package. Only Angelis assigns owner acceptance.

**Resume instruction:**

> Execute the Complete game design and production plan in Project/Docs/DeliveryPlan.md. Resume at D0
> or the subsequently recorded first unfinished package. Ownership is granted. The game has a hard cap
> of 30 controllable mobile entities per player including workers and command characters, with buildings
> excluded. First reconcile useful branches/worktrees and bind the complete game design; then implement
> the ordered packages, with complete unit/building abilities, economy/power, HP/damage animation,
> menus/HUD, faction strategy and purposeful maps. Preserve existing work, do not replan or restart the
> old audit, and keep a compact continuation receipt. Deliver packaged milestones and report actual
> player outcomes without claiming human acceptance from automated checks.
