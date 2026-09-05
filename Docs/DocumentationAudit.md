# Agent documentation synchronization audit

**Author and owner:** Angelis Pseftis
**Audit date:** 2026-09-04T19:58:22+00:00
**Standing:** maintenance evidence and coverage record; no normative or lifecycle authority.

The audit inspected the 119 original project-owned Markdown files: 33 documents/entry files and
86 canonical skills. It updated 118 of those in place; `WorldMapWorkLog.md` remains the active production
task's handoff and was read without editing it. That other task appended updates during the audit.
The audit also added Gemini/Copilot entry pointers, a read-only document checker, and this single report.
Workspace-level AGENTS/Claude/Gemini pointers route sessions opened above `Project/` into the same contract.
Third-party dependency documentation/license notices were excluded from project-policy editing. Five
existing generated Markdown evidence reports inventoried under `Project/BuildArtifacts/` were preserved.

The checkout started at `fc05cdf` on `release/world-map-concept-pass` with extensive pre-existing dirty
code/assets. The task changed documents/skills, removed the `AGENTS.md` ignore rule, and added
`Scripts/check_agent_docs.py`; it did not modify game source/assets, run builds, commit, push, or publish.
Other active work changed source, asset-register entries, and the world handoff during this audit. Those
changes were preserved; they are not this audit's implementation or validation evidence.

## Completed corrections

- [AGENTS.md](../AGENTS.md) now owns the shared operating contract. Client entry points refer to it.
  The contract is no longer hidden by `.gitignore`; it is a new local file and must be included explicitly
  when these changes are eventually committed. Workspace-level pointers are outside the Git checkout.
- [Docs/README.md](README.md) separates normative requirements, lifecycle/decisions, creative canon,
  operational references, and history. Archived paths that still hold active creative/build/provenance
  references are explained. The old untracked/no-history assertion was removed.
- All 86 skills use the same authority pointers. Missing external lane files and old Claude/Codex role
  assignments are retired. Live path ownership and exclusive heavy resources remain required where relevant.
- Eight combined skills are short compatibility routers. Specific domain checks remain in leaf skills;
  requirements authoring retains systematic derivation, existing acceptance cards, independent failure
  leaves, exact evidence classes, and owner-decision boundaries.
- Prompts, release/recovery sequencing, build references, art/audio direction, and public/security text
  were cleaned of stale instructions and unsupported current-state claims. Numeric audio direction now
  defers to the exact master body instead of the retired local ducking recipe.
- Historical owner feedback, state change-log entries, run receipts, and third-party provenance remain.
  Old gate PASS notes are explicitly historical engineering reports, not current owner acceptance.
- The master's governance references now use the common contract and sole state record. This editorial
  synchronization does not resolve all pre-existing semantic conflicts in the requirements themselves.

## Owner clarifications incorporated

[Requirements.md](Requirements.md) now states the following explicitly, with corresponding OPEN records in
[RequirementsState.md](RequirementsState.md):

| Record | Required outcome |
|---|---|
| `SPEC-MAP-004` | Fifteen unique story-driven campaign maps, one per M01–M15, with distinct spatial/presentation identity and source bindings. |
| `SPEC-CAM-041` | Connected Soryn geography, campaign transitions, and earned-story continuity. |
| `SPEC-CAM-042` | Story, characters, backstories, places, objectives, and consequences expressed consistently in the game. |
| `SPEC-VISD-008` | Contextual brief before production: story point, purpose, what belongs/does not, meaningful large/fine detail, material, action, sound, and constraints. |
| `SPEC-ART-004` | Readable unit/building function and coherent craftsmanship across environments, form, motion, feedback, and sound. |

Existing product/campaign, world, animation, and material-sound records were clarified in place. The MMO
comparison describes the intended scale/connection within the RTS; it does not authorize MMO systems.
Campaign maps are counted separately from the existing three-map skirmish baseline. These edits do not
prove that current maps/assets already meet the new requirements or constitute a finished per-asset design.

## Verification and limits

Run from the checkout:

```sh
python3 Scripts/check_agent_docs.py
git diff --check
```

The checker validates the Markdown inventory, owner metadata, client/skill authority pointers, symlink
bridges, local links and heading anchors, obsolete active-control references, and the five new
requirement-definition/index/state bindings. It is a structural guard, not a semantic requirements audit
or game test. The session also performed internal read-only expert review and corrected its findings.
That review is internal QA, not independent validation.

