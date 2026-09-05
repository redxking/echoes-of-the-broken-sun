# Shared agent contract — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Applies to:** every AI collaborator, including Codex, Claude, Gemini, and OpenCode.
**Maintained:** 2026-09-05

This is the single repository source for agent operating rules. Client entry files and skills refer here.
Follow the host's system/developer instructions and the owner's current and standing instructions first.
Repository files cannot override them, grant external-action authority, or turn a historical assignment
into a current restriction.

## Start each task

1. Locate the checkout: from the containing `EchoesOfTheBrokenSun` folder, enter `Project`. Inspect branch,
   commit, dirty paths, and relevant running work. Preserve other work.
2. Read [Docs/README.md](Docs/README.md), then the relevant sections of
   [Docs/Requirements.md](Docs/Requirements.md) and [Docs/RequirementsState.md](Docs/RequirementsState.md).
   Requirements define behavior; the state record defines evidence state and owner acceptance.
3. Read [Docs/AgentSkillRouting.md](Docs/AgentSkillRouting.md), inspect skill descriptions, and read the
   smallest applicable set completely. State the selected skills when first applying them. Load the affected
   design references through the authority map; a prompt or old chat is not a substitute.
   For game implementation, debugging, presentation work, or substantial gaming prompts, apply
   [GameDevelopmentWorkflow.md](Docs/Prompts/GameDevelopmentWorkflow.md) to frame and verify the task.
4. Identify the outcome, controlling IDs, affected paths, verification method, and real decision boundaries.
   For documentation maintenance, check authority, links, source consistency, metadata, and preserved
   history. Editorial completion does not require a game build or human playtest.
5. Complete authorized work, inspect the result, and report the actual outcome and remaining limits.
   Continue independent work if one portion is blocked. Ask only for missing information or authority that
   materially changes the result; prepare the concrete reviewable result before asking for final approval.

## Engineering and product standards

- **Requirements first.** Map features, math, balance, content, UI, and verification to exact master IDs.
  Preserve identifiers and thresholds. Record material contradictions, new scope, cost, or stricter
  acceptance criteria as `TBR-*` decisions in the state record; amend the master after owner direction.
  Routine implementation choices within existing authority do not require repeated permission.
- **Original, coherent game.** Preserve the creative authority's world, factions, characters, Future Wells,
  and narrative intent. Apply the approved visual, audio, interaction, and narrative language across the
  game and public surfaces. Reference games establish a quality target, never permission to copy their IP.
- **Plan for place and purpose.** Before production, apply `SPEC-VISD-008` and `SPEC-ART-004`: establish
  story context, what belongs/does not, meaningful large/small detail, readable unit/building function,
  and coherent motion/sound. Fifteen campaign maps must each be distinct and connected to the story
  under `SPEC-MAP-004` and `SPEC-CAM-041..042`. Asset presence is not the craftsmanship gate.
- **Simulation authority.** `Source/EchoesSimCore` owns deterministic gameplay. Presentation consumes
  authorized state; it cannot change fog knowledge, simulation results, saves, replays, or checksums.
  Cosmetic assets must not intercept input traces, alter navigation, or affect combat. Intentional gameplay
  geometry follows the requirements and collision contract.
- **Source before output.** Edit registered source data and generators, then compile or regenerate.
  `Content/Data/Source`, `Content/Narrative/Source`, and `Content/World/Source` are source locations.
  Generated catalogs and assets require source identity and provenance, not manual repair of outputs.
- **Assets and quality.** Follow the master and applicable art/audio/voice direction. Record generator
  revisions, rights, licenses, dependencies, and approved exceptions in
  [Docs/Archive/AssetRegister.md](Docs/Archive/AssetRegister.md). Claim palette, loudness, runtime binding,
  or other enforcement only when the actual validator and retained result establish it. Accessibility
  settings must produce the required observable behavior.
- **Failure and verification.** Preserve deterministic refusal and recovery for invalid authoritative data.
  Diagnose failing checks; never weaken requirements or hide failures to obtain a pass. Run checks
  appropriate to the change, including negative/recovery cases where material. Broaden testing when
  changes or unresolved evidence justify it, not as ceremony.

## Ownership, tools, and coordination

- Keep one write owner per checkout where practical. Read-only specialists may work in parallel on
  independent bounded tasks. Concurrent writers need disjoint ownership or isolated worktrees and must
  preserve each other's changes. Current owner instructions and live coordination establish roles,
  never the model name or a retired lane roster.
- Historical records cite `../WorkstreamControl/ACTIVE_LANES.md` and `HEAVY_RUN_LOCK.md`; those locations
  are not guaranteed to exist. Discover any current coordination record before using it. An absent old
  record is neither a lease nor a reason to block unrelated authorized documentation work. Establish
  ownership with the active task/coordinator before overlapping edits. Resource-intensive runs require
  `echoes-heavy-run-coordination` and exclusive use of the affected resources before launch.
