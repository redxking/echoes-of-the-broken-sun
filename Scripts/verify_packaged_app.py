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
EMPTY_SHA256 = hashlib.sha256(b"").hexdigest()
EXPECTED_BUNDLE_IDENTIFIER = "com.angelispseftis.echoesofthebrokensun"
EXPECTED_EXECUTABLE = "EchoesOfTheBrokenSun"
EXPECTED_UNREAL_ROOT = "/Users/Shared/Epic Games/UE_5.8"
EXPECTED_DEVELOPER_DIR = "/Applications/Xcode.app/Contents/Developer"
GENERATED_CONTENT_PACK_SOURCE = "Content/Data/Generated/EchoesContentPack.json"
GENERATED_CONTENT_PACK_DIGEST_SOURCE = f"{GENERATED_CONTENT_PACK_SOURCE}.sha256"

SCHEMA_2_EXACT_METADATA = {
    "manifest_schema": "2",
    "source_tree": "clean",
    "source_binding": "clean-pushed-main",
    "source_upstream_ref": "origin/main",
    "git_lfs_status": "clean",
    "git_lfs_fsck": "passed",
    "configuration": "Development",
    "platform": "Mac-arm64",
    "architecture": "arm64",
    "archive_outside_checkout": "true",
    "host_arch": "arm64",
    "unreal_engine": "5.8.2",
    "unreal_root": EXPECTED_UNREAL_ROOT,
    "unreal_promoted": "1",
    "developer_dir": EXPECTED_DEVELOPER_DIR,
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
    "claim_boundary": "local-development-package-only",
}

SCHEMA_2_EVIDENCE = (
    (
        "source_status_evidence",
        "source_status_sha256",
        "EchoesOfTheBrokenSun.source-status.porcelain-v2-z",
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


def _run_tool(arguments: list[str], purpose: str) -> str:
    try:
        completed = subprocess.run(
            arguments,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as exc:
        raise VerificationError(f"{purpose} could not run") from exc
    output = completed.stdout + completed.stderr
    if completed.returncode != 0:
        raise VerificationError(f"{purpose} failed: {output.strip()}")
    return output


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
        "git_version",
        "git_sha256",
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
        "git_lfs_status_sha256",
        "git_lfs_fsck_sha256",
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
    if not re.fullmatch(
        r"[A-Za-z0-9._/-]+|detached", metadata.get("source_branch", "")
    ):
        raise VerificationError("manifest source branch is missing or malformed")
    if not metadata.get("git_lfs_version", "").startswith("git-lfs/"):
        raise VerificationError("manifest Git LFS version is missing or malformed")
    if re.fullmatch(r"[0-9]{8}T[0-9]{6}Z", metadata.get("created_utc", "")) is None:
        raise VerificationError("manifest creation timestamp is missing or malformed")

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
        "git_version",
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
    for key in ("git_path", "macos_sdk_path", "clang_path", "metal_path"):
        if not pathlib.PurePosixPath(metadata[key]).is_absolute():
            raise VerificationError(f"manifest tool path {key} is not absolute")
    for key in ("clang_path", "metal_path", "macos_sdk_path"):
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
    if manifest_schema == "1":
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
        if (archive / metadata["git_lfs_status_evidence"]).stat().st_size != 0:
            raise VerificationError("Git LFS status evidence is not empty")
        _validate_schema_2_evidence(metadata, archive)

    return {
        "accepted": True,
        "acceptance_scope": "package-manifest-integrity-only",
        "release_qualified": False,
        "application_files": len(actual_files),
        "application_symlinks": len(actual_links),
        "manifest_schema": int(manifest_schema),
        "schema_2_provenance_validated": manifest_schema == "2",
        "manifest_sha256": manifest_sha256,
        "source_commit": metadata["source_commit"],
        "normal_startup_smoke_sha256": metadata["normal_startup_smoke_sha256"],
        "legacy_stress_startup_smoke_sha256": metadata[
            "legacy_stress_startup_smoke_sha256"
        ],
        "provenance": metadata,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", type=pathlib.Path, required=True)
    parser.add_argument("--manifest", type=pathlib.Path, required=True)
    parser.add_argument("--manifest-digest", type=pathlib.Path, required=True)
    parser.add_argument("--json-output", type=pathlib.Path)
    args = parser.parse_args()
    try:
        result = verify_package(args.app, args.manifest, args.manifest_digest)
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
