# Component Design Catalog: Menus, Structures & Visual Systems

**Author and owner:** Angelis Pseftis  
**Created:** 2026-09-05  
**Maintained:** 2026-09-05  
**Authority:** [Project/AGENTS.md](../AGENTS.md), [Docs/README.md](README.md), [Docs/ArtDirection.md](ArtDirection.md), [Docs/VisualTargetSpecification.md](VisualTargetSpecification.md), and [Docs/Requirements.md](Requirements.md).

---

## 1. Architectural Principles

This catalog provides the definitive design, silhouette, and functional specifications for user interfaces, faction structures, combat units, and visual effects in *Echoes of the Broken Sun*. 

Every building, menu, unit, and particle emitter is governed by three non-negotiable rules:
1. **Simulation Authority (`Source/EchoesSimCore`):** Presentation consumes simulation state; cosmetic structures, animations, and visual effects must never alter collision traces, navigation mesh, fog of war, save states, or replay checksums.
2. **Original Modern RTS Aesthetic:** Avoid 1998/2010 Blizzard layout clichés (no screen-hogging 3-box console, no generic space-marine power armor). Utilize floating contextual telemetry, utilitarian industrial engineering, living geological architecture, and phase-uncertain probability forms.
3. **Master Palette Conformance:** Every structure, effect, and UI element is attributable to the canonical palette: Charcoal, Pale Ceramic, Broken-Sun Amber, Cyan, Magenta-Fracture, and Indigo fill.

---

## 2. Faction Structure Profiles

### A. Meridian Compact Architecture (`SPEC-BLD-015`)
The Compact builds with repairable frames, exposed load paths, redundant conduits, modular hardpoints, and visual status bands. Pale ceramic heat-plating (`ceramic_civic`) is layered over dark machined steel.

#### 1. Anchor [Command Headquarters] (`SPEC-BLD-015.MC.ANCHOR`)
- **Footprint & Scale:** 6x6 build grid (1200 cm x 1200 cm), octagonal reinforced perimeter hull.
- **Visual Features:** Heavy pale ceramic thermal armor plating, high-visibility status indicator bands (yellow/black diagonal safety striping), dual heavy surveyor exit ramps with hydraulic blast doors, and twin cyan sensor antenna arrays.
- **Functional Animation:** When active, the roof-mounted communication dish rotates slowly and cyan sensor masts pulse. During worker assembly, hydraulic ramps open and emit interior amber work-light.
- **Placement & Damage States:** Under construction: welded steel space-frame with holographic alignment brackets. Damaged (>50% HP lost): ceramic plates fracture, exposing dark smoking conduits and intermittent electrical arcing.

#### 2. Power Link [Conduit Pylon] (`SPEC-BLD-015.MC.LINK`)
- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), slender 18-meter hexagonal pylon.
- **Visual Features:** Pale ceramic ring insulators, vertical conduit housing, and four ground-anchored energy spreaders.
- **Functional Animation:** Emits a continuous, low-intensity planar cyan energy grid spreading across the ground to adjacent Compact structures, visually mapping the logistics network boundary.
- **Damaged State:** Spreader pylons lose ground contact, sparking erratically; network radius flickers.

#### 3. Array Foundry [Mech & Vehicle Assembly] (`SPEC-BLD-015.MC.FOUNDRY`)
- **Footprint & Scale:** 8x6 build grid (1600 cm x 1200 cm), industrial fabrication facility.
- **Visual Features:** Overhead gantry crane framework with robotic welding arms, side component intake feed conduits, and a reinforced roll-out bay with floor guide tracks.
- **Functional Animation:** When manufacturing a unit (such as the Atlas Mech or Lancer), robotic arms swing into position emitting directional welding sparks and cyan plasma arcs. On 100% completion, the blast doors slide open and the unit rolls out onto the rally vector.

#### 4. Aegis Post [Defensive Turret] (`SPEC-BLD-015.MC.AEGIS`)
- **Footprint & Scale:** 3x3 build grid (600 cm x 600 cm), low-profile fortified bunker.
- **Visual Features:** Sloped ceramic-composite ballistic skirts, armor-plated observation visor, and an automated dual-axis turret mount housing twin kinetic accelerator cannons.
- **Functional Animation:** While supplied by a Power Link, the turret tracks enemy targets with 360-degree rotation and projects a subtle cyan barrier perimeter ring. When firing, cannons recoil alternately with high-contrast kinetic muzzle flash and casing ejection.

---

