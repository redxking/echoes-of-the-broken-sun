---
title: Strategic Depth Design — position, information, and consequence over mass
author: Angelis Pseftis
creator: Angelis Pseftis
status: Subordinate design reference; proposals become requirements only through owner rulings recorded in RequirementsState.md
created: 2026-09-11
updated: 2026-09-11
---

# Strategic Depth Design

**Owner intent (2026-09-11):** strategy, terrain, unit placement and in-game decisions must beat "zerg"
play. A larger army that attack-moves without scouting, terrain use or positioning must lose to a smaller
force that uses them. The three factions must each have a distinct play style, distinct goals, and be able
to beat themselves and the other two when the better strategy is played.

This document designs that. It is grounded in the release rosters (`REL-FAC-025..027`), structures
(`SPEC-BLD-015..017`), terrain matrix (`SPEC-TER-001..006`), combat model (`SPEC-CMB-002..007`), economy
(`REL-ECO-009/011/014`), Future Wells (`SPEC-WELLP-001..003`, `REL-WEL-018`) and maps (`SPEC-SKM-011..013`).
Numbers here are starting values to be tuned against `SPEC-BAL-003/005`; the *shape* of each rule is the
design. Every proposed rule is decided under a `TBR-STR-*` entry in [RequirementsState.md](RequirementsState.md).

---

## 1. The single law: force multiplies position, it does not replace it

Everything below serves one rule, stated so it can be tested:

> **Law of the frame.** Between two forces of equal quality, the one that chose the ground, saw first, and
> committed at the right time wins. Between a larger blind force and a smaller prepared force, the prepared
> force wins at a measured rate. Numbers only decide the fight when position, information and timing are
> equal.

Why the current rules do not yet enforce it: there is no splash damage (`SPEC-CMB-002` single damage
class), so a ball of units pays no price for being a ball; cliffs block fire (`SPEC-TER-003`) but nothing
rewards *standing* on anything; Logistics reaches 200 (`REL-ECO-011`) so mass is cheap; and the only
divergence test (`SPEC-BAL-005`) compares compositions, not positions. Four rules close that gap. Each is
deterministic, observable on screen, faction-neutral, and uses only variables the combat model already
allows (range, speed, health, vision, facing, state, position).

### Rule A — Firing lanes (the anti-blob rule)

A friendly unit's collision body blocks friendly projectiles exactly as Mineral Cover and a deployed
Bulwark shield already do (`SPEC-CMB-003`). A unit therefore needs a clear straight line from its muzzle to
the target through the *bodies of its own army*.

Consequences:

- A blob fires only with its outer rank. Twenty Lancers in a clump deliver the damage of six.
- Line formation (`SPEC-MOV-005`) becomes the thing that makes a big army *actually* big. Frontage is real.
- A chokepoint sets the width of the fight for both sides. Forty units in Ash Cut fight at the width of Ash
  Cut. The defender that chose the choke fights the same numbers the attacker can fit in it.
- Height and cover interact with it (Rule B, Rule C): a raised line shoots over a lower friendly line.

Presentation: a blocked unit shows a "no lane" state on its ring and on the selection card (`SPEC-HUD-003`
counterplay line), the same grammar as `LOGISTICS FULL`. Nothing is hidden.

What it costs: one raycast per shot against the friendly spatial hash, already required by
`SPEC-CTL-019`'s O(N) grid. Friendly-fire immunity (`SPEC-CMB-005`) is unchanged: the projectile stops, it
does not damage the blocker.

### Rule B — Ridge tier (elevation as a decision)

Add `SPEC-TER-007 — Ridge` to the terrain matrix: passable, ordinary speed, buildable, one tier above the
basin it borders. Two effects, both about *information*, none about hidden multipliers:

1. **Uphill blindness.** A unit on the basin cannot see or target a unit on the Ridge unless something of
   its own is on the Ridge or has a vision source that reaches it (scout, Skiff, Afterimage, Resonant ping is
   *not* enough, it is anonymous). Downhill vision is unrestricted. This is the classic RTS rule and it
   punishes attacking ground you have not scouted.
2. **Over-fire.** A Ridge unit's projectiles pass over basin-tier friendly bodies (Rule A does not apply
   across a tier boundary). A ranged line on the ridge with a melee or shield line below is a real
   two-rank formation.

