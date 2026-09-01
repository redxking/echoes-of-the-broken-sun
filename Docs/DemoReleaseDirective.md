# Demo Release Directive

**Author and control owner:** Angelis Pseftis
**Created:** 2026-09-01
**Applies to:** every Claude session working in this repository until the demo ships
**Companion file:** [`CLAUDE.md`](../CLAUDE.md) — the standing session contract, build commands, and environment traps

This directive defines the work required to take *Echoes of the Broken Sun* from a verified playable-systems
prototype to an initial public demo release: a single-player, player-versus-AI macOS build containing the
Glass Scar skirmish and all fifteen campaign operations, with complete visuals and audio.

It is a plan, not a record of completion. Every gate in section 9 starts at NOT RUN. Move a gate only when
you have run it and read the result.

---

## 1. Authority and sources

The repository is the sole authority on how this game works. Before designing anything, read the file that
owns the question:

- `Docs/Archive/DevelopmentBible.md` — world, factions, Future Wells, combat, controls, campaign outline,
  interface, accessibility, art and audio direction, writing rules.
- `Docs/Archive/TechnicalArchitecture.md` — simulation, Unreal integration, AI, networking, save, replay,
  build architecture.
- `Docs/Archive/ProjectLedger.md` — decisions, milestones, performance budgets, the evidence register, risks,
  and the current known limitations. This is where you learn what is actually proven.
- `Docs/Archive/AssetRegister.md` — asset provenance, generation records, and the rules that govern them.
- `Docs/Archive/SetupAndBuild.md` — host state, toolchain, and the build/test/package procedures.
- `Content/Data/Source`, `Content/Narrative/Source`, `Content/World/Source` — authoritative source data.

Design recollections carried from other tools or earlier conversations are hypotheses. Check them against
these files before acting on them. Where this directive and the Bible disagree about creative intent, the
Bible wins and this directive is wrong — fix it in place.

## 2. Locked scope

Decided by Angelis on 2026-09-01. Do not renegotiate these without him.

**In the demo:**

- Glass Scar skirmish, player versus the AI opponent, all three factions selectable — Meridian Compact,
  Kharuun Assemblies, Hollow Choir.
- All fifteen campaign operations, from **What the Ledger Keeps** through **The Broken Sun**, playable by an
  ordinary person from an empty ledger to an ending, using rendered UI and physical input.
- Complete visuals: environment, unit and structure presentation, motion, effects, fog, UI, and lighting —
  no visibly labeled development placeholders remaining in the shipped build.
- Complete audio: music, ambience, combat and unit cues, interface sounds, alerts, and a qualified mix.
- macOS Apple Silicon distribution: signed, notarized, installable, and launchable on a clean machine.

**Out of the demo:**

- Multiplayer of any kind. The networking work stays behind its existing gates and ships disabled.
- Recorded voice acting. Dialogue is delivered as on-screen text with subtitles — see section 8.
- Windows or Intel Mac builds.
- Maps beyond those the fifteen operations and the skirmish already require.

**Asset production method:** deterministic procedural generation through `Scripts/generate_art_assets.py` and
`Scripts/generate_audio_assets.py` only. No marketplace, stock, scraped, commissioned, or third-party assets.
Every new family gets an entry in `AssetRegister.md` with its revision string, generation inputs, and the
evidence that regeneration is byte-idempotent.

## 3. Definition of done

The demo is finished when all of the following are true and each has evidence recorded in the ledger:

1. A person who has never seen the game installs the notarized build on a clean Apple Silicon Mac, launches
   it, and reaches the title screen without a terminal, a developer tool, or an instruction from Angelis.
2. That person completes a Glass Scar skirmish against the AI as each of the three factions, and completes a
   fresh campaign from an empty ledger through Mission 15 to an ending, using mouse and keyboard on rendered
   UI. Not headless automation. Not a scripted controller route.
