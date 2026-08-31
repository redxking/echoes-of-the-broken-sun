#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
import math
import os
import pathlib
import re
import stat
import statistics
import sys
import tempfile
import zipfile
from types import ModuleType
from typing import Mapping


PACKAGE_MANIFEST_NAME = "EchoesOfTheBrokenSun.manifest.txt"
PACKAGE_MANIFEST_DIGEST_NAME = "EchoesOfTheBrokenSun.manifest.sha256"
RAW_LOG_NAME = "packaged_sustained_soak.log"
SAMPLES_NAME = "process_samples.csv"
VALIDATION_NAME = "sustained_log_validation.json"
METADATA_NAME = "run_metadata.txt"
RUNTIME_INVENTORY_NAME = "runtime_state_inventory.txt"
PREFLIGHT_SNAPSHOT_NAME = "preflight_evidence_snapshot.zip"
PREFLIGHT_BINDING_NAME = "preflight_binding.json"
TOOL_DIGEST_FIELDS = (
    "runner_sha256",
    "validator_sha256",
    "packager_sha256",
    "package_verifier_sha256",
    "evidence_finalizer_sha256",
    "preflight_verifier_sha256",
)
TOOL_FILES = {
    "runner_sha256": "soak_packaged_sustained_macos.used.sh",
    "validator_sha256": "validate_sustained_soak_log.used.py",
    "packager_sha256": "package_macos.used.sh",
    "package_verifier_sha256": "verify_packaged_app.used.py",
    "evidence_finalizer_sha256": "finalize_sustained_evidence.used.py",
    "preflight_verifier_sha256": "validate_sustained_preflight.used.py",
}
SHA256 = re.compile(r"^[0-9a-f]{64}$")
MAXIMUM_SNAPSHOT_ENTRIES = 10_000
MAXIMUM_SNAPSHOT_BYTES = 1024 * 1024 * 1024


class PreflightValidationError(RuntimeError):
    pass


def _load_exact_sibling(canonical_name: str, retained_name: str) -> ModuleType:
    verifier_path = pathlib.Path(__file__).absolute()
    dependency_name = (
        retained_name
        if verifier_path.name.endswith(".used.py")
        else f"{canonical_name}.py"
    )
    dependency_path = verifier_path.with_name(dependency_name)
    if not dependency_path.is_file() or dependency_path.is_symlink():
        raise PreflightValidationError(
            f"Exact verifier dependency is missing or unsafe: {dependency_name}"
        )
    module_name = (
        f"_echoes_exact_{canonical_name}_{os.getpid()}_{id(dependency_path)}"
    )
    specification = importlib.util.spec_from_file_location(module_name, dependency_path)
    if specification is None or specification.loader is None:
        raise PreflightValidationError(
            f"Could not load exact verifier dependency: {dependency_name}"
        )
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


_finalizer = _load_exact_sibling(
    "finalize_sustained_evidence", "finalize_sustained_evidence.used.py"
)
_log_validator = _load_exact_sibling(
    "validate_sustained_soak_log", "validate_sustained_soak_log.used.py"
)
COMPLETION_NAME = _finalizer.COMPLETION_NAME
MANIFEST_NAME = _finalizer.MANIFEST_NAME
SUMMARY_NAME = _finalizer.SUMMARY_NAME
FinalizationError = _finalizer.FinalizationError
verify_published_evidence = _finalizer.verify_published_evidence
LogValidationError = _log_validator.ValidationError
validate_log = _log_validator.validate_log