Map use, as found in the map sources (2026-09-11): Glass Scar has no raised ridge. Its
`glass_scar_map_source_v2.json` authors two height bands, `plain` (0) and `scar-depth` (−1), and every
crossing (the two edge corridors, Ash Cut, Buried Causeway, Folded Verge) is `scar-depth` low ground
between two `plain` basins. A compiled per-cell table (`kHeightBandOrdinal` in
`EchoesGlassScarCompiledMapPack.h`) already exists, but `EchoesCompiledMapBinding.cpp` only validates it;
nothing passes it to the simulation. So Rule B should be built as **height bands**, not a Ridge terrain
class: bind the authored band per tile into the simulation and apply uphill blindness between bands. On
Glass Scar that makes every crossing a place where the crosser is blind to the far rim until it has vision
there, which is exactly the chokepoint decision the design wants. Crownfall Basin's "ridges" are
impassable walls with gates (`crownfall_basin_map_source_v1.json`); making them walkable high ground is
an authoring change to that map, not a rule change. Reshape
(`SPEC-WELLP-003`) may manifest a ramp onto a Ridge as one of its authored possibilities, which makes the
Well the terrain decision it is meant to be.

### Rule C — Commitment tax (why mass costs more than its price)

Keep `REL-ECO-011`'s accounting; change the ceiling and add a curve:

- **Ceiling 120, not 200.** Core +12, each supply structure +5/+6: reaching 120 needs roughly eighteen
  supply structures spread across the map. Every one of them is a Meridian link to cut, a Kharuun Waystone
  that is defenceless while walking, or a Choir Loom with a Dawn deadline. Mass is *spatially exposed* by
  construction.
- **Committed band (as built, `REL-ECO-011.BAND`).** Above 80 fielded Logistics, every further two points
  of fielded population cost one more (a line unit effectively costs 3 instead of 2). The surcharge is
  computed from the army on the field, so it lands when a unit arrives and lifts when one dies; admitted
  production keeps its quoted cost. The HUD Logistics cell reads "(+N committed)" in warning tone. It is a
  soft cap, not a wall, and it is the same for all factions.
- **Dawn is only from Wells** (`REL-ECO-009`, unchanged). Every faction's power spike costs Dawn (Prismatic
  Targeting, molt, reconciliation, Relay burst, cover, research). So the *tempo* decision is the Well
  decision, and no faction can buy tempo with Matter alone.

### Rule D — Prepared ground beats blind arrival

Static and semi-static defence must be cost-efficient against an unscouted attack and *inefficient* as a
turtle. Current numbers already point this way; make them explicit:

- Aegis Post: 28 damage every 20 ticks at 900 cm outranges every unit in the game; it is powered or it is
  inert. Cut the link, the gun dies within one tick (`REL-FAC-002/004`). Cheap to punish, expensive to ignore.
- Listening Spine: 2,600 cm anonymous seismic pings (`REL-FAC-008`). It does not shoot. It tells you *where*
  to stand. Stationary attackers are invisible to it, so a patient attacker beats a lazy defender.
- Phase Anchor: no gun; it lowers Choir upkeep so a defended base is *cheaper*, and its loss spikes the
  cost back. Losing it is a timed economic wound, not a wall.
- Deployed Bulwark: 40% frontal reduction in a 120° arc; flank or rear bypasses it entirely (`REL-FAC-005`).
  A faced Bulwark in a choke is the picture of "prepared ground"; a Bulwark facing the wrong way is the
  picture of losing to it.

No faction gets a turret that works without its infrastructure obligation. That is why turtling is a
choice with a cost.

**Measured 2026-09-11, and the last sentence of this rule was wrong.** "An unscouted rush into any of them
should lose" is falsified for the Aegis at current numbers (BAL-STR-2, RequirementsState "prepared ground
does not beat a blind rush"). Across 6 or 8 defenders against 8, 10 or 12 attackers the Core falls in every
cell but one: 8 defenders plus a powered Aegis against 8 attackers survive 6/30, against 0/30 with the Link
cut. The Aegis contributes 28 damage every 20 ticks, about a tenth of one attacking soldier's output, on 520
HP that eight attackers strip in roughly 35 ticks of contact. A Meridian mirror and near-identical authored
line damage (Lancer 12.0, Riftstalker 12.7, Intervalist 12.8 per second) rule out a faction artifact.