3. Every rendered frame in that path uses registered authored assets. No Engine primitive stands in for a
   game object, no debug overlay is visible, and exposure and contrast are correct on the M1 Pro baseline.
4. Every gameplay event that the design says has a sound, has one; music and ambience play under the whole
   experience; the mix is qualified under simultaneous combat load and under reduced dynamic range.
5. The build holds the section 9 performance budgets in an ordinary packaged session and survives an
   uninterrupted 60-minute sustained run.
6. Accessibility settings all change behavior: remapping, UI scale, subtitles, high contrast, reduced motion,
   reduced flashing, color-vision-safe markers, audio category volumes, reduced dynamic range, offline pause.
7. Saves, autosave, and interrupted-session recovery work for both skirmish and campaign, with named slots.
8. Nothing in the shipped build, its store text, its manual, or its website claims a capability the evidence
   register does not support.

Any item you cannot achieve becomes a stated, visible limitation — not a silent omission and not a softened
claim.

## 4. Verified starting state

Recorded from the repository on 2026-09-01. Re-verify before you rely on it; the tree had uncommitted
modifications and recent unqualified commits at the time of writing.

**Proven:**

- Deterministic `EchoesSimCore` passes 37/37 native tests in optimized, debug, and ASan+UBSan configurations
  under `-Wall -Wextra -Wpedantic -Werror`.
- `Scripts/test_content.sh` passes 7 suites / 123 tests.
- Mac Development Editor and Game both compile with zero errors on merged `main`.
- The Unreal automation suite ran 56 tests with 55 passing; the Mission 08 route failure was repaired in
  `311e4cd` and a Mission 12 district-site repair landed in `f98f26e`. **Neither commit has a recorded
  post-fix full-suite run. Re-run the suite before trusting the count.**
- Fifteen campaign operations, fifteen ordered consequence records, a continuous journey controller,
  transactional New Campaign and Restore with one prior generation, and prerequisite-bound Mission 02–09
  checkpoint containers exist and pass headless controller/simulation automation.
- Three factions are source-reachable in skirmish with complete slice rosters and two technologies each.
- A Mac arm64 Development package exists, bound to a clean commit, ad-hoc signed, with the
  `M_EchoesWorldSurface` instancing fallback closed under packaged Metal.

**Not proven, and standing in the way of the demo:**

- No ordinary-player rendered traversal of the campaign or of a full skirmish. All campaign evidence is
  headless.
- Only one of the routes through Missions 09–15 has complete end-to-end evidence; three of the four endings
  have plan and reducer coverage only.
- Packaged capture shows severe blown highlights, a dense debug-like control overlay, and prototype-state
  text.
- Fog, atmosphere, health bars, ownership markers, minimap, and most HUD presentation are code-authored or
  Engine-provided prototypes.
- No production textures, no character or machine animation, no Niagara effects, no transparent dissolve.
- Audio consists of exactly four synthesized cues: one command confirmation and three faction destruction
  sounds. No music, ambience, interface audio, alerts, weapon audio, or mix architecture exists.
- The earlier packaged profile exceeded the GPU and render-thread budgets and failed the memory-slope
  diagnostic; the later package has never been profiled.
- No Developer ID signature, no notarization, no installer, no clean-machine result.
- Skirmish setup UI, AI personalities, remapping, subtitles, separate audio buses, autosave, save slots, and
  the complete accessibility menu are unimplemented.

## 5. Delivery tracks

Eight tracks. Each states its goal, what must be built, and the evidence that closes it. Work them in the
dependency order given in section 9, not in the order they are listed.

---

### Track A — Visual completion

**Goal:** every rendered element in the demo is a registered authored asset, correctly lit, readable under
combat load, and free of development scaffolding.

