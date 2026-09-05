---
name: echoes-asset-provenance-rights
description: Govern Echoes art, audio, voice, code-adjacent, and cinematic asset provenance, generation exceptions, license evidence, and AssetRegister completeness before use or release claims.
metadata:
  author: Angelis Pseftis
---

# Echoes asset provenance and rights

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/GameCompletionDirective.md` §7 and each applicable delivery track, `Docs/Archive/AssetRegister.md`, `Docs/Archive/ProjectLedger.md`, `Docs/ArtDirection.md`, `Docs/AudioDirection.md`, and `Docs/CharacterVoiceIdentityBible.md` as relevant, plus [echoes-session-control](../echoes-session-control/SKILL.md). Confirm a live task ownership before writing a register, source, generated output, or imported asset.

Classify every asset family before use: project-procedural generated, original commissioned, licensed third-party, or approved local generative exception. The register must identify source/method, inputs or stable revision, output paths, creator/rights holder where applicable, license/consent, restrictions, rationale for exceptions, and verification boundary. Generated assets are valid only when the registered pipeline reproduces the stated output/idempotence behavior.

Never treat an asset's presence, embedded metadata, marketplace label, model prompt, or URL as rights clearance. Never use scraped, ambiguous, noncommercial-only, incompatible, or unverifiable content. Do not alter compiled/generated output by hand; edit registered source and regenerate.

Acceptance output: completed register rows and supporting local evidence references, source/generated boundary check, license-risk disposition, and evidence status. Exclude legal conclusions beyond available documents and release-rights certification. Stop for a missing/ambiguous right, unrecorded exception, broken reproducibility, or ownership conflict; escalate the owner decision rather than importing.

Route open-weight TTS research to `echoes-open-weights-tts-selection`; it must verify current primary sources before a model is proposed. No generation/import/heavy inspection begins without first reading/acquiring-or-stopping on [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md). Route closure evidence to `echoes-evidence-gate-review`.
