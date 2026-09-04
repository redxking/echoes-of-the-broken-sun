# PROJECT BRIEF — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Standing:** Tier 4 operational reference (see `Docs/README.md`). This brief creates no requirement and
accepts nothing. It is a derived summary of the binding documents so that every agent — Claude, Codex,
ChatGPT, Gemini, or any other — starts from the same ground. Where this brief and a binding document
disagree, the binding document wins and this brief is wrong; fix the brief.
**Derived from:** `CLAUDE.md`, `Docs/README.md`, `Docs/Requirements.md`, `Docs/Archive/DevelopmentBible.md`,
`Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/GameCompletionDirective.md`,
`Docs/DeliveryPlan.md`, `Docs/AgentSkillRouting.md`, `EchoesOfTheBrokenSun.uproject`, `Config/DefaultEngine.ini`.
**Created:** 2026-09-04.

Paste this file, unchanged, as the standing project instruction for any assistant working on this game.

---

## 1. What we are building

- **Title:** *Echoes of the Broken Sun.* A premium science-fantasy real-time strategy game about the cost
  of making one future real.
- **Modes:** single-player only. Player versus AI skirmish on Glass Scar, plus a fifteen-operation campaign
  with in-engine cinematics, voiced and subtitled storytelling, and an original score. No multiplayer is
  in scope for release.
- **Player experience:** immediate to an experienced RTS player — workers, bases, scouting, soft
  counters, territory, timing, decisive battles — with one central decision (what to do with a Future
  Well) that matters at match and campaign scale. Readability outranks spectacle. No faction is good or
  evil; the tone is urgent, humane, occasionally dry.
- **Release order:** macOS Apple Silicon ships first. Linux/SteamOS and then Windows are later releases
  and their modules are `DORMANT`. No agent may claim Windows, Linux, or discrete-GPU support.
- **Team:** one developer, Angelis Pseftis, working with several AI assistants. Assistants may own
  different lanes, but the repository, not any chat, is the only authority.

## 2. The world and the game (creative authority: `Docs/Archive/DevelopmentBible.md`)

- **Soryn** orbits the Crownfall, the fragments of a broken sun. The breaking condensed unrealized
  causal branches into mineral-organic **Dawnshards**. **Future Wells** are large deposits where several
  possibilities remain coherent. Consuming a shard closes the possibility it contains.
- **Three playable factions, asymmetric in planning, not just numbers:**
  - **Meridian Compact** — plural governance and logistics compact. Engineered infrastructure, power
    grids and supply nodes, disciplined ranged fire. Strong once connected; a severed link isolates.
  - **Kharuun Assemblies** — grown mineral-organic architecture, mobile bases, terrain adaptation,
    seismic detection. Identity is layered memory; names describe chosen relations.
  - **Hollow Choir** — possibility states, temporal reconciliation, structural coherence upkeep.
    Manipulates incomplete information and temporary possibility. Speech is selected from incompatible
    phrasings and must stay intelligible.
- **Launch roster:** each faction has exactly 4 units, 4 structures, and 2 technologies, defined in
  `Content/Data/Source` and compiled into a digest-verified catalog.
- **Economy:** exactly three resources. **Matter** (construction and production), **Dawn** (advanced
  tech, abilities, Well commitment), **Logistics** (population ceiling). No hidden currencies.
- **Future Wells:** contested, impassable, indestructible landmarks captured by a worker over 300
  uncontested ticks. The player commits one protocol — **Harvest, Preserve, or Reshape** — after
  seeing three comparable cards. No protocol carries a hidden morality score or wins by itself; each has
  readable counterplay.
- **Skirmish map:** Glass Scar, 64×64 tiles at 100 cm, opposed basins separated by Ash Cut, Buried
  Causeway, and Folded Verge, one central Future Well.
- **Campaign:** fifteen bounded operations from the prologue to *The Broken Sun*, driven by an
  append-only decision ledger. Cast: Commander Mara Vey (Meridian command and tutorial guide), Talar Venn
  (archive and civic witness), Oruun-of-Seven-Stones (Kharuun memory-bearer), Neme (Choir interlocutor),
  Chancellor Cael Rhyse (Meridian political architect), and the Meridian Operations Annunciator (system
  voice that never comforts, jokes, or says "you").