**A1 — Exposure, tonemapping, and lighting.** *Blocks all other visual acceptance.* The packaged capture's
blown highlights make every downstream art judgment worthless. Author an explicit post-process configuration:
fixed or tightly bounded auto-exposure, a tonemapper setting chosen against the charcoal / pale-ceramic /
broken-sun amber / magenta-fracture / cyan-Matter palette, and a lighting rig for the Glass Scar and each
campaign site. Nanite and Virtual Shadow Maps stay off. Evidence: packaged Metal captures at 1920×1080 and
2560×1440 showing no clipped highlight regions on ceramic and glass surfaces, with the same palette readable
in high-contrast mode.

**A2 — Debug overlay and prototype text removal.** Audit every string and widget drawn in a shipping
configuration. Development diagnostics move behind a build flag or a developer key that is absent from the
packaged build. Placeholder labels required by `AssetRegister.md` disappear only when the asset they label is
replaced by a registered final one — not before. Evidence: an ordinary packaged capture and a stress capture
containing no diagnostic overlay, plus a source audit listing every remaining developer-only surface and its
gate.

**A3 — Production textures and materials.** Extend `generate_art_assets.py` to author `Texture2D` assets
deterministically from code — base color, roughness, metallic, normal, and emissive masks — and drive the
existing `M_EchoesSurface`, `M_EchoesWorldSurface`, and route material families through material instances
rather than flat parameter colors. Preserve the existing UV0 tiling / UV1 lightmap contract and the four
per-route material zones already established for the Ash Cut, Buried Causeway, and Folded Verge. Evidence: a
regeneration run proving byte-idempotence under the new revision strings, an isolated Metal composition
review per family, and an `AssetRegister.md` entry per family.

**A4 — Environment completion.** The Glass Scar dressing exists as authored shelves, ridges, shards, three
crossings, Matter deposits, and a shared world material. Extend the same pipeline to every site the fifteen
operations use — Lume Reach, the ark-city districts, the migration route, the census and Listening-Spine
sites, the Crownfall approach and accord sites — so no campaign mission renders on undressed terrain.
Environment actors stay presentation-only and non-colliding; deterministic terrain remains the sole route
authority. Evidence: one isolated Metal composition capture per site, plus a rendered pass through each
mission confirming no bare collision floor is visible.

**A5 — Motion.** The demo does not ship skeletal rigs. Author code-driven component motion instead, since it
composes with the existing static-mesh candidates and stays deterministic: tread and leg cycles keyed to
authoritative velocity, hover bob, turret and weapon tracking toward the authoritative target, recoil on
fire, construction assembly reveal, Carapace molt and Striker molt transitions, Waystone uproot and root,
Bulwark deploy and pack, and the four Future Well state silhouettes already authored. Motion reads from
authoritative state and writes nothing back. Reduced motion holds transforms steady. Evidence: a rendered
capture per motion family, plus a test asserting that no motion component mutates simulation state or enters
a checksum.

**A6 — Effects.** Weapon muzzle and impact, projectile trails, Matter gather and deliver, construction dust,
production emergence, research completion, Future Well activation and mode transition, Reshape terrain
opening and closing, destruction debris and smoke, and dissolve on removal. Prefer the existing authored
mesh-VFX approach where it reads well; introduce Niagara only where mesh VFX cannot carry the effect, and
record the performance cost of every Niagara system you add. Every effect disables collision, overlap,
navigation influence, and shadows, and respects reduced motion and reduced flashing. Evidence: an isolated
review capture per effect family and a stress capture showing the combat-load frame cost of all of them
active.

**A7 — Fog and shroud.** Replace the placeholder fog with production shroud presentation over the unchanged
fair visibility authority: unexplored, explored-but-not-currently-visible, and currently visible must be
distinguishable without relying on color alone, and explored terrain must persist. The presentation may never
reveal an entity the simulation says is hidden. Evidence: a rendered scouting sequence plus the existing fog
authority tests still passing, and a frame-cost measurement against the 1.5 ms fog budget at 400 units and
four teams.

