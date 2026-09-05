# Game development prompt workflow

**Author and owner:** Angelis Pseftis
**Created:** 2026-09-05
**Sources reviewed:** 2026-09-05
**Standing:** shared task procedure under [AGENTS.md](../../AGENTS.md) and the
[document authority map](../README.md); no independent game requirements or acceptance authority.

Use for game implementation, debugging, presentation work, and substantial gaming prompts. Scale the
brief to the change: a small correction needs a few sentences; a feature spanning systems needs a
bounded milestone. A request to write a prompt produces the prompt only. A request to implement carries
the authorized change through its relevant checks. The agent fills discoverable context from the checkout;
the owner does not have to complete a form before work starts.

This procedure adapts the official sources below to Echoes. Their browser-game examples demonstrate
useful workflows; they do not validate an Unreal production method or guarantee code/product quality.
The project-specific application follows `SPEC-AUTH-001..006` in
[Requirements.md](../Requirements.md) and the current [state/decisions](../RequirementsState.md).

## Frame the task

Use this brief in the active task or existing authoritative plan. Link to the controlling material instead
of copying its rules. Resolve ordinary choices within the current authority and ask only when a missing
decision materially changes scope, canon, rights, architecture, or acceptance.

```text
Goal: [Observable improvement for the player or developer; requested deliverable.]
Context: [Exact requirement IDs, current source paths, applicable design references,
reproduction steps or reference captures, current evidence and known failures.]
Constraints: [Owned paths and interfaces; simulation/presentation boundary;
target platform/configuration; applicable budgets and compatibility obligations.]
Done when: [Concrete behavior and required checks/evidence; completion boundary.]

For a feature: name the player action, system response, visible/audible feedback,
failure/recovery, and how it connects to the existing loop and win/loss contract.
For a defect: state expected versus observed behavior and the reproducing case.
For presentation: identify the exact frame, sound, or interaction to improve and
the approved reference and runtime conditions used for comparison.

Continue the authorized work. Report the change, checks actually run, attributable
evidence, remaining defects, and any decision still needed.
```

For broader gameplay work, bind the main loop, controls, objectives, win/fail states, progression,
difficulty, and visual/audio direction to existing requirements. For a narrow task, identify only the
affected part. Use the configured Unreal/C++ project and registered source/generator pipeline; external
sample stacks, hosting defaults, and example mechanics are not architecture decisions for Echoes.

## Implement and inspect