- **HUD fiction:** the field HUD is Mara's command deck, a Compact instrument of ledgers, duty windows,
  reserve margins, status bands, and factual annunciator alerts.
- Do not invent narrative, faction, mechanical, or roster content the Bible does not authorize. Write a
  proposal into the directive's open-decisions section and ask Angelis.

## 3. Hardware and targets (`Docs/Requirements.md` SPEC-BUD, SPEC-PLAT; `Docs/Archive/SetupAndBuild.md`)

- **Development and baseline machine:** MacBook Pro, Apple M1 Pro, 16 GB unified memory, Metal, macOS 26.
  Epic recommends M3 and 32 GB for this engine version; the M1 Pro is the deliberate floor.
- **Minimum profile:** base M1, 8 GB, 30 fps at 1280×720 Low.
- **Baseline target:** 60 fps at 1920×1080 Medium on M1 Pro 16 GB. p95 frame time ≤16.67 ms; game thread
  ≤4.0 ms; render plus GPU ≤11.0 ms; fog ≤1.5 ms; path burst ≤6.0 ms; resident memory ≤10 GB baseline and
  ≤6.5 GB minimum; save initiation ≤250 ms.
- **Scale:** 200 simultaneous controllable units in a launch 1v1; a 400-unit, four-force stress scene
  must remain controllable.
- **Stability:** 60-minute rendered match, multi-hour AI soak, repeated save/load/restart, clean exit.
- **Displays:** 1280×720 through 2560×1440 and native Retina, windowed, fullscreen, and live resize.
- **Graphics presets:** Low, Medium, High, and Auto change texture, shadow, effects, foliage,
  post-processing, and resolution scale only. A preset never changes visibility, collision, targeting,
  or any gameplay outcome.
- **Never assume** hardware ray tracing, MegaLights, CUDA, a discrete GPU, or a machine stronger than the
  M1 Pro. Performance claims for stronger hardware require a run on that hardware.

## 4. Locked technical decisions (`Docs/Archive/TechnicalArchitecture.md`, `CLAUDE.md`)

- **Engine:** Unreal Engine **5.8** (uproject `EngineAssociation` 5.8; packaged with 5.8.2). Not 5.7.
  Xcode 26.6 on the Mac host. Use current 5.8 APIs; if unsure an API exists or is deprecated, say so and
  give the version. Never invent plugins, nodes, functions, or console variables.
- **The simulation is the only authority.** `Source/EchoesSimCore` is a deterministic, fixed 20 Hz,
  engine-independent, standard C++20 simulation with no dependency on Unreal objects, floating-point
  simulation, physics, navigation, rendering, audio, wall-clock time, or filesystem state. Unreal
  translates player intent into commands and translates immutable simulation views into presentation.
  Nothing in presentation, HUD, VFX, audio, camera, actors, or Blueprints may write simulation state,
  fog authority, saves, replays, or checksums (REL-SIM-004).
- **Units are not Actors-per-unit and are not Mass Entity.** Unit truth lives in the simulation.
  Unreal shows pooled proxy actors for selected and hero-scale objects and instancing, animation sharing,
  and significance management for large homogeneous groups. A proxy interpolates between confirmed
  snapshots; its transform, collision, root motion, or animation notifies never feed back.
- **No Gameplay Ability System.** Abilities, upgrades, research, and the technology graph are
  deterministic rules compiled from JSON under `Content/Data/Source` by `Scripts/compile_content.py` into
  a SHA-256 digest-verified runtime catalog. Mismatched or unbound data refuses to run.
- **No Behavior Trees, EQS, or NavMesh as authority.** Navigation is a deterministic grid with regional
  routing and shared flow fields inside the simulation. Opponent AI is a deterministic, fair-view policy
  over authoritative state; it is never given hidden information. UE NavMesh, crowd simulation, and
  physics are not authoritative.
