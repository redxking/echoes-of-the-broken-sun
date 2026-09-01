#!/usr/bin/env python3
"""Deterministic, source-only assurance checks for the Glass Scar descriptor.

These checks qualify the descriptor and its stated topology. They do not exercise
the Unreal runtime, navigation, AI, rendered content, or packaged builds.
"""

from collections import deque
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DESCRIPTOR_PATH = (
    PROJECT_ROOT
    / "Content"
    / "World"
    / "Source"
    / "GlassScar"
    / "glass_scar_map_pack_v1.json"
)
SCHEMA_PATH = (
    PROJECT_ROOT
    / "Content"
    / "World"
    / "Schema"
    / "glass_scar_map_pack.schema.json"
)
DIGEST_PATH = DESCRIPTOR_PATH.with_suffix(".sha256")


def strict_json_loads(text):
    """Load interoperable JSON while rejecting ambiguous/non-standard input."""

    def reject_duplicate_keys(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("duplicate JSON object key: " + key)
            result[key] = value
        return result

    def reject_nonstandard_constant(value):
        raise ValueError("non-standard JSON numeric constant: " + value)

    return json.loads(
        text,
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonstandard_constant,
    )


def strict_json_load(path):
    return strict_json_loads(path.read_text(encoding="utf-8"))


def canonical_bytes(value):
    """Apply the descriptor's echoes-json-v1 canonicalization profile."""

    return json.dumps(
        value,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    ).encode("utf-8")


def reverse_object_key_order(value):
    """Recursively reorder object keys without changing array semantics."""

    if isinstance(value, dict):
        return {
            key: reverse_object_key_order(value[key])
            for key in reversed(list(value.keys()))
        }
    if isinstance(value, list):
        return [reverse_object_key_order(item) for item in value]
    return value


def json_values_equal(left, right):
    """Compare JSON values without treating booleans as integers."""

    if isinstance(left, bool) or isinstance(right, bool):
        return isinstance(left, bool) and isinstance(right, bool) and left == right
    if isinstance(left, (int, float)) and isinstance(right, (int, float)):
        return left == right
    if type(left) is not type(right):
        return False
    if isinstance(left, dict):
        return set(left) == set(right) and all(
            json_values_equal(left[key], right[key]) for key in left
        )
    if isinstance(left, list):
        return len(left) == len(right) and all(
            json_values_equal(left_item, right_item)
            for left_item, right_item in zip(left, right)
        )
    return left == right


class JsonSchemaSubsetValidator:
    """Validate the Draft 2020-12 keywords used by the checked-in schema.

    The test stays dependency-free so it can run with the system Python. Any new
    schema keyword must be implemented here before this validator will accept it.
    """

    SUPPORTED_KEYWORDS = {
        "$id",
        "$ref",
        "$schema",
        "$defs",
        "additionalProperties",
        "const",
        "enum",
        "items",
        "maximum",
        "maxItems",
        "minimum",
        "minItems",
        "properties",
        "prefixItems",
        "required",
        "title",
        "type",
        "uniqueItems",
    }

    def __init__(self, root_schema):
        self.root_schema = root_schema

    def validate(self, instance, schema=None, path="$"):
        schema = self.root_schema if schema is None else schema
        unsupported = set(schema) - self.SUPPORTED_KEYWORDS
        if unsupported:
            self._fail(path, "unsupported schema keyword(s): " + ", ".join(sorted(unsupported)))

        if "$ref" in schema:
            target = self._resolve_local_reference(schema["$ref"], path)
            self.validate(instance, target, path)
            return

        expected_type = schema.get("type")
        if expected_type is not None and not self._matches_type(instance, expected_type):
            self._fail(path, "expected type " + expected_type)

        if "const" in schema and not json_values_equal(instance, schema["const"]):
            self._fail(path, "does not match const value")

        if "enum" in schema and not any(
            json_values_equal(instance, allowed) for allowed in schema["enum"]
        ):
            self._fail(path, "is not an allowed enum value")

        if isinstance(instance, int) and not isinstance(instance, bool):
            if "minimum" in schema and instance < schema["minimum"]:
                self._fail(path, "is below minimum")
            if "maximum" in schema and instance > schema["maximum"]:
                self._fail(path, "is above maximum")

        if isinstance(instance, list):
            if "minItems" in schema and len(instance) < schema["minItems"]:
                self._fail(path, "has too few items")
            if "maxItems" in schema and len(instance) > schema["maxItems"]:
                self._fail(path, "has too many items")
            if schema.get("uniqueItems"):
                for index, item in enumerate(instance):
                    if any(
                        json_values_equal(item, prior)
                        for prior in instance[:index]
                    ):
                        self._fail(path, "contains duplicate items")
            if "items" in schema:
                for index, item in enumerate(instance):
                    self.validate(item, schema["items"], f"{path}[{index}]")
            for index, child_schema in enumerate(schema.get("prefixItems", [])):
                if index < len(instance):
                    self.validate(instance[index], child_schema, f"{path}[{index}]")

        if isinstance(instance, dict):
            required = schema.get("required", [])
            missing = [key for key in required if key not in instance]
            if missing:
                self._fail(path, "missing required key(s): " + ", ".join(missing))

            properties = schema.get("properties", {})
            if schema.get("additionalProperties") is False:
                extras = set(instance) - set(properties)
                if extras:
                    self._fail(path, "unexpected key(s): " + ", ".join(sorted(extras)))

            for key, child_schema in properties.items():
                if key in instance:
                    self.validate(instance[key], child_schema, f"{path}.{key}")

    def _resolve_local_reference(self, reference, path):
        if not reference.startswith("#/"):
            self._fail(path, "only local JSON Pointer references are supported")
        target = self.root_schema
        for raw_token in reference[2:].split("/"):
            token = raw_token.replace("~1", "/").replace("~0", "~")
            if not isinstance(target, dict) or token not in target:
                self._fail(path, "unresolvable schema reference " + reference)
            target = target[token]
        return target

    @staticmethod
    def _matches_type(instance, expected_type):
        if expected_type == "object":
            return isinstance(instance, dict)
        if expected_type == "array":
            return isinstance(instance, list)
        if expected_type == "integer":
            return isinstance(instance, int) and not isinstance(instance, bool)
        if expected_type == "string":
            return isinstance(instance, str)
        if expected_type == "boolean":
            return isinstance(instance, bool)
        raise AssertionError("unsupported JSON Schema type: " + expected_type)

    @staticmethod
    def _fail(path, message):
        raise AssertionError(f"schema validation failed at {path}: {message}")


class GlassScarMapPackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.descriptor = strict_json_load(DESCRIPTOR_PATH)
        cls.schema = strict_json_load(SCHEMA_PATH)
        cls.width = cls.descriptor["grid"]["width_tiles"]
        cls.height = cls.descriptor["grid"]["height_tiles"]

        scar = cls.descriptor["terrain"]["glass_scar_bounds"]
        cls.scar_tiles = cls.rectangle_tiles(scar)
        cls.crossing_tiles = set()
        for route in cls.descriptor["routes"]:
            cls.crossing_tiles.update(cls.rectangle_tiles(route["crossing"]))
        cls.blocked_tiles = cls.scar_tiles - cls.crossing_tiles

    @staticmethod
    def rectangle_tiles(rectangle):
        return {
            (x, y)
            for x in range(rectangle["min_x"], rectangle["max_x"] + 1)
            for y in range(rectangle["min_y"], rectangle["max_y"] + 1)
        }

    @classmethod
    def is_passable(cls, tile):
        x, y = tile
        return (
            0 <= x < cls.width
            and 0 <= y < cls.height
            and tile not in cls.blocked_tiles
        )

    @classmethod
    def distance(cls, start, goal):
        start = tuple(start)
        goal = tuple(goal)
        if not cls.is_passable(start) or not cls.is_passable(goal):
            raise AssertionError("distance endpoint is not passable")

        frontier = deque([(start, 0)])
        visited = {start}
        while frontier:
            tile, distance = frontier.popleft()
            if tile == goal:
                return distance
            x, y = tile
            for neighbor in ((x, y + 1), (x + 1, y), (x, y - 1), (x - 1, y)):
                if neighbor not in visited and cls.is_passable(neighbor):
                    visited.add(neighbor)
                    frontier.append((neighbor, distance + 1))
        raise AssertionError(f"no passable cardinal route from {start} to {goal}")

    def test_descriptor_conforms_to_checked_in_schema(self):
        JsonSchemaSubsetValidator(self.schema).validate(self.descriptor)

    def test_json_loader_rejects_ambiguous_input(self):
        with self.assertRaisesRegex(ValueError, "duplicate JSON object key"):
            strict_json_loads('{"map_id":"glass-scar","map_id":"other"}')
        for constant in ("NaN", "Infinity", "-Infinity"):
            with self.subTest(constant=constant):
                with self.assertRaisesRegex(ValueError, "non-standard JSON numeric constant"):
                    strict_json_loads('{"value":' + constant + "}")

    def test_schema_rejects_structural_mutations(self):
        validator = JsonSchemaSubsetValidator(self.schema)

        mutations = []
        missing_required = deepcopy(self.descriptor)
        del missing_required["map_id"]
        mutations.append(missing_required)

        unknown_property = deepcopy(self.descriptor)
        unknown_property["unreviewed"] = True
        mutations.append(unknown_property)

        invalid_route_name = deepcopy(self.descriptor)
        invalid_route_name["routes"][2]["display_name"] = " Folded Verge"
        mutations.append(invalid_route_name)

        duplicate_route = deepcopy(self.descriptor)
        duplicate_route["routes"][1] = deepcopy(duplicate_route["routes"][0])
        mutations.append(duplicate_route)

        mismatched_route_identity = deepcopy(self.descriptor)
        mismatched_route_identity["routes"][0]["display_name"] = "Folded Verge"
        mutations.append(mismatched_route_identity)

        reference_outside_crossing = deepcopy(self.descriptor)
        reference_outside_crossing["routes"][0]["reference_tile"] = [63, 63]
        mutations.append(reference_outside_crossing)

        inverted_crossing = deepcopy(self.descriptor)
        inverted_crossing["routes"][0]["crossing"]["min_x"] = 16
        mutations.append(inverted_crossing)

        duplicate_spawn = deepcopy(self.descriptor)
        duplicate_spawn["deployment"]["local_spawns"][1] = duplicate_spawn[
            "deployment"
        ]["local_spawns"][0]
        mutations.append(duplicate_spawn)

        out_of_bounds = deepcopy(self.descriptor)
        out_of_bounds["resources"]["matter_deposits"][0][0] = 64
        mutations.append(out_of_bounds)

        boolean_as_integer = deepcopy(self.descriptor)
        boolean_as_integer["claim_boundary"]["descriptor_only"] = 1
        mutations.append(boolean_as_integer)

        for index, mutation in enumerate(mutations):
            with self.subTest(mutation=index):
                with self.assertRaisesRegex(AssertionError, "schema validation failed"):
                    validator.validate(mutation)

    def test_source_snapshot_manifest_matches_git_objects(self):
        snapshot = self.descriptor["source_snapshot"]
        self.assertRegex(snapshot["git_commit"], re.compile(r"^[0-9a-f]{40}$"))
        expected_files = {
            "Source/EchoesOfTheBrokenSun/Public/EchoesSkirmishSetup.h": (
                "b1be786a06c4e96821d625d7fc4823dba47ff8d83532194cc9763ab8762a625f",
                "grid-dimensions",
            ),
            "Source/EchoesOfTheBrokenSun/Private/EchoesSkirmishSetup.cpp": (
                "c3e30f06a8a9abfd6033b5f7c1bec2841fd26594280e889cd392a1758eed3ab4",
                "base-topology-spawns-resources-objective",
            ),
            "Source/EchoesOfTheBrokenSun/Private/EchoesSimulationSubsystem.cpp": (
                "563f3b3f4f10e0b25723e9088b4992c44dbd73d5d9db1e8c2e60d2ff8fa8de44",
                "legacy-base-terrain-construction",
            ),
            "Source/EchoesOfTheBrokenSun/Private/EchoesGameMode.cpp": (
                "2e1165cc1feb75a8de8e30c67c5f575704c504e0eebbcff02a7766c9f6e8f3a2",
                "route-presentation-centers",
            ),
            "Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h": (
                "6ec25ed394637092bb5f145dcbdb920aa0c438e21da49d893136c33c9d87f3d4",
                "presentation-tile-scale",
            ),
            "Source/EchoesOfTheBrokenSun/Private/EchoesContentSubsystem.cpp": (
                "433c6c0d99cc061310e1a72229018b399afd97ee2c28c74676199b05903e8774",
                "simulation-centimeter-conversion",
            ),
        }
        actual_files = {
            item["path"]: (item["sha256"], item["role"])
            for item in snapshot["files"]
        }
        self.assertEqual(actual_files, expected_files)

        for relative_path, (declared_digest, _) in actual_files.items():
            with self.subTest(path=relative_path):
                self.assertRegex(declared_digest, re.compile(r"^[0-9a-f]{64}$"))
                snapshot_bytes = subprocess.run(
                    [
                        "git",
                        "show",
                        snapshot["git_commit"] + ":" + relative_path,
                    ],
                    cwd=PROJECT_ROOT,
                    check=True,
                    capture_output=True,
                ).stdout
                self.assertEqual(
                    hashlib.sha256(snapshot_bytes).hexdigest(),
                    declared_digest,
                    "manifest digest does not match the named git object",
                )

    def test_descriptor_matches_current_cpp_topology(self):
        setup_header = (
            PROJECT_ROOT
            / "Source/EchoesOfTheBrokenSun/Public/EchoesSkirmishSetup.h"
        ).read_text(encoding="utf-8")
        setup_source = (
            PROJECT_ROOT
            / "Source/EchoesOfTheBrokenSun/Private/EchoesSkirmishSetup.cpp"
        ).read_text(encoding="utf-8")
        simulation_source = (
            PROJECT_ROOT
            / "Source/EchoesOfTheBrokenSun/Private/EchoesSimulationSubsystem.cpp"
        ).read_text(encoding="utf-8")
        game_mode_source = (
            PROJECT_ROOT
            / "Source/EchoesOfTheBrokenSun/Private/EchoesGameMode.cpp"
        ).read_text(encoding="utf-8")
        simulation_header = (
            PROJECT_ROOT
            / "Source/EchoesOfTheBrokenSun/Public/EchoesSimulationSubsystem.h"
        ).read_text(encoding="utf-8")
        content_source = (
            PROJECT_ROOT
            / "Source/EchoesOfTheBrokenSun/Private/EchoesContentSubsystem.cpp"
        ).read_text(encoding="utf-8")

        width = re.search(r"MapWidthTiles\s*=\s*(\d+);", setup_header)
        height = re.search(r"MapHeightTiles\s*=\s*(\d+);", setup_header)
        self.assertIsNotNone(width)
        self.assertIsNotNone(height)
        self.assertEqual(int(width.group(1)), self.descriptor["grid"]["width_tiles"])
        self.assertEqual(int(height.group(1)), self.descriptor["grid"]["height_tiles"])

        blocked_function = setup_source[
            setup_source.index("FEchoesSkirmishSetupModel::IsBlockedTile"):
            setup_source.index("FEchoesSkirmishSetupModel::ExpectedBlockedTileCount")
        ]
        scar = re.search(
            r"bInScar\s*=\s*TileY\s*>=\s*(\d+)\s*&&\s*TileY\s*<=\s*(\d+)\s*&&\s*"
            r"TileX\s*>=\s*(\d+)\s*&&\s*TileX\s*<=\s*(\d+)",
            blocked_function,
        )
        self.assertIsNotNone(scar)
        min_y, max_y, min_x, max_x = map(int, scar.groups())
        self.assertEqual(
            self.descriptor["terrain"]["glass_scar_bounds"],
            {"min_x": min_x, "max_x": max_x, "min_y": min_y, "max_y": max_y},
        )
        crossing = re.search(
            r"bCrossing\s*=\s*\(TileX\s*>=\s*(\d+)\s*&&\s*TileX\s*<=\s*(\d+)\)\s*\|\|\s*"
            r"\(TileX\s*>=\s*(\d+)\s*&&\s*TileX\s*<=\s*(\d+)\)\s*\|\|\s*"
            r"\(TileX\s*>=\s*(\d+)\s*&&\s*TileX\s*<=\s*(\d+)\)",
            blocked_function,
        )
        self.assertIsNotNone(crossing)
        crossing_ranges = list(zip(
            map(int, crossing.groups()[::2]),
            map(int, crossing.groups()[1::2]),
        ))
        self.assertEqual(
            crossing_ranges,
            [
                (route["crossing"]["min_x"], route["crossing"]["max_x"])
                for route in self.descriptor["routes"]
            ],
        )
        for route in self.descriptor["routes"]:
            self.assertEqual(route["crossing"]["min_y"], min_y)
            self.assertEqual(route["crossing"]["max_y"], max_y)

        def extract_glass_scar_points(function_name):
            match = re.search(
                re.escape("FEchoesSkirmishSetupModel::" + function_name)
                + r"\s*\(.*?case\s+EEchoesSkirmishMapPreset::GlassScar:\s*"
                + r"return\s*\{(?P<body>.*?)\};",
                setup_source,
                re.DOTALL,
            )
            self.assertIsNotNone(match, function_name)
            return [
                [int(x), int(y)]
                for x, y in re.findall(r"\{\s*(-?\d+)\s*,\s*(-?\d+)\s*\}", match.group("body"))
            ]

        well = re.search(
            r"FEchoesSkirmishSetupModel::FutureWellTile\s*\(.*?"
            r"case\s+EEchoesSkirmishMapPreset::GlassScar:\s*"
            r"return\s*\{\s*(-?\d+)\s*,\s*(-?\d+)\s*\};",
            setup_source,
            re.DOTALL,
        )
        self.assertIsNotNone(well)
        self.assertEqual(
            self.descriptor["objectives"]["future_well"],
            [int(well.group(1)), int(well.group(2))],
        )
        self.assertEqual(
            self.descriptor["deployment"]["local_spawns"],
            extract_glass_scar_points("LocalSpawnTiles"),
        )
        self.assertEqual(
            self.descriptor["deployment"]["opponent_spawns"],
            extract_glass_scar_points("OpponentSpawnTiles"),
        )
        self.assertEqual(
            self.descriptor["resources"]["matter_deposits"],
            extract_glass_scar_points("ResourceNodeTiles"),
        )

        legacy_terrain = simulation_source[
            simulation_source.index("bool IsGlassScarCrossing"):
            simulation_source.index("int32 ConfigureSkirmishTerrain")
        ]
        legacy_crossings = [
            (int(min_x), int(max_x))
            for min_x, max_x in re.findall(
                r"TileX\s*>=\s*(\d+)\s*&&\s*TileX\s*<=\s*(\d+)",
                legacy_terrain[
                    :legacy_terrain.index("int32 ConfigureGlassScar")
                ],
            )
        ]
        self.assertEqual(legacy_crossings, crossing_ranges)
        legacy_bounds = re.search(
            r"for\s*\(int32\s+TileY\s*=\s*(\d+);\s*TileY\s*<=\s*(\d+);.*?"
            r"for\s*\(int32\s+TileX\s*=\s*(\d+);\s*TileX\s*<=\s*(\d+);",
            legacy_terrain,
            re.DOTALL,
        )
        self.assertIsNotNone(legacy_bounds)
        self.assertEqual(
            tuple(map(int, legacy_bounds.groups())),
            (min_y, max_y, min_x, max_x),
        )

        tile_world_size = re.search(
            r"TileWorldSize\s*=\s*([0-9.]+)f", simulation_header
        )
        self.assertIsNotNone(tile_world_size)
        presentation_scale = float(tile_world_size.group(1))
        self.assertEqual(
            presentation_scale,
            self.descriptor["grid"]["presentation_tile_world_units"],
        )

        route_block = re.search(
            r"const\s+FRouteSpec\s+Routes\[\]\s*=\s*\{(?P<body>.*?)\n\s*\};",
            game_mode_source,
            re.DOTALL,
        )
        self.assertIsNotNone(route_block)
        tags = {
            "ash-cut": "EchoesRouteAshCut",
            "buried-causeway": "EchoesRouteBuriedCauseway",
            "folded-verge": "EchoesRouteFoldedVerge",
        }
        presentation_centers = {
            tag: float(world_x)
            for world_x, tag in re.findall(
                r"\{[^{}]*?FVector\(\s*(-?[0-9.]+)f\s*,\s*0\.0f\s*,.*?"
                r"TEXT\(\"([^\"]+)\"\)\s*\}",
                route_block.group("body"),
                re.DOTALL,
            )
        }
        map_half_x = self.width * 0.5
        for route in self.descriptor["routes"]:
            tag = tags[route["id"]]
            self.assertIn(tag, presentation_centers)
            expected_world_x = (
                route["reference_tile"][0] - map_half_x
            ) * presentation_scale
            self.assertEqual(presentation_centers[tag], expected_world_x)

        movement_conversion = re.search(
            r"MovementDenominator\s*=\s*"
            r"static_cast<int64>\(TicksPerSecond\)\s*\*\s*(\d+);",
            content_source,
        )
        self.assertIsNotNone(movement_conversion)
        self.assertEqual(
            int(movement_conversion.group(1)),
            self.descriptor["grid"]["simulation_tile_centimeters"],
        )

    def test_canonical_digest_is_pinned_and_semantically_sensitive(self):
        declared_digest = DIGEST_PATH.read_text(encoding="ascii").strip()
        self.assertRegex(declared_digest, re.compile(r"^[0-9a-f]{64}$"))

        actual_digest = hashlib.sha256(canonical_bytes(self.descriptor)).hexdigest()
        self.assertEqual(declared_digest, actual_digest)

        reordered = reverse_object_key_order(self.descriptor)
        reordered_digest = hashlib.sha256(canonical_bytes(reordered)).hexdigest()
        self.assertEqual(actual_digest, reordered_digest)

        route_order_mutation = deepcopy(self.descriptor)
        route_order_mutation["routes"].reverse()
        self.assertNotEqual(
            actual_digest,
            hashlib.sha256(canonical_bytes(route_order_mutation)).hexdigest(),
        )

        coordinate_mutation = deepcopy(self.descriptor)
        coordinate_mutation["objectives"]["future_well"][0] -= 1
        self.assertNotEqual(
            actual_digest,
            hashlib.sha256(canonical_bytes(coordinate_mutation)).hexdigest(),
        )

    def test_topology_matches_the_source_snapshot(self):
        self.assertEqual(
            self.descriptor["terrain"],
            {
                "default_passable": True,
                "glass_scar_bounds": {
                    "min_x": 8,
                    "max_x": 55,
                    "min_y": 30,
                    "max_y": 34,
                },
                "blocked_rule": "inside-glass-scar-bounds-unless-inside-a-named-crossing",
            },
        )
        self.assertEqual(len(self.scar_tiles), 240)
        self.assertEqual(len(self.crossing_tiles), 75)
        self.assertEqual(len(self.blocked_tiles), 165)

        self.assertEqual(
            self.descriptor["routes"],
            [
                {
                    "id": "ash-cut",
                    "display_name": "Ash Cut",
                    "ordinal": 0,
                    "crossing": {"min_x": 12, "max_x": 15, "min_y": 30, "max_y": 34},
                    "reference_tile": [13, 32],
                },
                {
                    "id": "buried-causeway",
                    "display_name": "Buried Causeway",
                    "ordinal": 1,
                    "crossing": {"min_x": 29, "max_x": 35, "min_y": 30, "max_y": 34},
                    "reference_tile": [32, 32],
                },
                {
                    "id": "folded-verge",
                    "display_name": "Folded Verge",
                    "ordinal": 2,
                    "crossing": {"min_x": 48, "max_x": 51, "min_y": 30, "max_y": 34},
                    "reference_tile": [49, 32],
                },
            ],
        )

        self.assertEqual(
            self.descriptor["deployment"]["local_spawns"],
            [
                [10, 10], [14, 10], [6, 17], [8, 13], [11, 14], [14, 12],
                [8, 8], [12, 7], [16, 10], [7, 6], [15, 6], [6, 11],
            ],
        )
        self.assertEqual(
            self.descriptor["deployment"]["opponent_spawns"],
            [
                [54, 54], [50, 54], [58, 48], [51, 53], [54, 50], [57, 52],
                [50, 57], [54, 58], [57, 58], [49, 58], [58, 53],
            ],
        )
        self.assertEqual(
            self.descriptor["resources"]["matter_deposits"],
            [[16, 16], [21, 13], [25, 28], [33, 22], [31, 43], [43, 36], [47, 50], [52, 45]],
        )
        self.assertEqual(self.descriptor["objectives"]["future_well"], [32, 32])

    def test_mandatory_tiles_are_unique_passable_and_reachable(self):
        deployment = self.descriptor["deployment"]
        mandatory = (
            deployment["local_spawns"]
            + deployment["opponent_spawns"]
            + self.descriptor["resources"]["matter_deposits"]
            + [self.descriptor["objectives"]["future_well"]]
        )
        mandatory_tiles = [tuple(tile) for tile in mandatory]
        self.assertEqual(len(mandatory_tiles), 32)
        self.assertEqual(len(mandatory_tiles), len(set(mandatory_tiles)))

        local_core = tuple(self.descriptor["measurement_checkpoints"]["local_core"])
        for tile in mandatory_tiles:
            with self.subTest(tile=tile):
                self.assertTrue(self.is_passable(tile))
                self.assertGreaterEqual(self.distance(local_core, tile), 0)

    def test_crossings_are_usable_and_distances_are_deterministic(self):
        checkpoints = self.descriptor["measurement_checkpoints"]
        local_core = tuple(checkpoints["local_core"])
        opponent_core = tuple(checkpoints["opponent_core"])
        future_well = tuple(checkpoints["future_well"])

        self.assertEqual(local_core, tuple(self.descriptor["deployment"]["local_spawns"][0]))
        self.assertEqual(opponent_core, tuple(self.descriptor["deployment"]["opponent_spawns"][0]))
        self.assertEqual(future_well, tuple(self.descriptor["objectives"]["future_well"]))

        for route in self.descriptor["routes"]:
            crossing = route["crossing"]
            crossing_tiles = self.rectangle_tiles(crossing)
            reference = tuple(route["reference_tile"])
            with self.subTest(route=route["id"]):
                self.assertTrue(all(self.is_passable(tile) for tile in crossing_tiles))
                self.assertIn(reference, crossing_tiles)
                self.assertEqual(
                    reference,
                    tuple(checkpoints["crossing_reference_tiles"][route["id"]]),
                )
                below = (reference[0], crossing["min_y"] - 1)
                above = (reference[0], crossing["max_y"] + 1)
                self.assertEqual(self.distance(below, above), 6)
                self.assertGreaterEqual(self.distance(local_core, reference), 0)
                self.assertGreaterEqual(self.distance(opponent_core, reference), 0)

        self.assertEqual(self.distance(local_core, future_well), 44)
        self.assertEqual(self.distance(opponent_core, future_well), 44)
        self.assertEqual(self.distance(local_core, opponent_core), 88)

        # These are sums of shortest cardinal graph distances through each
        # declared reference tile. They are not measured travel times.
        expected_via_reference = {
            "ash-cut": (50, 88, 88),
            "buried-causeway": (44, 44, 88),
            "folded-verge": (84, 50, 88),
        }
        # The minimum-touch metric constrains a path-distance sum to touch any
        # tile in the named crossing. It deliberately does not claim that the
        # resulting path traverses the Scar through that crossing.
        expected_minimum_touch = {
            "ash-cut": (46, 80, 88),
            "buried-causeway": (44, 44, 88),
            "folded-verge": (78, 46, 88),
        }
        # Full traversal constrains the path to enter the named crossing from
        # one side of the Scar and exit on the other before reaching the goal.
        expected_full_traversal = {
            "ash-cut": (50, 84, 88),
            "buried-causeway": (50, 50, 88),
            "folded-verge": (82, 50, 88),
        }
        for route in self.descriptor["routes"]:
            reference = tuple(route["reference_tile"])
            crossing_tiles = self.rectangle_tiles(route["crossing"])
            actual_via_reference = (
                self.distance(local_core, reference) + self.distance(reference, future_well),
                self.distance(opponent_core, reference) + self.distance(reference, future_well),
                self.distance(local_core, reference) + self.distance(reference, opponent_core),
            )
            actual_minimum_touch = (
                min(
                    self.distance(local_core, tile) + self.distance(tile, future_well)
                    for tile in crossing_tiles
                ),
                min(
                    self.distance(opponent_core, tile) + self.distance(tile, future_well)
                    for tile in crossing_tiles
                ),
                min(
                    self.distance(local_core, tile) + self.distance(tile, opponent_core)
                    for tile in crossing_tiles
                ),
            )
            crossing = route["crossing"]
            south_to_north = []
            north_to_south = []
            local_to_opponent = []
            for x in range(crossing["min_x"], crossing["max_x"] + 1):
                south = (x, crossing["min_y"] - 1)
                north = (x, crossing["max_y"] + 1)
                crossing_distance = self.distance(south, north)
                south_to_north.append(
                    self.distance(local_core, south)
                    + crossing_distance
                    + self.distance(north, future_well)
                )
                north_to_south.append(
                    self.distance(opponent_core, north)
                    + crossing_distance
                    + self.distance(south, future_well)
                )
                local_to_opponent.append(
                    self.distance(local_core, south)
                    + crossing_distance
                    + self.distance(north, opponent_core)
                )
            actual_full_traversal = (
                min(south_to_north),
                min(north_to_south),
                min(local_to_opponent),
            )
            with self.subTest(route=route["id"]):
                self.assertEqual(
                    actual_via_reference,
                    expected_via_reference[route["id"]],
                )
                self.assertEqual(
                    actual_minimum_touch,
                    expected_minimum_touch[route["id"]],
                )
                self.assertEqual(
                    actual_full_traversal,
                    expected_full_traversal[route["id"]],
                )

    def test_scale_and_claim_boundary_remain_explicit(self):
        self.assertEqual(
            self.descriptor["grid"],
            {
                "width_tiles": 64,
                "height_tiles": 64,
                "coordinate_origin": "southwest",
                "simulation_tile_centimeters": 100,
                "presentation_tile_world_units": 200,
                "timing_status": "theoretical-only",
            },
        )
        self.assertEqual(self.descriptor["authority"], "non-authoritative-source-descriptor")
        self.assertEqual(self.descriptor["runtime_binding"], "none")
        self.assertTrue(self.descriptor["claim_boundary"]["descriptor_only"])
        self.assertEqual(
            set(self.descriptor["claim_boundary"]["not_evidence_for"]),
            {
                "runtime-authority",
                "playable-map",
                "rendered-map",
                "navigation-qualification",
                "ai-qualification",
                "performance-qualification",
                "packaged-build",
                "release-readiness",
            },
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
