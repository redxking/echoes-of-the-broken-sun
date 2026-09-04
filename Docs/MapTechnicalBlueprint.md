# Map Technical Blueprint & Implementation Specification — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis  
**Standing:** Tier 4 Operational Technical Reference (`Docs/README.md`). Directly binds to `Docs/Requirements.md` (§15, §16, §24), `Docs/MapConcepts.md`, `Docs/ArtDirection.md` (A4 Environment Completion), and `Docs/AudioDirection.md` (B3 Ambience Site Families).  
**Purpose:** Defines the exact, deterministic engineering specifications for all 15 campaign mission battlefields and 6 skirmish theaters. Any AI agent or developer implementing map source JSONs (`Content/World/Source/`), procedural mesh generators (`Scripts/generate_art_assets.py`), audio synthesizers (`Scripts/echoes_audio_synth.py`), or C++ presentation views (`Source/EchoesOfTheBrokenSun/Private/EchoesTerrainView.cpp`) must follow these exact parameters.

---

## 1. Global Simulation Grid Invariants

Every map in *Echoes of the Broken Sun* strictly adheres to the following engine and simulation constraints:

1. **Grid Dimensions:** 64 × 64 cells (`width_tiles = 64`, `height_tiles = 64`, total cells = 4,096).
2. **Coordinate Origin:** Southwest origin `[0, 0]` at lower-left; Northeast coordinate `[63, 63]` at upper-right.
3. **Array Layout:** Row-major index formula: `index = y * 64 + x`.
4. **Scale Authority:** 
   - Simulation tile size: `100 cm` (1.0 meter).
   - Unreal presentation tile world size: `200 uu` (2.0 meters).
   - World origin offset: `(TileWorldUnits * 0.5f)` centering.
5. **Movement Bits & Semantics:**
   - Bit 0 (`0x01`): `ground` movement class.
   - Entry cost: Default passable = `10`. Blocked cells = `0`. Difficult terrain (ash, deep shivergrass) = `12`–`16`.
6. **Zero Simulation Touch (SIM-002):**
   - Presentation meshes, terrain visualizers, VFX, lighting, and audio NEVER affect navigation, collision, line-of-sight, saves, replays, or simulation checksums.
   - All visual actors must configure `ECollisionEnabled::NoCollision`, `CanEverAffectNavigation() == false`, `CastShadow = false`, and `bGenerateOverlapEvents = false`.

---

## 2. Master Biome Architecture & Dressing Registry

The 21 battlefields of Soryn span six distinct regional biomes. Each biome requires a dedicated procedural dressing pack and lighting rig:

| Biome ID | Biome Name | `EDressingSiteProfile` | Master Meshes (`ART-*`) | Audio Bed (`AUDIO-*`) | Lighting Kelvin / Lux |
|---|---|---|---|---|---|
| `biome-vitrified` | Vitrified Impact Basin | `GlassScar` | `SM_World_GlassScarShelf`, `SM_World_GlassScarShard`, `SM_World_GlassScarRidge` | `AMB_GlassScar` (`AUDIO-005`) | 3,000K / 18,000 Lux |
| `biome-shivergrass` | Shivergrass Resonance Steppe | `ShivergrassBasin` | `SM_World_ShivergrassClump_01..03`, `SM_World_VaultbackShell_Prop` | `AMB_ShivergrassSteppe` (`AUDIO-007`) | 3,400K / 16,000 Lux |
| `biome-subterranean` | Subterranean Crystal Cavern | `SubterraneanCaverns` | `SM_World_GeodePillar_01..02`, `SM_World_CrystalSpan_Bridge`, `SM_World_StalactiteCluster` | `AMB_CavernArtery` (`AUDIO-007`) | 7,200K / 1,200 Lux |
| `biome-foundry` | Sheared Ark-City Foundry | `ArkCityFoundry` | `SM_World_ConcreteButtress_01`, `SM_World_IndustrialSilo_Prop`, `SM_World_CensusGrate` | `AMB_FoundryVoid` (`AUDIO-007`) | 4,800K / 12,000 Lux |
| `biome-void` | Crownfall Void Horizon | `CrownfallVoid` | `SM_World_AcousticMonolith_01..02`, `SM_World_VoidFractureSpur`, `SM_World_TemporalRefractor` | `AMB_Crownfall` (`AUDIO-005`) | 6,500K / 14,000 Lux |
| `biome-solardais` | Sub-Solar Floating Dais | `SolarFallDais` | `SM_World_ObsidianDaisTile_01..02`, `SM_World_CoronalPylon_Prop`, `SM_World_SolarRiftGlyph` | `AMB_SolarFallDais` (`AUDIO-007`) | 2,200K / 95,000 Lux |