The original append-only state history is retained verbatim. The five generated Markdown evidence files
were byte-checked against the starting snapshot and unchanged. Pre-existing asset rows received no edits
from this task; concurrent production corrected an asset ID and added a new sky record, which were retained.
Authorship fields in edited documents/skills identify Angelis Pseftis. No version-copy documents were made.

## Authorized reconciliation after the operating-guidance pass

The owner subsequently authorized resolving the ID/index and map/story conflicts and using M01 as the
representative qualification. The same documents were edited in place. The master now contains **1,125
parent identities and 1,892 subordinate definitions**, with one parent definition/index row per ID and
unique subordinate IDs; retained tombstones are included in those counts. The new registry checker also
checks family-navigation counts and truncated records. This is structural coverage, not proof of complete
semantic consistency, an exact test/evidence matrix, or implementation.

Nine ambiguous base IDs are now explicit tombstones with eighteen titled successor bindings. Exact
collided clauses remain under the successors, and duplicate same-meaning authority/outcome/technology
copies refer to one body. Scenario and asset/interface cards have distinct subordinate IDs. Fifty lost
body records were recovered from the committed masters at `2ca9e059` and `67a44c3`, with exact provenance
retained in the evidence receipt and restoration recorded in RequirementsState.md. No old evidence or
owner decision is relabeled as current successor acceptance. The theme/tone duplicate IDs point back to
their restored stable originals. Missing aggregate parent identities register already existing groups.

The map documents now have one current fifteen-row story/place brief plus an M01 asset/action/sound brief
and end-to-end qualification sequence. Obsolete mission profiles and their invented coordinates, casualty
claims, geographic events and mismatched six-map lists were removed from active guidance. M06/M08 command
roles and the M08 plan were reconciled to the detailed mission contracts and creative Bible. M09–M12
retain their authored allocation/contact/public-evidence/readback purposes. Source bindings and actual
map/narrative/visual/audio quality remain unfinished, separately owned production work.

The owner explicitly selected **including Conquest/roguelite, team battles and free-for-all in this
release**. The master, public description, map references, delivery directive and affected skills now
carry that scope. `SPEC-SKM-014..018` define format/participant/team/session/map eligibility; `REL-MP-018/019`
define full packaged-format qualification and trust/service contracts. Six-participant 3v3 and up-to-four
FFA preserve the intended format distinction. Offline network isolation and macOS-first scope remain.
This authorizes the mode deliverables, not an MMO, public accounts/ranking platform, service purchase or
claim that multiplayer is implemented. It does not make the fifteen story operations cooperative.

### Verification and remaining work

The integrated document checker, registry checker and whitespace check pass. The checker was challenged
with in-memory duplicate, missing-index, missing-parent, card-collision, family-count, truncated-body
and control-character mutations; each is rejected. Sixty-one corrupted LaTeX tab escapes in the active
master were repaired without altering numerical thresholds; historical receipts were preserved.
The pre-existing state history and the concurrent world-production entry remain intact. Source validation
passed M01 narrative63/63, compiled-map13/13 and overlay1/1. Glass Scar initially failed1/10; the runtime
owner repaired the stale source matcher and traced spawn expectations, and the focused recheck passes10/10.
Both results remain in the same retained evidence directory:
`BuildArtifacts/Evidence/doc-reconciliation-20260904T201127Z/`. It includes source identities, full logs,
restored-body provenance and the ID crosswalk. No Unreal/package/physical/human result is implied.

`TBR-DOC-001` is resolved for the obsolete map-reference conflicts; `DOC-SYNC-002` records completed
identity/index repair and its semantic/evidence limits. The owner accepted the worker recommendation: `TBR-DOC-004` is resolved as a design decision, and
canonical extraction, routing, depletion and reservation bodies are aligned. Implementation and balance
qualification remain open. `TBR-ECO-001` separately tracks inherited work-rate/cargo inconsistency;
`TBR-DOC-003` dialogue-mix alternatives remain pending. `TBR-NET-001` records exact network/service design decisions before
release implementation/qualification. The complete requirement-to-test/evidence matrix and inherited
numeric tuning still require semantic review at each affected implementation boundary. M01 remains at the
planning/source-check stage; full source binding, narrative delivery, real gameplay art/audio, packaged
journeys, human review and owner acceptance remain. No commit, push or release is claimed.

