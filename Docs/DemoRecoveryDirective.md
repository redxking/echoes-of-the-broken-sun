# Echoes of the Broken Sun — Human Feedback, Demo-Recovery Directive, Requirements Baseline, and Definition of Done

Human Owner and final acceptance authority: Angelis Pseftis
Applies to: Claude Code, ChatGPT Codex, and any delegated development agents
Project: The authoritative Echoes of the Broken Sun Unreal project
Current human verdict: REJECTED — NOT DEMO-READY
Received: 2026-09-02, verbatim from the owner via the coordinator session. This file is the
authoritative on-disk copy for all lanes. Write owner: the Claude Code lane fleet (coordinator
session). ChatGPT Codex, when active, is a read-only reviewer (owner may re-designate).
Related: `Docs/GameCompletionDirective.md` (professional-release backlog — unchanged);
`Docs/DemoReadinessRequirements.md` (the sole requirements/acceptance ledger, per §4).

Read this entire directive before continuing development. Treat it as binding human feedback and a corrective development order.

## 1. Direct human assessment

I do not accept the current build as a game demo.

It currently feels like a technical prototype that opens into a map, not a modern RTS designed for a human player. It does not adequately establish who the player is, what world they have entered, why they are there, what is happening, or what they should care about. It expects knowledge the player has not been given.

The current experience is missing or failing major player-facing elements:

* Mouse interaction does not reliably select units or activate menu controls.
* Too much of the experience depends on keyboard commands.
* The game does not progressively teach the player how to play.
* The player is dropped into gameplay without sufficient story, purpose, or context.
* There is no acceptable opening cinematic experience.
* Voice-over, dialogue delivery, music, ambience, and sound feedback are absent, incomplete, unqualified, or not functioning in the packaged player experience.
* The menu and HUD look unfinished and do not adequately explain their options.
* Units, buildings, terrain, resources, and interactive elements are too visually similar.
* The graphics lack the detail, visual hierarchy, animation, effects, and atmosphere expected from a modern RTS.
* The game does not appear to adapt its graphics intelligently to the capability of the computer.
* The complete path from story introduction, through learning, into an actual match against the computer has not been demonstrated.
* Existing code, automated tests, headless campaign routes, or packages that merely launch have been treated as stronger evidence than they actually are.

This is not a request for another superficial UI pass, another list of proposed features, or more planning without implementation. The player experience needs to be reconsidered and rebuilt as one coherent product journey.

The game should use the interaction familiarity, teaching discipline, readability, responsiveness, and presentation quality of successful desktop RTS games as benchmarks. Warcraft III and StarCraft II are useful ergonomic references, and current released RTS games should also be studied. Replicate proven interaction principles, not protected art, layouts, dialogue, names, sounds, assets, or other copyrighted expression. Echoes of the Broken Sun must remain original.

## 2. Immediate objective

Turn the current playable-systems prototype into a polished, understandable, emotionally engaging demo candidate that an unfamiliar player can use without instructions from the developer.

The mandatory demo journey is:

Cold launch → title and menu → opening story sequence → progressive playable tutorial → demonstrated mastery of the fundamentals → transition into a complete match against computer AI → victory or defeat → results → replay, restart, or return to menu

The demo must not begin by simply opening a map and expecting the player to determine what to do.

This immediate demo journey takes priority over website work, additional campaign missions, alternate endings, multiplayer, broad release packaging, and other long-range work unless that work directly unblocks the demo. The existing `Docs/GameCompletionDirective.md` remains the professional-release backlog; it must not distract from fixing the player's first experience.

## 3. Required operating behavior

Inspect the current checkout, active work, authoritative documents, input configuration, build state, and most recent packaged demo before changing anything. Preserve all user work and existing in-progress changes.

Reproduce the reported mouse, menu, onboarding, audio, UI, and visual failures in the packaged build. The presence of input mappings, event handlers, assets, or tests does not establish that the packaged experience works.

Create one grounded implementation plan, then execute it. Do not repeatedly return with revised plans while the same player-visible failures remain.

Use the majority of effort and tokens for: repository inspection; implementation; asset integration; building and packaging; physical-input testing; rendered visual and audio inspection; performance measurement; defect correction; evidence collection.

Keep progress messages short and substantive. Contact the owner when: a real owner-level decision blocks continued implementation; a proposed change affects canon, characters, art direction, voice identity, supported platforms, demo scope, or an approved performance target; external credentials, signing authority, licensed assets, unavailable hardware, or human participation are required; a player-visible milestone is actually ready for acceptance.