### B. Kharuun Assemblies Architecture (`SPEC-BLD-016`)
The Assemblies cultivate living mineral-organic geology. Structures are grown from the planet's basalt strata, altering porosity, heat flow, and acoustic resonance.

#### 1. Memory Hearth [Assembly Headquarters] (`SPEC-BLD-016.KA.HEARTH`)
- **Footprint & Scale:** 7x7 build grid (1400 cm x 1400 cm), 145-meter living geological dome.
- **Visual Features:** Layered basalt strata outer shell, concentric illuminated amber geothermal conduits, multiple subterranean worker emergence chasms, and a towering central resonance spire.
- **Functional Animation:** Geothermal heat pulses upwards through the concentric fissures. The resonance spire resonates with low-frequency acoustic tremors when ancestral memories are accessed or units are cultivated.

#### 2. Waystone [Mobile Supply & Root Node] (`SPEC-BLD-016.KA.WAYSTONE`)
- **Footprint & Scale:** 3x3 build grid (600 cm x 600 cm) rooted; 80-meter monolithic pillar.
- **Visual Features:** Banded mineral strata column studded with clusters of translucent amber quartz crystals, anchored by a sprawling root array of mineral tendrils.
- **Functional Animation:**
  - *Rooted Mode:* Root tendrils bore deep into the basalt rock, drawing geothermal energy and illuminating the crystal clusters.
  - *Migrating Mode:* Tendrils extract from the ground, shifting into heavy geological tripod legs that walk with slow, deliberate tremors.

#### 3. Growth Basin [Warform Gestation Facility] (`SPEC-BLD-016.KA.BASIN`)
- **Footprint & Scale:** 7x7 build grid (1400 cm x 1400 cm), circular stepped terrace.
- **Visual Features:** Concentric carved basalt terraces descending into an iridescent geothermal mineral pool, surrounded by crystalline gestation nodes and an emergence incline ramp.
- **Functional Animation:** The mineral pool swirls with warm amber and iridescent fluid. As a warform (Riftstalker, Cairnback) incubates, crystalline silhouettes coalesce beneath the surface before ascending the ramp.

#### 4. Listening Spine [Seismic Detection Array] (`SPEC-BLD-016.KA.SPINE`)
- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), slender 110-meter geological needle.
- **Visual Features:** Sharp volcanic basalt needle silhouette flanked by micro-arrays of resonant quartz whiskers.
- **Functional Animation:** The whiskers vibrate in harmonic response to ground motion, sending faint amber seismic rings rippling across the surrounding ground.

---

### C. Hollow Choir Architecture (`SPEC-BLD-017`)
The Choir does not construct or cultivate; it stabilizes unchosen timelines. Structures exhibit "maintained possibility": dual valid shadows, offset duplicate contours, and translucent chromatic refraction.

#### 1. Concordance [Choir Core Sanctuary] (`SPEC-BLD-017.HC.CONCORDANCE`)
- **Footprint & Scale:** 6x6 build grid (1200 cm x 1200 cm), levitating geometric hyper-structure.
- **Visual Features:** A massive central crystalline polyhedral core floating 15 meters above ground, bound by three rotating harmonic containment rings. Two distinct valid shadows fall at divergent angles across the terrain.
- **Functional Animation:** The containment rings counter-rotate in silent precision. Deep magenta possibility bleed leaks from the core seams, creating a localized field of chromatic aberration.

#### 2. Interval Loom [Possibility Resonator / Pylon] (`SPEC-BLD-017.HC.INTERVAL`)
- **Footprint & Scale:** 2x2 build grid (400 cm x 400 cm), 22-meter floating tuning fork silhouette.
- **Visual Features:** Dual vertical crystal tines hovering above an ethereal focal pedestal. Crystalline filament spans the gap between tines.
- **Functional Animation:** The tines vibrate with high-frequency micro-oscillations, projecting an ethereal magenta refraction envelope across connected Choir structures.

#### 3. Chorus Loom [Entity Synthesis Matrix] (`SPEC-BLD-017.HC.CHORUS`)
- **Footprint & Scale:** 6x6 build grid (1200 cm x 1200 cm), open-air spatial distortion basin.
- **Visual Features:** A ring of six levitating geometric mirrors angling inward toward an imploding spatial vortex.
- **Functional Animation:** When summoning units, the mirrors converge, bending light from the surrounding environment until a duplicate reality solidifies into a physical Choir combat entity.

