---
name: echoes-support-readiness
description: Prepare Echoes support intake, reproduction, privacy, escalation, and limitation handling without claiming a shipped support operation or contacting users.
metadata: { author: Angelis Pseftis }
---

# Echoes support readiness

Read `echoes-player-manual-known-limitations`, `Docs/Archive/ProjectLedger.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `echoes-security-privacy`. Define intake fields that minimize personal data: package/version/hash, macOS/hardware, install route, repro steps, expected/actual behavior, screenshots/logs only with consent, and accessibility impact. Never request secrets, credentials, private saves, or unrelated machine data.

Classify reports through `echoes-qa-defect-triage`; reproduce only in isolated paths. State current evidence and workarounds honestly; do not promise release dates, fixes, compatibility, refunds, service levels, or security conclusions. External support contact, accounts, telemetry, tickets, and publication require explicit owner authority.

For any actual GUI/package reproduction, read `../WorkstreamControl/HEAVY_RUN_LOCK.md`, acquire a current detailed lease, and explicitly release it. Route package identity to `echoes-package-provenance` and release decision to `echoes-regression-release-blockers`.

Before recommending a third-party skill, script, hook, dependency, updater, or network service, review source, revision/hash, license, privileges, data flow, and support impact. Catalog/scanner findings are inputs to review, not a security guarantee.
