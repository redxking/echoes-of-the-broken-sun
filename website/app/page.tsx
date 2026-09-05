'use client';

import { useState } from 'react';

const factions = [
  {
    id: 'meridian',
    name: 'Meridian Compact',
    tag: 'Build. Connect. Endure.',
    accent: 'teal',
    color: '#00f2ff',
    doctrine: 'Reliable power through measured systems',
    text: 'A network of culturally distinct city-states joined by logistics, Dawnshard accounting, and mutual defense. Anchors and Power Links create an interdependent grid for production, repair, sensors, and defensive fire.',
    unitsImg: '/assets/concepts/meridian-units.webp',
    structuresImg: '/assets/concepts/echoes-meridian-structures.webp',
    units: ['Surveyor (Engineer)', 'Lancer (Line Rifle)', 'Bulwark Team (Shield Screen)', 'Relay Skiff (Recon/Support)'],
    structures: ['Anchor (HQ)', 'Power Link (Pylon)', 'Array Foundry (Production)', 'Aegis Post (Defense Turret)']
  },
  {
    id: 'kharuun',
    name: 'Kharuun Assemblies',
    tag: 'Move. Adapt. Remember.',
    accent: 'amber',
    color: '#ffb300',
    doctrine: 'Living infrastructure and shared memory',
    text: 'Living mineral-organic assemblies that migrate before taking root. They cultivate the terrain, sense movement through stone vibrations, and select visible adaptations near Growth Basins.',
    unitsImg: '/assets/concepts/kharuun-units.webp',
    structuresImg: '/assets/concepts/echoes-kharuun-structures.webp',
    units: ['Tender (Worker)', 'Riftstalker (Quadruped Lance)', 'Cairnback (Mineral Cover)', 'Resonant (Vibration Scout)'],
    structures: ['Memory Hearth (HQ)', 'Waystone (Mobile Anchor)', 'Growth Basin (Bio-Terrace)', 'Listening Spine (Seismic Needle)']
  },
  {
    id: 'choir',
    name: 'Hollow Choir',
    tag: 'The future is not empty.',
    accent: 'magenta',
    color: '#b3417a',
    doctrine: 'Preserve possibility, then resolve it',
    text: 'Linked consciousness formed inside unrealized branches. The Choir controls temporary states, declared choices, and intervals between what is and what might be. Units telegraph phase shifts and must resolve before coherence breaks.',
    structuresImg: '/assets/concepts/echoes-choir-structures.webp',
    units: ['Threadkeeper (Thread Steward)', 'Intervalist (Phase Skirmisher)', 'Lacuna Warden (Recovery Controller)', 'Afterimage (Misdirection Scout)'],
    structures: ['Concordance (Core HQ)', 'Interval Loom (Supply Node)', 'Chorus Loom (Synthesis Basin)', 'Phase Anchor (Coherence Utility)']
  }
];

