#!/usr/bin/env python3
"""Echoes of the Broken Sun — Gemini Vision MCP Server for ChatGPT & Codex.

Author and owner: Angelis Pseftis
Applies to: Unreal Engine 5.8, Unreal MCP (http://127.0.0.1:8000/mcp), Gemini Vision,
ChatGPT (Codex CLI / Desktop client), and Antigravity.

This stdio MCP server exposes the 'See Loop' to ChatGPT and Codex, allowing it to:
  - Trigger Play-In-Editor (PIE) in Unreal
  - Capture real-time viewport/editor frames
  - Run Gemini Vision analysis with custom verification prompts
  - Obtain real-time visual-and-log feedback directly within its chat turns
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import sys
import traceback
from typing import Any, Dict, Optional

# Add script directory to sys.path to import echoes_see_loop
SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from echoes_see_loop import (
    SeeLoopSession,
    DEFAULT_UNREAL_MCP_URL,
    DEFAULT_GEMINI_MODEL,
    LIVE_EVIDENCE_DIR,
)

TOOLS = [
    {
        "name": "see_loop_capture_and_analyze",
        "description": (
            "Captures the current Unreal gameplay/editor frame, sends it to Gemini Vision with a custom test prompt, "
            "correlates with recent engine logs, and returns real-time visual feedback (UI layout, character positioning, "
            "clipping/floor penetration, visual defects, SUCCESS/BUG verdict)."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "prompt": {
                    "type": "string",
                    "description": (
                        "Specific visual question or criteria to evaluate (e.g. 'Did the player navigate past the obstacle without falling through the floor?')"
                    ),
                },
                "action_description": {
                    "type": "string",
                    "description": "Optional description of the test action or code change just applied.",
                },
                "capture_mode": {
                    "type": "string",
                    "enum": ["viewport", "editor"],
                    "default": "viewport",
                    "description": "Capture target: 'viewport' (in-game level view) or 'editor' (entire Unreal window).",
                },
            },
        },
    },
    {
        "name": "see_loop_step",
        "description": (
            "Executes an automated 'See' step in the gameplay loop: takes an action description and an expected visual outcome, "
            "captures the frame, and uses Gemini Vision to verify if the expected visual outcome occurred or if visual bugs appeared."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "action_description": {
                    "type": "string",
                    "description": "What test action or player order was just executed (e.g. 'Clicked move order to (1200, 450, 100)').",
                },
                "expected_visual_outcome": {
                    "type": "string",
                    "description": "What should visually happen on screen (e.g. 'Unit moves along navmesh, selection ring is active, no floor clipping').",
                },
                "capture_mode": {
                    "type": "string",
                    "enum": ["viewport", "editor"],
                    "default": "viewport",
                    "description": "Capture target.",
                },
            },
            "required": ["action_description", "expected_visual_outcome"],
        },
    },
    {
        "name": "see_loop_start_pie",
        "description": "Starts a Play-In-Editor (PIE) session in Unreal Engine so gameplay and GUI widgets can be tested live.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "b_simulate": {
                    "type": "boolean",
                    "default": False,
                    "description": "If true, starts Simulate-In-Editor without possessing player pawn; if false, standard PIE.",
                },
                "warmup_seconds": {
                    "type": "number",
                    "default": 2.0,
                    "description": "Seconds to wait for level and subsystems to settle after BeginPlay.",
                },
            },
        },
    },
    {
        "name": "see_loop_stop_pie",
        "description": "Stops the currently running Play-In-Editor (PIE) session in Unreal Engine.",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    {
        "name": "see_loop_get_recent_feedback",
        "description": "Reads back the latest See Loop visual assessment, verdict, and live frame file paths.",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
]


class StdioMcpServer:
    """Standard I/O MCP Server implementation for ChatGPT/Codex."""

    def __init__(self):
        self.session = SeeLoopSession()

    def handle_request(self, request: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        method = request.get("method", "")
        req_id = request.get("id")

        if method == "initialize":
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {"tools": {}},
                    "serverInfo": {
                        "name": "echoes-vision-mcp",
                        "title": "Echoes Gemini Vision See Loop",
                        "version": "1.0.0",
                    },
                },
            }

        elif method == "notifications/initialized":
            return None

        elif method == "tools/list":
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {"tools": TOOLS},
            }

        elif method == "tools/call":
            params = request.get("params", {})
            tool_name = params.get("name", "")
            args = params.get("arguments", {})

            try:
                result_text = self._call_tool(tool_name, args)
                return {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "content": [
                            {
                                "type": "text",
                                "text": result_text,
                            }
                        ]
                    },
                }
            except Exception as ex:
                tb = traceback.format_exc()
                return {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "isError": True,
                        "content": [
                            {
                                "type": "text",
                                "text": f"Error executing tool '{tool_name}': {ex}\n\nTraceback:\n{tb}",
                            }
                        ],
                    },
                }

        else:
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "error": {
                    "code": -32601,
                    "message": f"Method '{method}' not found",
                },
            }

    def _call_tool(self, name: str, args: Dict[str, Any]) -> str:
        if name == "see_loop_capture_and_analyze":
            prompt = args.get("prompt")
            action_desc = args.get("action_description")
            capture_mode = args.get("capture_mode", "viewport")

            step_res = self.session.execute_step(
                prompt=prompt,
                action_description=action_desc,
                capture_mode=capture_mode,
            )

            analysis_text = step_res.get('analysis') or step_res.get('message') or "(No analysis text generated)"
            output = (
                f"=== SEE LOOP VISUAL FEEDBACK ===\n"
                f"Verdict: {step_res.get('verdict')}\n"
                f"Capture Route: {step_res.get('capture_route')} ({step_res.get('capture_elapsed_sec')}s)\n"
                f"Analysis Time: {step_res.get('analysis_elapsed_sec')}s\n"
                f"Frame Saved: {step_res.get('frame_path')}\n"
                f"Live Frame: {step_res.get('live_frame_path')}\n\n"
                f"--- GEMINI VISION DIAGNOSIS ---\n"
                f"{analysis_text}\n\n"
                f"--- RECENT ENGINE LOGS CORRELATED ---\n"
                + "\n".join(step_res.get("recent_logs", []))
            )
            return output

        elif name == "see_loop_step":
            action_desc = args.get("action_description", "")
            expected_outcome = args.get("expected_visual_outcome", "")
            capture_mode = args.get("capture_mode", "viewport")

            custom_prompt = (
                f"ASSERTION CHECK:\n"
                f"Expected Outcome: {expected_outcome}\n\n"
                f"Check whether this expected outcome is satisfied in the screenshot. "
                f"Look for character/unit positioning, floor penetration or collision bugs, and UI defects. "
                f"If the expected outcome is met without bugs, reply 'VERDICT: SUCCESS'. "
                f"If there are visual glitches, clipping, or errors, reply 'VERDICT: BUG_FOUND' and describe the defects."
            )

            step_res = self.session.execute_step(
                prompt=custom_prompt,
                action_description=action_desc,
                capture_mode=capture_mode,
            )

            analysis_text = step_res.get('analysis') or step_res.get('message') or "(No analysis text generated)"
            output = (
                f"=== SEE LOOP STEP RESULT ===\n"
                f"Action: {action_desc}\n"
                f"Expected: {expected_outcome}\n"
                f"Verdict: {step_res.get('verdict')}\n"
                f"Frame Saved: {step_res.get('frame_path')}\n\n"
                f"--- GEMINI VISION VERIFICATION ---\n"
                f"{analysis_text}\n"
            )
            return output

        elif name == "see_loop_start_pie":
            b_sim = bool(args.get("b_simulate", False))
            warmup = float(args.get("warmup_seconds", 2.0))
            success = self.session.mcp_client.start_pie(b_simulate=b_sim, warmup_seconds=warmup)
            return f"Play In Editor (PIE) started: success={success}, b_simulate={b_sim}, warmup_seconds={warmup}"

        elif name == "see_loop_stop_pie":
            success = self.session.mcp_client.stop_pie()
            return f"Play In Editor (PIE) stopped: success={success}"

        elif name == "see_loop_get_recent_feedback":
            state_file = LIVE_EVIDENCE_DIR / "current_state.json"
            if state_file.is_file():
                return state_file.read_text(encoding="utf-8")
            return "No recent See Loop feedback recorded yet. Run see_loop_capture_and_analyze to generate feedback."

        else:
            raise ValueError(f"Unknown tool name: {name}")

    def run(self):
        """Run stdio JSON-RPC loop."""
        sys.stderr.write("[echoes-vision-mcp] Server started on stdio.\n")
        sys.stderr.flush()

        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            try:
                req = json.loads(line)
                resp = self.handle_request(req)
                if resp is not None:
                    out = json.dumps(resp)
                    sys.stdout.write(out + "\n")
                    sys.stdout.flush()
            except Exception as ex:
                err_resp = {
                    "jsonrpc": "2.0",
                    "id": None,
                    "error": {
                        "code": -32700,
                        "message": f"Parse error: {ex}",
                    },
                }
                sys.stdout.write(json.dumps(err_resp) + "\n")
                sys.stdout.flush()


def main():
    server = StdioMcpServer()
    server.run()


if __name__ == "__main__":
    main()
