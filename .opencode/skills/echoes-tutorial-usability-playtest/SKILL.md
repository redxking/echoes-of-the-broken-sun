---
name: echoes-tutorial-usability-playtest
description: Evaluate the rendered opening/tutorial path for discoverability, comprehension, input response, and progression without treating an agent route as novice-player evidence.
metadata: { author: Angelis Pseftis }
---

# Echoes tutorial usability playtest

Read `Docs/OpeningAndTutorialScript.md`, `Docs/Archive/DevelopmentBible.md`, `Docs/DemoRecoveryDirective.md`, `Docs/DemoReadinessRequirements.md`, `Docs/Archive/ProjectLedger.md`, the package-provenance record, and `../WorkstreamControl/ACTIVE_LANES.md`. Require a live GUI interface with fresh state and real mouse/keyboard events. Begin with no injected save or source-derived coordinates; work from the visible tutorial prompts and controls.

At each lesson, record prompt wording, what is visually highlighted, input attempted, acknowledgement, audio/voice/subtitle observation, elapsed time, ambiguity, recovery after an incorrect attempt, and whether the next step is discoverable. Check that tutorial claims match actual behavior and that high contrast, reduced motion/flashing, HUD scale, and reduced dynamic range visibly/audibly change behavior when available.

Do not use Unreal MCP, console/debug paths, test harnesses, save injection, source inspection as a shortcut, or scripted controller routes. Fresh screenshots support observations but do not establish usability alone. Escalate unresolved interpretation or owner design choices; agent operation is not unfamiliar-human validation.

Before this GUI session, read `../WorkstreamControl/HEAVY_RUN_LOCK.md`, acquire a current detailed lease, and explicitly release it. A stale, assumed, or self-invented lease requires stopping. Use `echoes-package-provenance` for package identity and `echoes-human-acceptance-session` for human evidence.
