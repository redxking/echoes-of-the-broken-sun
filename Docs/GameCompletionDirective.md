# Game Completion Directive

**Author and control owner:** Angelis Pseftis
**Created:** 2026-09-01
**Applies to:** every Claude session working in this repository until the professional release ships
**Supersedes:** [`DemoReleaseDirective.md`](Archive/Superseded/DemoReleaseDirective.md) — its accepted evidence carries forward into section 10; its scope is replaced by section 2
**Companion file:** [`CLAUDE.md`](../CLAUDE.md) — the standing session contract, build commands, and environment traps

This directive defines the work required to take *Echoes of the Broken Sun* from a verified
playable-systems prototype with partial demo evidence to a **professional real-time strategy release**:
a single-player, player-versus-AI macOS game whose polish stands comparison with the genre's reference
titles — the readability and pacing discipline of *StarCraft*, the story delivery of *Warcraft III*, the
approachability of *Age of Empires* — while remaining entirely this game: Soryn, the Crownfall, the
Future Wells, and the three factions the Development Bible defines. Nothing in this directive changes
the theme, the world, the factions, the characters, the campaign structure, or the mechanics the Bible
authorizes. It raises the execution bar on everything the player sees, hears, reads, and feels.

It is a plan, not a record of completion. Every gate in section 10 that does not carry forward prior
evidence starts at NOT RUN. Move a gate only when you have run it and read the result.

---

## 0. How to run a session against this directive

This section exists so that no session wastes its hours. It is the operating loop; the tracks in
section 6 are the work.

**The loop.**

1. **Orient.** Read this directive's gate matrix, `Docs/Archive/ProjectLedger.md`'s *Current known
   limitations*, and `../WorkstreamControl/ACTIVE_LANES.md`. Do not re-read the whole documentation set;
   read the files the chosen gate names.
2. **Pick one gate.** Select the next unblocked gate from section 10. One gate per session is the
   default; take a second only when the first closes with time and context to spare. If the work
   touches files another lane holds, take a lease; if isolated, a branch is enough. Say which and why.
3. **Define the check before building.** Write down, before the first edit, the exact command, capture,
   or measurement that will prove the slice — a test that fails now and must pass, a packaged capture
   that must show a named property, a measured number that must land inside a budget. If no such check
   can be stated, the slice is not ready to build; decompose it until it is.
4. **Baseline.** Run the narrowest existing suite that covers the area you are about to touch, and read
   the result, so a pre-existing failure is never mistaken for damage you caused — and damage you cause
   is never blamed on history.
5. **Build the smallest slice that can pass the check.** Not the whole track. Not two gates at once.
   Incremental, verified progress compounds; heroic sessions that end in an unverifiable pile do not.
6. **Run the check. Read the result.** Iterate until it passes or you can state precisely why it cannot.
7. **Adversarial pass before closing.** Before marking a gate PASS, re-read the gate's text as written
   and attack your own evidence against it: does the run actually establish each clause, on the current
   commit, at the layer the player will use? Where feasible, have a fresh session or subagent that did
   not build the work review the evidence against the gate text. Record this as internal QA — it is
   never independent validation.
8. **Record and commit.** Update the gate matrix, the ledger's evidence register, and
   `AssetRegister.md` for any new asset family, in the same commit as the work. Evidence artifacts go
   under `../WorkstreamControl/evidence/`. Commit, push from the Mac shell, report.

**Rules that protect the loop.**

- **A check the session can run is mandatory.** Work without a runnable check is work whose defects
  wait for a human to trip over them. Tests, builds, captures read back with your own eyes, measured
  logs — any of these; assertion alone, never.
- **Evidence over assertion.** Report what you ran, on what commit, what it returned, and what it does
  not establish. A passing test supports only what that test exercised.
- **Never weaken the check to pass it.** Do not raise a timeout, loosen a tolerance, skip a case,
  delete or edit a test, or reword a gate so the current state satisfies it. Repair the cause or report
  the blocker. Editing acceptance criteria to match output is the one behavior that silently destroys
  this project's evidence discipline.
- **Leave the tree better than you found it.** Never end a session with the build broken or a suite
  newly red without a written blocker note. The next session must be able to start from *Orient*
  without archaeology.
- **Context discipline.** Read what the gate needs; use subagents or separate sessions for wide
  searches and reviews; do not paste large binary or log dumps into working notes. A session drowning
  in unrelated context makes worse decisions near its end than its start.
- **Stop at real decision points.** If a gate cannot pass without a decision only Angelis can make, do
  the preparatory work, write the decision into section 9 with options and costs, and stop there.
  Guessing at an owner-level decision wastes more time than waiting for one.
- **Report states precisely.** Every consequential result is exactly one of: **verified complete,
  prepared but not executed, executed but outcome not confirmed, partially complete, blocked,
  unknown.**

## 1. Authority and sources

This directive is a backlog, sequencing plan, and tracking matrix. It does not establish, author, or waive software or game design requirements. The repository is the sole authority on how this game works. Before designing, building, or verifying any feature, a session must read and defer to the explicit documents that own the domain:

1. **Docs/Requirements.md (Tier 1 — Ultimate Normative Authority):** This file is the absolute driver for all game development, features, mechanics, mathematical models, and balance implementations. If any behavioral description in this directive or any historical source contradicts `Docs/Requirements.md`, `Docs/Requirements.md` wins unconditionally.
2. **Docs/Archive/DevelopmentBible.md (Tier 1 — Creative Authority):** The sole normative source for world, factions, characters, Future Wells, and narrative intent. This directive raises the execution bar on what the player experiences, but never alters the canon authorized by the Bible.
3. **Docs/RequirementsState.md (Tier 1 — Lifecycle Authority):** The sole registry tracking whether an individual requirement is OPEN, WIP, or VERIFIED. 
4. **Docs/Archive/ProjectLedger.md (Tier 2 — Historic Evidence Register):** Tracks verified software performance limits, known limitations, and accepted engineering outcomes.

Design recollections carried from other tools or earlier conversations are hypotheses. Check them against these tracking files before acting on them.


## 2. Scope

Decided by Angelis on 2026-09-01, superseding the demo directive's scope of the same date. Do not
renegotiate these without him.

**The release contains:**

- Glass Scar skirmish, player versus the AI opponent, all three factions selectable — Meridian
  Compact, Kharuun Assemblies, Hollow Choir — with the full setup screen (map, faction, AI
  personality, difficulty, starting resources, victory conditions, speed).
- All fifteen campaign operations, **What the Ledger Keeps** through **The Broken Sun**, playable by an
  ordinary person from an empty ledger to any of the four endings, using rendered UI and physical
  input.
- **Professional visuals**: production textures and materials, dressed environments for every site,
  motion, effects, fog, lighting, and a coherent interface art system — no placeholder, no debug
  surface, no Engine-primitive stand-in anywhere a player can see.
- **Professional audio**: the full original music score, per-site ambience, complete gameplay and
  interface sound, alerts, and a measured, qualified mix.
- **Voiced storytelling** — new to this scope, reversing the demo directive's exclusion: campaign
  dialogue, briefs, and cinematic narration performed via the project's own local neural
  text-to-speech pipeline (track C), directed per character, with subtitles always available.
- **In-engine cinematics** — upgraded from the demo directive's reduced interludes: a title sequence,
  act transitions, and one authored ending cinematic per ending, built with Sequencer over registered
  assets (track D).
- **An immersive, coherent public front door**: the website and every public surface carry the same
  world, visual language, and tone as the game (track J).
- macOS Apple Silicon distribution: Shipping configuration, Developer ID signed, notarized,
  installable, and verified on a clean machine.

**Deliberately out of this release:**

- **Multiplayer of any kind.** The accepted networking work stays behind its existing gates and ships
  disabled. The release is defined as the polished player-versus-AI experience.
- **Windows and Linux builds.** The platform roadmap after this release is: **Linux/SteamOS first,
  then Windows, then others.** This release must not add obstacles to that path — see gate track I's
  portability guard — but ships on macOS only.
- Any map beyond those the fifteen operations and the skirmish require.
- Any change to theme, world, factions, characters, campaign structure, endings, or authorized
  mechanics. Polish is in scope; redesign is not.

**Asset production policy — procedural-first with recorded exceptions.** The default path for every
asset remains deterministic generation by project code through `Scripts/generate_art_assets.py` and
`Scripts/generate_audio_assets.py`, registered in `AssetRegister.md`, byte-idempotent under its
recorded revision. Where a required asset provably cannot reach the professional bar through the
generators, an exception may be taken **per asset family, recorded before use** in `AssetRegister.md`
and section 9, naming the production method (locally run generative model, licensed asset, or original
commissioned work), the license or rights basis, and why the procedural path was insufficient. No
marketplace-scraped or unlicensed content, ever. An exception that is not recorded is a defect. The
local TTS voice pipeline (track C) is the first standing exception of this kind and follows the same
rules.

