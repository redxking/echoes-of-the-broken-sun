---
name: echoes-requirements-traceability
description: "Map an Echoes change to its controlling requirement, authoritative source, verification method, and evidence without relabeling unproven work as complete."
metadata:
  author: Angelis Pseftis
---

# Echoes requirements traceability

Use for requirement analysis, acceptance cards, gate evidence, or release claims; not for inventing requirements.

1. Read live `CLAUDE.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/GameCompletionDirective.md`, `Docs/InitialReleaseRequirements.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md` before mutation. Confirm lease, worktree, branch, commit, and affected paths.
2. Trace each claim as: requirement ID → exact authority path → implementation path → executable or observed check → evidence artifact → evidence class → current state. Keep DEMO and REL ledgers distinct.
3. Separate source inspection, static review, simulation tests, Unreal automation, packaged execution, physical-input play, human review, and independent validation. A screenshot or compile cannot substitute for the required class.
4. For source data, record source file, compiler/generator invocation, digest/catalog result, and generated output; never edit compiled outputs as a shortcut.
5. Preserve failures, negative results, and unknowns. Do not weaken criteria, overwrite historical evidence, or infer human acceptance.

## Acceptance output

Produce a compact requirement-to-evidence matrix identifying gaps, exact reruns required, and owner decisions. Update only files your verified lane permits; route the matrix to `echoes-evidence-gate-review` then `echoes-human-acceptance-session`. Completion remains `EVIDENCE READY` or `AWAITING HUMAN ACCEPTANCE` until Angelis decides otherwise.
