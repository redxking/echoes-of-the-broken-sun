---
name: echoes-qa-defect-triage
description: Triage Echoes defects into evidence-bound severity, reproducibility, ownership, and release-risk records without assigning closure or altering release criteria.
metadata:
  author: Angelis Pseftis
---

# Echoes QA defect triage

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Docs/GameCompletionDirective.md`, `Docs/Requirements.md`, `Docs/Archive/ProjectLedger.md`, [echoes-session-control](../echoes-session-control/SKILL.md), and the originating evidence. Capture package/build/commit, environment, preconditions, exact steps, expected/actual result, fresh state, logs/captures, reproducibility, affected player path, workarounds, and ownership conflict. Do not modify source or evidence while triaging.

Use S0–S4: S0 safety/security/data-loss or universal launch-blocker; S1 critical player-path/progression/crash/corruption defect; S2 material feature, accessibility, performance, or UX failure with a bounded workaround; S3 minor or isolated degradation; S4 cosmetic/documentation observation. Severity describes observed impact, not urgency theater. Unknown reproducibility remains unknown.

No known S0 or S1 may remain on a release-critical path. No known S2 may remain there absent an explicit Angelis waiver recorded against the exact defect/build; documentation or a bounded workaround is not a waiver. Route player-path findings to the relevant GUI/playtest skill, package identity to `echoes-package-provenance`, and release assessment to `echoes-regression-release-blockers`. Only Angelis authorizes human acceptance/rejection, an S2 waiver, or closure.
