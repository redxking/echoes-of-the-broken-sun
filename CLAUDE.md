# CLAUDE.md — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Repository root:** `/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project`
**Current objective:** finish the game to a professional public release — player versus AI, Glass Scar skirmish plus all fifteen campaign operations, with professional visuals, the full original score and audio, voiced and subtitled storytelling, and in-engine cinematics. macOS ships first; Linux/SteamOS and then Windows follow in later releases.

**Start at [Docs/README.md](Docs/README.md)** — the authority map for this project's documents. It states,
in tier order, which document binds, which records acceptance, and which is history.

The binding definition of the finished game is **[Docs/Requirements.md](Docs/Requirements.md)** — the sole
normative requirements authority. **[Docs/RequirementsState.md](Docs/RequirementsState.md)** is the sole
lifecycle and evidence-state authority. `Docs/MovementAndBalanceRequirements.md` was merged into the master
on 2026-09-03 and is retained only as a historical source. The per-track specifications and the release gate
matrix are in **[Docs/GameCompletionDirective.md](Docs/GameCompletionDirective.md)**; read it before starting
any release work. It supersedes `Docs/Archive/Superseded/DemoReleaseDirective.md` (2026-09-01), whose accepted
gate evidence it carries forward. This file is the standing session contract; the directive is the work.

## Mandatory project-skill selection

Before analysis, editing, testing, building, GUI operation, or a completion claim, read
`Docs/AgentSkillRouting.md`. Inspect the project-skill names and frontmatter descriptions under
`.claude/skills/`, select the smallest sufficient set of `echoes-*` skills, and read every selected
`SKILL.md` completely before acting. State which skills are being used and why. If the task changes
materially, repeat selection before expanding the work.

Do not load or claim to use all skills by default. Use the most specific matching skill plus the routing
contract's required session, lane, heavy-run, GUI, traceability, evidence, or release prerequisites. If a
required skill or interface is unavailable, stop that affected path with the status required by the skill;
do not improvise evidence or silently fall back to a weaker workflow.

---

## 1. Read before you act

The repository is the only authority on how this game works. Do not reconstruct design from memory, from a
prior chat, or from another assistant's summary. Any recollection you carry into a session is a hypothesis to
check against these files, never a fact to build on:

| Question | File |
|---|---|
| Which document wins when two disagree? | `Docs/README.md` |
| What must the finished game be? | `Docs/Requirements.md` |
| How does the game, world, faction, campaign, UI, art, or audio work? | `Docs/Archive/DevelopmentBible.md` |
| How is it built — simulation, adapter, AI, net, save, replay? | `Docs/Archive/TechnicalArchitecture.md` |
| What is actually proven, and what is still open? | `Docs/RequirementsState.md` |
| What assets exist and under what provenance? | `Docs/Archive/AssetRegister.md` |
| How do I build, test, and package? | `Docs/Archive/SetupAndBuild.md` |
| Who is allowed to touch what right now? | `../WorkstreamControl/ACTIVE_LANES.md` |
| What is required for the demo, and what did the owner accept? | `Docs/Requirements.md` and `Docs/RequirementsState.md` |
| What is required for initial release? | `Docs/Requirements.md` |
| What is the release missing? | `Docs/GameCompletionDirective.md` |

Source data lives in `Content/Data/Source` (JSON, compiled into a digest-verified runtime catalog),
`Content/Narrative/Source`, and `Content/World/Source`. Edit source, then recompile — never hand-edit compiled
output.

## 2. Non-negotiable project rules

1. **Evidence-bounded language.** A file, a stub, or a passing compile is not a working feature. Never write
   that something is complete, verified, balanced, playable, or release-ready unless you ran the thing and
   read the result in this session. State the exact boundary of what you proved and what you did not.
2. **The simulation is the only authority.** `Source/EchoesSimCore` is deterministic and engine-independent.
   Presentation, HUD, VFX, audio, and camera never enter simulation state, fog authority, saves, replays, or
   checksums. Any new visual or audio work must disable collision, overlaps, navigation influence, and
   shadows where applicable, and must be spawned from authoritative state rather than predicting it.
3. **Procedural-first assets with recorded exceptions.** The default for every mesh, material, texture,
   sound, music cue, and voice line is deterministic generation by project code through
   `Scripts/generate_art_assets.py` and `Scripts/generate_audio_assets.py`, registered in
   `Docs/Archive/AssetRegister.md` before use and byte-idempotent under its recorded revision string.
   Where the generators provably cannot reach the professional bar, a per-family exception (locally run
   generative model, licensed asset, or original commissioned work) may be taken only if recorded before
   use with its method, license, and rationale — see the directive's asset policy. Never
   marketplace-scraped or unlicensed content.
4. **Documents are edited in place.** One authoritative file per document. No `draft`, `v2`, `final`, or
   `copy` variants. Author fields, where a format has them, read exactly `Angelis Pseftis` — never Claude, an
   AI, a script, or a library.
5. **Fail closed.** Missing, mismatched, or unbound data must refuse to run rather than degrade silently.
   That pattern already governs the content catalog and campaign save containers; keep it.
6. **Accessibility is a behavior, not a menu entry.** A setting that does not change behavior does not pass.
   High contrast, reduced motion, reduced flashing, reduced dynamic range, and non-color ownership markers
   apply to every new visual and audio feature you add.

## 3. Where the work runs