const characters = [
  {
    code: 'MV',
    name: 'Commander Mara Vey',
    faction: 'Meridian Compact',
    role: 'Field Commander & Player Authority',
    type: 'meridian',
    visual: 'Mid-30s woman, disciplined and calculating, in pale ceramic & graphite modular field armor with exposed brass load joints and cyan power conduit status lines.',
    voice: 'Measured pace, clipped ends under pressure; concern reads as added precision. (af_sarah @1.0)',
    wants: 'A survivable, stable future through disciplined preparation.',
    mustFace: 'Turning every moral question into a control problem can become erasure.'
  },
  {
    code: 'O7',
    name: 'Oruun-of-Seven-Stones',
    faction: 'Kharuun Assemblies',
    role: 'Memory-Bearer & Custodian',
    type: 'kharuun',
    visual: 'Towering mineral-organic being with dark basalt and pale strata carapace; seven polished amber memory stones embedded across chest and shoulders.',
    voice: 'Deep register, resonant and unhurried; dry humor between inherited accounts and present evidence. (bm_george @0.92)',
    wants: 'Memory with accountability; protecting the birthing caverns.',
    mustFace: 'Some peace was built on deliberate omission by ancestral councils.'
  },
  {
    code: 'TV',
    name: 'Talar Venn',
    faction: 'Meridian Compact',
    role: 'Keeper of Archive Convoy',
    type: 'meridian',
    visual: 'Late-20s field archivist wearing durable utility webbing holding physical ledger folios and sealed data canisters over ceramic pauldrons.',
    voice: 'Earnest, hurried, emotionally open; the only voice in the opening allowed to sound afraid. (am_michael @1.0)',
    wants: 'The missing census names recovered before they are erased twice.',
    mustFace: 'Truth cannot protect itself when survival demands immediate power.'
  },
  {
    code: 'CR',
    name: 'Chancellor Cael Rhyse',
    faction: 'Meridian Compact',
    role: 'Civic Chancellor',
    type: 'meridian',
    visual: 'Distinguished statesman in his 50s, wearing a tailored civic mantle in ivory and graphite with elegant gold trim and the Compact seal.',
    voice: 'Polished public-address warmth over administrative certainty; persuasive rather than menacing. (bm_lewis @0.95)',
    wants: 'One stable, governable future for all of Soryn.',
    mustFace: 'Stability imposed from above can deny the right of other lives to exist.'
  },
  {
    code: 'NE',
    name: 'Neme',
    faction: 'Hollow Choir',
    role: 'Several Possible Selves',
    type: 'choir',
    visual: 'Translucent phase-shifted silhouette of near-black obsidian crystalline lattice, bleeding glowing magenta chromatic aberration and duplicate afterimages.',
    voice: 'Precise, deliberately even articulation; cool clarity where imprecision could let one self dominate. (af_nicole @0.95)',
    wants: 'Coexistence without surrender; making the erased count.',
    mustFace: 'Its component futures do not agree on whether peace with the realized world is possible.'
  }
];

const biomes = [
  {
    code: 'Sectors 02 & 07',
    badge: 'Shivergrass Steppe',
    title: 'The Shivergrass Basins',
    img: '/assets/concepts/shivergrass-basin.webp',
    text: 'Rolling resonance plains of probability-sensitive flora that shimmers before physical movement occurs. Massive Vaultbacks graze on mineral strata.',
    specs: ['3,400K Amber Dusk / 16,000 Lux', 'Prairie wind, grass resonance, Vaultback thrum', 'Grass ripples betray stealth movement']
  },
  {
    code: 'Sectors 04 & 12',
    badge: 'Subterranean Artery',
    title: 'Subterranean Crystal Caverns',
    img: '/assets/concepts/unburied-road-caverns.webp',
    text: 'Immense geode highways miles beneath the surface. Rich Matter seams and raw Dawnshards emit cyan bioluminescence amidst dripping mineral water.',
    specs: ['7,200K Cyan Glow / 1,200 Lux', 'Cavernous reverb, dripping water, crystal hum', 'Narrow bridge chokepoints & cave-in zones']
  },
  {
    code: 'Sectors 03, 06, 10',
    badge: 'Sheared Ark-City',
    title: 'Ark-City Foundations & Void',
    img: '/assets/concepts/arkcity-census-void.webp',
    text: 'Brutalist lower tiers of massive ark-cities, where concrete buttresses and automated foundries hang over deep chasms left by Crownfall shearing.',
    specs: ['4,800K Sodium White / 12,000 Lux', 'Industrial machinery thrum, steam vents', 'Lethal drop-offs & narrow industrial ramps']
  },
  {
    code: 'Sectors 13, 14, 15',
    badge: 'Sub-Solar Dais',
    title: 'The Solar Fall Platform',
    img: '/assets/concepts/broken-sun-solar-dais.webp',
    text: 'A colossal floating geometric dais of black vitrified stone hovering over the abyss directly beneath the blinding coronary fracture of the Broken Sun.',
    specs: ['2,200K Blinding Golden Corona / 95,000 Lux', 'Solar coronal roar, temporal shearing, choir wail', 'Periodic solar radiation pulses']
  }
];

