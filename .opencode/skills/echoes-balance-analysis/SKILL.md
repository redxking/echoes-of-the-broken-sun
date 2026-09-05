---
name: echoes-balance-analysis
description: "Measure Echoes faction and strategic balance against approved scenarios, using the exact approved balance requirement body, without replacing owner judgment with a single simulation result."
metadata:
  author: Angelis Pseftis
---

# Echoes balance analysis

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify exact requirements, approved test set, task ownership, worktree, branch, and SHA before mutation.

Use `REL-AI-042` (**Standard Matchup Competitive Balance Band**) and its exact thresholds, population
and conditions. `REL-AI-041` governs strategic-controller fair-fog weights. The former shared
`REL-AI-016` is a retired ambiguous identifier: resolve old evidence by title, clause and source revision;
never inherit acceptance by ID alone. New team/FFA formats need their own approved scenario/fairness
matrix; a 1v1 result does not qualify them.

Record scenario definitions, factions, maps, positions, AI/version, seeds, sample size, metrics,
uncertainty/limitations, and content digest. Tuning requires the appropriate source ownership; generated
data is regenerated from source. Do not turn one simulation result into a broad balance claim.

Separate deterministic simulation/AI results from human readability, fun, exploit resistance, and owner taste. Route corrective work to `echoes-faction-roster-design`, `echoes-opponent-ai`, and relevant mechanics skill; player runs via `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop for an unapproved test set, insufficient samples, a changed canonical objective, or statistical result outside owner decision authority.
