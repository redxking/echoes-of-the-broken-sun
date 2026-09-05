#!/usr/bin/env python3
"""Focused tests for the macOS no-touch save-test launcher policy."""

from __future__ import annotations

import importlib.util
import argparse
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "Scripts" / "echoes_test_sandbox.py"
SPEC = importlib.util.spec_from_file_location("echoes_test_sandbox", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
SANDBOX = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = SANDBOX
SPEC.loader.exec_module(SANDBOX)


class EchoesTestSandboxPolicyTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="EchoesSandboxPolicy.")
        self.root = Path(self.temporary.name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def test_manifest_routes_resolve_inside_root(self) -> None:
        real_root = self.root / "real root"
        real_root.mkdir()
        alias = self.root / "alias root"
        alias.symlink_to(real_root, target_is_directory=True)
        manifest = SANDBOX.build_manifest(
            alias, alias / "Save Games", alias / "User Dir")
        self.assertEqual(manifest["root"], str(real_root.resolve()))
        self.assertTrue(manifest["save_dir"].startswith(manifest["root"] + "/"))
        SANDBOX.validate_manifest_config(
            manifest, alias, alias / "Save Games", alias / "User Dir")

    def test_symlink_escape_and_forged_manifest_are_rejected(self) -> None:
        root = self.root / "root"
        root.mkdir()
        outside = self.root / "outside"
        outside.mkdir()
        (root / "escape").symlink_to(outside, target_is_directory=True)
        with self.assertRaises(ValueError):
            SANDBOX.build_manifest(root, root / "escape" / "saves", root / "user")

        manifest = SANDBOX.build_manifest(root, root / "saves", root / "user")
        forged = dict(manifest)
        forged["save_dir"] = str(outside / "forged")
        with self.assertRaises(ValueError):
            SANDBOX.validate_manifest_config(forged, root, root / "saves", root / "user")
        missing = dict(manifest)
        missing.pop("user_dir")
        with self.assertRaises(ValueError):
            SANDBOX.validate_manifest_config(missing, root, root / "saves", root / "user")

    def test_policy_escapes_quotes_and_protects_descendants(self) -> None:
        protected = self.root / 'quoted " player saves'
        profile = SANDBOX.build_sandbox_profile([protected])
        self.assertIn('\\"', profile)
        self.assertEqual(profile.count("(deny file-read*"), 1)
        self.assertEqual(profile.count("(deny file-write*"), 1)
        self.assertIn(str(protected.resolve()), profile.replace('\\"', '"'))
        SANDBOX.assert_targeted_deny_clauses(profile, [protected.resolve()])
        with self.assertRaises(ValueError):
            SANDBOX.assert_targeted_deny_clauses("(version 1)\n", [protected])

    def test_lexical_absolute_does_not_require_a_target_to_exist(self) -> None:
        absent = self.root / "does not exist" / "player data"
        self.assertEqual(
            SANDBOX._lexical_absolute(absent),
            Path(str(absent)))

    def test_protected_profile_paths_do_not_resolve_or_stat_targets(self) -> None:
        protected = self.root / "production save target"
        with mock.patch.object(
                Path, "resolve", side_effect=AssertionError("must not resolve")):
            profile = SANDBOX.build_sandbox_profile(
                [SANDBOX._lexical_absolute(protected)], resolve_paths=False)
            SANDBOX.assert_targeted_deny_clauses(
                profile, [SANDBOX._lexical_absolute(protected)])

    def test_editor_command_keeps_paths_and_arguments_atomic(self) -> None:
        root = self.root / "sandbox with spaces"
        root.mkdir()
        command = SANDBOX.build_editor_command(
            "/usr/bin/sandbox-exec", root / "policy.sb", root / "Unreal Editor",
            root / "Project File.uproject", root / "manifest.json",
            root / "Save Games", root / "User Dir",
            root / "Derived Data Cache",
            ["-ExecCmds=Automation RunTests Echoes.; Quit", "-Injected=not split"],
        )
        self.assertIn(f"-EchoesSaveGameDirectory={root / 'Save Games'}", command)
        self.assertIn("-DDC=(InstalledEnginePak,Local)", command)
        self.assertIn(f"-LocalDataCachePath={root / 'Derived Data Cache'}", command)
        self.assertIn("-Injected=not split", command)
        self.assertIn("-notraceserver", command)
        self.assertIn("-traceautostart=0", command)
        self.assertIn(f"-cvarsini={root / 'ConsoleVariables.ini'}", command)
        self.assertEqual(len(command), 16)

    def test_editor_command_rejects_sandbox_route_overrides(self) -> None:
        root = self.root / "sandbox"
        root.mkdir()
        common = (
            "/usr/bin/sandbox-exec", root / "policy.sb", root / "editor",
            root / "project", root / "manifest", root / "saves", root / "user",
            root / "ddc",
        )
        for forbidden in (
            "-UserDir=/elsewhere", "-EchoesSaveGameDirectory=/elsewhere",
            "-EchoesTestSandbox", "-DDC=(Shared)", "-LocalDataCachePath=/elsewhere",
            "-notraceserver=0", "-traceautostart=1",
            "-cvarsini=/elsewhere",
        ):
            with self.subTest(forbidden=forbidden), self.assertRaises(ValueError):
                SANDBOX.build_editor_command(*common, [forbidden])

    def test_timeout_terminates_a_synthetic_process_group(self) -> None:
        status, timed_out, log_path = SANDBOX.run_editor_with_timeout(
            [sys.executable, "-c", "import time; time.sleep(60)"],
            self.root / "report", 1)
        self.assertEqual(status, 124)
        self.assertTrue(timed_out)
        self.assertTrue(log_path.is_file())

    @unittest.skipUnless(sys.platform == "darwin" and shutil.which("sandbox-exec"),
                         "sandbox-exec is unavailable on this host")
    def test_launcher_end_to_end_with_synthetic_project_and_editor(self) -> None:
        project = self.root / "Synthetic.uproject"
        project.write_text("{}\n", encoding="utf-8")
        report = self.root / "report"
        # A stale or caller-provided TMPDIR must not choose where saves are made.
        with mock.patch.object(tempfile, "tempdir", str(self.root / "forbidden temp override")):
            status = SANDBOX.launch(argparse.Namespace(
                editor="/usr/bin/true",
                project=str(project),
                report_dir=str(report),
                editor_args=[],
                timeout_seconds=10,
            ))
        self.assertEqual(status, 0)
        result = json.loads((report / "SaveIsolation" / "launcher-result.json").read_text())
        self.assertTrue(result["synthetic_denial_probe"])
        self.assertTrue(result["protected_policy_clauses_verified"])
        self.assertFalse(result["prelaunch_failure"])
        self.assertTrue(result["cleanup_succeeded"])

    @unittest.skipUnless(sys.platform == "darwin" and shutil.which("sandbox-exec"),
                         "sandbox-exec is unavailable on this host")
    def test_macos_sandbox_exec_denies_synthetic_descendant(self) -> None:
        profile = self.root / "probe.sb"
        protected = self.root / "synthetic player saves"
        profile.write_text(
            SANDBOX.build_sandbox_profile([protected]), encoding="utf-8")
        SANDBOX.run_synthetic_denial_probe(
            shutil.which("sandbox-exec"), profile, self.root)


if __name__ == "__main__":
    unittest.main()