#### 4. Phase Anchor [Dimensional Interceptor Turret] (`SPEC-BLD-017.HC.PHASE_ANCHOR`)
- **Footprint & Scale:** 3x3 build grid (600 cm x 600 cm), tri-fold levitating prism.
- **Visual Features:** Three angular dark-crystal facets orbiting an inverted singularity core.
- **Functional Animation:** When idle, the facets drift in gentle equilibrium. Upon targeting hostile units, the facets snap forward, discharging a focused magenta probability fracture beam that leaves an after-image trail.

---

### D. Faction Combat Units Form Language (`SPEC-UNT-001`)

```
================================================================================
FACTION UNIT TAXONOMY & RECOGNITION GRAMMAR
================================================================================
Meridian Compact:     Angular, wheeled/tracked/hydraulic, pale ceramic hull, cyan emitters
Kharuun Assemblies:   Multi-legged, segmented basalt carapace, amber crystal nodules
Hollow Choir:         Levitating geometric prisms, dual shadows, magenta refraction trail
================================================================================
```

#### 1. Meridian Compact Units
- **Surveyor (Worker):** Heavy tracked chassis with twin hydraulic manipulator arms, pale ceramic cab armor, front-facing amber utility floodlights, and rear matter-storage hopper.
- **Lancer (Fast Raider):** High-mobility six-wheeled scout vehicle with low ground clearance, sloped ceramic armor prow, and top-mounted rapid-fire cyan kinetic rail-carbine.
- **Bulwark (Main Battle Tank):** Dual heavy treads, sloped composite glacis plate, deployable side stabilizer jacks for siege mode, and high-velocity smoothbore kinetic accelerator cannon.
- **Atlas Mech (Heavy Walker):** 14-meter bipedal heavy combat platform with reverse-knee hydraulic articulation, torso-mounted missile pods, and twin arm-mounted ionized railguns.

#### 2. Kharuun Assemblies Units
- **Tender (Worker):** Compact hexapedal mineral scavenger with chitinous basalt plates and glowing amber drill-mandibles for dissolving Dawnshard veins.
- **Riftstalker (Ambush Predator):** Quadrupedal crystalline warform that burrows into basalt faultlines. Features razor-edged basalt dorsal spines and an amber geothermal strike venom.
- **Shardbearer (Ranged Artillery):** Slow-moving monolithic hexapod supporting a massive quartz tuning column on its back that lobs explosive molten basalt projectiles.
- **Cairnback (Siege Colossus):** 28-meter walking geological mountain. Features multi-jointed basalt legs, internal magma vents, and the ability to root into the terrain to launch seismic shockwaves.

#### 3. Hollow Choir Units
- **Apparition (Worker / Scout):** Levitating crystalline teardrop with an offset phantom duplicate, capable of harvesting Dawnshards through quantum phasing.
- **Mirror Vanguard (Frontline Skirmisher):** Levitating geometric prism flanked by two physical decoy silhouettes that absorb incoming kinetic fire.
- **Resonant Weaver (Support / Control):** Floating tuning-fork entity that projects magenta harmonic fields, slowing enemy projectile velocity and bending ballistic paths.
- **Singularity Core (Super-Unit):** Self-contained gravitational tear bound in dark crystal rings, capable of temporarily folding terrain geometry into an impassable rift.

---

## 3. Menu Systems & User Interface Architecture

### A. Main Menu & Ark-City Command Bridge (`WBP_EchoesMainMenu` - `SPEC-UI-001`)
- **Scene Composition:** Real-time 3D interactive observation deck inside an Ark-City command bridge (`Entry_Menu.umap`). Commander Mara Vey stands in utilitarian ceramic armor at an interactive holographic war-table. Outside, the Broken Sun burns with fractured solar rings against an indigo twilight sky.
- **Navigation Integration:** Menu options project directly from the war-table as floating translucent glass brackets with pale ceramic typography and cyan accents:
  - `CAMPAIGN - SORYN'S FALL`
  - `SKIRMISH OPERATIONS`
  - `SECTOR CONQUEST`
  - `ARCHIVE & REPLAYS`
  - `SYSTEMS`
- **Audio Feedback:** Resonant, low-latency UI audio clicks conforming to `-24 LKFS` master calibration; zero harsh high-frequency chirps.

### B. Campaign Operations Map (`WBP_EchoesCampaignMap` - `SPEC-UI-002`)
- **Core Display:** Tactical planetary radar of Soryn divided into contested sectors with connected mission route nodes and sector stability meters.
- **Mission Dossier (Right Panel):**
  - Mission Header: e.g. `OPERATION M01: GLASS SCAR`
  - Primary & Secondary Objectives
  - Resource Yields: Matter, Dawnshards, Logistics points
  - Threat Assessment Meter: Dynamic risk rating bar (Low / Medium / High)
  - Audio Briefing: Audio transmission waveform with Commander Mara Vey voice line playback
