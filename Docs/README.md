# Project document authority and reading map

**Author and owner:** Angelis Pseftis
**Created:** 2026-09-03
**Maintained:** 2026-09-05

Start with [AGENTS.md](../AGENTS.md) for shared agent rules. This index routes work to the controlling
sources; it creates no game requirement and accepts no result. Current owner instructions and the host's
instruction hierarchy take precedence. A newer timestamp alone does not promote a document's authority.

## Which source controls which decision

| Source | Authority and use |
|---|---|
| [Requirements.md](Requirements.md) | Sole normative game requirements master: behavior, IDs, thresholds, decomposition, and acceptance criteria. Read the exact affected records before implementation. |
| [RequirementsState.md](RequirementsState.md) | Sole per-requirement lifecycle, evidence-state, owner-acceptance, and decision record. Read dated entries and their scope; summary rows are not new evidence. |
| [DevelopmentBible.md](Archive/DevelopmentBible.md) | Creative canon: world, factions, characters, and narrative intent. Its legacy `Archive/` location does not retire this role. Behavioral numbers and implementation snapshots defer to the two master files. |
| [AGENTS.md](../AGENTS.md) | Shared operating rules, ownership, evidence, authorship, communication, and completion discipline for every agent. |
| [AgentSkillRouting.md](AgentSkillRouting.md) | Selects the canonical task-specific procedures under `.opencode/skills/`. Client bridges contain no independent policies. |

The requirements master and creative canon govern different decisions. A contradiction between them is an
explicit owner decision, not permission to silently rewrite either. Record affected IDs and options in
`RequirementsState.md`; continue unaffected authorized work.

## Read by work area

| Work | Read after the shared contract and relevant requirements/state |
|---|---|
| Simulation, movement, combat, economy, AI, save/replay | [TechnicalArchitecture.md](Archive/TechnicalArchitecture.md), applicable source contracts and domain skills. |
| Campaign, story, character, tutorial | [DevelopmentBible.md](Archive/DevelopmentBible.md), [OpeningAndTutorialScript.md](OpeningAndTutorialScript.md), [CharacterVoiceIdentityBible.md](CharacterVoiceIdentityBible.md), and affected narrative source contracts. [NarrativeCoherenceReview.md](NarrativeCoherenceReview.md) is a dated assessment; verify adoption of its recommendations. |
| World, map, terrain, visual composition | [ArtDirection.md](ArtDirection.md), [MapConcepts.md](MapConcepts.md), [MapTechnicalBlueprint.md](MapTechnicalBlueprint.md), and affected world source. Map concepts/blueprints are subordinate design references, not a second requirements master. [WorldMapWorkLog.md](WorldMapWorkLog.md) records the current author's bounded work and owner direction; it cannot independently accept requirements. |
| Voice, music, ambience, mix | [AudioDirection.md](AudioDirection.md), character/script references above, [AssetRegister.md](Archive/AssetRegister.md), and source/generator contracts. Numeric gates come from the master. |
| Build, package, performance, runtime inspection | [SetupAndBuild.md](Archive/SetupAndBuild.md), live scripts/configuration, and specific build/verification skills. Recheck mounts, tool versions, processes, and storage before execution. |
| AI integration with Unreal Editor | [UnrealEditorIntegrationResearch.md](UnrealEditorIntegrationResearch.md): dated local connection audit, primary-source research, and a proposed efficiency pilot. Recommendations are not implemented integration or measured productivity results. |
| Asset generation, import, rights | [AssetRegister.md](Archive/AssetRegister.md), applicable direction and provenance skills. This register owns provenance, not human acceptance. |
| Release sequencing and demo recovery | [GameCompletionDirective.md](GameCompletionDirective.md), [DeliveryPlan.md](DeliveryPlan.md), and [DemoRecoveryDirective.md](DemoRecoveryDirective.md). These organize work; the master owns criteria and the state record owns acceptance. |
| Historical evidence lookup | [ProjectLedger.md](Archive/ProjectLedger.md) and exact retained artifacts. Dated runs remain historical until their applicability is checked. |
| Agent handoff or specialist prompt | [AgentProjectBrief.md](Prompts/AgentProjectBrief.md), [ArtDirectorSessionPrompt.md](Prompts/ArtDirectorSessionPrompt.md), [AudioDirectorSessionPrompt.md](Prompts/AudioDirectorSessionPrompt.md), [AudioVisualDirectorMasterPrompt.md](Prompts/AudioVisualDirectorMasterPrompt.md). These select work; they do not grant ownership or override shared rules. |
| Gaming prompts, implementation, debugging, presentation iteration | [GameDevelopmentWorkflow.md](Prompts/GameDevelopmentWorkflow.md): task brief, verification loop, review priorities, and dated official-source mapping. Subordinate procedure; it changes no game requirement or acceptance gate. |

