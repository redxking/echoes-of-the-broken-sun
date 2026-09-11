---
title: Echoes of the Broken Sun — Cutscene and Video Production List
author: Angelis Pseftis
creator: Angelis Pseftis
created: 2026-09-07
updated: 2026-09-10
status: Production inventory and detailed AI handoff briefs; proposed staging is not owner acceptance
---

# Cutscene and video production list

For descriptions to give another AI, start with the [Cross-media visual consistency guide](#cross-media-visual-consistency-guide), then use the [Detailed AI production handoff](#detailed-ai-production-handoff). Copy the shared direction, applicable cross-media reference cards, and the chosen scene brief. The inventory below is the short index.

Production baseline: **24 presentation items** — one title-screen environment, one world prologue, fifteen mission openings, three act transitions, and four endings. This count assumes the title cinematic and first-run world prologue share one sequence satisfying both requirements; the title-screen environment remains separate. If separate title and prologue edits are desired, the count becomes 25. Additional mission-critical scenes must be scoped against mission triggers; their final number is not established here.

This inventory was checked against the working tree on 2026-09-07 (main, base commit `3c033df`, with existing uncommitted work). It identifies required content and authored source, not a verified count of missing runtime assets. The requirements state records `REL-CIN-*` as OPEN; no sequence is accepted by this document. Only this inventory file is created by this task.

In-game cutscenes must use Unreal Level Sequencer and registered game assets, not pre-rendered video playback (`REL-CIN-001`). Promotional videos are separate captured deliverables. The inventory summarizes required content. The detailed briefs below reproduce existing mission shot descriptions and spoken lines, then add explicitly proposed camera, edit and sound treatments. These proposals do not change canon or establish owner acceptance.

## Front door

| ID | Item | Content and purpose | Timing / basis |
|---|---|---|---|
| FD-01 | Living title-screen environment | Soryn's shattered sun, vitrified landscape, atmospheric lighting, title music and ambience. Establish the world while the menu remains usable. | In-engine environment; no fixed loop length specified. `REL-FTU-002`. |
| FD-02 | World prologue / opening title cinematic | Establish the fractured sun, Crownfall, Dawnshards and the three factions. End with a coherent route into the playable readiness check. | Maximum 90 seconds; voiced, letterboxed and subtitled. `REL-FTU-004`, `REL-CIN-002`, `DEMO-NAR-007`. Shared-sequence packaging is a planning assumption. |

The world prologue is distinct from M01's existing evacuation brief. The character bible rules out a separate narrator for the M01/tutorial path; do not commission a new narrator for those scenes. Resolve the world prologue's voice treatment against that scope before recording.

## Fifteen mission opening cutscenes

Each listed source contains **four authored shots**, line references, visual/audio hooks, and an operation-start trigger. All fifteen currently declare `implementation_status: absent` and `binding_status: authored_unbound`. Those are source declarations, not a live runtime audit; the state record separately describes unfinished cinematic playback. Reuse and finish the authored material rather than commissioning fifteen new scripts.

Durations below are sums of existing editorial shot targets, not recorded playback lengths or final approved timing. Mission names and roles follow `SPEC-MSN-001..015` and the registered narrative sources.

| ID | Mission | Opening focus | Target | Authoritative storyboard |
|---|---|---|---|---|
| M01 | What The Ledger Keeps | Glass Scar evacuation corridor, archive recovery and the first Well decision; establish what Mara must secure. | 32.7 s | [nar_m01_cin_opening](../Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json) |
| M02 | Seven Accounts Of Rain | First-light migration basin, conflicting inherited accounts and the route Oruun must protect. | 31 s | [nar_m02_cine_opening](../Content/Narrative/Source/missions/m02_seven_accounts_of_rain.json) |
| M03 | A City On Reserve | Ark-city districts losing power; establish Mara’s grid-restoration problem. | 31 s | [nar_m03_cine_opening](../Content/Narrative/Source/missions/m03_a_city_on_reserve.json) |
| M04 | The Unburied Road | A missing memory shard and a gap in the Kharuun record; frame the recovery operation. | 32 s | [nar_m04_cine_opening](../Content/Narrative/Source/missions/m04_the_unburied_road.json) |
| M05 | Terms Of Continuance | Treaty terms and an expiring ceasefire window; retain uncertainty about the pressure’s origin. | 32 s | [nar_m05_cine_opening](../Content/Narrative/Source/missions/m05_terms_of_continuance.json) |
| M06 | Names Without Births | Erased Compact census entries; frame Talar’s investigation without assigning hidden authorship. | 31 s | [nar_m06_cine_opening](../Content/Narrative/Source/missions/m06_names_without_births.json) |
| M07 | The Shape Of Silence | Corresponding census and communal-memory absences; show correspondence without claiming causation. | 32 s | [nar_m07_cine_opening](../Content/Narrative/Source/missions/m07_the_shape_of_silence.json) |
| M08 | The Shape Beside Us | Overlapping terrain and reciprocal contact with Neme; Talar commands Meridian proxies. | 32 s | [nar_m08_cine_opening](../Content/Narrative/Source/missions/m08_the_shape_beside_us.json) |
| M09 | Reserve Authority | Three district demands and capacity for two; establish Mara’s allocation responsibility. | 31 s | [nar_m09_cine_opening](../Content/Narrative/Source/missions/m09_reserve_authority.json) |
| M10 | The Choir At Lume Reach | Lume Reach, quarantine boundaries, the deferred district and the Well; Oruun leads the contact operation. | 32 s | [nar_m10_cine_opening](../Content/Narrative/Source/missions/m10_the_choir_at_lume_reach.json) |
| M11 | No Neutral Ledger | The inherited ledger, the selected plan and paired public evidence interfaces. | 32 s | [nar_m11_cine_opening](../Content/Narrative/Source/missions/m11_no_neutral_ledger.json) |
| M12 | The Future That Won | Rhyse’s public demonstrator and Oruun’s independent readback; Rhyse is not a field commander. | 32 s | [nar_m12_cine_opening](../Content/Narrative/Source/missions/m12_the_future_that_won.json) |
| M13 | Assembly Of The Missing | Converging absences, paired records and the Crownfall public index; avoid assigning responsibility. | 32 s | [nar_m13_cine_opening](../Content/Narrative/Source/missions/m13_assembly_of_the_missing.json) |
| M14 | Several Voices One Command | Choir voices held in disagreement, inherited sites and the Crownfall crisis. | 32 s | [nar_m14_cine_opening](../Content/Narrative/Source/missions/m14_several_voices_one_command.json) |
| M15 | The Broken Sun | The fractured sky, three neutral witnesses, public record and approach to the final accord. | 34 s | [nar_m15_cine_opening](../Content/Narrative/Source/missions/m15_the_broken_sun.json) |

## Act transitions

| ID | Cutscene | Placement | Required content / voices | Authority |
|---|---|---|---|---|
| ACT-01 | Necessary Fires | Between M05 and M06 | Lume Reach ruins; reflect evacuation costs. Mara and Talar; Act I musical resolution. | `REL-CIN-003` |
| ACT-02 | The Cost of One Future | Between M10 and M11 | Unburied Road vaults; ancestral memory and escalation toward Crownfall. Oruun. | `REL-CIN-004` |
| ACT-03 | Crownfall | Before M15 | Fractured sky over the Broken Sun and the final conflict. Neme and Cael Rhyse are named in the requirement. | `REL-CIN-005` |

No fixed duration is specified for these transitions. Size them after script and voice timing. Treat ACT-03 and the M15 opening as distinct editorial purposes; coordinate their shared location to avoid repeating the same sky reveal.

**Reconciliation before scene production:** Track D describes act-boundary placement differently from the exact `REL-CIN-003..005` records; this list follows the exact requirement placements. The character bible restricts Rhyse to M12 public apparatus, while `REL-CIN-005` names him before M15. Resolve that scope conflict before recording ACT-03. Its staging must not invent physical Rhyse presence. Likewise, verify the specified Lume Reach ruins and Unburied Road staging against current mission continuity before treating location imagery as proof of unmodeled destruction or travel. These issues do not block the inventory or unaffected scenes.

## Four ending cutscenes

All four are required by `REL-CIN-006`, triggered only by the corresponding successful M15 resolution. Use the recorded eligibility and chosen ending; do not select or change the outcome inside the cinematic. Each needs its own resolution music and in-engine accord-site staging.

| ID | Ending | Required narrative direction |
|---|---|---|
| END-01 | Restoration | Rebuilding the old sun at terrible cost. Show only supported consequences; do not invent restored populations. |
| END-02 | Controlled Stabilization | Possibility constrained into rigid order, with its unresolved costs. |
| END-03 | Extinguishment | The shards die into dark peace; preserve the cost and uncertainty. |
| END-04 | Open Evolution | Unwritten futures remain open; avoid promising universal safety or a morally superior result. |

No fixed ending durations are specified. These require full scene treatment, shot plans and voice direction. Four distinct endings are required; separate full movies for every campaign permutation are not. Any branch-dependent lines or shots must read actual recorded facts.

## Additional scene work to scope

`SPEC-CIN-001` requires mission-critical scenes. Following owner direction (2026-09-11), major mid-mission turning points and mission completions must be treated as **fully production professional-style cutscenes**, not just UI panels or short camera pans. Review the authored dialogue/event triggers for the following presentation work to scope their full cinematic production:

| Package | Treatment to develop | Scope boundary |
|---|---|---|
| Tutorial deployment handoff | Brief in-engine transition from the prologue into Mara’s first camera-survey action. | May be the prologue’s final shot; preserve playable teaching. |
| Readiness completion / engagement handoff | Mara’s readiness close, deployment context and return to player control. | Use the approved tutorial script and current journey requirements. |
| Mission-critical discoveries and commitments | **Fully produced mid-mission cinematics** for major reveals, witness events, and irreversible choices (e.g., Future Well commitments). | Audit all fifteen mission triggers. These must be professional cutscenes that tell the story, not just a camera pan. Avoid interrupting minor routine objectives. |
| Mission outcomes | **Fully produced victory/defeat cinematics** for each mission. | Replace basic dialogue/result panels with state-aware professional cutscenes for mission success and failure. These must reflect the actual surviving units and base state. |

## Promotional videos — recommendations

The following is a recommended public-media set, not a required seven-video release quota. All footage must come from the running game, under `REL-PUB-011`; no generated target footage or CGI may be presented as gameplay.

| ID | Video | Suggested length | Purpose |
|---|---|---|---|
| VID-01 | Reveal / world teaser | 30–45 s | Establish Soryn, the broken sun and the central cost of using Dawn. |
| VID-02 | Gameplay / store trailer | 60–90 s | Show actual RTS command, economy, construction, combat and Future Well decisions. |
| VID-03 | Meridian Compact spotlight | 45–60 s | Show the faction’s implemented identity, infrastructure and battlefield roles. |
| VID-04 | Kharuun Assemblies spotlight | 45–60 s | Show mobile infrastructure, memory and implemented combat identity. |
| VID-05 | Hollow Choir spotlight | 45–60 s | Show implemented reality-state mechanics and faction identity without misrepresenting early campaign access. |
| VID-06 | Future Wells feature video | 45–60 s | Explain Harvest, Preserve and Reshape using actual effects and consequences. |
| VID-07 | Launch trailer | 60–90 s | Combine qualified gameplay and spoiler-controlled story footage; make availability claims only when true. |

Suggested lengths are editorial estimates. Short social excerpts can be exports from these edits. A credits screen, menu animation, tutorial lesson, skirmish result or Conquest introduction does not automatically require its own video.

## Production order and completion standard

1. Finish the M01 four-shot reference scene and its tutorial handoff to qualify camera language, voice timing and return to play; build the title environment and world prologue around that proven path.
2. Produce M02–M05 openings and ACT-01, then M06–M10 and ACT-02.
3. Produce M11–M15 and ACT-03 after its continuity decision; complete all four endings with their state-dependent content.
4. Scope additional mission-critical scenes from actual triggers, then capture promotional footage when the depicted content supports the claims.

For each sequence retain its source/shot/voice references, registered assets and rights, Sequencer binding, trigger and branch conditions, subtitle timing, score/ambience, and gameplay handoff. Check pause, replay, Escape/hold-Space skip, interruption/recovery, reduced motion/flashing, and safe camera/selection restoration. Combat must not advance invisibly; presentation must not mutate simulation state. Required subtitle synchronization is ±100 ms (`REL-CIN-007`). Final production requires packaged playback and listening evidence plus owner acceptance; authored text and source flags alone do not establish completion.

## Source references

- [Requirements master](Requirements.md): `SPEC-CIN-001..002`, `SPEC-MSN-001..015`, `REL-FTU-002/004`, `REL-CIN-001..008`, `REL-PUB-011`.
- [Requirements state](RequirementsState.md): cinematic family status and dated unfinished-playback observations.
- [Creative canon](Archive/DevelopmentBible.md): campaign outline and writing rules.
- [Opening and tutorial script](OpeningAndTutorialScript.md): existing M01 line authorship and tutorial handoffs.
- [Character and voice identity](CharacterVoiceIdentityBible.md): approved characterization and voice scope.
- [Game completion directive](GameCompletionDirective.md): Track D production workflow, subordinate to the exact requirements.


## Cross-media visual consistency guide

This section supplies the missing visual context for an AI that knows nothing about Echoes. It is the handoff's reusable reference, maintained inside this same document. It summarizes the creative canon; it does not replace the Development Bible or silently resolve a difference between a book edition and the game.

**Current book edition:** Angelis designated `Echoes_of_the_Broken_Sun_eBook.epub` from the Desktop KDP folder as the most up-to-date book on 2026-09-07. Use that edition for book comparisons; do not substitute the earlier expanded manuscript or infer book authority from a file timestamp.

**Use for every generation:** shared Soryn visual/sound direction + relevant character cards + relevant location card + relevant object/roster descriptions + chosen scene brief + the selected reference images. A shot prompt alone is insufficient. The book, game portraits/models, illustrations and videos must draw from the same chosen identities. A successful text prompt does not guarantee identity consistency between generated clips: use the same approved images/models and compare the output.

### What remains the same across media

| Keep fixed | May change for a specific scene | Must not be silently invented |
|---|---|---|
| Face, apparent age, build, hair silhouette, distinctive marks and body materials | Pose, camera distance, expression within character, motivated light | Human skin tone, eye color, exact facial proportions or height not established in source/reference |
| Costume construction, carried objects, left/right placement, seven-stone count | Source-supported damage, dirt or costume state carried continuously through a scene | New scars, weapons, jewelry, emblems, ranks, prosthetics, costume redesigns |
| Faction architecture, material families and ownership shapes | Exposure/lighting appropriate to place and time, consistent with gold/indigo light | Generic fantasy/cyberpunk replacements, another game's silhouettes |
| Landmark identity, geography and object proportions | Correct branch-specific power/Well/route state | A destroyed district substituted for a deferred dark district; a new landscape between cuts |
| Character role and source-specific physical presence | Book illustration versus in-game radio/proxy presentation, explicitly identified | Assuming a book scene authorizes a new controllable hero or physical cameo in the game |

**Important distinction:** uniform appearance does not require identical events in every medium. A novel can follow one branch of the campaign; the game may offer several. Preserve what a person/place looks like while recording which medium, chapter/mission and branch the scene represents.

### Character reference cards

The Appearance paragraphs below are reproduced from the current game creative canon. Additional framing instructions are production guidance. An unspecified feature is a real gap, not permission for each generation to choose differently. Use a previously selected likeness if available; otherwise first make one clearly labeled proposed character sheet, select its identity, and reuse it. Do not present that choice as something the book already states.

For the following cards, left and right always mean **the character's own** left and right. Do not mirror the finished image if doing so moves identifying marks or equipment. Exact numeric heights are not specified in the appearance records; use a common measured lineup when choosing production proportions rather than independently inventing heights for different clips.

#### CHAR-MARA — Mara Vey

**Established appearance:** A woman in her late thirties to early forties, tall and square-shouldered, with close-cropped dark hair going grey at one temple and a still, weather-lined face. She wears the Compact field coat: pale civic ceramic white with a charcoal load harness, a **cyan status band** on the left sleeve that shows her current duty window, and a ledger slate holstered at the right hip. A conduit burn scars the back of her left hand. She stands with her weight even and her hands still; when she is worried she gets slower and more precise, never louder.

**Identity anchors:** The left-sleeve cyan band, right-hip slate and scar on the back of the left hand are separate continuity anchors. The grey temple side is not specified: choose it once on the selected reference, document it, and never swap it. Her coat is a field coat, not ornate heavy combat armor.

**Motion/expression/voice:** Grounded adult woman, low-to-mid register; measured clipped declarations. Concern makes her slower and more exact, not louder.

**Scene-presence rule:** Portrait/full-body only where appropriate to the medium; the M01 opening remains voice-led with an anonymous carrier, not Mara walking the field.

**Still unspecified in this appearance record:** Human complexion/ethnicity, eye color, exact stature, detailed facial proportions, which temple is grey, coat fastenings and the precise sleeve-band graphic.

**Reference-sheet instruction:** Create or select front, three-quarter, profile and rear full-body views of this same individual, plus face, hands and signature-prop details. Neutral posture, neutral grey studio ground, soft even light, no dramatic color grade, consistent scale and one costume. This is a design reference sheet, not a scene illustration. Include the named identity anchors visibly; proposed fills for unspecified traits must be recorded as proposals. Use this exact selected sheet for future close-ups and video reference input.

#### CHAR-TALAR — Talar Venn

**Established appearance:** A man in his late twenties or early thirties, slight, forward-leaning, with untidy dark hair and ink at the fingertips. He wears the Archive block's grey-ceramic coat with too many pockets, a magnifying loupe on a cord, and a sealed pale-ceramic record cassette slung at his side that he touches when he is anxious. He talks with his hands. His gratitude is specific: consequence, never praise.

**Identity anchors:** Keep the slight frame distinct from Mara’s broad shoulders. Preserve loupe-on-cord, numerous pockets, fingertip ink and the sealed cassette; anxious contact with the cassette is a specific gesture, not random fidgeting. The cassette side is unspecified.

**Motion/expression/voice:** Light-to-mid adult male register, earnest and quicker than Mara; gestures follow concrete requests, never broad heroic speeches.

**Scene-presence rule:** Archive/radio roles in the game follow the mission. Do not add him physically to the M01 opening merely because this card defines his appearance.

**Still unspecified in this appearance record:** Human complexion/ethnicity, eye color, exact stature, facial hair, cassette carry side, exact pocket/cassette dimensions and fastenings.

**Reference-sheet instruction:** Create or select front, three-quarter, profile and rear full-body views of this same individual, plus face, hands and signature-prop details. Neutral posture, neutral grey studio ground, soft even light, no dramatic color grade, consistent scale and one costume. This is a design reference sheet, not a scene illustration. Include the named identity anchors visibly; proposed fills for unspecified traits must be recorded as proposals. Use this exact selected sheet for future close-ups and video reference input.

#### CHAR-ORUUN — Oruun-of-Seven-Stones

**Established appearance:** Tall and heavy-framed, with warm dark stone-toned skin banded in visible strata along the shoulders, forearms, and spine. A **collar of grown stone** seats seven translucent amber memory-stones across the collarbones; one is visibly darker than the rest. Oruun wears woven mineral-fiber wraps in charcoal and dull amber and carries a Tender's resonance staff worn smooth at the grip. Oruun moves slowly and stops completely; humor is timing, grief is a slowed cadence, and authority never needs volume.

**Identity anchors:** Exactly seven memory-stones belong in the grown collar; one is darker. Keep its chosen position fixed. Stone strata follow shoulders, forearms and spine, not random glowing cracks across every surface. Staff is a worn resonance tool, not a wizard weapon.

**Motion/expression/voice:** Deep resonant register, weighed pauses and complete stops. Humor is timing rather than a grin or laugh.

**Scene-presence rule:** Use the named figure only where the selected medium/scene supports it. M01 uses Oruun’s radio voice; do not reveal the birthing cavern or a physical Oruun cameo.

**Still unspecified in this appearance record:** Exact height, face/eye design, detailed mineral-band topology, which collar stone is darker, wrap pattern and staff dimensions. No biological sex/gender appearance inference beyond the authored description.

**Reference-sheet instruction:** Create or select front, three-quarter, profile and rear full-body views of this same individual, plus face, hands and signature-prop details. Neutral posture, neutral grey studio ground, soft even light, no dramatic color grade, consistent scale and one costume. This is a design reference sheet, not a scene illustration. Include the named identity anchors visibly; proposed fills for unspecified traits must be recorded as proposals. Use this exact selected sheet for future close-ups and video reference input.

#### CHAR-NEME — Neme

**Established appearance:** A figure that reads as one person at the center and two at the edges: an upright, still form of deep charcoal glass with fine magenta micro-fracture, its edges carrying a steady magenta luminance, and an **afterimage** displaced a hand's width to one side that lags when Neme moves and snaps into register when Neme finishes a sentence. The face is precise and calm with slightly over-articulated features; there is no clothing as such, only repeated pale light-edged panels that suggest a garment from one angle and armor from another. In reduced-motion presentation the afterimage holds a fixed offset.

**Identity anchors:** Retain one calm central identity with a coherent offset at its edge. Panels read as garment/armor depending on view without turning into a new costume. Afterimage alignment at speech endings is authored character language, not a combat power.

**Motion/expression/voice:** Even precise articulation, calm motion and deliberate wording; avoid layers that obscure intelligibility.

**Scene-presence rule:** First contact in M08 need not become an invented face-to-face meeting. M14/M15 presentation follows the actual character/unit binding.

**Still unspecified in this appearance record:** Exact face shape, eye treatment, height, panel pattern and offset side. Preserve the selected identity across all renders; do not alternate human faces or gender presentation between shots.

**Reference-sheet instruction:** Create or select front, three-quarter, profile and rear full-body views of this same individual, plus face, hands and signature-prop details. Neutral posture, neutral grey studio ground, soft even light, no dramatic color grade, consistent scale and one costume. This is a design reference sheet, not a scene illustration. Include the named identity anchors visibly; proposed fills for unspecified traits must be recorded as proposals. Use this exact selected sheet for future close-ups and video reference input.

#### CHAR-RHYSE — Chancellor Cael Rhyse

**Established appearance:** A man in his sixties with silver hair swept back, a rounded, kind face, and an immaculate pale-ceramic formal coat with the Compact's ledger-seal on a chain. The Demonstrator shows him as a half-scale cyan-white civic projection above the apparatus, steady, unhurried, with one hand open as if presenting a result.

**Identity anchors:** Rounded kind face and warm administrative manner, silver swept-back hair, immaculate coat and chain seal. His projection is half-scale cyan-white and steady. Do not change the face into a sinister villain because of the policy.

**Motion/expression/voice:** Warm, polished public-address certainty, unhurried and persuasive; no growl, sneer or threatening lighting.

**Scene-presence rule:** Game appearance: M12 only through the public Demonstrator, never a physical or playable body. The ACT-03 requirement conflict remains open. A book scene must be identified separately.

**Still unspecified in this appearance record:** Human complexion/ethnicity, eye color, precise facial proportions, exact hairline, chain geometry and ledger-seal design. Do not invent a heraldic emblem and call it canon.

**Reference-sheet instruction:** Create or select front, three-quarter, profile and rear full-body views of this same individual, plus face, hands and signature-prop details. Neutral posture, neutral grey studio ground, soft even light, no dramatic color grade, consistent scale and one costume. This is a design reference sheet, not a scene illustration. Include the named identity anchors visibly; proposed fills for unspecified traits must be recorded as proposals. Use this exact selected sheet for future close-ups and video reference input.

#### CHAR-ANNUNCIATOR — Meridian Operations Annunciator

A system voice, not an additional humanoid character. Its visual presence is the existing command instrumentation and relevant state indicator. Do not invent a robot face, avatar or new holographic guide. Voice is higher/lighter than Mara, metronomic and functional. Alerts do not imply extra narrative knowledge.

#### Anonymous and supporting roles

M01's archive carrier is an ordinary Relay Skiff with an archive cradle, not a portrait of Mara. M02/M04 memory-bearer representation is an ordinary Tender carrying a stone. Act III verifier/witness roles use the mission's ordinary Kharuun scouts, deliberately unnamed. Possible and Manifest identify Choir states, not two newly invented human characters. Civilian proxies do not authorize close-up named children, household counts or unsupported casualties. When an anonymous recurring individual must appear across shots, choose a stable local reference and do not promote it into a named canon character.

### Location reference cards

Use the following physical descriptions with the selected scene. These are transcribed from the expanded game canon. They establish the place's construction and history, not proof of its current runtime geometry or a particular book branch. The game uses the actual registered mission layout; an external image/video concept must not turn a schematic into authoritative coordinates.

For each location establish one reusable master view, one simple top-down spatial diagram and close references for the landmark/materials before filming. Keep horizon silhouette, entrances, crossings, major structures, relative scale and light direction consistent between shots. A reverse angle changes viewpoint, not geography. If the book does not name the exact same local layout, preserve the shared place identity without claiming the layout is a quotation from the novel.

#### LOC-LUME — Lume Reach — the ark-city

Lume Reach is the Compact's ark-city of the southern plateau, grown around the pre-Crownfall transit terminus. Seen
from the Glass Scar it is a long, low, pale-ceramic escarpment of stepped civic frames on a charcoal plinth, with
three tall causeway spans leaving it westward, a forest of paired conduit pylons, and status bands glowing cyan
along every trunk that still has power. Its **reserve** is a bank of Dawn-fed power cells under the central
exchange; when it sags, whole blocks of bands dim to charcoal in a visible ripple.

Three **gates** face south onto the forecourts: **Ration Gate** (west), the working gate where the Cisterns'
allocations are issued; **Census Gate** (center), the registrars' gate and the public record's public face; and
**Reserve Gate** (east), the engineers' gate above the exchange. Three **districts** stand behind them:

| District | Function | Look and sound |
|---|---|---|
| **Life Support** (north, "the Cisterns") | Air movement and water; the terminus's original reservoir works. | Conduit-dense: tiered ceramic cistern walls, discharge grilles, layered ducting in redundant pairs. Sound: layered air movement over a deep regular plant pulse. |
| **Transit** (west block) | The causeway terminus; spans, service galleries, the maintainers' quarter where Mara was born. | Elevated infrastructure: long ribbed pale-ceramic decks on dark coffers, civic ribs, broken parapets, restrained cyan conduits. Sound: long sympathetic ceramic tones under traffic rhythm. |
| **Archive** (east block) | The stacks; registers, census, ledgers; Talar's quarter. | Protected stacks: windowless stepped ceramic blocks with numbered storage bays, servo rails, and sealed cassette doors. Sound: near-silence with rare page, servo, and settling transients, the quietest place in the game. |

Missions that stand inside Lume Reach reuse these proportions, materials, and district marks so the city is
recognizable every time: M03 in a service district behind the gates; M09 at the **Authority Exchange**; M10 in the
deferred district and the **Lume Well court**; M11 on the **Census Forecourt**; M12 on the **Demonstrator Spine**
at Reserve Gate. Each has a different layout.

#### LOC-M01 — The Glass Scar (M01, and the shared skirmish map)

A narrow vitrified impact basin on Lume Reach's southern margin, split by the fractured pre-Crownfall transit span.
Dark glass shelves, broken ridge bands, black-glass shards, cyan-white Matter deposits, and long ember-dim
fracture arteries give the ground its identity. Three crossings define it: the raw scalloped trench of the **Ash
Cut** (west), the continuous ribbed pale-ceramic deck of the **Buried Causeway** (center), and the offset angular
plates of the **Folded Verge** (east). The **Glass Scar Well** sits under the broken span. Beneath the basin lies
**Understone**, the Kharuun birthing cavern; it is never shown, never given a coordinate, and exists in Mission 01
only as Oruun's stated concern.

The M01 **evacuation margin** is its own layout at the basin's civic edge: an arrival and service yard around the
Anchor; a carrier approach over fitted archive paving to the **archive working court** (a pale loading apron with
registration rails, cassette cradles, and lashings that explain the carrier's use); the worker's dark approach
between scar shoulders to the Well precinct; and the withdrawal corridor back to the **settlement threshold**, a
maintained receiving edge beside the drop-off with frames and conduit that must never read as a closed door. Glass
wind and sparse shard chimes; footsteps change from open glass to fitted paving.

#### LOC-M02 — The Shivergrass Basin (M02)

East of the Scar, a broad open basin of combed shivergrass between stepped basalt scarps, where Oruun's assembly
walks its vaultback migration every season. Polished stone shoulders, compressed grass margins, and observation
sills worn by generations of watchers show repeated passage. A basalt divider splits the basin's heart; the
inherited route (western fractured account, central archive-verified account, or eastern manifested account)
decides which crossing is open. Wind, grass friction, distant vaultback files, spacious pauses. Nothing sacred,
nothing tribal: this is a working landscape.

#### LOC-M04 — The Unburied Road (M04)

The Kharuun subsurface artery beneath the Scar country, an asymmetric sequence of grown mineral vault chambers
linked by narrow authoritative spans, with deep side voids and ribbed walls that carry load into the strata. Its
three roads bear the same names as the Glass Scar's crossings, because the crossings are where the buried roads
surface. Docking hollows and rooting fixtures show where Waystones have settled and moved on. The **shard site**
is a terminal chamber, a grown reliquary niche where a missing memory-stone was set down and not recorded.
Restrained amber cavity light, stone resonance, root-set friction, short cavern reflections.

#### LOC-M05 — The Line of Parity (M05)

The old provincial line west of Lume Reach, chosen for the ceasefire because it belongs to no one. A linear corridor
between two separately constructed networks, framed by north and south chasm faults and by repaired defensive
revetments set back from the route. The **Meridian relay** (a pale-ceramic pylon with a cyan confirm face) and the
**Kharuun spine** (a rooted Listening Spine) stand offset from each other; two **witness stations** are paired but
differently built, one a ceramic kiosk and one a grown strata bench. Relay ticks and answering resonances are
restrained; nothing here is a mirrored base.

#### LOC-M06 — Sector 9 (M06)

A census district on Lume Reach's outskirts that was sheared away: a diagonal stepped void cleaves the residential
blocks where whole neighborhoods stood in futures that were closed. The void has edges. Perimeter streets, archive
interfaces, and protected extraction routes surround it; service lines terminate cleanly at its rim with real wall
thickness and supporting brackets; clean unused foundations sit beside worn approaches. Numbered storage bays
follow the census labels. Local ventilation and relay noise fall away at the void. No graves, no names on walls, no
explanation: the disturbance is that everything around the absence still expects the absent to be there.

#### LOC-M07 — Listening-Spine Ridge, the Hollow (M07)

A stepped Kharuun ridge above Sector 9's border, where the communal memory goes quiet. Repeated resonant ribs with
maintained connection sockets and different fracture histories climb the ridge; two separated witness approaches, a
Waystone rooting place, the Spine setting, and a distinct **confluence** hollow form a listening geometry the
player reads as a whole. The hollow (Cinder, Held, or Folded by doctrine) has a center once the Waystone roots.
Paired response tones follow verified interaction; a held quiet interval separates observations.

#### LOC-M08 — The Confluence verge (M08)

Terrace ground at the edge of the Confluence Ring, where the incursions first held shape. Offset terraces and
near-matching structural frames create two distinct approach readings around a public **contact site**; duplicate
joints stop short of meeting, one frame aligns only from an authored view, and the ground reads as two places at
once. The contradiction is always in non-colliding framing, never in the walkable route. Slow offset edge motion
and answering tonal fragments respond to contact state.

#### LOC-M09 — The Authority Exchange (M09)

Lume Reach's allocation hall: a central exchange with three spatially separated branch interfaces (Life Support
conduits, Transit spans, Archive stacks) reusing the district vocabulary in a larger layout with distinct defense
fronts. Each interface has its own connection route, mechanical status position, and branch mark. The **deferred**
interface stays visibly intact and maintained: dark bands, standing structures, not rubble. The three machine
voices change only with authoritative allocation.

#### LOC-M10 — The liability district and the Lume Well court (M10)

The district Mara deferred, still dark and still standing, with a civic perimeter opening onto a **liability
interface**; two sequential Spine sites; and a separate **Lume Well court**, a Well set in a paved civic enclosure
where the city can watch a decision made. New public Choir infrastructure alters the composition: offset panes
standing beside civic frames, meeting them rather than replacing them. Controlled civic hum, spine resonance, and
contact tones keep separate sources.

#### LOC-M11 — The Census Forecourt (M11)

The public ground before Census Gate, where a coalition can assemble in the open. An inherited approach feeds two
separated district interfaces and two **public evidence interfaces** (one Meridian kiosk, one Kharuun pillar),
then a protocol **rally site**. Joining pieces use both construction languages with exposed adapters and serviced
cable-and-mineral junctions, each interface keeping its identity.

#### LOC-M12 — The Demonstrator Spine at Reserve Gate (M12)

Rhyse's apparatus: a formal demonstrator spine of redundant measurement frames, visible signal paths, and
replaceable instrument modules in immaculate pale ceramic, separating the neutral Meridian and Kharuun readback
stations, the paired district interfaces, and a distinct Well activation area. His half-scale civic projection
stands above the central module. Activation has anticipation, a stable hold, and an authoritative receipt cue. The
apparatus is neutral and attributable; it is persuasive and visibly bounded by its instruments.

#### LOC-M13 — The Crownfall public index (M13)

On the Crownfall approach, a tiered index precinct where the sky's harmonics are audible: distinct Meridian and
Kharuun record interfaces, a neutral **index** structure with readable access, and two separated witness sites.
Paired record housings retain different construction and fittings; blank or unasserted record faces stay visibly
unasserted. Measured interface tones against sparse fracture harmonics. It is an indexed structure, not a
graveyard.

#### LOC-M14 — The command-crisis basin (M14)

Higher on the approach, a basin of black glass under a sky that is visibly doubled: separate inherited Possible and
Manifest sites, a readable Neme command position, and a **crisis site** where a Phase Anchor must stand. Offset
repeated Choir structures establish the built language without obscuring force placement; the research loom and
anchor have distinct silhouettes. Held tones and interference beating are the activity level.

#### LOC-M15 — The Solar Fall Dais (M15)

The center of the Solar Fall: a geometric obsidian dais suspended directly beneath the shattered star, ringed by
void and cut by coronal rifts, where the fragment field hangs so low that its magenta halo lights the ground. A
distinct **Approach Anchor** precedes three separated **witnessed-accord sites**, one in each construction language
(civic ceramic, grown strata, offset glass), joined at the accord without becoming generic ornament. Each eligible
ending's **Resolution Conduit** has its own silhouette and stands at its own convergence offset. The shattered star
dominates cinematic framing; the gameplay view prioritizes protected sites, approaches, and hold boundaries. Music
recalls every established motif.

#### LOC-SKIRMISH — The skirmish battlefields

The three offline skirmish maps are the same country in the same war and stay canon-consistent: the **Glass Scar**
above; the **Crownfall Basin**, a skip-impact basin of twin ridges with three pale-tide gate cuts whose shelf walls
are collapsed ark-city foundations; and **The Confluence Ring**, the early coherent Choir incursion site, a central
walled ring with four cardinal entrances and repeated glass geometry with local contradictions. Skirmish matches
are engagements of the Present War with a stake (reserve versus cavern versus coherence), never abstract arenas.

#### LOC-M03 — Lume Reach service district

Use LOC-LUME as the master city identity. The M03 view is inside a service district behind the gates, not a newly designed second city. Pale-ceramic frames stand above charcoal load structure; paired conduits connect distinct interfaces along a causeway. Begin with the reserve and interface states specified by M03. Keep worker access, escort routes and the three interface positions legible; reference the actual mission layout for spatial placement. The opening's dusk lighting and inherited restoration order do not authorize a new district allocation or permanent blackout. Maintain the same district architectural vocabulary when the story returns in M09–M12.

### Shared objects and faction roster

Characters are only part of continuity. A Surveyor, Well or conduit that changes shape between the book illustration, game and a video breaks the same visual identity. The descriptions below are copied from the creative canon for the current project. State effects and timing remain governed by the requirements and actual game state; descriptive numbers in canon do not silently overrule them. A reference sheet must show the same object in its applicable states, not a fresh redesign per state.

#### Meridian Compact — unit and structure shapes

| Record | Silhouette and materials | Readable function | States, motion, sound |
|---|---|---|---|
| **Surveyor** (`SPEC-UNIT-001`) | A compact bipedal maintenance exoframe about as tall as a person and half again as wide at the shoulder: pale ceramic torso shell over a charcoal frame, two articulated tool arms (rotary drill on one, gripper/welder on the other), a small optical mast, and a rear cargo cradle holding cyan-white Matter canisters. Cyan status band across the chest. | Tools and cargo say *worker*; nothing on it says weapon. Its whole body language is service reach. | Walks with planted support and a short settle on stop. Drill spins only during actual gathering and stops on travel or cancel. Build pose extends both arms to the footprint. Repair (when authorized) shows the welder arm only. Sounds: ceramic-on-stone footfalls, a dry rotary drill, a soft cargo latch on delivery. |
| **Lancer** (`SPEC-UNIT-002`) | A two-legged line-fire frame, slightly taller than the Surveyor and narrow, with a long forward rail-lance carried at hip height, a recoil strut braced to the rear leg, and a low armored cowl. Ceramic plate over charcoal, cyan band along the lance. | The lance axis and the braced stance say *sustained ranged fire, forward-facing*. Flanks are visibly thin. | Halts, plants the strut, aims, fires with recoil returning through the mount, recovers. Never fires while moving. Muzzle is a clean cyan-white line; impact is a small engineered flash. Sound: a sharp, dry crack with a short metallic ring. |
| **Bulwark Team** (`SPEC-UNIT-003`) | A wide, low, two-operator chassis with a central emitter and six framed barrier cells on hinged wings. Packed, the wings tuck along the chassis and it reads as a heavy crawler; deployed, the wings unfold into a single directional shield face taller than the operators. | Shield facing and deployed footprint are unmistakable; the exposed rear says *flank me*. | Setup unfolds the wings at the hinges (anticipation, contact, settle); deployed movement is a slow drag; packing reverses. Impacts show on the face as brief cyan ripples. Sound: hydraulic hinge engagement, a low field hum while deployed, a clank on pack. |
| **Relay Skiff** (`SPEC-UNIT-004`) | A light, fast, low-slung skimmer with a tall relay mast, a dish, and a small forward weapon that is clearly secondary. In Mission 01 it carries an **archive cradle**: a sealed pale-ceramic cassette rack lashed to its deck with dark straps. | Mast and dish say *sees and connects*; the archive rack says *carrying something that matters*. Never reads as a hero body. | Glides with a slight nose-down lean; the mast lights cyan when relaying temporary logistics. The archive rack never changes unless an authoritative event binds it. Sound: a light turbine whine and a soft relay chirp. |
| **Anchor** (`SPEC-BLD-015.MC.ANCHOR`) | The Compact headquarters: a squat, wide ceramic drum on a charcoal plinth with a tall central mast, three visible worker bays at ground level, a Matter intake chute, and thick conduit roots running out to the network. Cyan bands ring the drum. | Root of the network, worker source, Matter drop-off. The bays and chute make production and delivery legible. | Working: bands lit, mast steady. Damaged: a band dark, panel plates visibly cracked. Destroyed: engineered collapse, drum sags on the plinth. Sound: a deep steady transformer tone, a chute clatter on delivery. |
| **Power Link** (`SPEC-BLD-015.MC.LINK`) | A slim ceramic pylon with a charcoal base, a ring of cyan conductor collars, and paired conduits that visibly leave the base and run toward its neighbors. | A pylon with cables, not a turret. Connection hardware is the whole message. | Connected: collars lit, conduits faintly pulsing along their length. Disconnected: collars dark, no pulse. Damaged: a collar dark, a conduit hanging. Sound: a thin electrical sustain only while connected. |
| **Array Foundry** (`SPEC-BLD-015.MC.FOUNDRY`) | A long rectangular hall with an intake ramp at one end, an open fabrication bay in the middle where frames are visibly assembled on a rail, an output door at the far end, and a research gantry with instruments on the roof. | Intake, work, output, and research each have their own place, so the player reads what it is doing from where the activity is. | Producing: the rail carries a half-built frame toward the door. Researching: the roof gantry lights and the rail stops. Interrupted: gantry dims, no refund animation. Sound: rhythmic fabrication clank; a rising tone during research. |
| **Aegis Post** (`SPEC-BLD-015.MC.AEGIS`) | A three-legged ceramic mount with a rotating twin-emitter head, a heavy power coupling on the base, and a cyan band that circles the head only while supplied. | Weapon direction and the power coupling say *defense that needs the network*. | Powered: head tracks, band lit. Offline: head droops, band dark, no smoke or sound implying life. Fires clean cyan-white bolts. Sound: a charged snap; silence when unpowered. |

#### Kharuun Assemblies — unit and structure shapes

| Record | Silhouette and materials | Readable function | States, motion, sound |
|---|---|---|---|
| **Tender** (`SPEC-UNIT-005`) | A stocky Kharuun cultivator carrying a resonance staff and a woven mineral-fiber sling of carried matter across the back; forearms thickened with working strata; amber nodules at the wrists glow while growing. | Carried matter and the staff say *cultivator*, not soldier. | Gathering is a kneeling press of the staff into strata; growing a structure is a slow circling walk that leaves the organism's first ring. Sounds: stone resonance, a soft crumble on gather, a low sustained tone while growing. |
| **Riftstalker** (`SPEC-UNIT-006`) | A lean, long-limbed warform with a low forward posture, a faceted carapace in charcoal with amber seams, and a shoulder-mounted shard-caster that fires while it moves. | The forward lean and light frame say *skirmisher that keeps moving*; it visibly lacks the mass for a frontal fight. | Fires on the move with a short sidestep after each shot. Molt at a Growth Basin shows a visible carapace or striker change. Sounds: a ceramic hiss on fire, sharp shard impacts. |
| **Cairnback** (`SPEC-UNIT-007`) | A broad, low assault warform whose back is a slab of layered heat-holding strata like a vaultback's; thick forelimbs; head low and protected. | The strata back says *absorbs fire, becomes cover*. | Creating mineral cover is a heave that leaves a grown barrier behind it. Damage chips strata; destruction is a ceramic slump. Sounds: heavy stone footfalls, a grinding heave, a low ceramic resonance on loss. |
| **Resonant** (`SPEC-UNIT-008`) | A tall, thin scout with sensor-fins of translucent amber along the spine and head, and a delicate frame. | Fins and delicacy say *listens, does not fight*. | Detecting shows the fins brightening in sequence. Sounds: a faint rising resonance; almost silent movement. |
| **Memory Hearth** (`SPEC-BLD-016`) | The Kharuun headquarters: a wide grown dome of banded strata with a warm amber glow from within, several arched worker hollows at the base, a matter-intake cleft, and a crown of rooted adaptation spires. | Hollows and cleft make growth and delivery legible; the crown says *adaptation root*. | Working: interior glow breathes slowly. Damaged: a spire dark, strata cracked. Destroyed: ceramic collapse inward. Sounds: a deep communal hum far below music tempo; cleft settle on delivery. |
| **Waystone** (`SPEC-BLD-016`) | A tall faceted monolith of dark strata with amber seams that, rooted, sinks a visible ring of root-strata into the ground; uprooted, it lifts on a grown carriage and moves slowly. | Rooted or moving is visible from the roots alone. | Rooting: preparation, contact, settling, release. Sounds: a grinding root-set, a low tone while rooted. |
| **Growth Basin** (`SPEC-BLD-016`) | A shallow bowl of grown strata with an amber-lit matrix pool at its center, ringed by visible molt niches. | The pool says *grows warforms*; niches say *adaptation choices*. | Growing shows the pool brightening; a molting warform stands in a niche and visibly changes. Sounds: liquid mineral resonance; a crack-and-settle on molt completion. |
| **Listening Spine** (`SPEC-BLD-016`) | A single tall rib of strata with amber sensor nodules climbing it, set into a rooted socket. | A spine, not a weapon. | Detecting shows nodules lighting in sequence toward the source direction. Sounds: a slow pulse that quickens with movement signatures. |

#### Hollow Choir — unit and structure shapes

| Record | Silhouette and materials | Readable function | States, motion, sound |
|---|---|---|---|
| **Threadkeeper** (`SPEC-UNIT-009`) | A slender, upright figure of charcoal glass whose forearms end in fine luminous filaments rather than tools; a small carried pane of Matter held against the chest; the afterimage trails a half-step behind. | Filaments and carried pane say *worker*; it has no weapon and no bulk. | Gathering: the filaments touch the deposit and brighten. Building: the filaments weave a luminous lattice that the structure then fills. Reconciling a structure shows its next upkeep tick. Sounds: a held glass tone that beats faintly against itself. |
| **Intervalist** (`SPEC-UNIT-010`) | A mid-height phase skirmisher with a long, thin emitter along one forearm and an angular, faceted body. Unresolved, it carries both silhouettes at equal strength; resolved **Manifest**, the afterimage snaps into the body and the edges thicken; resolved **Possible**, the body thins and the afterimage leads rather than lags. | Two-state identity is visible from silhouette alone. | Transition (160 ticks) shows both markers at once with no bonus; resolution snaps one way. Fires a thin magenta line with a doubled impact. Sounds: two tones converging to one on resolution. |
| **Lacuna Warden** (`SPEC-UNIT-011`) | A heavy, wide controller with a broad chest pane and two forward-mounted tether emitters; slow, planted, the densest Choir silhouette. | Mass and the tether emitters say *anchor and control*. | Bind Interval projects a visible tether beam that slows and locks a target; the beam shatters visibly when line or distance breaks. Sounds: a sustained low interference beat while tethering. |
| **Afterimage** (`SPEC-UNIT-012`) | The fastest and lightest Choir form: almost all edge and no body, a long low glider with its afterimage stretched far behind. | Speed and thinness say *scout and deception*. | Forked Trace releases two anonymous moving signatures into enemy fog; they read as vibration contacts, never as units, and carry no collision. Sounds: a whispering doubled glide. |
| **Concordance** (`SPEC-BLD-017.HC.CONCORDANCE`) | The Choir headquarters: a ring of tall glass panes standing in offset pairs around a central held tone, with worker emergence between panes and a Matter intake at the ring's one true gap. | The ring and the gap make emergence and delivery legible; the offset panes say *Choir*. | Working: panes hold a steady edge glow; the coherence ledger is readable from it. Damaged: a pane goes dark and its partner loses its offset. Destroyed: the ring collapses into register and goes out. Sounds: the deepest held tone on the field, with interference beating as activity. |
| **Interval Loom** (`SPEC-BLD-017.HC.INTERVAL`) | A small frame of two crossed luminous spans with a Matter drop-pane beneath; the spans cast two shadows. | Small, connective, a drop-off with a rent ticker. | Upkeep tick shows as a brief brightening then dimming. Insolvency shows the spans losing luminance before the structure fails. Sounds: a soft interval chime on each charge. |
| **Chorus Loom** (`SPEC-BLD-017.HC.CHORUS`) | A wider loom of many parallel luminous threads between two glass pylons, with a visible weaving space where units form and a research pane above. | Threads and the weaving space say *production*; the pane says *research*. | Producing: threads draw a silhouette that fills in. Researching: the pane brightens and the threads still. Sounds: layered tones that resolve in more than one direction before committing. |
| **Phase Anchor** (`SPEC-BLD-017.HC.ANCHOR`) | A single tall glass spire whose afterimage is exactly in register — the only Choir structure with no offset — projecting a faint magenta field ring at its 700 cm coverage. | Perfect register says *stability*; the ring says *aura*. | Field active: ring steady, structures inside show reduced upkeep. Field lost: the ring collapses and neighbors flicker once. Sounds: a pure sustained tone with the interference beating removed. |

#### PROP-DAWN / PROP-MATTER / PROP-WELL

**Dawnshard:** mineral-organic, warm amber-gold body with fine magenta fractures containing undecided possibility. It is not a cyan resource crystal or ordinary money. **Matter:** the working resource shown with cyan-white identity; worker tools and cargo handling must remain materially grounded.

**Future Well:** courtyard-scale vitrified-glass bowl with a central raised core spire, deep charcoal surfaces and fine magenta fractures, faint duplicated outlines/shadows and restrained mist. Keep bowl diameter, rim shape, spire profile and nearby landmark relationship identical across branch shots. Harvest, Preserve and Reshape are distinct state treatments of the same place; do not generate three unrelated monuments. Harvest is windfall/permanent loss; Preserve is continuing custody; Reshape is temporary manifested terrain. Specific event timing/colors must be reconciled with the master and current state before final integration.

**Archive cassette/ledger props:** use the character cards and Relay Skiff description. Talar's sealed carried record cassette, Mara's right-hip slate and the skiff's strapped archive rack are different objects with different scale and purpose. Do not reuse one model for all three. Text and seals should be composited from an approved design; generated illegible writing is not a final record.

**Ending conduits:** use END-01..04 in this document. Restoration's tall braided lattice, Stabilization's low clamp, Extinguishment's amber spire and Evolution's unfinished offset glass are four distinct authored silhouettes. Preserve their specific convergence and branch. Do not mix parts because all endings occupy the same general dais.

### Scene-to-reference assembly map

| Scene | Required place/object references | Character appearance versus voice |
|---|---|---|
| FD-01 / FD-02 | LOC-LUME, LOC-M01, shared sky, faction rosters, PROP-DAWN/WELL | No invented narrator body or hero montage; visible character requires a selected card and source-supported role. |
| M01 | LOC-M01, Compact roster, PROP-WELL and archive rack | Mara/Talar/Oruun voices; no physical named-character cameo. |
| M02 / M04 | LOC-M02 / LOC-M04, Kharuun roster | Oruun voice and mission-authorized ordinary proxies; use CHAR-ORUUN only for a justified portrait/illustration. |
| M03 | LOC-M03 + LOC-LUME, Compact roster | Mara voice; presence follows mission. |
| M05 | LOC-M05, both infrastructure languages and witness props | Preserve two separately authorized witness/escort roles. |
| M06 | LOC-M06, Archive material/props | Talar; don't invent exposed civilians' names/faces or numerical population claims. |
| M07 | LOC-M07, Waystone/Spine | Oruun and ordinary witness roles. |
| M08 | LOC-M08, Meridian proxies and coherent Choir geometry | Talar and Neme voices; contact imagery does not authorize a physical meeting. |
| M09 | LOC-M09 + LOC-LUME district vocabulary | Mara; preserve the actual allocation and intact deferred district. |
| M10 | LOC-M10 and inherited district | Oruun; Mara off-map; local Choir contact is not a playable roster. |
| M11 | LOC-M11, paired public interfaces | Oruun/ordinary verifier; no generic merged-faction army. |
| M12 | LOC-M12, separate Well/readbacks | Rhyse projection only, CHAR-RHYSE; Oruun and verifier roles. |
| M13 | LOC-M13, index/Spine | Authorized observers; no invented assembly crowd. |
| M14 | LOC-M14, Choir roster | Neme and distinct protected voice states, not newly invented personas. |
| M15 / END-01..04 | LOC-M15, branch conduit and actual witness sites | Neme commands; Mara/Oruun/Talar witness. Selected likenesses do not replace runtime bindings. |
| ACT-01..03 | Their specified location cards and exact continuity notes | Use approved voice scope; ACT-03 Rhyse conflict remains unresolved. |
| Promotional videos | Same references as the captured footage | Captures cannot invent a face, roster capability or campaign presence. |

### What to send the outside AI

Send the following as one scene package. An ID or a local filepath by itself means nothing to an AI that cannot access it: include the actual relevant text and attach the selected images or supported asset files.

1. The shared Soryn visual/sound direction and the complete chosen scene brief.
2. Every applicable character, location and object card above, plus the exact mission/book context and branch. State which characters are visible, which are only heard and which must not appear.
3. The same selected face/full-body turnaround images for each visible recurring character, and the same location establishing image/diagram/material references for every shot in that place. Annotate which image controls identity versus lighting/style. Do not attach conflicting concept variants without selecting one.
4. Current costume/prop/scene state: visible damage, carry side, powered/dark interfaces, Well protocol, time/light direction and any changes established by the previous shot. Use actual source facts; leave unestablished outcomes absent.
5. For a continuation shot, the accepted last frame of the preceding shot plus the selected identity references. Explicitly request preservation of facial proportions, costume, material, landmark spacing and screen direction. A seed may help reproducibility but is not an identity guarantee.
6. Output format and completion class: concept/animatic, book illustration, editable Unreal sequence or edit of captured gameplay. Keep readable text/logo/subtitles editable.

**Copyable instruction for the receiving AI:**

> Treat the attached character and environment references as a continuity contract. This is for a professional video game production. You must preserve the same individuals, faces, body proportions, costume construction, left/right marks, signature objects, faction forms, and location geometry in every image and shot so they match the concepts and models used in actual gameplay. Change only the pose, camera, light and state explicitly called for by this scene. Do not reinterpret the designs to suit a generic visual style, and do not let the quality drop below professional cinematic standards. Do not infer lore or physical character presence from a voice line. If a needed feature is absent from the references, identify it as unspecified and prepare a clearly marked proposed reference first; do not let it vary silently between shots. Return a short continuity comparison alongside the output.

### Selecting visual references before full video production

Existing concept art is useful but is not automatically the final design. The asset register's CONCEPT-025 language includes armor/pauldrons/brass/gold variations that differ from the Appearance paragraphs above. The separate concept-production worktree also retains multiple variants and review selections; those records explicitly distinguish delegated visual direction from individual owner approval or final production acceptance. Do not gather every available image and let the receiving AI average them together.

For each recurring character, select **one** identity sheet and its compatible views; for each recurring location, select **one** master environment reference with compatible reverse angles. Retain the chosen file identity and date in the production handoff. Any superseded image should be labeled superseded in the reference package. A selected sheet must preserve the established anchors, and fills for unspecified traits must be explicit. The next step after this written guide is selection/reconciliation of those reference sheets, not independent generation of every scene from prose alone. This document does not claim that those visual identities have already been owner-approved.

### Source and book comparison record

**Owner-selected current book:** [Echoes_of_the_Broken_Sun_eBook.epub](</Users/angelispseftis/Desktop/Writing/Books/Echoes of the Broken Sun/01_eBook_EPUB/Echoes_of_the_Broken_Sun_eBook.epub>), explicitly designated by Angelis on 2026-09-07. The file was read directly as an EPUB for this comparison; no book content or metadata was edited.

**Artifact identity:** SHA-256 `7c01e138fb8badb0e0795355817cf0ce7002205f5a28ebfc78d6c2559fb8c2cc`; package `EPUB/content.opf`; title *Echoes of the Broken Sun*; creator/publisher Angelis Pseftis; embedded modified timestamp `2026-09-06T16:14:55Z`; identifier `105bd1c1-7000-4821-ad87-aae4a30303d6`. The package spine includes navigation plus `ch001.xhtml`–`ch031.xhtml`; file numbers are not chapter numbers. The locators below are internal XHTML filenames plus the actual chapter, not invented printed pages. Hash identifies the checked bytes; a future edited EPUB requires a refreshed comparison.

**Authority boundary:** this EPUB controls what the current book says. The game's Development Bible supplies additional production appearance details, and the requirements control game behavior. A detail absent from the book is not a contradiction, but must not be represented as a book quotation. A direct disagreement is recorded below for reconciliation. This is a targeted visual continuity comparison, not an exhaustive line-by-line certification of the whole novel and game.

| Subject | Direct current-EPUB evidence | How to use it with the character/place card |
|---|---|---|
| Mara | Chapter 1, *What the Ledger Keeps*, `EPUB/text/ch004.xhtml`: “Mara Vey read the two reports without changing her expression.” | Supports composure. Age, haircut, grey temple, hand burn, coat/harness and sleeve/hip placements originate in the game Appearance record, not this book passage. Preserve them as game-canon elaboration, without falsely claiming the book describes them. |
| Talar | Chapter 1, `ch004.xhtml`: grey ceramic inserts, shelf-polished soft joints, too many pockets; carrying a cassette “as though it were something injured.” | Strong match for Archive costume and protective cassette handling. Hair, age, ink, loupe and cassette color are additional game-canon fields. The book's pale cheek line is scene evidence, not automatically a permanent scar for the identity sheet. |
| Oruun | Chapter 3, *Seven Accounts of Rain*, `ch006.xhtml`: “Warm dark strata crossed Oruun’s shoulders and forearms”; amber nodules/collar, seven stones including a darker one, staff. | Matches mineral-organic anatomy and memory collar. The book explicitly rejects a human occupant inside a machine. Tall/heavy build, spine bands, wraps and precise translucence are elaborated in game canon. |
| Neme | Chapter 15, *The Shape Beside Us*, `ch019.xhtml`: almost ordinary proportions centrally, two limb/shoulder arrangements at the edges, magenta fractures on charcoal face, pale-edged coat/protective panels; afterimage settles into register. | Direct visual match. The hand-width displacement and exact movement/speech/reduced-motion behavior come from the game specification. No ghost, magical robe or hidden original person. |
| Rhyse | Chapter 20, *The Future That Won*, `ch025.xhtml`: projection above the Demonstrator at half human scale, open hand, silver hair, pale formal coat, rounded face, ledger seal. | Direct match for projection identity. Apparent sixties age and cyan-white treatment come from the game Appearance record. A recorded presentation does not become an interactive bodily encounter. |
| Well / Glass Scar | Chapter 1, `ch004.xhtml`: core spire in a vitrified bowl under a broken transit span; on Harvest, “The spire folded into the bowl.” | Fix bowl/spire/transit identity across media. This chapter follows Harvest; Preserve/Reshape game branches are alternatives, not events to insert into the book's same sequence. |
| Broken-sun light / dais | Chapter 1, `ch004.xhtml`: gold/indigo shadow duality. Chapter 24, *The Broken Sun*, `ch029.xhtml`: obsidian planes, void, coronal rifts, thin magenta halos, doubled edges and low harmonic. | Supports shared sky and Solar Fall look. Game-specific site layout and convergence offsets remain game production details. |
| Confluence | Chapter 15, `ch019.xhtml`: dark-glass terrace, frame reading as four corners from one approach and five from another; precisely assembled structures disagree with themselves. | Use deliberate coherent contradiction. Game collision/walkability remains unambiguous; a visual paradox cannot rewrite a playable route. |

**Specific differences and scope limits to preserve**

- **Relay Skiff:** Chapter 1 calls it a fast, thin-hulled carrier with a tall sensor mast/archive rack and says **“It could not fight.”** The game creative roster describes a small secondary weapon. A book-faithful Chapter 1 video must not show the Skiff firing. Its weapon design/capability is an unresolved cross-media decision; do not silently erase the game feature or add it to the book. The shared carrier silhouette, mast and archive rack remain usable.
- **Understone:** Chapter 1 names the birthing cavern and Oruun's concern. The M01 game place brief explicitly keeps it unseen and without coordinates. No game-opening cutaway into the cavern. Any separate book illustration of a later interior needs that chapter's own direct setting reference; general Kharuun cavern imagery is not proof of Understone's layout.
- **Harvest visual effect:** the book and expanded canon emphasize amber buildup, collapse and permanent loss, while `REL-ART-014` calls for an intense vertical cyan geyser. **Reconciliation:** To satisfy both, the game effect begins with the intense vertical cyan geyser (`REL-ART-014`), which rapidly calcifies into the amber buildup and then collapses into permanent loss (book canon). This sequence unifies both visual signatures without conflict.
- **Mara's deployment:** her identity sheet does not authorize a named physical Mara character in M01; the game uses the anonymous carrier/command voice. A book depiction must use its own scene location and role.
- **Rhyse before M15:** the existing ACT-03 requirement names his voice while the game character scope restricts his appearance to M12 public apparatus. Do not add a live Rhyse, invented broadcast or projection location to solve it.
- **Concept variants:** armor, decorative brass/gold, alternate weapons, proportions and interface graphics in a development concept do not automatically supersede the book/game descriptions. Review and select references explicitly. Annotated concept-sheet dimensions are not accepted physical scale merely because they are printed on an image.

#### Book supporting characters — coverage and remaining reference work

The five principal cards are not the novel's entire cast. The following bounded findings prevent a receiving AI from substituting principal-character designs for named supporting people. These are book-scene references, not newly authorized game characters.

**BOOK-KESH / Kesh-Who-Returned:** Chapter 3 (`ch006.xhtml`) presents an ordinary Tender, broad forearms, comparatively smooth working strata and a wrapped recall-stone in a sling. Chapter 24 (`ch029.xhtml`) places her tending accord roots with a staff and later gives her a shoulder wound splitting her working strata. Keep mineral-organic anatomy, sling/recall-stone and staff consistent; do not give her Oruun's seven-stone collar or a human assistant's body. Make pre-wound and post-wound state references; do not put the later wound into earlier chapters. Exact face, side/shape of injury and full costume design require their source context or a selected proposal.

**BOOK-REL:** Chapter 24 (`ch029.xhtml`) identifies a Resonant/scout with brightening fins. Preserve sensor fins and the Resonant design family. This finding does not fix a complete individual face/body sheet; create a separately identified supporting reference before a prominent close-up.

**Hesh, Sera and Deren:** named recurring roles were located, but this bounded pass did not establish complete appearance sheets. Do not improvise prominent close-ups or reuse Mara/Talar/Kesh likenesses. If a requested book scene includes them, extract that scene's description and establish their reference first. Their absence from the game roster does not authorize adding them to a game mission.

**Reference status:** the written cross-media descriptions are now supplied. Final selected face/model sheets, full turnarounds and harmonized resolution of the listed conflicts are not established by this text review. Use this packet to produce/select consistent reference sheets first, then build the shots from those same inputs.


## Detailed AI production handoff

### How to use these briefs

Give the receiving AI **the shared direction below, the relevant cross-media character/location/object cards above, and the complete brief for the selected item**. The fifteen mission briefs include the actual authored shot descriptions and dialogue, so those sections are usable without opening JSON files. Supply approved game reference images and registered models when available; text alone does not establish exact asset likeness. If the AI has project access, the linked source remains controlling. If it does not, it must not guess a mission's branch state, map coordinates, character face or finished game asset.

**Choose the output honestly:** an AI that controls Unreal should build editable Level Sequences for in-game use. A text-to-video AI can make a visual concept/animatic from the same descriptions, but its rendered MP4 is a planning reference and does not satisfy the current in-game pipeline. This request expands descriptions; it does not change that requirement. Public trailers use real captured game footage and may use an editing AI to assemble it.

**What is fixed and what is proposed:** “Authored image” and “Authored dialogue” reproduce registered mission source. All added lens choices, camera moves, shot divisions outside the mission source, preview settings and extra transition durations are proposed production direction. The front-door, act and ending treatments interpret their cited requirements/canon; they are not existing approved scripts. Where dialogue is not established, produce an instrumental animatic and a marked voice slot, not invented supposedly approved narration.

**Suggested review output:** 1920×1080, 16:9, 30 fps, with 48 kHz audio. These are exchange/preview choices, not new release requirements. Preserve a clean full-frame master; apply letterbox consistently as a presentation overlay and keep subtitles separately editable. Provide individual numbered shots, the assembled reference edit, a shot/asset manifest and subtitle file. For Unreal delivery also supply the editable sequence, cameras, audio/subtitle bindings, state inputs and tested handoff. Use `<item-ID>_<shot-number>` names. Do not burn artificial interface text, watermarks or generated logos into the image. Use the exact game title as a separate typography layer when requested.

### Shared visual and sound direction — include with every brief

Create an original, restrained science-fiction world called **Soryn**. The ground is deep charcoal vitrified basalt with layered fractures, broken engineered causeways and sparse glass debris. Large ground surfaces are matte, quiet and low in saturation; reflections never make the terrain look wet or metallic. Pale ceramic infrastructure bears repairs at edges, joints and load points. Avoid uniform dirt, generic cyberpunk streets, medieval ruins, fantasy crystals used as treasure, and recognizable designs from another game.

The sun is **not a complete yellow disc and not a black hole**. Show a small, hot, ragged off-white/gold core low in one quarter of the sky. Opposite it, a broad granular arc of charcoal and ember stellar fragments hangs like a broken crown, thicker along one trailing edge. Its drift is no faster than clouds. The gold core supplies warm directional light; the arc side supplies cool indigo shadow fill. A thin magenta halo belongs to the nearest anomalous fragments, not a pink sky wash. Sky views belong in low-angle cinematic shots; normal tactical views inherit the light without tilting up to show the sky.

**Meridian Compact:** maintained civic engineering. Pale beveled ceramic over exposed charcoal frames, orthogonal rails, paired exterior conduits, modular sockets, cyan status bands and rectangular bracket marks. Powered bands are cyan, severed bands dark, alarms amber. Surveyors have tool arms and rear cargo cradles; Lancers have a long rail-lance and braced firing stance; Relay Skiffs have relay masts. No generic tanks or armored space marines substituted for the roster.

**Kharuun Assemblies:** sophisticated grown mineral construction, dark banded strata, facets, load-bearing ribs, amber nodules and polished docking hollows. Waystones are tall rooted monoliths that can move on grown carriages; Listening Spines are sensor ribs, not guns. Kharuun bodies are heavy-framed, humanoid, stone-toned with strata and amber memory nodules. Avoid primitive, tribal or monstrous coding.

**Hollow Choir:** deep charcoal glass, fine magenta fractures, repeated luminous edges and coherent offset silhouettes about a hand's width apart. Local geometries contradict each other deliberately while remaining stable. These are people of incompatible futures, not ghosts, demons or a hive mind. State changes alter the relation between body and afterimage; never create random body melting or continual glitches. A Phase Anchor has perfect registration, unlike other Choir forms.

**Future Well:** a courtyard-scale bowl of vitrified glass with a raised core spire, faint doubled outlines/shadows and restrained mist. It is a place, never a collectible floating crystal. Dawnshards are warm amber-gold mineral-organic material with fine magenta fractures; spending Dawn closes possibility. Matter is cyan-white working material. Do not interchange their visual meanings.

**Camera:** motivated, ground-referenced dolly/tracking moves with level horizons; no drone plunge from orbit, whip pans, handheld shake, rotating dutch angles or impossible passage through solid geometry. Lens references below are full-frame-equivalent composition suggestions. Maintain architecture, light direction, unit count and screen direction across cuts. Ease into movement and settle before the cut. Preserve a readable foreground/midground/background hierarchy and moderate depth of field; the story object must remain legible.

**Sound:** wind and sparse shard chimes in the Scar; dry servos, conduit hum and ceramic contact for Compact sites; root friction, stone resonance and amber tonal responses for Kharuun sites; controlled glass harmonics and interference beats for Choir phenomena. No stock thunderous trailer hits at every cut. Dialogue remains intelligible. Mara is measured, concise and grounded; Talar earnest and quicker; Oruun deep, deliberate and unhurried; Neme precise, even and intelligible rather than a chorus effect; Rhyse warm public-address certainty, never villainous growling. Existing voices remain radio/off-screen unless a specific source authorizes their visible representation. No improvised lip-sync close-ups of invented faces.

**Editing and state:** start at the specified event and leave the player in the correct state. Scene motion cannot perform a player objective, preselect a Well protocol, alter a ledger or silently advance combat. Authored images that depict a later action must be scheduled after the relevant event or presented as clearly marked briefing visualization; a briefing preview cannot claim the action already happened. For a standalone concept render, label the preview in delivery metadata. Preserve branch-specific lights, routes, interfaces and Well state. No crowds of rescued people, body counts, moral scores or causal villains added for emotional emphasis.

**Accessibility:** restrained motion, stable exposure, no strobing or whiteout transitions. Provide static-camera alternatives. Put exact spoken text into editable speaker-identified captions on a high-contrast background, timed to the actual take. Source shot durations are targets: if a line does not fit naturally, extend the shot and report the new timing rather than accelerating or truncating speech. World prologue remains ≤90 seconds. For source conflicts, prepare unaffected visuals and state the exact unresolved item.

### FD-01 — Living title-screen environment

**Purpose:** a quiet, recognizable Soryn vista behind a usable menu. **Proposed preview:** a seamless 24-second loop; a single restrained shot, no narrative event and no voiceover. The game title/menu are separate live UI layers.

**Build this image:** camera at a low overlook beside a worn pale-ceramic causeway parapet. A narrow dark ground shelf occupies the lower foreground; ember-dim fractures lead into a middle-distance vitrified basin. Farther back, Lume Reach reads as a long low ceramic escarpment with paired pylons and a few stable cyan trunk bands. Place the small ragged gold core on one side of the upper frame and the wide dim fragment arc on the other. Use negative space behind the actual menu layout; in the concept default, keep the left 40% quiet and put the principal sky/settlement detail to the right. Do not enlarge the sun into a full disc or add ships, battles or a city explosion.

**Camera/action:** 32 mm equivalent, level horizon, locked position by default. Over the loop only slow cloud/fragment drift and sparse dust move; phase and particle systems return seamlessly to their initial state. Optional camera drift must return without a visible reversal; reduced motion locks it. Keep the title legible throughout. **Sound:** sustained title theme, low wind, one or two distant glass chimes; seamless tails without a loud event at the seam. **End:** identical composition, exposure and sound envelope to the first frame. **Deliver/check:** inspect at least two consecutive loops for a jump and overlay the real menu to check readability.

### FD-02 — World prologue / title cinematic

**Purpose:** show the condition of the world, the nature of Dawn and the three civilizations before narrowing into the player's first responsibility. **Proposed edit:** 80 seconds, eight shots. Requirement ceiling: 90 seconds. This is a new editorial treatment, not a recovered approved screenplay. Depict the surviving aftermath of the breaking; do not invent its cause or an eyewitness historical reconstruction.

| Shot / time | Specific image, movement and action | Sound / transition |
|---|---|---|
| 01 / 0–10 s | 28 mm low vista. Start on a black-glass foreground ridge, then tilt slowly enough to reveal the ragged core and broken-crown arc above the same landscape. End on the sky with the ridge still anchoring scale. | Sparse wind; introduce a low fracture harmonic. No explosion or explanatory godlike voice. |
| 02 / 10–20 s | 50 mm side view of a fractured civic causeway: real wall thickness, separated ribs, ember seams in the ground beneath. Track a short distance along intact hand-worn parapet into the interruption. | A dry chime and distant infrastructure hum carry across the cut. |
| 03 / 20–30 s | 75 mm detail of a Dawnshard in an existing approved containment fixture. Amber-gold body, fine magenta internal fractures. Hold its scale and faceting, then shift focus to the paired conduits feeding an ark-city system behind it. Do not consume it on camera without an authored event. | The fracture harmonic meets a measured machine pulse; no coin or loot jingle. |
| 04 / 30–40 s | 35 mm lateral view through a Compact service court: repaired ceramic plates, visible load frames, Surveyor tools and an Anchor intake. Show ordinary working posture, not invented new equipment. | Ceramic foot contact, subdued servo/cargo sounds and a steady cyan-system hum. |
| 05 / 40–50 s | 35 mm matching lateral direction through a Kharuun passage: banded ribs, polished steps, rooted Waystone and maintained amber sensor nodules. Keep the scene civilized, purposeful and inhabited without adding a named hero. | Machine pulse recedes into root/stone resonance. |
| 06 / 50–60 s | 40 mm slow approach to a Choir span. Two near-matching edges and two valid shadows become visible through parallax; charcoal-glass body stays coherent. This is a world overview, not a claim that Choir is playable in M01. | Add two restrained glass tones; speech remains clear if later approved. |
| 07 / 60–70 s | 40 mm view of a Future Well from rim level. Bowl in foreground, core spire midframe, duplicated shadow on the far slope. Move only far enough to expose the offset. Do not play all three protocols or choose one. | Low held Well tone; tension comes from ambiguity, not a horror sting. |
| 08 / 70–80 s | Match-cut back to the actual readiness-check staging area. From a moderate overlook settle into its authored tactical starting view, leaving the starting unit and route readable. Fade the cinematic mask/UI separation away and reveal the real lesson prompt. | Bridge into tutorial ambience. Mara's first teaching line begins only at its tutorial trigger. |

**Voice:** The following approved world-prologue narration is spoken by an anonymous archivist (distinct from M01 voices). Final language retains that the cause of the sun's breaking is unknown to the people.
**Authored dialogue — preserve speaker and order:**
- **Shot 01:** "The sun broke, and we did not see who struck the blow. We only inherited the light that survived."
- **Shot 02:** "The old causeways fractured, dividing the world into isolated accounts of what used to be."
- **Shot 03:** "Now, the only currency that matters is possibility. The Dawnshards."
- **Shot 04:** "The Meridian Compact measures that possibility in ledgers and logistics, seeking to restore the grid."
- **Shot 05:** "The Kharuun Assemblies grow it into their very strata, remembering a past that refuses to die."
- **Shot 06:** "And the Hollow Choir... they exist in the space between futures, coherent but contradictory."
- **Shot 07:** "At the Future Wells, these three paths converge. Every choice consumes a possibility. Every commitment has a cost."
- **Shot 08:** "We have no more time to argue over the past. We must secure what remains."

**Dependency:** The world-prologue script/voice scope is now resolved and distinct from M01.

### M01 — What the Ledger Keeps — Opening Brief

**Place:** Glass Scar evacuation margin. **Authored timing target:** 32.7 seconds / four shots. **Start:** `nar_m01_evt_operation_started`. **Source:** [m01_what_the_ledger_keeps.json](../Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Glass wind, sparse shard chimes, soft relay contact; urgent restrained Compact pulse. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–10.7 s · `nar_m01_shot_001`**

**Authored image:** Hold on the Glass Scar and the evacuation corridor. Show objective geometry, not a named character body.

**Proposed camera and staging:** 32 mm elevated oblique wide. Hold the corridor as a continuous readable line from the civic arrival edge through the archive court toward the settlement threshold; a short sideways drift exposes the broken scar shoulders. Keep the Well subordinate to the evacuation route.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “The evacuation ledger closed an archive convoy that never reached Lume Reach.” (`nar_m01_line_talar_001`; operations_radio).
- **Mara Vey:** “Then the ledger is wrong. Mark the recovery site.” (`nar_m01_line_mara_001`; command_radio).

**Bindings:** visual `vis_m01_glass_scar_overview`; audio `aud_m01_ambience_glass_scar`. Use these as source references, not proof that assets exist.

**Shot 02 · 10.7–17.4 s · `nar_m01_shot_002`**

**Authored image:** Move to the recovery-site marker at tile 22,18 and the anonymous archive-carrier contact. Keep Mara as command voice only.

**Proposed camera and staging:** 50 mm medium-wide tracking move along archive paving toward the recovery marker. Stop with the anonymous Relay Skiff carrier and its sealed cassette rack in the same frame; maintain the previous direction of travel.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “Tile twenty-two, eighteen. The carrier still answers. The city's reserve does not have long.” (`nar_m01_line_talar_002`; operations_radio).

**Bindings:** visual `vis_m01_archive_route_overlay`; audio `aud_m01_ambience_glass_scar`. Use these as source references, not proof that assets exist.

**Shot 03 · 17.4–27.3 s · `nar_m01_shot_003`**

**Authored image:** Frame the Future Well and a legible propagation line toward the Kharuun birthing cavern. Oruun remains radio-only.

**Proposed camera and staging:** 45 mm controlled push toward the Well rim, retaining the core spire and surrounding fracture ground. Show only a schematic direction indicator for Oruun’s warning, explicitly attributed to his report; never reveal, map or cut inside Understone.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Collapse that Well and the fracture reaches a birthing cavern.” (`nar_m01_line_oruun_001`; cross_faction_radio).
- **Oruun-of-Seven-Stones:** “Seven accounts agree. I distrust the occasion.” (`nar_m01_line_oruun_002`; cross_faction_radio).

**Bindings:** visual `vis_m01_future_well_propagation`; audio `aud_m01_ambience_glass_scar`. Use these as source references, not proof that assets exist.

**Shot 04 · 27.3–32.7 s · `nar_m01_shot_004`**

**Authored image:** Return immediately to the tactical view on Mara's carrier-first order and restore full player control without implying a pre-rendered sequence.

**Proposed camera and staging:** Blend into the actual tactical camera pose rather than zooming out through a new invented landscape. Hold the carrier and its next task clearly before releasing control; restore only the appropriate current selection.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Carrier first. Well second. Withdrawal before the reserve fails.” (`nar_m01_line_mara_002`; command_radio).

**Bindings:** visual `vis_m01_tactical_handoff`; audio `aud_m01_ambience_glass_scar`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M02 — Seven Accounts of Rain, one basin at first light

**Place:** Shivergrass Basin. **Authored timing target:** 31 seconds / four shots. **Start:** `nar_m02_evt_operation_started`. **Source:** [m02_seven_accounts_of_rain.json](../Content/Narrative/Source/missions/m02_seven_accounts_of_rain.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Grass friction, distant migration movement, wind through scarps; deliberate Kharuun interlocking rhythm. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m02_shot_basin`**

**Authored image:** Open basin under layered strata at first light; shivergrass fields bend in a slow line ahead of unseen footfall while the migration dust rises at the horizon.

**Proposed camera and staging:** 28 mm wide at grass height with the stepped basalt divider in the middle distance. Track slowly sideways; the bending grass advances gently across the near edge while migration dust remains distant.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Seven accounts of this basin, and no two agree where the ground will hold. We walk the route my predecessors argue about.” (`nar_m02_line_oruun_op_orders_001`; command_radio).

**Bindings:** visual `vis_m02_migration_basin`; audio `aud_m02_ambience_route`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m02_shot_accounts`**

**Authored image:** Three divergent route overlays ghost across the same ground, each drawn with a different confidence; none is privileged by the frame.

**Proposed camera and staging:** 40 mm elevated locked frame on the same basin. Layer three thin route drawings one at a time with equal visual weight; do not use a green correct-route color. Hold enough ground texture to compare them.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “The third account says rain cut the western shelf. The fifth says the shelf never existed. Both of them are certain. I have learned to be neither.” (`nar_m02_line_oruun_op_accounts_002`; command_radio).

**Bindings:** visual `vis_m02_account_overlays`; audio `aud_m02_ambience_route`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–25 s · `nar_m02_shot_waystone`**

**Authored image:** The Waystone uproots, strata glowing amber along its base as it lifts; the anchor site waits as a shallow cut in the route ahead.

**Proposed camera and staging:** 50 mm low three-quarter view of Waystone roots and carriage. Keep the rooting ring in frame through lift/settle; if this is only an opening brief, render as a projected demonstration and leave the playable Waystone untouched.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Uproot the Waystone and carry it to the anchor the founding decision left us. The route we inherited decides where we may stand.” (`nar_m02_line_oruun_op_waystone_003`; command_radio).
- **Oruun-of-Seven-Stones:** “The herds move today whether we are ready or not. A migration route is only a route while someone holds it open.” (`nar_m02_line_oruun_op_stakes_004`; command_radio).

**Bindings:** visual `vis_m02_waystone_uproot`; audio `aud_m02_ambience_route`. Use these as source references, not proof that assets exist.

**Shot 04 · 25–31 s · `nar_m02_shot_ridge`**

**Authored image:** A dry ridge in close frame; heat shimmer stands in for the argued river. The column starts to move.

**Proposed camera and staging:** 65 mm ridge detail with distant column silhouettes in soft but readable focus. Hold dry cracked stone sharp and let heat shimmer occupy the background; do not materialize a river.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “My second account insists this ridge is a riverbed. If we drown on dry stone, it will at least settle one family argument.” (`nar_m02_line_oruun_op_humor_005`; command_radio).

**Bindings:** visual `vis_m02_argued_ridge`; audio `aud_m02_ambience_route`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M03 — A City on Reserve, districts dark in sequence

**Place:** Lume Reach service district. **Authored timing target:** 31 seconds / four shots. **Start:** `nar_m03_evt_operation_started`. **Source:** [m03_a_city_on_reserve.json](../Content/Narrative/Source/missions/m03_a_city_on_reserve.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Low reserve machinery, paired conduit hum, ceramic footsteps; measured pulse with missing space. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m03_shot_skyline`**

**Authored image:** An ark-city district skyline at dusk, pale ceramic frames dark against a charcoal sky; scattered status bands blink amber where power should be steady.

**Proposed camera and staging:** 35 mm skyline wide, ceramic structures occupying the lower two thirds. Minimal push; isolate scattered amber alarms from steady cyan systems. Use slow dim changes, not blinking strobe.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Three districts, one reserve, and less margin than the gauges admit. We bring them back in the order we inherited, not the order we would argue for.” (`nar_m03_line_op_grid_001`; command_radio).

**Bindings:** visual `vis_m03_district_skyline`; audio `aud_m03_ambience_reserve`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m03_shot_gridmap`**

**Authored image:** A grid schematic resolves over the districts, three interfaces marked in the inherited order; the sequence draws itself without commentary.

**Proposed camera and staging:** 50 mm oblique overview retaining recognizable district silhouettes. Draw the inherited three-interface order as discrete connecting segments; exact labels are composited from the recorded plan.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “The founding decision fixed the sequence. I have opinions about it. Opinions do not power a district.” (`nar_m03_line_op_order_002`; command_radio).

**Bindings:** visual `vis_m03_grid_schematic`; audio `aud_m03_ambience_reserve`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–24 s · `nar_m03_shot_crews`**

**Authored image:** Worker crews move along a conduit-dense causeway toward the first interface, escort silhouettes holding the flanks.

**Proposed camera and staging:** 40 mm track parallel to the causeway, worker tools and escort profiles separated against dark space. Keep movement away from camera toward the first interface; preview only if not yet authorized.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Workers to the first interface. Escort stays with them. Nothing improvised until the third district holds.” (`nar_m03_line_op_method_003`; command_radio).

**Bindings:** visual `vis_m03_worker_column`; audio `aud_m03_ambience_reserve`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–31 s · `nar_m03_shot_interface`**

**Authored image:** The first district interface in close frame, unpowered, its status band dark; the reserve gauge beside it sits near the bottom of its arc.

**Proposed camera and staging:** 70 mm close view of the interface and low gauge in one focus plane. Hold the dark band dark; do not power the district at the end of the shot.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Every structure out there is load-bearing. Lose one and the plan is no longer a plan.” (`nar_m03_line_op_stakes_004`; command_radio).

**Bindings:** visual `vis_m03_district_interface`; audio `aud_m03_ambience_reserve`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M04 — The Unburied Road, a gap in the record

**Place:** Unburied Road vaults. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m04_evt_operation_started`. **Source:** [m04_the_unburied_road.json](../Content/Narrative/Source/missions/m04_the_unburied_road.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Short cavern reflections, stone resonance, carriage/root friction; low amber-toned motif. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–8 s · `nar_m04_shot_record`**

**Authored image:** A Kharuun record wall in close frame: banded strata of inscribed mineral, one band interrupted by a clean, empty socket where a shard should sit.

**Proposed camera and staging:** 75 mm near-normal view of the mineral wall. Start on an intact inscribed band, track into the clean empty socket, then stop; no floating shard fills it.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Something is missing from the record, and the record knows it. A shard, unburied by nobody, waiting on a road we stopped walking.” (`nar_m04_line_op_road_001`; command_radio).

**Bindings:** visual `vis_m04_record_gap`; audio `aud_m04_ambience_vaults`. Use these as source references, not proof that assets exist.

**Shot 02 · 8–17 s · `nar_m04_shot_roads`**

**Authored image:** Three subsurface roads diverge from a single mouth, each lit differently by its own strata; the frame holds all three without preferring one.

**Proposed camera and staging:** 32 mm symmetric-enough wide to hold all three diverging mouths, but preserve the real asymmetric cavern geometry. Locked camera; none of the exits gets a spotlight.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “The founding decision picked our road for us. My accounts have opinions about all three. Today we only get to test one.” (`nar_m04_line_op_inherit_002`; command_radio).

**Bindings:** visual `vis_m04_three_roads`; audio `aud_m04_ambience_vaults`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–24 s · `nar_m04_shot_waystone`**

**Authored image:** The Waystone travels with the column, roots trailing like a held breath; the roadhead cut waits ahead in amber half-light.

**Proposed camera and staging:** 45 mm low side track parallel to the mobile Waystone. Keep the carriage grounded and the roadhead visible; movement is a source-authored preview unless the ordinary move already occurred.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Waystone first, at the roadhead. A road without an anchor is a rumor with a direction.” (`nar_m04_line_op_method_003`; command_radio).

**Bindings:** visual `vis_m04_waystone_column`; audio `aud_m04_ambience_vaults`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–32 s · `nar_m04_shot_vaults`**

**Authored image:** Grown mineral vaults recede overhead, amber light seeping along the strata; the column is small beneath them.

**Proposed camera and staging:** 28 mm grounded low wide. Tilt a few degrees up along ribs into the vaults without losing the small column at the bottom of frame. End with the road direction readable.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “The vaults under this ground are older than my oldest account, and she claims to remember the sun before it broke. Walk carefully.” (`nar_m04_line_op_vaults_004`; command_radio).

**Bindings:** visual `vis_m04_vault_ceiling`; audio `aud_m04_ambience_vaults`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M05 — Terms of Continuance, a window held open

**Place:** Line of Parity. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m05_evt_operation_started`. **Source:** [m05_terms_of_continuance.json](../Content/Narrative/Source/missions/m05_terms_of_continuance.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Separated relay ticks and Spine responses, settling wind; tentative interlock without a victory resolution. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–8 s · `nar_m05_shot_draft`**

**Authored image:** A treaty draft under instrument light: dense clauses, three variant riders, every signature line empty.

**Proposed camera and staging:** 65 mm overhead-oblique treaty insert. Slow shallow push across clauses toward empty signature areas. All readable text must be source typesetting, not generated fake script.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “The terms are drafted, the ground is chosen, and nobody has signed anything. What we hold today is not a peace. It is a window where one could stand.” (`nar_m05_line_op_terms_001`; command_radio).

**Bindings:** visual `vis_m05_draft_unsigned`; audio `aud_m05_ambience_accord`. Use these as source references, not proof that assets exist.

**Shot 02 · 8–17 s · `nar_m05_shot_networks`**

**Authored image:** Two masts on one horizon, a Meridian relay and a grown Kharuun spine, separated by scarred ground and aligned for the first time.

**Proposed camera and staging:** 40 mm wide along the neutral corridor with relay and Spine separated laterally. Keep different construction heights/forms; no mirrored bases or a connecting ownership flag.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Our relay and their spine have to carry one reading between them. Two networks in step, or no reading at all.” (`nar_m05_line_op_networks_002`; command_radio).

**Bindings:** visual `vis_m05_two_masts`; audio `aud_m05_ambience_accord`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–25 s · `nar_m05_shot_window`**

**Authored image:** A field gone still mid-war: weapons held, dust settling, a countdown lattice hanging over the ground between the masts.

**Proposed camera and staging:** 50 mm locked medium-wide across the intervening ground. Let dust settle naturally while weapons remain held; countdown is authored UI. Use only a paused safe state or marked briefing preview.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “The window opens now and closes on schedule. Between those marks the terms read out, and nothing on this field is allowed to interrupt.” (`nar_m05_line_sync_window_002`; command_radio).

**Bindings:** visual `vis_m05_held_field`; audio `aud_m05_ambience_accord`. Use these as source references, not proof that assets exist.

**Shot 04 · 25–32 s · `nar_m05_shot_witnesses`**

**Authored image:** Two witnesses walking out under separate escort, opposite gaits, carrying the same model of document case.

**Proposed camera and staging:** 55 mm lateral view holding the two witnesses on separate approaches. Match the document-case model but preserve different escorts and gaits; neither crosses to the other side or signs.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Two witnesses, one from each side of a war neither side has stopped fighting. Their work is to watch. Ours is to keep the watchers breathing.” (`nar_m05_line_op_witnesses_004`; command_radio).

**Bindings:** visual `vis_m05_witness_walk`; audio `aud_m05_ambience_accord`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M06 — Names Without Births, an erasure with edges

**Place:** Sector 9 census district. **Authored timing target:** 31 seconds / four shots. **Start:** `nar_m06_evt_operation_started`. **Source:** [m06_names_without_births.json](../Content/Narrative/Source/missions/m06_names_without_births.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Very low ventilation and archival servos; quiet drops around the absence. No horror sting. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m06_shot_register`**

**Authored image:** A Compact record hall in shallow light: ranks of pale-ceramic register stacks, one section conspicuously clean where entries should crowd — a void with edges, not an empty lot.

**Proposed camera and staging:** 50 mm slow aisle track through register stacks toward the clean gap. Keep solid wall edges and empty shelf proportions credible; no ghost faces appear.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “The register lists names with no births behind them. Someone recorded these people. Someone else made sure the recording did not keep.” (`nar_m06_line_op_names_001`; command_radio).

**Bindings:** visual `vis_m06_register_void`; audio `aud_m06_ambience_records`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m06_shot_trace`**

**Authored image:** An archival overlay traces thin cyan lines between census, power link, shelter, and extraction sites; the geometry is exact and unhurried.

**Proposed camera and staging:** 45 mm controlled top-oblique view of the trace diagram on the actual district context. Draw one site-to-site connection at a time and leave the full trace readable at the end.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “We are not here to decide what it means. We are here to find the census, keep it powered, and carry out what remains of it.” (`nar_m06_line_op_method_002`; command_radio).

**Bindings:** visual `vis_m06_trace_overlay`; audio `aud_m06_ambience_records`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–24 s · `nar_m06_shot_residences`**

**Authored image:** Two occupied residences at the trace's edge, warm interior light against charcoal; small ordinary movement inside.

**Proposed camera and staging:** 65 mm static medium view across two lit residences. Indicate occupancy with distant anonymous ordinary silhouettes; avoid close faces, counts or imminent attack.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “If the search exposes anyone living, they come under shelter before any record moves. Records wait better than people do.” (`nar_m06_line_op_care_003`; command_radio).

**Bindings:** visual `vis_m06_exposed_residences`; audio `aud_m06_ambience_records`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–31 s · `nar_m06_shot_erasure`**

**Authored image:** Close on a register surface: names in the original hand, and over them the precise, patient marks of removal.

**Proposed camera and staging:** 85 mm restrained detail of a register surface. Rake warm light across deletion marks and preserve the original marks beneath; no hand, culprit or invented readable name.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “Whoever erased this worked carefully. We will work more carefully.” (`nar_m06_line_op_quiet_004`; command_radio).

**Bindings:** visual `vis_m06_erasure_marks`; audio `aud_m06_ambience_records`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M07 — The Shape of Silence, a border in the accounts

**Place:** Listening-Spine Ridge and memory hollow. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m07_evt_operation_started`. **Source:** [m07_the_shape_of_silence.json](../Content/Narrative/Source/missions/m07_the_shape_of_silence.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Sparse stone response tones with held silence between them; sound never proves an explanation. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m07_shot_hollow`**

**Authored image:** A Kharuun memory hollow at dusk: grown mineral walls dense with inscribed strata, and one span where the inscriptions simply stop — clean stone, no scar.

**Proposed camera and staging:** 40 mm slow lateral track along densely inscribed mineral walls into the clean span. Maintain comparable light on both so absence is the subject, not a shadow trick.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “My accounts remember this district street by street. Then, for one neighborhood, all seven go quiet at once. Seven accounts do not agree on anything. Except, apparently, this.” (`nar_m07_line_op_silence_001`; command_radio).

**Bindings:** visual `vis_m07_quiet_span`; audio `aud_m07_ambience_hollow`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m07_shot_overlay`**

**Authored image:** Two translucent maps drift into alignment: the Compact census void and the Kharuun quiet, their borders meeting without merging.

**Proposed camera and staging:** 50 mm locked overlay comparison. Align the borders slowly, then stop with the two map layers visually distinct; do not dissolve them into one proven explanation.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Talar's erased census and our curated quiet share a border. I do not know what that correspondence means. I intend to stand where I can hear it.” (`nar_m07_line_op_census_002`; command_radio).

**Bindings:** visual `vis_m07_corresponding_voids`; audio `aud_m07_ambience_hollow`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–24 s · `nar_m07_shot_order`**

**Authored image:** The Waystone column moves toward the anchor cut; behind it the Spine segments ride disassembled, in the old processional order.

**Proposed camera and staging:** 45 mm side track at carriage height with the Waystone first and disassembled Spine pieces behind. Keep the anchor cut ahead; no ceremonial crowd.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Waystone to the anchor, then the Spine. We do this in the old order: anchor, listen, witness, walk.” (`nar_m07_line_op_method_003`; command_radio).

**Bindings:** visual `vis_m07_processional_column`; audio `aud_m07_ambience_hollow`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–32 s · `nar_m07_shot_witnesses`**

**Authored image:** Two witness figures stand at marked sites in the middle distance, small against the strata, facing the quiet span rather than each other.

**Proposed camera and staging:** 55 mm steady wide-medium with both witnesses separated and turned toward the quiet. Hold a deliberate empty interval in the center, without a revealing figure emerging.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Two witnesses, at the two places where the accounts thin. Not to speak. To stand where speaking stopped.” (`nar_m07_line_op_witness_004`; command_radio).

**Bindings:** visual `vis_m07_standing_witnesses`; audio `aud_m07_ambience_hollow`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M08 — The Shape Beside Us, ground that exists twice

**Place:** Confluence verge. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m08_evt_operation_started`. **Source:** [m08_the_shape_beside_us.json](../Content/Narrative/Source/missions/m08_the_shape_beside_us.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Survey equipment against low paired glass tones, each contact response separate; no attack sting. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m08_shot_overlap`**

**Authored image:** A stretch of vitrified ground rendered twice in the same frame, offset by a hand's width; both versions cast valid shadows and neither yields.

**Proposed camera and staging:** 40 mm low oblique locked view with a clear rock edge in front. Show a stable hand-width duplication and two consistent shadows; no visual noise, explosive portal or collision-changing terrain.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “The survey teams keep reporting the same thing: ground that reads as two places at once, and movement beside them that matches no roster.” (`nar_m08_line_op_beside_001`; command_radio).

**Bindings:** visual `vis_m08_doubled_ground`; audio `aud_m08_ambience_overlap`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m08_shot_address`**

**Authored image:** A luminous edge repeats along a ridge in near-identical copies, one copy always slightly ahead — an address spoken in geometry.

**Proposed camera and staging:** 65 mm shallow sideways move along repeated ridge edges. Keep successive copies identical in construction, with one leading slightly; sound answers but no attack fires.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “You are reading one page of a document that has two. I can hold the pages apart while you walk. This is an offer. It has been an offer for some time.” (`nar_m08_line_op_offer_002`; cross_faction_radio).

**Bindings:** visual `vis_m08_repeating_edge`; audio `aud_m08_ambience_overlap`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–24 s · `nar_m08_shot_terms`**

**Authored image:** Meridian proxies advance in careful file past the doubled ground; a survey lamp writes both versions into one exposure.

**Proposed camera and staging:** 45 mm side track of the Meridian proxy file and survey lamp. Keep cyan equipment distinct from magenta edge phenomena; do not replace the proxies with playable Choir troops.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “We accept guidance, not custody. Our proxies walk the route; you tell us where the pages turn. Everything gets recorded.” (`nar_m08_line_op_terms_003`; command_radio).

**Bindings:** visual `vis_m08_proxy_file`; audio `aud_m08_ambience_overlap`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–32 s · `nar_m08_shot_echo`**

**Authored image:** The first echo site: an opening that aligns only from this approach, holding a movement that has not happened yet.

**Proposed camera and staging:** 50 mm slow approach along the one alignment axis. Let the opening become coherent only through parallax, then stop short of entering; return the player at the contact approach.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “Recorded. Yes. Record that the echo is not an attack. It is an address. Approach the first one and I will show you the difference.” (`nar_m08_line_op_neme_004`; cross_faction_radio).

**Bindings:** visual `vis_m08_first_echo`; audio `aud_m08_ambience_overlap`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M09 — Reserve Authority, two of three

**Place:** Authority Exchange. **Authored timing target:** 31 seconds / four shots. **Start:** `nar_m09_evt_operation_started`. **Source:** [m09_reserve_authority.json](../Content/Narrative/Source/missions/m09_reserve_authority.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Three quiet district machine layers under one strained reserve hum; no emergency screams. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–8 s · `nar_m09_shot_gauges`**

**Authored image:** A reserve control wall in close frame: three district load gauges, one power curve, and the curve visibly unable to reach all three.

**Proposed camera and staging:** 65 mm near-frontal insert with all three gauges and the shared power curve readable simultaneously. No automatic allocation; keep the third requirement visibly beyond available capacity.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “The reserve carries two districts, not three. That is not a doctrine or an opinion; it is a gauge reading.” (`nar_m09_line_op_reserve_001`; command_radio).

**Bindings:** visual `vis_m09_reserve_gauges`; audio `aud_m09_ambience_reserve_hall`. Use these as source references, not proof that assets exist.

**Shot 02 · 8–16 s · `nar_m09_shot_doctrine`**

**Authored image:** The inherited doctrine renders as a recommendation overlay above the grid map — present, legible, and clearly marked advisory.

**Proposed camera and staging:** 50 mm oblique map insert. Present the inherited recommendation as advisory text/line style, leaving the current choice uncommitted; no ranking by moral color.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “The founding decision recommends an order. Recommends. The allocation is mine to make and mine to answer for, and it will not be reversible.” (`nar_m09_line_op_doctrine_002`; command_radio).

**Bindings:** visual `vis_m09_advisory_overlay`; audio `aud_m09_ambience_reserve_hall`. Use these as source references, not proof that assets exist.

**Shot 03 · 16–23 s · `nar_m09_shot_authority`**

**Authored image:** The authority site: a sealed allocation console under pale-ceramic civic framing, its status band waiting on a single authorization.

**Proposed camera and staging:** 55 mm restrained approach to the intact sealed console, ceramic framing on both sides. End at the uncommitted status band; no signature or lever pull.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “First the authority site. I will not shed a district's load on an ordinary work order. If it is done, it is done under authority, in the record, with my name on it.” (`nar_m09_line_op_authority_003`; command_radio).

**Bindings:** visual `vis_m09_allocation_console`; audio `aud_m09_ambience_reserve_hall`. Use these as source references, not proof that assets exist.

**Shot 04 · 23–31 s · `nar_m09_shot_district`**

**Authored image:** A district street on reserve light, windows dim but inhabited; the frame holds it steadily, without commentary.

**Proposed camera and staging:** 50 mm locked street view at window height from outside. Maintain warm modest occupancy and intact infrastructure under reserve lighting; no deaths, rubble or riot.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Whichever district we defer, someone in it is awake right now assuming the power returns. Keep that in mind and keep working.” (`nar_m09_line_op_weight_004`; command_radio).

**Bindings:** visual `vis_m09_reserve_street`; audio `aud_m09_ambience_reserve_hall`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M10 — The Choir at Lume Reach, a neighbor that answers

**Place:** Deferred district and Lume Well court. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m10_evt_operation_started`. **Source:** [m10_the_choir_at_lume_reach.json](../Content/Narrative/Source/missions/m10_the_choir_at_lume_reach.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Separate civic hum, Spine resonance and local Choir contact tones; no generalized battle bed. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m10_shot_reach`**

**Authored image:** Lume Reach at evening: pale-ceramic civic frames under charcoal ridges, warm interior light in the inhabited quarters, and the Well's glow standing south of the settlement.

**Proposed camera and staging:** 32 mm evening wide of the civic perimeter and distinct Well court. Keep warm inhabited quarters, charcoal ridges and the southern Well in their correct geographical relationship.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Lume Reach. A settlement, a Well, and a neighbor that answers when addressed. Three reasons to walk carefully, and we have business with all three.” (`nar_m10_line_op_reach_001`; command_radio).

**Bindings:** visual `vis_m10_lume_reach_evening`; audio `aud_m10_ambience_reach`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–16 s · `nar_m10_shot_quarantine`**

**Authored image:** A thin Meridian quarantine line holds at a distance, engineered silhouettes deliberately separate from the settlement's life.

**Proposed camera and staging:** 65 mm long view across empty separating ground toward the thin Meridian line. Weapons held, no orders from Mara and no civilians threatened by an invented attack.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Meridian units in your area are quarantine posture, not policy. I am off-map and staying there. What you commit at that Well is committed under Kharuun authority.” (`nar_m10_line_op_liaison_002`; operations_radio).

**Bindings:** visual `vis_m10_quarantine_line`; audio `aud_m10_ambience_reach`. Use these as source references, not proof that assets exist.

**Shot 03 · 16–24 s · `nar_m10_shot_liability`**

**Authored image:** The deferred district's liability site: a dark interface with its status band unlit, the Mission 09 deferral still legible in its stillness.

**Proposed camera and staging:** 60 mm close-medium on the inherited deferred district interface. Include intact framing, unlit band and maintained fittings; load the exact district look from M09.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “We also carry a debt: the district your allocation deferred still has a liability standing. We resolve it before we touch the Well. Order matters.” (`nar_m10_line_op_liability_003`; command_radio).

**Bindings:** visual `vis_m10_standing_liability`; audio `aud_m10_ambience_reach`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–32 s · `nar_m10_shot_well`**

**Authored image:** The Lume Well in close frame, three protocol states shimmering faintly over it in turn; the frame gives none of them precedence.

**Proposed camera and staging:** 55 mm steady rim-level Well view. If previewing protocol choices use equal-duration labeled presentation layers, never actual sequential commitments or a preferred outcome.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “And when the Well offers its three futures, the choice will be ours alone. My accounts are already arguing. Good. That is what they are for.” (`nar_m10_line_op_choice_004`; command_radio).

**Bindings:** visual `vis_m10_lume_well_offers`; audio `aud_m10_ambience_reach`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M11 — No Neutral Ledger, twenty-seven readings and one walk

**Place:** Census Forecourt. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m11_evt_operation_started`. **Source:** [m11_no_neutral_ledger.json](../Content/Narrative/Source/missions/m11_no_neutral_ledger.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Dry public-interface confirmations and exposed cable/mineral junction hum; restrained ledger rhythm. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m11_shot_ledger`**

**Authored image:** A ledger surface fills the frame: ten entries in three hands, each one annotated by both factions; no entry stands unmarked.

**Proposed camera and staging:** 75 mm top-oblique ledger insert. Ten source-bound entries and annotation layers remain readable as records; no secret trust meter or added eleventh completed record.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Ten records in the ledger, and every one of them takes a side. There is no neutral reading left. There is only walking what was written, in the open.” (`nar_m11_line_op_ledger_001`; command_radio).

**Bindings:** visual `vis_m11_ten_records`; audio `aud_m11_ambience_ledger`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m11_shot_lattice`**

**Authored image:** A lattice of twenty-seven plan lines resolves over the district map; twenty-six dim as the recorded combination brightens.

**Proposed camera and staging:** 45 mm district-plan diagram. Show all 27 thin plan traces, then retain the actual recorded combination while the rest dim uniformly. Do not choose the plan through the animation.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Doctrine, districts, protocol: twenty-seven ways those combine, and ours is fixed by what we actually did. My accounts appreciate the honesty of arithmetic.” (`nar_m11_line_op_plans_002`; command_radio).

**Bindings:** visual `vis_m11_plan_lattice`; audio `aud_m11_ambience_ledger`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–25 s · `nar_m11_shot_interfaces`**

**Authored image:** The two neutral evidence interfaces face each other across open ground, Meridian rectangular brackets west, Kharuun paired facets east.

**Proposed camera and staging:** 40 mm wide holding the western Meridian brackets and eastern Kharuun facets with open ground between. Neither interface absorbs or dominates the other.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Two evidence interfaces, two witnesses. I attest one; a second Kharuun witness attests the other. Nobody attests their own claim today.” (`nar_m11_line_op_witness_003`; command_radio).

**Bindings:** visual `vis_m11_paired_interfaces`; audio `aud_m11_ambience_ledger`. Use these as source references, not proof that assets exist.

**Shot 04 · 25–32 s · `nar_m11_shot_well`**

**Authored image:** The Well south of the interfaces, its committed protocol state steady; the frame holds it without ceremony.

**Proposed camera and staging:** 60 mm steady southern Well view. Match its recorded protocol state exactly; do not show a fresh three-way choice or repair a previously spent Well for appearance.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “And at the Well, we apply the Lume protocol as recorded. Not improved. Not corrected. As recorded.” (`nar_m11_line_op_exact_004`; command_radio).

**Bindings:** visual `vis_m11_recorded_well`; audio `aud_m11_ambience_ledger`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M12 — The Future That Won, a demonstration and its audit

**Place:** Demonstrator Spine at Reserve Gate. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m12_evt_operation_started`. **Source:** [m12_the_future_that_won.json](../Content/Narrative/Source/missions/m12_the_future_that_won.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Clean public-address apparatus, two distinct readback tones and controlled plaza ambience. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m12_shot_apparatus`**

**Authored image:** A neutral public demonstrator apparatus dominates a plaza: engineered Meridian framing, its display surfaces cycling verified records with unhurried confidence.

**Proposed camera and staging:** 35 mm measured low approach to the public apparatus with its visible signal paths. Rhyse may appear only as the canon half-scale civic projection above its module, never as a person on the plaza.

**Authored dialogue — preserve speaker and order:**

- **Chancellor Cael Rhyse:** “Citizens: the future is not a question. It is a result. You are watching it demonstrate itself, and the demonstration reads from your own districts' records.” (`nar_m12_line_op_won_001`; public_address).

**Bindings:** visual `vis_m12_demonstrator_plaza`; audio `aud_m12_ambience_plaza`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m12_shot_audit`**

**Authored image:** Oruun's column enters the plaza's edge; two scouts split toward the paired readback interfaces without ceremony.

**Proposed camera and staging:** 50 mm side view of the approaching Kharuun column and split toward two interfaces. Keep the apparatus in context but do not treat its claims as verified results before readback.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “That is the Chancellor's apparatus talking — a public machine with a persuasive voice. What it demonstrates, we will verify independently or not believe.” (`nar_m12_line_op_apparatus_002`; command_radio).

**Bindings:** visual `vis_m12_audit_column`; audio `aud_m12_ambience_plaza`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–24 s · `nar_m12_shot_interfaces`**

**Authored image:** The two neutral readback interfaces in one frame, Meridian brackets and Kharuun facets, both dark and waiting.

**Proposed camera and staging:** 45 mm locked two-interface composition with different fittings and both still waiting. Leave enough separation to read independent observation routes.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “Two readbacks, two of us: I take one neutral interface, the verifier takes the other. Then the inputs, then the protocol, then we hold it stable and watch.” (`nar_m12_line_op_method_003`; command_radio).

**Bindings:** visual `vis_m12_paired_readbacks`; audio `aud_m12_ambience_plaza`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–32 s · `nar_m12_shot_well`**

**Authored image:** The separate Well beyond the plaza, still and unbound; the apparatus's glow does not reach it.

**Proposed camera and staging:** 65 mm Well detail framed beyond a dark strip of plaza. Keep the apparatus light from spilling onto it; no activation until the correct authoritative event.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “The Well accepts what Lume Reach recorded and nothing else. If the demonstration needs a different protocol, then the demonstration is wrong.” (`nar_m12_line_op_exact_004`; command_radio).

**Bindings:** visual `vis_m12_separate_well`; audio `aud_m12_ambience_plaza`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M13 — Assembly of the Missing, witnessed onto the record

**Place:** Crownfall public index. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m13_evt_operation_started`. **Source:** [m13_assembly_of_the_missing.json](../Content/Narrative/Source/missions/m13_assembly_of_the_missing.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Sparse fracture harmonics, measured record-interface tones, quiet between witness observations. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m13_shot_convergence`**

**Authored image:** Three earlier absences render as thin overlays converging on one ground: the census void, the measured quiet, the doubled terrain — edges meeting at the assembly site.

**Proposed camera and staging:** 40 mm top-oblique precinct wide. Three source-derived overlays converge with separate line treatments; preserve the distinction between correspondence and causation.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “The erased census, the curated quiet, the shape beside us — they converge here. The missing are assembling, and nobody's record admits they exist.” (`nar_m13_line_op_missing_001`; command_radio).

**Bindings:** visual `vis_m13_converging_absences`; audio `aud_m13_ambience_assembly`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m13_shot_records`**

**Authored image:** The two public-record interfaces stand lit in the middle distance, Meridian brackets and Kharuun facets both reading, for once, the same event.

**Proposed camera and staging:** 50 mm medium-wide of paired public housings. Keep Meridian and Kharuun hardware visibly different; show no new names or populations on unasserted record faces.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “So we build the admission. Paired readback at both public records, a Spine linked to the Crownfall index, and one observation receipt completed in the open.” (`nar_m13_line_op_receipt_002`; command_radio).

**Bindings:** visual `vis_m13_public_records`; audio `aud_m13_ambience_assembly`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–24 s · `nar_m13_shot_spine`**

**Authored image:** An ordinary worker raises the Listening Spine near the Crownfall index; the index's magenta-fracture glow steadies as the link takes.

**Proposed camera and staging:** 55 mm worker-and-Spine three-quarter view with the index visible beyond. This is an event-gated construction image or explicit briefing visualization, never completed construction silently written at mission start.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “The verifier and I witness from separate sites. One pair of eyes is a claim. Two, apart, on the public record — that is a fact with structure.” (`nar_m13_line_op_witness_003`; command_radio).

**Bindings:** visual `vis_m13_index_link`; audio `aud_m13_ambience_assembly`. Use these as source references, not proof that assets exist.

**Shot 04 · 24–32 s · `nar_m13_shot_witnesses`**

**Authored image:** Two witness sites far apart, each holding one still figure; between them, the assembly ground waits, almost occupied.

**Proposed camera and staging:** 50 mm locked wide with distant separated witness sites. Leave assembly ground nearly empty; never reveal an invented crowd, graveyard or a count of the missing.

**Authored dialogue — preserve speaker and order:**

- **Oruun-of-Seven-Stones:** “We observe. We do not name, number, or claim the missing. The receipt records that an assembly was witnessed. It does not presume to say more.” (`nar_m13_line_op_care_004`; command_radio).

**Bindings:** visual `vis_m13_separate_witnesses`; audio `aud_m13_ambience_assembly`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M14 — Several Voices, One Command, held in disagreement

**Place:** Command-crisis basin. **Authored timing target:** 32 seconds / four shots. **Start:** `nar_m14_evt_operation_started`. **Source:** [m14_several_voices_one_command.json](../Content/Narrative/Source/missions/m14_several_voices_one_command.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Held glass tones and restrained interference beats; separate voices remain intelligible. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–9 s · `nar_m14_shot_choir`**

**Authored image:** A Hollow Choir force stands in formation that is almost a formation: offset duplicates, luminous edges, each unit accompanied by the faint suggestion of where else it might stand.

**Proposed camera and staging:** 40 mm wide of a Choir formation with coherent duplicated silhouettes. Slow sideways movement reveals offsets; do not resolve Possible/Manifest states early for dramatic effect.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “We are several voices. Today we require one command, and we have selected yours through me. This is not surrender. It is an experiment with exact terms.” (`nar_m14_line_op_voices_001`; command_radio).

**Bindings:** visual `vis_m14_choir_formation`; audio `aud_m14_ambience_crisis`. Use these as source references, not proof that assets exist.

**Shot 02 · 9–17 s · `nar_m14_shot_sites`**

**Authored image:** Two inherited sites in one wide frame, far apart; between them the ground carries both their lights without mixing them.

**Proposed camera and staging:** 35 mm wide from a stable point between the two inherited sites. Keep both lights separate and their ground paths visible; do not bridge them with a merging effect.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “One voice will become Possible. One will remain Manifest. The two states do not agree. They will stand in disagreement, in the open, at their own sites.” (`nar_m14_line_op_terms_002`; command_radio).

**Bindings:** visual `vis_m14_separate_sites`; audio `aud_m14_ambience_crisis`. Use these as source references, not proof that assets exist.

**Shot 03 · 17–25 s · `nar_m14_shot_loom`**

**Authored image:** The research loom in close frame: a Choir structure whose geometry contradicts itself deliberately, holding threads of unresolved decisions.

**Proposed camera and staging:** 65 mm close-medium of loom threads and contradictory joints. Minimal push; keep form/research pane legible without showing research already completed.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “The research loom holds what we could not decide alone. Begin with Held Alternatives. The Soldier voice selected this procedure from the alternatives it was offered — I report the selection, and leave the naming of it to your record-keepers.” (`nar_m14_line_op_loom_003`; command_radio).

**Bindings:** visual `vis_m14_research_loom`; audio `aud_m14_ambience_crisis`. Use these as source references, not proof that assets exist.

**Shot 04 · 25–32 s · `nar_m14_shot_crisis`**

**Authored image:** The Crownfall crisis site, magenta-fracture intensity rising along its edges; the anchor's empty socket waits at its center.

**Proposed camera and staging:** 45 mm view toward the empty crisis socket with controlled magenta at its edges. End with the build destination clear but the Phase Anchor absent until built.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “Every placement is exact. Every protection is literal. The contract does not forgive, and we did not write it to.” (`nar_m14_line_op_precision_004`; command_radio).

**Bindings:** visual `vis_m14_crisis_socket`; audio `aud_m14_ambience_crisis`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

### M15 — The Broken Sun, the last contract

**Place:** Solar Fall Dais. **Authored timing target:** 34 seconds / four shots. **Start:** `nar_m15_evt_operation_started`. **Source:** [m15_the_broken_sun.json](../Content/Narrative/Source/missions/m15_the_broken_sun.json).

**Scene instruction:** Build the following four shots in order using the shared Soryn direction. Preserve the exact authored image and spoken meaning; the added camera treatment is a proposal. **Sound bed:** Low fracture harmonics, sparse air movement over the dais and deliberate pauses for witness lines. **Exit:** settle into the actual operation camera and expose the next objective; do not complete any previewed action.

**Shot 01 · 0–10 s · `nar_m15_shot_sky`**

**Authored image:** The Crownfall fills the upper frame: stellar fragments in slow suspension, magenta-fracture light at its brightest of the campaign, the ground below carrying its glow like held breath.

**Proposed camera and staging:** 28 mm low wide, dais foreground and suspended Crownfall above. Slow tilt through the fragments but preserve a fixed ground reference. Brightest campaign magenta remains restrained and unclipped.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “This is the Crownfall. Every record your campaign kept is now load-bearing. We advised exact phrasing from the beginning; today you learn why.” (`nar_m15_line_op_crownfall_001`; command_radio).

**Bindings:** visual `vis_m15_crownfall_sky`; audio `aud_m15_ambience_crownfall`. Use these as source references, not proof that assets exist.

**Shot 02 · 10–19 s · `nar_m15_shot_witnesses`**

**Authored image:** Three witness stations in one wide frame — bracketed Meridian, faceted Kharuun, offset Choir — each holding one still figure, none nearer the center than another.

**Proposed camera and staging:** 40 mm wide with the three authorized witness stations at equal compositional weight. Use registered silhouettes at their actual bindings; no invented hero face close-ups or extra fourth faction station.

**Authored dialogue — preserve speaker and order:**

- **Mara Vey:** “Witness station one, standing. I spent fifteen operations making uncertainty governable. I am here to watch what that bought, not to steer it.” (`nar_m15_line_op_mara_002`; operations_radio).
- **Oruun-of-Seven-Stones:** “Witness station two. My accounts have argued the whole way here, and they have agreed to watch the ending together. All seven. That has never happened.” (`nar_m15_line_op_oruun_003`; cross_faction_radio).

**Bindings:** visual `vis_m15_witness_stations`; audio `aud_m15_ambience_crownfall`. Use these as source references, not proof that assets exist.

**Shot 03 · 19–26 s · `nar_m15_shot_register`**

**Authored image:** The public record interface, open, its surface ready; Talar's station light steady beside it.

**Proposed camera and staging:** 65 mm public-register insert, with Talar’s actual station light beside it. Keep the record open and not yet filled with an ending; avoid a triumphant closing gesture.

**Authored dialogue — preserve speaker and order:**

- **Talar Venn:** “The public record is open and I will keep it faithfully. Whatever is selected, the register will say what was done, by whom, under which earned eligibility. Nothing more, and absolutely nothing less.” (`nar_m15_line_op_talar_004`; operations_radio).

**Bindings:** visual `vis_m15_open_register`; audio `aud_m15_ambience_crownfall`. Use these as source references, not proof that assets exist.

**Shot 04 · 26–34 s · `nar_m15_shot_approach`**

**Authored image:** The Crownfall approach: a path of vitrified ground rising toward the accord sites, the anchor socket waiting at its head under the fractured sky.

**Proposed camera and staging:** 35 mm view along the actual approach toward the accord sites. Small forward movement, then tactical handoff. Anchor socket remains uncommitted; do not show any of the four ending conduits here.

**Authored dialogue — preserve speaker and order:**

- **Neme:** “This is the Crownfall. Every record your campaign kept is now load-bearing. We advised exact phrasing from the beginning; today you learn why.” (`nar_m15_line_op_crownfall_001`; command_radio).

**Bindings:** visual `vis_m15_approach_path`; audio `aud_m15_ambience_crownfall`. Use these as source references, not proof that assets exist.

**Continuity review:** Compare the final cut to the mission source and current state. An instruction spoken in anticipation is not evidence that the depicted action already happened. Keep any preview distinct from the playable state.

**M15 editorial issue:** the source binds the same opening Neme line to shots 01 and 04. It is reproduced here faithfully, but a repeated full take may be unintended. In the proposed animatic, carry or reprise it only as an explicitly marked editorial alternative; do not silently change the registered line binding or call the repeat approved.


### ACT-01 — Necessary Fires

**Placement:** M05→M06. **Proposed duration:** 40 seconds, four 10-second shots. **Basis:** `REL-CIN-003`. **Emotional intention:** the narrow ceasefire has not erased what keeping the evacuation moving cost. No invented casualty montage.

1. **0–10 s — Lume Reach margin.** 35 mm grounded wide of pre-established scarred civic framing, intact distant occupied infrastructure and exposed repairs. Slow lateral move behind a damaged parapet into a clear view. Gold grazing edges, indigo recesses. **Reconciliation of Lume Reach ruins:** The requirement's "ruins" refers to these specific exposed repairs and damaged parapets, not a new level of unmodeled destruction. The city remains visibly functioning.
2. **10–20 s — The working cost.** 65 mm detail across worn archive transfer fittings, patched ceramic and route-use marks. Keep any carrier/record visible only if the saved campaign supports it. No body, casualty name or newly destroyed asset. Sound moves from wind to a small servo/settling transient.
3. **20–30 s — Two accounts.** 50 mm stable view of an attributable operational record surface with space for Mara and Talar's exchange. Camera does not show them meeting physically. Draw only source-bound facts; unapproved text is absent from the animatic.
4. **30–40 s — Toward the census.** 45 mm cut to ceramic register architecture that visually leads into M06. Allow the archive ambience to take over before picture changes; end on the M06 launch/briefing boundary, not inside an automatically completed investigation.

**Voice:** Mara and Talar required; act-interlude script follows.
**Authored dialogue — preserve speaker and order:**
- **Mara Vey:** "We saved the archive at the cost of the reserve. The ceasefire holds, but the quiet is measured in empty cisterns."
- **Talar Venn:** "The ledger records what we brought out. It cannot record what is still waiting when that time runs out."

**Music:** Act I resolution, restrained rather than triumphant. **Do not:** reveal Choir authorship, show Understone, or claim a recorded evacuation proves wider civilian outcomes.

### ACT-02 — The Cost of One Future

**Placement:** M10→M11. **Proposed duration:** 40 seconds, four 10-second shots. **Basis:** `REL-CIN-004`. A reflective vault interlude, not an invented new underground mission.

1. **0–10 s.** 28 mm low, grounded wide beneath the Unburied Road’s asymmetric mineral vault ribs. A narrow real span leads through frame; deep side voids remain quiet. Slow forward movement ends before the next threshold. Amber cavity light, short stone reflections.
2. **10–20 s.** 70 mm lateral detail along a maintained docking hollow, worn root fixtures and banded record mineral. Use evidence of repeated passage, not tombs or ritual decoration. Let Oruun's future voice slot have space without competing sound.
3. **20–30 s.** 50 mm view of recorded account imagery held as an attributable presentation layer against the vault architecture. Do not stage a literal flashback of the first breaking or show its perpetrator. Keep competing accounts distinguishable.
4. **30–40 s.** Match the line of a grown rib to an exposed adapter at the Census Forecourt, introducing M11's two public construction languages. A low fracture harmonic joins the stone resonance, then drops beneath the briefing.

**Voice:** Oruun required; interlude script follows. The delivery reflects ancestral memory as an account, not omniscient proof.
**Authored dialogue — preserve speaker and order:**
- **Oruun-of-Seven-Stones:** "My oldest accounts agree on nothing but the weight of the past. If the future demands we walk this road, we must bear the cost of leaving our history buried where it fell."

**Music:** Act II theme resolves into a held unfinished interval. **Continuity:** do not imply that the M10 force physically traveled through these vaults unless an approved transition establishes that journey.

### ACT-03 — Crownfall

**Placement:** before M15; distinct from M15’s operational briefing. **Proposed duration:** 45 seconds. **Basis:** `REL-CIN-005`. Final voice composition needs the Rhyse-scope reconciliation already identified above.

1. **0–12 s.** 28 mm low view from the black-glass approach. Dais edges in foreground, fragment arc above. A controlled upward tilt reveals denser magenta at the nearest fragments, no new astronomical object.
2. **12–23 s.** 50 mm measured lateral view of fixed ground-reference structures as offset edges become more apparent. “Sky tearing” is expressed through local contradictory outlines and the fragment field, not a monster portal, exploding planet or visual strobe.
3. **23–34 s.** 60 mm view of Choir architectural edges holding several valid alignments. Keep the structures coherent. Provide a clear slot for Neme’s precise off-screen voice. Do not portray one true personality conquering the others.
4. **34–45 s.** 35 mm return to the known approach and distant accord geography. The motion settles; cut into M15's witness/operational framing without replaying another identical slow sky reveal.

**Sound:** sparse fracture harmonics becoming denser, then enough quiet to hear the final line and upcoming briefing.

**Reconciliation of Rhyse scope:** To satisfy both `REL-CIN-005` (which names him) and the character bible (which restricts him to M12 public apparatus), Rhyse is heard solely as a recovered audio transmission echoing from an abandoned demonstrator terminal in the environment, with no physical cameo or live projection.

**Authored dialogue — preserve speaker and order:**
- **Cael Rhyse (recorded audio):** "If the ledger cannot hold the cost, the structure will break. But we built this structure to endure."
- **Neme:** "The structure is already broken. We are merely deciding who inherits the glass."

**Do not:** select an ending, show an already successful conduit, injure protected witnesses or imply the game simulation changed during the scene.

### Ending production direction

These four proposed 48-second edits use the conduits and local light/sound described in the expanded canon. Each plays only after its matching successful M15 resolution. The visual descriptions of a hold are used as continuity references, not permission to replay or reset that gameplay hold. No montage of unmodeled downstream societies. Show only the selected ending, never all four in sequence.

The older cinematic requirement summarizes Restoration as rebuilding the old sun; the expanded canon supplies a narrower visible treatment of a returning possibility and cooling halos. Do not invent a fully restored stellar disc to bridge that difference. Use the canon's local imagery for the proposed animatic and flag any broader requested reconstruction for reconciliation. Existing M15 choice lines describe what the player is about to do; they are not automatically valid post-success ending narration. Therefore the edits below have music/ambience and an unrecorded final-script slot, not incorrectly reused pre-choice dialogue.

### END-01 — Restoration

**Meaning:** return has cost and uncertainty; no triumphant universal cure. **Look:** northern convergence; tall open lattice braiding Compact ceramic, Kharuun strata and Choir geometry around a cyan-held core.

1. **0–12 s:** 35 mm grounded wide of the actual successful northern conduit and witnessed accord geography. Begin with its already established state, not construction from nothing. Quiet forward dolly makes the lattice silhouette legible.
2. **12–24 s:** 65 mm detail tracks up the three material languages, each visibly distinct at the junctions. Pale-tide mist gathers at the base and flows gently outward; no resurrected figures emerge.
3. **24–36 s:** 40 mm low angle retaining the conduit edge as scale. The nearest fragment halos cool toward cyan as the harmonic lowers and slows. Preserve the ragged core/fragment field; no invented full sun, green planet or restored city.
4. **36–48 s:** 50 mm return to the local ground and protected witness arrangement, calm but unresolved. End on the conduit with room for a separately typeset “Restoration” result label. Fade into the real recorded result.

**Music/sound:** Compact pulse and Kharuun interlock settle into one cadence; restrained air and mist motion remain audible. 
**Authored dialogue — preserve speaker and order:**
- **Mara Vey:** "The lattice holds. The light returns. We paid everything to rebuild the old sun, but it is not our sun anymore. It is just the first piece of a new ledger."

**Reject:** crowd applause, moral gold glow, returned named civilians or a claim all deferred districts were restored.

### END-02 — Controlled Stabilization

**Meaning:** stability as a managed wound. **Look:** central convergence; squat heavy ceramic-and-strata clamp around a steady charcoal core, cyan/amber bands, no offset.

1. **0–12 s:** 40 mm wide-medium of the completed central conduit, visually heavier and lower than the Restoration lattice. Slow lateral reveal demonstrates clamped load paths.
2. **12–24 s:** 70 mm detail of ceramic clamps meeting grown strata, steady bands and perfectly registered joints. Nothing unfolds or grows. Camera comes to rest.
3. **24–36 s:** 35 mm low view across the same fixture to the sky. The Crownfall remains as it was: **no healing and no collapse**. Hold the composition long enough that continued stability is the event.
4. **36–48 s:** 50 mm steady witness/conduit composition. Keep the setting intact but emotionally unresolved; final label “Controlled Stabilization” is a separate overlay leading to the recorded result.

**Music/sound:** measured Compact pulse alone over one steady harmonic, unresolved ending cadence. 
**Authored dialogue — preserve speaker and order:**
- **Talar Venn:** "We clamped the wound shut. We stabilized the bleeding. The sky will not heal, but it will not fall. The record shows that we chose survival over hope."

**Reject:** authoritarian parade, cheering citizens, total cosmic stasis, an extinguished sky or an implied universal peace. No new downstream social imagery.

### END-03 — Extinguishment

**Meaning:** finality, including what can no longer answer. **Look:** western convergence; a single amber spire, with Choir offset panes around its base.

1. **0–12 s:** 35 mm grounded wide of the successful western spire. Preserve the actual approach and protected witnesses; amber ascends in deliberate bands rather than an explosive beam.
2. **12–24 s:** 60 mm side detail of the base panes. Their offsets register one by one; geometry remains stable until its authored visual change. No people are erased on camera as an invented casualty statement.
3. **24–36 s:** 40 mm low view of the fragment field. Amber reaches the visible fragments and the thin magenta halos go out sequentially. Do not make the entire world black; retain enough indirect light for the game/result transition and avoid implying the remaining core physically vanished without authority.
4. **36–48 s:** 50 mm nearly locked local view. At the authored final cessation, the fracture harmonic drops to silence. Hold the quiet composition, then add “Extinguishment” separately and transition to the result. No horror sting after the silence.

**Music/sound:** rising controlled harmonic into a hard final drop; fracture theme ends. 
**Authored dialogue — preserve speaker and order:**
- **Oruun-of-Seven-Stones:** "The embers go cold. The sky is finally silent. We refused the cost of a false future, and chose instead to close the book. The past is safe in the dark."

**Reject:** planetary explosion, falling dead bodies, an enemy-victory frame or an invented claim that all life ended.

### END-04 — Open Evolution

**Meaning:** release with no guarantee. **Look:** southwestern convergence; repeated glass geometry, multiplying afterimages, unfinished top, magenta-fracture edges.

1. **0–12 s:** 35 mm grounded approach revealing the successful southwestern conduit. It deliberately lacks a completed cap; do not “repair” it into a symmetric finished tower.
2. **12–24 s:** 60 mm lateral detail where additional coherent afterimages appear rather than registering. Keep each edge stable and legible, with slow motion and no random artifact flicker.
3. **24–36 s:** 40 mm view from the dais into the halo field. Halos spread and brighten within exposure limits; doubled outlines become visible on the dais itself. Preserve all protected actors and existing terrain contacts.
4. **36–48 s:** 50 mm steady local composition holding more than one valid outline. The music leaves more than one possible resolution audible. Add “Open Evolution” separately and transition to the recorded result while the held image remains open.

**Music/sound:** layered Choir harmony resolving in different directions without one final dominating note. 
**Authored dialogue — preserve speaker and order:**
- **Neme:** "The alignment is incomplete. The possibilities remain open. We did not choose one truth, so we must live with all of them. The contract is unwritten."

**Reject:** paradise montage, collapse into a single Choir personality, a demon reveal, new species or an asserted safe future.

### HANDOFF-01 — Prologue into the playable readiness check

**Proposed 8-second connective treatment; may be FD-02's final shot, not an extra video.** Start with the exact last prologue view of the readiness area. Over 0–5 s, ease to the real tutorial camera and keep the starting Anchor/unit context centered and unobstructed. Over 5–8 s, settle completely, restore player input and introduce the active survey lesson. Mara's authored line at the lesson trigger: **“Before anything moves, we see the ground. Sweep the basin.”** Keep it in the live tutorial audio/caption system; it may continue beyond the camera move. Ambient wind and network hum never disappear. The player, not the clip, performs the pan/zoom/recenter exercise. If the shot already appears in FD-02, reuse it once.

### HANDOFF-02 — Readiness complete into deployment

**Proposed 12-second connective treatment.** Use only after the current readiness gates are satisfied. For 0–4 s hold a clean tactical view of the achieved training state. For 4–9 s provide the verified deployment destination/context in the real briefing layer; avoid a travel montage that invents distance or a new map. For 9–12 s begin the authorized transition into the target operation or hold at its deployment confirmation, according to the active journey flow. Use Mara's close from the tutorial source only when its enumerated skills still match the current curriculum; its text is **“Readiness check complete — survey, roster, routes, reserve, links, foundry, perimeter, board, and one Well you'll be thinking about tonight. The window opens now. This one is real.”** The spoken take will likely exceed the proposed camera edit: let the camera hold and extend timing naturally. Do not mark an incomplete lesson passed. Do not use the Glass Scar skirmish engagement frame as if it were M01's campaign contract.

### Mission-critical cinematics and outcomes — reusable production descriptions

These are fully produced professional-level cinematic templates for major mid-mission events and completions. Before assigning one, identify the exact mission, line IDs, state condition and registered site. An external AI without that state should create an unbound concept cinematic, not fabricate the record.

**Discovery cinematic (10–15 s):** A fully produced scene starting from the player's current view, cutting to a professional cinematic sequence detailing the actual discovered record or interface. Use dynamic camera work that ends with the evidence readable. Play its existing triggered line. Show what was observed without cutting to a culprit or unsupported historical flashback.

**Well commitment cinematic (15–20 s):** A fully produced cinematic framing the bowl, spire and the relevant ground feature. Harvest uses rising amber followed by permanent dark collapse; Preserve retains a stable spire and slow custody indication; Reshape manifests only the registered temporary feature with readable expiry indication. These are three mutually exclusive branch versions. 

**Witness/readback cinematic (10–15 s):** A fully produced cinematic holding both independent sites, preserving their different hardware and separation. Present the receipt cue only after the authoritative observation. No merging of cultures, consent animation or population-recovery image. Readable text comes from the live record.

**Mission outcome cinematic (15–20 s):** A fully produced storytelling cinematic of the actual surviving scene with the matching authored line and cue. Success does not rebuild destroyed objects; failure does not invent a character death. This replaces simple UI dialogue panels to ensure professional-level storytelling at the conclusion of every major mission.

### Promotional-video editing briefs

Give the editing AI approved in-engine footage matching each beat and the shared identity direction. It may cut, mix and typeset; it must not generate nonexistent gameplay. The following timings are proposed picture edits. Any clip not available becomes a listed capture request, not a synthetic replacement. No dates, store/platform claims, awards, reviews or “available now” unless verified for the intended publication.

**VID-01 — Reveal / world teaser, 40 s.** 0–8 s: the FD-01-style in-engine sky/landscape establishes the core and broken arc, with low wind. 8–16 s: close environmental footage of Dawnshard material followed by the Well bowl/spire; no loot treatment. 16–25 s: three roughly three-second faction silhouettes, Compact → Kharuun → Choir, each preserving its own sound texture. 25–34 s: actual tactical footage approaching a Well, cutting before the irreversible outcome; show strategic scale rather than a fabricated duel. 34–40 s: clean game-title card over a quiet captured vista, final unresolved harmonic. No narrator necessary. Keep ending spoilers out.

**VID-02 — Gameplay / store trailer, 80 s.** 0–8 s: readable live battlefield with cursor selection and ordinary orders. 8–20 s: worker gathers, delivers, and resources update; let one real action complete. 20–32 s: placement preview, completed construction and production/rally, showing cause before result. 32–48 s: formation/composition and a fair visible engagement, tactical perspective retained across most shots. 48–64 s: three separately labeled captured Well branches, with cuts clearly indicating alternatives; never imply one Well can perform all three. 64–73 s: mission objective/branch consequence in the real interface, without revealing final endings. 73–80 s: game title and only an approved destination call-to-action. Mix game feedback prominently; avoid a generic cinematic-only montage that hides how the game plays.

**VID-03 — Meridian Compact spotlight, 54 s.** 0–9 s: ceramic/charcoal service network and powered cyan bands, 40 mm in-engine capture. 9–20 s: Surveyor's tools, Matter intake and real delivery loop. 20–32 s: Array Foundry output into a rally, with actual production state. 32–45 s: Lancer plants and fires, then Bulwark's directional protection, using implemented actions. 45–50 s: readable overhead shot connecting infrastructure and force. 50–54 s: faction name and game title. Use dry mechanical impacts and a measured pulse; no invented hero superweapon.

**VID-04 — Kharuun Assemblies spotlight, 54 s.** 0–9 s: banded maintained architecture, amber nodules and polished root fixtures. 9–23 s: real Waystone uproot/move/re-root; preserve chronological cause and state. 23–34 s: workers and Growth Basin/Listening Spine functions available in the capture build. 34–46 s: a readable engagement with Kharuun silhouettes and material effects. 46–50 s: route and rooted infrastructure in one tactical view. 50–54 s: faction name and game title. Stone resonance and interlocking rhythm, no drums or imagery implying primitive tribes.

**VID-05 — Hollow Choir spotlight, 54 s.** 0–9 s: stable offset glass geometry, slow in-engine camera. 9–21 s: Threadkeeper's gathering/building and coherent structure formation as implemented. 21–35 s: separate captured Possible/Manifest state examples, showing transition telegraph and resolved silhouette without claiming both benefits simultaneously. 35–46 s: real Warden control/Phase Anchor function if verified in the build, otherwise list that missing footage. 46–50 s: readable wider tactical composition. 50–54 s: faction name and game title. Sound is intelligible layered harmonics; no horror choir or fake shapeshifting.

**VID-06 — Future Wells feature video, 60 s.** 0–8 s: one neutral dormant Well with a worker approaching. 8–22 s: clearly labeled Harvest capture, retaining public warning and visible final loss; use an explicit edit if compressing wait time. 22–36 s: fresh independent Preserve capture, stable custody and actual ongoing return. 36–50 s: fresh independent Reshape capture, real temporary feature and an explicit cut to expiry/warning if needed. 50–56 s: three matched still/short captured branch views side by side, equal area and neutral labels; never good/bad colors. 56–60 s: game title. Overlay only costs/durations actually verified in the captured build. Do not copy potentially stale numbers from descriptive canon into player-facing footage.

**VID-07 — Launch trailer, 80 s.** 0–10 s: strong qualified world vista. 10–25 s: economy/build/order causal sequence from actual play. 25–43 s: faction contrast through readable combat clips, avoiding unsupported army scale. 43–57 s: non-spoiler campaign environment/briefing imagery with short approved dialogue excerpts whose contexts remain true. 57–70 s: Well decision and a tactical consequence, then one restrained Crownfall sky shot without showing an ending. 70–80 s: title and verified launch/store information only. Build one musical progression from atmosphere to conflict to an unresolved close; preserve gameplay effects and dialogue intelligibility. All clips must retain build/mission provenance in the delivery manifest.

### Final checks for the receiving AI

Before handing back any item, compare each shot to its brief: correct place and branch, stable faction forms, exact spoken text, no invented physical character, and no premature gameplay result. Review every transition for geometry/lighting continuity. Verify the final duration against the actual voice take; report changed timings. Inspect subtitles, reduced-motion/flashing alternatives and clean picture. Identify any unresolved source conflict by item/shot rather than hiding it in a generic disclaimer. Return the actual artifacts and state whether they are a concept video, an editable Unreal sequence, captured gameplay media or a tested in-game scene.

Detailed direction also uses [ArtDirection.md](ArtDirection.md) and the expanded sky, faction, place and ending descriptions in [DevelopmentBible.md](Archive/DevelopmentBible.md). This expansion preserves the single authoritative document and authorship; source storyboards and requirements have not been edited.

**2026-09-10 link maintenance:** the EPUB link now resolves under Desktop/Writing/Books. This repairs a missing local reference; the September 7 comparison was not repeated and no book content or authority was changed.
