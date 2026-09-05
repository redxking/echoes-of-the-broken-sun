---
name: echoes-world-level-design
description: "Build or assess Echoes maps, terrain, navigation, dressing, camera readability, and mission spaces while retaining compiled-world and simulation boundaries."
metadata:
  author: Angelis Pseftis
---

# Echoes world and level design

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use for Glass Scar, campaign maps, terrain presets, navigation, environmental dressing, map compilation, or gameplay-camera readability.

1. Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Requirements.md`, `Content/World/Source`, and [echoes-session-control](../echoes-session-control/SKILL.md). Verify task ownership, worktree, branch, and dirty paths before mutation.
2. Edit authoritative world source and compile it; do not hand-edit compiled map artifacts. Preserve registered map identity, terrain binding, authoritative passability, nav rules, and campaign/skirmish dispatch semantics.
3. Separate simulation terrain/navigation authority from decorative meshes, lighting, VFX, and audio. Decoration must not alter collision, overlap, navigation, shadow, fog, saves, replay, or checksums unless explicitly part of the authoritative contract.
4. Evaluate both technical and player layers: compiler/map checks, spawn/objective reachability, camera readability at gameplay zoom, high-contrast modes, performance budget, collision/nav hygiene, and a physical-input play route where required.
5. Stop for shared map/hotspot ownership claims, unregistered assets, visual redesign outside canon, missing hardware baseline, or an untestable required journey.

## Acceptance checks

Record source/digest and compiled binding identity, terrain/nav assertions, build/capture evidence, physical input route, visual inspection boundaries, and remaining defects. Route collision/placement authority to `echoes-construction-production`, player movement to `echoes-selection-movement-pathing`, GUI play to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.

For the 15 unique mission maps M01–M15, use the applicable master requirements and creative authority to preserve a distinct, story-driven identity for each mission map while maintaining connected geography across the campaign. Show connection through authored routes, landmarks, environmental continuity, strategic context, and travel/transition logic that remain feasible in an RTS map structure. The required sense of a massive connected world is a player-experience goal, not authority to add MMO networking, persistent shared-world, or other unapproved scope. Do not invent regional history, map lore, or connections absent from the master; record unresolved choices as `TBR-*` decisions.