Make reasonable, reversible implementation decisions without asking. Consolidate unavoidable decisions into one small decision packet with a recommendation, alternatives, consequences, and the latest date the decision is needed.

Continue the existing authoritative task and checkout. Do not create competing projects, duplicate checkouts, or parallel write streams. When Claude and Codex are both involved, one owns edits and the other performs bounded read-only research or evidence review.

## 4. Requirements control and human acceptance

Create and maintain one authoritative file: `Docs/DemoReadinessRequirements.md`. This file is the sole requirements and acceptance ledger for the immediate demo. Do not create draft, revised, final, or numbered copies. Link it to `GameCompletionDirective.md`, but do not duplicate the requirement bodies across multiple files.

Each requirement record must contain: Requirement ID; exact "shall" statement; player or product outcome; dependencies; planned verification method; required evidence; responsible implementation owner; engineering state; exact commit and packaged-build identity; evidence locations; known limitations; human acceptance state; human acceptance date and notes.

Permitted states — agents may assign: `OPEN`, `IN PROGRESS`, `IMPLEMENTED — NOT YET VERIFIED`, `AGENT VERIFIED`, `EVIDENCE READY`, `AWAITING HUMAN ACCEPTANCE`, `BLOCKED`.

Only Angelis Pseftis may assign: `HUMAN ACCEPTED`, `HUMAN REJECTED — CHANGES REQUIRED`.

The word `COMPLETE`, a checked box, a closed parent milestone, or an unconditional `PASS` is reserved for a requirement explicitly marked `HUMAN ACCEPTED`. An agent's automated test, visual review, or engineering judgment cannot close a requirement. An agent may record the owner's acceptance only when the owner has explicitly accepted the identified requirement for the identified build.

Requirements may be presented in logical milestone batches. Until the owner approves the batch, its requirements remain `AWAITING HUMAN ACCEPTANCE`. If accepted implementation later changes materially, the affected requirement automatically reopens.

Requirements audit — before freezing the baseline: (1) compare these requirements against the Development Bible, Game Completion Directive, Technical Architecture, Project Ledger, asset register, source data, current code, current tests, and current packaged build; (2) identify conflicts, omissions, ambiguous language, untestable statements, and unsupported prior completion claims; (3) make each requirement atomic, observable, traceable, and bounded; (4) add necessary technical child requirements without removing or weakening the human outcomes; (5) separate demo requirements from later professional-release requirements; (6) identify decisions only the owner can make and provide recommended defaults; (7) have a separate read-only reviewer audit the baseline for completeness and testability; (8) present the audited baseline to the owner in one concise review packet; (9) continue obvious, reversible blocker work while the audit proceeds unless a genuine owner decision prevents it.

Changing a requirement so existing behavior appears to pass is prohibited. Proposed changes, waivers, or deferrals require the owner's approval.

## 5. Mandatory demo requirements

(Minimum requirements; every requirement requires final human acceptance. The exact shall statements are transcribed into `Docs/DemoReadinessRequirements.md`, which is the ledger of record. Section keys:)

* A. Scope, integrity, and traceability — DEMO-GOV-001..010
* B. Complete player journey — DEMO-JRN-001..007
* C. Opening story and player orientation — DEMO-NAR-001..009
* D. Progressive tutorial and demonstrated learning — DEMO-TUT-001..022 (cycle: Explain → highlight or demonstrate → allow the player to act → verify the real game state → acknowledge success → explain why it mattered → unlock the next lesson)
* E. Mouse, keyboard, and interaction behavior — DEMO-INP-001..015
* F. Menu, HUD, and UX redesign — DEMO-UI-001..013
* G. Audio, voice, and cinematic sound — DEMO-AUD-001..013
* H. Art, animation, and battlefield readability — DEMO-VIS-001..013
* I. Automatic graphics calibration and performance — DEMO-PERF-001..015
* J. AI skirmish and complete match lifecycle — DEMO-AI-001..010
* K. Accessibility and learning support — DEMO-ACC-001..006
* L. Packaging, human testing, and final acceptance — DEMO-VAL-001..017

The complete verbatim requirement text for every ID above was delivered in the owner's 2026-09-02 directive and MUST be transcribed exactly into the ledger. The ledger transcription is authoritative once audited; until then the coordinator session's received copy governs.