So prepared ground is currently a **tripwire**: it buys time, warning and a small parity edge, not a won
fight. What does beat a larger blind attacker, measured, is position and information: the chokepoint result
(Rule A and role bodies) and the height-band result (Rule B). Whether the Aegis should become a real defence
is an owner decision, `TBR-STR-007`; until it is ruled, this rule should be read as delay, not denial.

---

## 2. Faction identity: what each faction is *for*

The three factions are three answers to the same question the world asks: *what do you do with a future you
cannot keep?* The Compact keeps it in a ledger. The Kharuun carry it in a body. The Choir *is* it.

| | Meridian Compact | Kharuun Assemblies | Hollow Choir |
|---|---|---|---|
| Strategic verb | **Hold** | **Move** | **Commit** |
| Wins by | Turning a network into a line nobody can cross, then advancing the network | Never fighting where the enemy is ready; reshaping the ground and moving the base through it | Choosing the one engagement that matters and being 30% stronger in it |
| Loses by | A cut link; fighting away from power; slow feet | Being caught rooting, molting, or in a long frontal fight | Being made to fight when its transitions are down or its Dawn is due |
| Scarcest thing | Mobility | Durability | Time |
| Natural Well doctrine | Preserve (income + vision under a powered line) | Reshape (a route only they are fast enough to use first) | Harvest or Preserve, whichever the upkeep clock demands |
| The "zerg" temptation | Mass Lancers a-move | Mass Riftstalkers swarm | Mass Manifest all-in |
| Why it fails | Firing lanes: a Lancer blob fires with its front rank; Bulwarks cannot keep up so the blob has no face | Riftstalkers (125 HP, 500 cm) lose every stationary trade to Lancers (650 cm) or Intervalists (550 cm); cover and molt are on cooldown | 160-tick public transition, 400-tick cooldown, and upkeep due whether the attack worked or not |

### 2.1 Meridian Compact: the network is the army

**Roster reading.** Surveyor (builds, no attack). Lancer: longest ordinary range in the game (650 cm),
18 damage/30 ticks, 145 HP, slow-ish (320). Bulwark: 260 HP, 40% frontal, 35% speed deployed. Relay Skiff:
fastest Meridian unit (500), 1,500 cm sight, +4 temporary Logistics when connected. Aegis Post: 900 cm gun,
needs power. Power Link: +6 Logistics, drop-off, grid extension, 2×2.

**Play style.** The Compact wins fights it has already drawn on the map. Its strength is a *faced* line: Bulwarks
deployed toward the expected approach, Lancers behind them with clear lanes (Rule A), Aegis inside power, a
Skiff forward for vision. Inside that geometry it out-ranges and out-lasts everything. Its expansion is the
grid: each Link is income, supply, and reach at once, which is why the Compact is strongest *once built*.

**Structural weakness (by design, per the pitch).** The grid is a graph with edges. One severed Link
unpowers an Aegis, halts a Foundry (`REL-FAC-002.PROD`), removes a drop-off, drops +6 Logistics, and can
strand the next Link. The line that is unbeatable from the front has a 240° arc where it is not a line.
Deployed Bulwarks move at 35%; a Compact army that has to *turn* is an army that has stopped.

**Goals per match.** (1) Scout with the Skiff before the first Bulwark is placed, so the face is right.
(2) Take the Well with Preserve and put it inside the powered perimeter; its 1,400 cm radar is the
Compact's early warning. (3) Extend the grid toward the enemy's routes, not toward open ground. (4) Convert:
the only time to advance is a Prismatic Targeting window (115% damage, 180 ticks) with Relay Supply live.

**How Meridian beats:**

- **Kharuun.** Deny the trade. Riftstalkers have 500 cm; Lancers 650. A faced line with lanes kills anything
  that stops to fight and ignores anything that does not. Use the Skiff's 1,500 cm sight to see Cairnbacks
  before they raise cover, and shoot the cover (180 HP) rather than around it. Put an Aegis at every Link
  the Kharuun raid; a raid into a powered Aegis is a lost Riftstalker. Attack when Resonant pings show the
  Kharuun *rooting* a Waystone (60 ticks, no Logistics while mobile) or when a Growth Basin lights up a
  molt (80 ticks at 150% damage).
