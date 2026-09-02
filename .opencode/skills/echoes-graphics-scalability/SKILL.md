---
name: echoes-graphics-scalability
description: Configure or validate Echoes graphics quality, frame-time, memory, and fallback behavior on the macOS Apple-Silicon baseline without misrepresenting unmeasured performance.
metadata:
  author: Angelis Pseftis
---

# Echoes graphics scalability

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Track H and the gate matrix, `Docs/Archive/TechnicalArchitecture.md` (§Performance, profiling, and Apple Silicon), `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify lane ownership before edits and preserve the M1 Pro baseline; Nanite and Virtual Shadow Maps remain off unless the owner records a decision.

Change scalable presentation settings without changing simulation, fog authority, command behavior, saves, replay, checksums, or asset provenance. Define quality tiers, target scenes, resolution/display mode, and degradation behavior before measuring. Prefer readability-preserving reductions over silently removing gameplay-relevant signals.

Run the applicable packaged/profile workflow on the Mac when evidence is required. Capture frame-time, GPU/CPU/memory behavior, scene/camera/action, settings, build identity, duration, thermal/power caveats, and failure/degraded states. Test the lowest supported tier and a representative stress scene; do not generalize to untested hardware.

Acceptance output: scoped settings change or audit, reproducible profile evidence, visual inspection, and bounded conclusion. Exclude performance guarantees, platform-port claims, or package acceptance. Stop for insufficient free storage, unavailable baseline hardware, invalid profiling run, or a regression without a readable fallback.

Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Mandatory: read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md` before every profile, build, package, Editor/runtime launch, or GPU-intensive review. Route subjective visual assessment to `echoes-realtime-visual-review` and evidence disposition to `echoes-evidence-gate-review`.