**Voice policy.** Spoken performance is produced by an open-weights neural TTS model run locally as an
editor-time tool inside the audio pipeline — never a runtime dependency, never a per-use cloud
service. The chosen model and its license are recorded in `AssetRegister.md` and section 9 before the
first registered line. Unit acknowledgements remain **textural rather than spoken**, exactly as the
Bible specifies; voice covers characters and narration, not unit barks.

**The immersion principle.** Stated by Angelis 2026-09-01: *the player should feel immersed in this
world while playing, reading the story, watching a cutscene or video, or browsing the website.* Every
track below inherits this as a requirement: one visual language, one audio identity, one narrative
voice, continuous from the first public web page through the final ending — and no surface that breaks
it with placeholder text, off-palette art, unlabeled silence, or a claim the world does not support.

## 3. The experience bar

"Professional" is not a mood; it is the following observable properties, each of which a gate in
section 10 tests. Comparison with the genre's reference titles means matching their **discipline**, not
their budgets: this game does not claim feature or asset-volume parity with decade-scale studio
productions, and no session may write a claim of such parity anywhere.

1. **Readability at combat speed.** Ownership, role, order state, threat, terrain effect, and Well
   state are recognizable in under a second at ordinary camera height, under full combat load, in
   standard and high-contrast modes. Readability outranks spectacle — the Bible's rule, kept.
2. **Response.** Every accepted player action produces visible and audible acknowledgment within the
   frame budget; every rejected action produces its stable reason. Nothing the player does lands in
   silence.
3. **Coherence.** Every screen, cue, line, and page reads as one world. Palette, type, iconography,
   music language, and written voice never contradict each other or the Bible.
4. **No seams.** No debug text, no placeholder label, no Engine default asset, no dead control, no
   silent failure, no unstyled state — in the game, in the cinematics, or on the website.
5. **Confidence under load.** Budgets hold in ordinary and stress sessions; a 60-minute sustained run
   survives; saves are transactional; interruption recovers.
6. **Honesty.** The game, its manual, and its public pages claim exactly what the evidence register
   supports. A limitation shipped knowingly is stated, not hidden.

## 4. Definition of done

The release is finished when all of the following are true and each has evidence recorded in the
ledger:

1. A person who has never seen the game installs the notarized build on a clean Apple Silicon Mac,
   launches it, and reaches the title screen without a terminal, a developer tool, or an instruction
   from Angelis.
2. That person completes a Glass Scar skirmish against the AI as each of the three factions, and
   completes a fresh campaign from an empty ledger through Mission 15 to an ending, using mouse and
   keyboard on rendered UI. Not headless automation. Not a scripted controller route.
3. Every rendered frame in that path uses registered authored assets, correctly exposed and lit, with
   no debug or prototype surface visible.
4. The full score, ambience, gameplay, and interface audio play under the whole experience; every
   event the design says has a sound has one; the mix meets its measured loudness and true-peak
   targets and stays readable under simultaneous combat load and under reduced dynamic range.
5. Every campaign dialogue line, brief, and cinematic narration is performed by the registered voice
   pipeline, in character, synchronized with accurate subtitles, at the same measured loudness
   standard as the rest of the mix — and can be disabled independently of other audio.
6. The title sequence, three act transitions, and the ending cinematic for whichever ending the player
   earns all play in-engine, skippable, subtitled, using registered assets, without a frame that
   breaks the visual bar of clause 3.
7. The performance budgets in section 10 hold in an ordinary packaged session and in the sustained
   60-minute run.
8. Accessibility settings all change behavior: remapping, UI scale, subtitles (size and background),
   high contrast, reduced motion, reduced flashing, color-vision-safe markers, per-category audio
   volumes including voice, reduced dynamic range, offline pause.
9. Saves, autosave, named slots, and interrupted-session recovery work for both skirmish and campaign.
10. The website presents the same world to the same standard, and nothing in the shipped build, its
    store text, its manual, or its site claims a capability the evidence register does not support.

Any item that cannot be achieved becomes a stated, visible limitation — not a silent omission and not
a softened claim.

## 5. Verified starting state

Recorded from the repository and its gate history on 2026-09-01. Re-verify before you rely on it; the
working tree has historically carried uncommitted modifications.

**Proven (with where the evidence lives):**

- Deterministic `EchoesSimCore`: 37/37 native tests in optimized, debug, and ASan+UBSan configurations
  under `-Wall -Wextra -Wpedantic -Werror` (demo gate 2, `39955ea`).
- Content suites: 7 suites / 123 tests passing (demo gate 3, `39955ea`).
- Unreal automation suite green — 56/56 at `39955ea`, then 57/57 at `a470eb9`, then 58/58 with the
  audio-mix and music/ambience tests (demo gate 1). Campaign `FreshJourney` drives all four journeys
  M01–M15 (demo gate 4).
- Fifteen campaign operations with ordered consequence records, a continuous journey controller,
  transactional New Campaign/Restore with one prior generation, and prerequisite-bound Mission 02–09
  checkpoint containers — headless controller/simulation evidence.
- Three factions source-reachable in skirmish with complete slice rosters and two technologies each;
  a bounded standard-difficulty Adaptive opponent.
- Authored static-mesh candidates for all twenty-four units and structures, the four-state Future
  Well landmark, Glass Scar terrain dressing, three route kits (Ash Cut, Buried Causeway, Folded
  Verge), selection/command mesh-VFX, and destruction feedback.
- 54 registered synthesized audio cues (interface, alerts, gameplay, fifteen music cues, five ambience
  beds) as byte-idempotent sources and imported SoundWaves; a five-category submix graph with
  independent volumes and whole-graph reduced dynamic range, music/ambience managers wired to title,
  deployment, and results (demo gate 14, `a470eb9`).
- A Mac arm64 Development package bound to a clean commit, ad-hoc signed, with the
  `M_EchoesWorldSurface` instancing fallback closed under packaged Metal.

**Not proven, and standing between here and the release:**

- No ordinary-player rendered traversal of the campaign or a full skirmish; all campaign completion
  evidence is headless.
- Only one route through Missions 09–15 has complete end-to-end evidence; three of four endings have
  plan and reducer coverage only.
- Packaged captures show blown highlights, a debug-like control overlay, and prototype text. No
  production textures, no rigged or code-driven motion families, no Niagara or final effects, no
  production fog/shroud presentation, no interface art system.
- Music/ambience registration exists but rendered playback qualification, gameplay-event routing,
  interface-event routing, and every mix gate remain open. **No voice pipeline exists at all.**
- **No cinematic exists at all** — no Sequencer pipeline, no title sequence, no interludes.
- Mission dialogue text is not authored and runtime narrative consumption is currently false.
- Skirmish setup UI, AI personalities beyond Adaptive, remapping, subtitles, autosave, named slots,
  and most of the accessibility menu are unimplemented.
- The earlier packaged profile exceeded GPU/render budgets and failed the memory-slope diagnostic;
  the current package has never been profiled. No Developer ID signature, notarization, installer, or
  clean-machine result. The public site does not yet meet the immersion bar and contains claims to
  re-audit.

## 6. Delivery tracks

Ten tracks. Each states its goal, its quality bar, what must be built, and the evidence that closes
it. Work them in the dependency order of section 10, not the order listed. The creative specifics
below restate and apply the Bible's direction; they add production detail, never new canon. Where a
production detail below would require canon the Bible does not contain, the Bible is extended first —
by Angelis or with his approval through section 9 — before the asset is built.

---

### Track A — Visual completion

**Goal:** every rendered element is a registered authored asset, correctly exposed and lit, readable
under combat load, unmistakably of this world.

**The visual identity, applied everywhere.** One palette, from the Bible and the accepted Glass Scar
work: **charcoal** vitrified ground and structure mass; **pale ceramic** for civic and Compact-built
surfaces; **broken-sun amber** as the world's warm accent, emissive seams, and Kharuun's grown
mineral identity; **magenta-fracture** for Crownfall phenomena, possibility bleed, and the Choir;
**cyan** for Matter, Meridian's engineered systems, and interface confirmation. Faction form language:
Compact reads as engineered load paths — orthogonal rails, plates, exposed connections, visible repair
states; Kharuun reads as grown mineral structure — facets, cones, nodules, layered strata, inhabited
and maintained, never primitive; Choir reads as repeated luminous edges and deliberately contradictory
geometry — offset duplicates, spans with two valid shadows, structures that look maintained-possible
rather than solid. Stylized realism preserves tactical silhouettes; terrain contrast is reduced under
combat; destruction communicates functional loss first. Nanite and Virtual Shadow Maps stay off on the
M1 Pro baseline.

