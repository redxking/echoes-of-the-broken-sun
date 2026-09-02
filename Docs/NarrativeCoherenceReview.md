---
title: Echoes of the Broken Sun — Narrative Coherence Review (DEMO-NAR-011)
author: Angelis Pseftis
status: Owner rulings of 2026-09-02 folded in; awaiting remaining lane inventories for full consolidation
created: 2026-09-02
updated: 2026-09-02
---

# Narrative Coherence Review

**Requirement**: DEMO-NAR-011. **Lead**: Campaign lane; contributions from Visual, Audio,
Player, World, and AI lanes are consolidated here as their handoff inventories land.
**Companion**: the Character & Voice Identity Bible (DEMO-NAR-010); the owner reviews both
together as the design foundation everything traces to.

**Method**: for every element on the demo path, answer three questions — *Why does this exist
in the world of Soryn? How does it tie into the Crownfall storyline and the player's role?
What should that imply for how it looks, acts, sounds, and feels?* Each element is classified:

- **GROUNDED** — canon justification exists; the Development Bible section is cited.
- **UNGROUNDED** — works mechanically; no story reason yet; a justification is proposed or
  the element is flagged for redesign/cut.
- **CONTRADICTS** — violates canon or a governing document; recorded as a defect.

Canon citations reference `Docs/Archive/DevelopmentBible.md` by section name. Nothing here
invents canon: every proposal is marked **[PROPOSAL]** and requires owner acceptance.

---

## 1. World and setting

| Element | Class | Why it exists in Soryn / implication |
|---|---|---|
| Soryn, the Crownfall, Dawnshards | **GROUNDED** (§Soryn) | The broken sun condensed unrealized futures into matter. Everything else in the game is downstream of this one fact. Implication: the sky/fragment field should be present in framing shots and the title treatment — the wound the world orbits. |
| Future Wells as contested places | **GROUNDED** (§Creative direction 1, §Future Wells) | "Contested places, not passive resource piles." Implication: a Well must never read visually or aurally as a pickup; it is a location with presence, telegraphs, and consequence. |
| The Glass Scar basin | **GROUNDED** (§Vertical slice) | A narrow impact basin split by a fractured transit span; the three crossings ARE canon by name — Ash Cut, Buried Causeway, Folded Verge — and the authored terrain code implements exactly those three crossings. Rare case of code and canon already in lockstep. |
| The war itself | **GROUNDED** (§Historical frame) | "A chain of unstable Wells threatens ark-city power reserves and Kharuun birthing caverns. Both sides mobilize under plausible survival claims." Implication: the demo match is an episode of THIS war, not an abstract battle — see item 4.3. |
| Ecology (shivergrass, vaultbacks, pale tides) | **GROUNDED** (§Ecology and architecture) | Probability-leakage organisms as readable systems. Not on the demo path today. Deferred: World lane to state whether any appears in the demo map; if none, record as post-demo. |

## 2. Factions and the player's role

| Element | Class | Why / implication |
|---|---|---|
| Meridian Compact identity | **GROUNDED** (§Meridian Compact, §Cultures and language) | Governance-and-logistics compact; ledger/tolerance vocabulary; "maintaining systems whose original builders are gone." Implication: everything the player touches as the Compact — HUD, voice, buildings — should carry maintained-infrastructure fiction (see 5.1). |
| Kharuun Assemblies identity | **GROUNDED** (§Kharuun Assemblies, §Cultures) | Layered memory custodianship; grown mineral architecture "inhabited and maintained, not wild or primitive." Implication binding on art/audio inventories. |
| Hollow Choir identity | **GROUNDED** (§Hollow Choir) | Beings of maintained possibility; playable after the vertical slice; skirmish faction-cycling to the Choir is sanctioned by §Skirmish and AI. |
| The player IS Mara Vey (demo) | **GROUNDED** (§Prologue, canon-continuity) | The player commands Mara's force in the opening/tutorial. Implication: instruction and feedback speak WITH her voice and register (NAR-010 recommendation), and the command fiction (5.1) must be hers. |
| Why the AI opponent fights | **GROUNDED** (§Historical frame, §Skirmish and AI) | Plausible survival claims + five named AI personalities with doctrinal descriptions. AWAITING AI-LANE INVENTORY: per-personality in-fiction doctrine statements to surface in skirmish setup UI. |

## 3. Demo-path story beats and mission premises

