"""Training staging contract regressions. Author: Angelis Pseftis."""
import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "training_staging", ROOT / "Content/World/Tools/compile_training_staging.py")
COMPILER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(COMPILER)


class TrainingStagingTests(unittest.TestCase):
    def setUp(self):
        self.source = (ROOT / COMPILER.SOURCE).read_bytes()
        self.data = json.loads(self.source)

    def test_registered_output_and_training_scope(self):
        output = COMPILER.render(self.source)
        self.assertEqual(output, (ROOT / COMPILER.HEADER).read_bytes())
        self.assertIn(b'kOperation = "training-readiness"', output)
        self.assertIn(b'kLinkRepairInitialHp = 440', output)

    def test_invalid_identity_geometry_and_health_refused(self):
        changes = [
            ("operation", "campaign-prologue"),
            ("source_version", True),
            ("link.build_site", [64, 14]),
            ("link.build_site", [True, 14]),
            ("link.repair_initial_hp", 450),
            ("link.repair_initial_hp", 0),
            ("link.completion_radius_sim_cm", 400),
            ("foundry.rally_site", [6, 14]),
        ]
        for key, value in changes:
            with self.subTest(key=key, value=value):
                data = copy.deepcopy(self.data)
                parts = key.split(".")
                parent = data if len(parts) == 1 else data[parts[0]]
                parent[parts[-1]] = value
                with self.assertRaises(ValueError):
                    COMPILER.render(json.dumps(data).encode())

    def test_unknown_fields_refused_and_identity_changes_with_source(self):
        data = copy.deepcopy(self.data)
        data["campaign_health_override"] = 440
        with self.assertRaises(ValueError):
            COMPILER.render(json.dumps(data).encode())
        self.assertNotEqual(COMPILER.render(self.source),
                            COMPILER.render(self.source + b"\n"))


if __name__ == "__main__":
    unittest.main()