- **Choir.** Refuse the timed fight. The Choir must pay 5 Dawn per structure per 600 ticks; the Compact pays
  nothing to exist. Hold a Preserved Well and simply be there longer. When the Choir reconciles to Manifest
  (public, 160 ticks), retreat one Bulwark depth and let the window expire; when it transitions to Possible,
  that is 400 ticks of no Manifest, so advance. Afterimages (70 HP) that hunt Skiffs die to one Aegis.
- **Meridian (mirror).** The mirror is a Well and a graph. Whoever Preserves first has +15 Dawn/300 ticks and
  radar the other lacks. The counter is not a bigger line but a *second* line: Relay Supply a small detachment
  to a Link the opponent cannot power in time and force the deployed army to pack (15 ticks), move at 230,
  and redeploy (20 ticks). The side that makes the other rotate wins.

**How Meridian is beaten:** raid the Links (a Skiff-less Compact is blind); make it pack and move; attack the
side of a deployed Bulwark, not its face; hit a Foundry outside power; strike during a Skiff cooldown
(800 ticks) when temporary Logistics has expired and production is stalled at `LOGISTICS FULL`.

### 2.2 Kharuun Assemblies: the ground is a body that moves

**Roster reading.** Tender (grows, no attack). Riftstalker: fires on the move (`SPEC-CMB-006` Slipfire),
410 speed, 500 cm, 14/22 ticks, 125 HP. Cairnback: 245 HP, 200 cm brawler, raises 180 HP Mineral Cover
(15 Dawn, 300 ticks, 600 cooldown). Resonant: 470 speed, 1,550 sight, 2,200 cm anonymous vibration
detection. Growth Basin: molt to Carapace (135% HP, 80% speed) or Striker (125% damage, 85% cooldown), 80
ticks at 150% damage taken. Waystone: +5 Logistics, drop-off, walks at 120 cm/s taking 125% damage.
Listening Spine: 2,600 cm seismic pings.

**Play style.** The Kharuun never accept the enemy's frame. Every unit is about *where the fight happens*:
Riftstalkers trade while moving and leave; Cairnbacks *build the terrain* the fight will happen in (cover
shatters projectiles, which under Rule A also carves lanes); Resonants and Spines tell you where the enemy
is moving before you can see it; Waystones move the economy behind the army. Molting is post-contact
adaptation: you see what they built, then you choose Carapace or Striker. Their multi-route pressure is
real because they are the fastest faction on the ground.

**Structural weakness.** Everything the Kharuun do well has a vulnerable moment: molt (150% damage for
80 ticks), uproot/root (40/60 ticks, no Logistics, 125% damage while walking), cover cooldown (600 ticks
after a 300-tick cover). A Kharuun force that stands still is a Kharuun force that has lost its advantage;
their line units are out-ranged by both other factions' line units. They cannot out-shoot a faced Compact
line or a Manifest Choir volley from the front, ever. Seismic detection is anonymous and blind to stationary
units, so a patient enemy is invisible to them.

**Goals per match.** (1) Two-route scouting with Resonants; never commit before the pings show a shape.
(2) Take the Well early with Reshape when a route exists that only 410-speed units can exploit before the
telegraph ends, or Preserve if the Well sits on ground they can keep moving through. (3) Root the second
Waystone on a contested deposit the enemy must walk to; move it again when the enemy arrives. (4) Fight
only where cover stands, and molt only after the enemy composition is known.

**How Kharuun beats:**

- **Meridian.** Never enter the face. Raid the Links from the sides with two groups; every raid that forces a
  pack/redeploy cycle is 35 ticks of the Compact not fighting. Raise cover *between* Bulwark and Lancer
  line to break the Lancer lanes (Rule A), then Striker-molted Riftstalkers close 650→500 through it.
  Attack the Aegis's power, not the Aegis. When Relay Supply is on cooldown, the Compact cannot rebuild what
  you kill.
- **Choir.** Read them. Seismic pings expose a Possible flank (130% speed makes a strong signature); the
  Choir's whole plan is timing, and you know when they moved. Raid Threadkeepers (80 HP) to break the Dawn
  cadence: a Choir that misses a 600-tick charge shuts a structure. Catch Intervalists in the 160-tick
  transition with Striker Riftstalkers; they are 115 HP and in that window they cannot change again for
  400 ticks. Cairnback cover shatters Intervalist volleys (550 cm cannot out-range a wall).