- **Progression Ledger (Left Panel):** Sector stability indicators (`M01–M15`), unlocked faction battle-groups, and casualty ledgers.

### C. In-Game Tactical HUD (`WBP_EchoesTacticalHUD` - `SPEC-UI-003`)
- **Contextual Command Arc (Bottom-Center):** Floating minimalist arc appearing only when units are selected, displaying clean geometric glyphs for Move, Stop, Hold, Patrol, Repair, and Special Abilities.
- **Topographical Minimap (Top-Left):** Vector contour map showing elevation changes, terrain chokepoints, fog of war, friendly/enemy units, and Future Well beacon states.
- **Live Resource Telemetry (Top-Right):** Clean horizontal readouts for **Matter** (cyan icon), **Dawn** (amber icon), and **Logistics** (supply meter).
- **Floating Unit Status Card:** Holographic card displaying 3D animated wireframe, dual health/shield bars, and armor rating.

### D. Faction Selection War Room (`WBP_EchoesFactionSelect` - `SPEC-UI-004`)
- **Layout:** Three interactive holographic pedestals showcasing the Meridian Compact, Kharuun Assemblies, and Hollow Choir.
- **Faction Dossiers:** Displays economic traits, signature unit rosters, macro-mechanics (Conduit Grid vs. Geothermal Rooting vs. Probability Duplication), and difficulty ratings.

### E. In-Game Pause & Field Operations Ledger (`WBP_EchoesPauseMenu` - `SPEC-UI-007`)
- **Layout:** Frosted charcoal glass overlay preserving dimmed tactical battlefield context underneath (`ART-A4-002`).
- **Mara's Operational Ledger:**
  - Live mission checklist with checked/unchecked tactical objectives.
  - Active time-elapsed counter and current casualty ratio.
  - Quick-action buttons: `RESUME OPERATION`, `MISSION OBJECTIVES`, `TACTICAL RESTART`, `SETTINGS`, `ABANDON TO SHELL`.
  - Zero full-screen blackout; combat audio transitions smoothly to low-pass muffled tactical filter.

### F. Post-Match Debriefing & Telemetry Analytics (`WBP_EchoesDebriefing` - `SPEC-UI-008`)
- **Header:** High-contrast `VICTORY` / `DEFEAT` banner with mission duration, faction heraldry, and difficulty rating.
- **Telemetry Performance Curves (Center):** Interactive multi-layer graph tracking:
  - Matter Harvest Rate (cyan curve)
  - Dawn Possibility Extraction (amber curve)
  - Army Value Over Time (white curve)
  - Timeline markers for Future Well protocol activations.
- **Tabbed Statistical Breakdown:** `OVERVIEW`, `COMBAT EFFICIENCY`, `ECONOMY & HARVEST`, `TECHNOLOGY & DOCTRINES`.

### G. Roguelite 25-Sector Conquest Map (`WBP_EchoesSectorConquest` - `SPEC-UI-009`)
- **Hex/Node Grid Structure:** 25 interconnected planetary sectors arranged across five escalating threat tiers.
- **Sector Node Archetypes:**
  - `Combat Skirmish`: Standard battle against hostile vanguard forces.
  - `Dawn Anomaly`: High-yield Future Well encounter with environmental hazards.
  - `Forward Depots`: Field repair, unit reinforcement, and doctrine card upgrades.
  - `Planetary Stronghold`: Tier boss battles requiring multi-phase base destruction.
- **Strategic Modifier Cards:** Choice of three persistent fleet doctrine buffs awarded upon sector victory.

### H. Skirmish & Multiplayer Operations Lobby (`WBP_EchoesSkirmishLobby` - `SPEC-UI-010`)
- **Map Selection Preview:** High-resolution topographical map preview with resource spawn indicators, Future Well coordinates, and player spawn locations.
- **Slot Configuration:** Up to 8 player/AI slots with faction drop-downs, handicap modifiers, AI difficulty settings (Easy, Medium, Hard, Simulation Master), and team alliances.
- **Game Parameter Toggles:** Starting resources, Future Well destabilization timer, fog of war density, and victory condition presets (Conquest, Well Dominance, Survival).

---

## 4. Visual Effects (VFX) & Niagara Particle System Grammar

