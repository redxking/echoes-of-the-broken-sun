# Map technical blueprint — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Maintained:** 2026-09-04
**Standing:** implementation reference under [AGENTS.md](../AGENTS.md) and the [authority map](README.md).

[Requirements.md](Requirements.md) owns behavior and numeric criteria. [MapConcepts.md](MapConcepts.md)
owns the subordinate production brief; [DevelopmentBible.md](Archive/DevelopmentBible.md) supplies creative
canon. [RequirementsState.md](RequirementsState.md) owns evidence, open defects and acceptance. This file
explains how to carry that design into source and qualification; it is not another requirements ledger.

The earlier blueprint's invented mission coordinates, blocked-cell totals, uniform grids, unregistered
profile/asset names, lighting values and six-skirmish-map list are retired. Do not recover those values as
implementation instructions from Git history. Preserve existing authoritative mission sites and outcomes
unless an explicit contract migration changes them.

## Campaign map contracts

Deliver fifteen unique campaign maps under `SPEC-MAP-004`, separately from the three offline skirmish maps in
`SPEC-SKM-003` and `SPEC-SKM-011..013`. The owner-approved Conquest and team/FFA expansion adds its own
sector/seed and map-format/spawn contracts under `REL-CAM-033..038` and `SPEC-SKM-014..018`; two-spawn
map parity is not multiplayer qualification. Available art/biome families supply reusable materials and forms; they are not a count of maps.
A mission title, a palette change or a different objective overlay on an unchanged route graph does not
establish the distinct authored battlefield required by the master.

Each mission binding must identify its mission ID, registered map/source ID, schema/compiler revision,
source digest, compiled output/digest, runtime selection and mission sites. Store executable bindings in
registered source/compiled contracts, not in this prose. The map contract includes grid/scale, starts,
terrain, passability, resources, Well(s), objective and branch sites, camera bounds, Reshape/fallback data
and deterministic identity required by `SPEC-MAP-002` and `SPEC-MAP-004`. Reuse of a kit or compiler is fine;
reuse must not erase the distinct layout or falsely imply a shared campaign/skirmish contract.

| Mission | Command and spatial obligation | Controlling plan / mission contract |
|---|---|---|
| M01 — What the Ledger Keeps | Mara/Meridian; distinct recovery, Well commitment and withdrawal route; surviving scout is archive carrier. | `SPEC-PLAN-001` / `SPEC-MSN-001` |
| M02 — Seven Accounts of Rain | Oruun/Kharuun; migration, account observation and Waystone sites with legible independent purposes. | `SPEC-PLAN-002` / `SPEC-MSN-002` |
| M03 — A City on Reserve | Mara/Meridian; three distributed reserve-grid sites and the connecting defense/economy routes. | `SPEC-PLAN-003` / `SPEC-MSN-003` |
| M04 — The Unburied Road | Oruun/Kharuun; moving infrastructure, roadhead and memory-shard recovery through a distinct transit route. | `SPEC-PLAN-004` / `SPEC-MSN-004` |
| M05 — Terms of Continuance | Meridian command with separately represented Kharuun presence; treaty relay and protected witness sites. No mixed-faction command inferred from allied presence. | `SPEC-PLAN-005` / `SPEC-MSN-005` |
| M06 — Names Without Births | Talar/Meridian; census evidence, archive power, two bounded civilian proxies and extraction in a civic district whose omissions remain unexplained. | `SPEC-PLAN-006` / `SPEC-MSN-006` |
| M07 — The Shape of Silence | Oruun/Kharuun and distinct witness; separated observations, Waystone, Listening-Spine and confluence. | `SPEC-PLAN-007` / `SPEC-MSN-007` |
| M08 — The Shape Beside Us | Talar/Meridian proxies; Neme-guided reciprocal contact through the selected overlap geometry, with no Choir command or base. | `SPEC-PLAN-008` / `SPEC-MSN-008` |
| M09 — Reserve Authority | Mara/Meridian; three district interfaces, exactly two powered and one explicitly deferred intact. | `SPEC-PLAN-009` / `SPEC-MSN-009` |
| M10 — The Choir at Lume Reach | Oruun/Kharuun; inherited approach and liability, two sequential Spines, new Lume Well choice and matching resolution site. Mara is an off-map liaison; Choir is nonplayable here. | `SPEC-PLAN-010` / `SPEC-MSN-010` |
| M11 — No Neutral Ledger | Oruun and distinct Kharuun witness; exact inherited route and powered pair, two public interfaces and protocol rally. | `SPEC-PLAN-011` / `SPEC-MSN-011` |
| M12 — The Future That Won | Oruun and verifier; independent readback, inherited district-link pair, Well and exact contracted activation hold. Rhyse is attributable neutral apparatus. | `SPEC-PLAN-012` / `SPEC-MSN-012` |
| M13 — Assembly of the Missing | Oruun and verifier; two record interfaces, index linkage and independent witness sites. | `SPEC-PLAN-013` / `SPEC-MSN-013` |
| M14 — Several Voices, One Command | Neme/Hollow Choir; distinct Possible/Manifest sites, command position and crisis anchor with irreversible failure semantics. | `SPEC-PLAN-014` / `SPEC-MSN-014` |
| M15 — The Broken Sun | Neme/Hollow Choir; approach anchor, three protected neutral witnesses and their accord sites, selected earned conduit and hold. | `SPEC-PLAN-015` / `SPEC-MSN-015` |

