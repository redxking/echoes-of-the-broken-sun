---
title: Echoes of the Broken Sun — Campaign and Tutorial Player Journey Plan
author: Angelis Pseftis
creator: Angelis Pseftis
last_modified_by: Angelis Pseftis
created: 2026-09-06
updated: 2026-09-06
status: Source-grounded assessment and proposed implementation plan; not gameplay acceptance
---

# Campaign and tutorial player journey plan

The campaign needs to be organized around decisions the player understands and consequences the player can see. Its central progression is already present in the story: recover a disputed record, keep people and infrastructure alive, compare incompatible accounts, make public decisions, and hold an earned final resolution. The immediate problem is translating that progression into purposeful RTS play.

The recommended change is to replace generic deployments and coordinate-driven instructions with a complete mission contract for each operation: **why we are here → what is at risk → what the player can command → what the player must accomplish → what changes because they succeeded.** Automate the execution and bookkeeping of accepted orders. Preserve human responsibility for priorities, positioning, investment, protection, and irreversible choices.

## Scope, authority, and evidence

This is the single detailed planning document for the requested tutorial/campaign analysis. It supports the existing [DeliveryPlan](DeliveryPlan.md), particularly P3, P5, and Conquest under P6; it does not restart those packages or supersede their prerequisites. [Requirements.md](Requirements.md) remains the behavioral authority, [RequirementsState.md](RequirementsState.md) owns status and decisions, and the [Development Bible](Archive/DevelopmentBible.md) owns creative canon. This document proposes implementation and design corrections; it does not modify the mission contracts, declare a new canonical event, or accept the game.

Coverage: all ten named guided lessons, the independent readiness assessment, the three acts and fifteen story missions, faction/mode onboarding, and the approved 25-sector Conquest mode. Network-session implementation and unrelated art production are outside this analysis. Multiplayer needs mode orientation, identified below, but no new multiplayer campaign is proposed.

The inspected checkout was `main` at `3c033df742ed9253bf78d1d117f449d10264a1b9` on 2026-09-06. Six tutorial/HUD paths were already modified at entry. Their changes were preserved. The evidence is source inspection of requirements, current source, authored narrative/world data, and mission models, supported by two read-only campaign reviews. There was no game launch, build, fresh gameplay test, or uncoached human session for this analysis. Therefore findings describe source behavior and design gaps; they do not assert how frequently an issue occurs in a packaged build.

Inspection identity and source hashes: [inspection-identity.json](../BuildArtifacts/Evidence/campaign-tutorial-analysis-20260906T233432Z/inspection-identity.json). This is an evidence receipt, not a second planning document. Historical test counts and source JSON fields such as `runtime_consumed:false` are not substitutes for checking the current runtime binding.

## What the analysis found

| Finding | Current evidence | Why it matters | Required correction |
|---|---|---|---|
| The visible tutorial stops halfway through its defined curriculum. | [EchoesPlayerTutorial.cpp](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerTutorial.cpp), `TickTutorialObservation`, handles Survey through Reserve and then displays “Further lessons are not available yet.” [EchoesPlayerFlow.h](../Source/EchoesOfTheBrokenSun/Public/EchoesPlayerFlow.h) limits individual practice to `0x001f`, the first five lessons. | A learner cannot reach construction, production, combat, objective management, and Well mastery through the complete intended instruction sequence. | Finish lessons 6–10 and the independent gate; verify the complete fresh-profile route without injected completion flags. |
| Distinct missions largely share a generic starting force. | [EchoesSimulationSubsystem.cpp](../Source/EchoesOfTheBrokenSun/Private/EchoesSimulationSubsystem.cpp), `SpawnForce`, special-cases M01/readiness but otherwise creates a Core, Barracks, Dropoff, three workers, three line units, heavy, scout, and utility structure before mission additions. | M06 explicitly calls for a small mobile force without initial production; other missions call for six/seven workers, full bases, or limited Choir forces. A generic force obscures what each operation is testing. | Author a capability and deployment manifest per mission, reconcile it with `SPEC-PLAN-001..015`, and stop spawning unrelated base packages. |
| Economy is insufficiently tailored to the operation. | The same subsystem creates eight shared Matter nodes with 1,600 Matter each. M03–M15 normally receive 1,000 Matter/500 Dawn; M02 uses the 500/30 fallback; M01/readiness use 500 Matter and at least the configured Reshape cost. | A mission described as scarcity, avoidance, or triage can receive an economy that lets the player ignore that constraint. Resource presence alone does not create a strategic decision. | Budget the required route, recovery margin, optional objective, reinforcements, and available tech against the actual operation. Preserve approved values until reconciliation is adopted. |
| Many objective gates are presence, construction, or hold predicates. | Mission models and subsystem adapters sample rooted structures, powered interfaces, actor positions, and hold durations. M11–M13 reuse many inherited site coordinates. | These are valid implementation primitives, but can produce repetitive “move here, build here, wait” play unless travel, information, defense, and commitment create distinct choices. | Give every phase a tactical problem, a visible progress condition, and a meaningful response to pressure. Preserve site semantics; differentiate route topology, logistics, threat access, and command distribution. |
| Internal conditions are exposed as player instructions. | [EchoesPlayerController.cpp](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp), campaign guidance near M09–M15, contains tile-coordinate checklists and command-key sequences. | The player learns how to satisfy an adapter instead of understanding the operation. Abstract interface names compound the problem. | Use named sites, actual selected-unit roles, map pings, contextual actions, prerequisite explanations, and visible before/after state. Keep coordinates in diagnostic tools. |
| Authored teaching, requirement text, and implementation have drifted apart. | [OpeningAndTutorialScript.md](OpeningAndTutorialScript.md) is an earlier ten-lesson script; `SPEC-LSN-*`, `SPEC-TUT-005/006`, and `REL-FTU-*` specify additional actions and different lesson groupings. The script’s ending still lists previously resolved production questions. | A correct observer may wait for an action the spoken lesson never asked the player to do. | Maintain one lesson-ID mapping with substeps, binding tokens, narration, proof, recovery, and unlocks. Correct the existing script in place after mapping. |
| Some legacy release mission bodies conflict with the detailed mission contracts. | `REL-CAM-*` still includes alternate mission titles/premises such as M12 “Transit Collapse,” while `SPEC-MSN-012`, current mission source, and canon define “The Future That Won.” | Implementers can accidentally produce a different mission while believing they are following requirements. | Reconcile exact conflicting bodies through RequirementsState and the master before production. Use the current detailed `SPEC-MSN-*`/canon bindings as the plan’s identity; do not revive discarded missions. |
| Important M01 design decisions remain unresolved. | `TBR-M01-ROSTER-001` now records the owner’s six-Surveyor/two-Lancer choice; `TBR-M01-ANCHOR-002` retains an extraction-site conflict; `TBR-M01-RESHAPE-003` records that the current Reshape cells are already open. | A promised terrain decision cannot teach strategy if it changes no usable route. | Use the resolved roster; retain the bound extraction site pending reconciliation; design and approve an actual Reshape route effect before presenting it as tactically useful. |

The existing reducers, persistence machinery, branch records, and fifteen map bindings should be reused. The missing layer is a deliberate relationship between a player decision, an appropriate force, the terrain, a threat, and the mission outcome.

