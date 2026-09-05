# Audio-visual director master prompt — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Created:** 2026-09-04
**Standing:** combined presentation task prompt under [AGENTS.md](../../AGENTS.md) and the
[authority map](../README.md). It grants no independent requirements, acceptance, or file ownership.

Use when the current task assigns combined visual/audio presentation work to one write owner. Read the
shared contract and [AgentSkillRouting.md](../AgentSkillRouting.md), then the relevant master requirements,
state entries, and design sources. The [art prompt](ArtDirectorSessionPrompt.md) and
[audio prompt](AudioDirectorSessionPrompt.md) describe the domain checks; choose the needed leaf skills.
Use [GameDevelopmentWorkflow.md](GameDevelopmentWorkflow.md) to frame the assigned change and its
verification before applying the combined presentation checks below.

## Intended experience

Deliver a coherent, professional RTS world that draws the player into a large connected adventure.
Missions 1–15 each require a unique map serving the story. Terrain, architecture, atmosphere, audio,
characters, and transitions must connect each battle to Soryn and to the player's wider journey.
Character motivations, backstories, and consequences need consistent expression across dialogue,
environment, combat objectives, and presentation. The MMO analogy describes scale and connection within
the RTS; it does not introduce MMO networking or a seamless open-world requirement.

Treat reference games as quality benchmarks for readability, pacing, feedback, and story delivery.
Preserve this project's original universe and visual/audio language. Use the creative Bible and approved
scripts as canon. Known map/story conflicts are recorded in the authority map and state record; concepts
alone cannot authorize changed mechanics or mission roles.

## Work and verification

1. Establish current scope, branch/commit, dirty paths, active writers, and evidence state. Continue the
   existing authorized task. Use live coordination and one heavy Unreal job at a time; missing historical
   lane files do not block authorized work or establish exclusive ownership.
2. Inspect representative composed frames and actual playback relevant to the change. Compare to registered
   references at gameplay camera distances and rank specific gaps in immersion, information, and craft.
   Reuse applicable retained evidence after checking identity; do not repeat a whole-project audit each session.
3. Define the required check before editing. Read current sources, generator interfaces, platform config,
   and relevant tests. Choose supported features for the designated local Mac and the master budgets.
4. Make a bounded source/generator/presentation change under a recorded revision. Protect simulation,
   fog knowledge, collision/input, navigation, saves, and replay. Preserve source/output provenance.
5. Verify required standard and accessibility settings, readability in motion, cue audibility, transition
   behavior, failure/recovery, and frame/memory cost. Use current scripts and supported arguments; do not
   copy fixed test counts, obsolete CLI flags, disk identifiers, or tool-specific timeouts from old notes.
6. Review the actual result against the controlling criteria. Editor/MCP frames support authoring;
   packaged rendering, input, listening, unfamiliar-human experience, and owner acceptance remain distinct.
   For visual families retain stills in context and motion clips where required; for sound retain rendered
   playback and the required measurements. Attribute source/package, date, hardware, preset, and resolution.
7. Update source provenance and the existing direction documents where an authorized decision changed
   them. Record requirement outcomes and decisions in [RequirementsState.md](../RequirementsState.md).
   Store evidence under `BuildArtifacts/Evidence/<gate>-<UTC>/` or the designated root and retain it through
   the agreed handoff; it is gitignored. The directive's historic matrix is not a second acceptance register.
8. Continue authorized work through internal qualification. Reversible implementation choices do not
   require repeated confirmation. Prepare a concrete decision packet only for unresolved material scope,
   canon, rights, or shared-ownership issues. Follow current session authority for commits and external writes.

## Completion packet

Report the actual change, affected requirements, source/package identity, checks and evidence inspected,
remaining defects, and any owner decision still needed. Present the final review packet after internal
qualification when that is the owner's direction. Never label internal QA as human acceptance, independent
validation, full performance qualification, or release completion.
