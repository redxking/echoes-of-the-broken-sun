#!/usr/bin/env python3
"""Validate and seal a non-qualifying packaged sustained-memory diagnostic."""

from __future__ import annotations

import argparse
import csv
import ctypes
import errno
import hashlib
import importlib.util
import json
import math
import os
import pathlib
import re
import shlex
import signal
import stat
import sys
from collections.abc import Callable
from typing import Any

PACKAGE_SOURCE_COMMIT = "ae2e8494e7ccc23524871ec2754916140ff4ab01"
PACKAGE_ROOT = pathlib.Path(
    "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Packages/"
    "Mac-Development-20260831T083359Z-m13-pooling-ae2e849"
)
APPLICATION = PACKAGE_ROOT / "EchoesOfTheBrokenSun.app"
PACKAGE_MANIFEST_SHA256 = (
    "c020390d2a13bb2e47991f1b56a8b42eb6a7c9595055277b85728ef385ad1260"
)
PACKAGE_MANIFEST_DIGEST_SHA256 = (
    "0f479c60df6c426c054b3bedd65ed48e4b9d58518987aed30f51b1f254f5b94c"
)
PACKAGE_EXECUTABLE_SHA256 = (
    "2702302a90d42b22895fefd02fe030868aa2e23a31c4e76a3bb0690e44d68922"
)
PACKAGE_EXTERNAL_SEAL_SHA256 = (
    "ff5ab774a1737c2cc8c15185a59a580955edc7bc657dc51b2e5ac345762fe949"
)
NORMAL_SMOKE_SHA256 = "227a03810881015e374bc5d08668024e2267e993d5d4bde341cff6bcfc396688"
STRESS_SMOKE_SHA256 = "f6bfad1d8d6a2d2399330c70fb6be767e9f23f8ed230dc284d3e83893d5c7a72"
RUNTIME_VALIDATOR_SHA256 = (
    "4d27edc7276a7d7e97afd91fc2789cdefebb11f8d69c52df499e154008826b93"
)
PACKAGE_VERIFIER_SHA256 = (
    "a7bbc35ee84f95b072d1f919436af5b980fe2fc06620f2b5923b22ea52062c70"
)
UNREAL_INSIGHTS = pathlib.Path(
    "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/"
    "UnrealInsights.app/Contents/MacOS/UnrealInsights"
)
UNREAL_INSIGHTS_SHA256 = (
    "b475184d29fbf4d4b342402929e5e7d3fab779cb77a3b615ac41607ea09fccbb"
)
UNREAL_INSIGHTS_VERSION = "5.8.2"
UNREAL_INSIGHTS_BUILD_ID = "55116800"
DURATION_SECONDS = 600
SAMPLE_INTERVAL_SECONDS = 5
EXPECTED_SAMPLE_COUNT = DURATION_SECONDS // SAMPLE_INTERVAL_SECONDS + 1
TRACE_NAME = "echoes-memory.utrace"
RAW_LOG_NAME = "packaged_sustained_memory_diagnostic.log"
SAMPLES_NAME = "process_samples.csv"
METADATA_NAME = "run_metadata.txt"
LAUNCH_COMMAND_NAME = "launch_command.txt"
INSIGHTS_LOG_NAME = "unreal-insights-parse.log"
TRACE_INSTRUCTIONS_NAME = "trace_analysis_instructions.txt"
RUNTIME_INVENTORY_NAME = "runtime_state_inventory.json"
VALIDATION_NAME = "sustained_memory_diagnostic_validation.json"
EVIDENCE_INVENTORY_NAME = "evidence_inventory.json"
EVIDENCE_MANIFEST_NAME = "diagnostic_evidence.sha256"
RUNNER_COPY_NAME = "run_packaged_sustained_memory_diagnostic_macos.used.sh"
VALIDATOR_COPY_NAME = "validate_sustained_memory_diagnostic.used.py"
RUNTIME_VALIDATOR_COPY_NAME = "validate_sustained_soak_log.used.py"
PACKAGE_VERIFIER_COPY_NAME = "verify_packaged_app.used.py"
MANIFEST_NAME = "EchoesOfTheBrokenSun.manifest.txt"
MANIFEST_DIGEST_NAME = "EchoesOfTheBrokenSun.manifest.sha256"
NORMAL_SMOKE_NAME = "EchoesOfTheBrokenSun.normal-startup-smoke.log"
STRESS_SMOKE_NAME = "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
DIAGNOSTIC_DIFF_PATHS = (
    "Docs/SetupAndBuild.md",
    "Scripts/run_packaged_sustained_memory_diagnostic_macos.sh",
    "Scripts/validate_sustained_memory_diagnostic.py",
    "Tests/Content/test_sustained_memory_diagnostic.py",
)
EXEC_COMMANDS = (
    "r.SetRes 1280x720w,r.VSync 0,t.MaxFPS 60,t.IdleWhenNotForeground 0,"
    "sg.ResolutionQuality 100,sg.ViewDistanceQuality 1,"
    "sg.AntiAliasingQuality 1,sg.ShadowQuality 1,"
    "sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,"
    "sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,"
    "sg.FoliageQuality 1,sg.ShadingQuality 1,sg.LandscapeQuality 1,"
    "r.AntiAliasingMethod 2"
)
CLAIM_BOUNDARY = (
    "This capture is diagnostic evidence only. It does not pass or replace the "
    "600-second preflight, establish a root cause, prove leak freedom, qualify "
    "a one-hour run, or establish release readiness."
)

