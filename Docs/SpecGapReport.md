# ECHOES OF THE BROKEN SUN — GAP REPORT AGAINST THE NEW BINDING SPEC

**Author and owner:** Angelis Pseftis
**Standing:** historical assessment of 2026-09-02, retained for traceability.

The observations below apply to that inspected baseline. They are not a current defect list and were not
re-executed in the 2026-09-04 documentation audit. Use [Requirements.md](Requirements.md),
[RequirementsState.md](RequirementsState.md), and current source/evidence before acting on a finding.

**Prepared for Angelis Pseftis. Read-only assessment. No builds, no editor, no test runs.**

---

## The blunt fact

**The build is a working deterministic RTS engine wearing a campaign, and it is roughly at the end of the spec's *Foundation* phase — not partway through Campaign.** Phases 2 through 8 of §28.1 have real fragments but no phase after Foundation meets its exit condition, and several of those fragments do things the spec forbids.

Three findings dominate everything else:

1. **Destroying the enemy Command Core makes every campaign mission FAIL.** All fifteen operations run as two-base skirmishes; every mission model treats `Outcome() != Ongoing` as a failure predicate. The player who wins the battle is shown MISSION FAILED, the tick loop freezes, and no campaign record is written. This is the exact inverse of OUT-004, and it is undisclosed in every briefing.
2. **Nothing in this game blocks a unit except terrain.** The only passability test in the simulation reads a terrain tile and never looks at entities. Units walk through enemy soldiers, neutral objects, buildings, and each other. MOV-003 body blocking, footprint clearance, reserved goals, and formation cohesion are absent, not partial.
3. **The fog does not hide the map.** Every blocked terrain tile on the whole 64×64 grid is instanced from tick 0 under a fog slab 6 units tall that ~159-unit cliff meshes tower over. Explored terrain is served live, not remembered — an enemy Well harvest repaints your map through fog. There is no object memory at all: a scouted enemy base vanishes the instant vision lapses.

Nothing here is unrecoverable. The simulation core underneath is genuinely good. But the distance from here to the spec is a rebuild of most of Core RTS, not a polish pass.

---

## What is SOLID and can be built on

This is not a hollow build. The following were verified by reading implementations, and survived adversarial re-checking:

**The deterministic simulation core.** `Source/EchoesSimCore` has zero `float`/`double`, zero wall-clock, zero filesystem, zero rendering. Signed Q22.10 fixed point, seeded `DeterministicRng` with serialized state, deterministic sorts before every serialization, ordered containers only. A true fixed-step 20 Hz accumulator drives it. This is the cleanest boundary in the project and everything else can be rebuilt on top of it.

**Save integrity and trust boundaries.** Snapshot load verifies an FNV-1a trailer *before* constructing the reader, then magic, a version whitelist, map bounds, an entity count bounded by remaining bytes before `reserve()`, and per-field numeric bounds. The campaign ledger does CRC32 + magic + exact length equation before allocating. Both writers are genuinely transactional: temp file → reopen-and-validate → rotate prior validated generation → atomic replace → rollback with distinct failure codes. Path traversal is rejected. This is release-grade work.

**The content pipeline.** Six authored JSON sources → validated (fail-closed on unknown keys, ID pattern, cross-references, numeric ranges) → deterministic compiled pack + SHA-256 sidecar, verified at load with stable error codes and no silent fallback. I recompiled from source and reproduced all three pack digests byte-for-byte. **All twelve unit stat blocks and all twelve building stat blocks match the new spec's tables exactly.**

