# Initial Release Requirements — Sole Normative REL-* Ledger

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

* 2026-09-02 — Ledger installed by coordinator from the owner's order. All REL-* records OPEN.
  Open ledger tasks: REL-GOV-002 bidirectional gate mapping; incremental verbatim section
  transcription + QA fidelity audit; TBR packet preparation (background, demo priority intact).