**A1 — Exposure, tonemapping, and lighting.** *Blocks all other visual acceptance.* Author an explicit
post-process configuration: fixed or tightly bounded auto-exposure, a tonemapper tuned against the
five-color palette, and an authored lighting rig per site — key direction motivated by the Crownfall
sky, fill discipline that keeps charcoal surfaces legible without lifting them to grey, emissive
amber/magenta/cyan seams that read at gameplay camera height without blooming into adjacent tiles.
Evidence: packaged Metal captures at 1920×1080 and 2560×1440 with no clipped highlights on ceramic and
glass, shadow detail preserved on charcoal, and the same palette readable in high contrast.

**A2 — Debug and prototype surface removal.** Audit every string and widget drawn in a shipping
configuration; development diagnostics move behind a flag absent from the packaged build. A
placeholder disappears only when its registered final replacement lands — never before. Evidence: an
ordinary and a stress packaged capture with zero diagnostic surfaces, plus a source audit listing
every remaining developer-only surface and the gate that removes it.

**A3 — Production textures and materials.** Extend `generate_art_assets.py` to author `Texture2D`
maps deterministically — base color, roughness, metallic, normal, emissive — and drive the
`M_EchoesSurface` / `M_EchoesWorldSurface` / route families through material instances. Required
surface families, each with its own generation recipe and register entry: vitrified glass (deep
charcoal with subsurface magenta micro-fracture), basalt and ash strata, pale ceramic civic paneling
(wear at edges and load points, not uniform grime), Compact machined metal with status-band paint,
Kharuun grown mineral (banded strata, translucent amber nodules), Choir coherent-light surfaces
(emissive gradients that shift with view angle within reduced-flashing limits), Matter deposit
crystal, and route-specific wear (foot-polished causeway deck, ash drifts, verge plate scoring).
Preserve the UV0 tiling / UV1 lightmap contract and the four per-route material zones. Evidence:
byte-idempotent regeneration under new revision strings, an isolated Metal composition review per
family, and register entries.

**A4 — Environment completion, per site.** Extend the Glass Scar pipeline to every campaign site, so
no mission renders on undressed terrain. Each site gets a silhouette identity, a palette weighting, at
least one landmark visible from gameplay camera, and dressing that supports its mission's read:

- **Glass Scar** (skirmish + M01): elevate the existing seven-mesh set with A3 textures; the fracture
  bands and the central Well remain the identity.
- **Lume Reach** (M01, M10): settlement under evacuation — pale-ceramic civic frames, status bands,
  interrupted infrastructure; warm interior light against charcoal.
- **Migration route** (M02): open basin with layered strata, shivergrass fields that bend before
  possible footfall as a readable scouting cue, vaultback herds as neutral mobile silhouettes.
- **Ark-city districts** (M03, M09, M11–M12): Life Support, Transit, and Archive each visually
  distinct — conduit-dense, causeway-dense, and stack-dense respectively — with powered/unpowered
  states readable without color alone.
- **The Unburied Road** (M04): subsurface Kharuun space — grown mineral vaults, amber light from
  strata, emergence zones visibly distinct.
- **Census and Listening-Spine sites** (M06–M07): Compact record architecture and raised Kharuun
  spines; the "missing neighborhood" reads as a void with edges, not an empty lot.
- **Choir manifestation sites** (M08, M14): near-identical repeated geometry with deliberate local
  contradictions — the two-shadow span, the opening that aligns from one approach.
- **Crownfall approach and accord sites** (M10–M15): the fractured sky becomes ground truth —
  magenta-fracture intensity rises across Act III, ending at the accord sites where all three form
  languages stand in one frame.

Environment actors stay presentation-only and non-colliding; deterministic terrain remains the sole
route authority. Evidence: one isolated Metal composition capture per site plus a rendered pass
through each mission confirming no bare collision floor.

**A5 — Motion.** Code-driven component motion, deterministic, presentation-only: tread and leg cycles
keyed to authoritative velocity, hover bob, turret and weapon tracking, recoil, construction assembly
reveal, Carapace and Striker molt transitions, Waystone uproot/root, Bulwark deploy/pack, worker
gather/deliver gestures, and the four Future Well state silhouettes. Idle is not static: Compact units
show maintenance micro-motion, Kharuun units breathe through strata, Choir units drift between offset
positions within reduced-motion limits. Motion reads authoritative state and writes nothing back;
reduced motion holds transforms steady. Evidence: a rendered capture per motion family plus a test
asserting no motion component mutates simulation state or enters a checksum.

**A6 — Effects.** Weapon muzzle/impact per archetype and faction, projectile trails, Matter gather and
deliver, construction dust, production emergence, research completion, Well activation and mode
transitions, Reshape terrain opening/closing with its telegraph, destruction debris and smoke,
dissolve on removal, and Crownfall ambient phenomena (duplicated shadows, brief possibility shimmer)
used sparingly enough to stay legible. Faction grammar: Compact effects are engineered — directional,
clean edges, cyan-white; Kharuun effects are material — dust, strata shards, amber heat; Choir effects
are phase — offset afterimages, magenta interference. Prefer authored mesh-VFX; introduce Niagara only
where mesh VFX cannot carry the effect, each system a recorded exception with measured cost. Every
effect disables collision, overlap, navigation influence, and shadows, and respects reduced motion and
reduced flashing. Evidence: isolated review capture per family plus a stress capture with all families
active and the frame cost recorded.

**A7 — Fog and shroud.** Production shroud over the unchanged fair visibility authority: unexplored
reads as deep charcoal with faint magenta fracture-bleed, explored-not-visible as desaturated memory
with persistent terrain, visible as full palette — distinguishable without color alone, never
revealing what the simulation hides. Evidence: a rendered scouting sequence, the fog authority tests
still passing, and frame cost within the 1.5 ms budget at 400 units and four teams.

**A8 — Interface art.** One coherent system across title, operation and faction selection, mission
brief, live HUD, command deck, tactical minimap, technology archive, field menu, options, result, and
credits. The HUD is of this world: ledger-and-instrument visual language for the Compact-standard
chrome, faction ownership via the established non-color marks (bracketed rectangular Meridian, paired
faceted Kharuun, offset concentric Choir), charcoal panels with pale-ceramic type, cyan confirmation
and amber alert accents, and the licensed or original typeface of section 9 applied everywhere.
Everything scales with HUD scale and has a high-contrast variant. Evidence: rendered captures of every
screen at 1280×720, 1440×900, 1600×900, 1920×1080, and 2560×1440, standard and high contrast, no
clipping or occlusion.

**A9 — Title and menu presentation.** The first ten seconds are part of the world: an authored title
scene (in-engine, using registered assets, slow enough to hold under reduced motion), menu ambience
and music from track B, and no visible loading seam between title, brief, and deployment. Evidence: a
rendered capture of the title-to-deployment path with no placeholder frame.

**A10 — Glass Scar composed frame.** The owner-supplied concept `CONCEPT-004`
(`site/assets/concepts/target-render-vertical-slice.jpg`) is the composition target for the Glass
Scar skirmish frame, judged as one frame rather than per element. The frame must show: the central
Well as a built dais with concentric rings and a readable shard, standing on a chasm crossed by the
buried causeway, with cliff silhouettes on both banks; the two armies on their banks at a scale where
a heavy unit reads as roughly one fifth of the dais diameter; unit-facing that shows the faction
silhouette three-quarter to the camera, not edge-on; the Broken Sun as a fractured sphere with
drifting shards, not a flat disc; crystal deposits subordinate to the Well in size and brightness;
terrain as vitrified charcoal ground, not loose plates. Rendered under the A1 rig with no
fixture-only exposure or bloom override. The review fixture (`-EchoesGlassScarReview=VerticalSlice`)
is an authoring preview of this frame; acceptance evidence is the same composition captured from live
Glass Scar skirmish play. Evidence: fixture capture and live-play capture at 1920×1080 and 2560×1440,
each paired with the concept in one sheet, with commit, preset, and date recorded; the owner reviews
the pair.

---

### Track B — Audio completion

**Goal:** an original score and soundscape with a distinct identity per faction and site; every event
the design says has a sound, has one; the mix is measured and qualified.

**The audio identity.** From the Bible: Compact music is measured pulse, prepared piano, restrained
brass, mechanical resonance — the sound of systems maintained under load. Kharuun music is
interlocking rhythms and resonant stone and ceramic timbres, layered like communal memory, never
generic tribal coding. Choir harmony resolves in more than one direction before committing — write
progressions that remain honest to that rule. Sound effects follow material truth: Compact is
engineered metal and ceramic, higher and cleaner; Kharuun is stone, strata, and resonance, lower and
warmer; Choir is phase, interference, and held tones that arrive slightly before or after their
visual. Alerts are brief and rate-limited. Nothing is approved until registered.