Before assigning a work package, apply [model and effort selection](#select-model-effort-and-work-ownership).
For delegated work or an authorized additional task, complete the
[handoff contract](#delegation-and-additional-task-handoff) from live evidence.

1. **Establish the baseline.** Read the relevant sources and current tests; reproduce the reported defect
   or inspect the affected experience. Identify pre-existing failures and inaccessible evidence. Completion
   of this step means the expected result and available comparison are explicit, including any unobserved
   baseline. Reuse relevant retained evidence only after checking its identity.
2. **Choose a coherent change.** Select the smallest increment that exercises the requested behavior across
   its real interfaces. For a feature, connect player input, authoritative state, presentation, and the
   relevant recovery/persistence path. For a difficult task, keep milestones in the existing plan named by
   the authority map. For a simple fix, proceed directly. Avoid unrelated refactoring or dependencies.
3. **Implement from source.** Reuse existing components and conventions. Investigate failing behavior before
   changing it; add a focused regression case when the defect or invariant warrants one. Inspect live
   scripts/configuration for commands and version-sensitive APIs. Registered generators remain responsible
   for derived assets and catalogs.
4. **Verify the affected experience.** Run the relevant source, build, integration, and runtime checks for
   this change through the selected [domain skills](../AgentSkillRouting.md). Inspect actual rendered,
   audible, or interactive output where required. A successful tool invocation or clean log does not prove
   the requested player behavior. Heavy runs follow the shared coordination rules.
5. **Review and finish the increment.** Inspect the task diff and artifacts against the brief and
   [review priorities](#review-priorities). Fix observed regressions within scope, then rerun affected
   checks. Report exact outcomes and limits. Full-task completion still requires all mandatory criteria;
   an intermediate increment is only a milestone. Keep owner acceptance in the state record.

For hard logic, balance, or visual iteration, use a stable reproducer/scenario and a written rubric tied to
the affected requirements. Record baseline, hypothesis, focused change, settings/seed, measurements, and
artifact paths per iteration in the existing evidence root. Compare equivalent conditions, then inspect
the artifacts that explain the metrics. Track regressions as well as improvements. Deterministic tests
check objective invariants; agent judgments help triage subjective defects. Neither accepts fun, coherence,
or readability on the owner's behalf. Sample scores from an external tutorial never replace project gates.
If repeated attempts produce no progress, revisit the hypothesis or obtain a bounded specialist review;
report unresolved evidence rather than iterating indefinitely or relaxing the criterion.

For complex interactions, reuse test-only scenario fixtures and observable state/counters; add the
smallest missing test interface when needed to diagnose the behavior. Keep fixtures isolated from player
saves and shipping authority. A directly initialized scenario checks that scenario; also exercise the
actual player-input journey through its transitions when the controlling requirement needs that evidence.

## Select model, effort and work ownership

Apply this procedure automatically for each bounded work package in [DeliveryPlan.md](../DeliveryPlan.md),
including follow-up repairs and substantial prompts. The parent owns selection, integration and final
verification. These are project routing judgments; no model is proven optimal for every task.

1. Identify the concrete outcome, controlling IDs, failure consequences and evidence needed. Decide
   whether the work is a scan, investigation, implementation, review or final assurance check. Use tools
   and existing scripts directly for routine operations; a model delegation must earn its overhead.
2. Inspect current callable model/effort options and applicable custom-role configuration. Use the
   table below as a starting route, then adjust for ambiguity, coupled systems and the cost of a wrong
   result. A remembered model name or generic web catalog does not establish availability on this host.
3. Delegate only a bounded task that can progress independently alongside useful parent work. Prefer
   read-only specialists. Assign one write owner for shared core/controller/HUD work; writers in parallel
   need disjoint paths or isolated worktrees and an explicit integration owner. Discover the live
   concurrency limit; use fewer agents whenever dependencies or shared resources remove the benefit.
4. Set both model and effort explicitly when the interface permits it, or select a verified custom role
   that fixes both. Check whether the chosen role overrides requested settings. Use the supported
   limited-context handoff when a full-history fork would inherit the parent's model instead. Preserve
   an explicitly selected user model; report an unavailable route and any material downgrade.
5. Record the package, role/model/effort, short rationale, path ownership, commands/evidence and outcome
   in the current evidence receipt. Record elapsed time and rework when available; record token/cost
   figures only when supplied by the runtime. Review progress against the same acceptance checks.
6. Escalate at the outset for known high-consequence logic, or when uncertainty/failure exceeds the
   selected route. After two attempts fail the same check without new evidence, revisit the reproducer
   or obtain a bounded higher-depth review instead of repeating the same approach. Return routine work
   to the economical route once the difficult question is resolved. Passing checks and actual artifacts
   determine progress; model prestige, effort level and agent agreement do not establish correctness.

| Work and trigger | Initial route, subject to live availability |
|---|---|
| File inventory, exact log extraction, deterministic source search | `fast_scan`: `gpt-5.6-luna`, `low`, read-only. Keep trivial one-command work in the parent. |
| Large-file/code exploration, primary-source checks, evidence distillation | `evidence_researcher`: `gpt-5.6-terra`, `medium`, read-only. |
| Contained implementation with stable interfaces and an objective check | Writable `worker`: `gpt-5.6-terra`, `medium`; use `gpt-5.6-sol`, `medium` when multiple implementation steps or integration dependencies warrant it. |
| Unreal/C++ integration, deterministic saves/replays, subtle combat/fog/AI logic, concurrency or recovery | Writable `worker`: `gpt-5.6-sol`, `high`; owner supplies exact invariant and negative/recovery cases. |
| Architecture, security, correctness, evidence or consequential code review | `expert_reviewer`: `gpt-5.6-sol`, `high`, read-only. |
| Broad unresolved architecture or a difficult bounded problem requiring stronger reasoning | `gpt-6-astra`, `high` or `xhigh`, with a specific question and check. Preserve current parent continuity; this row does not claim a mid-turn parent switch. |
| Exceptional final assurance where a marginal reliability gain matters | `critical_verifier`: `gpt-5.6-sol`, `xhigh`, read-only. Use stronger supported effort only for a documented unresolved risk; `max`/`ultra` are exceptional choices, not defaults for routine work. |
| Art, UI, audio and narrative production/review | Choose implementation or review route above by complexity and give it the required visual/audio tools and approved references. Higher text reasoning cannot substitute for rendered inspection, listening or physical input. |

The four named specialist role mappings were checked against local custom-agent configuration on
2026-09-05. Writable worker and Astra routes are project recommendations using the callable model options
observed that day. Recheck at dispatch. The active parent cannot replace its own model during a turn.
Use explicit settings for future authorized tasks/delegates; do not create a duplicate task just to change
the parent or edit global settings as a workaround. This policy does not silently change unrelated projects.

## Delegation and additional-task handoff

Prefer the existing task and subagents for its implementation packages. An additional user-visible task
must be authorized under the current host rules and have a distinct outcome or useful context/worktree
isolation. Conditional instructions about future chats are not themselves a request to launch a fleet.
When authorized, use the task-creation interface, select the exact project/environment and supported model
and effort, and identify the result as a separate user-owned task. Reuse an existing task for the same
outcome. Apply the same handoff quality to subagents; this procedure adds no external-message authority.

Write a concise, self-contained prompt with the following fields filled from the current checkout.
No unresolved placeholders may remain at dispatch. Link exact authoritative files and relevant records;
do not copy an entire rulebook or unrelated conversation into each prompt.

```text
Outcome and package: [DeliveryPlan package; observable player/developer result.]
Assignment: [Implement/review/research; selected model and effort; why appropriate.]
Identity: [Absolute checkout/worktree, branch, commit, relevant dirty-state/hash receipt.]
Authority: [AGENTS.md, Docs/README.md, this workflow, exact requirement IDs,
applicable state/decisions, canon/design references and selected leaf skills.]
Baseline: [Expected vs observed behavior, reproducible steps/seed, relevant source,
retained failures, commands already run, and evidence that remains unobserved.]
Ownership: [Exact writable paths or read-only scope, interfaces, integration owner,
other active owners. You are not alone; preserve others' edits and adapt to them.]
Behavior: [Player action, authoritative response, visible/audible feedback,
failure/recovery, persistence and connection to the match/mission outcome.]
Constraints: [Only applicable determinism/fog/save/compatibility/platform/budgets,
source-generation/provenance rules and existing approvals/decision boundaries.]
Execution: [Bounded steps and dependencies, available tools, shared resources and
exclusive-run owner, isolated test/save storage, safe failure/recovery path.]
Done when: [Observable acceptance checks, negative/recovery cases, commands or
verification method, evidence class/artifact location and source/package binding.]
Return: [Changed paths, behavior and rationale, checks actually executed/results,
evidence and hashes, unresolved risks/decisions, next dependency and handback point.]
Completion boundary: [Distinguish source/native/editor/package/physical/human/owner
evidence; state what this assignment can establish. Only Angelis accepts the game.]
```

Before dispatch, the parent checks that the prompt covers every requirement affected by the bounded
assignment, owns no conflicting path, supplies a reproducible/checkable result and has enough context
to work without guessing. Build/game/asset-generation authority and resources must be explicit where
needed. Missing facts that can be read locally are investigation steps, not questions for the owner.

At handback, inspect the scoped diff and evidence, reproduce relevant checks, resolve material findings
and integrate before moving dependent work forward. Use event-driven agent/task waits and concise updates;
avoid repeated unchanged polling and redundant reviews. Reuse scripts and discovered editor tools before
writing new wrappers. Keep immediate P0–P4 playable-journey work ahead of broader production that does not
unblock it; preserve the approved P5–P7 obligations and hosted-service deferral.

The mechanism and configuration precedence were checked against
[OpenAI's subagent documentation](https://learn.chatgpt.com/docs/agent-configuration/subagents) on
2026-09-05. It documents configurable model/effort, inheritance and role overrides. The table and handoff
gates above are the owner's project operating policy, not a vendor benchmark or a measured productivity
guarantee. Evaluate them through actual accepted work, elapsed time and rework.

## Choose the relevant quality checks

These are routing cues. The exact affected master records and leaf skills determine required cases and
thresholds; this table does not add a new release checklist.

| Change | Focus the brief and inspection on |
|---|---|
| Gameplay, economy, AI, pathing | Reproducible scenario/seed, authoritative result, player feedback, ordering, fog knowledge, invalid commands, recovery, and affected save/replay compatibility. Measure balance as a tested proposal against the approved baseline. |
| HUD, menus, camera, controls | One concrete visual/interaction issue at a time; reuse current components and style definitions. Compare under the same resolution, UI scale, camera, and scene. Exercise input, focus, hit testing, denial feedback, and affected accessibility settings. |
| Maps, art, animation, VFX | Mission identity and approved composition, then readability at gameplay scale and in motion. Use approved references from the angles needed to resolve silhouettes. Check fog, collision/input, navigation, and runtime export cost separately from editable asset complexity. Preserve reusable generation prompts and parameters with asset provenance. |
| Voice, music, ambience, narrative | Approved character/context, actual playback and transitions, intelligibility, mix, subtitles, interruption/recovery, settings, and rights/provenance. A generated file or measurement alone does not establish the experience. |
| Build, performance, release | Exact source/package identity, hardware, renderer/settings, workload, measured metric, applicable budget, failures and recovery. Headless/editor timing, algorithm simulation, and target GPU frame time establish different facts. Keep source checks, editor previews, packaged execution, physical play, listening, and owner acceptance distinct. |

## Review priorities

Review the change for a concrete failure scenario and player impact. Prioritize simulation determinism,
state authority/fog leaks, save/replay compatibility, lifetime/thread safety, input/navigation regressions,
asset provenance and unintended generated churn, and measured performance/accessibility obligations.
Check missing negative/recovery coverage when it is material. Cite the affected code and requirement or
contract for each actionable finding; label uncertain hypotheses and identify how to reproduce them.
Delegate consequential review when useful under the shared routing rules. Internal agent review remains
internal QA; required independent release verification retains its existing separation requirements.

For bug triage, first inspect the available failing checks, logs, captures, and repro notes. Deduplicate
by failure behavior; record severity, expected/observed result, source/package identity, evidence,
reproduction status, suspected cause, and next action in the current defect/evidence record. Rank observed
player impact and release blockers ahead of speculative cleanup. State which sources could not be read.
Scheduled triage or external PR review is a separate setup action under current authorization; validate
the manual workflow before proposing automation. This document does not activate a schedule or GitHub bot.

## Applying the brief

These examples show how to interpret common requests. Derive exact values and current states from the
checkout; the examples assert no implementation or test result.

- **“Fix workers getting stuck.”** Identify a seed/map and command sequence that reproduces the stall;
  bind expected behavior to the current movement/economy records. Inspect pathing, occupancy and delivery
  state, fix the demonstrated cause, and verify the reproducer plus affected known/unknown-terrain and
  save/replay cases. Show what the player sees when a route is unavailable.
- **“Make M01 look like the concept.”** Verify the mission binding and approved reference first. Compare
  whole-map composition, terrain silhouette, landmarks, scale, and atmosphere from matched gameplay
  views. Correct the largest demonstrated mismatch and inspect it in motion with fog and UI present.
  Retain the required rendered evidence before describing the visual outcome.
- **“Improve the command panel.”** Name the exact readability or interaction defect and the approved
  layout/style constraint. Make a focused change and compare equivalent before/after views; exercise the
  affected command, focus, hit region, remapping, and accessibility behavior through the relevant checks.

## Official sources and maintenance

Reviewed the [requested use-case index](https://learn.chatgpt.com/use-cases) and all five linked entries
in its [Game development collection](https://learn.chatgpt.com/use-cases/collections/game-development).
The table records OpenAI's guidance separately from the Echoes application above. These are instructional
examples and vendor recommendations, not comparative evidence that this workflow yields the best game.

| Official page | Practice used here |
|---|---|
| [Create browser-based games](https://learn.chatgpt.com/use-cases/browser-games) | Define the game loop and milestones, build and play the result, retain useful generation prompts. Browser stack/hosting defaults do not transfer to Unreal. |
| [Make granular UI changes](https://learn.chatgpt.com/use-cases/make-granular-ui-changes) | Focused UI edit, reuse existing patterns, inspect the running result, then iterate. |
| [Iterate on difficult problems](https://learn.chatgpt.com/use-cases/iterate-on-difficult-problems) | Establish evaluation and baseline, inspect metrics and artifacts, make a focused change, record results. Illustrative thresholds and agent judges do not confer acceptance. |
| [Automate bug triage](https://learn.chatgpt.com/use-cases/automation-bug-triage) | Evidence-backed, deduplicated, prioritized triage with unknowns; tune manual output before scheduling. |
| [Review GitHub pull requests](https://learn.chatgpt.com/use-cases/github-code-reviews) | Repository-specific review guidance and an additional review signal alongside tests and human review. |
| [Best practices](https://learn.chatgpt.com/guides/best-practices) | Goal, context, constraints, and completion criteria; concise durable guidance; relevant testing and diff review. |
| [Building games with Astra](https://learn.chatgpt.com/blog/how-to-build-games-with-astra) (2026-09-04) | Browser-game case study: observable test interfaces, real-input journeys, measurement boundaries, multi-angle asset references, and exported asset cost. Its engine techniques and reported results are specific to that example. |

On a substantial prompt/workflow revision, a tool/model/engine migration, or repeated workflow failures,
reopen the collection and affected official pages. Check for changed or added use cases and verify
version-specific Unreal/tool advice against the installed environment and its official documentation.
Update this same file's source-review date and rationale only after inspection. If sources are unavailable,
identify the last verified date and uncertainty; continue work supported by existing authority. Do not
claim continuous freshness. Adopt a change only when it addresses an observed gap without conflicting
with the master, canon, or owner decisions.

After guidance edits, run `python3 Scripts/check_agent_docs.py` from the checkout. It checks links,
authorship, required workflow routing, and the existing registry structure. Inspect the brief and source
mapping semantically as well. Retain the task diff, commands/results, and source identity under the shared
evidence rules. A structural pass proves routing, not future agent compliance or improved game quality.

**2026-09-05 change rationale:** connected the existing project prompts to one reusable task procedure;
adapted all five game use cases, the general best-practices guide, and the game-building case study to the existing Unreal authority,
verification, and evidence workflow. No game behavior, numeric gate, requirement status, or owner
acceptance is changed by this documentation update.
