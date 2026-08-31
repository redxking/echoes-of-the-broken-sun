#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import os
import pathlib
import sys
import tempfile
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT_ROOT / "Scripts"))

from verify_packaged_app import VerificationError, verify_package  # noqa: E402


def digest(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class PackageManifestVerifierTests(unittest.TestCase):
    def make_fixture(self, root: pathlib.Path) -> tuple[pathlib.Path, pathlib.Path, pathlib.Path]:
        app = root / "EchoesOfTheBrokenSun.app"
        contents = app / "Contents"
        (contents / "MacOS").mkdir(parents=True)
        binary = contents / "MacOS" / "EchoesOfTheBrokenSun"
        binary.write_bytes(b"deterministic-test-binary")
        link = contents / "CurrentBinary"
        os.symlink("MacOS/EchoesOfTheBrokenSun", link)

        normal_log = root / "EchoesOfTheBrokenSun.normal-startup-smoke.log"
        stress_log = root / "EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
        normal_log.write_text("normal isolated smoke\n", encoding="utf-8")
        stress_log.write_text("stress isolated smoke\n", encoding="utf-8")
        source_commit = "1" * 40
        manifest = root / "EchoesOfTheBrokenSun.manifest.txt"
        manifest.write_text(
            "\n".join(
                (
                    "artifact=EchoesOfTheBrokenSun.app",
                    "created_utc=20260831T000000Z",
                    f"source_commit={source_commit}",
                    f"origin_main={source_commit}",
                    f"remote_main={source_commit}",
                    "source_tree=clean",
                    "source_binding=clean-pushed-main",
                    "configuration=Development",
                    "platform=Mac-arm64",
                    "normal_startup_smoke=EchoesOfTheBrokenSun.normal-startup-smoke.log",
                    f"normal_startup_smoke_sha256={digest(normal_log)}",
                    "legacy_stress_startup_smoke=EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log",
                    f"legacy_stress_startup_smoke_sha256={digest(stress_log)}",
                    "",
                    "sha256  relative_path",
                    f"{digest(binary)}  Contents/MacOS/EchoesOfTheBrokenSun",
                    "SYMLINK  Contents/CurrentBinary -> MacOS/EchoesOfTheBrokenSun",
                )
            )
            + "\n",
            encoding="utf-8",
        )
        sidecar = root / "EchoesOfTheBrokenSun.manifest.sha256"
        sidecar.write_text(f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8")
        return app, manifest, sidecar

    def test_exact_file_and_internal_symlink_sets_are_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            result = verify_package(app, manifest, sidecar)
            self.assertTrue(result["accepted"])
            self.assertEqual(result["application_files"], 1)
            self.assertEqual(result["application_symlinks"], 1)

    def test_duplicate_record_cannot_hide_an_omitted_path(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            app, manifest, sidecar = self.make_fixture(pathlib.Path(temporary))
            text = manifest.read_text(encoding="utf-8")
            record = text.splitlines()[-2]
            manifest.write_text(text + record + "\n", encoding="utf-8")
            sidecar.write_text(f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8")
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
            sidecar.write_text(f"{digest(manifest)}  {manifest.name}\n", encoding="utf-8")
            with self.assertRaisesRegex(VerificationError, "escapes"):
                verify_package(app, manifest, sidecar)

    def test_tampered_file_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            app, manifest, sidecar = self.make_fixture(root)
            (app / "Contents" / "MacOS" / "EchoesOfTheBrokenSun").write_bytes(b"changed")
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