---

## 3. Detailed Campaign Operational Sector Specifications

### M01: The Glass Scar (`What the Ledger Keeps`)
* **Sector Code:** `SEC-01-GS`
* **Biome:** `biome-vitrified` (`EDressingSiteProfile::GlassScar`)
* **Player 1 (Meridian) Base:** `[10, 10]` | **Player 2 (Kharuun) Base:** `[54, 54]`
* **Contested Landmark:** Future Well at `[32, 32]` (Dormant state).
* **Archive Evacuation Objective:** Convoy spawn `[18, 12]`, extraction site `[6, 8]`.
* **Topography:**
  - South Basin: Passable plain `[0..63, 0..29]`.
  - North Basin: Passable plain `[0..63, 35..63]`.
  - Dividing Chasm: Impassable black glass ridge `[0..63, 30..34]`.
  - Crossing 1 (Ash Cut): Passable `[12..15, 30..34]`, entry cost 12.
  - Crossing 2 (Buried Causeway): Passable `[29..35, 30..34]`, entry cost 10.
  - Crossing 3 (Folded Verge): Passable `[48..51, 30..34]`, entry cost 10.
* **Blocked Cell Count:** 165 cells (`kExpectedBlockedCellCount = 165`).
* **Resource Seams:**
  - Player 1 Natural: Matter `[6, 14]`, `[14, 10]`.
  - Player 2 Natural: Matter `[58, 50]`, `[50, 54]`.
  - Contested High-Yield: Matter `[30, 26]`, `[34, 38]`.
* **Audio & Lighting:** `AMB_GlassScar` (`-16 LUFS`) | 3,000K, 18,000 Lux, Sun pitch -35°, roll 15°.

---

### M02: Shivergrass Basin (`Seven Accounts of Rain`)
* **Sector Code:** `SEC-02-SB`
* **Biome:** `biome-shivergrass` (`EDressingSiteProfile::ShivergrassBasin`)
* **Player 1 (Oruun / Kharuun) Base:** `[12, 14]` | **Opponent AI Base:** `[52, 50]`
* **Migration Objective:** 
  - Mobile Waystone starting anchor: `[12, 14]`.
  - Target Migration Re-root site: `[36, 42]`.
  - Memory Bearer witness account site: `[48, 48]`.
* **Topography:**
  - Rolling resonance steppe with 3 stepped elevation contours.
  - Northwest Ridge (impassable basalt cliffs): `[0..20, 48..63]`.
  - Southeast Bog (high movement cost = 14): `[40..63, 0..18]`.
  - Central Prairie: Expansive open ground `[15..45, 15..45]` rippling with probability-sensitive shivergrass.
* **Environmental Feature:** 3 wandering Vaultback megafauna (neutral dynamic terrain obstacles, radius 2 tiles).
* **Blocked Cell Count:** 210 cells.
* **Resource Seams:** Matter seams at `[8, 16]`, `[16, 8]`, `[46, 54]`, `[54, 46]`. Raw Dawnshard clusters at `[32, 30]`.
* **Audio & Lighting:** `AMB_ShivergrassSteppe` | 3,400K, 16,000 Lux, slight amber atmospheric haze (fog density 0.015).

---

