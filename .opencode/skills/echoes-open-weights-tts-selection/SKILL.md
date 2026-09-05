---
name: echoes-open-weights-tts-selection
description: Evaluate open-weight text-to-speech candidates for Echoes voice production using current primary-source model and license verification, consent, provenance, and canon constraints.
metadata:
  author: Angelis Pseftis
---

# Echoes open-weights TTS selection

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/CharacterVoiceIdentityBible.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, and [echoes-session-control](../echoes-session-control/SKILL.md). A live task ownership is required for any repository mutation; selection research alone does not authorize download, generation, import, or model execution.

For every candidate, verify current primary-source model card, weights license, code license, commercial-use terms, redistribution terms, voice-cloning restrictions, and documented capability/version. Record source URLs, retrieval date, version/hash where offered, applicable rights, consent, intended voice family, evaluation limitation, and owner decision in the prescribed register. Never imitate or clone a real person's voice; never infer consent from public recordings, a name, or a model feature.

Compare only against the Character Voice Identity Bible's fictional identity criteria and authorized script. A selected candidate is not an accepted voice asset, does not prove legal clearance, and does not establish intelligibility, in-engine integration, subtitle timing, or player acceptance.

Acceptance: evidence-backed candidate matrix and explicit owner decision request; no generated voice is an acceptance output. Route approved production to `echoes-voice-production`, in-engine listening to `echoes-audio-listening-review`, and evidence to `echoes-evidence-gate-review`. Before any local inference or Editor execution, coordinate an exclusive reservation through [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md). Stop for unverifiable primary sources/license, rights uncertainty, lack of consent, canon mismatch, or no owner decision.