- **Kharuun (mirror).** Information and cover. Whoever's pings show the other rooting wins the engagement;
  whoever molts blind loses the molt. Contest the Reshape route both can use (`SPEC-WELLP-003`), because the
  faster faction on a temporary route is decided by who reached it with cover already up.

**How Kharuun is beaten:** hold ground they cannot go around; make them stop (a rooted Waystone is a
target that cannot leave); catch molts; kill cover before it matters (180 HP); stand still where they
scout so the Spine sees nothing; force a frontal fight with a longer-ranged line.

### 2.3 Hollow Choir: hold every possibility, then commit to one

**Roster reading.** Threadkeeper (worker, exposes coherence ledger). Intervalist: 550 cm, 16/25 ticks,
115 HP, 350 speed. Lacuna Warden: 230 HP, 400 cm, 15/30, the Choir's center; needs an authored control
mechanic (`REL-FAC-027.HC.WARDEN`). Afterimage: fastest unit in the game (520), 1,600 sight, 70 HP.
Reconciliation: 20 Dawn to become Manifest (130% damage) or Possible (130% speed, 125% vision), 160-tick
public transition, 400-tick cooldown before the next change. Structures charge 5 Dawn per 600 ticks (4
inside a Phase Anchor field).

**Play style.** The Choir is the pitch's third sentence made mechanical: it does not hold territory, it
holds *possibilities*, and eventually has to commit to one. Every Choir unit carries two states; the army
in Possible is a scouting and flanking net (fastest, farthest-seeing faction); the army in Manifest is the
hardest-hitting volley in the game for 160 ticks plus its Manifest life. The economy is a countdown: Dawn
comes only from Wells and Threadkeeper cargo does not pay it, so *the Choir must own a Well or die on the
clock.* That is a goal no other faction is forced to hold, and it makes the Choir the faction that fights
over the Well most.

**Structural weakness.** Every strength is announced. Reconciliation is public and takes 160 ticks; after
it, 400 ticks pass before the army can change its mind. Upkeep is a deadline the opponent can read on the
map (`SPEC-BLD-017` "attack 30 ticks before the charge"). Its units are the lowest-HP roster. A Choir that
is made to fight *outside* its chosen window is fighting with the worst statline in the game.

**Goals per match.** (1) Afterimages on both enemy routes by the 45-second scout target; vision is what the
Choir spends Dawn to convert into a good decision. (2) Take and Preserve the Well by the first upkeep
charge (600 ticks) or plan the Harvest that pays the next three charges at once. (3) Enter Possible early,
find the fight, enter Manifest *once*, win it, and convert into Core damage before the 400-tick cooldown
would matter. (4) Keep the base under one Phase Anchor field so upkeep is 4 not 5, and know exactly what
losing the Anchor costs.

**Late-game commitment (proposal, `TBR-STR-004`).** "Resolution": once per match, all Choir combat units
may commit permanently to Manifest or Possible with no further cooldown and +10% on top of the state's
bonus, at the cost of the *other* state being unavailable for the rest of the match. This is the pitch's
"eventually have to commit to one" as a decision the player takes, visibly telegraphed for 180 ticks like a
Well protocol, and it gives the Choir a distinct end-state neither other faction has.

**How the Choir beats:**

- **Meridian.** Choose the engagement the line was not built for. Possible-state units (455 speed
  Intervalists) reach the rear arc of deployed Bulwarks before they can pack. Afterimages kill the Skiff first
  (75 HP, 1,500 sight): a Compact without its Skiff is a Compact that faces the wrong way. Then strike a
  Link, not the army: unpowered Aegis, halted Foundry, `LOGISTICS FULL`. Manifest only *after* the line has
  turned, and only at the isolated segment.
- **Kharuun.** Out-range and out-time. Intervalists (550) beat Riftstalkers (500) standing, and Manifest
  Intervalists (20.8 damage) kill a Riftstalker (125 HP) in six volleys. Watch the Growth Basin: a molt is
  80 ticks at 150% damage and the Choir's transition is 160 ticks, so a Choir that enters Manifest when
  the molt *starts* arrives while it is still vulnerable. Warden control on the Cairnback stops cover
  where it stands (pending the `.WARDEN` mechanic ruling).
