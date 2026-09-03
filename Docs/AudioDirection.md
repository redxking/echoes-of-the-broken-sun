# Audio Direction — Echoes of the Broken Sun

**Author and owner:** Angelis Pseftis
**Status:** authoritative audio-direction document, edited in place. Subordinate to
`Docs/Archive/DevelopmentBible.md` (canon) and `Docs/GameCompletionDirective.md` (Tracks B and C, the
work orders and gates). Where this document states a target the directive does not fix, that target is
an authored design decision of this document and is marked *(authored here)*; it binds future audio work
until revised, but is not evidence of anything implemented. Nothing in this document claims a cue,
route, or mix is accepted — acceptance lives only in `Docs/Archive/ProjectLedger.md`.

Every audio decision — new cue, revision, routing change, mix move, or voice direction — must be
checkable against this document. A cue that cannot be justified by a section below is not ready to
synthesize.

---

## 1. The master sonic identity

Soryn lives under a shattered sun. The whole soundscape descends from one idea: **futures that were
real enough to be heard, cut off before they resolved.**

- **The fracture motif.** The Broken Sun's signature is a harmonic series with missing members — a
  fundamental whose expected partials are displaced or absent, so the ear keeps reaching for a
  resolution that never fully arrives. Shard chimes, the Crownfall sky, Well transitions, and the
  Choir's material all draw on this motif. It is used sparingly everywhere else so that it stays
  legible as *the Sun's voice*.
- **Material truth.** Every effect sounds like the substance the design says it is: engineered metal
  and ceramic for the Compact, stone and strata for the Kharuun, phase and interference for the Choir,
  vitrified glass and wind for the Scar. No generic whooshes, no borrowed sci-fi vocabulary.
- **Information first.** Sound is a gameplay channel before it is a mood channel. Every cue carries a
  fact (what happened, whose it was, where it is, how serious it is); beauty is achieved inside that
  constraint, never at its expense.
- **Restraint.** The world is quiet enough that events mean something. Beds sit low; alerts are brief;
  nothing loops attention-seeking material. Loudness is spent, not defaulted.

## 2. Faction music languages

From the Bible, binding: all three factions are legitimate; none is scored as a villain.

| Faction | Instrumental language | Rhythmic identity | Harmonic identity |
|---|---|---|---|
| Meridian Compact | Prepared piano, restrained brass, mechanical resonance | Measured, metronomic pulse — systems maintained under load; subdivisions stay even, tempo never rushes | Functional, cadential harmony that *completes*; strain expressed as suspensions over a stable pulse, not as breakdown |
| Kharuun Assemblies | Resonant stone and ceramic timbres, layered percussion, low sustained resonance | Interlocking cycles of unequal length that phase against each other — communal memory in strata. Never generic "tribal" coding: no war-drum clichés | Modal, accretive; new layers reinterpret rather than replace old ones |
| Hollow Choir | Detuned sustained voices/pads, glass harmonics, the fracture motif | Rhythm is emergent, from interference beating rather than a struck pulse | Progressions that could resolve in more than one direction; commitment is withheld until dramatically earned |

Score architecture (Track B2 cue map): title theme carries all three languages with none dominant;
faction themes are pure statements; act themes are dramatic arcs (Act I urgency held in check, Act II
inquiry and unease, Act III scale and consequence); tension and combat layers exist per faction
*pairing* and crossfade on authoritative combat state without hard cuts; the four ending cues resolve
the Choir's multi-directional harmony four different ways — Restoration warmly but with loss present,
Controlled Stabilization with discipline and unresolved suspension retained, Extinguishment with the
fracture motif closed to silence rather than cadence, Open Evolution ending on genuinely unresolved
material that feels chosen rather than abandoned. **None triumphal by default.**

## 3. Effects: the material-truth table