## 6. Required implementation sequence

* Milestone 0 — Baseline and failure reproduction: audit and install the requirements ledger; preserve the current working tree; identify the exact authoritative build and source state; reproduce mouse, menu, audio, onboarding, UI, and visual failures; record the starting state without overstating prior evidence.
* Milestone 1 — Interaction foundation: repair mouse activation throughout all menus; repair selection, drag selection, contextual orders, command activation, cursor feedback, camera controls, cancellation, and remapping; package and physically test the complete interaction matrix; present the relevant requirements as one human review batch.
* Milestone 2 — The first five minutes: final title treatment; voiced and subtitled opening sequence; player identity, situation, stakes, first objective; transition into first tutorial lessons with final-quality UI, voice, sound, and visual presentation; review as an integrated player experience.
* Milestone 3 — Complete tutorial: entire curriculum; validate every progression condition against real player behavior; hints, retries, checkpoints, resume, recovery; verify a new player can finish without coaching.
* Milestone 4 — Complete AI demo loop: tutorial mastery → skirmish; setup, AI behavior, victory, defeat, results, restart, rematch, menu return; player applies what the tutorial taught.
* Milestone 5 — Production presentation: complete UI remake; raise terrain, units, buildings, animation, effects, iconography, lighting, fog, music, ambience, voice, and sound to the accepted demo bar; review at normal gameplay distance under combat load; remove every remaining debug or placeholder surface.
* Milestone 6 — Hardware adaptation and stability: first-run benchmark and Auto quality; presets, overrides, persistence, fallback, M1 qualification, stronger-hardware evidence; full rendered stability session.
* Milestone 7 — Human acceptance: uncoached human sessions; fix and retest failures; one exact candidate package; request acceptance in logical requirement batches; no completion declaration until the owner accepts every mandatory requirement.

A milestone may be agent-verified without being human-complete. Work may continue on later unblocked requirements, but no roll-up may conceal an unaccepted parent or dependency.

## 7. Evidence required for every acceptance request

Requirement IDs; what changed; exact commit and package; whether the source tree was clean; hardware, operating system, resolution, and preset; automated checks run and their bounded meaning; packaged physical-input path completed; visual and audio evidence actually inspected; known defects and limitations; exact steps the owner should perform; requested decision: accept or reject each listed requirement.

Do not ask the owner to approve an implementation based only on code, a mockup, a screenshot, an asset inventory, or a passing automated test when the requirement concerns interaction, presentation, sound, comprehension, enjoyment, or human play.

## 8. Communication format

Current milestone: / Requirement IDs in work: / Implemented since the last update: / Evidence obtained: / Remaining blocker or next action: / Decision needed from Angelis: None, or one clearly framed decision.

Do not repeat the full plan, narrate routine commands, or stop merely because an intermediate test passed. Continue working until a real decision, external dependency, acceptance gate, or genuine blocker requires the owner.

## 9. Persistent goal

Continue the authoritative development of Echoes of the Broken Sun from its currently rejected prototype/demo state to one polished, human-accepted demo candidate. Deliver one continuous packaged macOS player journey from cold launch through a voiced and subtitled opening, progressive mastery-gated RTS tutorial, and complete mouse-and-keyboard human-versus-AI match with results and replay/return paths. Repair all menu, mouse, selection, command, UI, audio, voice, cinematic, visual-readability, graphics-calibration, performance, accessibility, and stability deficiencies defined in `Docs/DemoReadinessRequirements.md`. Preserve canon, originality, asset provenance, current work, and evidence boundaries. Spend working time on implementation and verification, keep communication concise, and request human input only for genuine owner decisions or review-ready milestones. An agent may mark requirements evidence-ready but may never mark them complete. The demo is complete only when every mandatory requirement is tied to the same packaged build and explicitly accepted by Angelis Pseftis.

## 10. Final meaning of "done"

"Done" does not mean: the code exists; a test passes; a map loads; a package launches; an automated controller completes a route; a cinematic timeline triggers; audio files were generated; UI mockups look attractive; one screenshot looks improved; an agent believes the result is good enough.

For this demo, "done" means an unfamiliar human can launch it, understand the world and their role, use the mouse and keyboard naturally, learn every required fundamental through guided play, hear and see a coherent story and responsive game, distinguish what is happening, complete an actual match against AI, and leave wanting to continue — and the owner has explicitly accepted every mandatory requirement for that exact packaged build.
