# Session context packet

**Author and owner:** Angelis Pseftis
**Generated:** 2026-09-11 15:43 UTC by `Scripts/build_context.py`. Do not edit; regenerate.

Read this, then fetch only the requirement records you touch:
`python3 Scripts/req.py both <ID> [<ID>...]` · `req.py family <FAM>` · `req.py search "<text>"` · `req.py section "<heading>"`. Do not open Requirements.md or RequirementsState.md whole. Record outcomes with `python3 Scripts/record_state.py --id <ID> --state <STATE> --class <CLASS> --evidence <path> --note "<what>"` and rerun this script before handing off. Rules: [AGENTS.md](../AGENTS.md); authority map: [Docs/README.md](README.md); skills: [AgentSkillRouting.md](AgentSkillRouting.md).

## Checkout

- Branch `main` at `8ca5633` (upstream `27aaf68`); 417 dirty paths, of which non-asset: `AGENTS.md`, `Docs/Archive/ProjectLedger.md`, `Docs/Prompts/GeminiContinuationHandoff.md -> Docs/Archive/Superseded/GeminiContinuationHandoff.md`, `Docs/MovementAndBalanceRequirements.md -> Docs/Archive/Superseded/MovementAndBalanceRequirements.md`, `Docs/SeeLoopWorkflow.md -> Docs/Archive/Superseded/SeeLoopWorkflow.md`, `Docs/SpecGapReport.md -> Docs/Archive/Superseded/SpecGapReport.md`, `Docs/DeliveryPlan.md`, `Docs/DocumentationAudit.md`.
- Another lane may own the dirty paths. Preserve them; stage only your own files by path.

## Active execution state (from DeliveryPlan.md)

- **First unfinished package.** D2 — Establish integrated foundation. In progress; not exited.
- **Completed indexing.** D0 preservation (`f8bfd47`), D1 binding (`23e7230`), retained main work (`bdd2d8a`), first D2 code (`120b60c`: 30-cap count, 4×4 Foundry, resource-depletion and unit-abilities salvage integrated). `b0108c1` ("D3") is a deck-card change that was reverted in this continuation; D3 has not started.
- **Saved baseline.** `main` = `origin/main` after the schema 31 movement fix (code, tests and records `a8607f8`: masked-ground escape in the route field and the move-order gate, replay schema 31, native corridor tests, D2 review driver hardening, record correction; this baseline row follows it). … (full text: DeliveryPlan.md, Active execution state)
- **Ownership.** Owner granted checkout ownership. On 2026-09-11 the owner ordered the push: main was fast-forwarded to `68653ab` and pushed to `origin/main` (862d7b2..68653ab); the checkout now works on `main`, single writer. `integration/d0-reconciliation` is folded in and retained only as a label.
- **Preservation boundary.** Main dirty files in `BuildArtifacts/Evidence/resume-preservation-20260910`; Antigravity's scratch scripts, `tests.log` and walkthrough in `BuildArtifacts/Evidence/d2-foundation-20260911T0050Z/antigravity-scratch/`. Salvage worktrees untouched.
- **Last retained evidence.** Field console rebuilt as a non-scrolling instrument (2026-09-11, RequirementsState "Field console rebuilt as a non-scrolling instrument"): `BuildArtifacts/Evidence/hud-console-20260911T135529Z` — `build-09.log` Result Succeeded on the combined tree, `automation-04/index.json` 138/139 (the one failure is the concurrent play-test-findings lane's new `Presentation.FieldHudAuthority`), rendered D2 exit review `review-128 … (full text: DeliveryPlan.md, Active execution state)
- **Next exact action after resume.** The owner plays with the rebuilt console (no scrolling in any field panel, console-edge camera pan, DPI-scaled on large surfaces) and rules on SPEC-UI-007 / REL-UI-025. Then: Owner play test of the D2 chain on 2026-09-11: FAILED (owner's ruling; four findings in RequirementsState "D2 owner play test — FAILED"): production must require network power (owner ruling to author and implement), the command deck must show unit names, costs, layout hotkeys and disabled reasons (no "Semicolon"/"Apostrophe" drawn over tiles), a selected deposit must show remaining Matter and a legible exhausted state, and production refusals must name the missing resource and its source. Repair these, prove them through native and Unreal suites and a rendered check, then return the chain to the owner. Carry the open decisions: SPEC-MOV-003 route-field body blindness (the review-run builder stalls were NOT this: they were a frozen worker in the half-tile gap between the Core and a supply node, a SPEC-MOV-006/008 route-mask defect, fixed at replay schema 31 and tested; see RequirementsState), REL-AI-024, REL-AI-031; TBR-SCP-012 is decided (option B, D7).
- **Next dependency.** D2 exit requires the affected integration suite green and a player able to gather, build, train, move, fight, repair and recover under 30 on an ordinary map; D3 (Meridian slice, REL-UI-025 3×3 deck, practice lessons) waits on it.
- **Open evidence.** Owner acceptance of D2 is not given: the 2026-09-11 owner play test FAILED on four player-facing points (see RequirementsState). Agent-driven rendered review and automation are green but are not human evidence. Current game reproduction and packaged verification remain open. Retained replays containing a Foundry predate the 4×4 correction and no longer reproduce (recorded in RequirementsState).

## Newest RequirementsState entries

- D2 integrated foundation — continuation record, 2026-09-11
- D2 continuation, second slice — the seven remaining failures, 2026-09-11
- D2 exit check — rendered agent-driven review of the player chain, 2026-09-11
- D2 owner play test — FAILED, 2026-09-11

## Before you stop

- `record_state.py` for every ID whose evidence state you changed; only Angelis sets acceptance.
- Update the DeliveryPlan active-state table if the next action changed, then rerun `build_context.py`.
- New evidence goes under `BuildArtifacts/Evidence/<gate>-<UTC>/`; cite its absolute path.
