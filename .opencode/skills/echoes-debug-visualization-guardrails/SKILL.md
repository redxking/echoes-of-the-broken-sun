---
name: echoes-debug-visualization-guardrails
description: Create or audit Echoes debug overlays and tuning views as non-authoritative development aids that cannot leak hidden state, contaminate evidence, or ship unintentionally.
metadata:
  author: Angelis Pseftis
---

# Echoes debug-visualization guardrails

Read `CLAUDE.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/GameCompletionDirective.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify a live owned lane before changing diagnostic code, widgets, flags, commands, or assets.

Define the diagnostic question, data source, build configuration, enable/disable path, authority boundary, hidden-information risk, capture labeling, and removal or compile-out rule before implementation. A debug view may observe authoritative state but must never mutate simulation, command validity, fog, AI, save/replay, checksum, or player progression. It must not expose concealed units or internal coordinates in player evidence.

Prove the overlay is absent or inaccessible in the qualifying package, defaults off, leaves behavior/checksums unchanged, and is clearly marked in every diagnostic capture. Never use a debug-only route, injected state, or internal overlay to establish ordinary player discovery, usability, or release appearance.

Editor/runtime checks require `echoes-heavy-run-coordination`; visual evidence routes to `echoes-realtime-visual-review`, package exclusion to `echoes-package-provenance`, and findings to `echoes-evidence-gate-review`. Stop for a shipping exposure, authority mutation, unregistered shortcut, or evidence whose debug state cannot be identified.
