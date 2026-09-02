---
name: echoes-cross-device-interaction-check
description: Check Echoes UI and controls across authorized displays, resolutions, pixel densities, input devices, and target Macs while keeping future-platform and human-usability claims separate.
metadata:
  author: Angelis Pseftis
---

# Echoes cross-device interaction check

Read `CLAUDE.md`, `Docs/InitialReleaseRequirements.md`, `Docs/DemoReadinessRequirements.md`, `Docs/Archive/SetupAndBuild.md`, `Docs/Archive/ProjectLedger.md`, `../WorkstreamControl/ACTIVE_LANES.md`, and `../WorkstreamControl/HEAVY_RUN_LOCK.md`. Define the authorized matrix before execution: exact package, Mac hardware, OS, display, resolution, scale, window/fullscreen mode, pointer/keyboard/controller route, accessibility settings, power/thermal conditions, and expected evidence.

Use the live REL-UI resolution matrix when release work is authorized: 1280x720, 1440x900, 1600x900, 1920x1080, 2560x1440, baseline native, windowed, fullscreen, and live resize. Exercise normal task completion, focus, clipping, safe areas, target size, scrolling, remapping where implemented, device disconnect/reconnect, and settings persistence using real player surfaces.

Each device/result binds only its observed combination. Simulator, resized screenshot, source inspection, or one Mac does not prove another device, accessibility population, Windows, Linux, SteamOS, or broad compatibility. Record absent hardware and unsupported paths as gaps.

Acquire and explicitly release the heavy lease; require `echoes-gui-control-readiness`. Route UI/input defects to `echoes-ui-hud-menu-design` or `echoes-input-controls`, performance/thermal results to `echoes-performance-profiling`, evidence to `echoes-evidence-gate-review`, and human usability to `echoes-human-acceptance-session`.
