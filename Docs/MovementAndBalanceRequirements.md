# Movement, Control, and Balance-Validation Requirements

**Author and owner:** Angelis Pseftis

Follow [Requirements.md](Requirements.md) and [RequirementsState.md](RequirementsState.md) for current
requirements and outcomes. The owner wording and original numbering below are retained as historical input.

**Status: SUPERSEDED — MERGED INTO `Requirements.md` ON 2026-09-03.**
This file is retained as a historical source only. Its requirements were incorporated into the sole normative
authority, `Docs/Requirements.md`; it creates no current requirement and does not override that master.

Two additions are captured here, both from owner direction on 2026-09-03:

1. **Unit movement and control must reach the StarCraft II standard.** Owner: *"we want this to feel
   like they are playing StarCraft 2 with how units are moved and controlled. that is the gold
   standard for RTS Engines."*
2. **Balance must be validated by mass AI-versus-AI play**, at the scale of a thousand matches,
   measuring maps and factions, and confirming that **a player's strategy is what wins**.

Every requirement below states a measurable acceptance condition. A requirement whose acceptance
condition cannot be measured is not a requirement; it is an aspiration, and is marked as such.

---

## Part A — Movement and control (MOV-006 .. MOV-013, CTL-001 .. CTL-004)

These extend, and do not replace, MOV-001..MOV-005 in the master document. MOV-003 (soft separation
and yield rules) and MOV-005 (formations) are currently **specified but not implemented**; several
requirements below are the measurable form of those two.

### MOV-006 — Any-angle movement
Units move at arbitrary angles and take the most direct route their footing allows. Where an
unobstructed straight line to the destination exists, the unit walks it.

**Acceptance:** on open ground, a unit ordered along an exact 45-degree line deviates from that line
by no more than 0.25 tile at every tick of the journey.
**Status: IMPLEMENTED.** `Simulation::FindSmoothedWaypoint` string-pulls over the grid distance
field; `Simulation::HasLineOfSight` is an exact integer supercover tile walk.
**Test:** native `any-angle movement takes straight lines`. Verified to fail if string pulling is
reverted.

### MOV-007 — Direction-independent speed
A unit covers the same ground per tick regardless of heading. Movement rate is a property of the
unit, never of the direction it happens to face.

**Acceptance:** distance travelled over a fixed tick count on a diagonal is within **2%** of the
distance travelled on an axis, for the same unit and duration.
**Status: IMPLEMENTED.** Step distribution normalises by Euclidean distance (`IntegerSqrt64`) rather
than the former Manhattan `|dx|+|dy|`, which made diagonal travel roughly 29% slower.
**Test:** native `any-angle movement takes straight lines`, speed-ratio assertion. Verified to fail
if the Manhattan metric is restored.

### MOV-008 — Soft separation between allied units
The measurable form of MOV-003. Allied mobile units maintain spacing, push past one another rather
than colliding rigidly, and **may never permanently imprison one another**.

**Acceptance, all three required:**
- No two allied mobile units remain overlapped beyond their combined clearance for more than 20 ticks
  (one second).
- Given any two units ordered to swap positions on open ground, both reach their destinations.
- In a 200-tick soak with 40 units ordered to a single point, **zero** units are permanently blocked;
  every unit either arrives or is within its arrival radius.

**Status: NOT IMPLEMENTED.** No separation, cohesion, alignment, avoidance, or unit-versus-unit
collision exists anywhere in `EchoesSimCore`. Movement validates terrain passability only; footprint
is checked at spawn but not during travel.

### MOV-009 — Choke behaviour without deadlock
Units squeeze through narrow terrain rather than jamming.

**Acceptance:** 12 units ordered through a one-tile-wide gap all pass within a bounded tick budget
derived from unit speed and count, with no unit stationary for more than 40 consecutive ticks while
its path remains valid. Deadlock is a release blocker, not a tuning issue.
**Status: NOT IMPLEMENTED.**

### MOV-010 — Travel facing
Units face the direction they are travelling and turn at a bounded rate rather than snapping.

**Acceptance:** a unit's rendered facing is within a stated tolerance of its travel direction while
moving; facing changes at no more than the declared turn rate; **reduced-motion keeps the facing and
removes only the sweep.**
**Status: IMPLEMENTED (presentation only).** `AEchoesEntityView::UpdateTravelFacing`. Deliberately
presentational: the simulation carries no travel facing, so nothing about targeting, damage, or
pathing depends on where a body is pointed. **If facing is ever made to affect gameplay it must move
into the deterministic core and this requirement must be rewritten.**