### M03: Lume Reach Outskirts (`A City on Reserve`)
* **Sector Code:** `SEC-03-LR`
* **Biome:** `biome-foundry` (`EDressingSiteProfile::LumeReach`)
* **Player 1 (Mara Vey / Meridian) Base:** `[8, 32]` | **Hostile Incursion Spawns:** `[58, 16]`, `[58, 48]`
* **Grid Objectives:** Three vital district Aegis posts requiring unbroken Power Links:
  - Life Support Post: `[24, 18]`
  - Transit Nexus Post: `[24, 46]`
  - Archive Continuity Post: `[40, 32]`
* **Topography:**
  - Urban paved highways (movement cost 8): Central avenues connecting `[8, 32]` to `[40, 32]`.
  - Collapsed Foundation Blocks (impassable building rubble): Rectangles at `[16..20, 24..40]`, `[32..36, 8..24]`, `[32..36, 40..56]`.
* **Blocked Cell Count:** 288 cells.
* **Resource Seams:** Power Link transformer nodes at `[14, 32]`, `[24, 32]`, `[34, 32]`. Matter stockpiles at `[10, 26]`, `[10, 38]`.
* **Audio & Lighting:** `AMB_LumeReach` | 4,500K Cool White, 8,000 Lux, heavy overcast industrial fog (density 0.035).

---

### M04: The Unburied Road (`The Unburied Road`)
* **Sector Code:** `SEC-04-UR`
* **Biome:** `biome-subterranean` (`EDressingSiteProfile::SubterraneanCaverns`)
* **Player 1 (Oruun) Base:** `[14, 10]` | **Opponent Core:** `[50, 54]`
* **Objective:** Establish Waystone roadhead at `[28, 28]`; escort Memory Bearer across the ancient transit bridge to missing shard at `[32, 40]`.
* **Topography:**
  - Deep cavern walls (blocked outer perimeter): `[0..8, 0..63]`, `[56..63, 0..63]`.
  - Subterranean Abyssal Chasm: `[8..56, 28..34]` with only two bridge crossings:
    - Primary Transit Span: `[30..34, 28..34]` (passable stone arch).
    - Flanking Service Siphon: `[14..16, 28..34]` (narrow pedestrian catwalk).
* **Blocked Cell Count:** 412 cells.
* **Resource Seams:** Rich bioluminescent Matter veins along cavern walls `[12, 18]`, `[18, 12]`, `[46, 50]`, `[50, 44]`.
* **Audio & Lighting:** `AMB_CavernArtery` | 7,200K Cyan Bioluminescence, 1,200 Lux (directional sun disabled; lit via ambient cavern radiance and local point lights).

---

### M05: The Line of Parity (`Terms of Continuance`)
* **Sector Code:** `SEC-05-LP`
* **Biome:** `biome-void` (`EDressingSiteProfile::CrownfallVoid`)
* **Joint Allied Command (Meridian / Kharuun):** Local proxies at `[10, 32]` and `[54, 32]`.
* **Objective:** Synchronize twin treaty proxy pylons at `[26, 32]` and `[38, 32]`; maintain link from tick 300 to 900 while fending off unresolved apparitions.
* **Topography:**
  - Symmetric demarcation glacis. Open flat no-man's-land in the center.
  - Defensive earthwork revetments: `[20..22, 16..48]`, `[42..44, 16..48]`.
  - Northern and Southern Chasm Faults: `[24..40, 0..8]`, `[24..40, 56..63]`.
* **Blocked Cell Count:** 240 cells.
* **Resource Seams:** Symmetrical Matter deposits at `[14, 20]`, `[14, 44]`, `[50, 20]`, `[50, 44]`.
* **Audio & Lighting:** `AMB_Crownfall` | 3,800K Pale Twilight, 10,000 Lux, stark low-angle illumination.

---

### M06: Ark-City Sector 9 (`Names Without Births`)
* **Sector Code:** `SEC-06-AS`
* **Biome:** `biome-foundry` (`EDressingSiteProfile::ArkCityFoundry`)
* **Player 1 (Talar Venn) Base:** `[8, 12]` | **Hostile Encroachment Spawns:** `[56, 52]`, `[12, 56]`
* **Objectives:** Locate the Census Trace at `[30, 45]`; escort civilian transport proxies to hardened blast shelters at `[12, 16]`.
* **Topography:**
  - Sheared concrete foundations overlooking a bottomless void.
  - Void cleave: Diagonal chasm running from `[0, 36]` to `[36, 63]` where district structures plummeted into the earth.
  - Connecting steel industrial ramps: `[18..20, 42..46]`, `[28..30, 52..56]`.
