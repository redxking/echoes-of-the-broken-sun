---
name: echoes-camera-navigation
description: "Implement or validate Echoes camera movement, framing, zoom, navigation, focus, and accessibility behavior through real input and gameplay readability checks."
metadata:
  author: Angelis Pseftis
---

# Echoes camera and navigation

Read `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify lease/worktree/branch/SHA before mutation.

Define input routes, focus/modal/Escape behavior, bounds, zoom, follow/frame rules, motion reduction, camera-relative input, resize/display behavior, and recovery after menus/cinematics/load. Camera is presentation: it cannot alter deterministic simulation state, fog authority, saves, replays, or checksums.

Test across relevant resolution/window and accessibility settings with actual pointer/keyboard through `echoes-gui-control-readiness`; observe selection/combat readability rather than screenshots alone. Route selection links to `echoes-selection-movement-pathing`, UI to the player-facing implementation lane, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop on shared input lease, unavailable GUI capability, untested transition, or accessibility behavior that is cosmetic only.