### MOV-011 — Group movement
A selected group ordered to one point moves as a group and arrives as a group.

**Acceptance:**
- The group's units spread across an arrival area sized to the group; they do not stack on one point
  or fight over a single destination tile.
- Group cohesion is preserved in transit to the tolerance stated by the chosen formation (MOV-005).
- No unit in a group outruns the group so far that it engages alone.
**Status: NOT IMPLEMENTED.** Multi-unit orders currently send every unit to the identical destination.

### MOV-012 — Clean arrival
Units stop when they arrive and stay stopped.

**Acceptance:** after arrival, a unit's position changes by no more than one movement-tick's worth for
20 consecutive ticks. No oscillation between waypoints, no drift, no jitter against neighbours.
**Status: PARTIAL.** Single-unit arrival lands exactly on target. Group and crowded arrival are
untested because MOV-008 and MOV-011 do not exist.

### MOV-013 — Movement remains deterministic
Every behaviour in Part A is computed in the deterministic core under the existing rules: fixed point
Q22.10, fixed 20 Hz tick, no floating point, no wall clock, deterministic iteration order.

**Acceptance:** identical inputs produce byte-identical state checksums across optimized, debug, and
address+undefined sanitizer builds, and across machines. Any per-unit steering state added is
serialized into the snapshot and hashed into the state checksum, and `kSnapshotVersion` is incremented
in the same change.
**Status: ENFORCED.** Native suite runs in all three configurations on every gate.

### CTL-001 — Command responsiveness
A player's command produces visible acknowledgement immediately and takes effect within the
lockstep input delay.

**Acceptance:** acknowledgement is presented in the frame the click is received; the authoritative
order takes effect within `minimumInputDelayTicks` (currently 3 ticks, 150 ms) in single player, and
within the negotiated delay in network play. The player is never left unsure whether a click
registered.

### CTL-002 — Interruptibility
Any in-flight order can be replaced at any tick, with no stall and no replan penalty visible to the
player.

**Acceptance:** issuing a new move to a unit already moving changes its course on the next simulation
tick; measured tick cost of re-pathing does not exceed the per-tick budget in CTL-004.

### CTL-003 — Micro-management is honoured
The control scheme must not punish the player for doing what an RTS player does: selecting subsets,
retargeting mid-fight, pulling damaged units, issuing per-unit orders in rapid succession.

**Acceptance:** stated as a **usability requirement measured with humans**, not an automated one.
Validated under VAL-001's comprehension protocol with players who play RTS games. *This is deliberately
not machine-measurable; do not claim it from automated evidence.*

### CTL-004 — Per-tick cost ceiling
Movement, steering, and pathing together must fit the frame budget at full unit count on the
macOS Apple-Silicon baseline.

**Acceptance:** a stated maximum simulation-tick cost at the supported maximum unit count, measured
and recorded, with headroom stated. Neighbour queries must not be O(n^2) at that count.

---

## Part B — Balance validation by mass AI play (BAL-001 .. BAL-008)

Owner direction: once the game is playable, build AI models to play against each other across
roughly a thousand matches, measuring maps and factions, to confirm the game is fair and that
**a player's strategy is what wins**.

This part is **NOT IMPLEMENTED**. It is recorded now so it is designed for rather than retrofitted.

### BAL-001 — Headless batch harness
A harness that runs simulation-only matches with no rendering, no editor, and no human input, at a
rate that makes a thousand matches practical.

**Acceptance:** N matches run to a terminal outcome from a seed list, emitting one machine-readable
record per match: seed, map, factions, spawn positions, AI configurations, outcome, duration in ticks,
and final state checksum. No match may end in an unresolved state; a match that neither resolves nor
concedes is a defect, reported and not discarded.

### BAL-002 — Statistical reporting, with uncertainty
Results are reported as rates **with sample size and confidence intervals**, never as bare
percentages.

**Acceptance:** every reported rate carries n and an interval. A difference is only called a
difference when the intervals separate. This requirement exists because a 55% win rate over 40
matches and over 4,000 matches are different claims, and the master document's VAL-002 already warns
against treating one simulation result as the whole story.

### BAL-003 — Faction balance
Realises VAL-002. No non-mirror Standard matchup sits outside **40–60%** without an accepted design
reason recorded by the owner.

**Acceptance:** the full matchup matrix is covered, each cell with sufficient n for its interval to
be narrower than the 40–60 band. Mirror matchups are reported as a control and must sit near 50%; a
mirror that does not is evidence of a **map or spawn** problem, not a faction one.

