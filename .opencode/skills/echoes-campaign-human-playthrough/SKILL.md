---
name: echoes-campaign-human-playthrough
description: Evidence a visible fresh-ledger Echoes campaign journey through rendered UI and physical-input-equivalent mouse/keyboard events while preserving the distinction between agent operation and human acceptance.
metadata: { author: Angelis Pseftis }
---

# Echoes campaign journey playthrough

Read `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, the campaign contracts in `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/TechnicalArchitecture.md`, the exact `echoes-package-provenance` record, and `../WorkstreamControl/ACTIVE_LANES.md`. Require a GUI-control interface with fresh state/screenshots and actual mouse/keyboard events. If absent, stop and request human/tool access. Start from a verified empty player ledger through the normal title/new-campaign surface; record save location and isolation.

For every mission record title, inherited choices, briefing comprehension, deployment, core objective, visible input sequence, audio/subtitle/cinematic behavior, terminal result, continuation, save transition, and exact package identity. Work only through player-facing UI. No Unreal MCP, console/debug mode, direct simulation, hidden commands, save injection, test route, or source-guided coordinate shortcut. A failure/retry belongs in the record.

Capture timings, input/state trail, audio observations, defects, package and save hashes, and final ledger result. A model-operated route is rendered evidence, not proof that an unfamiliar person can complete the campaign and never `HUMAN ACCEPTED`.

Before any rendered campaign run, read `../WorkstreamControl/HEAVY_RUN_LOCK.md`, acquire a current detailed lease, and explicitly release it. Never use a stale or self-authored lease. Verify package identity through `echoes-package-provenance`; route human acceptance only to `echoes-human-acceptance-session`.
