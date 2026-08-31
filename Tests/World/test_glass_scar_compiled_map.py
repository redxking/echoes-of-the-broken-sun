#!/usr/bin/env python3
"""Adversarial source-only tests for the Glass Scar compiled-map contract."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[2]
SOURCE_PATH = ROOT / "Content/World/Source/GlassScar/glass_scar_map_source_v2.json"
SOURCE_SCHEMA_PATH = ROOT / "Content/World/Schema/glass_scar_map_source_v2.schema.json"
COMPILED_SCHEMA_PATH = ROOT / "Content/World/Schema/compiled_map_pack_v1.schema.json"
FIXTURE_PATH = (
    ROOT
    / "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
)
DIGEST_PATH = FIXTURE_PATH.with_suffix(".sha256")
COMPILER_PATH = ROOT / "Content/World/Tools/compile_map_pack.py"
BASE_COMMIT = "309e3547c8fc1116c2c5fb4914e1ba12303e0c7b"

SPEC = importlib.util.spec_from_file_location("compile_map_pack", COMPILER_PATH)
assert SPEC is not None and SPEC.loader is not None
COMPILER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COMPILER)

V1_HASHES = {
    "Content/World/Schema/glass_scar_map_pack.schema.json": (
        "36197c15a980663370eb9b50e6991bef1065b2f30ce09218ce725746b3fdb83d"
    ),
    "Content/World/Source/GlassScar/glass_scar_map_pack_v1.json": (
        "394e71070af09b22eb4acc72ddc145cf6758d4b8e658932637fd012718181d3e"
    ),
    "Content/World/Source/GlassScar/glass_scar_map_pack_v1.sha256": (
        "41f5b6393730666316aaa774fa999fb695f4d99e2ed62faeecd9c3a1d4b7fc10"
    ),
    "Tests/World/test_glass_scar_map_pack.py": (
        "6210fac13aae02560cf9afe66fed029e60232955780886715aadec98d3fafd6b"
    ),
}


def reverse_object_keys(value):
    if isinstance(value, dict):
        return {
            key: reverse_object_keys(value[key])
            for key in reversed(list(value.keys()))
        }
    if isinstance(value, list):
        return [reverse_object_keys(item) for item in value]
    return value


class GlassScarCompiledMapTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = COMPILER.strict_json_load(SOURCE_PATH)
        cls.source_schema = COMPILER.strict_json_load(SOURCE_SCHEMA_PATH)
        cls.compiled_schema = COMPILER.strict_json_load(COMPILED_SCHEMA_PATH)
        cls.fixture = COMPILER.strict_json_load(FIXTURE_PATH)

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="echoes-world-v2-")
        self.temp_root = Path(self.temporary.name)
        self.temp_source = self.temp_root / "source.json"
        self.temp_output = self.temp_root / "compiled.json"
        self.temp_digest = self.temp_root / "compiled.sha256"

    def tearDown(self):
        self.temporary.cleanup()

    def write_source(self, value, *, sort_keys=False):
        self.temp_source.write_text(
            json.dumps(value, ensure_ascii=False, sort_keys=sort_keys),
            encoding="utf-8",
        )

    def assert_source_invalid(self, value, expected):
        self.write_source(value)
        with self.assertRaisesRegex(COMPILER.MapContractError, expected):
            COMPILER.build_pack(self.temp_source, ROOT)

    def assert_compiled_invalid(self, value, expected):
        with self.assertRaisesRegex(COMPILER.MapContractError, expected):
            COMPILER.validate_compiled_pack(value)

    def test_draft_2020_12_schemas_and_instances_are_valid(self):
        Draft202012Validator.check_schema(self.source_schema)
        Draft202012Validator.check_schema(self.compiled_schema)
        Draft202012Validator(self.source_schema).validate(self.source)
        Draft202012Validator(self.compiled_schema).validate(self.fixture)

    def test_accepted_v1_files_remain_byte_identical(self):
        for relative_path, expected_digest in V1_HASHES.items():
            with self.subTest(path=relative_path):
                current = (ROOT / relative_path).read_bytes()
                baseline = subprocess.run(
                    ["git", "show", f"{BASE_COMMIT}:{relative_path}"],
                    cwd=ROOT,
                    check=True,
                    capture_output=True,
                ).stdout
                self.assertEqual(current, baseline)
                self.assertEqual(hashlib.sha256(current).hexdigest(), expected_digest)

    def test_fixture_is_reproducible_idempotent_and_digest_pinned(self):
        fixture_bytes = FIXTURE_PATH.read_bytes()
        self.assertEqual(fixture_bytes, COMPILER.canonical_bytes(self.fixture))
        expected_digest = DIGEST_PATH.read_text(encoding="ascii").strip()
        self.assertRegex(expected_digest, r"^[0-9a-f]{64}$")
        self.assertEqual(hashlib.sha256(fixture_bytes).hexdigest(), expected_digest)
        self.assertEqual(
            hashlib.sha256(COMPILER.canonical_bytes(self.source)).hexdigest(),
            self.fixture["source_contract"]["authoring_canonical_sha256"],
        )

        first = self.temp_root / "first.json"
        second = self.temp_root / "second.json"
        first_digest = COMPILER.compile_pack(SOURCE_PATH, first, project_root=ROOT)
        second_digest = COMPILER.compile_pack(SOURCE_PATH, second, project_root=ROOT)
        self.assertEqual(first.read_bytes(), fixture_bytes)
        self.assertEqual(second.read_bytes(), fixture_bytes)
        self.assertEqual(first_digest, expected_digest)
        self.assertEqual(second_digest, expected_digest)
        self.assertEqual(
            first.with_suffix(".sha256").read_text(encoding="ascii"),
            f"{expected_digest}\n",
        )

        reordered_source = reverse_object_keys(self.source)
        self.write_source(reordered_source)
        reordered = self.temp_root / "reordered.json"
        reordered_digest = COMPILER.compile_pack(
            self.temp_source, reordered, project_root=ROOT
        )
        self.assertEqual(reordered.read_bytes(), fixture_bytes)
        self.assertEqual(reordered_digest, expected_digest)

        changed_source = copy.deepcopy(self.source)
        changed_source["regions"][3]["base_move_cost"] = 11
        self.write_source(changed_source)
        changed = self.temp_root / "changed.json"
        changed_digest = COMPILER.compile_pack(
            self.temp_source, changed, project_root=ROOT
        )
        self.assertNotEqual(changed.read_bytes(), fixture_bytes)
        self.assertNotEqual(changed_digest, expected_digest)

    def test_ambiguous_json_and_structural_drift_fail_without_output_changes(self):
        self.temp_output.write_bytes(b"retained-output")
        self.temp_digest.write_bytes(b"retained-digest")
        source_text = SOURCE_PATH.read_text(encoding="utf-8")
        duplicate = source_text.replace(
            '"map_id": "glass-scar",',
            '"map_id": "glass-scar",\n  "map_id": "duplicate",',
            1,
        )
        self.temp_source.write_text(duplicate, encoding="utf-8")
        with self.assertRaisesRegex(COMPILER.MapContractError, "duplicate object key"):
            COMPILER.compile_pack(
                self.temp_source,
                self.temp_output,
                self.temp_digest,
                project_root=ROOT,
            )
        self.assertEqual(self.temp_output.read_bytes(), b"retained-output")
        self.assertEqual(self.temp_digest.read_bytes(), b"retained-digest")

        for token in ("NaN", "Infinity", "-Infinity"):
            with self.subTest(token=token):
                ambiguous = source_text.replace(
                    '"schema_version": 2,', f'"schema_version": {token},', 1
                )
                self.temp_source.write_text(ambiguous, encoding="utf-8")
                with self.assertRaisesRegex(
                    COMPILER.MapContractError, "non-standard numeric constant"
                ):
                    COMPILER.build_pack(self.temp_source, ROOT)

        unknown = copy.deepcopy(self.source)
        unknown["unreviewed"] = True
        self.assert_source_invalid(unknown, "unknown fields: unreviewed")

        boolean_integer = copy.deepcopy(self.source)
        boolean_integer["regions"][0]["base_move_cost"] = True
        self.assert_source_invalid(boolean_integer, "must be an integer")

        floating_integer = copy.deepcopy(self.source)
        floating_integer["regions"][0]["base_move_cost"] = 10.0
        self.assert_source_invalid(floating_integer, "must be an integer")

    def test_output_transaction_and_authoritative_inputs_are_protected(self):
        self.temp_output.write_bytes(b"retained-output")
        self.temp_digest.write_bytes(b"retained-digest")

        with self.assertRaisesRegex(COMPILER.MapContractError, "must be distinct"):
            COMPILER.compile_pack(
                SOURCE_PATH,
                self.temp_output,
                self.temp_output,
                project_root=ROOT,
            )
        self.assertEqual(self.temp_output.read_bytes(), b"retained-output")

        digest_directory = self.temp_root / "digest-directory"
        digest_directory.mkdir()
        with self.assertRaisesRegex(COMPILER.MapContractError, "regular file"):
            COMPILER.compile_pack(
                SOURCE_PATH,
                self.temp_output,
                digest_directory,
                project_root=ROOT,
            )
        self.assertEqual(self.temp_output.read_bytes(), b"retained-output")

        controlled_cli_failure = subprocess.run(
            [
                sys.executable,
                str(COMPILER_PATH),
                "--output",
                str(self.temp_output),
                "--digest-output",
                str(digest_directory),
            ],
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(controlled_cli_failure.returncode, 2)
        self.assertIn("Map contract validation failed", controlled_cli_failure.stderr)
        self.assertNotIn("Traceback", controlled_cli_failure.stderr)
        self.assertEqual(self.temp_output.read_bytes(), b"retained-output")

        original_replace = COMPILER.os.replace

        def fail_second_replacement(source, destination):
            source_path = Path(source)
            destination_path = Path(destination)
            if (
                source_path.name.startswith(f".{self.temp_digest.name}.staged.")
                and destination_path == self.temp_digest
            ):
                raise OSError("injected digest replacement failure")
            return original_replace(source, destination)

        with (
            mock.patch.object(
                COMPILER.os,
                "replace",
                side_effect=fail_second_replacement,
            ),
            self.assertRaisesRegex(
                COMPILER.MapContractError, "transactional write failed"
            ),
        ):
            COMPILER.compile_pack(
                SOURCE_PATH,
                self.temp_output,
                self.temp_digest,
                project_root=ROOT,
            )
        self.assertEqual(self.temp_output.read_bytes(), b"retained-output")
        self.assertEqual(self.temp_digest.read_bytes(), b"retained-digest")
        self.assertFalse(list(self.temp_root.glob(".*.staged.*")))
        self.assertFalse(list(self.temp_root.glob(".*.backup.*")))

        base_descriptor = ROOT / COMPILER.BASE_DESCRIPTOR_PATH
        protected_inputs = (SOURCE_PATH, base_descriptor, base_descriptor.with_suffix(".sha256"))
        protected_bytes = {path: path.read_bytes() for path in protected_inputs}
        for index, protected in enumerate(protected_inputs):
            with (
                self.subTest(protected=protected, destination="output"),
                self.assertRaisesRegex(COMPILER.MapContractError, "must not overwrite"),
            ):
                COMPILER.compile_pack(
                    SOURCE_PATH,
                    protected,
                    self.temp_root / f"protected-{index}.sha256",
                    project_root=ROOT,
                )
            with (
                self.subTest(protected=protected, destination="digest"),
                self.assertRaisesRegex(COMPILER.MapContractError, "must not overwrite"),
            ):
                COMPILER.compile_pack(
                    SOURCE_PATH,
                    self.temp_root / f"protected-{index}.json",
                    protected,
                    project_root=ROOT,
                )
        self.assertEqual(
            {path: path.read_bytes() for path in protected_inputs},
            protected_bytes,
        )

    def test_base_binding_grid_and_coverage_fail_closed(self):
        bad_digest = copy.deepcopy(self.source)
        bad_digest["base_descriptor"]["canonical_sha256"] = "0" * 64
        self.assert_source_invalid(bad_digest, "must equal")

        missing_base = copy.deepcopy(self.source)
        missing_base["base_descriptor"]["path"] = "Content/World/missing.json"
        self.assert_source_invalid(missing_base, "must equal")

        traversal = copy.deepcopy(self.source)
        traversal["base_descriptor"]["path"] = "../outside.json"
        self.assert_source_invalid(traversal, "must equal")

        wrong_grid = copy.deepcopy(self.source)
        wrong_grid["grid"]["width_tiles"] = 63
        self.assert_source_invalid(wrong_grid, "accepted Glass Scar grid")

        overlap = copy.deepcopy(self.source)
        overlap["blocked_zones"][0]["rectangle"]["min_x"] = 7
        self.assert_source_invalid(overlap, "stable blocked-zone geometry")

        gap = copy.deepcopy(self.source)
        gap["blocked_zones"][0]["rectangle"]["min_x"] = 9
        self.assert_source_invalid(gap, "stable blocked-zone geometry")

        reordered_regions = copy.deepcopy(self.source)
        reordered_regions["regions"][0], reordered_regions["regions"][1] = (
            reordered_regions["regions"][1],
            reordered_regions["regions"][0],
        )
        self.assert_source_invalid(reordered_regions, "ordinals must be contiguous")

        swapped_corridors = copy.deepcopy(self.source)
        west = swapped_corridors["regions"][2]
        east = swapped_corridors["regions"][6]
        west["rectangle"], east["rectangle"] = east["rectangle"], west["rectangle"]
        west["seed_tile"], east["seed_tile"] = east["seed_tile"], west["seed_tile"]
        self.assert_source_invalid(swapped_corridors, "stable region geometry")

    def test_movement_cost_and_height_contract_fail_closed(self):
        unknown_class = copy.deepcopy(self.source)
        unknown_class["regions"][0]["movement_classes"] = ["missing"]
        self.assert_source_invalid(unknown_class, "unknown movement class")

        empty_class = copy.deepcopy(self.source)
        empty_class["regions"][0]["movement_classes"] = []
        self.assert_source_invalid(empty_class, "must contain at least one")

        zero_cost = copy.deepcopy(self.source)
        zero_cost["regions"][0]["base_move_cost"] = 0
        self.assert_source_invalid(zero_cost, "must be at least 1")

        unknown_height = copy.deepcopy(self.source)
        unknown_height["regions"][0]["height_band_id"] = "missing"
        self.assert_source_invalid(unknown_height, "unknown height band")

        out_of_range_height = copy.deepcopy(self.source)
        out_of_range_height["height_bands"][0]["relative_level"] = -9
        self.assert_source_invalid(out_of_range_height, "must be at least -8")

        extra_movement = copy.deepcopy(self.source)
        extra_movement["movement_classes"].append(
            {"id": "phase", "ordinal": 1, "bit": 1}
        )
        self.assert_source_invalid(extra_movement, "requires exactly ground")

    def test_region_and_portal_identity_and_endpoint_failures_are_rejected(self):
        duplicate_region = copy.deepcopy(self.source)
        duplicate_region["regions"][1]["id"] = "south-basin"
        self.assert_source_invalid(duplicate_region, "duplicate id")

        unknown_region = copy.deepcopy(self.source)
        unknown_region["portals"][0]["region_ids"][1] = "missing"
        self.assert_source_invalid(unknown_region, "stable portal identity")

        same_region = copy.deepcopy(self.source)
        same_region["portals"][0]["region_ids"] = ["south-basin", "south-basin"]
        self.assert_source_invalid(same_region, "stable portal identity")

        diagonal = copy.deepcopy(self.source)
        diagonal["portals"][0]["edge_pairs"][0] = [[0, 29], [1, 30]]
        self.assert_source_invalid(diagonal, "cardinally adjacent")

        blocked_endpoint = copy.deepcopy(self.source)
        blocked_endpoint["portals"][2]["edge_pairs"][0] = [[12, 29], [8, 30]]
        self.assert_source_invalid(blocked_endpoint, "cardinally adjacent|do not match")

        duplicate_edge = copy.deepcopy(self.source)
        duplicate_edge["portals"][0]["edge_pairs"][1] = copy.deepcopy(
            duplicate_edge["portals"][0]["edge_pairs"][0]
        )
        self.assert_source_invalid(duplicate_edge, "duplicate portal edge")

        truncated = copy.deepcopy(self.source)
        truncated["portals"][0]["edge_pairs"].pop()
        self.assert_source_invalid(truncated, "exactly declare every")

        reversed_edges = copy.deepcopy(self.source)
        reversed_edges["portals"][0]["edge_pairs"].reverse()
        self.assert_source_invalid(reversed_edges, "strictly ordered")

        swapped_portal_identities = copy.deepcopy(self.source)
        first = swapped_portal_identities["portals"][0]
        second = swapped_portal_identities["portals"][2]
        first["region_ids"], second["region_ids"] = second["region_ids"], first["region_ids"]
        first["edge_pairs"], second["edge_pairs"] = second["edge_pairs"], first["edge_pairs"]
        self.assert_source_invalid(swapped_portal_identities, "stable portal identity")

    def test_objective_fallback_and_camera_failures_are_rejected(self):
        blocked_primary = copy.deepcopy(self.source)
        blocked_primary["objectives"][0]["primary_tile"] = [25, 32]
        self.assert_source_invalid(blocked_primary, "accepted v1 Future Well")

        duplicate_fallback = copy.deepcopy(self.source)
        duplicate_fallback["objectives"][0]["fallbacks"][1]["tile"] = [31, 32]
        self.assert_source_invalid(duplicate_fallback, "must be unique")

        wrong_region = copy.deepcopy(self.source)
        wrong_region["objectives"][0]["fallbacks"][0]["region_id"] = "north-basin"
        self.assert_source_invalid(wrong_region, "does not match")

        reserved = copy.deepcopy(self.source)
        reserved["objectives"][0]["fallbacks"][0] = {
            "ordinal": 0,
            "tile": [33, 22],
            "region_id": "south-basin",
        }
        self.assert_source_invalid(reserved, "fixed deployment or resource")

        out_of_bounds = copy.deepcopy(self.source)
        out_of_bounds["objectives"][0]["fallbacks"][0]["tile"] = [64, 32]
        self.assert_source_invalid(out_of_bounds, "must be at most 63")

        inverted = copy.deepcopy(self.source)
        inverted["camera_bounds"]["min_x"] = 63
        inverted["camera_bounds"]["max_x_exclusive"] = 1
        self.assert_source_invalid(inverted, "minimum must be less")

        inclusive_max = copy.deepcopy(self.source)
        inclusive_max["camera_bounds"]["max_x_exclusive"] = 63
        self.assert_source_invalid(inclusive_max, "contain every passable")

        outside = copy.deepcopy(self.source)
        outside["camera_bounds"]["max_x_exclusive"] = 65
        self.assert_source_invalid(outside, "must be at most 64")

    def test_compiled_cell_layout_counts_and_sentinels_are_exact(self):
        pack = self.fixture
        cells = pack["cells"]
        self.assertEqual(pack["grid"]["cell_count"], 4096)
        for name in (
            "movement_mask",
            "base_move_cost",
            "height_band_ordinal",
            "region_ordinal",
        ):
            self.assertEqual(len(cells[name]), 4096)

        self.assertEqual(COMPILER.tile_index((0, 0), 64), 0)
        self.assertEqual(COMPILER.tile_index((63, 0), 64), 63)
        self.assertEqual(COMPILER.tile_index((0, 1), 64), 64)
        self.assertEqual(COMPILER.tile_index((63, 63), 64), 4095)

        expected = {
            (10, 10): (1, 10, 1, 1),
            (54, 54): (1, 10, 1, 2),
            (7, 32): (1, 10, 0, 3),
            (12, 30): (1, 10, 0, 4),
            (32, 32): (1, 10, 0, 5),
            (49, 32): (1, 10, 0, 6),
            (56, 32): (1, 10, 0, 7),
            (8, 30): (0, 0, 0, 0),
        }
        for tile, values in expected.items():
            index = COMPILER.tile_index(tile, 64)
            with self.subTest(tile=tile):
                self.assertEqual(
                    (
                        cells["movement_mask"][index],
                        cells["base_move_cost"][index],
                        cells["height_band_ordinal"][index],
                        cells["region_ordinal"][index],
                    ),
                    values,
                )

        counts = {
            record["id"]: record["cell_count"] for record in pack["regions"]
        }
        self.assertEqual(counts, COMPILER.REQUIRED_REGION_COUNTS)
        self.assertEqual(cells["region_ordinal"].count(0), 165)
        self.assertEqual(sum(value != 0 for value in cells["region_ordinal"]), 3931)
        self.assertEqual(len(pack["portals"]), 10)
        self.assertEqual(
            sum(len(portal["edge_index_pairs"]) for portal in pack["portals"]),
            62,
        )

    def test_all_five_corridors_and_weighted_reachability_are_explicit(self):
        pack = self.fixture
        regions = {record["id"]: record for record in pack["regions"]}
        south = regions["south-basin"]["ordinal"]
        north = regions["north-basin"]["ordinal"]
        local_core = COMPILER.tile_index((10, 10), 64)
        opponent_core = COMPILER.tile_index((54, 54), 64)
        future_well = COMPILER.tile_index((32, 32), 64)

        self.assertEqual(COMPILER.shortest_cost(pack, local_core, future_well), 440)
        self.assertEqual(COMPILER.shortest_cost(pack, opponent_core, future_well), 440)
        self.assertEqual(COMPILER.shortest_cost(pack, local_core, opponent_core), 880)

        constrained_core_costs = {
            "west-edge-corridor": 940,
            "ash-cut": 880,
            "buried-causeway": 880,
            "folded-verge": 880,
            "east-edge-corridor": 920,
        }
        for corridor_id, (south_tile, north_tile) in COMPILER.CORRIDOR_ANCHORS.items():
            corridor = regions[corridor_id]["ordinal"]
            with self.subTest(corridor=corridor_id):
                self.assertEqual(
                    COMPILER.shortest_cost(
                        pack,
                        COMPILER.tile_index(south_tile, 64),
                        COMPILER.tile_index(north_tile, 64),
                        allowed_region_ordinals={south, north, corridor},
                    ),
                    60,
                )
                self.assertEqual(
                    COMPILER.shortest_cost(
                        pack,
                        local_core,
                        opponent_core,
                        allowed_region_ordinals={south, north, corridor},
                    ),
                    constrained_core_costs[corridor_id],
                )

        self.assertIn("west-edge-south", {item["id"] for item in pack["portals"]})
        self.assertIn("west-edge-north", {item["id"] for item in pack["portals"]})
        self.assertIn("east-edge-south", {item["id"] for item in pack["portals"]})
        self.assertIn("east-edge-north", {item["id"] for item in pack["portals"]})

    def test_compiled_mutations_fail_closed(self):
        wrong_base_digest = copy.deepcopy(self.fixture)
        wrong_base_digest["source_contract"]["base_descriptor_canonical_sha256"] = (
            "0" * 64
        )
        self.assert_compiled_invalid(wrong_base_digest, "must equal")

        extra_movement = copy.deepcopy(self.fixture)
        extra_movement["movement_classes"].append(
            {"id": "phase", "ordinal": 1, "bit": 1, "mask": 2}
        )
        extra_movement["cells"]["movement_mask"][0] = 3
        self.assert_compiled_invalid(extra_movement, "requires exactly ground")

        cost_catalog_drift = copy.deepcopy(self.fixture)
        cost_catalog_drift["cells"]["base_move_cost"][0] = 11
        self.assert_compiled_invalid(cost_catalog_drift, "match its region catalog")

        height_catalog_drift = copy.deepcopy(self.fixture)
        height_catalog_drift["cells"]["height_band_ordinal"][0] = 0
        self.assert_compiled_invalid(height_catalog_drift, "match its region catalog")

        blocked_height_drift = copy.deepcopy(self.fixture)
        blocked_index = COMPILER.tile_index((8, 30), 64)
        blocked_height_drift["cells"]["height_band_ordinal"][blocked_index] = 1
        self.assert_compiled_invalid(blocked_height_drift, "blocked-zone catalog")

        wrong_objective_type = copy.deepcopy(self.fixture)
        wrong_objective_type["objectives"][0]["type"] = "other"
        self.assert_compiled_invalid(wrong_objective_type, "must equal")

        wrong_objective_identity = copy.deepcopy(self.fixture)
        wrong_objective_identity["objectives"][0]["id"] = "other"
        self.assert_compiled_invalid(wrong_objective_identity, "Future Well identity")

        short_array = copy.deepcopy(self.fixture)
        short_array["cells"]["movement_mask"].pop()
        self.assert_compiled_invalid(short_array, "exactly 4096 entries")

        unknown_mask = copy.deepcopy(self.fixture)
        unknown_mask["cells"]["movement_mask"][0] = 2
        self.assert_compiled_invalid(unknown_mask, "undeclared movement bit")

        passable_zero_cost = copy.deepcopy(self.fixture)
        passable_zero_cost["cells"]["base_move_cost"][0] = 0
        self.assert_compiled_invalid(passable_zero_cost, "positive cost")

        blocked_positive_cost = copy.deepcopy(self.fixture)
        blocked_positive_cost["cells"]["base_move_cost"][blocked_index] = 1
        self.assert_compiled_invalid(blocked_positive_cost, "zero mask, zero cost")

        unknown_height = copy.deepcopy(self.fixture)
        unknown_height["cells"]["height_band_ordinal"][0] = 99
        self.assert_compiled_invalid(unknown_height, "unknown height band")

        unknown_region = copy.deepcopy(self.fixture)
        unknown_region["cells"]["region_ordinal"][0] = 99
        self.assert_compiled_invalid(unknown_region, "stable region geometry")

        boolean_cell = copy.deepcopy(self.fixture)
        boolean_cell["cells"]["movement_mask"][0] = True
        self.assert_compiled_invalid(boolean_cell, "must be an integer")

        transposed = copy.deepcopy(self.fixture)
        for name in (
            "movement_mask",
            "base_move_cost",
            "height_band_ordinal",
            "region_ordinal",
        ):
            original = self.fixture["cells"][name]
            transposed["cells"][name] = [
                original[x * 64 + y] for y in range(64) for x in range(64)
            ]
        self.assert_compiled_invalid(transposed, "stable region geometry")

        disconnected = copy.deepcopy(self.fixture)
        south_cell = COMPILER.tile_index((0, 0), 64)
        north_cell = COMPILER.tile_index((0, 35), 64)
        disconnected["cells"]["region_ordinal"][south_cell] = 2
        disconnected["cells"]["region_ordinal"][north_cell] = 1
        self.assert_compiled_invalid(disconnected, "stable region geometry")

        missing_portal_edge = copy.deepcopy(self.fixture)
        missing_portal_edge["portals"][0]["edge_index_pairs"].pop()
        self.assert_compiled_invalid(missing_portal_edge, "do not match region boundaries")

        swapped_portal_identities = copy.deepcopy(self.fixture)
        first = swapped_portal_identities["portals"][0]
        second = swapped_portal_identities["portals"][2]
        first["region_ordinals"], second["region_ordinals"] = (
            second["region_ordinals"],
            first["region_ordinals"],
        )
        first["edge_index_pairs"], second["edge_index_pairs"] = (
            second["edge_index_pairs"],
            first["edge_index_pairs"],
        )
        self.assert_compiled_invalid(swapped_portal_identities, "stable portal identity")

        diagonal_portal = copy.deepcopy(self.fixture)
        diagonal_portal["portals"][0]["edge_index_pairs"][0] = [1856, 1921]
        self.assert_compiled_invalid(diagonal_portal, "cardinally adjacent")

        blocked_objective = copy.deepcopy(self.fixture)
        blocked_objective["objectives"][0]["primary_index"] = blocked_index
        blocked_objective["objectives"][0]["primary_region_ordinal"] = 0
        self.assert_compiled_invalid(blocked_objective, "accepted v1 Future Well")

        camera_off_by_one = copy.deepcopy(self.fixture)
        camera_off_by_one["camera_bounds"]["max_x_exclusive"] = 63
        self.assert_compiled_invalid(camera_off_by_one, "contain every passable")

    def test_claim_boundary_and_output_are_non_runtime_source_data(self):
        pack = self.fixture
        self.assertEqual(pack["authority"], "checked-in-source-fixture")
        self.assertEqual(pack["runtime_binding"], "none")
        self.assertTrue(pack["claim_boundary"]["proposed_contract"])
        self.assertTrue(pack["claim_boundary"]["compiled_fixture_is_source_data"])
        self.assertEqual(
            pack["claim_boundary"]["not_evidence_for"],
            COMPILER.CLAIM_EXCLUSIONS,
        )
        fixture_text = FIXTURE_PATH.read_text(encoding="utf-8")
        self.assertNotIn("/Volumes/", fixture_text)
        self.assertNotIn("timestamp", fixture_text.lower())
        self.assertNotIn("worktree", fixture_text.lower())


if __name__ == "__main__":
    unittest.main(verbosity=2)
