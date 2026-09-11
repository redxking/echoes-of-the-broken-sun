---
name: echoes-story-character-experience-review
description: Review Echoes narrative, character, dialogue, subtitles, choices, and cinematic experience against the Development Bible without inventing canon or claiming unplayed story delivery.
metadata:
  author: Angelis Pseftis
---

# Echoes story and character experience review

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

Read the relevant sections of `Docs/Archive/DevelopmentBible.md`, `Docs/OpeningAndTutorialScript.md`, the mission narrative source, Track D in `Docs/GameCompletionDirective.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/CharacterVoiceIdentityBible.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). Treat the Bible as creative authority; implementation files and past summaries are not permission to invent content.

Trace a named player-facing path: setup, spoken or written line, speaker, subtitle, trigger, choice, gameplay consequence, later callback, and ending boundary. Inspect delivery in rendered play where available; record whether voice, subtitle timing, camera/cinematic, UI emphasis, and choice feedback are actually observed. Evaluate clarity, continuity, character voice, stakes, agency, and absence of contradictions without manufacturing personal reactions or asserting broad audience response.

Separate authored design, implemented text, automated trigger evidence, rendered agent observation, and human evaluation. New lore, lines, character claims, ending consequences, or voice production changes require their owning task ownership and provenance; owner-only `HUMAN ACCEPTED` is never inferred.

If reviewing rendered story/cinematics, read [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md), confirm an exclusive resource reservation, then record and release the reservation; otherwise stop. Bind the viewed build through `echoes-package-provenance` or clearly state editor boundary.

For the 15 unique mission maps M01–M15, review the applicable master requirements for a distinct story-driven mission identity, connected geography and narrative progression, and continuous character development. Assess the RTS-appropriate massive-world feeling through authored briefings, recurring landmarks, transitions, consequences, and callbacks; do not infer an MMO, shared-world, or multiplayer commitment. Missing backstory, a character turn, or a cross-mission connection is a `TBR-*` decision, never material to invent during review.
