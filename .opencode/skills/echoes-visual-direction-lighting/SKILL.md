---
name: echoes-visual-direction-lighting
description: Design or review Echoes lighting, composition, palette, and environmental readability without altering deterministic play state or treating a screenshot as player validation.
metadata:
  author: Angelis Pseftis
---

# Echoes visual direction and lighting

Read `CLAUDE.md`, then Track A in `Docs/GameCompletionDirective.md`, `Docs/Archive/DevelopmentBible.md` (§Creative direction, §Art and audio), `Docs/ArtDirection.md`, `Docs/Archive/TechnicalArchitecture.md` (§Presentation), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Before any mutation, confirm an ACTIVE lane owns every target path; otherwise produce a proposal only.

Build lighting and visual hierarchy from the Bible's world/faction language and the current directive—not generic fantasy conventions. Presentation must be spawned from authoritative state and must not affect collision, overlaps, navigation, shadows, fog authority, saves, replays, or checksums. Keep Nanite and VSM off unless the owner records a changed baseline decision.

For each slice, state the gameplay question it improves (selection, terrain, threat, objective, faction, or modal state), affected camera distances, and accessibility response. Render the actual camera paths at target quality and inspect in motion at normal play pace; screenshots are supporting evidence only. Record exact build, map, settings, hardware, and capture boundary in the ledger/evidence location required by the directive.

Acceptance output: scoped implementation or review, before/after rendered evidence, visual-readability observations, and a truthful evidence status. Exclude simulation, generated-content compiler bypasses, and unregistered assets. Stop when ownership is absent, the source basis is unclear, a visual change masks gameplay information, or required rendered review cannot run.

Use exact authorities: `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Before GPU-intensive renders, Editor/runtime launch, or profiling, read `../WorkstreamControl/HEAVY_RUN_LOCK.md`; acquire a coordinator-issued lease or stop. Route live visual assessment to `echoes-realtime-visual-review`, evidence closure to `echoes-evidence-gate-review`, and owner signoff to `echoes-human-acceptance-session`.
