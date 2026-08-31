#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT_ROOT / "Scripts"))

from finalize_sustained_evidence import (  # noqa: E402
    COMPLETION_NAME,
    MANIFEST_NAME,
    PREFLIGHT_BINDING_NAME,
    PREFLIGHT_SNAPSHOT_NAME,
    SUMMARY_NAME,
    FinalizationError,
    finalize_evidence,
    verify_published_evidence,
)
from sustained_preflight_test_support import create_staged_one_hour  # noqa: E402


TRUSTED_PREFLIGHT_VERIFIER = (
    PROJECT_ROOT / "Scripts" / "validate_sustained_preflight.py"
)


class InjectedFailure(RuntimeError):
    pass


class SustainedEvidenceFinalizerTests(unittest.TestCase):
    def make_fixture(
        self,
        root: pathlib.Path,
        *,
        run_class: str = "one_hour_qualification",
        runtime_qualified: bool = True,
        budgets_pass: bool = True,
    ) -> tuple[pathlib.Path, pathlib.Path, list[str]]:
        if run_class == "one_hour_qualification":
            staging, final, names = create_staged_one_hour(
                root, budgets_pass=budgets_pass
            )
            if not runtime_qualified:
                summary = json.loads(
                    (staging / SUMMARY_NAME).read_text(encoding="utf-8")
                )
                summary["runtime_contract_qualified_one_hour"] = False
                (staging / SUMMARY_NAME).write_text(
                    json.dumps(summary, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8",
                )
            return staging, final, names

        staging = root / ".evidence.incomplete-test"
        final = root / "evidence"
        staging.mkdir()
        summary = {
            "all_measured_budgets_pass": budgets_pass,
            "qualified_one_hour": False,
            "run_class": run_class,
            "runtime_contract_qualified_one_hour": runtime_qualified,
            "source_commit": "1" * 40,
            "preflight_evidence_directory": None,
            "preflight_evidence_manifest_sha256": None,
            "preflight_snapshot_sha256": None,
        }
        names = [SUMMARY_NAME, "runtime.log"]
        (staging / SUMMARY_NAME).write_text(
            json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        (staging / "runtime.log").write_text("bounded evidence\n", encoding="utf-8")
        return staging, final, names

    def finalize(
        self,
        staging: pathlib.Path,
        final: pathlib.Path,
        names: list[str],
        failure_injector=None,
    ) -> pathlib.Path:
        return finalize_evidence(
            staging,
            final,
            names,
            failure_injector,
            trusted_preflight_verifier=TRUSTED_PREFLIGHT_VERIFIER,
        )

    def write_stale_qualified_claim(self, staging: pathlib.Path) -> None:
        summary_path = staging / SUMMARY_NAME
        summary = json.loads(summary_path.read_text(encoding="utf-8"))
        summary["qualified_one_hour"] = True
        summary_path.write_text(
            json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )

    def assert_not_published_or_qualified(
        self, staging: pathlib.Path, final: pathlib.Path
    ) -> None:
        self.assertFalse(final.exists())
        summary = json.loads((staging / SUMMARY_NAME).read_text(encoding="utf-8"))
        self.assertFalse(summary["qualified_one_hour"])

    def test_one_hour_result_is_atomically_published(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            staging, final, names = self.make_fixture(pathlib.Path(temporary))
            published = self.finalize(staging, final, names)
            self.assertEqual(published, final)
            self.assertFalse(staging.exists())
            summary = json.loads((final / SUMMARY_NAME).read_text(encoding="utf-8"))
            completion = json.loads(
                (final / COMPLETION_NAME).read_text(encoding="utf-8")
            )
            self.assertTrue(summary["qualified_one_hour"])
            self.assertEqual(completion["status"], "complete")
            self.assertTrue(completion["qualified_one_hour"])
            self.assertEqual(
                completion["preflight_snapshot_sha256"],
                summary["preflight_snapshot_sha256"],
            )
            for line in (final / MANIFEST_NAME).read_text(encoding="utf-8").splitlines():
                expected, name = line.split("  ", 1)
                actual = hashlib.sha256((final / name).read_bytes()).hexdigest()
                self.assertEqual(actual, expected)

    def test_preflight_publishes_complete_but_not_one_hour(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            staging, final, names = self.make_fixture(
                pathlib.Path(temporary), run_class="ten_minute_preflight",
                runtime_qualified=False,
            )
            self.finalize(staging, final, names)
            summary = json.loads((final / SUMMARY_NAME).read_text(encoding="utf-8"))
            completion = json.loads(
                (final / COMPLETION_NAME).read_text(encoding="utf-8")
            )
            self.assertFalse(summary["qualified_one_hour"])
            self.assertFalse(completion["qualified_one_hour"])

    def test_terminal_cli_is_the_successful_publication_process(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            staging, final, names = self.make_fixture(pathlib.Path(temporary))
            command = [
                sys.executable,
                str(PROJECT_ROOT / "Scripts" / "finalize_sustained_evidence.py"),
                "--staging-dir",
                str(staging),
                "--final-dir",
                str(final),
            ]
            for name in names:
                command.extend(("--evidence-file", name))
            command.extend(
                ("--trusted-preflight-verifier", str(TRUSTED_PREFLIGHT_VERIFIER))
            )
            command.append("--terminal")
            completed = subprocess.run(
                command, check=False, capture_output=True, text=True
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertIn("one-hour packaged sustained qualification passed", completed.stdout)
            self.assertIn("Atomic completion record:", completed.stdout)
            self.assertTrue(final.is_dir())
            self.assertFalse(staging.exists())

    def test_failed_budget_never_publishes_one_hour_result(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            staging, final, names = self.make_fixture(
                pathlib.Path(temporary), budgets_pass=False
            )
            with self.assertRaisesRegex(FinalizationError, "not satisfied"):
                self.finalize(staging, final, names)
            self.assertFalse(final.exists())
            summary = json.loads((staging / SUMMARY_NAME).read_text(encoding="utf-8"))
            self.assertFalse(summary["qualified_one_hour"])

    def test_one_hour_requires_an_external_trusted_verifier(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            staging, final, names = self.make_fixture(pathlib.Path(temporary))
            with self.assertRaisesRegex(FinalizationError, "external trusted"):
                finalize_evidence(staging, final, names)
            self.assertFalse(final.exists())

    def test_tampered_runtime_inputs_never_publish(self) -> None:
        mutations = {
            "packaged_sustained_soak.log": "claimed pass\n",
            "process_samples.csv": "elapsed_seconds,rss_mib,cpu_percent\n",
            "run_metadata.txt": "fixture=Stress400Sustained\n",
        }
        for name, payload in mutations.items():
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temporary:
                staging, final, names = self.make_fixture(pathlib.Path(temporary))
                (staging / name).write_text(payload, encoding="utf-8")
                with self.assertRaises(FinalizationError):
                    self.finalize(staging, final, names)
                self.assertFalse(final.exists())

    def test_unexpected_top_level_entry_never_publishes(self) -> None:
        for entry_kind in ("file", "directory"):
            with self.subTest(entry_kind=entry_kind), tempfile.TemporaryDirectory() as temporary:
                staging, final, names = self.make_fixture(pathlib.Path(temporary))
                self.write_stale_qualified_claim(staging)
                unexpected = staging / "unexpected-entry"
                if entry_kind == "file":
                    unexpected.write_text("not evidence\n", encoding="utf-8")
                else:
                    unexpected.mkdir()
                with self.assertRaisesRegex(FinalizationError, "schema is not exact"):
                    self.finalize(staging, final, names)
                self.assert_not_published_or_qualified(staging, final)

    def test_trusted_verifier_path_or_digest_tampering_never_publishes(self) -> None:
        for mutation in ("artifact_path", "digest_mismatch"):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as temporary:
                root = pathlib.Path(temporary)
                staging, final, names = self.make_fixture(root)
                self.write_stale_qualified_claim(staging)
                if mutation == "artifact_path":
                    trusted = staging / "validate_sustained_preflight.used.py"
                    expected_error = "cannot come from evidence"
                else:
                    trusted = root / "different-verifier.py"
                    trusted.write_text("# different trusted input\n", encoding="utf-8")
                    expected_error = "digest differs"
                with self.assertRaisesRegex(FinalizationError, expected_error):
                    finalize_evidence(
                        staging,
                        final,
                        names,
                        trusted_preflight_verifier=trusted,
                    )
                self.assert_not_published_or_qualified(staging, final)

    def test_static_publication_check_never_executes_retained_python(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            staging, final, names = self.make_fixture(root)
            self.finalize(staging, final, names)
            marker = root / "artifact-code-executed"
            retained = final / "validate_sustained_preflight.used.py"
            retained.write_text(
                "import pathlib\n"
                f"pathlib.Path({str(marker)!r}).write_text('unsafe', encoding='utf-8')\n",
                encoding="utf-8",
            )
            summary_path = final / SUMMARY_NAME
            summary = json.loads(summary_path.read_text(encoding="utf-8"))
            summary["preflight_verifier_sha256"] = hashlib.sha256(
                retained.read_bytes()
            ).hexdigest()
            summary_path.write_text(
                json.dumps(summary, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            manifest_path = final / MANIFEST_NAME
            manifest_path.write_text(
                "".join(
                    f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
                    for path in sorted(final.iterdir(), key=lambda value: value.name)
                    if path.is_file() and path.name != MANIFEST_NAME
                ),
                encoding="utf-8",
            )
            verify_published_evidence(final)
            self.assertFalse(marker.exists())

    def test_missing_or_mismatched_preflight_binding_never_publishes(self) -> None:
        for mutation, expected_error in (
            ("missing", "missing|must retain"),
            ("mismatched", "differs|mismatch"),
        ):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as temporary:
                staging, final, names = self.make_fixture(pathlib.Path(temporary))
                if mutation == "missing":
                    (staging / PREFLIGHT_SNAPSHOT_NAME).unlink()
                else:
                    summary = json.loads(
                        (staging / SUMMARY_NAME).read_text(encoding="utf-8")
                    )
                    summary["preflight_evidence_manifest_sha256"] = "f" * 64
                    (staging / SUMMARY_NAME).write_text(
                        json.dumps(summary, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8",
                    )
                with self.assertRaisesRegex(FinalizationError, expected_error):
                    self.finalize(staging, final, names)
                self.assertFalse(final.exists())

    def test_snapshot_change_after_initial_validation_never_publishes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            staging, final, names = self.make_fixture(pathlib.Path(temporary))

            def mutate(phase: str) -> None:
                if phase == "before_publish":
                    with (staging / PREFLIGHT_SNAPSHOT_NAME).open("ab") as handle:
                        handle.write(b"post-validation mutation")

            with self.assertRaisesRegex(FinalizationError, "snapshot digest differs"):
                self.finalize(staging, final, names, mutate)
            self.assertFalse(final.exists())

    def test_failures_after_true_staging_do_not_publish(self) -> None:
        for phase in ("after_summary", "after_completion", "before_publish"):
            with self.subTest(phase=phase), tempfile.TemporaryDirectory() as temporary:
                staging, final, names = self.make_fixture(pathlib.Path(temporary))

                def fail(observed: str) -> None:
                    if observed == phase:
                        raise InjectedFailure(phase)

                with self.assertRaises(InjectedFailure):
                    self.finalize(staging, final, names, fail)
                self.assertFalse(final.exists())
                self.assertTrue(staging.exists())
                summary = json.loads(
                    (staging / SUMMARY_NAME).read_text(encoding="utf-8")
                )
                self.assertFalse(summary["qualified_one_hour"])
                self.assertFalse((staging / COMPLETION_NAME).exists())
                self.assertFalse((staging / MANIFEST_NAME).exists())

    def test_transaction_mutation_before_publish_is_rejected(self) -> None:
        for name in (COMPLETION_NAME, MANIFEST_NAME):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temporary:
                staging, final, names = self.make_fixture(pathlib.Path(temporary))

                def mutate(phase: str) -> None:
                    if phase == "before_publish":
                        (staging / name).write_text("tampered\n", encoding="utf-8")

                with self.assertRaises(FinalizationError):
                    self.finalize(staging, final, names, mutate)
                self.assert_not_published_or_qualified(staging, final)
                self.assertFalse((staging / COMPLETION_NAME).exists())
                self.assertFalse((staging / MANIFEST_NAME).exists())

    def test_existing_final_directory_is_never_replaced(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            staging, final, names = self.make_fixture(root)
            self.write_stale_qualified_claim(staging)
            final.mkdir()
            sentinel = final / "sentinel"
            sentinel.write_text("preserve\n", encoding="utf-8")
            with self.assertRaisesRegex(FinalizationError, "already exists"):
                self.finalize(staging, final, names)
            self.assertEqual(sentinel.read_text(encoding="utf-8"), "preserve\n")
            summary = json.loads((staging / SUMMARY_NAME).read_text(encoding="utf-8"))
            self.assertFalse(summary["qualified_one_hour"])

    def test_other_early_rejections_clear_stale_qualification(self) -> None:
        for mutation in ("parent_mismatch", "invalid_name", "missing_file"):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as temporary:
                root = pathlib.Path(temporary)
                staging, final, names = self.make_fixture(root)
                self.write_stale_qualified_claim(staging)
                expected_error = ""
                if mutation == "parent_mismatch":
                    (root / "other-parent").mkdir()
                    final = root / "other-parent" / "evidence"
                    expected_error = "share one parent"
                elif mutation == "invalid_name":
                    names.append("../unsafe")
                    expected_error = "Unsafe evidence file name"
                else:
                    (staging / "packaged_sustained_soak.log").unlink()
                    expected_error = "missing or unsafe"
                with self.assertRaisesRegex(FinalizationError, expected_error):
                    self.finalize(staging, final, names)
                self.assert_not_published_or_qualified(staging, final)

    def test_destination_race_cannot_replace_an_empty_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            staging, final, names = self.make_fixture(root)

            def collide(phase: str) -> None:
                if phase == "before_publish":
                    final.mkdir()

            with self.assertRaisesRegex(FinalizationError, "appeared"):
                self.finalize(staging, final, names, collide)
            self.assertTrue(final.is_dir())
            self.assertEqual(list(final.iterdir()), [])
            summary = json.loads((staging / SUMMARY_NAME).read_text(encoding="utf-8"))
            self.assertFalse(summary["qualified_one_hour"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
