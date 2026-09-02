---
name: echoes-subtitles-localization
description: Implement or audit Echoes subtitles, captions, text presentation, and localization-ready content linkage while preserving canonical source, timing, readability, and player control.
metadata:
  author: Angelis Pseftis
---

# Echoes subtitles and localization

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Tracks C/G, `Docs/CharacterVoiceIdentityBible.md`, `Docs/Archive/DevelopmentBible.md` (§Writing rules, §Interface and accessibility), the authoritative narrative source, `Docs/Archive/TechnicalArchitecture.md` (§UI and accessibility), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Check live ownership before changing source, UI, or generated output.

Preserve canonical text and digest-verified source boundaries. Do not silently rewrite mission narrative for localization or invent translated content; propose unresolved copy/locale decisions to the owner. Keep speaker identification, timing, interruption/replay behavior, text scale, contrast, safe area, and user enable/disable behavior explicit.

Validate during live playback with voice where present: line start/end, overlap, skip, pause/resume, cinematic and gameplay camera, maximum line load, text scale, and high contrast. Confirm critical audio-only signals retain text or non-audio alternatives where required. Treat a subtitle file or static menu view as insufficient evidence.

Acceptance output: source-to-display mapping, runtime timing/readability evidence, locale readiness limitations, and status. Exclude unsupported language claims, canon edits outside lane, and timing claims without playback. Stop for missing source ownership, unsynchronized voice, inaccessible text behavior, or unknown localization rights/process.

This combined router delegates actual captions/subtitle behavior to `echoes-subtitle-caption-runtime` and language/process readiness to `echoes-localization-readiness`. Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. Use `echoes-ui-accessibility-playtest`, `echoes-audio-listening-review`, and `echoes-evidence-gate-review`; runtime playback requires read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md`.