**A8 — Interface art.** One coherent visual system across title, operation and faction selection, mission
brief, live HUD, command deck, tactical minimap, technology archive, field menu, options, result, and
credits. Bracketed rectangular Meridian marks, paired faceted Kharuun marks, offset concentric Choir marks
stay non-color-coded. Everything scales with the HUD scale setting and has a high-contrast variant. Evidence:
rendered captures of every screen at 1280×720, 1440×900, 1600×900, 1920×1080, and 2560×1440, in standard and
high contrast, with no clipping or occlusion.

**A9 — Interludes.** The Bible calls for cinematics; the demo ships the honest reduced form: in-engine camera
moves over authored sites with on-screen text, plus act-transition cards generated by the art pipeline. No
pre-rendered video, no motion-captured performance. Evidence: one rendered interlude per act transition and
per ending, skippable, with subtitles.

---

### Track B — Audio completion

**Goal:** every event the design says has a sound, has an original registered one; music and ambience run
under the whole game; the mix is qualified.

**B1 — Mix architecture.** *Blocks every other audio acceptance.* Build the submix graph the Bible requires:
music, dialogue, interface, ambience, and effects as separate categories with independent player-facing
volume controls in the options menu. Reduced dynamic range operates across the graph rather than on the
effects bus alone. Rate limiting and the existing throttles move into this architecture. Evidence: a test
asserting each category's volume control changes measured output for that category and no other, plus the
existing effects-volume and reduced-dynamic-range tests still passing.

**B2 — Music.** Deterministically synthesized in Python, exported as PCM, imported as `SoundWave` assets, the
same path `generate_audio_assets.py` already uses. Compose to the Bible's direction: Compact uses measured
pulse, prepared piano, restrained brass, and mechanical resonance; Kharuun uses interlocking rhythms and
resonant stone or ceramic timbres without generic tribal coding; Choir harmony resolves in more than one
direction before committing. Required cues: title theme; one theme per faction; one theme per campaign act;
a tension layer and a combat layer that transition without a hard cut; victory, defeat, and ending stingers.
Evidence: a provenance record proving synthesis from project code, a loudness and peak measurement per cue,
and a rendered session capture showing correct state transitions.

**B3 — Ambience.** Beds for the Glass Scar, Lume Reach, the ark-city districts, and Crownfall — wind across
vitrified glass, structural resonance, distant settlement, Well hum keyed to the active mode. Ambience is
positional where the design implies a source and non-positional where it is a bed. Evidence: a rendered
capture per site and a measurement that ambience does not mask combat cues at default levels.

**B4 — Gameplay audio.** Weapon fire and impact per unit archetype and faction; Matter gather and deliver;
construction start, progress, and completion; production start and unit emergence; research start,
completion, and no-refund interruption; Future Well claim and mode transition; Reshape open and close;
structure and unit destruction, which already exists and should be extended to the Choir; and unit
acknowledgement that is textural rather than spoken. Evidence: a coverage test enumerating every authoritative
gameplay event against its registered cue, with no unmapped event.

**B5 — Interface audio and alerts.** Hover, select, confirm, and a distinct rejection cue paired with the
stable failure reason the UI already returns; menu open and close; brief and result transitions; and brief,
rate-limited alerts for attack, structure loss, production complete, research complete, and low capacity,
routed to the `Space` jump-to-alert behavior. Evidence: a rate-limit test and a rendered capture of each
alert under a simultaneous-event burst.

**B6 — Mix qualification.** Set and verify a loudness target and a true-peak ceiling for the packaged build.
Then run the readability case that matters: a simultaneous combat load in which the player can still identify
an alert, a destruction cue, and a rejection. Evidence: measured loudness and peak for an ordinary session,
plus a stress session capture with the identification result stated explicitly as a bounded observation, not
as a claim of broad mixing quality.

---

### Track C — Campaign playability

**Goal:** an ordinary person plays all fifteen operations from an empty ledger, with a mouse and a keyboard,
and reaches an ending.

