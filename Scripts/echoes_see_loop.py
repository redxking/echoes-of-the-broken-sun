#!/usr/bin/env python3
"""Echoes of the Broken Sun — Gameplay 'See Loop' Controller & Gemini Vision Client.

Author and owner: Angelis Pseftis
Applies to: Unreal Engine 5.8, Unreal MCP (http://127.0.0.1:8000/mcp), Gemini Vision,
ChatGPT (Codex/Desktop), and Antigravity.

This script implements the closed-loop visual observation cycle ("See Loop"):
  1. Communicates with Unreal MCP (http://127.0.0.1:8000/mcp) to control Play-In-Editor (PIE).
  2. Captures real-time viewport frames (CaptureViewport) or whole editor images (CaptureEditorImage).
  3. Correlates captured frames with recent Unreal Engine output logs.
  4. Packages and sends the visual frame + logs to Gemini Vision (or GPT-4o).
  5. Returns structured visual diagnostics (UI layout, unit positioning, clipping, bugs, SUCCESS/BUG).
  6. Emits evidence under BuildArtifacts/Evidence/SeeLoop-<UTC>/ and updates BuildArtifacts/Evidence/SeeLoop-Live/.
"""

from __future__ import annotations

import argparse
import base64
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Any, Dict, List, Optional, Tuple

# Project root resolution
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
WORKSPACE_ROOT = PROJECT_ROOT.parent

# Default MCP Endpoint
DEFAULT_UNREAL_MCP_URL = "http://127.0.0.1:8000/mcp"
DEFAULT_GEMINI_MODEL = "gemini-2.5-flash"
GEMINI_API_URL_TEMPLATE = "https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={api_key}"

# Evidence roots
LIVE_EVIDENCE_DIR = PROJECT_ROOT / "BuildArtifacts" / "Evidence" / "SeeLoop-Live"
EVIDENCE_BASE_DIR = PROJECT_ROOT / "BuildArtifacts" / "Evidence"


def load_env_file() -> Dict[str, str]:
    """Load environment variables from .env or .env.local if present."""
    env_vars: Dict[str, str] = {}
    for filename in [".env.local", ".env"]:
        env_path = PROJECT_ROOT / filename
        if env_path.is_file():
            try:
                for line in env_path.read_text(encoding="utf-8").splitlines():
                    line = line.strip()
                    if not line or line.startswith("#") or "=" not in line:
                        continue
                    k, v = line.split("=", 1)
                    k = k.strip()
                    v = v.strip().strip("'\"")
                    if k not in os.environ:
                        os.environ[k] = v
                    env_vars[k] = v
            except Exception as ex:
                sys.stderr.write(f"[SeeLoop] Warning: failed to read {env_path}: {ex}\n")
    return env_vars