- **Choir (mirror).** The Well and the clock. Both sides pay upkeep; the one holding Preserve pays it from
  income, the other from reserve. The mirror is decided by who transitions *second* with knowledge of the
  first (400-tick cooldown means the first mover is locked while the second chooses), which is why
  Afterimage vision is worth more than any unit in the mirror.

**How the Choir is beaten:** take the Well from them and let the clock work; force fights during cooldown;
kill Threadkeepers to break the cadence; kill the Phase Anchor 30 ticks before a charge; never engage a
Manifest army in the open, always a Possible one.

---

## 3. Matchup web

Every faction has a designed way to beat each other one; the arrows are *conditions*, not favourites.
`SPEC-BAL-003` still demands 40–60% in every cell under equal AI competence.

```
Meridian  ──(faced line + powered Aegis at raid points)──▶ Kharuun
Kharuun   ──(cut links, break lanes with cover, force rotation)──▶ Meridian
Meridian  ──(outlast the upkeep clock, refuse the Manifest window)──▶ Choir
Choir     ──(Possible flank on the rear arc, kill Skiff, hit a Link)──▶ Meridian
Kharuun   ──(seismic read of the flank, raid Threadkeepers, catch transitions)──▶ Choir
Choir     ──(out-range 550>500, Manifest into the molt window)──▶ Kharuun
```

Mirror deciders: Meridian, the second line; Kharuun, information and cover; Choir, second transition
with vision.

---

## 4. What "zerging" looks like in each faction and the exact rule that beats it

| Attempt | Rule that punishes it | Observable result |
|---|---|---|
| 24 Lancers a-move a choke held by 12 Lancers + 3 faced Bulwarks | Rule A frontage; Bulwark 40% frontal | Attackers fire six at a time; defenders fire twelve with 40% reduction; attackers lose |
| 30 Riftstalkers swarm a Meridian base | Aegis 900 cm > 500 cm; Rule A; molt cooldown | Swarm loses its front rank per volley to guns it cannot reach without cutting power first |
| 20 Manifest Intervalists all-in at 160 ticks | 400-tick lock; upkeep continues; Rule B if the target holds a ridge | If the all-in fails there is no second state for 400 ticks and two charges come due |
| Any faction at 110 Logistics turtling on one base | Rule C committed band; Wells only give Dawn | Production at +1 cost each; no Dawn without map presence; the opponent who holds the Well out-techs them |
| Any faction attacking a ridge without vision | Rule B uphill blindness | Cannot target; walks into fire it cannot return |

---

## 5. Validation: doctrinal tests that prove the law

Add to `SPEC-BAL-*` (proposal, `TBR-STR-005`). All `PKG-AUTO`, deterministic seeds, 500 matches each, Standard
AI competence (`SPEC-DIF-002`), Glass Scar unless stated.

| Test | Setup | Pass |
|---|---|---|
| **BAL-STR-1 Blob vs frontage** | 1.6× force attack-moves into a Line-formation defender in Ash Cut | Defender wins ≥ 70% |
| **BAL-STR-2 Blind rush** | Unscouted 90-second attack vs a scouted Warden-doctrine defender with one prepared structure | Defender wins ≥ 75% |
| **BAL-STR-3 Ridge** | Equal forces; one holds the central ridge with vision, the other attacks from the basin without vision | Ridge holder wins ≥ 75%; with a scout on the ridge, falls to 50 ± 8% |
| **BAL-STR-4 Well tempo** | Equal play; one side ignores the Well, one Preserves it | Preserver wins ≥ 65% by 12 minutes; Harvester vs Preserver within 45–55% (neither dominant, `REL-WEL-018`) |
| **BAL-STR-5 Committed band** | Mass to 120 on one base vs 70 across three | Expander wins ≥ 65% |
| **BAL-STR-6 Doctrine matrix** | Warden/Raider/Steward/Expansionist/Adaptive × three factions | No doctrine > 65% overall (`SPEC-BAL-005`); every faction cell 40–60% (`SPEC-BAL-003`) |
| **BAL-STR-7 Mirror deciders** | Each mirror with the designed decider applied by one side | Decider side ≥ 65%; absent the decider, 50 ± 8% |

A test that passes without Rule A enabled is not evidence for Rule A; run BAL-STR-1 with the rule off as
the control.

---

## 6. Sequencing (proposal)

1. **Rule A firing lanes** first: pure simulation, one raycast, biggest effect, and it makes formations
   matter today. Native test plus BAL-STR-1 with control.
