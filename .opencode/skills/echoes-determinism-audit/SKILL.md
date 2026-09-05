---
name: echoes-determinism-audit
description: Audit Echoes fixed-tick simulation, ordering, numeric, save, replay, and content-identity behavior for reproducible state without treating one matching checksum as universal proof.
metadata:
  author: Angelis Pseftis
---

# Echoes determinism audit

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Verify the exact source/content commit, catalog digest, schema versions, seed, configuration, platform, task ownership, and worktree before mutation.

Inspect fixed-step boundaries, random-number ownership, container/iteration ordering, command ordering, floating-point assumptions, clocks, asynchronous callbacks, serialization, content lookup, save/load, replay, and checksum inputs. Define a red-capable scenario and compare repeated independent runs in the approved optimized, debug, and sanitizer configurations where applicable. Record the first divergent tick and smallest differing authoritative field; never hide a divergence by changing a checksum, seed, tolerance, or ordering rule.

Presentation, editor state, audio, frame timing, and GUI input timing must not enter authoritative state unless the architecture explicitly says so. One platform/seed/route match establishes only that boundary, not every machine, mission, faction, compiler, or long-duration run.

Route fixes to `echoes-simulation-core`, persistence findings to `echoes-save-progression-recovery` or `echoes-replay-qol`, heavy automation to `echoes-heavy-run-coordination`, and retained results to `echoes-evidence-gate-review`. Stop for ambiguous authority, missing seed/content identity, active ownership conflict, or an owner-controlled schema/canon decision.
