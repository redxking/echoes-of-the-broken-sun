---
name: echoes-qa-defect-triage
description: Triage Echoes defects into evidence-bound severity, reproducibility, ownership, and release-risk records without assigning closure or altering release criteria.
metadata: { author: Angelis Pseftis }
---

# Echoes QA defect triage

Read `CLAUDE.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/GameCompletionDirective.md`, `Docs/InitialReleaseRequirements.md`, `Docs/Archive/ProjectLedger.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and the originating evidence. Capture package/build/commit, environment, preconditions, exact steps, expected/actual result, fresh state, logs/captures, reproducibility, affected player path, workarounds, and lane conflict. Do not modify source or evidence while triaging.

Use S0–S4: S0 safety/security/data-loss or universal launch-blocker; S1 critical player-path/progression/crash/corruption defect; S2 material feature, accessibility, performance, or UX failure with a bounded workaround; S3 minor or isolated degradation; S4 cosmetic/documentation observation. Severity describes observed impact, not urgency theater. Unknown reproducibility remains unknown.

No known S0 or S1 may remain on a release-critical path. No known S2 may remain there absent an explicit Angelis waiver recorded against the exact defect/build; documentation or a bounded workaround is not a waiver. Route player-path findings to the relevant GUI/playtest skill, package identity to `echoes-package-provenance`, and release assessment to `echoes-regression-release-blockers`. Only Angelis authorizes human acceptance/rejection, an S2 waiver, or closure.
