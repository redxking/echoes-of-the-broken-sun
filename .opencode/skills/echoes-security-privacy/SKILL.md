---
name: echoes-security-privacy
description: "Assess Echoes security, privacy, local data, credentials, package integrity, and failure handling with evidence-bounded claims and no secret exposure."
metadata:
  author: Angelis Pseftis
---

# Echoes security and privacy

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, and [echoes-session-control](../echoes-session-control/SKILL.md); verify task ownership, worktree, branch, and SHA before mutation.

Identify data classes, local storage, save/replay integrity, credentials, telemetry, logs, anti-cheat boundaries, modding/plugin/script/network boundaries, package/signing inputs, offline-mode network isolation and session-scoped network execution, dependencies, licenses, and recovery behavior. The owner activated bounded session multiplayer on 2026-09-04. Apply `REL-MP-012/016/018/019` to authentication, trust, relay/service boundaries, abuse, recovery and privacy; the skill grants no additional service/deployment scope or implementation claim. Keep secrets and user data out of source, screenshots, terminal logs, and evidence. Generated manifests and hashes must derive from official sources; do not treat their existence as a security claim.

Use safe non-destructive checks, preserve findings, and bind each claim to an observed package/build/host. Route persistence to `echoes-save-progression-recovery`, platform work to `echoes-platform-portability`, heavy package checks to `echoes-heavy-run-coordination`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`. Stop for secrets, missing authorization, active production service, uncertain license, or a remediation outside assigned path scope.