* **Blocked Cell Count:** 345 cells.
* **Resource Seams:** Industrial salvage scrap Matter at `[14, 10]`, `[10, 22]`, `[42, 38]`.
* **Audio & Lighting:** `AMB_FoundryVoid` | 4,800K Sodium Vapor, 12,000 Lux, steam emissions and heavy drop shadows.

---

### M07: Cinder Hollow (`The Shape of Silence`)
* **Sector Code:** `SEC-07-CH`
* **Biome:** `biome-shivergrass` (`EDressingSiteProfile::ShivergrassBasin`)
* **Player 1 (Oruun) Base:** `[12, 12]` | **AI Adversary Base:** `[52, 52]`
* **Objective:** Deploy Listening Spine at `[28, 28]`; position twin witness scouts at paired acoustic resonance nodes `[20, 40]` and `[40, 20]`.
* **Topography:**
  - Smoldering crater basin filled with calcified ash dunes (movement cost 13).
  - Dead Future Well at `[32, 28]` (dormant, un-harvestable landmark).
  - Stepped basalt scarp edges blocking direct approach: `[16..48, 16..18]`, `[16..48, 46..48]`.
* **Blocked Cell Count:** 198 cells.
* **Resource Seams:** Carbonized ash Matter clusters at `[16, 24]`, `[24, 16]`, `[48, 40]`, `[40, 48]`.
* **Audio & Lighting:** `AMB_ShivergrassSteppe` (muffled filter) | 2,600K Smoldering Ember Red, 6,000 Lux, zero wind audio bed.

---

### M08: The Mirrored Rift (`The Shape Beside Us`)
* **Sector Code:** `SEC-08-MR`
* **Biome:** `biome-void` (`EDressingSiteProfile::CrownfallVoid`)
* **Player 1 (Talar Venn) Base:** `[16, 20]` | **Choir Emergence Vector:** `[48, 44]`
* **Objective:** Construct Echo Relay at `[32, 32]`; cross paired state sites without triggering defensive resonance collapse.
* **Topography:**
  - Vertical bifurcated chasm splitting the map from `[30..34, 0..63]`.
  - Offset quantum bridge spans (bridges shift visibility phase every 45 seconds):
    - Span Alpha: `[30..34, 16..18]`
    - Span Beta: `[30..34, 46..48]`
* **Blocked Cell Count:** 290 cells.
* **Resource Seams:** Shimmering crystal Matter nodes at `[12, 32]`, `[22, 12]`, `[52, 32]`, `[42, 52]`.
* **Audio & Lighting:** `AMB_Crownfall` | 6,500K Prismatic Shimmer, 14,000 Lux, double-shadow projection active.

---

### M09: Hollow Resonator Ridge (`Reserve Authority`)
* **Sector Code:** `SEC-09-RR`
* **Biome:** `biome-void` (`EDressingSiteProfile::CrownfallVoid`)
* **Player 1 (Mara Vey / Joint) Base:** `[10, 10]` | **Choir Vanguard Spawns:** `[54, 54]`
* **Objective:** Capture and harmonize three acoustic monolith resonators at `[20, 36]`, `[32, 32]`, and `[44, 28]`.
* **Topography:**
  - High-altitude ridge running diagonally from Southwest to Northeast `[8..56, 8..56]`.
  - Steep drop-offs on both sides force fights onto the elevated spine.
* **Blocked Cell Count:** 254 cells.
* **Resource Seams:** Matter seams situated at low-ground base approaches `[8, 20]`, `[20, 8]`, `[56, 44]`, `[44, 56]`.
* **Audio & Lighting:** `AMB_Crownfall` | 3,200K Crimson Dusk, 11,000 Lux, high wind shearing audio bed.

---

