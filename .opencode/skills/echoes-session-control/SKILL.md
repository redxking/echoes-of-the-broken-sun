---
name: echoes-session-control
description: "Govern a single Echoes work session: establish live authority, path ownership, evidence boundaries, and a safe stop before any implementation."
metadata:
  author: Angelis Pseftis
---

# Echoes session control

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use before consequential work. It establishes a bounded task; it does not itself authorize edits, builds, releases, or owner acceptance.

1. Identify the outcome, controlling requirement IDs, authority sources, affected paths, evidence state, verification method, and any owner-only decision. Historical assignments do not establish current ownership.
2. Inspect the checkout, base commit, dirty paths, and active work. Preserve unrelated changes. Establish live ownership with the active task or coordinator before writing an overlapping path; safe read-only work may continue if an old record is absent.
3. Use registered source locations and their generators. Generated catalogs and assets are outputs, never manual-edit authorities.
4. State the stop condition and evidence boundary. Record only observed status using the [state vocabulary](../../../Docs/RequirementsState.md#state-vocabulary); only Angelis assigns owner acceptance or requirement completion.

Return the exact scope, paths, base identity, checks performed, retained evidence location, outcome, and unresolved limitation.
