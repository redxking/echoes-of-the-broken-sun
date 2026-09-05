---
name: echoes-content-data-pipeline
description: "Safely author and compile Echoes game, narrative, and world source data with digest, schema, provenance, and fail-closed guarantees."
metadata:
  author: Angelis Pseftis
---

# Echoes content data pipeline

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use for catalog, narrative, world, asset-registration, compiler, and generated-data work; not for hand-editing runtime output.

1. Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/AssetRegister.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md), plus the relevant compiler/schema docs. Verify task ownership, worktree, branch, and dirty paths before mutation.
2. Edit only authoritative source under `Content/Data/Source`, `Content/Narrative/Source`, or `Content/World/Source`. Use the official compiler/generator to create output; never patch compiled packs, digests, headers, catalogs, or assets manually.
3. Preserve declared schema, stable IDs, canonical ordering, pins, and cross-reference integrity. New assets need provenance, method, rights basis, and recorded exception before use where procedural generation is insufficient.
4. Run focused schema/compiler/content checks and inspect outputs, digests, and rejection behavior. Missing, stale, mismatched, or unbound content must fail closed.
5. Stop for ownership conflict, invalid canon, unregistered external asset, altered protected pin, or an incompatible save/replay/content migration requirement.

## Acceptance checks

Record source inputs, compiler/generator revision and command, digest/output identity, test results, provenance records, and known untested engine consumption. Route runtime consumption to `echoes-unreal-runtime-integration`, then `echoes-evidence-gate-review`; owner review of player-facing material belongs to `echoes-human-acceptance-session`.