BASE_FILES = {
    MANIFEST_NAME,
    MANIFEST_DIGEST_NAME,
    NORMAL_SMOKE_NAME,
    STRESS_SMOKE_NAME,
    TRACE_NAME,
    RAW_LOG_NAME,
    SAMPLES_NAME,
    METADATA_NAME,
    LAUNCH_COMMAND_NAME,
    INSIGHTS_LOG_NAME,
    TRACE_INSTRUCTIONS_NAME,
    RUNTIME_INVENTORY_NAME,
    RUNNER_COPY_NAME,
    VALIDATOR_COPY_NAME,
    RUNTIME_VALIDATOR_COPY_NAME,
    PACKAGE_VERIFIER_COPY_NAME,
    "player-save-before.manifest",
    "player-save-after.manifest",
}
FINAL_OUTPUT_FILES = {
    VALIDATION_NAME,
    EVIDENCE_INVENTORY_NAME,
    EVIDENCE_MANIFEST_NAME,
}
REQUIRED_METADATA_KEYS = {
    "record_type",
    "evidence_class",
    "fixture",
    "created_utc",
    "completed_utc",
    "requested_active_seconds",
    "sample_interval_seconds",
    "process_sample_count",
    "qualification_eligible",
    "diagnostic_instrumentation",
    "forced_gc",
    "forced_gc_log_observed",
    "thresholds_modified",
    "root_cause_established",
    "package_source_commit",
    "diagnostic_tooling_commit",
    "diagnostic_tooling_origin_main",
    "diagnostic_tooling_remote_main",
    "diagnostic_tooling_source_tree",
    "diagnostic_tooling_binding",
    "package_source_is_ancestor",
    "diagnostic_diff_paths",
    "application",
    "capture_staging_directory",
    "requested_final_directory",
    "configuration",
    "platform",
    "package_version",
    "package_manifest_sha256",
    "package_manifest_digest_sha256",
    "package_executable_sha256",
    "package_external_seal_sha256",
    "runner_sha256",
    "diagnostic_validator_sha256",
    "runtime_validator_sha256",
    "package_verifier_sha256",
    "normal_startup_smoke_sha256",
    "legacy_stress_startup_smoke_sha256",
    "launch_command_sha256",
    "runtime_log_sha256",
    "process_samples_sha256",
    "trace_sha256",
    "trace_size_bytes",
    "trace_channels_requested",
    "trace_channels_observed",
    "trace_parseability",
    "trace_parseability_log_sha256",
    "runtime_log_trace_anchor_mapping",
    "trace_analysis_required",
    "trace_analysis_instructions_sha256",
    "tick_2400_anchor_present",
    "tick_12000_anchor_present",
    "player_save_before_sha256",
    "player_save_after_sha256",
    "player_save_unchanged",
    "isolated_save_game_directory_during_capture",
    "isolated_user_directory_during_capture",
    "runtime_state_inventory_sha256",
    "termination_status",
    "wrapper_pid",
    "wrapper_pgid",
    "game_pid",
    "game_pgid",
    "host_model",
    "cpu_brand",
    "physical_memory_bytes",
    "macos_version",
    "macos_build",
    "host_architecture",
    "unreal_insights_path",
    "unreal_insights_sha256",
    "unreal_insights_sha256_scope",
    "unreal_insights_parser_module_closure_inventoried",
    "unreal_insights_version",
    "unreal_insights_build_id",
    "claim_boundary",
}


class DiagnosticValidationError(RuntimeError):
    """The diagnostic evidence failed a required safety or provenance gate."""


def _load_runtime_validator() -> tuple[Any, type[Exception]]:
    retained = pathlib.Path(__file__).with_name(RUNTIME_VALIDATOR_COPY_NAME)
    if retained.is_file():
        spec = importlib.util.spec_from_file_location(
            "validate_sustained_soak_log_retained", retained
        )
        if spec is None or spec.loader is None:
            raise ModuleNotFoundError("retained sustained validator cannot be loaded")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module.validate_log, module.ValidationError
    from validate_sustained_soak_log import (  # type: ignore
        ValidationError,
        validate_log,
    )

    return validate_log, ValidationError


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def canonical_json(value: Any) -> bytes:
    return (
        json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
        + "\n"
    ).encode("utf-8")


def _require_regular(path: pathlib.Path, *, minimum_size: int = 0) -> os.stat_result:
    try:
        info = path.lstat()
    except OSError as exc:
        raise DiagnosticValidationError(
            f"required file is missing: {path.name}"
        ) from exc
    if not stat.S_ISREG(info.st_mode) or path.is_symlink():
        raise DiagnosticValidationError(
            f"required path is not a regular file: {path.name}"
        )
    if info.st_nlink != 1:
        raise DiagnosticValidationError(
            f"hard-linked evidence file is unsafe: {path.name}"
        )
    if info.st_size < minimum_size:
        raise DiagnosticValidationError(f"required file is too small: {path.name}")
    return info


def _safe_entries(root: pathlib.Path) -> list[dict[str, Any]]:
    root_info = root.lstat()
    if not stat.S_ISDIR(root_info.st_mode) or root.is_symlink():
        raise DiagnosticValidationError(
            f"evidence root is not a real directory: {root}"
        )
    entries: list[dict[str, Any]] = [{"path": "", "type": "directory"}]

    def visit(directory: pathlib.Path) -> None:
        children = sorted(
            os.scandir(directory), key=lambda child: os.fsencode(child.name)
        )
        for child in children:
            if "\n" in child.name or "\r" in child.name:
                raise DiagnosticValidationError(
                    "evidence names may not contain newlines"
                )
            item = pathlib.Path(child.path)
            relative = item.relative_to(root).as_posix()
            info = child.stat(follow_symlinks=False)
            if stat.S_ISLNK(info.st_mode):
                raise DiagnosticValidationError(
                    f"symlink evidence is unsafe: {relative}"
                )
            if stat.S_ISDIR(info.st_mode):
                entries.append({"path": relative, "type": "directory"})
                visit(item)
            elif stat.S_ISREG(info.st_mode):
                if info.st_nlink != 1:
                    raise DiagnosticValidationError(
                        f"hard-linked evidence file is unsafe: {relative}"
                    )
                entries.append(
                    {
                        "path": relative,
                        "type": "file",
                        "size": info.st_size,
                        "sha256": sha256_file(item),
                    }
                )
            else:
                raise DiagnosticValidationError(
                    f"special filesystem entry is unsafe: {relative}"
                )

    visit(root)
    return entries


