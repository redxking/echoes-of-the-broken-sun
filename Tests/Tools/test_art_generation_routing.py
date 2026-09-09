"""Exercise the art shell entry point without launching Unreal.

Author: Angelis Pseftis.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

LAUNCHER_FIXTURE = '''import json, os, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
args = sys.argv[1:]
(root / "invocation.json").write_text(json.dumps(args))
report = Path(args[args.index("--report-dir") + 1]) / "SaveIsolation"
report.mkdir(parents=True)
mode = os.environ.get("ART_ROUTING_FIXTURE", "pass")
result = {"synthetic_denial_probe": True, "protected_policy_clauses_verified": True,
          "scoped_save_directory_empty_after_run": True, "cleanup_succeeded": True,
          "prelaunch_failure": False}
if mode in result:
    result[mode] = not result[mode]
if mode != "missing_report":
    (report / "launcher-result.json").write_text(json.dumps(result))
log = Path(next(arg.split("=", 1)[1] for arg in args if arg.startswith("-abslog=")))
log.write_text("[ECHOES_EVACUATION_PROPS_READY] assets=6 lods=2 collision=0\\n"
               if mode != "missing_ready" else "incomplete generation\\n")
raise SystemExit(7 if mode == "engine_failure" else 0)
'''


class ArtGenerationRoutingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="EchoesArtRouting.")
        self.root = Path(self.temp.name).resolve() / "project with spaces"
        scripts = self.root / "Scripts"
        scripts.mkdir(parents=True)
        shutil.copy2(ROOT / "Scripts/generate_art_assets.sh", scripts)
        (scripts / "echoes_test_sandbox.py").write_text(LAUNCHER_FIXTURE)
        (scripts / "generate_art_assets.py").write_text("# Fixture only\n")
        (self.root / "EchoesOfTheBrokenSun.uproject").write_text("{}")
        self.engine = self.root / "engine fixture"
        editor = self.engine / "Engine/Binaries/Mac/UnrealEditor-Cmd"
        editor.parent.mkdir(parents=True)
        editor.write_text('#!/bin/zsh\ntouch "${0:A:h}/DIRECT_LAUNCH"\nexit 77\n')
        editor.chmod(0o755)
        self.editor = editor

    def tearDown(self):
        self.temp.cleanup()

    def run_fixture(self, mode="pass"):
        env = {key: value for key, value in os.environ.items()
               if not key.startswith("ECHOES_")}
        env.update(UE_ROOT=str(self.engine), ECHOES_EVACUATION_PROPS_ONLY="1",
                   ART_ROUTING_FIXTURE=mode)
        result = subprocess.run(["/bin/zsh", str(self.root / "Scripts/generate_art_assets.sh")],
                                env=env, capture_output=True, text=True, timeout=15)
        self.assertFalse((self.editor.parent / "DIRECT_LAUNCH").exists())
        return result

    def test_generation_routes_through_launcher_with_original_generator_arguments(self):
        result = self.run_fixture()
        self.assertEqual(result.returncode, 0, result.stderr)
        args = json.loads((self.root / "invocation.json").read_text())
        self.assertEqual(args[args.index("--editor") + 1], str(self.editor))
        self.assertEqual(args[args.index("--project") + 1],
                         str(self.root / "EchoesOfTheBrokenSun.uproject"))
        self.assertIn("--reuse-local-ddc", args[:args.index("--")])
        report = Path(args[args.index("--report-dir") + 1])
        self.assertTrue(report.is_relative_to(self.root / "BuildArtifacts/Evidence"))
        engine_args = args[args.index("--") + 1:]
        self.assertEqual(engine_args, ["-unattended", "-nop4", "-nosplash", "-nullrhi",
            "-NoSound", "-SCCProvider=None",
            "-ExecutePythonScript=" + str(self.root / "Scripts/generate_art_assets.py"),
            "-abslog=" + str(self.root / "Saved/Logs/ArtAssetGeneration.log")])

    def test_unproven_isolation_refuses_success(self):
        for mode in ("synthetic_denial_probe", "protected_policy_clauses_verified",
                     "scoped_save_directory_empty_after_run", "cleanup_succeeded",
                     "prelaunch_failure"):
            with self.subTest(mode=mode):
                self.assertEqual(self.run_fixture(mode).returncode, 8)

    def test_missing_report_engine_failure_and_missing_asset_ready_refuse_success(self):
        for mode, code in (("missing_report", 9), ("engine_failure", 7), ("missing_ready", 1)):
            with self.subTest(mode=mode):
                self.assertEqual(self.run_fixture(mode).returncode, code)


if __name__ == "__main__":
    unittest.main()
