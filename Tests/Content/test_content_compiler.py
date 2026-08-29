#!/usr/bin/env python3
"""Adversarial tests for the authoritative Echoes content compiler."""

from __future__ import annotations

import copy
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("compile_content", ROOT / "Scripts/compile_content.py")
assert SPEC is not None and SPEC.loader is not None
COMPILER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COMPILER)


class ContentCompilerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="echoes-content-test-")
        self.source = Path(self.temporary.name) / "Source"
        self.source.mkdir()
        for filename in COMPILER.SOURCE_FILES:
            original = ROOT / "Content/Data/Source" / filename
            (self.source / filename).write_bytes(original.read_bytes())

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def load(self, filename: str) -> dict:
        return json.loads((self.source / filename).read_text(encoding="utf-8"))

    def write(self, filename: str, value: dict) -> None:
        (self.source / filename).write_text(json.dumps(value), encoding="utf-8")

    def assert_invalid(self, expected: str) -> None:
        with self.assertRaisesRegex(COMPILER.ContentValidationError, expected):
            COMPILER.build_pack(self.source)

    def test_valid_sources_compile_canonically(self) -> None:
        first = Path(self.temporary.name) / "first.json"
        second = Path(self.temporary.name) / "second.json"
        first_digest = COMPILER.compile_pack(self.source, first)
        first_pack = json.loads(first.read_text(encoding="utf-8"))
        self.assertEqual(
            first.with_suffix(".json.sha256").read_text(encoding="ascii"),
            f"{first_digest}\n",
        )

        units = self.load("units.json")
        units["units"].reverse()
        self.write("units.json", units)
        second_digest = COMPILER.compile_pack(self.source, second)

        self.assertEqual(first.read_bytes(), second.read_bytes())
        self.assertEqual(first_digest, second_digest)
        self.assertEqual(first_pack["pack_format"], "echoes-content-pack")
        self.assertEqual([item["id"] for item in first_pack["units"]], sorted(item["id"] for item in first_pack["units"]))
        bulwark = next(item for item in first_pack["units"] if item["id"] == "mc_bulwark_team")
        self.assertEqual(bulwark["deployment"]["damage_reduction_percent"], 40)
        relay = next(item for item in first_pack["units"] if item["id"] == "mc_relay_skiff")
        self.assertEqual(relay["supply_extension"]["capacity_bonus"], 4)
        waystone = next(item for item in first_pack["buildings"] if item["id"] == "ka_waystone")
        self.assertEqual(waystone["migration"]["move_speed_cm_s"], 120)
        self.assertEqual(waystone["migration"]["mobile_damage_taken_percent"], 125)
        growth_basin = next(item for item in first_pack["buildings"] if item["id"] == "ka_growth_basin")
        self.assertEqual(growth_basin["adaptation"]["molt_ticks"], 80)
        self.assertEqual(growth_basin["adaptation"]["molt_damage_taken_percent"], 150)
        self.assertEqual(growth_basin["adaptation"]["carapace_health_percent"], 135)
        self.assertEqual(growth_basin["adaptation"]["striker_damage_percent"], 125)

    def test_unknown_field_fails_closed(self) -> None:
        units = self.load("units.json")
        units["units"][0]["unreviewed_bonus"] = 99
        self.write("units.json", units)
        self.assert_invalid("unknown fields: unreviewed_bonus")

    def test_duplicate_identifier_is_rejected(self) -> None:
        units = self.load("units.json")
        units["units"].append(copy.deepcopy(units["units"][0]))
        self.write("units.json", units)
        self.assert_invalid("duplicate unit id")

    def test_unknown_faction_reference_is_rejected(self) -> None:
        buildings = self.load("buildings.json")
        buildings["buildings"][0]["faction"] = "missing_faction"
        self.write("buildings.json", buildings)
        self.assert_invalid("unknown faction 'missing_faction'")

    def test_missing_playable_roster_role_is_rejected(self) -> None:
        units = self.load("units.json")
        units["units"] = [
            item
            for item in units["units"]
            if item["faction"] != "kharuun_assemblies" or item["role"] == "worker"
        ]
        self.write("units.json", units)
        self.assert_invalid("requires a worker and three distinct combat/support roles")

    def test_missing_production_structure_is_rejected(self) -> None:
        buildings = self.load("buildings.json")
        buildings["buildings"] = [
            item
            for item in buildings["buildings"]
            if item["faction"] != "meridian_compact" or item["role"] != "production"
        ]
        self.write("buildings.json", buildings)
        self.assert_invalid("requires a production structure")

    def test_invalid_numeric_range_is_rejected(self) -> None:
        wells = self.load("future_wells.json")
        wells["rules"]["reshape"]["manifest_duration_ticks"] = -1
        self.write("future_wells.json", wells)
        self.assert_invalid("must be between 1 and 10000000")

    def test_malformed_json_is_rejected(self) -> None:
        (self.source / "factions.json").write_text("{not-json", encoding="utf-8")
        self.assert_invalid("invalid JSON")

    def test_missing_bulwark_deployment_rules_are_rejected(self) -> None:
        units = self.load("units.json")
        bulwark = next(item for item in units["units"] if item["id"] == "mc_bulwark_team")
        del bulwark["deployment"]
        self.write("units.json", units)
        self.assert_invalid("requires authored deployment rules")

    def test_deployment_rules_on_other_units_are_rejected(self) -> None:
        units = self.load("units.json")
        bulwark = next(item for item in units["units"] if item["id"] == "mc_bulwark_team")
        lancer = next(item for item in units["units"] if item["id"] == "mc_lancer")
        lancer["deployment"] = copy.deepcopy(bulwark["deployment"])
        self.write("units.json", units)
        self.assert_invalid("reserved for the Meridian heavy screen")

    def test_missing_relay_supply_rules_are_rejected(self) -> None:
        units = self.load("units.json")
        relay = next(item for item in units["units"] if item["id"] == "mc_relay_skiff")
        del relay["supply_extension"]
        self.write("units.json", units)
        self.assert_invalid("requires authored supply-extension rules")

    def test_supply_rules_on_other_units_are_rejected(self) -> None:
        units = self.load("units.json")
        relay = next(item for item in units["units"] if item["id"] == "mc_relay_skiff")
        resonant = next(item for item in units["units"] if item["id"] == "ka_resonant")
        resonant["supply_extension"] = copy.deepcopy(relay["supply_extension"])
        self.write("units.json", units)
        self.assert_invalid("reserved for the Meridian scout support")

    def test_missing_waystone_migration_rules_are_rejected(self) -> None:
        buildings = self.load("buildings.json")
        waystone = next(item for item in buildings["buildings"] if item["id"] == "ka_waystone")
        del waystone["migration"]
        self.write("buildings.json", buildings)
        self.assert_invalid("requires authored migration rules")

    def test_migration_rules_on_other_buildings_are_rejected(self) -> None:
        buildings = self.load("buildings.json")
        waystone = next(item for item in buildings["buildings"] if item["id"] == "ka_waystone")
        power_link = next(item for item in buildings["buildings"] if item["id"] == "mc_power_link")
        power_link["migration"] = copy.deepcopy(waystone["migration"])
        self.write("buildings.json", buildings)
        self.assert_invalid("reserved for the Kharuun mobile supply node")

    def test_missing_growth_basin_adaptation_rules_are_rejected(self) -> None:
        buildings = self.load("buildings.json")
        growth_basin = next(item for item in buildings["buildings"] if item["id"] == "ka_growth_basin")
        del growth_basin["adaptation"]
        self.write("buildings.json", buildings)
        self.assert_invalid("requires authored adaptation rules")

    def test_adaptation_rules_on_other_buildings_are_rejected(self) -> None:
        buildings = self.load("buildings.json")
        growth_basin = next(item for item in buildings["buildings"] if item["id"] == "ka_growth_basin")
        foundry = next(item for item in buildings["buildings"] if item["id"] == "mc_array_foundry")
        foundry["adaptation"] = copy.deepcopy(growth_basin["adaptation"])
        self.write("buildings.json", buildings)
        self.assert_invalid("reserved for the Kharuun production structure")


if __name__ == "__main__":
    unittest.main(verbosity=2)