def build_runtime_inventory(runtime_state: pathlib.Path) -> dict[str, Any]:
    entries = _safe_entries(runtime_state)
    children = list(runtime_state.iterdir())
    immediate = {entry.name for entry in children}
    expected = {"save-games", "user-dir", "insights-user-dir"}
    if immediate != expected or any(
        not entry.is_dir() or entry.is_symlink() for entry in children
    ):
        raise DiagnosticValidationError(
            "runtime-state must contain only the three isolated runtime directories"
        )
    return {
        "format": "echoes-runtime-state-inventory-v1",
        "root": "runtime-state",
        "entries": entries,
    }


def write_runtime_inventory(evidence_dir: pathlib.Path) -> None:
    output = evidence_dir / RUNTIME_INVENTORY_NAME
    if output.exists() or output.is_symlink():
        raise DiagnosticValidationError("refusing to overwrite runtime-state inventory")
    payload = canonical_json(build_runtime_inventory(evidence_dir / "runtime-state"))
    with output.open("xb") as handle:
        handle.write(payload)


def _parse_metadata(path: pathlib.Path) -> dict[str, str]:
    _require_regular(path, minimum_size=1)
    result: dict[str, str] = {}
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), 1
    ):
        if not line or "=" not in line:
            raise DiagnosticValidationError(f"malformed metadata at line {line_number}")
        key, value = line.split("=", 1)
        if re.fullmatch(r"[a-z0-9_]+", key) is None or not value or key in result:
            raise DiagnosticValidationError(
                f"invalid or duplicate metadata key at line {line_number}"
            )
        result[key] = value
    if set(result) != REQUIRED_METADATA_KEYS:
        missing = sorted(REQUIRED_METADATA_KEYS - set(result))
        extra = sorted(set(result) - REQUIRED_METADATA_KEYS)
        raise DiagnosticValidationError(
            f"metadata key set drifted; missing={missing}, extra={extra}"
        )
    return result


def _validate_metadata(metadata: dict[str, str], root: pathlib.Path) -> None:
    exact = {
        "record_type": "Echoes packaged sustained memory diagnostic",
        "evidence_class": "non_qualifying_memory_diagnostic",
        "fixture": "Stress400Sustained",
        "requested_active_seconds": str(DURATION_SECONDS),
        "sample_interval_seconds": str(SAMPLE_INTERVAL_SECONDS),
        "process_sample_count": str(EXPECTED_SAMPLE_COUNT),
        "qualification_eligible": "false",
        "diagnostic_instrumentation": "UE5.8_MemoryTrace_full",
        "forced_gc": "false",
        "forced_gc_log_observed": "false",
        "thresholds_modified": "false",
        "root_cause_established": "false",
        "package_source_commit": PACKAGE_SOURCE_COMMIT,
        "diagnostic_tooling_source_tree": "clean",
        "diagnostic_tooling_binding": "clean-pushed-main",
        "package_source_is_ancestor": "true",
        "diagnostic_diff_paths": ",".join(DIAGNOSTIC_DIFF_PATHS),
        "application": str(APPLICATION),
        "configuration": "Development",
        "platform": "Mac-arm64",
        "package_version": "0.93.0",
        "package_manifest_sha256": PACKAGE_MANIFEST_SHA256,
        "package_manifest_digest_sha256": PACKAGE_MANIFEST_DIGEST_SHA256,
        "package_executable_sha256": PACKAGE_EXECUTABLE_SHA256,
        "package_external_seal_sha256": PACKAGE_EXTERNAL_SEAL_SHA256,
        "runtime_validator_sha256": RUNTIME_VALIDATOR_SHA256,
        "package_verifier_sha256": PACKAGE_VERIFIER_SHA256,
        "normal_startup_smoke_sha256": NORMAL_SMOKE_SHA256,
        "legacy_stress_startup_smoke_sha256": STRESS_SMOKE_SHA256,
        "trace_channels_requested": "default,Memory",
        "trace_channels_observed": "not_independently_enumerated",
        "trace_parseability": "launcher_exit_zero_and_analysis_completed_marker",
        "runtime_log_trace_anchor_mapping": "analyst_verification_required",
        "trace_analysis_required": "true",
        "tick_2400_anchor_present": "true",
        "tick_12000_anchor_present": "true",
        "player_save_unchanged": "true",
        "unreal_insights_path": str(UNREAL_INSIGHTS),
        "unreal_insights_sha256": UNREAL_INSIGHTS_SHA256,
        "unreal_insights_sha256_scope": "launcher_executable_only",
        "unreal_insights_parser_module_closure_inventoried": "false",
        "unreal_insights_version": UNREAL_INSIGHTS_VERSION,
        "unreal_insights_build_id": UNREAL_INSIGHTS_BUILD_ID,
        "claim_boundary": CLAIM_BOUNDARY,
    }
    for key, expected in exact.items():
        if metadata[key] != expected:
            raise DiagnosticValidationError(
                f"metadata {key} must be {expected!r}, observed {metadata[key]!r}"
            )
    for key in ("created_utc", "completed_utc"):
        if re.fullmatch(r"[0-9]{8}T[0-9]{6}Z", metadata[key]) is None:
            raise DiagnosticValidationError(f"metadata {key} is malformed")
    if metadata["completed_utc"] < metadata["created_utc"]:
        raise DiagnosticValidationError("completion precedes diagnostic creation")
    commit = metadata["diagnostic_tooling_commit"]
    if re.fullmatch(r"[0-9a-f]{40}", commit) is None or commit == PACKAGE_SOURCE_COMMIT:
        raise DiagnosticValidationError(
            "diagnostic tooling commit is malformed or stale"
        )
    for key in ("diagnostic_tooling_origin_main", "diagnostic_tooling_remote_main"):
        if metadata[key] != commit:
            raise DiagnosticValidationError("diagnostic tooling refs are not identical")
    staging = pathlib.Path(metadata["capture_staging_directory"])
    final = pathlib.Path(metadata["requested_final_directory"])
    if not staging.is_absolute() or not final.is_absolute() or staging == final:
        raise DiagnosticValidationError("diagnostic staging/final provenance is unsafe")
    if root not in {staging, final}:
        raise DiagnosticValidationError(
            "evidence root does not match staging/final provenance"
        )
    if re.fullmatch(r"\..+\.incomplete\.[A-Za-z0-9]{6}", staging.name) is None:
        raise DiagnosticValidationError("capture staging basename is malformed")
    if final.name.startswith(".") or final.parent != staging.parent:
        raise DiagnosticValidationError("requested final directory is unsafe")
    if metadata["isolated_save_game_directory_during_capture"] != str(
        staging / "runtime-state" / "save-games"
    ):
        raise DiagnosticValidationError("isolated save directory provenance drifted")
    if metadata["isolated_user_directory_during_capture"] != str(
        staging / "runtime-state" / "user-dir"
    ):
        raise DiagnosticValidationError("isolated user directory provenance drifted")
    for key in (
        "wrapper_pid",
        "wrapper_pgid",
        "game_pid",
        "game_pgid",
        "physical_memory_bytes",
        "trace_size_bytes",
    ):
        if re.fullmatch(r"[0-9]+", metadata[key]) is None or int(metadata[key]) <= 0:
            raise DiagnosticValidationError(
                f"metadata {key} must be a positive integer"
            )
    if metadata["game_pgid"] != metadata["game_pid"]:
        raise DiagnosticValidationError("game process-group leader provenance drifted")
    if metadata["game_pgid"] in {
        metadata["wrapper_pgid"],
        metadata["wrapper_pid"],
    }:
        raise DiagnosticValidationError(
            "game process group must be distinct from the diagnostic wrapper"
        )
    if metadata["termination_status"] not in {"0", "143"}:
        raise DiagnosticValidationError("game termination status is not accepted")
    for key in ("host_model", "cpu_brand", "macos_version", "macos_build"):
        if not metadata[key].strip():
            raise DiagnosticValidationError(f"metadata {key} is empty")
    if metadata["host_architecture"] != "arm64":
        raise DiagnosticValidationError("diagnostic host architecture must be arm64")
    for key, value in metadata.items():
        if key != "qualification_eligible" and "qualification" in key:
            raise DiagnosticValidationError(
                "unexpected qualification metadata is forbidden"
            )
        if re.search(
            r"(?:qualification|qualified)\s*=\s*(?:true|pass)", value, re.IGNORECASE
        ):
            raise DiagnosticValidationError("metadata contains a qualification claim")