The exact coordinate sets, branch alternatives, initial roster and failure causes live in the detailed
mission contracts and corresponding registered source. Validate **every branch**, not just the currently
selected route. A layout must support initial-entity placement, worker/build footprints, protected witnesses,
objective access, Well telegraphs and Reshape fallback without silently moving a mission anchor.

Campaign layouts may be intentionally asymmetric to serve their mission. The mirrored-start fairness
criterion in `SPEC-MAP-001` applies to the standard competitive skirmish maps; it does not turn all fifteen
campaign missions into mirrored skirmish arenas. Campaign access, validity and difficulty remain governed
by their own contracts. Neither visual scale nor tile count alone proves suitable battle scale: qualify
travel and readability at the actual camera, movement rules and encounter pacing.

## Source, runtime and presentation boundaries

Read the selected schema and compiler before editing. Existing entry points include
`Content/World/Tools/compile_map_pack.py`, `compile_overlay_pack.py`, `compile_dressing_pack.py`, and
`emit_compiled_map_pack_header.py`; obtain supported arguments with `--help`. The existing base schema is
`Content/World/Schema/glass_scar_map_source_v2.schema.json`. A new registered map family may require a
schema/compiler extension with explicit compatibility and invalid-data checks. Never hand-edit generated
packs/headers to manufacture a binding, bypass validation or substitute a generic map on a failed lookup.

Simulation owns terrain, movement, placement, line of sight, objective state, save and replay identity.
Presentation consumes that state. Cosmetic geometry has no collision, overlap, navigation influence,
input interception or gameplay authority. Follow the existing rendering contract for shadows and fog;
a decoration must not disclose hidden forces or cover a public protocol warning. Gameplay geometry must
be represented in authoritative data and qualified as such, not smuggled in as dressing.

Grid dimensions, origin, array layout, tile scale and render scale are read from their actual contracts
and named runtime constants. Inspect conversion in both directions and map-edge behavior. Lighting,
materials and audio use the applicable master gates and art/audio direction; this blueprint creates no
independent lux, loudness, movement-cost or performance thresholds.

Before comparing maps visually, compare source layouts and mission bindings. The comparison must show
meaningful differences in routes, starts, objective staging, landmarks, sightlines and strategic choices,
then establish those differences in packaged gameplay views. Verify legitimate shared regional vocabulary
without copying the battlefield. Invalid IDs, mismatched source/compiled identities or missing mission
bindings must follow `SPEC-MAP-004.REFUSAL`; no success receipt may conceal a fallback.

## M01 end-to-end qualification