| Source | Register and texture | Binding rules |
|---|---|---|
| Meridian units/structures | Higher, cleaner; engineered metal, ceramic plate, servo and relay transients | Destruction is an *engineered collapse* — ordered failure, higher pitched than Kharuun (established contract, extend not break) |
| Kharuun units/structures | Lower, warmer; stone mass, strata shear, ceramic resonance with long decay | Destruction is a *ceramic/stone resonance*, lower than Meridian |
| Hollow Choir | Phase, interference, held tones; onset arrives slightly before or after its visual (±60–120 ms, authored per cue) *(authored here)* | Choir sounds never share an envelope shape with either material faction |
| Weapons | Per archetype **and per faction**: the same role reads as kin across factions but the material differs (Compact light = ceramic snap; Kharuun light = stone crack, etc.) | Faction identifiable blind from the fire sound alone |
| Terrain / Glass Scar | Vitrified glass, wind excitation, shard chimes on the fracture motif | Environmental sound never mimics a unit or alert envelope |

Event grammar *(authored here)*: **starts** are rising or opening gestures; **completions** are
resolved, downward-settling gestures; **interruptions/losses** are truncated gestures — the start
gesture audibly cut; **telegraphs** (Reshape open/close, Harvest) are the only long cues in the effects
family and always precede their consequence by their full stated length.

## 4. Ambience site families

Each bed states *place*, never *event*. Target family map (Track B3; existing beds noted, the rest are
required build-out):

| Family | Identity | Status |
|---|---|---|
| Glass Scar | Wind across vitrified glass; sparse shard chimes on the fracture motif | Bed exists (`AMB_GlassScar`), unqualified |
| Lume Reach | Settlement resonance; failing-reserve electrical strain (irregular sag, not hum) | Bed exists, unqualified |
| Ark-city — Life Support | Circulation: layered air movement, deep regular plant pulse | Required; one generic `AMB_ArkCity` stands in |
| Ark-city — Transit | Causeway resonance: long metallic/ceramic sympathetic tones under traffic rhythm | Required |
| Ark-city — Archive | Stillness: near-silence with rare page/servo/settling transients; the quietest bed in the game | Required |
| Migration route | Open wind, shivergrass movement, distant vaultbacks | Required |
| Kharuun interiors | Strata resonance and warmth; slow communal rhythm far below music tempo | Required |
| Choir sites | Held tones and interference beating; the beating rate is the "activity level" | Required |
| Crownfall approach | The sky is audible: deep fracture harmonics **keyed to magenta-fracture intensity** | Bed exists, static; intensity keying required |
| Future Well | Hum keyed to mode — Dormant low, Harvest telegraph rising, Preserve steady, Reshape phase-shifting | One static bed exists; per-mode keying required |

Binding rule: **ambience never masks information.** Beds sit at the bottom of the loudness order
(section 7) and are measured against the masking case in B3's evidence before acceptance.

## 5. Interface and alert grammar

- Interface cues are non-spatial, short (≤ 250 ms except brief/menu transitions), and pitched in one
  coherent family so the UI sounds like one instrument *(authored here)*.
- Rejection is always **paired** with the stable visible failure reason; the rejection cue is the most
  distinct interface cue and is never reused for anything else.
- Alerts are brief (≤ 0.8 s), rate-limited (one per class per admission window — implemented at 4 s),
  and semantically pitched *(authored here)*: completion alerts settle downward-resolved
  (production/research complete); threat alerts are pitched tense and unresolved (under attack,
  structure lost); capacity is a patient repeating figure, not an alarm. Every alert has a visible
  counterpart and routes to `Space` jump-to-alert.
- Alerts must remain identifiable over full combat plus bed — this is the B6 readability case.

## 6. Voice direction

Policy (Track C, binding): performances come from an open-weights TTS model run locally as an
editor-time pipeline stage, deterministic under recorded seed and revision, never a runtime or cloud
dependency, registered before use. Voice covers characters and narration, not unit barks. Every line is
subtitled verbatim; the game must remain complete with voice off.

Character profiles (from the Bible; each requires an approved profile sheet and directed calibration
line before batch generation):

- **Mara Vey** — level, precise, engineering cadence; urgency compressed into economy, never volume.
- **Oruun-of-Seven-Stones** — layered and deliberate; certainty arrives qualified; dry humor from
  inherited-memory mismatch.
- **Talar Venn** — careful, archival, quietly persistent.
- **Chancellor Cael Rhyse** — persuasive, warm, reasonable in exactly the way that makes his program
  dangerous; never played as a villain by tone.
