---
name: echoes-gameplay-interface-audio
description: Implement or audit Echoes gameplay and UI sound feedback so authoritative events, player acknowledgement, mix priority, and accessibility alternatives remain aligned.
metadata:
  author: Angelis Pseftis
---

# Echoes gameplay and interface audio

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Tracks B/G, `Docs/AudioDirection.md`, `Docs/Archive/DevelopmentBible.md` (§Combat and controls, §Interface and accessibility), `Docs/Archive/TechnicalArchitecture.md` (§Audio, §Presentation), `Docs/Archive/AssetRegister.md`, `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Verify live ownership before a write.

Audio is presentation-only. Bind cues to confirmed authoritative events and adapter state, never to predicted commands or a separate gameplay clock. A cue must not affect collision, navigation, fog, simulation, saves, replay, or checksums. Define what succeeds, fails, is unavailable, selected, targeted, completed, or requires attention.

Test player sequences in the running game: hover, select, issue valid and invalid commands, receive completion/failure, use modal menus, pause/resume, and create simultaneous cue load. Verify mute/category/volume behavior and non-audio equivalents for critical information. Listen for duplicate, stale, delayed, or misleading feedback.

Acceptance output: event-to-cue contract, registered asset references, gameplay listening evidence, mix/accessibility observations, and status. Exclude cue generation without provenance, automatic acceptance, and state-changing audio. Stop for missing authoritative binding, ambiguous cue meaning, unavailable fallback, or ownership conflict.

Route listening to `echoes-audio-listening-review`, control flow to `echoes-mouse-keyboard-playtest`, accessibility behavior to `echoes-ui-accessibility-playtest`, and evidence to `echoes-evidence-gate-review`. Coordinate an exclusive resource reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md) before runtime/Editor/listening-heavy execution.

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
