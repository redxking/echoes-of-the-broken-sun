---
name: echoes-regression-release-blockers
description: Assess Echoes regression and release-blocker status from current evidence, known defects, package provenance, and human-acceptance boundaries without declaring release closure.
metadata: { author: Angelis Pseftis }
---

# Echoes regression and release blockers

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md`, `Docs/Archive/ProjectLedger.md`, `../WorkstreamControl/ACTIVE_LANES.md`, `../WorkstreamControl/HEAVY_RUN_LOCK.md`, package provenance, latest defect triage, and controlling gate evidence. Build a current matrix: release-critical path, package identity, latest regression boundary, open S0–S4 defects, human/clean-machine evidence, ownership, and next verification. Do not infer that a historical green suite covers a changed package or player path.

Treat any known S0 or S1 on a release-critical path as a blocker until corrected and re-evidenced. Zero known S2 may remain on a release-critical path absent an explicit Angelis waiver bound to the exact defect/build. S2–S4 still require documented impact, ownership, workaround status, and gate relevance; documentation alone is not a waiver and defects do not silently disappear. Re-run work only with the correct lane and, for build/package/profile/soak/GUI/player/audio/visual activity, a current detailed Heavy-Run lease that is explicitly released afterward.

Use DemoRecovery engineering states only: `OPEN`, `IN PROGRESS`, `IMPLEMENTED — NOT YET VERIFIED`, `AGENT VERIFIED`, `EVIDENCE READY`, `AWAITING HUMAN ACCEPTANCE`, or `BLOCKED`. Only Angelis assigns `HUMAN ACCEPTED` or `HUMAN REJECTED — CHANGES REQUIRED`; `COMPLETE`/`PASS` closure is reserved. This is assessment, not authority to ship, upload, publish, or edit gates.
