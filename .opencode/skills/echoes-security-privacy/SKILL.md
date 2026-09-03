---
name: echoes-security-privacy
description: "Assess Echoes security, privacy, local data, credentials, package integrity, and failure handling with evidence-bounded claims and no secret exposure."
metadata:
  author: Angelis Pseftis
---

# Echoes security and privacy

Read `CLAUDE.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`; verify lease/worktree/branch/SHA before mutation.

Identify data classes, local storage, save/replay integrity, credentials, telemetry, logs, anti-cheat boundaries, modding/plugin/script/network boundaries, package/signing inputs, network-disabled release posture, dependencies, licenses, and recovery behavior. Multiplayer/replication work is conditional only when the live `REL-MP-*` scope is owner-activated; do not activate it here. Keep secrets and user data out of source, screenshots, terminal logs, and evidence. Generated manifests and hashes must derive from official sources; do not treat their existence as a security claim.

Use safe non-destructive checks, preserve findings, and bind each claim to an observed package/build/host. Route persistence to `echoes-save-progression-recovery`, platform work to `echoes-platform-portability`, heavy package checks to `echoes-heavy-run-coordination`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop for secrets, missing authorization, active production service, uncertain license, or a remediation outside lane scope.
