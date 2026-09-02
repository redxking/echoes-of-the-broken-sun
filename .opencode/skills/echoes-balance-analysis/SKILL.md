---
name: echoes-balance-analysis
description: "Measure Echoes faction and strategic balance against approved scenarios, including REL-AI-016 boundaries, without replacing owner judgment with a single simulation result."
metadata:
  author: Angelis Pseftis
---

# Echoes balance analysis

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify exact requirements, approved test set, lease/worktree/branch/SHA before mutation.

REL-AI-016 boundary: no non-mirror Standard matchup outside 40–60%, and no start-position advantage above 5 points over the approved test set, absent an owner-accepted design reason. Record scenario definitions, factions, maps, position, AI/version, seeds, sample size, metrics, confidence/limitations, and content digest. Do not tune source or generated data without the appropriate roster/mechanics lease.

Separate deterministic simulation/AI results from human readability, fun, exploit resistance, and owner taste. Route corrective work to `echoes-faction-roster-design`, `echoes-opponent-ai`, and relevant mechanics skill; player runs via `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop for an unapproved test set, insufficient samples, a changed canonical objective, or statistical result outside owner decision authority.
