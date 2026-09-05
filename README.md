# Echoes of the Broken Sun

*A science-fantasy real-time strategy game about the cost of making one future real.*

Created by Angelis Pseftis.

## What is this game?

A long time ago, the sun over the world of Soryn shattered. What broke off didn't just become rubble — it became **Dawnshards**: fragments that carry futures which never got to happen. Burn one for power and you can light a city. But you also close the door on whatever that future would have been. Some of those futures, it turns out, were aware enough to know they'd been erased.

That's the idea at the center of *Echoes of the Broken Sun*: every strategic choice you make has a cost, and the game makes you look at it. You'll build bases, gather resources, scout the map, and fight tactical battles the way any real-time strategy fan expects. But the map's most important resource, the **Future Well**, never lets you forget what you're spending. Do you **Harvest** it for an immediate windfall and destroy it forever? **Preserve** it and profit slowly while everyone fights to hold the ground around it? Or **Reshape** it, bending the battlefield itself into a possibility that was never supposed to exist — temporarily?

There's no "good" button. Every choice is visible, permanent in its own way, and remembered by the story that follows.

## Who you play as

Three factions, three completely different ways of thinking about the war:

- **The Meridian Compact** — engineers and administrators holding together ark-cities on power grids, supply lines, and disciplined logistics. They're strongest once they've built a network; cut a single link and even their strongest positions can come apart.
- **The Kharuun Assemblies** — a people whose identity is built from layered ancestral memory, fighting from mobile, living infrastructure grown out of mineral-organic terrain. They don't hold ground so much as reshape it and move through it.
- **The Hollow Choir** — the erased futures themselves, learning to exist. They don't hold territory in the normal sense; they hold *possibilities*, and eventually have to commit to one.

None of them are the villain. None of them are purely right. That's on purpose.

## The campaign we are building

Fifteen missions, fifteen distinct battlefields, one connected world. Each location must matter to the
story: who lives there, what is at stake, why the characters have come, and what the battle changes.
The campaign should feel like a journey across Soryn, with strong characters and backstories connecting
individual battles to the larger adventure. These are design commitments, not a claim that the campaign
has completed player testing.

## Development status

This is an Unreal Engine RTS in active development, not a finished public release. The repository contains
simulation, campaign, three-faction, world, UI, and audio work at different stages of implementation and
verification. Automated tests and development captures cover bounded parts of that work; they do not
establish a fully qualified ordinary-player experience.

The intended macOS release includes the fifteen-mission story campaign, a separate Conquest/roguelite
mode, offline PvAI and bounded multiplayer with team battles and free-for-all. These are development
commitments, not claims that those modes are ready. Linux/SteamOS and Windows remain later targets.
Campaign maps and skirmish formats have separate contracts; the connected world is not an MMO.

The [requirements](Docs/Requirements.md) define the game, and the [state record](Docs/RequirementsState.md)
records evidence, unresolved work, and owner acceptance. Read those records for a specific capability claim.

## Want to know more, or get involved?

If you're curious about the deeper worldbuilding, the campaign, the factions, and the design philosophy behind all of it, the full design bible lives at [`Docs/Archive/DevelopmentBible.md`](Docs/Archive/DevelopmentBible.md). The nuts-and-bolts engineering documentation — architecture, build instructions, and the detailed, evidence-based development log — lives alongside it in [`Docs/Archive/`](Docs/Archive/).

This is an ambitious, one-person effort to build something with real craft behind it, not cut corners. If that's the kind of project you want to watch grow — or you're the kind of person who might want to help build it — following along here is the best way to see it happen. Reach out to Angelis Pseftis directly if you'd like to talk about the project.

## Repository map

- `Source/EchoesSimCore` — the deterministic, engine-independent simulation at the heart of the game
- `Source/EchoesOfTheBrokenSun` — the Unreal Engine presentation and interaction layer
- `Content/Data/Source` — the source data defining factions, units, structures, technologies, and Future Wells
- `Content/Art/Generated` — in-progress art for the vertical slice
- `site` — website source; source presence does not verify the current live deployment
- `Docs/Archive/DevelopmentBible.md` — the world, story, campaign, and design bible
- `Docs/Archive/TechnicalArchitecture.md` — how the simulation, engine integration, AI, networking, and build pipeline work
- `Docs/Archive/ProjectLedger.md` — the detailed, evidence-based development log: decisions, test results, and known limitations
- `Docs/Archive/AssetRegister.md` — where every asset comes from and its licensing status

Agents and contributors start with [AGENTS.md](AGENTS.md) and the [document authority map](Docs/README.md).
Active references and retained historical evidence have different roles; the map identifies both.
