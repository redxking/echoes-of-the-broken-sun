---
name: echoes-requirements-authoring
description: "Take a new, changed, or missing Echoes requirement, situate it against project authority and the existing ledgers, write and maintain its normative record, derive the requirements it implies but does not state, and decompose all of them to leaf requirements that each fail on their own evidence."
metadata:
  author: Angelis Pseftis
---

# Echoes requirements authoring

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use when a requirement arrives, changes, conflicts, or needs decomposition. This skill edits normative requirement text only. It does not invent scope or canon, and it does not claim that a requirement is met.

1. Locate the owner source and quote its relevant wording. Search [Requirements.md](../../../Docs/Requirements.md) before adding an ID. Classify the intake as a refinement, duplicate, contradiction, or genuinely new requirement. Preserve exact existing IDs and thresholds.
2. Write requirement bodies in `Docs/Requirements.md` using the master's existing schema, namespace conventions, and acceptance cards. Write lifecycle, evidence references, owner decisions, and reopen conditions in `Docs/RequirementsState.md`. Do not create a competing ledger, card schema, or state vocabulary.
3. State observable player/system behavior, preconditions, dependencies, threshold or cited constant, failure and recovery behavior, and the verification boundary needed for that particular requirement. Do not add implementation tasks as requirements.
4. Derive only what the binding parent sources support. Systematically check forward scope, negative space (invalid input, refusal, interruption, cancellation, and recovery), cross-cutting controls, interfaces, persistence, and measurement boundaries. Mark supported implications with `DERIVED FROM <parent IDs>`. If an implication adds scope, cost, a stricter acceptance criterion, or a creative decision, prepare a `TBR-*` decision with alternatives, dependencies, and consequence; do not author it as binding until Angelis decides. Drop anything unsupported.
5. Decompose only to bounded, independently failable leaves: one observable outcome or failure mode; actor, trigger, surface, and result; a precise threshold with unit/tolerance or cited constant; exactly one evidence class; one accountable owner; and named negative/recovery coverage. Stop before turning leaves into implementation steps.
6. Never renumber, reuse, delete, soften, or rewrite IDs and thresholds to match an implementation. Retire records in place with their authoritative successor or withdrawal decision. Use the [state vocabulary](../../../Docs/RequirementsState.md#state-vocabulary); authoring or agent checks do not establish owner acceptance or `COMPLETE`.

Report changed IDs, source parentage, derived bindings, open `TBR-*` decisions, evidence boundary, and explicit gaps. Route evidence mapping to [echoes-requirements-traceability](../echoes-requirements-traceability/SKILL.md).
