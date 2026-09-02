# Demo Readiness Requirements — Sole Requirements and Acceptance Ledger

Authority: `Docs/DemoRecoveryDirective.md` (owner directive, 2026-09-02). Owner and final
acceptance authority: Angelis Pseftis. Linked release backlog: `Docs/GameCompletionDirective.md`
(bodies not duplicated here). This is the ONLY copy — no drafts, revisions, or numbered copies.

## Global verdicts and identities

* DEMO-GOV-001 stands: the current demo is `HUMAN REJECTED` (owner, 2026-09-02). Rejected
  candidate identity: package `BuildArtifacts/Packages/Mac-Development-20260902T011241Z-f0cf042b/`
  from clean `f0cf042bea800c474b1c3e08c557d0aae49ff744` (origin/main), macOS Apple Silicon.
* Authoritative source state at ledger creation: `origin/main = f0cf042`, tree clean.
* Write owner: Claude Code lane fleet (coordinator). Read-only reviewer: ChatGPT Codex when
  active. Ledger author: coordinator. Baseline auditor (separate, read-only): QA lane.

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

## State register

All requirements: `OPEN` as of ledger creation (2026-09-02, source `f0cf042` clean), except
DEMO-GOV-001 which is ACTIVE as stated. Per-requirement state changes, commit/package bindings,
evidence locations, limitations, and acceptance records are appended below this line as dated
entries — never by rewriting the requirement bodies above.

### Change log

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
