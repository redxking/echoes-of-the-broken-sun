---
name: echoes-requirements-authoring
description: "Take a new, changed, or missing Echoes requirement, situate it against project authority and the existing ledgers, write and maintain its normative record, derive the requirements it implies but does not state, and decompose all of them to leaf requirements that each fail on their own evidence."
metadata:
  author: Angelis Pseftis
---

# Echoes requirements authoring

Use when a requirement arrives, changes, contradicts another, or must be split into testable parts. It writes
requirement text only. It never invents scope or canon, never claims a requirement is met, and does not map
finished work to evidence — that stays with `echoes-requirements-traceability`.

1. **Establish authority before writing.** Read live `CLAUDE.md`, `Docs/GameCompletionDirective.md`,
   `Docs/Requirements.md`, `Docs/DemoRecoveryDirective.md`,
   `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, and
   `../WorkstreamControl/ACTIVE_LANES.md`. Confirm lease, worktree, branch, and dirty paths before mutation.
   Record the intake's exact source and quote owner wording verbatim; a paraphrase is not authority.

2. **Situate before adding an ID.** Search `Docs/Requirements.md` for the same player outcome across all
   three namespaces. Classify the intake as new, a refinement of an existing ID, a duplicate, a contradiction,
   or a scope change. Refinements amend their existing record; duplicates cross-reference. Never treat a shared
   prefix name as a match — `SPEC-SIM-005` and `REL-SIM-005` are unrelated. A contradiction with a ledger
   constant (Future Well values, REL-AI-016 balance band, REL-UI-013 resolution matrix, REL-AUD-010 mix,
   REL-PERF-007 budgets, REL-SIM-018 outcome scope) or a scope change is an owner decision: write a `TBR-*`
   packet with alternatives, cost, dependencies, and playable consequence, and stop that path.

3. **Write the record to the master's schema.** Every record carries: ID; normative *shall* statement; player
   outcome; preconditions; dependencies; acceptance threshold; failure and recovery behavior; accessibility
   behavior; automated verification; packaged verification; human-play requirement; owner acceptance
   requirement; evidence location (`../WorkstreamControl/evidence/demo-recovery/<req-id>/` for `DEMO-*`,
   `../WorkstreamControl/evidence/release/<req-id>/` for `REL-*`); source and package identity;
   state — which lives in `Docs/RequirementsState.md`, never in the body;
   engineering state; human acceptance state; reopening conditions. State the requirement as required player-
   observable behavior, not as an implementation, a file, or a task.

4. **Derive what the record implies.** A ledger holds only what somebody thought to write down; the inferred
   set is part of the requirement, not commentary on it. Work these classes against every record and write each
   implication an existing ID does not already cover. **Forward scope** — a stated later platform, hardware
   class, dormant module, or deferred feature binds work happening *now*, so write the constraint as a
   requirement on the current release, never on the future one. **Negative space** — a stated success path
   implies its invalid input, refusal, interruption, cancellation, and recovery behavior. **Cross-cutting** —
   `CLAUDE.md`'s standing rules (simulation is the only authority, fail closed, accessibility is a behavior,
   procedural-first provenance, determinism) bind whether or not the record repeats them. **Interface** — a
   record naming two systems implies a requirement on the boundary between them. **Persistence** — anything
   stateful implies save/load, replay equivalence, schema version, and migration behavior. **Measurement** — a
   stated threshold implies its method, sample, tolerance, and behavior exactly at the boundary. Mark each
   `DERIVED FROM <parent ids>` and cite the directive, architecture, or `CLAUDE.md` clause it makes testable.
   Derivation that only makes an existing binding testable is authored directly; derivation that adds scope,
   cost, or a stricter acceptance bar is a `TBR-*` proposal and stays unauthored until the owner decides. A
   derived requirement the parents do not support is an invention — drop it.

5. **Decompose to leaves.** Split until every leaf satisfies all of: **one failure mode** (any `and`, `or`, or
   list whose parts can fail separately is still a parent); **observable** — names actor, trigger, surface, and
   required result as a tester reads them off the running game; **bounded** — every threshold has a number,
   unit, and tolerance, or cites a ledger constant rather than restating it; **one verification class** — exactly
   one of `SRC`, `PKG-AUTO`, `PKG-REND`, `PKG-PHYS`, `EDT`, `HUM`, `OWNER` (needing two classes means two
   leaves); **independently failable** — it can fail while every sibling passes; **negative covered** — invalid
   input, failure, and recovery live in this leaf or in a named sibling; **one owner lane**. Stop splitting at
   the first level where all seven hold; splitting further produces implementation steps, not requirements.

6. **Populate the acceptance card.** Each requirement beyond `IN PROGRESS` gets children
   `.PRE .ACT .AUTH .VIS .AUD .FAIL .REC .ACC .PERF .AUTO .PKG .HUM .OWNER .EVID .REOPEN` at hierarchical IDs
   (`DEMO-INP-002.AUTH`); deeper splits append `.N`. Match the specificity of the owner's `DEMO-INP-002`
   worked example. An inapplicable child records `NOT APPLICABLE` plus technical reason, reviewer concurrence,
   and owner acceptance — never silent omission. Handler or editor tests never discharge a `PKG-PHYS`, `HUM`,
   or `OWNER` child.

7. **Maintain in place.** `Docs/Requirements.md` holds every requirement body; `Docs/RequirementsState.md`
   holds every state. No draft, revision, or numbered copies, and no third file that defines or accepts a
   requirement. IDs are
   permanent: never renumber, reuse, or delete. A withdrawn record keeps its ID and reads `SUPERSEDED BY <id>`
   or `WITHDRAWN — <authority, date>`. Never soften a threshold, drop a verification class, or reword an
   acceptance condition to match what was built; repair the work or report the blocker. Engineering states are
   `OPEN → IN PROGRESS → IMPLEMENTED → AGENT VERIFIED → EVIDENCE READY → AWAITING HUMAN ACCEPTANCE`; only
   Angelis assigns `HUMAN ACCEPTED`, `HUMAN REJECTED — CHANGES REQUIRED`, or `COMPLETE`. A parent stays open
   until every mandatory child is accepted. When a parent changes, re-run step 4 and reconcile its derived
   set: the inferred requirements are a maintained view of the ledger, not a one-time pass.

## Acceptance output

Deliver the edited section of `Docs/Requirements.md`, the added or changed IDs with parentage and states, the derived set with
its `DERIVED FROM` bindings, each acceptance card,
the open `TBR-*` and owner decisions, and an explicit coverage statement naming what the intake asked for that
the records do **not** yet cover. Authoring a requirement proves nothing about the game: report new records at
`OPEN` and never as satisfied. Route design questions to `echoes-canon-game-design`, implementation to the
domain skill, evidence mapping to `echoes-requirements-traceability`, gate review to
`echoes-evidence-gate-review`, and owner sign-off to `echoes-human-acceptance-session`.