**Fifteen authored mission contracts.** Real objective/failure state machines with spec-exact tile coordinates and tick windows (M05's 300/900 window, M10's twin anchors at 28,39 and 36,39, M13's 3-tile Crownfall index, M14's 160-tick crisis latch, M15's 320/280/240 base hold + 80/0/40/120 ending modifiers). Branch geometry genuinely inherits the M01 Well decision through a persisted 27-byte ledger. The four-ending eligibility math matches the spec axis-for-axis.

**Anti-rewrite protection.** Replaying a completed operation returns `ReplayConflict` and leaves the original decision unmutated, surfaced correctly on the result screen. CAM-005's hardest clause is done.

**Fair AI information.** `PlayerView` has a private constructor with `friend class Simulation`; the AI producer is a `static` pure function of that view. Enemy entities are redacted (health→1, movement/vision/attack/cargo/orders/production/cooldowns zeroed). The AI cannot cheat, structurally.

**Six implemented faction abilities** with complete authored rule blocks: Warform Adaptation (all eight values exact, replaces rather than stacks, interruptible, public), Resonant vibration sense (2200cm/40 ticks/200cm, anonymous, untargetable), Listening Spine detection, Aegis powered attack with a real transitive power-chain walk, Choir coherence charge, Relay Supply capacity.

**Combat hygiene.** No random accuracy, no crits, no armor-class multipliers, no friendly fire, no area damage, no suppression/morale/stun/stealth/regeneration. Every damage modifier is authored and named.

**Test and measurement infrastructure that exists and works:** native SimCore suite, UE automation tests, Python content/narrative/world suites, a frame-time profiler with encoded p95 thresholds, and a 60-minute soak harness.

---

## CONTRADICTS — the build does what the spec forbids

These are defects, not gaps. They must be removed, not merely completed.

### Blocking

| # | Finding | Where | Effort |
|---|---|---|---|
| C1 | **Corefall fails every campaign mission.** `Facts.bSkirmishStillOngoing` gates all 15 mission models; killing the AI core sets Player0Victory → Failed. | `Echoes*MissionModel.cpp` (15 files); `EchoesSimulationSubsystem.cpp:10733` etc. | medium |
| C2 | **Unlimited free Command Cores.** `IsBuildingType` accepts CommandCore; all three HQs cost 0/0; `Outcome()` counts any core with HP>0 and never checks `completed`. A player who plants spares cannot be eliminated. | `Simulation.cpp:184, 222, 2005`; `buildings.json:4,8,12` | small |
| C3 | **Pathfinding reads authoritative terrain, not player knowledge.** Units route around walls their owner never scouted — a free intelligence channel and an observable fog leak. | `Simulation.cpp:2220` (no PlayerId parameter) | medium |
| C4 | **Impossible orders are receipted as accepted, then silently deadlock.** Move admits any in-map tile, returns `Applied`, then `MoveTowards` fails forever with no message. `NO PATH` / `ROUTE BLOCKED` / `DESTINATION OCCUPIED` exist nowhere. | `Simulation.cpp:2680-2686, 2333` | medium |
| C5 | **Fog renders the whole map's terrain from tick 0.** Terrain instancing has no visibility gate; the fog overlay is an ankle-height decal that cannot occlude 159-unit cliffs. | `EchoesTerrainView.cpp:274-297`; `EchoesFogView.cpp:46-61` | medium |
| C6 | **Explored terrain is live, and objects are not remembered.** Enemy Harvests repaint your map through fog; scouted structures disappear from world and minimap on vision loss. | `Simulation.cpp:4072-4073, 4095` | large |
| C7 | **Two information boundaries, not one.** World render, terrain and minimap read `*Simulation` directly; only AI and network clients use `PlayerView`. Single-player is drawn from the omniscient object. | `EchoesSimulationSubsystem.cpp:18091, 17908`; `EchoesHUD.cpp:5959` | large |
| C8 | **Rate limiting suppresses the only warning of a terminal threat.** Command Core loss shares the `StructureLost` class under a blanket 4s cooldown; and with interface audio muted, critical alerts have no text/shape/minimap channel at all. | `EchoesInterfaceAudioSubsystem.cpp:179`; `EchoesSimulationSubsystem.cpp:17923` | large |
| C9 | **Future Wells are not contested sites.** A worker within half a tile seizes a dormant Well instantly and permanently; the authored 420cm radius, 300-tick capture and both 180-tick telegraphs are parsed into a struct nothing reads. Control can never change hands. | `Simulation.cpp:3311, 3306`; `EchoesContentSubsystem.cpp:1103-1115` | large |
| C10 | **Mirror matchups are actively rejected.** `[SKIRMISH_MATCHUP_INVALID]`; the setup cycler skips the opponent's faction. Only 6 of 9 required matchups are playable. | `EchoesSkirmishSetup.cpp:162-166, 409-416` | small |

### Structural / balance

| # | Finding | Where | Effort |
|---|---|---|---|
| C11 | Crownfall Basin Well is 35 tiles from one start and 49 from the other — **40% apart against MAP-001's 5% ceiling** (measured by BFS over the shipping tables). Nearest deposit 10 vs 5 tiles. Glass Scar second-deposit 46% apart. Only Soryn Confluence is fair. | `EchoesSkirmishSetup.cpp:441-590` | medium |
| C12 | **The human player starts every skirmish with an extra Soldier** — 12 local spawn entities vs 11 opponent, hard-coded into validation. | `EchoesSimulationSubsystem.cpp:3388-3411`; `EchoesSkirmishSetup.cpp:190` | small |
| C13 | Every ECO-001 Matter preset is wrong (320/500/800 vs 250/400/700), and the opening is a pre-built base plus a five-unit army rather than one Core and five workers. | `EchoesSkirmishSetup.cpp:394` | small |
| C14 | Gathering is **20× too fast** — `workRate` is added every tick, so a Surveyor fills in one tick (0.05s), not the specified 20 ticks. No deposit saturation exists; unlimited workers gather one node at full rate. | `Simulation.cpp:3045-3051` | medium |
| C15 | **The human gather loop is manual.** A full worker clears its order and idles until the player right-clicks a drop-off. Only the AI cycles. | `Simulation.cpp:3052-3054, 3074` | medium |
| C16 | `construction_ticks` is not ticks, it is work units. **Every construction time is ~9-10× faster than the binding value**, differs per faction, and divides by N builders with no falloff. A 400-tick Anchor takes 2.0s, not 20.0s. | `Simulation.cpp:3088`; `EchoesContentSubsystem.cpp:518` | medium |
| C17 | BLD-003 assist falloff absent — every extra builder adds a full 100%, not 60%/40%/0%. | `Simulation.cpp:3088-3090` | small |
| C18 | **No projectiles anywhere.** Damage is same-tick hitscan; zero occurrences of `projectile` in the codebase. CMB-003's 1,200 cm/s travel and impact resolution do not exist. | `Simulation.cpp:3120-3127` | large |
| C19 | Choir Identity **transition holds both bonuses simultaneously** for 160 of every 560 ticks — 130% damage AND 130% movement AND 125% vision. The spec forbids this by name; the intended liability window is the unit's strongest state. | `Simulation.cpp:1133-1157, 2833` | small |
| C20 | The Phase Anchor's entire stated purpose (5→4 Dawn coherence inside 700cm) **does not exist in code, rules, or schema** — the content compiler would reject a `coherence` key today. It is a 120/35 structure that costs upkeep and does nothing. | `Simulation.h:468-475`; `compile_content.py:349` | medium |
| C21 | Reshape **manifests nothing on any shipped map** — its only effect is un-blocking Blocked tiles, and all three maps' Well neighbourhoods are already passable. `reshapeVariant` is a random number nothing reads. | `Simulation.cpp:1296-1310, 3357` | large |
| C22 | Reshape expiry **teleports units an unbounded distance and wipes their orders**, where MOV-004 requires stopping at the last safe position with the order preserved. | `Simulation.cpp:3755-3762` | small |
| C23 | Terrain `Scarred` has no 85% speed penalty and is invisible to routing; there is no "stabilize" mechanic, so scarred build space is permanently lost. | `Simulation.cpp:2347-2358, 2220` | small |
| C24 | Skirmish spawn validation clears Meridian/Kharuun HQ footprints as **2×2 against a simulation that places 5×5** — a stale `DefaultSimulationRules` table only half-migrated. | `EchoesSkirmishSetup.cpp:46-52`; `Simulation.cpp:807, 833` | small |
| C25 | That same stale table gives the **Meridian Surveyor and Kharuun Tender attack weapons**, which the spec forbids twice, and the entire native SimCore suite runs against it. A green native run is not evidence about the shipped roster. | `Simulation.cpp:799-870`; `Tests/Native/SimCoreTests.cpp:1748` | small |
| C26 | Chorus Loom sight is 600cm, not 550 — vision is integer tiles, so the value is unrepresentable. | `EchoesContentSubsystem.cpp:517` | medium |
| C27 | Three of five AI doctrines have the **wrong Well preference** (Warden/Expansionist/Adaptive all hard-coded to Harvest), and a sixth undocumented `BALANCED` doctrine ships in the selector. | `Simulation.cpp:4563-4567`; `EchoesSkirmishSetup.cpp:343, 434` | large |
| C28 | Every campaign operation deploys the **identical twelve-entity force on the same map**. M06 ("no initial production") gets a Barracks; M08 ("no full base") gets a full Meridian base. §18.1's per-mission packages are not represented. | `EchoesSimulationSubsystem.cpp:3388-3411, 3563-3565` | large |
| C29 | **M03's inherited district order is displayed but not enforced** — power them in any order and the mission completes. Three "authored reserve plans" are mechanically identical. | `EchoesCityReserveMissionModel.cpp:38-66` | small |
| C30 | **M09 treats "a district is already powered" as proof the authority site was secured** — one boolean disjunct means Mara never has to go there. | `EchoesSimulationSubsystem.cpp:11235-11241` | small |
| C31 | **M05 has a seventh, unauthored, hidden instant-fail**: moving a witness *toward* the extraction tile before tick 900 fails the operation, reported as `generic`. This is precisely what OUT-005 forbids. | `EchoesSimulationSubsystem.cpp:10917-10931` | small |
| C32 | M01's HUD asserts the contract's own prohibited claim: "RECOVERED — MARA VEY SECURE" when the scout reaches a tile. Mara is not an entity in the mission. | `EchoesHUD.cpp:2646-2650` | small |
| C33 | **A near-miss right-click silently becomes Move.** No proximity snap, no plausibility test — the spec forbids this by name and the default context order is built on it. Additionally, an accepted Deliver is silently rewritten to Move per-unit while the banner still reads DELIVER. | `EchoesPlayerController.cpp:9077-9127, 9456-9467` | small |
| C34 | The **shipped default keybinds contradict §20.1 on 12 of 16 rows** (A/S/P/G, B/R, Q/W/E, C, F, Tab, Space, Ctrl+1-0, F1/F2/F3). A, S and P are load-bearing for camera and pause, so adopting the table is a scheme-wide rebind. | `Config/DefaultInput.ini` | medium |
| C35 | HUD scale clamps to **85–135%, not the required 80–150%**, in three independent places including a hand-edited config. | `EchoesGameUserSettings.cpp:7-8`; layout headers | medium |
| C36 | **Reduced-flashing deletes information rather than preserving it** — the damage pulse is set to zero duration and the colour held, so the "unit was hit" event disappears. Under reduced motion, the Well protocol is left with colour as its only channel, which ACC-001 forbids outright. | `EchoesEntityView.cpp:711-713, 819-826, 686` | medium |
| C37 | **Ability aura radii are hard-coded mesh scales** — the relay disc renders ~82cm against a true 1400cm, the Aegis disc ~68cm against 1600cm. The player reads a false area of effect. | `EchoesEntityView.cpp:343, 352` | medium |
| C38 | Music masters are **−9.95 to −11.55 LUFS against a binding −16 ±1**, with no runtime normalisation, no limiter, and no side-chain ducking of any kind. | measured with the project's own BS.1770-4 tool | medium |
| C39 | **Saves are fully synchronous on the game thread** — four whole-file I/O passes and three complete snapshot decodes per quicksave, blocking the frame. "Background completion reports success/failure" is architecturally contradicted; the 250 ms budget is never measured. | `EchoesSimulationSubsystem.cpp:7779-8330` | large |
| C40 | **The only packaging path produces a Development-configuration, ad-hoc-signed, unnotarized, un-stapled app with no installer**, and actively *fails the run* if the signature is anything but ad-hoc. The script prints "not release-qualified." | `Scripts/package_macos.sh:398, 455-466, 804` | large |
| C41 | Decorative art contradicts collision truth: 7 of 12 shard clusters and two 5-tile "scar band" cliff ridges stand on fully passable tiles — including inside all three authored Glass Scar crossings — and render ~2.3× taller than genuinely impassable terrain. | `EchoesGameMode.cpp:2062-2158` | medium |
| C42 | The primary time readout everywhere is **raw ticks**; SIM-007 makes seconds mandatory and ticks optional. There is no elapsed-match clock at all. | `EchoesHUD.cpp:557-565, 4646-4712` | small |

---

## State by spec §28.1 build phase

### Phase 1 — Foundation *(exit: content schemas, deterministic simulation, commands, save/replay skeleton, test harness, player-scoped view)*

**Closest to done. Not closed.**

Solid: simulation core, content compiler, save transactionality, trust-boundary validation, the PlayerView type itself, and a real test harness.

Open:
- **Authority separation is violated (C7 + SIM-002).** Objectives, mission phase state, hold timers, resolution eligibility, named-witness entity IDs, and terrain/entity authoring all live in the 18,215-line Unreal subsystem, outside the snapshot and checksum. Six operations need bespoke quicksave envelopes to compensate; the other nine reconstruct handles from in-memory members, so their checkpoints are only reloadable in the authoring process session. **large**
- **Replay is a primitive, not a feature.** `ExportReplay`/`ReplayToEnd` work and are tested, but nothing ever writes a replay to disk — no format, no file, no browser. `LoadSnapshot` also clears `commandLog_`, so a loaded match cannot export a reproducing replay. SAV-004 partial, SAV-005 absent. **large**
- **No autosave, no checkpoints, no named slots.** A complete three-journey slot API exists and is dead code — its only caller is its own unit test. At runtime there is exactly one campaign ledger and one manual quicksave key. **large**
- Settings bypass the transactional contract entirely (engine ini flush, no temp/validate/prior generation). **medium**
- Command validation is a client-side pre-check on the local player only; the authoritative layer collapses every semantic rejection to `NoEffect`, deliberately, and nothing consumes the resolution receipt. Path is never validated. **large**
- No difficulty concept exists anywhere in the build — zero hits project-wide.

### Phase 2 — Core RTS *(exit: selection, input, camera, movement, terrain, fog, economy, construction, production, combat, outcomes)*

**This phase is the largest hole in the project and it is not partly done.**

Absent, with no substrate to build on:
- Entity collision, avoidance, soft separation, yield rules, chokepoint clearance (**large**)
- Path cost model — the BFS is uniform-cost and 4-connected, so terrain type cannot influence routing (**medium**)
- Footprint-aware pathing; reserved goals; deterministic formation slots by footprint/speed/ID (slots are selection-array order); regroup; cohesion (**large**)
- Sight and fire occlusion — `UpdateVisibility` is an unoccluded disc; attacks are range-only. Cliffs and walls are shoot-through and see-through. (**large**)
- Water/void terrain class — not merely unauthored, the 2-bit wire format cannot represent it (**small**)
- The entire §7.1 Kharuun subsurface passage system — zero code, zero data, zero mentions (**large**)
- Last-known contact state, public-event telegraph, alert records of any kind (**large**)
- The whole §8.1 reconnaissance layer: four orders, six SCT rules, policies, authority filter (**large**)
- Rally points, production queues (depth is 1, not 5), reorder, cancel, refunds, repair (repair does not exist anywhere — it is a listed common order for all three workers) (**large**)
- Stances — no stance concept exists; the *default* behaviour is Hold Fire, since an idle unit never acquires or returns fire (**large**)
- Overkill avoidance, threat-based target priority, wind-up/recovery, unit facing (**medium–large**)
- 200-tick remains, salvageable wreckage, exhausted-deposit markers, drop-off assignment/priority, idle-worker marker and alert, SPAWN BLOCKED alert (**medium**)
- Stalemate rule, single-player concession, AI concession (**medium**)

Present and usable: Move/Attack/Attack-move/Patrol/Guard/Hold verbs, Box/Line/Wedge arrival shapes, mineral cover as a real segment-tested projectile blocker, terrain-change repathing, deterministic combat resolution, the Corefall/Draw outcome rule.

### Phase 3 — Faction slice *(exit: Meridian + Kharuun rosters, abilities, structures, AI, Glass Scar, Well protocols, tutorial path)*

Solid: twelve named units and twelve named structures with spec-exact stats, distinct meshes and display names; Glass Scar terrain and its three crossings; six working abilities.

Open:
- **Six of twelve units have no signature rule at all**: Surveyor Network Repair, Tender Stabilize Scar, Riftstalker Slipfire, Threadkeeper Reconcile Structure, Lacuna Warden Bind Interval, Afterimage Forked Trace. Two of these need mechanics that do not exist in the sim (repair, debuffs). **large**
- Bulwark Deploy is instantaneous — the 20-tick setup and 15-tick pack, i.e. the entire commitment cost, are missing, and the test encodes the instantaneous behavior. **small**
- Of ~35 selection options the spec lists across the twelve structures, **only "produce a unit" and "research" exist.** No rally, no queue, no repair, no drop-off priority, no hold-fire, no target priority, no coherence forecast, no per-structure inspection. **large**
- The AI produces only Workers and Soldiers and never replaces its starting Heavy/Scout, so after the opening dies it fields two archetypes for the rest of the match and never builds a defense, detection or coherence structure. **medium**
- Tutorial path: the §21 curriculum reducer exists, is well designed, and **has zero runtime callers** — no code emits any `tutorial_lesson_*` signal, and the narrative loader never parses the `demo` subtree the tutorial lives in. Ten of eleven lessons authored; the Readiness gate does not exist. Graded ABSENT after downgrade. **large**

### Phase 4 — Strategic breadth *(exit: Choir, remaining maps, technologies, reconnaissance, resource monitor, full AI doctrines/difficulty)*

- **Technologies: the strongest row in the build.** Six technologies, exact costs/ticks/prerequisites, real slot contention both directions, immediate retro-application to living units, irreversible. Downgraded from EXISTS only because integer truncation makes three technology/unit pairings deliver *zero* effect (Relay Skiff 6→6 damage, Afterimage 7→7, Lacuna Warden 9→9 vision) and the archive can only ever print one of two effect columns. **small** to fix.
- **Reconnaissance: 100% absent** (see Phase 2).
- **The §9.1 resource monitor does not exist.** The entire economy readout is one line: `Matter N Dawnshards N Logistics N/N`. No income windows, no worker breakdown, no per-deposit figures, no drop-off status, no Logistics bands, no commitment ledger, no forecasts, and none of the seven required alerts. A Hollow Choir structure is destroyed outright the instant Dawn is short at its charge tick — the exact event the spec requires be forecast three times first. **large**
- **AI: there is no strategic controller.** The entire opponent is one stateless ~620-line per-entity reflex function run once per second, differentiated across doctrines by three integers and two boolean branches. §16.3's eleven strategic states are absent; §16.2 difficulty is 100% absent; eight of AI-004's nine perceived-intelligence behaviours are absent. Its actual command rate (~1 per owned unit per second) exceeds Sovereign's ceiling threefold. **large**
- Maps: only Glass Scar has a compiled pack, and that pack declares `runtime_binding: "none"` and disclaims runtime authority in its own claim boundary. Live terrain for all three maps is C++ literals. **medium**

### Phase 5 — Campaign *(exit: persistent ledger, all mission contracts, rewards, branches, dialogue, cinematics, four endings)*

Solid: the ledger, the fifteen contracts, branch inheritance, the four endings, double-confirmation before the irreversible resolution, replay-conflict protection.

Open:
- **Campaign rewards do not exist** — no field, no enum, no code, and the ledger validator actively *rejects* any record carrying a fact bit outside the mandatory mask, so optional objectives need a format-version change, not a new bit. All fifteen reward cells in §18.1 are unimplemented. **large**
- **No capability manifest** for any operation; the five-state teaching vocabulary is unmodelled. **large**
- **No campaign map.** The player sees one F9-cycled operation name and a record count — never completed operations, their consequences, or available next operations side by side. **medium**
- **Failure reasons are computed, bound to authored lines, and then discarded unrendered.** All fifteen result screens show one hardcoded disjunctive sentence; M01 always reads "the archive carrier or withdrawal line was lost" even when you won on Corefall. The result overlay suppresses the subtitle lane for its entire lifetime, so this is not a line-ordering fix. **small**
- Scale/duration (MICRO/HYBRID/MACRO, 20–45 min) are declared nowhere. Acts have no data model — the 5/5/5 grouping exists only as an inline music-bed index, and the act names appear nowhere. **medium**
- Six of eight OUT-006 result fields absent; no elapsed time, resources, units, rewards, or optional objectives.

### Phase 6 — Presentation *(exit: production art, animation, VFX, UI, audio, voice, music, accessibility, localization readiness)*

- **There is no animation of any kind.** Zero skeletal meshes, zero animation assets, zero animation code. Every unit and building is a rigid static mesh. ART-002's entire state set is unimplemented. **large**
- **No weapon-fire, impact, ability, or terrain-transformation VFX exists.** Combat is audio plus a 0.18s colour tint. **large**
- **No voice audio in the build at all** — 308 authored lines across five speakers, none voiced; the Dialogue submix has zero registered entries and the game ships a dialogue volume slider that controls nothing.
- **Exactly one cinematic exists** — an 8-second camera push that no game code triggers, only an automation test. CIN-001's five required cinematics: zero.
- **There is no interface layer.** Every screen is printf-style `DrawText` on an immediate-mode Canvas; zero UMG/Slate widgets project-wide. The selection panel is one line showing no health, order, owner, cargo, stance or cost. No alert history, no build preview, no remapping, no resource monitor. **large**
- **Zero externalized strings** — no `LOCTEXT`/`NSLOCTEXT` anywhere; ~951 raw literals in the HUD alone. The narrative pipeline *does* author `loc_key` and `text_budget` and then strips both at compile time. LOC-001 ABSENT. **large**
- Six of ten ACC-002 settings absent (text scale, colour-vision presets, subtitle size/background, cursor scale, effect density). Five of six volume controls are unreachable by any player, and gain changes only take effect at the next map load. Subtitles are a single unwrapped line that overflows off-screen at 302 characters.
- Genuinely good here: procedural music families that implement their authored direction, twelve brief non-retro interface cues with two-tier rate limiting, faction-differentiated destruction audio, shape-coded command markers, and 24 hand-composed roster meshes with deliberate silhouettes.

### Phase 7 — Qualification *(exit: balance, performance, soak, save migration, adversarial testing, clean-machine package, documentation)*

- The baseline profiler exists with encoded p95 thresholds — but it gates render and GPU **separately** at 11 ms each where the spec says render **plus** GPU ≤ 11 ms, so the harness is more permissive than the requirement. No captured result is retained anywhere in the repo, so the target is unproven. **small**
- **No minimum-profile harness at all** (30 fps / 720p / Low / 8 GB M1), and no 6.5 GB memory threshold. **medium**
- Fog and path-burst budgets have **no instrumentation whatsoever** — no CSV stat, no scope counter, no constant. They cannot be evaluated even if someone ran the profiler today. **small**
- The 400-unit stress scene is an endurance *fixture*, not a controllable scene: excluded from Shipping, self-maintaining units outside the command stream, save/load/replay all refused. The 200-unit 1v1 target has no scenario.
- No repeated save/load/restart soak, no killed-process recovery test — exactly the loop that would exercise the four-pass synchronous save.
- MAP-001 spawn fairness fails on two of three maps and **nothing in the code measures or asserts the tolerance**.
- No graphics quality scaling of any kind (PLAT-002 ABSENT); no missing-asset fallback registry or diagnostic — a missing mesh renders nothing and a missing sound plays nothing, both without a log line.

### Phase 8 — Release

Not started. See C40. The only packaging path enforces the opposite of PLAT-003. Privacy (PLAT-004) is genuinely clean, with the caveat that the binary carries a Sockets/listen-server multiplayer stack for a feature §2.2 excludes from scope.

---

## Downgrades — how much to trust the first pass

Every grade was re-verified adversarially against the implementation. **Thirty-nine grades changed letter. Twenty-five of those came down from EXISTS.**

| Direction | Count | Examples |
|---|---|---|
| EXISTS → PARTIAL or worse | 25 | OUT-001, MOV-001, MOV-005 formations, Terrain Open, Mineral Cover, §12 factions, Relay Skiff, §13 roster, §13 binding values, §14 tech table, TEC-001, TEC-002, §17 starting resources, M03, M04, M05, M06/07/08, M09, M10, Meridian music, SimCore module |
| PARTIAL → CONTRADICTS | 11 | OUT-005, SIM-002, MOV-004, 7.2 bridges, 7.2 dressing, FOG-002, WEL-003, UI-003, UI-004 markers, Effects, AUD-003 |
| PARTIAL → ABSENT | 3 | Power Link selection options, TUT-003, Kharuun form |
| Letter upheld, evidence corrected | ~15 | ACC-003, CAM-001, ARC-001, §17 maps, Game adapter module, CIN-002, Style |

**The recurring failure mode was grading a clause instead of the requirement.** Typical pattern: verify sentence one, silently relocate sentence two to "a separate finding," and grade EXISTS. Five of the mission rows were graded EXISTS on correct tile coordinates while the ordering rule in the same paragraph went unenforced. Two rows were graded EXISTS because a prohibition held vacuously — nothing existed to violate it.

**Second recurring failure: negatives asserted from one grep.** Several "absent" claims were falsified by searching under a different name — weather and fog drift *are* built (`AEchoesWeatherView`), the `occluder` flag *does* exist in map-dressing data, `role` *is* parsed from `units.json`, cargo *is* in the simulation. Where the correction ran the other way it was worse, not better: the "one flat trim" masquerading as reduced dynamic range, the M05 hidden fail, the 7-of-12 shard clusters standing on passable tiles.

Take the corrected grades as the baseline. Treat any surviving EXISTS as a claim about a *clause*, not a requirement, unless the citation covers every sentence.

---

## Coverage — what was and was not assessed

**Assessed read-only, by reading implementations and citing file:line:** all requirement IDs across §6–§28 — outcomes, simulation, movement/terrain, fog/recon, economy, construction/production/research/combat/automation, the twelve-unit roster, the twelve-structure roster, technologies and Future Wells, AI/skirmish/maps, campaign structure and persistence, all fifteen mission contracts, interface and controls, onboarding/accessibility/localization, presentation and audio, saves/architecture/performance/platform. Content packs were recompiled and digests reproduced. Map fairness was measured by BFS over the shipping tables. Audio loudness was measured with the project's own BS.1770-4 implementation. Capture exposure was measured by direct PNG decode.

**Not assessed:**
- Advisory "How to use" / "Counterplay" prose rows that encode no testable engine behaviour.
- The per-mission "System focus" and "Primary purpose" rows for M02–M08 and M11–M14 objective-by-objective against §18.1 (the ledger contract and phase models were verified; the eleven mission models were not read end to end against their system-focus text).
- The minimap and AI clauses of the §7.2 route-change rule.
- Whether tuning produces the balance §12–§13 describes in play.

**UNKNOWN read-only, and what would settle each:**
- **Real path cost under load.** LRU thrash is argued from code, not a profile. `ProfilePathRequest` sits behind `ECHOES_SIMCORE_PROFILE` with no callers. → a profile build.
- **Whether the frame-time, memory and soak targets are met.** Harnesses exist; no captured result is retained in the repo. → run and retain them.
- **Cross-process determinism of a campaign checkpoint.** The native determinism tests exist but require compiling. → run the suite plus a cross-process Mission 03 load.
- **Choir form language** (§ art direction) — eight builder bodies unread, no Choir gameplay capture exists.
- **One-second unit readability** — a perceptual threshold nothing in the build measures.
- **Briefing truncation at the narrowest supported HUD scale** — the wrapper caps at three lines; longest briefing is 281 chars. → render the panel at 0.85 scale.

**Two standing warnings about the test suite:**
1. The native SimCore suite runs against `DefaultSimulationRules()`, which contradicts the shipped content for four of twelve units and gives two workers weapons (C25). A green native run is not evidence about the authored roster.
2. There is no test tying rendered output to a player-scoped view. The terrain-through-fog defect (C5) and the enemy-health-bar disclosure survive a fully green suite.

---

## Where the build actually is

Foundation is close to its exit condition and worth defending. **Core RTS — the phase everything else sits on — has movement without collision, fog without concealment, an economy that is manual and 20× mis-tuned, combat without projectiles or occlusion, and no stances, queues, rally points, repair, or alerts.** The Faction slice ships six of twelve signature rules and two of ~35 structure actions. Strategic breadth has technologies and nothing else. Campaign has a good ledger and fifteen contracts that instantly fail when you win the fight. Presentation has no animation, no VFX, no voice, no UI layer, and no localization. Qualification has harnesses without retained results. Release enforces the opposite of what the spec requires.

The most efficient path is to close the CONTRADICTS list first — most of those are small or medium, several are one-line fixes (C2, C19, C30, C32), and each one removes a defect that would otherwise be inherited and re-tested at every later phase — then treat Core RTS as new work rather than as work in progress.