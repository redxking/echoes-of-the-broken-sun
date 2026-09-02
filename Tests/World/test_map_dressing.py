#!/usr/bin/env python3
"""Standalone assurance tests for the compiled map-dressing pack.

The load-bearing property is the conformance gate: dressing is presentation
data, so it must never imply a gameplay state the compiled map contract denies.
These tests re-derive cell states directly from the digest-pinned map pack —
independently of the compiler's own bookkeeping — and require every placed
record to agree with them, then probe the compiler with deliberate violations
to prove it refuses rather than warns.

Dependency-free by design (standard library only); optional Draft 2020-12
validation runs when ``jsonschema`` is importable and is otherwise reported as
an explicit skip, never silently passed.
"""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
SOURCE_PATH = REPO_ROOT / "Content/World/Source/GlassScar/glass_scar_dressing_v1.json"
PACK_PATH = REPO_ROOT / "Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.json"
SIDECAR_PATH = REPO_ROOT / "Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.sha256"
SCHEMA_PATH = REPO_ROOT / "Content/World/Schema/map_dressing_v1.schema.json"
COMPILER_PATH = REPO_ROOT / "Content/World/Tools/compile_dressing_pack.py"
MAP_PACK_PATH = (
    REPO_ROOT / "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
)

GRID = 64
EXPECTED_TOTAL_RECORDS = 29
EXPECTED_POPULATED_SITES = {"glass-scar"}
EXPECTED_EMPTY_SITES = {"crownfall-basin", "confluence-ring"}
# Recorded 2026-09-02: ridge geometry implies elevation the simulation does not
# model, so this class stays deferred until the elevation ruling lands. If it
# ever becomes active, that is a decision to make consciously, not by drift.
EXPECTED_DEFERRED_CLASSES = {"basin_ridge"}


