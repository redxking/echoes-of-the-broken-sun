---
name: echoes-see-loop-visual-testing
description: Operate the automated See Loop connecting Unreal MCP, Gemini Vision, and ChatGPT to visually qualify gameplay, UI widgets, navigation, and collisions.
metadata:
  author: Angelis Pseftis
---

# Echoes See Loop visual testing

## Project authority

Follow [Project/AGENTS.md](../../../AGENTS.md) and the authority map in [Docs/README.md](../../../Docs/README.md). Read the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md).

Use [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md) for skill selection, path ownership, heavy-run coordination, and evidence handling. Read [SeeLoopWorkflow.md](../../../Docs/SeeLoopWorkflow.md) for architectural and runtime details.

Route general editor inspection to `echoes-unreal-mcp-editor-inspection`, real OS device play to `echoes-mouse-keyboard-playtest`, and post-build visual verification to `echoes-realtime-visual-review`.

## Operating procedure

1. **Connect and verify.** Confirm the interactive editor is running with MCP (`http://127.0.0.1:8000/mcp`) via `Scripts/open_editor.command`. Verify server responsiveness before invoking test commands.
2. **Execute closed-loop steps.** When testing gameplay or UI changes, use `Scripts/echoes_see_loop.py` or the `echoes-vision` MCP tools (`see_loop_step`, `see_loop_capture_and_analyze`). Always provide explicit test goals and expected visual outcomes.
3. **Correlate with logs.** Do not evaluate visual frames in isolation or rely on logs alone. Correlate visual verdicts with engine log outputs to distinguish presentation glitches from logic failures.
4. **Preserve evidence.** Retain all captured frames and Gemini Vision diagnostic JSON/Markdown files under `BuildArtifacts/Evidence/SeeLoop-<UTC>/`. Keep `BuildArtifacts/Evidence/SeeLoop-Live/` updated for real-time agent synchronization.
5. **Collaborative boundary.** ChatGPT drives rapid in-editor tool steps, asset tweaks, and localized adjustments. Antigravity performs deep C++ simulation audits, shader reviews, and requirement verification. Both agents share the See Loop evidence base.
