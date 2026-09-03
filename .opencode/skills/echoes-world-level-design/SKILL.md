---
name: echoes-world-level-design
description: "Build or assess Echoes maps, terrain, navigation, dressing, camera readability, and mission spaces while retaining compiled-world and simulation boundaries."
metadata:
  author: Angelis Pseftis
---

# Echoes world and level design

Use for Glass Scar, campaign maps, terrain presets, navigation, environmental dressing, map compilation, or gameplay-camera readability.

1. Read live `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Requirements.md`, `Content/World/Source`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify exact lease/worktree/branch/dirty paths before mutation.
2. Edit authoritative world source and compile it; do not hand-edit compiled map artifacts. Preserve registered map identity, terrain binding, authoritative passability, nav rules, and campaign/skirmish dispatch semantics.
3. Separate simulation terrain/navigation authority from decorative meshes, lighting, VFX, and audio. Decoration must not alter collision, overlap, navigation, shadow, fog, saves, replay, or checksums unless explicitly part of the authoritative contract.
4. Evaluate both technical and player layers: compiler/map checks, spawn/objective reachability, camera readability at gameplay zoom, high-contrast modes, performance budget, collision/nav hygiene, and a physical-input play route where required.
5. Stop for shared map/hotspot leases, unregistered assets, visual redesign outside canon, missing hardware baseline, or an untestable required journey.

## Acceptance checks

Record source/digest and compiled binding identity, terrain/nav assertions, build/capture evidence, physical input route, visual inspection boundaries, and remaining defects. Route collision/placement authority to `echoes-construction-production`, player movement to `echoes-selection-movement-pathing`, GUI play to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.