def load_compiler():
    spec = importlib.util.spec_from_file_location("compile_dressing_pack", COMPILER_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def blocked_cells_from_map_pack() -> set[int]:
    pack = json.loads(MAP_PACK_PATH.read_text(encoding="utf-8"))
    return {
        index
        for index, value in enumerate(pack["cells"]["movement_mask"])
        if (value & 1) == 0
    }


class MapDressingTests(unittest.TestCase):
    pack_bytes: bytes
    pack: dict
    blocked: set

    @classmethod
    def setUpClass(cls) -> None:
        cls.pack_bytes = PACK_PATH.read_bytes()
        cls.pack = json.loads(cls.pack_bytes.decode("utf-8"))
        cls.blocked = blocked_cells_from_map_pack()

    def test_sidecar_digest_matches_pack_bytes(self) -> None:
        self.assertEqual(
            hashlib.sha256(self.pack_bytes).hexdigest(),
            SIDECAR_PATH.read_text().strip(),
        )

    def test_recompilation_is_deterministic_and_matches_fixture(self) -> None:
        compiler = load_compiler()
        first = compiler.render_bytes(compiler.compile_pack(REPO_ROOT))
        second = compiler.render_bytes(compiler.compile_pack(REPO_ROOT))
        self.assertEqual(first, second)
        self.assertEqual(first, self.pack_bytes)

    def test_base_contract_digest_is_the_live_map_pack(self) -> None:
        computed = hashlib.sha256(MAP_PACK_PATH.read_bytes()).hexdigest()
        self.assertEqual(computed, self.pack["base_contract"]["compiled_pack_sha256"])

    def test_source_provenance_digest_matches_source_file(self) -> None:
        computed = hashlib.sha256(SOURCE_PATH.read_bytes()).hexdigest()
        self.assertEqual(computed, self.pack["source_provenance"]["raw_sha256"])

    def test_every_record_conforms_to_the_gameplay_contract(self) -> None:
        """The load-bearing check: no record may imply a state the map denies."""
        states = {
            entry["id"]: entry["permitted_cell_state"]
            for entry in self.pack["dressing_classes"]
        }
        seen = 0
        for site in self.pack["sites"]:
            for record in site["records"]:
                with self.subTest(record=record["id"]):
                    index = record["y"] * GRID + record["x"]
                    self.assertEqual(index, record["cell_index"])
                    actual = "blocked" if index in self.blocked else "passable"
                    self.assertEqual(actual, states[record["class"]])
                    seen += 1
        self.assertEqual(seen, EXPECTED_TOTAL_RECORDS)

    def test_occluders_are_permitted_only_on_blocked_cells(self) -> None:
        for entry in self.pack["dressing_classes"]:
            if entry["occluder"]:
                self.assertEqual(entry["permitted_cell_state"], "blocked", entry["id"])

    def test_record_ids_and_counts_are_consistent(self) -> None:
        ids = [r["id"] for site in self.pack["sites"] for r in site["records"]]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertEqual(len(ids), self.pack["total_record_count"])
        self.assertEqual(len(ids), EXPECTED_TOTAL_RECORDS)
        for site in self.pack["sites"]:
            self.assertEqual(site["record_count"], len(site["records"]), site["id"])

    def test_site_vocabulary_status_is_pinned_with_reasons(self) -> None:
        populated = {s["id"] for s in self.pack["sites"] if s["vocabulary_status"] == "populated"}
        empty = {s["id"] for s in self.pack["sites"] if s["vocabulary_status"] == "empty"}
        self.assertEqual(populated, EXPECTED_POPULATED_SITES)
        self.assertEqual(empty, EXPECTED_EMPTY_SITES)
        for site in self.pack["sites"]:
            if site["vocabulary_status"] == "empty":
                self.assertEqual(site["records"], [])
                self.assertTrue(site["empty_reason"].strip())

    def test_ridge_class_remains_deferred(self) -> None:
        deferred = {entry["id"] for entry in self.pack["deferred_classes"]}
        active = {entry["id"] for entry in self.pack["dressing_classes"]}
        self.assertEqual(deferred, EXPECTED_DEFERRED_CLASSES)
        self.assertFalse(deferred & active)
        for entry in self.pack["deferred_classes"]:
            self.assertTrue(entry["deferred_reason"].strip())

    def test_runtime_binding_remains_none(self) -> None:
        self.assertEqual(self.pack["runtime_binding"], "none")
        self.assertTrue(self.pack["claim_boundary"]["proposed_contract"])

    def test_compiler_refuses_a_record_on_the_wrong_cell_state(self) -> None:
        compiler = load_compiler()
        source = json.loads(SOURCE_PATH.read_text(encoding="utf-8"))
        mutated = copy.deepcopy(source)
        # Move an occluder onto a passable cell: the exact failure the gate exists for.
        mutated["sites"][0]["records"][0]["x"] = 32
        mutated["sites"][0]["records"][0]["y"] = 32
        self._assert_source_refused(compiler, mutated)

    def test_compiler_refuses_unknown_keys_and_classes(self) -> None:
        compiler = load_compiler()
        source = json.loads(SOURCE_PATH.read_text(encoding="utf-8"))
        for mutate in (
            lambda d: d.update(extra_key=1),
            lambda d: d["sites"][0]["records"][0].update(extra="x"),
            lambda d: d["sites"][0]["records"][0].__setitem__("class", "no_such_class"),
            lambda d: d["sites"][0]["records"][0].__setitem__("orientation_ordinal", 4),
            lambda d: d["sites"][0]["records"][0].__setitem__("scale_band", 3),
            lambda d: d["sites"][0]["records"][0].__setitem__("x", 64),
            lambda d: d["sites"][1].__setitem__("records", [{"id": "x"}]),
            lambda d: d["base_contract"].__setitem__("compiled_pack_sha256", "0" * 64),
            lambda d: d["dressing_classes"][0].__setitem__("permitted_cell_state", "passable"),
        ):
            mutated = copy.deepcopy(source)
            mutate(mutated)
            with self.subTest(mutation=str(mutate)):
                self._assert_source_refused(compiler, mutated)

    def test_compiler_refuses_duplicate_record_ids(self) -> None:
        compiler = load_compiler()
        source = json.loads(SOURCE_PATH.read_text(encoding="utf-8"))
        mutated = copy.deepcopy(source)
        records = mutated["sites"][0]["records"]
        records[1]["id"] = records[0]["id"]
        self._assert_source_refused(compiler, mutated)

    def _assert_source_refused(self, compiler, mutated_source: dict) -> None:
        """Compile a mutated source from a scratch tree; require a refusal."""
        import tempfile

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "Content/World/Source/GlassScar").mkdir(parents=True)
            (root / "Content/World/Generated/GlassScar").mkdir(parents=True)
            (root / "Content/World/Source/GlassScar/glass_scar_dressing_v1.json").write_text(
                json.dumps(mutated_source), encoding="utf-8"
            )
            (
                root / "Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json"
            ).write_bytes(MAP_PACK_PATH.read_bytes())
            with self.assertRaises(compiler.CompileError):
                compiler.compile_pack(root)

    def test_schema_document_parses(self) -> None:
        schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
        self.assertEqual(schema.get("$id"), "echoes.world.map-dressing-pack.v1")

    @unittest.skipUnless(
        importlib.util.find_spec("jsonschema") is not None,
        "jsonschema not installed on this machine (environment finding recorded "
        "2026-09-01); structural schema validation skipped EXPLICITLY, not passed",
    )
    def test_pack_validates_against_schema(self) -> None:
        import jsonschema

        schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
        jsonschema.Draft202012Validator.check_schema(schema)
        jsonschema.Draft202012Validator(schema).validate(self.pack)


if __name__ == "__main__":
    unittest.main(verbosity=2)