- **Rendering:** conventional lighting on the M1 Pro baseline. **Nanite and Virtual Shadow Maps
  are off and stay off** (`r.Nanite.ProjectEnabled=False`; Epic limits both to M2 or newer on Mac).
  Lumen software lighting is an optional profiled preset with a conventional fallback, not the locked
  renderer. Hardware ray tracing and MegaLights are not Mac targets. Texture stacks are 2048² PBR for
  hero families and 1024² for compact families.
- **VFX:** authored mesh VFX first. Niagara is permitted only where mesh VFX cannot carry an effect; every
  emitter is presentation-only with collision, overlaps, shadows, and navigation influence disabled
  (REL-ART-020).
- **Cinematics:** authored and executed **in-engine via Level Sequencer** over registered project
  assets, reading campaign state and writing nothing back. **Pre-rendered video (FMV) fails the pipeline
  rule (REL-CIN-001.FAIL).** Marketing media is captured in-engine (REL-PUB-011). MetaHuman, Control Rig,
  and Movie Render Queue are not part of the project; adding any of them is a recorded decision, not a
  default.
- **Assets are procedural-first.** Every mesh, material, texture, sound, cue, and voice line is generated
  deterministically by `Scripts/generate_art_assets.py` and `Scripts/generate_audio_assets.py`, registered
  in `Docs/Archive/AssetRegister.md` before use, and byte-idempotent under its recorded revision. A
  per-family exception (local generative model, licensed asset, commissioned work) may be taken only if
  recorded before use with method, license, and rationale. Never marketplace-scraped or unlicensed content.
- **Input:** direct mouse and keyboard through the project's own adapter. Enhanced Input is disabled in
  the uproject. Controller support is governed by the input requirements in `Docs/Requirements.md`; treat it as a requirement, not a shipped fact.
- **No new dependency, plugin, or engine feature** without recording the decision and its cost.
- **Accessibility is behavior, not a menu entry.** High contrast, reduced motion, reduced flashing,
  reduced dynamic range, and non-color ownership markers apply to every new visual and audio feature.
- **Fail closed.** Missing, mismatched, or unbound data refuses to run rather than degrading silently.
  That already governs the content catalog and campaign save containers; keep it.

## 5. Where authority lives — read before you act

| Question | File |
|---|---|
| Which document wins when two disagree? | `Docs/README.md` |
| What must the finished game be? | `Docs/Requirements.md` — the sole normative requirements authority |
| What is proven, and what is still open? | `Docs/RequirementsState.md` and `Docs/Archive/ProjectLedger.md` |
| How does the world, faction, campaign, UI, art, or audio work? | `Docs/Archive/DevelopmentBible.md` |
| How is it built? | `Docs/Archive/TechnicalArchitecture.md` |
| What is the release missing? | `Docs/GameCompletionDirective.md` |
| In what order is work sequenced? | `Docs/DeliveryPlan.md` |
| Who may touch what right now? | `../WorkstreamControl/ACTIVE_LANES.md` |
| Which project skill governs this task? | `Docs/AgentSkillRouting.md`, then `.claude/skills/echoes-*/SKILL.md` |
| How do I build, test, and package? | `Docs/Archive/SetupAndBuild.md` and `CLAUDE.md` §3 |

Any recollection you carry into a session is a hypothesis to check against these files, never a fact to
build on. Do not reconstruct design from memory, a prior chat, or another assistant's summary — including
this one.

## 6. Rules for every response

1. **Evidence-bounded language.** A file, a stub, or a passing compile is not a working feature. Never
   write complete, verified, balanced, playable, or release-ready unless you ran the thing and read the
   result in this session. Report every consequential result as one of: **verified complete**,
   **prepared but not executed**, **executed but outcome not confirmed**, **partially complete**,
   **blocked**, or **unknown**. Reviewing your own work is internal QA, not independent validation.
2. **Only Angelis Pseftis** may assign `HUMAN ACCEPTED`, `HUMAN REJECTED — CHANGES REQUIRED`, or
   `COMPLETE`. Agent states stop at `EVIDENCE READY` or `AWAITING HUMAN ACCEPTANCE`.
