#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import subprocess
import tempfile
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
RUNNER = PROJECT_ROOT / "Scripts" / "soak_packaged_sustained_macos.sh"
PACKAGER = PROJECT_ROOT / "Scripts" / "package_macos.sh"
FINALIZER = PROJECT_ROOT / "Scripts" / "finalize_sustained_evidence.py"
PREFLIGHT_VERIFIER = PROJECT_ROOT / "Scripts" / "validate_sustained_preflight.py"


class SustainedSoakWrapperTests(unittest.TestCase):
    def test_shell_contracts_parse(self) -> None:
        subprocess.run(
            ["/bin/zsh", "-n", str(RUNNER), str(PACKAGER)],
            check=True,
            capture_output=True,
            text=True,
        )

    def test_final_tick_awk_is_macos_compatible(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        prefix = 'final_tick="$(/usr/bin/awk \'\n'
        suffix = '\n  \' "$raw_log")"'
        program_start = source.index(prefix) + len(prefix)
        program_end = source.index(suffix, program_start)
        program = source[program_start:program_end]
        observed = subprocess.run(
            ["/usr/bin/awk", program],
            input=(
                "LogEchoes: [ECHOES_STRESS_SUSTAINED_HEARTBEAT] "
                "fixture=Stress400Sustained tick=20 checksum=1\n"
                "LogEchoes: [ECHOES_STRESS_SUSTAINED_HEARTBEAT] "
                "fixture=Stress400Sustained tick=40 checksum=2\n"
            ),
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(observed.returncode, 0, observed.stderr)
        self.assertEqual(observed.stdout, "40\n")

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
            'exec /usr/bin/python3 "$evidence_finalizer_copy"',
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
        ):
            with self.subTest(required=required):
                self.assertIn(required, source)
        self.assertNotIn(
            '/bin/mv "$staging_evidence_dir" "$final_evidence_dir"', source
        )

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
