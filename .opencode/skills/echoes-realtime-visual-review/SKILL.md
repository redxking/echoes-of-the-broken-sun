---
name: echoes-realtime-visual-review
description: Inspect Echoes real-time rendered gameplay, UI, VFX, terrain, lighting, fog, and cinematics at player-relevant moments without confusing static captures with live visual qualification.
metadata: { author: Angelis Pseftis }
---

# Echoes real-time visual review

Read `Docs/ArtDirection.md`, `Docs/Archive/DevelopmentBible.md`, the visual gates in `Docs/GameCompletionDirective.md` and `Docs/DemoRecoveryDirective.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/ProjectLedger.md`, the package identity, and `../WorkstreamControl/ACTIVE_LANES.md`. Review only a known package/build and state resolution, display mode, settings, hardware, and visual accessibility mode. Use active rendered play or a callable GUI control interface; screenshots are corroboration, never a complete motion/readability review.

Inspect title, briefing, normal camera, high-action combat, fog transitions, selection/orders, Well states, destruction, minimap/HUD, menus, cinematics, and restored default. Check for placeholders, Engine defaults, clipping, occlusion, unreadable ownership/order/threat state, flicker, frame pacing artifacts, palette/type/icon inconsistency, and visual-accessibility behavior.

Record captures, timestamps, motion observations, settings, package/asset provenance, defects, and exact conditions. Do not change authoritative state with visual components or claim final assets, performance, or release quality without their separate gates.

Before any live visual session, read `../WorkstreamControl/HEAVY_RUN_LOCK.md`, acquire a current detailed lease, and explicitly release it. Do not operate under a stale/self-invented lease. Use `echoes-package-provenance`, `echoes-performance-profiling`, and `echoes-evidence-gate-review` for the corresponding separate claims.
