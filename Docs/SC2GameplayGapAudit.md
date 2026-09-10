# Echoes of the Broken Sun — SC2 gameplay completeness audit

**Author and owner:** Angelis Pseftis  
**Inspection date:** 2026-09-09  
**Scope:** current Unreal checkout, `main` at `6b559889a4e7b0ed7fd86aee37b64728bd516017`, including dirty work.  
**Standing:** dated assessment and recommendations. The subsequent owner-authorized intake is now captured in [Requirements.md](Requirements.md#gameplay-completeness-intake--sc2-audit-2026-09-09), [RequirementsState.md](RequirementsState.md#sc2-audit-intake-and-continuous-work--2026-09-09) and the [existing DeliveryPlan](Archive/ProjectLedger.md#sc2-audit-integration-into-continuous-work--2026-09-09). This audit remains a historical source observation, not a competing live requirements/state record.

Echoes already implements much of an RTS simulation. Its largest SC2-class shortcomings are incomplete player command access, incomplete teaching, partial presentation and analysis tools, and unfinished supported modes. A list saying it lacks gathering, repair, research, rally points, control groups, fog, replays, sound, or saves would be incorrect.

This report covers the complete player journey and action families, including success, refusal, interruption, feedback and recovery. It is a comprehensive functional inventory, not proof that every current runtime defect has been discovered. No game build, launch, physical-input session or listening review was performed for this audit. Negative source findings mean no route was found in the identified implementation surfaces, not a mathematical proof of absence. Existing code and recorded tests do not establish perceived quality or owner acceptance.

## How to read the inventory

| Status | Meaning | Work implied |
|---|---|---|
| M | Missing route in the inspected source surface | Implement or locate and demonstrate the complete route. |
| P | Partial implementation or current source/requirement mismatch | Complete the specified missing behavior or reconcile the conflict. |
| V | Source support exists, but complete player behavior was not verified by this audit | Exercise and qualify; do not automatically reimplement it. |
| Q | Quality/content/coverage question remains unresolved | Inspect the actual game, test against the named criterion, then fix demonstrated deficits. |
| D | Proposed addition or intentional difference | Make an explicit design decision; not an accepted defect. |

Priorities: **0** = control, comprehension, correctness or recovery; **1** = complete existing strategy and feedback; **2** = advanced learning, modes and convenience. These are recommendations within the existing delivery sequence, not a replacement delivery plan. Inventory IDs are audit references, not requirement IDs. A source key after a row identifies the evidence/authority group in the source map below.

**Inventory totals:** 162 functional items: 22 missing source routes, 20 partial/conflicting implementations, 82 source-supported capabilities to qualify, 30 unresolved quality/coverage checks, and 8 design decisions. A separate 32-event feedback checklist follows. These are audit items, not 162 missing features.

## Highest-confidence implementation findings

1. **Hotkey contract mismatch.** The master specifies A/S/H/P/G for attack-move/stop/hold/patrol/guard. Current config maps those actions to F/X/H/T/J, uses P for pause, and uses G to arm group assignment. Well choices use Z/C/V instead of the specified contextual Q/W/E. Correct the whole context map, teaching and displayed bindings together; merely rebinding A while WASD remains camera movement would create another conflict. [DefaultInput.ini](../Config/DefaultInput.ini) and [SPEC-CTL defaults](Requirements.md#201-default-controls).
2. **Same-type selection and rapid macro selectors are incomplete.** Click/box/Shift selection and ten control groups exist. The controller/input audit found no same-type double-click route, idle-worker cycle, completed-production-building cycle, or alert-jump handler. Combat-force selection already exists on F7; it is not absent. Its binding/eligibility still needs alignment with the intended army selector. See C1/C2.
3. **Subgroup navigation is partial.** Tab and Shift+Tab change an active type and display it. The inspected dispatch does not establish that this type controls command-card availability and orders. Complete that connection while retaining the broader selection; do not indiscriminately limit every ordinary group command. See C2.
4. **Queued unit orders lack a complete visual route.** A 16-order backend exists, but the scanned field HUD/command-deck surfaces do not expose a complete unit-order queue and Shift breadcrumb presentation. Production queue controls are a separate implemented system. See C3.
5. **Tutorial practice and the approved replacement curriculum are not aligned.** The legacy Help surface declares ten lessons but `ImplementedLessonMask=0x001f` enables only five; Link restoration, Array Foundry, Probe, Board and Future Well remain unavailable. More importantly, SPEC-TUT-008 (2026-09-08) supersedes that sequence with seven optional chapters, from establishing a foothold to independent command. Complete that approved replacement and its practice/specialization coverage; do not revive superseded camera drills or infer chapter mastery from legacy lesson bits. See J1.
6. **Conquest has no discovered ordinary player entry.** The inspected title/shell action model lacks the required 25-sector run flow. Sector selection, progression, run loss/recovery and rewards need a connected player route. This is an Echoes obligation, not an SC2 feature to copy. See J2.
7. **Multiplayer is incomplete as a player product.** Direct 1v1 networking exists; no complete ordinary Create/Join/LAN lobby appears in the inspected shell. Protocol and reconnect code must not be mistaken for a usable multiplayer front door. See N1.
8. **3v3 exceeds current simulation capacity.** `kMaximumPlayers=4` conflicts with the six participants expressly required by SPEC-SKM-015. This is a concrete capacity gap. The master, not a subordinate eight-slot design sketch, sets the target. See N2.
9. **Advanced replay tools are missing from the inspected transport.** Browser metadata, filters, seek, speeds, tick step, perspectives and bookmarks already exist. Take Command, analytical observer overlays and the specified cinematic freecam do not appear in that transport/action model. See R1.
10. **Results analysis is partial.** Outcome, duration, trained/lost units, Matter, actions, Well decisions and five charts exist. The rendered report model lacks the full requested damage, scouting coverage, idle-worker time, Logistics-block and economy/decision analysis. See R2.
11. **Accessibility settings are incomplete.** Baseline UI scale, contrast, reduced motion/flashing, camera and audio controls exist. The inspected Options flow lacks a full remapping editor, color-vision presets and independent subtitle/text controls. Expanded assistance features require separate implementation/design reconciliation. See A1.
12. **Cinematic and destruction implementations are scaffolds relative to the authored presentation target.** Title/act/ending sequences largely call a generic camera-transform builder; M01 has a separate four-shot contract. The shared destruction actor changes faction color and size but uses the same ring/core/two-shard arrangement. These are partial presentation mechanisms, not proof of finished dramatic scenes or faction-specific structural collapse. See V1/V2.

## Complete functional inventory

Each row names a player capability or a player-visible system obligation. Rows marked V or Q are intentionally retained so an existing capability is not falsely called missing and a whole gameplay area is not omitted.

### Selection and command access

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 001 | Click a unit or structure; identify exactly what was selected | V | 0: verify silhouette hits, ownership, moving targets and selection feedback | C1 |
| 002 | Drag-select; add/remove with Shift; cancel an unfinished selection | V | 0: verify drag threshold, UI boundaries and additive/subtractive behavior | C1 |
| 003 | Double-click a unit to select matching visible units | M | 0: add the required same-type interaction and test screen/fog limits | C1 |
| 004 | Ctrl-click and Ctrl+Shift-click type selection | D | 1: consider familiar type-selection shortcuts without conflicting with other modifiers | B1/C1 |
| 005 | Inspect mixed selection by type without losing the full group | P | 0: finish subgroup/card/ability-dispatch integration | C2 |
| 006 | Assign, append, recall and double-tap-center ten control groups | V | 0: verify modifiers, dead-member removal and save/recovery policy | C2 |
| 007 | Remove/transfer group membership without accidentally changing another group | D | 2: consider explicit group-management shortcuts; preserve ordinary overlapping groups | C2 |
| 008 | Select and center idle workers; cycle them from a visible counter | M | 0: implement the required macro shortcut and HUD action | C2 |
| 009 | Cycle completed production facilities | M | 1: implement the required selector and eligibility rules | C2 |
| 010 | Select the combat army while leaving workers behind | P | 0: existing F7 route needs prescribed binding and eligibility reconciliation | C2 |
| 011 | Learn/use consistent command hotkeys across factions and contexts | P | 0: reconcile current bindings, contextual grid, camera and tutorial prompts | C1 |
| 012 | Change stance explicitly and read its current behavior | M | 1: expose the required five-stance cycle; Hold alone does not supply it | C1/S1 |

### Camera, orientation and tactical overview

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 013 | Pan with keys, screen edges and middle-drag | V | 0: verify focus, window edges, direction/speed and reduced-motion settings | C4 |
| 014 | Zoom while preserving a useful battlefield view | V | 0: verify limits, cursor/selection context and terrain readability | C4 |
| 015 | Click/drag the minimap to navigate; see camera bounds | V | 0: verify accurate mapping under every supported aspect/UI scale | C4 |
| 016 | Save and recall camera locations | M | 1: no gameplay camera-bookmark route found; recommended SC2 ergonomic addition | B1/C4 |
| 017 | Cycle economic bases/outposts | D | 1: adapt base cycling to one Core plus drop-offs/outposts | B1/S2 |
| 018 | Jump to the latest attack or objective alert; cycle recent locations | M | 0: implement spatial history and resolve the Space targeting conflict | C2 |
| 019 | Center a selected unit/group after it moves offscreen | V | 0: exercise existing group/force camera routes and dead-target handling | C2/C4 |
| 020 | Read unexplored, remembered and currently visible areas immediately | V | 0: inspect real fog transitions and minimap/world agreement | S3/C4 |
| 021 | Distinguish forces, resources, objectives, Wells and contacts on minimap | V | 0: verify non-color symbols and clutter under combat load | C4 |
| 022 | Adjust edge speed/dead-zone, cursor and camera comfort separately | P | 1: baseline speed/toggle exists; expose and test missing fine controls | A1/C4 |

### Orders, pathing and tactical movement

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 023 | Right-click ground to move, visible hostile to attack, resource to gather | V | 0: verify context resolution and plausible target misses | C1/S1 |
| 024 | Issue attack-move with a clear targeting state | V | 0: verify target cursor, cancel, engagement and final destination | C1/S1 |
| 025 | Stop immediately; distinguish Stop from Hold | V | 0: verify queue clearing and explicit micro override | S1 |
| 026 | Hold position without unintended chasing | V | 0: verify stance/order distinction, range and target loss | S1 |
| 027 | Patrol and guard/follow an allied force | V | 1: verify routes, moving target, death, fog and interruption | S1 |
| 028 | Shift-queue mixed movement, attack, gather and build orders | V | 0: backend exists; test every legal sequence and overflow refusal | C3/S1 |
| 029 | See queued paths, order glyphs and current-versus-next action | P | 0: complete visible breadcrumbs/queue, including offscreen paths | C3 |
| 030 | Remove or revise future unit orders without resetting all work | D | 1: choose exact queue-edit semantics; distinct from production reorder | C3 |
| 031 | Cross chokepoints without permanent allied trapping | V | 0: verify yielding, opposing flows and congested exits | S1 |
| 032 | Move at consistent speed in every direction | V | 0: qualify diagonal/any-angle movement in the real maps | S1 |
| 033 | Use Box, Line and Wedge; reform after obstacles | V | 1: verify facing, slowest-unit behavior, slot spacing and recovery | S1/C2 |
| 034 | Understand no-path, occupied destination and route-blocked outcomes | V | 0: pair refusal with a visible location and useful recovery | S1/F1 |
| 035 | Override AI/automation instantly with a direct order | V | 0: test queued/automated actions do not overwrite deliberate micro | S1 |
| 036 | Use passages and changed routes safely | Q | 1: test capacity, entry/exit loss, fallback and fair telegraphs on authored maps | S3 |

### Workers, economy and expansion

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 037 | Train workers and begin an economy from ordinary entry | V | 0: confirm cost, production completion and useful initial orders | S2 |
| 038 | Assign workers to deposits and understand saturation | V | 0: expose assignment/yield limits; do not copy SC2 worker ratios | S2 |
| 039 | See cargo, harvest progress, return route and actual deposited income | V | 1: distinguish carried resources from spendable resources | S2/C3 |
| 040 | Reassign or pull workers; queue construction then return to work | V | 1: test cargo, interrupt and resumed-order semantics | S1/S2 |
| 041 | Respond when a deposit empties | V | 0: current source idles after final delivery; verify depletion/idle alerts | S2 |
| 042 | Recover after losing a drop-off or blocking a hauling route | V | 0: retain cargo and show the exact stalled reason | S2/F1 |
| 043 | Expand via allowed drop-offs, power/logistics nodes and protection | V | 1: prove travel-time/defense tradeoffs; no second Core under current rules | S2 |
| 044 | Read Matter, Dawn, Logistics, reservations and income | V | 0: confirm distinctions and update timing in actual HUD | S2/C3 |
| 045 | Recognize a Logistics block before placing another production order | V | 0: show capacity, commitments, cause and remedy | S2/F1 |
| 046 | Predict Relay expiry and Choir upkeep shortfalls | Q | 1: test advance warnings, loss of connection and conservative forecasts | S2/F1 |
| 047 | Transfer economy between exposed and safer positions | V | 1: demonstrate meaningful expansion/retreat choices against an opponent | S2/S4 |
| 048 | Use worker repair and construction assistance deliberately | V | 0: new maintenance code exists; verify rates, costs, interruption and stop | S2/F1 |

### Construction, production and technology

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 049 | Choose a building from a contextual worker menu | P | 0: align existing direct build commands with prescribed menu/input behavior | C1/S2 |
| 050 | Preview footprint, occupied cells and legal placement | V | 0: later fixes exist; recheck current candidate, not the old preview defect | C3/F1 |
| 051 | Understand connection radius, chain and operational consequences | V | 0: Power Link/Aegis explanations were repaired; qualify roster-wide | F1/S2 |
| 052 | Queue several buildings and return workers to prior work | V | 1: verify reservation, interruption, illegal subsequent sites and recovery | S1/S2 |
| 053 | See construction progress, damage, assistance and completion | V | 0: verify the visible building matches its authoritative availability | S2/V2 |
| 054 | Cancel incomplete construction and see the correct refund | V | 0: verify no refund ambiguity or double spending | S2/F1 |
| 055 | Train multiple units and read remaining time/reservations | V | 0: production panel exists; test physical operation and UI scale | C3/S2 |
| 056 | Cancel/reorder queued production | V | 0: source supports cancellation and waiting-slot reorder; qualify exact pointer UX | C3/S2 |
| 057 | Rally to ground, a resource, allied force or queued route | P | 1: ground/Shift-append player route exists; resource/allied target UI and auto-gather need completion or demonstration | C3/S2 |
| 058 | Recover when a completed unit cannot emerge | V | 0: show blocked exit and preserve the completed unit/resources | C3/S2 |
| 059 | Read available technology, prerequisite, cost and producer contention | V | 0: technology panel exists; verify complete explanatory content | S2/C3 |
| 060 | Start/cancel research and see completion change actual unit behavior | V | 1: qualify research loss/cost policy and effective-stat feedback | S2/F1 |
| 061 | Make deeper branching technology/roster decisions | D | 2: compact roster/two sequential techs are current design; breadth expansion remains a decision | S2/D1 |

### Combat and faction abilities

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 062 | Focus fire and deliberately retreat/kite | V | 0: qualify target acquisition, chase bounds and immediate movement override | S1 |
| 063 | Read attack range, facing, cooldown and effective threat | Q | 0: validate those cues under moving mixed-unit combat | S1/V2 |
| 064 | See attack windup, release, projectile/beam, impact and damage agreement | Q | 0: inspect timing and sound; no misleading cosmetic hit | V2/F1 |
| 065 | Recognize health loss, protection, death and structure destruction | P | 0: basic effects exist; shared destruction is not bespoke faction collapse | V2 |
| 066 | Use terrain, obstacles, spacing and cover to change a fight | V | 1: prove gameplay geometry matches rendered cover/passability | S1/S3 |
| 067 | Cast from one eligible unit in a mixed group | V | 0: source repairs exist; verify availability, cost and exact executor | C2/F1 |
| 068 | Cast from all intended eligible units only when explicitly requested | Q | 0: test modifier, duplicate, queued and remote dispatch on one candidate | C2/N1 |
| 069 | Preview an ability target and cancel without spending | V | 0: exercise range, vision, footprint and mode-specific refusal | C1/F1 |
| 070 | Read ability cost, cooldown, active duration and interrupted state | V | 0: carry Relay/Bulwark feedback discipline across all abilities | F1 |
| 071 | Deploy/pack Bulwark and understand directional protection | V | 0: current and historical policies have bounded evidence; physical/mixed/remote coverage remains | F1 |
| 072 | Extend Relay and understand connection/expiry/cooldown | V | 0: existing feedback is not missing; qualify pointer and multiplayer behavior | F1 |
| 073 | Build/repair Meridian infrastructure and understand Aegis power | V | 1: verify network endpoints, disabled defenses and broken chains | S2/F1 |
| 074 | Root/uproot/migrate Kharuun Waystones | V | 1: qualify transformation commitments, movement and network consequences | S1/V2 |
| 075 | Use Kharuun adaptation, cover, sensing and terrain repair | V | 1: verify each full action/refusal/counterplay path | S1/S3 |
| 076 | Reconcile Choir identity and manage coherence/Phase Anchors | V | 1: expose mutually exclusive benefits, upkeep and loss consequences | S1/S2 |
| 077 | Understand every unit/building's use, limitation and counter | Q | 0: full roster explanation and real matchup demonstration are needed | C3/S4 |
| 078 | Distinguish meaningful counters from simply having more units | Q | 1: test matchups, composition, positioning and faction economy interactions | S4 |

### Scouting, maps and Future Wells

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 079 | Scout with normal vision; react without hidden-state knowledge | V | 0: verify targets, AI, minimap, effects and sound use the same knowledge boundary | S3 |
| 080 | Recognize last-known information as stale | V | 0: verify moving enemies and newly built structures stay hidden | S3/C4 |
| 081 | Use anonymous sensors without gaining forbidden identities | V | 1: demonstrate source, radius, freshness and uncertainty of contacts | S3 |
| 082 | Run Explore/Find Matter/Locate Hostiles/Screen Route policies | Q | 1: verify complete player interface and lifecycle; enum presence is insufficient | S3 |
| 083 | Choose cautious/observe/persist behavior and cancel automation | Q | 1: expose route, discoveries, damage threshold, return and completion | S3 |
| 084 | Recognize main routes, flanks, chokes and expansion risk | Q | 0: inspect maps at gameplay zoom and measure practical travel times | S3 |
| 085 | Fight over optional vision objectives or attackable route blockers | D | 2: add only authored Echoes objects with full rules; decoration cannot imply them | B3/S3 |
| 086 | Choose Harvest/Preserve/Reshape with clear benefits and sacrifice | V | 0: verify cost, irreversible commitment, confirmation and counterplay | S3/F1 |
| 087 | Read Well ownership, progress, contention and active protocol without selecting | Q | 0: inspect world/minimap states and battle readability | S3/V2 |
| 088 | Respond to Reshape with a fair telegraph and safe route recovery | V | 0: test visible warnings, movement re-pathing and AI response | S3/F1 |
| 089 | Understand how a Well decision affects mission and future progression | V | 1: verify exact modeled consequences and ledger text | J2/S3 |
| 090 | Play fair, distinct maps with useful scale and varied tactics | Q | 1: qualify all three skirmish maps and fifteen campaign locations | S3/J2 |

### AI and match configuration

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 091 | Pick player/opponent faction, doctrine, difficulty, map, economy and speed | V | 0: setup exists; qualify all legal combinations and rejected ones | S4/J2 |
| 092 | Know the win/loss condition before deployment | V | 0: Corefall is the current offline 1v1 contract and implementation | S4 |
| 093 | Fight AI that gathers, builds, produces, researches and expands coherently | Q | 0: evaluate complete ordinary matches, not isolated command tests | S4 |
| 094 | Fight AI that scouts fairly and counters observed information | Q | 0: test adaptation, fog fairness and reaction limits | S4/S3 |
| 095 | Experience distinct Warden/Raider/Steward/Expansionist/Adaptive play | Q | 1: show recognizable strategy and recovery in actual matches | S4 |
| 096 | Adjust difficulty without unexplained cheating or dead opponents | Q | 1: qualify all four levels and communicate intentional assistance | S4 |
| 097 | Face multi-front pressure, defense, retreat and economic recovery | Q | 1: measure opponent competence after disruption and partial defeat | S4 |
| 098 | Reach valid victory, defeat, draw or concession without stalled endings | V | 0: test outcome cause, simultaneous loss and all return routes | S4/R2 |
| 099 | Pause/resume and change allowed speed without ambiguity | V | 0: verify single-player/network differences and orders during pause | J2/A1 |
| 100 | Rematch with settings preserved or restart with the same seed | V | 0: test results-to-match continuity and clean state reset | J2/R2 |

### Tutorial, campaign and progression

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 101 | Enter through a usable title screen and choose an understandable mode | V | 0: shell exists; qualify cold launch and mouse-only navigation | J1/J2 |
| 102 | Learn who commands, why the operation matters and the immediate goal | Q | 0: inspect opening story, briefings and novice comprehension | J1/V1 |
| 103 | Learn the seven approved chapters from foothold to independent command | P | 0: replace legacy lesson flow under SPEC-TUT-008; credit actual actions, not demonstration or old lesson bits | J1 |
| 104 | Play/practice lesson 6 — Link restoration | M | 0: legacy practice entry is unavailable; supply coverage through approved chapters/specializations | J1 |
| 105 | Play/practice lesson 7 — Array Foundry | M | 0: legacy practice entry is unavailable; supply coverage through approved chapters/specializations | J1 |
| 106 | Play/practice lesson 8 — Probe | M | 0: legacy practice entry is unavailable; supply coverage through approved chapters/specializations | J1 |
| 107 | Play/practice lesson 9 — Board | M | 0: legacy practice entry is unavailable; supply coverage through approved chapters/specializations | J1 |
| 108 | Play/practice lesson 10 — Future Well | M | 0: legacy practice entry is unavailable; supply coverage through approved chapters/specializations | J1 |
| 109 | Skip tutorial and retain the choice across save/restart | V | 0: recent bounded recovery evidence exists; broaden to ordinary front-door routes | J1/F1 |
| 110 | Repeat lessons safely without changing campaign history | V | 0: test available practice routes and failed/abandoned lessons | J1 |
| 111 | Complete tutorial, then a full AI match, results and replay/menu | Q | 0: connected novice/physical-player demonstration remains open | J1/J2 |
| 112 | Play fifteen distinct operations through all authored objectives | Q | 1: code/map contracts exist; full connected playable content is unqualified | J2/S3 |
| 113 | Read primary/optional/protected/timed objectives and failure reasons | V | 0: qualify every mission-specific objective surface and transition | C3/J2 |
| 114 | See and use earned rewards and branch consequences | V | 1: verify unlocks, eligibility, exact effects and anti-rewrite behavior | J2 |
| 115 | Reach the four authored endings with coherent audiovisual closure | P | 1: outcome logic and camera sequences do not establish finished endings | J2/V1 |
| 116 | Start and complete the separate 25-sector Conquest run | M | 2: no ordinary shell route found; implement map, run, loss/recovery and rewards | J2 |

### Multiplayer and social coordination

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 117 | Open multiplayer, create/join by code and discover LAN games | M | 2: complete visible shell/lobby workflow; direct networking already exists | N1 |
| 118 | Configure participants, factions, AI, teams, map and ready state | P | 2: replace fixed-rule direct-1v1 limitation with supported format setup | N1/N2 |
| 119 | Play 2v2, 3v3, comp-stomp and up-to-four-player FFA | P | 2: six-player 3v3 exceeds current four-player core; qualify each format/map | N2 |
| 120 | See loading/readiness, connection state, latency and actionable join errors | Q | 0 for exposed network mode: test actual peers and failure return | N1 |
| 121 | Send ally/all chat and map pings through discoverable controls | M | 2: no complete player workflow found in inspected shell/network UI | N1 |
| 122 | Understand allied vision, unit authority, tribute and victory membership | Q | 2: qualify explicit permissions and fair information boundaries | N2 |
| 123 | Recover from disconnect/reconnect and host loss without false success | V | 0 for exposed network mode: backend exists; real-peer recovery is separate | N1 |
| 124 | Receive identical ability acceptance/refusal/feedback locally and remotely | Q | 0: active dispatch work needs one current source/build and both player views | N1/F1 |
| 125 | Observe a live match with explicit spectator permissions | M | 2: replay perspectives are not a live observer join workflow | N1/R1 |
| 126 | Use public ranked matchmaking, accounts, friends and hosted relay | D | 2/later: hosted offering is deferred; do not silently enlarge this release | D1 |

### Replay, results and learning tools

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 127 | Save/find replays by date/map with useful match metadata | V | 1: implemented browser/filter/archive flow; test ordinary saved matches | R1 |
| 128 | Play/pause/seek, change speed and step one tick | V | 1: implemented transport; verify state synchronization and responsiveness | R1 |
| 129 | Switch player fog/omniscient view and jump event bookmarks | V | 1: implemented; qualify fog correctness and UI clarity | R1 |
| 130 | Take Command from a replay to try another decision | M | 2: add isolated live branch and protect campaign progression | R1 |
| 131 | Inspect live replay income/army/production/loss analytical overlays | M | 2: advanced observer deck absent from inspected transport | R1 |
| 132 | Follow recorded camera/unit or use a cinematic spectator freecam | P | 2: ordinary camera exists; advanced replay camera workflow not established | R1/C4 |
| 133 | See why the match ended and what each player produced/lost | V | 0: implemented result dossier; verify all outcomes and partial-data labeling | R2 |
| 134 | Review damage, scouting, idle time, Logistics blocks and economy trends | P | 1: extend current five-chart report to the requested strategic diagnostics | R2 |
| 135 | Compare a loss with timings and decisions instead of only APM | P | 1: connect useful analysis to replay events, costs and visible decisions | R1/R2 |
| 136 | Open a unit/building codex, model viewer and mechanics glossary | M | 2: no player-facing route found in current shell | J3 |
| 137 | Practice counters in a configurable combat lab | M | 2: no ordinary sandbox/lab route found; keep test fixtures separate | J3 |
| 138 | Create and play custom scenarios/maps | M | 2: no end-user editor/custom-map journey established | J3 |

### Graphics, animation, sound and readability

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 139 | Recognize all unit/building roles and affiliation at normal zoom | Q | 0: inspect complete roster silhouettes, values and non-color identifiers | V2 |
| 140 | See grounded locomotion, turning, stopping and attack motion | Q | 0: procedural/static-mesh mechanisms exist; judge final motion in play | V2 |
| 141 | Read deploy/root/identity/construction transformations over time | Q | 0: verify every intermediate state, interruption and visibility | V2/F1 |
| 142 | Read weapon identity, impacts, protection and death under heavy effects | Q | 0: verify timing, sound, reduced-flash alternatives and no obscured targets | V2/F1 |
| 143 | See faction-specific building collapse rather than one recolored effect | P | 1: shared ring/core/shards need authored material-specific destruction | V2 |
| 144 | Navigate attractive terrain without mistaking scenery for gameplay geometry | Q | 0: inspect paths, cliffs, cover, interactables and landmarks | V2/S3 |
| 145 | Understand title/act/ending scenes as authored drama | P | 1: camera interpolation is only part of scene production | V1 |
| 146 | Hear selection, orders, refusals, combat, economy and completion clearly | V | 0: subsystems exist; perform full listening/priority/coverage review | A2/F1 |
| 147 | Hear voices over battle and music without losing urgent alerts | V | 0: ducking/submix source exists; qualify real intelligibility | A2 |
| 148 | Read synchronized subtitles with speaker and interruption context | V | 0: queue exists; verify playback, pauses, skipping and translation layout | A2 |
| 149 | Hear location/urgency without relying on stereo or perfect hearing | P | 0: verify mono/directional visual equivalents and critical-cue coverage | A1/A2 |
| 150 | Play with stable frame pacing and readable low-quality settings | Q | 0: measure actual supported hardware/full participant load; no test run here | A1/V2 |

### Settings, accessibility and recovery

| ID | Player action / expected result | State | Priority / remaining work | Source |
|---|---|---|---|---|
| 151 | Remap every gameplay action, detect conflicts and retain the profile | M | 0: no complete remapping editor/persistence flow found | A1/C1 |
| 152 | Use color-vision presets and non-color tactical indicators | P | 0: high contrast exists; complete missing presets and perception tests | A1 |
| 153 | Adjust text/subtitle size/background independently of the full HUD | M | 0: missing exposed independent controls in inspected Options | A1 |
| 154 | Reduce motion/flashing and tune separate audio categories | V | 0: controls exist; demonstrate effective behavior and persistent settings | A1/A2 |
| 155 | Play menu and battlefield actions through pointer and keyboard equivalents | Q | 0: full physical-input parity remains unqualified | C1/A1/F1 |
| 156 | Recover focus after Alt-Tab, resizing, modal menus or lost input | V | 0: exercise held modifiers, cancelled targets and accidental-order prevention | C1/A1 |
| 157 | Change display settings and revert safely when unusable | V | 0: confirmation route exists; test timeout and restart persistence | A1 |
| 158 | Use additional tactical-pause/speed/macro/threat assistance | D | 1: reconcile conflicting pause/speed rules before adding assistance behavior | A1/D1 |
| 159 | Save, see pending versus completed state and reload the same state | V | 0: candidate39 has bounded keyboard recovery evidence; no blanket save defect | F1/J2 |
| 160 | Recover valid backup after corruption/incompatibility without losing active play | V | 0: retain existing protections; test ordinary user-facing failure/recovery | F1 |
| 161 | Understand load waits, save failure, disk error and what can be retried | P | 0: historical-load waiting UX remains recorded as unpolished | F1 |
| 162 | Quit/concede/restart with clear consequences and protected progression | V | 0: shell routes exist; test confirmations and all mode transitions | J2/R2 |

## Feedback inventory: what every action must communicate

These are **coverage tests**, not 32 additional confirmed missing features. Existing markers, status messages, audio and the new gameplay-feedback code cover parts of this matrix. Review every row across local, multiplayer where applicable, save/load and replay. The detailed design below is an Echoes recommendation grounded in SPEC-UI-001..004, REL-UI-019..024 and the associated mechanic contracts.

| ID | Trigger | Player should see/hear | Failure/recovery requirement |
|---|---|---|---|
| F01 | Hover an interactable | Target identity, ownership, legal action and suitable cursor | No false affordance on decorative objects or hidden targets |
| F02 | Select/deselect | Stable ring/bracket, identity/composition and restrained acknowledgement | Explain unavailable selection where useful; do not leak hidden entities |
| F03 | Arm an order | Cursor, action name, target type, range/footprint and cancel hint | Escape/right-click behavior is consistent and spends nothing |
| F04 | Accept an order | Distinct action glyph/marker, brief sound and updated current order | Distinguish command submission from actual execution |
| F05 | Queue an order | Queue position, route/target and append acknowledgement | Explain full queue or incompatible action; preserve prior orders |
| F06 | Reject a command | Plain-language reason beside the action, with appropriate sound | Identify remedy: cost, prerequisite, range, vision, state, ownership or mode |
| F07 | Lose target vision | Last lawful action state and loss-of-contact explanation | Do not track the hidden target through UI, sound, effects or automation |
| F08 | Block a route | Location and no-path/occupied/blocked distinction | Give a useful next action; no endless wandering or silent teleport |
| F09 | Start/finish harvesting | Resource, cargo/return status and actual delivery feedback | Do not count cargo as already spendable |
| F10 | Exhaust a deposit | Depleted location and newly idle worker notification | Finish lawful delivery, then let the player choose reassignment |
| F11 | Lose a drop-off | Interrupted route and retained cargo | Explain alternate valid route or need for a new drop-off |
| F12 | Reach Logistics limit | Used/reserved/capacity breakdown and relevant blocked action | Show which network/provider action can resolve the shortage |
| F13 | Place a structure | Footprint, valid/invalid symbol, cost and network advisory | No misleading preview; accepted placement can still have a later explained interruption |
| F14 | Build/assist/repair | Progress, contributing workers, health and resource expenditure | Show interruption, insufficient funds, destroyed target or maximum health |
| F15 | Complete construction | Distinct completion cue and now-available function | Show completed-but-unpowered/disconnected state separately |
| F16 | Queue/cancel production | Item, time, reservation and exact refund | Distinguish active/waiting slot; preserve unaffected items |
| F17 | Block emergence | Completed item retained, blocked exit and location | Resume after clearance without duplicate unit or additional charge |
| F18 | Set rally | Flag/vector or equivalent target marker and queued route | Clear/update when target becomes invalid; disclose fallback |
| F19 | Start/complete research | Prerequisite, cost, producer commitment, progress and actual effect | Explain cancellation/lost facility outcome without phantom upgrades |
| F20 | Activate an ability | Executing unit(s), cost, duration, state transition and cooldown | Suppress accidental duplicate commitments; explain partial group execution |
| F21 | Interrupt a channel/transformation | Interrupted state and retained/lost cost or progress | Show exactly when commands become available again |
| F22 | Change network/identity/adaptation | Active benefit, coverage, restriction and commitment | Immediate truthful feedback when connection or eligibility ends |
| F23 | Face impending upkeep/Relay expiry | Conservative warning with countdown and consequence | Do not promise capacity or currency that will no longer exist |
| F24 | Fire/hit/take damage | Readable attack/impact/protection cue, directional threat and health change | Cosmetic effects must agree with authoritative combat |
| F25 | Come under attack offscreen | Prioritized location-bearing alert and minimap cue | Repeated attacks coalesce without hiding a second dangerous location |
| F26 | Lose a unit/building/Core | Readable death/destruction, appropriate sound and consequence | Remove invalid selections/groups/commands without losing player orientation |
| F27 | Scout a discovery | Resource/route/hostile/objective category, location and certainty | Separate discovery from inference and stale from current knowledge |
| F28 | Commit a Future Well protocol | Choice, price, permanence, timer and map/world state | Explain interrupted/invalid commitment and downstream modeled consequences |
| F29 | Update an objective | New/changed/completed/failed state, timer and location | No objective silently advances or fails behind a cinematic/modal overlay |
| F30 | Save/load/recover | Pending, completed, failed or recovered state with usable next action | Preserve valid saves; no success message before durable completion |
| F31 | Lose connection/desync | Participant state, what is paused/controllable and recovery options | No false reconnect success; safe leave and offline return |
| F32 | End a match/mission | Outcome cause, relevant statistics, rewards and next-step choices | Preserve progression and label unavailable/partial replay or statistics |

## SC2 comparisons that should not automatically become Echoes features

SC2 offers established selection/command ergonomics, economic expansion, terrain information play and powerful replay tools. Blizzard's guides document these principles; the inventory above applies them to Echoes' own requirements rather than treating SC2's complete feature set as mandatory. [Controls](https://news.blizzard.com/en-us/article/6640645/game-guide-simplified-controls), [special control](https://news.blizzard.com/en-us/article/4552955/game-guide-special-control), [economy](https://news.blizzard.com/en-us/article/4488313/game-guide-economy), [map elements](https://news.blizzard.com/en-us/article/4546768/game-guide-map-elements), [replays](https://news.blizzard.com/en-gb/article/9975972/press-the-replay-button).

| SC2 feature or principle | Echoes decision |
|---|---|
| Air armies, air/ground target layers and terrain-bypassing flight | Excluded by current ground-only contract. Visual hovering does not grant flight. |
| Arbitrary burrowing, transport drops and unrestricted garrisons | Do not add by analogy. Echoes has explicit authored-passage rules, and completed structures cannot be garrisoned under SPEC-BLD-009. |
| High-ground vision advantage | A genuine SC2 terrain mechanic, but Echoes currently makes elevation presentation-only. Adoption requires changed authority, maps, fog, AI and tests. Do not add a fictional generic SC2 high-ground miss-chance rule. |
| Smoke/vision blockers, capturable towers and destructible route blockers | Consider purpose-built equivalents only where map contracts define targeting, fog, timers, counterplay and route changes. Do not turn arbitrary scenery into gameplay. |
| Multiple headquarters and race-specific macro cycles | Echoes allows one active Core. Expansion should use permitted drop-offs, networks and outposts; copying SC2 townhall/larva/injection mechanics would change the game. |
| SC2 supply and roster sizes | The inspected Unreal master uses a 200 Logistics cap, weighted by unit footprint. That is not 200 identical units. The owner-approved 30-entity limit is binding design direction, operating concurrently with the 200 Logistics cap. |
| Large tech trees, broad roster and race-specific production rules | Current compact roster/technology design is intentional; TBR-SCP-003 records a breadth proposal. Test strategic depth before choosing more units/upgrades. |
| Battle.net accounts, ranked ladder, public matchmaking, social graph and hosted relay | Hosted services are deferred. Direct/LAN lobby, supported formats and recovery remain current obligations; the deferral does not excuse them. |
| Co-op commanders, persistent commander leveling, mutations or an Arcade ecosystem | SC2 modes are useful references, not automatic Echoes scope. Comp-stomp, Conquest and scenario-editor obligations should be delivered on their own contracts. |
| Replay Take Command | Particularly useful for learning and already explicitly required in Echoes; implement without rewriting campaign history. |
| Cosmetic achievements, portraits and public progression | Optional unless bound to an existing trophy/profile requirement; do not prioritize over usable gameplay. |

SC2 replay branching and shared replay review are documented by Blizzard; the Echoes recommendation is an isolated practice branch, not an unrequested online review service. [Replay and resume features](https://news.blizzard.com/en-gb/article/10054757/new-replay-and-resume-features-coming-in-heart-of-the-swarm). SC2's campaign, versus, co-op and custom-game modes provide the broader comparison context. [Official game overview](https://starcraft2.blizzard.com/en-us/).

## Recommended execution order and completion tests

1. **Finish reliable control and teaching.** Reconcile bindings; add same-type/idle-worker/production/alert selectors; finish subgroup execution and visible unit queues; deliver the seven approved chapters and replace the incomplete legacy practice surface. A fresh player must be able to complete the tutorial-to-AI-match journey without developer instructions.
2. **Complete every existing action's feedback.** Apply F01–F32 to the roster, economy, production, Wells and recovery. Test real targeting, not only direct command calls. Retain the recent Power Link, Relay, Bulwark and save fixes.
3. **Prove strategic play and production presentation.** Qualify ordinary all-faction matches, AI disruption/recovery, map readability, counterplay, animation, audio and mission continuity. Complete the representative M01 route before treating its patterns as campaign-wide proof.
4. **Deliver the remaining required player tools and modes.** Conquest, real direct/LAN lobby and supported formats, six-player capacity, advanced replay tools, codex/editor and remaining accessibility work. Keep hosted infrastructure deferred.

For each action: exercise success, insufficient resources, wrong target, out-of-range/blocked path, interruption, repeat input, mixed selection, fog change, save/load, replay and multiplayer where applicable. Not every negative case applies to every action; document the applicable set rather than inventing mechanics. Use an identified build and normal UI controls. A complete capability has the input path, authoritative result, understandable feedback, failure recovery and required persistence—not merely a command enum.

## Evidence and source map

The source references below support implementation observations and identify where outstanding work belongs. Family references route to relevant requirements; they do not grant blanket acceptance or claim every subordinate requirement was individually proven. Line numbers are inspection anchors in a concurrently edited checkout and may move.

| Key | Authority / implementation inspected | What the evidence establishes |
|---|---|---|
| C1 | [Requirements: controls/UI](Requirements.md#20-interface-selection-controls-and-player-feedback); [DefaultInput](../Config/DefaultInput.ini); [PlayerController](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp) around 9626–9817, 9957; [TacticalInput](../Source/EchoesOfTheBrokenSun/Private/EchoesTacticalInput.cpp) | Basic selection/context input exists; current hotkey mismatch and scoped absent same-type route. |
| C2 | [PlayerController](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp) around 7150, 10549–10812; [FieldHudView](../Source/EchoesOfTheBrokenSun/Private/EchoesFieldHudView.cpp) around 1854; Requirements SPEC-CTL-010..014 and REL-QOL-001..009 | Groups, subgroup display and F7 combat-force route; missing rapid selectors/history and partial subgroup dispatch. |
| C3 | [Simulation.h](../Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h) around 690; [FieldHudView](../Source/EchoesOfTheBrokenSun/Private/EchoesFieldHudView.cpp) around 599; [FieldHudWidget](../Source/EchoesOfTheBrokenSun/Private/EchoesFieldHudWidget.cpp) around 1846; [TacticalInput](../Source/EchoesOfTheBrokenSun/Private/EchoesTacticalInput.cpp) around 498 | Backend queues and production/rally UI; unit-order breadcrumb coverage not established. SPEC-CMD-012, SPEC-HUD-001..007, REL-UI-026/028. |
| C4 | [RTSCameraPawn](../Source/EchoesOfTheBrokenSun/Private/EchoesRTSCameraPawn.cpp) around 486, 889; [FieldHudView](../Source/EchoesOfTheBrokenSun/Private/EchoesFieldHudView.cpp) around 255, 1227 | Camera/minimap implementations, not physical usability. |
| S1 | [Simulation.h](../Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h) around 146–237; [Simulation.cpp](../Source/EchoesSimCore/Private/Simulation.cpp) around 4040–4731; [FormationLayout](../Source/EchoesOfTheBrokenSun/Public/EchoesFormationLayout.h); Requirements SPEC-CMD, SPEC-MOV, REL-CMB, REL-FAC | Core command, combat, faction and movement mechanisms. |
| S2 | [Simulation.cpp](../Source/EchoesSimCore/Private/Simulation.cpp) around 1641–1656, 2339–2385, 4071–4304, 4517–4608, 5036–5041, 5121–5130; Requirements sections 9–10, REL-ECO and REL-BLD | Economy, depletion handling, research, production, repair, rally and current one-Core/Logistics rules. |
| S3 | [Requirements](Requirements.md) around 432–555, 1250–1290 and REL-CMB-025..027; [MapTechnicalBlueprint](MapTechnicalBlueprint.md); [Simulation.h](../Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h) around 183 | Fog/map/passage/scouting/Well obligations and model; broad map experience remains unobserved. |
| S4 | [SkirmishSetup](../Source/EchoesOfTheBrokenSun/Private/EchoesSkirmishSetup.cpp) around 212; [Requirements](Requirements.md) SPEC-DIF, SPEC-SKM, SPEC-BAL and REL-AI | Configured offline Corefall and intended AI/match quality. No live AI balance evaluation here. |
| J1 | [PlayerFlow.h](../Source/EchoesOfTheBrokenSun/Public/EchoesPlayerFlow.h) around 29–44; [PlayerShell](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerShell.cpp) around 183–202, 302–327; [PlayerTutorial](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerTutorial.cpp) around 551 | Ten legacy lessons, first-five mask and unavailable practice buttons; SPEC-TUT-008 supersedes the old sequence with seven optional chapters, requiring coherent migration. |
| J2 | [PlayerShell](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerShell.cpp); [PlayerFlow.h](../Source/EchoesOfTheBrokenSun/Public/EchoesPlayerFlow.h); [Requirements](Requirements.md) SPEC-CAM/SPEC-MSN, REL-CAM and Conquest around 4014–4047; [DeliveryPlan](DeliveryPlan.md) | Existing local route model; no discovered Conquest shell flow; campaign obligations exceed source-route proof. |
| J3 | [Requirements](Requirements.md) around 5393 and 5737 onward; [PlayerShell](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerShell.cpp); [PlayerFlow.h](../Source/EchoesOfTheBrokenSun/Public/EchoesPlayerFlow.h) | Codex/lab/editor requirements lack an ordinary route in inspected shell. |
| N1 | [NetworkSession](../Source/EchoesOfTheBrokenSun/Private/EchoesNetworkSession.cpp); [SkirmishSetup.h](../Source/EchoesOfTheBrokenSun/Public/EchoesSkirmishSetup.h) around 97; [FieldHudView](../Source/EchoesOfTheBrokenSun/Private/EchoesFieldHudView.cpp) around 1660; PlayerShell/PlayerFlow above | Direct networking exists; fixed-rule 1v1 presentation and missing general lobby/communication surface. |
| N2 | [Simulation.h](../Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h) line 35; [Requirements](Requirements.md) SPEC-SKM-014..018, REL-MP-018/019 | Four-player core versus required six-participant 3v3; format and team validation obligations. |
| R1 | [PlayerReplay](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerReplay.cpp) around 277–442, 736; [PlayerFlow](../Source/EchoesOfTheBrokenSun/Public/EchoesPlayerFlow.h); Requirements REL-QOL-010..016 | Implemented browser/transport/bookmarks, absent advanced transport actions. |
| R2 | [PlayerResults](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerResults.cpp) around 21–103; Requirements SPEC-MAP-003 | Existing dossier and five charts versus broader desired analysis. |
| A1 | [GameUserSettings.h](../Source/EchoesOfTheBrokenSun/Public/EchoesGameUserSettings.h); [PlayerShell](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerShell.cpp) around 336–370; [DefaultInput](../Config/DefaultInput.ini); Requirements SPEC-ACC and REL-ACC | Exposed baseline settings and scoped absence of expanded controls; no claim that disabling Enhanced Input alone proves remapping impossible. |
| A2 | [AudioMixSubsystem](../Source/EchoesOfTheBrokenSun/Private/EchoesAudioMixSubsystem.cpp); [GameplayAudioSubsystem](../Source/EchoesOfTheBrokenSun/Public/EchoesGameplayAudioSubsystem.h); [InterfaceAudioSubsystem](../Source/EchoesOfTheBrokenSun/Private/EchoesInterfaceAudioSubsystem.cpp); [NarrativeSubsystem](../Source/EchoesOfTheBrokenSun/Public/EchoesNarrativeSubsystem.h) | Audio/subtitle systems exist. Full audibility, performance and perception are not verified here. |
| V1 | [CinematicSubsystem](../Source/EchoesOfTheBrokenSun/Private/EchoesCinematicSubsystem.cpp) around 484–565, 580, 755–850; Requirements REL-CIN | Generic camera-track construction for title/acts/endings; separate M01 shots. Not finished cinematic production evidence. |
| V2 | [EntityView](../Source/EchoesOfTheBrokenSun/Private/EchoesEntityView.cpp); [DestructionView](../Source/EchoesOfTheBrokenSun/Private/EchoesDestructionView.cpp) around 17–180; [CombatEffectView](../Source/EchoesOfTheBrokenSun/Private/EchoesCombatEffectView.cpp); Requirements REL-ART | Visual mechanisms and common destruction geometry; production art/motion quality requires actual inspection. |
| F1 | [GameplayFeedback](../Source/EchoesOfTheBrokenSun/Public/EchoesGameplayFeedback.h); [PlayerGameplayFeedback](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerGameplayFeedback.cpp); [RequirementsState](RequirementsState.md) around 3267–3300, 3368–3401, 3628–3725 | Active feedback work and scoped historical fixes; candidate39 save/recovery evidence remains bounded. |
| D1 | [Requirements](Requirements.md) SPEC-MOV-001, environment section 7.2, SPEC-BLD-009, TBR-SCP-003, REL-MP-013/018/019 | Design constraints, unresolved breadth proposals and precise hosted-service deferral. |
| B1 | [Blizzard simplified controls](https://news.blizzard.com/en-us/article/6640645/game-guide-simplified-controls) and [special control](https://news.blizzard.com/en-us/article/4552955/game-guide-special-control) | SC2 reference for selection, groups, queued actions, macro/camera access; not live SC2 testing. |
| B3 | [Blizzard map elements](https://news.blizzard.com/en-us/article/4546768/game-guide-map-elements) | SC2 terrain/vision/objective comparison, adapted only through Echoes authority. |

### Current evidence that must not be erased by the gap list

RequirementsState records candidate39's focused 7/7 and full Unreal 117/117 checks, plus historical/current rendered keyboard save–exit–restart–recover routes on 2026-09-09. Those are recorded prior-session results, **not rerun by this audit**. The same entry preserves physical-mouse/held-modifier, human-comprehension and owner-acceptance limits, and calls historical-load waiting UX unpolished. Later Power Link, Relay and Bulwark fixes supersede older negative snapshots only within their documented scope. This report therefore does not repeat the original preview-color or depleted-worker-retarget defects as current facts.

The source was being edited by other tasks during inspection. The retained [source identity](../BuildArtifacts/Evidence/sc2-gameplay-gap-audit-20260909/source-identity.json) and [Git status](../BuildArtifacts/Evidence/sc2-gameplay-gap-audit-20260909/git-status.txt) identify one audit checkpoint, not a frozen/rebuilt release candidate. Preserve and recheck changed files before implementing a row. The initial read-only audit owned only this document and its evidence directory; during that initial audit no gameplay files, requirements, acceptance records or active presentation work were changed. The subsequent documentation intake is recorded in the handoff below.

**Document check:** one authoritative Markdown report, edited in place; author/owner is Angelis Pseftis. Inventory/source-link/status checks are document QA only. No new gameplay acceptance is assigned.

## Requirements capture handoff — 2026-09-09

The owner's follow-up requested full integration into ongoing requirements work. All 162 functional references and 32 feedback events now have an explicit master binding/disposition. Missing camera bookmarks, session chat and live-observer access received bounded requirements; existing display recovery and unnumbered passage contracts received explicit acceptance identities. The seven optional tutorial chapters remain controlling. Optional/contradictory branches have named decisions; hosted services and excluded mechanics were not silently adopted. Audit M/P/V/Q/D counts above remain the original source-inspection classification, not updated lifecycle states. Continue through the linked master/state and existing P0–P7 plan.
