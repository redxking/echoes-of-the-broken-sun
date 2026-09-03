# Docs — what binds, what records, what is history

**Author and owner:** Angelis Pseftis
**Purpose:** one entry point for this folder, so no session has to guess which document wins.
**Created:** 2026-09-03.

This file is a map. It creates no requirement and accepts nothing. Every authority statement below is
quoted from the document it describes; where two documents disagree, the conflict is named here rather
than resolved.

Read in tier order. A lower tier never overrides a higher one.

---

## Tier 1 — What the game must be (binding)

| Document | Standing |
|---|---|
| [`Requirements.md`](Requirements.md) | **The sole normative requirements authority.** Defines requirement bodies, decomposition, acceptance criteria, and exact crosswalks in the preserved `SPEC-*`, `DEMO-*`, and `REL-*` namespaces. Holds no lifecycle or evidence state. |
| [`RequirementsState.md`](RequirementsState.md) | **The sole lifecycle and evidence-state authority.** Records per-requirement engineering state, evidence locations, owner acceptance, decision history, and reopen conditions. It restates no requirement body. |
| [`MovementAndBalanceRequirements.md`](MovementAndBalanceRequirements.md) | **Historical source — superseded and merged 2026-09-03.** Its content was incorporated into `Requirements.md`; it creates no current requirement and does not override the master. |
| [`Archive/DevelopmentBible.md`](Archive/DevelopmentBible.md) | **Sole creative authority** for world, factions, characters, and narrative intent. `Requirements.md` defines behaviour; the Bible defines intent. |

## Tier 2 — What has actually been proven

| Document | Standing |
|---|---|
| [`Archive/ProjectLedger.md`](Archive/ProjectLedger.md) | **Evidence register** — what has actually been run and proven, and what is still open. A requirement is not satisfied because code exists; it is satisfied when this file records the run. |

## Tier 3 — How the work is sequenced

| Document | Standing |
|---|---|
| [`DemoRecoveryDirective.md`](DemoRecoveryDirective.md) | Owner directive of 2026-09-02 that created the demo ledger and carries the current verdict: REJECTED — NOT DEMO-READY. |
| [`GameCompletionDirective.md`](GameCompletionDirective.md) | Professional-release backlog and gate matrix. Written 2026-09-01, before the binding `.docx`. Scope and gates, not requirements. |
| [`DeliveryPlan.md`](DeliveryPlan.md) | Sequencing, adopted 2026-09-02: one lane at a time, each phase ending in something the owner can sit down and try. |
| [`../../WorkstreamControl/ACTIVE_LANES.md`](../../WorkstreamControl/ACTIVE_LANES.md) | Who may touch which paths right now. Take a lease before editing a shared file. |
| [`AgentSkillRouting.md`](AgentSkillRouting.md) | Mandatory skill-selection contract. Read before analysis, editing, testing, building, GUI work, or any completion claim. |

## Tier 4 — Operational references (subordinate to Tiers 1–3)

| Document | Covers |
|---|---|
| [`ArtDirection.md`](ArtDirection.md) | The page every visual pass is checked against. |
| [`AudioDirection.md`](AudioDirection.md) | Cue, route, and mix decisions. Explicitly claims nothing is accepted; acceptance lives in the ledger. |
| [`CharacterVoiceIdentityBible.md`](CharacterVoiceIdentityBible.md) | DEMO-NAR-010. Owner accepted 2026-09-02; casting may proceed against these specs. |
| [`OpeningAndTutorialScript.md`](OpeningAndTutorialScript.md) | Approved production text (owner rulings #19, #20). |
| [`NarrativeCoherenceReview.md`](NarrativeCoherenceReview.md) | DEMO-NAR-011 consolidation; four items await owner adoption. |
| [`Archive/TechnicalArchitecture.md`](Archive/TechnicalArchitecture.md) | Simulation, adapter, AI, net, save, replay structure. |
| [`Archive/AssetRegister.md`](Archive/AssetRegister.md) | Every asset family, its generator, and its provenance. Register before use. |
| [`Archive/SetupAndBuild.md`](Archive/SetupAndBuild.md) | Build, test, and package procedure. |
| [`Prompts/`](Prompts) | Art and audio director session prompts. |

## Tier 5 — Assessments and history (never a requirement source)

| Document | What it is |
|---|---|
| [`SpecGapReport.md`](SpecGapReport.md) | Read-only assessment of 2026-09-02 measuring the build against the binding `.docx`. It reports gaps; it does not create or waive requirements. |
| [`Archive/Superseded/DemoReleaseDirective.md`](Archive/Superseded/DemoReleaseDirective.md) | Superseded 2026-09-01 by `GameCompletionDirective.md`. Retained as record only. |
| [`Archive/Superseded/EchoesOfTheBrokenSun_CompleteGameRequirements.docx`](Archive/Superseded/EchoesOfTheBrokenSun_CompleteGameRequirements.docx) | The authored origin of the specification. Migrated into `Requirements.md` on 2026-09-03 with zero content loss (2,256/2,256 text units verified). No longer authoritative. |
| [`Archive/Superseded/DemoReadinessRequirements.md`](Archive/Superseded/DemoReadinessRequirements.md) | The `DEMO-*` ledger as it stood. Migrated 2026-09-03. Defines nothing, accepts nothing. |
| [`Archive/Superseded/InitialReleaseRequirements.md`](Archive/Superseded/InitialReleaseRequirements.md) | The `REL-*` ledger as it stood. Migrated 2026-09-03. Defines nothing, accepts nothing. |

---

## Open conflicts a session should not resolve on its own

1. **None of this is in version control.** `Requirements.md`, `RequirementsState.md`, this map, the retired
   ledgers, the origin `.docx`, `MovementAndBalanceRequirements.md`, `DeliveryPlan.md`, `SpecGapReport.md`,
   and `AudioDirection.md` are all untracked. Every requirement the game has exists on one disk with no
   history and no remote copy. This is the first thing to fix.

2. **`GameCompletionDirective.md` predates the requirements it gates.** Written 2026-09-01, it declares the
   repository the sole authority on how the game works. `Requirements.md` now holds that authority. The
   directive keeps its gate matrix and sequencing; it has not yet been edited to stop restating requirements,
   and until it is, prefer `Requirements.md` wherever the two describe the same behaviour.

3. **Historical release-body provenance.** All 369 previously undeclared release requirement bodies across
   §6–§26 have been fully authored and decomposed into atomic leaves in `Requirements.md` on 2026-09-03,
   calibrated against `GameCompletionDirective.md`, `DevelopmentBible.md`, and `TechnicalArchitecture.md`.
   All are initialized to `OPEN` in `RequirementsState.md` awaiting engineering and verification.
