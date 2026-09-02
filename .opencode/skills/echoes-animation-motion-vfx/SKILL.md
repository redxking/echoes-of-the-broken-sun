---
name: echoes-animation-motion-vfx
description: Implement or review Echoes animation, camera motion, and VFX as presentation-only feedback with gameplay readability, reduced-motion behavior, and evidence from real play paths.
metadata:
  author: Angelis Pseftis
---

# Echoes animation, motion, and VFX

Read `CLAUDE.md`, `Docs/GameCompletionDirective.md` Tracks A/G/H, `Docs/Archive/DevelopmentBible.md` (§Combat and controls, §Art and audio), `Docs/ArtDirection.md`, `Docs/Archive/TechnicalArchitecture.md` (§Presentation), `Docs/Archive/ProjectLedger.md`, and `../WorkstreamControl/ACTIVE_LANES.md`. Mutate only under a live owned lane.

Derive motion from authoritative events; never feed animation, particles, camera shake, or timing back into simulation, commands, fog, saves, replay, checksums, collision, overlaps, or navigation. Treat effect timing as communicative rather than authoritative. Keep threats, order outcomes, selection, and ownership legible at normal RTS zoom.

Specify reduced-motion and reduced-flashing behavior before implementation: disable/attenuate shake, strobing, large screen displacement, and high-frequency flashes while retaining equivalent non-motion feedback. Test normal, reduced-motion, and reduced-flashing paths in the running game, including interrupted, repeated, and congested effects.

Acceptance output: authoritative event binding, behavior matrix, in-motion captured inspection, performance observation, and evidence status. Exclude unbound spectacle, simulated state changes, and screenshot-only approval. Stop for a missing authoritative event, unavailable accessibility alternative, lane conflict, or unmeasured frame/legibility regression.

This combined skill routes animation systems to `echoes-animation-systems` and effects to `echoes-vfx-effects`; do not duplicate their implementation detail. Use exact authorities `Docs/Archive/DevelopmentBible.md`, `Docs/Archive/TechnicalArchitecture.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Archive/AssetRegister.md`, `Docs/Archive/SetupAndBuild.md`, current `Docs/` directives/ledgers, and `../WorkstreamControl/ACTIVE_LANES.md`. For runtime/GPU review, read/acquire-or-stop on `../WorkstreamControl/HEAVY_RUN_LOCK.md`, then use `echoes-realtime-visual-review`; route accessibility interaction to `echoes-ui-accessibility-playtest` and gate evidence to `echoes-evidence-gate-review`.