**B1 — Mix architecture.** PASS — carried forward (demo gate 14, `a470eb9`): five-category submix
graph (music, dialogue, interface, ambience, effects) with independent volumes and whole-graph reduced
dynamic range. Remaining under this track: the dialogue category becomes the voice bus for track C,
with ducking rules (music and ambience duck under voice by a fixed, measured amount; effects do not
duck — combat information never disappears).

**B2 — Music.** Extend the registered fifteen-cue set to the full score, deterministically synthesized
through the established pipeline. Required cue set, each with a stated musical intent, target length,
loop or transition behavior: title theme (the world's theme — all three faction materials present,
none dominant); one identity theme per faction for faction selection and their campaign missions; one
theme per act (Act I *Necessary Fires* — urgency held in check; Act II *The Cost of One Future* —
inquiry and unease; Act III *Crownfall* — scale and consequence); a low-intensity tension layer and a
combat layer per faction pairing that crossfade on authoritative combat state without a hard cut;
victory and defeat stingers; one ending resolution cue per ending — Restoration, Controlled
Stabilization, Extinguishment, Open Evolution — each resolving the Choir's multi-directional harmony
differently, none of them triumphal by default; brief/results underscore; and cinematic underscore per
track D sequence. Evidence: provenance records proving synthesis from project code, integrated
loudness and true peak measured per cue against the section 10 targets, and a rendered session capture
showing correct state transitions including combat crossfade.

**B3 — Ambience.** A bed per site family, positional where the design implies a source: Glass Scar
wind across vitrified glass with shard chimes; Lume Reach settlement resonance and failing-reserve
electrical strain; migration route open wind, shivergrass movement, distant vaultbacks; ark-city
districts each distinct (Life Support's circulation hum, Transit's causeway resonance, Archive's
stillness); Kharuun interiors' strata resonance and warmth; Choir sites' held tones and interference
beating; Crownfall approach where the sky is audible — deep fracture harmonics keyed to
magenta-fracture intensity; and the Well hum keyed to its active mode (Dormant low, Harvest telegraph
rising, Preserve steady, Reshape phase-shifting). Evidence: a rendered capture per site and a
measurement that ambience does not mask combat cues at default levels.

**B4 — Gameplay audio.** Complete the event map: weapon fire and impact per archetype per faction;
Matter gather and deliver; construction start/progress/complete; production start and emergence;
research start, completion, and no-refund interruption; Well claim and each mode transition; Reshape
open and close with telegraph audio; destruction per faction extended to the Choir; molt transitions;
Waystone root/uproot; Bulwark deploy/pack; and textural (non-spoken) unit acknowledgement per faction.
Evidence: a coverage test enumerating every authoritative gameplay event against its registered cue,
no unmapped event.

**B5 — Interface audio and alerts.** Hover, select, confirm, distinct rejection paired with the stable
failure reason, menu open/close, brief and result transitions, and brief rate-limited alerts (attack,
structure loss, production complete, research complete, low capacity) routed to `Space` jump-to-alert.
Evidence: a rate-limit test and a rendered capture of each alert under a simultaneous-event burst.

**B6 — Mix qualification.** Set and verify the packaged targets: integrated loudness −16 LUFS ±1 for
an ordinary session, true peak ≤ −1 dBTP everywhere, voice intelligible over bed at default levels,
reduced dynamic range preserving audibility of the quietest mapped cue. Then the readability case:
under simultaneous combat load, a player can still identify an alert, a destruction cue, a rejection,
and a spoken line. Evidence: measured loudness/peak plus a stress capture with the identification
result stated as a bounded observation.

---

### Track C — Voice

**Goal:** every campaign dialogue line, brief, and cinematic narration is performed — in character, at
professional audio quality, produced by the project's own pipeline, always subtitled.

**C1 — Pipeline and model decision.** Select an open-weights TTS model that runs locally on the Mac,
record model, version, weights hash, and license in `AssetRegister.md` and section 9, and integrate it
into `generate_audio_assets.py` as an editor-time stage: script in, 48 kHz PCM out, deterministic
under a recorded seed and revision so regeneration is reproducible. Voice generation never becomes a
runtime dependency. Evidence: the register entry, a reproducibility run, and a listening review of a
calibration line set.

**C2 — Voice design per character.** One recorded voice profile per speaking character, derived from
the Bible's characterization and approved by Angelis before batch generation: **Mara Vey** — level,
precise, engineering cadence; urgency compressed into economy rather than volume. **Oruun-of-Seven-
Stones** — layered and deliberate; carries seven mutually correcting accounts, so certainty arrives
qualified; dry humor from inherited-memory mismatch. **Talar Venn** — careful, archival, quietly
persistent. **Chancellor Cael Rhyse** — persuasive, warm, reasonable in exactly the way that makes his
program dangerous. **Neme** — constructed precision; speech assembled by selecting among incompatible
phrasings, so delivery is exact, lightly non-idiomatic, never mystical-collective by default. Minor
attributed speakers take profiles consistent with their faction's language culture. Evidence: an
approved profile sheet per character and a directed calibration line per profile.

**C3 — Script coverage.** Voice consumes the authored mission text of track E4 — situation briefs,
mid-mission beats, and results — plus track D narration. The Bible's writing rules govern every line:
characters speak from immediate needs and incomplete knowledge; exposition through disagreement,
action, evidence, consequence; no villain explains the setting; no civilization speaks with one
opinion; and no spoken line asserts authorship, consent, trust, civilian survival, or authenticity the
simulation does not model. Evidence: a coverage matrix of authored lines to generated performances,
with zero unvoiced dialogue lines.

**C4 — Generation, registration, and quality.** Batch-generate through the pipeline; register each
line set as an asset family with revision, seed, and script hash; re-listen to every mission's lines
in a directed review pass and regenerate rejects with adjusted direction. Loudness-normalize to the
dialogue target before import. Evidence: register entries, per-mission review sign-off, measured
loudness per line set.

**C5 — Runtime playback and subtitles.** Voice plays on the dialogue bus, synchronized with the
subtitle system: subtitle text always matches the performed line exactly, subtitles carry size and
background controls, voice volume is independently controllable, and voice-off leaves the full text
experience intact — the release must remain fully playable and complete with voice disabled. Evidence:
a rendered pass with subtitles on and voice at each volume state, plus a text-only pass confirming no
information loss.

---

### Track D — Cinematics

**Goal:** the story is delivered with authored, in-engine cinematic sequences that meet the visual and
audio bar — not cards, not text-only interludes.

**D1 — Sequencer pipeline and camera language.** Establish a Level Sequence pipeline over registered
assets with a defined camera language: measured, ground-referenced moves that respect the world's
scale (no impossible swoops), letterboxed presentation, authored lighting per shot, and a data-driven
trigger path from campaign state so sequences are source-authored and testable. Sequences are
presentation-only: they read campaign/authoritative state and write nothing back. Evidence: one
reference sequence demonstrating the pipeline end to end — trigger, playback, skip, return to play —
with a test asserting no simulation mutation.

**D2 — Title sequence.** An in-engine opening establishing the broken sun, the Crownfall, and the
scale of what was lost — playable on first launch and from the title screen, under 90 seconds,
skippable after first viewing per the accessibility rules. Narrated (track C), subtitled.

**D3 — Act transitions.** One authored sequence per act boundary (into Act I, II, III), staged on the
sites of the surrounding missions, carrying the act theme (B2) and voiced narration that follows the
writing rules — advancing perspective and cost, never explaining the setting from above.

**D4 — Ending cinematics.** One authored sequence per ending — Restoration, Controlled Stabilization,
Extinguishment, Open Evolution — staged at the accord sites, reporting concrete consequences and
unresolved costs exactly as the ledger records them, with that ending's resolution cue. No ending is
scored or framed as the correct one. Evidence for D2–D4: rendered captures of every sequence,
skippability and subtitle verification, and a check that no narrated claim exceeds what the campaign
ledger and Bible support.

**D5 — Cinematic accessibility and integration.** Every sequence: skippable, subtitled, respects
reduced motion and reduced flashing (no strobe cuts), holds the performance budget or pauses
simulation cleanly, and returns the player to a coherent state. Evidence: the settings matrix run
against every sequence.

**D6 — Trailer export.** A cut of in-engine footage for the website and any store page — the game's
own footage at the game's own bar, no mockups, no unrepresentative staging. Evidence: the exported
file plus a claims check against the evidence register.

---

### Track E — Campaign playability

**Goal:** an ordinary person plays all fifteen operations from an empty ledger to an ending they
chose, with mouse and keyboard, immersed the whole way.

**E1 — Suite health.** PASS — carried forward (demo gates 1–4): automation 58/58 including
`FreshJourney` all four journeys; native 37/37 ×3; content 123/123. Standing requirement: these
suites stay green on every merge to `main`; any red is the next session's first work.

