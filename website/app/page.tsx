'use client';

import { useState } from 'react';

const factions = [
  { name: 'Meridian Compact', tag: 'Build. Connect. Endure.', accent: 'teal', text: 'A coalition of ark-cities that turns engineering, logistics, and precision fire into a fortress that moves outward.', units: ['Surveyor', 'Lancer', 'Bulwark Team', 'Relay Skiff'] },
  { name: 'Kharuun Assemblies', tag: 'Move. Adapt. Remember.', accent: 'violet', text: 'Living assemblies that grow their war machines into the terrain, sense movement through stone, and change the shape of a fight.', units: ['Tender', 'Riftstalker', 'Cairnback', 'Resonant'] },
  { name: 'Hollow Choir', tag: 'The future is not empty.', accent: 'rose', text: 'A later playable faction born from erased possibilities. It controls temporary states, competing outcomes, and the space between what is and what might be.', units: ['Coming later'] },
];

const wells = [
  ['Harvest', 'Take the power now.', 'Gain an immediate Dawn reserve, but collapse the Well and erase the possibility inside it forever.'],
  ['Preserve', 'Keep the question alive.', 'Earn Dawn over time and expand your intelligence—if you can hold the ground.'],
  ['Reshape', 'Change the battlefield.', 'Spend Dawn to manifest a temporary bridge, cover line, or route. Both sides get time to react.'],
];

