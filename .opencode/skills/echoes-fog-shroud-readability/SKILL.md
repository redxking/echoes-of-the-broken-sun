---
name: echoes-fog-shroud-readability
description: Review or improve what players can read through Echoes fog, shroud, occlusion, selection, and ownership presentation without modifying visibility authority.
metadata:
  author: Angelis Pseftis
---

# Echoes fog and shroud readability

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Tracks A/E/F/G, `Docs/Archive/DevelopmentBible.md` (§Combat and controls, §Interface and accessibility), `Docs/Archive/TechnicalArchitecture.md` (§Visibility, detection, and fog; §Presentation), `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Obtain a live task ownership for any write.

Fog and detection truth belongs exclusively to deterministic simulation. Presentation may visualize only information already exposed through the authoritative adapter; it must not reveal concealed units, infer future state, alter targetability, or create an alternate fog state. Explicitly distinguish visible, detected, remembered, and unknown states using the project-approved semantics—not color alone.

Check camera-distance readability for unit silhouettes, terrain, objectives, Future Wells, selection, and ownership across normal, high-contrast, color-vision, and reduced-effects settings. Exercise reveal/loss-of-vision transitions, multiplayer/replay-safe paths where relevant, and crowded engagements. Use real interaction/play evidence, not static captures alone.

Acceptance output: state-to-presentation mapping, rendered interaction observations, accessibility matrix, and bounded evidence. Exclude fog-rule, simulation, replay, or checksum changes. Stop if adapter data cannot establish the state, a presentation leaks information, an ownership marker depends solely on color, or a task ownership is not established.

GPU/rendered review requires reading and coordinator acquisition of [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md), or stopping. Route rendered judgement to `echoes-realtime-visual-review`, accessibility behavior to `echoes-ui-accessibility-playtest`, and evidence disposition to `echoes-evidence-gate-review`.