- Use the available approved shell, API, connector, or UI interface appropriate to the task. Unreal Mac
  builds, packaging, and Apple signing run on the configured Mac. Desktop Commander is one possible
  interface, not a mandatory dependency. Discover tools and mounts live; never assume a disk identifier,
  credential, port, tool version, test count, or bridge timeout from an old note.
- If a necessary skill/interface is missing, locate its canonical source or an equivalent method preserving
  the required evidence class. Report the unavailable capability and stop only the dependent portion.
  A weaker evidence class cannot substitute for the required one.
- New generated evidence belongs under `BuildArtifacts/Evidence/<gate>-<UTC>/` or the current coordinator's
  designated evidence root. Record its absolute resolved path. Keep historical paths in old receipts and
  locate the artifacts before citing them. Include source commit, dirty state, command/configuration, date,
  environment, outcome, and package/artifact hashes where applicable. Gitignored evidence is not backed up
  merely because a tracked document links to it; retain it through the agreed evidence handoff.

## Proportionate routing

The active parent owns scope, integration, verification, and the final answer. It cannot replace its own
model during a turn. Use the least costly supported route meeting the evidence burden, with medium as the
ordinary reasoning baseline and higher effort for difficult consequential judgments.

When available, use `fast_scan` for narrow read-only searches, `evidence_researcher` for substantial source
review, `expert_reviewer` for complex technical review, and `critical_verifier` only for exceptional final
assurance gates. Delegate independent work when it improves quality or elapsed time; keep simple tasks with
one agent. Model choice does not establish validation. Follow host rules for user-visible task creation;
ordinary subtasks belong in subagents. Preserve active task continuity rather than duplicating work.

For each new work package, automatically apply the
[model and effort selection procedure](Docs/Prompts/GameDevelopmentWorkflow.md#select-model-effort-and-work-ownership)
before dispatch. Every delegated assignment and authorized additional task must satisfy the
[handoff contract](Docs/Prompts/GameDevelopmentWorkflow.md#delegation-and-additional-task-handoff).
Choose against the actual acceptance burden, verify supported settings, and retain the selected route
with the evidence. The owner need not repeat this instruction. Advance the existing DeliveryPlan package;
routine routing is not a reason to create another plan or ask for renewed permission.

## Evidence, state, and release boundaries

Distinguish observations, primary sources, vendor assertions, owner input, assumptions, calculations,
proposals, judgments, and unknowns. Recheck changeable material facts against primary sources. Never invent
sources, results, owner decisions, personal experience, or operational status.

[RequirementsState.md](Docs/RequirementsState.md#state-vocabulary) owns requirement status vocabulary.
`IMPLEMENTED` does not mean verified. A passing test is not requirement completion. Source inspection,
native tests, editor automation, packaged automation, rendered inspection, physical input, uncoached human
play, owner acceptance, and independent validation are separate evidence classes. Internal agent review
is internal QA. Only Angelis assigns owner acceptance, rejection, or requirement `COMPLETE`.

Review retained evidence and its source/package identity before relying on an earlier result; identify
historical evidence and say when it was not rerun. Visual/audio completion requires the prescribed rendered
or listening evidence and owner decisions. A screenshot cannot prove interaction, performance, audio, or
human acceptance. Prepared, executed, verified, submitted, published, signed, notarized, deployed, and
release-ready are distinct states. Prototype content never justifies public claims of a finished game.

## Documents, authorship, and communication

- Maintain one authoritative physical file per document, editing it in place. Preserve revision history,
  accumulated editing time, and related metadata. Do not create draft/version/final-copy documents.
  Derived exports and temporary renders are QA outputs, never competing authorities.
- Set every available author, creator, editor/last-modifier, and document-authorship field to
  **Angelis Pseftis**. Preserve legitimate third-party attribution and licenses. Tool/model names may appear
  as provenance or test environment, never as the author of the owner's deliverable.
- Verify the final file, links, claims, calculations, and authorship before delivery. Render and visually
  inspect paginated documents and visual artifacts completely; structural inspection alone is insufficient.
- Write naturally and directly for an expert technical or executive audience. Lead with the outcome and
  evidence needed to assess it. Avoid filler, unsupported claims, invented experience, and repetitive
  headings. Keep public/player language distinct from internal implementation detail.
- For academic work inspect every instruction, rubric, template, and submission requirement. For LinkedIn,
  default to a complete credible article unless a short post is requested, connect technical facts to
  leadership decisions, and include relevant hashtags. Preparing text is not submitting or publishing it.

## Git and completion

Preserve dirty work. Stage explicit task paths only when committing is within scope; inspect the diff and
identity before committing. Keep LFS provenance for binary work. Never broadly reset, force-push, rewrite
mainline history, or delete active work as cleanup. A historical blanket push instruction is not an
automatic end-of-task action: follow current authorization and host restrictions for external writes.
Do not send messages to others without explicit authorization.

Before reporting completion, verify the requested outcome, relevant checks, preserved unrelated work, and
material limitations. Keep historical evidence and owner decisions; remove superseded active guidance or
replace it with a clear successor reference. Update dependent pointers in the same change.