### M10: The Census Vault (`The Choir at Lume Reach`)
* **Sector Code:** `SEC-10-CV`
* **Biome:** `biome-foundry` (`EDressingSiteProfile::LumeReach`)
* **Player 1 Base:** `[14, 14]` | **Choir Assault Wave Vector:** `[50, 50]`
* **Objective:** Secure the subterranean inverted pyramid at `[32, 32]`; hold data conduit until download reaches 100%.
* **Topography:**
  - Concentric chamber walls forming a subterranean fortress maze.
  - Choke corridors at `[24, 32]`, `[40, 32]`, `[32, 24]`, `[32, 40]`.
* **Blocked Cell Count:** 380 cells.
* **Resource Seams:** Internal vault generator cores at `[18, 28]`, `[28, 18]`, `[46, 36]`, `[36, 46]`.
* **Audio & Lighting:** `AMB_FoundryVoid` | 5,000K Phosphor Blue, 2,400 Lux, echoing metallic reverberation.

---

### M11: The Verge of Erasure (`No Neutral Ledger`)
* **Sector Code:** `SEC-11-VE`
* **Biome:** `biome-void` (`EDressingSiteProfile::CrownfallVoid`)
* **Allied Coalition Base:** `[12, 32]` | **Choir Incursion Front:** `[52, 32]`
* **Objective:** Secure stable central dais before outer perimeter dissolves into the void.
* **Topography:**
  - Disintegrating basalt shelf. Outer 4-tile perimeter converts to blocked cells every 180 seconds.
  - Central refuge plateau: `[24..40, 20..44]`.
* **Blocked Cell Count:** Starts at 180 cells; expands dynamically to 512 cells.
* **Resource Seams:** Rapidly depleting high-yield Matter caches at `[20, 24]`, `[20, 40]`, `[44, 24]`, `[44, 40]`.
* **Audio & Lighting:** `AMB_Crownfall` | 8,000K Harsh Cold White, 20,000 Lux, reverse temporal audio echoes.

---

### M12: Kharuun Sovereign Caverns (`The Future That Won`)
* **Sector Code:** `SEC-12-SC`
* **Biome:** `biome-subterranean` (`EDressingSiteProfile::SubterraneanCaverns`)
* **Player 1 (Oruun) Base:** `[12, 16]` | **Intruding Extraction Strike Force:** `[52, 48]`
* **Objective:** Defend the Ancestral Geode and core birthing shrines at `[36, 32]`.
* **Topography:**
  - Immense geode cavern with spiraling mineral pillars and deep subterranean water pools (water movement cost 18).
  - Living crystalline spires provide natural defensive bastions.
* **Blocked Cell Count:** 320 cells.
* **Resource Seams:** Pure virgin Matter crystals at `[10, 28]`, `[22, 10]`, `[42, 54]`, `[54, 36]`.
* **Audio & Lighting:** `AMB_CavernArtery` | 3,600K Bioluminescent Amber, 4,000 Lux, rhythmic communal singing resonance.

---

### M13: Meridian High Bastion (`Assembly of the Missing`)
* **Sector Code:** `SEC-13-HB`
* **Biome:** `biome-solardais` (`EDressingSiteProfile::SolarFallDais`)
* **Player 1 (Mara Vey) Base:** `[16, 32]` | **Choir Siege Armada:** `[48, 32]`
* **Objective:** Hold the three citadel gatehouses against overwhelming frontal assault.
* **Topography:**
  - 3-tier terraced military fortress climbing toward the celestial ascent.
  - Ramparts and engineered killzones: `[28..30, 12..52]`.
* **Blocked Cell Count:** 310 cells.
* **Resource Seams:** Fortified internal supply depots at `[12, 20]`, `[12, 44]`, `[22, 32]`.
* **Audio & Lighting:** `AMB_SolarFallDais` | 4,000K Steel Gray, 25,000 Lux, heavy artillery thuds and siren blasts.

---