M01 is the representative slice because it combines story orientation, normal RTS controls/economy/combat,
a protected carrier, three irreversible-choice paths, evacuation, results and persistence. Qualifying it
establishes a repeatable production method; it cannot qualify M02–M15 or the full campaign by extrapolation.
Use the detailed [M01 production brief](MapConcepts.md#m01-representative-production-brief).
The owner's sequential production direction is implemented by the
[bounded M01 build packets](MapConcepts.md#bounded-m01-build-packets-and-stop-conditions).
Finish the current map's plan and packets before activating another map. This document owns the
qualification sequence; MapConcepts owns the single detailed production plan.

The source trace inspected on 2026-09-04 is:

- Narrative: `Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json`, content
  `nar_m01_what_the_ledger_keeps`, mission `WhatTheLedgerKeeps`, operation `CampaignPrologue`.
- World input (refreshed 2026-09-05): `Content/World/Source/Campaign/m01_glass-scar-evacuation-margin_v1.json`
  supplies the dedicated campaign terrain through `EchoesCampaignTerrainBinding` and the generated
  campaign pack. `Content/World/Source/Presentation/m01_evacuation_landmarks_v1.json` supplies the
  source-hash-bound M01 presentation. The GlassScar source/compiled pack remains a separate shared/skirmish
  reference; it is not the dedicated M01 map authority.
- Mission/runtime: `EchoesPrologueMissionModel`, `EchoesSimulationSubsystem` scenario dispatch,
  `GetArchiveRecoverySite` (22,18), `GetEvacuationSite` (6,17), and the actual Well/terrain binding.
- Continuation: `EchoesCampaignProgress` atomic store, result handling in `EchoesPlayerController`,
  campaign-map presentation and the M01-to-M02 transition.

Those are source observations, not proof of a dedicated M01 map or complete narrative delivery. Recheck
at execution because the runtime/map registry is under active development. The narrative source inspected
here explicitly marks runtime consumption false and several deliveries unbound. Authored text, a working
mechanical objective, or an art-review capture does not establish voice/subtitle/cinematic delivery.

| Stage | Required execution and retained result | Evidence boundary |
|---|---|---|
| Design and binding | Review M01 story/place/unit/building briefs; bind mission to its unique registered source/compiled map; verify every fixed/branch site and initial entity against source and runtime. Record source hashes and generated identities. | `SRC`; planning and executable-source checks only. |
| Focused regression | Run applicable narrative, map/compiler/overlay and native mission tests, then the coordinated Unreal mission, compiled-binding, narrative delivery, dressing, fog and save/progress cases. Include absent/mismatched-map refusal and illegal/unreachable sites. | Native/editor evidence stays native/editor; `PKG-AUTO` requires the identified package. |
| Ordinary entry | Start the identified package from title/new campaign with an isolated fresh player ledger; observe opening, teaching, briefing, loading and deployment. No hidden startup shortcut can stand in for this path. | `PKG-PHYS` for actual mouse/keyboard journey; capture input/state trail and rendered/audio context. |
| Three Well branches | Separately recover carrier at22,18; keep it alive/on site while a worker commits each of Harvest, Preserve and Reshape; observe choice warnings and consequences; evacuate surviving carrier to6,17 after commitment. Compare branch presentation and exact ledger receipt. | `PKG-AUTO` for deterministic cases; separately `PKG-PHYS`/`PKG-REND` for interaction, visibility, animation and sound. |
| Failure and recovery | Exercise carrier/Core loss, interrupted or invalid commitment, ownership/terminal-outcome failure, retry/checkpoint and restart. No failure writes a campaign consequence. Verify Added, AlreadyRecorded, ReplayConflict and StorageFailure behavior through the appropriate executable fault cases, with truthful visible results. | Fault injection/automation stays `PKG-AUTO`; normal player retry/restart needs `PKG-PHYS`. Never inject a save and call it a fresh journey. |
| Continuation and durability | Commit success, continue to M02, exit/relaunch/continue and replay M01. Preserve the established ledger, inherited record and any earned optional reward, and show the connected campaign transition without revealing unearned story. | `PKG-AUTO` state assertions and separately `PKG-PHYS`/`PKG-REND` ordinary flow. |
| Integrated craft | Review tactical/close gameplay views, roles, collision truth, all Well states, fog, accessibility, motion, dialogue/alerts over combat, and material sound. Use real gameplay with units and fog active. Art-review modes that hide them cannot substitute. | `PKG-REND` including listening and synchronization; measurements use the master's hardware/workload/preset gates. |
| Experience and acceptance | Have the required uncoached participants explain the stakes, roles and next action, then play; record observed comprehension, difficulty and emotional/story continuity. Submit exact build/evidence and unresolved defects to Angelis. | `HUM` then `OWNER`; model-operated play is neither. |

Run the lightweight source checks from the checkout, retaining command, exit code and complete output:

```sh
python3 Tests/Narrative/test_m01_narrative_contract.py
python3 Tests/World/test_glass_scar_map_pack.py
python3 Tests/World/test_glass_scar_compiled_map.py
python3 Tests/World/test_overlay_map_packs.py
```

Before Unreal/build/package/capture work, read the applicable heavy-run and package-provenance skills and
coordinate exclusive resources. The existing Unreal harness is `Scripts/run_unreal_tests.sh`; inspect its
current supported selection and isolated-save behavior. Useful existing tests include
`EchoesPrologueMissionTest`, `EchoesFreshCampaignJourneyTest`, `EchoesCompiledMapBindingTest`,
`EchoesCampaignOperationsMapTest` and save/progress cases. Their names here are discovery pointers, not
claims of coverage or a frozen test count.

An editor `-EchoesCampaignPrologue` launch is useful diagnostic evidence but bypasses normal campaign entry.
A packaged run needs its own verified source/build/save identity. Record commit plus dirty-source hashes,
compiler/schema version, package hash, configuration, command/input sequence, logs/video/audio and every
failure at the existing evidence location. Update only the exact evidence classes in RequirementsState.md.
Do not promote an old screenshot or an editor PASS to packaged qualification.

## Extend after the representative review

Fix the M01 defects exposed by each stage before treating its asset and interaction patterns as the
production standard. Reuse the reviewed construction, motion, audio and evidence method across the fifteen
briefs; build each map's own routes, objective composition, landmarks and narrative connections. Re-run
branch/persistence/fog/refusal checks for each map and compare the complete set. M09–M15 require their
inherited-record and ending combinations; a successful M01 run cannot waive them. Whole-campaign, skirmish,
performance, soak, distribution and human acceptance remain their separate master gates.
