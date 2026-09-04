#!/usr/bin/env python3
"""Standalone assurance tests for Lume Reach compiled map-dressing pack.

Verifies:
1. Sidecar digest matches pack bytes.
2. Recompilation is deterministic and matches fixture.
3. Every placed record conforms to the blocked cells in overlay_map_packs_v1.json.
4. Occluders are permitted only on blocked cells (Passability Truth, REL-ART-026).
5. Compiler refuses records placed on passable cells.
6. Generated C++ header matches emission script.
"""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
SOURCE_PATH = REPO_ROOT / "Content/World/Source/LumeReach/lume_reach_dressing_v1.json"
PACK_PATH = REPO_ROOT / "Content/World/Generated/Dressing/lume_reach_dressing_pack_v1.json"
SIDECAR_PATH = REPO_ROOT / "Content/World/Generated/Dressing/lume_reach_dressing_pack_v1.sha256"
SCHEMA_PATH = REPO_ROOT / "Content/World/Schema/map_dressing_v1.schema.json"
COMPILER_PATH = REPO_ROOT / "Content/World/Tools/compile_dressing_pack.py"
HEADER_EMITTER_PATH = REPO_ROOT / "Content/World/Tools/emit_dressing_pack_header.py"
HEADER_PATH = REPO_ROOT / "Source/EchoesOfTheBrokenSun/Public/EchoesLumeReachDressingPack.h"
OVERLAYS_PATH = REPO_ROOT / "Content/World/Generated/Overlays/overlay_map_packs_v1.json"

GRID = 64
EXPECTED_TOTAL_RECORDS = 39
EXPECTED_POPULATED_SITES = {"lume-reach"}
EXPECTED_DEFERRED_CLASSES = {"elevation_causeway"}
EXPECTED_DRESSING_CLASSES = {"civic_frame", "conduit_pylon"}


def load_compiler():
    spec = importlib.util.spec_from_file_location("compile_dressing_pack", COMPILER_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_emitter():
    spec = importlib.util.spec_from_file_location("emit_dressing_pack_header", HEADER_EMITTER_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def blocked_cells_from_overlays() -> set[int]:
    pack = json.loads(OVERLAYS_PATH.read_text(encoding="utf-8"))
    variants = [
        v for v in pack.get("variants", [])
        if v.get("family") == "lume-reach" or str(v.get("id", "")).startswith("lume-reach-")
    ]
    assert len(variants) > 0, "No lume-reach variants found in overlay pack"
    return set.intersection(*(set(v["blocked_cell_indices"]) for v in variants))


class LumeReachDressingTests(unittest.TestCase):
    pack_bytes: bytes
    pack: dict
    blocked: set[int]

    @classmethod
    def setUpClass(cls) -> None:
        cls.pack_bytes = PACK_PATH.read_bytes()
        cls.pack = json.loads(cls.pack_bytes.decode("utf-8"))
        cls.blocked = blocked_cells_from_overlays()

    def test_sidecar_digest_matches_pack_bytes(self) -> None:
        self.assertEqual(
            hashlib.sha256(self.pack_bytes).hexdigest(),
            SIDECAR_PATH.read_text().strip(),
        )

    def test_recompilation_is_deterministic_and_matches_fixture(self) -> None:
        compiler = load_compiler()
        first = compiler.render_bytes(
            compiler.compile_pack(
                REPO_ROOT, Path("Content/World/Source/LumeReach/lume_reach_dressing_v1.json")
            )
        )
        second = compiler.render_bytes(
            compiler.compile_pack(
                REPO_ROOT, Path("Content/World/Source/LumeReach/lume_reach_dressing_v1.json")
            )
        )
        self.assertEqual(first, second)
        self.assertEqual(first, self.pack_bytes)

    def test_base_contract_digest_is_the_live_overlays_pack(self) -> None:
        computed = hashlib.sha256(OVERLAYS_PATH.read_bytes()).hexdigest()
        self.assertEqual(computed, self.pack["base_contract"]["compiled_pack_sha256"])

    def test_source_provenance_digest_matches_source_file(self) -> None:
        computed = hashlib.sha256(SOURCE_PATH.read_bytes()).hexdigest()
        self.assertEqual(computed, self.pack["source_provenance"]["raw_sha256"])

    def test_every_record_conforms_to_the_gameplay_contract(self) -> None:
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

    def test_site_vocabulary_status(self) -> None:
        populated = {s["id"] for s in self.pack["sites"] if s["vocabulary_status"] == "populated"}
        self.assertEqual(populated, EXPECTED_POPULATED_SITES)

    def test_classes_match_expected(self) -> None:
        classes = {entry["id"] for entry in self.pack["dressing_classes"]}
        self.assertEqual(classes, EXPECTED_DRESSING_CLASSES)
        deferred = {entry["id"] for entry in self.pack["deferred_classes"]}
        self.assertEqual(deferred, EXPECTED_DEFERRED_CLASSES)

    def test_header_emission_matches_header_file(self) -> None:
        emitter = load_emitter()
        base_pack, base_digest = emitter.load_digest_pinned(
            REPO_ROOT,
            OVERLAYS_PATH.relative_to(REPO_ROOT),
            OVERLAYS_PATH.with_suffix('.sha256').relative_to(REPO_ROOT),
            'compiled map pack',
        )
        pack, digest = emitter.load_digest_pinned(
            REPO_ROOT,
            PACK_PATH.relative_to(REPO_ROOT),
            SIDECAR_PATH.relative_to(REPO_ROOT),
            'dressing pack',
        )
        rendered = emitter.render_header(
            pack,
            digest,
            base_pack,
            base_digest,
            pack_relative=PACK_PATH.relative_to(REPO_ROOT),
            base_pack_relative=OVERLAYS_PATH.relative_to(REPO_ROOT),
            header_relative=HEADER_PATH.relative_to(REPO_ROOT),
            expected_site_id="lume-reach",
            cpp_namespace="lume_reach_dressing",
            class_order=("civic_frame", "conduit_pylon"),
        )
        self.assertEqual(rendered, HEADER_PATH.read_text(encoding="utf-8"))

    def test_compiler_refuses_a_record_on_passable_cell(self) -> None:
        compiler = load_compiler()
        source = json.loads(SOURCE_PATH.read_text(encoding="utf-8"))
        mutated = copy.deepcopy(source)
        # Move first record to a known passable cell (e.g. 0, 0)
        mutated["sites"][0]["records"][0]["x"] = 0
        mutated["sites"][0]["records"][0]["y"] = 0

        with tempfile.TemporaryDirectory() as tmpdir:
            tmp_source = Path(tmpdir) / "source.json"
            tmp_source.write_text(json.dumps(mutated), encoding="utf-8")
            with self.assertRaises(compiler.CompileError):
                compiler.compile_pack(REPO_ROOT, tmp_source)


if __name__ == "__main__":
    unittest.main()