Builds, the Unreal automation suite, packaging, and asset generation run **on the Mac**, through Desktop
Commander (`start_process`, `interact_with_process`). The Cowork Linux VM cannot build this project and its
`$HOME/mnt` mount of the Seagate volume is unreliable. Native simulation tests and Python content tests can
run in either place.

```sh
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project"

./Scripts/test_sim.sh                                        # native EchoesSimCore, 3 configurations
./Scripts/test_content.sh                                    # content/schema/compiler suites
./Scripts/build_editor.sh                                    # Mac Development Editor + Game
env TMPDIR="$(getconf DARWIN_USER_TEMP_DIR)" ./Scripts/run_unreal_tests.sh    # full automation suite
./Scripts/package_macos.sh                                   # Mac arm64 Development package
./Scripts/profile_packaged_macos.sh                          # frame/GPU/memory profile
./Scripts/generate_art_assets.sh                             # regenerate registered meshes/materials
./Scripts/generate_audio_assets.sh                           # regenerate registered audio
```

### Four traps that produce failures unrelated to your code

1. **`TMPDIR` is unset** in the Desktop Commander shell, so `run_unreal_tests.sh` falls back to `/tmp` and
   every storage-touching test fails with `[ECHOES_TEST_STORAGE_NON_TEMP_ROOT]`. Always launch it as
   `env TMPDIR="$(getconf DARWIN_USER_TEMP_DIR)" ./Scripts/run_unreal_tests.sh`.
2. **The Seagate volume can drop out mid-build**, producing `cannot open file … Device not configured` and
   missing `.rsp` files. That is I/O, not a compiler error. Remount with `diskutil mount disk18s1` and wrap
   long builds in `caffeinate -dimsu`.
3. **The device bridge caps every call at about 60 seconds.** Launch long jobs detached —
   `nohup … </dev/null > /tmp/echoes-verify/<name>.log 2>&1 &` — then poll with `sleep 50; tail`. Bash
   block-buffers redirected stdout, so write explicit progress markers with `>>` if you need to watch phases.
4. **`Tests/Content/test_package_manifest_verifier.py` cannot run on Linux.** It pins
   `/opt/homebrew/bin/git` 2.55.0 and `git-lfs` 3.8.0. Its Linux failure is never a regression.

Storage discipline: stop prototype builds below 10 GiB free on the internal volume, restore 12 GiB before
large imports, 60 GiB before packaging. The Seagate volume must stay mounted.

## 4. Git and GitHub

Angelis has authorized this session family to keep GitHub current. Work on the Mac shell, where the
credential manager is configured; the Linux VM has no credentials, no `git-lfs` on `PATH`, and no delete
permission.

- Remote: `origin` → `https://github.com/redxking/echoes-of-the-broken-sun.git`. Mainline is `main`.
- Feature work goes on a branch named for its track, for example `release/visual-terrain-textures`. Merge to
  `main` only after that track's acceptance evidence exists and is recorded.
- Set identity per invocation rather than writing into the user's config:
  `GIT_AUTHOR_NAME="Angelis Pseftis" GIT_AUTHOR_EMAIL="44432751+redxking@users.noreply.github.com"` and the
  matching `GIT_COMMITTER_*`.
- **Stage explicit paths.** The working tree carries unrelated modifications and 11 GB of build artifacts.
  Never `git add -A` or `git commit -a`.
- LFS tracks `*.uasset`, `*.umap`, `*.wav`, `*.png`, `*.fbx`, `*.glb`, `*.flac`, `*.exr`. Confirm `git lfs
  status` before pushing binary asset work, and push LFS objects with it.
- Clear the injected `GIT_PAGER` before packager invocations; its reject-all policy trips on it.
- Never rewrite history on `main`, never force-push, and never delete a branch that a lane still holds.
- Commit messages state what changed and what evidence backs it. Do not credit Claude or an AI as author.

## 5. How to run a session

1. Read `Docs/GameCompletionDirective.md`, then `ProjectLedger.md`'s *Current known limitations* and
   *Immediate next task*, then `../WorkstreamControl/ACTIVE_LANES.md`.
2. Pick the next unblocked item from the directive's gate matrix. If two tracks touch the same files, take a
   lane lease in `ACTIVE_LANES.md` first; if the work is isolated, a branch is enough. Use judgment, and say
   which you chose and why.
3. Build the smallest slice that is genuinely testable, and test it before claiming it.
4. Record the result: update `ProjectLedger.md`'s evidence register, `AssetRegister.md` for any new asset
   family, and the directive's gate matrix. Evidence logs belong under `../WorkstreamControl/evidence/`.
5. Commit, push, and report: what you built, what you ran, what passed, what is still unproven.

Report the state of every consequential result as one of: **verified complete**, **prepared but not
executed**, **executed but outcome not confirmed**, **partially complete**, **blocked**, or **unknown**.
Reviewing your own work is internal QA, not independent validation.

## 6. Standing limits

- Do not weaken a test, raise a timeout, or lower a gate to make something pass. Repair the cause or report
  the blocker.
- Do not claim performance, balance, usability, notarization, or release readiness without the run that
  establishes it.
- Do not invent narrative, faction, or mechanical content that the Bible does not authorize. If the demo
  needs something the design does not cover, write the proposal into the directive's open-decisions section
  and ask Angelis.
- Do not add a dependency, plugin, or engine feature — Nanite and Virtual Shadow Maps stay off on the M1 Pro
  baseline — without recording the decision and its cost.