def expected_launch_argv(staging: pathlib.Path) -> list[str]:
    return [
        str(APPLICATION / "Contents/MacOS/EchoesOfTheBrokenSun"),
        "/Engine/Maps/Entry",
        "-game",
        "-unattended",
        "-nop4",
        "-nosplash",
        "-nosound",
        "-EchoesStress400Sustained",
        "-EchoesMemoryDiagnostic",
        "-trace=default,Memory",
        f"-tracefile={staging / TRACE_NAME}",
        "-tracefiletrunc",
        f"-EchoesSaveGameDirectory={staging / 'runtime-state/save-games'}",
        f"-UserDir={staging / 'runtime-state/user-dir'}",
        "-stdout",
        "-FullStdOutLogOutput",
        "-windowed",
        "-ForceRes",
        "-ResX=1280",
        "-ResY=720",
        f"-ExecCmds={EXEC_COMMANDS}",
    ]


def _validate_launch(root: pathlib.Path, metadata: dict[str, str]) -> None:
    command_path = root / LAUNCH_COMMAND_NAME
    _require_regular(command_path, minimum_size=1)
    raw = command_path.read_text(encoding="utf-8")
    if not raw.endswith("\n") or raw.count("\n") != 1:
        raise DiagnosticValidationError("launch command must be one terminated line")
    try:
        observed = shlex.split(raw.strip(), posix=True)
    except ValueError as exc:
        raise DiagnosticValidationError(
            "launch command cannot be parsed safely"
        ) from exc
    staging = pathlib.Path(metadata["capture_staging_directory"])
    if observed != expected_launch_argv(staging):
        raise DiagnosticValidationError(
            "launch command drifted from the diagnostic contract"
        )
    folded = " ".join(observed).casefold()
    if "memreport" in folded or re.search(r"(?:^|[\s,;])mem(?:$|[\s,;])", folded):
        raise DiagnosticValidationError(
            "launch command requests a forbidden memory command"
        )
    if "collectgarbage" in folded or "forcegc" in folded:
        raise DiagnosticValidationError(
            "launch command requests forced garbage collection"
        )
    if sha256_file(command_path) != metadata["launch_command_sha256"]:
        raise DiagnosticValidationError("launch command hash does not match metadata")


def _parse_manifest(path: pathlib.Path) -> dict[str, str]:
    _require_regular(path, minimum_size=1)
    lines = path.read_text(encoding="utf-8").splitlines()
    try:
        end = lines.index("sha256  relative_path")
    except ValueError as exc:
        raise DiagnosticValidationError(
            "retained package manifest header is missing"
        ) from exc
    values: dict[str, str] = {}
    for line in lines[:end]:
        if not line:
            continue
        if "=" not in line:
            raise DiagnosticValidationError("retained package metadata is malformed")
        key, value = line.split("=", 1)
        if not key or not value or key in values:
            raise DiagnosticValidationError("retained package metadata is invalid")
        values[key] = value
    return values


