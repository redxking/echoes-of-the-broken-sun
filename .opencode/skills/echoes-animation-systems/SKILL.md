---
name: echoes-animation-systems
description: Build or inspect Echoes animation-state systems, transitions, and authoritative-event bindings without allowing animation to alter simulation truth.
metadata:
  author: Angelis Pseftis
---

# Echoes animation systems

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, applicable current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Only mutate paths under a live coordinator-issued lane; never self-assign a lease.

Bind visual animation states to confirmed adapter events. Animation cannot alter deterministic simulation, commands, fog, collision, navigation, saves, replay, checksums, or outcome timing. Source changes precede generated output; register any asset family/provenance in `Docs/Archive/AssetRegister.md`. Define idle, move, order, attack, damage, death, cancel, interruption, and recovery behavior only where the authoritative game contracts expose them.

Acceptance: state/event mapping, source/generated record, rendered transition observations at gameplay zoom, reduced-motion behavior, and bounded evidence. Before Editor/runtime/GPU work read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md`; use `echoes-realtime-visual-review` and `echoes-evidence-gate-review`. Stop for absent lane, ambiguous adapter state, transition that misleads the player, missing provenance, or unavailable runtime evidence.
