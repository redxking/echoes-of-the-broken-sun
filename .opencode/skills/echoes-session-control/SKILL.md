---
name: echoes-session-control
description: "Govern a single Echoes work session: establish live authority, lane ownership, evidence boundaries, and a safe stop before any implementation."
metadata:
  author: Angelis Pseftis
---

# Echoes session control

Use before any consequential Echoes work. It does not authorize edits, builds, releases, or human acceptance.

1. Read live `CLAUDE.md`, `Docs/GameCompletionDirective.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; obey the latter if it limits Codex to review-only.
2. Identify one gate/slice, its authoritative files, its current evidence state, the exact check, and the owner-only decision boundary. Never infer current status from a prior session.
3. Before mutation, inspect the exact worktree, branch, commit, dirty paths, and current lease. Do not edit a leased/shared/owner-held path without a matching current lease; never self-authorize from old lane records.
4. Use source paths (`Content/Data/Source`, `Content/Narrative/Source`, `Content/World/Source`) and the registered compiler/generator; generated artifacts are outputs, never hand-edited authority.
5. Define a stop condition: missing owner decision, contradictory authority, unavailable mounted volume, failed baseline, or evidence that cannot establish the gate. Report it rather than broadening scope.

## Completion record

State the exact commit/worktree, paths touched, commands or human exercise performed, retained evidence location, outcome, and only an allowed state: `OPEN`, `IN PROGRESS`, `IMPLEMENTED — NOT YET VERIFIED`, `AGENT VERIFIED`, `EVIDENCE READY`, `AWAITING HUMAN ACCEPTANCE`, or `BLOCKED`. Only Angelis may mark `HUMAN ACCEPTED` or `HUMAN REJECTED — CHANGES REQUIRED`; `COMPLETE`/`PASS` are reserved. Route evidence to `echoes-evidence-gate-review` and owner review to `echoes-human-acceptance-session`.
