---
name: echoes-campaign-missions
description: "Implement or validate one Echoes campaign operation, its objectives, state transitions, checkpoints, and win/loss paths against authorized mission contracts."
metadata:
  author: Angelis Pseftis
---

# Echoes campaign missions

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Use for M01–M15 mission systems, objective triggers, campaign progression, briefing-to-results flow, or mission verification.

1. Read live [Project/AGENTS.md](../../../AGENTS.md), `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/DemoRecoveryDirective.md`, `Docs/Requirements.md`, `Content/Narrative/Source`, and [echoes-session-control](../echoes-session-control/SKILL.md). Verify task ownership, worktree, branch, and dirty paths before mutation.
2. Preserve mission contract IDs, source pins, campaign order, objective semantics, and canonical outcomes. Mission/narrative source changes go through the approved source compiler; never patch compiled packs or relax validators.
3. Specify the ordinary-player path from empty ledger/entry through briefing, action, objective completion, failure, retry/checkpoint, result, and next-state continuity. Include all required loss and recovery conditions.
4. Validate deterministic mission logic and then exercise the rendered physical-input path. Record which mission sections were automated, GUI-driven, or observed by a human; screenshots alone do not prove the flow.
5. Stop for a frozen contract, narrative/canon ambiguity, campaign ownership conflict, owner decision on pacing/outcome, or inability to test the real entry path.

## Acceptance checks

Capture mission ID, source digest, save/progression state, input method, build/commit, objective and recovery outcomes, logs/captures, and evidence class. Route narrative to `echoes-narrative-character-writing`, persistence to `echoes-save-progression-recovery`, GUI exercise to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`.

For the 15 unique mission maps M01–M15, use the applicable master requirements and creative authority to preserve one distinct, story-driven mission identity per operation, with connected geography, escalating narrative context, and character continuity across the campaign. Express the sense of a massive connected world through RTS-appropriate briefings, transitions, objectives, landmarks, and consequences; it does not authorize MMO or shared persistent-world scope. The separate 2026-09-04 approval activates Conquest and bounded team/FFA multiplayer through the master; it does not make the story campaign cooperative. Do not invent mission events, backstory, regional links, or character outcomes: record a material gap as a `TBR-*` decision.
