---
name: echoes-stability-soak-crash
description: Run and assess Echoes sustained-load, crash, recovery, and save-interruption evidence without claiming power-loss, clean-machine, or human-play validation that was not exercised.
metadata: { author: Angelis Pseftis }
---

# Echoes stability, soak, and crash review

Read `Docs/GameCompletionDirective.md`, `CLAUDE.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/TechnicalArchitecture.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`. Acquire a current detailed Heavy-Run lease before any run and explicitly release it afterward; otherwise stop. Confirm the exact package, commit, dirty state, free storage, and isolated save location before beginning. A current provenance-verified package is mandatory for package claims; route there through `echoes-package-provenance`.

For a soak, maintain the required active workload continuously through warm-up and the full approved duration; retain periodic frame/memory samples and terminal state. For recovery, use only the explicitly authorized interruption method and retain before/after save manifests, hashes, process logs, and visible recovery behavior. Separate controlled API/rename failure, process termination, filesystem crash consistency, and power-loss evidence; none substitutes for another.

Treat crash, assertion, warning, corrupted save, unexplained state change, or resource exhaustion as a defect or blocker. Never inject saves, edit ledgers, use debug flags, or clear failures to obtain a pass. Record exact observed scope and unproven durability; owner acceptance remains separate.

Capture target-hardware memory, thermal/throttling boundary where observable, and active-workload heartbeats; missing telemetry remains a limitation. Cross-device crash/recovery results require separate authorized devices and cannot be generalized from one Mac.