2. **Rule C ceiling 120 + committed band**: constants and one reservation branch in `SPEC-RES-007`'s path.
3. **Rule B Ridge tier**: terrain enum, vision rule, map authoring on Glass Scar's existing ridge.
4. **Rule D** is mostly already implemented; write the acceptance clause and BAL-STR-2.
5. **Choir Resolution** and the Warden/Afterimage mechanics (`REL-AI-024` blockers) as one Choir package.
6. Tune numbers against the BAL-STR matrix. Everything numeric above is a starting point.

Each step lands as a requirement amendment after the corresponding `TBR-STR-*` ruling, then code under
`Source/EchoesSimCore`, then a retained `PKG-AUTO` evidence directory under `BuildArtifacts/Evidence/`.

---

## 7. Status and the footprint package (updated 2026-09-11)

**Decided under the owner's delegation:** Rules A (firing lanes, `SPEC-CMB-013`) and C (ceiling 120 plus
committed band, `REL-ECO-011.BAND`) are implemented under replay schema 33. Rule B (Ridge tier) and Choir
Resolution are decided and not yet built. `SPEC-BAL-009` (BAL-STR-1) exists as a native measurement.

**What the first measurement showed.** Defenders won 0 of 60 with lanes on and 0 of 60 with lanes off.
Firing lanes work at the unit level (a dense blob has 12 of 16 units without a lane before contact), but
the blob never stays dense at the moment of contact. The cause is body size, not the lane rule.

**Mechanism, from the code.** Mobile units have a 12.5 cm footprint (`footprintHalfExtentRaw =
kFixedScale / 8`, the same default in the Unreal content binding). `ApplySoftSeparation` pushes two
same-owner units apart only until their centres are the sum of those footprints: 25 cm. Sixteen soldiers
therefore fit in about one square metre, a two-tile corridor admits the whole blob almost at once, and a
chokepoint sets no width on the fight. Mobile occupancy is rasterised per tile, so it does not throttle
either. Until bodies have real size, Rules A and B cannot do what section 1 promises.

**TBR-STR-006 — authored mobile bodies. Plan.**

1. **Values.** Worker 30 cm, line 40 cm, heavy 55 cm, scout 30 cm half-extent, authored in
   `Content/Data/Source/units.json` as `body_radius_cm` and bound through `EchoesContentSubsystem.cpp`
   instead of the hard-coded eighth of a tile. Line units at 40 cm put two abreast in a two-tile gap
   with clearance, which is the geometry `SPEC-MOV-009` already assumes.