### BAL-004 — Map and spawn fairness
Realises the spawn clause of VAL-002. No spawn position changes the win rate by more than **five
percentage points**.

**Acceptance:** every map is played from every spawn arrangement, with factions swapped so spawn
effects separate from faction effects. Reported per map. A map that fails is a map defect and is
fixed or withdrawn, not averaged away.

### BAL-005 — Strategy must be what wins
The central requirement of this part, and the owner's stated purpose. The game must reward the
player's decisions rather than the draw.

**Acceptance, all three required:**
- **Mirror control:** two identical AI configurations on a mirror matchup and mirrored spawns win
  within the interval of 50%. A deviation here means something other than strategy is deciding
  outcomes, and invalidates every other balance number until explained.
- **Strategy separation:** a deliberately stronger strategy beats a deliberately weaker one at a rate
  whose interval sits clearly above 50%. If a known-better strategy does not reliably win, the game
  is not rewarding play.
- **No dominant line:** no single strategy beats every other strategy in the tested set above a
  stated ceiling. A strategy that always wins is the absence of strategy.

### BAL-006 — The harness is deterministic and replayable
A result that cannot be reproduced cannot be investigated.

**Acceptance:** identical seed and configuration reproduce an identical match, verified by final state
checksum. Any match can be replayed for inspection. Batch results are reproducible from the seed list
alone.

### BAL-007 — Balance is re-validated when the rules change
Balance evidence expires when the simulation changes underneath it.

**Acceptance:** any change to simulation rules, unit statistics, movement, or economy invalidates the
current balance run and requires a fresh one before balance claims are repeated. The run records the
commit it was produced from.

### BAL-008 — Instrument validity: the AI must be good enough to measure with
**The most important caveat in this part.** Balance measured with weak AI measures the AI's weakness,
not the game's balance. An AI that walks its army into defences and never retreats will report a map
or faction as broken when the real finding is that the AI cannot play.

**Acceptance, before any balance number is quoted as evidence:**
- The AI demonstrably performs the fundamentals the measurement depends on: it retreats damaged units,
  focuses fire, reacts to losing units, does not path its army into an obviously defended position,
  and uses the map.
- AI competence is stated alongside every balance result, so a reader knows what the instrument was.
- Where a result is sensitive to AI competence, that sensitivity is reported rather than hidden.

**This is not hypothetical.** On 2026-09-03 the `CompleteSkirmish` fixture reported a strike force
wiped out with the enemy Command Core untouched (`survivors=0 enemyCoreHp=1300`) after a movement
change. That was initially read as a movement regression. It was not: the fixture issues one scripted
order and then runs 3,010 ticks with no micro-management. It measured an unmicro-managed AI, not the
movement, and not anything a human player would experience. Owner correction, verbatim: *"humans wont
be using scripts to do that.. they will be moving their units and micro controlling."*

The lesson is now a requirement: **state what the instrument was before quoting what it measured.**

---

## Traceability summary

| ID | Requirement | Status | Evidence |
|---|---|---|---|
| MOV-006 | Any-angle movement | Implemented | native `any-angle movement takes straight lines` |
| MOV-007 | Direction-independent speed | Implemented | same test, speed-ratio assertion |
| MOV-008 | Soft separation, no imprisonment | **Not implemented** | — |
| MOV-009 | Choke behaviour, no deadlock | **Not implemented** | — |
| MOV-010 | Travel facing | Implemented (presentation) | `UpdateTravelFacing` |
| MOV-011 | Group movement | **Not implemented** | — |
| MOV-012 | Clean arrival | Partial | single-unit only |
| MOV-013 | Movement determinism | Enforced | native suite, 3 sanitizer configs |
| CTL-001 | Command responsiveness | Partial | needs measurement |
| CTL-002 | Interruptibility | Needs measurement | — |
| CTL-003 | Micro-management honoured | **Human-measured only** | VAL-001 protocol |
| CTL-004 | Per-tick cost ceiling | Needs measurement | — |
| BAL-001..008 | Mass AI balance validation | **Not implemented** | — |

**Open owner decisions**
1. Movement corrections MOV-006 and MOV-007 change match outcomes, because every mission and AI
   behaviour was tuned against the previous slower, staircased movement. Re-tuning is required and is
   a design decision. This is the same class as the parked economy correction, where the corrected
   gather rate breaks M10 and the reverted rate breaks M04.
2. The supported maximum unit count, needed to set the CTL-004 ceiling.
3. The strategy set to be used as the measuring instrument for BAL-005.