- **Neme** — constructed precision; exact, lightly non-idiomatic, never mystical-collective.
- Minor attributed speakers take profiles consistent with their faction's language culture.

Writing-rule bounds apply to performance direction too: no line is delivered as omniscient; no
performance asserts certainty the text does not carry.

## 7. Mix architecture, priority, and loudness

**Buses** (implemented, gate 14): master → music, dialogue, interface, ambience, effects; independent
per-category volumes; whole-graph reduced dynamic range.

**Ducking** *(authored here, pending implementation and measurement — the B1 remainder)*: music ducks
−6 dB and ambience −4 dB under active voice, with 150 ms attack / 400 ms release; **effects never
duck** — combat information never disappears. Exact amounts are fixed by measurement at implementation
and recorded in the ledger.

**Priority order** when simultaneous (highest first) *(authored here)*: voice → alerts → rejection →
combat-critical effects (destruction, weapons, impacts) → other effects → interface → music → ambience.

**Loudness targets.** Directive-fixed: packaged ordinary-session integrated loudness **−16 LUFS ±1**;
true peak **≤ −1 dBTP everywhere**; voice intelligible over bed at default levels; reduced dynamic
range preserves the quietest mapped cue. Per-cue source normalization targets *(authored here — these
order the categories so bus gains start near unity and the measured ambience-over-alert inversion is
removed at the source)*:

| Category | Source target (integrated) | Tolerance |
|---|---|---|
| Voice lines | −16 LUFS | ±0.5 LU |
| Alerts | −14 LUFS | ±1 LU |
| Gameplay effects | −15 LUFS | ±1.5 LU |
| Interface | −17 LUFS | ±1 LU |
| Music | −16 LUFS | ±1 LU |
| Ambience beds | −21 LUFS | ±1 LU |

All measurements are BS.1770-4 integrated loudness with 4× inter-sample peak, via
`Scripts/measure_audio_loudness.py`. Record numbers, never impressions.

## 8. Accessibility behaviors (binding on every cue)

- Per-category volume genuinely controls every sound; a cue outside the five categories does not ship.
- Reduced dynamic range keeps the quietest mapped cue audible and narrows, never inverts, the
  established level separations.
- No information is hearing-only: alerts pair with visible markers, rejections with the stated reason,
  voice with verbatim subtitles carrying size/background controls.
- Nothing in the soundscape encourages sustained high level; alert and destruction limiters and
  admission windows are part of the design, not an afterthought.

## 9. Provenance, registration, and definition of done

Every cue is synthesized deterministically by `Scripts/echoes_audio_synth.py` through
`Scripts/generate_audio_assets.py`, byte-idempotent under its recorded revision, and registered in
`Docs/Archive/AssetRegister.md` **before** use. Voice is the recorded standing exception (local TTS,
editor-time, deterministic seed). No third-party recordings, samples, or stock audio, ever.

A cue or family is **done** only when: (a) deterministic and byte-idempotent, (b) registered before
use, (c) inside its section-7 loudness/peak targets, (d) carries its information without hearing-only
dependence and behaves under reduced dynamic range and per-category volume, (e) has no simulation or
determinism impact, (f) has passed a **directed listening review** with an accept verdict, and (g) is
recorded in the ProjectLedger with explicit non-coverage.

## 10. The directed listening review

The review that gates (f) above is a human pass over rendered cues against this document. Verdicts:

- **Accept** — meets its family identity, event grammar, and the professional bar; may proceed to
  normalization and mix.
- **Redirect** — right idea, wrong execution; reviewer states the direction change; resynthesis under a
  new revision.
- **Regenerate** — fails identity or reads as placeholder; the family section above is re-consulted
  before any new attempt.

Review criteria per family: identity (is it unmistakably this faction/site/event?), information (is the
fact it carries unambiguous at gameplay attention?), craft (envelope, spectrum, and decay free of
synthesis artifacts; nothing reads as a raw oscillator), and restraint (earns its place in a quiet
world). A review sheet template lives with each review's evidence under `WorkstreamControl/evidence/`.