| Element | Class | Why / implication |
|---|---|---|
| Opening: What the Ledger Keeps | **GROUNDED** (§Prologue) | Evacuation, failing reserve, Talar's convoy, Oruun's cavern — the opening establishes identity, situation, stakes, and the Well decision in one scene. The m01 storyboard (`nar_m01_cin_opening`) already scripts it. |
| Tutorial-as-prologue vs dedicated progressive tutorial | **RESOLVED — OWNER RULED 2026-09-02 (#8)** | Ruling: the tutorial lives INSIDE the prologue's fiction, restructured to the directive's lesson cycle — teaching through operational problems with verified mastery. Canon and directive now agree; recorded in the DEMO ledger change log. |
| Tutorial lesson fiction | **GROUNDED BY RULING (#8, 2026-09-02)** | Lessons need in-world reasons. Proposal: each lesson is a pre-evacuation readiness check in Compact duty-window fiction — Mara verifying her force before the operation ("We check the route before we need it"). Camera lesson = survey sweep; selection = roster check; gathering = reserve shortfall response; construction = link restoration; combat = perimeter probe. Grounded in Compact civic ritual (§Cultures). |
| Transition tutorial -> AI match | **GROUNDED BY RULING (#8/#13)** | Proposal: mastery completes the readiness check; the match is the operation the readiness was FOR — continuous fiction, satisfying DEMO-TUT-020/DEMO-NAR-007. |
| The demo AI match fiction | **GROUNDED BY RULING (#13, 2026-09-02): REAL ENGAGEMENT** | A bare "skirmish" has no story frame, and the golden path ENDS here. Proposal: the match is a Glass Scar engagement of the Present War — the player's Compact detachment against a Kharuun assembly contesting the basin's Well, framed by one briefing paragraph and result copy that names the stakes (reserve vs cavern). Alternative (weaker): an operations rehearsal/simulation within Compact training fiction. Recommend the real-engagement frame: the demo should end inside the war the opening started. |
| Victory/defeat/results meaning | **UNGROUNDED -> [PROPOSAL]** | Results today are outcome labels + tick counts. Proposal: result copy states consequence in world terms (what the basin's Well now feeds; per §Campaign outline, "endings report concrete consequences and unresolved costs"), and the replay/restart offer uses possibility language sparingly (see 6.4). |

## 4. Mechanics

| Element | Class | Why / implication |
|---|---|---|
| Economy trio: Matter / Dawn / Logistics | **GROUNDED** (§Economy and territory) | Matter from strata and wreckage; Dawn from shards; Logistics from infrastructure. THE DEEP TIE: Dawn is consumed possibility — §Soryn: "Consuming it also closes the possibility it contains." Spending Dawn is the game's theme in miniature. Implication (currently unexpressed — see ranked list #3): Dawn's UI, sound, and spend feedback should feel weightier and more final than Matter's; it must never read as generic gold. **OWNER RULED (#6, 2026-09-02): Dawn is presented as CONSUMED POSSIBILITY — this implication is now a binding presentation requirement.** |
| Future Well modes (Harvest/Preserve/Reshape) | **GROUNDED** (§Future Wells) | Each mode's mechanics ARE its story (permanent collapse / held possibility / temporary manifestation). The sim implements canon directly — Reshape's temporary terrain opening (`IsReshapedOpen`) is "manifest a map-authored possibility," including the deterministic expiry displacement canon requires. Confirmation-panel requirement (§Future Wells: immediate gain, irreversible change, telegraph, campaign consequence) is a canon UI obligation to carry into DEMO-UI work. |
| Worker gather/deliver routes | **GROUNDED** (§Economy) | "Workers carry limited cargo to a valid drop-off, so routes, harassment exposure, and drop-off placement matter." Tutorial lesson TUT-008 teaches exactly the canonical loop. |
| Command set incl. Guard/Hold | **GROUNDED** (§Combat and controls) | The full command vocabulary is canon, with stable failure reasons required by canon ("Invalid actions return a stable reason") — the tagged-feedback channel implements this. Guard's fiction is load-bearing for the campaign: escorting protected people (Talar's convoy, the witnesses) IS the story; the demo's combat lesson should let Guard mean protecting someone, not just a stat behavior. |
| Fog, vision, vibration intelligence | **GROUNDED** (§Creative direction 3, §Kharuun table, §0.56.0) | Readability doctrine + Kharuun asymmetry: Resonants/Listening Spines sense "movement signatures without full unit identity" — the anonymous-vibration contact system is canon made mechanical, including its non-targetability ("useful movement warning without hostile identity"). Fair fog also enforces the Choir rule "no unrestricted rewinds of hidden information." |
| Research (two-tier per faction) | **GROUNDED** (§Skirmish and AI) | Authored names/costs/interruption semantics are canon text. The no-refund interruption rule carries fiction weight (committed cost — very Soryn) worth one line of in-game framing. **[PROPOSAL]** |
| Formations (Box/Line/Wedge) | **UNGROUNDED -> [PROPOSAL]** | Mechanically accepted (0.54.0) with no fiction. Proposal: Compact drill doctrine framing (formation names in Mara's register); low priority. |
| Determinism / restart / checkpoints | **GROUNDED (mechanically), fiction opportunity** | Transactional saves and deterministic replay are engineering canon (§0.58.0, §0.67.0: records, generations, replay-without-rewriting-history — deeply consonant with ledger fiction). See 6.4 for the player-facing framing proposal. |

## 5. Screen elements and interaction surfaces (Campaign view; Player-lane inventory will deepen)

| Element | Class | Why / implication |
|---|---|---|
| What IS the HUD in-fiction? | **GROUNDED BY RULING (#5, 2026-09-02): MARA'S COMMAND DECK** | The player looks through SOMETHING. Proposal: the demo HUD is Mara's command instrumentation — the Compact operations ledger made visual: status bands (§Ecology: "visual status bands" are literal Compact architecture), duty-window objective entries, reserve-margin resource readouts, annunciator alerts (NAR-010 §6). This single decision gives the DEMO-UI remake its design language and makes every screen element answerable to fiction. |
| Title screen | **UNGROUNDED -> [PROPOSAL]** | Today: text modal over a paused battlefield. Proposal: the title IS the Crownfall — fragment field over Soryn, the basin below — with the menu as command-network entries; "available operation" already uses Compact operations vocabulary (semi-grounded seed). |
| Mission briefing screens | **GROUNDED (partially)** | Operations-brief fiction is native to the Compact register; authored briefing text from the narrative pack already renders. Needs presentation to match the fiction, not new fiction. |
| Objective tracker | **GROUNDED (partially) -> [PROPOSAL]** | Phase objectives from authoritative state = duty-window ledger entries. Proposal: present rows as ledger entries (opened/held/closed) in the HUD fiction above. |
| Minimap ("tactical overview") | **GROUNDED** (§Interface and accessibility) | Canon specifies its information classes and non-color ownership marks (bracketed/faceted/concentric — faction-derived shapes, already canon-differentiated). |
| Selection/order markers, destruction cues | **GROUNDED** (§0.69.0/0.70.0/0.53.0) | Broken-sun ring motif ties feedback to the Crownfall visually; faction-distinct destruction reads. AWAITING VISUAL-LANE INVENTORY for the full element-by-element pass. |

## 6. Sound classes (Campaign view; Audio-lane inventory will deepen)

| Element | Class | Why / implication |
|---|---|---|
| Faction destruction cues | **GROUNDED** (§0.71.0, §Art and audio) | "Meridian's higher engineered collapse and Kharuun's lower ceramic resonance" — material fiction in audio, already canon-differentiated. |
| Command confirmation | **GROUNDED (thin)** | A UI sound today; becomes fully grounded when it belongs to the command-instrumentation fiction (5.1) / annunciator voice family (NAR-010 §6). |
| Music languages | **GROUNDED** (§Art and audio) | Per-faction musical canon exists (measured pulse/prepared piano vs interlocking stone/ceramic vs multi-directional resolution). AWAITING AUDIO-LANE INVENTORY: map each existing cue and required demo state to this canon. |
| Voice classes | **GROUNDED via NAR-010** | Character voices + annunciator boundary defined there; the two documents cross-reference. |
| 6.4 Replay/restart language | **[PROPOSAL]** | Restart re-runs a deterministic scenario — in Soryn terms, revisiting an unrealized branch. One line of result-screen copy ("The ledger holds. Run the account again.") grounds the replay offer without mechanical change. Light touch; never gamify canon metaphysics. |

## 7. Awaiting lane inventories (intake sections)

- **Visual** — per-screen-element and per-asset-family pass against §Art and §Ecology/architecture canon. *(pending handoff)*
- **Audio** — per-cue/per-class pass against §Art and audio. *(pending handoff; rulings #9-#12 already recorded in §8)*
- **Player** — UI/control surface inventory against the ruled command-deck fiction; interaction failure reasons. *(pending handoff)*
- **AI** — per-personality doctrine statements; why each faction fights as it does in-mission. *(pending handoff)*

### 7.1 World inventory — DELIVERED (world-levels handoff, 2026-09-02T11:37:57Z) and classified

World's seven UNGROUNDED items arrived with grounding proposals, written to the ruled framing
(Dawn = consumed possibility; engagements are real). Campaign-lead classification:

| # | Item | Proposal (abridged) | Classification |
|---|---|---|---|
| 1 | Edge corridors | Rim benches survived because "nothing worth condensing reached the rim" — no possibility there for a Well fate to disturb | **ACCEPTED — canon-consistent** (pure §Soryn physics; explains existence, safety, slowness, and branch-invariance; invents no entity or event) |
| 2 | Preserve-branch closures | A held Well's stability envelope re-freezes the marginal crossings; only the engineered Causeway bears traffic | **ACCEPTED — canon-consistent** (extends §Future Wells/Preserve behavior with a physical rationale; center's stability priced at the flanks — thematically exact) |
| 3 | Lume Reach gates + district blocks | Gates named Ration Gate (west), Census Gate (center), Reserve Gate (east); west block = Transit, east block = Archive | **OWNER ADOPTED 2026-09-02, as proposed** — the names are canon; "Census Gate" serves the census-erasure arc. Binding on map labeling, briefing copy, and line-authoring |
| 4 | SevenAccounts-Reshape columns | The manifested widening followed the Causeway's old service verges | **ACCEPTED — canon-consistent** (inside §Reshape's explicit route-opening license) |
| 5 | Crownfall Basin geography | Skip-impact twin ridges; pale-tide gate cuts; shelf walls as collapsed ark-city foundations | **ACCEPTED — canon-consistent** (assembles only existing canon elements; no new names or events; owner sees it here in review) |
| 6 | Soryn Confluence | Grounded as an early coherent Choir incursion site — repeated near-identical geometry with deliberate local contradictions (§Ecology and architecture), grounding §Historical frame's "apparitions" and foreshadowing the Act II contact missions | **OWNER ADOPTED 2026-09-02 (option a: ground + rename)**. Proposed display name, recorded with this adoption: **"The Confluence Ring"** (World's proposal, endorsed — the ring is the incursion's repeated geometry made legible). Map preset display name and skirmish setup copy update under a later Player/World slice |
| 7 | Base placements | Accept as vertical-slice convention | **ACCEPTED — convention** (no story work needed) |

World's four inverse gaps (observation ridge, ecology dressing, subsurface routes, Crownfall
phenomena) are recorded as owner-approved thematic material for M5 map dressing; World specs
anchors when that slice opens.

## 8. Ranked findings — status after the 2026-09-02 owner rulings

Rulings recorded in the DEMO ledger change log (14-item batch). Status of this review's items:

1. **Demo AI match frame** — RULED (#13): a REAL ENGAGEMENT of the Present War. Framing brief + result copy move to line-authoring.
2. **HUD in-fiction identity** — RULED (#5): MARA'S COMMAND DECK. Binding design language for the DEMO-UI remake.
3. **Dawn as consumed possibility** — RULED (#6): binding presentation requirement for UI/audio/VFX treatment.
4. **Tutorial governance contradiction** — RESOLVED (#8): tutorial inside the prologue fiction, lesson-cycle structure.
5. **Tutorial lesson fiction + transition** — grounded by #8/#13; readiness-check beats move to line-authoring.
6. **Title screen fiction** — proposal stands (Crownfall title); not in the ruling batch — remains open for a later packet or design execution under #5's command-deck language.
7. **Results/replay meaning** — partially ruled: ledger-frame results underscore ACCEPTED (#11, Audio batch); consequence-language copy moves to line-authoring; 6.4's replay line remains a light-touch proposal.
8. **Formations / research-interruption framing** — LOW; proposals stand.

Cross-lane items ruled in the same batch and folded here for the record: Reshape-Well overlay CONTRADICTS defect — FIX THE OVERLAY, the Well stays reachable, canon rule stands (#7); per-faction weapon-fire variants with the blind-identification rule standing (#9); alerts voiced by the Annunciator (#10); economy sounds shared — "matter voices itself" (#12); DEMO-INP-010 remapping deferred to release scope with a view+reset controls screen in the demo (#14). Full per-cue and per-element classification lands with the Audio/Visual/Player/World/AI inventories.

The review's central finding stands and is now owner-endorsed by implication: the coherence debt was presentation and framing; the rulings convert every HIGH item into an executable design directive. Soryn's story is already load-bearing in the simulation; the demo's job is to let the player see it.
