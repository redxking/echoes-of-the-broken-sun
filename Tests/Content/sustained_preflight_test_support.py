from __future__ import annotations

import hashlib
import json
import pathlib
import shutil


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]

from finalize_sustained_evidence import SUMMARY_NAME, finalize_evidence
from test_sustained_soak_validator import valid_log
from validate_sustained_preflight import (
    METADATA_NAME,
    PACKAGE_MANIFEST_DIGEST_NAME,
    PACKAGE_MANIFEST_NAME,
    PREFLIGHT_BINDING_NAME,
    PREFLIGHT_SNAPSHOT_NAME,
    RAW_LOG_NAME,
    RUNTIME_INVENTORY_NAME,
    SAMPLES_NAME,
    TOOL_FILES,
    VALIDATION_NAME,
    recompute_process_measurements,
    snapshot_verified_preflight,
    validate_log,
)


NORMAL_SMOKE_NAME = "EchoesOfTheBrokenSun.normal-startup-smoke.log"
STRESS_SMOKE_NAME = "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"


def digest(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def create_published_preflight(
    root: pathlib.Path,
    *,
    budgets_pass: bool = True,
) -> tuple[pathlib.Path, dict[str, object]]:
    staging = root / ".preflight.incomplete-test"
    final = root / "preflight"
    staging.mkdir(parents=True)
    runtime_state = staging / "runtime-state"
    (runtime_state / "save-games").mkdir(parents=True)
    (runtime_state / "user-dir").mkdir()

    source_tools = {
        "runner_sha256": PROJECT_ROOT / "Scripts" / "soak_packaged_sustained_macos.sh",
        "validator_sha256": PROJECT_ROOT / "Scripts" / "validate_sustained_soak_log.py",
        "packager_sha256": PROJECT_ROOT / "Scripts" / "package_macos.sh",
        "package_verifier_sha256": PROJECT_ROOT / "Scripts" / "verify_packaged_app.py",
        "evidence_finalizer_sha256": PROJECT_ROOT / "Scripts" / "finalize_sustained_evidence.py",
        "preflight_verifier_sha256": PROJECT_ROOT / "Scripts" / "validate_sustained_preflight.py",
    }
    expected: dict[str, object] = {
        "artifact": "EchoesOfTheBrokenSun.app",
        "configuration": "Development",
        "platform": "Mac-arm64",
        "package_version": "0.93.0",
        "source_commit": "1" * 40,
    }
    for field, source in source_tools.items():
        target = staging / TOOL_FILES[field]
        target.write_bytes(source.read_bytes())
        expected[field] = digest(target)

    normal_smoke = staging / NORMAL_SMOKE_NAME
    stress_smoke = staging / STRESS_SMOKE_NAME
    normal_smoke.write_text("normal package startup passed\n", encoding="utf-8")
    stress_smoke.write_text("legacy stress package startup passed\n", encoding="utf-8")
    expected["normal_startup_smoke_sha256"] = digest(normal_smoke)
    expected["legacy_stress_startup_smoke_sha256"] = digest(stress_smoke)

    package_manifest = staging / PACKAGE_MANIFEST_NAME
    package_manifest.write_text(
        "\n".join(
            (
                "artifact=EchoesOfTheBrokenSun.app",
                "created_utc=20260831T000000Z",
                f"source_commit={expected['source_commit']}",
                f"origin_main={expected['source_commit']}",
                f"remote_main={expected['source_commit']}",
                "source_tree=clean",
                "source_binding=clean-pushed-main",
                "configuration=Development",
                "platform=Mac-arm64",
                f"normal_startup_smoke={NORMAL_SMOKE_NAME}",
                f"normal_startup_smoke_sha256={expected['normal_startup_smoke_sha256']}",
                f"legacy_stress_startup_smoke={STRESS_SMOKE_NAME}",
                f"legacy_stress_startup_smoke_sha256={expected['legacy_stress_startup_smoke_sha256']}",
                "unreal_engine=5.8.0",
                "xcode=26.6",
                "bundle_identifier=com.angelispseftis.echoesofthebrokensun",
                "bundle_short_version=0.93.0",
                "",
                "sha256  relative_path",
                "",
            )
        ),
        encoding="utf-8",
    )
    expected["manifest_sha256"] = digest(package_manifest)
    (staging / PACKAGE_MANIFEST_DIGEST_NAME).write_text(
        f"{expected['manifest_sha256']}  {PACKAGE_MANIFEST_NAME}\n",
        encoding="utf-8",
    )

    raw_log = staging / RAW_LOG_NAME
    raw_log.write_text(valid_log(600), encoding="utf-8")
    runtime_contract = validate_log(raw_log.read_text(encoding="utf-8"), 600)
    (staging / VALIDATION_NAME).write_text(
        json.dumps(runtime_contract, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    samples = staging / SAMPLES_NAME
    samples.write_text(
        "elapsed_seconds,rss_mib,cpu_percent\n"
        + "".join(f"{second},512.000,10.000\n" for second in range(0, 601, 5)),
        encoding="utf-8",
    )
    measured = recompute_process_measurements(samples)
    if not budgets_pass:
        measured["all_measured_budgets_pass"] = False
        measured["budgets"] = dict(measured["budgets"])
        measured["budgets"][
            "steady_linear_growth_mib_per_hour_le_128"
        ] = False

    summary = dict(expected)
    summary.update(measured)
    summary.update(
        {
            "fixture": "Stress400Sustained",
            "all_measured_budgets_pass": budgets_pass,
            "qualified_one_hour": False,
            "requested_active_seconds": 600,
            "run_class": "ten_minute_preflight",
            "runtime_contract": runtime_contract,
            "runtime_contract_qualified_one_hour": False,
            "preflight_evidence_directory": None,
            "preflight_evidence_manifest_sha256": None,
            "preflight_snapshot_sha256": None,
        }
    )
    (staging / SUMMARY_NAME).write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    metadata_lines = [
        "fixture=Stress400Sustained",
        "requested_active_seconds=600",
        "run_class=ten_minute_preflight",
        f"source_commit={expected['source_commit']}",
        f"origin_main={expected['source_commit']}",
        f"remote_main={expected['source_commit']}",
        "source_tree=clean",
        "source_binding=clean-pushed-main",
        "package_version=0.93.0",
        "configuration=Development",
        "platform=Mac-arm64",
        f"manifest_sha256={expected['manifest_sha256']}",
    ]
    metadata_lines.extend(f"{field}={expected[field]}" for field in TOOL_FILES)
    metadata_lines.extend(
        (
            "termination_status=143",
            f"post_run_source_commit={expected['source_commit']}",
            f"post_run_origin_main={expected['source_commit']}",
            f"post_run_remote_main={expected['source_commit']}",
            "post_run_source_tree=clean",
            "post_run_package_integrity=verified",
        )
    )
    (staging / METADATA_NAME).write_text(
        "\n".join(metadata_lines) + "\n", encoding="utf-8"
    )
    (staging / RUNTIME_INVENTORY_NAME).write_text(
        "runtime_state=/retained/runtime-state\n"
        "save_game_directory=/retained/runtime-state/save-games\n"
        "user_directory=/retained/runtime-state/user-dir\n"
        "file_count=0\n",
        encoding="utf-8",
    )

    evidence_names = [
        PACKAGE_MANIFEST_NAME,
        PACKAGE_MANIFEST_DIGEST_NAME,
        RAW_LOG_NAME,
        SAMPLES_NAME,
        VALIDATION_NAME,
        SUMMARY_NAME,
        METADATA_NAME,
        RUNTIME_INVENTORY_NAME,
        NORMAL_SMOKE_NAME,
        STRESS_SMOKE_NAME,
        *TOOL_FILES.values(),
    ]
    finalize_evidence(staging, final, evidence_names)
    return final, expected


def create_staged_one_hour(
    root: pathlib.Path,
    *,
    budgets_pass: bool = True,
) -> tuple[pathlib.Path, pathlib.Path, list[str]]:
    preflight, expected = create_published_preflight(root / "source")
    staging = root / ".evidence.incomplete-test"
    final = root / "evidence"
    staging.mkdir()
    runtime_state = staging / "runtime-state"
    (runtime_state / "save-games").mkdir(parents=True)
    (runtime_state / "user-dir").mkdir()

    for name in (
        PACKAGE_MANIFEST_NAME,
        PACKAGE_MANIFEST_DIGEST_NAME,
        NORMAL_SMOKE_NAME,
        STRESS_SMOKE_NAME,
        *TOOL_FILES.values(),
    ):
        shutil.copy2(preflight / name, staging / name)

    snapshot = staging / PREFLIGHT_SNAPSHOT_NAME
    binding = snapshot_verified_preflight(preflight, snapshot, expected)
    (staging / PREFLIGHT_BINDING_NAME).write_text(
        json.dumps(binding, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    raw_log = staging / RAW_LOG_NAME
    raw_log.write_text(valid_log(3600, qualified=True), encoding="utf-8")
    runtime_contract = validate_log(raw_log.read_text(encoding="utf-8"), 3600)
    (staging / VALIDATION_NAME).write_text(
        json.dumps(runtime_contract, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    samples = staging / SAMPLES_NAME
    samples.write_text(
        "elapsed_seconds,rss_mib,cpu_percent\n"
        + "".join(
            f"{second},{512.0 + (0.1 * second if not budgets_pass else 0.0):.3f},10.000\n"
            for second in range(0, 3601, 5)
        ),
        encoding="utf-8",
    )
    measured = recompute_process_measurements(
        samples,
        duration_seconds=3600,
        warmup_seconds=120,
        sample_interval=5,
        minimum_steady_samples=10,
    )
    if bool(measured["all_measured_budgets_pass"]) != budgets_pass:
        raise AssertionError("Synthetic one-hour budget fixture did not match its request")

    final_text = str(final.absolute())
    host = {
        "model": "Mac-test",
        "cpu": "Apple-test",
        "physical_memory_bytes": 34359738368,
        "macos_version": "15.0",
        "macos_build": "24A000",
        "architecture": "arm64",
    }
    summary = dict(expected)
    summary.update(measured)
    summary.update(
        {
            "fixture": "Stress400Sustained",
            "qualified_one_hour": False,
            "requested_active_seconds": 3600,
            "run_class": "one_hour_qualification",
            "runtime_contract": runtime_contract,
            "runtime_contract_qualified_one_hour": True,
            "preflight_evidence_directory": str(preflight.absolute()),
            "preflight_evidence_manifest_sha256": binding[
                "preflight_evidence_manifest_sha256"
            ],
            "preflight_snapshot_sha256": binding["preflight_snapshot_sha256"],
            "host": host,
            "claim_boundary": "Synthetic unit-test evidence only.",
        }
    )
    (staging / SUMMARY_NAME).write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    metadata_lines = [
        "fixture=Stress400Sustained",
        "created_utc=20260831T000000Z",
        "application=/tmp/EchoesOfTheBrokenSun.app",
        f"evidence_directory={final_text}",
        "requested_active_seconds=3600",
        "run_class=one_hour_qualification",
        f"source_commit={expected['source_commit']}",
        f"origin_main={expected['source_commit']}",
        f"remote_main={expected['source_commit']}",
        "source_tree=clean",
        "source_binding=clean-pushed-main",
        "package_version=0.93.0",
        "configuration=Development",
        "platform=Mac-arm64",
        f"manifest_sha256={expected['manifest_sha256']}",
    ]
    metadata_lines.extend(f"{field}={expected[field]}" for field in TOOL_FILES)
    metadata_lines.extend(
        (
            f"preflight_evidence_directory={preflight.absolute()}",
            "preflight_evidence_manifest_sha256="
            f"{binding['preflight_evidence_manifest_sha256']}",
            f"preflight_snapshot_sha256={binding['preflight_snapshot_sha256']}",
            f"normal_startup_smoke_sha256={expected['normal_startup_smoke_sha256']}",
            "legacy_stress_startup_smoke_sha256="
            f"{expected['legacy_stress_startup_smoke_sha256']}",
            f"isolated_save_game_directory={final_text}/runtime-state/save-games",
            f"isolated_user_directory={final_text}/runtime-state/user-dir",
            f"host_model={host['model']}",
            f"cpu_brand={host['cpu']}",
            f"physical_memory_bytes={host['physical_memory_bytes']}",
            f"macos_version={host['macos_version']}",
            f"macos_build={host['macos_build']}",
            f"host_architecture={host['architecture']}",
            "termination_status=143",
            "completed_utc=20260831T010000Z",
            f"post_run_source_commit={expected['source_commit']}",
            f"post_run_origin_main={expected['source_commit']}",
            f"post_run_remote_main={expected['source_commit']}",
            "post_run_source_tree=clean",
            "post_run_package_integrity=verified",
        )
    )
    (staging / METADATA_NAME).write_text(
        "\n".join(metadata_lines) + "\n", encoding="utf-8"
    )
    (staging / RUNTIME_INVENTORY_NAME).write_text(
        f"runtime_state={final_text}/runtime-state\n"
        f"save_game_directory={final_text}/runtime-state/save-games\n"
        f"user_directory={final_text}/runtime-state/user-dir\n"
        "file_count=0\n",
        encoding="utf-8",
    )

    evidence_names = [
        PACKAGE_MANIFEST_NAME,
        PACKAGE_MANIFEST_DIGEST_NAME,
        RAW_LOG_NAME,
        SAMPLES_NAME,
        VALIDATION_NAME,
        SUMMARY_NAME,
        METADATA_NAME,
        RUNTIME_INVENTORY_NAME,
        NORMAL_SMOKE_NAME,
        STRESS_SMOKE_NAME,
        *TOOL_FILES.values(),
        PREFLIGHT_BINDING_NAME,
        PREFLIGHT_SNAPSHOT_NAME,
    ]
    return staging, final, evidence_names
