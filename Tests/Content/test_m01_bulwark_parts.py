"""Static source contract for M01's presentation-only Bulwark deployment parts."""
import ast
from pathlib import Path
import unittest


ROOT = Path(__file__).parents[2]
GENERATOR = ROOT / "Scripts" / "generate_art_assets.py"
PIPELINE = ROOT / "Scripts" / "generate_art_assets.sh"


class M01BulwarkPartsContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = GENERATOR.read_text(encoding="utf-8")
        cls.tree = ast.parse(cls.source)
        cls.functions = {
            node.name: node for node in cls.tree.body if isinstance(node, ast.FunctionDef)
        }
        cls.assignments = {
            target.id: node
            for node in cls.tree.body
            if isinstance(node, ast.Assign)
            for target in node.targets
            if isinstance(target, ast.Name)
        }

    def source_for(self, name):
        return ast.get_source_segment(self.source, self.functions[name])

    def assignment_for(self, name):
        return ast.get_source_segment(self.source, self.assignments[name])

    def test_derivatives_are_m01_only_and_do_not_mutate_the_roster(self):
        self.assertIn(
            'M01_BULWARK_ARTICULATION_ASSET_REVISION = "m01-bulwark-deployment-parts-v1"',
            self.source,
        )
        parts = self.assignment_for("M01_BULWARK_PARTS_ASSETS")
        roster = self.assignment_for("ASSETS")
        self.assertIn("M01_BULWARK_PARTS_ASSETS = (", parts)
        self.assertIn('AssetSpec("Meridian", "Units", "Bulwark"', roster)
        for name in ("M01BulwarkBody", "M01BulwarkLeftWing", "M01BulwarkRightWing"):
            self.assertIn(f'"{name}"', parts)
            self.assertNotIn(f'"{name}"', roster)
        self.assertEqual(parts.count('AssetSpec("Meridian", "Units",'), 3)

    def test_wings_preserve_the_existing_deployed_geometry_at_hinge_origins(self):
        wings = self.source_for("m01_bulwark_wing")
        original = self.source_for("meridian_bulwark")
        hinges = self.assignment_for("M01_BULWARK_WING_HINGES")
        self.assertIn('"Left": (26.0, -24.0, 72.0)', hinges)
        self.assertIn('"Right": (26.0, 24.0, 72.0)', hinges)
        self.assertIn("global_at[index] - hinge[index]", wings)
        # These are the six original cell centers; local() makes zero-rotation
        # placement reconstruct their exact previous deployed transforms.
        for center in (
            "(56.0, side * 34.0, 72.0, side * 8.0, 29.0)",
            "(56.0, side * 34.0, 114.0, side * 8.0, 26.0)",
            "(50.0, side * 64.0, 92.0, side * 20.0, 25.0)",
        ):
            self.assertIn(center, wings)
            self.assertIn(center, original)
        self.assertIn("m01_bulwark_wing(mesh, high, -1.0)", self.source_for("m01_bulwark_left_wing"))
        self.assertIn("m01_bulwark_wing(mesh, high, 1.0)", self.source_for("m01_bulwark_right_wing"))

    def test_body_excludes_wings_and_keeps_the_deployed_emitter_chassis(self):
        body = self.source_for("m01_bulwark_body")
        self.assertNotIn("cell_specs", body)
        self.assertNotIn("side * 38.0", body)
        for canonical_fragment in (
            "(78.0, 96.0, 30.0)",
            "(26.0, 0.0, 72.0)",
            "(46.0, 0.0, 72.0)",
            "(56.0, 0.0, 72.0)",
        ):
            self.assertIn(canonical_fragment, body)

    def test_per_lod_material_zones_preserve_real_low_detail_wing_sections(self):
        zones = self.assignment_for("M01_BULWARK_PART_MATERIAL_ZONES")
        self.assertIn('"M01BulwarkBody": ((0, 1, 2, 3), (0, 1, 2, 3))', zones)
        for wing in ("M01BulwarkLeftWing", "M01BulwarkRightWing"):
            self.assertIn(f'"{wing}": ((0, 1, 2, 3), (1, 2, 3))', zones)
        self.assertIn("Low-detail wings omit only their high-LOD sensor/cone fittings", self.source)
        audit = self.source_for("audit_m01_bulwark_part")
        self.assertIn("get_num_lods() != 2", audit)
        self.assertIn("get_simple_collision_count(asset) != 0", audit)
        self.assertIn("get_lod_material_slot", audit)
        self.assertIn("M01_BULWARK_ARTICULATION_ASSET_REVISION", audit)
        self.assertIn("M01_BULWARK_PART_MATERIAL_ZONES[spec.name][lod_index]", audit)

    def test_pipeline_is_narrow_and_runs_only_the_three_derivatives(self):
        pipeline = PIPELINE.read_text(encoding="utf-8")
        self.assertIn("ECHOES_M01_BULWARK_PARTS_ONLY", pipeline)
        self.assertIn("ECHOES_M01_BULWARK_PARTS_READY", pipeline)
        self.assertIn("assets=3 lods=2 collision=0", pipeline)
        branch = self.source.split('if os.environ.get("ECHOES_M01_BULWARK_PARTS_ONLY") == "1":', 1)[1]
        branch = branch.split('if os.environ.get("ECHOES_M01_SURVEYOR_PARTS_ONLY")', 1)[0]
        self.assertIn("M01_BULWARK_PARTS_ASSETS", branch)
        self.assertIn("audit_m01_bulwark_part(part, spec)", branch)
        self.assertNotIn("purge_stale_art_masters.py", branch)


if __name__ == "__main__":
    unittest.main()
