---
name: echoes-animation-systems
description: Build or inspect Echoes animation-state systems, transitions, and authoritative-event bindings without allowing animation to alter simulation truth.
metadata:
  author: Angelis Pseftis
---

# Echoes animation systems

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Only mutate paths under a task-owner-confirmed assignment; never self-assign a live task ownership.

Bind visual animation states to confirmed adapter events. Animation cannot alter deterministic simulation, commands, fog, collision, navigation, saves, replay, checksums, or outcome timing. Source changes precede generated output; register any asset family/provenance in `Docs/Archive/AssetRegister.md`. Define idle, move, order, attack, damage, death, cancel, interruption, and recovery behavior only where the authoritative game contracts expose them.

Acceptance: state/event mapping, source/generated record, rendered transition observations at gameplay zoom, reduced-motion behavior, and bounded evidence. Before Editor/runtime/GPU work, coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md); use `echoes-realtime-visual-review` and `echoes-evidence-gate-review`. Stop for missing task ownership, ambiguous adapter state, transition that misleads the player, missing provenance, or unavailable runtime evidence.

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