Paths shown as code inside skills are relative to the checkout unless explicitly absolute. Markdown links
resolve relative to their containing file. Historical external evidence paths must be located and verified
before use. Current coordination and evidence-location rules live in `AGENTS.md`; the old missing
`WorkstreamControl` directory is not an operational prerequisite.

## Campaign design and remaining decisions

The owner-authorized reconciliation retains fifteen distinct M01–M15 campaign maps and the separate
three-map offline skirmish baseline in the master. Angelis separately approved the 25-sector
Conquest/roguelite and team/FFA multiplayer on 2026-09-04; their sector and map-format contracts are
additional release obligations, while MMO/shared persistent-world scope remains excluded. MapConcepts now supplies one current story-to-place brief;
MapTechnicalBlueprint maps all fifteen missions to their controlling contracts and defines M01's
representative qualification. Obsolete conflicting mission studies, unsupported geographic events,
coordinate lists and the retired six-map proposal’s unsupported map/format claims have been removed
from those active references.

Use the detailed `SPEC-MSN-*` contracts for command roles, objective sites, branch consequences and
prohibited claims. M09 is Mara's district allocation; M10–M12 are Oruun-led contact/public-evidence/readback
operations. M06 and M08 follow Talar's Meridian command. Presentation cannot replace these with a different
battle or a causal story that the contract does not establish.

Identifier migration and any unresolved numerical/scope decisions are recorded in RequirementsState.md.
Use titled successor bindings for retired ambiguous IDs; old evidence does not transfer automatically.
Read exact current decisions before implementing a disputed rule. A source-map or editor pass cannot
substitute for M01's packaged journey, integrated visual/audio review or human acceptance.

## History and superseded material

| Retained file | Treatment |
|---|---|
| [MovementAndBalanceRequirements.md](MovementAndBalanceRequirements.md) | Historical owner input merged into the master. Follow current `SPEC-MOV-*`, `SPEC-CTL-*`, and `SPEC-BAL-*` bodies. |
| [SpecGapReport.md](SpecGapReport.md) | Dated 2026-09-02 assessment of an earlier baseline, not a current defect list or acceptance record. |
| [DemoReleaseDirective.md](Archive/Superseded/DemoReleaseDirective.md) | Superseded release scope; original evidence and decisions retained as history. |
| [DemoReadinessRequirements.md](Archive/Superseded/DemoReadinessRequirements.md) | Retired `DEMO-*` snapshot. Bodies and lifecycle moved to the master pair. |
| [InitialReleaseRequirements.md](Archive/Superseded/InitialReleaseRequirements.md) | Retired `REL-*` snapshot. Bodies and lifecycle moved to the master pair. |
| [Original requirements DOCX](Archive/Superseded/EchoesOfTheBrokenSun_CompleteGameRequirements.docx) | Origin record; superseded by `Requirements.md`. Do not resume drafting or accepting requirements in it. |

Keep historical owner decisions, IDs, negative evidence, and license/provenance records. Correct active
pointers and remove stale instructions in place. Do not rewrite past results as current successes. Inspect
Git state live rather than copying counts, branch names, or tracking assertions into standing guidance.


## Maintenance verification

Run `python3 Scripts/check_agent_docs.py` from the checkout after guidance edits. It checks local links,
client/skill pointers, metadata, requirement identity/index coverage and family-navigation counts.
`python3 Scripts/check_requirement_registry.py --write-index` rebuilds the master navigation in place
from its definitions. These checks do not certify gameplay or resolve semantic requirement conflicts. [DocumentationAudit.md](DocumentationAudit.md) records this synchronization's
coverage, changes, preserved history, and unresolved issues.
