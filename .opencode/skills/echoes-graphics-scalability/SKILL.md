---
name: echoes-graphics-scalability
description: Configure or validate Echoes graphics quality, frame-time, memory, and fallback behavior on the macOS Apple-Silicon baseline without misrepresenting unmeasured performance.
metadata:
  author: Angelis Pseftis
---

# Echoes graphics scalability

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Track H and the gate matrix, `Docs/Archive/TechnicalArchitecture.md` (§Performance, profiling, and Apple Silicon), `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Verify path ownership before edits and preserve the M1 Pro baseline; Nanite and Virtual Shadow Maps remain off unless the owner records a decision.

Change scalable presentation settings without changing simulation, fog authority, command behavior, saves, replay, checksums, or asset provenance. Define quality tiers, target scenes, resolution/display mode, and degradation behavior before measuring. Prefer readability-preserving reductions over silently removing gameplay-relevant signals.

Run the applicable packaged/profile workflow on the Mac when evidence is required. Capture frame-time, GPU/CPU/memory behavior, scene/camera/action, settings, build identity, duration, thermal/power caveats, and failure/degraded states. Test the lowest supported tier and a representative stress scene; do not generalize to untested hardware.

Acceptance output: scoped settings change or audit, reproducible profile evidence, visual inspection, and bounded conclusion. Exclude performance guarantees, platform-port claims, or package acceptance. Stop for insufficient free storage, unavailable baseline hardware, invalid profiling run, or a regression without a readable fallback.

Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before every profile, build, package, Editor/runtime launch, or GPU-intensive review. Route subjective visual assessment to `echoes-realtime-visual-review` and evidence disposition to `echoes-evidence-gate-review`.