def _validate_package_copies(root: pathlib.Path, metadata: dict[str, str]) -> None:
    bindings = {
        MANIFEST_NAME: "package_manifest_sha256",
        MANIFEST_DIGEST_NAME: "package_manifest_digest_sha256",
        NORMAL_SMOKE_NAME: "normal_startup_smoke_sha256",
        STRESS_SMOKE_NAME: "legacy_stress_startup_smoke_sha256",
        RUNNER_COPY_NAME: "runner_sha256",
        VALIDATOR_COPY_NAME: "diagnostic_validator_sha256",
        RUNTIME_VALIDATOR_COPY_NAME: "runtime_validator_sha256",
        PACKAGE_VERIFIER_COPY_NAME: "package_verifier_sha256",
    }
    for name, key in bindings.items():
        path = root / name
        _require_regular(path, minimum_size=1)
        if sha256_file(path) != metadata[key]:
            raise DiagnosticValidationError(f"retained file hash drifted: {name}")
    manifest_values = _parse_manifest(root / MANIFEST_NAME)
    expected_manifest = {
        "source_commit": PACKAGE_SOURCE_COMMIT,
        "origin_main": PACKAGE_SOURCE_COMMIT,
        "remote_main": PACKAGE_SOURCE_COMMIT,
        "source_tree": "clean",
        "source_binding": "clean-pushed-main",
        "configuration": "Development",
        "platform": "Mac-arm64",
        "bundle_short_version": "0.93.0",
        "normal_startup_smoke": NORMAL_SMOKE_NAME,
        "normal_startup_smoke_sha256": NORMAL_SMOKE_SHA256,
        "legacy_stress_startup_smoke": STRESS_SMOKE_NAME,
        "legacy_stress_startup_smoke_sha256": STRESS_SMOKE_SHA256,
    }
    for key, expected in expected_manifest.items():
        if manifest_values.get(key) != expected:
            raise DiagnosticValidationError(f"retained package manifest {key} drifted")
    sidecar = (root / MANIFEST_DIGEST_NAME).read_text(encoding="utf-8").splitlines()
    if sidecar != [f"{PACKAGE_MANIFEST_SHA256}  {MANIFEST_NAME}"]:
        raise DiagnosticValidationError("retained package manifest sidecar is invalid")


def _validate_samples(root: pathlib.Path, metadata: dict[str, str]) -> dict[str, float]:
    path = root / SAMPLES_NAME
    _require_regular(path, minimum_size=1)
    if sha256_file(path) != metadata["process_samples_sha256"]:
        raise DiagnosticValidationError("process-sample hash does not match metadata")
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    if not rows or set(rows[0]) != {"elapsed_seconds", "rss_mib", "cpu_percent"}:
        raise DiagnosticValidationError("process-sample schema is invalid")
    if len(rows) != EXPECTED_SAMPLE_COUNT:
        raise DiagnosticValidationError(
            f"expected {EXPECTED_SAMPLE_COUNT} process samples, observed {len(rows)}"
        )
    rss_values: list[float] = []
    cpu_values: list[float] = []
    for index, row in enumerate(rows):
        try:
            elapsed = int(row["elapsed_seconds"])
            rss = float(row["rss_mib"])
            cpu = float(row["cpu_percent"])
        except (TypeError, ValueError) as exc:
            raise DiagnosticValidationError("process sample is malformed") from exc
        if elapsed != index * SAMPLE_INTERVAL_SECONDS:
            raise DiagnosticValidationError("process-sample cadence drifted")
        if not math.isfinite(rss) or rss <= 0 or not math.isfinite(cpu) or cpu < 0:
            raise DiagnosticValidationError("process sample contains an invalid value")
        rss_values.append(rss)
        cpu_values.append(cpu)
    return {
        "rss_first_mib": rss_values[0],
        "rss_last_mib": rss_values[-1],
        "rss_peak_mib": max(rss_values),
        "cpu_peak_percent": max(cpu_values),
    }


def _has_anchor(text: str, marker: str, tick: int) -> bool:
    pattern = re.compile(rf"(?:^| )tick={tick}(?: |$)")
    return any(marker in line and pattern.search(line) for line in text.splitlines())


def _validate_runtime(root: pathlib.Path, metadata: dict[str, str]) -> dict[str, Any]:
    path = root / RAW_LOG_NAME
    _require_regular(path, minimum_size=1)
    if sha256_file(path) != metadata["runtime_log_sha256"]:
        raise DiagnosticValidationError("runtime log hash does not match metadata")
    text = path.read_text(encoding="utf-8", errors="replace")
    validate_log, runtime_error = _load_runtime_validator()
    try:
        result = validate_log(text, DURATION_SECONDS)
    except runtime_error as exc:
        raise DiagnosticValidationError(
            f"sustained runtime contract failed: {exc}"
        ) from exc
    if result.get("qualified_one_hour") is not False:
        raise DiagnosticValidationError("diagnostic runtime cannot be qualified")
    if "[ECHOES_STRESS_SUSTAINED_QUALIFIED]" in text:
        raise DiagnosticValidationError(
            "qualification marker is forbidden in diagnostics"
        )
    if re.search(r"\bforcedGc=true\b", text, re.IGNORECASE):
        raise DiagnosticValidationError("forced garbage collection was observed")
    for tick in (2400, 12000):
        for marker in (
            "[ECHOES_STRESS_SUSTAINED_HEARTBEAT]",
            "[ECHOES_STRESS_SUSTAINED_MEMORY]",
        ):
            if not _has_anchor(text, marker, tick):
                raise DiagnosticValidationError(
                    f"required {marker} tick-{tick} anchor is absent"
                )
    return result