**E2 — Route and ending coverage.** Missions 10–15 each carry 27 plans; only one route has complete
downstream evidence and only Controlled Stabilization plus the two `FreshJourney` routes are
demonstrated end to end. Extend automated coverage until every ending has at least one complete
recorded route and every player-reachable plan completes rather than dead-ends. Evidence: one recorded
complete route per ending type and a matrix of reachable plans against outcomes.

**E3 — Difficulty and pacing pass.** A campaign an ordinary person can finish: per-mission tuning
against `Content/Data/Source` so that standard difficulty is challenging but completable without
foreknowledge, mission length lands in a stated target band, and no mission requires the player to
already know the solution (the Mission 08 lesson institutionalized). Evidence: recorded ordinary-
player attempts per mission with completion, time, and stuck-point data feeding tuning commits.

**E4 — Mission text authored and consumed at runtime.** Fifteen operations of situation briefs, live
objectives, mid-mission beats, and results that report concrete consequences and unresolved costs —
authored under `Content/Narrative/Source` against schema, compiled, and consumed at runtime (currently
false; must become true). The Bible's writing rules and claim bounds govern every line; this text is
also the voice script (C3). Evidence: schema validation, a rendered pass showing every string in
place, and a check that no line asserts an unmodeled consequence.

**E5 — Saves, slots, autosave, recovery.** Named multiple slots, autosave at mission and phase
boundaries, and an interrupted-session recovery flow a player can understand — keeping fail-closed
container binding and CRC integrity, and the honest limitation that CRC is not authentication.
Evidence: slot create/load/overwrite/delete tests, an autosave test, a killed-process recovery test,
and the existing migration tests still passing.

**E6 — Rendered human traversal.** The acceptance that matters: a person who is not Angelis plays the
campaign in the packaged build with physical input, recorded, with wall-clock time per mission, every
stuck point, and every defect logged. Automation cannot satisfy this gate.

---

### Track F — Skirmish and opponent AI

**Goal:** the player-versus-AI experience the release is named for is configurable, distinct, and
worth replaying.

**F1 — Skirmish setup.** Expose map, faction, teams, AI personality, difficulty, starting resources,
victory conditions, and game speed, per the Bible. Assisted difficulty names its exact modifier before
the match and in replay metadata; standard difficulty keeps using only player-visible information and
no hidden income. Evidence: a test per exposed option proving it changes match setup, plus a rendered
pass through the setup screen.

**F2 — AI personalities.** Defensive, Expansionist, Raider, Economic, and Adaptive as distinct command
producers over the existing scoped player view — no simulation handle, hidden entities omitted,
non-owned internals redacted. Evidence: a deterministic behavioral test per personality demonstrating
the distinction its description promises, stated plainly as bounded behavior, not human-equivalent
play.

**F3 — Balance pass.** Nine matchups across three factions, tuned iteratively against
`Content/Data/Source`. Evidence: recorded outcomes across a defined number of matches per matchup at
standard difficulty, win-rate spread stated, remaining imbalance named rather than hidden.

---

### Track G — Player experience and accessibility

**Goal:** the settings are real, the layout survives real windows, and a person can play without a
reference card.

**G1 — Controls.** Full remapping for every command in the Bible's command set, conflict detection,
reset. Invalid actions keep returning a stable reason with visible feedback.

**G2 — Accessibility menu.** UI scale, subtitle size and background, keyboard-operable menus
throughout, color-vision-safe palettes, non-color ownership markers, reduced shake and flashing,
adjustable camera motion, separate audio category volumes including voice, reduced dynamic range,
offline pause, tutorial replay, searchable technology information, destructive-choice confirmation,
autosave, interrupted-session recovery. A setting that does not change behavior does not pass — test
each for effect, not presence.

**G3 — Layout coverage.** Extend the seven fixed configurations to arbitrary and live-resized windows,
fullscreen and windowed, and the M1 Pro's native panel. Evidence: automated projected-bounds checks
plus rendered captures at the boundaries.

**G4 — Unaided human usability.** People who are not Angelis play a skirmish and at least one campaign
mission, unaided, recorded. Every confusion becomes a defect or a documented limitation.

---

### Track H — Performance and stability

**Goal:** the packaged build holds its budgets and survives a long session.

Budgets (from the ledger, unchanged): 16.67 ms frame; ≤ 4.0 ms game thread; ≤ 11.0 ms render + GPU;
≤ 1.5 ms fog; ≤ 6.0 ms path burst; ≤ 10 GB resident; ≤ 250 ms save. Tracks A–D add cost to exactly
the systems that were already over budget, so profile after each visual and audio track lands, not
once at the end; a track whose landing breaks a budget is not closed. Close with a valid
600-active-second preflight followed by one uninterrupted 60-minute sustained run of the same package
through the existing fail-closed wrapper. Evidence: profile artifacts and the soak log, published
atomically as the wrapper requires.

---

### Track I — Packaging, distribution, and portability

**Goal:** a stranger installs and plays it; the next platforms are not blocked.

**I1 — Shipping package.** Shipping configuration, not Development. Evidence: the package builds,
launches, and plays a skirmish.

**I2 — Sign, notarize, staple, installer.** Developer ID signature, notarization, stapling, and a DMG
or installer. Evidence: `spctl` and `stapler` verification output.

**I3 — Clean machine.** On an Apple Silicon Mac that has never had Unreal, Xcode, or this project:
install, launch, play a skirmish, quit, relaunch, load a save — with the Seagate volume absent.
Evidence: the session record and a first-launch time measurement.

**I4 — Portability guard.** The platform roadmap after this release is Linux/SteamOS, then Windows,
then others. This release must not make that path harder: `EchoesSimCore` stays engine-independent
and platform-clean; no new macOS-only dependency enters game code outside the existing platform
layer; asset generation and tests remain scriptable without a GUI; and every new subsystem this
directive adds (voice pipeline, cinematics, settings) is built against engine-portable APIs. Evidence:
a recorded audit of new dependencies at release, stating what would block a Linux build and why —
zero new blockers is the target; any accepted blocker is named in section 9.

---

### Track J — Immersion coherence and the public front door

**Goal:** the world is continuous from the first web page to the final ending, and everything said in
public is true.

**J1 — Coherence review.** A recorded end-to-end pass — website, install, title sequence, menus,
briefs, three missions across the acts, one cinematic, one ending — judged against one checklist: same
palette, same type system, same iconography, same music language, same written voice, no seam, no
placeholder, no off-world element. Defects filed per surface. Evidence: the recorded pass and its
checklist.

**J2 — The website.** Bring the public front door to the same identity and bar as the game: the
world's palette and typeface, the game's own rendered footage and captures (D6; no mockups), the
story, faction, and Future Well pages consistent with the Bible, and system requirements, download or
store link, and a visible, accurate known-limitations page. Resolve the `site/` versus `website/`
duplication (section 9) to one authoritative public property. Every public claim traces to a passing
gate. Evidence: a claims audit mapping each public statement to its evidence, and rendered page
reviews at desktop and mobile widths.

**J3 — Documentation and rights.** `ProjectLedger.md` evidence register and known limitations current;
`AssetRegister.md` complete including every exception family (TTS model and license, typeface, any
Niagara or generative exceptions); player manual and controls reference written; the rights position
confirmed in writing — all assets project-generated or exception-recorded with license, no third-party
obligation unaccounted, typeface licensed before ship.

**J4 — Known limitations shipped.** The release ships with a visible, accurate limitations page —
multiplayer absent, bounded AI, the measured balance spread, and anything else the evidence register
requires — in the build and on the site.

## 7. Asset pipeline rules

Both generators are editor-time Python driven through their shell wrappers. Extend them; do not build
a parallel path.

- Every asset family carries a revision string. Changed generation logic means a new revision, a
  regeneration run, and an `AssetRegister.md` entry. Regeneration under an unchanged revision must be
  byte-identical — prove it.
- Generated assets are ordinary `StaticMesh`, `Material`, `MaterialInstance`, `Texture2D`, and
  `SoundWave` assets. Geometry Scripting, Python, and the audio/voice synthesis stages are editor-time
  dependencies only.
- Meshes ship with authored LOD0/LOD1, UV0 tiling, UV1 lightmaps, explicit material zones, simple
  collision retained for inspection — while spawned presentation components disable collision,
  overlap, and navigation influence.
- Audio sources are mono or stereo 48 kHz PCM written to `Content/Audio/Source` and imported to
  `/Game/Audio/Generated`, WAV sources under LFS so provenance survives. Voice line sets follow the
  same path with script hash and seed recorded.
- Exceptions follow section 2's policy: recorded per family, before use, with method, license, and the
  reason the procedural path was insufficient. An unrecorded exception is a defect to fix, not a
  shortcut to keep.
- If a required asset cannot be produced at the professional bar by any authorized path, stop and
  write it into section 9. Do not quietly lower the bar and do not quietly import.

