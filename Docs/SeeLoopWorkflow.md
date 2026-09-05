# Gameplay "See Loop" workflow: real-time visual feedback with Gemini Vision & Unreal MCP

**Author and owner:** Angelis Pseftis  
**Maintained:** 2026-09-05  
**Applies to:** Unreal Engine 5.8, Epic Unreal MCP (`http://127.0.0.1:8000/mcp`), Google Gemini Vision, ChatGPT (Codex CLI / Desktop), and Antigravity.  
**Authority:** [Project/AGENTS.md](../AGENTS.md) and [Docs/README.md](README.md).

---

## 1. Purpose and Architecture

Automating gameplay and UI tests with logs alone creates a significant blind spot: an engine test can pass clean in the logs while the screen displays clipping units, missing textures, overlapping widgets, unreadable text, or broken animations.

The **See Loop** closes this observation gap by providing real-time, vision-guided feedback. When an agent (such as ChatGPT or Antigravity) modifies code, triggers a Play-In-Editor (PIE) test, or executes an order, the See Loop:
1. Triggers and monitors Play-In-Editor (in-viewport or simulate).
2. Captures high-fidelity viewport or full-editor frames via native Unreal MCP (`CaptureViewport` / `CaptureEditorImage`).
3. Correlates visual frames with recent Unreal Engine log lines.
4. Sends the multimodal package to Gemini Vision (`gemini-2.5-flash` or `gemini-1.5-pro`) with structured inspection criteria.
5. Returns actionable visual verdicts (`SUCCESS`, `BUG_FOUND`, `WARNING`), detailed defect descriptions, and concrete recommendations directly to the AI agent.
6. Preserves reproducible evidence under `BuildArtifacts/Evidence/SeeLoop-<UTC>/` and updates `BuildArtifacts/Evidence/SeeLoop-Live/` for real-time collaboration.

```
+-----------------------------------------------------------------------------------+
|                                 UNREAL ENGINE 5.8                                 |
|  - Echoes Editor / Play-In-Editor (PIE) session                                   |
|  - Unreal MCP Server on 127.0.0.1:8000/mcp                                        |
|    * EditorAppToolset: StartPIE, StopPIE, CaptureViewport, CaptureEditorImage     |
|    * LogsToolset: Engine log category stream                                      |
+----------------------------------------+------------------------------------------+
                                         |
                         JSON-RPC / HTTP | Streamable HTTP
                                         v
+-----------------------------------------------------------------------------------+
|                     ECHOES SEE-LOOP CONTROLLER & MCP BRIDGE                       |
|                 (Project/Scripts/echoes_see_loop.py)                              |
|                 (Project/Scripts/echoes_vision_mcp_server.py)                     |
|                                                                                   |
|  - PIE lifecycle & session tracking                                               |
|  - Frame capture (UE 5.8.2 schema-conforming annotations & camera pose)           |
|  - Engine log fetcher & timestamp synchronizer                                    |
|  - Gemini Vision API client (urllib / zero external pip dependencies)             |
|  - Evidence persistence under BuildArtifacts/Evidence/SeeLoop-<UTC>/              |
|  - Live status export to BuildArtifacts/Evidence/SeeLoop-Live/                    |
+-------------------+-------------------------------------------+-------------------+
                    |                                           |
                    | stdio MCP Tools                           | Direct Inspection
                    v                                           v
+---------------------------------------+   +---------------------------------------+
|                CHATGPT                |   |              ANTIGRAVITY              |
|        (Codex / Desktop Client)       |   |               (Gemini)                |
|                                       |   |                                       |
|  Calls MCP tools:                     |   |  - Real-time visual review            |
|  - see_loop_capture_and_analyze(...)  |   |  - inspects latest_frame.png          |
|  - see_loop_step(...)                 |   |  - Core C++ / SimCore audit           |
|  - see_loop_start_pie / stop_pie      |   |  - Architectural steering             |
|  Receives visual diagnosis & fixes code|  - Syncs with shared project state       |
+---------------------------------------+   +---------------------------------------+
```

---

## 2. Configuration & Setup

### A. Environment API Key
Gemini Vision uses Google GenAI API. Configure your key in either:
1. Environment variable:
   ```bash
   export GEMINI_API_KEY="AIzaSy..."
   ```
