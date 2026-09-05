---
name: echoes-tutorial-onboarding
description: "Design and validate Echoes onboarding that teaches authorized play through physical input, observable mastery, accessibility behavior, and recoverable failure states."
metadata:
  author: Angelis Pseftis
---

# Echoes tutorial and onboarding

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use for first-five-minutes flow, lessons, prompts, system voice, affordances, tutorial gating, or new-player recovery.

1. Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, campaign/narrative source contracts, and [echoes-session-control](../echoes-session-control/SKILL.md). Verify task ownership, worktree, branch, and dirty paths before mutation.
2. Define each lesson as intent, required physical action, success observation, misconception/failure path, feedback, retry/skip policy, accessibility behavior, and evidence required. Do not equate a displayed prompt with learned behavior.
3. Keep authoritative lesson prerequisites and state in approved simulation/content contracts; UI/system voice renders and guides but cannot grant success without the specified authoritative condition.
4. Test a cold-start player path with mouse and keyboard, including focus loss, modal interruption, wrong input, timing, pause/Escape, reduced-motion/high-contrast/audio settings, and recovery. Distinguish agent GUI runs from owner human play.
5. Stop for changes to frozen mission contracts, tutorial/campaign ownership conflict, unapproved Annunciator/narrative scope, missing actual input path, or owner decision on pedagogy.

## Acceptance checks

Record lesson IDs, source digest, build/commit, input method, outcomes for success/failure/retry/skip/accessibility paths, and evidence class. Route GUI capability to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`; only Angelis can state that onboarding is understandable or accepted.
