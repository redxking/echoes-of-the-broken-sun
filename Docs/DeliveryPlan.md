# Delivery plan — coherent, owner-visible progress

**Author and owner:** Angelis Pseftis
**Adopted:** 2026-09-02
**Maintained:** 2026-09-05
**Standing:** sequencing reference under [AGENTS.md](../AGENTS.md) and the [authority map](README.md).

The owner rejected a technically demonstrated prototype that did not yet provide the expected player
experience. Keep the recovery sequence focused on usable outcomes. [Requirements.md](Requirements.md)
defines those outcomes; [RequirementsState.md](RequirementsState.md) records evidence and acceptance.
This plan does not assert which phase is currently complete.

The [2026-09-05 code audit and execution plan](#code-audit-and-execution-plan--2026-09-05) below
provides the concrete next work packages. It is a dated assessment of the inspected source, not a new
requirements authority or acceptance record.

Use the current owner's task scope and accepted decisions to choose the next slice. Preserve the active
work rather than restarting from a dated phase note. One write owner per checkout is the default;
bounded independent read-only review and explicitly disjoint work are allowed under the shared contract.
The retired lane-fleet roster and missing lock files do not govern current work.

## Sequence and observable outcomes

| Focus | Work to qualify | Outcome to demonstrate |
|---|---|---|
| Repair relevant failures | Reproduce and resolve current failing checks in the affected path. Historical failure lists are investigation leads, not current results. | A trustworthy baseline for the next slice, with unresolved unrelated failures disclosed. |
| Reliable controls | Selection, movement, targeting, formation use, placement, focus, and recovery from invalid input. | The player controls a real match reliably with mouse and keyboard. |
| Readable battlefield | Distinct units, buildings, terrain, effects, fog, camera, menus, and HUD across required settings. | The player can read ownership, threat, objectives, and available actions in motion. |
| Learning by playing | Progressive teaching integrated with actual gameplay and feedback. | An uncoached new player understands the premise and reaches the required first success. |
| Story and sound | Approved character performance, subtitles, opening, music, ambience, feedback, and cinematics. | The player understands the characters' immediate stakes and connects actions to the story. |
| Connected campaign | Fifteen missions on fifteen distinct story-driven maps, with a coherent geographic and narrative journey. | The player experiences connected battles in one large world, with character and consequence continuity through the ending. |
| Conquest and multiplayer | Separate 25-sector Conquest/roguelite progression, seeded runs, team/FFA/comp-stomp maps, sessions, security, reconnect and host migration. | Packaged complete runs and supported multiplayer formats work at full participant load, with truthful results, persistence, isolation and required human/owner review. |
| Distribution and full qualification | Required package, performance, stability, accessibility, rights, installation, and owner gates. | The identified release candidate has the exact evidence required by the master. |

The last two rows reflect the owner's explicit 2026-09-04 clarification of the campaign experience; they do
not add an MMO or seamless world streaming. The owner separately approved Conquest/roguelite and
team/FFA multiplayer on 2026-09-04; those release obligations and map-format bindings remain separately qualified.

Define a check before implementation, make a bounded change, and inspect the result. Continue authorized
reversible work without pausing for intermediate review when the owner has delegated those decisions.
Human acceptance remains a separate recorded event; internal qualification never supplies it. A genuine
canon or scope decision blocks only the dependent portion and is recorded with options and consequences.

## Representative mission before campaign-wide production

The owner authorized the 2026-09-04 conflict-reconciliation sequence. First repair requirement identity and
source/index traceability and align mission references with the master and creative canon. Then use M01
as the representative production/qualification slice defined in
[MapConcepts.md](MapConcepts.md#m01-representative-production-brief) and
[MapTechnicalBlueprint.md](MapTechnicalBlueprint.md#m01-end-to-end-qualification).

Bind its distinct map, implement its approved narrative delivery, and qualify ordinary entry, all three
Well choices, failure/retry, evacuation, persistence, M02 continuation, role readability, motion and sound.
Fix the observed defects before treating its construction and interaction patterns as reusable. Extend
that proven method to each mission's distinct geography, branch sites, characters and consequences.
An M01 success never substitutes for M02–M15, skirmish, release, or human/owner gates. Exact progress and
current runtime dependencies remain in RequirementsState.md, not this sequencing reference.

## Planning maintenance

Keep live status in the state record. Update this plan only when the sequence or rationale changes.
Preserve historical failures and results in their existing evidence records; remove stale line counts,
fixed suite totals, and old implementation diagnoses from active instructions. Re-read source and tests
before proposing a repair. Completion of a document, phase, test, or release is reported at its actual boundary.

## Hosting deferred until after this version

Owner direction on 2026-09-04 defers multiplayer hosting services, hosted relay deployment and associated
service spending to the next game version, after this version is fully completed. Prioritize the current
game and M01 qualification. Current direct/LAN session modes retain their own security, recovery and
qualification obligations and must disclose unsupported connectivity. Conquest, team and FFA scope is
otherwise unchanged; read REL-MP-013/018/019 for the hosting applicability boundary.

## Code audit and execution plan — 2026-09-05

### Assessment and first playable target

The project has a substantial RTS simulation and campaign foundation. The remaining work includes
unfinished gameplay integration and missing player-facing systems, as well as content production and
qualification. It is not defensible to describe the remaining effort as only polish or testing.

The first delivery target is the existing demo journey: **cold launch with a fresh profile → usable
title menu → opening story → progressive playable tutorial → complete match against AI → victory or
defeat → results → working replay/restart/menu routes**. M01 is the representative campaign production
slice and must also demonstrate its three Well choices, failure/retry, persistence and M02 continuation.
A working M01 map alone does not satisfy this target. Controlling records are `DEMO-JRN-001..007`,
`DEMO-TUT-*`, `DEMO-AI-003..009`, `SPEC-MSN-001` and `SPEC-CAM-041..042`.

The full approved game remains larger: fifteen distinct campaign maps and four endings, all three
factions and three offline skirmish maps, the separate 25-sector Conquest mode, supported direct/LAN
team/FFA/comp-stomp sessions, production presentation, accessibility and Mac release qualification.
Hosted relay/services remain deferred. This sequence does not waive any full-release requirement.

### Inspection identity and verification boundary

This audit inspected `release/world-map-concept-pass` at
`b7adbb4b00add12980812decdb72a44ab4a8e544`, including the substantial staged and unstaged work present
on 2026-09-05. The commit alone does not identify those edits. File hashes and the initial Git status
are retained in [the audit evidence directory](../BuildArtifacts/Evidence/playable-code-audit-20260905T142243Z/).
No gameplay source was changed by this audit. This document remains the one delivery-plan authority,
edited in place with authorship retained as Angelis Pseftis.

| Evidence inspected or executed | Observed result and applicable limit |
|---|---|
| Current `bash Scripts/test_sim.sh`, executed 2026-09-05 at 14:23 UTC | **Compilation failed before any tests ran.** `Tests/Native/SimCoreTests.cpp:187` defines unused `ConvertSnapshotV27ToV26`; strict `-Werror` stops the optimized build. Debug and sanitizers were not reached. See [current log](../BuildArtifacts/Evidence/playable-code-audit-20260905T142243Z/native-current.log) and [result](../BuildArtifacts/Evidence/playable-code-audit-20260905T142243Z/native-current-result.json). |
| Current requirement registry, campaign-map compiler `--check`, landmark compiler `--check` | All three checks passed. The campaign compiler found 17 current outputs. These establish identity/index and generated-source consistency, not gameplay, art quality or acceptance. See [static results](../BuildArtifacts/Evidence/playable-code-audit-20260905T142243Z/static-results.json). |
| Earlier native regression evidence, 2026-09-05 | The retained RTS run reports 100/100 in optimized, debug and sanitizers. A later Slice 1 log reports 91/100. Both precede the current compile failure and intervening test/source edits; neither is the current suite result. |
| Retained Unreal automation report, 2026-09-05 at 12:43 UTC | [Report](../BuildArtifacts/Evidence/rts-regression-implementation-20260905T112003Z/unreal-automation-4/index.json): 81 succeeded, nine failed. Failures include `CompleteSkirmish`, `FreshJourney`, and seven campaign checks. Some assertions concern older snapshot schemas; others concern mission/Well progress or reaching victory. Later build evidence reports a successful editor build at 13:06 UTC, but 13 of its 236 recorded source inputs differ from the audited tree. This is a regression investigation list, not a claim that all nine still fail. No Unreal run was performed in this audit. |
| Inspected September 4 package | [Provenance record](../../BuildArtifacts/Packages/Mac-Development-20260904T171524Z-c3dc5722/EchoesOfTheBrokenSun.provenance.json) identifies source `c3dc5722`, a Development app, ad-hoc signing, no notarization/stapling and a rejected Gatekeeper assessment. It records startup smoke success and explicitly says `not-release-qualified`. It does not cover this dirty `b7adbb4b` tree or establish an accepted player journey. |

The following findings are source-review observations and dated evidence gaps. They do not assign
requirement lifecycle states or owner acceptance; those remain in [RequirementsState.md](RequirementsState.md).

### What exists and what still needs work

The deterministic core already includes fixed-step movement/pathfinding, fog-scoped views,
gathering/delivery, construction, production, research, combat/projectiles, Command-Core outcomes,
snapshot persistence and command replay. Faction-specific rules include deployment, relay supply,
rooting, adaptation, mineral cover, power and Choir coherence. Unreal adapters, mission reducers,
fifteen campaign map identities, text narrative, basic controls and presentation are present.
Preserve and integrate these systems. Their existence does not establish every required ability,
balance outcome or player route.

| Finding | Concrete gap and implementation consequence | Source anchors and controlling records |
|---|---|---|
| F01 — Regression baseline | The unused schema-27 conversion helper stops the native gate. The migration chain still passes a current snapshot directly to `ConvertSnapshotV26ToV25`. Connect and test the actual conversion chain; then investigate remaining schema/Well assertions against intended behavior. Do not suppress the warning or replace checksum constants merely to obtain green output. | [Native tests](../Tests/Native/SimCoreTests.cpp), lines 187 and 1859; [snapshot contract](../Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h), line 37. `SPEC-SAV-003`, `REL-SAV-*`, `REL-SIM-*`. |
| F02 — Misleading setup choices | The UI cycles Corefall, Well Control and Conquest, but the outcome engine evaluates surviving completed Command Cores. Conquest is an approved separate mode, not an alternate skirmish win selector. Remove/clamp the false choices and handle saved configurations explicitly. Difficulty selections also exceed demonstrated behavior: the runtime distinguishes Assisted cadence, while higher tiers follow the ordinary path; its logged 0.80 damage modifier has no matching application in the inspected dispatch/core path. Implement the prescribed differences and accurate disclosures. | [Setup model](../Source/EchoesOfTheBrokenSun/Private/EchoesSkirmishSetup.cpp), lines 670–675; [outcome](../Source/EchoesSimCore/Private/Simulation.cpp), lines 2157–2196; [AI dispatch](../Source/EchoesOfTheBrokenSun/Private/EchoesSimulationSubsystem.cpp), lines 16493–16560. `SPEC-OUT-*`, `SPEC-DIF-*`, `DEMO-AI-003/005`, `REL-AI-033`. |
| F03 — Front door and tutorial | The title is an operation/briefing selector rather than the required mode/options/tutorial hub. The curriculum reducer has no discovered runtime consumer; its tests supply lesson facts directly. A player cannot receive the required progressive teaching and persisted mastery simply because this model passes tests. | [Title layout](../Source/EchoesOfTheBrokenSun/Public/EchoesTitleOverlayLayout.h), line 57; [curriculum tests](../Source/EchoesOfTheBrokenSun/Private/Tests/EchoesTutorialCurriculumTest.cpp), lines 9–46. `DEMO-JRN-*`, `DEMO-TUT-*`, `REL-FTU-003..012`. |
| F04 — Battlefield interaction | The minimap is rendered but consumed as generic HUD chrome instead of panning/ordering. Middle-mouse camera drag is absent from the inspected bindings. Modal pause disables camera/selection/orders, whereas active tactical pause requires those functions. Context cursor feedback remains a static crosshair. These need real input dispatch and recovery paths. | [HUD layout](../Source/EchoesOfTheBrokenSun/Public/EchoesHudLayout.h), line 122; [controller](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp), lines 9411, 12577 and 12940; [camera](../Source/EchoesOfTheBrokenSun/Private/EchoesRTSCameraPawn.cpp), line 478. `SPEC-CTL-012`, `DEMO-INP-*`, `REL-UI-005/011`, `REL-ACC-018`. |
| F05 — UI/settings integration | Player UI remains Canvas `DrawText`/`DrawRect`; `REL-UI-001` requires modular UMG/Slate. Settings are mostly hotkeys, input user settings are disabled, and HUD-scale endpoints differ by mission. Build focusable title/pause/options/HUD/result components, externalized text and persistent accessibility/remapping. Keep the simulation independent of this replacement. | [HUD](../Source/EchoesOfTheBrokenSun/Private/EchoesHUD.cpp), lines 459, 2530, 5380; [input configuration](../Config/DefaultInput.ini), line 111; [HUD scaling](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp), line 11784. `REL-UI-001/008`, `REL-LOC-001`, `REL-ACC-*`. |
| F06 — Save, results and replay UX | Atomic/checksum/backup primitives exist, but named journey slots lack discovered runtime callers and no complete profile/slot/recovery UI is connected. Current writes are synchronous; campaign phase autosaves exclude skirmish. The results action labelled Replay restarts the scenario. Core replay APIs do not supply recording persistence, a browser or playback controls. | [Save/autosave adapter](../Source/EchoesOfTheBrokenSun/Private/EchoesSimulationSubsystem.cpp), lines 8517, 10130 and 10414; [slot store](../Source/EchoesOfTheBrokenSun/Private/EchoesCampaignProgress.cpp), line 1162; [results action](../Source/EchoesOfTheBrokenSun/Private/EchoesPlayerController.cpp), lines 12834 and 13007. `SPEC-SAV-001..005`, `DEMO-JRN-005/006`, `REL-SAV-*`, `REL-UI-009`. |
| F07 — Voice and cinematics | The narrative runtime queues speaker/text subtitles. The audio importer intentionally permits an empty Dialogue category under an obsolete subtitle-only assumption. M01 voice candidates explicitly remain unregistered and unbound. Cinematic code builds transient camera sequences, but no campaign-flow caller was found. Author and register approved performances, connect line IDs/audio/subtitles and story triggers, and qualify the audible route. | [Narrative runtime](../Source/EchoesOfTheBrokenSun/Private/EchoesNarrativeSubsystem.cpp), lines 505–527; [audio importer](../Scripts/generate_audio_assets.py), lines 152–171; [voice candidates](../Scripts/synth_m01_voice_candidates.py), lines 313–321; [cinematic runtime](../Source/EchoesOfTheBrokenSun/Private/EchoesCinematicSubsystem.cpp), lines 142 and 317. `DEMO-NAR-*`, `DEMO-AUD-*`, `REL-CIN-*`, `REL-AUD-020`. |
| F08 — M01 tactical and presentation closure | M01's documented Reshape region consists of already-open cells, leaving its promised terrain choice without a meaningful route change in that binding. Starting roster and extraction-anchor references conflict. Recent visual/camera/worker/Well changes need matched current-build interaction, motion and sound inspection. Resolve the geometry decision before polishing an ineffective terrain choice. | [Recorded decisions](RequirementsState.md), lines 2185–2220; [M01 map source](../Content/World/Source/Campaign/m01_glass-scar-evacuation-margin_v1.json); [representative map brief](MapConcepts.md). `TBR-M01-ROSTER-001`, `TBR-M01-ANCHOR-002`, `TBR-M01-RESHAPE-003`, `SPEC-MSN-001`, `SPEC-ART-004`, `SPEC-VISD-008`. |
| F09 — Campaign production | Fifteen mission/map identities and reducers exist. Registered mission-landmark presentation sources cover M01–M03; comparable M04–M15 presentation packs were not found in the inspected pipeline. Existing shared kits and map IDs cannot establish fifteen finished, distinct environments, integrated voices/transitions or all four ending playthroughs. | [Campaign registry](../Content/World/Generated/Campaign/campaign_map_registry_v1.json); [presentation sources](../Content/World/Source/Presentation/); [map blueprint](MapTechnicalBlueprint.md). `SPEC-MAP-004`, `SPEC-CAM-041/042`, `SPEC-MSN-001..015`, `REL-CAM-*`. |
| F10 — Conquest and multiplayer breadth | No Conquest run/sector implementation or authored run data was found in the inspected gameplay source. The simulation ceiling is four players, below required six-seat 3v3. The normal network setup is pinned to canonical 1v1 and Glass Scar. Complete the separate Conquest mode and explicit participant/team/map/session architecture; hosting deferral does not remove them. | [Player ceiling](../Source/EchoesSimCore/Public/EchoesSimCore/Simulation.h), line 32; [canonical setup](../Source/EchoesOfTheBrokenSun/Private/EchoesSkirmishSetup.cpp), lines 154–168; [network compatibility](../Source/EchoesOfTheBrokenSun/Private/EchoesNetworkSession.cpp), lines 46–68. `REL-CAM-033..038`, `SPEC-SKM-014..018`, `REL-AI-037..040`, `REL-MP-*`. |
| F11 — Full-match and release evidence | AI source exercises economic/military actions, but short deterministic smoke coverage does not establish a complete balanced match. No current source-bound packaged golden journey was established by this audit. Sustained performance, full participant load, save recovery, listening, unfamiliar-human play and distribution remain separate work. | [AI smoke](../Source/EchoesOfTheBrokenSun/Private/Tests/EchoesAiSkirmishDeterminismSmokeTest.cpp), line 27; [package script](../Scripts/package_macos.sh), lines 198–245; retained evidence above. `DEMO-AI-004/006/008/009`, `DEMO-VAL-*`, `SPEC-BUD-*`, `REL-QA-*`, `SPEC-PLAT-003`. |

Source line numbers refer to the audit snapshot. Before implementation, use the named symbols and retained
hashes to relocate changes. Absence findings describe the searched source/call graph, not an exhaustive
proof about every external file or possible runtime configuration.

### Ordered implementation packages

One integration owner controls shared gameplay, controller and HUD edits. Read-only review can run in
parallel. Voice/content production may run alongside engineering when its approved source paths and
outputs are disjoint. Builds, cooks and physical-play sessions remain serialized. The accountable roles
below are responsibilities to assign, not claims that another task has started work.
For every package and follow-up, apply the shared
[model/effort selection](Prompts/GameDevelopmentWorkflow.md#select-model-effort-and-work-ownership) and
[handoff contract](Prompts/GameDevelopmentWorkflow.md#delegation-and-additional-task-handoff).
Select the route from the actual task and verify its result; the package label alone is not a model choice.

| Package and dependency | Implementation scope and accountable role | Exit evidence |
|---|---|---|
| P0 — Restore the integrated baseline. Start here. | **Integration/core owner.** Preserve all dirty work; identify intended camera, harvesting, Well and persistence changes. Wire schema 27→26→older migration fixtures correctly, then repair actual regressions exposed by strict native runs. Revisit the nine retained Unreal failures against current rules and current adapters. Regenerate only from registered source. Findings F01, F08. | Native optimized/debug/ASan+UBSan all pass; generated-source checks pass; current editor builds; current Unreal report has no unexplained failure on the selected journey. Include invalid/corrupt and legacy snapshot cases, Well capture/telegraph/cancellation/expiry, harvesting interruption and projectile restoration. Record commit plus dirty-input hashes. Never treat a compile-only result as a playable build. |
| P1 — Complete the shell and persistence routes. After P0. | **Player-experience owner.** Introduce a single player-flow coordinator and modular widgets for title, mode selection, briefing, pause/options, save/load and results. Connect fresh-profile/mastery/settings persistence and three journey slots through existing storage authorities. Keep back, cancel, error, retry and exit routes explicit. Externalize strings as surfaces move. Findings F03, F05, F06. | Controller/widget tests traverse all routes, including cancellation, invalid saves and backup recovery. Fresh-profile routing survives restart; slots remain isolated; existing saves are preserved. Mouse and keyboard focus/activation work at display/HUD-scale endpoints. Shipping UI migration remains required under `REL-UI-001`. |
| P2 — Make one complete offline match reliable. After P0; shares P1 integration owner. | **Gameplay/player-experience owner.** Restrict the selector to the authorized Corefall contract; implement real difficulty policies and accurate modifiers. Finish minimap input, middle-drag camera, context cursors, selection/placement/order feedback, modal focus recovery and separate tactical pause. Integrate harvesting/depot behavior, construction, production, research, combat, defeat and deterministic restart. Complete recording/playback entry and distinguish Replay, Restart and Rematch. Findings F02, F04, F06, F11. | One normal victory and one loss reach correct results using actual mechanics, with no fixture-granted win. Saving/loading mid-economy, combat and Well activity preserves state. AI long-run fixtures reach terminal outcomes or report an actionable stall. Player-scoped information remains enforced. Replay reproduces recorded history; result actions return to usable play/menu. |
| P3 — Finish the representative M01 experience and teaching. Requires P1/P2 contracts; approved voice/art work can prepare earlier. | **Campaign/presentation owner with disjoint audio/art ownership.** Resolve the three M01 decisions below. Complete meaningful Harvest/Preserve/Reshape outcomes and visible consequences. Connect curriculum facts to real player actions and authoritative game events, with staged prompts, recovery and mastery. Wire the opening, approved voice playback, subtitle timing, mix and skip/control restoration. Finish M01 geography, unit/building identity, worker/combat motion, fog, effects and camera readability through the ordinary route. Findings F03, F07, F08. | Fresh profile receives story and learning before full AI access. Wrong actions cannot falsely complete lessons. All three Well routes plus failure/retry, evacuation, save/reload and M02 continuation work. Inspect normal camera and display settings in motion and listen to the complete sequence. This qualifies a representative slice only. |
| P4 — Deliver the first playable candidate. Requires P0–P3. | **Integration/QA owner.** Integrate the reviewed source through the existing package provenance process, preserving unselected work. Build the current candidate and execute the full demo journey from a clean test profile. Fix player-visible defects and recook after changes. The packager currently requires a clean source identity at pushed canonical main; prepare reviewed integration for that requirement rather than weakening or bypassing it. External Git actions remain a distinct execution step, not performed by this plan. | Same identified package: cold launch → menu → audible opening → player-driven tutorial/mastery → AI match → win and controlled loss → results → replay/restart/menu. Physical mouse/keyboard, focus loss, invalid input, settings, save recovery and baseline performance are demonstrated. Conduct the prescribed uncoached human sessions and submit the exact candidate to Angelis. Only his recorded acceptance closes the milestone. |
| P5 — Complete campaign, roster and offline breadth. Reuse patterns accepted in P4. | **Campaign/gameplay/content owners, disjoint work packages.** Produce M02–M15 as distinct places and complete their objective/branch/retry/checkpoint/transition chains. Finish all twelve units and twelve structures against their action/purpose contracts, including any missing repair/rally/research/queue/scouting behavior found in per-element checks. Complete all three maps, faction mirrors, five doctrines and four difficulties; qualify differences through actual matches. Author remaining voices, act transitions and four endings. Findings F07, F09, F11. | Mission-specific normal/failure/branch/save tests; continuous empty-ledger M01→M15 playthrough and all four ending routes; all declared faction/map/difficulty choices affect play truthfully. Per-element matrix covers selection, action, effect, cost, failure, counterplay, AI use and save/replay. Use focused causal balance tests and the required long-run matrix; do not infer balance from unit counts or short smoke tests. |
| P6 — Implement remaining approved modes. Begin after the offline foundation is stable. | **Mode/network owner.** Build Conquest's 25-sector data, seed/run reducer, encounters, rewards, win/loss/restart and independent persistence. Separately expand participant capacity and every per-player state/protocol/save/fog array for six seats; implement team outcomes, format/slot/AI lobby setup, map eligibility/spawns and result attribution. Resolve applicable authentication/peer-trust/reconnect/host-migration decisions before dependent network work. Hosted relay/provider deployment stays deferred. Finding F10. | Complete reproducible Conquest runs and recovery with campaign-state isolation. Direct/LAN 1v1/2v2/3v3/FFA/comp-stomp sessions pass their exact participant, security, disconnect/reconnect/migration, outcome and full-load checks. Four-player evidence cannot qualify six-player modes. |
| P7 — Qualify the complete release. Final gate after P5/P6, with profiling and accessibility work starting earlier. | **QA/release owner.** Finish cross-mode accessibility and localization readiness, production art/audio/cinematics, runtime event coverage and rights records. Measure frame time, memory, fog/path bursts, save latency and stability on required machines. Freeze a source-bound candidate; produce Developer-ID-signed, notarized/stapled distribution and clean-machine installation/recovery evidence. Complete manual/support materials and owner review. | Apply exact master thresholds: M1 Pro/16 GB at 1080p Medium 60 fps and baseline p95≤16.67 ms; base M1/8 GB at 720p Low 30 fps; corresponding memory/load gates; 60-minute rendered match plus prescribed multi-hour/headless tests and six-seat load where applicable. No unresolved release-critical defect without the exact permitted owner decision. Signed/notarized/installed/accepted are separate recorded outcomes. |

The following responsibilities are part of those package exits, not optional follow-up work:

* **P1 front door:** explicitly deliver Campaign, Skirmish, Tutorial, Options, Credits and Quit under
  `REL-FTU-003`, with the required hover/focus response and `UI_Hover`/`UI_Click` feedback. P1 owns the
  profile/mastery schema and denies full-AI entry before the demo's learning gate is satisfied.
* **P1 persistence:** capture immutable, tick-consistent state without letting a background writer mutate
  live simulation; move encoding/write/rotation work off the game thread as required. Implement objective
  and 6,000-tick autosaves for applicable offline campaign/skirmish, separate manual/autosave generations,
  and explicit interruption, corruption and backup recovery. Measure actual snapshot initiation and write
  latency against `SPEC-BUD-007`/`REL-SAV-007`; an asynchronous API alone is insufficient.
* **P2 results:** collect authoritative duration, trained/lost units, resources and Well decisions, plus
  the required command/APM and timeline records. Present the `REL-UI-009` dossier and verify that replay,
  restart, rematch and return-to-menu have distinct, correct behavior.
* **P3 mastery:** only verified tutorial actions may award completion. **P4** must demonstrate denial of
  premature full-AI entry, legitimate completion, persisted unlock after a cold restart, and physical
  cold-launch save recovery. Tests that inject a completed profile do not satisfy these gates.

P0–P4 are the immediate playable-game path. P5–P7 complete the approved game. Do not expand map production,
add a new faction mechanic or start hosted infrastructure to avoid closing an unfinished player route.
Conversely, do not postpone responsiveness, readable visuals, sound or performance until the final release
gate: the first playable candidate must already work as a coherent player experience.

### Decisions to resolve during execution

| Existing decision | Recommended disposition for the next implementation slice |
|---|---|
| `TBR-M01-ROSTER-001` — starting force | Resolve 6 Surveyors/2 Lancers versus the current 3 workers/3 line-unit deployment before tuning the tutorial or mission pacing. Use the current deployed force as a measured baseline; do not silently change the normative roster. |
| `TBR-M01-ANCHOR-002` — extraction anchor | Retain the current source-bound 6,17 route while preparing the owner reconciliation of legacy 42,18 text. Do not move the extraction point solely to satisfy an older coordinate assertion. |
| `TBR-M01-RESHAPE-003` — tactical terrain effect | Prepare one small authored route change with useful counterplay and occupied-cell/expiry handling, then obtain the owner decision on its geometry. The effect must create a real tactical alternative; a timed purple effect over already-open cells is insufficient. |
| `TBR-ECO-001` — faction work/cargo versus common throughput | Retain the approved single-extractor FSM. Resolve faction numeric throughput before final economic tuning; the later single-extractor decision already supersedes the old two/three-worker debate. |
| `TBR-DOC-003` — overlapping ducking policies | Select one explicit critical-dialogue policy before mix qualification; preserve essential combat/interface cues and verify the selected bus gains and attack/release behavior by listening and measurement. Asset registration can proceed independently. |
| `TBR-NET-001` — direct-session security and recovery | Decide participant identity/key lifecycle, peer hidden-state exposure, authenticated envelope, disconnect disposition and reconnect/host-election ordering before P6 implementation. Defer only provider/relay/service spending as already directed. |

These are prepared execution decisions, not requests to reconfirm this analysis task. No rule, scope,
canon or acceptance decision is changed by the recommendations here.

### Tracking, sizing and immediate handoff

Track progress by completed player routes and open behavioral defects, with evidence recorded against
the controlling IDs in RequirementsState. Avoid percentages derived from files, assets, test counts or
the number of requirement rows: this audit cannot convert those into a credible completion percentage.
No defensible completion date follows from source inspection alone. P0 is bounded regression/integration
work; UI/onboarding/voice/replay and the new modes are substantive implementation; the fifteen-map and
performance gates require measured production/qualification effort. Estimate later packages from the
accepted P4 slice and remaining per-mission/asset counts, not an assumed linear completion rate.

The next implementation assignment is **P0**, beginning with the schema conversion chain in
`Tests/Native/SimCoreTests.cpp`. Its completion packet must identify the integrated source, current
native and Unreal results, and any remaining failure's player impact. Then assign P1/P2 to a single
integration owner and prepare approved M01 art/voice assets in disjoint paths. Keep the existing core,
mission reducers and persistence authorities; add missing adapters and player flow rather than creating
parallel gameplay implementations. No implementation, new package, release or human acceptance is
claimed by this analysis-and-planning task.

### 2026-09-05 execution follow-up — P0 native gate restored

Following the owner's adaptive-execution instruction, the mandatory routing/handoff procedure was
installed in the shared contract and applied to a bounded P0 repair. The schema migration fixture chain,
valid Well snapshot restoration and abandoned-capture decay are repaired. The current dirty inputs passed
100/100 native tests in each of optimized, debug and ASan/UBSan configurations. The exact source hashes,
preserved failed attempt, scoped review and remaining limits are recorded in
[RequirementsState.md](RequirementsState.md#2026-09-05--adaptive-execution-policy-and-p0-native-baseline-repair).

P0 is still in progress: complete the current generated-source/editor/Unreal checks and reconcile any
remaining failures before advancing dependent integration. Reshape's required 180-tick public telegraph
(`REL-WEL-010` / `SPEC-WELLP-003`) remains an explicit P0/P3 gameplay gap. The next assignment starts from
this successful native receipt, not from the earlier compile failure. No packaged-play or owner-acceptance
gate has closed, and the earlier audit remains a historical assessment of its stated source snapshot.
