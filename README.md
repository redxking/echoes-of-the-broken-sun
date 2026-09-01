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
- **The Hollow Choir** — the erased futures themselves, learning to exist. They don't hold territory in the normal sense; they hold *possibilities*, and eventually have to commit to one. (This faction unlocks a bit later in development — see below.)

None of them are the villain. None of them are purely right. That's on purpose.

## Where the project stands today

Here's the honest picture, in plain terms:

*Echoes of the Broken Sun* is being built solo, in the open, as a real Unreal Engine 5 project — not a pitch deck or a concept. Right now it's best described as a **working prototype that's growing into its first real vertical slice**: a full, playable slice of the game called the **Glass Scar**, plus a first pass at the entire 15-mission campaign that carries a single ongoing story from start to finish.

What's actually built and running:

- A complete, deterministic combat and economy simulation — workers, production, logistics, combat, fog of war, and all three Future Well decisions — for the Meridian Compact and Kharuun Assemblies, playable against a functioning AI opponent.
- All 15 campaign missions exist as working, testable content, chained together into one continuous campaign you can play through from the first evacuation to the final confrontation at the Broken Sun, with save/restore and consequences that carry forward from mission to mission.
- Early versions of every unit and structure, the Future Well, and the Glass Scar battlefield itself, built with the game's own visual identity rather than placeholder blocks.
- The first working feedback systems a strategy game needs to actually feel good to play: order confirmation, selection, formations, destruction effects, and a first pass at sound design — all original work.

What's still ahead, also in plain terms: this is **not yet a finished, publicly playable game**. The presentation is still rough in places — you'll see development-grade art, an unfinished interface, and missing music, voice, and ambience. Balance across all three factions hasn't been proven out. The Hollow Choir isn't playable yet. And it's been tested primarily through automated and controller-driven playthroughs, not the kind of hands-on, "hand a stranger a mouse and see what happens" testing that a real release needs.

The current direction — locked in as of this month — is to finish this as a **complete, professional-quality single-player game**: the full Glass Scar skirmish and all fifteen campaign missions, with real voice acting, a finished score, finished art, and one consistent look and feel from the game itself through to its website and story materials. It's launching on macOS first, with Linux/SteamOS and Windows support planned to follow. Online multiplayer exists in the code but is intentionally switched off for now — this release is about finishing the single-player experience right.

## Want to know more, or get involved?

If you're curious about the deeper worldbuilding, the campaign, the factions, and the design philosophy behind all of it, the full design bible lives at [`Docs/Archive/DevelopmentBible.md`](Docs/Archive/DevelopmentBible.md). The nuts-and-bolts engineering documentation — architecture, build instructions, and the detailed, evidence-based development log — lives alongside it in [`Docs/Archive/`](Docs/Archive/).

This is an ambitious, one-person effort to build something with real craft behind it, not cut corners. If that's the kind of project you want to watch grow — or you're the kind of person who might want to help build it — following along here is the best way to see it happen. Reach out to Angelis Pseftis directly if you'd like to talk about the project.

## Repository map

- `Source/EchoesSimCore` — the deterministic, engine-independent simulation at the heart of the game
- `Source/EchoesOfTheBrokenSun` — the Unreal Engine presentation and interaction layer
- `Content/Data/Source` — the source data defining factions, units, structures, technologies, and Future Wells
- `Content/Art/Generated` — in-progress art for the vertical slice
- `site` — the public game website, deployed through GitHub Pages
- `Docs/Archive/DevelopmentBible.md` — the world, story, campaign, and design bible
- `Docs/Archive/TechnicalArchitecture.md` — how the simulation, engine integration, AI, networking, and build pipeline work
- `Docs/Archive/ProjectLedger.md` — the detailed, evidence-based development log: decisions, test results, and known limitations
- `Docs/Archive/AssetRegister.md` — where every asset comes from and its licensing status

All project documents are living files, kept honest and up to date as the game develops.
