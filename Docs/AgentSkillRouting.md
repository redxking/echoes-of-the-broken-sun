# Agent skill routing

**Author and owner:** Angelis Pseftis

Follow [AGENTS.md](../AGENTS.md) first. The canonical skill bodies live under `.opencode/skills/`.
`.agents/skills/` and `.claude/skills/` are discovery symlinks to that same library. Keep one body per skill;
client names do not change authority, permission, evidence, or ownership.

## Selection

Inspect names and frontmatter descriptions, select the smallest set that covers the requested outcome,
and read each selected body completely before applying it. Use the specific implementation/review skill;
a router only selects skills and does not replace their checks. Re-select if scope materially changes.

Every skill inherits the shared contract and [document authority map](README.md). Read affected master
requirements and state entries, then task-relevant design and source contracts. The completion directive
provides sequencing; historical ledgers provide dated evidence. Neither defines a new requirement.

## Required combinations

| Trigger | Procedure |
|---|---|
| Mutation or consequential verification | `echoes-session-control`: identity, dirty work, scope, ownership, and verification boundary. |
| Gaming prompts, implementation, debugging, or presentation iteration | Apply [GameDevelopmentWorkflow.md](Prompts/GameDevelopmentWorkflow.md) for the task brief and verification loop, then select the affected domain skills. |
| Work across domains | `echoes-production-orchestration` for sequencing; `echoes-workstream-integration` when integrating or sharing paths. Read-only parallel review remains permitted under the shared contract. |
| Unreal builds/automation, packaging, asset generation, GPU review, profiling, soak, or game launch | `echoes-heavy-run-coordination` plus the appropriate domain skill. Establish exclusive resource use through live coordination, not missing historical lock files. |
| Live UI or input evidence | `echoes-gui-control-readiness`, then the narrowest smoke, mouse/keyboard, skirmish, campaign, tutorial, accessibility, visual, audio, or human-review skill. Require only capabilities needed for the actual check. |
| Normative requirement changes or decomposition | `echoes-requirements-authoring`: edit the master in place, preserve IDs and owner decisions. |
| Requirement-to-evidence mapping | `echoes-requirements-traceability` plus the affected domain skill. |
| Readiness or release-gate review | `echoes-evidence-gate-review`; add `echoes-regression-release-blockers` for defects or release closure. |
| Third-party skills, plugins, models, or packages | `echoes-third-party-agent-skill-review` before installation/execution within authorized scope. |
| Mac distribution | Separate package provenance, signing/notarization, clean-machine installation, performance, stability, and human-acceptance checks. No gate substitutes for another. |

For documentation-only maintenance, session control and the applicable writing/review procedure are
sufficient unless the change alters normative requirements or makes a game readiness claim. Verify links,
authority, metadata, preserved history, and relevant source facts; do not require runtime gates just to
correct an instruction or index.

## Domain routing

- Gameplay: simulation core, movement/pathing, formations, combat, economy, construction, research,
  faction roster, Future Wells, opponent AI, balance, save/progression, replay.
- Player experience: input, camera, HUD/menu, fog readability, tutorial, accessibility, game feel.
- Content: world/level, material/texture, lighting, animation, VFX, narrative, cinematics, voice, score,
  ambience, gameplay/interface audio, subtitles, localization, asset provenance.
- Verification and delivery: build/runtime, determinism, content pipeline, GUI/play/listening review,
  performance, stability, evidence, package, security/privacy, portability, public documentation/support.

Older combined skill names are compatibility routers to the relevant specific skills. Maintain operational
checks in those leaf skills; keep the routers short so their policies cannot diverge.

## State and limitations

Use [RequirementsState.md](RequirementsState.md#state-vocabulary) for lifecycle labels and
[Requirements.md](Requirements.md#verification-classes) for the evidence required by each record. A passing
test may be reported as a passing test; it is not owner acceptance or requirement `COMPLETE`.

A missing tool blocks only work requiring that capability. Locate the canonical skill or an equivalent
method with the same evidence strength before stopping. A skill cannot require renewed approval for work
already authorized, grant publication/signing authority, or impose a retired model-specific role. Resolve
conflicts through the instruction hierarchy and the controlling source, with explicit owner decisions for
material requirement/canon changes. Report the exact unresolved boundary and continue independent work.