Current external reference points support the direction, not a claim of industry-wide consensus. Microsoft’s [Xbox Accessibility Guideline 109](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/109), updated March 2026, recommends reviewable objectives/progress, contextual guidance, and replayable interactive tutorials. The official [Age of Empires IV quickstart](https://www.ageofempires.com/news/quickstart-guide-age-of-empires-iv/) separates basic training, focused practice challenges, and continued campaign learning. The proposed Echoes application is persistent purpose/progress information, individual practice, and gradual transfer to independent command. Those references do not validate Echoes’ present usability.

## The intended journey

| Stage | Player’s question | What they learn or decide | Completion gives them |
|---|---|---|---|
| Opening and readiness | Who am I, what is endangered, and how do I issue a reliable order? | Mara’s immediate evacuation responsibility; controls, economy, construction, combat, information, and Wells. | Demonstrated basic control and access through the approved mastery/opt-out rules. |
| Act I — Necessary Fires, M01–M05 | How do I preserve a route, a reserve, and a witness when demands conflict? | Evacuation, unfamiliar faction tools, distributed infrastructure, mobile bases, and ceasefire discipline. | The founding doctrine and an initial record of what the two societies could establish together. |
| Act II — The Cost of One Future, M06–M10 | What has been omitted, and who pays when resources cannot cover everyone? | Evidence protection, comparison, reciprocal contact, city allocation, and a second Well commitment. | Publicly understandable liabilities and decisions that shape the finale. |
| Act III — Crownfall, M11–M15 | What can we actually demonstrate, and what future can we responsibly hold? | Coalition coordination, independent verification, public linkage, Choir command, and earned resolution. | One chosen ending, with eligibility and cost traceable to recorded actions. |
| Replay and Conquest | What would a different decision or strategy change? | Alternate mission outcomes without rewriting the established story ledger; separate strategic runs with disclosed variation. | Comprehensible replayability rather than random content accumulation. |

The target is that no story mission becomes a generic extermination match merely because an enemy Core exists. Corefall belongs to its authorized match/assessment context. `REL-CAM-021` requires hostile-Core destruction not to create automatic failure in an escort/hold/witness operation, but current detailed mission failure bodies and reducers still include “skirmish no longer ongoing.” This is a live requirement/source contradiction to resolve together; this plan does not assert it is already fixed or silently delete those failure conditions. Destroying a hostile Core must not substitute for the required evacuation/readback. `SPEC-CAM-003` resets ordinary armies, resources, bases, and skirmish research between operations. Persist only the authorized campaign records and rewards; explain the new deployment rather than implying an army has vanished.

## A common design contract for every lesson and map

Before a map enters detailed production, its manifest must answer these questions. The capability manifest already required by `SPEC-CAM-002` is the foundation; this table specifies the proposed authoring detail needed to make it useful.

| Contract field | Required content |
|---|---|
| Purpose and stakes | One plain sentence naming the operation, beneficiary, immediate risk, and why this commander is responsible. Avoid promising unmodeled casualties or political outcomes. |
| Entry and prior knowledge | Mission ID, map ID, commander/faction, inherited records, prerequisite lessons, player-visible briefing, and exact reset/persistence behavior. |
| Capability manifest | Every unit, building, command, ability, resource, terrain interaction, and relevant UI system marked introduced, practiced, assessed, retained, or locked. A newly required tool gets a safe demonstration first. |
| Deployment | Exact types/counts, named special actors, initial ownership, resources, known terrain, usable buildings, available production/research, starting orders, and why each belongs. |
| Phase graph | Active goal, dependencies, permitted alternate order, player command, state predicate, progress feedback, failure condition, retry point, and outgoing transition. |
| Spatial plan | Safe orientation area, main route, meaningful alternatives, objective sites, work/defense space, attack approaches, resource routes, camera bounds, and any Well transformation/expiry fallback. |
| Opposition | What each force wants, where it comes from, what it can know, warning, approach, target priorities, reinforcement condition, retreat/stop condition, and difficulty changes. |
| Economy and time | Mandatory expenditure, optional expenditure, recovery reserve, supply capacity, travel/hold durations, and the reasons for pressure. Use current authoritative costs; proposed tuning requires measurement. |
| Outcome | Exact success/failure predicate, optional conditions, records written, rewards, debrief, next briefing, and replay/save isolation. |
| Evidence | Source identity, positive/negative branch cases, normal mouse/keyboard path, recovery tests, complete rendered/audio inspection, uncoached comprehension, and owner decision. |

Every object must pass a purpose test: **what can the player do with it; when will that matter; what decision does it enable; and how will its state be readable?** A decorative asset instead needs a credible place/story role and must not resemble an unavailable control. A combat unit needs an authored opponent, screening role, positional task, or reserve decision. A production building needs an actual reinforcement/economic choice. Remove unjustified scaffolding through the authoritative deployment source after checking mission, save, and narrative dependencies.

### What the game should automate

| System responsibility | Automate | Human responsibility and limit |
|---|---|---|
| Lesson setup | Stage the required actors/sites, reserve safe working space, show current bindings, spotlight the target, and freeze unrelated pressure. | Player selects and issues the taught action. Setup never awards mastery. |
| Orders | Pathfinding, movement around obstacles, approved formation behavior, attack execution, and appropriate feedback after an accepted command. | Player chooses force, destination, target, stance, timing, and retreat. No hidden objective-solving orders. |
| Economy | Continue the approved gather → carry → valid drop-off → return loop after assignment; show delivery and idle/blocked states. | Player assigns routes, builds relevant infrastructure, protects the route, and reallocates workers when necessary. Do not silently add depletion retargeting where the current contract forbids it. |
| Construction/production | Workers execute assigned work; queues consume time/cost/capacity; units use the selected rally; completion alerts identify the result. | Player chooses location, investment, queue, cancellation, reinforcement mix, and rally. No free completion or unrelated auto-build. |
| Mission observation | Detect reached sites, completed buildings, power, capture, witness separation, eligible hold start, interruption, and completion. | Player establishes and protects those conditions. Presence can complete an ordinary mission task if that is its contract; it cannot prove an unperformed tutorial action. |
| Opposition/events | Run authored deterministic conditions, visible warnings, fair-information AI, and recorded variation. | Player responds, accepts a risk, or changes strategy. Never silently scale attacks to every player action or teleport enemies into protected space. |
| Information | Update objectives, minimap, local observations, resource state, errors, and accessible alerts. | Player scouts, interprets evidence, compares options, and prioritizes. A marker must not disclose hidden enemies or unearned branch information. |
| Irreversible decisions | Calculate and display valid choices, costs, known consequences, eligibility, telegraphs, and confirmation state. | Player chooses and confirms Well protocol, district allocation, or ending. Never choose for a hesitant player. |
| Persistence/results | Autosave at authorized points; validate and atomically commit successful records; show branch-aware results and the next operation. | Player chooses retry, resume, replay, next deployment, or exit. Storage failure must offer recovery without claiming successful commitment. |

Automation of production and testing is separate: compile mission/map/lesson manifests, validate reachability and required actors, generate objective bindings, and test all branch/failure paths. These tools should reduce authoring mistakes. They must not become the player’s source of instructions or be counted as human understanding.

### The objective interface

Keep the current objective persistent and concise. Selecting it reveals **why / who / where / how / progress / risk**, pings the named site, and offers a camera jump. Present the next necessary action only when a prerequisite blocks progress. Retain completed objectives and decision history in the ledger; separate optional tasks from required ones.

For M01, an example proposed presentation is: “Recover the archive carrier — bring the carrier to Archive Recovery and keep it safe while the records are secured.” The supporting panel identifies the carrier, shows the recovery site and status, then explains why it must remain while the Surveyor commits the Well. On completion it states what was secured and exposes withdrawal. The UI must not ask the player to infer actor identity or stand on an unexplained tile coordinate.

Use short in-world labels with plain descriptions: “District power connection” beside the named civic district, or “Kharuun public record station” beneath its proper title. World terminology can remain distinctive without making the action obscure. All copied wording here is proposed player copy, not a replacement for approved dialogue.

## Tutorial production plan — approved replacement, 2026-09-08

Owner-approved implementation sequence: bottom console and shared chapter framework, complete Chapter 1, then individually qualified Chapters 2–7, then specializations. Tutorial access is optional and strongly recommended. Remove mandatory camera exercises. Historical ten-lesson and first-run gating findings above describe the superseded design; preserve their evidence, not their active sequence. SPEC-TUT-008 is the current contract.

### Shared map and screen

Use a training-only scenario, without modifying M01 deployment or narrative consequences. Semantic locations: Home clearing (Anchor only), nearby Matter seam, connected Expansion pad, rear Assembly pad with clear exit/rally, one Defense approach and Retreat position, Scouting loop, Practice Well, and a separate independent-operation variation. Validate footprints, pathing, network distance and resource feasibility against simulation data. No unexplained initial units or enemies. No inferred protection from decorative geometry.

Use the bottom console for minimap, object information and commands. Captions and objectives remain readable without covering the world target. One current action plus purpose, visible demonstration, actual player action, specific acknowledgement. Camera help is contextual. Learned controls remain available; restrict only actions that break the current safe setup. Unrelated threats wait during teaching. Replay explanation/demonstration and the approved 1.5-second hold-to-skip remain available.

| Chapter | Ordered player experience | Completion and learning |
|---|---|---|
| 1 Establish your foothold | Welcome and commander role; inspect Anchor; demonstrate worker production; train Surveyor; explain/select worker; demonstrate gathering; assign seam; observe cargo and credited delivery; independently train/assign second worker. | Two genuinely produced workers deliver Matter. Understand headquarters, selection and continuing income. |
| 2 Build a useful outpost | Explain Matter/Dawn and Logistics as they become relevant; introduce/place/build Power Link beside work route; observe operational benefits; introduce/place/build rear Array Foundry with a clear exit. | Operational player-built Link and Foundry. Understand purpose, placement, connectivity and unfinished foundations. Invalid placement costs nothing. |
| 3 Prepare your first force | Introduce Lancer; queue one; inspect cost/capacity; set safe rally; produce three; select/move group; distinguish selection/movement/attack. | Three produced Lancers arrive at assembly. Understand reinforcement and rallying. |
| 4 Protect what you built | Explain/build connected Aegis; show network dependency; announce drill; demonstrate/perform attack-move; practice withdrawal in separate exchange; repair damaged structure and inspect cost. | Threat defeated, headquarters survives, genuine repair. Retry combat phase, not earlier chapters. |
| 5 Know what is happening | Produce/explain Relay Skiff; named discovery objective; scout; immediately acknowledge discovery; explain fog states; use spatial alert; distinguish last-known contact; return safely. | Site discovered, objective response and safe return. Camera centering earns no credit. |
| 6 Choose a future | Explain practice Well; establish control; compare real Harvest/Preserve/Reshape values; intentional choice/confirmation; announce threat; protect commitment and observe result. | Actual chosen protocol completion. No campaign doctrine changes; alternatives available in practice. |
| 7 Independent command | Fresh Anchor/resource start; explicit enemy-Core victory and own-Core defeat; establish economy/infrastructure/force, scout and win without prescribed order. Optional requested hints; phase retry. | Distinguish assisted/independent completion. Offer campaign/skirmish/replay/practice without access gates or perfect-score requirement. |

### Specializations

All independently replayable from Help: efficient command (mixed selection/groups/stop/patrol/guard); production/research (queue/reorder/refund/cancel/research occupancy/blocked exit); Meridian support (Bulwark protection, Skiff support, network recovery); Kharuun foundations (Hearth/Tender/Waystone migration/Basin/Spine); Kharuun force (Riftstalker/Cairnback/Resonant and real signature actions); Choir foundations (Concordance/Threadkeeper/Interval/Chorus/Phase Anchor/upkeep); Choir force (Intervalist/Warden/Afterimage); each Well protocol/interruption; campaign objective/save/resume/results/replay orientation. No invented signature behavior or unqualified mode introduction.

### State and verification

Stable authored chapter/step identities drive explanation, spotlight and success together. UI actions update independently of fixed ticks; authoritative receipts prove production/delivery/construction/combat. Persist chapter state separately from old ten-bit masks. Retain legacy records without credit conversion. Match resumed progress to compatible training checkpoint state. Demonstrated/skipped/verified and assisted/independent remain distinct. Chapter pacing targets 5–10 minutes, not timed gates.

For every chapter validate setup, correct/wrong/repeated input, missing target, interruption/recovery, no false credit, rendered input/DPI/platform keys, save/load/replay and integration. Then conduct uncoached beginner review of comprehension and friction. Passing observer tests is not evidence of fun or owner acceptance.

## Mission-by-mission campaign plan

The mission cards below preserve the fifteen canonical operations. “Target force” refers to the current `SPEC-PLAN-*` obligation; exact counts not specified there require a proposed deployment manifest and tuning, not invented authority. Each card’s encounter, staging, and feedback recommendations are proposed design work. Canonical required objectives, protected actors, failure semantics, branch sites, and hold durations remain controlled by its corresponding `SPEC-MSN-*` contract and mission model.

For every card, use a safe orientation, a first comprehensible task, an escalation with an authored cause, a commitment or coordination challenge, and a result that explains the next operation. The phase structure can vary. A map need not contain a base, Well, research structure, or enemy Core unless its contract or tactical design requires one.

### Act I — Necessary Fires

#### M01 — What the Ledger Keeps

**Purpose and place.** Mara commands the evacuation margin of Glass Scar. Recover the archive convoy that the ledger has prematurely closed, decide the Well’s future, and withdraw the carrier. Oruun’s concern supplies a competing claim; it does not establish modeled cavern casualties. Authority: `SPEC-MSN-001`, `SPEC-PLAN-001`; [mission source](../Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json), [map source](../Content/World/Source/Campaign/m01_glass-scar-evacuation-margin_v1.json).

- **Required force and space:** Anchor, six Surveyors, two Lancers, Bulwark, Relay Skiff/identified archive-carrier role. Explain any prebuilt Foundry, drop-off or utility asset rather than treating it as roster filler. Give the base, archive approach, Well approach and withdrawal corridor distinct readable functions. Preserve the carrier’s identity; Mara is command authority, not that unit.
- **Human journey:** establish a useful economy and protection group; take the carrier to Archive Recovery; leave it holding there while a worker reaches the Well; compare and commit one protocol; protect the commitment; withdraw the carrier to the bound evacuation site. Current sites are recovery `(22,18)` and evacuation `(6,17)`; these are authoring coordinates, not player instructions.
- **Automation and feedback:** acknowledge recovered records; display carrier-held and worker/control prerequisites; execute actual protocol timing; mark the resulting route/resource state; expose withdrawal only when appropriate. Show exactly why a choice is unavailable. Do not issue the carrier’s orders automatically.
- **Tactical design:** the decision is how much protection to leave with the carrier versus commit at the Well, how to fund the chosen response, and how to preserve the withdrawal route. Reshape needs real traversability value; Harvest and Preserve need visibly different consequences. All three must support a valid mission completion.
- **Success, failure, and recovery:** success is the recorded Well decision plus carrier evacuation. Protect Core/carrier and satisfy the exact Well-control contract. Qualify hostile-Core destruction separately so it cannot replace evacuation or create an unintended loss. Checkpoint before the commitment; a failed attempt writes no founding doctrine.
- **Learning and consequence:** first independent use of controls, economy and force protection; the founding Harvest/Preserve/Reshape doctrine changes subsequent mission plans. Optional target: save both outer reserve stations for the specified later Meridian worker reward; verify that real sites and reward consumption exist before offering it.
- **Verification:** all three protocols, missing/moved carrier, worker loss/replacement, contested/expired Well where applicable, withdrawal, each protected loss, save/retry and M02 transition. Resolve the extraction/Reshape decision records before declaring the tactical design complete.

#### M02 — Seven Accounts of Rain

**Purpose and place.** Oruun must move the assembly through Shivergrass Basin, where inherited accounts disagree with current terrain. The result establishes a held route and a recall, not which account is finally true. Authority: `SPEC-MSN-002`, `SPEC-PLAN-002`; [mission](../Content/Narrative/Source/missions/m02_seven_accounts_of_rain.json), [map](../Content/World/Source/Campaign/m02_shivergrass-migration-basin_v1.json).

- **Required force and space:** Memory Hearth, six Tenders, two Riftstalkers, Resonant, and the mission-bound Waystone/bearer roles. Reconcile the plan with the current generic force and protected Waystone. Build a migration route with staging ground, a rooted roadhead and account site; branch terrain must make the inherited doctrine understandable.
- **Human journey:** inspect present ground and vibration contacts; protect the bearer; uproot/move/root the Waystone at the inherited anchor; reach the corresponding account site with the bearer; hold the route as required. Teach faction differences before the move.
- **Automate:** Waystone state transitions and ordinary routing after commands; validate correct rooted site and bearer; show which inherited account determined the route and what was observed now. Do not select the route doctrine anew.
- **Meaningful pressure:** expose the risk of moving support before a screen is established. Give detection information an actual route or defense use. Optional seven-marker verification must add comprehensible observations rather than seven unrelated collectibles.
- **End/recovery:** keep Core, bearer and Waystone alive; restore the last checkpoint on failure without writing a recall. Show the campaign-only vibration-contact reward only when its optional proof exists. Test all three inherited routes, wrong root site, uprooting, bearer/Waystone loss and M03 handoff.

#### M03 — A City on Reserve

**Purpose and place.** Mara reconnects Lume Reach’s service districts. One restored interface is insufficient to stabilize a distributed network. Authority: `SPEC-MSN-003`, `SPEC-PLAN-003`; [mission](../Content/Narrative/Source/missions/m03_a_city_on_reserve.json), [map](../Content/World/Source/Campaign/m03_ark-city-reserve-service_v1.json).

- **Required force and space:** Anchor, seven Surveyors, three Lancers, Bulwark, Skiff; Life Support, Transit and Archive interfaces with readable service routes and defense positions. Each district needs a recognizable civic function and a distinct approach.
- **Human journey:** read the inherited restoration order; choose economical link placement; assign construction/repair teams; screen the work; restore all three in order while keeping the reserve chain viable. Harvest doctrine prioritizes Life Support, Preserve prioritizes Archive, Reshape prioritizes Transit; show the whole prescribed sequence without presenting it as a new choice.
- **Automate:** power propagation and phase recognition, network overlays, disconnected-load warnings and repair progress. Explain whether completed districts must remain powered; bind that explanation to the exact contract.
- **Tactical decision:** workers and defenders cannot cover all sites equally. Terrain should support choices between short exposed links and defensible routing within the existing power rules. Avoid making success three adjacent clicks at a safe base.
- **End/recovery:** protect Core and all three district interfaces; do not claim the entire city or its population has been restored. Optional continuous-link condition earns the specified faster campaign link construction. Test out-of-order work, broken earlier connections, district loss, optional-history tracking and M04 transition.

#### M04 — The Unburied Road

**Purpose and place.** Oruun moves infrastructure through contested subsurface vaults to recover a missing memory shard. Recovery reveals an omission; it does not identify who buried it or why. Authority: `SPEC-MSN-004`, `SPEC-PLAN-004`; [mission](../Content/Narrative/Source/missions/m04_the_unburied_road.json), [map](../Content/World/Source/Campaign/m04_unburied-road-vaults_v1.json).

- **Required force and space:** Hearth, seven Tenders, three Riftstalkers, Cairnback, Resonant and the protected bearer/Waystone. A constrained transit passage, defensible roadhead, Listening Spine site and separate reliquary make mobile support useful.
- **Human journey:** scout the inherited road; move and root the Waystone at its roadhead; raise the correct Listening Spine at the vault site; escort the bearer to recover the shard. Introduce applicable adaptation and Mineral Cover through a safe tactical comparison before relying on them.
- **Automate:** movement, rooting/construction, support-state updates and recovery acknowledgement. Validate intended faction/building role instead of crediting an unrelated utility object simply because its generic enum matches.
- **Tactical decision:** select an adaptation/escort arrangement and decide when to move infrastructure without abandoning protection. Place resources along the operation’s support problem, not a copied base layout.
- **End/recovery:** protect Core, bearer and Waystone; checkpoint before the committed move. Optional no-rooted-Waystone-loss reward reduces the first adaptation in each later Kharuun mission by the required 10 Dawn. Test every route, wrong structure/site, infrastructure interruption, protected loss and recovered-shard result.

#### M05 — Terms of Continuance

**Purpose and place.** Mara holds the Line of Parity so two networks can carry witnessed ceasefire terms. Reading the terms does not sign them or identify the source of the third pressure. Authority: `SPEC-MSN-005`, `SPEC-PLAN-005`; [mission](../Content/Narrative/Source/missions/m05_terms_of_continuance.json), [map](../Content/World/Source/Campaign/m05_terms-of-continuance-corridor_v1.json).

- **Required force and space:** Meridian detachment, separately represented Kharuun network/witness presence, neutral corridor, two defensible network positions and a common extraction route. Correct the current local-Meridian proxies for the supposed Kharuun Spine/witness before visualizing a two-party operation. Preserve Meridian command authority.
- **Human journey:** establish both network connections before the published window; divide guards to maintain synchrony; protect both witnesses through the reading; then escort both to extraction. Each network and witness must be named and selectable/inspectable according to its true ownership.
- **Automate:** synchronized-state display and the authored window. Current source uses absolute ticks 300–900; communicate the setup deadline, window start and remaining time. Losing synchrony after the window starts is irreversible in the current contract; late repair must not falsely restore success.
- **Tactical decision:** maintain two fronts and a withdrawal reserve under announced pressure. Encounter design must establish why attackers threaten the corridor without attributing them to an unrevealed actor.
- **End/recovery:** protect Core, both networks and witnesses; a compromised window needs a clear failure reason and pre-window retry. Early extraction earns no completion. Optional undamaged networks add evidence/dialogue, not invented combat power. Test preparation deadline, each disruption/loss, both extractions and the bounded M06 debrief.

### Act II — The Cost of One Future

#### M06 — Names Without Births

**Purpose and place.** Talar leads Meridian proxies through Sector 9 to locate erased census evidence, power its archive, shelter two civilians and extract. Authority: `SPEC-MSN-006`, `SPEC-PLAN-006`; [mission](../Content/Narrative/Source/missions/m06_names_without_births.json), [map](../Content/World/Source/Campaign/m06_names-without-births-district_v1.json).

- **Required force and space:** small mobile reconnaissance/protection force, Talar, two clearly identified civilian proxies and the census archive. Remove the generic starting production package after reconciling all Core/failure dependencies. Use broken district sightlines, shelter space and an extraction approach to make avoidance meaningful.
- **Human journey:** locate the census with Talar; provide the archive’s required power; move each civilian to shelter; extract Talar with the established evidence. The optional census fragments must be attributable parts of that task.
- **Automate:** state observation and a visible evidence/shelter checklist. The current source lets archive power imply census discovery. Reconcile that shortcut with the intended discovery step; do not show “Talar found it” without an attributable event.
- **Tactical decision:** choose safe approach, escort coverage and which optional evidence risks are affordable with limited force. Resource provision should fund the required connection/recovery, not enable rebuilding a large army that erases the constraint.
- **End/recovery:** protect Talar, archive, both civilians and any retained contractual Core; show the exact missing shelter/evidence condition before extraction. Failure writes no record. Verify power-first ordering, separate civilian arrival/loss, blocked extraction, optional fragment receipt and the Archive-candidate reward’s actual effect on M09 eligibility.

#### M07 — The Shape of Silence

**Purpose and place.** At Listening-Spine Ridge, Oruun compares erased census entries with communal-memory omissions through paired observation. Authority: `SPEC-MSN-007`, `SPEC-PLAN-007`; [mission](../Content/Narrative/Source/missions/m07_the_shape_of_silence.json), [map](../Content/World/Source/Campaign/m07_listening-spine-ridge_v1.json).

- **Required force and space:** Kharuun scouting/mobile infrastructure force, Oruun, two distinct witnesses, Waystone and a correct Listening Spine. A ridge with separated observation flanks and a final confluence must be visibly different from M04’s transit corridor.
- **Human journey:** secure/root the inherited anchor; build the Spine; place each witness at its separate observation; bring Oruun to the confluence while maintaining the required protections.
- **Automate:** detect rooting, valid apparatus and separate witnesses; display two independently sourced observations and their bounded correspondence. One witness visiting both sites cannot substitute for two required simultaneous actors where the contract requires separation.
- **Tactical decision:** distribute detection and protection across the ridge; maintain observation rather than merely transporting a bearer. The evidence payoff must differ from M04’s recovered object.
- **End/recovery:** protect Core, Oruun, Waystone and both witnesses. Verify missing/displaced witness, wrong/destroyed Spine, all route variants and exact persistence. Optional untouched neutral records affect ending context only as authorized; observation must not claim attribution, consent or a resolved memory dispute.

#### M08 — The Shape Beside Us

**Purpose and place.** Talar establishes bounded reciprocal contact with Neme at the Confluence verge. Command remains Meridian; contact is not control of a Choir army. Authority: `SPEC-MSN-008`, `SPEC-PLAN-008`; [mission](../Content/Narrative/Source/missions/m08_the_shape_beside_us.json), [map](../Content/World/Source/Campaign/m08_reciprocal-contact-verge_v1.json).

- **Required force and space:** Meridian proxy force, Talar and paired-state witnesses; no playable Choir base. Give the echo, relay, state traversals and convergence different spatial/visual roles, using readable state cues rather than decorative doubling.
- **Human journey:** observe the first echo with Talar; raise the required relay; direct each witness through its assigned state; reach convergence with Talar after reciprocal conditions are established.
- **Automate:** observe and retain distinct traversal facts, publish state feedback and the contact result. Current relay completion can imply the first observation. Resolve that bypass so the player experiences the contact premise before performing its infrastructure step.
- **Tactical decision:** coordinate routes that carry different observations while protecting the witnesses. This mission’s challenge is reciprocal state coordination, not another supply-chain repair.
- **End/recovery:** protect Core, Talar and both witnesses. Checkpoint before traversals; explain which state is missing. Test relay-first behavior, each missing/incorrect traversal, actor loss and convergence. Optional non-destruction of neutral defense grants only the specified extra M15 dialogue path. Do not imply Neme is permanently unified or that full Choir command has unlocked.

#### M09 — Reserve Authority

**Purpose and place.** Mara allocates reserve power at the Authority Exchange: exactly two of Life Support, Transit and Archive receive power; the remaining district becomes an explicit deferred liability. Authority: `SPEC-MSN-009`, `SPEC-PLAN-009`; [mission](../Content/Narrative/Source/missions/m09_reserve_authority.json), [map](../Content/World/Source/Campaign/m09_reserve-authority-exchange_v1.json).

- **Required force and space:** full Compact base/roster as required, Mara, three distinct civic fronts and their interfaces. Reconcile “full” into explicit buildings, tech access and budget. The third district remains physically intact and protected; its lack of power is not an assertion of casualties.
- **Human journey:** reach the inherited authority site; inspect the district comparison and any earned eligibility; decide the pair; build the two connections; defend the city’s required apparatus; take Mara to acknowledge the deferred district.
- **Automate:** compute the powered count, show power reach before placement and bind the deferred district to the record. Do not select the two districts for the player. Warn before an action would violate the exact-two rule; do not silently weaken the underlying failure contract.
- **Tactical decision:** protect two distributed investments while retaining the third district. The player must understand the liability before it becomes irreversible. Any proposed new in-mission district benefits must first be authored and approved; existing downstream meaning alone does not justify invented bonuses.
- **End/recovery:** current failures include more than two powered, Mara/Core loss and loss of a district. Test all valid pairs, a third power connection, changing connections, protected losses and reload before/after commitment. M10’s liability and M15’s hold duration must display the recorded outcome consistently.

#### M10 — The Choir at Lume Reach

**Purpose and place.** Oruun returns to Lume Reach to establish contact, work at the inherited liability and commit a new Well protocol. Mara is an off-map liaison; public Choir apparatus is not a controllable third faction. Authority: `SPEC-MSN-010`, `SPEC-PLAN-010`; [mission](../Content/Narrative/Source/missions/m10_the_choir_at_lume_reach.json), [map](../Content/World/Source/Campaign/m10_lume-reach-liability-district_v1.json).

- **Required force and space:** full Kharuun base/roster, Oruun, protected Waystone, liability interface, two sequential Spine sites and a separate Well court. Make civic and Kharuun structures visibly meet through their respective interfaces.
- **Human journey:** reach the inherited contact; root the Waystone at the deferred liability; construct the first then second Spine; compare and commit a Lume Well protocol; bring Oruun to its corresponding resolution site within the applicable window.
- **Automate:** track contact, liability and both construction stages separately; disclose prerequisite/early-commit failure before interaction; apply chosen protocol and record it independently from M01’s doctrine.
- **Tactical decision:** divide a supported force between the liability, advancing contact infrastructure and the Well. This is a new choice in a known civic context, not an unexplained repetition of the first Well.
- **End/recovery:** protect Core, Oruun, Waystone and Well; early commitment or expired required Reshape resolution fails under the current model. Test each founding doctrine × deferred district × Lume choice, legitimate setup order, interruption/expiry, reload and M11 transfer. Outcome must not claim a restored neighborhood or quantified population benefit.

### Act III — Crownfall

#### M11 — No Neutral Ledger

**Purpose and place.** Oruun and a distinct witness make the inherited coalition record inspectable on public interface ground. Route, district pair and Lume protocol come from prior play. Authority: `SPEC-MSN-011`, `SPEC-PLAN-011`; [mission](../Content/Narrative/Source/missions/m11_no_neutral_ledger.json), [map](../Content/World/Source/Campaign/m11_public-coalition-interface_v1.json).

- **Required force and space:** full Kharuun base, Waystone, Oruun and separate witness; inherited approach, two district connections, separate public evidence sites and protocol rally. Distinct lanes and safe withdrawal paths must make distributed protection practical.
- **Human journey:** establish the inherited route; connect the recorded two district interfaces; split Oruun/witness across the public evidence sites; apply the recorded Lume protocol; rally both after the attestation. The protocol here is an inherited obligation, not a new moral choice.
- **Automate:** load the exact ten-record history and display why these sites/protocol apply. Credit separate attestation, reject conflicting/early protocol behavior according to the contract, and show which required element is missing.
- **Tactical decision:** sustain a distributed public operation with limited simultaneous protection. Use route shape and attack access to make allocating forces matter; do not pad the mission with extra markers.
- **End/recovery:** protect required Core, characters and interfaces; protocol misuse is irreversible under the current model. Qualify all 27 doctrine × liability × Lume plans, failure paths and timed Reshape cases. The optional simultaneous-interface achievement can shorten M12 only through the specified recorded reward. No mixed command, faction merger, trust score or invented consent.

#### M12 — The Future That Won

**Purpose and place.** Oruun and a verifier test Rhyse’s public apparatus at the Demonstrator Spine. The operation demonstrates a bounded readback and hold, not the truth of a promised future. Authority: `SPEC-MSN-012`, `SPEC-PLAN-012`; [mission](../Content/Narrative/Source/missions/m12_the_future_that_won.json), [map](../Content/World/Source/Campaign/m12_public-readback-demonstrator_v1.json).

- **Required force and space:** full Kharuun base, Oruun/verifier, independent Meridian/Kharuun readbacks, the two recorded inputs, demonstrator and Well. Preserve independent observation lanes and a defendable shared apparatus.
- **Human journey:** send separate readers to the two public records; connect the correct district inputs; bind the recorded protocol; protect the activation for the required 300-tick hold, applying only an authorized recorded modifier; then complete the two input observations.
- **Automate:** show each independent readback and input connection, activation readiness, remaining hold and interruption state. Maintain attribution to Rhyse’s apparatus without making Rhyse a commanded unit.
- **Tactical decision:** prepare defense before activation, divide coverage across inputs, and maintain observation as pressure changes. Differentiate this experiment from M11’s assembling/rallying operation through defense geometry and visibly changing apparatus state.
- **End/recovery:** protect readers and required apparatus; wrong protocol, broken required activation conditions or protected loss follow the exact failure rules. Test distinct-reader enforcement, wrong pair, early observation, hold interruption, optional-link history, save during activation and M13 transfer. Never label the result proof of a restored city or permanent future.

#### M13 — Assembly of the Missing

**Purpose and place.** Oruun and the verifier bring separate public records to the Crownfall index and witness an assembly. The evidence does not authorize identifying victims, assigning blame or counting survivors. Authority: `SPEC-MSN-013`, `SPEC-PLAN-013`; [mission](../Content/Narrative/Source/missions/m13_assembly_of_the_missing.json), [map](../Content/World/Source/Campaign/m13_crownfall-public-index_v1.json).

- **Required force and space:** mobile Kharuun force with construction support, both observers, protected public interfaces and index, a correct Listening Spine and two observation sites. Replace the generic starting base with the intended mobile-support package through contract reconciliation.
- **Human journey:** read separate Meridian/Kharuun records; establish the index link within the authored radius; position the two witnesses at their separate observation sites; maintain protected apparatus until the actual result is recorded.
- **Automate:** link/radius validation, separate record and witness status, and a precise public-result acknowledgement. Retain those identities through save/load.
- **Tactical decision:** protect public apparatus while moving a constrained force and its observers. Make the index precinct’s access and protected-fire lanes distinctive; avoid replaying M12’s power-activation loop with renamed buildings.
- **End/recovery:** Core/observer/required-interface loss fails according to the contract. Test radius boundaries, wrong building, observations before linkage, witness separation and protected apparatus destruction. Optional public-apparatus preservation reduces the first M15 Phase Anchor cost only if the reward is recorded and consumed. No invented names, culpability, consent or cryptographic-authenticity claim.

#### M14 — Several Voices, One Command

**Purpose and place.** Neme leads playable Choir forces at the command-crisis basin, maintaining incompatible identities within one bounded command. Authority: `SPEC-MSN-014`, `SPEC-PLAN-014`; [mission](../Content/Narrative/Source/missions/m14_several_voices_one_command.json), [map](../Content/World/Source/Campaign/m14_crownfall-command-crisis_v1.json).

- **Required force and space:** Concordance, Threadkeepers, limited Choir force, the designated Possible line unit, Manifest heavy, Neme, research loom and crisis-anchor site. Provide a safe economy/identity lesson before requiring the first crisis response; “MICRO” does not excuse omitting the required Choir systems.
- **Human journey:** research Held Alternatives; establish the designated Possible and Manifest states at their separate sites; position Neme; research Shared Resolution; construct the Phase Anchor only when ready; protect the entire contract for 160 fixed ticks.
- **Automate:** correct identity/coherence/upkeep calculations, prerequisite status, anchor readiness and hold progression. Show the reason early construction is invalid before a player accidentally commits it.
- **Tactical decision:** keep different roles solvent and protected across separated sites. Make upkeep and identity useful tactical constraints rather than a prescribed hotkey recipe.
- **End/recovery:** a breach after hold start is irreversible; repairing afterward does not erase failure. Checkpoint before anchor commitment. Test early anchor, each identity/actor/site breach, insufficient upkeep, post-breach repair, duration boundary and M15 handoff. Optional solvent temporary structures grant the specified +10 M15 Dawn. The result is not permanent personality unification.

#### M15 — The Broken Sun

**Purpose and place.** At the Solar Fall Dais, Neme establishes three witnessed accords and holds one earned resolution. Mara, Oruun and Talar are protected neutral witnesses. Authority: `SPEC-MSN-015`, `SPEC-PLAN-015`, `SPEC-CAM-007`, `SPEC-END-001..004`; [mission](../Content/Narrative/Source/missions/m15_the_broken_sun.json), [map](../Content/World/Source/Campaign/m15_broken-sun-accord-dais_v1.json).

- **Required force and space:** full Choir base, required worker/line/heavy/Neme roles, all protected witnesses, approach anchor, three distinct accord sites and a separate Resolution Conduit site. Make the three cultures recognizable without granting control of neutral witnesses.
- **Human journey:** secure the inherited approach and anchor; establish Possible, Manifest and Neme positions for the accords; inspect earned endings and eligibility reasons; select and confirm one ending twice; construct its distinct conduit; defend every required element through the final hold.
- **Automate:** compute eligibility from the real record, apply district/ending hold duration and valid cost rewards, show witness and conduit status, observe continuous protection, commit only the selected result, then present the ending/archive/menu path.
- **Tactical decision:** prepare reserves and coverage for three protected sites before committing to the chosen cost. The decision panel informs; it must not choose a “best” ending or narrate unearned alternatives. The finale assesses previously learned tools and introduces no unexplained command.
- **End/recovery:** protections apply at the stages specified by the contract, with immediate protected-actor obligations retained. A broken final hold cannot be repaired retroactively. Test every earned ending, unavailable selection, single/double confirmation, each protected failure, wrong/early conduit, hold boundary, save during commitment, storage failure and replay isolation. End at the authorized archive/title boundary; there is no Mission 16.

| Ending | Existing eligibility | Existing extra hold | What the player is deciding |
|---|---|---|---|
| Restoration | Preserve at Lume Reach and Life Support powered | +80 ticks | Return held futures slowly under guard. |
| Controlled Stabilization | Always eligible | +0 ticks | Maintain Crownfall as a managed wound with unresolved long-term cost. |
| Extinguishment | Harvest founding doctrine or Lume protocol | +40 ticks | Spend remaining held futures to close Crownfall. |
| Open Evolution | Reshape founding doctrine or Lume protocol | +120 ticks | Permit continued becoming without central control or guaranteed outcome. |

The final base hold is 320 ticks for deferred Life Support, 280 for Transit, and 240 for Archive, as mapped by [EchoesBrokenSunMissionModel.cpp](../Source/EchoesOfTheBrokenSun/Private/EchoesBrokenSunMissionModel.cpp); add the chosen ending’s modifier and show the resolved duration before confirmation. The result must name the actual deferred liability and selected resolution without inventing a universal happy ending.

### Campaign pacing and transition discipline

The current plan targets 20–25 minutes for MICRO, 25–30 for HYBRID, 30–35 for M09/M11 MACRO, and 35–45 for M15. These are requirements/planning targets, not measured play durations. At a 20 Hz simulation, 160/300/600-tick holds are 8/15/30 seconds: they cannot by themselves produce a meaningful twenty-minute mission. Playtime must come from reconnaissance, positioning, construction, threats, meaningful commitment and recovery—not artificial waiting or an enlarged walk between identical tasks. If satisfying a mission’s canonical actions produces substantially less play, propose a duration or encounter correction with evidence instead of padding it.

Before each deployment, show: current region, commander/faction, the immediately relevant prior decision, today’s job, protected assets, available force, victory/loss and any irreversible deadline. Afterward, show completed required/optional actions, what was actually established, the recorded cost/liability, reward effect and why command moves to the next operation. Branch variants should alter tactical conditions and dialogue only where the source supports them. A narrated fact does not create a hidden gameplay modifier.

Optional objectives must support the main operation and change a later decision through an implemented reward. Verify exact eligibility, recording, caps and later consumption. In particular, reconcile M06’s Archive-candidate unlock with M09’s default district options; an “unlock” that does not change availability is misleading. Do not create reward popups for a multiplier or dialogue branch that has no consumer.

### Roster and building inclusion rules

“Full base/roster” must become an explicit source manifest, not an excuse to expose every button immediately. The following maps the existing roster to mission purpose. It defines why an element may be available; starting counts, research access and unlock state remain mission-specific. Production availability does not imply the player starts with every unit or building.

| Faction | Units and reason to include | Buildings and reason to include | Application |
|---|---|---|---|
| Meridian | Surveyor: economy/construction/repair. Lancer: ranged protection and reinforcement. Bulwark: a directional screen for workers, carrier or links. Relay Skiff: scouting/support; M01’s bound carrier has an archive cradle. | Anchor: command/economy root. Power Link: network reach and Logistics. Array Foundry: replenishment/composition. Aegis Post: supplied static defense with an actual approach to protect. | Teach in readiness; practice M01/M03; constrained detachment M05/M06/M08; broader availability M09. Give each defensive asset a useful facing/route. |
| Kharuun | Tender: economic/construction support. Riftstalker: mobile protection. Cairnback: heavy screen and applicable cover/adaptation. Resonant: scout/contact information, with bearer/Oruun roles explicitly bound where used. | Memory Hearth: economic root. Waystone: mobile support and rooted mission anchors. Growth Basin: replenishment/adaptation. Listening Spine: detection or explicitly bound mission apparatus. | Introduce M02; mobile practice M04/M07; broaden for M10–M12; constrain support in M13. Vibration signatures must not be narrated as confirmed unit identities. |
| Choir | Threadkeeper: economy and coherence support. Intervalist: declared Possible/Manifest role. Lacuna Warden: heavy/control role. Afterimage: scouting/deception; named Neme binding remains distinct from generic replacements. | Concordance: command/economy root. Interval Loom: supply/drop-off with upkeep. Chorus Loom: production/research. Phase Anchor: coherence support and specifically bound crisis/approach/conduit objectives. | Preview contact vocabulary M08; actual playable introduction M14; full application M15. Explain upkeep, identity, scope and counterplay before using them as failure conditions. |

These are requirement/canon roles, not a declaration that every listed behavior currently works. Check the action, cost, invalid state, AI interaction and save/replay contract for each before including it. A mission-specific public apparatus or character proxy is not interchangeable with a generic buildable structure or disposable scout. Replacing a missing protected actor with a newly trained unit must not silently satisfy its identity predicate.

## Conquest: purposeful variation across 25 sectors

The approved mode is separate from the fifteen-operation story: `REL-CAM-033..038` requires a 25-sector strategic map, supply connections, disclosed anomalies, run-scoped blueprints, deterministic seeds, enemy counterattacks, territory loss and home-base defeat. The source inventory found no dedicated Conquest runtime/sector registry in the inspected Source and narrative/world source paths. Treat it as a required design/implementation workstream, not a currently playable campaign.

The mode’s player loop should be **inspect the frontier → choose a strategic reason to attack → plan for the sector’s known rule → command the battle → decide what to develop/defend → see the enemy response**. Automated AI turns and generated modifiers should create a decision context; the player chooses invasion, defense, investment and acceptable risk.

The 25 entries below are **proposed functional content slots**, not existing sector names, canon geography, approved new rewards, or a compulsory linear route. Seeded topology assigns the slots under validated supply/adjacency rules. Each slot has one primary purpose; local optional tasks must directly support it. Attack-battle completion should reuse Corefall unless an explicitly authored alternate contract is adopted; defensive holds need their own complete start, duration, interruption and loss rules before implementation.

| Slot | Strategic purpose and learning | Required battlefield content | Human accomplishment |
|---|---|---|---|
| C01 | Home base; understand the run-loss condition. | Defensible faction home, required economic root, clear neighboring fronts. | Maintain the home Core and an operational defense; its loss ends the run. |
| C02 | Safe first expansion; connect tactical victory to strategic progress. | Simple frontier, one clear hostile Core, accessible economy. | Scout, fund a force, win and verify the new sector/supply connection. |
| C03 | Matter-rich approach; decide whether economy is worth exposure. | Legible deposits with vulnerable transport routes. | Protect income while advancing on the hostile Core. |
| C04 | Supply junction; learn connectivity. | A junction whose strategic neighbors and supply effect are displayed. | Secure it and keep the front connected; no hidden supply bonus. |
| C05 | First defensive operation; prepare before an attack. | Existing forward fortifications derived from actual sector development. | Deploy protection and satisfy the disclosed defense contract or knowingly forfeit. |
| C06 | Constrained approach; use scouting and composition. | Narrow attack lane with a readable alternate approach where allowed. | Defeat a known defensive position without sacrificing the economy. |
| C07 | Exposed wide front; manage a reserve. | Two separated approaches with room for maneuver. | Protect the base while concentrating enough force to win. |
| C08 | Well investment; weigh immediate and sustained value. | Defensible Well, meaningful protocol effects and alternate economic route. | Choose a protocol for a stated battlefield reason and turn it into victory. |
| C09 | First technology prize; understand run-only rewards. | Authored blueprint reward and its exact effect shown before battle. | Win, claim the permitted blueprint and see its scope in the run ledger. |
| C10 | Forward defensive hub; plan for counterattack. | Developed defenses, attack warning and supply exposure. | Hold or forfeit deliberately; inspect the resulting frontier. |
| C11 | Accelerated Well Bleed; adapt to faster Well timing. | Approved 2× Well modifier, explicit affected clocks and comparison display. | Adjust commitment/defense timing and win without guessing the altered rule. |
| C12 | Atmospheric Scarcity; prioritize a reduced budget. | Approved −25% starting Matter and reachable recovery economy. | Choose an affordable opening and achieve the declared battle objective. |
| C13 | Seismic Fractures; respond to disclosed hazards. | Seeded tremors, authored valid warning/affected areas and recovery rules. | Move and protect the force through hazards while completing the battle. |
| C14 | Solar Flare; distinguish temporary revelation from lasting knowledge. | Periodic fog change with start/end feedback and explicit information scope. | Use the reveal window without treating expired visibility as current knowledge. |
| C15 | Second technology option; choose a different investment route. | A different approved blueprint or approved alternative reward; visible opportunity cost. | Select this frontier for its actual later benefit and win it. |
| C16 | Route to an enemy stronghold; plan staging. | A defended approach with suitable reinforcement space and visible strategic adjacency. | Secure a staging position for the stronghold attack. |
| C17 | Alternate stronghold approach; make route choice matter. | Different attack access and resource exposure from C16. | Choose a tactically different approach with the same known strategic purpose. |
| C18 | Frontier under divided pressure; prioritize fronts. | Two threatened connections and a truthful enemy-turn forecast. | Defend the more important front or accept an explained territory loss. |
| C19 | Recapture operation; apply lessons from a loss. | Lost territory with retained development according to the adopted persistence rule. | Reclaim it and verify exactly what development/supply was retained or lost. |
| C20 | Third technology opportunity; complete a chosen run strategy. | Remaining approved faction blueprint with explicit magnitude and stacking rule. | Earn a benefit that supports the intended stronghold assault. |
| C21 | First enemy stronghold; demonstrate combined systems. | A full authored enemy defense and fair-information reinforcement rules. | Destroy the required Core(s); strategic elimination effects require an adopted contract. |
| C22 | Second enemy stronghold; test another defensive pattern. | A different layout/composition tied to its faction’s actual mechanics. | Adapt composition and route, then achieve the declared Corefall result. |
| C23 | Exposed late frontier; resist overextension. | Long supply access, clear counterattack pressure and enough recovery space. | Decide whether to extend, consolidate or defend before a final push. |
| C24 | Final approach; spend earned advantages deliberately. | Route whose modifiers and defensive composition reward the chosen run strategy. | Establish the attack position and retain enough force/economy for completion. |
| C25 | Run culmination; demonstrate and explain victory. | The adopted final strategic condition, declared enemy objectives and home-risk context. | Satisfy the run-victory contract; receive an accurate dossier and eligible commemorative unlocks. |

**Decisions needed before Conquest implementation:** exact whole-run victory; which sector roles confer which strategic effects; the 25-node topology/adjacency constraints; defensive hold contract; supply disruption behavior; loss/recapture development retention; blueprint availability, magnitude and stacking; what “2× Well timing” affects; tremor warnings, duration and damage/displacement rules; flare information scope; and seed format/version compatibility. Recommended run victory is defeat the designated enemy strongholds while retaining home. Full territorial control can be an alternative, but neither should be silently inferred as approved from the current high-level requirement. These are bounded P6 design decisions, not reasons to delay tutorial/M01 work.

Randomness is acceptable only inside a disclosed authored rule. Generate layout, faction placement and modifier assignment deterministically from the eight-character seed; save the generator/version and run state needed for replay. Validate home access, supply connectivity, required actors, reachable victory and viable starting economies before deployment. Do not combine modifiers that make a required objective impossible. Never randomize a mission’s stated objective after commitment without an authored warning and recovery path.

Conquest orientation should teach one invasion, one supply consequence, one counterattack/forfeit, one modifier, one run-scoped blueprint and the home-loss condition. Present later modifiers when encountered, with on-demand practice. Run loss may preserve only authorized profile/cosmetic records; no blueprint bonus may leak into story, normal skirmish or multiplayer without its own authority.

## Implementation order and ownership

One integration owner retains shared controller, simulation and save authority. Narrative/map specialists may prepare disjoint source assets once their functional contracts are stable. Serialize builds, game launches, rendered review and physical-input sessions with the active owner. Existing dirty work and the ongoing integration sequence take precedence over starting another implementation lane from this plan.

| Work package | Concrete work and source ownership | Dependency and exit |
|---|---|---|
| **J1 — Reconcile the player contract** (within P3 planning) | Integration/design owner maps lesson IDs, active script, skip/mastery paths, mission deployment requirements, inherited objective semantics and known conflicts. Record decisions in RequirementsState; correct the existing master/script/map references in place only within authorized scope. | First. Exit: no lesson requires an untaught action; each mission has a purpose/deployment/capability manifest; each unresolved semantic decision is explicit. This document supplies the reviewable starting content. |
| **J2 — Shared lesson/objective authoring** (P3) | Extend registered source/schema/compiler paths for lesson substeps, allowed actions, actor bindings, objective states, feedback, recovery and capability flags. Reuse existing reducers and world/narrative compilers. Integrate through `EchoesPlayerTutorial`, field HUD, player flow and simulation interfaces. | J1. Exit: a complete lesson can be authored and played through success, wrong input, skip, retry and save/resume without hard-coded coordinate instructions. Generated outputs remain generated. |
| **J3 — Finish readiness** (P3) | Complete lessons 6–10 and all required substeps in 1–5; wire the independent assessment and individual replay; resolve current bindings; integrate approved voice/subtitles and recovery. | J2 plus the current P1/P2 foundation. Exit: fresh-profile end-to-end readiness and real AI entry work through ordinary controls, with no injected mastery. |
| **J4 — Qualify M01→M02** (P3/P4) | Reconcile roster/source, extraction reference and useful Reshape geometry; finish carrier/Well/withdrawal choices, purposeful deployment/encounters, debrief and M02 handoff. | J1–J3 and existing P4 provenance gates. Exit: every Well route, failure/retry/save path and optional reward; representative human/owner evidence. Do not start expensive campaign-wide art as a substitute. |
| **J5 — Act I production** (P5) | Build M02–M05 manifests, distinct resource/route/encounter layouts, Kharuun transition lesson, multi-site grid, mobile support and synchronized ceasefire. Fix M05 faction representation. | Representative method qualified. Exit: four independently playable missions plus a continuous M01–M05 journey with correct recorded consequences. |
| **J6 — Act II production** (P5) | Build M06–M10; remove unjustified generic bases, reconcile observation shortcuts, teach contact/allocation, implement optional reward effects, and distinguish the second Well decision. | J5. Exit: three district-pair paths and all founding/Lume protocol combinations covered, with purposeful maps and valid loss/recovery. |
| **J7 — Act III production** (P5) | Differentiate M11–M13’s coordination problems and layouts; introduce full Choir learning at M14; complete M15 eligibility, two confirmations, conduit/hold, results and ending delivery. | J6 plus complete required roster abilities. Exit: continuous campaign and all four earned endings through normal controls; no unsupported narrative outcomes. |
| **J8 — Conquest contracts and run** (P6) | Mode owner resolves the decisions above, creates the 25-sector registry/generator, mission-role bindings, enemy turn rules, run storage, blueprint isolation, onboarding and results. | Stable offline foundation; disjoint planning may prepare earlier. Exit: reproducible complete runs, territorial/home loss, counterattacks, all modifiers, saves and reward isolation. |
| **J9 — Journey qualification** (P4/P7 as applicable) | QA integrates source identity, content checks, package, normal controls, audio/visual evidence, accessibility, complete playthrough and owner review. | Each deliverable’s applicable predecessors. Exit: exact required evidence; one accepted map never qualifies the other fourteen or Conquest. |

Effort should be estimated after J1 exposes the actual source changes. The largest likely work is objective/lesson integration, mission-specific deployment and encounter design, and rendered/audio delivery across fifteen distinct maps. That is an assessment of work concentration, not a duration estimate. Reuse schema, observers, widgets and production kits; do not reuse generic mission gameplay unchanged.

### Required production automation

Add source checks that reject a mission/lesson with missing actor IDs, unjustified exposed capability, impossible prerequisites, stale map binding, unreachable required site, insufficient mandatory resource/supply budget, invisible irreversible deadline, orphaned dialogue/event, or missing result/retry route. Compare generated registry identity with the deployed map and lesson version. Author capability records and objective definitions in registered sources; avoid maintaining a competing gameplay authority in this document or presentation code.

Expand deterministic tests to exercise every mission/branch’s ordered and permitted out-of-order actions, wrong unit/building/owner, duplicate command, failed prerequisite, paused/expired timer, protected loss, save/restore at each transition and result-commit failures. Use test-only scenario setup for those checks, then separately run the complete physical player path. A fixture may prove a reducer’s correctness; it cannot demonstrate that a learner can discover its required action.

### Decision register for integration

These are review packets for the existing RequirementsState process, not new accepted requirement IDs. Resolve already-decided items by applying the recorded ruling; ask for a new owner decision only where scope, canon or an existing numerical/behavioral contract would change.

| Decision | Recommended treatment | Dependency |
|---|---|---|
| Legacy mission bodies and tutorial numbering | Align legacy references to detailed current mission identities and map each release tutorial obligation onto the ten lessons plus assessment. `TBR-DOC-001` already resolves map-reference identity; do not reopen that decision merely because stale text remains. | Before mission/task assignment. |
| Mission-specific forces and starting economy | Implement explicit `SPEC-PLAN-*` deployments; determine exact unspecified “small/full” counts and economy through budgeted prototypes. Preserve the owner’s resolved M01 roster. | Before encounter tuning and costly unit placement. |
| Tutorial three-Lancer gate versus M01 two-Lancer force | `REL-FTU-007.AUTH` requires three selected Lancers to reach the beacon within 400 cm. Recommend a training-only third Lancer; alternative is an owner-approved amendment to that tutorial count. Update staging, script, lesson observations and tests together. The resolved two-Lancer M01 force does not satisfy the current tutorial gate. | Before J2/J3 training deployment and selection/movement qualification. |
| M01 extraction/Reshape | Retain bound extraction `(6,17)` until the legacy reference is reconciled; prepare an actual source-authored Reshape route and expiry fallback for the open decision. | Before three-branch teaching/qualification. |
| Observation shortcuts and generic apparatus | Decide whether derived “already powered/built” state is a permitted alternate action or whether explicit observation/identity proof is required. If changed, amend predicate, feedback, save compatibility and tests together. | M06/M08/M10 and generic structure gates. |
| Mission termination vs Corefall | Preserve non-Corefall mission success. Resolve remaining terminal-battle coupling against `REL-CAM-021` with explicit mission loss rules. | Before each ordinary mission’s final test. |
| Skip and mastery | Apply `SPEC-TUT-006`; distinguish skipping guidance, completing practice and verified readiness. Reconcile menu opt-out eligibility with the first-run full-AI gate and test progression after a skipped step. | Before readiness is exposed as complete. |
| Optional rewards and district eligibility | Verify which target rewards currently have consumers. Reconcile M06 Archive access/M09 candidate rules and modifiers to later timing/cost without erasing old records. | Before optional objectives are presented as rewarding. |
| Mission pacing | Prototype the required journey, measure active decisions/travel/waiting and compare the specified duration. Propose changes only when the current contract cannot sustain useful play. | Before campaign-wide encounter or voice timing freeze. |
| Conquest run/sector rules | Adopt the run-victory, topology, defense, modifiers, supply and reward contracts listed above. The 25-slot proposal is the concrete design starting point. | Before P6 implementation; independent of J3/J4. |

## Verification and completion criteria

| Evidence stage | What to verify | What it does not establish |
|---|---|---|
| Source/content | All 11 learning stages and 15 story cards mapped to authority; per-element purpose/capability/deployment; branch/economy/terrain bindings; reachable objective graphs; exact rewards and state transitions. | A readable or enjoyable game. |
| Automated gameplay | Positive/negative mission predicates, all 27 inherited late plans, four ending eligibility classes, save/replay/result integrity, skip/mastery isolation and Conquest seed/run isolation. | Ordinary input discovery or human learning. |
| Packaged physical input | Clean profile → opening → every lesson → independent assessment → real AI result → campaign and replay/menu; all mission branches and required loss/retry routes. No developer coordinates, injected saves, hidden commands or coaching. | Unfamiliar-player comprehension or owner acceptance. |
| Rendered/audio | At normal tactical camera and supported HUD/accessibility settings, units/sites/roles and ongoing actions are legible; objectives, prompts, voice, subtitles and sound agree; all fifteen maps are distinguishable. | Interaction proof from a still image, or fun from asset presence. |
| Human learning and purpose | Preserve `DEMO-TUT-021`’s at least four of five project-naive testers completing without verbal coaching. For each campaign mission, observe whether the player can identify purpose, next action, protected asset, choice and consequence using the game. Record actual confusion and retry behavior. | Approval automatically derived from a numeric score. |
| Owner | Angelis reviews the identified experience and recorded limitations. Apply the exact requirement acceptance criteria. | Global acceptance of untested maps, modes or later source changes. |

The proposed comprehension questions are practical diagnostic prompts for debrief, not a quiz that interrupts combat: “What were you trying to accomplish?”, “Why did you need that unit or building?”, “What could cause failure?”, “What did you choose?”, and “What changed afterward?” Record answers without filling in missing understanding. A player reaching a trigger while unable to answer these questions is a design finding even if the mission model passes.

Track where players stop making progress, wrong-actor/target attempts, time spent searching for the goal, forced waiting, repeated failed commands, hint escalation, loss causes, optional-objective comprehension and abandonment. Link each finding to its actual step, source version and capture. Do not set invented acceptance percentages beyond the existing master; use the data to target changes and then repeat the affected experience.

This planning task is complete when the single document covers the whole requested journey, its recommendations are traceable and explicitly distinguished from current behavior, links and metadata are verified, and material inconsistencies are retained for integration. Game implementation and gameplay acceptance remain future work under the existing DeliveryPlan.

## Source map and revision record

Primary project references: [Requirements](Requirements.md) (`SPEC-CAM-*`, `SPEC-PLAN-*`, `SPEC-MSN-*`, `SPEC-LSN-*`, `SPEC-TUT-*`, `SPEC-END-*`, `DEMO-TUT-*`, `REL-FTU-*`, `REL-CAM-033..038`); [RequirementsState](RequirementsState.md); [Development Bible](Archive/DevelopmentBible.md); [Opening and Tutorial Script](OpeningAndTutorialScript.md); [Character and Voice Identity Bible](CharacterVoiceIdentityBible.md); [MapConcepts](MapConcepts.md); [MapTechnicalBlueprint](MapTechnicalBlueprint.md); [DeliveryPlan](DeliveryPlan.md).

Implementation references: [tutorial controller](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerTutorial.cpp), [curriculum model](../Source/EchoesOfTheBrokenSun/Private/EchoesTutorialCurriculumModel.cpp), [practice/player flow](../Source/EchoesOfTheBrokenSun/Public/EchoesPlayerFlow.h), [player shell](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerShell.cpp), [player profile](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerProfile.cpp), [campaign controller guidance](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp), [simulation setup and mission adapters](../Source/EchoesOfTheBrokenSun/Private/EchoesSimulationSubsystem.cpp), [campaign map manifest](../Content/World/Source/Campaign/campaign_map_manifest_v1.json), and the mission/map sources linked in each card. Mission-specific `*MissionModel.cpp` implementations and their hashes are retained in the inspection receipt.

2026-09-06 — Created this authoritative file for the owner-requested comprehensive analysis and plan. Reviewed all fifteen story missions with two read-only evidence specialists; integrated ten lessons plus independent assessment, all three acts, per-element purpose, automation boundaries, and the 25-slot proposed Conquest program. No gameplay/source behavior or existing acceptance record was changed. All revisions and QA corrections remain in this same document.


2026-09-08 additional owner direction: maps may be much larger than current prototypes, using Age of Empires/StarCraft II as mission-dependent scale references. SPEC-MAP-005 records this authorization without inventing equivalent tile sizes or enlarging every map. Keep the tutorial's initial home clearing readable; allow the enclosing operation to expand with its learning needs. Retain current named-map contracts until a specific authored enlargement is qualified. This steering does not interrupt closure of the current HUD batch.
