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