2. **Separate the two radii.** Keep `footprintHalfExtentRaw` as the terrain and structure clearance
   (SC2's inner radius) and add a body radius used by soft separation, firing lanes and spawn spacing
   (SC2's radius). `Docs/SC2SpatialMetricsReference.md` warns against treating one radius as both; the
   current code does exactly that.
3. **Separation cost.** `ApplySoftSeparation` compares every same-owner pair. That is the O(N²) scan
   `SPEC-CTL-019` prohibits for 400 units, and larger bodies mean more pairs actually interact. Move it
   onto the spatial hash the steering budget already requires, in the same package.
4. **Replay schema 34** with a legacy flag so every retained recording keeps its geometry.
5. **Tests.** Native: two abreast through a two-tile gap, a column cannot occupy its own lane, spawn
   admission with the new spacing, worker queues beside one extractor still fit. Unreal: the full suite,
   with `REL-QA-023` (chokepoint deadlock) and the movement tests read first. Then BAL-STR-1 must reach
   its 70% bar; that is the pass condition for this package.
6. **Order.** After the other lane's current AI slice is committed, because this changes every fight and
   every path its tests measure.

**Sweep result (supersedes the plan's assumption above).** Measured with the BAL-STR-1 geometry, 30 seeds
per cell, defender wins out of 30:

| Attackers vs 10 | current, lanes on | current, lanes off | 30/40/55/30 cm bodies, lanes on | bodies, lanes off |
|---|---|---|---|---|
| 10 | 30/30 | 30/30 | 30/30 | 30/30 |
| 11 | 30/30 | 30/30 | 30/30 | 30/30 |
| 12 | 26/30 | 26/30 | 30/30 | 30/30 |
| 13 | 9/30 | 3/30 | 30/30 | 30/30 |
| 14 | 0/30 | 0/30 | 3/30 | 4/30 |
| 16 | 0/30 | 0/30 | 0/30 | 0/30 |

Prepared ground beats equal numbers every time and the chokepoint holds to about 1.2x (current rules) or
1.3x (role bodies). No geometry holds 1.6x; that is the square law of massed fire, so BAL-STR-1's bar is now
1.3x. Firing lanes matter at the margin (13 attackers: 9 against 3). Role bodies are the change that moves
the break point, which confirms TBR-STR-006 as the next package. Beating larger forces than 1.3x is the job
of Rule D (powered Aegis, faced Bulwarks, cover) and Rule B (height bands), measured by BAL-STR-2 and 3.

**Role bodies landed (schema 35, commit 4155bdd).** Moving units are spaced by role body radius (worker
30, line 40, heavy 55, scout 30 cm); resting pairs tolerate half that so a settled group does not drift;
deployed Bulwarks are never pushed. BAL-STR-1 at 13 against 10: defender 60/60, schema-32 control (no lanes,
no bodies) 7/60. Next: Rule B as height bands, then BAL-STR-2 (blind rush into prepared ground) and
BAL-STR-3 (height bands with and without a scout).

**Rule B as height bands — implementation plan (from the code, 2026-09-11).**

- *Vision today* (`Simulation::UpdateVisibility`) marks every tile within a unit's vision radius as a disc,
  with no line of sight at all; cliffs do not block sight. Uphill blindness is therefore the game's first
  vision occlusion, and it is one rule inside the disc raster: skip a tile whose band is higher than the
  band of the tile the viewer stands on. Downhill and level vision are unchanged.
- *State.* A per-tile signed band (0 plain, −1 low ground, +1 high ground) stored beside `terrain_`, set by
  a scenario through a `SetHeightBand` call, exposed on `PlayerView` for presentation. Terrain is saved as one
  byte per tile at snapshot version 31; bands are a new saved field, so snapshot 32, with older snapshots
  loading as all-plain. No replay gate is needed: recordings made before bands carry none and behave as
  before.
- *Glass Scar.* The live map is the hand-coded preset `ConfigureGlassScar` in the simulation subsystem, not
  the compiled map pack (that binding is only called from two tests). Rows 30–34 across the full width
  become low ground, matching the authored `scar-depth` regions: every crossing, including the two edge
  corridors. A unit crossing sees only the crossing until it climbs out, while a defender on the rim sees
  down into it.
- *Fairness and presentation.* The opponent AI already plans from `PlayerView`, and the fog presentation
  reads `VisibilityAt`, so both follow the rule without separate changes.
- *Tests.* Native: a low-ground viewer cannot see an adjacent high tile, a high viewer sees down, a scout
  standing on the high tile restores sight, and snapshot round-trip plus a version-31 load. Unreal: the Glass
  Scar and full-match suites, which cross the scar. Balance: BAL-STR-3 (crossing blind against a rim
  defender, with and without a scout on the rim).

**Height bands landed (inert).** BAL-STR-3 at 10 against 10: crossing blind the defender wins 30/30, with
two scouts on the rim 0/30, on flat ground 0/30. Glass Scar's low rows are wired in a separate step after an
Unreal run.

**Only Glass Scar has band data (2026-09-11).** `glass_scar_map_source_v2.json` is the one map source in the
region-and-band format: two bands (`plain` 0, `scar-depth` −1) with every crossing and both edge corridors
on the low band, rows 30–34 across the full width, which is exactly what the live preset now writes.
`crownfall_basin_map_source_v1.json` and `soryn_confluence_map_source_v1.json` use the older
variant-and-operations format and author no bands at all, so height plays no part on those two maps yet.
Crownfall Basin's "ridges" are impassable walls with gates; making them walkable high ground is an
authoring change to that map. Until both are re-authored, `SPEC-INFO-004` is correct but only Glass Scar
exercises it, and BAL-STR-3's map-level evidence rests on Glass Scar alone.

**Open question on the defeat test.** `CompleteSkirmishDefeat` failed while an AI posture gate from the
other lane was live, so whether firing lanes slow the opponent's assault is not yet measured. The budget
was raised provisionally from 60,000 to 90,000 ticks; it returns to 60,000 if the clean run finishes
inside it.
