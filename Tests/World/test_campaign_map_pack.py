#!/usr/bin/env python3
"""Synthetic contract tests for the terrain-only campaign map compiler.

No checked-in campaign source is required: these tests create an isolated
fifteen-mission source tree and exercise the compiler's refusal behavior.
"""

from __future__ import annotations

import hashlib
import importlib.util
import json
import os
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
COMPILER_PATH = REPO_ROOT / "Content/World/Tools/compile_campaign_map_pack.py"


def load_compiler():
    spec = importlib.util.spec_from_file_location("compile_campaign_map_pack", COMPILER_PATH)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


COMPILER = load_compiler()


def encoded(value: object) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def write_json(path: Path, value: object) -> str:
    raw = encoded(value)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(raw)
    return hashlib.sha256(raw).hexdigest()


def source(mission_number: int, *, x: int | None = None) -> dict:
    mission = f"M{mission_number:02d}"
    x = mission_number if x is None else x
    return {
        "source_format": "echoes-campaign-map-source",
        "source_version": 1,
        "author": "Angelis Pseftis",
        "mission_code": mission,
        "map_id": f"campaign-map-{mission_number:02d}",
        "operation_mode": COMPILER.OPERATION_BY_MISSION[mission],
        "production_brief": {
            "mission_requirement": f"SPEC-MSN-{mission_number:03d}",
            "map_concepts_reference": f"MapConcepts#{mission}",
        },
        "grid": {"width_tiles": 64, "height_tiles": 64, "index_formula": "y*width+x", "coordinate_origin": "southwest"},
        "base_terrain": "open",
        "terrain_region_ops": [{"id": f"ridge-{mission_number:02d}", "op": "block", "x0": x, "x1": x, "y0": 31, "y1": 31}],
        "founding_doctrine_variants": [
            {"doctrine": "Harvest", "terrain_region_ops": [], "expected_blocked_cell_count": 1},
            {"doctrine": "Preserve", "terrain_region_ops": [], "expected_blocked_cell_count": 1},
            {"doctrine": "Reshape", "terrain_region_ops": [], "expected_blocked_cell_count": 1},
        ],
        "required_clearance": [
            {"id": "deployment-footprint", "x": 2, "y": 2, "half_extent_raw": 1024},
            *([{"id": "future-well", "x": 32, "y": 32, "half_extent_raw": 1024}] if mission_number <= 9 else []),
        ],
        "required_passable": [{"id": "deployment", "x": 0, "y": 0}],
    }


class CampaignMapPackTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.source_dir = self.root / "Content/World/Source/Campaign"
        self.manifest_path = self.source_dir / "campaign_map_manifest_v1.json"
        self.rows: list[dict] = []
        for number in range(1, 16):
            rel = f"Content/World/Source/Campaign/m{number:02d}.json"
            digest = write_json(self.source_dir / f"m{number:02d}.json", source(number))
            self.rows.append({"mission_code": f"M{number:02d}", "map_id": f"campaign-map-{number:02d}", "operation_mode": COMPILER.OPERATION_BY_MISSION[f"M{number:02d}"], "source_path": rel, "source_sha256": digest})
        self.write_manifest()

    def tearDown(self) -> None:
        self.temp.cleanup()

    def write_manifest(self) -> None:
        write_json(self.manifest_path, {"manifest_format": "echoes-campaign-map-manifest", "manifest_version": 1, "author": "Angelis Pseftis", "mission_maps": self.rows})

    def refresh_row_hash(self, number: int) -> None:
        path = self.source_dir / f"m{number:02d}.json"
        self.rows[number - 1]["source_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
        self.write_manifest()

    def compile(self) -> dict[Path, bytes]:
        return COMPILER.compile_outputs(self.root, self.manifest_path, self.root / "Content/World/Generated/Campaign", self.root / "Content/World/Generated/Campaign/EchoesCampaignMapPack.h", self.root / "Content/World/Generated/Campaign/campaign_map_registry_v1.json")

    def test_deterministic_bytes_and_header_identity(self) -> None:
        first, second = self.compile(), self.compile()
        self.assertEqual(first, second)
        self.assertEqual(len(first), 17)
        header = next(content for path, content in first.items() if path.name.endswith(".h")).decode("utf-8")
        self.assertIn("std::array<MissionIdentity, 15>", header)
        self.assertIn("std::array<TerrainVariant, 45>", header)
        self.assertIn("kM01HarvestBlockedCells", header)
        self.assertIn("kHarvestMask", header)
        self.assertIn("std::array<MissionAnchor, 9>", header)
        for ordinal in range(1, 10):
            self.assertIn(f"{{{ordinal}, \"future-well\", 32, 32}}", header)
        self.assertNotIn("UObject", header)

    def test_write_then_check_and_stale_output_refusal(self) -> None:
        outputs = self.compile()
        COMPILER.atomic_write_all(outputs)
        self.assertEqual(COMPILER.main(["--root", str(self.root), "--check"]), 0)
        path = self.root / "Content/World/Generated/Campaign/m01_campaign-map-01_v1.json"
        path.write_text("stale", encoding="utf-8")
        self.assertEqual(COMPILER.main(["--root", str(self.root), "--check"]), 2)
        self.assertEqual(COMPILER.main(["--root", str(self.root), "--write"]), 0)

    def test_hash_duplicate_identity_and_path_refusals(self) -> None:
        self.rows[0]["source_sha256"] = "0" * 64
        self.write_manifest()
        with self.assertRaisesRegex(COMPILER.CompileError, "SHA-256 mismatch"):
            self.compile()
        self.rows[0]["source_sha256"] = hashlib.sha256((self.source_dir / "m01.json").read_bytes()).hexdigest()
        self.rows[1]["map_id"] = self.rows[0]["map_id"]
        self.write_manifest()
        with self.assertRaisesRegex(COMPILER.CompileError, "duplicate map ID"):
            self.compile()
        self.rows[1]["map_id"] = "campaign-map-02"
        self.rows[1]["source_path"] = "Content/World/Source/Campaign/../../outside.json"
        self.write_manifest()
        with self.assertRaisesRegex(COMPILER.CompileError, "path must"):
            self.compile()
        self.rows[1]["source_path"] = "Content/World/Source/Campaign/link.json"
        os.symlink(self.source_dir / "m02.json", self.source_dir / "link.json")
        self.write_manifest()
        with self.assertRaisesRegex(COMPILER.CompileError, "symlink"):
            self.compile()

    def test_unknown_types_geometry_census_and_connectivity_refuse(self) -> None:
        path = self.source_dir / "m01.json"
        value = json.loads(path.read_text(encoding="utf-8"))
        value["unexpected"] = True
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "exact-key mismatch"):
            self.compile()
        value.pop("unexpected")
        value["grid"]["width_tiles"] = "64"
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "must be an integer"):
            self.compile()
        value["grid"]["width_tiles"] = 64
        value["terrain_region_ops"][0]["x1"] = 64
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "out of bounds"):
            self.compile()
        value["terrain_region_ops"] = [{"id": "wall", "op": "block", "x0": 32, "x1": 32, "y0": 0, "y1": 63}]
        for variant in value["founding_doctrine_variants"]:
            variant["expected_blocked_cell_count"] = 64
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "disconnected"):
            self.compile()
        for variant in value["founding_doctrine_variants"]:
            variant["expected_blocked_cell_count"] = 63
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "census"):
            self.compile()

    def test_duplicate_complete_layout_refuses(self) -> None:
        second = source(2, x=1)
        write_json(self.source_dir / "m02.json", second)
        self.refresh_row_hash(2)
        with self.assertRaisesRegex(COMPILER.CompileError, "same complete terrain layout"):
            self.compile()

    def test_start_scenario_local_force_footprint_is_validated(self) -> None:
        path = self.source_dir / "m01.json"
        value = json.loads(path.read_text(encoding="utf-8"))
        value["terrain_region_ops"].append(
            {"id": "blocked-local-core", "op": "block", "x0": 10, "x1": 10, "y0": 10, "y1": 10}
        )
        for variant in value["founding_doctrine_variants"]:
            variant["expected_blocked_cell_count"] = 2
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "StartScenario local command-core footprint"):
            self.compile()

    def test_start_scenario_opponent_full_footprint_is_validated(self) -> None:
        path = self.source_dir / "m01.json"
        value = json.loads(path.read_text(encoding="utf-8"))
        # 51,54 is within the catalog 5x5 command-core footprint at 54,54.
        value["terrain_region_ops"].append(
            {"id": "blocked-opponent-core-edge", "op": "block", "x0": 51, "x1": 51, "y0": 54, "y1": 54}
        )
        for variant in value["founding_doctrine_variants"]:
            variant["expected_blocked_cell_count"] = 2
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "opponent command-core footprint"):
            self.compile()

    def test_start_scenario_barracks_footprint_is_validated(self) -> None:
        path = self.source_dir / "m01.json"
        value = json.loads(path.read_text(encoding="utf-8"))
        # 48,54 is inside the 4x4 opposing barracks footprint at 50,54,
        # while remaining outside the opposing command-core footprint.
        value["terrain_region_ops"].append(
            {"id": "blocked-opponent-barracks-edge", "op": "block", "x0": 48, "x1": 48, "y0": 54, "y1": 54}
        )
        for variant in value["founding_doctrine_variants"]:
            variant["expected_blocked_cell_count"] = 2
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "opponent barracks footprint"):
            self.compile()

    def test_required_clearance_footprint_is_validated(self) -> None:
        path = self.source_dir / "m01.json"
        value = json.loads(path.read_text(encoding="utf-8"))
        value["terrain_region_ops"].append(
            {"id": "blocked-semantic-footprint", "op": "block", "x0": 1, "x1": 1, "y0": 2, "y1": 2}
        )
        for variant in value["founding_doctrine_variants"]:
            variant["expected_blocked_cell_count"] = 2
        write_json(path, value)
        self.refresh_row_hash(1)
        with self.assertRaisesRegex(COMPILER.CompileError, "required footprint"):
            self.compile()


if __name__ == "__main__":
    unittest.main()
