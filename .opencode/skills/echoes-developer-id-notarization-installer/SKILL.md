---
name: echoes-developer-id-notarization-installer
description: Prepare a separately authorized Echoes Developer ID signing, notarization, stapling, and installer workflow after a provenance-verified candidate exists.
metadata: { author: Angelis Pseftis }
---

# Echoes Developer ID, notarization, and installer

Read `Docs/GameCompletionDirective.md` release gate, `Docs/Requirements.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, `../WorkstreamControl/handoffs/build-distribution.md`, `../WorkstreamControl/ACTIVE_LANES.md`, `../WorkstreamControl/HEAVY_RUN_LOCK.md`, and the exact candidate package-provenance record. This is credentialed and external-state work: keychain access, Developer ID signing, notary submission, stapling, installer creation, upload, and publication require separate explicit Angelis authority. Without it, prepare a checklist only and stop.

Before any command, acquire a current detailed Heavy-Run lease and explicitly release it afterward. Verify candidate identity, signature identity/team scope, entitlements, nested code, archive hashes, current Apple tooling, and the permitted operation. Never expose, copy, log, or store credentials, tokens, private keys, or passwords in source/evidence.

Retain exact candidate hashes, signed artifact hashes, validation output, submission identifier/status, notarization result, stapling/assessment result, installer identity and hash, dates, and claim limits. A successful upload/submission is not notarization; notarization is not clean-machine install; none is publication or `HUMAN ACCEPTED`.
