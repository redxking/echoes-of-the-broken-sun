#!/usr/bin/env python3
"""Verify an Echoes package against its exact signed-content manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import re


HASH_RECORD = re.compile(r"^([0-9a-f]{64})  (.+)$")
LINK_RECORD = re.compile(r"^SYMLINK  (.+) -> (.*)$")
SIDECAR_RECORD = re.compile(r"^([0-9a-f]{64})  ([^/]+)$")


class VerificationError(RuntimeError):
    pass


def _sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _bundle_entries(contents: pathlib.Path, app: pathlib.Path) -> tuple[set[str], set[str]]:
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
        raise VerificationError("manifest digest sidecar must contain exactly one record")
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
    for key in (
        "source_commit",
        "origin_main",
        "remote_main",
        "normal_startup_smoke_sha256",
        "legacy_stress_startup_smoke_sha256",
    ):
        if re.fullmatch(r"[0-9a-f]{40}" if key.endswith("main") or key == "source_commit" else r"[0-9a-f]{64}", metadata.get(key, "")) is None:
            raise VerificationError(f"manifest metadata {key} is missing or malformed")
    if not (
        metadata["source_commit"]
        == metadata["origin_main"]
        == metadata["remote_main"]
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
                raise VerificationError(f"duplicate or malformed symlink path: {relative}")
            link_records[relative] = target
        else:
            raise VerificationError(f"malformed manifest record: {line!r}")

    actual_files, actual_links = _bundle_entries(contents, app)
    if set(file_records) != actual_files:
        raise VerificationError("manifest file path set differs from the application")
    if set(link_records) != actual_links:
        raise VerificationError("manifest symlink path set differs from the application")
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

    archive = manifest.parent
    for name_key, digest_key in (
        ("normal_startup_smoke", "normal_startup_smoke_sha256"),
        ("legacy_stress_startup_smoke", "legacy_stress_startup_smoke_sha256"),
    ):
        log_path = archive / metadata[name_key]
        if not log_path.is_file() or _sha256(log_path) != metadata[digest_key]:
            raise VerificationError(f"startup-smoke evidence changed: {metadata[name_key]}")

    return {
        "accepted": True,
        "application_files": len(actual_files),
        "application_symlinks": len(actual_links),
        "manifest_sha256": manifest_sha256,
        "source_commit": metadata["source_commit"],
        "normal_startup_smoke_sha256": metadata["normal_startup_smoke_sha256"],
        "legacy_stress_startup_smoke_sha256": metadata[
            "legacy_stress_startup_smoke_sha256"
        ],
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
