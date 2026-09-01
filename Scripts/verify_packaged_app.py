#!/usr/bin/env python3
"""Verify an Echoes package against its exact signed-content manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import plistlib
import re
import subprocess

HASH_RECORD = re.compile(r"^([0-9a-f]{64})  (.+)$")
LINK_RECORD = re.compile(r"^SYMLINK  (.+) -> (.*)$")
SIDECAR_RECORD = re.compile(r"^([0-9a-f]{64})  ([^/]+)$")
SHA40 = re.compile(r"^[0-9a-f]{40}$")
SHA256 = re.compile(r"^[0-9a-f]{64}$")
GIT_LFS_INVENTORY_RECORD = re.compile(r"^([0-9a-f]{64}) ([*-]) (.+)$")
EMPTY_SHA256 = hashlib.sha256(b"").hexdigest()
EXPECTED_BUNDLE_IDENTIFIER = "com.angelispseftis.echoesofthebrokensun"
EXPECTED_EXECUTABLE = "EchoesOfTheBrokenSun"
EXPECTED_UNREAL_ROOT = "/Users/Shared/Epic Games/UE_5.8"
EXPECTED_DEVELOPER_DIR = "/Applications/Xcode.app/Contents/Developer"
EXPECTED_GIT_PATH = "/opt/homebrew/bin/git"
EXPECTED_GIT_VERSION = "git version 2.55.0"
EXPECTED_GIT_RESOLVED_PATH = "/opt/homebrew/Cellar/git/2.55.0/bin/git"
EXPECTED_GIT_SHA256 = "9048038886ac36210fbb616b49b0707465f63683cb04e33a2013baf95f746938"
EXPECTED_GIT_LFS_PATH = "/opt/homebrew/bin/git-lfs"
EXPECTED_GIT_LFS_VERSION = "git-lfs/3.7.1 (GitHub; darwin arm64; go 1.25.3)"
EXPECTED_GIT_LFS_RESOLVED_PATH = "/opt/homebrew/Cellar/git-lfs/3.7.1/bin/git-lfs"
EXPECTED_GIT_LFS_SHA256 = (
    "8a62ba6b8bc9ab15cae4b2704c434568b2d8bd4bda9468a0d48fb70131191501"
)
EXPECTED_ORIGIN_URL = "https://github.com/redxking/echoes-of-the-broken-sun.git"
GIT_ENVIRONMENT_PREFIX = "GIT_"
CONTROLLED_GIT_ENVIRONMENT = {
    "GIT_ATTR_NOSYSTEM": "1",
    "GIT_CONFIG_GLOBAL": "/dev/null",
    "GIT_CONFIG_NOSYSTEM": "1",
    "GIT_CONFIG_SYSTEM": "/dev/null",
    "GIT_NO_REPLACE_OBJECTS": "1",
    "GIT_TERMINAL_PROMPT": "0",
}
GIT_ENVIRONMENT_POLICY = (
    "reject-all-inherited-GIT-prefix;controlled="
    "GIT_ATTR_NOSYSTEM=1,GIT_CONFIG_GLOBAL=/dev/null,GIT_CONFIG_NOSYSTEM=1,"
    "GIT_CONFIG_SYSTEM=/dev/null,GIT_NO_REPLACE_OBJECTS=1,GIT_TERMINAL_PROMPT=0"
)
GENERATED_CONTENT_PACK_SOURCE = "Content/Data/Generated/EchoesContentPack.json"
GENERATED_CONTENT_PACK_DIGEST_SOURCE = f"{GENERATED_CONTENT_PACK_SOURCE}.sha256"

SCHEMA_2_EXACT_METADATA = {
    "manifest_schema": "2",
    "source_tree": "clean",
    "source_binding": "clean-pushed-main",
    "source_branch": "detached",
    "source_upstream_ref": "origin/main",
    "source_origin_fetch_url": EXPECTED_ORIGIN_URL,
    "source_origin_push_url": EXPECTED_ORIGIN_URL,
    "source_remote_authority": "github.com/redxking/echoes-of-the-broken-sun",
    "source_checkout_kind": "dedicated-linked-worktree-detached-at-main",
    "source_top_level_binding": "canonical-exact",
    "git_environment_policy": GIT_ENVIRONMENT_POLICY,
    "repo_local_derived_state_before": "clean",
    "repo_local_derived_state_scope": "git-ignored-paths-within-source-checkout",
    "source_index_concealment": "absent",
    "source_index_concealment_scope": "git-ls-files-v-and-f-non-H-records",
    "git_lfs_status": "clean",
    "git_lfs_fsck": "passed",
    "git_lfs_fsck_outcome": "passed",
    "git_lfs_restrictive_fetch_config": "absent",
    "git_lfs_restrictive_fetch_config_scope": (
        "effective-git-lfs-FetchInclude-and-FetchExclude"
    ),
    "git_lfs_hydration": "complete",
    "git_lfs_hydration_scope": "all-git-lfs-tracked-working-tree-files",
    "git_lfs_hydration_outcome": "passed",
    "content_preflight_outcome": "passed",
    "environment_preflight_outcome": "passed",
    "package_preflight_outcome": "passed",
    "build_cook_run_outcome": "passed",
    "normal_startup_smoke_outcome": "passed",
    "legacy_stress_startup_smoke_outcome": "passed",
    "configuration": "Development",
    "platform": "Mac-arm64",
    "architecture": "arm64",
    "archive_outside_checkout": "true",
    "host_arch": "arm64",
    "unreal_engine": "5.8.2",
    "unreal_root": EXPECTED_UNREAL_ROOT,
    "unreal_promoted": "1",
    "developer_dir": EXPECTED_DEVELOPER_DIR,
    "git_path": EXPECTED_GIT_PATH,
    "git_resolved_path": EXPECTED_GIT_RESOLVED_PATH,
    "git_version": EXPECTED_GIT_VERSION,
    "git_sha256": EXPECTED_GIT_SHA256,
    "git_lfs_path": EXPECTED_GIT_LFS_PATH,
    "git_lfs_resolved_path": EXPECTED_GIT_LFS_RESOLVED_PATH,
    "git_lfs_version": EXPECTED_GIT_LFS_VERSION,
    "git_lfs_sha256": EXPECTED_GIT_LFS_SHA256,
    "ignored_cook_inputs": (
        f"{GENERATED_CONTENT_PACK_SOURCE};{GENERATED_CONTENT_PACK_DIGEST_SOURCE}"
    ),
    "generated_content_pack_source": GENERATED_CONTENT_PACK_SOURCE,
    "generated_content_pack_digest_source": GENERATED_CONTENT_PACK_DIGEST_SOURCE,
    "bundle_identifier": EXPECTED_BUNDLE_IDENTIFIER,
    "signature_class": "adhoc",
    "signature_team_identifier": "none",
    "signature_verification": "passed",
    "developer_id_signing": "not-performed",
    "notarization_status": "not-submitted-by-package-tool",
    "installer_status": "not-produced",
    "release_qualification": "not-release-qualified",
    "build_context_record": "package-tool-observed",
    "manifest_authority": "self-consistency-record-not-independent-attestation",
    "claim_boundary": "local-development-package-only",
}

SCHEMA_2_EVIDENCE = (
    (
        "source_status_evidence",
        "source_status_sha256",
        "EchoesOfTheBrokenSun.source-status.porcelain-v2-z",
    ),
    (
        "repo_local_ignored_state_evidence",
        "repo_local_ignored_state_sha256",
        "EchoesOfTheBrokenSun.repo-local-ignored-before.txt",
    ),
    (
        "source_index_concealment_evidence",
        "source_index_concealment_sha256",
        "EchoesOfTheBrokenSun.source-index-concealment.txt",
    ),
    (
        "git_lfs_status_evidence",
        "git_lfs_status_sha256",
        "EchoesOfTheBrokenSun.git-lfs-status.porcelain",
    ),
    (
        "git_lfs_fsck_evidence",
        "git_lfs_fsck_sha256",
        "EchoesOfTheBrokenSun.git-lfs-fsck.txt",
    ),
    (
        "git_lfs_hydration_evidence",
        "git_lfs_hydration_sha256",
        "EchoesOfTheBrokenSun.git-lfs-hydration.txt",
    ),
    (
        "git_lfs_restrictive_config_evidence",
        "git_lfs_restrictive_config_sha256",
        "EchoesOfTheBrokenSun.git-lfs-restrictive-config.txt",
    ),
    (
        "package_preflight_log",
        "package_preflight_log_sha256",
        "EchoesOfTheBrokenSun.package-preflight.log",
    ),
    (
        "build_log",
        "build_log_sha256",
        "EchoesOfTheBrokenSun.BuildCookRun.log",
    ),
    (
        "generated_content_pack",
        "generated_content_pack_sha256",
        "EchoesContentPack.cooked-input.json",
    ),
    (
        "generated_content_pack_digest",
        "generated_content_pack_digest_sha256",
        "EchoesContentPack.cooked-input.json.sha256",
    ),
    (
        "normal_startup_smoke",
        "normal_startup_smoke_sha256",
        "EchoesOfTheBrokenSun.normal-startup-smoke.log",
    ),
    (
        "legacy_stress_startup_smoke",
        "legacy_stress_startup_smoke_sha256",
        "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log",
    ),
    (
        "signature_evidence",
        "signature_evidence_sha256",
        "EchoesOfTheBrokenSun.signature-assessment.txt",
    ),
    (
        "gatekeeper_evidence",
        "gatekeeper_evidence_sha256",
        "EchoesOfTheBrokenSun.gatekeeper-assessment.txt",
    ),
    (
        "stapler_evidence",
        "stapler_evidence_sha256",
        "EchoesOfTheBrokenSun.stapler-validation.txt",
    ),
    (
        "toolchain_evidence",
        "toolchain_evidence_sha256",
        "EchoesOfTheBrokenSun.toolchain.txt",
    ),
    ("packager_copy", "packager_sha256", "package_macos.used.sh"),
    (
        "package_verifier_copy",
        "package_verifier_sha256",
        "verify_packaged_app.used.py",
    ),
)


class VerificationError(RuntimeError):
    pass


def _sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _bundle_entries(
    contents: pathlib.Path, app: pathlib.Path
) -> tuple[set[str], set[str]]:
    files: set[str] = set()
    links: set[str] = set()
    for directory, directory_names, file_names in os.walk(contents, followlinks=False):
        parent = pathlib.Path(directory)
        for name in directory_names:
            path = parent / name
            relative = path.relative_to(app).as_posix()
            if path.is_symlink():
                links.add(relative)
            elif not path.is_dir():
                raise VerificationError(f"unsupported special bundle entry: {relative}")
        for name in file_names:
            path = parent / name
            relative = path.relative_to(app).as_posix()
            if path.is_symlink():
                links.add(relative)
            elif path.is_file():
                files.add(relative)
            else:
                raise VerificationError(f"unsupported special bundle entry: {relative}")
    return files, links


def _require_sha(metadata: dict[str, str], key: str, pattern: re.Pattern[str]) -> str:
    value = metadata.get(key, "")
    if pattern.fullmatch(value) is None:
        raise VerificationError(f"manifest metadata {key} is missing or malformed")
    return value


def _require_nonnegative_integer(metadata: dict[str, str], key: str) -> int:
    value = metadata.get(key, "")
    if re.fullmatch(r"0|[1-9][0-9]*", value) is None:
        raise VerificationError(f"manifest metadata {key} is missing or malformed")
    return int(value)


def _run_tool(
    arguments: list[str],
    purpose: str,
    *,
    cwd: pathlib.Path | None = None,
    environment: dict[str, str] | None = None,
    timeout_seconds: int = 300,
) -> str:
    try:
        completed = subprocess.run(
            arguments,
            check=False,
            capture_output=True,
            text=True,
            cwd=cwd,
            env=environment,
            timeout=timeout_seconds,
        )
    except OSError as exc:
        raise VerificationError(f"{purpose} could not run") from exc
    except subprocess.TimeoutExpired as exc:
        raise VerificationError(
            f"{purpose} timed out after {timeout_seconds} seconds"
        ) from exc
    output = completed.stdout + completed.stderr
    if completed.returncode != 0:
        raise VerificationError(f"{purpose} failed: {output.strip()}")
    return output


def _sanitized_git_environment() -> dict[str, str]:
    rejected_names = sorted(
        name
        for name, value in os.environ.items()
        if name.startswith(GIT_ENVIRONMENT_PREFIX)
        and CONTROLLED_GIT_ENVIRONMENT.get(name) != value
    )
    if rejected_names:
        raise VerificationError(
            "live build context refuses inherited Git or Git LFS environment "
            f"variables: {', '.join(rejected_names)}"
        )
    environment = {
        name: value
        for name, value in os.environ.items()
        if not name.startswith(GIT_ENVIRONMENT_PREFIX)
    }
    environment.update(CONTROLLED_GIT_ENVIRONMENT)
    return environment


def _structured_values(
    path: pathlib.Path,
    required_keys: tuple[str, ...],
) -> tuple[dict[str, str], str]:
    text = path.read_text(encoding="utf-8")
    required = set(required_keys)
    values: dict[str, str] = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key not in required:
            continue
        if not value or key in values:
            raise VerificationError(f"evidence {path.name} has invalid {key}")
        values[key] = value
    missing = required.difference(values)
    if missing:
        raise VerificationError(
            f"evidence {path.name} is missing {', '.join(sorted(missing))}"
        )
    return values, text


def _validate_git_lfs_inventory_lines(
    lines: list[str], expected_count: int, purpose: str
) -> None:
    if expected_count < 1 or len(lines) != expected_count:
        raise VerificationError(f"{purpose} has an invalid tracked-file count")
    observed_paths: set[str] = set()
    for line in lines:
        match = GIT_LFS_INVENTORY_RECORD.fullmatch(line)
        if match is None:
            raise VerificationError(f"{purpose} has a malformed inventory record")
        _, hydration_marker, tracked_path = match.groups()
        if hydration_marker != "*":
            raise VerificationError(f"{purpose} contains an unhydrated pointer stub")
        if tracked_path in observed_paths:
            raise VerificationError(f"{purpose} contains a duplicate tracked path")
        observed_paths.add(tracked_path)


def _git_lfs_hydration_evidence_text(
    inventory_lines: list[str], tracked_file_count: int
) -> str:
    return (
        "echoes_git_lfs_hydration_outcome=passed\n"
        f"echoes_git_lfs_tracked_file_count={tracked_file_count}\n"
        + "\n".join(inventory_lines)
        + "\n"
    )


def _validate_schema_2_app(metadata: dict[str, str], app: pathlib.Path) -> None:
    if app.name != "EchoesOfTheBrokenSun.app":
        raise VerificationError("schema 2 application name is invalid")

    plist_path = app / "Contents" / "Info.plist"
    try:
        with plist_path.open("rb") as handle:
            plist = plistlib.load(handle)
    except (OSError, plistlib.InvalidFileException) as exc:
        raise VerificationError("application Info.plist is missing or invalid") from exc
    expected_plist = {
        "CFBundleIdentifier": EXPECTED_BUNDLE_IDENTIFIER,
        "CFBundleExecutable": EXPECTED_EXECUTABLE,
        "CFBundlePackageType": "APPL",
        "CFBundleShortVersionString": metadata.get("bundle_short_version"),
        "CFBundleVersion": metadata.get("bundle_build_version"),
    }
    for key, expected in expected_plist.items():
        if not expected or plist.get(key) != expected:
            raise VerificationError(f"application Info.plist {key} is invalid")

    executable = app / "Contents" / "MacOS" / EXPECTED_EXECUTABLE
    if executable.is_symlink() or not executable.is_file():
        raise VerificationError("application executable is missing or is a symlink")
    if not os.access(executable, os.X_OK):
        raise VerificationError("application executable is not executable")
    if _sha256(executable) != metadata["application_executable_sha256"]:
        raise VerificationError("manifest executable digest changed")

    file_identity = _run_tool(
        ["/usr/bin/file", "--brief", str(executable)],
        "application architecture inspection",
    ).strip()
    if not file_identity.startswith("Mach-O 64-bit executable arm64"):
        raise VerificationError("application executable is not a native arm64 Mach-O")

    _run_tool(
        ["/usr/bin/codesign", "--verify", "--deep", "--strict", str(app)],
        "live application signature verification",
    )
    signature_display = _run_tool(
        ["/usr/bin/codesign", "--display", "--verbose=4", str(app)],
        "live application signature inspection",
    )
    for expected_line in (
        f"Identifier={EXPECTED_BUNDLE_IDENTIFIER}",
        "Signature=adhoc",
        "TeamIdentifier=not set",
    ):
        if expected_line not in signature_display.splitlines():
            raise VerificationError(f"live application signature omits {expected_line}")


def _validate_schema_2_evidence(
    metadata: dict[str, str], archive: pathlib.Path
) -> None:
    hydration_path = archive / metadata["git_lfs_hydration_evidence"]
    hydration_values, hydration_text = _structured_values(
        hydration_path,
        (
            "echoes_git_lfs_hydration_outcome",
            "echoes_git_lfs_tracked_file_count",
        ),
    )
    hydration_expected = {
        "echoes_git_lfs_hydration_outcome": metadata["git_lfs_hydration_outcome"],
        "echoes_git_lfs_tracked_file_count": metadata["git_lfs_tracked_file_count"],
    }
    if hydration_values != hydration_expected:
        raise VerificationError(
            "Git LFS hydration evidence contradicts manifest metadata"
        )
    hydration_lines = hydration_text.splitlines()
    expected_headers = [f"{key}={value}" for key, value in hydration_expected.items()]
    if hydration_lines[:2] != expected_headers:
        raise VerificationError("Git LFS hydration evidence headers are invalid")
    _validate_git_lfs_inventory_lines(
        hydration_lines[2:],
        int(metadata["git_lfs_tracked_file_count"]),
        "Git LFS hydration evidence",
    )

    outcome_contracts = (
        (
            "git_lfs_fsck_evidence",
            {"echoes_git_lfs_fsck_outcome": metadata["git_lfs_fsck_outcome"]},
        ),
        (
            "package_preflight_log",
            {
                "echoes_content_preflight_outcome": metadata[
                    "content_preflight_outcome"
                ],
                "echoes_environment_preflight_outcome": metadata[
                    "environment_preflight_outcome"
                ],
                "echoes_package_preflight_outcome": metadata[
                    "package_preflight_outcome"
                ],
            },
        ),
        (
            "build_log",
            {"echoes_build_cook_run_outcome": metadata["build_cook_run_outcome"]},
        ),
        (
            "normal_startup_smoke",
            {
                "echoes_normal_startup_smoke_outcome": metadata[
                    "normal_startup_smoke_outcome"
                ]
            },
        ),
        (
            "legacy_stress_startup_smoke",
            {
                "echoes_legacy_stress_startup_smoke_outcome": metadata[
                    "legacy_stress_startup_smoke_outcome"
                ]
            },
        ),
    )
    for evidence_key, expected_values in outcome_contracts:
        observed_values, _ = _structured_values(
            archive / metadata[evidence_key], tuple(expected_values)
        )
        if observed_values != expected_values:
            raise VerificationError(
                f"evidence {metadata[evidence_key]} reports a non-passing outcome"
            )

    signature_values, signature_text = _structured_values(
        archive / metadata["signature_evidence"],
        (
            "echoes_signature_class",
            "echoes_signature_team_identifier",
            "echoes_signature_verification",
        ),
    )
    signature_expected = {
        "echoes_signature_class": metadata["signature_class"],
        "echoes_signature_team_identifier": metadata["signature_team_identifier"],
        "echoes_signature_verification": metadata["signature_verification"],
    }
    if signature_values != signature_expected:
        raise VerificationError("signature evidence contradicts manifest metadata")
    signature_raw_expected = {
        "Identifier": EXPECTED_BUNDLE_IDENTIFIER,
        "Signature": "adhoc",
        "TeamIdentifier": "not set",
    }
    signature_lines = signature_text.splitlines()
    for key, expected in signature_raw_expected.items():
        observed = [
            line.split("=", 1)[1]
            for line in signature_lines
            if line.startswith(f"{key}=")
        ]
        if observed != [expected]:
            raise VerificationError(f"signature evidence has invalid {key}")

    gatekeeper_values, gatekeeper_text = _structured_values(
        archive / metadata["gatekeeper_evidence"],
        (
            "echoes_gatekeeper_policy",
            "echoes_gatekeeper_assessment",
            "echoes_gatekeeper_exit_code",
        ),
    )
    gatekeeper_expected = {
        "echoes_gatekeeper_policy": metadata["gatekeeper_policy"],
        "echoes_gatekeeper_assessment": metadata["gatekeeper_assessment"],
        "echoes_gatekeeper_exit_code": metadata["gatekeeper_exit_code"],
    }
    if gatekeeper_values != gatekeeper_expected:
        raise VerificationError("Gatekeeper evidence contradicts manifest metadata")
    policy = metadata["gatekeeper_policy"]
    assessment = metadata["gatekeeper_assessment"]
    gatekeeper_lower = gatekeeper_text.lower()
    if policy == "enabled" and "assessments enabled" not in gatekeeper_lower:
        raise VerificationError("Gatekeeper evidence does not show enabled policy")
    if policy == "disabled" and "assessments disabled" not in gatekeeper_lower:
        raise VerificationError("Gatekeeper evidence does not show disabled policy")
    if (
        assessment == "accepted"
        and re.search(r"(?m)^[^\n]*:\s*accepted\s*$", gatekeeper_lower) is None
    ):
        raise VerificationError("Gatekeeper evidence does not show acceptance")
    if (
        assessment == "rejected"
        and re.search(r"rejected|not accepted|no usable signature", gatekeeper_lower)
        is None
    ):
        raise VerificationError("Gatekeeper evidence does not show rejection")

    stapler_values, stapler_text = _structured_values(
        archive / metadata["stapler_evidence"],
        ("echoes_stapling_status", "echoes_stapler_exit_code"),
    )
    stapler_expected = {
        "echoes_stapling_status": metadata["stapling_status"],
        "echoes_stapler_exit_code": metadata["stapler_exit_code"],
    }
    if stapler_values != stapler_expected:
        raise VerificationError("stapler evidence contradicts manifest metadata")
    stapling_status = metadata["stapling_status"]
    stapler_lower = stapler_text.lower()
    if stapling_status == "validated" and "validate action worked" not in stapler_lower:
        raise VerificationError("stapler evidence does not show validation")
    if (
        stapling_status == "not-stapled"
        and re.search(
            r"does not have a ticket|could not validate|not stapled", stapler_lower
        )
        is None
    ):
        raise VerificationError("stapler evidence does not show an absent ticket")

    toolchain_keys = (
        "unreal_root",
        "unreal_engine",
        "unreal_changelist",
        "unreal_branch",
        "unreal_promoted",
        "engine_build_file_sha256",
        "uat_sha256",
        "uat_driver_sha256",
        "git_path",
        "git_resolved_path",
        "git_version",
        "git_sha256",
        "git_lfs_path",
        "git_lfs_resolved_path",
        "git_lfs_version",
        "git_lfs_sha256",
        "developer_dir",
        "xcode",
        "macos_sdk_version",
        "macos_sdk_path",
        "clang_path",
        "clang_version",
        "clang_sha256",
        "metal_path",
        "metal_version",
        "metal_sha256",
    )
    toolchain_values, _ = _structured_values(
        archive / metadata["toolchain_evidence"], toolchain_keys
    )
    if toolchain_values != {key: metadata[key] for key in toolchain_keys}:
        raise VerificationError("toolchain evidence contradicts manifest metadata")

    generated_content_pack = archive / metadata["generated_content_pack"]
    try:
        generated_content = json.loads(
            generated_content_pack.read_text(encoding="utf-8")
        )
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise VerificationError(
            "generated cooked content evidence is invalid JSON"
        ) from exc
    if not isinstance(generated_content, dict):
        raise VerificationError(
            "generated cooked content evidence is not a JSON object"
        )
    generated_digest = archive / metadata["generated_content_pack_digest"]
    try:
        declared_digest = generated_digest.read_text(encoding="ascii")
    except (OSError, UnicodeDecodeError) as exc:
        raise VerificationError(
            "generated cooked content digest evidence is unreadable"
        ) from exc
    if declared_digest != f"{metadata['generated_content_pack_sha256']}\n":
        raise VerificationError(
            "generated cooked content digest contradicts the retained input"
        )


def _validate_live_build_context(metadata: dict[str, str]) -> None:
    git_environment = _sanitized_git_environment()
    source_checkout_record = pathlib.Path(metadata["source_checkout_path"])
    if not source_checkout_record.is_dir():
        raise VerificationError("live source checkout is unavailable")
    try:
        source_checkout = source_checkout_record.resolve(strict=True)
    except (OSError, RuntimeError) as exc:
        raise VerificationError("live source checkout cannot be resolved") from exc
    if str(source_checkout) != metadata["source_checkout_path"]:
        raise VerificationError("live source checkout path is not canonical")

    git_path = pathlib.Path(metadata["git_path"])
    git_lfs_path = pathlib.Path(metadata["git_lfs_path"])
    if (
        not git_path.is_file()
        or not git_lfs_path.is_file()
        or str(git_path.resolve(strict=True)) != metadata["git_resolved_path"]
        or str(git_lfs_path.resolve(strict=True)) != metadata["git_lfs_resolved_path"]
    ):
        raise VerificationError("live Git or Git LFS path identity changed")
    if _sha256(git_path) != metadata["git_sha256"]:
        raise VerificationError("live Git executable digest changed")
    if _sha256(git_lfs_path) != metadata["git_lfs_sha256"]:
        raise VerificationError("live Git LFS executable digest changed")
    git_execution_path = pathlib.Path(metadata["git_resolved_path"])
    git_lfs_execution_path = pathlib.Path(metadata["git_lfs_resolved_path"])
    if (
        _run_tool(
            [str(git_execution_path), "--version"],
            "live Git version",
            environment=git_environment,
        ).strip()
        != metadata["git_version"]
    ):
        raise VerificationError("live Git version changed")
    if (
        _run_tool(
            [str(git_lfs_execution_path), "version"],
            "live Git LFS version",
            environment=git_environment,
        ).strip()
        != metadata["git_lfs_version"]
    ):
        raise VerificationError("live Git LFS version changed")

    def git_output(*arguments: str) -> str:
        return _run_tool(
            [str(git_execution_path), "-C", str(source_checkout), *arguments],
            f"live Git {' '.join(arguments)}",
            environment=git_environment,
        ).strip()

    observed_top_level_record = git_output(
        "rev-parse", "--path-format=absolute", "--show-toplevel"
    )
    try:
        observed_top_level = pathlib.Path(observed_top_level_record).resolve(
            strict=True
        )
    except (OSError, RuntimeError) as exc:
        raise VerificationError(
            "live Git checkout top level cannot be resolved"
        ) from exc
    if observed_top_level != source_checkout:
        raise VerificationError(
            "live Git checkout top level differs from the intended source checkout"
        )

    observed_source = {
        "source_commit": git_output("rev-parse", "--verify", "HEAD"),
        "source_tree_hash": git_output("rev-parse", "--verify", "HEAD^{tree}"),
        "source_branch": (
            "detached"
            if git_output("rev-parse", "--abbrev-ref", "HEAD") == "HEAD"
            else git_output("rev-parse", "--abbrev-ref", "HEAD")
        ),
        "origin_main": git_output("rev-parse", "--verify", "origin/main"),
        "source_origin_fetch_url": git_output("remote", "get-url", "origin"),
        "source_origin_push_url": git_output("remote", "get-url", "--push", "origin"),
    }
    for key, observed in observed_source.items():
        if observed != metadata[key]:
            raise VerificationError(f"live source context changed: {key}")
    git_directory = pathlib.Path(
        git_output("rev-parse", "--path-format=absolute", "--git-dir")
    ).resolve(strict=True)
    git_common_directory = pathlib.Path(
        git_output("rev-parse", "--path-format=absolute", "--git-common-dir")
    ).resolve(strict=True)
    if git_directory == git_common_directory:
        raise VerificationError(
            "live source checkout is not a dedicated linked worktree"
        )
    for tag_mode in ("-v", "-f"):
        index_records = git_output("ls-files", tag_mode).splitlines()
        if any(not record.startswith("H ") for record in index_records):
            raise VerificationError("live source checkout has concealed index state")
    if git_output("status", "--porcelain", "--untracked-files=normal"):
        raise VerificationError("live source checkout is not clean")

    remote_record = _run_tool(
        [
            str(git_execution_path),
            "-C",
            str(source_checkout),
            "ls-remote",
            "--exit-code",
            EXPECTED_ORIGIN_URL,
            "refs/heads/main",
        ],
        "live remote main observation",
        environment=git_environment,
        timeout_seconds=60,
    ).strip()
    remote_parts = remote_record.split()
    if len(remote_parts) != 2 or remote_parts[0] != metadata["remote_main"]:
        raise VerificationError("live remote main changed")

    lfs_environment = git_environment.copy()
    lfs_environment["PATH"] = (
        f"{git_execution_path.parent}:{git_lfs_execution_path.parent}:"
        f"{lfs_environment.get('PATH', '')}"
    )
    live_lfs_environment = _run_tool(
        [str(git_lfs_execution_path), "env"],
        "live Git LFS effective configuration",
        cwd=source_checkout,
        environment=lfs_environment,
    ).splitlines()
    restrictive_lfs_config = [
        line
        for line in live_lfs_environment
        if line.startswith(("FetchInclude=", "FetchExclude="))
    ]
    if restrictive_lfs_config:
        raise VerificationError(
            "live Git LFS context has restrictive fetch configuration"
        )
    if _run_tool(
        [str(git_lfs_execution_path), "status", "--porcelain"],
        "live Git LFS status",
        cwd=source_checkout,
        environment=lfs_environment,
    ).strip():
        raise VerificationError("live Git LFS status is not clean")
    _run_tool(
        [str(git_lfs_execution_path), "fsck", "--objects", "--pointers"],
        "live Git LFS fsck",
        cwd=source_checkout,
        environment=lfs_environment,
    )
    live_lfs_inventory = _run_tool(
        [str(git_lfs_execution_path), "ls-files", "-l"],
        "live Git LFS hydration inventory",
        cwd=source_checkout,
        environment=lfs_environment,
    ).splitlines()
    tracked_file_count = int(metadata["git_lfs_tracked_file_count"])
    _validate_git_lfs_inventory_lines(
        live_lfs_inventory,
        tracked_file_count,
        "live Git LFS hydration inventory",
    )
    live_hydration_evidence = _git_lfs_hydration_evidence_text(
        live_lfs_inventory, tracked_file_count
    ).encode("utf-8")
    if (
        hashlib.sha256(live_hydration_evidence).hexdigest()
        != metadata["git_lfs_hydration_sha256"]
    ):
        raise VerificationError("live Git LFS hydration inventory changed")

    live_files = (
        (
            pathlib.Path(EXPECTED_UNREAL_ROOT) / "Engine/Build/Build.version",
            "engine_build_file_sha256",
        ),
        (
            pathlib.Path(EXPECTED_UNREAL_ROOT)
            / "Engine/Build/BatchFiles/RunUAT.command",
            "uat_sha256",
        ),
        (
            pathlib.Path(EXPECTED_UNREAL_ROOT) / "Engine/Build/BatchFiles/RunUAT.sh",
            "uat_driver_sha256",
        ),
        (pathlib.Path(metadata["clang_path"]), "clang_sha256"),
        (pathlib.Path(metadata["metal_path"]), "metal_sha256"),
        (
            source_checkout / "EchoesOfTheBrokenSun.uproject",
            "project_file_sha256",
        ),
        (source_checkout / "Scripts/package_macos.sh", "packager_sha256"),
        (
            source_checkout / "Scripts/verify_packaged_app.py",
            "package_verifier_sha256",
        ),
    )
    for live_path, digest_key in live_files:
        if not live_path.is_file() or _sha256(live_path) != metadata[digest_key]:
            raise VerificationError(f"live build-context file changed: {digest_key}")

    developer_environment = {
        name: value
        for name, value in os.environ.items()
        if not name.startswith(GIT_ENVIRONMENT_PREFIX)
    }
    developer_environment["DEVELOPER_DIR"] = EXPECTED_DEVELOPER_DIR
    _run_tool(
        ["/usr/bin/xcodebuild", "-checkFirstLaunchStatus"],
        "live Xcode first-launch status",
        environment=developer_environment,
    )
    xcode_lines = _run_tool(
        ["/usr/bin/xcodebuild", "-version"],
        "live Xcode version",
        environment=developer_environment,
    ).splitlines()
    if ";".join(xcode_lines) != metadata["xcode"]:
        raise VerificationError("live Xcode version changed")
    live_sdk_path = _run_tool(
        ["/usr/bin/xcrun", "--sdk", "macosx", "--show-sdk-path"],
        "live macOS SDK path",
        environment=developer_environment,
    ).strip()
    live_sdk_version = _run_tool(
        ["/usr/bin/xcrun", "--sdk", "macosx", "--show-sdk-version"],
        "live macOS SDK version",
        environment=developer_environment,
    ).strip()
    if (
        live_sdk_path != metadata["macos_sdk_path"]
        or live_sdk_version != metadata["macos_sdk_version"]
    ):
        raise VerificationError("live macOS SDK identity changed")
    for tool_name, path_key, version_key in (
        ("clang", "clang_path", "clang_version"),
        ("metal", "metal_path", "metal_version"),
    ):
        observed_path = _run_tool(
            ["/usr/bin/xcrun", "--find", tool_name],
            f"live {tool_name} path",
            environment=developer_environment,
        ).strip()
        if observed_path != metadata[path_key]:
            raise VerificationError(f"live {tool_name} path changed")
        observed_version_lines = _run_tool(
            ["/usr/bin/xcrun", tool_name, "--version"],
            f"live {tool_name} version",
            environment=developer_environment,
        ).splitlines()
        if (
            not observed_version_lines
            or observed_version_lines[0] != metadata[version_key]
        ):
            raise VerificationError(f"live {tool_name} version changed")


def _validate_schema_2_metadata(
    metadata: dict[str, str],
    app: pathlib.Path,
    archive: pathlib.Path,
) -> list[tuple[str, str]]:
    for key, expected in SCHEMA_2_EXACT_METADATA.items():
        if metadata.get(key) != expected:
            raise VerificationError(f"manifest metadata {key} is missing or invalid")

    for key in ("source_commit", "source_tree_hash", "origin_main", "remote_main"):
        _require_sha(metadata, key, SHA40)
    for key in (
        "source_status_sha256",
        "repo_local_ignored_state_sha256",
        "source_index_concealment_sha256",
        "git_lfs_status_sha256",
        "git_lfs_fsck_sha256",
        "git_lfs_restrictive_config_sha256",
        "git_lfs_hydration_sha256",
        "package_preflight_log_sha256",
        "build_log_sha256",
        "generated_content_pack_sha256",
        "generated_content_pack_digest_sha256",
        "normal_startup_smoke_sha256",
        "legacy_stress_startup_smoke_sha256",
        "engine_build_file_sha256",
        "uat_sha256",
        "uat_driver_sha256",
        "git_sha256",
        "git_lfs_sha256",
        "toolchain_evidence_sha256",
        "clang_sha256",
        "metal_sha256",
        "project_file_sha256",
        "packager_sha256",
        "package_verifier_sha256",
        "application_executable_sha256",
        "signature_evidence_sha256",
        "gatekeeper_evidence_sha256",
        "stapler_evidence_sha256",
    ):
        _require_sha(metadata, key, SHA256)

    if metadata["source_status_sha256"] != EMPTY_SHA256:
        raise VerificationError(
            "source-status evidence is not the empty clean-tree digest"
        )
    if metadata["git_lfs_status_sha256"] != EMPTY_SHA256:
        raise VerificationError("Git LFS status evidence is not empty")
    if metadata["git_lfs_restrictive_config_sha256"] != EMPTY_SHA256:
        raise VerificationError("Git LFS restrictive-config evidence is not empty")
    if metadata["repo_local_ignored_state_sha256"] != EMPTY_SHA256:
        raise VerificationError("repo-local ignored-state evidence is not empty")
    if metadata["source_index_concealment_sha256"] != EMPTY_SHA256:
        raise VerificationError("source-index concealment evidence is not empty")
    if not re.fullmatch(
        r"[A-Za-z0-9._/-]+|detached", metadata.get("source_branch", "")
    ):
        raise VerificationError("manifest source branch is missing or malformed")
    if re.fullmatch(r"[0-9]{8}T[0-9]{6}Z", metadata.get("created_utc", "")) is None:
        raise VerificationError("manifest creation timestamp is missing or malformed")
    if _require_nonnegative_integer(metadata, "git_lfs_tracked_file_count") < 1:
        raise VerificationError("manifest Git LFS tracked-file count is empty")

    archive_path = pathlib.Path(metadata.get("archive_path", ""))
    checkout_path = pathlib.PurePosixPath(metadata.get("source_checkout_path", ""))
    if not archive_path.is_absolute() or not checkout_path.is_absolute():
        raise VerificationError(
            "manifest archive and source-checkout paths must be absolute"
        )
    try:
        archive_path.relative_to(checkout_path)
    except ValueError:
        pass
    else:
        raise VerificationError("manifest archive path is inside the source checkout")
    if archive_path != archive:
        raise VerificationError(
            "manifest archive path differs from its physical archive"
        )
    if app.parent != archive:
        raise VerificationError("application is outside its manifested archive")
    for key in ("archive_free_gib_before", "internal_free_gib_before"):
        if _require_nonnegative_integer(metadata, key) < 60:
            raise VerificationError(
                f"manifest metadata {key} is below the package floor"
            )

    try:
        build_argv = json.loads(metadata.get("build_command_argv", ""))
    except json.JSONDecodeError as exc:
        raise VerificationError("manifest build command is not valid JSON") from exc
    if (
        not isinstance(build_argv, list)
        or not build_argv
        or not all(isinstance(argument, str) and argument for argument in build_argv)
        or build_argv[0]
        != f"{EXPECTED_UNREAL_ROOT}/Engine/Build/BatchFiles/RunUAT.command"
        or build_argv[1:2] != ["BuildCookRun"]
    ):
        raise VerificationError("manifest build command argv is malformed")
    expected_project_argument = (
        f"-project={metadata['source_checkout_path']}/EchoesOfTheBrokenSun.uproject"
    )
    expected_archive_argument = f"-archivedirectory={metadata['archive_path']}"
    parallel_arguments = [
        argument
        for argument in build_argv
        if argument.startswith("-ubtargs=-MaxParallelActions=")
    ]
    if (
        len(parallel_arguments) != 1
        or re.fullmatch(
            r"-ubtargs=-MaxParallelActions=[1-9][0-9]*", parallel_arguments[0]
        )
        is None
    ):
        raise VerificationError("manifest build command parallelism is invalid")
    expected_build_argv = [
        f"{EXPECTED_UNREAL_ROOT}/Engine/Build/BatchFiles/RunUAT.command",
        "BuildCookRun",
        expected_project_argument,
        "-noP4",
        "-platform=Mac",
        "-target=EchoesOfTheBrokenSun",
        "-clientconfig=Development",
        parallel_arguments[0],
        "-build",
        "-cook",
        "-stage",
        "-pak",
        "-package",
        "-archive",
        expected_archive_argument,
        "-utf8output",
    ]
    if build_argv != expected_build_argv:
        if (
            any(argument.startswith("-project=") for argument in build_argv)
            and expected_project_argument not in build_argv
        ):
            raise VerificationError("manifest build command project path disagrees")
        if (
            any(argument.startswith("-archivedirectory=") for argument in build_argv)
            and expected_archive_argument not in build_argv
        ):
            raise VerificationError("manifest build command archive path disagrees")
        raise VerificationError(
            "manifest build command differs from the approved command"
        )

    for key in (
        "unreal_changelist",
        "unreal_branch",
        "git_path",
        "git_resolved_path",
        "git_version",
        "git_lfs_path",
        "git_lfs_resolved_path",
        "xcode",
        "macos_sdk_version",
        "macos_sdk_path",
        "clang_path",
        "clang_version",
        "metal_path",
        "metal_version",
        "host_os_version",
        "host_os_build",
        "host_model",
        "bundle_short_version",
        "bundle_build_version",
    ):
        if not metadata.get(key):
            raise VerificationError(f"manifest metadata {key} is missing")
    for key in (
        "git_path",
        "git_resolved_path",
        "git_lfs_path",
        "git_lfs_resolved_path",
        "macos_sdk_path",
        "clang_path",
        "metal_path",
    ):
        if not pathlib.PurePosixPath(metadata[key]).is_absolute():
            raise VerificationError(f"manifest tool path {key} is not absolute")
    for key in ("clang_path", "macos_sdk_path"):
        try:
            pathlib.PurePosixPath(metadata[key]).relative_to(EXPECTED_DEVELOPER_DIR)
        except ValueError as exc:
            raise VerificationError(
                f"manifest tool path {key} is outside the selected Xcode"
            ) from exc

    gatekeeper_assessment = metadata.get("gatekeeper_assessment")
    if gatekeeper_assessment not in {
        "accepted",
        "rejected",
        "assessment-error",
        "not-enforced",
    }:
        raise VerificationError("manifest Gatekeeper assessment is invalid")
    if metadata.get("gatekeeper_policy") not in {"enabled", "disabled", "unknown"}:
        raise VerificationError("manifest Gatekeeper policy is invalid")
    gatekeeper_exit = _require_nonnegative_integer(metadata, "gatekeeper_exit_code")
    gatekeeper_policy = metadata["gatekeeper_policy"]
    if gatekeeper_assessment == "accepted" and not (
        gatekeeper_policy == "enabled" and gatekeeper_exit == 0
    ):
        raise VerificationError("Gatekeeper acceptance is not policy-enforced")
    if gatekeeper_assessment == "not-enforced" and gatekeeper_policy != "disabled":
        raise VerificationError("Gatekeeper non-enforcement and policy disagree")
    if gatekeeper_policy == "disabled" and gatekeeper_assessment != "not-enforced":
        raise VerificationError("disabled Gatekeeper policy is overstated")
    if (
        gatekeeper_assessment in {"rejected", "assessment-error"}
        and gatekeeper_exit == 0
    ):
        raise VerificationError("Gatekeeper assessment and exit code disagree")

    stapling_status = metadata.get("stapling_status")
    if stapling_status not in {"validated", "not-stapled", "validation-error"}:
        raise VerificationError("manifest stapling status is invalid")
    stapler_exit = _require_nonnegative_integer(metadata, "stapler_exit_code")
    if (stapling_status == "validated") != (stapler_exit == 0):
        raise VerificationError("stapling status and exit code disagree")

    _validate_schema_2_app(metadata, app)

    evidence_records: list[tuple[str, str]] = []
    for name_key, digest_key, expected_name in SCHEMA_2_EVIDENCE:
        if metadata.get(name_key) != expected_name:
            raise VerificationError(f"manifest evidence name {name_key} is invalid")
        evidence_records.append((expected_name, metadata[digest_key]))
    return evidence_records


def verify_package(
    app_path: pathlib.Path,
    manifest_path: pathlib.Path,
    digest_path: pathlib.Path,
    *,
    require_live_build_context: bool = False,
) -> dict[str, object]:
    app = app_path.resolve()
    manifest = manifest_path.resolve()
    sidecar = digest_path.resolve()
    contents = app / "Contents"
    if not contents.is_dir() or not manifest.is_file() or not sidecar.is_file():
        raise VerificationError("package, manifest, or manifest digest is missing")

    sidecar_lines = sidecar.read_text(encoding="utf-8").splitlines()
    if len(sidecar_lines) != 1:
        raise VerificationError(
            "manifest digest sidecar must contain exactly one record"
        )
    sidecar_match = SIDECAR_RECORD.fullmatch(sidecar_lines[0])
    if sidecar_match is None or sidecar_match.group(2) != manifest.name:
        raise VerificationError("manifest digest sidecar has an invalid target")
    manifest_sha256 = _sha256(manifest)
    if sidecar_match.group(1) != manifest_sha256:
        raise VerificationError("manifest digest does not match the manifest")

    lines = manifest.read_text(encoding="utf-8").splitlines()
    try:
        record_index = lines.index("sha256  relative_path")
    except ValueError as exc:
        raise VerificationError("manifest record header is missing") from exc
    metadata: dict[str, str] = {}
    for line in lines[:record_index]:
        if not line:
            continue
        if "=" not in line:
            raise VerificationError(f"malformed manifest metadata: {line!r}")
        key, value = line.split("=", 1)
        if not key or not value or key in metadata:
            raise VerificationError(f"invalid or duplicate manifest metadata: {line!r}")
        metadata[key] = value

    required_metadata = {
        "artifact": "EchoesOfTheBrokenSun.app",
        "source_tree": "clean",
        "source_binding": "clean-pushed-main",
        "configuration": "Development",
        "platform": "Mac-arm64",
        "normal_startup_smoke": "EchoesOfTheBrokenSun.normal-startup-smoke.log",
        "legacy_stress_startup_smoke": "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log",
    }
    for key, expected in required_metadata.items():
        if metadata.get(key) != expected:
            raise VerificationError(f"manifest metadata {key} is missing or invalid")
    for key in ("source_commit", "origin_main", "remote_main"):
        _require_sha(metadata, key, SHA40)
    for key in ("normal_startup_smoke_sha256", "legacy_stress_startup_smoke_sha256"):
        _require_sha(metadata, key, SHA256)
    if not (
        metadata["source_commit"] == metadata["origin_main"] == metadata["remote_main"]
    ):
        raise VerificationError("manifest source refs are not identical")

    file_records: dict[str, str] = {}
    link_records: dict[str, str] = {}
    for line in lines[record_index + 1 :]:
        file_match = HASH_RECORD.fullmatch(line)
        link_match = LINK_RECORD.fullmatch(line)
        if file_match is not None:
            digest, relative = file_match.groups()
            if relative in file_records or relative in link_records:
                raise VerificationError(f"duplicate manifest path: {relative}")
            file_records[relative] = digest
        elif link_match is not None:
            relative, target = link_match.groups()
            if relative in file_records or relative in link_records or not target:
                raise VerificationError(
                    f"duplicate or malformed symlink path: {relative}"
                )
            link_records[relative] = target
        else:
            raise VerificationError(f"malformed manifest record: {line!r}")

    actual_files, actual_links = _bundle_entries(contents, app)
    if set(file_records) != actual_files:
        raise VerificationError("manifest file path set differs from the application")
    if set(link_records) != actual_links:
        raise VerificationError(
            "manifest symlink path set differs from the application"
        )
    for relative, expected_digest in file_records.items():
        if _sha256(app / relative) != expected_digest:
            raise VerificationError(f"manifested file digest changed: {relative}")

    for relative, expected_target in link_records.items():
        link = app / relative
        if os.readlink(link) != expected_target:
            raise VerificationError(f"manifested symlink target changed: {relative}")
        if pathlib.Path(expected_target).is_absolute():
            raise VerificationError(
                f"manifested symlink escapes the app lexically: {relative}"
            )
        lexical_target = pathlib.Path(
            os.path.normpath(os.path.join(link.parent, expected_target))
        )
        try:
            lexical_target.relative_to(app)
        except ValueError as exc:
            raise VerificationError(
                f"manifested symlink escapes the app lexically: {relative}"
            ) from exc
        try:
            resolved = link.resolve(strict=True)
            resolved.relative_to(app)
        except (FileNotFoundError, RuntimeError, ValueError) as exc:
            raise VerificationError(
                f"manifested symlink escapes or does not resolve inside the app: {relative}"
            ) from exc

    manifest_schema = metadata.get("manifest_schema", "1")
    archive = manifest.parent
    live_current_source_toolchain_cross_checked = False
    if manifest_schema == "1":
        if require_live_build_context:
            raise VerificationError(
                "schema 1 cannot satisfy a live build-context cross-check"
            )
        evidence_records = [
            (metadata["normal_startup_smoke"], metadata["normal_startup_smoke_sha256"]),
            (
                metadata["legacy_stress_startup_smoke"],
                metadata["legacy_stress_startup_smoke_sha256"],
            ),
        ]
    elif manifest_schema == "2":
        evidence_records = _validate_schema_2_metadata(metadata, app, archive)
    else:
        raise VerificationError("manifest schema is unsupported")

    for evidence_name, expected_digest in evidence_records:
        evidence_path = archive / evidence_name
        if not evidence_path.is_file() or _sha256(evidence_path) != expected_digest:
            raise VerificationError(f"package evidence changed: {evidence_name}")

    if manifest_schema == "2":
        if (archive / metadata["source_status_evidence"]).stat().st_size != 0:
            raise VerificationError("source-status evidence is not empty")
        if (
            archive / metadata["repo_local_ignored_state_evidence"]
        ).stat().st_size != 0:
            raise VerificationError("repo-local ignored-state evidence is not empty")
        if (
            archive / metadata["source_index_concealment_evidence"]
        ).stat().st_size != 0:
            raise VerificationError("source-index concealment evidence is not empty")
        if (archive / metadata["git_lfs_status_evidence"]).stat().st_size != 0:
            raise VerificationError("Git LFS status evidence is not empty")
        if (
            archive / metadata["git_lfs_restrictive_config_evidence"]
        ).stat().st_size != 0:
            raise VerificationError("Git LFS restrictive-config evidence is not empty")
        _validate_schema_2_evidence(metadata, archive)
        if require_live_build_context:
            _validate_live_build_context(metadata)
            live_current_source_toolchain_cross_checked = True

    if manifest_schema == "1":
        acceptance_scope = "package-manifest-integrity-only"
    elif live_current_source_toolchain_cross_checked:
        acceptance_scope = (
            "package-integrity-evidence-semantics-record-self-consistency-"
            "and-live-current-source-toolchain"
        )
    else:
        acceptance_scope = (
            "package-integrity-evidence-semantics-and-record-self-consistency"
        )

    return {
        "accepted": True,
        "acceptance_scope": acceptance_scope,
        "release_qualified": False,
        "artifact_integrity_validated": True,
        "evidence_semantics_validated": manifest_schema == "2",
        "recorded_build_context_self_consistent": manifest_schema == "2",
        "live_current_source_toolchain_cross_checked": (
            live_current_source_toolchain_cross_checked
        ),
        "historical_build_context_independently_attested": False,
        "independent_attestation_present": False,
        "application_files": len(actual_files),
        "application_symlinks": len(actual_links),
        "manifest_schema": int(manifest_schema),
        "manifest_sha256": manifest_sha256,
        "source_commit": metadata["source_commit"],
        "normal_startup_smoke_sha256": metadata["normal_startup_smoke_sha256"],
        "legacy_stress_startup_smoke_sha256": metadata[
            "legacy_stress_startup_smoke_sha256"
        ],
        "recorded_manifest_metadata": metadata,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", type=pathlib.Path, required=True)
    parser.add_argument("--manifest", type=pathlib.Path, required=True)
    parser.add_argument("--manifest-digest", type=pathlib.Path, required=True)
    parser.add_argument("--require-live-build-context", action="store_true")
    parser.add_argument("--json-output", type=pathlib.Path)
    args = parser.parse_args()
    try:
        result = verify_package(
            args.app,
            args.manifest,
            args.manifest_digest,
            require_live_build_context=args.require_live_build_context,
        )
    except (OSError, VerificationError) as exc:
        print(f"package verification failed: {exc}")
        return 1
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json_output is not None:
        args.json_output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