All visual effects must conform strictly to `Docs/ArtDirection.md` layer luminance budgets (`ART-A4-002`) to ensure tactical clarity at high unit densities.

```
================================================================================
VFX LUMINANCE HIERARCHY
================================================================================
Layer 0: Terrain / Vitrified Basalt  --> Luma: 30-70  (Roughness 0.85 floor)
Layer 1: Unit Mesh Base Material      --> Luma: 80-140 (Pale ceramic / dark rock)
Layer 2: Ambient Status / Conduit     --> Luma: 150-180 (Subtle cyan/amber glow)
Layer 3: Weapon Projectiles / Impact  --> Luma: 200-240 (Peak tactical read)
Layer 4: Future Well Core Phenomena   --> Luma: 255 (Controlled peak, no strobe)
================================================================================
```

### A. Faction Combat VFX Grammar (`SPEC-VFX-001`)

| Faction | Primary Weapon VFX | Impact & Debris VFX | Defensive / Shield VFX |
|---|---|---|---|
| **Meridian Compact** | High-velocity cyan railgun beam with ionized particle ribbon (`NS_MC_RailgunBeam`). | Crisp kinetic shrapnel, pale ceramic smoke puffs, directional casing ejection. | Planar hexagonal cyan barrier with localized impact ripple (`NS_MC_HexBarrier`). |
| **Kharuun Assemblies** | Molten amber magma globule / volcanic basalt kinetic shards (`NS_KA_MagmaBlast`). | Explosive rocky flak, amber seismic ground fissures, rising heat distortion ripples. | Living basalt crust hardening, ember glow across mineral fracture plates. |
| **Hollow Choir** | Magenta probability fracture beam with chromatic aberration (`NS_HC_PhaseBeam`). | Imploding violet spatial rift, reality-variance pixel dissolve, null-sound shockwave. | Decoy silhouette duplication, phase-shift jitter blur, ethereal refraction envelope. |

### B. Future Well 3-Protocol VFX Grammar (`SPEC-VFX-002`)

The Future Well is the game's core celestial anomaly. Its visual states must convey immense power while maintaining clear tactical telegraphing:

1. **Harvest Protocol (`NS_FW_Harvest`):**
   - Vertical amber solar plasma vortex funneling upward into orbital siphon spires.
   - Ground fissures glow white-hot; high-speed particle acceleration rings rotate around the core.
   - Ground terrain suffers permanent basalt vitrification scarring along extraction vectors.
2. **Preserve Protocol (`NS_FW_Preserve`):**
   - Serene spherical harmonic containment field in deep indigo and cyan geometric rings.
   - Cools turbulent solar flares into steady crystalline orbits; suppresses ground tremors.
   - Projects a protective defensive aura granting regeneration and damage reduction to defending units.
3. **Reshape Protocol (`NS_FW_Reshape`):**
   - Violent magenta reality-fracture shockwave rippling outwards through the ground.
   - Basalt rock crystalline spines erupt from the earth, physically reconfiguring chokepoints.
   - Prismatic refraction fields that distort unit vision and alter line-of-sight cones.

---

## 5. AI Implementation Guidelines for Unreal Engine 5.8

When AI collaborators (ChatGPT, Codex, Antigravity, Claude) implement UI widgets, Niagara emitters, or mesh actors from this catalog, they must adhere to the following file conventions:

### A. Asset Pathing & Naming Conventions
- **UI Widgets:** `Content/UI/Screens/` (`WBP_EchoesMainMenu`, `WBP_EchoesTacticalHUD`, `WBP_EchoesDebriefing`).
- **Niagara Systems:** `Content/Effects/Niagara/` (`NS_MC_RailgunBeam`, `NS_KA_MagmaBlast`, `NS_FW_Harvest`).
- **Static & Skeletal Meshes:** `Content/Meshes/Buildings/` (`SM_MC_Anchor`, `SM_KA_Hearth`, `SM_HC_Concordance`).
- **Materials:** `Content/Materials/` (`M_CeramicCivic`, `M_VitrifiedBasalt`, `M_PhaseRefraction`).

### B. CommonUI & Viewport Rules
- All full-screen shell menus inherit from `UCommonActivatableWidget`.
- Tactical HUD elements must respect safe-zone margins (minimum 32px padding from screen edge).
- Any button click or hover event must trigger sound cue routed to `SC_UIMaster` calibrated to `-24 LKFS`.
- Flashing threshold: No emitter or material pulse may exceed 3 Hz frequency (photosensitivity compliance).