## Original corpus disposition

Each original source file was read by the coordinator or a bounded read-only reviewer; changes were
integrated by the coordinator or the worker assigned exclusively to the skill directory.

| Original file | Disposition |
|---|---|
| [.opencode/skills/echoes-accessibility/SKILL.md](../.opencode/skills/echoes-accessibility/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-ambience-world-soundscape/SKILL.md](../.opencode/skills/echoes-ambience-world-soundscape/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-animation-motion-vfx/SKILL.md](../.opencode/skills/echoes-animation-motion-vfx/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-animation-systems/SKILL.md](../.opencode/skills/echoes-animation-systems/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-asset-provenance-rights/SKILL.md](../.opencode/skills/echoes-asset-provenance-rights/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-audio-listening-review/SKILL.md](../.opencode/skills/echoes-audio-listening-review/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-audio-score-ambience/SKILL.md](../.opencode/skills/echoes-audio-score-ambience/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-balance-analysis/SKILL.md](../.opencode/skills/echoes-balance-analysis/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-build-automation/SKILL.md](../.opencode/skills/echoes-build-automation/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-camera-navigation/SKILL.md](../.opencode/skills/echoes-camera-navigation/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-campaign-human-playthrough/SKILL.md](../.opencode/skills/echoes-campaign-human-playthrough/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-campaign-missions/SKILL.md](../.opencode/skills/echoes-campaign-missions/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-canon-game-design/SKILL.md](../.opencode/skills/echoes-canon-game-design/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-cinematics-sequencer/SKILL.md](../.opencode/skills/echoes-cinematics-sequencer/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-clean-machine-install-qualification/SKILL.md](../.opencode/skills/echoes-clean-machine-install-qualification/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-combat-targeting-damage/SKILL.md](../.opencode/skills/echoes-combat-targeting-damage/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-construction-production/SKILL.md](../.opencode/skills/echoes-construction-production/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-content-data-pipeline/SKILL.md](../.opencode/skills/echoes-content-data-pipeline/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-cross-device-interaction-check/SKILL.md](../.opencode/skills/echoes-cross-device-interaction-check/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-debug-visualization-guardrails/SKILL.md](../.opencode/skills/echoes-debug-visualization-guardrails/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-determinism-audit/SKILL.md](../.opencode/skills/echoes-determinism-audit/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-developer-id-notarization-installer/SKILL.md](../.opencode/skills/echoes-developer-id-notarization-installer/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-economy-logistics/SKILL.md](../.opencode/skills/echoes-economy-logistics/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-evidence-gate-review/SKILL.md](../.opencode/skills/echoes-evidence-gate-review/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-faction-roster-design/SKILL.md](../.opencode/skills/echoes-faction-roster-design/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-factions-rosters-balance/SKILL.md](../.opencode/skills/echoes-factions-rosters-balance/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-fog-shroud-readability/SKILL.md](../.opencode/skills/echoes-fog-shroud-readability/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-formations-unit-cohesion/SKILL.md](../.opencode/skills/echoes-formations-unit-cohesion/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-future-wells-gameplay/SKILL.md](../.opencode/skills/echoes-future-wells-gameplay/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-game-feel-feedback/SKILL.md](../.opencode/skills/echoes-game-feel-feedback/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-gameplay-interface-audio/SKILL.md](../.opencode/skills/echoes-gameplay-interface-audio/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-gameplay-mechanics/SKILL.md](../.opencode/skills/echoes-gameplay-mechanics/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-graphics-scalability/SKILL.md](../.opencode/skills/echoes-graphics-scalability/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-gui-control-readiness/SKILL.md](../.opencode/skills/echoes-gui-control-readiness/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-heavy-run-coordination/SKILL.md](../.opencode/skills/echoes-heavy-run-coordination/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-human-acceptance-session/SKILL.md](../.opencode/skills/echoes-human-acceptance-session/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-input-controls/SKILL.md](../.opencode/skills/echoes-input-controls/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-localization-readiness/SKILL.md](../.opencode/skills/echoes-localization-readiness/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-material-texture-assets/SKILL.md](../.opencode/skills/echoes-material-texture-assets/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-mouse-keyboard-playtest/SKILL.md](../.opencode/skills/echoes-mouse-keyboard-playtest/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-narrative-character-writing/SKILL.md](../.opencode/skills/echoes-narrative-character-writing/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-open-weights-tts-selection/SKILL.md](../.opencode/skills/echoes-open-weights-tts-selection/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-opponent-ai/SKILL.md](../.opencode/skills/echoes-opponent-ai/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-package-provenance/SKILL.md](../.opencode/skills/echoes-package-provenance/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-package-sign-notarize/SKILL.md](../.opencode/skills/echoes-package-sign-notarize/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-packaged-gui-smoke/SKILL.md](../.opencode/skills/echoes-packaged-gui-smoke/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-performance-profiling/SKILL.md](../.opencode/skills/echoes-performance-profiling/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-platform-portability/SKILL.md](../.opencode/skills/echoes-platform-portability/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-player-manual-known-limitations/SKILL.md](../.opencode/skills/echoes-player-manual-known-limitations/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-portability-security/SKILL.md](../.opencode/skills/echoes-portability-security/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-production-orchestration/SKILL.md](../.opencode/skills/echoes-production-orchestration/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-public-front-door/SKILL.md](../.opencode/skills/echoes-public-front-door/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-qa-defect-triage/SKILL.md](../.opencode/skills/echoes-qa-defect-triage/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-realtime-visual-review/SKILL.md](../.opencode/skills/echoes-realtime-visual-review/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-regression-release-blockers/SKILL.md](../.opencode/skills/echoes-regression-release-blockers/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-replay-qol/SKILL.md](../.opencode/skills/echoes-replay-qol/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-requirements-authoring/SKILL.md](../.opencode/skills/echoes-requirements-authoring/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-requirements-traceability/SKILL.md](../.opencode/skills/echoes-requirements-traceability/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-research-technology/SKILL.md](../.opencode/skills/echoes-research-technology/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-save-progression-recovery/SKILL.md](../.opencode/skills/echoes-save-progression-recovery/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-save-replay-recovery/SKILL.md](../.opencode/skills/echoes-save-replay-recovery/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-score-music-state/SKILL.md](../.opencode/skills/echoes-score-music-state/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-security-privacy/SKILL.md](../.opencode/skills/echoes-security-privacy/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-selection-movement-pathing/SKILL.md](../.opencode/skills/echoes-selection-movement-pathing/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-session-control/SKILL.md](../.opencode/skills/echoes-session-control/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-simulation-core/SKILL.md](../.opencode/skills/echoes-simulation-core/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-skirmish-full-match-playtest/SKILL.md](../.opencode/skills/echoes-skirmish-full-match-playtest/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-stability-soak-crash/SKILL.md](../.opencode/skills/echoes-stability-soak-crash/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-story-character-experience-review/SKILL.md](../.opencode/skills/echoes-story-character-experience-review/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-subtitle-caption-runtime/SKILL.md](../.opencode/skills/echoes-subtitle-caption-runtime/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-subtitles-localization/SKILL.md](../.opencode/skills/echoes-subtitles-localization/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-support-readiness/SKILL.md](../.opencode/skills/echoes-support-readiness/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-third-party-agent-skill-review/SKILL.md](../.opencode/skills/echoes-third-party-agent-skill-review/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-tutorial-onboarding/SKILL.md](../.opencode/skills/echoes-tutorial-onboarding/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-tutorial-usability-playtest/SKILL.md](../.opencode/skills/echoes-tutorial-usability-playtest/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-ui-accessibility-playtest/SKILL.md](../.opencode/skills/echoes-ui-accessibility-playtest/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-ui-hud-menu-design/SKILL.md](../.opencode/skills/echoes-ui-hud-menu-design/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-unreal-mcp-editor-inspection/SKILL.md](../.opencode/skills/echoes-unreal-mcp-editor-inspection/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-unreal-mcp-editor-review/SKILL.md](../.opencode/skills/echoes-unreal-mcp-editor-review/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-unreal-runtime-integration/SKILL.md](../.opencode/skills/echoes-unreal-runtime-integration/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-vfx-effects/SKILL.md](../.opencode/skills/echoes-vfx-effects/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-visual-direction-lighting/SKILL.md](../.opencode/skills/echoes-visual-direction-lighting/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-voice-production/SKILL.md](../.opencode/skills/echoes-voice-production/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-workstation-toolchain-readiness/SKILL.md](../.opencode/skills/echoes-workstation-toolchain-readiness/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-workstream-integration/SKILL.md](../.opencode/skills/echoes-workstream-integration/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [.opencode/skills/echoes-world-level-design/SKILL.md](../.opencode/skills/echoes-world-level-design/SKILL.md) | Shared rules, domain routing, metadata and link review; updated in place. |
| [AGENTS.md](../AGENTS.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [CLAUDE.md](../CLAUDE.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/AgentSkillRouting.md](AgentSkillRouting.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Archive/AssetRegister.md](Archive/AssetRegister.md) | Authority/header clarified; production rows left to the active writer. |
| [Docs/Archive/DevelopmentBible.md](Archive/DevelopmentBible.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Archive/ProjectLedger.md](Archive/ProjectLedger.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Archive/SetupAndBuild.md](Archive/SetupAndBuild.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Archive/Superseded/DemoReadinessRequirements.md](Archive/Superseded/DemoReadinessRequirements.md) | Historical body retained; retirement boundary and navigation corrected. |
| [Docs/Archive/Superseded/DemoReleaseDirective.md](Archive/Superseded/DemoReleaseDirective.md) | Historical body retained; retirement boundary and navigation corrected. |
| [Docs/Archive/Superseded/InitialReleaseRequirements.md](Archive/Superseded/InitialReleaseRequirements.md) | Historical body retained; retirement boundary and navigation corrected. |
| [Docs/Archive/TechnicalArchitecture.md](Archive/TechnicalArchitecture.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/ArtDirection.md](ArtDirection.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/AudioDirection.md](AudioDirection.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/CharacterVoiceIdentityBible.md](CharacterVoiceIdentityBible.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/DeliveryPlan.md](DeliveryPlan.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/DemoRecoveryDirective.md](DemoRecoveryDirective.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/GameCompletionDirective.md](GameCompletionDirective.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/MapConcepts.md](MapConcepts.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/MapTechnicalBlueprint.md](MapTechnicalBlueprint.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/MovementAndBalanceRequirements.md](MovementAndBalanceRequirements.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/NarrativeCoherenceReview.md](NarrativeCoherenceReview.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/OpeningAndTutorialScript.md](OpeningAndTutorialScript.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Prompts/AgentProjectBrief.md](Prompts/AgentProjectBrief.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Prompts/ArtDirectorSessionPrompt.md](Prompts/ArtDirectorSessionPrompt.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Prompts/AudioDirectorSessionPrompt.md](Prompts/AudioDirectorSessionPrompt.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Prompts/AudioVisualDirectorMasterPrompt.md](Prompts/AudioVisualDirectorMasterPrompt.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/README.md](README.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/Requirements.md](Requirements.md) | Governance references synchronized; five owner-directed records added; old semantic conflicts retained explicitly. |
| [Docs/RequirementsState.md](RequirementsState.md) | Stale summaries corrected; original history preserved; new OPEN records and audit findings appended. |
| [Docs/SpecGapReport.md](SpecGapReport.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [Docs/WorldMapWorkLog.md](WorldMapWorkLog.md) | Read; retained under active production ownership. Concurrent updates preserved. |
| [README.md](../README.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |
| [SECURITY.md](../SECURITY.md) | Authority, current/history boundary, applicable requirements and stale guidance reviewed; updated in place. |

## Retained generated Markdown evidence

- `BuildArtifacts/Acceptance/FreshJourney-20260831T012838Z/acceptance_manifest.md` — retained unchanged; no new acceptance inferred.
- `BuildArtifacts/Acceptance/SaveIsolation-20260831T020102Z/acceptance_manifest.md` — retained unchanged; no new acceptance inferred.
- `BuildArtifacts/Acceptance/SaveIsolation-20260831T020826Z/acceptance_manifest.md` — retained unchanged; no new acceptance inferred.
- `BuildArtifacts/Acceptance/WorldSurface-20260831T022650Z/acceptance_manifest.md` — retained unchanged; no new acceptance inferred.
- `BuildArtifacts/AiBalance/balance_report.md` — retained unchanged; no new acceptance inferred.
