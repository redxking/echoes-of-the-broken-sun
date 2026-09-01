#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import os
import pathlib
import plistlib
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT_ROOT / "Scripts"))
PACKAGER = PROJECT_ROOT / "Scripts" / "package_macos.sh"

from verify_packaged_app import (
    CONTROLLED_GIT_ENVIRONMENT,
    GIT_ENVIRONMENT_POLICY,
    VerificationError,
    _sanitized_git_environment,
    verify_package,
)

REPOSITORY_SHAPING_GIT_VARIABLES = (
    "GIT_DIR",
    "GIT_WORK_TREE",
    "GIT_COMMON_DIR",
    "GIT_INDEX_FILE",
    "GIT_INDEX_VERSION",
    "GIT_OBJECT_DIRECTORY",
    "GIT_ALTERNATE_OBJECT_DIRECTORIES",
    "GIT_NAMESPACE",
    "GIT_REPLACE_REF_BASE",
    "GIT_NO_REPLACE_OBJECTS",
    "GIT_CEILING_DIRECTORIES",
    "GIT_DISCOVERY_ACROSS_FILESYSTEM",
    "GIT_CONFIG_GLOBAL",
    "GIT_CONFIG_SYSTEM",
    "GIT_CONFIG_NOSYSTEM",
    "GIT_CONFIG_PARAMETERS",
    "GIT_CONFIG_COUNT",
    "GIT_CONFIG_KEY_0",
    "GIT_CONFIG_VALUE_0",
    "GIT_ATTR_NOSYSTEM",
    "GIT_EXEC_PATH",
    "GIT_SSH",
    "GIT_SSH_COMMAND",
    "GIT_PROXY_COMMAND",
    "GIT_ASKPASS",
    "GIT_TERMINAL_PROMPT",
    "GIT_ALLOW_PROTOCOL",
    "GIT_PROTOCOL_FROM_USER",
    "GIT_LFS_SKIP_SMUDGE",
    "GIT_LFS_FUTURE_SWITCH",
    "GIT_PAGER",
    "GIT_FUTURE_UNDOCUMENTED_SWITCH",
)


