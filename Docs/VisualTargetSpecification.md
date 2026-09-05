# Visual Target Specification & Design Direction

**Author and owner:** Angelis Pseftis  
**Created:** 2026-09-05  
**Maintained:** 2026-09-05  
**Authority:** [Project/AGENTS.md](../AGENTS.md), [Docs/README.md](README.md), [Docs/ArtDirection.md](ArtDirection.md), and [Docs/Requirements.md](Requirements.md).

---

## 1. Quality Target & Originality Mandate

*Echoes of the Broken Sun* targets the graphical polish, responsive micro-feedback, and tactile clarity of tier-one competitive RTS benchmarks (such as *StarCraft II*), while strictly avoiding derivative visual clichés.

### The Originality Standard
- **No Legacy 3-Box Console:** The game rejects the rigid 1998/2010 Blizzard layout (`[Minimap | Unit Wireframe | 3x5 Grid]` occupying the bottom 25% of the viewport). Instead, *Echoes* employs a **floating, modular tactical HUD** featuring an elegant contextual command arc, corner telemetry, and an uncluttered battlefield canvas.
- **No Generic Sci-Fi Power Armor:** The Meridian Compact is an industrial and civic engineering society. Combat units wear utilitarian pale ceramic heat-plating (`ceramic_civic`) over dark carbon-composite frames with visible hydraulic load paths, heavy surveyor optics, and deployable planar kinetic barriers.
- **No Generic Mineral or Bio-Zerg Clones:** The Kharuun Assemblies are living geological custodians and memory-bearers. Their structures and warforms are grown layered basalt strata with faceted amber mineral nodes, not mindless alien beasts or generic crystals.
- **No Generic Ghostly Psionics:** The Hollow Choir embodies unchosen futures that were denied existence. Their architecture and units exhibit "two valid shadows", offset probability duplicate contours, and magenta possibility bleed.
- **Diegetic Command Deck Framing:** In accordance with `SPEC-UI-005`, the interface is Commander Mara Vey's operational instrument: utilizing ledger entries, duty windows, reserve margins, status bands, and factual annunciators.

---

## 2. Master Color & Value Hierarchy

Every asset, shader, and UI element must conform to the five canonical notes plus one temperature complement:

| Note | Hex / Linear Value | Operational Role in *Echoes* |
|---|---|---|
| **Charcoal** | Linear `0.02–0.07`, roughness floor `0.85` | Vitrified basalt terrain, structural base mass, UI panel backings. Never lifted to grey by fill light. |
| **Pale Ceramic** | Tone `~0.86`, roughness `~0.34`, metallic `0.04` | Meridian civic plating, primary UI typography, high-contrast borders. Wear appears at edges and load points. |
| **Broken-Sun Amber/Gold** | RGB `(1.0, 0.82, 0.62)`, intensity `10.0` | Key sunlight, Dawnshard fracture veins, Kharuun mineral heat, tactical alert states. |
| **Cyan** | RGB `(0.0, 0.95, 1.0)` | Matter deposits, Meridian power conduit lines, UI confirmation states. |
| **Magenta-Fracture** | RGB `(0.88, 0.14, 0.76)` | Crownfall reality bleed, Hollow Choir phase identity, anomalous Future Well reactions. |
| *Indigo (Complement)* | RGB `(0.48, 0.60, 0.88)`, intensity `1.6` | Crownfall sky fill light, shadow temperature. Not a surface paint color. |

---

## 3. Visual Exposure & Layer Budget (ART-A4-002)

Every composed frame adheres to a strict five-tier luminance budget:
1. **Terrain (Lowest):** Matte, low saturation, no competing emissives. Mean luma sits between `50–70`; clipped highlights must remain `≤ 0.005%`.
2. **Landmarks (Future Wells, Impact Basins):** Orienting emissive presence, held strictly below actor silhouettes.
3. **Actors (Units & Structures):** Own the primary saturation and emissive budgets. Every unit and structure must be recognizable in silhouette alone (and in black-and-white).
4. **Transient Combat Effects:** Brightest short-lived elements (kinetic impact sparks, beam traces, destruction bursts), disappearing within seconds.
5. **Interface (Brightest Stable Layer):** Pale ceramic typography and clean holographic telemetry framing the action without flooding the battlefield.

---

## 4. UI Architecture & Screen Suite

### 1. Main Menu (`WBP_EchoesMainMenu`)
- **Setting:** Real-time 3D interactive observation deck inside an Ark-City command bridge (`Entry_Menu.umap`).
- **Composition:** Commander Mara Vey standing over an active holographic tactical table projecting Soryn's conflict sectors.
- **Vista:** Outside reinforced angled glass, the Broken Sun burns with fractured golden plasma rings in an indigo sky.
- **Controls:** Floating diegetic holographic menu tabs (`CAMPAIGN - SORYN'S FALL`, `SKIRMISH OPERATIONS`, `SECTOR CONQUEST`, `ARCHIVE & REPLAYS`, `SYSTEMS`).

### 2. Tactical In-Game HUD (`WBP_EchoesTacticalHUD`)
- **Top-Left:** Minimalist topographical radar minimap with contour lines, elevation tiers, fog of war, and Future Well beacon status.
- **Top-Right:** Live tactical telemetry for **Matter**, **Dawn**, and **Logistics**.
- **Bottom-Center:** Floating contextual command arc that appears on unit selection, featuring geometric vector glyphs for Move, Stop, Hold, Patrol, Repair, and Abilities.
- **Selection Card:** Floating holographic unit status readout displaying Health, Shield, and Armor matrices.

### 3. Faction Selection War Room (`WBP_EchoesFactionWarRoom`)
- **Center:** 3D holographic globe of Soryn displaying glowing Dawnshard faultlines and contested sectors.
- **Three Faction Dossiers:** Side-by-side comparative cards with 3D animated unit showcases, faction crests, lore dossiers, and distinct mechanical strengths.

### 4. Campaign Operations Map (`WBP_EchoesCampaignMap`)
- **Format:** Sector map of Soryn mapping all 15 campaign operations (`M01–M15`).
- **Dossier:** Mission objectives, secondary recovery protocols, and Dawnshard yield estimates.
