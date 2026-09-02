---
name: echoes-factions-rosters-balance
description: "Author or assess authorized faction rosters and balance using deterministic data, scenario evidence, and player-readable differentiation—not intuition alone."
metadata:
  author: Angelis Pseftis
---

# Echoes factions, rosters, and balance

Use for Meridian Compact, Kharuun Assemblies, Hollow Choir roles, costs, counters, progression, or balance tuning.

1. Read live `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/InitialReleaseRequirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify lease/worktree/branch/dirty paths before mutation.
2. Preserve canon faction identity and authorized mechanics. Express tunables in authoritative source data; compile the catalog and do not hand-edit generated runtime data.
3. Define scenario matrix, faction symmetry/asymmetry expectations, AI/player starting conditions, victory condition, seeds, metrics, and acceptance ranges before tuning. Do not tune from one anecdotal match.
4. Separate deterministic balance results, AI behavior, performance, player comprehension, and owner judgment. Assess counterplay, dead states, exploitability, pacing, and accessibility/readability at ordinary camera height.
5. Stop for a canon change, missing metric/acceptance range, insufficient roster scope, conflict with active AI/campaign lanes, or a request requiring owner taste judgment.

## Acceptance checks

Record data revision/digest, scenario inputs, seeds, results, statistical limitations, human-play observations, and open risks. Route roster work to `echoes-faction-roster-design`, metrics to `echoes-balance-analysis`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.
