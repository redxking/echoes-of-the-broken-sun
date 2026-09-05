---
name: echoes-simulation-core
description: "Implement or verify deterministic EchoesSimCore rules, state transitions, and invariants without allowing engine presentation to affect authority."
metadata:
  author: Angelis Pseftis
---

# Echoes simulation core

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use for deterministic rules, command resolution, fog, economy, combat, checksum, or simulation state. Do not use for visual-only work.

1. Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and dirty paths before mutation.
2. Keep authoritative rules in `Source/EchoesSimCore`, deterministic and engine-independent. No HUD, camera, VFX, audio, physics presentation, frame time, or non-deterministic engine object may enter state, fog authority, saves, replays, or checksums.
3. Define invariants and adversarial cases before coding: valid/invalid command behavior, ordering, resource bounds, ownership, determinism across supported configurations, and fail-closed handling of missing or mismatched data.
4. Use existing native simulation tests and focused additions within live task ownership. A passing adapter/UI test cannot prove core determinism; test the core directly.
5. Stop if the requested behavior contradicts `Docs/Archive/DevelopmentBible.md`, data authority, replay/save contract, or live task-ownership conflict.

## Acceptance checks

Record the exact source change, configurations run, seed/input, assertions, checksums or state equivalence where applicable, failures investigated, and retained logs. Route runtime proof to `echoes-unreal-runtime-integration`, player input to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.
