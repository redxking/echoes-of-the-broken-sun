---
title: Echoes of the Broken Sun — Opening & Tutorial Script (M2 line-authoring)
author: Angelis Pseftis
status: Draft for owner review — transcribes to the m01 narrative pack under a content lease after acceptance
created: 2026-09-02
updated: 2026-09-02
---

# Opening & Tutorial Script

**Scope**: the M2 critical-path line-authoring against the accepted Character & Voice Identity
Bible (DEMO-NAR-010) and the ruled framings (tutorial inside the prologue fiction as a
readiness check, ruling #8; demo match as a real engagement, ruling #13; Annunciator voices
alerts, ruling #10; Dawn as consumed possibility, ruling #6). Verification gates reference the
lesson-to-gate map (campaign handoff, 2026-09-02T05:40Z). Line ids and signals are proposed in
narrative-pack shape (`nar_m01_line_tut_*`, `nar_m01_evt_tut_*`) for direct transcription.

**What already exists and is NOT re-authored**: the opening cinematic's 4 shots and the
mission's 28 authored lines (`nar_m01_cin_opening`; Talar's ledger discrepancy, Mara's
"Carrier first. Well second.", Oruun's seven accounts) — the opening script is DONE at line
level; its open work is timing/VO, not words. This document adds what is absent: the tutorial
curriculum, the Annunciator, and the engagement frame.

**Voice discipline** (binding, from the accepted bible): Mara — short declaratives, orders as
shared checklists, Compact vocabulary, care expressed as logistics, acknowledgment = precisely
naming what the player secured, dry humor at systems never people. Annunciator — reports state
only; no "you", no reassurance, no opinion. Writing rules apply: immediate needs, incomplete
knowledge, no lore dumps.

---

## Part A — Tutorial: "The Readiness Check"

Fiction: the evacuation deploys within the hour. Mara walks her command through the readiness
check the Compact runs before any operation — "We check the route before we need it." Each
lesson ends only when the game state proves the action (DEMO-TUT-015); Mara's acknowledgment
names the thing secured; her why-line prices it in operational terms.

### Lesson 1 — Survey (camera; TUT-003)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_survey_01` — "Before anything moves, we see the ground. Sweep the basin." |
| highlight | Mara | `tut_survey_02` — "Edge of the world moves the view. The wheel brings it closer. [recenter key] brings you home to the Anchor." |
| act | — | *player pans, zooms both bounds, recenters* |
| verify | gate | camera pose delta + zoom bounds crossed + recenter within epsilon (Player-lane reads) |
| acknowledge | Mara | `tut_survey_03` — "Good. The basin's on your board — scar, crossings, and our corridor." |
| why | Mara | `tut_survey_04` — "A commander who can't look can't warn anyone. Seeing first is the cheapest thing we do." |

### Lesson 2 — Roster (selection; TUT-004)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_roster_01` — "Now the roster. Every asset answers when named. Left-click the Surveyor." |
| highlight | — | *Surveyor highlighted* |
| verify | gate | selection set == {staged Surveyor}; then cleared on terrain click |
| acknowledge | Mara | `tut_roster_02` — "That ring means it's listening. Click open ground — released. Selection is a channel, not a leash." |
| why | Mara | `tut_roster_03` — "You'll command forty things at once out there. Knowing exactly who's listening is the difference between an order and an accident." |

### Lesson 3 — Section muster (drag-select; TUT-005)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_muster_01` — "One at a time is for repairs. Drag a box around the whole section." |
| verify | gate | selection ⊇ staged units, provenance = drag |
| acknowledge | Mara | `tut_muster_02` — "Whole section, one motion. That's how the Compact moves anything heavier than a wrench." |
| why | Mara | `tut_muster_03` — "Duty windows are short. Muster fast, and the window is yours instead of the enemy's." |

### Lesson 4 — The route check (move, context orders, stop; TUT-007)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_route_01` — "Route check. Right-click sends them where you point. The order reads the target — ground means go, a shard seam means gather, trouble means fight." |
| act/verify | gate | Move order issued + arrival (order cleared at destination); Stop clears an active order |
| acknowledge | Mara | `tut_route_02` — "Held exactly where you stopped them. An order you can end is the only kind worth giving." |
| why | Mara | `tut_route_03` — "Routes are promises. The check is how we keep them." |

### Lesson 5 — The reserve (gather/deliver; TUT-008; Dawn framing per ruling #6)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_reserve_01` — "The city's reserve is thin. Put the Surveyor on the Matter seam — it cuts, carries, and books the load at the Anchor." |
| verify | gate | Gather->Deliver cycle observed; resource delta >= threshold |
| acknowledge | Mara | `tut_reserve_02` — "First load booked. Matter is honest work — the strata always has more." |
| why (Dawn) | Mara | `tut_reserve_03` — "Dawn is different. Every shard we spend is a future somebody doesn't get. We spend it — but we log WHY. Remember that when the cost feels light." |

### Lesson 6 — Link restoration (build/placement; TUT-009)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_link_01` — "Severed link on the corridor. Put a Power Link at the marked footprint — the ghost shows green where the ground will take it." |
| invalid-feedback | Mara | `tut_link_02` — *(on rejected placement)* "Red means the ground disagrees. Find footing the network can hold." |
| verify | gate | one rejected placement observed + completed Power Link within radius (relay-discovery pattern) |
| acknowledge | Mara | `tut_link_03` — "Link's live. The corridor has power and eyes again." |
| why | Mara | `tut_link_04` — "The Compact is the network. Every structure you place is a promise the next crew can stand on." |

### Lesson 7 — The foundry (production/rally; TUT-010)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_foundry_01` — "Spin up the Foundry. Queue a Lancer — the cost comes off the ledger the moment you commit." |
| verify | gate | new Lancer entity + resource decrease + arrival at rally point |
| acknowledge | Mara | `tut_foundry_02` — "Fresh Lancer, on station at your rally. The queue keeps building while you fight — set it and trust it." |
| why | Mara | `tut_foundry_03` — "Battles are won by whoever still has a next unit. The Foundry is your next unit." |

### Lesson 8 — Perimeter probe (combat, guard; TUT-011; Guard's fiction is protection)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_probe_01` — "Contact on the perimeter — small, and ours to handle. Lancers forward. The Bulwark holds the line; [guard key] puts it on the Surveyor — its whole job becomes that one life." |
| verify | gate | staged hostiles destroyed, player retains >= K units, Guard order held on the worker |
| acknowledge | Mara | `tut_probe_02` — "Probe broken. Nobody we're responsible for got touched. That's the whole report." |
| why | Mara | `tut_probe_03` — "Out there it won't be a probe. Composition, cover, and a guarded worker — that's how a fight becomes arithmetic instead of grief." |

### Lesson 9 — The board (objectives/minimap/alerts; TUT-012)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_board_01` — "Top of your deck: the duty ledger — what's open, what's held, what's closed. Corner: the basin entire. When something happens off-screen, the deck flags it; [alert key] takes you straight there." |
| verify | gate | objective row transitioned + camera moved to alert site |
| acknowledge | Mara | `tut_board_02` — "You found it without hunting. The deck watches so you can think." |
| why | Mara | `tut_board_03` — "A commander's real enemy is the thing happening where she isn't looking." |

### Lesson 10 — The Well (Future Well choice; TUT-013)

| Beat | Speaker | Line |
|---|---|---|
| explain | Mara | `tut_well_01` — "Last item, and it isn't drill. That's a Future Well. Inside are futures that never happened — held, coherent, and ours to decide about. Walk the Surveyor up and read the panel. Harvest ends it for power now. Preserve holds it and pays slowly. Reshape spends it to change the ground itself, for a while." |
| verify | gate | wellChoice committed on the well entity; immediate effect observed in sim state |
| acknowledge | Mara | `tut_well_02` — "Committed and logged. Whatever you chose — that's now the only version of this that ever happens." |
| why | Mara | `tut_well_03` — "There's no clean answer at a Well. There's only the answer you can stand behind in the ledger. You just made your first one. The next one won't be practice." |

### Mastery gate close (JRN-003 transition; TUT-020)

| Beat | Speaker | Line |
|---|---|---|
| close | Mara | `tut_close_01` — "Readiness check complete — survey, roster, routes, reserve, links, foundry, perimeter, board, and one Well you'll be thinking about tonight. The window opens now. This one is real." |

### Hint escalation lines (TUT-017 — one per lesson, second-stage; fired on ticks-without-fact-change, never completing the action)

`tut_hint_survey` — "The view answers the screen edge and the wheel. Try the corner of the world."
`tut_hint_roster` — "One click, on the Surveyor itself. It's waiting."
`tut_hint_muster` — "Press, drag a corner across the section, release."
`tut_hint_route` — "Right-click the marked ground. They know the way from there."
`tut_hint_reserve` — "Right-click the bright seam. The Surveyor handles the rest."
`tut_hint_link` — "The footprint marker, on open ground near the corridor. Green ghost, then commit."
`tut_hint_foundry` — "Select the Foundry, then its Lancer entry. The ledger covers it."
`tut_hint_probe` — "Lancers on the contact; the guard order on the Surveyor. In that order is fine."
`tut_hint_board` — "The flash in the corner is real. [alert key] jumps to it."
`tut_hint_well` — "The Surveyor, to the Well. The panel does not commit until you do."

---

## Part B — Annunciator copy (ruling #4/#10; hard personality boundary: state only, no "you", no reassurance)

**Revision 2026-09-02 — constraint fold.** Revised against Audio's binding
`AnnunciatorAlertCopyConstraints.md` (≤0.8 s spoken, 2–4 words, ONE breath group with no comma
clause; one stable never-reused line per class; opening word distinct across classes for
pre-attentive recognition; copy must read correctly as verbatim HUD text; no duplication of what
the tonal cue already carries). Every departure from the ruling-#19 approved text is itemized in
the campaign handoff (2026-09-02T13:05Z) — nothing was changed silently. Lines marked
**FLAGGED** were resolved by owner ruling #20; see §B.2.

### B.1 — Class lines

| Class | Line | Status |
|---|---|---|
| UnderAttack | "Contact." | REVISED — dropped the trailing sector clause (second breath group); the minimap pulse and HUD alert text carry location per the layering constraint |
| StructureLost | "Structure lost." | OWNER RULED #20 (Option A) — fixed class line; the structure name rides the HUD alert text |
| ConstructionComplete | "Build complete." | OWNER RULED #20 (Option A) — the three approved variants collapse to one stable class line |
| ProductionComplete | "Unit fielded." | OWNER RULED #20 (Option A) — fixed class line |
| ResearchComplete | "Adaptation ready." | OWNER RULED #20 (Option A) — fixed class line; adopts the ruling-#11 shard-adaptation framing |
| CapacityLow | "Logistics limit." | NEW — class present in Audio's alert surface, absent from the approved script; authored to constraint (canon term: Logistics is the capacity resource) |
| MatterLow | "Matter reserve low." | UNCHANGED — compliant |
| ResourcesInsufficient | "Cost exceeds reserve." | UNCHANGED — compliant |
| DawnSpent (ruling #6 weight) | "Dawn committed." | REVISED — dropped the balance readout (second breath group); the ruled weight rides on "committed" (irreversibility) plus the tonal cue, and the balance belongs to the HUD, not the voice |
| WellTelegraph | "Telegraph running." | REVISED — was a two-clause sentence, far over budget; "telegraph" is the canon term for the public commitment window (§Future Wells) |
| HostileCoreDown | "Hostile core down." | UNCHANGED — compliant |
| AnchorCritical | "Anchor critical." | UNCHANGED — compliant. Owner note (#20): "Anchor"/"Adaptation" are first-phoneme neighbours and both are now owner-approved words — Audio resolves the separation with pace offset or timbre, NOT by changing the copy |

### B.2 — RESOLVED by owner ruling #20 (2026-09-02)

The four placeholder-opening lines could not be recorded as stable pinned takes (a first word
that varies at runtime defeats both the take pool and pre-attentive recognition). The owner
ruled Option A: fixed class lines, with the specifics carried by the HUD alert text and the
minimap pulse. The approved replacements are in §B.1 above — "Structure lost." /
"Build complete." / "Unit fielded." / "Adaptation ready." — and ConstructionComplete's three
variants collapse to its single class line. Every Annunciator line is now constraint-compliant:
2-4 words, one breath group, one stable line per class, fixed opening word.

---

## Part C — The engagement frame (ruling #13: the demo match is a real engagement)

**Briefing paragraph** (skirmish deployment, demo path, Meridian):
"Glass Scar, second duty window. The readiness check bought us the corridor; it did not buy us
the basin. A Kharuun assembly is moving on the Well with a claim as good as ours — their
caverns fail without it, our reserve fails without it. Nobody is wrong. Somebody is leaving.
Hold the network, watch the crossings, and decide what the Well becomes."

**Victory result copy**:
"The basin holds. The Well feeds the reserve — Lume Reach keeps its lights, and the ledger
records what that cost. The Kharuun withdrew in order, carrying their claim with them. This
is not finished; it is paid for."

**Defeat result copy**:
"The basin is theirs. The cavern lives on what the reserve needed — the ledger records who
paid, and that we knew why. Withdrawal was clean. The Compact rebuilds from what survives;
that is what the Compact is for."

**Replay offer line** (per review 6.4, light touch):
"The ledger holds. Run the account again."

---

## Open items with this draft

1. Owner review of all Part A/B/C lines (DEMO-TUT-022 pacing/clarity bar; DEMO-NAR-009 for the
   frame copy).
2. Bracketed key references (`[recenter key]`, `[guard key]`, `[alert key]`) resolve to the
   player's current bindings at render time per TUT-014 — the pack transcription carries them
   as binding tokens, not literals.
3. Transcription to `m01_what_the_ledger_keeps.json` (speakers exist; new line/trigger records
   per the id scheme above) under a content lease after acceptance.
4. Annunciator VO synthesis needs its casting decision (new sixth voice, no pin yet — flagged
   in NAR-010).
