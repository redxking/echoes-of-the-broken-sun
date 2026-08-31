#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import os
import pathlib
import plistlib
import shutil
import shlex
import signal
import subprocess
import sys
import tempfile
import time
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
RUNNER = PROJECT_ROOT / "Scripts" / "soak_packaged_sustained_macos.sh"
PACKAGER = PROJECT_ROOT / "Scripts" / "package_macos.sh"
FINALIZER = PROJECT_ROOT / "Scripts" / "finalize_sustained_evidence.py"
PREFLIGHT_VERIFIER = PROJECT_ROOT / "Scripts" / "validate_sustained_preflight.py"


class SustainedSoakWrapperTests(unittest.TestCase):
    @staticmethod
    def _embedded_program(marker: str) -> str:
        source = RUNNER.read_text(encoding="utf-8")
        prefix = f"<<'{marker}'"
        suffix = f"\n{marker}\n"
        marker_start = source.index(prefix)
        program_start = source.index("\n", marker_start) + 1
        program_end = source.index(suffix, program_start)
        return source[program_start:program_end]

    @staticmethod
    def _shell_function(name: str) -> str:
        source = RUNNER.read_text(encoding="utf-8")
        start = source.index(f"{name}() {{")
        end = source.index("\n}\n", start) + 3
        return source[start:end]

    @staticmethod
    def _scan(
        program: str,
        log: pathlib.Path,
        state: list[int],
        forbidden: str,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                "/usr/bin/python3",
                "-c",
                program,
                str(log),
                *(str(value) for value in state),
                forbidden,
            ],
            check=False,
            capture_output=True,
            text=True,
        )

    @staticmethod
    def _next_scan_state(fields: list[int]) -> list[int]:
        return list(fields)

    def _make_manifested_fake_package(
        self, root: pathlib.Path
    ) -> tuple[pathlib.Path, pathlib.Path]:
        project = root / "project"
        scripts = project / "Scripts"
        scripts.mkdir(parents=True)
        for name in (
            "soak_packaged_sustained_macos.sh",
            "validate_sustained_soak_log.py",
            "package_macos.sh",
            "verify_packaged_app.py",
            "finalize_sustained_evidence.py",
            "validate_sustained_preflight.py",
        ):
            shutil.copy2(PROJECT_ROOT / "Scripts" / name, scripts / name)

        subprocess.run(
            ["/usr/bin/git", "init", "-b", "main", str(project)],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            ["/usr/bin/git", "-C", str(project), "add", "Scripts"],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            [
                "/usr/bin/git",
                "-C",
                str(project),
                "-c",
                "user.name=Echoes Test",
                "-c",
                "user.email=echoes-test.invalid@example.invalid",
                "-c",
                "commit.gpgsign=false",
                "commit",
                "-m",
                "Test sustained wrapper",
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        remote = root / "origin.git"
        subprocess.run(
            ["/usr/bin/git", "init", "--bare", str(remote)],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            [
                "/usr/bin/git",
                "-C",
                str(project),
                "remote",
                "add",
                "origin",
                str(remote),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            ["/usr/bin/git", "-C", str(project), "push", "-u", "origin", "main"],
            check=True,
            capture_output=True,
            text=True,
        )
        commit = subprocess.run(
            ["/usr/bin/git", "-C", str(project), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()

        package = root / "package"
        app = package / "EchoesOfTheBrokenSun.app"
        executable = app / "Contents" / "MacOS" / "EchoesOfTheBrokenSun"
        executable.parent.mkdir(parents=True)
        executable.write_text(
            "#!/bin/zsh\n"
            "trap 'exit 143' TERM\n"
            "print '[ECHOES_STRESS_SUSTAINED_STABILIZED] tick=0 stableFrames=20 stableWallUs=1000000'\n"
            "print '[ECHOES_STRESS_SUSTAINED_READY] fixture=Stress400Sustained tick=0'\n"
            "tick=0\n"
            "while true; do\n"
            "  tick=$((tick + 20))\n"
            "  print \"[ECHOES_STRESS_SUSTAINED_HEARTBEAT] fixture=Stress400Sustained tick=$tick wall_ms=$((tick * 50))\"\n"
            "  sleep 1\n"
            "done\n",
            encoding="utf-8",
        )
        executable.chmod(0o755)
        with (app / "Contents" / "Info.plist").open("wb") as handle:
            plistlib.dump(
                {
                    "CFBundleExecutable": "EchoesOfTheBrokenSun",
                    "CFBundleIdentifier": "com.angelispseftis.echoes-test",
                    "CFBundlePackageType": "APPL",
                    "CFBundleShortVersionString": "0.93.0",
                    "CFBundleVersion": "1",
                },
                handle,
            )
        subprocess.run(
            [
                "/usr/bin/codesign",
                "--force",
                "--deep",
                "--sign",
                "-",
                str(app),
            ],
            check=True,
            capture_output=True,
            text=True,
        )

        normal_smoke = package / "EchoesOfTheBrokenSun.normal-startup-smoke.log"
        stress_smoke = package / "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
        normal_smoke.write_text("bounded normal startup\n", encoding="utf-8")
        stress_smoke.write_text("bounded stress startup\n", encoding="utf-8")

        def digest(path: pathlib.Path) -> str:
            return hashlib.sha256(path.read_bytes()).hexdigest()

        records = []
        for candidate in sorted(app.rglob("*")):
            if candidate.is_file() and not candidate.is_symlink():
                records.append(
                    f"{digest(candidate)}  {candidate.relative_to(app).as_posix()}"
                )
        manifest = package / "EchoesOfTheBrokenSun.manifest.txt"
        manifest.write_text(
            "\n".join(
                (
                    "artifact=EchoesOfTheBrokenSun.app",
                    "created_utc=20260831T060000Z",
                    f"source_commit={commit}",
                    f"origin_main={commit}",
                    f"remote_main={commit}",
                    "source_tree=clean",
                    "source_binding=clean-pushed-main",
                    "configuration=Development",
                    "platform=Mac-arm64",
                    f"normal_startup_smoke={normal_smoke.name}",
                    f"normal_startup_smoke_sha256={digest(normal_smoke)}",
                    f"legacy_stress_startup_smoke={stress_smoke.name}",
                    f"legacy_stress_startup_smoke_sha256={digest(stress_smoke)}",
                    "bundle_short_version=0.93.0",
                    "",
                    "sha256  relative_path",
                    *records,
                    "",
                )
            ),
            encoding="utf-8",
        )
        (package / "EchoesOfTheBrokenSun.manifest.sha256").write_text(
            f"{digest(manifest)}  {manifest.name}\n",
            encoding="utf-8",
        )
        return scripts / "soak_packaged_sustained_macos.sh", app

    def _wait_for_active_fake_attempt(
        self,
        process: subprocess.Popen[str],
        final_evidence: pathlib.Path,
    ) -> pathlib.Path:
        deadline = time.monotonic() + 20.0
        pattern = f".{final_evidence.name}.incomplete.*"
        while time.monotonic() < deadline:
            if process.poll() is not None:
                _, stderr = process.communicate()
                self.fail(
                    f"Sustained wrapper exited before the test gate: "
                    f"{process.returncode}: {stderr}"
                )
            candidates = list(final_evidence.parent.glob(pattern))
            if len(candidates) == 1:
                log = candidates[0] / "packaged_sustained_soak.log"
                samples = candidates[0] / "process_samples.csv"
                if (
                    log.is_file()
                    and "[ECHOES_STRESS_SUSTAINED_READY]"
                    in log.read_text(encoding="utf-8")
                    and samples.is_file()
                    and len(samples.read_text(encoding="utf-8").splitlines()) >= 2
                ):
                    return candidates[0]
            time.sleep(0.05)
        self.fail("Sustained wrapper did not reach its active first-sample gate")

    def test_shell_contracts_parse(self) -> None:
        subprocess.run(
            ["/bin/zsh", "-n", str(RUNNER), str(PACKAGER)],
            check=True,
            capture_output=True,
            text=True,
        )

    def test_incremental_log_scanner_preserves_partial_lines_and_state(self) -> None:
        program = self._embedded_program("PY_SCAN")
        forbidden = r"\[ECHOES_[A-Z0-9_]*FAILED\]|Fatal error:"
        initial_state = [0] * 10

        with tempfile.TemporaryDirectory() as temporary:
            log = pathlib.Path(temporary) / "runtime.log"
            complete_prefix = (
                b"LogEchoes: [ECHOES_STRESS_SUSTAINED_READY] tick=0\n"
                b"LogEchoes: [ECHOES_STRESS_SUSTAINED_HEARTBEAT] "
                b"fixture=Stress400Sustained tick=20 checksum=1\n"
            )
            log.write_bytes(
                complete_prefix
                + b"LogEchoes: [ECHOES_STRESS_SUSTAINED_HEARTBEAT] tick=4"
            )
            first = self._scan(program, log, initial_state, forbidden)
            self.assertEqual(first.returncode, 0, first.stderr)
            first_fields = [int(value) for value in first.stdout.split()]
            self.assertEqual(len(first_fields), 10)
            self.assertEqual(first_fields[0], len(complete_prefix))
            self.assertGreater(first_fields[1], 0)
            self.assertGreater(first_fields[2], 0)
            self.assertEqual(first_fields[3], log.stat().st_size)
            self.assertEqual(first_fields[4], len(complete_prefix))
            self.assertEqual(first_fields[5], log.stat().st_size - len(complete_prefix))
            self.assertEqual(first_fields[6:], [0, 1, 0, 20])

            with log.open("ab") as handle:
                handle.write(
                    b"0 checksum=2\n"
                    b"LogEchoes: [ECHOES_STRESS_SUSTAINED_QUALIFIED] tick=40\n"
                    b"fAtAl Er"
                )
            second = self._scan(
                program,
                log,
                self._next_scan_state(first_fields),
                forbidden,
            )
            self.assertEqual(second.returncode, 0, second.stderr)
            second_fields = [int(value) for value in second.stdout.split()]
            self.assertLess(second_fields[0], log.stat().st_size)
            self.assertEqual(second_fields[1:4], first_fields[1:3] + [log.stat().st_size])
            self.assertEqual(second_fields[5], len(b"fAtAl Er"))
            self.assertEqual(second_fields[6:], [0, 1, 1, 40])

            with log.open("ab") as handle:
                handle.write(b"RoR: synthetic regression marker\n")
            split_forbidden = self._scan(
                program,
                log,
                self._next_scan_state(second_fields),
                forbidden,
            )
            self.assertEqual(split_forbidden.returncode, 0, split_forbidden.stderr)
            split_fields = [int(value) for value in split_forbidden.stdout.split()]
            self.assertEqual(split_fields[0], log.stat().st_size)
            self.assertEqual(split_fields[5], 0)
            self.assertEqual(split_fields[6:], [1, 1, 1, 40])

            unchanged = self._scan(
                program,
                log,
                self._next_scan_state(split_fields),
                forbidden,
            )
            self.assertEqual(unchanged.returncode, 0, unchanged.stderr)
            unchanged_fields = [int(value) for value in unchanged.stdout.split()]
            self.assertEqual(unchanged_fields, split_fields)

    def test_incremental_log_scanner_fails_closed_on_reset_or_replacement(self) -> None:
        program = self._embedded_program("PY_SCAN")
        forbidden = r"\[ECHOES_[A-Z0-9_]*FAILED\]|Fatal error:"
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            log = root / "runtime.log"
            log.write_bytes(
                b"[ECHOES_STRESS_SUSTAINED_READY] [ECHOES_STRESS_SUSTAINED_READY]\n"
                b"[ECHOES_STRESS_SUSTAINED_QUALIFIED] "
                b"[ECHOES_STRESS_SUSTAINED_QUALIFIED]\n"
                b"[ECHOES_ALPHA_FAILED] [ECHOES_BETA_FAILED]\n"
                b"[ECHOES_STRESS_SUSTAINED_HEARTBEAT] tick=20 "
                b"[ECHOES_STRESS_SUSTAINED_HEARTBEAT] tick=40\n"
            )
            first = self._scan(program, log, [0] * 10, forbidden)
            self.assertEqual(first.returncode, 0, first.stderr)
            fields = [int(value) for value in first.stdout.split()]
            self.assertEqual(fields[6:], [2, 2, 2, 40])

            regressed_state = self._next_scan_state(fields)
            regressed_state[0] -= 1
            regressed = self._scan(program, log, regressed_state, forbidden)
            self.assertNotEqual(regressed.returncode, 0)
            self.assertIn("cursor and partial-line state disagree", regressed.stderr)

            corrupted_total_state = self._next_scan_state(fields)
            corrupted_total_state[4] += 1
            corrupted_total = self._scan(
                program, log, corrupted_total_state, forbidden
            )
            self.assertNotEqual(corrupted_total.returncode, 0)
            self.assertIn("processed-byte state disagrees", corrupted_total.stderr)

            log.write_bytes(b"short\n")
            truncated = self._scan(
                program, log, self._next_scan_state(fields), forbidden
            )
            self.assertNotEqual(truncated.returncode, 0)
            self.assertIn("shrank", truncated.stderr)

            replacement = root / "replacement.log"
            replacement.write_bytes(
                b"replacement with enough bytes to exceed prior size" * 20 + b"\n"
            )
            os.replace(replacement, log)
            replaced = self._scan(
                program, log, self._next_scan_state(fields), forbidden
            )
            self.assertNotEqual(replaced.returncode, 0)
            self.assertIn("changed identity", replaced.stderr)

            malformed = self._scan(
                program,
                log,
                [-1, *([0] * 9)],
                forbidden,
            )
            self.assertNotEqual(malformed.returncode, 0)
            self.assertIn("cannot be negative", malformed.stderr)

    def test_incremental_log_scanner_work_is_bounded_as_log_grows(self) -> None:
        program = self._embedded_program("PY_SCAN")
        forbidden = r"\[ECHOES_[A-Z0-9_]*FAILED\]|Fatal error:"
        with tempfile.TemporaryDirectory() as temporary:
            log = pathlib.Path(temporary) / "runtime.log"
            neutral_line = b"LogEchoes: Display: bounded neutral runtime record\n"
            repeats = 6 * 1024 * 1024 // len(neutral_line) + 1
            log.write_bytes(neutral_line * repeats)
            state = [0] * 10
            started = time.perf_counter()
            first = self._scan(program, log, state, forbidden)
            self.assertEqual(first.returncode, 0, first.stderr)
            fields = [int(value) for value in first.stdout.split()]
            state = self._next_scan_state(fields)
            for tick in range(20, 660, 20):
                appended = (
                    "LogEchoes: [ECHOES_STRESS_SUSTAINED_HEARTBEAT] "
                    f"fixture=Stress400Sustained tick={tick} checksum={tick}\n"
                ).encode("ascii")
                with log.open("ab") as handle:
                    handle.write(appended)
                observed = self._scan(program, log, state, forbidden)
                self.assertEqual(observed.returncode, 0, observed.stderr)
                fields = [int(value) for value in observed.stdout.split()]
                state = self._next_scan_state(fields)
            elapsed = time.perf_counter() - started
            self.assertEqual(fields[0], log.stat().st_size)
            self.assertEqual(fields[4], log.stat().st_size)
            self.assertEqual(fields[9], 640)
            self.assertLess(elapsed, 10.0)

    def test_monotonic_sample_timing_keeps_exact_two_second_limit(self) -> None:
        program = self._embedded_program("PY_SAMPLE_TIMING")

        def observe(observed_nanoseconds: int) -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [
                    "/usr/bin/python3",
                    "-c",
                    program,
                    "1000000000",
                    "5",
                    str(observed_nanoseconds),
                ],
                check=False,
                capture_output=True,
                text=True,
            )

        on_time = observe(6_000_000_000)
        self.assertEqual(on_time.returncode, 0, on_time.stderr)
        self.assertEqual(on_time.stdout, "5 0\n")
        exact_limit = observe(8_000_000_000)
        self.assertEqual(exact_limit.returncode, 0, exact_limit.stderr)
        self.assertEqual(exact_limit.stdout, "7 2000000000\n")
        late = observe(8_000_000_001)
        self.assertEqual(late.returncode, 15)

    def test_completion_waits_for_sample_boundary_crossed_during_log_scan(self) -> None:
        function = self._shell_function("sustained_completion_ready")

        def check(next_sample: int, qualified_count: int = 1) -> int:
            script = "\n".join(
                (
                    "elapsed=60",
                    "duration_seconds=60",
                    f"next_sample={next_sample}",
                    "last_sample_boundary=60",
                    "final_tick=1200",
                    "required_tick=1200",
                    "requires_qualification=1",
                    f"qualified_count={qualified_count}",
                    function,
                    "sustained_completion_ready",
                )
            )
            return subprocess.run(
                ["/bin/zsh", "-c", script],
                check=False,
                capture_output=True,
                text=True,
            ).returncode

        # The post-scan clock crossed 60 seconds, but the due 60-second
        # observation has not run yet because next_sample still names it.
        self.assertNotEqual(check(60), 0)
        self.assertEqual(check(65), 0)
        self.assertNotEqual(check(65, qualified_count=0), 0)

    def test_abort_provenance_records_cadence_and_each_external_signal(self) -> None:
        program = self._embedded_program("PY_ABORT")
        cases = (
            ("process_sample_cadence_missed", "15", "none"),
            ("external_signal_int", "130", "INT"),
            ("external_signal_term", "143", "TERM"),
            ("external_signal_hup", "129", "HUP"),
            ("external_signal_quit", "131", "QUIT"),
        )
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for index, (reason, exit_code, termination_signal) in enumerate(cases):
                record = root / f"abort-{index}.txt"
                observed = subprocess.run(
                    [
                        "/usr/bin/python3",
                        "-c",
                        program,
                        str(record),
                        "20260831T060000Z",
                        reason,
                        "Synthetic exact abort detail.",
                        "123",
                        "120",
                        "456",
                        "450",
                        "833",
                        "830",
                        exit_code,
                        "TERM",
                        termination_signal,
                    ],
                    check=False,
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(observed.returncode, 0, observed.stderr)
                values = dict(
                    line.split("=", 1)
                    for line in record.read_text(encoding="utf-8").splitlines()
                )
                self.assertEqual(values["reason"], reason)
                self.assertEqual(values["detail"], "Synthetic exact abort detail.")
                self.assertEqual(values["wrapper_exit_code"], exit_code)
                self.assertEqual(values["termination_signal"], termination_signal)
                self.assertEqual(values["elapsed_seconds"], "833")
                self.assertEqual(values["next_sample_boundary_seconds"], "830")
                self.assertEqual(values["cleanup_signal"], "TERM")

    @unittest.skipUnless(sys.platform == "darwin", "zsh signal supervision contract")
    def test_terminal_finalizer_supervisor_fails_precommit_and_accepts_commit(self) -> None:
        handle_signal = self._shell_function("handle_signal")
        wait_for_finalizer = self._shell_function("wait_for_terminal_finalizer")

        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for mode, expected_status in (("precommit", 143), ("committed", 0)):
                with self.subTest(mode=mode):
                    staging = root / f".{mode}.incomplete"
                    final = root / mode
                    ready = root / f"{mode}.ready"
                    result = root / f"{mode}.result"
                    staging.mkdir()
                    child = "\n".join(
                        (
                            "import os, pathlib, signal, sys, time",
                            "mode, staging, final, ready = sys.argv[1:]",
                            "if mode == 'committed':",
                            "    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))",
                            "    os.rename(staging, final)",
                            "pathlib.Path(ready).write_text('ready\\n', encoding='utf-8')",
                            "while True:",
                            "    time.sleep(1)",
                        )
                    )
                    harness = "\n".join(
                        (
                            "set -u",
                            "termination_signal=none",
                            "abort_reason=unexpected_wrapper_exit",
                            "abort_detail=unexpected",
                            "publication_committed=0",
                            "finalizer_pid=''",
                            "finalizer_reaped=0",
                            handle_signal,
                            wait_for_finalizer,
                            "trap 'handle_signal TERM 143' TERM",
                            f"/usr/bin/python3 -c {shlex.quote(child)} {mode!r} {str(staging)!r} {str(final)!r} {str(ready)!r} &",
                            "finalizer_pid=$!",
                            "wait_for_terminal_finalizer",
                            "observed_status=$?",
                            f"if [[ -d {str(final)!r} && ! -e {str(staging)!r} ]]; then",
                            "  publication_committed=1",
                            "  observed_status=0",
                            "fi",
                            f"print -r -- \"status=$observed_status signal=$termination_signal committed=$publication_committed\" > {str(result)!r}",
                            "exit $observed_status",
                        )
                    )
                    process = subprocess.Popen(
                        ["/bin/zsh", "-c", harness],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True,
                    )
                    deadline = time.monotonic() + 10.0
                    while not ready.exists() and time.monotonic() < deadline:
                        if process.poll() is not None:
                            _, stderr = process.communicate()
                            self.fail(
                                f"Finalizer supervisor exited before ready: {stderr}"
                            )
                        time.sleep(0.01)
                    self.assertTrue(ready.exists(), "fake finalizer never became ready")
                    process.send_signal(signal.SIGTERM)
                    process.send_signal(signal.SIGTERM)
                    _, stderr = process.communicate(timeout=10)
                    self.assertEqual(process.returncode, expected_status, stderr)
                    values = dict(
                        field.split("=", 1)
                        for field in result.read_text(encoding="utf-8").split()
                    )
                    self.assertEqual(values["status"], str(expected_status))
                    self.assertEqual(values["signal"], "TERM")
                    self.assertEqual(
                        values["committed"], "1" if mode == "committed" else "0"
                    )
                    if mode == "precommit":
                        self.assertTrue(staging.is_dir())
                        self.assertFalse(final.exists())
                    else:
                        self.assertFalse(staging.exists())
                        self.assertTrue(final.is_dir())

    @unittest.skipUnless(sys.platform == "darwin", "macOS package contract")
    def test_wrapper_lifecycle_records_real_cadence_and_signal_failures(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            runner, app = self._make_manifested_fake_package(root)
            evidence_root = root / "evidence"
            evidence_root.mkdir()
            historical = evidence_root / ".historical.incomplete.PRESERVE"
            historical.mkdir()
            historical_sentinel = historical / "sentinel.txt"
            historical_sentinel.write_text("preserve exactly\n", encoding="utf-8")
            historical_digest = hashlib.sha256(
                historical_sentinel.read_bytes()
            ).hexdigest()

            cadence_final = evidence_root / "cadence"
            cadence = subprocess.Popen(
                [str(runner), str(app), str(cadence_final), "60"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            cadence_staging = self._wait_for_active_fake_attempt(
                cadence, cadence_final
            )
            cadence_pgid = os.getpgid(cadence.pid)
            os.kill(cadence.pid, signal.SIGSTOP)
            time.sleep(8.0)
            os.kill(cadence.pid, signal.SIGCONT)
            _, cadence_stderr = cadence.communicate(timeout=30)
            self.assertEqual(cadence.returncode, 15, cadence_stderr)
            cadence_values = dict(
                line.split("=", 1)
                for line in (cadence_staging / "qualification_abort.txt")
                .read_text(encoding="utf-8")
                .splitlines()
            )
            self.assertEqual(cadence_values["reason"], "process_sample_cadence_missed")
            self.assertEqual(cadence_values["wrapper_pid"], str(cadence.pid))
            self.assertEqual(cadence_values["wrapper_pgid"], str(cadence_pgid))
            self.assertEqual(cadence_values["game_pgid"], str(cadence_pgid))
            self.assertGreaterEqual(int(cadence_values["elapsed_seconds"]), 8)
            self.assertEqual(cadence_values["next_sample_boundary_seconds"], "5")
            self.assertEqual(cadence_values["wrapper_exit_code"], "15")
            self.assertEqual(cadence_values["cleanup_signal"], "TERM")
            self.assertEqual(cadence_values["termination_signal"], "none")
            self.assertFalse(cadence_final.exists())
            self.assertFalse((cadence_staging / "qualification_completion.json").exists())
            self.assertFalse((cadence_staging / "sustained_evidence.sha256").exists())

            for signal_name, signal_value, exit_code in (
                ("INT", signal.SIGINT, 130),
                ("TERM", signal.SIGTERM, 143),
                ("HUP", signal.SIGHUP, 129),
                ("QUIT", signal.SIGQUIT, 131),
            ):
                final_evidence = evidence_root / f"signal-{signal_name.lower()}"
                process = subprocess.Popen(
                    [str(runner), str(app), str(final_evidence), "60"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                )
                staging = self._wait_for_active_fake_attempt(
                    process, final_evidence
                )
                process_pgid = os.getpgid(process.pid)
                process.send_signal(signal_value)
                _, stderr = process.communicate(timeout=30)
                self.assertEqual(process.returncode, exit_code, stderr)
                values = dict(
                    line.split("=", 1)
                    for line in (staging / "qualification_abort.txt")
                    .read_text(encoding="utf-8")
                    .splitlines()
                )
                self.assertEqual(
                    values["reason"], f"external_signal_{signal_name.lower()}"
                )
                self.assertEqual(values["wrapper_pid"], str(process.pid))
                self.assertEqual(values["wrapper_pgid"], str(process_pgid))
                self.assertEqual(values["game_pgid"], str(process_pgid))
                self.assertEqual(values["wrapper_exit_code"], str(exit_code))
                self.assertEqual(values["termination_signal"], signal_name)
                self.assertEqual(values["cleanup_signal"], "TERM")
                self.assertFalse(final_evidence.exists())
                self.assertFalse((staging / "qualification_completion.json").exists())
                self.assertFalse((staging / "sustained_evidence.sha256").exists())

            self.assertEqual(
                hashlib.sha256(historical_sentinel.read_bytes()).hexdigest(),
                historical_digest,
            )

    def test_minimum_duration_is_executable_and_sampling_consistent(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            rejected = subprocess.run(
                [str(RUNNER), "/missing/Echoes.app", str(root / "evidence"), "59"],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(rejected.returncode, 2)
            self.assertIn("60 through 3600", rejected.stderr)

            admitted = subprocess.run(
                [str(RUNNER), "/missing/Echoes.app", str(root / "evidence"), "60"],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(admitted.returncode, 3)
            self.assertIn("not a manifested Echoes package", admitted.stderr)

            missing_preflight = subprocess.run(
                [str(RUNNER), "/missing/Echoes.app", str(root / "hour"), "3600"],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(missing_preflight.returncode, 2)
            self.assertIn("requires an absolute successful 600-second", missing_preflight.stderr)

    def test_runner_retains_fail_closed_cleanup_and_evidence_contract(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        for required in (
            "trap finalize_exit EXIT",
            "trap 'handle_signal INT 130' INT",
            "trap 'handle_signal TERM 143' TERM",
            "trap 'handle_signal HUP 129' HUP",
            "trap 'handle_signal QUIT 131' QUIT",
            "kill -TERM \"$game_pid\"",
            "kill -KILL \"$game_pid\"",
            "wait \"$game_pid\"",
            "verify_source_binding || ! verify_package_integrity",
            "verify_packaged_app.py",
            '"${raw_log:t}"',
            '"${samples:t}"',
            '"${validation:t}"',
            '"${summary:t}"',
            '"${metadata:t}"',
            '"${runner_copy:t}"',
            '"${validator_copy:t}"',
            '"${packager_copy:t}"',
            '"${package_verifier_copy:t}"',
            '"packager_sha256"',
            "sampled_active_window_resident_memory_peak_mib_le_10240",
            "duration_seconds - observations[-1][0] >= sample_interval",
            "next_sample <= last_sample_boundary",
            "next_sample > last_sample_boundary",
            "sustained_completion_ready",
            "scan_appended_log",
            "log_scan_offset",
            "log_scan_device",
            "log_scan_total_bytes",
            'complete_length = chunk.rfind(b"\\n") + 1',
            "mach_absolute_time",
            "sample_elapsed_after_observation",
            "process_sample_cadence_missed",
            "qualification_abort.txt",
            '"wrapper_exit_code": wrapper_exit_code',
            '"-EchoesSaveGameDirectory=$runtime_state/save-games"',
            '"-UserDir=$runtime_state/user-dir"',
            '"$normal_smoke_name"',
            '"$stress_smoke_name"',
            "run_class=one_hour_qualification",
            '"runtime_contract_qualified_one_hour":',
            'result["all_measured_budgets_pass"]',
            'result["qualified_one_hour"] = False',
            '"${evidence_finalizer_copy:t}"',
            'finalize_sustained_evidence.used.py',
            '--staging-dir "$evidence_dir"',
            '--final-dir "$final_evidence_dir"',
            'wait_for_terminal_finalizer',
            'publication_committed=1',
            '--wrapper-pid "$wrapper_pid"',
            '--wrapper-pgid "$wrapper_pgid"',
            '/usr/bin/python3 "$preflight_verifier_copy"',
            '/usr/bin/python3 "$project_root/Scripts/validate_sustained_soak_log.py"',
            '/usr/bin/python3 - "$samples"',
            '--trusted-preflight-verifier "$project_root/Scripts/validate_sustained_preflight.py"',
            '--terminal',
            'validate_sustained_preflight.py',
            'preflight_evidence_manifest_sha256',
            'preflight_evidence_snapshot.zip',
            'preflight_snapshot_sha256',
            '--snapshot-output "$preflight_snapshot"',
            '"${preflight_binding:t}"',
            '"${preflight_snapshot:t}"',
            '--abort-provenance "$abort_record"',
            '--game-pid "$launched_game_pid"',
            '--game-pgid "$launched_game_pgid"',
        ):
            with self.subTest(required=required):
                self.assertIn(required, source)
        self.assertNotIn(
            '/bin/mv "$staging_evidence_dir" "$final_evidence_dir"', source
        )
        self.assertNotIn('/usr/bin/grep -Eqi "$forbidden_pattern"', source)
        self.assertNotIn('final_tick="$(/usr/bin/awk', source)

        finalizer_source = FINALIZER.read_text(encoding="utf-8")
        for required in (
            "renameatx_np",
            "rename_exclusive = 0x00000004",
            "signal.pthread_sigmask",
            "os._exit(0)",
            "Atomic completion record:",
            "this is not one-hour qualification",
        ):
            with self.subTest(finalizer=required):
                self.assertIn(required, finalizer_source)

        preflight_source = PREFLIGHT_VERIFIER.read_text(encoding="utf-8")
        for required in (
            '"run_class": "ten_minute_preflight"',
            '"requested_active_seconds": 600',
            '"all_measured_budgets_pass": True',
            '"qualified_one_hour": False',
            "TOOL_DIGEST_FIELDS",
            "preflight_evidence_manifest_sha256",
            "recompute_process_measurements",
            "verify_preflight_archive",
            "runtime validation records disagree",
            "_load_exact_sibling",
            "verify_staged_one_hour",
        ):
            with self.subTest(preflight=required):
                self.assertIn(required, preflight_source)
        self.assertNotIn("__import__", preflight_source)

        verifier_source = (
            PROJECT_ROOT / "Scripts" / "verify_packaged_app.py"
        ).read_text(encoding="utf-8")
        for required in (
            "set(file_records) != actual_files",
            "set(link_records) != actual_links",
            "os.readlink(link)",
            "resolved.relative_to(app)",
        ):
            with self.subTest(package_verifier=required):
                self.assertIn(required, verifier_source)

    def test_packager_rechecks_clean_pushed_source(self) -> None:
        source = PACKAGER.read_text(encoding="utf-8")
        for phase in ("before build", "after build", "before manifest", "after manifest"):
            self.assertIn(f'verify_clean_pushed_source "{phase}"', source)
        self.assertIn('print "source_binding=clean-pushed-main"', source)
        self.assertIn('print "origin_main=$origin_commit"', source)


if __name__ == "__main__":
    unittest.main(verbosity=2)