def digest(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def refresh_sidecar(manifest: pathlib.Path, sidecar: pathlib.Path) -> None:
    sidecar.write_text(f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8")


def environment_without_git_overrides() -> dict[str, str]:
    return {
        name: value for name, value in os.environ.items() if not name.startswith("GIT_")
    }


def replace_manifest_metadata(
    manifest: pathlib.Path,
    sidecar: pathlib.Path,
    updates: dict[str, str],
) -> None:
    lines = manifest.read_text(encoding="utf-8").splitlines()
    remaining = set(updates)
    for index, line in enumerate(lines):
        if line == "sha256  relative_path":
            break
        if "=" not in line:
            continue
        key, _ = line.split("=", 1)
        if key in updates:
            lines[index] = f"{key}={updates[key]}"
            remaining.remove(key)
    if remaining:
        raise AssertionError(
            f"fixture manifest omits metadata: {', '.join(sorted(remaining))}"
        )
    manifest.write_text("\n".join(lines) + "\n", encoding="utf-8")
    refresh_sidecar(manifest, sidecar)


class PackageManifestVerifierTests(unittest.TestCase):
    @staticmethod
    def run_packager_with_git_environment(
        variable_name: str, value: str = "/tmp/redirected-git-context"
    ) -> subprocess.CompletedProcess[str]:
        environment = environment_without_git_overrides()
        environment[variable_name] = value
        return subprocess.run(
            [str(PACKAGER)],
            cwd=PROJECT_ROOT,
            env=environment,
            check=False,
            capture_output=True,
            text=True,
        )

    @staticmethod
    def resign_and_refresh_records(
        app: pathlib.Path,
        manifest: pathlib.Path,
        sidecar: pathlib.Path,
    ) -> None:
        subprocess.run(
            ["/usr/bin/codesign", "--force", "--deep", "--sign", "-", str(app)],
            check=True,
            capture_output=True,
            text=True,
        )
        text = manifest.read_text(encoding="utf-8")
        metadata_text = text.split("sha256  relative_path", 1)[0]
        records: list[str] = []
        contents = app / "Contents"
        for directory, directory_names, file_names in os.walk(
            contents, followlinks=False
        ):
            parent = pathlib.Path(directory)
            for name in sorted(directory_names):
                path = parent / name
                if path.is_symlink():
                    relative = path.relative_to(app).as_posix()
                    records.append(f"SYMLINK  {relative} -> {os.readlink(path)}")
            for name in sorted(file_names):
                path = parent / name
                relative = path.relative_to(app).as_posix()
                if path.is_symlink():
                    records.append(f"SYMLINK  {relative} -> {os.readlink(path)}")
                else:
                    records.append(f"{digest(path)}  {relative}")
        manifest.write_text(
            metadata_text
            + "sha256  relative_path\n"
            + "\n".join(sorted(records))
            + "\n",
            encoding="utf-8",
        )
        refresh_sidecar(manifest, sidecar)

    def make_fixture(
        self, root: pathlib.Path
    ) -> tuple[pathlib.Path, pathlib.Path, pathlib.Path]:
        app = root / "EchoesOfTheBrokenSun.app"
        contents = app / "Contents"
        (contents / "MacOS").mkdir(parents=True)
        binary = contents / "MacOS" / "EchoesOfTheBrokenSun"
        git_binary = pathlib.Path("/opt/homebrew/bin/git")
        git_lfs_binary = pathlib.Path("/opt/homebrew/bin/git-lfs")
        if not git_binary.is_file() or not git_lfs_binary.is_file():
            self.fail(
                "the package verifier fixture requires the approved Git and Git LFS"
            )
        git_resolved = git_binary.resolve(strict=True)
        git_lfs_resolved = git_lfs_binary.resolve(strict=True)
        shutil.copy2(git_binary, binary)
        binary.chmod(0o755)
        with (contents / "Info.plist").open("wb") as handle:
            plistlib.dump(
                {
                    "CFBundleExecutable": "EchoesOfTheBrokenSun",
                    "CFBundleIdentifier": "com.angelispseftis.echoesofthebrokensun",
                    "CFBundlePackageType": "APPL",
                    "CFBundleShortVersionString": "0.93.0",
                    "CFBundleVersion": "1",
                },
                handle,
                sort_keys=True,
            )
        link = contents / "CurrentBinary"
        os.symlink("MacOS/EchoesOfTheBrokenSun", link)
        subprocess.run(
            ["/usr/bin/codesign", "--force", "--deep", "--sign", "-", str(app)],
            check=True,
            capture_output=True,
            text=True,
        )

        signature_verify = subprocess.run(
            [
                "/usr/bin/codesign",
                "--verify",
                "--deep",
                "--strict",
                "--verbose=4",
                str(app),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        signature_display = subprocess.run(
            ["/usr/bin/codesign", "--display", "--verbose=4", str(app)],
            check=True,
            capture_output=True,
            text=True,
        )
        signature_evidence = (
            signature_verify.stdout
            + signature_verify.stderr
            + signature_display.stdout
            + signature_display.stderr
            + "echoes_signature_class=adhoc\n"
            + "echoes_signature_team_identifier=none\n"
            + "echoes_signature_verification=passed\n"
        ).encode()

        normal_log = root / "EchoesOfTheBrokenSun.normal-startup-smoke.log"
        stress_log = root / "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
        normal_log.write_text(
            "normal isolated smoke\n" "echoes_normal_startup_smoke_outcome=passed\n",
            encoding="utf-8",
        )
        stress_log.write_text(
            "stress isolated smoke\n"
            "echoes_legacy_stress_startup_smoke_outcome=passed\n",
            encoding="utf-8",
        )
        sdk_path = (
            "/Applications/Xcode.app/Contents/Developer/Platforms/"
            "MacOSX.platform/Developer/SDKs/MacOSX26.5.sdk"
        )
        clang_path = (
            "/Applications/Xcode.app/Contents/Developer/Toolchains/"
            "XcodeDefault.xctoolchain/usr/bin/clang"
        )
        metal_path = (
            "/Applications/Xcode.app/Contents/Developer/Toolchains/"
            "XcodeDefault.xctoolchain/usr/bin/metal"
        )
        toolchain_evidence = (
            "\n".join(
                (
                    "unreal_root=/Users/Shared/Epic Games/UE_5.8",
                    "unreal_engine=5.8.2",
                    "unreal_changelist=56702186",
                    "unreal_branch=++UE5+Release-5.8",
                    "unreal_promoted=1",
                    f"engine_build_file_sha256={'4' * 64}",
                    f"uat_sha256={'5' * 64}",
                    f"uat_driver_sha256={'6' * 64}",
                    f"git_path={git_binary}",
                    f"git_resolved_path={git_resolved}",
                    "git_version=git version 2.55.0",
                    f"git_sha256={digest(git_binary)}",
                    f"git_lfs_path={git_lfs_binary}",
                    f"git_lfs_resolved_path={git_lfs_resolved}",
                    "git_lfs_version=git-lfs/3.8.0 (GitHub; darwin arm64; go 1.27.0)",
                    f"git_lfs_sha256={digest(git_lfs_binary)}",
                    "developer_dir=/Applications/Xcode.app/Contents/Developer",
                    "xcode=Xcode 26.6;Build version 17F113",
                    "macos_sdk_version=26.5",
                    f"macos_sdk_path={sdk_path}",
                    f"clang_path={clang_path}",
                    "clang_version=Apple clang version test",
                    f"clang_sha256={'8' * 64}",
                    f"metal_path={metal_path}",
                    "metal_version=Apple metal version test",
                    f"metal_sha256={'9' * 64}",
                )
            )
            + "\n"
        )
        evidence_contents = {
            "EchoesOfTheBrokenSun.source-status.porcelain-v2-z": b"",
            "EchoesOfTheBrokenSun.repo-local-ignored-before.txt": b"",
            "EchoesOfTheBrokenSun.source-index-concealment.txt": b"",
            "EchoesOfTheBrokenSun.git-lfs-status.porcelain": b"",
            "EchoesOfTheBrokenSun.git-lfs-fsck.txt": (
                b"Git LFS fsck OK\n" b"echoes_git_lfs_fsck_outcome=passed\n"
            ),
            "EchoesOfTheBrokenSun.git-lfs-restrictive-config.txt": b"",
            "EchoesOfTheBrokenSun.git-lfs-hydration.txt": (
                b"echoes_git_lfs_hydration_outcome=passed\n"
                b"echoes_git_lfs_tracked_file_count=1\n"
                + (b"a" * 64)
                + b" * Content/Test.uasset\n"
            ),
            "EchoesOfTheBrokenSun.package-preflight.log": (
                b"preflight passed\n"
                b"echoes_content_preflight_outcome=passed\n"
                b"echoes_environment_preflight_outcome=passed\n"
                b"echoes_package_preflight_outcome=passed\n"
            ),
            "EchoesOfTheBrokenSun.BuildCookRun.log": (
                b"BuildCookRun passed\n" b"echoes_build_cook_run_outcome=passed\n"
            ),
            "EchoesContentPack.cooked-input.json": b'{"records":[],"schema":"test"}\n',
            "EchoesOfTheBrokenSun.signature-assessment.txt": signature_evidence,
            "EchoesOfTheBrokenSun.gatekeeper-assessment.txt": (
                b"assessments enabled\nEchoesOfTheBrokenSun.app: rejected\n"
                b"echoes_gatekeeper_policy=enabled\n"
                b"echoes_gatekeeper_assessment=rejected\n"
                b"echoes_gatekeeper_exit_code=3\n"
            ),
            "EchoesOfTheBrokenSun.stapler-validation.txt": (
                b"does not have a ticket stapled\n"
                b"echoes_stapling_status=not-stapled\n"
                b"echoes_stapler_exit_code=65\n"
            ),
            "EchoesOfTheBrokenSun.toolchain.txt": toolchain_evidence.encode(),
            "package_macos.used.sh": b"#!/bin/zsh\n",
            "verify_packaged_app.used.py": b"#!/usr/bin/env python3\n",
        }
        content_pack_digest = hashlib.sha256(
            evidence_contents["EchoesContentPack.cooked-input.json"]
        ).hexdigest()
        evidence_contents["EchoesContentPack.cooked-input.json.sha256"] = (
            f"{content_pack_digest}\n".encode("ascii")
        )
        evidence_paths: dict[str, pathlib.Path] = {}
        for name, contents_bytes in evidence_contents.items():
            path = root / name
            path.write_bytes(contents_bytes)
            evidence_paths[name] = path

        source_status_digest = digest(
            evidence_paths["EchoesOfTheBrokenSun.source-status.porcelain-v2-z"]
        )
        package_preflight_digest = digest(
            evidence_paths["EchoesOfTheBrokenSun.package-preflight.log"]
        )
        signature_evidence_digest = digest(
            evidence_paths["EchoesOfTheBrokenSun.signature-assessment.txt"]
        )
        gatekeeper_evidence_digest = digest(
            evidence_paths["EchoesOfTheBrokenSun.gatekeeper-assessment.txt"]
        )
        source_commit = "1" * 40
        source_tree_hash = "2" * 40
        archive_path = str(root.resolve())
        build_command = json.dumps(
            [
                "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.command",
                "BuildCookRun",
                "-project=/source/EchoesOfTheBrokenSun.uproject",
                "-noP4",
                "-platform=Mac",
                "-target=EchoesOfTheBrokenSun",
                "-clientconfig=Development",
                "-ubtargs=-MaxParallelActions=4",
                "-build",
                "-cook",
                "-stage",
                "-pak",
                "-package",
                "-archive",
                f"-archivedirectory={archive_path}",
                "-utf8output",
            ],
            separators=(",", ":"),
        )
        manifest = root / "EchoesOfTheBrokenSun.manifest.txt"
        manifest.write_text(
            "\n".join(
                (
                    "manifest_schema=2",
                    "artifact=EchoesOfTheBrokenSun.app",
                    "created_utc=20260831T000000Z",
                    f"source_commit={source_commit}",
                    f"source_tree_hash={source_tree_hash}",
                    "source_branch=detached",
                    "source_checkout_path=/source",
                    "source_checkout_kind=dedicated-linked-worktree-detached-at-main",
                    "source_top_level_binding=canonical-exact",
                    f"git_environment_policy={GIT_ENVIRONMENT_POLICY}",
                    f"origin_main={source_commit}",
                    f"remote_main={source_commit}",
                    "source_tree=clean",
                    "source_binding=clean-pushed-main",
                    "source_upstream_ref=origin/main",
                    "source_origin_fetch_url=https://github.com/redxking/echoes-of-the-broken-sun.git",
                    "source_origin_push_url=https://github.com/redxking/echoes-of-the-broken-sun.git",
                    "source_remote_authority=github.com/redxking/echoes-of-the-broken-sun",
                    f"source_status_sha256={source_status_digest}",
                    "source_status_evidence=EchoesOfTheBrokenSun.source-status.porcelain-v2-z",
                    "repo_local_derived_state_before=clean",
                    "repo_local_derived_state_scope=git-ignored-paths-within-source-checkout",
                    "repo_local_ignored_state_evidence=EchoesOfTheBrokenSun.repo-local-ignored-before.txt",
                    f"repo_local_ignored_state_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.repo-local-ignored-before.txt'])}",
                    "source_index_concealment=absent",
                    "source_index_concealment_scope=git-ls-files-v-and-f-non-H-records",
                    "source_index_concealment_evidence=EchoesOfTheBrokenSun.source-index-concealment.txt",
                    f"source_index_concealment_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.source-index-concealment.txt'])}",
                    "git_lfs_version=git-lfs/3.8.0 (GitHub; darwin arm64; go 1.27.0)",
                    f"git_lfs_path={git_lfs_binary}",
                    f"git_lfs_resolved_path={git_lfs_resolved}",
                    f"git_lfs_sha256={digest(git_lfs_binary)}",
                    "git_lfs_status=clean",
                    f"git_lfs_status_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.git-lfs-status.porcelain'])}",
                    "git_lfs_status_evidence=EchoesOfTheBrokenSun.git-lfs-status.porcelain",
                    "git_lfs_fsck=passed",
                    "git_lfs_fsck_outcome=passed",
                    f"git_lfs_fsck_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.git-lfs-fsck.txt'])}",
                    "git_lfs_fsck_evidence=EchoesOfTheBrokenSun.git-lfs-fsck.txt",
                    "git_lfs_restrictive_fetch_config=absent",
                    "git_lfs_restrictive_fetch_config_scope=effective-git-lfs-FetchInclude-and-FetchExclude",
                    "git_lfs_restrictive_config_evidence=EchoesOfTheBrokenSun.git-lfs-restrictive-config.txt",
                    f"git_lfs_restrictive_config_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.git-lfs-restrictive-config.txt'])}",
                    "git_lfs_hydration=complete",
                    "git_lfs_hydration_scope=all-git-lfs-tracked-working-tree-files",
                    "git_lfs_hydration_outcome=passed",
                    "git_lfs_tracked_file_count=1",
                    "git_lfs_hydration_evidence=EchoesOfTheBrokenSun.git-lfs-hydration.txt",
                    f"git_lfs_hydration_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.git-lfs-hydration.txt'])}",
                    "configuration=Development",
                    "platform=Mac-arm64",
                    "architecture=arm64",
                    f"archive_path={archive_path}",
                    "archive_outside_checkout=true",
                    "archive_free_gib_before=100",
                    "internal_free_gib_before=100",
                    f"build_command_argv={build_command}",
                    "package_preflight_log=EchoesOfTheBrokenSun.package-preflight.log",
                    f"package_preflight_log_sha256={package_preflight_digest}",
                    "content_preflight_outcome=passed",
                    "environment_preflight_outcome=passed",
                    "package_preflight_outcome=passed",
                    "build_log=EchoesOfTheBrokenSun.BuildCookRun.log",
                    f"build_log_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.BuildCookRun.log'])}",
                    "build_cook_run_outcome=passed",
                    "ignored_cook_inputs=Content/Data/Generated/EchoesContentPack.json;Content/Data/Generated/EchoesContentPack.json.sha256",
                    "generated_content_pack_source=Content/Data/Generated/EchoesContentPack.json",
                    "generated_content_pack_digest_source=Content/Data/Generated/EchoesContentPack.json.sha256",
                    "generated_content_pack=EchoesContentPack.cooked-input.json",
                    f"generated_content_pack_sha256={digest(evidence_paths['EchoesContentPack.cooked-input.json'])}",
                    "generated_content_pack_digest=EchoesContentPack.cooked-input.json.sha256",
                    f"generated_content_pack_digest_sha256={digest(evidence_paths['EchoesContentPack.cooked-input.json.sha256'])}",
                    "normal_startup_smoke=EchoesOfTheBrokenSun.normal-startup-smoke.log",
                    f"normal_startup_smoke_sha256={digest(normal_log)}",
                    "normal_startup_smoke_outcome=passed",
                    "legacy_stress_startup_smoke=EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log",
                    f"legacy_stress_startup_smoke_sha256={digest(stress_log)}",
                    "legacy_stress_startup_smoke_outcome=passed",
                    "unreal_engine=5.8.2",
                    "unreal_root=/Users/Shared/Epic Games/UE_5.8",
                    "unreal_changelist=56702186",
                    "unreal_branch=++UE5+Release-5.8",
                    "unreal_promoted=1",
                    f"engine_build_file_sha256={'4' * 64}",
                    f"uat_sha256={'5' * 64}",
                    f"uat_driver_sha256={'6' * 64}",
                    f"git_path={git_binary}",
                    f"git_resolved_path={git_resolved}",
                    "git_version=git version 2.55.0",
                    f"git_sha256={digest(git_binary)}",
                    "toolchain_evidence=EchoesOfTheBrokenSun.toolchain.txt",
                    f"toolchain_evidence_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.toolchain.txt'])}",
                    "developer_dir=/Applications/Xcode.app/Contents/Developer",
                    "xcode=Xcode 26.6;Build version 17F113",
                    "macos_sdk_version=26.5",
                    f"macos_sdk_path={sdk_path}",
                    f"clang_path={clang_path}",
                    "clang_version=Apple clang version test",
                    f"clang_sha256={'8' * 64}",
                    f"metal_path={metal_path}",
                    "metal_version=Apple metal version test",
                    f"metal_sha256={'9' * 64}",
                    "host_os_version=26.6.2",
                    "host_os_build=25G83",
                    "host_arch=arm64",
                    "host_model=MacBookPro18,3",
                    f"project_file_sha256={'3' * 64}",
                    "packager_copy=package_macos.used.sh",
                    f"packager_sha256={digest(evidence_paths['package_macos.used.sh'])}",
                    "package_verifier_copy=verify_packaged_app.used.py",
                    f"package_verifier_sha256={digest(evidence_paths['verify_packaged_app.used.py'])}",
                    "bundle_identifier=com.angelispseftis.echoesofthebrokensun",
                    "bundle_short_version=0.93.0",
                    "bundle_build_version=1",
                    f"application_executable_sha256={digest(binary)}",
                    "signature_class=adhoc",
                    "signature_team_identifier=none",
                    "signature_verification=passed",
                    "signature_evidence=EchoesOfTheBrokenSun.signature-assessment.txt",
                    f"signature_evidence_sha256={signature_evidence_digest}",
                    "developer_id_signing=not-performed",
                    "gatekeeper_policy=enabled",
                    "gatekeeper_assessment=rejected",
                    "gatekeeper_exit_code=3",
                    "gatekeeper_evidence=EchoesOfTheBrokenSun.gatekeeper-assessment.txt",
                    f"gatekeeper_evidence_sha256={gatekeeper_evidence_digest}",
                    "notarization_status=not-submitted-by-package-tool",
                    "stapling_status=not-stapled",
                    "stapler_exit_code=65",
                    "stapler_evidence=EchoesOfTheBrokenSun.stapler-validation.txt",
                    f"stapler_evidence_sha256={digest(evidence_paths['EchoesOfTheBrokenSun.stapler-validation.txt'])}",
                    "installer_status=not-produced",
                    "release_qualification=not-release-qualified",
                    "build_context_record=package-tool-observed",
                    "manifest_authority=self-consistency-record-not-independent-attestation",
                    "claim_boundary=local-development-package-only",
                    "",
                    "sha256  relative_path",
                    "PLACEHOLDER",
                )
            )
            + "\n",
            encoding="utf-8",
        )
        sidecar = root / "EchoesOfTheBrokenSun.manifest.sha256"
        self.resign_and_refresh_records(app, manifest, sidecar)
        return app, manifest, sidecar

    def test_exact_file_and_internal_symlink_sets_are_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            result = verify_package(app, manifest, sidecar)
            self.assertTrue(result["accepted"])
            self.assertEqual(
                result["acceptance_scope"],
                "package-integrity-evidence-semantics-and-record-self-consistency",
            )
            self.assertFalse(result["release_qualified"])
            self.assertGreaterEqual(result["application_files"], 3)
            self.assertEqual(result["application_symlinks"], 1)
            self.assertEqual(result["manifest_schema"], 2)
            self.assertTrue(result["artifact_integrity_validated"])
            self.assertTrue(result["evidence_semantics_validated"])
            self.assertTrue(result["recorded_build_context_self_consistent"])
            self.assertFalse(result["live_current_source_toolchain_cross_checked"])
            self.assertFalse(result["historical_build_context_independently_attested"])
            self.assertFalse(result["independent_attestation_present"])
            self.assertNotIn("schema_2_provenance_validated", result)
            self.assertEqual(
                result["recorded_manifest_metadata"]["release_qualification"],
                "not-release-qualified",
            )

    def test_schema_1_historical_manifest_remains_supported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            text = manifest.read_text(encoding="utf-8").replace(
                "manifest_schema=2\n", ""
            )
            manifest.write_text(text, encoding="utf-8")
            refresh_sidecar(manifest, sidecar)
            result = verify_package(app, manifest, sidecar)
            self.assertEqual(result["manifest_schema"], 1)
            self.assertFalse(result["evidence_semantics_validated"])
            self.assertFalse(result["recorded_build_context_self_consistent"])
            self.assertFalse(result["live_current_source_toolchain_cross_checked"])
            self.assertEqual(
                result["acceptance_scope"], "package-manifest-integrity-only"
            )

    def test_schema_2_rejects_bundle_identifier_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            plist_path = app / "Contents" / "Info.plist"
            with plist_path.open("rb") as handle:
                plist = plistlib.load(handle)
            plist["CFBundleIdentifier"] = "com.example.contradiction"
            with plist_path.open("wb") as handle:
                plistlib.dump(plist, handle, sort_keys=True)
            self.resign_and_refresh_records(app, manifest, sidecar)
            with self.assertRaisesRegex(VerificationError, "CFBundleIdentifier"):
                verify_package(app, manifest, sidecar)

    def test_schema_2_rejects_nonexecutable_bundle_binary(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            binary = app / "Contents" / "MacOS" / "EchoesOfTheBrokenSun"
            binary.chmod(0o644)
            with self.assertRaisesRegex(VerificationError, "not executable"):
                verify_package(app, manifest, sidecar)

    def test_schema_2_cross_binds_build_archive_argument(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            lines = manifest.read_text(encoding="utf-8").splitlines()
            for index, line in enumerate(lines):
                if line.startswith("build_command_argv="):
                    build_argv = json.loads(line.split("=", 1)[1])
                    build_argv = [
                        (
                            "-archivedirectory=/contradictory/archive"
                            if argument.startswith("-archivedirectory=")
                            else argument
                        )
                        for argument in build_argv
                    ]
                    lines[index] = "build_command_argv=" + json.dumps(
                        build_argv, separators=(",", ":")
                    )
                    break
            manifest.write_text("\n".join(lines) + "\n", encoding="utf-8")
            refresh_sidecar(manifest, sidecar)
            with self.assertRaisesRegex(VerificationError, "archive path disagrees"):
                verify_package(app, manifest, sidecar)

    def test_schema_2_rejects_semantically_contradictory_signature_evidence(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            evidence = root / "EchoesOfTheBrokenSun.signature-assessment.txt"
            old_digest = digest(evidence)
            evidence.write_text(
                evidence.read_text(encoding="utf-8").replace(
                    "echoes_signature_class=adhoc",
                    "echoes_signature_class=developer-id",
                ),
                encoding="utf-8",
            )
            manifest.write_text(
                manifest.read_text(encoding="utf-8").replace(
                    f"signature_evidence_sha256={old_digest}",
                    f"signature_evidence_sha256={digest(evidence)}",
                ),
                encoding="utf-8",
            )
            refresh_sidecar(manifest, sidecar)
            with self.assertRaisesRegex(
                VerificationError, "signature evidence contradicts"
            ):
                verify_package(app, manifest, sidecar)

    def test_schema_2_requires_exact_source_tree_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            text = manifest.read_text(encoding="utf-8")
            text = (
                "\n".join(
                    line
                    for line in text.splitlines()
                    if not line.startswith("source_tree_hash=")
                )
                + "\n"
            )
            manifest.write_text(text, encoding="utf-8")
            sidecar.write_text(
                f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(VerificationError, "source_tree_hash"):
                verify_package(app, manifest, sidecar)

    def test_schema_2_binds_distribution_assessment_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            (root / "EchoesOfTheBrokenSun.gatekeeper-assessment.txt").write_text(
                "changed after manifest\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(VerificationError, "gatekeeper-assessment"):
                verify_package(app, manifest, sidecar)

    def test_schema_2_records_disabled_gatekeeper_as_not_enforced(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            evidence = root / "EchoesOfTheBrokenSun.gatekeeper-assessment.txt"
            old_digest = digest(evidence)
            evidence.write_text(
                "assessments disabled\n"
                "echoes_gatekeeper_policy=disabled\n"
                "echoes_gatekeeper_assessment=not-enforced\n"
                "echoes_gatekeeper_exit_code=0\n",
                encoding="utf-8",
            )
            text = manifest.read_text(encoding="utf-8")
            text = text.replace(
                "gatekeeper_policy=enabled", "gatekeeper_policy=disabled"
            )
            text = text.replace(
                "gatekeeper_assessment=rejected",
                "gatekeeper_assessment=not-enforced",
            )
            text = text.replace("gatekeeper_exit_code=3", "gatekeeper_exit_code=0")
            text = text.replace(
                f"gatekeeper_evidence_sha256={old_digest}",
                f"gatekeeper_evidence_sha256={digest(evidence)}",
            )
            manifest.write_text(text, encoding="utf-8")
            refresh_sidecar(manifest, sidecar)
            result = verify_package(app, manifest, sidecar)
            self.assertTrue(result["evidence_semantics_validated"])

    def test_schema_2_cross_checks_generated_content_digest_sidecar(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            evidence = root / "EchoesContentPack.cooked-input.json.sha256"
            old_digest = digest(evidence)
            evidence.write_text(f"{'f' * 64}\n", encoding="ascii")
            manifest.write_text(
                manifest.read_text(encoding="utf-8").replace(
                    f"generated_content_pack_digest_sha256={old_digest}",
                    f"generated_content_pack_digest_sha256={digest(evidence)}",
                ),
                encoding="utf-8",
            )
            refresh_sidecar(manifest, sidecar)
            with self.assertRaisesRegex(
                VerificationError, "digest contradicts the retained input"
            ):
                verify_package(app, manifest, sidecar)

    def test_schema_2_cannot_promote_the_release_boundary(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            text = manifest.read_text(encoding="utf-8").replace(
                "release_qualification=not-release-qualified",
                "release_qualification=release-qualified",
            )
            manifest.write_text(text, encoding="utf-8")
            sidecar.write_text(
                f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(VerificationError, "release_qualification"):
                verify_package(app, manifest, sidecar)

    def test_schema_2_rejects_an_archive_inside_the_checkout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            lines = manifest.read_text(encoding="utf-8").splitlines()
            lines = [
                (
                    "archive_path=/source/BuildArtifacts/package"
                    if line.startswith("archive_path=")
                    else line
                )
                for line in lines
            ]
            text = "\n".join(lines) + "\n"
            manifest.write_text(text, encoding="utf-8")
            sidecar.write_text(
                f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(
                VerificationError, "inside the source checkout"
            ):
                verify_package(app, manifest, sidecar)

    def test_schema_2_rejects_coherently_recorded_failed_outcomes(self) -> None:
        cases = (
            (
                "EchoesOfTheBrokenSun.git-lfs-fsck.txt",
                "git_lfs_fsck_sha256",
                "echoes_git_lfs_fsck_outcome=passed",
                "git_lfs_fsck_outcome",
            ),
            (
                "EchoesOfTheBrokenSun.git-lfs-hydration.txt",
                "git_lfs_hydration_sha256",
                "echoes_git_lfs_hydration_outcome=passed",
                "git_lfs_hydration_outcome",
            ),
            (
                "EchoesOfTheBrokenSun.package-preflight.log",
                "package_preflight_log_sha256",
                "echoes_content_preflight_outcome=passed",
                "content_preflight_outcome",
            ),
            (
                "EchoesOfTheBrokenSun.package-preflight.log",
                "package_preflight_log_sha256",
                "echoes_environment_preflight_outcome=passed",
                "environment_preflight_outcome",
            ),
            (
                "EchoesOfTheBrokenSun.package-preflight.log",
                "package_preflight_log_sha256",
                "echoes_package_preflight_outcome=passed",
                "package_preflight_outcome",
            ),
            (
                "EchoesOfTheBrokenSun.BuildCookRun.log",
                "build_log_sha256",
                "echoes_build_cook_run_outcome=passed",
                "build_cook_run_outcome",
            ),
            (
                "EchoesOfTheBrokenSun.normal-startup-smoke.log",
                "normal_startup_smoke_sha256",
                "echoes_normal_startup_smoke_outcome=passed",
                "normal_startup_smoke_outcome",
            ),
            (
                "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log",
                "legacy_stress_startup_smoke_sha256",
                "echoes_legacy_stress_startup_smoke_outcome=passed",
                "legacy_stress_startup_smoke_outcome",
            ),
        )
        for evidence_name, digest_key, marker, outcome_key in cases:
            with self.subTest(
                outcome=outcome_key
            ), tempfile.TemporaryDirectory() as temporary:
                root = pathlib.Path(temporary)
                app, manifest, sidecar = self.make_fixture(root)
                evidence = root / evidence_name
                evidence.write_text(
                    evidence.read_text(encoding="utf-8").replace(
                        marker, marker.replace("=passed", "=failed")
                    ),
                    encoding="utf-8",
                )
                replace_manifest_metadata(
                    manifest,
                    sidecar,
                    {
                        digest_key: digest(evidence),
                        outcome_key: "failed",
                    },
                )
                with self.assertRaisesRegex(VerificationError, outcome_key):
                    verify_package(app, manifest, sidecar)

    def test_schema_2_rejects_nonempty_clean_state_evidence(self) -> None:
        cases = (
            (
                "EchoesOfTheBrokenSun.source-status.porcelain-v2-z",
                "source_status_sha256",
                "source-status evidence",
            ),
            (
                "EchoesOfTheBrokenSun.repo-local-ignored-before.txt",
                "repo_local_ignored_state_sha256",
                "repo-local ignored-state evidence",
            ),
            (
                "EchoesOfTheBrokenSun.source-index-concealment.txt",
                "source_index_concealment_sha256",
                "source-index concealment evidence",
            ),
            (
                "EchoesOfTheBrokenSun.git-lfs-status.porcelain",
                "git_lfs_status_sha256",
                "Git LFS status evidence",
            ),
            (
                "EchoesOfTheBrokenSun.git-lfs-restrictive-config.txt",
                "git_lfs_restrictive_config_sha256",
                "Git LFS restrictive-config evidence",
            ),
        )
        for evidence_name, digest_key, error_text in cases:
            with self.subTest(
                evidence=evidence_name
            ), tempfile.TemporaryDirectory() as temporary:
                root = pathlib.Path(temporary)
                app, manifest, sidecar = self.make_fixture(root)
                evidence = root / evidence_name
                evidence.write_text("FAILED\n", encoding="utf-8")
                replace_manifest_metadata(
                    manifest,
                    sidecar,
                    {digest_key: digest(evidence)},
                )
                with self.assertRaisesRegex(VerificationError, error_text):
                    verify_package(app, manifest, sidecar)

    def test_schema_2_rejects_git_lfs_pointer_stub_inventory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            evidence = root / "EchoesOfTheBrokenSun.git-lfs-hydration.txt"
            evidence.write_text(
                evidence.read_text(encoding="utf-8").replace(
                    " * Content/Test.uasset", " - Content/Test.uasset"
                ),
                encoding="utf-8",
            )
            replace_manifest_metadata(
                manifest,
                sidecar,
                {"git_lfs_hydration_sha256": digest(evidence)},
            )
            with self.assertRaisesRegex(VerificationError, "unhydrated pointer stub"):
                verify_package(app, manifest, sidecar)

    def test_schema_2_pins_source_and_tool_authorities(self) -> None:
        cases = (
            ("source_branch", "main"),
            ("source_checkout_kind", "dedicated-linked-worktree"),
            ("source_origin_fetch_url", "https://example.invalid/repo.git"),
            ("source_origin_push_url", "ssh://example.invalid/repo.git"),
            ("git_version", "git version 2.54.0"),
            ("git_sha256", "0" * 64),
            ("git_lfs_version", "git-lfs/3.7.0"),
            ("git_lfs_sha256", "f" * 64),
        )
        for key, value in cases:
            with self.subTest(metadata=key), tempfile.TemporaryDirectory() as temporary:
                app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
                replace_manifest_metadata(manifest, sidecar, {key: value})
                with self.assertRaisesRegex(VerificationError, key):
                    verify_package(app, manifest, sidecar)

    def test_live_context_is_a_separate_explicit_gate(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            missing_source = root / "unavailable-source"
            build_argv = json.loads(
                next(
                    line.split("=", 1)[1]
                    for line in manifest.read_text(encoding="utf-8").splitlines()
                    if line.startswith("build_command_argv=")
                )
            )
            build_argv = [
                (
                    f"-project={missing_source}/EchoesOfTheBrokenSun.uproject"
                    if argument.startswith("-project=")
                    else argument
                )
                for argument in build_argv
            ]
            replace_manifest_metadata(
                manifest,
                sidecar,
                {
                    "source_checkout_path": str(missing_source),
                    "build_command_argv": json.dumps(build_argv, separators=(",", ":")),
                },
            )
            offline_result = verify_package(app, manifest, sidecar)
            self.assertFalse(
                offline_result["live_current_source_toolchain_cross_checked"]
            )
            with self.assertRaisesRegex(
                VerificationError, "live source checkout is unavailable"
            ), mock.patch.dict(
                os.environ, environment_without_git_overrides(), clear=True
            ):
                verify_package(
                    app,
                    manifest,
                    sidecar,
                    require_live_build_context=True,
                )

    def test_live_verifier_rejects_repository_redirection_environment_first(
        self,
    ) -> None:
        for variable_name in ("GIT_DIR", "GIT_WORK_TREE"):
            with self.subTest(
                variable=variable_name
            ), tempfile.TemporaryDirectory() as temporary:
                root = pathlib.Path(temporary)
                app, manifest, sidecar = self.make_fixture(root)
                missing_source = root / "unavailable-source"
                build_argv = json.loads(
                    next(
                        line.split("=", 1)[1]
                        for line in manifest.read_text(encoding="utf-8").splitlines()
                        if line.startswith("build_command_argv=")
                    )
                )
                build_argv = [
                    (
                        f"-project={missing_source}/EchoesOfTheBrokenSun.uproject"
                        if argument.startswith("-project=")
                        else argument
                    )
                    for argument in build_argv
                ]
                replace_manifest_metadata(
                    manifest,
                    sidecar,
                    {
                        "source_checkout_path": str(missing_source),
                        "build_command_argv": json.dumps(
                            build_argv, separators=(",", ":")
                        ),
                    },
                )
                environment = environment_without_git_overrides()
                environment[variable_name] = "/tmp/redirected-git-context"
                with mock.patch.dict(
                    os.environ, environment, clear=True
                ), self.assertRaisesRegex(
                    VerificationError,
                    rf"inherited Git or Git LFS environment variables: {variable_name}",
                ):
                    verify_package(
                        app,
                        manifest,
                        sidecar,
                        require_live_build_context=True,
                    )

    def test_packager_rejects_git_dir_redirection_before_tool_use(self) -> None:
        rejected = self.run_packager_with_git_environment("GIT_DIR")
        self.assertEqual(rejected.returncode, 2)
        self.assertIn("before tool use: GIT_DIR", rejected.stderr)

    def test_packager_rejects_git_work_tree_redirection_before_tool_use(self) -> None:
        rejected = self.run_packager_with_git_environment("GIT_WORK_TREE")
        self.assertEqual(rejected.returncode, 2)
        self.assertIn("before tool use: GIT_WORK_TREE", rejected.stderr)

    def test_packager_rejects_the_complete_git_environment_prefix_policy(
        self,
    ) -> None:
        for variable_name in REPOSITORY_SHAPING_GIT_VARIABLES:
            with self.subTest(variable=variable_name):
                rejected = self.run_packager_with_git_environment(variable_name)
                self.assertEqual(rejected.returncode, 2)
                self.assertIn(
                    f"before tool use: {variable_name}",
                    rejected.stderr,
                )

    def test_live_verifier_rejects_and_sanitizes_the_complete_prefix_policy(
        self,
    ) -> None:
        for variable_name in REPOSITORY_SHAPING_GIT_VARIABLES:
            with self.subTest(variable=variable_name):
                environment = environment_without_git_overrides()
                environment[variable_name] = "/tmp/redirected-git-context"
                with mock.patch.dict(
                    os.environ, environment, clear=True
                ), self.assertRaisesRegex(VerificationError, variable_name):
                    _sanitized_git_environment()

        environment = environment_without_git_overrides()
        environment.update(CONTROLLED_GIT_ENVIRONMENT)
        with mock.patch.dict(os.environ, environment, clear=True):
            sanitized = _sanitized_git_environment()
        self.assertEqual(
            {
                name: value
                for name, value in sanitized.items()
                if name.startswith("GIT_")
            },
            CONTROLLED_GIT_ENVIRONMENT,
        )

    def test_live_verifier_controls_only_the_approved_git_environment(self) -> None:
        self.assertEqual(
            CONTROLLED_GIT_ENVIRONMENT,
            {
                "GIT_ATTR_NOSYSTEM": "1",
                "GIT_CONFIG_GLOBAL": "/dev/null",
                "GIT_CONFIG_NOSYSTEM": "1",
                "GIT_CONFIG_SYSTEM": "/dev/null",
                "GIT_NO_REPLACE_OBJECTS": "1",
                "GIT_TERMINAL_PROMPT": "0",
            },
        )
        source = (PROJECT_ROOT / "Scripts" / "verify_packaged_app.py").read_text(
            encoding="utf-8"
        )
        environment_guard = "git_environment = _sanitized_git_environment()"
        source_lookup = (
            'source_checkout_record = pathlib.Path(metadata["source_checkout_path"])'
        )
        top_level_lookup = '"rev-parse", "--path-format=absolute", "--show-toplevel"'
        commit_lookup = '"source_commit": git_output("rev-parse", "--verify", "HEAD")'
        self.assertLess(source.index(environment_guard), source.index(source_lookup))
        self.assertLess(source.index(top_level_lookup), source.index(commit_lookup))
        self.assertGreaterEqual(source.count("environment=git_environment"), 4)

    def test_packager_emits_external_schema_2_provenance(self) -> None:
        source = PACKAGER.read_text(encoding="utf-8")
        self.assertIn(
            'artifact_root_default="${repository_checkout:h}/BuildArtifacts"',
            source,
        )
        self.assertIn(
            'artifact_root="${ECHOES_BUILD_ARTIFACT_ROOT:-$artifact_root_default}"',
            source,
        )
        self.assertIn('"$archive_dir" == "$project_root"/*', source)
        self.assertNotIn(
            "$project_root/BuildArtifacts/Packages/Mac-Development-",
            source,
        )
        for required in (
            'print "manifest_schema=2"',
            'print "source_tree_hash=$source_tree_hash"',
            'print "source_checkout_path=$project_root"',
            'print "source_checkout_kind=dedicated-linked-worktree-detached-at-main"',
            'print "source_top_level_binding=canonical-exact"',
            'print "git_environment_policy=$git_environment_policy"',
            'print "source_status_sha256=$source_status_evidence_sha256"',
            'print "source_origin_fetch_url=$origin_fetch_url"',
            'print "source_index_concealment=absent"',
            'print "repo_local_derived_state_before=clean"',
            'print "git_lfs_fsck=passed"',
            'print "git_lfs_fsck_outcome=passed"',
            'print "git_lfs_restrictive_fetch_config=absent"',
            'print "git_lfs_hydration=complete"',
            'print "git_lfs_hydration_outcome=passed"',
            'print "build_command_argv=$build_command_argv"',
            'print "content_preflight_outcome=passed"',
            'print "environment_preflight_outcome=passed"',
            'print "package_preflight_outcome=passed"',
            'print "build_cook_run_outcome=passed"',
            'print "generated_content_pack_sha256=$generated_content_pack_copy_sha256"',
            'print "generated_content_pack_digest_sha256=$generated_content_pack_digest_copy_sha256"',
            'print "unreal_root=$ue_root"',
            'print "toolchain_evidence_sha256=$toolchain_evidence_sha256"',
            'print "clang_sha256=$clang_sha256"',
            'print "metal_sha256=$metal_sha256"',
            'print "signature_class=$signature_class"',
            'print "gatekeeper_assessment=$gatekeeper_assessment"',
            'print "notarization_status=not-submitted-by-package-tool"',
            'print "stapling_status=$stapling_status"',
            'print "release_qualification=not-release-qualified"',
            "--require-live-build-context",
            '--json-output "$provenance"',
        ):
            self.assertIn(required, source)
        self.assertIn('if [[ "$source_branch" != detached ]]', source)
        self.assertIn('if [[ "$git_dir" == "$git_common_dir" ]]', source)
        self.assertIn("source_index_concealment_records", source)
        self.assertIn('git -C "$project_root" ls-files -f', source)
        self.assertIn("git_lfs_restrictive_config_records", source)
        self.assertIn("git_lfs ls-files -l", source)
        self.assertIn("git_lfs fsck --objects --pointers", source)
        self.assertIn("ls-files --others --ignored --exclude-standard", source)
        self.assertIn(
            'approved_origin_url="https://github.com/redxking/echoes-of-the-broken-sun.git"',
            source,
        )
        self.assertIn(
            'approved_git_sha256="9048038886ac36210fbb616b49b0707465f63683cb04e33a2013baf95f746938"',
            source,
        )
        self.assertIn(
            'approved_git_lfs_sha256="8e4ed79ff8562870a7f3b88993c221994e2bf44f8c65b930764b3f0ab0fcefa5"',
            source,
        )
        self.assertIn(f'git_environment_policy="{GIT_ENVIRONMENT_POLICY}"', source)
        for variable_name, value in CONTROLLED_GIT_ENVIRONMENT.items():
            self.assertIn(f"export {variable_name}={value}", source)
        self.assertLess(
            source.index('git_sha256="$(/usr/bin/shasum'),
            source.index('git_version="$($git_resolved_path --version)"'),
        )
        environment_rejection = "Packaging refuses inherited Git or Git LFS environment variables before tool use"
        initial_top_level = (
            'source_top_level="$(git -C "$project_root" rev-parse '
            "--path-format=absolute --show-toplevel"
        )
        self.assertLess(
            source.index(environment_rejection),
            source.index('git_version="$($git_resolved_path --version)"'),
        )
        self.assertLess(
            source.index(initial_top_level),
            source.index('source_commit="$(git -C "$project_root"'),
        )
        self.assertGreaterEqual(source.count("--show-toplevel"), 2)

    def test_packager_tracks_the_real_generated_content_pair(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = pathlib.Path(temporary) / "EchoesContentPack.json"
            subprocess.run(
                [
                    sys.executable,
                    str(PROJECT_ROOT / "Scripts" / "compile_content.py"),
                    "--source",
                    str(PROJECT_ROOT / "Content" / "Data" / "Source"),
                    "--output",
                    str(output),
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            digest_output = output.with_suffix(".json.sha256")
            self.assertEqual(
                {path.name for path in output.parent.iterdir()},
                {"EchoesContentPack.json", "EchoesContentPack.json.sha256"},
            )
            self.assertEqual(
                digest_output.read_text(encoding="ascii"), f"{digest(output)}\n"
            )

        source = PACKAGER.read_text(encoding="utf-8")
        self.assertIn(
            "expected_ignored_cook_inputs=$'Content/Data/Generated/"
            "EchoesContentPack.json\\nContent/Data/Generated/"
            "EchoesContentPack.json.sha256'",
            source,
        )
        self.assertIn(
            'generated_content_pack_declared_sha256="$(/usr/bin/tr -d \'\\r\\n\' < "$generated_content_pack_digest")"',
            source,
        )

    def test_packager_pins_xcode_before_preflight_and_exports_it(self) -> None:
        source = PACKAGER.read_text(encoding="utf-8")
        equality_check = (
            'if [[ ! -d "$developer_dir" || '
            '"$developer_dir" != "$approved_developer_dir" ]]'
        )
        export = 'export DEVELOPER_DIR="$approved_developer_dir"'
        preflight = 'verify_clean_pushed_source "before build"'
        self.assertIn(
            'approved_developer_dir="/Applications/Xcode.app/Contents/Developer"',
            source,
        )
        self.assertIn(
            'developer_dir="${DEVELOPER_DIR:-$(/usr/bin/xcode-select -p)}"',
            source,
        )
        self.assertLess(source.index(equality_check), source.index(preflight))
        self.assertLess(source.index(export), source.index(preflight))
        environment = environment_without_git_overrides()
        environment["DEVELOPER_DIR"] = "/tmp"
        rejected = subprocess.run(
            [str(PACKAGER)],
            cwd=PROJECT_ROOT,
            env=environment,
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(rejected.returncode, 2)
        self.assertIn("authorized only with the verified Xcode", rejected.stderr)

    def test_packager_does_not_use_distribution_credentials(self) -> None:
        source = PACKAGER.read_text(encoding="utf-8")
        self.assertIn("--sign -", source)
        self.assertIn('print "developer_id_signing=not-performed"', source)
        for forbidden in (
            "notarytool submit",
            "--keychain-profile",
            "--apple-id",
            "--password",
            "--team-id",
            "Developer ID Application:",
        ):
            self.assertNotIn(forbidden, source)

    def test_duplicate_record_cannot_hide_an_omitted_path(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            text = manifest.read_text(encoding="utf-8")
            record = text.splitlines()[-2]
            manifest.write_text(text + record + "\n", encoding="utf-8")
            sidecar.write_text(
                f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8"
            )
            with self.assertRaises(VerificationError):
                verify_package(app, manifest, sidecar)

    def test_symlink_must_resolve_inside_the_application(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            link = app / "Contents" / "CurrentBinary"
            link.unlink()
            os.symlink("../../../outside", link)
            (root / "outside").write_text("outside", encoding="utf-8")
            text = manifest.read_text(encoding="utf-8").replace(
                "SYMLINK  Contents/CurrentBinary -> MacOS/EchoesOfTheBrokenSun",
                "SYMLINK  Contents/CurrentBinary -> ../../../outside",
            )
            manifest.write_text(text, encoding="utf-8")
            sidecar.write_text(
                f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(VerificationError, "escapes"):
                verify_package(app, manifest, sidecar)

    def test_tampered_file_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            (app / "Contents" / "MacOS" / "EchoesOfTheBrokenSun").write_bytes(
                b"changed"
            )
            with self.assertRaises(VerificationError):
                verify_package(app, manifest, sidecar)

    def test_special_bundle_entry_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            os.mkfifo(app / "Contents" / "UnexpectedPipe")
            with self.assertRaisesRegex(VerificationError, "special"):
                verify_package(app, manifest, sidecar)


if __name__ == "__main__":
    unittest.main(verbosity=2)