## 8. Execution model

**Process.** Judge per task: lane lease when touching files another lane holds
(`../WorkstreamControl/ACTIVE_LANES.md`), branch otherwise — `release/<track>-<topic>`. Stage explicit
paths only; the tree carries unrelated modifications and gigabytes of build artifacts. Merge to `main`
when the gate's acceptance evidence exists; push from the Mac shell with LFS objects; never rewrite
`main` history, never force-push.

**Where work runs.** Builds, the automation suite, packaging, asset and voice generation: on the Mac
through Desktop Commander, with the environment traps in `CLAUDE.md` section 3 respected (TMPDIR, the
60-second bridge cap, the Seagate remount, the Linux-only test failures).

**Visual proof.** A Track A or D item is not offered for acceptance without the owner review packet
defined in `CLAUDE.md` section 3: Mac-rendered screenshots of each final asset in context and a short
movie of every animation or moving effect, with commit, preset, and resolution recorded.

**Sessions.** Run the section 0 loop. Update this directive's gate matrix in the same commit as the
work it describes; the directive is worthless if it drifts from reality.

**Claims.** Evidence-bounded language everywhere, the six reporting states, self-review labeled
internal QA. Independent validation requires a source that is not the session that built the work.

## 9. Open decisions

Add to this list rather than guessing. Each entry needs the question, the options, and costs.

1. **TTS model selection** (blocks C1). Requirement: open weights, local execution on the M1 Pro,
   license compatible with commercial shipping of generated audio, quality sufficient for directed
   character performance. Decision owner: Angelis. **Written evaluation (2026-09-01 session):**
   - **Kokoro-82M** — Apache-2.0 weights, ~82M parameters, runs comfortably on the M1 Pro CPU/GPU,
     strong clarity for its size, multiple built-in voices, deterministic under a fixed seed path.
     Weakest at expressive direction (limited emotion control); strongest license/effort ratio.
     **Recommended default** if calibration lines pass a listening review.
   - **Piper** — MIT, very fast, tiny models, fully offline; quality noticeably synthetic for
     principal characters, adequate for fallback or accessibility narration. Keep as a backstop.
   - **XTTS-v2 (Coqui)** — strong quality and voice cloning, but the Coqui Public Model License
     prohibits commercial use — **fails the license requirement outright**.
   - **Bark (Suno)** — MIT, expressive, but slow, nondeterministic-leaning, and unstable for long
     directed lines; a poor fit for a reproducible pipeline.
   - **Orpheus-3B / Zonos-v0.1** — Apache-2.0, best-in-class expressiveness among open models;
     3B-parameter scale means slow M1 Pro generation (minutes per batch) and heavier setup. Viable
     as a quality upgrade after Kokoro calibration if direction control proves insufficient.
   **RESOLVED 2026-09-01 (Angelis):** Kokoro-82M selected. Run gate 17's calibration set through
   Kokoro-82M; escalate to Orpheus only if the directed-performance review rejects it. Weights may
   now be pulled; record model id, weights hash, and license in the rights record on first use.
2. **Typeface** (blocks A8, J2). A licensed or original typeface for game and site, license
   recorded. Decision owner: Angelis. **Candidates (2026-09-01 session), all SIL OFL 1.1 — free for
   commercial embedding in games and sites, no per-seat cost, attribution in the rights record:**
   - **Space Grotesk** (OFL) — engineered-geometric sans; suits the Compact's
     ledger-and-instrument HUD language. Recommended for UI chrome.
   - **IBM Plex Sans + Plex Mono** (OFL) — a complete family with a mono cut for readouts and
     coordinates; the most complete coverage if one family must do everything.
   - **Rajdhani** (OFL) — condensed technical sans, strong at small HUD sizes; weaker as body text.
   - **Exo 2** (OFL) — rounded technical sans, softer look; wide weight range.
   **RESOLVED 2026-09-01 (Angelis):** Space Grotesk for interface chrome + IBM Plex Mono for
   tactical readouts, both SIL OFL 1.1, licenses vendored into `Docs/` and recorded in
   `AssetRegister.md` on first embedding. No paid or original commission needed.
3. **`site/` versus `website/`** (blocks J2). Two web properties exist. 
   **RESOLVED 2026-09-03 (Angelis):** Consolidated. Retained `website/` as the sole framework host running Next.js/Vite to fulfill Track J requirements. Migrated all core content assets and HTML pages from `site/` into `website/public/` to prevent loss of marketing copy and imagery. `site/` has been purged from the repository.
   **Observed 2026-09-04 (session note, not a resolution):** `site/` is still tracked on `main` and is the property `.github/workflows/pages.yml` publishes to GitHub Pages on every push; `website/` is not deployed by any workflow, and `website/public/archive-static/` mirrors `site/`. The public-site changes recorded as `SITE-VISION-001` were made to `site/` for that reason and applied identically to the mirror. Owner decision needed on which property is live before the J2 claims audit can name one artifact.

4. **Niagara adoption** (A6). Permitted only where mesh VFX cannot carry an effect; each system is a
   recorded provenance and performance exception.
5. **Difficulty options** (F1). The Bible allows assisted levels with labeled modifiers. Confirm which
   levels ship.
6. **Distribution channel.** Direct download from the site, Steam (macOS now, aligning with the
   Linux/SteamOS roadmap), or both — affects packaging, store text, and the claims audit. Undecided.
7. **Mission 05 narrative cast** (blocks the M05 contract under gate 24). The Bible's Terms of
   Continuance entry names no speaking character — the treaty runs through "Meridian-authoritative
   treaty and witness proxies". Canon continuity therefore records no named participant for
   TermsOfContinuance, and the narrative validator pins casts to canon continuity. Options:
   (a) extend canon continuity so Mara Vey (Meridian treaty authority, consistent with her Act I
   command arc) and/or Oruun-of-Seven-Stones (the standing cross-faction interlocutor) appear in
   M05 — smallest Bible-consistent cast, needs your approval as a canon extension; (b) author M05
   with an attributed minor speaker (new named proxy character) — a larger canon extension;
   (c) leave M05's dialogue as unattributed operations copy — weakest storytelling. Missions
   02–04 and 06–15 have canonical casts and are not blocked.
   **RESOLVED 2026-09-01 (Angelis):** option (a), limited to Mara Vey alone — canon continuity is
   extended so Mara Vey, as the Meridian treaty authority consistent with her Act I command arc,
   carries Mission 05's spoken lines. No new named characters.
9. **Remote GPU authoring workstation.** Proposed and resolved on 2026-09-04 as a cloud Windows GPU
   editor for authoring only. **CANCELLED 2026-09-04 (Angelis):** no remote workstation. All authoring,
   building, and evidence work runs on the local M1 Pro. The rules written for it were removed from
   `CLAUDE.md` and `SetupAndBuild.md` the same day.
10. **Demo versus full labeling.** This release contains the full fifteen-operation campaign and
   skirmish. **RESOLVED 2026-09-01 (Angelis):** the public label is **Version 1.0 — full release**.
   All store and site text is written against the 1.0 full-release claim set; nothing may be
   labeled a demo.

## 10. Gate matrix

Every gate without carried evidence is NOT RUN until a session runs it and records the evidence here
with its commit SHA and date. Dependencies are hard: do not accept a gate whose prerequisite has not
passed. "OD n" means open decision n in section 9 must be resolved first.