**C1 — Close the open failures.** Re-run the full automation suite on current `main` with the correct
`TMPDIR`. Confirm the state of `Echoes.Runtime.Campaign.FreshJourney` after `311e4cd` and `f98f26e`, and the
two Campaign plan-17 focused paths that failed at integration head `5cb039b`. Repair by fixing the cause —
never by widening a timeout or lowering a gate. Route defects follow the Mission 08 precedent: read
passability with `Simulation::IsPositionPassable` rather than guessing, and remember that Reshape can open and
then close a tile mid-run.

**C2 — Complete route and ending coverage.** Missions 10 through 15 each carry 27 plans; only the
Harvest-founding / Life-Support-plus-Transit / Preserve-Lume route has complete downstream evidence, and only
Controlled Stabilization and the two `FreshJourney` routes are demonstrated end to end. Extend automated
coverage until every ending type has at least one complete route, and until every plan that a demo player can
reach completes rather than dead-ends. Evidence: one recorded complete route per ending type, and a matrix of
reachable plans against their outcomes.

**C3 — Rendered human traversal.** The acceptance that actually matters. A person plays the campaign in the
packaged build with physical input. Record the session, the wall-clock time per mission, every point at which
the player was stuck or confused, and every defect. This is the gate that converts headless evidence into a
playable campaign; it cannot be satisfied by automation, by a controller route, or by Angelis alone, since
he knows the intended solutions.

**C4 — Dialogue, briefs, and subtitles.** Fifteen operations need authored mission text: situation brief,
live objectives, mid-mission beats, and results that report concrete consequences and unresolved costs rather
than a hidden score. Follow the Bible's writing rules and its explicit bounds — the prototype must not claim
authorship, consent, trust, civilian survival, or cryptographic authenticity that the simulation does not
model. Text is source-authored under `Content/Narrative/Source` against its schema, compiled, and consumed at
runtime; runtime consumption is currently false and must become true. Subtitles carry size and background
controls. Evidence: schema validation, a rendered pass showing every string in place, and a check that no
authored line asserts an unmodeled consequence.

**C5 — Saves, slots, autosave, and recovery.** The demo needs named multiple slots, autosave at mission and
phase boundaries, and an interrupted-session recovery flow a player can understand — not the current
active-plus-one-prior-generation model alone. Keep the fail-closed container binding and CRC integrity, and
keep the honest limitation that CRC is not authentication. Evidence: slot create, load, overwrite, and delete
tests; an autosave test; a recovery test from a killed process; and the existing migration tests still
passing.

---

### Track D — Skirmish and opponent AI

**Goal:** the player-versus-AI experience the demo is named for is actually configurable and worth replaying.

**D1 — Skirmish setup.** Expose map, faction, teams, AI personality, difficulty, starting resources, victory
conditions, and game speed, as the Bible specifies. The current build exposes only a faction cycle. Any
assisted difficulty must name its exact modifier before the match and in any replay metadata; standard
difficulty keeps using only player-visible information and no hidden income. Evidence: a test per exposed
option proving it changes match setup, and a rendered pass through the setup screen.

**D2 — AI personalities.** Implement Defensive, Expansionist, Raider, Economic, and Adaptive as distinct
command producers over the existing scoped player view. The current Adaptive implementation is the reference:
no simulation or world handle, hidden entities omitted, non-owned internals redacted, anonymous contacts
carrying no entity ID. Evidence: a deterministic behavioral test per personality demonstrating the distinction
its description promises, and a note stating plainly that this is bounded behavior, not human-equivalent play.

**D3 — Balance pass.** Nine faction matchups across the three factions. This is iterative tuning against
`Content/Data/Source`, not a one-time change. Evidence: recorded outcomes across a defined number of matches
per matchup at standard difficulty, with the win-rate spread stated and the remaining imbalance named rather
than hidden.

---

### Track E — Player experience and accessibility

