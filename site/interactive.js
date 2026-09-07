/**
 * Echoes of the Broken Sun - Tactical Interactive Terminal
 * Author: Angelis Pseftis
 * Pure vanilla JavaScript + Web Audio API. Zero external dependencies.
 */

(function () {
  'use strict';

  // --- 1. Tactical Audio Synthesizer (Web Audio API) ---
  const AudioEngine = (function () {
    let ctx = null;
    let muted = true; // Default muted for respectful UX

    // Try to load saved mute preference
    try {
      const saved = localStorage.getItem('ebs_audio_muted');
      if (saved !== null) {
        muted = saved === 'true';
      }
    } catch (e) {}

    function getContext() {
      if (!ctx && (window.AudioContext || window.webkitAudioContext)) {
        ctx = new (window.AudioContext || window.webkitAudioContext)();
      }
      if (ctx && ctx.state === 'suspended') {
        ctx.resume();
      }
      return ctx;
    }

    function isMuted() {
      return muted;
    }

    function setMuted(state) {
      muted = state;
      try {
        localStorage.setItem('ebs_audio_muted', String(muted));
      } catch (e) {}
      updateAudioButtonUI();
    }

    function toggleMute() {
      const newMuted = !muted;
      setMuted(newMuted);
      if (!newMuted) {
        playBeep(880, 0.08, 'sine', 0.15);
      }
    }

    function playBeep(freq = 600, duration = 0.06, type = 'sine', vol = 0.12) {
      if (muted) return;
      try {
        const audioCtx = getContext();
        if (!audioCtx) return;
        const osc = audioCtx.createOscillator();
        const gain = audioCtx.createGain();

        osc.type = type;
        osc.frequency.setValueAtTime(freq, audioCtx.currentTime);

        gain.gain.setValueAtTime(vol, audioCtx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + duration);

        osc.connect(gain);
        gain.connect(audioCtx.destination);

        osc.start();
        osc.stop(audioCtx.currentTime + duration);
      } catch (e) {}
    }

    function playTacticalPing() {
      if (muted) return;
      playBeep(1200, 0.05, 'triangle', 0.1);
      setTimeout(() => playBeep(1800, 0.08, 'sine', 0.08), 40);
    }

    function playFactionChord(faction) {
      if (muted) return;
      try {
        const audioCtx = getContext();
        if (!audioCtx) return;

        let freqs = [440, 660, 880]; // default Meridian cyan
        if (faction === 'kharuun') freqs = [330, 495, 660]; // Amber resonance
        if (faction === 'choir') freqs = [554.37, 740, 932]; // Phase magenta interval

        freqs.forEach((f, i) => {
          setTimeout(() => playBeep(f, 0.18, 'sine', 0.09), i * 50);
        });
      } catch (e) {}
    }

    function playWellPulse(protocol) {
      if (muted) return;
      try {
        const audioCtx = getContext();
        if (!audioCtx) return;

        if (protocol === 'harvest') {
          // Sharp power surge followed by low collapse
          playBeep(300, 0.1, 'sawtooth', 0.12);
          setTimeout(() => playBeep(150, 0.25, 'triangle', 0.15), 90);
        } else if (protocol === 'preserve') {
          // Harmonic sustaining pulse
          playBeep(520, 0.15, 'sine', 0.1);
          setTimeout(() => playBeep(650, 0.22, 'sine', 0.08), 80);
        } else if (protocol === 'reshape') {
          // Frequency-modulating phase shift
          playBeep(440, 0.08, 'square', 0.08);
          setTimeout(() => playBeep(880, 0.15, 'sine', 0.12), 70);
        } else {
          // Dormant idle hum
          playBeep(220, 0.12, 'sine', 0.06);
        }
      } catch (e) {}
    }

    function updateAudioButtonUI() {
      const btn = document.getElementById('audio-toggle-btn');
      if (!btn) return;
      const icon = btn.querySelector('.audio-icon');
      const text = btn.querySelector('.audio-text');
      if (muted) {
        btn.setAttribute('aria-pressed', 'false');
        btn.classList.remove('active');
        if (icon) icon.textContent = '🔇';
        if (text) text.textContent = 'Audio: OFF';
      } else {
        btn.setAttribute('aria-pressed', 'true');
        btn.classList.add('active');
        if (icon) icon.textContent = '🔊';
        if (text) text.textContent = 'Audio: ON';
      }
    }

    return {
      init: function () {
        updateAudioButtonUI();
        const btn = document.getElementById('audio-toggle-btn');
        if (btn) {
          btn.addEventListener('click', () => {
            toggleMute();
          });
        }
      },
      isMuted,
      setMuted,
      playTacticalPing,
      playFactionChord,
      playWellPulse,
      playBeep
    };
  })();

  // --- 2. Broken Sun Solar Particle / Dawnshard Canvas Hero ---
  function initHeroParticles() {
    const canvas = document.getElementById('broken-sun-canvas');
    if (!canvas) return;

    // Check reduced motion
    const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    if (prefersReducedMotion) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let width = (canvas.width = canvas.parentElement.offsetWidth);
    let height = (canvas.height = canvas.parentElement.offsetHeight);

    window.addEventListener('resize', () => {
      if (!canvas.parentElement) return;
      width = canvas.width = canvas.parentElement.offsetWidth;
      height = canvas.height = canvas.parentElement.offsetHeight;
    });

    const particles = [];
    const count = Math.min(45, Math.floor(width / 35));

    for (let i = 0; i < count; i++) {
      particles.push({
        x: Math.random() * width,
        y: Math.random() * height,
        radius: Math.random() * 2.2 + 0.8,
        speedX: (Math.random() - 0.5) * 0.4,
        speedY: -Math.random() * 0.7 - 0.2, // drifting upwards toward the Broken Sun
        alpha: Math.random() * 0.6 + 0.2,
        color: Math.random() > 0.4 ? 'rgba(255, 185, 40, ' : 'rgba(0, 242, 255, ',
        shardShape: Math.random() > 0.6 // Some are geometric shards
      });
    }

    function render() {
      ctx.clearRect(0, 0, width, height);

      particles.forEach((p) => {
        p.y += p.speedY;
        p.x += p.speedX;

        if (p.y < 0) {
          p.y = height + 10;
          p.x = Math.random() * width;
        }
        if (p.x < 0) p.x = width;
        if (p.x > width) p.x = 0;

        ctx.fillStyle = p.color + p.alpha + ')';

        if (p.shardShape) {
          ctx.beginPath();
          ctx.moveTo(p.x, p.y - p.radius * 2);
          ctx.lineTo(p.x + p.radius, p.y);
          ctx.lineTo(p.x, p.y + p.radius * 2);
          ctx.lineTo(p.x - p.radius, p.y);
          ctx.closePath();
          ctx.fill();
        } else {
          ctx.beginPath();
          ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2);
          ctx.fill();
        }
      });

      requestAnimationFrame(render);
    }

    render();
  }

  // --- 3. Interactive Faction War Room ---
  const FactionData = {
    meridian: {
      name: 'Meridian Compact',
      motto: 'Build. Connect. Endure.',
      accent: '#00f2ff',
      accentGlow: 'rgba(0, 242, 255, 0.4)',
      doctrineTitle: 'Logistics Grid & Precision Defense',
      doctrineBody:
        'A culturally plural alliance joined by Dawnshard accounting, civic engineering, and collective defense. Anchors and Power Links generate an interconnected energy web that powers defensive Aegis posts, rapid repair hubs, and long-range rail rifles.',
      quote: '“A choice is not clean because the alternative disappears.” — Lume Reach Ledger',
      structuresImg: 'assets/concepts/echoes-meridian-structures.webp',
      structuresAlt: 'Concept target: Meridian Compact Industrial Structures Architecture Sheet showing Anchor HQ, Power Link Pylon, Array Foundry, and Aegis Post.',
      structuresRegister: 'CONCEPT-017',
      unitsImg: 'assets/concepts/meridian-units.webp',
      unitsAlt: 'Concept target: Meridian Compact Units presentation sheet showing Surveyor, Lancer, Bulwark Team, and Relay Skiff.',
      unitsRegister: 'CONCEPT-001',
      unitList: [
        { name: 'Surveyor', role: 'Engineer & Grid Constructor', desc: 'Authorizes field anchors and power links; repairs damaged frontline structures.' },
        { name: 'Lancer', role: 'Precision Kinetic Rifle', desc: 'Long-range kinetic fire that penetrates soft cover and gains range near active Power Links.' },
        { name: 'Bulwark Team', role: 'Directional Shield Screen', desc: 'Projects interlocking cyan energy barriers that absorb incoming projectiles.' },
        { name: 'Relay Skiff', role: 'Sensor Uplink & Light Support', desc: 'High-speed recon platform extending grid line-of-sight across broken ridges.' }
      ],
      structureList: [
        { name: 'Anchor', role: 'Command & Grid Hub', desc: 'Primary logistic node establishing perimeter grid power and worker production.' },
        { name: 'Power Link', role: 'Energy Pylon Conduit', desc: 'Extends the operational network; speeds unit movement and repairs within conduit bounds.' },
        { name: 'Array Foundry', role: 'Modular Vehicle Factory', desc: 'Fabricates motorized skiffs, mobile artillery, and heavy logistics transports.' },
        { name: 'Aegis Post', role: 'Automated Defense Turret', desc: 'Rapid rail turret that draws auxiliary power from linked anchors to amplify fire rate.' }
      ]
    },
    kharuun: {
      name: 'Kharuun Assemblies',
      motto: 'Move. Adapt. Remember.',
      accent: '#ffb300',
      accentGlow: 'rgba(255, 179, 0, 0.4)',
      doctrineTitle: 'Living Mineral Assemblies & Shared Memory',
      doctrineBody:
        'A civilization of layered personhood, stone carapace warforms, and living ancestral memory. The Kharuun migrate across Soryn with mobile Waystones before rooting near rich mineral veins, cultivating terrain, and reading subterranean seismic vibrations.',
      quote: '“Memory is not a museum; it is the foundation stone that keeps the cavern from caving in.” — Oruun-of-Seven-Stones',
      structuresImg: 'assets/concepts/echoes-kharuun-structures.webp',
      structuresAlt: 'Concept target: Kharuun Assemblies Living Geological Structures Architecture Sheet showing Memory Hearth HQ, Waystone, Growth Basin, and Listening Spine.',
      structuresRegister: 'CONCEPT-018',
      unitsImg: 'assets/concepts/kharuun-units.webp',
      unitsAlt: 'Concept target: Kharuun Assemblies Units presentation sheet showing Tender, Riftstalker, Cairnback, and Resonant.',
      unitsRegister: 'CONCEPT-001',
      unitList: [
        { name: 'Tender', role: 'Symbiotic Harvester', desc: 'Cultivates mineral veins and nurtures Growth Basins to expand assembly biomass.' },
        { name: 'Riftstalker', role: 'Quadruped Seismic Striker', desc: 'Fast quadruped beast equipped with vibrating horn lances to flank enemy lines.' },
        { name: 'Cairnback', role: 'Living Basalt Bastion', desc: 'Massive mobile boulder carapace offering physical projectile cover to trailing units.' },
        { name: 'Resonant', role: 'Seismic Scout & Disruptor', desc: 'Channels ground vibration pulses through basalt soil to expose cloaked threats.' }
      ],
      structureList: [
        { name: 'Memory Hearth', role: 'Assembly Root HQ', desc: 'Ancestral center that records tactical experiences and births new warform strains.' },
        { name: 'Waystone', role: 'Mobile Root Monolith', desc: 'Can uproot and walk across hostile terrain to relocate base camps to fresh resources.' },
        { name: 'Growth Basin', role: 'Biomineral Terrace', desc: 'Cultivates specialized organic mutations, improving armor toughness and unit speed.' },
        { name: 'Listening Spine', role: 'Seismic Detection Needle', desc: 'Drives deep into continental bedrock to detect movement through fog-of-war.' }
      ]
    },
    choir: {
      name: 'Hollow Choir',
      motto: 'The future is not empty.',
      accent: '#e056fd',
      accentGlow: 'rgba(224, 86, 253, 0.4)',
      doctrineTitle: 'Phase Intervals & Probability Synthesis',
      doctrineBody:
        'A linked collective formed from the consciousness of erased causal branches. The Choir manipulates temporal intervals, probability shields, and declared timeline choices. Units flicker through phase space, displacing incoming damage before collapsing into reality.',
      quote: '“You call us ghosts because your calendar only had room for one sun.” — Neme',
      structuresImg: 'assets/concepts/echoes-choir-structures.webp',
      structuresAlt: 'Concept target: Hollow Choir Phase-Uncertain Structures Architecture Sheet showing Concordance Core HQ, Interval Loom, Chorus Loom, and Phase Anchor.',
      structuresRegister: 'CONCEPT-019',
      unitsImg: 'assets/concepts/meridian-units.webp',
      unitsAlt: 'Concept target: Hollow Choir temporal combatants and phase skirmishers.',
      unitsRegister: 'CONCEPT-026',
      unitList: [
        { name: 'Threadkeeper', role: 'Probability Weaver', desc: 'Manipulates luminous thread strands to synthesize buildings directly from phase intervals.' },
        { name: 'Intervalist', role: 'Phase Skirmisher', desc: 'Teleports short distances and fires phase-shifted needle bolts that bypass kinetic armor.' },
        { name: 'Lacuna Warden', role: 'Folded Obsidian Anchor', desc: 'Emits a localized temporal dampening field that slows enemy attack frequencies.' },
        { name: 'Afterimage', role: 'Temporal Echo Scout', desc: 'Leaves a false sensor signature behind while scouting enemy forward bases unseen.' }
      ],
      structureList: [
        { name: 'Concordance', role: 'Core Timeline HQ', desc: 'Maintains localized timeline coherence and anchors all choir units in current reality.' },
        { name: 'Interval Loom', role: 'Phase Energy Pylon', desc: 'Harvests probability flux from unactualized timelines to supply building energy.' },
        { name: 'Chorus Loom', role: 'Synthesis Basin', desc: 'Folds phase matter into physical form, rapidly constructing advanced combatants.' },
        { name: 'Phase Anchor', role: 'Spatial Distortion Beacon', desc: 'Disrupts enemy targeting and provides temporal recall for threatened units.' }
      ]
    }
  };

  function initFactionWarRoom() {
    const warRoom = document.getElementById('faction-war-room');
    if (!warRoom) return;

    const tabs = warRoom.querySelectorAll('.faction-tab');
    let currentFaction = 'meridian';

    function setFaction(factionKey) {
      if (!FactionData[factionKey]) return;
      currentFaction = factionKey;
      const data = FactionData[factionKey];

      tabs.forEach((tab) => {
        const isActive = tab.getAttribute('data-faction') === factionKey;
        tab.classList.toggle('active', isActive);
        tab.setAttribute('aria-selected', isActive ? 'true' : 'false');
      });

      warRoom.setAttribute('data-active-faction', factionKey);
      document.documentElement.style.setProperty('--current-faction-accent', data.accent);
      document.documentElement.style.setProperty('--current-faction-glow', data.accentGlow);

      const nameEl = document.getElementById('faction-display-name');
      const mottoEl = document.getElementById('faction-display-motto');
      const docTitleEl = document.getElementById('faction-doctrine-title');
      const docBodyEl = document.getElementById('faction-doctrine-body');
      const quoteEl = document.getElementById('faction-quote-text');

      if (nameEl) nameEl.textContent = data.name;
      if (mottoEl) mottoEl.textContent = data.motto;
      if (docTitleEl) docTitleEl.textContent = data.doctrineTitle;
      if (docBodyEl) docBodyEl.textContent = data.doctrineBody;
      if (quoteEl) quoteEl.textContent = data.quote;

      const structFig = document.getElementById('faction-structures-figure');
      if (structFig) {
        structFig.setAttribute('data-register', data.structuresRegister);
        const img = structFig.querySelector('img');
        const source = structFig.querySelector('source');
        if (img) {
          img.src = data.structuresImg;
          img.alt = data.structuresAlt;
        }
        if (source) {
          source.srcset = data.structuresImg;
        }
        const capTitle = structFig.querySelector('figcaption b');
        if (capTitle) capTitle.textContent = `${data.name} Architecture Target`;
      }

      const unitGrid = document.getElementById('faction-units-grid');
      if (unitGrid) {
        unitGrid.innerHTML = data.unitList
          .map(
            (u) => `
          <div class="roster-card">
            <div class="roster-card-header">
              <span class="roster-name">${u.name}</span>
              <span class="roster-role">${u.role}</span>
            </div>
            <p class="roster-desc">${u.desc}</p>
          </div>
        `
          )
          .join('');
      }

      const structGrid = document.getElementById('faction-structures-grid');
      if (structGrid) {
        structGrid.innerHTML = data.structureList
          .map(
            (s) => `
          <div class="roster-card">
            <div class="roster-card-header">
              <span class="roster-name">${s.name}</span>
              <span class="roster-role">${s.role}</span>
            </div>
            <p class="roster-desc">${s.desc}</p>
          </div>
        `
          )
          .join('');
      }

      AudioEngine.playFactionChord(factionKey);
    }

    tabs.forEach((tab) => {
      tab.addEventListener('click', () => {
        const f = tab.getAttribute('data-faction');
        if (f !== currentFaction) {
          setFaction(f);
        }
      });
    });

    setFaction('meridian');
  }

  // --- 4. Interactive Future Well Tactical Simulator ---
  const WellStates = {
    dormant: {
      name: 'Dormant Alignment',
      kicker: 'State 00 // Unresolved Possibility',
      coreColor: '#ffd700',
      glowRadius: '12px',
      ringSpeed: '40s',
      yieldText: '0 Dawn/sec (Neutral)',
      tacticalEffect: 'Latent landmark. Perimeter contested. No faction holds causal authority.',
      loreNote: 'Concentric stone rings revolve around a suspended obsidian shard. The possibility is asleep.',
      conceptCompareImg: 'assets/concepts/echoes-future-well-landmark.webp',
      conceptCompareAlt: 'Concept target: Future Well Celestial Battle Landmark under the Broken Sun.',
      engineCompareImg: 'assets/engine/future-well-dormant-in-engine.webp',
      engineCompareAlt: 'Current in-engine capture: Future Well in Dormant state, dual rotating rings around central core.',
      audioPulse: 'dormant'
    },
    harvest: {
      name: 'Harvest Protocol',
      kicker: 'State 01 // Total Collapse For Power',
      coreColor: '#ff4444',
      glowRadius: '24px',
      ringSpeed: '12s',
      yieldText: '+1,200 Dawn (Instant Lump Sum)',
      tacticalEffect: 'Permanent destruction of the Well. High immediate war economy boost. The sector possibility branch is permanently erased.',
      loreNote: 'The core fractures, shattering the shard. The surrounding terrain vitrifies as lost futures burn away for immediate military leverage.',
      conceptCompareImg: 'assets/concepts/future-well-states.webp',
      conceptCompareAlt: 'Concept target: Future Well 4-state sheet showing Harvest destruction.',
      engineCompareImg: 'assets/engine/future-well-harvest-in-engine.webp',
      engineCompareAlt: 'Current in-engine capture: Future Well in Harvest state, cracked core with expelled radiant energy.',
      audioPulse: 'harvest'
    },
    preserve: {
      name: 'Preserve Protocol',
      kicker: 'State 02 // Enduring Siphon & Sensor Grid',
      coreColor: '#00f2ff',
      glowRadius: '20px',
      ringSpeed: '22s',
      yieldText: '+45 Dawn/sec + Wide Fog-of-War Radar',
      tacticalEffect: 'Continuous strategic income and continuous sector radar sweep as long as defensive troops maintain the perimeter.',
      loreNote: 'Resonant harmonic field stabilized. The shard floats peacefully inside an energy vortex, rewarding territorial patience.',
      conceptCompareImg: 'assets/concepts/echoes-future-well-landmark.webp',
      conceptCompareAlt: 'Concept target: Preserved Future Well bathed in cyan energy rings.',
      engineCompareImg: 'assets/engine/future-well-preserve-in-engine.webp',
      engineCompareAlt: 'Current in-engine capture: Future Well in Preserve state, stable cyan conduits and orbit ring.',
      audioPulse: 'preserve'
    },
    reshape: {
      name: 'Reshape Protocol',
      kicker: 'State 03 // Causal Battlefield Manipulation',
      coreColor: '#e056fd',
      glowRadius: '28px',
      ringSpeed: '8s',
      yieldText: '-300 Dawn (Spent) -> Geometry Manifested',
      tacticalEffect: 'Brings an unmade bridge, trench, or cliff barrier into physical reality for 180 seconds. Enemies receive audio warning.',
      loreNote: 'The boundary between what is and what could have been is dissolved. Matter re-aligns to grant sudden tactical elevation or bypass chokepoints.',
      conceptCompareImg: 'assets/concepts/future-well-states.webp',
      conceptCompareAlt: 'Concept target: Reshape state showing displaced stone causeways.',
      engineCompareImg: 'assets/engine/future-well-reshape-in-engine.webp',
      engineCompareAlt: 'Current in-engine capture: Future Well in Reshape state, purple phase flare with floating rock strata.',
      audioPulse: 'reshape'
    }
  };

  function initWellSimulator() {
    const sim = document.getElementById('well-simulator');
    if (!sim) return;

    const btns = sim.querySelectorAll('.well-ctrl-btn');
    let currentState = 'dormant';

    function setWellState(stateKey) {
      if (!WellStates[stateKey]) return;
      currentState = stateKey;
      const data = WellStates[stateKey];

      btns.forEach((b) => {
        const isSelected = b.getAttribute('data-well-state') === stateKey;
        b.classList.toggle('active', isSelected);
        b.setAttribute('aria-pressed', isSelected ? 'true' : 'false');
      });

      const titleEl = document.getElementById('well-state-title');
      const kickerEl = document.getElementById('well-state-kicker');
      const yieldEl = document.getElementById('well-yield-value');
      const effectEl = document.getElementById('well-tactical-effect');
      const loreEl = document.getElementById('well-lore-note');
      const coreCircle = document.getElementById('well-visual-core');

      if (titleEl) titleEl.textContent = data.name;
      if (kickerEl) kickerEl.textContent = data.kicker;
      if (yieldEl) yieldEl.textContent = data.yieldText;
      if (effectEl) effectEl.textContent = data.tacticalEffect;
      if (loreEl) loreEl.textContent = data.loreNote;

      if (coreCircle) {
        coreCircle.style.background = `radial-gradient(circle, ${data.coreColor} 0%, rgba(10,15,25,0.9) 70%)`;
        coreCircle.style.boxShadow = `0 0 ${data.glowRadius} ${data.coreColor}`;
      }

      const engineImg = document.getElementById('well-engine-capture-img');
      const conceptImg = document.getElementById('well-concept-capture-img');

      if (engineImg) {
        engineImg.src = data.engineCompareImg;
        engineImg.alt = data.engineCompareAlt;
        const source = engineImg.parentElement.querySelector('source');
        if (source) source.srcset = data.engineCompareImg;
      }
      if (conceptImg) {
        conceptImg.src = data.conceptCompareImg;
        conceptImg.alt = data.conceptCompareAlt;
        const source = conceptImg.parentElement.querySelector('source');
        if (source) source.srcset = data.conceptCompareImg;
      }

      AudioEngine.playWellPulse(data.audioPulse);
    }

    btns.forEach((b) => {
      b.addEventListener('click', () => {
        const s = b.getAttribute('data-well-state');
        if (s !== currentState) {
          setWellState(s);
        }
      });
    });

    setWellState('dormant');
  }

  // --- 5. Interactive Tactical Atlas & Planetary Radar ---
  const AtlasRegions = {
    'glass-scar': {
      name: 'The Glass Scar',
      operation: 'Mission 01: What the Ledger Keeps',
      biome: 'Vitrified Chasm & Ash Flats',
      threat: 'ELEVATED // MIGRATION CONFLICT',
      control: 'Contested (Meridian Evacuation vs Kharuun Vanguard)',
      conceptImg: 'assets/concepts/target-render-vertical-slice.webp',
      conceptAlt: 'Concept target: Glass Scar vertical slice at dusk with Future Well center.',
      register: 'CONCEPT-004',
      briefing:
        'A fractured obsidian transit span outside Lume Reach. As civic reserves collapse, Commander Mara Vey coordinates civilian evacuation while Talar Venn attempts to recover missing archive canisters from the census void.',
      coordinates: '34°12′N, 118°04′E // ELEV: -120m VITRIFIED SECTOR'
    },
    shivergrass: {
      name: 'Shivergrass Basin',
      operation: 'Mission 02: Seven Accounts of Rain',
      biome: 'Probability-Sensitive Steppes',
      threat: 'SEVERE // SEISMIC FAULTLINES',
      control: 'Kharuun Assemblies Ancestral Territory',
      conceptImg: 'assets/concepts/shivergrass-basin.webp',
      conceptAlt: 'Concept target: Shivergrass Basin with rippling amber steppes and grazing Vaultback megafauna.',
      register: 'CONCEPT-007',
      briefing:
        'Rolling steppes where probability-sensitive vegetation ripples like liquid amber before artillery impacts. Massive Vaultback megafauna carry ancient minerals, creating natural mobile high-ground.',
      coordinates: '51°20′N, 89°14′W // ELEV: +430m STEPPE BASIN'
    },
    'unburied-road': {
      name: 'The Unburied Road',
      operation: 'Mission 04: The Unburied Road',
      biome: 'Subterranean Bioluminescent Geodes',
      threat: 'EXTREME // CHOKEPOINT TUNNELS',
      control: 'Sub-Basalt Deep Caverns',
      conceptImg: 'assets/concepts/unburied-road-caverns.webp',
      conceptAlt: 'Concept target: Subterranean geode transit span with glowing Matter crystals.',
      register: 'CONCEPT-008',
      briefing:
        'Ancient pre-Crownfall transport tunnels cutting deep beneath the volcanic crust. Massive clusters of cyan Matter crystals illuminate narrow bridges spanning bottomless chasms.',
      coordinates: '12°08′S, 44°33′E // ELEV: -1,850m DEEP ARTERY'
    },
    'ark-city': {
      name: 'Ark-City Sector 9',
      operation: 'Mission 06: Names Without Births',
      biome: 'Brutalist Monolithic Sheared Foundries',
      threat: 'CRITICAL // FOUNDATION SUBSIDENCE',
      control: 'Meridian Municipal Authority',
      conceptImg: 'assets/concepts/arkcity-census-void.webp',
      conceptAlt: 'Concept target: Sheared industrial towers over the gaping census void.',
      register: 'CONCEPT-009',
      briefing:
        'Gigantic concrete silos and modular foundries suspended above a sheared continental abyss. Talar Venn tracks missing civic registries that reveal unrecorded populations omitted during the Crownfall.',
      coordinates: '64°15′N, 172°40′W // ELEV: +1,100m INDUSTRIAL ARCH'
    },
    'solar-dais': {
      name: 'Solar Fall Dais',
      operation: 'Mission 15: The Broken Sun',
      biome: 'Sub-Solar Orbital Obsidian Altar',
      threat: 'EXISTENTIAL // CAUSAL SINGULARITY',
      control: 'The Crown Singularity',
      conceptImg: 'assets/concepts/broken-sun-solar-dais.webp',
      conceptAlt: 'Concept target: Floating geometric dais beneath the blinding golden coronary fracture.',
      register: 'CONCEPT-010',
      briefing:
        'The geometric epicenter where the Crownfall pierced Soryn’s sky. Suspended directly beneath the blinding solar fracture, all three factions meet in a decisive final operation to determine which future endures.',
      coordinates: '00°00′N, 00°00′E // ELEV: +12,400m CORONAL SUMMIT'
    }
  };

  function initTacticalAtlas() {
    const atlas = document.getElementById('tactical-atlas-terminal');
    if (!atlas) return;

    const sectorButtons = atlas.querySelectorAll('.atlas-sector-btn');
    let currentRegion = 'glass-scar';

    function setAtlasRegion(regionKey) {
      if (!AtlasRegions[regionKey]) return;
      currentRegion = regionKey;
      const data = AtlasRegions[regionKey];

      sectorButtons.forEach((btn) => {
        const isSelected = btn.getAttribute('data-sector') === regionKey;
        btn.classList.toggle('active', isSelected);
        btn.setAttribute('aria-pressed', isSelected ? 'true' : 'false');
      });

      const nameEl = document.getElementById('atlas-region-name');
      const opEl = document.getElementById('atlas-operation-title');
      const biomeEl = document.getElementById('atlas-biome-desc');
      const threatEl = document.getElementById('atlas-threat-level');
      const controlEl = document.getElementById('atlas-control-status');
      const briefEl = document.getElementById('atlas-briefing-text');
      const coordEl = document.getElementById('atlas-coords-text');

      if (nameEl) nameEl.textContent = data.name;
      if (opEl) opEl.textContent = data.operation;
      if (biomeEl) biomeEl.textContent = data.biome;
      if (threatEl) threatEl.textContent = data.threat;
      if (controlEl) controlEl.textContent = data.control;
      if (briefEl) briefEl.textContent = data.briefing;
      if (coordEl) coordEl.textContent = data.coordinates;

      const fig = document.getElementById('atlas-concept-figure');
      if (fig) {
        fig.setAttribute('data-register', data.register);
        const img = fig.querySelector('img');
        const src = fig.querySelector('source');
        if (img) {
          img.src = data.conceptImg;
          img.alt = data.conceptAlt;
        }
        if (src) src.srcset = data.conceptImg;
        const cap = fig.querySelector('figcaption b');
        if (cap) cap.textContent = `${data.name} Visual Target`;
      }

      AudioEngine.playTacticalPing();
    }

    sectorButtons.forEach((btn) => {
      btn.addEventListener('click', () => {
        const reg = btn.getAttribute('data-sector');
        if (reg !== currentRegion) {
          setAtlasRegion(reg);
        }
      });
    });

    setAtlasRegion('glass-scar');
  }

  // --- 6. Interactive Character Command Dossiers ---
  const Characters = {
    'mara-vey': {
      name: 'Commander Mara Vey',
      faction: 'Meridian Compact',
      role: 'Field Commander & Operations Authority',
      accentColor: '#00f2ff',
      crestCode: 'MV',
      visualSummary: 'Mid-30s commander in pale ceramic and graphite field armor with exposed brass load joints and glowing cyan conduit diagnostics.',
      voiceProfile: 'Measured pace, clipped sentence ends under combat load; anxiety registers as razor-sharp procedural precision (af_sarah @1.0).',
      wants: 'A survivable, predictable future for Soryn through disciplined preparation, fortified logistics, and clear accountability.',
      mustFace: 'Turning every moral question into a control problem risks making erasure of inconvenient people feel like administrative necessity.',
      quote: '“If we cannot measure the reserve, we cannot guarantee the evacuation. Order is not pride—it is the difference between survival and slaughter.”'
    },
    'oruun': {
      name: 'Oruun-of-Seven-Stones',
      faction: 'Kharuun Assemblies',
      role: 'Memory-Bearer & Carapace Custodian',
      accentColor: '#ffb300',
      crestCode: 'O7',
      visualSummary: 'Towering mineral-organic humanoid with dark basalt plates and pale sedimentary carapace; seven amber memory stones embedded across chest.',
      voiceProfile: 'Deep seismic register, unhurried and resonant; sharp dry wit emerging from centuries of accumulated ancestral contradictions (bm_george @0.92).',
      wants: 'Unbroken communal memory, protection of the deep birthing caverns, and accountability for ancestral agreements.',
      mustFace: 'Some historic eras of peace were bought by the deliberate omission of inconvenient atrocities from the ancestral stone ledgers.',
      quote: '“The stone remembers the fire long after the ash has cooled. You cannot build a new road by claiming the mountain never stood there.”'
    },
    'talar-venn': {
      name: 'Talar Venn',
      faction: 'Meridian Compact',
      role: 'Keeper of the Archive Convoy',
      accentColor: '#5fd8e8',
      crestCode: 'TV',
      visualSummary: 'Late-20s field archivist wearing rugged logistics webbing strapped with physical data canisters, leather ledger folios, and field stylus.',
      voiceProfile: 'Earnest, hurried, emotionally transparent; the singular voice in the opening campaign operations permitted to sound genuinely frightened (am_michael @1.0).',
      wants: 'To locate and preserve the missing census names from Sector 9 before institutional convenience erases them a second time.',
      mustFace: 'Truth lacks the physical power to defend itself when hungry armies demand immediate fuel and ammunition.',
      quote: '“Names are not extra baggage. If we evacuate only the factories and leave the people’s records to burn, who exactly did we save?”'
    },
    'cael-rhyse': {
      name: 'Chancellor Cael Rhyse',
      faction: 'Meridian Compact',
      role: 'Civic Chancellor & State Architect',
      accentColor: '#ffdf70',
      crestCode: 'CR',
      visualSummary: 'Distinguished statesman in his 50s, wearing an architectural civic mantle of tailored ivory and graphite with gold trim and the Compact seal.',
      voiceProfile: 'Polished diplomatic warmth over cold administrative finality; persuasive, paternal, and utterly uncompromising in statecraft (bm_lewis @0.95).',
      wants: 'One unified, governable future for the shattered continent of Soryn under a single sustainable logistics framework.',
      mustFace: 'Stability imposed from above inevitably denies the moral right of other societies and emergent cultures to exist.',
      quote: '“A thousand competing futures produce only war. History requires one spine, one ledger, and one hand on the helm.”'
    },
    'neme': {
      name: 'Neme',
      faction: 'Hollow Choir',
      role: 'Several Possible Selves',
      accentColor: '#e056fd',
      crestCode: 'NE',
      visualSummary: 'Translucent phase-shifted silhouette of obsidian crystalline lattice, trailing duplicate afterimages and violet chromatic diffraction.',
      voiceProfile: 'Even, cool, multifaceted articulation; clarity maintained because imprecision could allow one competing timeline to dominate (af_nicole @0.95).',
      wants: 'Coexistence without total erasure; granting the unmade futures their rightful presence in the realized world.',
      mustFace: 'Its own component futures do not agree on whether peaceful coexistence with the living timeline is truly possible or a slow death.',
      quote: '“We were not born from nothing. We are the daughters and sons of the days your sun decided not to rise.”'
    }
  };

  function initCharacterDossiers() {
    const terminal = document.getElementById('character-dossier-terminal');
    if (!terminal) return;

    const tabs = terminal.querySelectorAll('.character-tab');
    let currentHero = 'mara-vey';

    function setHero(heroKey) {
      if (!Characters[heroKey]) return;
      currentHero = heroKey;
      const data = Characters[heroKey];

      tabs.forEach((tab) => {
        const isSelected = tab.getAttribute('data-hero') === heroKey;
        tab.classList.toggle('active', isSelected);
        tab.setAttribute('aria-selected', isSelected ? 'true' : 'false');
      });

      const nameEl = document.getElementById('hero-dossier-name');
      const factionEl = document.getElementById('hero-dossier-faction');
      const roleEl = document.getElementById('hero-dossier-role');
      const crestEl = document.getElementById('hero-crest-code');
      const visualEl = document.getElementById('hero-visual-spec');
      const voiceEl = document.getElementById('hero-voice-spec');
      const wantsEl = document.getElementById('hero-wants-spec');
      const mustFaceEl = document.getElementById('hero-mustface-spec');
      const quoteEl = document.getElementById('hero-quote-text');

      if (nameEl) nameEl.textContent = data.name;
      if (factionEl) {
        factionEl.textContent = data.faction;
        factionEl.style.color = data.accentColor;
      }
      if (roleEl) roleEl.textContent = data.role;
      if (crestEl) crestEl.textContent = data.crestCode;
      if (visualEl) visualEl.textContent = data.visualSummary;
      if (voiceEl) voiceEl.textContent = data.voiceProfile;
      if (wantsEl) wantsEl.textContent = data.wants;
      if (mustFaceEl) mustFaceEl.textContent = data.mustFace;
      if (quoteEl) quoteEl.textContent = data.quote;

      AudioEngine.playTacticalPing();
    }

    tabs.forEach((tab) => {
      tab.addEventListener('click', () => {
        const h = tab.getAttribute('data-hero');
        if (h !== currentHero) {
          setHero(h);
        }
      });
    });

    setHero('mara-vey');
  }

  // --- 7. Interactive Interface Comparison Lab ---
  function initInterfaceLab() {
    const lab = document.getElementById('interface-comparison-lab');
    if (!lab) return;

    const hudButtons = lab.querySelectorAll('.lab-hud-toggle');
    const hudModern = document.getElementById('lab-hud-modern');
    const hudClassic = document.getElementById('lab-hud-classic');

    hudButtons.forEach((btn) => {
      btn.addEventListener('click', () => {
        const target = btn.getAttribute('data-hud-target');
        hudButtons.forEach((b) => {
          const isMatch = b === btn;
          b.classList.toggle('active', isMatch);
          b.setAttribute('aria-pressed', isMatch ? 'true' : 'false');
        });

        if (target === 'modern') {
          if (hudModern) hudModern.style.display = 'block';
          if (hudClassic) hudClassic.style.display = 'none';
        } else {
          if (hudModern) hudModern.style.display = 'none';
          if (hudClassic) hudClassic.style.display = 'block';
        }
        AudioEngine.playTacticalPing();
      });
    });

    const menuButtons = lab.querySelectorAll('.lab-menu-toggle');
    const menuBridge = document.getElementById('lab-menu-bridge');
    const menuClassic = document.getElementById('lab-menu-classic');

    menuButtons.forEach((btn) => {
      btn.addEventListener('click', () => {
        const target = btn.getAttribute('data-menu-target');
        menuButtons.forEach((b) => {
          const isMatch = b === btn;
          b.classList.toggle('active', isMatch);
          b.setAttribute('aria-pressed', isMatch ? 'true' : 'false');
        });

        if (target === 'bridge') {
          if (menuBridge) menuBridge.style.display = 'block';
          if (menuClassic) menuClassic.style.display = 'none';
        } else {
          if (menuBridge) menuBridge.style.display = 'none';
          if (menuClassic) menuClassic.style.display = 'block';
        }
        AudioEngine.playTacticalPing();
      });
    });
  }

  // --- Initializer ---
  document.addEventListener('DOMContentLoaded', () => {
    AudioEngine.init();
    initHeroParticles();
    initFactionWarRoom();
    initWellSimulator();
    initTacticalAtlas();
    initCharacterDossiers();
    initInterfaceLab();
  });
})();