### M14: The Resonant Chasm (`Several Voices, One Command`)
* **Sector Code:** `SEC-14-RC`
* **Biome:** `biome-solardais` (`EDressingSiteProfile::SolarFallDais`)
* **Coalition Bases:** Meridian at `[12, 12]`, Kharuun at `[12, 52]` | **Choir Nexus:** `[52, 32]`
* **Objective:** Cross the abyssal chasm via dual suspended anchors; link minds at the celestial focus.
* **Topography:**
  - Titanic planetary rift splitting the ground horizontally. Bottomless void beneath.
  - Suspended bridge anchors at `[28..36, 20..24]` and `[28..36, 40..44]`.
* **Blocked Cell Count:** 440 cells.
* **Resource Seams:** Raw Dawnshard clusters on cliff edges `[18, 22]`, `[18, 42]`, `[46, 26]`, `[46, 38]`.
* **Audio & Lighting:** `AMB_SolarFallDais` | 2,800K Blazing Solar Flare, 45,000 Lux, deep gravitational bass groan.

---

### M15: The Solar Fall Dais (`The Broken Sun`)
* **Sector Code:** `SEC-15-SD`
* **Biome:** `biome-solardais` (`EDressingSiteProfile::SolarFallDais`)
* **Allied Convergence Base:** `[14, 32]` | **Choir Transcendence Core:** `[50, 32]`
* **Contested Epicenter:** The Broken Sun Dais at `[32, 32]`.
* **Objective:** Execute final campaign protocol (Controlled Stabilization, Open Horizon, Memory Preserved, or Reciprocal Accord).
* **Topography:**
  - Floating geometric obsidian platform suspended in void space directly beneath the shattered star.
  - Zero natural cover; all fortifications must be player-built.
  - Outer void drop-offs: all cells outside `[10..54, 10..54]` are blocked void.
* **Blocked Cell Count:** 620 cells.
* **Resource Seams:** Pure condensed Dawn energy nodes at `[20, 20]`, `[20, 44]`, `[44, 20]`, `[44, 44]`.
* **Audio & Lighting:** `AMB_SolarFallDais` | 2,200K Blinding Golden Corona, 95,000 Lux, full cosmic choir wail.

---

## 4. Competitive Skirmish Theaters (S01–S06)

| Preset ID | Map Name | Format | Biome | Key Mechanical Affordance |
|---|---|---|---|---|
| `SK_01` | Twin Wells | 1v1 | `GlassScar` | Two mirrored high-ground Future Wells; split-force economy. |
| `SK_02` | Crucible Ridge | 1v1 / 2v2 | `ShivergrassBasin` | High-ground center mesa overlooking vulnerable low-ground Matter seams. |
| `SK_03` | Sunken Foundry | 2v2 / 4-FFA | `SubterraneanCaverns` | Four corner spawns; low fog obscuring movement through slag canals. |
| `SK_04` | Frostshard Expanse | 1v1 | `CrownfallVoid` | Brittle Dawnshard drifts act as destructible barriers opening sudden flanks. |
| `SK_05` | Shattered Causeway | 3v3 / 6-Player | `ArkCityFoundry` | Three parallel transit spans; coordination across all bridges required. |
| `SK_06` | Choir Sanctum | 1v1 Tournament | `SolarFallDais` | Perfectly symmetrical tournament arena with acoustic line-of-sight blockers. |

---

## 5. Architectural Verification & Acceptance Criteria

Before any map build is merged to `main`:
1. **Validation Tool Execution:**
   - Source JSON must validate against `Content/World/Schema/map_source_v2.schema.json`.
   - Python compilation via `Content/World/Tools/compile_map_pack.py` must succeed with zero errors.
   - SHA-256 sidecars must match compiled output bytes exactly.
2. **C++ Header Generation:**
   - Static headers emitted via `emit_compiled_map_pack_header.py` must compile cleanly in Unreal Engine 5.8.2.
3. **Simulation Authority Verification:**
   - Native sim tests (`./Scripts/test_sim.sh`) must pass 96/96 tests asserting zero simulation impact from dressing actors.
4. **Performance Gate:**
   - Must sustain ≥ 60 FPS on Apple Silicon M1 Pro under Metal SM5 with full dressing instances drawn.
