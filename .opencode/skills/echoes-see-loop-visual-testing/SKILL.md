---
name: echoes-see-loop-visual-testing
description: "Retired: the automated Gemini Vision See Loop is out of scope and its implementation was removed. Do not select this skill for work; route visual qualification to echoes-realtime-visual-review and editor inspection to echoes-unreal-mcp-editor-inspection."
metadata:
  author: Angelis Pseftis
---

# Echoes See Loop visual testing — retired

Inherits [AGENTS.md](../../../AGENTS.md), [Docs/README.md](../../../Docs/README.md) and [AgentSkillRouting.md](../../../Docs/AgentSkillRouting.md). Fetch the affected [Requirements.md](../../../Docs/Requirements.md) and [RequirementsState.md](../../../Docs/RequirementsState.md) records with `python3 Scripts/req.py both <ID>...`, never the masters whole.

## Status

This procedure is retired. The owner removed Gemini Vision from scope on 2026-09-06; the analyzer never authenticated, so it returned no verdict in any recorded run. Its controller, MCP server, and server registration were removed. The retirement record is [SeeLoopWorkflow.md](../../../Docs/SeeLoopWorkflow.md).

Do not restore the integration, reauthenticate it, or request secrets for it. An automated visual verdict was never an accepted evidence class and is not one now.

## Where the work goes instead

- Rendered gameplay, UI, VFX, terrain, lighting, fog, and cinematic qualification: [echoes-realtime-visual-review](../echoes-realtime-visual-review/SKILL.md).
- Editor, asset, actor, and Sequencer inspection over the surviving `unreal-mcp` server: [echoes-unreal-mcp-editor-inspection](../echoes-unreal-mcp-editor-inspection/SKILL.md).
- Real operating-system mouse and keyboard interaction: [echoes-mouse-keyboard-playtest](../echoes-mouse-keyboard-playtest/SKILL.md) with [echoes-gui-control-readiness](../echoes-gui-control-readiness/SKILL.md).
- Whether retained evidence supports a claim: [echoes-evidence-gate-review](../echoes-evidence-gate-review/SKILL.md).

Captures retained under `BuildArtifacts/Evidence/SeeLoop-<UTC>/` stay historical evidence. They are frames without analysis, and they do not qualify any requirement.
