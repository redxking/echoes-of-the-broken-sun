"""Static source contract for M01's presentation-only articulated Surveyor parts."""
import ast
from pathlib import Path
import unittest


ROOT = Path(__file__).parents[2]
GENERATOR = ROOT / "Scripts" / "generate_art_assets.py"
PIPELINE = ROOT / "Scripts" / "generate_art_assets.sh"


class M01SurveyorPartsContract(unittest.TestCase):
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
        node = self.functions[name]
        return ast.get_source_segment(self.source, node)

    def assignment_for(self, name):
        return ast.get_source_segment(self.source, self.assignments[name])

    def test_derivative_manifest_is_m01_only_and_separate_from_roster(self):
        self.assertIn('M01_SURVEYOR_ARTICULATION_ASSET_REVISION = "m01-surveyor-articulation-v1"', self.source)
        parts = self.assignment_for("M01_SURVEYOR_PARTS_ASSETS")
        roster = self.assignment_for("ASSETS")
        self.assertIn("M01_SURVEYOR_PARTS_ASSETS = (", parts)
        self.assertNotIn('AssetSpec("Meridian", "Units", "M01SurveyorBody"', roster)
        for name in ("M01SurveyorBody", "M01SurveyorUpper", "M01SurveyorLower", "M01SurveyorFoot"):
            self.assertIn(f'"{name}"', self.source)
        self.assertIn('return f"SM_{self.faction}_{self.name}"', self.source)
        self.assertIn('return f"{ART_ROOT}/{self.faction}/{self.category}/{self.asset_name}"', self.source)

    def test_body_preserves_surveyor_form_without_welded_legs(self):
        body = self.source_for("meridian_surveyor_body")
        self.assertNotIn("paired_leg(", body)
        for canonical_fragment in (
            "(58.0, 48.0, 36.0)",
            "(68.0, 52.0, 54.0)",
            "(-28.0, side * 18.0, 90.0)",
            "(-18.0, 0.0, 137.0)",
        ):
            self.assertIn(canonical_fragment, body)
        roster = self.source_for("meridian_surveyor")
        self.assertIn("meridian_surveyor_body(mesh, high)", roster)
        self.assertEqual(roster.count("paired_leg("), 2)

    def test_part_pivots_dimensions_and_material_zones_are_exact(self):
        upper = self.source_for("m01_surveyor_upper")
        lower = self.source_for("m01_surveyor_lower")
        foot = self.source_for("m01_surveyor_foot")
        self.assertIn("(42.0, width, width), (21.0, 0.0, 0.0), DARK", upper)
        self.assertIn("width * 0.72, (0.0, 0.0, 0.0), GLOW", upper)
        self.assertIn("(46.0, width, width), (23.0, 0.0, 0.0), LIGHT", lower)
        self.assertIn("(16.0, width + 3.0, 14.0), (8.0, 0.0, 0.0), PRIMARY", lower)
        self.assertIn("(32.0, width + 8.0, 10.0), (0.0, 0.0, 5.0), DARK", foot)
        self.assertIn("(14.0, width + 6.0, 6.0), (12.0, 0.0, 4.0), LIGHT", foot)
        self.assertIn('"M01SurveyorUpper": (1, 3)', self.source)
        self.assertIn('"M01SurveyorLower": (0, 2)', self.source)
        self.assertIn('"M01SurveyorFoot": (1, 2)', self.source)

    def test_pipeline_is_narrow_and_audits_lods_collision_and_revision(self):
        pipeline = PIPELINE.read_text(encoding="utf-8")
        self.assertIn("ECHOES_M01_SURVEYOR_PARTS_ONLY", pipeline)
        self.assertIn("ECHOES_M01_SURVEYOR_PARTS_READY", pipeline)
        self.assertIn("assets=4 lods=2 collision=0", pipeline)
        self.assertNotIn("purge_stale_art_masters.py", pipeline.split("ECHOES_M01_SURVEYOR_PARTS_ONLY", 1)[1].split("fi", 1)[0])
        branch = self.source.split('if os.environ.get("ECHOES_M01_SURVEYOR_PARTS_ONLY") == "1":', 1)[1].split('if os.environ.get("ECHOES_M01_SHROUD_ONLY")', 1)[0]
        self.assertIn("M01_SURVEYOR_PARTS_ASSETS", branch)
        self.assertIn("audit_m01_surveyor_part(part, spec)", branch)
        self.assertIn("M01_SURVEYOR_ARTICULATION_ASSET_REVISION", branch)
        audit = self.source_for("audit_m01_surveyor_part")
        self.assertIn("get_simple_collision_count(asset) != 0", audit)
        self.assertIn("get_lod_material_slot", audit)


if __name__ == "__main__":
    unittest.main()
