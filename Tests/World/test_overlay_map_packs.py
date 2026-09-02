#!/usr/bin/env python3
"""Standalone assurance tests for the compiled overlay/preset map pack.

Independent of the compiler's own rectangle-op evaluation, these tests mirror
the runtime terrain predicates verbatim (EchoesSimulationSubsystem.cpp and
EchoesSkirmishSetup.cpp at commit 6fa2d6f, which added the Reshape well spur) and require the compiled fixture to
match them cell-for-cell, so a transcription error in the authoring rects and
a drift in the checked-in fixture both fail loudly.

Dependency-free by design (standard library only); the optional Draft 2020-12
schema validation runs when ``jsonschema`` is importable and is otherwise
reported as an explicit skip, never silently.
"""

from __future__ import annotations

import hashlib
import importlib.util
import json
import unittest
from collections import deque
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
PACK_PATH = REPO_ROOT / "Content/World/Generated/Overlays/overlay_map_packs_v1.json"
SIDECAR_PATH = REPO_ROOT / "Content/World/Generated/Overlays/overlay_map_packs_v1.sha256"
SCHEMA_PATH = REPO_ROOT / "Content/World/Schema/overlay_map_pack_v1.schema.json"
COMPILER_PATH = REPO_ROOT / "Content/World/Tools/compile_overlay_pack.py"
BASE_PACK_PATH = (
    REPO_ROOT / "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
)

GRID = 64
CELLS = GRID * GRID

EXPECTED_CENSUS = {
    "lume-reach-harvest": 223,
    "lume-reach-preserve": 223,
    "lume-reach-reshape": 223,
    "seven-accounts-harvest": 200,
    "seven-accounts-preserve": 165,
    "seven-accounts-reshape": 145,
    "skirmish-crownfall-basin": 282,
    "skirmish-soryn-confluence": 200,
    "unburied-road-harvest": 220,
    "unburied-road-preserve": 205,
    "unburied-road-reshape": 217,
}
EXPECTED_BURIED_WELL_VARIANTS = {
    "seven-accounts-harvest",
    "unburied-road-harvest",
}

# Owner ruling 2026-09-02 (DEMO-NAR-011): Reshape is temporary and
# non-destructive, so unburied-road-reshape keeps a dead-end southern access
# spur to the intact Future Well. The runtime repair landed as commit
# 6fa2d6f1355687da990b57fca6c620c3e18866c1 ("Open the reshaped well spur and
# record naming adoptions"), so the runtime mirror below includes the spur and
# the contract and runtime agree cell-for-cell with NO tolerated divergence.
WELL_ACCESS_SPUR = {30 * GRID + 32, 31 * GRID + 32, 32 * GRID + 32}