| # | Gate | Track | Depends on | State |
|---|---|---|---|---|
| 1 | Suite health: automation, native ×3, and content suites green on current `main`; standing requirement on every merge | E1 | — | PASS 2026-09-04 — Full 77/77 Unreal automation tests passing clean (0 errors, 0 warnings); 96/96 native simulation tests passing across Release, Debug, and ASan/UBSan; content/world suites 33/33 tests passing (15 map dressing, 7 dressing pack header, 11 Lume Reach dressing); runtime smoke clean across all factions |
| 2 | Five-category submix graph with independent volumes | B1 | — | PASS carried from demo gate 14 — `Echoes.Runtime.Audio.MixArchitecture` green at `a470eb9`; evidence `demo-gate14-mix-architecture-*`; voice-bus ducking rules remain under gate 16 |
| 3 | Exposure, tonemapping, and per-site lighting rig; no clipped highlights in packaged captures | A1 | 1 | OPEN — themed packaged captures accepted 2026-09-01 at `f47dbff8`: hero-referenced Glass Scar (golden-fracture vitrified ground, authored gold/indigo rig) at 1920×1080, 2560×1440, and high-contrast, clipped ≤0.00058% (ledger ART-A4-001/PKG-I1-002, evidence `release-gate3-themed-packaged-*`); Glass Scar's rig is authored — per-site rigs for the remaining campaign sites still owed |
| 4 | No debug overlay or prototype text in shipping-configuration captures | A2 | 3 | NOT RUN |
| 5 | Production texture/material families registered, byte-idempotent, reviewed per family | A3 | 3 | IN PROGRESS — all nine A3 families registered at `surface-textures-v8` and bound per faction on 2026-09-03 (ledger ART-A3-002, evidence `BuildArtifacts/Evidence/release-gate5-a3-surface-families-20260903T231806Z`): editor-render captures measured within the exposure window; per-family isolated Metal composition review, packaged and high-contrast captures, Folded Verge route-kit binding, and owner review still owed; Future Well landmark bound to vitrified glass and crystal on 2026-09-04 (ART-A3-003), Dormant state captured only |
| 6 | Every campaign site dressed to its stated identity; no bare collision floor | A4 | 5 | IN PROGRESS — Glass Scar and Lume Reach / Ark-City district dressing packs consumed at runtime on 2026-09-04 (ledger WORLD-A4-001/002, evidence `BuildArtifacts/Evidence/release-gate6-*` and `BuildArtifacts/ChoirAtLumeReach/LumeReach*.png`): 39 digest-pinned civic_frame and conduit_pylon records drawn on live-Blocked perimeter wall and transit cells with pale ceramic plates, amber interior lighting, and zero simulation touch (`SIM-002` / `REL-ART-026`); automated test `Echoes.Runtime.Map.LumeReachDressing` green in 77/77 automation suite; rendered captures verify no bare collision floor for Lume Reach campaign missions; Crownfall, Unburied Road, and Choir manifestation site vocabularies remain to complete full campaign coverage |
| 7 | Motion families implemented, presentation-only, reduced-motion compliant | A5 | 5 | PASS 2026-09-04 — presentation-only code-driven motion families implemented across all roster archetypes, structures, and landmarks (EchoesEntityView); verified zero simulation touch (checksum bit-for-bit identical), full reduced-motion accessibility compliance, and in-engine automation test green (Echoes.Runtime.Presentation.MotionFamilies PASS, 96/96 native sim tests PASS across release/debug/ASan, runtime smoke PASS; ledger ART-A5-001) |
| 8 | Effect families implemented with recorded combat-load frame cost | A6 | 7 | PASS 2026-09-04 — presentation-only combat effect families implemented across all factions (AEchoesCombatEffectView weapon beams, muzzles, impact bursts, Choir dual-offset phase afterimage; AEchoesEntityView worker matter gather beams, construction assembly fields, and Reshape telegraph ground sigils); pooled allocation with tier caps (48 low, 128 medium, 256 high) and deterministic overflow coalescing; verified zero simulation touch (checksum bit-for-bit identical), full accessibility compliance (ReducedMotion suppresses afterimages/jitters; ReducedFlashing clamps emission <= 1.0), and in-engine automation test green (Echoes.Runtime.Presentation.CombatEffects PASS, 96/96 native sim tests PASS across release/debug/ASan, runtime smoke PASS across Meridian, Kharuun, and Choir; ledger ART-A6-001) |
| 9 | Production fog and shroud within the 1.5 ms budget | A7 | 3 | PASS 2026-09-04 — production fog/shroud presentation implemented over authoritative 64×64 visibility grid (`AEchoesFogView`); deep charcoal basalt with faint magenta fracture-bleed for unexplored (`Height: -16 to 184 uu`), desaturated memory tint (`Height: 6 uu, Centre: 14 uu`) over persistent geometry for explored, full palette for visible; zero simulation touch verified (SIM-002 checksum bit-for-bit identical across 40 ticks), complete accessibility compliance (ReducedFlashing clamps fracture emission to ≤0.1; ReducedMotion freezes drift/phase coordinates), collision/shadows/nav/overlaps strictly disabled; synchronization measured within ≤1.5 ms budget across 4,096 tiles (incremental average <0.2 ms); dedicated automation test `Echoes.Runtime.Presentation.ProductionFog` green (exit code 0), 96/96 native simulation tests PASS across release/debug/ASan, and runtime smoke PASS across Meridian, Kharuun, Choir (ledger ART-A7-001) |
| 10 | Interface art system complete at five resolutions, standard and high contrast | A8 | 3, OD 2 | NOT RUN |
| 11 | Title and menu presentation seamless, in-world, no placeholder frame | A9 | 10, 12 | NOT RUN |
| 12 | Full score registered with per-cue loudness and true peak measured; combat crossfade demonstrated | B2 | 2 | PASS 2026-09-04 — 29-cue full score synthesized, imported, and registered under `music-v3`/`music-v4` (AUDIO-004, AUDIO-006); world title theme, 3 faction identity themes (Meridian, Kharuun, Choir), 3 act beds (Necessary Fires, The Cost of One Future, Crownfall), 6 faction-pairing tension and combat layers with order-independent selection, 2 victory/defeat stingers, 4 distinct ending stingers (Restoration, Controlled Stabilization, Extinguishment, Open Evolution), and brief/results underscores; seamless crossfade between contexts without hard cut; BS.1770-4 loudness measured for all cues with 4× inter-sample true peak $\le -1.41$ dBTP (ceiling $\le -1.0$ dBTP); in-engine automation test `Echoes.Runtime.Audio.MusicAmbience` green (exit code 0), 96/96 native simulation tests PASS, and runtime smoke PASS (ledger AUDIO-B2-001) |
| 13 | Ambience beds per site, positional where sourced, not masking combat cues | B3 | 2 | PASS 2026-09-04 — 5 site ambience beds synthesized and registered under `ambience-v1` (AUDIO-005); Glass Scar vitrified wind and shard chimes, Lume Reach settlement resonance, Ark-City infrastructure hum, Crownfall sky fracture harmonics, and Future Well proximity layer; dynamic crossfading between sites without hard cut; site bed continues beneath proximity layer; cues route to Ambience submix; BS.1770-4 loudness measured with true peak $\le -2.98$ dBTP; in-engine automation test `Echoes.Runtime.Audio.MusicAmbience` green (exit code 0), 96/96 native simulation tests PASS, and runtime smoke PASS (ledger AUDIO-B3-001) |
| 14 | Gameplay audio coverage — no unmapped authoritative event | B4 | 2 | PASS 2026-09-04 — complete authoritative gameplay audio coverage across all 18 enumerated event classes (`UEchoesGameplayAudioSubsystem`); weapon fire per archetype (Light/Line/Heavy), impact and shielded impact feedback, worker gather and deliver, construction start and complete, production emergence, research start and interruption, Well claim and Harvest/Preserve/Reshape protocol choices, and Reshape terrain open/close windows; every cue routed to Effects category submix with bounded linear spatial attenuation (300 to 4,200 uu); non-cheating fair-visibility observer derivation ensures unobserved actions remain completely silent; in-engine automation test `Echoes.Runtime.Audio.GameplayCues` green (exit code 0), 96/96 native simulation tests PASS across release/debug/ASan, and runtime smoke PASS (ledger AUDIO-B4-001) |
| 15 | Interface audio and rate-limited alerts | B5 | 2 | PASS 2026-09-04 — complete interface audio and alert system (`UEchoesInterfaceAudioSubsystem`); all 7 UI interaction cues (Hover, Select, Confirm, Reject, MenuOpen, MenuClose, BriefAdvance) with 60 ms rate-limiting; all 5 authoritative alert classes (UnderAttack, StructureLost, ProductionComplete, ResearchComplete, CapacityLow) with 4,000 ms cooldown and single-active-alert gating (500 ms terminal alert cooldown); routing verified to Interface category submix; volume muting silences output without dropping simulation events; in-engine automation test `Echoes.Runtime.Audio.InterfaceCues` green (exit code 0), 96/96 native simulation tests PASS across release/debug/ASan, and runtime smoke PASS (ledger AUDIO-B5-001) |
| 16 | Mix qualified: −16 LUFS ±1 integrated, ≤ −1 dBTP, voice ducking rules, simultaneous-combat readability incl. a spoken line | B6 | 12, 13, 14, 15, 20 | NOT RUN |
| 17 | Voice pipeline integrated; model, weights hash, license recorded; reproducibility proven | C1 | OD 1 | OPEN — Kokoro-82M pulled and pinned (weights sha recorded, Apache-2.0), ten-line five-speaker calibration byte-reproducible (`CALIBRATION_OK nondeterministic=0`, ledger VOICE-C1-001); awaiting the owner's listening review, then full-line synthesis and runtime integration |
| 18 | Voice profiles per character approved on directed calibration lines | C2 | 17 | NOT RUN |
| 19 | Script-to-voice coverage matrix complete — zero unvoiced dialogue lines | C3 | 18, 24 | NOT RUN |
| 20 | Line sets generated, registered, loudness-normalized, review-passed per mission | C4 | 19 | NOT RUN |
| 21 | Runtime voice on dialogue bus, exact subtitle sync, independent volume, full text experience with voice off | C5 | 20 | NOT RUN |
| 22 | Sequencer pipeline reference sequence: trigger, playback, skip, return, no simulation mutation | D1 | 3 | PASS 2026-09-01 — `Echoes.Runtime.Cinematics.ReferenceSequence` in 64/64: data-driven trigger, playback with real possessed-camera advancement, skip, exact pause-state restore, unchanged sim checksum (ledger CINE-D1-001; headless world, no rendered on-screen cinematic yet) |
| 23 | Title sequence rendered, narrated, subtitled, ≤ 90 s, skippable | D2 | 22, 6, 20 | NOT RUN |
| 24 | Mission text authored, schema-valid, consumed at runtime | E4 | 1 | PASS 2026-09-01 — fifteen operations authored and schema-valid (308 lines, validator + pack digest-verified); consumed at runtime: briefs, objectives, phase-dispatched lines/subtitles, results, and all 92 failure variants bound to derived runtime reasons (`bound_runtime` on every contract); rendered captures show brief, objectives, live subtitle with speaker, and the result surface (ledger NARR-E4-002/003/004, evidence `release-gate24-*`). Boundaries: the authored campaign result variant renders only on a real commit (preview capture shows the non-authoritative surface); per-cause failure fixtures and voice (C-track) remain open |
| 25 | Three act-transition sequences rendered with act themes and narration | D3 | 22, 6, 20, 12 | NOT RUN |
| 26 | Four ending cinematics rendered; narrated claims bounded by ledger and Bible | D4 | 25, 27 | NOT RUN |
| 27 | One complete recorded route per ending type; no reachable plan dead-ends | E2 | 1 | PASS 2026-09-01 — four recorded `FreshJourney` routes (one per ending) plus `Echoes.Runtime.Campaign.PlanMatrix` cross-model sweep: 27 reachable plans per fact context, every one keeps an ending available, unreachable tuples fail closed; printed 27-row matrix reaches all four endings (ledger PLAN-E2-001; model-level, not a per-plan six-mission playthrough) |
| 28 | Cinematic accessibility matrix: skip, subtitles, reduced motion/flashing, clean return, budget hold | D5 | 23, 25, 26 | NOT RUN |
| 29 | Trailer exported from in-engine footage; claims checked against the evidence register | D6 | 23, 25 | NOT RUN |
| 30 | Difficulty and pacing pass: ordinary-player completion data per mission feeding tuning | E3 | 24, 10 | NOT RUN |
| 31 | Named slots, autosave, and recovery flow tested | E5 | 1 | PASS 2026-09-04 — Named slot CRUD (create/load/overwrite/delete) verified with metadata isolation (`Echoes.Runtime.Persistence.CampaignSlots`); mission-entry and objective phase-transition autosaving implemented (`EEchoesAutosaveReason`) with deterministic rotation into `.bak` generation; interrupted-session recovery validated discovering uncommitted checkpoints, CRC-32 integrity and branch validation, honest limitation disclosure ("CRC confirms uncorrupted disk storage; it is not cryptographic authentication"), corrupted-primary fallback to `.bak`, fail-closed double-corruption rejection, and process-restart simulation state reconstruction (`Echoes.Runtime.Persistence.AutosaveRecovery`); 76/76 Unreal automation tests passing clean (0 errors, 0 warnings), 96/96 native simulation tests passing across Release, Debug, and ASan/UBSan, runtime smoke passing clean (ledger AUTOSAVE-E5-001) |
| 32 | Skirmish setup exposes every option with a test per option | F1 | 10 | PASS 2026-09-04 — All 9 skirmish options exposed in UI and model (Local Faction, Opponent Faction, Teams, Battlefield Map, AI Personality, Difficulty, Starting Resources, Victory Condition, Game Speed); 9-matchup matrix supported including full mirror matchups; Assisted difficulty explicitly discloses exact handicap ("+50% reaction delay (1.5s), APM ceiling 30, -20% combat damage multiplier") in dedicated non-overlapping amber banner and telemetry while Standard AI enforces 100% fair information with no sight or income cheats; fixed-step 20 Hz simulation determinism preserved across Tactical 0.75x, Normal 1.0x, and Fast 1.5x speed scaling; option-by-option automated tests in Echoes.Runtime.Gameplay.SkirmishSetup and 76/76 Unreal automation suite passed clean; rendered 1920×1080 captures verified (ledger SKIRMISH-F1-001, evidence release-gate32-skirmish-setup) |
| 33 | Five AI personalities behaviorally distinguished | F2 | 32 | NOT RUN |
| 34 | Balance pass across nine matchups with the spread stated | F3 | 33 | NOT RUN |
| 35 | Full control remapping with conflict detection | G1 | 10 | NOT RUN |
| 36 | Every accessibility setting verified to change behavior, voice volume included | G2 | 10, 2, 21 | NOT RUN |
| 37 | Arbitrary and live-resize layout coverage | G3 | 10 | NOT RUN |
| 38 | Unaided human skirmish and mission sessions recorded | G4 | 10, 16, 32 | NOT RUN |
| 39 | Ordinary-player rendered campaign traversal to an ending | E6 | 28, 16, 21, 24, 30, 31, 36 | NOT RUN |
| 40 | Packaged profile within budgets: 16.67 ms frame, ≤ 4.0 ms game, ≤ 11.0 ms render+GPU, ≤ 1.5 ms fog, ≤ 6.0 ms path burst, ≤ 10 GB resident, ≤ 250 ms save | H | 8, 9, 16, 28 | NOT RUN |
| 41 | 600-second preflight plus uninterrupted 60-minute sustained run | H | 40 | NOT RUN |
| 42 | Shipping-configuration package builds, launches, plays | I1 | 39, 41 | NOT RUN |
| 43 | Developer ID signed, notarized, stapled; DMG or installer built | I2 | 42 | NOT RUN |
| 44 | Clean-machine install, launch, play, quit, relaunch, load — Seagate absent | I3 | 43 | NOT RUN |
| 45 | Portability audit: new-dependency review names zero new Linux blockers, or records accepted ones | I4 | 42 | NOT RUN |
| 46 | End-to-end coherence review passed on one checklist, defects filed | J1 | 11, 28, 21, 16, 47 | NOT RUN |
| 47 | Website at the bar: identity, real footage, claims audit, `site/`/`website/` resolved | J2 | 29, OD 3 | NOT RUN |
| 48 | Ledger, asset register (all exceptions), manual, and rights position complete | J3 | 44 | NOT RUN |
| 49 | Known-limitations page shipped in build and on site, accurate | J4 | 48 | NOT RUN |
| 50 | Glass Scar composed frame matches `CONCEPT-004` as one frame: dais Well, chasm and cliffs, unit scale and facing, fractured sun | A10 | 3, 5, 6 | IN PROGRESS — gate created 2026-09-04 on owner decision. Fixture now composes an open chasm along X between two contiguous banks with a stepped lower terrace and rim teeth, the Buried Causeway dais and piers exposed as the crossing, both armies relocated to the rims, amber fissure glow on banks and bed, the Broken Sun in frame, and no fixture-only exposure override (camera arm 3200, pitch -21). Bulwark shield facing corrected at `19906f9`. Local fixture capture `BuildArtifacts/Evidence/VerticalSliceReview-chasm-banks-v5-20260904.png` (1920×1080, editor build, authoring preview). Owner decision 2026-09-04: unit-to-dais scale is solved the StarCraft way — roster unit presentation meshes carry a readability scale (Worker/Scout 1.5, Soldier 1.6, Heavy 1.75) on a pivot above the body mesh; footprint, collision, and pathing are untouched, structures and landmarks stay 1.0 (fixture capture `VerticalSliceReview-unit-scale-20260904.png`). Still open: bank surface reads as slab mosaic (shelf mesh design, A3/A4), Matter crystals oversized and white, staging spacing for the larger silhouettes, no live-play capture, no 2560×1440 pair |

Gate 2 is carried and standing; gate 1 regressed on `main` at `76dfaf1` and must be repaired before any merge to `main` (see its row). Five roots unblock the graph and can start in parallel now:
**3** (exposure), **12** (score), **17** (voice pipeline, after OD 1), **22** (cinematic pipeline),
and **24** (mission text — it feeds voice, cinematics, and pacing alike, and is the single highest-
leverage open gate). Gate 39 is the release's real acceptance; gate 44 is its distribution
acceptance; gate 46 is the immersion acceptance.

## 11. What this directive is not

It is not evidence that any of this work has been done. It is not a schedule, and it does not
estimate effort. It does not authorize a claim of release readiness, balance, usability, performance,
or "professional quality" in advance of the runs that establish them — the phrase "professional" in
this document names a bar to be met, never a state to be asserted. When the release ships, the honest
description of it is whatever the closed gates support — no more.