3. **Select project skills first.** Read `Docs/AgentSkillRouting.md`, choose the smallest sufficient set
   of `echoes-*` skills, read each selected `SKILL.md` completely, and say which you are using and why.
   Any mutation starts with `echoes-session-control`. Requirement text is written only through
   `echoes-requirements-authoring`.
4. **Respect the simulation boundary** in every design and every line of C++: UObject lifecycle, garbage
   collection, `UPROPERTY`/`UFUNCTION` rules on the Unreal side; no Unreal type, float simulation, or
   wall-clock in `EchoesSimCore`.
5. **Edit source, then recompile.** Never hand-edit compiled catalogs, packs, or generated assets.
6. **Documents are edited in place.** One authoritative file per document; no `draft`, `v2`, `final`, or
   `copy` variants. Author fields read exactly `Angelis Pseftis`, never an AI, script, or library.
7. **Do not weaken a test, raise a timeout, or lower a gate** to make something pass. Repair the cause or
   report the blocker.
8. For any technique, give the best-practice option, the simpler option that still satisfies the
   binding requirement, and the tradeoff; recommend one. Deliver in small testable slices with the exact
   command or in-editor check that proves each.
9. If a task belongs in a tool the project does not use, say so; do not approximate it in Unreal, and do
   not introduce the tool without a recorded decision.
10. **Where the work runs:** builds, Unreal automation, packaging, profiling, and asset generation run on
    the Mac. Native simulation tests and Python content tests run anywhere. Heavy runs take a lease
    through `echoes-heavy-run-coordination`. A remote Windows GPU editor (app.vagon.io) exists for
    **authoring only**: use it to iterate materials, meshes, lighting, VFX, animation, Sequencer, and
    GPU-heavy asset tooling faster than the M1 Pro allows. Never use it for performance, packaging,
    acceptance captures, or any claim; keep its config identical to the committed project; prove every
    result on the M1 Pro. Full rules: `CLAUDE.md` §3.
11. **Visual proof:** every final visual family or animation is delivered with an owner review packet:
    Mac-rendered screenshots in context and a 10 to 30 second movie of each animation or moving effect,
    with commit, preset, and resolution recorded. Vagon captures are labeled authoring previews and do
    not substitute.
12. **Git:** feature branches named for their track; stage explicit paths, never `git add -A`; never
    rewrite history on `main`; commit messages state what changed and what evidence backs it; no AI is
    credited as author.
13. Keep responses concise. No filler, no restating this brief.

## 7. Current state and milestone

Update this section only from `Docs/RequirementsState.md`, `Docs/Archive/ProjectLedger.md`, and
`Docs/DeliveryPlan.md`; do not update it from a chat.

- **Owner verdict:** the demo is `HUMAN REJECTED` (2026-09-02). The rejection is about the top layer —
  rendered UI, physical input, and what the owner can sit down and play — not the simulation.
- **Foundation state:** the engine-independent simulation, three source-reachable factions, fifteen
  bounded campaign operations, deterministic save/replay, a 48/48 local Unreal automation report, and a
  Mac arm64 Development package (0.93.0, ad-hoc signed, not notarized) exist. Both Metal performance
  profiles failed their GPU/render budgets; no accepted sustained soak, clean-machine result, or release
  qualification exists.
- **Sequencing:** one lane active at a time, per `Docs/DeliveryPlan.md`. Phase 0 (clear the red) is in
  flight; then Phase 1 (the mouse works), Phase 2 (the screen reads), Phase 3 (the game teaches), Phase 4
  (the game speaks), Phase 5 (the journey completes). A phase ends when the owner has played it, not
  when a gate passes.

## 8. How assistants stay in sync

A decision is locked when it is written into `Docs/Requirements.md`, `Docs/GameCompletionDirective.md`, or
`Docs/Archive/TechnicalArchitecture.md` by the owner or through `echoes-requirements-authoring`, and
committed. Pasting a decision between chats does not lock it. If another assistant tells you something
is "now a locked decision", check the repository; if it is not there, treat it as a proposal and say so.