2. Local gitignored project file `Project/.env.local`:
   ```bash
   echo 'GEMINI_API_KEY=AIzaSy...' >> "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/.env.local"
   ```

### B. Registering with ChatGPT (OpenAI Codex / Desktop)
`Project/.mcp.json` is already configured with both `unreal-mcp` and `echoes-vision`.

To register `echoes-vision` into user Codex configuration:
```bash
codex mcp add echoes-vision -- python3 "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project/Scripts/echoes_vision_mcp_server.py"
```

Verify registration:
```bash
codex mcp get echoes-vision
```

---

## 3. Available Tools for ChatGPT

When registered, ChatGPT has access to the following typed MCP tools:

| Tool Name | Parameters | Purpose |
|---|---|---|
| `see_loop_capture_and_analyze` | `prompt` (str), `action_description` (str), `capture_mode` ("viewport" \| "editor") | Captures the active frame, queries Gemini Vision with your question/criteria, correlates with logs, and returns a visual diagnostic report. |
| `see_loop_step` | `action_description` (str, required), `expected_visual_outcome` (str, required), `capture_mode` | Visual assertion: verifies whether the expected outcome occurred on screen (e.g. unit reached goal without clipping) and reports any bugs. |
| `see_loop_start_pie` | `b_simulate` (bool, default False), `warmup_seconds` (float, default 2.0) | Starts Play In Editor session in Unreal. |
| `see_loop_stop_pie` | None | Stops running Play In Editor session. |
| `see_loop_get_recent_feedback` | None | Reads the latest visual assessment and file paths. |

---

## 4. Operational Modes

### Mode 1: On-Demand Tool Invocation by ChatGPT
During code and asset iteration, ChatGPT invokes `see_loop_step` after executing an order:
```json
{
  "action_description": "Player issued move order across the canyon bridge",
  "expected_visual_outcome": "Unit moves smoothly along bridge navmesh without jitter or falling through terrain; selection ring remains green"
}
```
Gemini Vision evaluates the screenshot, correlates with recent engine logs, and responds with:
- **Verdict:** `SUCCESS` or `BUG_FOUND`
- **UI & HUD Status:** Readability and HUD placement.
- **Character & Environment Status:** Position, terrain contact, collisions.
- **Bugs & Defects Found:** Specific issues detected.
- **Recommendation for ChatGPT:** Concrete advice on what code, blueprint, or transform to adjust next.

### Mode 2: Standalone / Background Daemon
For live monitoring while you or the AI interact with the game:
```bash
cd "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/Project"
python3 Scripts/echoes_see_loop.py --daemon --interval 2.0 --prompt "Check unit movement and HUD layout"
```
The daemon runs continuously, saving frames to `BuildArtifacts/Evidence/SeeLoop-<UTC>/frames/` and streaming real-time status to `BuildArtifacts/Evidence/SeeLoop-Live/live_feedback.md`.

### Mode 3: Single CLI Step
```bash
python3 Scripts/echoes_see_loop.py --step --prompt "Check if the field menu resume control fits above key legend"
```

---

## 5. Dual-Monitor Live Viewport Workflow

1. **Monitor 1 (Left / Main):** Unreal Editor open with viewport or PIE running.
2. **Monitor 2 (Right):** AI coding interface (ChatGPT Codex CLI, IDE, or Antigravity terminal).
3. As the AI modifies code or triggers gameplay tests, the editor updates, the AI calls the See Loop, Gemini Vision inspects the frame, and the AI immediately acts on the visual feedback.

---

## 6. Multi-Agent Synergy: ChatGPT + Antigravity

- **ChatGPT:** Specializes in rapid MCP tool execution, asset tweaks, Level Editor commands, and localized logic modifications.
- **Antigravity:** Specializes in deep C++ simulation determinism (`EchoesSimCore`), architectural consistency, shader and rendering analysis, requirements traceability (`Requirements.md`), and comprehensive review under `Project/AGENTS.md`.
- **Shared Evidence:** Both agents inspect the same live state in `BuildArtifacts/Evidence/SeeLoop-Live/` (`latest_frame.png` and `current_state.json`), ensuring consistent visual perception and zero desynchronization.
