# Delivery plan — coherent, owner-visible progress

**Author and owner:** Angelis Pseftis
**Adopted:** 2026-09-02
**Maintained:** 2026-09-09
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

## Connected player input outcome — 2026-09-09

The P1/P2/P4 connected-input qualification is carried forward on route
`connected-input-20260909T204502Z`: phase1 and phase2 both exited 0, the route is finalized,
and scoped cleanup was verified. Pointer selection, command-card production with visible
hotkeys, settings persistence across a cold restart, quick save and reload, a terminal match
outcome with statistics, replay playback and archive persistence, and concession each
produced an observed result. A keyboard-only control path (Tab cycling plus a reticle) also
works, so keyboard and pointer are separately usable.

One reproduced defect was repaired: arming a build action overwrote its own prompt, hiding
both the blueprint instructions and every specific refusal reason. It is fixed, covered by a
regression assertion proven to fail without the fix, and rechecked in a rendered route.

The display half of the first open defect is now repaired: see
[Recoverable display-setting changes](RequirementsState.md#recoverable-display-setting-changes--2026-09-09).
The auto-revert restored the stored window mode rather than the live one, so a display
change made from a window whose presentation disagreed with the saved preference left the
window borderless at the full display. It now restores the presentation the player is
actually looking at, proven in a rendered route and by the engine's own viewport log.
Whether the field HUD reads correctly in a *deliberately* borderless session at a
sub-native resolution is untouched by that repair and stays open. Concession reporting a
Command Core loss that did not happen is now repaired: see
[Outcome cause after a concession](RequirementsState.md#outcome-cause-after-a-concession--2026-09-09).
The
[bounded evidence record](RequirementsState.md#connected-player-input-settings-recovery-result-and-replay--2026-09-09)
keeps agent-synthetic input, packaged execution, audio, performance, and owner acceptance
distinct — none of them is claimed. The predecessor route
`connected-input-20260909T201800Z` is retired unclosable after a concurrent lane changed its
pinned candidate mid-run; treat that as the reason to keep one writer per candidate while a
protected route is live, not as a defect in the game.

## Three-failure repair outcome — 2026-09-09

The requested controls-persistence oracle, resource-content overflow, and Options grouping repairs
are agent verified on one rebuilt candidate: four focused checks and all 133 Unreal tests passed
without warnings/errors. Rendered keyboard checks covered the resource strip at 80%, 100%, and
150%; a camera rebind and UI scale survived normal exit and a fresh process. The layout decision
is Accessibility → Controls → Camera → Display → Audio, with semantic paired-row assertions.
The [bounded evidence record](RequirementsState.md#controls-persistence-resource-strip-and-options-grouping--2026-09-09)
keeps pointer, packaged, durability, and owner-acceptance gates distinct. Continue the existing
player-journey sequence from this repaired baseline; do not restart these three investigations.

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


### 2026-09-05 execution follow-up — integrated P0 repair

The P0 automated baseline is qualified on the local dirty source based on `main` at `15008d55`.
Reshape now has its pending public warning and delayed activation; schema 28 preserves the authored
hostility relation, and same-frame movement/research scheduling is corrected. Native tests pass
**101/101** in optimized, debug and ASan+UBSan configurations. Generated-source checks and the current
editor build pass. The final protected Unreal run passes **92/92**, including all four FreshJourney
routes, ending conflict and reset/restore checks. Save protections and cleanup pass. All 356 candidate
inputs and seven native inputs matched their retained evidence at qualification.

Exact source manifests, retained failures, final `p0-qualification.json`, and the evidence boundary are in
[RequirementsState.md](RequirementsState.md#2026-09-05--p0-integrated-continuation-and-reshape-warning-repair).
This closes the local automated P0 integration gate, not requirement or owner acceptance. The latest
owner instruction keeps this continuation within P0; P1/P2 were not started. M01 geometry, packaged
play, physical interaction, rendered/audio review and owner acceptance remain separate open gates.


### 2026-09-05 execution follow-up — P1 authorized and underway

The owner subsequently instructed “Proceed to P1,” superseding the preceding P0-only scope note.
The active increment implements the shell coordinator, UMG menu routes, independent profile storage,
three selected journey slots and recovery controls. The current local editor build and protected suite pass 95/95;
rendered checks have verified the title, scale/contrast changes and keyboard modal navigation. The full
pointer/display matrix and remaining options controls keep P1 open. Requirement acceptance,
Shipping HUD migration, P2 result telemetry, P3 curriculum integration and packaged/owner gates
remain open. See the P1 entry in [RequirementsState.md](RequirementsState.md).


### 2026-09-05 execution follow-up — P1/P2 completion continuation

The owner authorized continuous implementation and verification through both P1 and P2. The selected
AI contract is Story / Standard / Veteran / Sovereign with equal combat rules and reaction delays of
3.0 / 1.5 / 0.9 / 0.5 seconds. Current source adds the full modular field HUD, asynchronous checkpoints,
recovery repairs, Corefall match/result routes and detached replay transport. Native qualification passed
102/102 in three configurations including replay/snapshot cancellation, streaming-history differential coverage, dense-tick receipt
retention and 64-bit APM.
Candidate 20 compiled and passed all 111 registered Unreal fixtures with zero test warnings/errors.
All 501 runtime input hashes, exact test inventory and save-isolation gates passed. Mac Shipping also
compiled, linked and received an ad-hoc local signature; this is not a fresh cooked or distributable package. The suite covers canonical
interrupted skirmish recovery, display/profile/mastery routes, normal victory/defeat, replay fidelity and
non-primary mouse refusal. The current 401-entity save measured 44 µs capture, 471 µs main-thread initiation
and 20.768 ms total completion; the tick-52,764 loss measured 181 µs, 653 µs and 3.139 seconds respectively.
These are development-fixture measurements on this host.

The implemented P1/P2 changes pass automated checks; their delivery exit remains open for the
rendered mouse/keyboard, actual display/HUD-scale endpoint and focus/control matrix. Candidate 19 rendered
Options at 80%/150% and exercised tutorial deployment and HUD production with native Slate input. OS/CUA
pointer evidence was inconsistent. A later standalone candidate-20 session received Quit/Confirm input
while the coordinator was reading metadata and exited normally, so live input is paused for an idle
computer window. No editor or heavy runner remains active. The next action is the coordinated standalone
input/display pass, followed by targeted repair/reverification where evidence requires it. Gemini See Loop
captures work, but analysis returned `API_KEY_INVALID` and is not claimed as visual QA.

Historical failed runs remain retained. See the current P1/P2 integration entry in
[RequirementsState.md](RequirementsState.md). This continuation does not advance P3/P4/P7 package,
distribution or owner-acceptance gates.

### 2026-09-06 — pushed P1/P2 checkpoint; P3 implementation started

The owner requested pushing all current work to main and then starting P3. Commit
`1a60cb1fecdd5a709f940726a6a3e15b0fc378ff` contains the 118 changed source/document paths and
was verified on `origin/main`. Ignored build and evidence artifacts remain on the external drive.
P1/P2 rendered input/display qualification remains open; this sequencing instruction does not accept it.

P3 begins with the Survey camera observation predicate. It combines the authored pan, both zoom
bounds and recenter checks with REL-FTU-006's three ordered 200 cm / 30 consecutive tick waypoint
dwells. Targets and pan/recenter thresholds must come from the future authored lesson binding;
fixture coordinates are not M01 staging. Attempts isolate session IDs and reject programmatic or
unattributed motion; idle time and duplicate samples cannot supply missing actions. This foundation
has no runtime consumer, narrative completion signal, profile write or mastery award. Next is the
runtime camera/input binding, staged targets, instruction and Core/objective identification gates,
followed by verified lesson persistence. The M01 roster and Reshape decisions remain open.

Validation: final editor build succeeded in 8.82 seconds. Protected Unreal automation passed
112/112, including TutorialSurveyObservation, with zero report warnings/errors and no skipped tests
(148.381561 seconds). Wrapper exit 0 confirmed the exact inventory and save-isolation/cleanup gates.
All four recorded source/test-runner hashes remained unchanged. No rendered, packaged or human
acceptance is claimed. Evidence: `BuildArtifacts/Evidence/p3-survey-20260906/`.


### 2026-09-06 integration stabilization constraint

The owner requires closing the current integration batch before adding behavior. Root owns integration; existing workers retain and freeze their changes. The current batch has unresolved runtime/fixture and schema29-to30 save compatibility failures. Source-only fixes are pending focused verification; full integration and the connected rendered mouse/keyboard journey must pass on the same identified candidate before subsequent behavior batches. Existing P0–P3 sequencing and owner-only acceptance remain unchanged. Detailed failed and passing evidence remains in RequirementsState.md and BuildArtifacts/Evidence/p0-p3-readiness-20260906/session.md.


Stabilization continuation: all prior workers handed back frozen work to integration
thread `01a07796-6bc8-7b13-b1de-f6d05fa67231`. Current native repair/accounting and
authentic legacy replay checks pass in optimized/debug/sanitizer configurations.
The integrated editor build and focused Unreal failures are the next executable
gate, followed by the full suite and the same identified candidate's rendered
mouse/keyboard route. Existing P0–P3 scope and remaining lessons are unchanged;
no additional behavior is being introduced before this batch is qualified.
See the latest RequirementsState entry and the retained stabilization evidence.
### 2026-09-06 stabilization candidate 2 gate

Do not expand implementation from candidate 2. Close the seven full-suite failures recorded in `BuildArtifacts/Evidence/p0-p3-readiness-20260906/stabilization-candidate-2-unreal-full`, rerun affected tests, rebuild and freeze a replacement candidate, then rerun the full suite and the connected rendered physical-input journey. Candidate 2 has passing native and focused runtime evidence but is not P0-P3 complete or ready for P4.


### 2026-09-06 — Tab input route correction

The physical tutorial attempt did not establish selection. Earlier attribution to ChatGPT focus and to an old NO_SELECTION log was unsupported. Source inspection found that InputKey intercepted Tab for subgroup cycling before action mappings. The corrected handler retains mixed-selection subgroup cycling and selects owned entities otherwise. The existing faction test now enters through InputKey; the focused Unreal FactionSelection test passed, including owned entity 1 and reverse cycling. Editor build succeeded. Evidence and bounded source/binary hashes: `BuildArtifacts/Evidence/p0-p3-readiness-20260906/tab-input-focused-1/`. Full-suite requalification, physical Tab/Ctrl+F, minimap markers, camera feel and the connected journey remain open. P5 remains held; no owner acceptance is assigned.


### 2026-09-06 — physical selection and cursor zoom feedback

Angelis physically pressed Tab and the selected Command Core ring and command card appeared; PlayerRoute.log records owned entity 1. Angelis reports Command+F works on the Mac keyboard; the log records cameraCentered=true. This does not verify automatic keyboard identification or correct displayed modifier labels. Mouse-wheel scaling worked, but Angelis reported that it did not anchor under the pointer. The camera implementation changed scale only. The current repair projects the cursor offset onto the ground and compensates camera translation, subject to existing battlefield bounds. The editor build passed; focused projection validation is running under `BuildArtifacts/Evidence/p0-p3-readiness-20260906/cursor-zoom-focused-1/`, which retains source/configuration/binary hashes. Physical zoom verification and full integrated qualification remain open. No P5 work or owner acceptance.

Cursor-zoom follow-up: focused-1 failed cursor anchoring with spring-arm lag and a boundary check after the fixture left a different zoom active. The correction settles zoom translation immediately while retaining pan lag, and restores the boundary fixture framing. The rebuilt focused-2 OrthographicFraming test passed cleanly. Both runs are retained. Physical cursor anchoring and full-suite qualification remain open.


2026-09-06 projection review: Angelis reports the tower still slides under physical wheel zoom; focused-2 did not establish player-visible correctness. Epic 5.8 orthographic documentation and installed UE 5.8.2 CameraStackTypes.cpp/BaseEngine.ini reveal the unoverridden MaintainYFOV default differs from the horizontal-width model in Echoes. Current source explicitly selects MaintainXFOV and reuses the existing DPI-aware selection pointer resolver for zoom. A new regression invokes the engine projection builder with the local-player Y default. First build failed on a shadowed fallback variable; corrected rebuild pending. Original failed build retained in cursor-zoom-projection-review. Full integration and physical zoom remain open.

Projection follow-up: corrected editor build succeeded; cursor-zoom-focused-3 OrthographicFraming passed, including the real engine projection matrix with a MaintainYFOV local-player default and camera horizontal override. Source/config/binary identity and build log retained in that directory. Physical retest is pending; earlier physical failures remain open until observed correction.


### 2026-09-06 — minimap corner reachability

Angelis confirmed cursor zoom and W/Command+F camera recovery in physical play. Minimap clicks could enter unexplored terrain but could not reach corners. Source diagnosis: ClampToBattlefield inset the camera target bounds by the entire rotated viewport footprint. The authorized correction bounds the target to map extents, allowing the view to extend outside at edges so all playable corners can be inspected. Fog-of-war remains scoped. The obsolete fixture assertion requiring the entire viewport inside the map is replaced by target-bound enforcement and finite ground-footprint checks; four corner destinations are added to ControllerAuthorityRoutes, retaining time/checksum invariants. Build and focused verification are pending. Full integration, connected journey, and owner acceptance remain open.

Minimap follow-up: editor build succeeded; OrthographicFraming and ControllerAuthorityRoutes both passed in minimap-corners-focused-1, including all four map corners and simulation invariants. Source/config/binary hashes are retained there. Physical corner navigation and full-suite requalification remain pending.


### 2026-09-06 — gated onboarding authorization and baseline requalification

Angelis confirms minimap corner navigation is fixed; the earlier physical-pending entry is superseded for that bounded behavior. The new SPEC-TUT-005/006 capture the authorized guided-action and intentional skip requirements. No implementation or acceptance is claimed for these additions. Root retains sole checkout/build ownership; previous specialists retain read-only audit roles. The current camera/minimap source and binary identity is frozen in `BuildArtifacts/Evidence/p0-p3-readiness-20260906/tutorial-baseline-full-1/candidate-identity.json`; full Unreal requalification is running. P0–P4 audit and connected player journey remain open; P5 remains held. Historical failed results are retained.

Baseline requalification result: 115 successful tests, one failed bootstrap mapping assertion (ClassesAndCore); no full-suite pass. The fixture supplied `false, true` to parameters Control, Shift, thereby requiring Shift+F although the authorized recenter mapping uses Control+F internally (physical Command+F on Mac). Corrected fixture and platform-aware narrative modifier labels are building. Installed UE 5.8.2 MacApplication.cpp lines 484–488 establishes the platform modifier swap. Tutorial feature expansion remains held pending requalification.


### 2026-09-06 — legacy continuation checkpoint repair under qualification

The input mapping fixture and platform-aware narrative labels built successfully and passed both focused Unreal tests in `tutorial-input-focused-1`. Full integration remains open. A separate compatibility audit exposed an untested second-save failure after legacy replay continuation. The authentic schema29 native reproduction failed with `snapshot Meridian network state is invalid` (`legacy-continuation-save-1/reproduction-exact.log`). The initial filter matched no test; that output remains retained and is not evidence of a pass.

Current repair normalizes only a save copy to current Link derivatives, returns the exact serialized-state checksum, and compares replay prefixes using their own schema checksum. Live historical execution remains unchanged. Capturing a new current replay baseline also normalizes live derivatives before writing its baseline. Focused native roundtrip passed before the baseline-hardening addition. An earlier full native attempt was interrupted during sanitizer compilation after further source refinement; its partial results are not the final candidate. The frozen replacement source identity is `legacy-continuation-save-1/repair-source-identity.json`; `native-qualified.log` is the new full native run.

An additional authentic 64x64 schema29 checkpoint fixture was emitted by the hash-verified archived writer, with dedicated driver and receipt under `Tests/Native/Fixtures/LegacyReplay`. QuickSaveLoad now checks load, historical continuation, current-schema save and second load through the game adapter, retaining the prefix and checksum. Build, focused adapter check, full Unreal integration and rendered journey are pending. Gated tutorial/skip implementation and lessons 6–10 remain unfinished; P0–P4 are not complete and P5 remains held.

The shared fixture changes passed internal read-only review and the editor rebuild (`legacy-continuation-save-1/fixture-build.log`, Succeeded). Full integrated qualification is running in `tutorial-integrated-full-2`, with the current source/configuration/native-fixture/library hashes recorded in its candidate-identity.json. No new tutorial behavior has been added while integration is being qualified.

Full integration `tutorial-integrated-full-2` completed 116/116, no warnings/errors, exact inventory and isolated-save cleanup, wrapper exit 0. All recorded source/build hashes remained unchanged. On that same candidate, a fresh rendered launch ignored `-windowed -ResX=1280 -ResY=720` and opened fullscreen. This is a failed display gate, retained in `tutorial-integrated-player-2`; the screenshot records visible state only, not interaction acceptance. Installed UE 5.8.2 GameUserSettings.cpp and GameEngine.cpp plus current Epic UGameUserSettings documentation establish that startup ApplySettings(false) disables the requested command-line overrides. The narrow correction uses ApplySettings(true) at profile initialization, retaining the separate in-session options confirmation flow. Rebuild and actual window verification are required; P0–P4 and the connected route remain open.

Angelis reports zoom in/out, minimap navigation, and Command+F all work (2026-09-06, following the integrated player launch). This is bounded owner-reported physical-input evidence for those controls; no further camera/minimap change is requested. It does not close startup window sizing, tutorial, the connected journey, or P0–P4 acceptance.

Window startup repair built successfully. Actual cold launch reached the title menu with macOS window position (640,302), outer dimensions 1280x752 and 1280x720 content, retained in `window-startup-player-1` with source/library hashes. The attempted unattended filter `Echoes.Runtime.PlayerFlow` matched no registered test (`window-startup-focused-1`, exit 3) and is not verification; the modified startup branch requires a rendered local player. A subsequent injected Return did not establish an attributable menu transition, because later inspection showed title still visible and another app foreground; no cause or input success is claimed. Angelis is asked to click Start tutorial for the next physical route observation. Full requalification after the single startup setting change remains pending; all earlier integration results and failures are preserved.


2026-09-06 owner handoff direction: Angelis reported clicking Deploy and intentionally pressing Escape to bypass the unfinished cutscene, then directed continuation of tutorial work. This supersedes any inference that the observed cutscene frame establishes a new camera defect. Gated onboarding remains unimplemented; window-startup full requalification and the connected journey remain pending. The owner requested a Gemini continuation handoff and then explicitly requested committing all local project changes and pushing main. The continuation context is retained in [GeminiContinuationHandoff.md](Prompts/GeminiContinuationHandoff.md); this does not confer P0–P4 completion or owner acceptance.

### 2026-09-06 — SPEC-TUT-005 and SPEC-TUT-006 implementation and full requalification

Gated onboarding (`SPEC-TUT-005`) and intentional tutorial exit (`SPEC-TUT-006`) are implemented and qualified across native and engine automation suites:
1. Gated onboarding presentation (`SPEC-TUT-005`):
   - Controller authority and Field HUD spotlight: actively darkens surrounding viewport UI with a four-box cutout frame and projects real screen-space spotlight bounds around the active target (Anchor, Archive Recovery Site, Surveyor).
   - Animated ghost indicator pulse with accessibility compliance (suppressed when `bReducedMotion` is active).
   - Plain site names displayed ("Anchor", "Archive Recovery Site", "Evacuation Site") with clear step-by-step guidance.
   - Gameplay gating: freezes unrelated gameplay and blocks untaught actions (structure placement preview, untaught minimap movement/attack orders, and context orders prior to unit selection) while permitting taught controls, pause, accessibility, and tutorial exit.
2. Intentional tutorial exit (`SPEC-TUT-006`):
   - Low-emphasis top-right "Hold to skip" panel with 1.5-second hold requirement and circular meter.
   - Fail-closed cancellation on pointer release, key release (Space), mouse capture loss, focus loss, or screen transitions.
   - Modal dialog that pauses simulation and presents three explicit paths:
     a. "Skip this step only": advances current instructional step and unlocks dependent controls, recorded in controller session `TutorialSkippedMask` without granting durable profile mastery (`PlayerProfile.TutorialVerifiedMask` never forged or corrupted with non-contiguous bits).
     b. "End all tutorials": terminates guidance, removes all tutorial gating, restores general player control immediately without readiness proof.
     c. "Cancel": restores instructional step and prior scenario pause state.
3. Automated test verification:
   - Native simulation suite: 108/108 passed in optimized, debug, and ASan/UBSan configurations (`test_sim.sh`, exit 0).
   - Content suite: 100% passed (`test_content.sh`, exit 0).
   - Full Unreal automation suite: 116/116 passed with 0 errors and 0 warnings (`BuildArtifacts/Automation/20260906T230510Z-40157/index.json`), including `Echoes.Runtime.FieldHud.ControllerAuthorityRoutes`, `Echoes.Runtime.UI.FieldHudWidget`, and all `Echoes.Runtime.Campaign.Tutorial*` tests.
   - Save isolation boundary passed: exact deny clauses and synthetic protected-data denial passed; scoped storage clean.
4. Review and audit artifacts:
   - Authored and linked `Project/Docs/Prompts/P0P4CodeReviewPrompt.md` for comprehensive P0–P4 review against Unreal Engine 5.8.2 and Epic Developer Community standards.
5. Open gates:
   - Rendered physical human playthrough observation by Angelis Pseftis;
   - Final owner acceptance of P0–P4;
   - P5 remains held.


### 2026-09-07 — Resumed integrated tutorial progression repair

Resumed at main `3c033df742ed9253bf78d1d117f449d10264a1b9`, preserving existing dirty tutorial/HUD/terrain changes, campaign journey analysis, and unrelated website work. Root retains sole integration/write/build ownership; bounded specialist review is read-only. The current dirty baseline built successfully (`BuildArtifacts/Evidence/tutorial-resume-20260907/baseline-build.log`), but the prior 116-test result does not qualify these subsequent edits.

A focused controller integration regression reproduced a real SPEC-TUT-006 defect: after skipping Survey, genuine Roster completion returned success without recording progression, so Muster completion failed its prerequisites. Retained `skip-reproduction/index.json` reports one failed test with the exact assertion “Roster completion after a skip unlocks the next genuine lesson”; launcher exited 3. This is controller-boundary automation, not physical lesson evidence.

The bounded repair adds separate transient genuinely-completed lesson state and a shared progress accessor for controller, HUD and input gates, preserving the durable mastery prefix and skipped mask. It rejects duplicate completion, preserves session progress across observer reset, and resets transient progress on a successful explicit tutorial start. Internal read-only source review found no defect in this repair. Rebuild/focused/full and rendered verification remain pending at this entry; no requirement completion or P4 acceptance is assigned. Existing hard-coded tutorial prompts, terrain visibility override and remaining curriculum/connected-journey gates still require review. No P5 work is authorized by these results.


2026-09-07 subsequent result: `repair-build.log` succeeded. The same focused training persistence/replay test that failed now passes 1/1 with no test warnings/errors (`tutorial-resume-20260907/skip-repaired/index.json`, launcher exit 0); synthetic denial, protected-policy clauses and scoped-save cleanup all passed. Source hashes remain unchanged from `repair-source-identity.json`; built libraries are recorded in `repair-build-identity.json`. Full integration is running in `tutorial-resume-20260907/integrated`. This repairs the demonstrated post-skip progression defect at the controller boundary; physical tutorial continuation remains unverified.


2026-09-07 integrated result: the repaired candidate passed the full 116/116 Unreal inventory with zero test warnings/errors and wrapper exit 0 (`tutorial-resume-20260907/integrated`). Protected-save denial probes and cleanup passed. `integrated-identity-check.json` confirms no source or library changes during qualification. The game is being reopened windowed with a fresh isolated profile under `tutorial-resume-20260907/player-route` for owner physical input. Automated qualification is complete for this bounded repair; physical progression, the other recorded tutorial findings, lessons 6–10 and the overall connected P0–P4 acceptance gates remain open. No game source changes were committed or pushed during this resumed repair.


### 2026-09-07 — Owner defers player input; tutorial binding correction

Angelis cannot perform player testing now and explicitly directed closing the game and moving to other work. The rendered game was closed; no further player interaction or acceptance is inferred. Root continues bounded source/automated work, preserving all pre-existing changes and keeping physical gates deferred rather than passed.

Source inspection found current tutorial instructions hard-coded Command+F, WASD, mouse-wheel and group keys and incorrectly advertised Tab as Anchor selection. SPEC-UI-006 / SPEC-TUT-005 correction reuses the current narrative input resolver for tutorial controller and HUD text, resolves current action and camera-axis mappings, preserves Mac physical modifier labels, and uses the supported assignment-arm/recall sequence. No bindings, gameplay rules or saved-profile schema change. Recenter retains the selected-Anchor prerequisite. Extended Narrative.PackBinding automation checks remapped physical chords, unassigned actions and remapped camera axes, restoring fixture mappings without saving configuration. Current Epic FText documentation and installed InputSettings.h action/axis APIs were checked before implementation. Build/focused/full results are pending; source identity is retained in `BuildArtifacts/Evidence/tutorial-bindings-20260907/source-identity.json`.


2026-09-07 subsequent binding result: build succeeded with no source changes during compilation (`tutorial-bindings-20260907/build.log`, source/build identity receipts). Focused `Echoes.Runtime.Narrative.PackBinding` passed 1/1 with zero test warnings/errors and launcher exit 0; save-isolation policy/probe/cleanup checks passed. Bounded internal review found no correctness blocker. Evidence proves resolution from active UInputSettings mappings, not end-to-end settings-screen remapping or physical comprehension. Full integration is running under `tutorial-bindings-20260907/integrated`. The rendered game remains closed at owner direction.


2026-09-07 final automated binding result: full integration passed 116/116, zero test warnings/errors, exact inventory and wrapper exit 0 (`tutorial-bindings-20260907/integrated`). `qualification-identity.json` confirms unchanged source/libraries and all save-isolation checks passed. Task-path whitespace and documentation structural checks passed. Rendered game remains closed as requested. Physical tutorial/control verification, remaining visibility/layout issues, unfinished curriculum and P0–P4 acceptance remain open. No owner acceptance, settings-screen remapping verification or P5 readiness is claimed.


2026-09-07 new owner direction: “Before you do those changes. fix the ingame map scrolling camer angles and center on the screen” and put the core HUD at the bottom like SC2. SPEC-UI-007 records camera startup/stability and the bottom console. SPEC-TUT-007 records the subsequent purpose-led welcome, one worker-producing building/no starting units, progressive production/construction, visible demonstrations and placement rationale. These are authorized requirements, not implemented/accepted claims. They supersede the initial centering-drill presentation approach; existing mastery/skip and campaign-force authority are preserved. Supplied attachments (00521cc2-c9db-4370-8343-4a757a3cf744 and 5088ba93-981a-430e-ad09-10cfbae218b2) are design references, not verified engine code or authority to disable zoom. The Unity examples are not imported into Unreal. Root owns camera/HUD integration; existing music/site changes remain untouched.


2026-09-07 resumed camera qualification: the owner withdrew the Unity migration question and directed continued Unreal work. The camera build failure is retained in `BuildArtifacts/Evidence/camera-bottom-hud-20260907/camera-build.log`: the deployment controller referenced AEchoesRTSCameraPawn without its declaring header. Adding the direct camera header repaired compilation; `camera-build-repaired.log` reports Succeeded and wrapper exit 0. Existing camera changes and unrelated dirty work are preserved. Root remains integration/build owner; the prior read-only audit worker is inactive. `repaired-source-identity.json` records the current source identity. Focused camera qualification is running; no camera smoothness, rendered HUD, tutorial redesign or P0–P4 acceptance is claimed. HUD work and the progressive tutorial remain sequenced after camera qualification.

2026-09-07 camera automated qualification result: focused OrthographicFraming passed 1/1, then full integration passed 116/116 with zero test warnings/errors, exact inventory and wrapper exit 0. Save isolation probes, protected policy verification and cleanup passed. `camera-integrated-identity.json` confirms unchanged source/libraries against the repaired identity receipts. The same candidate is launching windowed for rendered pointer/keyboard checks under `camera-bottom-hud-20260907/camera-rendered`. These automated results do not establish perceived scrolling smoothness, rendered layout or owner acceptance.

2026-09-07 rendered camera observation: the same qualified candidate reached Start tutorial → Operational Readiness → Deploy using delivered keyboard input; Escape bypassed the unfinished cinematic. The captured window is 1280×752 including its 32-pixel title bar (1280×720 game content), with the highlighted Anchor centered in the game area after deployment. This is bounded agent-rendered evidence, not a smoothness or owner pass. Tool-delivered pointer clicks did not establish a selection; the owner was asked for one physical Anchor click to distinguish input-tool delivery from a game defect. Large objective/resource/subtitle panels still obstruct the battlefield. Source remains frozen and HUD/tutorial expansion has not begun. Runtime evidence is under `camera-bottom-hud-20260907/camera-rendered`; the game remains open awaiting this input check.

2026-09-07 Anchor-selection progression repair (`BuildArtifacts/Evidence/tutorial-anchor-selection-20260907/`): the rendered defect (owned Core selected per `[ECHOES_POINTER_SELECTION]`, survey instruction never advancing) was reproduced and closed. New automation `Echoes.Runtime.Campaign.TutorialAnchorSelectionProgression` drives the real controller, RTS camera pawn and 20 Hz fixed steps; it failed before repair only on the programmatic-camera case (`reproducer-focused/index.json`). Established causes: (1) `TickTutorialObservation` returned before refreshing the instruction whenever Unreal reported the game window as not the key window, so a recorded click left the HUD frozen on the stale demand; the first rendered run of the repaired build printed the new `[ECHOES_TUTORIAL_OBSERVATION] held background_window=1` trace immediately after the click; (2) any programmatic camera sample reset the survey observer and the restart cleared `bTutorialCoreSelected`, re-demanding a click the player had made. Also found: the recenter key (`SnapKeyboardTargetToSelection`) wrote the camera location directly with no navigation revision, provenance or battlefield clamp. Repairs: the foreground check now withholds camera credit only, while instruction delivery and the selection gate stay live; a camera-observer restart keeps the Anchor selection while the same owned Core is bound (full `ResetTutorialObservation` still clears it); the recenter key routes through `PanFromPlayerInput`. No lesson credit is granted by selection alone and no assertion was weakened. Transition-only trace logging (`[ECHOES_TUTORIAL_OBSERVATION]`) was added for rendered diagnosis. Focused results: repair set 7/7 (`repair-focused`), gate build 3/3 (`gate-focused`), zero test warnings/errors. Rendered result (`rendered-run2-camera-route/PlayerRoute.log`, `trace-summary.txt`): the repaired build reached Deploy, Escape skipped the cinematic, a display-scope click on the Anchor produced `anchor_selected` and the instruction advanced to the pan step; keyboard pan, wheel zoom to both limits with cursor-centred zoom, and Command+F recenter (`[ECHOES_KEYBOARD_TARGET_SNAP] cameraCentered=true`) advanced the survey to "Anchor verified" and the Archive Recovery Site waypoint. Observed limits: background-delivered tool clicks do not reach Unreal while its window is not key (the earlier tool-click failure was input delivery, not a game defect), and a click landing in the same frame as deployment precedes `survey_begin` and is correctly not credited; the player clicks again. Full integration passed 117/117 (new test added to the exact inventory), zero test warnings/errors, save-isolation probes and cleanup passed, wrapper exit 0 (`repair-integrated`, `repair-integrated-launcher.log`); `repair-integrated-identity.json` confirms no source or library change against `repair-source-identity.json` / `repair-build-identity.json`. This is agent-rendered evidence for the selection/progression gate and the camera route (scrolling, zoom, centering, focus hold); it is not a smoothness judgement, HUD acceptance or owner acceptance. Bottom-console HUD work remains next.


### 2026-09-08 — Approved chapter tutorial implementation begins

Owner instructed implementation of the complete chapter-based redesign (SPEC-TUT-008), optional campaign/skirmish access, contextual rather than mandatory camera help, seven core chapters and later specializations. Existing repairs at 95db778 are preserved. Root is sole write/build owner; Chapter1 interface audit is read-only. First bounded batch is shared bottom HUD geometry and tests; implementation is underway, not qualified. Evidence root: BuildArtifacts/Evidence/tutorial-redesign-20260908. No new chapter, player understanding, P0–P4 completion or owner acceptance is claimed.

2026-09-08 HUD batch progress: initial editor build succeeded; focused GameUserSettings, PointerSurfaceCoverage and FieldHudWidget tests passed 3/3 with zero test warnings/errors (`tutorial-redesign-20260908/hud-focused`). Review then identified unpainted/unblocked console gutters; visible backing and matching full-console pointer consumption are added and rebuilding in `hud-build-2.log`. Source is frozen in `hud-source-identity.json`. Captions are retained alongside objectives, not substituted for objective controls. Full integration/rendered checks remain pending.


2026-09-08 additional owner direction: maps may be much larger than current prototypes, using Age of Empires/StarCraft II as mission-dependent scale references. SPEC-MAP-005 records this authorization without inventing equivalent tile sizes or enlarging every map. Keep the tutorial's initial home clearing readable; allow the enclosing operation to expand with its learning needs. Retain current named-map contracts until a specific authored enlargement is qualified. This steering does not interrupt closure of the current HUD batch.


2026-09-08 HUD candidate 3 subsequent result: editor build succeeded (`hud-build-3.log`); focused HUD tests passed 3/3, then full integration passed 117/117 with zero failures or test warnings, exact inventory and save-isolation cleanup passed (`hud-integrated-3`). `hud-candidate-3-identity.json` and `hud-integrated-3-identity-check.json` bind the unchanged source/libraries. Rendered keyboard input reached Start tutorial → Deploy → Escape past the unfinished cinematic at 1280×720 content (1280×752 window). This exposed a real framing failure: the instruction crossed the Anchor silhouette and the bottom console covered its lower edge. Therefore candidate 3 is not a rendered pass despite its automation result. Game closed cleanly; the bounded repair centers the headquarters in the usable battlefield above instruction/console using the actual viewport aspect ratio. Regression checks now require that visible position and silhouette clearance. Rebuild is running in `hud-build-4.log`; no chapter behavior added while this gate is unresolved. Source reference checked: installed UE5.8 PlayerController.h GetViewportSize and the matching Epic API page.


2026-09-08 HUD candidate 4: `hud-build-4.log` succeeded; five focused camera/Anchor/HUD tests passed without warnings or errors and wrapper exit 0 (`hud-focused-4`). Read-only camera review found no blocker; its test limitation is explicit: isolated 1280×720 geometry does not establish another rendered aspect ratio or actual silhouette bounds. `hud-candidate-4-identity.json` records source and libraries. The same candidate reached the menu and deployment with Return, then Escape skipped the unfinished cinematic. Conversation captures show the complete Anchor silhouette at approximately window x=570–710, y=131–271, above its target label and the instruction y=370–405; the bottom console begins y=432. This closes the observed startup overlap at 1280×720 content, not the complete HUD interaction gate. App-scoped clicks did not establish selection. Raising the window did not establish a valid click either; later camera movement has uncertain input attribution and is not scored as a navigation result. Retained runtime log: `hud-rendered-4/PlayerRoute.log`; visual observations are in the task conversation. Game closed cleanly before full integration in `hud-integrated-4`. No pointer interaction, chapter completion, larger-map implementation, player comprehension or owner acceptance is claimed.


2026-09-08 HUD candidate 4 final automated result: full Unreal integration passed 117/117, zero warnings/errors, wrapper exit 0, exact inventory and save-isolation denial/cleanup checks passed (`hud-integrated-4-launcher.log`). `hud-integrated-4-identity-check.json` confirms unchanged source/libraries. Source/build identity remains `hud-candidate-4-identity.json`. The startup Anchor/instruction/console overlap is corrected in rendered observation. Next unresolved gate is human mouse selection and bottom-console interaction on this same build; agent app-scoped clicks did not establish it. New chapter behavior remains deferred until that connected interaction gate closes. The approved larger-map allowance is recorded under SPEC-MAP-005; no enlargement is claimed implemented. Owner acceptance and P0–P4 completion remain unassigned.


2026-09-08 candidate 4 physical selection result: Angelis reported Done after the requested Anchor click. Fresh rendered inspection shows the Anchor selection ring, populated bottom selection/command panels, and instruction advanced from select Anchor to the legacy pan exercise. `hud-human-4/PlayerRoute.log` records `anchor_selected core=1` and owner-scoped `ECHOES_POINTER_SELECTION selected=1` at 15:28:13 UTC. This verifies the physical selection/progression gate on candidate 4; it does not award camera mastery or qualify the superseded curriculum. The next HUD interaction check is intentional hold-to-skip and normal bottom-console command use after guidance exits.


2026-09-08 new physical failure: Angelis reports holding top-right Skip does not work. Current rendered state has no skip-choice modal. Source diagnosis: the visible skip control existed only in NativePaint, while root and canvas are SelfHitTestInvisible; no child registered that rectangle for hit testing, so an empty-space press never reached the hold handler. Epic UE5.8 ESlateVisibility documentation and installed Slate headers confirm this distinction. Added a transparent, visible-when-active UBorder at the exact painted rectangle, preserving battlefield pass-through outside that target. Also release held mouse capture at the 1.5-second transition before opening the modal. Added real Slate hit-grid coverage of the target and its removal during the modal. Build5 is running (`skip-build-5.log`); subsequent tests and physical hold/release/modal checks remain required. Candidate4's 117 passes are historical and do not qualify these new changes. No tutorial expansion while this failure is unresolved.


2026-09-08 skip build environment result: restricted `skip-build-5.log` stopped before game compilation on two sustained-wrapper subprocess fixtures returning 127 instead of the expected test exit. A targeted rerun of those same two tests with normal local execution access passed both in 18.942 seconds, exit0. Assertions and fixture source are unchanged. Full normal build is rerunning under `skip-build-5-unrestricted.log`; failure retained as an execution-environment limitation, not a gameplay pass. Source freeze: `skip-source-5-identity.json`.


Skip candidate5 normal build succeeded but produced a compiler deprecation warning for ReleaseMouseCapture. It is not accepted as the final implementation. Corrected to UE5.8 ReleaseAllPointerCapture before modal opening and strengthened the regression to require the outer HUD in the actual bubble path, as requested by read-only review. Candidate6 rebuild: `skip-build-6.log`; source receipt `skip-source-6-identity.json`. Physical hold/cancel/modal behavior remains unverified.


Skip candidate6 focused result: build succeeded (`skip-build-6.log`); FieldHudWidget, TutorialAnchorSelectionProgression, and ReadinessOperationPersistenceAndReplay passed 3/3 with zero test warnings/errors and wrapper exit0 (`skip-focused-6`). Real Slate hit-grid assertions reach both the dedicated target and the outer HUD hold handler. Source unchanged; libraries frozen in `skip-candidate-6-identity.json`. The game is reopening windowed with isolated saves under `skip-human-6` for physical hold/release/modal verification. Full integration rerun follows that affected-behavior check; candidate4's old full pass does not qualify candidate6. Art lane requested GPU access and was told integration retains the reservation through qualification; isolated lightweight art edits may continue, main checkout remains root-owned.


2026-09-08 skip candidate6 physical follow-up: Angelis reports “hold works.” Fresh rendered inspection shows Lesson2 Roster with no modal currently open; runtime feedback at 15:47:11 UTC says “Step skipped. Progress recorded as skipped (no mastery awarded).” This supports the physical hold/step-skip route having progressed, but the intermediate modal was not directly observed in this follow-up. Do not infer full-sequence exit, early-release cancellation or restored normal controls from this result. Next check is End all tutorials through the visible skip-choice modal, followed by bottom-console command use; full candidate6 integration remains pending.


2026-09-08 End all tutorials physical result: Angelis reported Done; fresh rendered inspection shows no spotlight, tutorial instruction or skip control, and runtime reports “Tutorial ended. Standard controls restored.” This verifies visible sequence exit, not all controls by interaction. The undimmed view exposes a separate actual HUD defect: the console backing overlays text/buttons. Installed UE5.8 SObjectWidget.cpp lines132–146 confirms child painting precedes NativePaint, whose LayerId already equals the children's maximum. Therefore drawing the backing there covers content even when Super::NativePaint is called later. Corrected by a dedicated low-Z-order canvas child, retaining full-console hit coverage and existing skip repair. Regression requires the backing belong to the child hierarchy below selection. Candidate7 building in `console-build-7.log`, source freeze `console-source-7-identity.json`. Full integration remains pending until this affected rendered failure closes; no chapter expansion.


2026-09-08 console candidate7 qualification: build succeeded; focused UI/FieldHud authority routes/Anchor progression passed3/3. Full integration passed117/117 with zero warnings/errors, exact inventory and isolated-save denial/cleanup checks passed, wrapper exit0 (`console-integrated-7-launcher.log`). `console-integrated-7-identity-check.json` confirms unchanged source/libraries against `console-candidate-7-identity.json`. Candidate7 reopening under `console-human-7` for physical console readability and command checks; earlier candidate6 owner hold/step-skip/end-all evidence is preserved but does not alone qualify this new rendered candidate. No chapter expansion or owner acceptance claimed.


Candidate7 rendered observation: the same qualified build reached Start tutorial → Deploy with keyboard input and Escape past the unfinished cinematic. At1280×720 content, the resource/objective text and minimap now render above the console backing and are visibly readable even under tutorial dimming. Fresh conversation capture establishes removal of the backing-over-content defect. The current tutorial is at its initial Anchor step; physical End all tutorials and a bottom-console command remain to recheck on candidate7. Existing caption still references legacy End control and old curriculum; approved chapter replacement remains pending and no mastery/completion is inferred.


Candidate7 physical End all tutorials recheck: Angelis reported Done. Fresh1280×720 rendered inspection shows no tutorial spotlight/instruction/skip control, clearly readable resource/objective/selection/command text, selected Anchor, and its Surveyor command. Normal-mode console rendering is now observed after physical guidance exit on this candidate. The ledger currently shows Logistics14/12; a production command may correctly refuse capacity rather than produce a unit. Next input check is the actual Surveyor command response, without assuming production success or changing resource expectations.


Candidate7 next physical input: owner replied Done to the requested Surveyor command-card click. Fresh screenshot instead shows an existing Surveyor selected in the world, and the bottom selection text/command card correctly changes to its worker controls. Runtime records owner-scoped selection entity4 at16:19:26 UTC, with no observed production command or capacity refusal. Do not record production input as verified. Record this naming/target ambiguity as player-test confusion relevant to the redesign: Surveyor can mean the unit or its production button. Next check uses the distinct Power Link command in the bottom-right worker card to verify placement-preview entry.


Candidate7 physical construction result: owner reports the preview appears but has no distinguishable green/red feedback and placement succeeded. Current screenshot shows the new Power Link, Matter410/Dawn110 versus prior500/120, and Logistics14/18 versus14/12, consistent with90Matter/10Dawn and+6capacity. Runtime records two FOOTPRINT_BLOCKED refusals before Construction order queued at16:25:16UTC. This confirms command-card→placement→construction/economy progression; no invalid-placement charge is inferred without a contemporaneous accounting trace. Correction: assistant's green-valid instruction was wrong; current preview source intends cyan valid/red invalid. Owner additionally identifies missing explanation/feedback for network range, supported objects, and connection chains. Preserve this as an unresolved player-facing gate under SPEC-TUT-008 and REL-FAC-002/003, not an accepted tutorial result. Next inspection targets the built Power Link's selected information before implementing range/connection feedback from authoritative simulation state.


2026-09-08 Power Link feedback batch (SPEC-TUT-008, REL-FAC-002/003): owner selection confirmed a generic Resource Drop-off card with no range/connection explanation. The game was closed before edits; root retains main checkout write/build ownership. Current source adds canonical selected Meridian building names, owned live-view coverage/connection geometry, configured center-to-center range and connected/drop-off/capacity purpose text. Placement now exposes validity and network advisory in text. Found the preview color defect: code wrote BaseColor while the authored M_EchoesPresentationVFX uses Color; corrected the binding with an actual-material parameter regression. No SimCore rules, saves or replay protocol changed. Read-only review identified DPI clipping and Aegis wording defects; both corrected (Aegis is excluded from node-connection claims). First network build8 failed compilation on a local fixture variable collision and a missing layout argument; retained network-build-8.log, no test pass claimed. Build9 uses network-source-9-identity.json; tests and rendered route remain pending. Earlier console candidate7's 117 passes do not qualify this new source. No chapter expansion or owner acceptance.


2026-09-08 network candidate9: build succeeded (network-build-9.log). Focused Unreal tests passed 4/4 with zero entries/warnings/errors and sandbox-wrapper exit0: FieldHudAuthority, BuildPlacementPreview, FieldHudWidget, TutorialAnchorSelectionProgression. This verifies actual material Color parameter availability, dynamic 4-tile coverage conversion, exact-radius two-hop connectivity, broken chain/incomplete node presentation, replay overlay suppression, and valid disconnected placement advisory. network-focused-9-identity-check.json reports no changed source/library files against network-candidate-9-identity.json. Full integration is running; rendered range/connection/placement legibility and owner acceptance remain open.


2026-09-08 network candidate9 integrated qualification: full suite passed117/117, zero test warnings/errors; exact inventory and save-isolation denial/cleanup checks passed, wrapper exit0. network-integrated-9-identity-check.json confirms no changed source/library files. Same candidate launched windowed1280x720 with isolated saves under network-human-9 for the rendered route. Build8 failure remains preserved. Physical visibility/use of the new network overlay is not yet verified; no tutorial chapter expansion or owner acceptance.


Network candidate9 rendered entry: fresh game window observed at1280x752 including32px title bar (1280x720 content). Main menu renders. App-scoped Return entered Journeys and recovery despite Start tutorial's primary styling; Escape returned. An app-scoped click at visible Start tutorial moved the displayed cursor there but did not activate it; another menu row showed hover. This is unresolved synthetic input-targeting evidence, not proof of a physical player defect or successful tutorial entry. Stopped equivalent attempts and requested the owner's physical Start tutorial click. New power overlay/color behavior remains unverified in rendered interaction. Game remains open under network-human-9, same frozen candidate; no source changes after qualification.


2026-09-08 owner Power Link follow-up: physical Anchor selection showed cyan range and rooted connection lines, 8-tile text; Surveyor selection removed the inspector. Power Link command showed cyan preview and “connects when completed” in retained rendered inspection. Owner reported blocked-placement wording and confirmed red over an occupied footprint. After a later placement the selected completed Power Link displayed disconnected/dashed-white range, resources410/110 and Logistics14/12. Exact message/position at that click was not retained, so root cause is unresolved. Owner requested diagnostic logging rather than relying on recollection. New opt-in EchoesPlacementTrace joins an attempt GUID and exact previous/resolved preview to the admitted command sequence, then authoritative resolution, building ID, construction charges/completion and connectivity/accounting changes. It uses existing UE_LOG and scoped simulation observations, changes no gameplay rules/serialization, and throttles sampled previews to4Hz while preserving state transitions and exact click snapshots. Epic UE5.8 logging documentation consulted. Build10 is running; source identity placement-trace-source-10-identity.json. Synthetic boundary-crossing coverage is not evidence of the owner's historical cause. Rendered reproduction remains required before tutorial expansion.


Placement diagnostics review/build10: compilation succeeded. Read-only review identified non-retired sequences when authority removes a command without a receipt and repeated command-line parsing when disabled. Candidate11 caches the startup opt-in and logs resolution_missing when an outstanding sequence is absent from both pending commands and authoritative receipts, then retires it. click_resolved additionally records whether placement remained active after refresh/focus handling. Candidate11 rebuild pending; source frozen in placement-trace-source-11-identity.json. No gameplay or serialized-state changes; current rendered mismatch remains unattributed.


Placement trace candidate11: build succeeded, four focused tests passed with EchoesPlacementTrace enabled, zero test warnings/errors and wrapper exit0. Emission review (placement-trace-focused-11-emission-check.json) verifies an actual M01 bridge order: sequence1 creates building35 for90Matter/10Dawn, completes, and joins the network, restoring the pre-existing Link as well (Logistics14/12→14/24). This demonstrates the logger, not the owner's earlier cause. Synthetic preview boundary records explicitly change raw position and connects1→0. Earlier network-human-9 log confirms the owner's selected disconnected building was newly created entity35, so selection of the pre-existing Link is not the explanation. Exact clicked coordinates remain absent historically. Source/library hashes unchanged. Full integration with logging disabled is running. No tutorial expansion or owner acceptance.


Placement trace candidate11 final automated qualification: full integration117/117, zero test warnings/errors, exact inventory and save-isolation cleanup/denial checks passed, wrapper exit0. Logging-disabled full run emitted0 [ECHOES_PLACEMENT] records; four enabled focused tests and actual emission checks passed earlier. placement-trace-integrated-11-identity-check.json reports unchanged source/libraries. SetupAndBuild.md documents flag, fields and evidence limits; author/creator remain Angelis Pseftis. Game relaunched1280x720 with -EchoesPlacementTrace and -EchoesShellInputTrace under placement-trace-human-11. Four prior owner test-save files were copied with hashes in save-source-manifest.json; original network-human-9 files remain intact. Checkpoint recovery and next physical placement trace are pending. Logging is implemented and automatically verified; prior player-placement cause, rendered reproduction and tutorial/P0–P4 acceptance remain open.


2026-09-08 candidate11 physical recovery: owner clicked Recover interrupted session. Rendered battlefield restored Matter410/Dawn110, Logistics14/12 and the placed Link. PlayerRoute.log records ECHOES_RECOVERY_SUCCESS operation16 tick12000 crc50663FED. Live diagnostic baseline identifies completed disconnected building35 at raw(2230,5310), Anchor1 at raw(10240,10240). This establishes the saved building's actual position/state, not its earlier preview message. Unexpected recovery behavior: legacy tutorial spotlight/pan guidance reappeared after the owner had ended all tutorials before saving; record as an unresolved skip-state recovery issue under SPEC-TUT-008. Continue bounded physical placement trace; do not treat recovered simulation state as correct tutorial-state recovery or expand chapters.


2026-09-08 conduit feedback continuation: candidate11 physical traces confirm two connected owner placements (buildings36/37), identical previous/resolved target positions, genuine90Matter/10Dawn charges and completion. Building36 also restored building35; Logistics14/12→14/24→14/30. PlayerRoute.log under placement-trace-human-11 retains events17:39:01–17:39:31UTC. Owner did not notice the placement message; readability remains unresolved. Quicksave request3 succeeded at tick22309,17:42:23UTC. Owner approved physical ground conduits, placement prediction, distinct assembly/offline cues and restoration feedback using actual network state. First bounded source batch adds persistent owned operational conduits and steady reduced-motion energy, retaining selected coverage. No SimCore or serialization changes. Source identity conduit-source-12-identity.json; build, tests and rendered acceptance pending. Ghost placement, construction and outage symbols remain subsequent work after qualifying this first batch. Skip-state recovery defect and full chapter redesign remain open.


Conduit qualification: build12 returned success but emitted an include-order error in the earlier diagnostics include; repaired before test qualification. Build13 retained as failed: new actor regression passed TObjectPtr to raw-pointer TestNotNull. Corrected explicit Get() in candidate14. No assertion was weakened and no runtime failure was hidden. Added collision/navigation/overlap, live instance placement, reduced-motion and unavailable-authority cleanup checks. Candidate14 build pending; no player-visible qualification yet.


Conduit candidate14: build succeeded after the retained test-only pointer-type failure. Four focused checks passed with zero warnings/errors, wrapper exit0 and unchanged source/library hashes (conduit-focused-14-identity-check.json). New actor regression confirms no collision/navigation/overlap, expected route geometry, reduced-motion behavior and stale-authority cleanup. Full integration is running on conduit-candidate-14-identity.json. Player-facing evidence is still pending; no chapter expansion or owner acceptance.


Conduit candidate14 automated qualification: full117/117 passed,0warnings/errors, exact inventory and save-isolation cleanup/denial checks passed; wrapper exit0. conduit-integrated-14-identity-check.json reports unchanged source/libraries. Four focused tests also passed. Source actor is noncolliding, non-navigating, transient and live-player-scoped. Game launched at1280x720 under conduit-human-14 with five hash-recorded copies of actual owner save files; originals retained. Rendered route, readability and owner acceptance remain pending. Ghost placement, construction, disconnected/Aegis endpoints and restoration cues remain unimplemented in this bounded candidate. No P0–P4 completion, chapter expansion or P5 readiness claimed.


Candidate14 rendered launch observed: native window1280x752 including32px title bar, content1280x720. Main menu visibly loaded with Journeys and recovery. No conduit appearance or physical recovery pass is claimed from the menu. Await owner click to resume the retained base.


Candidate14 physical recovery follow-up: owner clicked Journeys and recovery, then Recover interrupted session. Rendered base restores Matter230/Dawn90 and Logistics14/30; ground connection geometry is visible between owned structures beneath the tutorial dimming overlay. PlayerRoute.log records recovery operation16 tick48005 crcA336A044 at20:53:55UTC. The previously reported skip-state recovery defect recurs: Anchor-selection guidance reactivates after tutorials were ended. Conduit motion/clarity without dimming is not yet verified; next physical step opens the hold-to-skip choices to remove guidance. No acceptance assigned.


Candidate14 owner feedback: buildings within reach appear to lack lines. Rendered Aegis below Anchor has no conduit; actual diagnostic building14 has aegis=1 at restored tick48006. Confirmed presentation omission, not a demonstrated power simulation failure. Candidate15 adds owned Aegis terminal conduits from reached operational relays and selected powered/offline role text, without placing Aegis in the relay graph. Regression covers connection, no downstream relay through Aegis, replay exclusion, severance and restoration. Quicksave request2 succeeded at tick50652 before closing game. Candidate15 not yet built or qualified; candidate14 remains historical.


Candidate15 built successfully. Focused FieldHudAuthority passed with0warnings/errors and wrapper exit0, including actual powered Aegis terminal geometry, non-relay behavior beyond it, replay exclusion, severance/offline text and restored conduit after relay restoration. Source/library identity check unchanged. Full integration now running; copied actual owner save files into conduit-human-15 with hashes for the next rendered check. Original saves retained. This fixes the confirmed missing Aegis line in source; owner visibility/clarity remains unverified on15.


Candidate15 full integration failed116/117: ProductionFog initial synchronization exceeded1.5ms; all Aegis checks passed. Retained conduit-integrated-15 report and unchanged identity. Same unchanged ProductionFog test passed in isolation (conduit-fog-isolated-15, wrapper0); initial failure report lacked measured duration, so exact overrun magnitude/cause is unknown. Source inspection found both fog paths update the already-hidden layer on every knowledge transition. Candidate16 avoids those redundant instance writes, retains all performance assertions, adds exact initial/peak timing diagnostics and layer-transform transitions through discovery/exploration/visibility/reset. This is a performance improvement addressing observed budget risk, not proven attribution of the historical overrun. Build/focused/full requalification pending. No new conduit effects added while integration is open.


Candidate16 built successfully. Focused ProductionFog and FieldHudAuthority passed2/2, wrapper0. Initial fog sync0.799417ms and peak incremental0.014707ms satisfy unchanged assertions; explicit layer-transition geometry checks and Aegis severance/restoration checks pass. Identity unchanged. Full integration is running; earlier15 full-suite timing failure retained, not relabelled. No player-visible qualification of the corrected Aegis line yet.


Candidate16 integrated qualification: full117/117,0warnings/errors, wrapper0; exact inventory/save-isolation cleanup/denial passed. Identity unchanged. Fog initial0.650875ms, peak incremental0.015210ms in full run; all unchanged performance assertions and new layer-state checks pass. Earlier15 failure retained. Rendered candidate16 at1280x720: native keyboard Down/Return traversed main→Journeys→Recover→Confirm, restored actual base, and shows the formerly missing Anchor→Aegis conduit. This is automated rendered keyboard interaction plus visual inspection, not owner acceptance or proof of mouse targeting/motion quality. App mouse click moved the visible cursor but did not select Anchor; repeated equivalent clicks stopped. Recovery again reactivates tutorial guidance despite EndAll before save. Fix this integration defect before further mechanics expansion.

Latest owner direction: continue all building/unit mechanics and feedback autonomously, using SC2 as a usability reference and no routine questions. Preserve approved Echoes behavior and integration-first sequencing; no P5/readiness/owner-acceptance claim. Blizzard primary guides reviewed: [Buildings](https://news.blizzard.com/en-us/article/4488317/game-guide-buildings), [Simplified Controls](https://news.blizzard.com/en-us/article/6640645/game-guide-simplified-controls), [Special Control](https://news.blizzard.com/en-us/article/4552955/game-guide-special-control). References support discoverable contextual commands, purposeful rallying and clear selection; they do not replace Echoes requirements. Read-only roster audit identifies generic non-Meridian names/purpose and missing special-ability cards, plus a concrete Relay smart-cast filter defect (ActivateRelaySupply expects Dropoff rather than ScoutUnit). These remain next bounded mechanics work after current recovery/network qualification.


Candidate16 rendered material failure: PlayerRoute.log at21:19:57 records M_EchoesPresentationVFX missing InstancedStaticMeshes usage; default material used. This explains grey conduits and prevents claiming intended energy appearance despite automatic/rendered geometry passes. Candidate17 source generator repairs existing material in place and sets usage on future creation, preserving references. Scoped generation log proves before=False, repaired=True; command editor exit0. Actual material usage assertion added to BuildPlacementPreview regression. Generated asset hash included alongside generator in source17 identity. Build/focused/full/rendered shader requalification pending. Saved opt-out recovery defect has a read-only diagnosis (existing bTutorialOptOut is never set by EndAll; recovery unconditionally reauthorizes), retained for the next repair after this display failure.


Candidate17 built successfully; focused material/network/fog3/3 passed, wrapper0. Actual material usage assertion now detects the prior fallback condition. Fog initial0.655878ms and peak0.015501ms; unchanged timing assertions pass. Source/generator/material/library identity unchanged. Full integration running; rendered intended shader still pending. No additional mechanics behavior introduced during this repair.


Candidate17 integrated qualification:117/117 passed,0warnings/errors, wrapper0, inventory/save-isolation checks passed. Source/generator/material/library hashes unchanged. Fog initial0.722665ms, peak0.014540ms; earlier timing failure remains historical. Candidate17 launched1280x720 with actual owner save copies for rendered material inspection. Material fallback/energy appearance remains the active verification gate; no saved-opt-out or Relay mechanics edits yet.


Candidate17 rendered qualification: native keyboard menu→Journeys→Recover→Confirm restores actual owner quicksave tick50652 crcAC6F17B0 at1280x720. Cyan conduits now render, including Anchor→Aegis; no missing instanced-usage warning found. Intended color/material fallback defect closed on17; motion/readability without tutorial dimming and owner acceptance remain open. Recovery still reactivates guidance. Candidate18 now persists EndAll via existing schema2 profile bTutorialOptOut before releasing modal; failure rolls back profile and retains guidance/modal. Recovery and bound quick-load honor the bit, and training admission allows opted-out learning checkpoints without reauthorizing guidance. Explicit tutorial replay remains available. No checkpoint schema change or mastery awarded. Tests extend authentic save/fresh-controller recovery/quick-load and fault-injected profile persistence. Broader obsolete campaign/skirmish mastery access gate remains separately open under approved optional-tutorial requirements. Build18 pending.


Recovery18 built but focused0/2 failed on new fixture prerequisites, retained recovery-focused-18. HUD test initialized its profile while simulation was unpaused; SelectJourneySlot correctly refuses that. Manual-replay test invoked Tutorial from Gameplay, where no such menu action is offered, so dispatch correctly ignored it. Candidate19 moves profile initialization inside the paused skip modal and enters Title before invoking its actual Tutorial action. Assertions retained; no production relaxation. Build/retest pending.


Candidate19 built; focused PlayerShellRoutes passes durable EndAll/fresh recovery/bound quick-load/manual replay. ControllerAuthorityRoutes fails after pause resume because its now-initialized real profile activates the obsolete mastery access guard. This is a production compatibility defect under SPEC-TUT-008, not a reason to seed mastery in the fixture. Candidate20 replaces that guard with valid-profile access across deployment, resume and recovery; keeps campaign progression and profile failure boundaries. Briefing and skip text remove the obsolete lock. Historical denial assertions are superseded by the owner-approved optional-access contract: test actual unmastered skirmish deploy/resume/recovery and campaign briefing, retaining no-mastery, persistence failure, checksum and byte-preservation assertions. No new unit/building mechanic introduced. Epic UE5.8 save/load documentation rechecked: separate profile/playthrough data and small paused/menu persistence fit the existing schema2 store; no save-format migration or claim of replacing the store with USaveGame. Build20 pending.


Candidate20 built. Focused ControllerAuthorityRoutes passes; PlayerShellRoutes failed three dependent recovery assertions because the added campaign route created another checkpoint in the same timestamp interval as training. Candidate21 moves campaign coverage after training recovery checks, preserving all assertions and authentic saves. Read-only review also identified explicit Start Tutorial retaining the opt-out preference;21 now commits opt-in for explicit tutorial/practice and rolls back profile/practice on failed storage. Adds failure/retry, durable opt-in, real checkpoint/load retaining guidance, and corrupt-profile admission rejection. Removes unused restored-mode variable. No mastery or campaign progression granted by opting in/out. Focused/full/rendered requalification pending.


Candidate21 built; HUD routes pass, shell route failed two new navigation assertions. Title Journeys offers Recover (Load is pause-only), so the direct Load dispatch was correctly ignored. After explicit opt-in, Campaign correctly offers the first-run optional-training confirmation, which the fixture omitted. Candidate22 follows offered Recover and Campaign confirmation, adds early prerequisite guards for both, retains guidance/mastery assertions, and changes no production code. Earlier failures retained; qualification pending.


Candidate22 built; focused PlayerShellRoutes and ControllerAuthorityRoutes pass2/2 with wrapper0 and unchanged source/library identity. Covers EndAll commit failure/retry, opted-out fresh recovery/quick-load, explicit restart opt-in persistence/recovery, unmastered skirmish deployment/resume/recovery, campaign briefing, corrupt-profile refusal and existing HUD routes. Full integration now pending/running. Structural document check passed142 Markdown/87 skills/1297 links; no owner acceptance or broader roster completion implied.


Candidate22 full integration117/117, zero test warnings/errors, source/library identity unchanged. SaveIsolation records editor exit0, cleanup success, synthetic denial and protected-policy checks passed. Rendered qualification next uses recovery-human-22 copies of six original candidate14 save files with SHA-256 manifest; candidate21 copy was never launched. No roster completion or owner acceptance assigned.


Candidate22 rendered native-keyboard route at1280x720 passed optional skirmish setup/deploy/pause/resume/return, Slot1 owner training recovery, pause-menu save and load, and complete process restart/recovery. Original tick50652 crcAC6F17B0 restored with230Matter/90Dawn/14-of-30Logistics and no tutorial spotlight/dimming; Anchor-to-Aegis cyan conduit visible. New explicit save tick51472 completed. This closes opted-out recovery and mandatory-access defects for this candidate at automated keyboard/rendered evidence level; physical Hold-to-EndAll was not repeated, human understanding and owner acceptance remain open. Rendered route JSON/logs retained under recovery-human-22. Found next presentation defects: loaded battlefield retains FIELD MENU status, and save progress exposes raw codes/ticks. Candidate23 is limited to clear shared save/recovery receipts, keeping diagnostic logs and persistence unchanged, before Relay feedback work.


Candidate23 built; affected integration checks AsyncCheckpointLifecycle, AsyncCheckpointReplayBindingFailure and PlayerShellRoutes pass3/3, wrapper0, identity unchanged. Small presentation-only receipt correction is qualified against real pending/completed storage, retained failure preservation and both restored-status routes; last full117 suite belongs to22, not relabelled as23. Rendered save/load receipt check is next with copies of22 saves/profile; no simulation or serialization change.


Candidate23 rendered keyboard recovery/save receipt check passed at1280x720: recovery acknowledgement visible in pause, autosave uses plain language, manual pending and actual completion display distinct Saving checkpoint… / Checkpoint saved. messages. Source serialization and diagnostic evidence unchanged. Recovery-load waiting indicator and notification priority remain future feedback refinements; no smooth-load/human-acceptance claim. Candidate23 game closed normally. Next bounded mechanic: correct Relay smart-cast selector and expose its implemented ability state/command without changing authored +4/400/800 mechanics.


Candidate24 Relay batch: reproduced smart-cast selector choosing an invalid infrastructure caster (relay-repro-24-result.log); selector now delegates admission to ValidateRelaySupply. Focused corrected regression passed. Full native optimized/debug/address-undefined-sanitizer configurations each pass109/109 (relay-native-24.log, exit0). Relay Skiff roster name, role/limitation, Extend Relay command-card dispatch and owned-state readiness/active/disconnected/cooldown feedback implemented in source; no +4/400/800 balance change. Source frozen in relay-source-24-identity.json (493 files). Unreal build and integration/rendered checks pending; no completion or acceptance assigned. Network-client scoped ability feedback remains outside this live-local batch.


Candidate24 review found a compatibility display defect before qualification: HUD reconstruction of current Power Link connectivity does not represent historical replay-bound link rules. Candidate24 is not accepted for integration; after its in-flight build ends, repair by consuming authoritative owner-scoped Relay connectivity and add the command-card controller route regression. Online Relay feedback remains open (REL-FAC-006): scoped network snapshots currently omit ability timers/state, so this batch must not claim multiplayer HUD equivalence.


Candidate25 corrects candidate24 before scope expansion. Candidate24 editor build succeeded, but its reviewed historical display mismatch prevented qualification. PlayerView now supplies owner-only transient ConnectedRelayUnits computed by simulation authority; HUD no longer reconstructs network rules. Authentic schema29 replay baseline checksum is verified before adding test-only Link/Skiff probes under that historical policy. All110 native tests pass in optimized/debug/address-undefined-sanitizer configurations (relay-native-25.log, exit0). Added mixed-force command-card dispatch, executed activation, disabled-state and stale-repeat regression. Source frozen in relay-source-25-identity.json; editor build and Unreal/rendered qualification pending. No save/replay schema or balance change.


Candidate25a: editor build25 succeeded; only a native test assertion changed afterward to exercise a real opponent view (all110 native tests pass across optimized/debug/sanitizers, exit0). Focused Unreal result3/4: controller dispatch, Relay adapter and widget pass; FieldHudAuthority fails disconnect feedback, retained in relay-focused-25a (wrapper3). Isolated relay-disconnect-probe confirms tick201, destination reached, active=false, capacity18, connected0: existing simulation ends the ability on disconnection and retains cooldown. HUD hid disconnection behind cooldown. Candidate26 fixes presentation to show Disconnected: +0 Logistics plus remaining cooldown, preserving the failed assertion and all gameplay rules. Build/integration/rendered requalification pending.


Candidate26 build succeeded; focused Unreal checks pass4/4 (FieldHudAuthority, RelaySupply, ControllerAuthorityRoutes, FieldHudWidget), wrapper0. Previously failing disconnect assertion remains unchanged and now passes. Source/library identity recorded in relay-candidate-26-identity.json. Full integration and rendered route pending; no roster completion or owner acceptance.


Candidate26 full integration passes117/117 with zero test warnings/errors, wrapper0, save isolation/cleanup passed, source/library changed[]. Evidence: relay-integrated-26 and relay-integrated-26-identity-check.json. Rendered route launched with isolated copied saves under relay-human-26 at requested1280x720; qualification underway.


Candidate26 rendered native-keyboard route verified at1280x720: authentic copied outpost recovery, F7 mixed-force selection, Equals Extend Relay activation (Logistics30->34), pause/save/load restoring active bonus, expiry to30 with cooldown, D pan plus Space reticle order moving actual force out of connection, visible Disconnected:+0 Logistics/Cooldown26s, pause/exit confirmation/title, CmdQ exit0. No controller calls/seeded completion used for this rendered route. CUA mouse click and scroll moved pointer but did not trigger actions; mouse route remains unqualified. Found faint disabled text and raw coded keyboard refusal. Candidate27 applies explicit FButtonStyle disabled brush per Epic UE5.8 docs/installed SButton.cpp, keeps disabled admission intact, and logs Relay refusal codes while showing plain explanations. Build/affected integration/rendered recheck pending.


Candidate27 built and affected Unreal checks pass4/4, wrapper0. Disabled-action readability test also confirms activation remains refused. Latest full117 suite is candidate26; no simulation/native source changed in27. Rendered recheck launched under relay-human-27 with copied26 saves.


Candidate27 rendered check: plain Relay refusal is verified and diagnostics retained, but disabled child text still dims despite explicit button brush. Candidate27 is not accepted as the readability fix. Candidate28 moves the authoritative ability status into a normal text line above the command grid and leaves the disabled button label/state intact; timer updates no longer expand the button. Earlier lifecycle and failure assertions retained. Candidate27 exited normally; candidate28 build/affected integration/rendered checks pending.


Candidate28 build succeeded (relay-build-28.log, exit0), source unchanged and libraries frozen in relay-candidate-28-identity.json. Following the host permission-mode change to restricted workspace execution with escalation unavailable, the required focused launcher failed BEFORE starting Unreal: relay-focused-28/SaveIsolation/launcher-result.json records prelaunch_failure=true, synthetic denial probe failure71, sandbox-exec: sandbox_apply: Operation not permitted; wrapper exit9, cleanup succeeded. This is an unavailable verification prerequisite, not a gameplay test failure. Do not bypass the protected-save policy. Candidate28 focused integration and rendered readability remain unverified; candidate26 retains last full117 pass and rendered Relay lifecycle/disconnection, candidate27 retains4 focused passes/plain refusal but failed rendered readability. Resume candidate28 focused checks and rendered route when the host permits the required sandbox launcher, before any new mechanics. No completion/P4 readiness/owner acceptance assigned.


Candidate28 verification resumed after owner restored full execution access. Unchanged source/library identity confirmed; relay-focused-28-retry passes4/4 with wrapper0. Original prelaunch permission failure remains preserved. Rendered status-line verification launched under relay-human-28 with copied saves; final visual result pending.


Candidate28 rendered keyboard recheck passes at1280x720: separate bright cooldown/ready/active status, fixed button grid through changes, actual Equals activation with30->34capacity, pause/resume retaining active state. Plain refusal retained. relay-human-28/rendered-route.json records scope; mouse route and human acceptance remain open. This closes the rendered readability defect from27 at automated keyboard evidence level. Latest full117 suite remains26, affected checks28 pass4/4. Next bounded work is Bulwark deployment control/state feedback, preserving mechanics and exact requirements.


Bulwark batch29 diagnosis: new native Bulwark deployment commitment timing test reproduces immediate shield activation (bulwark-repro-29.log,0/1, exit1), violating SPEC-UNIT-003/REL-FAC-00520-tick Deploy/15-tick Pack. Inspection also finds DamageAfterDirectionalCover tests attackerForward>0 only: front half-plane180degrees instead of REL-FAC-005.AUTH120degrees. Normal local selection dispatch multicasts rather than SPEC-CMD-013 smart-cast, and selector excludes deployed units needed for Pack. These are open gameplay defects, not presentation acceptance. Transition/arc compatibility architecture is under review before production edits; latest qualified runtime remains28. New native failing regression is intentionally retained; no full-suite pass claimed for dirty test29.

Bulwark batch29 implementation update (qualification pending): the explicit 20-tick Deploy and
15-tick Pack commitments now run in EchoesSimCore; the completed deployed state changes only
at the deadline. Repeated phase requests are refused with a readable adapter explanation.
Current combat applies the required 120-degree frontal arc; replay versions24–27 retain the
old instant toggle and half-plane behavior through a separate version cutoff. Schema31 appends
ordered transition records; protocol5/view3 transmit facing, phase and deadline. Historical
schemas remain loadable. The migration fixtures now explicitly project representable schema31
state to30 before older conversions and refuse active commitments rather than erase them.

A genuine pre-repair candidate28 schema30/replay27 writer was archived before production edits.
`Tests/Native/Fixtures/LegacyReplay/schema30-bulwark-receipt.json` binds its source/driver/baseline
hashes and original tick1/tick2 checksums. It is not a released-build claim or a new-schema
projection. Native debug29c passes114/114, including exact timing, 16 mirrored angular-boundary
cases, repeated input, mid-deploy/mid-pack save/load, malformed transition refusal, keyframe/delta
round trips and the original replay oracle. Build29 succeeds (208.18s). Source/build identity is
`BuildArtifacts/Evidence/tutorial-redesign-20260908/bulwark-candidate-29-identity.json`.
Affected Unreal integration is running; full integration and rendered interaction remain open.
Candidate28 remains the latest rendered-qualified runtime. Existing movement/order behavior is
preserved during commitments; the review suggestion to immobilize the unit is not adopted as an
unstated balance change. SPEC-UNIT-003 describes movable cover and REL-FAC-005 explicitly defines
35% deployed movement; their coordinate-anchor wording does not by itself define a phase movement
lock. Smart-cast dispatch, command-card direction targeting and complete Bulwark feedback remain
in the same open unit work package. No P0–P4 completion, P5 advancement or owner acceptance claim.

Batch29 qualification update: affected Unreal checks pass5/5 with zero test warnings/errors
(`bulwark-focused-29/index.json`): BulwarkDeployment, ProtocolAdmission,
SeveralVoicesOneCommand, TheBrokenSun and alternate-resolution persistence. The launcher exits0;
save-isolation cleanup, protected-path policy and denial probe all pass. The same source then
passes114/114 native tests in optimized, debug and address/undefined-sanitizer configurations
(`bulwark-native-qualified-29.log`, wrapper0). macOS leak-capture limitations remain unchanged.
The full Unreal suite is running against identified candidate29; rendered qualification is pending.
A fresh skirmish is required to exercise current commitments: a resumed replay-bound old match
intentionally preserves its original gameplay policy and is not evidence of new-match timing.

Full Unreal candidate29 result:116/117 pass; FactionResearch fails only its stale assertion
that the current replay version is27. The version was intentionally advanced to28 for Bulwark
commitments, with27 retained as the Link mechanics compatibility cutoff. This is an outdated
fixture, not a passed candidate or a research gameplay defect. The failing report remains at
`bulwark-integrated-29/index.json`; version-assertion repair and subsequent results must qualify
the successor before rendered testing or additional mechanics. No coverage removal is authorized.

Candidate30 closes the integration fixture failure: Research focused1/1 and full Unreal117/117,
zero test warnings/errors, wrapper0 and save-isolation cleanup pass; source/build recheck has no
changes. Rendered30 is NOT qualified: native keyboard fresh-skirmish route at1280x720 accepted
Bulwark deployment but focusing the selected force via the alternate End diagnostic key puts the
force behind the bottom console. Source diagnosis: SnapKeyboardTargetToSelection moves the raw
camera pivot to the centroid, whereas headquarters deployment correctly compensates the actual
ground projection and HUD. A missing regression is being added before repair. CUA super+F/ctrl+F
did not reproduce the owner's previously confirmed physical Command+F behavior; this is retained
as an input-tool limitation pending stronger attribution, not a claimed Mac user regression.
Mixed selection also reported four unrelated units as rejected and lacks Bulwark state feedback.
The route was paused, saved through the visible menu (Checkpoint saved), and closed normally.
`bulwark-human-30/rendered-route.json` retains exact scope and saved-file identities. Camera
framing must close before expanding unit behavior. No rendered/P0–P4 completion or acceptance claim.

Focus regression31 reproduces the rendered framing defect in
TutorialAnchorSelectionProgression: the measured clear-battlefield ground center does not match
the selected Anchor (`focus-repro-31/index.json`,0/1,wrapper3). The original test only checked
navigation revision/selection survival. Its new projection assertion is retained unchanged.
The related keyboard reticle also used the full viewport midpoint over the console. Repair now
shares clear-battlefield geometry across headquarters framing, selected-force framing, keyboard
ground/entity targeting and the rendered reticle. Keyboard offsets remain relative to that
stable area, including when status text fades, and cannot target through bottom chrome. Current
build31 is in progress; neither the regression repair nor rendered framing is claimed passed yet.
Epic UE5.8 orthographic-camera documentation and installed LocalPlayer/PlayerController projection
API declarations were checked before these changes. The no-Blueprint framing repair remains
presentation-only; Bulwark simulation/save/replay/wire code is unchanged from qualified30.


Camera candidate31 qualification update: `focus-build-31b.log` records a successful rebuild.
The projection regression and six affected camera/HUD/input checks pass (7/7,
`focus-fixed-31/index.json`, launcher exit0). Read-only review found an invalid-projection
failure path that moved the camera before returning false; this is repaired by resolving the
complete ground projection before mutating navigation/input. The added horizontal-camera
regression verifies refusal preserves position, navigation revision and movement/edge input.
`focus-candidate-31-identity.json` freezes the seven changed source files and build hashes
against candidate30. Full integration and same-candidate rendered qualification are running;
no rendered or release acceptance is asserted. Historical focus-repro31 failure is retained.

Candidate31 full integration passes117/117 (`focus-integrated-31/index.json`,wrapper0,
protected denial and cleanup pass). Identity check reports no source/build drift. The same
candidate's native-keyboard rendered route recovered the prior checkpoint and focused five
combat units visibly above the console at1280x720 content size. Pause/save receipt and clean
close were observed (`focus-human-31/rendered-route.json`). End was an alternate automated
diagnostic; this does not replace physical Mac Command+F evidence. Camera obstruction is
closed for this route. Bulwark state feedback and mixed-selection rejection wording remain
the next unit-specific failure; no complete Bulwark/P0-P4 or owner acceptance claim.


Bulwark feedback candidate32 is under qualification. Owned local and scoped-network views
now share state/progress text and a barrier direction command. Mixed-selection role feedback
is compact and keeps Relay status; non-Bulwarks no longer inflate local rejection counts.
The card arms a battlefield target rather than casting into its own UI coordinates. Source
regressions exercise genuine deployment/packing, remote owner visibility and card arming.
`bulwark-feedback-build-32.log` failed (exit6): root wrote the widget while its compile was
running; the compiler read a partial file ending in a function parameter. The complete source
was subsequently checked; this is an integration-process failure, not a waived result. Source
writes are frozen for rebuild32b and qualification. No32 build/test/rendered pass is asserted.
Closest-only ability dispatch / Ctrl-all remains a separate unresolved SPEC-CMD-013 gate;
this feedback batch does not claim to fix it. Network keyframes have no tick-rate field, so
network feedback reports exact phase/progress without inventing a seconds conversion.


Rebuild32b passed (`bulwark-feedback-build-32b.log`). The four selected feedback/runtime
checks pass (`bulwark-feedback-focused-32/index.json`,wrapper0): FieldHudAuthority,
ControllerAuthorityRoutes, FieldHudWidget and BulwarkDeployment. A requested CommandDeck
filter was incorrectly named and matched no test; CommandDeckModel is included by the full
integration run now executing. The network-owner assertion passes with unchanged fixture:
ScopedEntityState defaults are health1/max1/completedtrue, contrary to a review
assumption disproved by the header and focused result. This is model-level network coverage; no remote rendered session is claimed.
`bulwark-feedback-candidate-32-identity.json` identifies six source changes from qualified31.
No32 rendered pass or complete unit acceptance is claimed pending direct inspection.

Candidate32 full integration passed117/117 with wrapper0, cleanup/protected denial pass,
and unchanged source/build identity (`bulwark-feedback-integrated-32`). On2026-09-09 the
same candidate was recovered via the rendered menus. Native Backslash input visibly produced
Packing26% with protection retained, Mobile/inactive, Deploying20% without protection,
and Deployed frontal protection. The mixed-force card and state remained readable at1280x720;
unrelated units no longer counted as rejected. Pause/save receipt was observed before closing.
Evidence: `bulwark-feedback-human-32b/rendered-route.json`. CUA mouse and short-drag did not
activate the card; prior alternate input armed it, but the interrupted target gesture has no
verified result. Mouse-card, remote-rendered and owner gates remain explicitly open. The
next existing gameplay failure is SPEC-CMD-013 multi-cast dispatch;32 does not fix that behavior.


Local Bulwark dispatch33 is in qualification for SPEC-CMD-013. The controller now resolves
its caster list through the existing SimCore smart-cast eligibility/distance rules, filtering
pending ToggleDeploy actors before resolution. Empty selected candidates explicitly produce
no order (the core resolver's empty-list all-unit search is not used). Ctrl selects every
eligible caster exactly once; normal input selects one. New regression uses genuine queued
commands and transitions to check successive clicks, tie order, mixed ownership/roles,
duplicate IDs, no-selection safety and pack/redeploy eligibility. Build33 is pending.
The network path still uses the older multi-cast route and is not qualified for CMD-013;
its aggregate admission response needs a bounded reconciliation repair before claiming
network rapid-click correctness. SimCore/save/replay formats are unchanged by local33.


Build33 passed; its five focused checks passed with wrapper0 and unchanged identity
(`bulwark-dispatch-focused-33`). Read-only review then found an uncovered admission mismatch:
a packed Bulwark exactly under the target is selected first but the adapter rejects its missing
facing, preventing fallback to the next caster. Existing fixture casters were all equidistant,
so those checks did not establish geometric nearest ordering. Candidate33's rendered run was
closed at the title before qualification. Repro34 adds missing-direction normal/Ctrl cases,
an unequal-distance case and the valid same-position packing exception; assertions are not
weakened. No completed dispatch or owner acceptance claim is made from the earlier5/5.


Repro34 failed as expected: CommandDeckModel0/1, two errors identifying missing-direction
normal and Ctrl selection (`bulwark-direction-repro-34/index.json`,wrapper3). Unequal-distance
and same-position packing checks passed. The resolver now mirrors the adapter's direction
prerequisite before choosing candidates: packed actors at the target are excluded, deployed
actors remain eligible to pack. The no-caster message now explains pointing away and waiting
for active transitions. Build34 is running; subsequent results remain required.


Direction build34 passed (`bulwark-direction-build-34.log`), then all five affected checks
passed (`bulwark-direction-focused-34/index.json`,wrapper0), including both unchanged
previously failing assertions. `bulwark-direction-candidate-34-identity.json` freezes the
source/build for full integration now running. Rendered two-caster qualification remains
pending; the prior33 five-test pass is not substituted for this repaired candidate's results.


Candidate34 full integration completed: `bulwark-direction-integrated-34/index.json`
117/117, wrapper0; save isolation cleanup and protected denial passed, source/build identity
unchanged. Rendered fresh skirmish at 1280x720 produced a second Bulwark through the Foundry;
normal Backslash visibly deployed exactly one of two selected Bulwarks (20% then1/2 deployed).
Checkpoint save receipt was visible before normal exit. Retained route:
`bulwark-direction-human-34/rendered-route.json`. CUA Ctrl chord only dispatched one packing
order; modifier and mouse-card input remain unqualified, with no owner acceptance claim.

This route reproduced a new input integration failure: semicolon also activated the inherited
Unreal debug camera. Installed UE5.8 BaseInput.ini binds Semicolon to ToggleDebugCamera; its
PlayerInput::GetBind permits extra modifiers. Other inherited function/page shortcuts overlap
game actions too. Candidate35 clears the development DebugExecBindings layer in project input
config and extends SharedKeyDispatch to detect actual initialized debug/action overlaps.
Build35 is running; no pass is inferred yet. No gameplay behavior expansion in this repair.

Separate existing feedback gate confirmed by read-only review: a v27 replay-bound checkpoint
can resume live with instant Bulwark semantics while the HUD advertises current 1s/0.75s timing.
That mismatch remains unresolved; current local dispatch fixes do not qualify online multicast.


Candidate35 qualified for the inherited-debug-key repair: build passed, focused3/3 and
full117/117 passed (`input-debug-focused-35`, `input-debug-integrated-35`), wrapper0 and
save-isolation cleanup/protected-denial checks passed. `input-debug-candidate-35-identity.json`
and full-run `identity-check.json` retain unchanged source/build attribution. Rendered fresh
1280x720 skirmish: semicolon queued and completed one Bulwark without debug-camera takeover;
F2 opened technology without changing render mode; normal Backslash deployed one of two
selected Bulwarks with visible progress and1/2deployed state. Pause/save produced a visible
Checkpoint saved receipt before normal exit. Route: `input-debug-human-35/rendered-route.json`.
This closes the reproduced development-key collision, not physical modifier/mouse, online,
legacy feedback, complete player journey or owner acceptance gates. Legacy feedback review
also found120-degree guidance applied to the old front-half-plane barrier, so the historical
repair must address both arc and timing claims.


Historical Bulwark HUD reproduction36 failed exactly two assertions in
`bulwark-legacy-repro-36/index.json` (0/1, wrapper3): live historical guidance advertised timed
deployment and the current120-degree arc. The test loads the authentic schema30 baseline,
verifies original replay27 final checksum17785241889350991135, restores a snapshot and
continues its replay prefix before inspecting the live HUD; instantaneous deployment remains
asserted. Its build and source identity are retained separately from the fix.

Repair36 adds transient `PlayerView::UsesBulwarkCommitmentRules()` from simulation authority.
Historical live HUD now suppresses commitment seconds and uses front-facing protection text;
current local and compatible online games keep current arc guidance. No snapshot, replay,
checksum or network format changes. The original failing assertions are unchanged; build36
is running and subsequent focused/full/rendered results remain required. Root remains the
single write/build/editor owner.


Repair36 build passed (`bulwark-legacy-build-36.log`,198.21s), and the affected HUD,
Bulwark mechanics and command-deck checks passed3/3 (`bulwark-legacy-focused-36/index.json`,
wrapper0, cleanup/protected-denial pass), including the unchanged previously failing historical
assertions and current timing assertion. `bulwark-legacy-candidate-36-identity.json` freezes
four changed source files from candidate35. Standalone simulation and full integration are
next; rendered recovery remains pending. A copied candidate28 checkpoint has an embedded
replay27 header; its original files and copy hashes are retained in
`bulwark-legacy-human-36/checkpoint-provenance.json`. Header inspection does not replace
runtime checksum admission. Document routing/link checks passed separately.


Candidate36 full qualification and Art handoff — 2026-09-09: optimized native114/114 and
full Unreal117/117 passed; full wrapper0, zero warnings/errors, cleanup/protected-denial pass,
and source/build hashes unchanged. Evidence: `bulwark-legacy-native-36`,
`bulwark-legacy-integrated-36`, `bulwark-legacy-candidate-36-identity.json`.
Rendered1280x720 recovery of a retained replay27 checkpoint succeeded. Deployment/packing
showed matching states, single-Bulwark text omitted current commitment timings, and deployed
text correctly described front-facing protection. A subsequent save visibly reached
Checkpoint saved. `bulwark-legacy-human-36/rendered-route.json` retains the route and limits;
reloading that newly saved continuation was not performed before handoff. No owner acceptance.

Owner assigned HUD/all-menu presentation to Art task01a0772f-7d98-7cd2-8d56-2254d585cf17.
After this ledger update, mechanics root releases the editor/build slot and presentation
widgets/layout/tests to Art; it will not edit those paths or launch competing heavy runs.
Simulation, checkpoint, command dispatch, HUD data-model and tutorial progression logic remain
with the mechanics task, with no source writes during Art qualification without coordination.
Preserve all dirty changes. Art becomes integration owner for its presentation candidate and
must establish a new source/build identity and affected input/recovery/rendered results.
Outstanding mechanics gate: online Bulwark still multicasts through the generic selection
path; bounded client pending reconciliation review is available. Physical mouse/modifier,
connected journey, tutorial redesign, full roster and owner gates remain open; P5 not cleared.


Save/recovery connected qualification resumed — 2026-09-09: the owner renewed the
save/recovery gate before additional mechanics. Root retains main checkout integration and
exclusive build/editor ownership; Art, backend and M01 environment tasks confirmed isolated
source-only work. This supersedes the earlier presentation handoff until recovery qualification
finishes. Candidate36 production policy/HUD repair remains unchanged. Candidate37 extends
existing QuickSaveLoad coverage across authentic replay27 and current rules, two changed-state
saves, fresh scenario reload, deadlines/facing/checksums and displayed timing/coverage. This
adapter restart is not a process-restart claim. The original schema29 second-save test remains.
Corrupt-generation admission now checks active tick/checksum non-mutation, and asynchronous
write fault injection checks byte-identical prior primary/backup plus failed player feedback.
Build37 passed; incremental build37b and focused/full results remain pending.

Required regression gate for gameplay-rule or HUD changes affecting restored state: run the
existing QuickSaveLoad, FieldHudAuthority, asynchronous checkpoint lifecycle/binding-failure,
checkpoint worker, autosave recovery and PlayerShellRoutes checks; qualify the required full
suite; then exercise historical and current load/play/save/process-exit/restart/reload/play
through the rendered UI on the same identified build. Include tutorial opt-out and actual
policy feedback. Preserve original saves and use isolated copies/fault injection. Native or
adapter checks alone cannot close this gate. Owner acceptance remains with Angelis.

Candidate37 focused result:6/7 passed; QuickSaveLoad failed before gameplay because the
24x24 historical combat fixture did not satisfy the game adapter's 64x64 Glass Scar context.
This is a fixture prerequisite defect, not save restoration failure. Original failure retained
in recovery-roundtrip-focused-37 (wrapper3). Candidate38 uses a separately emitted fixture
from the hash-verified unchanged candidate28 writer, with the actual Glass Scar terrain/seed;
its own historical checksum16781565848365372905 is recorded by that writer. The original
24x24 fixture and checksum remain unchanged. Current fixture likewise uses legal context.
Admission checks and behavioral assertions are unchanged. Build38/qualification pending.

Candidate38 focused result:6/7 passed; corrected map admission succeeded, then the
command adapter correctly refused MATCH_FINISHED because the combat-only fixture lacked
headquarters. Candidate39 supplies both headquarters in the authentic writer and current
fixture, preserving the same transition/checksum/HUD assertions. Writer39 self-verified the
new scenario checksum4590309749637731261; writer38 fixture/driver/receipt remain retained
under bulwark-checkpoint-writer-38. Scope cleanup now also stops presentation before world
teardown on failed prerequisites. No production changes. Build39/qualification pending.

Candidate39 build passed (25.04s) and focused recovery checks passed7/7, zero warnings/errors,
wrapper0 and isolated-save cleanup/denial checks passed. This includes both changed-state
Bulwark generations under historical/current policy, retained schema29 second-save regression,
corrupt non-mutation, failed primary/backup preservation, completion-only saved feedback and
tutorial preference/controller recovery. Identity:recovery-roundtrip-candidate-39-identity.json.
Full integration is running. Actual process restart/reload UI remains required; copied
historical/current player checkpoints and their hashes are prepared in recovery-roundtrip-human-39.

Candidate39 full Unreal suite passed117/117, zero failures/warnings/not-run, wrapper0.
Save-isolation cleanup, denial probe and empty scoped storage passed; files/build identity
comparison is unchanged. Evidence:recovery-roundtrip-integrated-39. Production simulation
remains byte-identical to candidate36's native114/114 qualification; no native rerun claimed.
Rendered historical/current process-restart routes are now underway on candidate39.

Save/recovery candidate39 qualification — 2026-09-09: both connected rendered keyboard
routes passed in actual1280x720 Development game windows on the unchanged identified build.
Historical: loaded retained checkpoint55036, deployed Bulwark12 with historical front-facing
coverage and no current timing claim, saved55807, saw Checkpoint saved, exited process0,
restarted/recovered55807 from primary, verified deployed state/resources and packed afterward.
Tutorial opt-out remained effective throughout historical recovery. Current: loaded1399,
observed current120-degree coverage and1s/.75s guidance, started packing (visible26%), paused,
saved1813, saw Checkpoint saved, exited0, restarted/recovered1813 from primary, continued from
Mobile into a new timed deployment (visible15%, then deployed with matching coverage).
Both restarted processes exited0. Original save hashes and source/build hashes remain intact.

Evidence:recovery-roundtrip-human-39/rendered-route.json, copy-provenance.json,
original-save-integrity.json and identity-check.json; historical/current PlayerRoute.log and
RestartRoute.log support the actual CUA menu/keyboard interaction recorded in this task.
Focused7/7 and fullUnreal117/117 remain passed. The current repair is a presentation-policy
repair; second-save compatibility and tutorial profile preferences remain separately protected.
Write fault injection fails before filesystem replacement and preserves both prior valid byte
generations; corrupt/incompatible admission preserves active state. No persistence rewrite.

Remaining evidence limits: automated rendered keyboard interaction is not physical mouse or
held-modifier qualification, human comprehension or owner acceptance. End was only a diagnostic
alternate. Historical load waiting UX remains unpolished; exact transition deadlines and replay
checksums are tested by the connected adapter regression, not inferred from screenshots.
No outstanding failure remains in this bounded recovery batch. Original37/38 fixture failures
and36 presentation reproduction are retained with subsequent39 results. Other gameplay,
tutorial, presentation, packaged-journey and P0–P4 acceptance gates remain open; P5 not cleared.

## SC2 audit integration into continuous work — 2026-09-09

Owner-authorized intake: all 162 functional items and 32 feedback events are captured in the [master crosswalk and event contracts](Requirements.md#gameplay-completeness-intake--sc2-audit-2026-09-09), with [lifecycle and decisions](RequirementsState.md#sc2-audit-intake-and-continuous-work--2026-09-09) in the existing state authority. The [audit](SC2GameplayGapAudit.md) is a dated finding source, not another plan or current defect ledger. This section extends the current P0–P7 work packages; it creates no parallel roadmap, duplicate tasks, scheduler or blanket acceptance.

| Existing package | Audit/event coverage to carry with the current work | Delivery responsibility / outcome |
|---|---|---|
| P0 — current integration baseline | Any affected row/event, especially current input, save/replay, authoritative feedback and capacity invariants | Existing integration owner first preserves passing candidate evidence and diagnoses real current regressions. Do not reopen the repaired historical preview/depletion/save defects just because they appear in an old note. |
| P1 — shell and persistence | 101, 109–110, 127–135, 151–162; F29–F32 | Player Experience: ordinary entry/exit/settings/recovery, accessible navigation, safe display changes and understandable results/replay routes. Complete source-present routes through their missing evidence class; advanced analysis can finish with P5/P7 after the basic journey is stable. |
| P2 — controls and complete offline match | 001–100, 133–135, 146–150, 151–162; F01–F28 plus match/recovery events | Gameplay/Player Experience: selectors, binding reconciliation, subgroups, visible queues, camera bookmarks, production/rally, economy, combat, scouting, AI and feedback. Only the decision-dependent extensions wait for their named TBR. |
| P3 — representative M01 and teaching | 071–090, 102–115, 139–149; applicable F01–F32 | Campaign/Presentation: SPEC-TUT-008 seven optional chapters/specializations, purpose-first foothold, ordinary M01 branches/retry/continuation, coherent art/audio/cinematics and input. Preserve explicit skip/opt-out and genuine mastery separation. |
| P4 — identified playable candidate | Complete connected P1–P3 path and its mapped feedback events | Integration/QA: actual same-candidate mouse/keyboard, rendered/listening, failure/recovery and required novice/owner evidence. Exercise tutorial completion and intentional opt-out routes. Earlier plan wording that conditions campaign/skirmish access on tutorial mastery is superseded by SPEC-TUT-008; it is not an active access gate. |
| P5 — campaign/roster/offline breadth | 037–100, 112–115, 127–150, roster-specific learning/accessibility | Existing content/gameplay owners finish all factions, maps, missions, counterplay and production presentation; complete advanced replay/codex/lab/editor work against their exact IDs without claiming P4's representative slice proves it. |
| P6 — approved additional modes | 116–126 plus every shared control/feedback/save/fog obligation used online or in Conquest | Mode/Network: connected Conquest runs, usable direct/LAN lobby, six active participants, team/FFA/comp-stomp, session chat, explicit observers and real-peer recovery/security. Resolve TBR-UX-008 before adding observer connections; observers cannot replace playing seats. Hosted accounts/relay/public service work stays deferred. |
| P7 — full release qualification | All bound rows/events and documented disposition of optional branches | QA/Release: close exact missing evidence for cross-mode accessibility, final art/audio, full-load performance, installation/recovery and owner acceptance. No row is closed solely because a generic test suite or this traceability guard passes. |

Before each ongoing increment, cite its audit row/event and controlling requirement IDs, then consult their latest RequirementsState evidence. After the increment, update only those IDs/classes with identified source/build and retained evidence. Maintain one owner per affected path and the existing heavy-run coordination. If a later finding changes an audit recommendation, update the master/state disposition rather than silently dropping it or editing a historical observation into a claimed pass.

Run `python3 Scripts/check_gameplay_audit_traceability.py` after intake/mapping changes; it is also part of `python3 Scripts/check_agent_docs.py`. This prevents missing/duplicate functional or feedback bindings, unknown requirement IDs and unrecorded TBR references. It does not evaluate gameplay. Any future additions to the audit must receive explicit master coverage/disposition before the guard passes.