def load_compiler():
    spec = importlib.util.spec_from_file_location(
        "compile_overlay_pack", COMPILER_PATH
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


# --- Verbatim runtime predicate mirrors (commit 6fa2d6f) ---------------------


def is_glass_scar_crossing(x: int) -> bool:
    return (12 <= x <= 15) or (29 <= x <= 35) or (48 <= x <= 51)


def base_glass_scar_blocked() -> set[int]:
    return {
        y * GRID + x
        for y in range(30, 35)
        for x in range(8, 56)
        if not is_glass_scar_crossing(x)
    }


def unburied_road_blocked(branch: str) -> set[int]:
    blocked = base_glass_scar_blocked()
    for y in range(30, 35):
        for x in range(8, 56):
            western = 12 <= x <= 15
            central = 29 <= x <= 35
            eastern = 48 <= x <= 51
            selected = (
                (branch == "harvest" and western)
                or (branch == "preserve" and central)
                or (branch == "reshape" and eastern)
            )
            if (western or central or eastern) and not selected:
                blocked.add(y * GRID + x)
    if branch == "reshape":
        # Well-access spur re-opened by the runtime repair at 6fa2d6f.
        for y in range(30, 33):
            blocked.discard(y * GRID + 32)
    return blocked


def seven_accounts_blocked(branch: str) -> set[int]:
    blocked = base_glass_scar_blocked()
    if branch == "harvest":
        for y in range(30, 35):
            for x in range(29, 36):
                blocked.add(y * GRID + x)
    elif branch == "reshape":
        for y in range(30, 35):
            for x in (27, 28, 36, 37):
                blocked.discard(y * GRID + x)
    return blocked


def lume_reach_blocked(branch: str) -> set[int]:
    blocked: set[int] = set()
    for y in range(28, 31):
        for x in range(8, 56):
            public_gate = (16 <= x <= 19) or (30 <= x <= 34) or (45 <= x <= 48)
            if not public_gate:
                blocked.add(y * GRID + x)
    for y in range(36, 45):
        for x in range(20, 26):
            blocked.add(y * GRID + x)
        for x in range(39, 45):
            blocked.add(y * GRID + x)
    if branch == "harvest":
        for y in range(34, 39):
            for x in (29, 30):
                blocked.add(y * GRID + x)
    elif branch == "preserve":
        for y in range(34, 39):
            for x in (33, 34):
                blocked.add(y * GRID + x)
    elif branch == "reshape":
        for y in range(34, 36):
            for x in range(30, 35):
                blocked.add(y * GRID + x)
    return blocked


def crownfall_basin_blocked() -> set[int]:
    blocked: set[int] = set()
    for y in range(GRID):
        for x in range(GRID):
            ridge = ((27 <= x <= 29) or (35 <= x <= 37)) and 6 <= y <= 57
            gate = (13 <= y <= 17) or (30 <= y <= 34) or (46 <= y <= 50)
            north_shelf = 39 <= y <= 41 and 10 <= x <= 22 and not (15 <= x <= 17)
            south_shelf = 22 <= y <= 24 and 42 <= x <= 54 and not (47 <= x <= 49)
            if (ridge and not gate) or north_shelf or south_shelf:
                blocked.add(y * GRID + x)
    return blocked


def soryn_confluence_blocked() -> set[int]:
    blocked: set[int] = set()
    for y in range(GRID):
        for x in range(GRID):
            outer_horizontal = y in (19, 20, 43, 44) and 20 <= x <= 43
            outer_vertical = x in (20, 21, 42, 43) and 19 <= y <= 44
            north_south_gate = 30 <= x <= 33
            west_east_gate = 30 <= y <= 33
            ring = (outer_horizontal and not north_south_gate) or (
                outer_vertical and not west_east_gate
            )
            west_shard = 9 <= x <= 16 and 25 <= y <= 27
            east_shard = 47 <= x <= 54 and 36 <= y <= 38
            if ring or west_shard or east_shard:
                blocked.add(y * GRID + x)
    return blocked


RUNTIME_MIRRORS = {
    "unburied-road-harvest": lambda: unburied_road_blocked("harvest"),
    "unburied-road-preserve": lambda: unburied_road_blocked("preserve"),
    "unburied-road-reshape": lambda: unburied_road_blocked("reshape"),
    "seven-accounts-harvest": lambda: seven_accounts_blocked("harvest"),
    "seven-accounts-preserve": lambda: seven_accounts_blocked("preserve"),
    "seven-accounts-reshape": lambda: seven_accounts_blocked("reshape"),
    "lume-reach-harvest": lambda: lume_reach_blocked("harvest"),
    "lume-reach-preserve": lambda: lume_reach_blocked("preserve"),
    "lume-reach-reshape": lambda: lume_reach_blocked("reshape"),
    "skirmish-crownfall-basin": crownfall_basin_blocked,
    "skirmish-soryn-confluence": soryn_confluence_blocked,
}


def component_count(blocked: set[int]) -> int:
    seen = [False] * CELLS
    components = 0
    for start in range(CELLS):
        if seen[start] or start in blocked:
            continue
        components += 1
        queue = deque((start,))
        seen[start] = True
        while queue:
            index = queue.popleft()
            x, y = index % GRID, index // GRID
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < GRID and 0 <= ny < GRID:
                    neighbor = ny * GRID + nx
                    if not seen[neighbor] and neighbor not in blocked:
                        seen[neighbor] = True
                        queue.append(neighbor)
    return components


class OverlayMapPackTests(unittest.TestCase):
    pack_bytes: bytes
    pack: dict
    variants: dict

    @classmethod
    def setUpClass(cls) -> None:
        cls.pack_bytes = PACK_PATH.read_bytes()
        cls.pack = json.loads(cls.pack_bytes.decode("utf-8"))
        cls.variants = {v["id"]: v for v in cls.pack["variants"]}

    def test_sidecar_digest_matches_fixture_bytes(self) -> None:
        digest = hashlib.sha256(self.pack_bytes).hexdigest()
        self.assertEqual(digest, SIDECAR_PATH.read_text().strip())

    def test_recompilation_is_deterministic_and_matches_fixture(self) -> None:
        compiler = load_compiler()
        first = compiler.render_bytes(compiler.build_pack(REPO_ROOT))
        second = compiler.render_bytes(compiler.build_pack(REPO_ROOT))
        self.assertEqual(first, second)
        self.assertEqual(first, self.pack_bytes)

    def test_variant_inventory_and_censuses_are_pinned(self) -> None:
        self.assertEqual(sorted(self.variants), sorted(EXPECTED_CENSUS))
        for variant_id, expected in EXPECTED_CENSUS.items():
            variant = self.variants[variant_id]
            self.assertEqual(variant["blocked_cell_count"], expected, variant_id)
            self.assertEqual(
                variant["passable_cell_count"], CELLS - expected, variant_id
            )
            self.assertEqual(
                len(variant["blocked_cell_indices"]), expected, variant_id
            )

    def test_fixture_matches_runtime_predicate_mirrors_exactly(self) -> None:
        for variant_id, mirror in RUNTIME_MIRRORS.items():
            with self.subTest(variant=variant_id):
                expected = mirror()
                actual = set(self.variants[variant_id]["blocked_cell_indices"])
                self.assertEqual(actual, expected)

    def test_well_access_spur_contract(self) -> None:
        # Owner-ruled spur: passable in both contract and runtime mirror,
        # dead-ending north of the Well so the Folded Verge remains the only
        # interior through-route.
        blocked = set(self.variants["unburied-road-reshape"]["blocked_cell_indices"])
        runtime_blocked = unburied_road_blocked("reshape")
        for cell in WELL_ACCESS_SPUR:
            self.assertNotIn(cell, blocked)
            self.assertNotIn(cell, runtime_blocked)
        self.assertIn(33 * GRID + 32, blocked)
        self.assertIn(34 * GRID + 32, blocked)
        self.assertIn(33 * GRID + 32, runtime_blocked)
        self.assertIn(34 * GRID + 32, runtime_blocked)
        well = [
            s
            for s in self.variants["unburied-road-reshape"]["sites"]
            if s["id"] == "future-well"
        ]
        self.assertTrue(well and well[0]["passable"] and well[0]["reachable"])

    def test_skirmish_glass_scar_preset_equals_base_contract(self) -> None:
        base_pack = json.loads(BASE_PACK_PATH.read_text(encoding="utf-8"))
        base_blocked = {
            index
            for index, value in enumerate(base_pack["cells"]["movement_mask"])
            if (value & 1) == 0
        }
        self.assertEqual(base_blocked, base_glass_scar_blocked())

    def test_every_variant_is_single_component(self) -> None:
        for variant_id, variant in self.variants.items():
            with self.subTest(variant=variant_id):
                blocked = set(variant["blocked_cell_indices"])
                self.assertEqual(variant["passable_component_count"], 1)
                self.assertEqual(component_count(blocked), 1)

    def test_buried_well_facts_are_pinned(self) -> None:
        for variant_id, variant in self.variants.items():
            well = [s for s in variant["sites"] if s["id"] == "future-well"]
            if not well:
                continue
            with self.subTest(variant=variant_id):
                expected_buried = variant_id in EXPECTED_BURIED_WELL_VARIANTS
                self.assertEqual(well[0]["passable"], not expected_buried)

    def test_passable_sites_are_reachable(self) -> None:
        for variant_id, variant in self.variants.items():
            for site in variant["sites"]:
                with self.subTest(variant=variant_id, site=site["id"]):
                    if site["passable"]:
                        self.assertTrue(site["reachable"])

    def test_compiler_refuses_census_drift(self) -> None:
        compiler = load_compiler()
        source_path = (
            REPO_ROOT / "Content/World/Source/GlassScar/glass_scar_overlays_v1.json"
        )
        source, _ = compiler.load_strict_json(source_path)
        source["variants"][0]["expected_blocked_cells"] += 1
        base = compiler.load_base_blocked(REPO_ROOT)
        with self.assertRaises(compiler.CompileError):
            compiler.compile_variants(source, "mutated", base)

    def test_compiler_refuses_duplicate_keys_and_nonfinite(self) -> None:
        compiler = load_compiler()
        with self.assertRaises(compiler.CompileError):
            json.loads(
                '{"a":1,"a":2}', object_pairs_hook=compiler.reject_duplicate_keys
            )
        with self.assertRaises(compiler.CompileError):
            json.loads(
                '{"a":NaN}',
                object_pairs_hook=compiler.reject_duplicate_keys,
                parse_constant=compiler.reject_nonfinite,
            )

    def test_provenance_pins_sources_and_base(self) -> None:
        provenance = self.pack["source_provenance"]
        self.assertEqual(
            provenance["runtime_mirror_commit"],
            "6fa2d6f1355687da990b57fca6c620c3e18866c1",
        )
        for source in provenance["sources"]:
            path = REPO_ROOT / source["path"]
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            self.assertEqual(digest, source["raw_sha256"], source["path"])
        base = self.pack["base_contract"]
        base_digest = hashlib.sha256(BASE_PACK_PATH.read_bytes()).hexdigest()
        self.assertEqual(base_digest, base["compiled_pack_sha256"])

    def test_schema_document_parses(self) -> None:
        schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
        self.assertEqual(schema.get("$id"), "echoes.world.overlay-map-pack.v1")

    @unittest.skipUnless(
        importlib.util.find_spec("jsonschema") is not None,
        "jsonschema not installed on this machine (environment finding recorded "
        "2026-09-01); structural schema validation skipped EXPLICITLY, not passed",
    )
    def test_fixture_validates_against_schema(self) -> None:
        import jsonschema

        schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
        jsonschema.Draft202012Validator.check_schema(schema)
        jsonschema.Draft202012Validator(schema).validate(self.pack)


if __name__ == "__main__":
    unittest.main(verbosity=2)