def _validate_trace(root: pathlib.Path, metadata: dict[str, str]) -> dict[str, Any]:
    trace = root / TRACE_NAME
    trace_info = _require_regular(trace, minimum_size=4096)
    with trace.open("rb") as handle:
        magic = handle.read(4)
    if magic not in {b"2CRT", b"ECRT"}:
        raise DiagnosticValidationError(
            "trace file has an unrecognized Unreal trace header"
        )
    digest = sha256_file(trace)
    if digest != metadata["trace_sha256"] or trace_info.st_size != int(
        metadata["trace_size_bytes"]
    ):
        raise DiagnosticValidationError("trace size or hash does not match metadata")
    insights_log = root / INSIGHTS_LOG_NAME
    _require_regular(insights_log, minimum_size=1)
    insights_text = insights_log.read_text(encoding="utf-8", errors="replace")
    if "Analysis has completed" not in insights_text:
        raise DiagnosticValidationError(
            "Unreal Insights launcher did not emit the analysis-completed marker"
        )
    if "session analysis failed to start" in insights_text.casefold():
        raise DiagnosticValidationError(
            "UE 5.8 Insights failed to start trace analysis"
        )
    if sha256_file(insights_log) != metadata["trace_parseability_log_sha256"]:
        raise DiagnosticValidationError(
            "Insights parse-log hash does not match metadata"
        )
    instructions = root / TRACE_INSTRUCTIONS_NAME
    _require_regular(instructions, minimum_size=1)
    instruction_text = instructions.read_text(encoding="utf-8")
    for required in (
        "Memory Insights",
        "tick 2400",
        "tick 12000",
        "analyst-confirmed trace-relative timestamps",
        "AaBf",
        "launcher executable only",
        "semantic correctness",
        "not qualification evidence",
        "root_cause_established=false",
    ):
        if required not in instruction_text:
            raise DiagnosticValidationError(
                f"trace analysis instructions omit required boundary: {required}"
            )
    if sha256_file(instructions) != metadata["trace_analysis_instructions_sha256"]:
        raise DiagnosticValidationError(
            "trace instruction hash does not match metadata"
        )
    return {"trace_size_bytes": trace_info.st_size, "trace_sha256": digest}


