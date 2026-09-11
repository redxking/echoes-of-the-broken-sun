# Shared agent contract — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Applies to:** every AI collaborator, including Codex, Claude, Gemini, and OpenCode.
**Maintained:** 2026-09-11

This is the single repository source for agent operating rules. Client entry files and skills refer here.
Host system/developer instructions and the owner's current and standing instructions come first. Repository
files cannot override them, grant external-action authority, or turn a historical assignment into a current
restriction.

## Start each task

1. **Packet.** From the containing `EchoesOfTheBrokenSun` folder enter `Project`. Read
   [Docs/CONTEXT.md](Docs/CONTEXT.md), the generated session packet (`python3 Scripts/build_context.py`
   refreshes it). It carries checkout identity, dirty paths, the
   [active execution state](Docs/DeliveryPlan.md#active-execution-state), package routing and pause boundary.
   Preserve other work. Documentation-only maintenance respects the boundary but needs no execution preflight.
2. **Records.** Fetch only the requirement records you touch: `python3 Scripts/req.py both <ID>...`
   (also `family`, `search`, `section`). Never open [Requirements.md](Docs/Requirements.md) or
   [RequirementsState.md](Docs/RequirementsState.md) whole. Requirements define behavior; the state record
   defines evidence state and owner acceptance. [Docs/README.md](Docs/README.md) maps every other authority.
3. **Skills.** Select the smallest applicable set through [AgentSkillRouting.md](Docs/AgentSkillRouting.md),
   read each body completely, and name them when first applied. Load design references through the authority
   map; a prompt or old chat is not a substitute. For game implementation, debugging, presentation work, or
   substantial gaming prompts apply [GameDevelopmentWorkflow.md](Docs/Prompts/GameDevelopmentWorkflow.md).
4. **Frame.** State the outcome, controlling IDs, affected paths, verification method, and real decision
   boundaries. Editorial completion does not require a game build or human playtest.
5. **Finish.** Complete authorized work, inspect the result, record per-ID outcomes with
   `python3 Scripts/record_state.py`, update the DeliveryPlan active-state table if the next action changed,
   rebuild the packet, and report the actual outcome and remaining limits. Continue independent work when one
   portion is blocked; ask only for missing information or authority that materially changes the result, with
   the concrete reviewable result prepared first.

## Engineering and product standards

- **Requirements first.** Map features, math, balance, content, UI, and verification to exact master IDs and
  preserve identifiers and thresholds. Record material contradictions, new scope, cost, or stricter acceptance
  as `TBR-*` decisions in the state record; amend the master after owner direction. Routine implementation
  choices within existing authority need no repeated permission.
- **Original, coherent game.** Preserve the creative authority's world, factions, characters, Future Wells,
  and narrative intent, and apply the approved visual, audio, interaction, and narrative language across the
  game and public surfaces. Reference games set a quality target, never permission to copy IP.
- **Plan for place and purpose.** Before production apply `SPEC-VISD-008` and `SPEC-ART-004`: story context,
  what belongs, meaningful detail, readable function, coherent motion and sound. Fifteen campaign maps must be
  distinct and story-connected under `SPEC-MAP-004` and `SPEC-CAM-041..042`. Asset presence is not craft.
- **Simulation authority.** `Source/EchoesSimCore` owns deterministic gameplay. Presentation consumes
  authorized state and cannot change fog knowledge, results, saves, replays, or checksums. Cosmetic assets
  never intercept input, alter navigation, or affect combat; gameplay geometry follows the collision contract.
- **Source before output.** Edit registered source data and generators (`Content/Data/Source`,
  `Content/Narrative/Source`, `Content/World/Source`), then compile or regenerate. Outputs need source identity
  and provenance, never manual repair.
- **Assets and quality.** Follow the master and the applicable art, audio, and voice direction. Record
  generator revisions, rights, licenses, dependencies, and exceptions in
  [AssetRegister.md](Docs/Archive/AssetRegister.md). Claim palette, loudness, binding, or other enforcement
  only when the actual validator and retained result establish it. Accessibility settings must produce the
  required observable behavior.
- **Failure and verification.** Preserve deterministic refusal and recovery for invalid authoritative data.
  Diagnose failing checks; never weaken requirements or hide failures. Run checks appropriate to the change,
  including negative and recovery cases where material; broaden testing when evidence justifies it.

## Ownership, tools, and coordination

- One write owner per checkout where practical. Read-only specialists may work in parallel on bounded tasks.
  Concurrent writers need disjoint ownership or isolated worktrees and must preserve each other's changes.
  Current owner instructions and live coordination establish roles, never a model name or retired lane roster.
- Old records cite `../WorkstreamControl/ACTIVE_LANES.md` and `HEAVY_RUN_LOCK.md`; those may not exist and
  an absent record is neither a lease nor a block. Establish ownership with the active coordinator before
  overlapping edits. Resource-intensive runs require `echoes-heavy-run-coordination` and exclusive use of the
  affected resources.
- Use the approved shell, API, connector, or UI interface that fits. Unreal Mac builds, packaging, and Apple
  signing run on the configured Mac. Discover tools and mounts live; never assume a disk identifier,
  credential, port, tool version, test count, or timeout from an old note. If a needed capability is missing,
  find its canonical source or an equivalent with the same evidence class; otherwise report it and stop only
  the dependent portion.
- New evidence belongs under `BuildArtifacts/Evidence/<gate>-<UTC>/` or the coordinator's designated root;
  record its absolute path with source commit, dirty state, command, date, environment, outcome, and hashes.
  Locate historical artifacts before citing them. Gitignored evidence is retained through the agreed handoff.

## Proportionate routing

The active parent owns scope, integration, verification, and the final answer and cannot replace its own
model mid-turn. Use the least costly supported route that meets the evidence burden: medium reasoning as the
baseline, higher effort for difficult consequential judgments. Use `fast_scan`, `evidence_researcher`,
`expert_reviewer`, and `critical_verifier` (final assurance only) when available; delegate independent work
when it improves quality or elapsed time. Model choice does not establish validation. For each work package
apply the [model and effort selection](Docs/Prompts/GameDevelopmentWorkflow.md#select-model-effort-and-work-ownership)
and the [handoff contract](Docs/Prompts/GameDevelopmentWorkflow.md#delegation-and-additional-task-handoff)
without being asked; advance the existing DeliveryPlan package rather than creating another plan.

## Evidence, state, and release boundaries

Distinguish observations, primary sources, vendor assertions, owner input, assumptions, calculations,
proposals, judgments, and unknowns. Never invent sources, results, owner decisions, experience, or status.
[RequirementsState.md](Docs/RequirementsState.md#state-vocabulary) owns status vocabulary. `IMPLEMENTED` is
not verified and a passing test is not completion. Source inspection, native tests, editor automation,
packaged automation, rendered inspection, physical input, uncoached human play, owner acceptance, and
independent validation are separate evidence classes; agent review is internal QA. Only Angelis assigns owner
acceptance, rejection, or `COMPLETE`. Check retained evidence identity before relying on it and say when it
was not rerun. A screenshot cannot prove interaction, performance, audio, or acceptance. Prepared, executed,
verified, submitted, published, signed, notarized, deployed, and release-ready are distinct states. Prototype
content never justifies public claims of a finished game.

## Documents, authorship, and communication

- One authoritative physical file per document, edited in place with history and metadata preserved. No
  draft, version, or final-copy documents; exports and renders are QA outputs.
- Every author, creator, and last-modifier field reads **Angelis Pseftis**. Keep legitimate third-party
  attribution and licenses. Tool and model names appear only as provenance or test environment.
- Verify the final file, links, claims, calculations, and authorship before delivery; render and inspect
  paginated documents and visual artifacts completely.
- Write directly for an expert technical or executive audience: outcome and evidence first, no filler or
  invented experience. Keep player-facing language separate from implementation detail. For academic work
  inspect every rubric and requirement; for LinkedIn default to a complete credible article with relevant
  hashtags. Preparing text is not publishing it.

## Git and completion

Preserve dirty work. Stage explicit task paths only when committing is in scope; inspect the diff and identity
first. Keep LFS provenance for binaries. Never broadly reset, force-push, rewrite mainline history, or delete
active work as cleanup. A historical push instruction is not an automatic end-of-task action; follow current
authorization. Do not message others without explicit authorization. Before reporting completion verify the
requested outcome, relevant checks, preserved unrelated work, and material limits; keep historical evidence
and owner decisions, replace superseded guidance with a clear successor, and update dependent pointers in the
same change.