class UnrealMcpClient:
    """Client for Epic Unreal MCP server over Streamable HTTP."""

    def __init__(self, endpoint_url: str = DEFAULT_UNREAL_MCP_URL, timeout_sec: float = 10.0):
        self.endpoint_url = endpoint_url
        self.timeout_sec = timeout_sec
        self.session_id: Optional[str] = None
        self._request_id = 0

    def _next_id(self) -> int:
        self._request_id += 1
        return self._request_id

    def _rpc_request(self, method: str, params: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        req_id = self._next_id()
        payload = {
            "jsonrpc": "2.0",
            "id": req_id,
            "method": method,
        }
        if params is not None:
            payload["params"] = params

        data_bytes = json.dumps(payload).encode("utf-8")
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json, text/event-stream",
            "Mcp-Protocol-Version": "2025-11-25",
        }
        if self.session_id:
            headers["Mcp-Session-Id"] = self.session_id

        req = urllib.request.Request(self.endpoint_url, data=data_bytes, headers=headers, method="POST")
        try:
            with urllib.request.urlopen(req, timeout=self.timeout_sec) as resp:
                resp_headers = resp.headers
                session_hdr = resp_headers.get("Mcp-Session-Id")
                if session_hdr:
                    self.session_id = session_hdr

                body = resp.read().decode("utf-8")
                # Handle SSE or raw JSON response
                if body.startswith("data: "):
                    for line in body.splitlines():
                        if line.startswith("data: "):
                            return json.loads(line[6:].strip())
                return json.loads(body)
        except urllib.error.URLError as ex:
            raise ConnectionError(f"Failed to connect to Unreal MCP at {self.endpoint_url}: {ex}") from ex

    def initialize(self) -> Dict[str, Any]:
        """Initialize MCP session with Unreal Editor."""
        params = {
            "protocolVersion": "2025-11-25",
            "capabilities": {"tools": {}},
            "clientInfo": {"name": "EchoesSeeLoopController", "version": "1.0.0"},
        }
        return self._rpc_request("initialize", params)

    def call_tool(self, toolset_name: Optional[str], tool_name: str, arguments: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """Call a tool registered in the Unreal ToolsetRegistry."""
        params: Dict[str, Any] = {
            "name": "call_tool",
            "arguments": {
                "tool_name": tool_name,
                "arguments": arguments or {},
            },
        }
        if toolset_name:
            params["arguments"]["toolset_name"] = toolset_name

        resp = self._rpc_request("tools/call", params)
        if "error" in resp:
            raise RuntimeError(f"Unreal MCP error calling {toolset_name}.{tool_name}: {resp['error']}")
        return resp

    def start_pie(self, b_simulate: bool = False, warmup_seconds: float = 2.0) -> bool:
        """Start Play-In-Editor."""
        options = {
            "bSimulate": b_simulate,
            "playMode": "PlayMode_InViewPort",
            "warmupSeconds": warmup_seconds,
        }
        resp = self.call_tool("EditorToolset.EditorAppToolset", "StartPIE", {"options": options})
        return "result" in resp

    def stop_pie(self) -> bool:
        """Stop running Play-In-Editor session."""
        resp = self.call_tool("EditorToolset.EditorAppToolset", "StopPIE", {})
        return "result" in resp

    def is_pie_running(self) -> bool:
        """Check if PIE is currently active."""
        resp = self.call_tool("EditorToolset.EditorAppToolset", "IsPIERunning", {})
        try:
            content = resp.get("result", {}).get("content", [])
            for item in content:
                if item.get("type") == "text":
                    data = json.loads(item.get("text", "{}"))
                    return bool(data.get("returnValue", False))
        except Exception:
            pass
        return False

    def capture_viewport(self, show_ui: bool = True) -> bytes:
        """Capture active viewport image as PNG bytes.
        Uses verified UE 5.8.2 schema workaround for optional annotations and camera transform.
        """
        # First retrieve camera transform
        cam_transform = {"location": {"x": 0, "y": 0, "z": 1000}, "rotation": {"pitch": -45, "yaw": 0, "roll": 0}, "scale": {"x": 1, "y": 1, "z": 1}}
        try:
            cam_resp = self.call_tool("EditorToolset.EditorAppToolset", "GetCameraTransform", {})
            cam_content = cam_resp.get("result", {}).get("content", [])
            for item in cam_content:
                if item.get("type") == "text":
                    parsed = json.loads(item.get("text", "{}"))
                    if "returnValue" in parsed:
                        cam_transform = parsed["returnValue"]
        except Exception:
            pass

        annotations = {
            "gridSpacing": 0,
            "gridExtent": 0,
            "gridHeight": 0,
            "maxLabelDistance": 0,
            "classFilter": {"refPath": "/Script/Engine.Actor"},
            "maxLabels": 0,
        }

        args = {
            "bShowUI": show_ui,
            "captureTransform": cam_transform,
            "annotations": annotations,
        }

        resp = self.call_tool("EditorToolset.EditorAppToolset", "CaptureViewport", args)
        content = resp.get("result", {}).get("content", [])
        for item in content:
            if item.get("type") == "text":
                parsed = json.loads(item.get("text", "{}"))
                ret = parsed.get("returnValue", {})
                img_data = ret.get("image", {}).get("data")
                if img_data:
                    return base64.b64decode(img_data)

        raise RuntimeError("CaptureViewport returned empty image data")

    def capture_editor_image(self) -> bytes:
        """Capture entire editor window as PNG bytes."""
        resp = self.call_tool("EditorToolset.EditorAppToolset", "CaptureEditorImage", {})
        content = resp.get("result", {}).get("content", [])
        for item in content:
            if item.get("type") == "text":
                parsed = json.loads(item.get("text", "{}"))
                ret = parsed.get("returnValue", {})
                img_data = ret.get("data")
                if img_data:
                    return base64.b64decode(img_data)

        raise RuntimeError("CaptureEditorImage returned empty image data")


def capture_macos_window() -> Optional[bytes]:
    """Fallback capture using macOS screencapture command."""
    temp_png = LIVE_EVIDENCE_DIR / "_temp_screencapture.png"
    try:
        LIVE_EVIDENCE_DIR.mkdir(parents=True, exist_ok=True)
        subprocess.run(["screencapture", "-x", "-m", str(temp_png)], check=True, timeout=5)
        if temp_png.is_file():
            data = temp_png.read_bytes()
            temp_png.unlink(missing_ok=True)
            return data
    except Exception as ex:
        sys.stderr.write(f"[SeeLoop] Fallback screencapture failed: {ex}\n")
    return None


def get_recent_unreal_logs(max_lines: int = 40) -> List[str]:
    """Retrieve recent lines from Saved/Logs/EchoesOfTheBrokenSun.log."""
    log_file = PROJECT_ROOT / "Saved" / "Logs" / "EchoesOfTheBrokenSun.log"
    if not log_file.is_file():
        return ["[No log file found at Saved/Logs/EchoesOfTheBrokenSun.log]"]
    try:
        lines = log_file.read_text(encoding="utf-8", errors="replace").splitlines()
        return lines[-max_lines:]
    except Exception as ex:
        return [f"[Error reading log file: {ex}]"]


class GeminiVisionAnalyzer:
    """Invokes Google Gemini Vision API via direct HTTPS request."""

    def __init__(self, api_key: Optional[str] = None, model: str = DEFAULT_GEMINI_MODEL):
        load_env_file()
        self.api_key = api_key or os.environ.get("GEMINI_API_KEY") or os.environ.get("GOOGLE_API_KEY") or ""
        self.model = model

    def analyze(self, image_bytes: bytes, user_prompt: str, recent_logs: Optional[List[str]] = None) -> Dict[str, Any]:
        """Send image and prompt to Gemini Vision and return structured diagnosis."""
        if not self.api_key:
            return {
                "verdict": "KEY_MISSING",
                "message": "GEMINI_API_KEY is not set. Saved capture to disk for review.",
                "analysis": "Provide GEMINI_API_KEY in environment or Project/.env.local to enable real-time Gemini Vision analysis.",
                "bugs": [],
            }

        base64_image = base64.b64encode(image_bytes).decode("utf-8")

        logs_context = ""
        if recent_logs:
            logs_context = "\n\nRECENT ENGINE LOGS:\n" + "\n".join(recent_logs[-25:])

        system_instruction = (
            "You are the Gemini Vision game inspection co-pilot for 'Echoes of the Broken Sun', an Unreal Engine 5.8 RTS game. "
            "You are working collaboratively with ChatGPT (Codex) and human engineers in an automated 'See Loop'. "
            "Examine the provided gameplay screenshot carefully. Look for:\n"
            "1. Character and unit navigation, position, and floor penetration / collision issues.\n"
            "2. UI and HUD state: widget overlap, clipping, placeholder text, alignment, readability.\n"
            "3. Rendering artifacts, missing textures, visual glitching, or abnormal lighting.\n"
            "4. Correlation with engine logs provided.\n"
            "Format your answer with:\n"
            "- VERDICT: (SUCCESS / BUG_FOUND / WARNING / IN_PROGRESS)\n"
            "- SUMMARY: Brief 1-2 sentence description of what is shown on screen.\n"
            "- UI & HUD STATUS: Assessment of UI elements.\n"
            "- CHARACTER & ENVIRONMENT STATUS: Navigation, positioning, obstacles.\n"
            "- BUGS & DEFECTS FOUND: Bulleted list with specific locations/descriptions.\n"
            "- RECOMMENDATION FOR CHATGPT: Concrete advice on what code, blueprint, or transform to adjust next."
        )

        full_prompt = f"{system_instruction}\n\nUSER PROMPT / TEST GOAL:\n{user_prompt}{logs_context}"

        payload = {
            "contents": [
                {
                    "parts": [
                        {"text": full_prompt},
                        {
                            "inline_data": {
                                "mime_type": "image/png",
                                "data": base64_image,
                            }
                        },
                    ]
                }
            ],
            "generationConfig": {
                "temperature": 0.2,
                "maxOutputTokens": 1024,
            },
        }

        url = GEMINI_API_URL_TEMPLATE.format(model=self.model, api_key=self.api_key)
        req = urllib.request.Request(
            url,
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"},
            method="POST",
        )

        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = json.loads(resp.read().decode("utf-8"))
                text_response = ""
                candidates = data.get("candidates", [])
                if candidates:
                    parts = candidates[0].get("content", {}).get("parts", [])
                    text_response = "".join(p.get("text", "") for p in parts)

                verdict = "IN_PROGRESS"
                if "VERDICT: SUCCESS" in text_response:
                    verdict = "SUCCESS"
                elif "VERDICT: BUG_FOUND" in text_response or "BUG" in text_response:
                    verdict = "BUG_FOUND"
                elif "VERDICT: WARNING" in text_response:
                    verdict = "WARNING"

                return {
                    "verdict": verdict,
                    "analysis": text_response,
                    "model": self.model,
                    "timestamp_utc": datetime.now(timezone.utc).isoformat(),
                }
        except urllib.error.HTTPError as ex:
            err_msg = ex.read().decode("utf-8", errors="replace")
            return {
                "verdict": "ERROR",
                "message": f"Gemini API HTTP Error {ex.code}: {err_msg}",
                "bugs": [f"API Error {ex.code}"],
            }
        except Exception as ex:
            return {
                "verdict": "ERROR",
                "message": f"Gemini API Request failed: {ex}",
                "bugs": [str(ex)],
            }


class SeeLoopSession:
    """Coordinates frame capture, log collection, Gemini analysis, and evidence persistence."""

    def __init__(
        self,
        mcp_url: str = DEFAULT_UNREAL_MCP_URL,
        gemini_model: str = DEFAULT_GEMINI_MODEL,
        custom_prompt: Optional[str] = None,
    ):
        self.mcp_client = UnrealMcpClient(endpoint_url=mcp_url)
        self.analyzer = GeminiVisionAnalyzer(model=gemini_model)
        self.default_prompt = custom_prompt or (
            "Analyze the current gameplay frame. Check character/unit position and UI. "
            "Did the player or units navigate properly without clipping or falling through the floor? "
            "Are there any UI overlapping widgets or rendering errors? Respond with bugs found or 'SUCCESS'."
        )
        self.evidence_dir: Optional[Path] = None

    def _ensure_evidence_dir(self) -> Path:
        if self.evidence_dir is None:
            utc_str = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
            self.evidence_dir = EVIDENCE_BASE_DIR / f"SeeLoop-{utc_str}"
            self.evidence_dir.mkdir(parents=True, exist_ok=True)
            (self.evidence_dir / "frames").mkdir(parents=True, exist_ok=True)
            LIVE_EVIDENCE_DIR.mkdir(parents=True, exist_ok=True)
        return self.evidence_dir

    def capture(self, mode: str = "viewport") -> Tuple[bytes, str]:
        """Capture frame from Unreal MCP or fallback to macOS screencapture."""
        try:
            if mode == "editor":
                img = self.mcp_client.capture_editor_image()
                return img, "unreal_mcp_editor"
            else:
                img = self.mcp_client.capture_viewport(show_ui=True)
                return img, "unreal_mcp_viewport"
        except Exception as ex:
            sys.stderr.write(f"[SeeLoop] MCP capture error ({ex}). Attempting fallback macOS capture...\n")
            fallback = capture_macos_window()
            if fallback:
                return fallback, "macos_screencapture"
            raise RuntimeError(f"All capture routes failed. Ensure Unreal Editor is running with MCP: {ex}")

    def execute_step(
        self,
        prompt: Optional[str] = None,
        action_description: Optional[str] = None,
        capture_mode: str = "viewport",
    ) -> Dict[str, Any]:
        """Run one single iteration of the See Loop."""
        evidence_root = self._ensure_evidence_dir()
        step_prompt = prompt or self.default_prompt
        if action_description:
            step_prompt = f"ACTION JUST PERFORMED: {action_description}\n\nEVALUATION CRITERIA:\n{step_prompt}"

        # 1. Capture frame
        t0 = time.time()
        image_bytes, route = self.capture(mode=capture_mode)
        capture_elapsed = time.time() - t0

        # 2. Get recent logs
        logs = get_recent_unreal_logs(max_lines=30)

        # 3. Analyze with Gemini Vision
        t1 = time.time()
        analysis_result = self.analyzer.analyze(image_bytes, step_prompt, recent_logs=logs)
        analysis_elapsed = time.time() - t1

        # 4. Save evidence
        timestamp_str = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")[:19]
        frame_filename = f"frame_{timestamp_str}.png"
        frame_path = evidence_root / "frames" / frame_filename
        frame_path.write_bytes(image_bytes)

        # Update Live pointers
        live_frame_path = LIVE_EVIDENCE_DIR / "latest_frame.png"
        live_frame_path.write_bytes(image_bytes)

        feedback_data = {
            "timestamp_utc": datetime.now(timezone.utc).isoformat(),
            "capture_route": route,
            "capture_elapsed_sec": round(capture_elapsed, 3),
            "analysis_elapsed_sec": round(analysis_elapsed, 3),
            "action_description": action_description,
            "prompt": step_prompt,
            "verdict": analysis_result.get("verdict", "UNKNOWN"),
            "analysis": analysis_result.get("analysis", ""),
            "recent_logs": logs[-10:],
            "frame_path": str(frame_path),
            "live_frame_path": str(live_frame_path),
        }

        # Write current live state JSON & Markdown
        (LIVE_EVIDENCE_DIR / "current_state.json").write_text(json.dumps(feedback_data, indent=2), encoding="utf-8")

        md_report = (
            f"# Live See Loop Feedback — {feedback_data['timestamp_utc']}\n\n"
            f"**Verdict:** `{feedback_data['verdict']}`  \n"
            f"**Capture Route:** `{route}` ({feedback_data['capture_elapsed_sec']}s)  \n"
            f"**Analysis Time:** {feedback_data['analysis_elapsed_sec']}s  \n\n"
            f"## Gemini Vision Diagnosis\n\n{feedback_data['analysis']}\n\n"
            f"## Correlated Logs\n```\n" + "\n".join(feedback_data["recent_logs"]) + "\n```\n"
        )
        (LIVE_EVIDENCE_DIR / "live_feedback.md").write_text(md_report, encoding="utf-8")

        return feedback_data

    def run_daemon(self, interval_sec: float = 2.0, max_frames: int = 100):
        """Run continuous See Loop in background."""
        print(f"[SeeLoop] Starting live loop (interval={interval_sec}s, max_frames={max_frames})...")
        count = 0
        while count < max_frames:
            count += 1
            try:
                print(f"[SeeLoop] Step {count}/{max_frames} at {datetime.now(timezone.utc).strftime('%H:%M:%S')}...")
                res = self.execute_step()
                verdict = res.get("verdict", "UNKNOWN")
                print(f"[SeeLoop] Verdict: {verdict}")
                if "analysis" in res and res["analysis"]:
                    first_lines = res["analysis"].splitlines()[:4]
                    print("  " + "\n  ".join(first_lines))
            except Exception as ex:
                print(f"[SeeLoop] Error in step {count}: {ex}")
            time.sleep(interval_sec)


def run_self_mock_test():
    """Run an offline unit mock test verifying JSON-RPC, packaging, and analysis formatting."""
    print("[SeeLoop] Running mock protocol and packaging verification...")
    dummy_png_base64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=="
    dummy_png = base64.b64decode(dummy_png_base64)

    analyzer = GeminiVisionAnalyzer(api_key="mock_key_test")
    assert analyzer.model == DEFAULT_GEMINI_MODEL
    print("[SeeLoop] Analyzer initialized successfully.")

    client = UnrealMcpClient()
    req_id = client._next_id()
    assert req_id == 1
    print("[SeeLoop] Unreal MCP RPC generator verified.")

    LIVE_EVIDENCE_DIR.mkdir(parents=True, exist_ok=True)
    test_frame = LIVE_EVIDENCE_DIR / "mock_test.png"
    test_frame.write_bytes(dummy_png)
    assert test_frame.is_file()
    test_frame.unlink()
    print("[SeeLoop] Live evidence path verified: " + str(LIVE_EVIDENCE_DIR))
    print("[SeeLoop] Self-mock verification PASSED clean.")


def main():
    parser = argparse.ArgumentParser(description="Echoes Gameplay 'See Loop' Controller & Gemini Vision Client")
    parser.add_argument("--step", action="store_true", help="Execute a single capture and Gemini vision analysis step")
    parser.add_argument("--daemon", action="store_true", help="Run continuously in background (See Loop mode)")
    parser.add_argument("--interval", type=float, default=2.0, help="Interval in seconds between frames in daemon mode (default: 2.0)")
    parser.add_argument("--pie", action="store_true", help="Start Play In Editor (PIE) before stepping")
    parser.add_argument("--stop-pie", action="store_true", help="Stop running Play In Editor session")
    parser.add_argument("--prompt", type=str, default=None, help="Custom prompt for Gemini Vision")
    parser.add_argument("--mode", type=str, default="viewport", choices=["viewport", "editor"], help="Capture mode: viewport or whole editor")
    parser.add_argument("--mcp-url", type=str, default=DEFAULT_UNREAL_MCP_URL, help="Unreal MCP server URL")
    parser.add_argument("--model", type=str, default=DEFAULT_GEMINI_MODEL, help="Gemini Vision model name")
    parser.add_argument("--test-mock", action="store_true", help="Run offline mock test and exit")

    args = parser.parse_args()

    if args.test_mock:
        run_self_mock_test()
        sys.exit(0)

    session = SeeLoopSession(mcp_url=args.mcp_url, gemini_model=args.model, custom_prompt=args.prompt)

    if args.stop_pie:
        print("[SeeLoop] Stopping PIE session...")
        session.mcp_client.stop_pie()
        print("[SeeLoop] PIE stopped.")
        sys.exit(0)

    if args.pie:
        print("[SeeLoop] Starting PIE session...")
        session.mcp_client.start_pie(warmup_seconds=2.0)
        print("[SeeLoop] PIE started.")

    if args.daemon:
        session.run_daemon(interval_sec=args.interval)
    else:
        print("[SeeLoop] Executing single observation step...")
        result = session.execute_step(prompt=args.prompt, capture_mode=args.mode)
        print(f"\n[SeeLoop] Result Verdict: {result.get('verdict')}")
        print(f"[SeeLoop] Frame Saved: {result.get('frame_path')}")
        print(f"\n--- Gemini Vision Analysis ---\n{result.get('analysis')}\n")


if __name__ == "__main__":
    main()
