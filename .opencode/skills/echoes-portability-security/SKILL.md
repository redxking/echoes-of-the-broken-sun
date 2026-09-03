---
name: echoes-portability-security
description: "Assess or implement Echoes macOS-first release hardening, data integrity, privacy, and future-platform guardrails without overstating shipping or security evidence."
metadata:
  author: Angelis Pseftis
---

# Echoes portability and security

Use for packaging, signing/notarization readiness, local data/privacy, network-disabled release posture, dependency risk, platform portability, or secure failure handling.

1. Read live `CLAUDE.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify exact lease/worktree/branch/dirty paths before mutation.
2. Treat macOS Apple Silicon as the current release target; Linux/SteamOS and Windows are future compatibility constraints, not shipping claims. Multiplayer ships disabled; do not reactivate network features without owner authorization and requirements.
3. Preserve fail-closed content, save, replay, and package verification. Keep secrets, credentials, user data, telemetry, local model assets, and signing materials out of source and evidence artifacts. Do not log sensitive values.
4. Make signing, notarization, installability, clean-machine launch, dependency/license, and package-manifest claims only from their named executed checks. A development package, static scan, or local build is not shipping evidence.
5. Stop for missing signing authority, credential access, license uncertainty, unleased build scripts, package mismatch, unmounted storage, or owner decision on privacy/distribution.

## Acceptance checks

Record platform/architecture, commit, build configuration, package/signature/notarization/install results, manifest/digest checks, security finding disposition, and evidence limits. Route operational checks to `echoes-workstation-toolchain-readiness`, `echoes-heavy-run-coordination`, `echoes-security-privacy`, and `echoes-platform-portability`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.
