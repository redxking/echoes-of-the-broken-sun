---
name: echoes-canon-game-design
description: "Turn authorized Echoes design intent into a testable game-design slice without inventing canon, mechanics, factions, or release scope."
metadata:
  author: Angelis Pseftis
---

# Echoes canon game design

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use for design decisions spanning play loop, factions, world, UI, narrative, or pacing. It does not authorize creative redesign.

1. Read live [Project/AGENTS.md](../../../AGENTS.md), [echoes-session-control](../echoes-session-control/SKILL.md), the controlling requirement ledger, `Docs/Archive/DevelopmentBible.md`, and `Docs/GameCompletionDirective.md`. Verify task ownership, worktree, branch, and dirty paths before changing any design-owned file.
2. Extract explicit canon constraints, player promise, interaction contract, accessibility behavior, and measurable acceptance condition. Treat prior chat, mockups, and generated ideas as proposals until `Docs/Archive/DevelopmentBible.md` authorizes them.
3. Preserve the three factions, Future Wells, campaign structure, settings, and original world language. Design polish may improve delivery, but may not silently change theme, scope, balance identity, or endings.
4. Route data changes through authoritative source folders and compilers. Route deterministic rules through simulation; presentation can render state but cannot become authority.
5. If canon leaves a material choice open, write a decision packet with alternatives, player impact, implementation cost, and evidence needed; stop for Angelis rather than choosing canon.

## Acceptance output

Deliver a `Docs/Archive/DevelopmentBible.md`-cited design slice with requirement IDs, affected systems, explicit exclusions, test/play evidence plan, and owner decision points. Route mechanics to `echoes-gameplay-mechanics`, `echoes-faction-roster-design`, or `echoes-world-level-design`; then route evidence to `echoes-evidence-gate-review` and owner review to `echoes-human-acceptance-session`.

When designing the campaign experience across the 15 unique mission maps M01–M15, use the applicable master requirements to maintain a distinct story-driven identity for every mission and a coherent connected geography, narrative progression, and character continuity across the whole campaign. The intended massive-world feeling is an RTS presentation and progression requirement, not an authorization for MMO, shared-world persistence, or new network scope. Treat any missing map history, character backstory, transition, or campaign consequence as a `TBR-*` decision rather than new canon.
