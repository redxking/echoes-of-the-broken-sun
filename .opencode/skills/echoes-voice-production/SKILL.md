---
name: echoes-voice-production
description: Produce or review Echoes character, system, and mission voice with canon fidelity, documented performer/model rights, intelligibility, and in-engine listening evidence.
metadata:
  author: Angelis Pseftis
---

# Echoes voice production

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` Track C and §7, `Docs/CharacterVoiceIdentityBible.md`, `Docs/Archive/DevelopmentBible.md` (§Writing rules), `Docs/OpeningAndTutorialScript.md` or the authoritative mission source as relevant, `Docs/AudioDirection.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/ProjectLedger.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). A current task ownership is required for edits.

Do not invent dialogue, speakers, character traits, mission facts, or line placement. Treat canonical scripts and digest-verified mission contracts as immutable unless an authorized owner decision and task ownership explicitly cover them. For every line/family, record speaker, source text identity, take/source method, performer or model rights, license/consent, processing, revision, and generated output in the designated register.

Validate in engine for pronunciation, identity separation, pacing, interruption, subtitle sync, mix intelligibility, reduced dynamic range, and repeat suppression. A waveform, generated file, or clean import is not evidence that a player can understand it.

Acceptance output: canon/provenance record, runtime listening evidence, subtitle linkage check, limitations, and truthful evidence state. Exclude imitation of real voices without documented authority, fabricated canon, and mission-contract changes outside live task ownership. Stop for rights uncertainty, script mismatch, unavailable line ownership, or unresolved intelligibility.

For open-weight voice-model selection use `echoes-open-weights-tts-selection`; this skill does not decide model suitability. Route listening to `echoes-audio-listening-review`, subtitle runtime behavior to `echoes-subtitle-caption-runtime`, evidence to `echoes-evidence-gate-review`, and owner acceptance to `echoes-human-acceptance-session`; before local inference, Editor, runtime listening, or other resource-intensive execution, coordinate an exclusive reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md).

Before production or review, read the applicable master requirement and its linked context brief; when in scope, apply `SPEC-VISD-008` and `SPEC-ART-004` as written. Do not infer unapproved detail, motion, sound, or role meaning.
