#!/usr/bin/env python3

from __future__ import annotations

import json
import pathlib
import sys
import tempfile
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT_ROOT / "Scripts"))

from finalize_sustained_evidence import SUMMARY_NAME  # noqa: E402
from sustained_preflight_test_support import (  # noqa: E402
    create_published_preflight,
)
from validate_sustained_preflight import (  # noqa: E402
    PREFLIGHT_SNAPSHOT_NAME,
    PreflightValidationError,
    recompute_process_measurements,
    snapshot_verified_preflight,
    verify_preflight,
    verify_preflight_archive,
)


class SustainedPreflightTests(unittest.TestCase):
    def test_ready_to_high_water_growth_is_an_independent_budget(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            samples = pathlib.Path(temporary) / "samples.csv"
            samples.write_text(
                "elapsed_seconds,rss_mib,cpu_percent\n"
                + "".join(
                    f"{second},{650.0 if second == 5 else 512.0:.3f},10.000\n"
                    for second in range(0, 601, 5)
                ),
                encoding="utf-8",
            )
            measured = recompute_process_measurements(samples)
            self.assertFalse(measured["all_measured_budgets_pass"])
            self.assertFalse(
                measured["budgets"][
                    "ready_baseline_to_active_window_rss_high_water_growth_mib_le_128"
                ]
            )
            self.assertTrue(
                measured["budgets"][
                    "sampled_active_window_resident_memory_peak_mib_le_10240"
                ]
            )
            self.assertTrue(
                measured["budgets"]["steady_window_growth_mib_le_64"]
            )
            self.assertTrue(
                measured["budgets"][
                    "steady_linear_growth_mib_per_hour_le_128"
                ]
            )

    def test_valid_preflight_is_recomputed_bound_and_snapshotted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            preflight, expected = create_published_preflight(root)
            result = verify_preflight(preflight, expected)
            self.assertTrue(result["accepted"])
            self.assertEqual(result["requested_active_seconds"], 600)
            summary = json.loads((preflight / SUMMARY_NAME).read_text(encoding="utf-8"))
            self.assertFalse(summary["qualified_one_hour"])

            snapshot = root / PREFLIGHT_SNAPSHOT_NAME
            binding = snapshot_verified_preflight(preflight, snapshot, expected)
            self.assertTrue(snapshot.is_file())
            self.assertEqual(
                verify_preflight_archive(
                    snapshot, expected, str(preflight.absolute())
                ),
                binding,
            )

    def test_missing_preflight_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            with self.assertRaisesRegex(PreflightValidationError, "missing"):
                verify_preflight(root / "missing", {})

    def test_claimed_summary_without_real_runtime_evidence_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            preflight, expected = create_published_preflight(root)
            summary = json.loads((preflight / SUMMARY_NAME).read_text(encoding="utf-8"))
            summary["runtime_contract"]["final_tick"] = 99_999
            (preflight / SUMMARY_NAME).write_text(
                json.dumps(summary, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                PreflightValidationError, "digest mismatch|disagree"
            ):
                verify_preflight(preflight, expected)

    def test_tampered_log_samples_or_validation_are_rejected(self) -> None:
        for name, replacement in (
            ("packaged_sustained_soak.log", "claimed pass\n"),
            ("process_samples.csv", "elapsed_seconds,rss_mib,cpu_percent\n"),
            ("sustained_log_validation.json", "{}\n"),
        ):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temporary:
                preflight, expected = create_published_preflight(pathlib.Path(temporary))
                (preflight / name).write_text(replacement, encoding="utf-8")
                with self.assertRaisesRegex(PreflightValidationError, "digest mismatch"):
                    verify_preflight(preflight, expected)

    def test_failed_preflight_budget_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            preflight, expected = create_published_preflight(
                pathlib.Path(temporary), budgets_pass=False
            )
            with self.assertRaisesRegex(PreflightValidationError, "mismatch"):
                verify_preflight(preflight, expected)

    def test_package_or_tool_mismatch_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            preflight, expected = create_published_preflight(pathlib.Path(temporary))
            wrong_package = dict(expected)
            wrong_package["manifest_sha256"] = "f" * 64
            with self.assertRaisesRegex(PreflightValidationError, "manifest_sha256"):
                verify_preflight(preflight, wrong_package)
            wrong_tool = dict(expected)
            wrong_tool["runner_sha256"] = "e" * 64
            with self.assertRaisesRegex(PreflightValidationError, "runner_sha256"):
                verify_preflight(preflight, wrong_tool)

    def test_snapshot_tampering_after_capture_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            preflight, expected = create_published_preflight(root)
            snapshot = root / PREFLIGHT_SNAPSHOT_NAME
            snapshot_verified_preflight(preflight, snapshot, expected)
            snapshot.write_bytes(b"post-capture mutation")
            with self.assertRaises(PreflightValidationError):
                verify_preflight_archive(snapshot, expected, str(preflight.absolute()))


if __name__ == "__main__":
    unittest.main(verbosity=2)
