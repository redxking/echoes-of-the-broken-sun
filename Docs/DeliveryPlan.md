# Delivery Plan — sequential, one lane, owner-visible

**Author and owner:** Angelis Pseftis
**Adopted:** 2026-09-02, replacing parallel multi-lane operation.
**Standing:** this file governs sequencing. `Docs/Requirements.md` is the sole normative requirements
authority and `Docs/RequirementsState.md` is the sole lifecycle and evidence-state authority; only Angelis
assigns HUMAN ACCEPTED.

## Why the owner's five phases are adopted in DISCIPLINE but reordered in SEQUENCE

The owner proposed: sim core → pathfinding → state machines → economy → UI last. **The discipline is
right and is adopted: one thing at a time, proven green before the next begins.** The ORDER is
wrong for this codebase, for a checked reason rather than a preference.

Verified 2026-09-02, not recalled: `Source/EchoesSimCore` is ~8,900 lines and already contains all
five layers — `enum class Terrain` and tile grid (192 refs), entity/unit types (70 refs), pathing
(21 refs), 20+ state enums covering orders/commands/production/research/placement, and Matter/Dawn
economy. Native sim tests pass. 15 campaign missions exist. A packaged build exists.

**Building bottom-up again would rebuild what is already green and show the owner nothing for
weeks.** Every failure found on 2026-09-02 was ABOVE the simulation: a self-contradictory test file,
a schema rejecting all 15 missions, a suite reporting green because a library was absent, a
test-count message wrong for three revisions. The foundation held; the plumbing above it was never
checked end to end.

**Therefore the sequence is ordered by the owner's own rejection list, so each phase ends in
something they can sit down and try.** The rejection is entirely about the top layer, which is
exactly where the defects are.

## Operating rules

1. **ONE lane active at a time**, plus the coordinator. Every other lane stays held.
2. **A phase ends when the owner has PLAYED it**, not when a gate passes. Agent verification is a
   precondition for showing it, never a substitute for showing it.
3. **No phase begins until the previous one is accepted.** No parallel starts, no "while we wait".
4. **RED before NEW.** A failing test or a red suite is repaired before any feature work.
5. Evidence discipline unchanged: derivations published with command and path-sorted order and
   re-run to confirm; every assertion in a test evaluated, not identity-checked; claim boundaries
   stated.

## Phase 0 — CLEAR THE RED (in flight, finishes under the fleet hold)

Nothing new starts while anything is red.
- `EchoesTutorialCurriculumTest.cpp` — self-contradictory assertions; repair frozen, awaiting a
  verdict that RUNS the test, then a fresh full-suite run for 70/70.
- `mission_contract.schema.json` — rejects ALL 15 mission contracts (462 errors), and the Narrative
  suite is RED at 61/62 while ACTIVE_LANES recorded it green. Includes making a missing `jsonschema`
  FAIL rather than silently skip.
**Exit:** both suites green, verified by execution.

## Phase 1 — THE MOUSE WORKS

Owner's first rejection item: *"Mouse doesn't reliably select"* and *"too keyboard-dependent"*.
This is a RELIABILITY defect in existing code, not missing code — selection, drag threshold, cursor
and command submission all exist in `EchoesPlayerController.cpp`, alongside four `[NO_WORLD_HIT]`
failure paths where a pointer trace finds no world. That is the prime suspect: pointer orders trace
against Unreal collision, so anything with collision left on silently intercepts ground orders.
**Scope:** diagnose why traces miss, repair, and prove reliability by driving the real game with a
real mouse — select single, select multiple, drag-select, move, attack-move, patrol, build placement.
**Exit:** the owner plays a Glass Scar skirmish using the mouse and does not fight the controls.

## Phase 2 — THE SCREEN READS

Owner: *"menu and HUD unfinished"*, *"units/buildings/terrain too visually similar"*, *"graphics
lack detail, hierarchy, animation, atmosphere"*.
The UVScale repair is proven (units and buildings now carry visible surface texture, +50% to +113%
local detail, confirmed by rendered A/B with a working negative control) but its final value is NOT
adopted — `1.0` is measurably under-scaled for large structures. Terrain sameness has a separate
cause: all three maps draw from one Glass Scar mesh family.
**Scope:** adopt a UVScale value judged on a composed frame containing both a small unit and a large
building; per-site landmark vocabulary via the accepted dressing contract; HUD legible enough to
read state at a glance; the palette and terrain-matte corrections already diagnosed.
**Exit:** the owner looks at a frame and can tell units, buildings and terrain apart without effort.

## Phase 3 — THE GAME TEACHES

Owner: *"no progressive tutorial"*. The curriculum model is receipted and correct, and is wired to
NOTHING — the contract is `authored_unbound` with `runtime_consumed=false`. Venue is a dedicated
Glass Scar-derived scenario (ruling #30).

**SCOPE CORRECTION (verified by the coordinator 2026-09-02, reported by the Narrative lane): this is
a SMALLER job than "authored but unwired" implies.** The content and the pipeline are already done —
`Content/Narrative/Generated/EchoesNarrativePack.json` carries a top-level `demo` block containing
`system_voice` and `tutorial`, with `demo_line_count` **55**, each entry already holding
`content_id`, `surface`, `scope`, `opens_after_signal` and `lines`. The subtitle lane that would
render it exists and works (`EchoesHUD.cpp:4166`).

What is missing is ONE ADDRESSING SEAM in ONE RUNTIME FILE: `EchoesNarrativeSubsystem.cpp` contains
**ZERO** references to the demo block (verified by count) and addresses everything through
`OperationPackKey(EEchoesOperationMode)` — 14 references — while the demo contracts are deliberately
NOT operation-scoped, per the additive-namespace ruling. So the gap is an addressing model, not
content and not pipeline. Narrative-side files required: none. The work is a runtime-integration
slice, and it is the same seam Phase 4's narrative delivery needs, so one piece of work gates both.

**Exit:** a new player is taught to play by playing, and reaches a first win unaided.

## Phase 4 — THE GAME SPEAKS

Owner: *"no story or context"*, *"no opening cinematic"*, and the wholesale audio rejection —
*"they sound like 1980s games... Modern games use voices"*, with synthetic voice explicitly allowed.
Character identities and approved script text already exist; the game currently has NO VOICE AT ALL.
**Exit:** the opening plays, characters speak, and the owner cares who they are.

## Phase 5 — THE JOURNEY COMPLETES

Owner: *"the complete journey is never demonstrated"*, plus hardware adaptation.
**Exit:** an uncoached player installs, plays start to finish, and finishes.

## What this plan refuses to do

- Rebuild a working, deterministic, tested simulation core to satisfy a build order written for a
  greenfield project.
- Run twelve lanes so that findings arrive faster than one person can act on them.
- Report a phase complete on agent evidence alone.