def _digest(path: pathlib.Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def _read_json(path: pathlib.Path, label: str) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise PreflightValidationError(f"{label} is unreadable: {error}") from error
    if not isinstance(value, dict):
        raise PreflightValidationError(f"{label} is not an object")
    return value


def _read_key_values(path: pathlib.Path, label: str) -> dict[str, str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise PreflightValidationError(f"{label} is unreadable: {error}") from error
    values: dict[str, str] = {}
    for line in lines:
        if not line:
            break
        if "=" not in line:
            raise PreflightValidationError(f"{label} contains a malformed field")
        key, value = line.split("=", 1)
        if not key or key in values:
            raise PreflightValidationError(f"{label} contains an invalid duplicate field")
        values[key] = value
    return values


def _require_sha256(value: object, label: str) -> str:
    if not isinstance(value, str) or SHA256.fullmatch(value) is None:
        raise PreflightValidationError(f"{label} is not a SHA-256 digest")
    return value


def _safe_leaf(value: object, label: str) -> str:
    if not isinstance(value, str) or not value:
        raise PreflightValidationError(f"{label} is missing")
    candidate = pathlib.PurePath(value)
    if candidate.name != value or value in {".", ".."}:
        raise PreflightValidationError(f"{label} is not a safe file name")
    return value


def recompute_process_measurements(
    samples_path: pathlib.Path,
    duration_seconds: int = 600,
    warmup_seconds: int = 120,
    sample_interval: int = 5,
    minimum_steady_samples: int = 10,
) -> dict[str, object]:
    try:
        with samples_path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            if reader.fieldnames != ["elapsed_seconds", "rss_mib", "cpu_percent"]:
                raise PreflightValidationError("Preflight process-sample schema is not exact")
            observations = []
            for row in reader:
                if set(row) != set(reader.fieldnames) or any(
                    row[key] is None for key in reader.fieldnames
                ):
                    raise PreflightValidationError(
                        "Preflight process sample contains an unexpected field"
                    )
                observations.append(
                    (
                        int(row["elapsed_seconds"]),
                        float(row["rss_mib"]),
                        float(row["cpu_percent"]),
                    )
                )
    except (OSError, UnicodeError, ValueError) as error:
        raise PreflightValidationError(
            f"Preflight process samples are unreadable: {error}"
        ) from error

    expected_samples = duration_seconds // sample_interval + 1
    if len(observations) != expected_samples:
        raise PreflightValidationError(
            f"Preflight process sample count drifted: expected {expected_samples}, "
            f"observed {len(observations)}"
        )
    for index, (elapsed, rss, cpu) in enumerate(observations):
        expected_elapsed = index * sample_interval
        if not expected_elapsed <= elapsed <= expected_elapsed + 2:
            raise PreflightValidationError(
                f"Preflight process sample cadence drifted at index {index}"
            )
        if not math.isfinite(rss) or not math.isfinite(cpu) or rss <= 0 or cpu < 0:
            raise PreflightValidationError("Preflight process sample is non-finite or invalid")
    if observations[0][0] > 2 or duration_seconds - observations[-1][0] >= sample_interval:
        raise PreflightValidationError("Preflight process samples do not cover the active window")

    steady = [row for row in observations if row[0] >= warmup_seconds]
    if len(steady) < minimum_steady_samples:
        raise PreflightValidationError("Preflight has insufficient steady-state samples")
    elapsed = [row[0] for row in steady]
    rss = [row[1] for row in steady]
    cpu = [row[2] for row in steady]
    all_rss = [row[1] for row in observations]
    all_cpu = [row[2] for row in observations]
    mean_elapsed = statistics.fmean(elapsed)
    mean_rss = statistics.fmean(rss)
    denominator = sum((value - mean_elapsed) ** 2 for value in elapsed)
    slope_per_hour = (
        sum((x - mean_elapsed) * (y - mean_rss) for x, y in zip(elapsed, rss))
        / denominator
        * 3600.0
        if denominator
        else 0.0
    )
    window_size = max(5, len(rss) // 10)
    window_growth = statistics.fmean(rss[-window_size:]) - statistics.fmean(
        rss[:window_size]
    )

    def percentile(values: list[float], fraction: float) -> float:
        ordered = sorted(values)
        return ordered[math.ceil(fraction * len(ordered)) - 1]

    budgets = {
        "structured_sustained_contract": True,
        "sampled_active_window_resident_memory_peak_mib_le_10240":
            max(all_rss) <= 10240.0,
        "steady_window_growth_mib_le_64": window_growth <= 64.0,
        "steady_linear_growth_mib_per_hour_le_128": slope_per_hour <= 128.0,
    }
    return {
        "all_measured_budgets_pass": all(budgets.values()),
        "budgets": budgets,
        "samples": len(observations),
        "steady_samples": len(steady),
        "warmup_seconds": warmup_seconds,
        "sampled_active_window_resident_memory_mib": {
            "peak": round(max(all_rss), 6),
            "steady_mean": round(mean_rss, 6),
            "steady_p95": round(percentile(rss, 0.95), 6),
            "steady_peak": round(max(rss), 6),
            "steady_window_growth": round(window_growth, 6),
            "steady_linear_slope_mib_per_hour": round(slope_per_hour, 6),
        },
        "sampled_active_window_cpu_percent": {
            "peak": round(max(all_cpu), 6),
            "steady_mean": round(statistics.fmean(cpu), 6),
            "steady_p95": round(percentile(cpu, 0.95), 6),
            "steady_peak": round(max(cpu), 6),
        },
    }


def _validate_runtime_inventory(preflight_directory: pathlib.Path) -> None:
    inventory_path = preflight_directory / RUNTIME_INVENTORY_NAME
    try:
        lines = inventory_path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise PreflightValidationError(f"Runtime-state inventory is unreadable: {error}") from error
    if len(lines) < 4 or not lines[0].startswith("runtime_state=") or not lines[1].startswith(
        "save_game_directory="
    ) or not lines[2].startswith("user_directory=") or not lines[3].startswith(
        "file_count="
    ):
        raise PreflightValidationError("Runtime-state inventory header is malformed")
    try:
        recorded_count = int(lines[3].split("=", 1)[1], 10)
    except ValueError as error:
        raise PreflightValidationError("Runtime-state inventory count is invalid") from error
    records: dict[str, str] = {}
    pattern = re.compile(r"^([0-9a-f]{64})  (.+)$")
    for line in lines[4:]:
        match = pattern.fullmatch(line)
        if match is None:
            raise PreflightValidationError("Runtime-state inventory record is malformed")
        digest, relative = match.groups()
        candidate = pathlib.PurePosixPath(relative)
        if (
            candidate.is_absolute()
            or not candidate.parts
            or any(part in {"", ".", ".."} for part in candidate.parts)
            or relative in records
        ):
            raise PreflightValidationError("Runtime-state inventory path is unsafe")
        records[relative] = digest
    runtime_state = preflight_directory / "runtime-state"
    actual: dict[str, str] = {}
    for candidate in sorted(runtime_state.rglob("*")):
        if candidate.is_file() and not candidate.is_symlink():
            relative = candidate.relative_to(runtime_state).as_posix()
            actual[relative] = _digest(candidate)
    if recorded_count != len(records) or records != actual:
        raise PreflightValidationError("Runtime-state inventory does not match retained files")


def _validate_staged_tree(
    directory: pathlib.Path,
    evidence_names: list[str],
    transaction_records_present: bool,
) -> None:
    if not directory.is_dir() or directory.is_symlink():
        raise PreflightValidationError("Staged evidence directory is missing or unsafe")
    for name in evidence_names:
        if pathlib.PurePath(name).name != name or name in {".", ".."}:
            raise PreflightValidationError("Staged evidence contains an unsafe file name")
    top_files: set[str] = set()
    top_directories: set[str] = set()
    for candidate in directory.iterdir():
        if candidate.is_symlink():
            raise PreflightValidationError(
                f"Staged evidence contains a symlink: {candidate.name}"
            )
        mode = candidate.stat().st_mode
        if stat.S_ISREG(mode):
            top_files.add(candidate.name)
        elif stat.S_ISDIR(mode):
            top_directories.add(candidate.name)
        else:
            raise PreflightValidationError(
                f"Staged evidence contains a special entry: {candidate.name}"
            )
    expected_top_files = set(evidence_names)
    if transaction_records_present:
        expected_top_files.update({COMPLETION_NAME, MANIFEST_NAME})
    if top_files != expected_top_files or top_directories != {"runtime-state"}:
        raise PreflightValidationError("Staged one-hour evidence schema is not exact")
    runtime_state = directory / "runtime-state"
    for root, directory_names, file_names in os.walk(runtime_state, followlinks=False):
        for name in directory_names + file_names:
            candidate = pathlib.Path(root) / name
            if candidate.is_symlink():
                raise PreflightValidationError("Runtime-state evidence contains a symlink")
            mode = candidate.stat().st_mode
            if not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
                raise PreflightValidationError(
                    "Runtime-state evidence contains a special entry"
                )


def _loaded_dependency_digest(module: ModuleType, label: str) -> str:
    value = getattr(module, "__file__", None)
    if not isinstance(value, str) or not value:
        raise PreflightValidationError(f"Loaded {label} path is unavailable")
    path = pathlib.Path(value).absolute()
    if not path.is_file() or path.is_symlink():
        raise PreflightValidationError(f"Loaded {label} is missing or unsafe")
    return _digest(path)


def verify_staged_one_hour(
    directory: pathlib.Path,
    expected: Mapping[str, object],
    evidence_names: list[str],
    expected_qualified: bool,
) -> dict[str, object]:
    directory = directory.absolute()
    _validate_staged_tree(directory, evidence_names, expected_qualified)
    summary = _read_json(directory / SUMMARY_NAME, "One-hour summary")
    required_state = {
        "artifact": "EchoesOfTheBrokenSun.app",
        "fixture": "Stress400Sustained",
        "run_class": "one_hour_qualification",
        "requested_active_seconds": 3600,
        "qualified_one_hour": expected_qualified,
    }
    for key, value in required_state.items():
        if summary.get(key) != value:
            raise PreflightValidationError(
                f"One-hour summary field mismatch: {key}"
            )
    for key in (
        "artifact",
        "configuration",
        "platform",
        "package_version",
        "source_commit",
        "manifest_sha256",
        "normal_startup_smoke_sha256",
        "legacy_stress_startup_smoke_sha256",
        *TOOL_DIGEST_FIELDS,
    ):
        if summary.get(key) != expected.get(key):
            raise PreflightValidationError(f"One-hour identity mismatch: {key}")

    package_manifest = directory / PACKAGE_MANIFEST_NAME
    if not package_manifest.is_file() or package_manifest.is_symlink():
        raise PreflightValidationError("One-hour package manifest is missing or unsafe")
    manifest_sha256 = _require_sha256(expected.get("manifest_sha256"), "manifest_sha256")
    if _digest(package_manifest) != manifest_sha256:
        raise PreflightValidationError("One-hour package manifest content identity differs")
    manifest_values = _read_key_values(package_manifest, "One-hour package manifest")
    normal_smoke_name = _safe_leaf(
        manifest_values.get("normal_startup_smoke"), "normal startup-smoke name"
    )
    stress_smoke_name = _safe_leaf(
        manifest_values.get("legacy_stress_startup_smoke"),
        "legacy stress startup-smoke name",
    )
    manifest_expected = {
        "artifact": expected.get("artifact"),
        "source_commit": expected.get("source_commit"),
        "origin_main": expected.get("source_commit"),
        "remote_main": expected.get("source_commit"),
        "source_tree": "clean",
        "source_binding": "clean-pushed-main",
        "configuration": expected.get("configuration"),
        "platform": expected.get("platform"),
        "bundle_short_version": expected.get("package_version"),
        "normal_startup_smoke_sha256": expected.get(
            "normal_startup_smoke_sha256"
        ),
        "legacy_stress_startup_smoke_sha256": expected.get(
            "legacy_stress_startup_smoke_sha256"
        ),
    }
    for key, value in manifest_expected.items():
        if manifest_values.get(key) != value:
            raise PreflightValidationError(
                f"One-hour package manifest mismatch: {key}"
            )

    required_files = {
        PACKAGE_MANIFEST_NAME,
        PACKAGE_MANIFEST_DIGEST_NAME,
        RAW_LOG_NAME,
        SAMPLES_NAME,
        VALIDATION_NAME,
        SUMMARY_NAME,
        METADATA_NAME,
        RUNTIME_INVENTORY_NAME,
        PREFLIGHT_BINDING_NAME,
        PREFLIGHT_SNAPSHOT_NAME,
        normal_smoke_name,
        stress_smoke_name,
        *TOOL_FILES.values(),
    }
    if set(evidence_names) != required_files:
        raise PreflightValidationError("One-hour evidence file schema is not exact")

    digest_sidecar = directory / PACKAGE_MANIFEST_DIGEST_NAME
    expected_sidecar = f"{manifest_sha256}  {PACKAGE_MANIFEST_NAME}\n"
    try:
        if digest_sidecar.read_text(encoding="utf-8") != expected_sidecar:
            raise PreflightValidationError(
                "One-hour package-manifest sidecar is invalid"
            )
    except (OSError, UnicodeError) as error:
        raise PreflightValidationError(
            f"One-hour package-manifest sidecar is unreadable: {error}"
        ) from error
    for field, name in TOOL_FILES.items():
        if _digest(directory / name) != _require_sha256(expected.get(field), field):
            raise PreflightValidationError(f"One-hour retained tool mismatch: {field}")
    loaded_digests = {
        "validator_sha256": _loaded_dependency_digest(
            _log_validator, "sustained log validator"
        ),
        "evidence_finalizer_sha256": _loaded_dependency_digest(
            _finalizer, "evidence finalizer"
        ),
        "preflight_verifier_sha256": _digest(pathlib.Path(__file__).absolute()),
    }
    for field, actual in loaded_digests.items():
        if actual != _require_sha256(expected.get(field), field):
            raise PreflightValidationError(
                f"Trusted verifier dependency identity differs: {field}"
            )
    if _digest(directory / normal_smoke_name) != _require_sha256(
        expected.get("normal_startup_smoke_sha256"),
        "normal_startup_smoke_sha256",
    ):
        raise PreflightValidationError("One-hour normal startup smoke differs")
    if _digest(directory / stress_smoke_name) != _require_sha256(
        expected.get("legacy_stress_startup_smoke_sha256"),
        "legacy_stress_startup_smoke_sha256",
    ):
        raise PreflightValidationError("One-hour legacy stress startup smoke differs")

    try:
        raw_log = (directory / RAW_LOG_NAME).read_text(
            encoding="utf-8", errors="replace"
        )
        revalidated_runtime = validate_log(raw_log, 3600)
    except (OSError, LogValidationError) as error:
        raise PreflightValidationError(
            f"One-hour retained runtime log failed validation: {error}"
        ) from error
    validation = _read_json(directory / VALIDATION_NAME, "One-hour validation")
    if (
        validation != revalidated_runtime
        or summary.get("runtime_contract") != revalidated_runtime
        or revalidated_runtime.get("qualified_one_hour") is not True
        or summary.get("runtime_contract_qualified_one_hour") is not True
    ):
        raise PreflightValidationError("One-hour runtime validation records disagree")

    measured = recompute_process_measurements(
        directory / SAMPLES_NAME,
        duration_seconds=3600,
        warmup_seconds=120,
        sample_interval=5,
        minimum_steady_samples=10,
    )
    for key, value in measured.items():
        if summary.get(key) != value:
            raise PreflightValidationError(f"One-hour measured result mismatch: {key}")

    metadata = _read_key_values(directory / METADATA_NAME, "One-hour metadata")
    metadata_expected = {
        "fixture": "Stress400Sustained",
        "requested_active_seconds": "3600",
        "run_class": "one_hour_qualification",
        "source_commit": str(expected.get("source_commit")),
        "origin_main": str(expected.get("source_commit")),
        "remote_main": str(expected.get("source_commit")),
        "source_tree": "clean",
        "source_binding": "clean-pushed-main",
        "package_version": str(expected.get("package_version")),
        "configuration": str(expected.get("configuration")),
        "platform": str(expected.get("platform")),
        "manifest_sha256": manifest_sha256,
        "normal_startup_smoke_sha256": str(
            expected.get("normal_startup_smoke_sha256")
        ),
        "legacy_stress_startup_smoke_sha256": str(
            expected.get("legacy_stress_startup_smoke_sha256")
        ),
        "preflight_evidence_directory": str(
            summary.get("preflight_evidence_directory")
        ),
        "preflight_evidence_manifest_sha256": str(
            summary.get("preflight_evidence_manifest_sha256")
        ),
        "preflight_snapshot_sha256": str(summary.get("preflight_snapshot_sha256")),
        "post_run_source_commit": str(expected.get("source_commit")),
        "post_run_origin_main": str(expected.get("source_commit")),
        "post_run_remote_main": str(expected.get("source_commit")),
        "post_run_source_tree": "clean",
        "post_run_package_integrity": "verified",
    }
    for field in TOOL_DIGEST_FIELDS:
        metadata_expected[field] = str(expected.get(field))
    for key, value in metadata_expected.items():
        if metadata.get(key) != value:
            raise PreflightValidationError(f"One-hour metadata mismatch: {key}")
    if metadata.get("termination_status") not in {"0", "143"}:
        raise PreflightValidationError("One-hour termination status is not accepted")
    evidence_directory = metadata.get("evidence_directory")
    if (
        not isinstance(evidence_directory, str)
        or not pathlib.Path(evidence_directory).is_absolute()
        or metadata.get("isolated_save_game_directory")
        != f"{evidence_directory}/runtime-state/save-games"
        or metadata.get("isolated_user_directory")
        != f"{evidence_directory}/runtime-state/user-dir"
    ):
        raise PreflightValidationError("One-hour isolated runtime paths are invalid")
    host = summary.get("host")
    if not isinstance(host, dict):
        raise PreflightValidationError("One-hour host record is missing")
    host_expected = {
        "model": metadata.get("host_model"),
        "cpu": metadata.get("cpu_brand"),
        "physical_memory_bytes": int(metadata.get("physical_memory_bytes", "0")),
        "macos_version": metadata.get("macos_version"),
        "macos_build": metadata.get("macos_build"),
        "architecture": metadata.get("host_architecture"),
    }
    if host != host_expected or host_expected["physical_memory_bytes"] <= 0:
        raise PreflightValidationError("One-hour host records disagree")
    _validate_runtime_inventory(directory)
    return {
        "accepted": True,
        "qualified_one_hour": bool(
            revalidated_runtime.get("qualified_one_hour")
            and measured["all_measured_budgets_pass"]
        ),
        "source_commit": expected.get("source_commit"),
        "manifest_sha256": manifest_sha256,
    }


def verify_preflight(
    preflight_directory: pathlib.Path,
    expected: Mapping[str, object],
) -> dict[str, object]:
    preflight_directory = preflight_directory.absolute()
    try:
        published = verify_published_evidence(preflight_directory)
    except FinalizationError as error:
        raise PreflightValidationError(str(error)) from error
    summary = published["summary"]
    if not isinstance(summary, dict):
        raise PreflightValidationError("Preflight summary is not an object")

    required_state = {
        "run_class": "ten_minute_preflight",
        "requested_active_seconds": 600,
        "all_measured_budgets_pass": True,
        "runtime_contract_qualified_one_hour": False,
        "qualified_one_hour": False,
    }
    for key, value in required_state.items():
        if summary.get(key) != value:
            raise PreflightValidationError(f"Preflight summary field mismatch: {key}")

    for key in (
        "artifact",
        "configuration",
        "platform",
        "package_version",
        "source_commit",
        "manifest_sha256",
        "normal_startup_smoke_sha256",
        "legacy_stress_startup_smoke_sha256",
        *TOOL_DIGEST_FIELDS,
    ):
        if summary.get(key) != expected.get(key):
            raise PreflightValidationError(f"Preflight identity mismatch: {key}")
    if summary.get("preflight_evidence_directory") is not None or summary.get(
        "preflight_evidence_manifest_sha256"
    ) is not None or summary.get("preflight_snapshot_sha256") is not None:
        raise PreflightValidationError("A preflight cannot claim another preflight binding")

    package_manifest = preflight_directory / PACKAGE_MANIFEST_NAME
    if not package_manifest.is_file() or package_manifest.is_symlink():
        raise PreflightValidationError("Preflight package manifest copy is missing or unsafe")
    manifest_sha256 = _require_sha256(expected.get("manifest_sha256"), "manifest_sha256")
    if _digest(package_manifest) != manifest_sha256:
        raise PreflightValidationError("Preflight package manifest content identity differs")
    manifest_values = _read_key_values(package_manifest, "Preflight package manifest")
    normal_smoke_name = _safe_leaf(
        manifest_values.get("normal_startup_smoke"), "normal startup-smoke name"
    )
    stress_smoke_name = _safe_leaf(
        manifest_values.get("legacy_stress_startup_smoke"),
        "legacy stress startup-smoke name",
    )
    manifest_expected = {
        "artifact": expected.get("artifact"),
        "source_commit": expected.get("source_commit"),
        "origin_main": expected.get("source_commit"),
        "remote_main": expected.get("source_commit"),
        "source_tree": "clean",
        "source_binding": "clean-pushed-main",
        "configuration": expected.get("configuration"),
        "platform": expected.get("platform"),
        "bundle_short_version": expected.get("package_version"),
        "normal_startup_smoke_sha256": expected.get(
            "normal_startup_smoke_sha256"
        ),
        "legacy_stress_startup_smoke_sha256": expected.get(
            "legacy_stress_startup_smoke_sha256"
        ),
    }
    for key, value in manifest_expected.items():
        if manifest_values.get(key) != value:
            raise PreflightValidationError(f"Preflight package manifest mismatch: {key}")

    required_files = {
        PACKAGE_MANIFEST_NAME,
        PACKAGE_MANIFEST_DIGEST_NAME,
        RAW_LOG_NAME,
        SAMPLES_NAME,
        VALIDATION_NAME,
        SUMMARY_NAME,
        METADATA_NAME,
        RUNTIME_INVENTORY_NAME,
        COMPLETION_NAME,
        MANIFEST_NAME,
        normal_smoke_name,
        stress_smoke_name,
        *TOOL_FILES.values(),
    }
    if set(published.get("file_names", ())) != required_files:
        raise PreflightValidationError("Preflight evidence file schema is not exact")
    if set(published.get("directory_names", ())) != {"runtime-state"}:
        raise PreflightValidationError("Preflight runtime-state directory schema is not exact")

    digest_sidecar = preflight_directory / PACKAGE_MANIFEST_DIGEST_NAME
    expected_sidecar = f"{manifest_sha256}  {PACKAGE_MANIFEST_NAME}\n"
    try:
        if digest_sidecar.read_text(encoding="utf-8") != expected_sidecar:
            raise PreflightValidationError("Preflight package-manifest sidecar is invalid")
    except (OSError, UnicodeError) as error:
        raise PreflightValidationError(
            f"Preflight package-manifest sidecar is unreadable: {error}"
        ) from error
    for field, name in TOOL_FILES.items():
        if _digest(preflight_directory / name) != _require_sha256(expected.get(field), field):
            raise PreflightValidationError(f"Preflight retained tool mismatch: {field}")
    if _digest(preflight_directory / normal_smoke_name) != _require_sha256(
        expected.get("normal_startup_smoke_sha256"),
        "normal_startup_smoke_sha256",
    ):
        raise PreflightValidationError("Preflight normal startup smoke differs")
    if _digest(preflight_directory / stress_smoke_name) != _require_sha256(
        expected.get("legacy_stress_startup_smoke_sha256"),
        "legacy_stress_startup_smoke_sha256",
    ):
        raise PreflightValidationError("Preflight legacy stress startup smoke differs")

    try:
        raw_log = (preflight_directory / RAW_LOG_NAME).read_text(
            encoding="utf-8", errors="replace"
        )
        revalidated_runtime = validate_log(raw_log, 600)
    except (OSError, LogValidationError) as error:
        raise PreflightValidationError(
            f"Preflight retained runtime log failed validation: {error}"
        ) from error
    validation = _read_json(preflight_directory / VALIDATION_NAME, "Preflight validation")
    if validation != revalidated_runtime or summary.get("runtime_contract") != revalidated_runtime:
        raise PreflightValidationError("Preflight runtime validation records disagree")

    measured = recompute_process_measurements(preflight_directory / SAMPLES_NAME)
    for key, value in measured.items():
        if summary.get(key) != value:
            raise PreflightValidationError(f"Preflight measured result mismatch: {key}")

    metadata = _read_key_values(preflight_directory / METADATA_NAME, "Preflight metadata")
    metadata_expected = {
        "requested_active_seconds": "600",
        "run_class": "ten_minute_preflight",
        "source_commit": str(expected.get("source_commit")),
        "origin_main": str(expected.get("source_commit")),
        "remote_main": str(expected.get("source_commit")),
        "source_tree": "clean",
        "source_binding": "clean-pushed-main",
        "package_version": str(expected.get("package_version")),
        "configuration": str(expected.get("configuration")),
        "platform": str(expected.get("platform")),
        "manifest_sha256": manifest_sha256,
        "post_run_source_commit": str(expected.get("source_commit")),
        "post_run_origin_main": str(expected.get("source_commit")),
        "post_run_remote_main": str(expected.get("source_commit")),
        "post_run_source_tree": "clean",
        "post_run_package_integrity": "verified",
    }
    for field in TOOL_DIGEST_FIELDS:
        metadata_expected[field] = str(expected.get(field))
    for key, value in metadata_expected.items():
        if metadata.get(key) != value:
            raise PreflightValidationError(f"Preflight metadata mismatch: {key}")
    if metadata.get("termination_status") not in {"0", "143"}:
        raise PreflightValidationError("Preflight termination status is not accepted")
    _validate_runtime_inventory(preflight_directory)

    return {
        "accepted": True,
        "package_manifest_sha256": manifest_sha256,
        "preflight_evidence_directory": str(preflight_directory),
        "preflight_evidence_manifest_sha256": published["manifest_sha256"],
        "requested_active_seconds": 600,
        "run_class": "ten_minute_preflight",
        "source_commit": expected["source_commit"],
    }


def _safe_snapshot_path(name: str) -> pathlib.PurePosixPath:
    if "\\" in name:
        raise PreflightValidationError("Preflight snapshot contains an unsafe path")
    candidate = pathlib.PurePosixPath(name.rstrip("/"))
    if (
        not name
        or candidate.is_absolute()
        or not candidate.parts
        or any(part in {"", ".", ".."} for part in candidate.parts)
    ):
        raise PreflightValidationError("Preflight snapshot contains an unsafe path")
    return candidate


def _create_snapshot(source: pathlib.Path, output: pathlib.Path) -> None:
    output = output.absolute()
    if output.exists() or output.is_symlink():
        raise PreflightValidationError("Preflight snapshot output already exists")
    temporary = output.with_name(f".{output.name}.tmp-{os.getpid()}")
    entries = sorted(source.rglob("*"), key=lambda path: path.relative_to(source).as_posix())
    if len(entries) > MAXIMUM_SNAPSHOT_ENTRIES:
        raise PreflightValidationError("Preflight snapshot contains too many entries")
    total_bytes = 0
    try:
        with zipfile.ZipFile(temporary, "x", compression=zipfile.ZIP_STORED) as archive:
            for candidate in entries:
                relative = candidate.relative_to(source).as_posix()
                mode = candidate.lstat().st_mode
                information = zipfile.ZipInfo(
                    relative + ("/" if stat.S_ISDIR(mode) else ""),
                    date_time=(1980, 1, 1, 0, 0, 0),
                )
                information.create_system = 3
                if stat.S_ISDIR(mode):
                    information.external_attr = (stat.S_IFDIR | 0o755) << 16
                    archive.writestr(information, b"")
                elif stat.S_ISREG(mode):
                    payload = candidate.read_bytes()
                    total_bytes += len(payload)
                    if total_bytes > MAXIMUM_SNAPSHOT_BYTES:
                        raise PreflightValidationError("Preflight snapshot exceeds the size limit")
                    information.external_attr = (stat.S_IFREG | 0o644) << 16
                    archive.writestr(information, payload)
                else:
                    raise PreflightValidationError(
                        "Preflight snapshot source contains a symlink or special entry"
                    )
        os.replace(temporary, output)
    finally:
        try:
            temporary.unlink()
        except FileNotFoundError:
            pass


def _extract_snapshot(archive_path: pathlib.Path, destination: pathlib.Path) -> None:
    try:
        with zipfile.ZipFile(archive_path, "r") as archive:
            entries = archive.infolist()
            if not entries or len(entries) > MAXIMUM_SNAPSHOT_ENTRIES:
                raise PreflightValidationError("Preflight snapshot entry count is invalid")
            names: set[str] = set()
            total_bytes = 0
            for information in entries:
                candidate = _safe_snapshot_path(information.filename)
                canonical_name = candidate.as_posix()
                if canonical_name in names:
                    raise PreflightValidationError("Preflight snapshot has a duplicate path")
                names.add(canonical_name)
                mode = information.external_attr >> 16
                is_directory = information.is_dir()
                if mode and not (
                    (is_directory and stat.S_ISDIR(mode))
                    or (not is_directory and stat.S_ISREG(mode))
                ):
                    raise PreflightValidationError(
                        "Preflight snapshot contains a symlink or special entry"
                    )
                if information.flag_bits & 0x1:
                    raise PreflightValidationError("Preflight snapshot is encrypted")
                total_bytes += information.file_size
                if total_bytes > MAXIMUM_SNAPSHOT_BYTES:
                    raise PreflightValidationError("Preflight snapshot exceeds the size limit")
                target = destination.joinpath(*candidate.parts)
                if is_directory:
                    target.mkdir(parents=True, exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with archive.open(information, "r") as source, target.open("xb") as sink:
                        for chunk in iter(lambda: source.read(1024 * 1024), b""):
                            sink.write(chunk)
    except (OSError, zipfile.BadZipFile, RuntimeError) as error:
        raise PreflightValidationError(f"Preflight snapshot is unreadable: {error}") from error


def verify_preflight_archive(
    archive_path: pathlib.Path,
    expected: Mapping[str, object],
    recorded_directory: str,
) -> dict[str, object]:
    archive_path = archive_path.absolute()
    if not archive_path.is_file() or archive_path.is_symlink():
        raise PreflightValidationError("Preflight snapshot is missing or unsafe")
    if not isinstance(recorded_directory, str) or not pathlib.Path(recorded_directory).is_absolute():
        raise PreflightValidationError("Recorded preflight directory is not absolute")
    with tempfile.TemporaryDirectory(
        prefix=".preflight-verify-", dir=archive_path.parent.parent
    ) as temporary:
        extracted = pathlib.Path(temporary) / "evidence"
        extracted.mkdir()
        _extract_snapshot(archive_path, extracted)
        result = verify_preflight(extracted, expected)
    result["preflight_evidence_directory"] = recorded_directory
    result["preflight_snapshot_sha256"] = _digest(archive_path)
    return result


def snapshot_verified_preflight(
    preflight_directory: pathlib.Path,
    snapshot_output: pathlib.Path,
    expected: Mapping[str, object],
) -> dict[str, object]:
    source_result = verify_preflight(preflight_directory, expected)
    _create_snapshot(preflight_directory.absolute(), snapshot_output)
    snapshot_result = verify_preflight_archive(
        snapshot_output,
        expected,
        str(preflight_directory.absolute()),
    )
    if (
        snapshot_result["preflight_evidence_manifest_sha256"]
        != source_result["preflight_evidence_manifest_sha256"]
    ):
        raise PreflightValidationError(
            "Preflight changed while its immutable snapshot was captured"
        )
    return snapshot_result


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Verify a published ten-minute preflight for one-hour qualification."
    )
    parser.add_argument("--preflight-dir", required=True, type=pathlib.Path)
    parser.add_argument("--snapshot-output", type=pathlib.Path)
    parser.add_argument("--json-output", required=True, type=pathlib.Path)
    parser.add_argument("--artifact", required=True)
    parser.add_argument("--configuration", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--package-version", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--manifest-sha256", required=True)
    parser.add_argument("--normal-startup-smoke-sha256", required=True)
    parser.add_argument("--legacy-stress-startup-smoke-sha256", required=True)
    for field in TOOL_DIGEST_FIELDS:
        parser.add_argument("--" + field.replace("_", "-"), required=True)
    return parser


def main() -> int:
    arguments = _parser().parse_args()
    expected = {
        key: value
        for key, value in vars(arguments).items()
        if key not in {"preflight_dir", "snapshot_output", "json_output"}
    }
    try:
        if arguments.snapshot_output is None:
            result = verify_preflight(arguments.preflight_dir, expected)
        else:
            result = snapshot_verified_preflight(
                arguments.preflight_dir, arguments.snapshot_output, expected
            )
    except PreflightValidationError as error:
        print(f"Sustained preflight verification failed: {error}", file=sys.stderr)
        return 1
    payload = (json.dumps(result, indent=2, sort_keys=True) + "\n").encode("utf-8")
    temporary = arguments.json_output.with_name(
        f".{arguments.json_output.name}.tmp-{os.getpid()}"
    )
    try:
        with temporary.open("xb") as handle:
            handle.write(payload)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, arguments.json_output)
    finally:
        try:
            temporary.unlink()
        except FileNotFoundError:
            pass
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