export default function Home() {
  const [activeFaction, setActiveFaction] = useState(0);
  const [menuOpen, setMenuOpen] = useState(false);

  return (
    <main>
      <nav className="nav shell">
        <a className="brand" href="#top"><span className="brand-mark">✦</span><span>Echoes <i>of the</i> Broken Sun</span></a>
        <div className={`nav-links ${menuOpen ? 'open' : ''}`}>
          <a href="#world" onClick={() => setMenuOpen(false)}>The world</a><a href="#factions" onClick={() => setMenuOpen(false)}>Factions</a><a href="#systems" onClick={() => setMenuOpen(false)}>How it plays</a><a href="#roadmap" onClick={() => setMenuOpen(false)}>Roadmap</a>
        </div>
        <button className="menu-button" onClick={() => setMenuOpen(!menuOpen)} aria-label="Toggle navigation">☰</button>
      </nav>

      <section id="top" className="hero">
        <div className="hero-art" /><div className="hero-glow" />
        <div className="hero-copy shell">
          <p className="eyebrow"><span className="pulse" /> A science-fantasy RTS in development</p>
          <h1>Every future<br /><em>has a cost.</em></h1>
          <p className="hero-lede">Soryn is a broken world where lost possibilities still burn beneath the ground. Build a force, lead an evacuation, and decide what deserves to survive.</p>
          <div className="hero-actions"><a className="button primary" href="#world">Enter the archive <span>↗</span></a><a className="text-link" href="#roadmap">Follow development <span>↓</span></a></div>
          <p className="status-note"><strong>Development note</strong> The game is not playable yet. Current builds use placeholder shapes while the systems and world take form.</p>
        </div>
        <div className="hero-stamp">CROWNFALL<br /><span>FIELD RECORD 01</span></div>
      </section>

      <section className="intro shell" id="world"><div className="section-kicker">01 / The world</div><div className="split"><div><h2>The sun broke.<br /><em>The future stayed.</em></h2></div><div className="intro-copy"><p>When the Crownfall tore across Soryn, it fractured more than the sky. It left behind <strong>Dawnshards</strong>—mineral-organic remnants holding technologies, cities, and lives that never happened.</p><p>Future Wells gather those possibilities in one place. They can power a city, open a lost route, or erase an entire branch of history. Three factions now fight over the same impossible resource, each with a different idea of what “saving the world” means.</p></div></div><div className="quote">“A choice is not clean because the alternative disappears.”<span>— Fragment from the Lume Reach ledger</span></div></section>

      <section className="dark-section" id="factions"><div className="shell"><div className="section-kicker light">02 / The factions</div><div className="faction-intro"><div><h2>Three answers<br />to one question.</h2></div><p>No side is simply good or evil. The Compact wants certainty. The Assemblies protect memory. The Choir wants the erased to count. On the battlefield, those beliefs become completely different ways to play.</p></div><div className="faction-tabs">{factions.map((f, i) => <button key={f.name} className={`${activeFaction === i ? 'active' : ''} ${f.accent}`} onClick={() => setActiveFaction(i)}><span>0{i + 1}</span>{f.name}</button>)}</div><div className={`faction-card ${factions[activeFaction].accent}`}><div className="faction-symbol">{activeFaction === 0 ? '⌁' : activeFaction === 1 ? '◈' : '◎'}</div><div><p className="card-tag">{factions[activeFaction].tag}</p><h3>{factions[activeFaction].name}</h3><p>{factions[activeFaction].text}</p><div className="unit-row">{factions[activeFaction].units.map(u => <span key={u}>{u}</span>)}</div></div><span className="card-number">0{activeFaction + 1}</span></div></div></section>

      <section className="systems shell" id="systems"><div className="section-kicker">03 / How it plays</div><div className="systems-head"><h2>Strategy with<br /><em>consequences.</em></h2><p>Classic real-time strategy foundations—workers, scouting, bases, combat, territory—meet a world that remembers what you chose not to save.</p></div><div className="well-grid">{wells.map((w, i) => <article className={`well well-${i}`} key={w[0]}><span className="well-number">0{i + 1}</span><div className="well-orbit">✦</div><h3>{w[0]}</h3><p className="well-tag">{w[1]}</p><p>{w[2]}</p></article>)}</div><div className="resource-strip"><div><span className="resource-icon matter">◆</span><strong>Matter</strong><small>Build bodies and structures</small></div><div><span className="resource-icon dawn">✦</span><strong>Dawn</strong><small>Power advanced decisions</small></div><div><span className="resource-icon logistics">⟲</span><strong>Logistics</strong><small>Keep your network alive</small></div></div></section>

      <section className="story-section"><div className="shell story-grid"><div><div className="section-kicker light">04 / The first operation</div><h2>The Glass Scar</h2><p className="story-lede">A fractured transit span. A failing city reserve. A Future Well that could save Lume Reach—or remove the only future where it survives.</p><a className="button outline" href="#roadmap">Read the story so far <span>↗</span></a></div><div className="story-card"><span className="story-label">PROLOGUE / WHAT THE LEDGER KEEPS</span><p>Commander Mara Vey leads an evacuation outside Lume Reach. Talar Venn is trying to recover an archive convoy. Oruun-of-Seven-Stones arrives to stop the Well’s collapse from reaching a Kharuun birthing cavern.</p><p>The mission is not about total destruction. It is about what you protect, what you spend, and whether anyone can leave before the choice becomes permanent.</p></div></div></section>

      <section className="roadmap shell" id="roadmap"><div className="section-kicker">05 / Where we are going</div><div className="roadmap-head"><h2>Built in the open.<br /><em>Shaped with care.</em></h2><p>We are building the foundation first: a fair, readable strategy game with a world worth returning to. The visuals come next.</p></div><div className="timeline"><div className="timeline-item current"><span className="dot" /><div><small>NOW / FOUNDATION</small><h3>Systems prototype</h3><p>Core simulation, economy, combat, fog, factions, Future Wells, and the first Glass Scar operation are being assembled with placeholder geometry.</p></div></div><div className="timeline-item"><span className="dot" /><div><small>NEXT / VERTICAL SLICE</small><h3>A world you can see</h3><p>Finalize the first playable slice with terrain, readable unit silhouettes, sound, UI polish, and the complete prologue experience.</p></div></div><div className="timeline-item"><span className="dot" /><div><small>AFTER / THE CAMPAIGN</small><h3>Three futures, one war</h3><p>Expand the campaign, bring the Hollow Choir into the field, and let your decisions reshape the fate of Soryn.</p></div></div></div></section>

      <footer><div className="shell footer-inner"><a className="brand" href="#top"><span className="brand-mark">✦</span><span>Echoes <i>of the</i> Broken Sun</span></a><p>A game about the futures we choose to keep.</p><span className="footer-note">Concept / prototype phase</span></div></footer>
    </main>
  );
}
