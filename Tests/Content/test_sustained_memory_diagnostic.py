from __future__ import annotations

import contextlib
import hashlib
import json
import os
import pathlib
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT_ROOT / "Scripts"))
sys.path.insert(0, str(PROJECT_ROOT / "Tests" / "Content"))

import validate_sustained_memory_diagnostic as diagnostic
from test_sustained_soak_validator import valid_log

RUNNER = PROJECT_ROOT / "Scripts" / "run_packaged_sustained_memory_diagnostic_macos.sh"
VALIDATOR = PROJECT_ROOT / "Scripts" / "validate_sustained_memory_diagnostic.py"


def digest(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class DiagnosticFixture:
    def __init__(self, parent: pathlib.Path, suffix: str = "ABC123") -> None:
        self.root = parent / f".MemoryDiagnostic-unit.incomplete.{suffix}"
        self.final = parent / "MemoryDiagnostic-unit"
        self.root.mkdir()
        runtime = self.root / "runtime-state"
        for name in ("save-games", "user-dir", "insights-user-dir"):
            (runtime / name).mkdir(parents=True, exist_ok=True)

        self._write_package_copies()
        shutil.copy2(RUNNER, self.root / diagnostic.RUNNER_COPY_NAME)
        shutil.copy2(VALIDATOR, self.root / diagnostic.VALIDATOR_COPY_NAME)
        shutil.copy2(
            PROJECT_ROOT / "Scripts" / "validate_sustained_soak_log.py",
            self.root / diagnostic.RUNTIME_VALIDATOR_COPY_NAME,
        )
        shutil.copy2(
            PROJECT_ROOT / "Scripts" / "verify_packaged_app.py",
            self.root / diagnostic.PACKAGE_VERIFIER_COPY_NAME,
        )
        (self.root / diagnostic.RAW_LOG_NAME).write_text(
            valid_log(600), encoding="utf-8"
        )
        self._write_samples()
        (self.root / diagnostic.TRACE_NAME).write_bytes(b"2CRT" + b"\0" * 8188)
        (self.root / diagnostic.INSIGHTS_LOG_NAME).write_text(
            "LogInsights: Analysis has completed in 1.0 seconds\n", encoding="utf-8"
        )
        (self.root / diagnostic.TRACE_INSTRUCTIONS_NAME).write_text(
            "Memory Insights channel-presence review\n"
            "Compare tick 2400 with tick 12000.\n"
            "Use analyst-confirmed trace-relative timestamps before export.\n"
            "AaBf is a retained-growth query.\n"
            "The hash covers the launcher executable only, not its module closure or "
            "semantic correctness.\n"
            "This is not qualification evidence.\n"
            "Retain root_cause_established=false.\n",
            encoding="utf-8",
        )
        save_manifest = (
            json.dumps(
                [{"path": "", "type": "root-absent"}],
                ensure_ascii=True,
                sort_keys=True,
                separators=(",", ":"),
            )
            + "\n"
        )
        (self.root / "player-save-before.manifest").write_text(
            save_manifest, encoding="utf-8"
        )
        (self.root / "player-save-after.manifest").write_text(
            save_manifest, encoding="utf-8"
        )
        (self.root / diagnostic.RUNTIME_INVENTORY_NAME).write_bytes(
            diagnostic.canonical_json(
                diagnostic.build_runtime_inventory(self.root / "runtime-state")
            )
        )
        (self.root / diagnostic.LAUNCH_COMMAND_NAME).write_text(
            shlex.join(diagnostic.expected_launch_argv(self.root)) + "\n",
            encoding="utf-8",
        )
        self._write_metadata()

    def _write_package_copies(self) -> None:
        normal = self.root / diagnostic.NORMAL_SMOKE_NAME
        stress = self.root / diagnostic.STRESS_SMOKE_NAME
        normal.write_text("bounded normal startup\n", encoding="utf-8")
        stress.write_text("bounded sustained startup\n", encoding="utf-8")
        self.normal_hash = digest(normal)
        self.stress_hash = digest(stress)
        manifest = self.root / diagnostic.MANIFEST_NAME
        manifest.write_text(
            "\n".join(
                (
                    "artifact=EchoesOfTheBrokenSun.app",
                    "created_utc=20260831T100000Z",
                    f"source_commit={diagnostic.PACKAGE_SOURCE_COMMIT}",
                    f"origin_main={diagnostic.PACKAGE_SOURCE_COMMIT}",
                    f"remote_main={diagnostic.PACKAGE_SOURCE_COMMIT}",
                    "source_tree=clean",
                    "source_binding=clean-pushed-main",
                    "configuration=Development",
                    "platform=Mac-arm64",
                    f"normal_startup_smoke={diagnostic.NORMAL_SMOKE_NAME}",
                    f"normal_startup_smoke_sha256={self.normal_hash}",
                    f"legacy_stress_startup_smoke={diagnostic.STRESS_SMOKE_NAME}",
                    f"legacy_stress_startup_smoke_sha256={self.stress_hash}",
                    "engine_version=5.8.2",
                    "xcode_version=26.6",
                    "bundle_identifier=com.angelispseftis.echoesofthebrokensun",
                    "bundle_short_version=0.93.0",
                    "",
                    "sha256  relative_path",
                    "",
                )
            ),
            encoding="utf-8",
        )
        self.manifest_hash = digest(manifest)
        sidecar = self.root / diagnostic.MANIFEST_DIGEST_NAME
        sidecar.write_text(
            f"{self.manifest_hash}  {diagnostic.MANIFEST_NAME}\n", encoding="utf-8"
        )
        self.manifest_digest_hash = digest(sidecar)

    def _write_samples(self) -> None:
        lines = ["elapsed_seconds,rss_mib,cpu_percent"]
        for elapsed in range(0, 601, 5):
            lines.append(f"{elapsed},{512.0 + elapsed / 100.0:.3f},25.000")
        (self.root / diagnostic.SAMPLES_NAME).write_text(
            "\n".join(lines) + "\n", encoding="utf-8"
        )

    @property
    def patches(self) -> dict[str, str]:
        return {
            "PACKAGE_MANIFEST_SHA256": self.manifest_hash,
            "PACKAGE_MANIFEST_DIGEST_SHA256": self.manifest_digest_hash,
            "NORMAL_SMOKE_SHA256": self.normal_hash,
            "STRESS_SMOKE_SHA256": self.stress_hash,
        }

    def _metadata_values(self) -> dict[str, str]:
        staging = self.root
        trace = staging / diagnostic.TRACE_NAME
        values = {
            "record_type": "Echoes packaged sustained memory diagnostic",
            "evidence_class": "non_qualifying_memory_diagnostic",
            "fixture": "Stress400Sustained",
            "created_utc": "20260831T100000Z",
            "completed_utc": "20260831T101100Z",
            "requested_active_seconds": "600",
            "sample_interval_seconds": "5",
            "process_sample_count": "121",
            "qualification_eligible": "false",
            "diagnostic_instrumentation": "UE5.8_MemoryTrace_full",
            "forced_gc": "false",
            "forced_gc_log_observed": "false",
            "thresholds_modified": "false",
            "root_cause_established": "false",
            "package_source_commit": diagnostic.PACKAGE_SOURCE_COMMIT,
            "diagnostic_tooling_commit": "b" * 40,
            "diagnostic_tooling_origin_main": "b" * 40,
            "diagnostic_tooling_remote_main": "b" * 40,
            "diagnostic_tooling_source_tree": "clean",
            "diagnostic_tooling_binding": "clean-pushed-main",
            "package_source_is_ancestor": "true",
            "diagnostic_diff_paths": ",".join(diagnostic.DIAGNOSTIC_DIFF_PATHS),
            "application": str(diagnostic.APPLICATION),
            "capture_staging_directory": str(staging),
            "requested_final_directory": str(self.final),
            "configuration": "Development",
            "platform": "Mac-arm64",
            "package_version": "0.93.0",
            "package_manifest_sha256": self.manifest_hash,
            "package_manifest_digest_sha256": self.manifest_digest_hash,
            "package_executable_sha256": diagnostic.PACKAGE_EXECUTABLE_SHA256,
            "package_external_seal_sha256": diagnostic.PACKAGE_EXTERNAL_SEAL_SHA256,
            "runner_sha256": digest(staging / diagnostic.RUNNER_COPY_NAME),
            "diagnostic_validator_sha256": digest(
                staging / diagnostic.VALIDATOR_COPY_NAME
            ),
            "runtime_validator_sha256": diagnostic.RUNTIME_VALIDATOR_SHA256,
            "package_verifier_sha256": diagnostic.PACKAGE_VERIFIER_SHA256,
            "normal_startup_smoke_sha256": self.normal_hash,
            "legacy_stress_startup_smoke_sha256": self.stress_hash,
            "launch_command_sha256": digest(staging / diagnostic.LAUNCH_COMMAND_NAME),
            "runtime_log_sha256": digest(staging / diagnostic.RAW_LOG_NAME),
            "process_samples_sha256": digest(staging / diagnostic.SAMPLES_NAME),
            "trace_sha256": digest(trace),
            "trace_size_bytes": str(trace.stat().st_size),
            "trace_channels_requested": "default,Memory",
            "trace_channels_observed": "not_independently_enumerated",
            "trace_parseability": "launcher_exit_zero_and_analysis_completed_marker",
            "trace_parseability_log_sha256": digest(
                staging / diagnostic.INSIGHTS_LOG_NAME
            ),
            "runtime_log_trace_anchor_mapping": "analyst_verification_required",
            "trace_analysis_required": "true",
            "trace_analysis_instructions_sha256": digest(
                staging / diagnostic.TRACE_INSTRUCTIONS_NAME
            ),
            "tick_2400_anchor_present": "true",
            "tick_12000_anchor_present": "true",
            "player_save_before_sha256": digest(
                staging / "player-save-before.manifest"
            ),
            "player_save_after_sha256": digest(staging / "player-save-after.manifest"),
            "player_save_unchanged": "true",
            "isolated_save_game_directory_during_capture": str(
                staging / "runtime-state" / "save-games"
            ),
            "isolated_user_directory_during_capture": str(
                staging / "runtime-state" / "user-dir"
            ),
            "runtime_state_inventory_sha256": digest(
                staging / diagnostic.RUNTIME_INVENTORY_NAME
            ),
            "termination_status": "0",
            "wrapper_pid": "123",
            "wrapper_pgid": "100",
            "game_pid": "456",
            "game_pgid": "456",
            "host_model": "MacBookPro18,3",
            "cpu_brand": "Apple M1 Pro",
            "physical_memory_bytes": "17179869184",
            "macos_version": "26.0",
            "macos_build": "25A000",
            "host_architecture": "arm64",
            "unreal_insights_path": str(diagnostic.UNREAL_INSIGHTS),
            "unreal_insights_sha256": diagnostic.UNREAL_INSIGHTS_SHA256,
            "unreal_insights_sha256_scope": "launcher_executable_only",
            "unreal_insights_parser_module_closure_inventoried": "false",
            "unreal_insights_version": diagnostic.UNREAL_INSIGHTS_VERSION,
            "unreal_insights_build_id": diagnostic.UNREAL_INSIGHTS_BUILD_ID,
            "claim_boundary": diagnostic.CLAIM_BOUNDARY,
        }
        self.assert_metadata_complete(values)
        return values

    @staticmethod
    def assert_metadata_complete(values: dict[str, str]) -> None:
        if set(values) != diagnostic.REQUIRED_METADATA_KEYS:
            raise AssertionError(
                f"fixture metadata drifted: missing={diagnostic.REQUIRED_METADATA_KEYS - set(values)}, "
                f"extra={set(values) - diagnostic.REQUIRED_METADATA_KEYS}"
            )

    def _write_metadata(self) -> None:
        values = self._metadata_values()
        (self.root / diagnostic.METADATA_NAME).write_text(
            "".join(f"{key}={value}\n" for key, value in values.items()),
            encoding="utf-8",
        )

    def replace_metadata(self, key: str, value: str) -> None:
        path = self.root / diagnostic.METADATA_NAME
        lines = path.read_text(encoding="utf-8").splitlines()
        replacement = f"{key}={value}"
        for index, line in enumerate(lines):
            if line.startswith(f"{key}="):
                lines[index] = replacement
                break
        else:
            raise AssertionError(f"metadata key not found: {key}")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    def refresh_hash(self, metadata_key: str, evidence_name: str) -> None:
        self.replace_metadata(metadata_key, digest(self.root / evidence_name))


class SustainedMemoryDiagnosticTests(unittest.TestCase):
    def fixture(self, root: pathlib.Path, suffix: str = "ABC123") -> DiagnosticFixture:
        return DiagnosticFixture(root, suffix)

    @staticmethod
    def shell_function_source(name: str) -> str:
        source = RUNNER.read_text(encoding="utf-8")
        match = re.search(
            rf"^{re.escape(name)}\(\) \{{\n.*?^\}}\n",
            source,
            flags=re.MULTILINE | re.DOTALL,
        )
        if match is None:
            raise AssertionError(f"runner function not found: {name}")
        return match.group(0)

    @contextlib.contextmanager
    def patched(self, fixture: DiagnosticFixture):
        with mock.patch.multiple(diagnostic, **fixture.patches):
            yield

    def assert_rejected(
        self, fixture: DiagnosticFixture, message: str | None = None
    ) -> None:
        with (
            self.patched(fixture),
            self.assertRaises(diagnostic.DiagnosticValidationError) as caught,
        ):
            diagnostic.validate_evidence(fixture.root)
        if message is not None:
            self.assertIn(message, str(caught.exception))

    @staticmethod
    def terminal_publisher_command(
        fixture: DiagnosticFixture, marker: pathlib.Path | None = None
    ) -> list[str]:
        harness = r"""
import importlib.util
import json
import pathlib
import sys
import time

validator_path, patches_json, staging, final, marker = sys.argv[1:]
spec = importlib.util.spec_from_file_location("retained_memory_diagnostic", validator_path)
if spec is None or spec.loader is None:
    raise SystemExit("could not load retained validator")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
for key, value in json.loads(patches_json).items():
    setattr(module, key, value)
if marker != "-":
    original_sha256_file = module.sha256_file

    def pause_after_signal_handlers(path):
        pathlib.Path(marker).write_text("ready\n", encoding="utf-8")
        time.sleep(30)
        return original_sha256_file(path)

    module.sha256_file = pause_after_signal_handlers
sys.argv = [
    validator_path,
    "--evidence-dir",
    staging,
    "--final-dir",
    final,
    "--publish",
]
raise SystemExit(module.main())
"""
        return [
            "/usr/bin/python3",
            "-I",
            "-c",
            harness,
            str(fixture.root / diagnostic.VALIDATOR_COPY_NAME),
            json.dumps(fixture.patches, sort_keys=True),
            str(fixture.root),
            str(fixture.final),
            str(marker) if marker is not None else "-",
        ]

    def test_valid_capture_is_accepted_sealed_and_reverified(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = self.fixture(pathlib.Path(temporary))
            real_fsync = os.fsync
            with self.patched(fixture):
                result = diagnostic.validate_evidence(fixture.root)
                self.assertTrue(result["accepted_non_qualifying_diagnostic"])
                self.assertFalse(result["qualification_eligible"])
                self.assertFalse(result["root_cause_established"])
                diagnostic.seal_evidence(fixture.root)
                with mock.patch.object(
                    diagnostic.os, "fsync", side_effect=real_fsync
                ) as fsync_call:
                    published = diagnostic.publish_evidence(fixture.root, fixture.final)
                verified = diagnostic.verify_seal(fixture.final)
            self.assertEqual(published, fixture.final)
            self.assertGreater(fsync_call.call_count, len(diagnostic.BASE_FILES))
            self.assertEqual(verified, result)
            self.assertTrue(
                (fixture.final / diagnostic.EVIDENCE_INVENTORY_NAME).is_file()
            )
            self.assertTrue(
                (fixture.final / diagnostic.EVIDENCE_MANIFEST_NAME).is_file()
            )

    def test_publication_refuses_existing_and_racing_destinations(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            existing = self.fixture(root, "ABC123")
            with self.patched(existing):
                diagnostic.seal_evidence(existing.root)
                existing.final.mkdir()
                sentinel = existing.final / "sentinel.txt"
                sentinel.write_text("preserve\n", encoding="utf-8")
                with self.assertRaises(diagnostic.DiagnosticValidationError):
                    diagnostic.publish_evidence(existing.root, existing.final)
            self.assertTrue(existing.root.is_dir())
            self.assertEqual(sentinel.read_text(encoding="utf-8"), "preserve\n")

            racing_parent = root / "racing"
            racing_parent.mkdir()
            racing = self.fixture(racing_parent, "ABC124")
            with self.patched(racing):
                diagnostic.seal_evidence(racing.root)

                def create_destination(stage: str) -> None:
                    self.assertEqual(stage, "before_commit")
                    racing.final.mkdir()
                    (racing.final / "racer.txt").write_text(
                        "preserve\n", encoding="utf-8"
                    )

                with self.assertRaises(diagnostic.DiagnosticValidationError):
                    diagnostic.publish_evidence(
                        racing.root,
                        racing.final,
                        failure_injector=create_destination,
                    )
            self.assertTrue(racing.root.is_dir())
            self.assertEqual(
                (racing.final / "racer.txt").read_text(encoding="utf-8"),
                "preserve\n",
            )

    def test_signal_before_publication_commit_leaves_only_staging(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = self.fixture(pathlib.Path(temporary))
            with self.patched(fixture):
                diagnostic.seal_evidence(fixture.root)
                original_handler = signal.getsignal(signal.SIGTERM)

                def interrupted(signum: int, _frame: object) -> None:
                    raise diagnostic.DiagnosticValidationError(
                        f"test publication interrupted by signal {signum}"
                    )

                signal.signal(signal.SIGTERM, interrupted)
                try:
                    with self.assertRaises(diagnostic.DiagnosticValidationError):
                        diagnostic.publish_evidence(
                            fixture.root,
                            fixture.final,
                            failure_injector=lambda _stage: os.kill(
                                os.getpid(), signal.SIGTERM
                            ),
                        )
                finally:
                    signal.signal(signal.SIGTERM, original_handler)
            self.assertTrue(fixture.root.is_dir())
            self.assertFalse(os.path.lexists(fixture.final))

    def test_terminal_publisher_cli_handles_success_and_precommit_signal(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            success_parent = root / "success"
            success_parent.mkdir()
            success = self.fixture(success_parent, "ABC123")
            with self.patched(success):
                diagnostic.seal_evidence(success.root)
            decoy = success_parent / "decoy-live-module"
            decoy.mkdir()
            (decoy / "validate_sustained_soak_log.py").write_text(
                'raise RuntimeError("live runtime validator must not execute")\n',
                encoding="utf-8",
            )
            isolated_environment = os.environ.copy()
            isolated_environment["PYTHONPATH"] = str(decoy)
            completed = subprocess.run(
                self.terminal_publisher_command(success),
                cwd=success_parent,
                env=isolated_environment,
                check=False,
                capture_output=True,
                text=True,
                timeout=10,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertTrue(success.final.is_dir())
            self.assertFalse(os.path.lexists(success.root))

            signal_parent = root / "signal"
            signal_parent.mkdir()
            interrupted = self.fixture(signal_parent, "ABC124")
            with self.patched(interrupted):
                diagnostic.seal_evidence(interrupted.root)
            marker = signal_parent / "publisher-ready"
            process = subprocess.Popen(
                self.terminal_publisher_command(interrupted, marker),
                cwd=signal_parent,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            deadline = time.monotonic() + 5
            while not marker.is_file() and process.poll() is None:
                if time.monotonic() >= deadline:
                    process.kill()
                    self.fail("terminal publisher did not reach the precommit gate")
                time.sleep(0.01)
            self.assertIsNone(process.poll())
            process.send_signal(signal.SIGTERM)
            _stdout, stderr = process.communicate(timeout=5)
            self.assertEqual(process.returncode, 143, stderr)
            self.assertIn("interrupted by signal", stderr)
            self.assertTrue(interrupted.root.is_dir())
            self.assertFalse(os.path.lexists(interrupted.final))

    def test_wrapper_reaps_publisher_for_signals_timeout_cleanup_and_commit(
        self,
    ) -> None:
        game_group_is_safe = self.shell_function_source("game_group_is_safe")
        handle_signal = self.shell_function_source("handle_signal")
        wait_for_publisher = self.shell_function_source("wait_for_terminal_publisher")
        cleanup_children = self.shell_function_source("cleanup_children")
        with tempfile.TemporaryDirectory() as temporary:
            for mode in ("single", "repeated", "timeout", "exit_cleanup", "commit"):
                root = pathlib.Path(temporary) / mode
                root.mkdir()
                if mode == "single":
                    action = "handle_signal TERM 143\nwait_for_terminal_publisher\nobserved_status=$?"
                elif mode == "repeated":
                    action = (
                        "handle_signal TERM 143\nhandle_signal INT 130\n"
                        "wait_for_terminal_publisher\nobserved_status=$?"
                    )
                elif mode == "timeout":
                    action = (
                        "publisher_deadline=$(/bin/date +%s)\n"
                        "wait_for_terminal_publisher\nobserved_status=$?"
                    )
                elif mode == "exit_cleanup":
                    action = (
                        "handle_signal TERM 143\nhandle_signal INT 130\n"
                        "cleanup_children\nobserved_status=137"
                    )
                else:
                    action = "wait_for_terminal_publisher\nobserved_status=$?"
                harness = f"""
{game_group_is_safe}
{handle_signal}
{wait_for_publisher}
{cleanup_children}
publication_committed=0
termination_signal=none
abort_reason=none
abort_detail=none
cleanup_signal=none
publisher_reaped=0
publisher_deadline=0
publisher_signal_deadline=0
publisher_kill_deadline=0
publisher_interrupt_grace=0
publisher_kill_grace=0
game_pid=""
game_pgid=""
launched_game_pid=none
launched_game_pgid=none
game_status=none
game_cleanup_timeout=1
game_group_kill_escalated=0
insights_pid=""
wrapper_pid=$$
wrapper_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$$" | /usr/bin/awk 'NF {{ print $1; exit }}')"
evidence_dir="$1/staging"
final_evidence_dir="$1/final"
/bin/mkdir "$evidence_dir"
if [[ "$2" == commit ]]; then
  /usr/bin/python3 -I -c 'import os,pathlib,sys,time; pathlib.Path(sys.argv[3]).write_text("ready"); time.sleep(0.2); os.rename(sys.argv[1], sys.argv[2]); time.sleep(0.2)' "$evidence_dir" "$final_evidence_dir" "$1/ready" &
else
  /usr/bin/python3 -I -c 'import pathlib,signal,sys,time; [signal.signal(item, signal.SIG_IGN) for item in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGQUIT)]; pathlib.Path(sys.argv[1]).write_text("ready"); time.sleep(30)' "$1/ready" &
fi
publisher_pid=$!
original_publisher_pid=$publisher_pid
publisher_deadline=$(( $(/bin/date +%s) + 30 ))
ready_deadline=$(( $(/bin/date +%s) + 5 ))
while [[ ! -f "$1/ready" ]] && kill -0 "$publisher_pid" 2>/dev/null; do
  (( $(/bin/date +%s) < ready_deadline )) || exit 30
  /bin/sleep 0.05
done
{action}
(( publisher_reaped == 1 )) || exit 31
! kill -0 "$original_publisher_pid" 2>/dev/null || exit 32
if [[ "$2" == commit ]]; then
  (( observed_status == 0 )) || exit 33
  (( publication_committed == 1 )) || exit 34
  [[ -d "$final_evidence_dir" && ! -e "$evidence_dir" ]] || exit 35
  [[ "$termination_signal" == none ]] || exit 36
else
  (( observed_status == 137 )) || exit 37
  [[ "$cleanup_signal" == TERM+KILL ]] || exit 38
  [[ -d "$evidence_dir" && ! -e "$final_evidence_dir" ]] || exit 39
fi
if [[ -n "$publisher_pid" ]]; then
  (( publisher_reaped == 1 )) || exit 40
  publisher_pid=""
fi
cleanup_children
! kill -0 "$original_publisher_pid" 2>/dev/null || exit 41
"""
                completed = subprocess.run(
                    [
                        "/bin/zsh",
                        "-c",
                        harness,
                        "publisher-harness",
                        str(root),
                        mode,
                    ],
                    check=False,
                    capture_output=True,
                    text=True,
                    timeout=12,
                )
                self.assertEqual(
                    completed.returncode,
                    0,
                    f"mode={mode}\nstdout={completed.stdout}\nstderr={completed.stderr}",
                )

    def test_game_launch_group_cleanup_reaps_leader_and_stubborn_descendant(
        self,
    ) -> None:
        function_names = (
            "spawn_game_in_isolated_group",
            "launch_game_in_isolated_group",
            "read_game_group_handshake",
            "capture_game_process_group",
            "game_leader_is_live",
            "game_group_is_safe",
            "game_group_has_live_members",
            "signal_game_group",
            "reap_game_leader",
            "terminate_game_group",
        )
        functions = "\n".join(
            self.shell_function_source(name) for name in function_names
        )
        leader_source = """
import pathlib
import signal
import subprocess
import sys
import time

child_code = (
    "import signal,time; "
    "[signal.signal(item, signal.SIG_IGN) for item in "
    "(signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGQUIT)]; "
    "time.sleep(30)"
)
child = subprocess.Popen([sys.executable, "-I", "-c", child_code])
pathlib.Path(sys.argv[1]).write_text(str(child.pid), encoding="utf-8")
for item in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGQUIT):
    signal.signal(item, signal.SIG_IGN)
time.sleep(30)
"""
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            (root / "leader.py").write_text(leader_source, encoding="utf-8")
            harness = f"""
{functions}
wrapper_pid=$$
wrapper_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$$" | /usr/bin/awk 'NF {{ print $1; exit }}')"
binary=/usr/bin/python3
launch_arguments=(-I "$1/leader.py" "$1/child.pid")
raw_log="$1/game.log"
game_group_handshake="$1/game-process-group.handshake"
game_pid=""
game_pgid=""
launched_game_pid=none
launched_game_pgid=none
game_status=none
game_cleanup_timeout=1
game_group_kill_escalated=0
cleanup_signal=none
launch_game_in_isolated_group || exit 50
original_game_pid=$game_pid
game_group_is_safe || exit 51
[[ "$game_pgid" == "$game_pid" && "$game_pgid" != "$wrapper_pgid" ]] || exit 52
child_deadline=$(( $(/bin/date +%s) + 5 ))
while [[ ! -f "$1/child.pid" ]] && kill -0 "$game_pid" 2>/dev/null; do
  (( $(/bin/date +%s) < child_deadline )) || exit 53
  /bin/sleep 0.05
done
child_pid="$(<"$1/child.pid")"
[[ "$child_pid" == <-> ]] || exit 54
[[ "$(LC_ALL=C /bin/ps -o pgid= -p "$child_pid" | /usr/bin/awk 'NF {{ print $1; exit }}')" == "$game_pgid" ]] || exit 55
terminate_game_group || exit 56
(( game_group_kill_escalated == 1 )) || exit 57
[[ "$cleanup_signal" == TERM+KILL && -z "$game_pid" ]] || exit 58
(( game_status == 137 )) || exit 59
! game_group_has_live_members || exit 60
descendant_deadline=$(( $(/bin/date +%s) + 5 ))
while kill -0 "$child_pid" 2>/dev/null && (( $(/bin/date +%s) < descendant_deadline )); do
  /bin/sleep 0.05
done
! kill -0 "$original_game_pid" 2>/dev/null || exit 61
! kill -0 "$child_pid" 2>/dev/null || exit 62
kill -0 "$$" 2>/dev/null || exit 63
"""
            completed = subprocess.run(
                ["/bin/zsh", "-c", harness, "game-group-harness", str(root)],
                check=False,
                capture_output=True,
                text=True,
                timeout=15,
            )
            self.assertEqual(
                completed.returncode,
                0,
                f"stdout={completed.stdout}\nstderr={completed.stderr}",
            )

    def test_pre_exec_group_handshake_cleans_early_leader_descendant(self) -> None:
        function_names = (
            "spawn_game_in_isolated_group",
            "read_game_group_handshake",
            "game_group_is_safe",
            "game_group_has_live_members",
            "signal_game_group",
            "reap_game_leader",
            "terminate_game_group",
            "wait_for_terminal_publisher",
            "cleanup_children",
        )
        functions = "\n".join(
            self.shell_function_source(name) for name in function_names
        )
        leader_source = """
import pathlib
import signal
import subprocess
import sys
import time

child_code = (
    "import signal,time; "
    "[signal.signal(item, signal.SIG_IGN) for item in "
    "(signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGQUIT)]; "
    "time.sleep(30)"
)
child = subprocess.Popen([sys.executable, "-I", "-c", child_code])
pathlib.Path(sys.argv[1]).write_text(str(child.pid), encoding="utf-8")
time.sleep(0.1)
"""
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            (root / "early-leader.py").write_text(leader_source, encoding="utf-8")
            harness = f"""
{functions}
wrapper_pid=$$
wrapper_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$$" | /usr/bin/awk 'NF {{ print $1; exit }}')"
binary=/usr/bin/python3
launch_arguments=(-I "$1/early-leader.py" "$1/child.pid")
raw_log="$1/game.log"
game_group_handshake="$1/game-process-group.handshake"
game_pid=""
game_pgid=""
launched_game_pid=none
launched_game_pgid=none
game_status=none
game_cleanup_timeout=1
game_group_kill_escalated=0
cleanup_signal=none
insights_pid=""
publisher_pid=""
publisher_reaped=0
publisher_deadline=0
publisher_signal_deadline=0
publisher_kill_deadline=0
publisher_interrupt_grace=0
publisher_kill_grace=0
spawn_game_in_isolated_group || exit 70
original_game_pid=$game_pid
leader_deadline=$(( $(/bin/date +%s) + 5 ))
while kill -0 "$game_pid" 2>/dev/null &&
      [[ "$(LC_ALL=C /bin/ps -o state= -p "$game_pid" | /usr/bin/awk 'NF {{ print $1; exit }}')" != Z* ]]; do
  (( $(/bin/date +%s) < leader_deadline )) || exit 71
  /bin/sleep 0.05
done
child_pid="$(<"$1/child.pid")"
[[ "$child_pid" == <-> ]] || exit 74
child_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$child_pid" 2>/dev/null | /usr/bin/awk 'NF {{ print $1; exit }}')"
[[ "$child_pgid" == <-> && "$child_pgid" != "$wrapper_pgid" ]] || exit 75
cleanup_children
game_group_is_safe || exit 73
(( game_group_kill_escalated == 1 )) || exit 77
! game_group_has_live_members || exit 78
descendant_deadline=$(( $(/bin/date +%s) + 5 ))
while kill -0 "$child_pid" 2>/dev/null && (( $(/bin/date +%s) < descendant_deadline )); do
  /bin/sleep 0.05
done
! kill -0 "$original_game_pid" 2>/dev/null || exit 79
! kill -0 "$child_pid" 2>/dev/null || exit 80
kill -0 "$$" 2>/dev/null || exit 81
"""
            completed = subprocess.run(
                ["/bin/zsh", "-c", harness, "early-leader-harness", str(root)],
                check=False,
                capture_output=True,
                text=True,
                timeout=15,
            )
            self.assertEqual(
                completed.returncode,
                0,
                f"stdout={completed.stdout}\nstderr={completed.stderr}",
            )

    def test_exit_trap_converts_uncommitted_success_to_failure(self) -> None:
        finalize_exit = self.shell_function_source("finalize_exit")
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            harness = f"""
{finalize_exit}
cleanup_children() {{ :; }}
evidence_dir="$1/staging"
/bin/mkdir "$evidence_dir"
publication_committed=0
abort_reason=unexpected_wrapper_exit
abort_detail="unexpected"
wrapper_pid=$$
wrapper_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$$" | /usr/bin/awk 'NF {{ print $1; exit }}')"
launched_game_pid=none
launched_game_pgid=none
elapsed=-1
next_sample=-1
cleanup_signal=none
termination_signal=none
trap finalize_exit EXIT
exit 0
"""
            completed = subprocess.run(
                ["/bin/zsh", "-c", harness, "exit-trap-harness", str(root)],
                check=False,
                capture_output=True,
                text=True,
                timeout=10,
            )
            self.assertEqual(completed.returncode, 20, completed.stderr)
            abort_record = (root / "staging" / "diagnostic_abort.txt").read_text(
                encoding="utf-8"
            )
            self.assertIn("reason=publication_not_committed\n", abort_record)
            self.assertIn("wrapper_exit_code=20\n", abort_record)

    def test_post_commit_parent_fsync_failure_does_not_revoke_publication(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            parent = pathlib.Path(temporary)
            fixture = self.fixture(parent)
            real_fsync_directory = diagnostic._fsync_directory

            def fail_only_for_parent(path: pathlib.Path) -> None:
                if path == parent:
                    raise OSError("directory fsync unsupported")
                real_fsync_directory(path)

            with self.patched(fixture):
                diagnostic.seal_evidence(fixture.root)
                with mock.patch.object(
                    diagnostic,
                    "_fsync_directory",
                    side_effect=fail_only_for_parent,
                ):
                    published = diagnostic.publish_evidence(fixture.root, fixture.final)
                verified = diagnostic.verify_seal(fixture.final)
            self.assertEqual(published, fixture.final)
            self.assertTrue(verified["accepted_non_qualifying_diagnostic"])
            self.assertFalse(os.path.lexists(fixture.root))

    def test_publication_rechecks_manifest_and_rejects_tampering(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = self.fixture(pathlib.Path(temporary))
            with self.patched(fixture):
                diagnostic.seal_evidence(fixture.root)
                with (fixture.root / diagnostic.SAMPLES_NAME).open(
                    "a", encoding="utf-8"
                ) as handle:
                    handle.write("605,999.0,1.0\n")
                with self.assertRaises(diagnostic.DiagnosticValidationError):
                    diagnostic.publish_evidence(fixture.root, fixture.final)
            self.assertTrue(fixture.root.is_dir())
            self.assertFalse(os.path.lexists(fixture.final))

    def test_retained_validator_copy_executes_from_isolated_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            isolated = pathlib.Path(temporary)
            evidence = isolated / "evidence"
            for name in ("save-games", "user-dir", "insights-user-dir"):
                (evidence / "runtime-state" / name).mkdir(parents=True, exist_ok=True)
            retained_validator = isolated / diagnostic.VALIDATOR_COPY_NAME
            retained_runtime = isolated / diagnostic.RUNTIME_VALIDATOR_COPY_NAME
            shutil.copy2(VALIDATOR, retained_validator)
            shutil.copy2(
                PROJECT_ROOT / "Scripts" / "validate_sustained_soak_log.py",
                retained_runtime,
            )
            completed = subprocess.run(
                [
                    "/usr/bin/python3",
                    "-I",
                    str(retained_validator),
                    "--evidence-dir",
                    str(evidence),
                    "--write-runtime-inventory",
                ],
                cwd=isolated,
                check=True,
                capture_output=True,
                text=True,
            )
            result = json.loads(completed.stdout)
            self.assertTrue(result["runtime_state_inventory_written"])
            self.assertTrue((evidence / diagnostic.RUNTIME_INVENTORY_NAME).is_file())

    def test_runner_python_isolation_rejects_hostile_import_environment(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            hostile = pathlib.Path(temporary)
            marker = hostile / "hostile-import-executed.txt"
            payload = (
                f"with open({str(marker)!r}, 'a', encoding='utf-8') as output:\n"
                "    output.write(__name__ + '\\n')\n"
                "raise RuntimeError('hostile shadow module executed')\n"
            )
            for module_name in (
                "csv",
                "hashlib",
                "ctypes",
                "json",
                "pathlib",
                "sitecustomize",
                "usercustomize",
            ):
                (hostile / f"{module_name}.py").write_text(payload, encoding="utf-8")
            environment = os.environ.copy()
            environment.update(
                {
                    "PYTHONPATH": str(hostile),
                    "PYTHONHOME": str(hostile),
                    "PYTHONSTARTUP": str(hostile / "sitecustomize.py"),
                }
            )
            commands = (
                [
                    "/usr/bin/python3",
                    "-I",
                    "-c",
                    "import csv,ctypes,hashlib,json,pathlib; print('isolated')",
                ],
                ["/usr/bin/python3", "-I", str(VALIDATOR), "--help"],
                [
                    "/usr/bin/python3",
                    "-I",
                    str(PROJECT_ROOT / "Scripts" / "verify_packaged_app.py"),
                    "--help",
                ],
            )
            for command in commands:
                completed = subprocess.run(
                    command,
                    cwd=hostile,
                    env=environment,
                    check=False,
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                self.assertEqual(
                    completed.returncode,
                    0,
                    f"command={command!r}\nstderr={completed.stderr}",
                )
                self.assertFalse(marker.exists(), command)

    def test_qualification_and_forced_gc_claims_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            qualification = self.fixture(root, "ABC123")
            qualification.replace_metadata("qualification_eligible", "true")
            self.assert_rejected(qualification, "qualification_eligible")

            forced = self.fixture(root, "ABC124")
            forced.replace_metadata("forced_gc", "true")
            self.assert_rejected(forced, "forced_gc")

            forced_log = self.fixture(root, "ABC125")
            log = forced_log.root / diagnostic.RAW_LOG_NAME
            log.write_text(
                log.read_text(encoding="utf-8").replace(
                    "forcedGc=false", "forcedGc=true", 1
                ),
                encoding="utf-8",
            )
            forced_log.refresh_hash("runtime_log_sha256", diagnostic.RAW_LOG_NAME)
            self.assert_rejected(forced_log, "runtime contract")

            thresholds = self.fixture(root, "ABC126")
            thresholds.replace_metadata("thresholds_modified", "true")
            self.assert_rejected(thresholds, "thresholds_modified")

            root_cause = self.fixture(root, "ABC127")
            root_cause.replace_metadata("root_cause_established", "true")
            self.assert_rejected(root_cause, "root_cause_established")

    def test_missing_trace_channel_configuration_or_provenance_is_rejected(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            metadata_case = self.fixture(root, "ABC123")
            metadata_case.replace_metadata("trace_channels_requested", "default")
            self.assert_rejected(metadata_case, "trace_channels_requested")

            command_case = self.fixture(root, "ABC124")
            command = command_case.root / diagnostic.LAUNCH_COMMAND_NAME
            argv = shlex.split(command.read_text(encoding="utf-8"))
            argv.remove("-trace=default,Memory")
            command.write_text(shlex.join(argv) + "\n", encoding="utf-8")
            command_case.refresh_hash(
                "launch_command_sha256", diagnostic.LAUNCH_COMMAND_NAME
            )
            self.assert_rejected(command_case, "launch command drifted")

            parse_case = self.fixture(root, "ABC125")
            parse_log = parse_case.root / diagnostic.INSIGHTS_LOG_NAME
            parse_log.write_text("session analysis failed to start\n", encoding="utf-8")
            parse_case.refresh_hash(
                "trace_parseability_log_sha256", diagnostic.INSIGHTS_LOG_NAME
            )
            self.assert_rejected(parse_case, "analysis-completed marker")

    def test_process_group_and_insights_provenance_is_strict(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            mutations = (
                ("game_pgid", "455", "process-group leader provenance"),
                ("wrapper_pgid", "456", "distinct from the diagnostic wrapper"),
                ("unreal_insights_sha256_scope", "full_parser", "sha256_scope"),
                (
                    "unreal_insights_parser_module_closure_inventoried",
                    "true",
                    "parser_module_closure_inventoried",
                ),
                ("trace_parseability", "parser_verified", "trace_parseability"),
                (
                    "runtime_log_trace_anchor_mapping",
                    "automated",
                    "runtime_log_trace_anchor_mapping",
                ),
            )
            for index, (key, value, message) in enumerate(mutations, start=1):
                fixture = self.fixture(root, f"PG{index:04d}")
                fixture.replace_metadata(key, value)
                self.assert_rejected(fixture, message)

    def test_missing_tick_2400_or_tick_12000_anchor_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for index, tick in enumerate((2400, 12000), start=1):
                fixture = self.fixture(root, f"ABC12{index}")
                log = fixture.root / diagnostic.RAW_LOG_NAME
                retained = [
                    line
                    for line in log.read_text(encoding="utf-8").splitlines()
                    if f"tick={tick} " not in line
                ]
                log.write_text("\n".join(retained) + "\n", encoding="utf-8")
                fixture.refresh_hash("runtime_log_sha256", diagnostic.RAW_LOG_NAME)
                self.assert_rejected(fixture, "runtime contract")

    def test_malformed_or_qualification_like_metadata_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            duplicate = self.fixture(root, "ABC123")
            metadata = duplicate.root / diagnostic.METADATA_NAME
            with metadata.open("a", encoding="utf-8") as handle:
                handle.write("fixture=Stress400Sustained\n")
            self.assert_rejected(duplicate, "duplicate metadata")

            extra = self.fixture(root, "ABC124")
            metadata = extra.root / diagnostic.METADATA_NAME
            with metadata.open("a", encoding="utf-8") as handle:
                handle.write("qualified_one_hour=true\n")
            self.assert_rejected(extra, "metadata key set drifted")

            missing = self.fixture(root, "ABC125")
            metadata = missing.root / diagnostic.METADATA_NAME
            lines = [
                line
                for line in metadata.read_text(encoding="utf-8").splitlines()
                if not line.startswith("root_cause_established=")
            ]
            metadata.write_text("\n".join(lines) + "\n", encoding="utf-8")
            self.assert_rejected(missing, "metadata key set drifted")

    def test_unsafe_file_layouts_trace_links_and_overwrite_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            extra = self.fixture(root, "ABC123")
            (extra.root / "unexpected.txt").write_text("unsafe\n", encoding="utf-8")
            self.assert_rejected(extra, "top-level layout")

            linked = self.fixture(root, "ABC124")
            trace = linked.root / diagnostic.TRACE_NAME
            target = root / "outside.utrace"
            target.write_bytes(trace.read_bytes())
            trace.unlink()
            trace.symlink_to(target)
            self.assert_rejected(linked, "symlink evidence")

            sealed = self.fixture(root, "ABC125")
            (sealed.root / diagnostic.VALIDATION_NAME).write_text(
                "do not overwrite\n", encoding="utf-8"
            )
            with (
                self.patched(sealed),
                self.assertRaises(diagnostic.DiagnosticValidationError),
            ):
                diagnostic.seal_evidence(sealed.root)

    def test_runtime_inventory_rejects_unsafe_children(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            fixture = self.fixture(root, "ABC123")
            outside = pathlib.Path(temporary) / "outside"
            outside.write_text("x", encoding="utf-8")
            (fixture.root / "runtime-state" / "user-dir" / "escape").symlink_to(outside)
            self.assert_rejected(fixture, "symlink evidence")

            top_level_file = self.fixture(root, "ABC124")
            (top_level_file.root / "runtime-state" / "unexpected.txt").write_text(
                "unsafe\n", encoding="utf-8"
            )
            self.assert_rejected(top_level_file, "three isolated runtime directories")

    def test_shell_and_static_non_qualification_contracts(self) -> None:
        subprocess.run(
            ["/bin/zsh", "-n", str(RUNNER)],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            ["/usr/bin/python3", "-I", "-m", "py_compile", str(VALIDATOR)],
            check=True,
            capture_output=True,
            text=True,
        )
        source = RUNNER.read_text(encoding="utf-8")
        validator_source = VALIDATOR.read_text(encoding="utf-8")
        for required in (
            "-EchoesMemoryDiagnostic",
            "-trace=default,Memory",
            "-tracefile=$trace_file",
            "-tracefiletrunc",
            "-EchoesStress400Sustained",
            "-EchoesSaveGameDirectory=$runtime_state/save-games",
            "-UserDir=$runtime_state/user-dir",
            "sample_interval=5",
            "qualification_eligible=false",
            "root_cause_established=false",
            "os.setsid()",
            "os.getpgrp()",
            "game_group_handshake",
            "game_pgid=$launched_game_pgid",
            'trap - EXIT\n  exit "$exit_status"',
            "unreal_insights_sha256_scope=launcher_executable_only",
            "unreal_insights_parser_module_closure_inventoried=false",
            "runtime_log_trace_anchor_mapping=analyst_verification_required",
        ):
            self.assertIn(required, source)
        self.assertGreaterEqual(
            source.count('/usr/bin/python3 -I "$validator_copy"'), 4
        )
        self.assertGreaterEqual(source.count("if ! verify_trace_launcher_identity"), 3)
        self.assertIsNone(re.search(r"/usr/bin/python3(?!\s+-I(?:\s|$))", source))
        self.assertNotRegex(source, r"(?<!/usr/bin/)\bpython3\b")
        self.assertNotIn(
            '/usr/bin/python3 -I "$project_root/Scripts/'
            'validate_sustained_memory_diagnostic.py"',
            source,
        )
        for required in (
            "git_blob_sha256",
            "verify_retained_tooling_copies",
            "publisher_pid",
            "wait_for_terminal_publisher",
            "terminate_game_group",
            "--publish &",
            "TRACE_EVIDENCE_DIR=",
            "derived-analysis record",
        ):
            self.assertIn(required, source)
        for required in (
            "renameatx_np",
            "RENAME_EXCL".casefold(),
            "pthread_sigmask",
            "_fsync_evidence_tree",
            "os._exit(0)",
        ):
            self.assertIn(required.casefold(), validator_source.casefold())
        self.assertNotRegex(source.casefold(), r"memreport")
        self.assertIsNone(
            re.search(r"(?:^|[\s,;])mem(?:$|[\s,;])", source, re.IGNORECASE)
        )
        self.assertNotIn("finalize_sustained_evidence", source)
        self.assertNotIn("qualification_completion.json", source)
        self.assertNotIn("sustained_evidence.sha256", source)
        self.assertNotIn("parser drifted", source)


if __name__ == "__main__":
    unittest.main(verbosity=2)
