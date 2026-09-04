#!/usr/bin/env python3
"""Fail-closed tests for the Glass Scar dressing pack header emitter.

The checked-in header must be a pure function of the two frozen packs, and the
emitter must refuse any record that would stand on passable ground, any digest
drift, and any base-contract drift — without writing.
Author and owner: Angelis Pseftis.
"""

from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EMITTER = ROOT / "Content/World/Tools/emit_dressing_pack_header.py"
DRESSING_PACK = ROOT / "Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.json"
DRESSING_SIDECAR = DRESSING_PACK.with_suffix(".sha256")
BASE_PACK = ROOT / "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
BASE_SIDECAR = BASE_PACK.with_suffix(".sha256")
HEADER = ROOT / "Source/EchoesOfTheBrokenSun/Public/EchoesGlassScarDressingPack.h"

RELATIVE = [
    "Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.json",
    "Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.sha256",
    "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json",
    "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.sha256",
    "Source/EchoesOfTheBrokenSun/Public/EchoesGlassScarDressingPack.h",
]


def run(repo_root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(EMITTER), "--repo-root", str(repo_root), *args],
        capture_output=True,
        text=True,
        check=False,
    )


class DressingPackHeaderTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = Path(tempfile.mkdtemp(prefix="echoes-dressing-header-"))
        for relative in RELATIVE:
            destination = self.temp / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, destination)

    def tearDown(self) -> None:
        shutil.rmtree(self.temp, ignore_errors=True)

    def _rewrite_dressing_pack(self, mutate) -> None:
        pack_path = self.temp / RELATIVE[0]
        pack = json.loads(pack_path.read_text(encoding="utf-8"))
        mutate(pack)
        raw = json.dumps(pack, indent=2, sort_keys=True).encode("utf-8") + b"\n"
        pack_path.write_bytes(raw)
        (self.temp / RELATIVE[1]).write_text(hashlib.sha256(raw).hexdigest() + "\n", encoding="utf-8")

    def test_checked_in_header_matches_the_frozen_packs(self) -> None:
        result = run(ROOT, "--check")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("header matches packs", result.stdout)

    def test_stdout_render_is_byte_identical_to_the_checked_in_header(self) -> None:
        result = run(ROOT, "--stdout")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, HEADER.read_text(encoding="utf-8"))
        again = run(ROOT, "--stdout")
        self.assertEqual(result.stdout, again.stdout)

    def test_header_pins_both_digests(self) -> None:
        header = HEADER.read_text(encoding="utf-8")
        self.assertIn(DRESSING_SIDECAR.read_text(encoding="utf-8").strip(), header)
        self.assertIn(BASE_SIDECAR.read_text(encoding="utf-8").strip(), header)
        self.assertIn("kRecordCount = 29", header)

    def test_record_on_passable_ground_is_refused_without_writing(self) -> None:
        base = json.loads(BASE_PACK.read_text(encoding="utf-8"))
        mask = base["cells"]["movement_mask"]
        passable_cell = next(index for index, value in enumerate(mask) if (value & 1) == 1)

        def move_first_record(pack: dict) -> None:
            site = next(entry for entry in pack["sites"] if entry["id"] == "glass-scar")
            record = site["records"][0]
            record["x"] = passable_cell % 64
            record["y"] = passable_cell // 64
            record["cell_index"] = passable_cell

        self._rewrite_dressing_pack(move_first_record)
        before = (self.temp / RELATIVE[4]).read_bytes()
        result = run(self.temp, "--write")
        self.assertEqual(result.returncode, 1)
        self.assertIn("passable cell", result.stderr)
        self.assertEqual((self.temp / RELATIVE[4]).read_bytes(), before)

    def test_digest_drift_is_refused(self) -> None:
        (self.temp / RELATIVE[1]).write_text("0" * 64 + "\n", encoding="utf-8")
        result = run(self.temp, "--check")
        self.assertEqual(result.returncode, 1)
        self.assertIn("digest mismatch", result.stderr)

    def test_base_contract_drift_is_refused(self) -> None:
        def wrong_base(pack: dict) -> None:
            pack["base_contract"]["compiled_pack_sha256"] = "f" * 64

        self._rewrite_dressing_pack(wrong_base)
        result = run(self.temp, "--check")
        self.assertEqual(result.returncode, 1)
        self.assertIn("base_contract digest does not match", result.stderr)

    def test_stale_header_is_reported_by_check(self) -> None:
        header_path = self.temp / RELATIVE[4]
        header_path.write_text(header_path.read_text(encoding="utf-8") + "// drift\n", encoding="utf-8")
        result = run(self.temp, "--check")
        self.assertEqual(result.returncode, 1)
        self.assertIn("does not match", result.stderr)


if __name__ == "__main__":
    unittest.main()