export default function Home() {
  const [activeFaction, setActiveFaction] = useState(0);

  return (
    <main>
      {/* HEADER / NAVIGATION */}
      <nav className="site-nav">
        <a className="brand" href="#top">
          <b>✦</b> <span>Echoes <i>of the</i> Broken Sun</span>
        </a>
        <div className="nav-links">
          <a href="#vision">Vision</a>
          <a href="#ui-target">Modern HUD</a>
          <a href="#factions">Factions</a>
          <a href="#vfx">Combat VFX</a>
          <a href="#wells">Future Wells</a>
          <a href="#atlas">Tactical Atlas</a>
          <a href="#characters">Characters</a>
          <a href="#campaign">Campaign</a>
          <a className="archive-pill" href="/archive-static/index.html">Deep Archive ↗</a>
        </div>
      </nav>

      {/* HERO SECTION */}
      <header id="top" className="hero">
        <div className="hero-overlay" />
        <div className="shell hero-copy">
          <span className="eyebrow"><span className="pulse" /> A science-fantasy RTS in development</span>
          <h1>Every future<br /><em>has a cost.</em></h1>
          <p className="hero-lede">
            Soryn is a broken world where lost possibilities still burn beneath the ground. Command distinct asymmetric forces, navigate living battlefields, and decide what deserves to survive.
          </p>
          <div className="hero-actions">
            <a className="button primary" href="#vision">Explore Visual Target <span>↓</span></a>
            <a className="button outline" href="#ui-target">Inspect Modern HUD <span>→</span></a>
            <a className="button outline" href="/archive-static/atlas.html">Open Tactical Atlas ↗</a>
          </div>
          <div className="status-note">
            <strong>DEVELOPMENT STATUS · GATES & ARCHITECTURE</strong>
            Every current Meridian, Kharuun, and Hollow Choir unit and structure has an authored Unreal Engine static mesh pass. Characters, animation, production textures, and gameplay audio remain in development. All figures shown are registered concept targets and in-engine review captures.
          </div>
        </div>
      </header>

      {/* SECTION 01: VISUAL TARGET SPECIFICATION */}
      <section id="vision" className="dark-band">
        <div className="shell">
          <span className="section-kicker">01 / Visual Target Specification</span>
          <div className="section-head">
            <h2>The world we are building.<br /><em>Not yet the world you can play.</em></h2>
            <p>
              This is the visual target for the Glass Scar: a dark vitrified basalt basin under a broken golden sun, a central Future Well, and two armies whose shapes read before their colors do. Beside it is the accepted in-engine Metal capture from the development build.
            </p>
          </div>

          <div className="frame-pair">
            <figure className="evidence-figure">
              <span className="evidence-badge">Concept Target · CONCEPT-004</span>
              <img src="/assets/concepts/target-render-vertical-slice.webp" alt="Glass Scar Vertical Slice Visual Target" width={1376} height={768} />
              <figcaption>
                <b>Glass Scar Vertical-Slice Target</b>
                <span>Concept target · not gameplay</span>
              </figcaption>
            </figure>
            <figure className="evidence-figure">
              <span className="evidence-badge">Engine Capture · CAPTURE-003</span>
              <img src="/assets/engine/glass-scar-overview-in-engine.webp" alt="Glass Scar Overview In-Engine Metal Capture" width={1600} height={900} />
              <figcaption>
                <b>Glass Scar In-Engine Review</b>
                <span>Current build · Metal review fixture</span>
              </figcaption>
            </figure>
          </div>

          <div className="palette-grid">
            <div className="palette-card">
              <div className="palette-swatch" style={{ background: '#1c1f22' }} />
              <h4>Charcoal Basalt</h4>
              <small>#1c1f22 · Terrain & Mass</small>
            </div>
            <div className="palette-card">
              <div className="palette-swatch" style={{ background: '#ecebe6' }} />
              <h4>Pale Ceramic</h4>
              <small>#ecebe6 · Civic Surfaces & Type</small>
            </div>
            <div className="palette-card">
              <div className="palette-swatch" style={{ background: '#f0b45a' }} />
              <h4>Broken-Sun Amber</h4>
              <small>#f0b45a · Veins & Key Light</small>
            </div>
            <div className="palette-card">
              <div className="palette-swatch" style={{ background: '#b3417a' }} />
              <h4>Magenta Fracture</h4>
              <small>#b3417a · Possibility Bleed</small>
            </div>
            <div className="palette-card">
              <div className="palette-swatch" style={{ background: '#5fd8e8' }} />
              <h4>Cyan Confirm</h4>
              <small>#5fd8e8 · Matter & Tech</small>
            </div>
            <div className="palette-card">
              <div className="palette-swatch" style={{ background: '#7a99e0' }} />
              <h4>Indigo Zenith</h4>
              <small>#7a99e0 · Sky Fill & Shadow</small>
            </div>
          </div>
        </div>
      </section>

      {/* SECTION 02: MODERN RTS INTERFACE VS THE CLONE TRAP */}
      <section id="ui-target" className="content-band">
        <div className="shell">
          <span className="section-kicker">02 / Beyond the Clone Trap</span>
          <div className="section-head">
            <h2>Tactical clarity.<br /><em>Original modern RTS interfaces.</em></h2>
            <p>
              StarCraft II proved the power of instant readability and high contrast. But copying its 1998 three-box bottom console robs an RTS of screen real estate. Echoes replaces the heavy bottom shelf with a floating contextual command arc, opening the field while keeping hotkey muscle memory razor sharp.
            </p>
          </div>

          <div className="frame-pair">
            <figure className="evidence-figure">
              <span className="evidence-badge">Original Design · CONCEPT-011</span>
              <img src="/assets/concepts/echoes-hud-modern-arc.webp" alt="Modern Floating Command Arc HUD" width={1376} height={768} />
              <figcaption>
                <b>Original Modern Floating Command Arc HUD</b>
                <span>WBP_EchoesTacticalHUD · Clean viewport</span>
              </figcaption>
            </figure>
            <figure className="evidence-figure">
              <span className="evidence-badge">Baseline Reference · CONCEPT-012</span>
              <img src="/assets/concepts/echoes-hud-traditional-console.webp" alt="Traditional 3-Box Console Baseline" width={1376} height={768} />
              <figcaption>
                <b>Traditional 3-Box Command Console Baseline</b>
                <span>Comparative evaluation against SC2</span>
              </figcaption>
            </figure>
          </div>

          <div className="frame-pair">
            <figure className="evidence-figure">
              <span className="evidence-badge">Diegetic Menu · CONCEPT-013</span>
              <img src="/assets/concepts/echoes-main-menu-bridge.webp" alt="Diegetic Ark-City Command Bridge Main Menu" width={1376} height={768} />
              <figcaption>
                <b>Diegetic Ark-City Command Bridge Menu</b>
                <span>Mara Vey at holographic war-table</span>
              </figcaption>
            </figure>
            <figure className="evidence-figure">
              <span className="evidence-badge">Shell Baseline · CONCEPT-014</span>
              <img src="/assets/concepts/echoes-main-menu-traditional.webp" alt="Traditional Left-Stacked Shell Menu" width={1376} height={768} />
              <figcaption>
                <b>Traditional Left-Stacked Menu Baseline</b>
                <span>Legacy vertical column navigation</span>
              </figcaption>
            </figure>
          </div>
        </div>
      </section>

      {/* SECTION 03: THE THREE FACTIONS */}
      <section id="factions" className="tint-band">
        <div className="shell">
          <span className="section-kicker">03 / The Three Factions</span>
          <div className="section-head">
            <h2>Three answers<br />to one question.</h2>
            <p>
              No faction is simply good or evil. The Meridian Compact trusts systems that can be measured. The Kharuun Assemblies protect a living chain of memory. The Hollow Choir speaks for futures that were erased.
            </p>
          </div>

          <div className="faction-tabs">
            {factions.map((f, i) => (
              <button
                key={f.id}
                className={`faction-tab-btn ${activeFaction === i ? 'active' : ''}`}
                onClick={() => setActiveFaction(i)}
                style={{ color: activeFaction === i ? f.color : undefined, borderBottomColor: activeFaction === i ? f.color : undefined }}
              >
                0{i + 1} / {f.name}
              </button>
            ))}
          </div>

          <div className="detail-grid" style={{ gridTemplateColumns: '1.2fr 1fr' }}>
            <div>
              <span className="section-kicker" style={{ color: factions[activeFaction].color }}>{factions[activeFaction].tag}</span>
              <h3 style={{ fontSize: '32px' }}>{factions[activeFaction].name}</h3>
              <p style={{ fontSize: '16px', lineHeight: '1.8', marginTop: '14px' }}>{factions[activeFaction].text}</p>
              
              <div style={{ marginTop: '24px' }}>
                <small style={{ color: factions[activeFaction].color, textTransform: 'uppercase', font: '500 10px monospace' }}>Authoritative Units</small>
                <div style={{ display: 'flex', gap: '8px', flexWrap: 'wrap', marginTop: '8px' }}>
                  {factions[activeFaction].units.map(u => (
                    <span key={u} style={{ padding: '6px 12px', background: '#080d16', border: '1px solid rgba(255,255,255,0.1)', fontSize: '12px', color: '#cbd5e1' }}>{u}</span>
                  ))}
                </div>
              </div>

              <div style={{ marginTop: '20px' }}>
                <small style={{ color: factions[activeFaction].color, textTransform: 'uppercase', font: '500 10px monospace' }}>Structural Architecture</small>
                <div style={{ display: 'flex', gap: '8px', flexWrap: 'wrap', marginTop: '8px' }}>
                  {factions[activeFaction].structures.map(s => (
                    <span key={s} style={{ padding: '6px 12px', background: '#080d16', border: '1px solid rgba(255,255,255,0.1)', fontSize: '12px', color: '#cbd5e1' }}>{s}</span>
                  ))}
                </div>
              </div>
            </div>

            <div>
              <figure className="evidence-figure">
                <span className="evidence-badge">Structures · {factions[activeFaction].id.toUpperCase()}</span>
                <img src={factions[activeFaction].structuresImg} alt={`${factions[activeFaction].name} Structures Architecture`} width={1376} height={768} />
                <figcaption>
                  <b>{factions[activeFaction].name} Architecture</b>
                  <span>SPEC-BLD-015..017</span>
                </figcaption>
              </figure>
            </div>
          </div>

          <div style={{ marginTop: '40px' }}>
            <figure className="evidence-figure">
              <span className="evidence-badge">Strategic War Room · CONCEPT-015</span>
              <img src="/assets/concepts/echoes-faction-selection.webp" alt="Tri-Faction Strategic War Room" width={1376} height={768} />
              <figcaption>
                <b>Tri-Faction Strategic Staging War Room</b>
                <span>WBP_EchoesFactionSelect · Holographic planet & dossiers</span>
              </figcaption>
            </figure>
          </div>
        </div>
      </section>

      {/* SECTION 04: COMBAT VFX GRAMMAR */}
      <section id="vfx" className="dark-band">
        <div className="shell">
          <span className="section-kicker">04 / Combat VFX Grammar</span>
          <div className="section-head">
            <h2>Weapons of light and stone.<br /><em>Tactical feedback by faction.</em></h2>
            <p>
              In high-speed combat, weapon discharges, projectile tracers, and barrier ripples must be recognizable instantly. Silhouette and color tell you what is attacking before numbers appear.
            </p>
          </div>

          <figure className="evidence-figure">
            <span className="evidence-badge">Combat VFX Grammar · CONCEPT-024</span>
            <img src="/assets/concepts/echoes-combat-vfx-grammar.webp" alt="Combat Visual Effects Grammar Sheet" width={1376} height={768} />
            <figcaption>
              <b>Tri-Faction Combat Visual Effects Grammar</b>
              <span>SPEC-VFX-001 · Rail beams, magma bursts, probability fractures</span>
            </figcaption>
          </figure>

          <div className="detail-grid">
            <article className="detail-card">
              <small>Meridian Compact</small>
              <h3>Kinetic & Ionized Beams</h3>
              <p>Piercing cyan railgun lines with ionized ribbons, modular hexagonal energy barrier ripples, and pale ceramic smoke puffs.</p>
            </article>
            <article className="detail-card">
              <small>Kharuun Assemblies</small>
              <h3>Magma & Basalt Flak</h3>
              <p>Molten amber magma globes, volcanic basalt splinter flak, and concentric amber seismic shockwaves through the terrain.</p>
            </article>
            <article className="detail-card">
              <small>Hollow Choir</small>
              <h3>Reality Distortions</h3>
              <p>Magenta probability fracture beams, chromatic aberration phase-shift shimmer, duplicate phantom silhouettes, and imploding spatial rifts.</p>
            </article>
          </div>
        </div>
      </section>

      {/* SECTION 05: FUTURE WELLS */}
      <section id="wells" className="content-band">
        <div className="shell">
          <span className="section-kicker">05 / Landmark Objectives</span>
          <div className="section-head">
            <h2>Future Wells.<br /><em>The objective that changes the map.</em></h2>
            <p>
              Future Wells belong to no single faction. Their broken crown, suspended core, and concentric stone rings form a vital strategic landmark. Harvesting, preserving, or reshaping alters both geometry and light.
            </p>
          </div>

          <div className="frame-pair">
            <figure className="evidence-figure">
              <span className="evidence-badge">Celestial Landmark · CONCEPT-016</span>
              <img src="/assets/concepts/echoes-future-well-landmark.webp" alt="Future Well Celestial Landmark" width={1376} height={768} />
              <figcaption>
                <b>Future Well Celestial Landmark</b>
                <span>SPEC-OBJ-002 · Concentric rings & Dawnshard vortex</span>
              </figcaption>
            </figure>
            <figure className="evidence-figure">
              <span className="evidence-badge">Four States Sheet · CONCEPT-002</span>
              <img src="/assets/concepts/future-well-states.webp" alt="Future Well Four States" width={1254} height={1254} />
              <figcaption>
                <b>Future Well Four Action States</b>
                <span>Dormant, Harvest, Preserve, Reshape</span>
              </figcaption>
            </figure>
          </div>

          <div className="choice-grid">
            <article className="detail-card">
              <small>01 / Harvest</small>
              <h3>Take the power now</h3>
              <p>Gain an immediate Dawn reserve to rush advanced research, but collapse the Well and erase the possibility branch forever.</p>
            </article>
            <article className="detail-card">
              <small>02 / Preserve</small>
              <h3>Keep the question alive</h3>
              <p>Earn Dawn continuously over time and expand your sensor network—if you have the defensive force to hold the open ground.</p>
            </article>
            <article className="detail-card">
              <small>03 / Reshape</small>
              <h3>Change the battlefield</h3>
              <p>Manifest a temporary bridge, cover line, or bypass route across a lethal chasm. Both sides receive time to react to the new passage.</p>
            </article>
          </div>
        </div>
      </section>

      {/* SECTION 06: TACTICAL ATLAS & REGIONAL BIOMES */}
      <section id="atlas" className="dark-band">
        <div className="shell">
          <span className="section-kicker">06 / Continental Tactical Atlas</span>
          <div className="section-head">
            <h2>Fifteen battlefields.<br /><em>Six distinct planetary ecologies.</em></h2>
            <p>
              From the vitrified fractures of the Glass Scar to the blinding corona of the Solar Fall Dais, every sector on Soryn carries a distinct geological history, lighting profile, and audio bed.
            </p>
          </div>

          <figure className="evidence-figure" style={{ marginBottom: '32px' }}>
            <span className="evidence-badge">Master Cartography · CONCEPT-006</span>
            <img src="/assets/concepts/soryn-world-map.webp" alt="Continental Cartography of Soryn" width={1376} height={768} />
            <figcaption>
              <b>Soryn Continental Theater & Transit Arteries</b>
              <span>Regional fault lines, ark-city conduits, and sector connections</span>
            </figcaption>
          </figure>

          <div className="detail-grid">
            {biomes.map(b => (
              <article key={b.title} className="detail-card" style={{ padding: '0', overflow: 'hidden' }}>
                <img src={b.img} alt={b.title} style={{ width: '100%', height: '180px', objectFit: 'cover' }} />
                <div style={{ padding: '20px' }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '8px' }}>
                    <small>{b.code}</small>
                    <span style={{ font: '500 9px monospace', color: 'var(--gold)', textTransform: 'uppercase' }}>{b.badge}</span>
                  </div>
                  <h3>{b.title}</h3>
                  <p>{b.text}</p>
                  <div style={{ marginTop: '14px', borderTop: '1px solid rgba(255,255,255,0.08)', paddingTop: '10px' }}>
                    {b.specs.map(s => (
                      <div key={s} style={{ font: '11px monospace', color: 'var(--muted)', marginTop: '4px' }}>• {s}</div>
                    ))}
                  </div>
                </div>
              </article>
            ))}
          </div>

          <div style={{ marginTop: '32px', textAlign: 'center' }}>
            <a className="button outline" href="/archive-static/atlas.html">Explore All 15 Maps on the Tactical Atlas ↗</a>
          </div>
        </div>
      </section>

      {/* SECTION 07: CHARACTERS & HEROES */}
      <section id="characters" className="tint-band">
        <div className="shell">
          <span className="section-kicker">07 / People of Soryn</span>
          <div className="section-head">
            <h2>Five lives.<br /><em>No complete truth.</em></h2>
            <p>
              The war is not driven by simple heroes and villains. It is driven by commanders making defensible choices under existential pressure—and discovering that survival can become a reason to control or erase.
            </p>
          </div>

          <div className="character-grid">
            {characters.map(c => (
              <article key={c.name} className={`character-card ${c.type}`}>
                <div className={`character-avatar ${c.type}`}>{c.code}</div>
                <small>{c.faction} / {c.role}</small>
                <h3>{c.name}</h3>
                <p><strong>Visual Identity:</strong> {c.visual}</p>
                <p><strong>Voice Register:</strong> {c.voice}</p>
                <div className="character-facts">
                  <div>
                    <span className="fact-label">Wants</span>
                    <b>{c.wants}</b>
                  </div>
                  <div>
                    <span className="fact-label">Must Face</span>
                    <b>{c.mustFace}</b>
                  </div>
                </div>
              </article>
            ))}
          </div>

          <div style={{ marginTop: '32px', textAlign: 'center' }}>
            <a className="button outline" href="/archive-static/characters.html">Read Character Identity Dossiers ↗</a>
          </div>
        </div>
      </section>

      {/* SECTION 08: PLANETARY CAMPAIGN & OPERATIONAL DOSSIER */}
      <section id="campaign" className="content-band">
        <div className="shell">
          <span className="section-kicker">08 / Campaign Operations</span>
          <div className="section-head">
            <h2>Fifteen operations.<br /><em>Four possible endings.</em></h2>
            <p>
              The campaign begins with a localized evacuation outside Lume Reach and expands into a planetary struggle over which futures remain real. Decisions carry forward as facts into subsequent missions.
            </p>
          </div>

          <div className="frame-pair">
            <figure className="evidence-figure">
              <span className="evidence-badge">Operations Map · CONCEPT-020</span>
              <img src="/assets/concepts/echoes-campaign-map.webp" alt="Planetary Campaign Operations Map" width={1376} height={768} />
              <figcaption>
                <b>Planetary Campaign Operations Map</b>
                <span>WBP_EchoesCampaignMap · Route nodes & waveforms</span>
              </figcaption>
            </figure>
            <figure className="evidence-figure">
              <span className="evidence-badge">Field Pause Ledger · CONCEPT-021</span>
              <img src="/assets/concepts/echoes-pause-field-ledger.webp" alt="Field Operations Ledger Pause Menu" width={1376} height={768} />
              <figcaption>
                <b>Mara's Field Operations Ledger & Pause Menu</b>
                <span>WBP_EchoesPauseMenu · Telemetry & objectives</span>
              </figcaption>
            </figure>
          </div>

          <div className="frame-pair">
            <figure className="evidence-figure">
              <span className="evidence-badge">Debriefing Curves · CONCEPT-022</span>
              <img src="/assets/concepts/echoes-post-match-debriefing.webp" alt="Post-Match Combat Debriefing" width={1376} height={768} />
              <figcaption>
                <b>Post-Match Combat Debriefing</b>
                <span>WBP_EchoesDebriefing · Interactive telemetry</span>
              </figcaption>
            </figure>
            <figure className="evidence-figure">
              <span className="evidence-badge">Accessibility & Audio · CONCEPT-023</span>
              <img src="/assets/concepts/echoes-settings-accessibility.webp" alt="Settings & Accessibility Terminal" width={1376} height={768} />
              <figcaption>
                <b>Settings & Accessibility Terminal</b>
                <span>WBP_EchoesSettings · -24 LKFS calibration</span>
              </figcaption>
            </figure>
          </div>

          <div style={{ marginTop: '32px', textAlign: 'center' }}>
            <a className="button primary" href="/archive-static/campaign.html">View 15-Mission Campaign Dossier ↗</a>
          </div>
        </div>
      </section>

      {/* FOOTER */}
      <footer className="site-footer">
        <div className="shell footer-inner">
          <div>
            <a className="brand" href="#top">
              <b>✦</b> <span>Echoes <i>of the</i> Broken Sun</span>
            </a>
            <p style={{ marginTop: '8px' }}>A science-fantasy real-time strategy game about the futures we choose to keep.</p>
          </div>
          <div style={{ display: 'flex', gap: '20px', alignItems: 'center' }}>
            <a className="archive-pill" href="/archive-static/index.html">Static Archive Portal</a>
            <span style={{ font: '10px monospace', color: 'var(--muted)' }}>Copyright © 2026 Angelis Pseftis</span>
          </div>
        </div>
      </footer>
    </main>
  );
}

