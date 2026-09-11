# See Loop visual feedback — retired

**Author and owner:** Angelis Pseftis
**Standing:** retired procedure record. Creates no current workflow, capability, or evidence class.
**Retired:** 2026-09-09
**Authority:** [Project/AGENTS.md](../../../AGENTS.md) and [Docs/README.md](../../README.md).

## What replaces it

| Need | Current procedure |
|---|---|
| Rendered gameplay, UI, VFX, lighting and fog qualification | `echoes-realtime-visual-review` |
| Editor and asset inspection over Unreal MCP | `echoes-unreal-mcp-editor-inspection` |
| Real mouse and keyboard interaction on the running game | `echoes-mouse-keyboard-playtest` and `echoes-gui-control-readiness` |
| Evidence sufficiency for a gate claim | `echoes-evidence-gate-review` |

Select them through [AgentSkillRouting.md](../../AgentSkillRouting.md). The `unreal-mcp` server in
`.mcp.json` remains available for editor inspection; it was never the retired part.

## Why it was retired

The See Loop paired Unreal MCP frame capture with a Google Gemini Vision analyzer to return automated
visual verdicts. Two facts ended it:

- **The owner removed Gemini Vision from scope on 2026-09-06**, directing that it not be restored,
  reauthenticated, or supplied with secrets, and that unavailable Vision or audio capture never be
  reported as a pass.
- **It never produced a single analysis.** Every recorded attempt failed authentication against
  Google's API — see [ProjectLedger.md](../ProjectLedger.md) and the dated entries in
  [RequirementsState.md](../../RequirementsState.md). Frames were captured; no verdict was ever returned.

The retired design also assigned standing roles to named third-party assistants. Model-specific lane
assignments do not establish current permissions under [AGENTS.md](../../../AGENTS.md), and no automated
visual verdict substitutes for rendered inspection or owner acceptance.

## What was removed

`Scripts/echoes_see_loop.py` (controller and Gemini client), `Scripts/echoes_vision_mcp_server.py`
(the `see_loop_*` MCP tool surface), and the `echoes-vision` entry in `.mcp.json`. Both scripts
required a `GEMINI_API_KEY` in the environment or in a local `.env.local`; that instruction is
withdrawn with them. Recover either file from Git history if a future authorized integration needs
the capture path.

## What is preserved

Frames and state captured under `BuildArtifacts/Evidence/SeeLoop-<UTC>/` and
`BuildArtifacts/Evidence/SeeLoop-Live/` remain historical evidence. They are captures, not visual
qualification, and carry no analysis. Concept-pipeline records that cite those paths stay valid as
written.
