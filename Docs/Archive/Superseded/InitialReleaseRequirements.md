# Initial Release Requirements — Sole Normative REL-* Ledger

**Author and owner:** Angelis Pseftis
**Status:** SUPERSEDED 2026-09-03 by [`Requirements.md`](../../Requirements.md) and [`RequirementsState.md`](../../RequirementsState.md). Its 106 transcribed `REL-*` bodies and all governance (record schema, acceptance-card rule, quality target, scope boundary, TBR decisions, release gates, final definition of done) migrated to Part III of the master; its change log migrated to the state file. The 369 identifiers it declared as ranges without ever transcribing a statement are now listed individually in the master as `DECLARED — NO TEXT`. Retained as the record of the ledger as it stood; it defines nothing and accepts nothing.

---


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

## Change log

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
