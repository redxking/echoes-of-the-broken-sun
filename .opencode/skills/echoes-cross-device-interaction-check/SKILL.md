---
name: echoes-cross-device-interaction-check
description: Check Echoes UI and controls across authorized displays, resolutions, pixel densities, input devices, and target Macs while keeping future-platform and human-usability claims separate.
metadata:
  author: Angelis Pseftis
---

# Echoes cross-device interaction check

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling.

Read [Project/AGENTS.md](../../../AGENTS.md), `Docs/Requirements.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, [echoes-session-control](../echoes-session-control/SKILL.md), and [echoes-heavy-run-coordination](../echoes-heavy-run-coordination/SKILL.md). Define the authorized matrix before execution: exact package, Mac hardware, OS, display, resolution, scale, window/fullscreen mode, pointer/keyboard/controller route, accessibility settings, power/thermal conditions, and expected evidence.

Use the live REL-UI resolution matrix when release work is authorized: 1280x720, 1440x900, 1600x900, 1920x1080, 2560x1440, baseline native, windowed, fullscreen, and live resize. Exercise normal task completion, focus, clipping, safe areas, target size, scrolling, remapping where implemented, device disconnect/reconnect, and settings persistence using real player surfaces.

Each device/result binds only its observed combination. Simulator, resized screenshot, source inspection, or one Mac does not prove another device, accessibility population, Windows, Linux, SteamOS, or broad compatibility. Record absent hardware and unsupported paths as gaps.

Acquire and explicitly release the exclusive resource reservation; require `echoes-gui-control-readiness`. Route UI/input defects to `echoes-ui-hud-menu-design` or `echoes-input-controls`, performance/thermal results to `echoes-performance-profiling`, evidence to `echoes-evidence-gate-review`, and human usability to `echoes-human-acceptance-session`.
