#!/usr/bin/env python3
"""Regression checks for the EBS-MER-BLD-003 Array Foundry concept blockout.

Author: Angelis Pseftis. Run: python3 test_foundry_build.py
"""
from __future__ import annotations

import json
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import build_foundry as fo  # noqa: E402


class FoundryBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0 = fo.assemble(0, "producing")
        cls.m1 = fo.assemble(1, "producing")
        cls.research = fo.assemble(0, "researching")
        cls.interrupted = fo.assemble(0, "interrupted")
        cls.inv = fo.contract_inventory(cls.m0)

    def test_contract_inventory(self):
        self.assertEqual(self.inv["long_hall"]["built"], 1)
        self.assertTrue(self.inv["intake"]["hood"] and self.inv["intake"]["ramp"])
        self.assertEqual(self.inv["open_fabrication_bay"]["rails"], 2)
        self.assertGreaterEqual(self.inv["open_fabrication_bay"]["gantry_legs"], 6)
        self.assertTrue(self.inv["output"]["portal"] and self.inv["output"]["ramp"])
        self.assertTrue(self.inv["research_gantry"]["platform"])
        self.assertGreaterEqual(self.inv["research_gantry"]["instruments"], 3)

    def test_material_slots(self):
        self.assertEqual(self.m0.slots, [fo.CERAMIC, fo.FRAME, fo.STATUS])

    def test_everything_stays_inside_the_four_by_four_footprint(self):
        for mesh, label in ((self.m0, "LOD0"), (self.m1, "LOD1"), (self.research, "researching"),
                            (self.interrupted, "interrupted")):
            (x0, y0, _), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), fo.HALF + 1e-6, label)

    def test_nothing_below_the_ground(self):
        for mesh in (self.m0, self.m1, self.research, self.interrupted):
            self.assertGreaterEqual(mesh.bounds()[0][2], 0.0)

    def test_intake_and_output_are_at_opposite_ends(self):
        intake = self.m0.component_bounds("intake_hood")
        output = self.m0.component_bounds("output_portal")
        self.assertLess(intake[1][0], 0.0, "the intake hood sits on the -X end")
        self.assertGreater(output[0][0], 0.0, "the output portal sits on the +X end")
        ramps = (self.m0.component_bounds("intake_ramp"), self.m0.component_bounds("output_ramp"))
        self.assertLess(ramps[0][1][0], intake[0][0] + 1.0, "the intake ramp runs out beyond the hood")
        self.assertGreater(ramps[1][0][0], output[1][0] - 1.0, "the output ramp runs out beyond the portal")

    def test_the_hall_is_a_long_hall(self):
        self.assertGreaterEqual(fo.HALL_LEN / fo.HALL_W, 1.8)

    def test_the_middle_bay_is_open_with_rails_over_it(self):
        roofs = [c for c in self.m0.components() if c.startswith("roof_")]
        self.assertEqual(sorted(roofs), ["roof_intake", "roof_output"], "the middle third must have no roof")
        for side in ("l", "r"):
            rail = self.m0.component_bounds(f"rail_{side}")
            self.assertGreater(rail[0][2], fo.WALL_Z, "the rails ride above the wall line")

    def test_states_differ_where_the_concept_says(self):
        # producing: a carriage with a lit seam on the rail, gantry dark
        self.assertIn("rail_carriage", self.m0.components())
        self.assertIn("frame_in_progress_seam", self.m0.components())
        # researching: no carriage, the gantry lamps are the status slot
        self.assertNotIn("rail_carriage", self.research.components())
        lamps = [p for p in self.research.polygons if p.component.startswith("gantry_lamp_")]
        self.assertTrue(lamps)
        self.assertEqual({self.research.slots[p.slot] for p in lamps}, {fo.STATUS})
        dark = [p for p in self.m0.polygons if p.component.startswith("gantry_lamp_")]
        self.assertEqual({self.m0.slots[p.slot] for p in dark}, {fo.FRAME})
        # interrupted: the frame is on the rail but its seam is not lit, and the gantry is dark
        self.assertIn("rail_carriage", self.interrupted.components())
        self.assertNotIn("frame_in_progress_seam", self.interrupted.components())
        idark = [p for p in self.interrupted.polygons if p.component.startswith("gantry_lamp_")]
        self.assertEqual({self.interrupted.slots[p.slot] for p in idark}, {fo.FRAME})

    def test_the_carriage_moves_toward_the_output_when_producing(self):
        producing = self.m0.component_bounds("rail_carriage")
        stalled = self.interrupted.component_bounds("rail_carriage")
        self.assertGreater(producing[0][0], stalled[0][0], "producing carries the frame toward the door")

    def test_sockets(self):
        names = [s.name for s in self.m0.sockets]
        self.assertEqual(names, self.inv["sockets"]["contract"])
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Intake_Mouth"].position[0], 0.0)
        self.assertGreater(by_name["Output_Door"].position[0], 0.0)
        self.assertAlmostEqual(by_name["Intake_Mouth"].yaw_deg, 180.0, places=6)
        self.assertLess(by_name["Rail_Start"].position[0], by_name["Rail_End"].position[0])

    def test_collision_covers_the_hall_not_the_ramps(self):
        self.assertEqual(len(self.m0.collision), 2)
        ramp = self.m0.component_bounds("intake_ramp")
        for box in self.m0.collision:
            low = box.center[0] - box.size[0] / 2.0
            self.assertGreater(low, ramp[0][0], "no collision box reaches over the intake ramp")

    def test_whole_asset_budget(self):
        # card REL-BLD-015.MC.FOUNDRY 8,000 / 3,400; owner ruling 2026-09-07 applies them to the
        # complete asset including any articulated components, so the manifest sum is what is tested
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        lod0 = lod0 or self.m0.triangle_count()
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(lod0, 8000)
        self.assertLessEqual(lod1, 3400)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_states_are_declared(self):
        self.assertEqual(tuple(fo.STATES), ("producing", "researching", "interrupted"))
        with self.assertRaises(ValueError):
            fo.assemble(0, "destroyed")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = fo.assemble(0, "producing").write_glb(os.path.join(tmp, "a.glb"))
            b = fo.assemble(0, "producing").write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(fo.REVISION, f"{fo.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