**Goal:** the settings menu is real, the layout survives the resolutions people actually use, and a person
with a mouse can play without a keyboard reference card.

**E1 — Controls.** Full remapping for every command in the Bible's command set, with conflict detection and
a reset. Invalid actions keep returning a stable reason with visible feedback.

**E2 — Accessibility menu.** UI scale, subtitle size and background, keyboard-operable menus throughout,
color-vision-safe palettes, non-color ownership markers, reduced shake and flashing, adjustable camera motion,
separate audio category volumes, reduced dynamic range, offline pause, tutorial replay, searchable technology
information, destructive-choice confirmation, autosave, and interrupted-session recovery. A setting that does
not change behavior does not pass — test each one for its effect, not its presence.

**E3 — Layout coverage.** The current pointer and HUD evidence covers seven fixed configurations across four
resolutions and two aspect ratios. Extend to arbitrary and live-resized windows, fullscreen and windowed, and
the M1 Pro's native panel. Evidence: automated projected-bounds checks plus rendered captures at the
boundaries.

**E4 — Unaided human usability.** People who are not Angelis play both a skirmish and at least one campaign
mission, unaided, with their sessions recorded. Every point of confusion becomes a defect or a documented
limitation. This closes the gap the ledger has flagged since version 0.66.0.

---

### Track F — Performance and stability

**Goal:** the packaged build holds its budgets and survives a long session.

Re-profile the current package; the earlier one exceeded GPU and render-thread budgets and failed the memory
slope diagnostic, and the later one has never been profiled. Track A adds cost to exactly the systems that
were already over budget, so profile after each visual track lands, not once at the end. Budgets are in
`ProjectLedger.md` and repeated in section 9 below. Close with a valid 600-active-second preflight followed
by one uninterrupted 60-minute run of the same package, using the existing sustained fixture and its
fail-closed wrapper. Evidence: the profile artifacts and the soak log, published atomically as the wrapper
requires.

---

### Track G — Packaging and distribution

**Goal:** a stranger can install and run it.

Produce a Shipping configuration package, not Development. Sign with a Developer ID, notarize, staple, and
build a DMG or installer. Verify the whole chain on a clean Apple Silicon machine that has never had Unreal,
Xcode, or this project on it: install, launch, play a skirmish, quit, relaunch, and load a save. Confirm the
build runs with the Seagate volume absent, since the development checkout lives there. Evidence:
`spctl`/`stapler` verification output, the clean-machine session record, and a first-launch time measurement.

---

### Track H — Documentation, rights, and public front door

**Goal:** what the game says about itself is true.

Update `ProjectLedger.md`'s evidence register and known limitations as each gate closes; add every new asset
family to `AssetRegister.md` with its generation record; write the player manual and controls reference; and
update the `site/` archive and any store or download text. Every public claim must trace to a passing gate.
Ship a visible, accurate known-limitations page with the demo — multiplayer absent, no voice acting, bounded
AI, the balance spread as measured. Confirm the rights position: all assets project-generated, no third-party
license obligations, typeface licensing checked before shipping any font.

## 6. Procedural asset pipeline rules

Both generators are editor-time Python driven through their shell wrappers. Extend them; do not build a
parallel path.

- Every asset family carries a revision string. Changing generation logic means a new revision, a
  regeneration run, and an `AssetRegister.md` entry. Regeneration under an unchanged revision must produce
  byte-identical output — prove it, do not assume it.
- Generated assets are ordinary `StaticMesh`, `Material`, `MaterialInstance`, `Texture2D`, and `SoundWave`
  assets. Geometry Scripting, Python, and the audio synthesizer are editor-time dependencies and must never
  become runtime dependencies.
- Meshes ship with authored LOD0 and LOD1, UV0 for surface tiling, UV1 reserved for lightmaps, explicit
  material zones, and simple collision data retained for asset inspection — while the spawned presentation
  component disables collision, overlap, and navigation influence.
