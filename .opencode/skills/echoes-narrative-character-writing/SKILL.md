---
name: echoes-narrative-character-writing
description: "Write, revise, or validate Echoes dialogue, characters, subtitles, and cinematic text while protecting canon, pinned mission contracts, and player comprehension."
metadata:
  author: Angelis Pseftis
---

# Echoes narrative and character writing

Use for story, dialogue, voice direction, subtitle text, character arcs, briefs, or cinematic writing. It does not authorize new canon.

1. Read live `CLAUDE.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Requirements.md`, current `Content/Narrative/Source` mission contract, and `../WorkstreamControl/ACTIVE_LANES.md`. Verify exact lease/worktree/branch/dirty paths before mutation.
2. Preserve canon characters, faction voice, world terms, mission order, and authorized endings. Do not invent events, relationships, speakers, or plot resolution beyond `Docs/Archive/DevelopmentBible.md`; send genuine gaps as an owner decision packet.
3. Treat mission pins and compiler invariants as contracts. Edit only authorized narrative source and rebuild through the official pipeline; never alter compiled packs, expected counts, hashes, or validators to accommodate prose.
4. Check narrative intent alongside operational clarity: trigger timing, gameplay-state binding tokens, subtitle readability, accessibility, local-TTS provenance, and whether a player can act on the line.
5. Stop for frozen source, unleased mission, unapproved voice/asset provenance, conflict with gameplay timing, or owner-only creative choice.

## Acceptance checks

Record source/pack digest, validator results, exact rendered or audio/subtitle exercise, canon citations, and limitations. Route voice/listening readiness to `echoes-gui-control-readiness`, then `echoes-evidence-gate-review` and `echoes-human-acceptance-session`; text review is not voice performance, cinematic, or owner acceptance evidence.