def _validate_player_saves(root: pathlib.Path, metadata: dict[str, str]) -> None:
    before = root / "player-save-before.manifest"
    after = root / "player-save-after.manifest"
    _require_regular(before, minimum_size=1)
    _require_regular(after, minimum_size=1)
    if before.read_bytes() != after.read_bytes():
        raise DiagnosticValidationError("real player SaveGames manifest drifted")
    if sha256_file(before) != metadata["player_save_before_sha256"]:
        raise DiagnosticValidationError(
            "pre-run player-save hash does not match metadata"
        )
    if sha256_file(after) != metadata["player_save_after_sha256"]:
        raise DiagnosticValidationError(
            "post-run player-save hash does not match metadata"
        )
    try:
        records = json.loads(before.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise DiagnosticValidationError(
            "player-save manifest is not valid JSON"
        ) from exc
    if not isinstance(records, list) or not records:
        raise DiagnosticValidationError("player-save manifest must be a non-empty list")
    for record in records:
        if not isinstance(record, dict) or not isinstance(record.get("path"), str):
            raise DiagnosticValidationError("player-save manifest record is malformed")
        relative = pathlib.PurePath(record["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise DiagnosticValidationError("player-save manifest path is unsafe")


def _validate_runtime_inventory(root: pathlib.Path, metadata: dict[str, str]) -> None:
    path = root / RUNTIME_INVENTORY_NAME
    _require_regular(path, minimum_size=1)
    expected = canonical_json(build_runtime_inventory(root / "runtime-state"))
    if path.read_bytes() != expected:
        raise DiagnosticValidationError(
            "runtime-state inventory does not match retained state"
        )
    if sha256_file(path) != metadata["runtime_state_inventory_sha256"]:
        raise DiagnosticValidationError(
            "runtime-state inventory hash does not match metadata"
        )


def _validate_layout(root: pathlib.Path, *, sealed: bool) -> None:
    entries = _safe_entries(root)
    immediate_files = {
        entry.name
        for entry in root.iterdir()
        if entry.is_file() and not entry.is_symlink()
    }
    immediate_dirs = {
        entry.name
        for entry in root.iterdir()
        if entry.is_dir() and not entry.is_symlink()
    }
    expected_files = BASE_FILES | (FINAL_OUTPUT_FILES if sealed else set())
    if immediate_files != expected_files or immediate_dirs != {"runtime-state"}:
        raise DiagnosticValidationError(
            "diagnostic evidence top-level layout is missing, extra, or unsafe"
        )
    if any(entry["type"] not in {"directory", "file"} for entry in entries):
        raise DiagnosticValidationError("diagnostic evidence contains an unsafe type")


def validate_evidence(root: pathlib.Path, *, sealed: bool = False) -> dict[str, Any]:
    root = root.absolute()
    _validate_layout(root, sealed=sealed)
    metadata = _parse_metadata(root / METADATA_NAME)
    _validate_metadata(metadata, root)
    _validate_launch(root, metadata)
    _validate_package_copies(root, metadata)
    samples = _validate_samples(root, metadata)
    runtime = _validate_runtime(root, metadata)
    trace = _validate_trace(root, metadata)
    _validate_player_saves(root, metadata)
    _validate_runtime_inventory(root, metadata)
    result: dict[str, Any] = {
        "accepted_non_qualifying_diagnostic": True,
        "qualification_eligible": False,
        "diagnostic_instrumentation": "UE5.8_MemoryTrace_full",
        "forced_gc": False,
        "thresholds_modified": False,
        "root_cause_established": False,
        "package_source_commit": PACKAGE_SOURCE_COMMIT,
        "diagnostic_tooling_commit": metadata["diagnostic_tooling_commit"],
        "trace_channels_requested": ["default", "Memory"],
        "trace_channels_observed": None,
        "trace_channel_presence_gate": "manual_post_run_required",
        "trace_parseability": "launcher_exit_zero_and_analysis_completed_marker",
        "unreal_insights_sha256_scope": "launcher_executable_only",
        "unreal_insights_parser_module_closure_inventoried": False,
        "runtime_log_trace_anchor_mapping": "analyst_verification_required",
        "runtime_contract": runtime,
        "process_samples": {
            "count": EXPECTED_SAMPLE_COUNT,
            "interval_seconds": SAMPLE_INTERVAL_SECONDS,
            **samples,
        },
        **trace,
        "claim_boundary": CLAIM_BOUNDARY,
        "evidence_inventory": EVIDENCE_INVENTORY_NAME,
        "evidence_manifest": EVIDENCE_MANIFEST_NAME,
    }
    return result


def _inventory_without_self(root: pathlib.Path) -> dict[str, Any]:
    entries = [
        entry
        for entry in _safe_entries(root)
        if entry["path"] not in {EVIDENCE_INVENTORY_NAME, EVIDENCE_MANIFEST_NAME}
    ]
    current_paths = {entry["path"] for entry in entries if entry["path"]}
    expected_paths = sorted(
        current_paths | {EVIDENCE_INVENTORY_NAME, EVIDENCE_MANIFEST_NAME}
    )
    return {
        "format": "echoes-diagnostic-evidence-inventory-v1",
        "root": ".",
        "excluded_self_paths": [EVIDENCE_INVENTORY_NAME, EVIDENCE_MANIFEST_NAME],
        "expected_paths": expected_paths,
        "entries": entries,
    }


def _write_exclusive(path: pathlib.Path, content: bytes) -> None:
    if path.exists() or path.is_symlink():
        raise DiagnosticValidationError(
            f"refusing to overwrite evidence output: {path.name}"
        )
    with path.open("xb") as handle:
        handle.write(content)


def seal_evidence(root: pathlib.Path) -> dict[str, Any]:
    root = root.absolute()
    result = validate_evidence(root, sealed=False)
    _write_exclusive(root / VALIDATION_NAME, canonical_json(result))
    inventory = _inventory_without_self(root)
    _write_exclusive(root / EVIDENCE_INVENTORY_NAME, canonical_json(inventory))
    regular_files = sorted(
        entry["path"]
        for entry in _safe_entries(root)
        if entry["type"] == "file" and entry["path"] != EVIDENCE_MANIFEST_NAME
    )
    lines = [
        f"{sha256_file(root / relative)}  {relative}" for relative in regular_files
    ]
    _write_exclusive(root / EVIDENCE_MANIFEST_NAME, ("\n".join(lines) + "\n").encode())
    verify_seal(root)
    return result


def verify_seal(root: pathlib.Path) -> dict[str, Any]:
    root = root.absolute()
    result = validate_evidence(root, sealed=True)
    validation = root / VALIDATION_NAME
    if validation.read_bytes() != canonical_json(result):
        raise DiagnosticValidationError("stored diagnostic validation result drifted")
    inventory_path = root / EVIDENCE_INVENTORY_NAME
    try:
        inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise DiagnosticValidationError("evidence inventory is invalid JSON") from exc
    regenerated = _inventory_without_self(root)
    if inventory != regenerated:
        raise DiagnosticValidationError(
            "evidence inventory does not match exact evidence"
        )
    actual_paths = sorted(
        entry["path"] for entry in _safe_entries(root) if entry["path"]
    )
    if actual_paths != inventory.get("expected_paths"):
        raise DiagnosticValidationError("evidence path set differs from inventory")
    manifest_path = root / EVIDENCE_MANIFEST_NAME
    _require_regular(manifest_path, minimum_size=1)
    records: dict[str, str] = {}
    for line in manifest_path.read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  ([^\r\n]+)", line)
        if match is None:
            raise DiagnosticValidationError("diagnostic evidence manifest is malformed")
        digest, relative = match.groups()
        pure = pathlib.PurePosixPath(relative)
        if pure.is_absolute() or ".." in pure.parts or relative in records:
            raise DiagnosticValidationError(
                "diagnostic evidence manifest path is unsafe"
            )
        records[relative] = digest
    expected_files = {
        entry["path"]
        for entry in _safe_entries(root)
        if entry["type"] == "file" and entry["path"] != EVIDENCE_MANIFEST_NAME
    }
    if set(records) != expected_files:
        raise DiagnosticValidationError("diagnostic evidence manifest path set drifted")
    for relative, expected in records.items():
        if sha256_file(root / relative) != expected:
            raise DiagnosticValidationError(
                f"diagnostic evidence manifest hash drifted: {relative}"
            )
    return result


def _fsync_regular_file(path: pathlib.Path) -> None:
    flags = os.O_RDONLY
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    descriptor = os.open(path, flags)
    try:
        info = os.fstat(descriptor)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
            raise DiagnosticValidationError(
                f"evidence file changed type before synchronization: {path.name}"
            )
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def _fsync_directory(path: pathlib.Path) -> None:
    descriptor = os.open(path, os.O_RDONLY)
    try:
        info = os.fstat(descriptor)
        if not stat.S_ISDIR(info.st_mode):
            raise DiagnosticValidationError(
                f"evidence directory changed type before synchronization: {path.name}"
            )
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def _fsync_evidence_tree(root: pathlib.Path) -> None:
    entries = _safe_entries(root)
    files = [entry for entry in entries if entry["type"] == "file"]
    directories = [entry for entry in entries if entry["type"] == "directory"]
    for entry in files:
        _fsync_regular_file(root / entry["path"])
    for entry in sorted(
        directories,
        key=lambda record: len(pathlib.PurePosixPath(record["path"]).parts),
        reverse=True,
    ):
        _fsync_directory(root / entry["path"])


def _rename_directory_no_replace(
    staging_directory: pathlib.Path, final_directory: pathlib.Path
) -> None:
    if sys.platform != "darwin":
        raise DiagnosticValidationError(
            "atomic no-replace diagnostic publication requires macOS"
        )
    renameatx_np = ctypes.CDLL(None, use_errno=True).renameatx_np
    renameatx_np.argtypes = (
        ctypes.c_int,
        ctypes.c_char_p,
        ctypes.c_int,
        ctypes.c_char_p,
        ctypes.c_uint,
    )
    renameatx_np.restype = ctypes.c_int
    at_fdcwd = -2
    rename_exclusive = 0x00000004
    if (
        renameatx_np(
            at_fdcwd,
            os.fsencode(staging_directory),
            at_fdcwd,
            os.fsencode(final_directory),
            rename_exclusive,
        )
        != 0
    ):
        error_number = ctypes.get_errno()
        if error_number == errno.EEXIST:
            raise DiagnosticValidationError(
                "exclusive publication refused an existing final path"
            )
        raise OSError(error_number, os.strerror(error_number), str(final_directory))


def _best_effort_fsync_parent(path: pathlib.Path) -> None:
    try:
        _fsync_directory(path)
    except (DiagnosticValidationError, OSError):
        # The rename is the commit boundary. Some removable-drive filesystems do not
        # expose directory synchronization, so parent durability cannot be guaranteed.
        pass


def publish_evidence(
    staging_directory: pathlib.Path,
    final_directory: pathlib.Path,
    *,
    terminal_publish: bool = False,
    failure_injector: Callable[[str], None] | None = None,
) -> pathlib.Path:
    staging_directory = staging_directory.absolute()
    final_directory = final_directory.absolute()
    try:
        staging_info = staging_directory.lstat()
    except OSError as exc:
        raise DiagnosticValidationError(
            "staging evidence directory is missing"
        ) from exc
    if not stat.S_ISDIR(staging_info.st_mode) or staging_directory.is_symlink():
        raise DiagnosticValidationError("staging evidence directory is unsafe")
    if os.path.lexists(final_directory):
        raise DiagnosticValidationError("final diagnostic evidence path already exists")
    if staging_directory.parent.resolve() != final_directory.parent.resolve():
        raise DiagnosticValidationError(
            "staging and final diagnostic directories must share one parent"
        )

    verify_seal(staging_directory)
    _fsync_evidence_tree(staging_directory)
    verify_seal(staging_directory)
    if failure_injector is not None:
        failure_injector("before_commit")
    if terminal_publish:
        signal.pthread_sigmask(
            signal.SIG_BLOCK,
            {signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGQUIT},
        )
    _rename_directory_no_replace(staging_directory, final_directory)
    _best_effort_fsync_parent(final_directory.parent)
    if terminal_publish:
        try:
            os.write(
                1,
                (
                    "Non-qualifying sustained memory diagnostic captured: "
                    f"{final_directory}\n"
                ).encode(),
            )
        except OSError:
            pass
        os._exit(0)
    return final_directory


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True, type=pathlib.Path)
    parser.add_argument("--final-dir", type=pathlib.Path)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write-runtime-inventory", action="store_true")
    mode.add_argument("--seal", action="store_true")
    mode.add_argument("--verify-seal", action="store_true")
    mode.add_argument("--publish", action="store_true")
    args = parser.parse_args()

    if args.publish != (args.final_dir is not None):
        parser.error("--publish and --final-dir must be supplied together")

    handled_signals = {
        signal.SIGINT,
        signal.SIGTERM,
        signal.SIGHUP,
        signal.SIGQUIT,
    }
    original_signal_mask: set[signal.Signals] | None = None
    termination_signal = "none"
    if args.publish:
        original_signal_mask = signal.pthread_sigmask(signal.SIG_BLOCK, handled_signals)

        def interrupted(signum: int, _frame: object) -> None:
            nonlocal termination_signal
            signal.pthread_sigmask(signal.SIG_BLOCK, handled_signals)
            termination_signal = signal.Signals(signum).name.removeprefix("SIG")
            raise DiagnosticValidationError(
                f"diagnostic publication interrupted by signal {signum} before commit"
            )

        for handled_signal in handled_signals:
            signal.signal(handled_signal, interrupted)
        signal.pthread_sigmask(signal.SIG_SETMASK, original_signal_mask)

    try:
        if args.write_runtime_inventory:
            write_runtime_inventory(args.evidence_dir)
            result: dict[str, Any] = {
                "runtime_state_inventory_written": True,
                "qualification_eligible": False,
            }
        elif args.seal:
            result = seal_evidence(args.evidence_dir)
        elif args.verify_seal:
            result = verify_seal(args.evidence_dir)
        else:
            assert args.final_dir is not None
            publish_evidence(
                args.evidence_dir,
                args.final_dir,
                terminal_publish=True,
            )
            raise AssertionError("terminal publication returned without exiting")
    except (DiagnosticValidationError, OSError, ValueError) as exc:
        if args.publish:
            signal.pthread_sigmask(signal.SIG_BLOCK, handled_signals)
        print(f"SUSTAINED_MEMORY_DIAGNOSTIC_VALIDATION_FAILED: {exc}", file=sys.stderr)
        exit_code = {
            "INT": 130,
            "TERM": 143,
            "HUP": 129,
            "QUIT": 131,
        }.get(termination_signal, 1)
        if args.publish:
            try:
                sys.stdout.flush()
                sys.stderr.flush()
            finally:
                os._exit(exit_code)
        return exit_code
    if original_signal_mask is not None:
        signal.pthread_sigmask(signal.SIG_SETMASK, original_signal_mask)
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
