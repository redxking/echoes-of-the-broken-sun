---
name: echoes-performance-profiling
description: Profile current Echoes packaged or editor workloads against the M1 Pro budgets without mistaking automation, startup, or terminal matches for sustained gameplay evidence.
metadata:
  author: Angelis Pseftis
---

# Echoes performance profiling

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/SetupAndBuild.md`, [echoes-session-control](../echoes-session-control/SKILL.md), the current task handoff, and [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md). Before any profile, confirm an exclusive resource reservation through the live control process; record and release the reservation afterward. A stale, unconfirmed, or unrelated reservation is invalid. Profile the exact current commit and package identity; first state whether the run is editor, Development package, or signed Shipping package. The M1 Pro baseline keeps Nanite and VSM off. Do not profile while a local coding model competes for GPU/unified memory.

Use `Scripts/profile_packaged_macos.sh` only after `echoes-package-provenance` verifies the package. Capture workload definition, warm-up, active-unit/faction count, map/mission, resolution/settings, duration, frame, GPU, render-thread, game-thread, fog, path burst, save timing, resident memory, and terminal-state observations. Bind each relevant gate to its precise threshold: frame p95 <=16.67 ms; game p95 <=4.0 ms; render plus GPU p95 <=11.0 ms; fog p95 <=1.5 ms; path burst <=6.0 ms; resident memory <=10 GB; save <=250 ms. The 400-unit/four-team stress profile, 600-active-second preflight, uninterrupted 60-minute same-package rendered session, and multi-hour AI soak are distinct evidence, not interchangeable runs.

Reject a soak if the match terminates before warm-up or if the workload becomes inactive. Preserve raw profiler/log output, hashes, and package identity. Report measured results and gaps; do not call a diagnostic, editor, or short run release performance qualification.

Record target-hardware thermal/power/throttling observations when available, including ambient/charging/display conditions and sensor/tool boundary; absence of a reliable measurement is a stated gap. Cross-device performance checks require separately authorized hardware and their own package identities.