- Audio sources are mono or stereo 48 kHz PCM synthesized from project code, written to
  `Content/Audio/Source`, and imported to `/Game/Audio/Generated`. Keep the WAV sources in version control
  under LFS so provenance survives.
- If a required asset genuinely cannot be produced procedurally at acceptable quality, stop and record the
  problem in section 8 as an open decision. Do not quietly import something from elsewhere.

## 7. Execution model

**Choosing a process.** Judge per task. If the work touches files another lane holds in
`../WorkstreamControl/ACTIVE_LANES.md`, take a lease there first and record the candidate in your own file
under `handoffs/`. If it is isolated, a branch and a ledger entry are enough. Say which you chose and why in
your report. Never write to coordinator-owned files without saying so.

**Git.** Branch per track — `demo/<track>-<topic>`. Stage explicit paths only; the working tree carries
unrelated modifications and gigabytes of build artifacts. Merge to `main` when the track's acceptance
evidence exists. Push to `origin` from the Mac shell so GitHub stays current, including LFS objects for asset
work. Never rewrite `main` history and never force-push.

**Session loop.** Read this directive and the ledger's current limitations; pick the next unblocked gate;
build the smallest testable slice; run the test; record the evidence; commit; push; report. Update this
directive's gate matrix in the same commit as the work it describes.

**Reporting.** Every consequential result is reported as one of: verified complete, prepared but not executed,
executed but outcome not confirmed, partially complete, blocked, or unknown. State what you ran, on what
commit, and what the result does not establish. A passing test supports only what that test exercised. Your
own review of your own work is internal QA, never independent validation.

**Blockers.** If a gate cannot pass without a decision only Angelis can make, do the preparatory work, write
the decision into section 8 with the options and their costs, and stop there.

## 8. Open decisions

Add to this list rather than guessing. Each entry needs the question, the options, and what each costs.

1. **Voice acting.** Locked out of the demo — dialogue ships as text with subtitles. If Angelis later wants
   spoken performance, it needs either recorded human performance with releases and a rights record, or
   synthesized speech with a service, terms, and provenance entry. Neither is authorized now. The Choir's
   non-linguistic vocal texture may be synthesized under the existing audio pipeline; confirm before
   building it.
2. **Niagara adoption.** Track A6 permits Niagara only where mesh VFX cannot carry an effect. Niagara systems
   are not producible by the existing Python generator in the same deterministic way, so each one is a
   provenance and performance exception that needs an explicit decision and an `AssetRegister.md` entry.
3. **Typeface.** The interface currently uses whatever the Engine provides. A shipped demo needs a licensed
   or original typeface with its license recorded. Unresolved.
4. **Demo length and content gating.** A public demo containing all fifteen operations is the full campaign.
   Confirm this is the intent rather than a subset, and confirm what, if anything, is held back for a later
   full release.
5. **Difficulty options.** The Bible allows assisted levels provided the exact modifier is labeled before the
   match. Confirm whether the demo ships more than standard difficulty.

## 9. Gate matrix

Every gate is NOT RUN until a session runs it and records the evidence here with its commit SHA and date.
Dependencies are hard: do not accept a gate whose prerequisite has not passed.

| # | Gate | Track | Depends on | State |
|---|---|---|---|---|
| 1 | Full automation suite green on current `main` with correct `TMPDIR` | C1 | — | NOT RUN |
| 2 | Native sim 37/37 in three configurations on current `main` | C1 | — | NOT RUN |
| 3 | Content suites 123/123 on current `main` | C1 | — | NOT RUN |
| 4 | Campaign plan-17 focused paths pass | C1 | 1 | NOT RUN |
| 5 | Exposure and lighting corrected; no clipped highlights in packaged capture | A1 | 1 | NOT RUN |
| 6 | No debug overlay or prototype text in a shipping-configuration capture | A2 | 5 | NOT RUN |
| 7 | Production textures and material instances registered and byte-idempotent | A3 | 5 | NOT RUN |
| 8 | Every campaign site dressed with authored environment assets | A4 | 7 | NOT RUN |
| 9 | Motion families implemented, presentation-only, reduced-motion compliant | A5 | 7 | NOT RUN |
| 10 | Effect families implemented with recorded combat-load frame cost | A6 | 9 | NOT RUN |
| 11 | Production fog and shroud within the 1.5 ms budget | A7 | 5 | NOT RUN |
| 12 | Interface art complete at five resolutions, standard and high contrast | A8 | 5 | NOT RUN |
| 13 | Act and ending interludes rendered and skippable with subtitles | A9 | 8, 12 | NOT RUN |
| 14 | Submix graph with five independent category volumes | B1 | — | NOT RUN |
| 15 | Music set registered with loudness and peak measured | B2 | 14 | NOT RUN |
| 16 | Ambience beds per site, not masking combat cues | B3 | 14 | NOT RUN |
| 17 | Gameplay audio coverage — no unmapped authoritative event | B4 | 14 | NOT RUN |
| 18 | Interface audio and rate-limited alerts | B5 | 14 | NOT RUN |
| 19 | Mix qualified: loudness, true peak, simultaneous-combat readability | B6 | 15, 16, 17, 18 | NOT RUN |
| 20 | One complete route recorded per ending type | C2 | 4 | NOT RUN |
| 21 | Mission text authored, schema-valid, and consumed at runtime | C4 | 4 | NOT RUN |
| 22 | Named slots, autosave, and recovery flow tested | C5 | 4 | NOT RUN |
| 23 | Skirmish setup exposes every option with a test per option | D1 | — | NOT RUN |
| 24 | Five AI personalities behaviorally distinguished | D2 | 23 | NOT RUN |
| 25 | Balance pass across nine matchups with the spread stated | D3 | 24 | NOT RUN |
| 26 | Full control remapping with conflict detection | E1 | — | NOT RUN |
| 27 | Every accessibility setting verified to change behavior | E2 | 12, 14 | NOT RUN |
| 28 | Arbitrary and live-resize layout coverage | E3 | 12 | NOT RUN |
| 29 | Unaided human skirmish and mission sessions recorded | E4 | 12, 19, 23 | NOT RUN |
| 30 | Ordinary-player rendered campaign traversal to an ending | C3 | 13, 19, 21, 22, 27 | NOT RUN |
| 31 | Packaged profile within budget: 16.67 ms frame, ≤4.0 ms game thread, ≤11.0 ms render+GPU, ≤1.5 ms fog, ≤6.0 ms path burst, ≤10 GB resident, ≤250 ms save | F | 10, 11, 19 | NOT RUN |
| 32 | 600-second preflight plus uninterrupted 60-minute sustained run | F | 31 | NOT RUN |
| 33 | Shipping-configuration package builds and launches | G | 30, 32 | NOT RUN |
| 34 | Developer ID signed, notarized, stapled; DMG or installer built | G | 33 | NOT RUN |
| 35 | Clean-machine install, launch, play, quit, relaunch, load | G | 34 | NOT RUN |
| 36 | Ledger, asset register, manual, and site updated; every public claim traced to a passing gate | H | 35 | NOT RUN |
| 37 | Known-limitations page shipped with the build and accurate | H | 36 | NOT RUN |

Gates 1–4 come first and unblock nearly everything. Gates 5 and 14 are the two roots of the visual and audio
tracks and should start in parallel immediately after. Gate 30 is the demo's real acceptance; gate 35 is its
distribution acceptance.

## 10. What this directive is not

It is not evidence that any of this work has been done. It is not a schedule, and it does not estimate effort.
It does not authorize a claim of release readiness, balance, usability, or performance in advance of the run
that establishes it. When the demo ships, the honest description of it is whatever the closed gates support —
no more.
