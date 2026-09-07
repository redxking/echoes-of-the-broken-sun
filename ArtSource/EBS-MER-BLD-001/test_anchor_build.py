#!/usr/bin/env python3
"""Regression checks for the EBS-MER-BLD-001 Anchor concept blockout.

Author: Angelis Pseftis. Run: python3 test_anchor_build.py

Structural checks against `concept-fidelity.md` and the rules that bound it. Not gate acceptance.
"""
from __future__ import annotations

import math
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import build_anchor as an  # noqa: E402


class AnchorBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0 = an.assemble(0, "working")
        cls.m1 = an.assemble(1, "working")
        cls.damaged = an.assemble(0, "damaged")
        cls.inv = an.contract_inventory(cls.m0)
        cls.meas = an.measurements(cls.m0)

    # --- contract -------------------------------------------------------------------------------
    def test_contract_inventory(self):
        self.assertEqual(self.inv["worker_bays"]["built"], 3)
        self.assertEqual(self.inv["worker_bays"]["ramps"], 3)
        self.assertEqual(self.inv["matter_intake_chute"]["built"], 1)
        self.assertEqual(self.inv["conduit_roots"]["built"], an.ARMS)
        self.assertEqual(self.inv["central_mast"]["built"], 1)
        self.assertEqual(self.inv["charcoal_plinth"]["built"], 2)      # plinth + step
        self.assertGreaterEqual(self.inv["ceramic_drum"]["built"], 2)  # lower + upper (+ dome courses)
        self.assertGreaterEqual(sum(1 for c in self.m0.components() if c.startswith("cap_course_")), 3,
                                "the top must step as a dome, not read as flat discs")

    def test_material_slots_are_the_three_contracted(self):
        self.assertEqual(self.m0.slots, [an.CERAMIC, an.FRAME, an.STATUS])
        self.assertEqual(self.m1.slots, [an.CERAMIC, an.FRAME, an.STATUS])

    # --- the footprint is the outer bound --------------------------------------------------------
    def test_everything_stays_inside_the_five_by_five_footprint(self):
        for mesh, label in ((self.m0, "LOD0"), (self.m1, "LOD1"), (self.damaged, "damaged")):
            (x0, y0, _), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), an.HALF + 1e-6, label)

    def test_nothing_is_authored_below_the_ground(self):
        for mesh in (self.m0, self.m1, self.damaged):
            (_, _, z0), _ = mesh.bounds()
            self.assertGreaterEqual(z0, 0.0)

    # --- proportions measured on the candidate ---------------------------------------------------
    def test_drum_is_squat_and_wide(self):
        # canon SPEC-BLD-015.MC.ANCHOR: "a squat, wide ceramic drum". Mass height over drum diameter.
        self.assertLessEqual(an.CAP_Z / (2 * an.DRUM_R), 0.65)
        self.assertGreater(2 * an.DRUM_R, an.CAP_Z)

    def test_drum_dominates_the_footprint_width(self):
        # the candidate draws a big drum with stub arms, not a small drum with long arms
        self.assertGreaterEqual(2 * an.DRUM_R / an.F, 0.65)
        stub = an.NODE_R + an.NODE_SIZE[0] / 2.0 - an.DRUM_R
        room = an.HALF - an.DRUM_R          # all the room the 5x5 footprint leaves past the drum
        self.assertLessEqual(stub, room)    # the arms may not cross the footprint
        self.assertLessEqual(stub, 0.42 * an.DRUM_R)

    def test_mast_is_about_a_third_of_the_total_height(self):
        # re-measured on the comparison sheet: the candidate's mast runs 0.38-0.40 of the silhouette
        fraction = (an.WHIP_Z - an.CAP_Z) / an.WHIP_Z
        self.assertAlmostEqual(fraction, 0.383, delta=0.03)

    def test_mast_is_slim_not_a_reactor_spire(self):
        # the shaft, its rungs and the head — the collar is the drum's fitting, not the mast shaft
        shaft = [c for c in self.m0.components()
                 if c.startswith(("mast_column_", "mast_rung_", "mast_whip_")) or c in ("mast_head", "mast_head_lens")]
        widest = max(self.m0.component_bounds(c)[1][0] - self.m0.component_bounds(c)[0][0] for c in shaft)
        self.assertLessEqual(widest, 0.30 * an.DRUM_R)
        collar = self.m0.component_bounds("mast_collar")
        self.assertLessEqual(collar[1][0] - collar[0][0], 0.6 * an.DRUM_R)

    # --- the three bays, their ramps and the separate chute --------------------------------------
    def test_bay_portals_read_from_outside_the_drum(self):
        for index in (1, 2, 3):
            (bx0, _, _), (bx1, _, _) = self.m0.component_bounds(f"bay_{index:02d}_interior")
            radius = max(math.hypot(*p) for p in self._corners(f"bay_{index:02d}_interior"))
            self.assertGreater(radius, an.DRUM_R, f"bay {index} portal is buried in the drum wall")
        interiors = [p for p in self.m0.polygons if p.component.endswith("_interior")]
        self.assertTrue(interiors)
        for p in interiors:
            self.assertEqual(self.m0.slots[p.slot], an.STATUS, "the bay interior must read as lit")

    def _corners(self, component):
        (x0, y0, _), (x1, y1, _) = self.m0.component_bounds(component)
        return [(x0, y0), (x0, y1), (x1, y0), (x1, y1)]

    def test_each_bay_has_a_ramp_that_reaches_the_ground(self):
        for index in (1, 2, 3):
            (_, _, z0), (_, _, z1) = self.m0.component_bounds(f"bay_{index:02d}_ramp")
            self.assertLessEqual(z0, 0.5)
            self.assertLessEqual(z1, an.STEP_Z + 1.0)

    def test_the_chute_is_separate_from_the_worker_bays(self):
        chute = self.m0.component_bounds("chute_housing")
        cx = (chute[0][0] + chute[1][0]) / 2.0
        cy = (chute[0][1] + chute[1][1]) / 2.0
        for index in (1, 2, 3):
            b = self.m0.component_bounds(f"bay_{index:02d}_interior")
            bx, by = (b[0][0] + b[1][0]) / 2.0, (b[0][1] + b[1][1]) / 2.0
            self.assertGreater(math.hypot(cx - bx, cy - by), 120.0, f"chute overlaps bay {index}")
        self.assertGreater(abs(an.CHUTE_YAW), max(abs(y) for y in an.BAY_YAWS))

    # --- sockets ---------------------------------------------------------------------------------
    def test_sockets_named_and_placed(self):
        names = [s.name for s in self.m0.sockets]
        self.assertEqual(names, self.inv["sockets"]["contract"])
        by_name = {s.name: s for s in self.m0.sockets}
        for index, yaw in enumerate(an.BAY_YAWS, start=1):
            s = by_name[f"Worker_Bay_{index:02d}"]
            self.assertAlmostEqual(s.yaw_deg, yaw, places=6)
            self.assertGreater(math.hypot(s.position[0], s.position[1]), an.DRUM_R)
        for i in range(1, an.ARMS + 1):
            s = by_name[f"Conduit_Node_{i:02d}"]
            self.assertAlmostEqual(math.hypot(s.position[0], s.position[1]), an.NODE_R, delta=1.0)
        self.assertAlmostEqual(by_name["Mast_Top"].position[2], an.HEAD_Z, places=6)
        self.assertEqual(by_name["Target_Anchor_Center"].position[:2], (0.0, 0.0))

    # --- collision -------------------------------------------------------------------------------
    def test_collision_covers_the_mass_only(self):
        self.assertEqual(len(self.m0.collision), 2)
        for box in self.m0.collision:
            half_x = box.size[0] / 2.0 + abs(box.center[0])
            half_y = box.size[1] / 2.0 + abs(box.center[1])
            self.assertLessEqual(max(half_x, half_y), an.HALF)
            # the ramps and the conduit nodes must be outside every collision box
            self.assertLess(box.size[0] / 2.0, an.NODE_R)

    def test_lod1_carries_no_collision_in_its_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.m1.write_glb(os.path.join(tmp, "lod1.glb"), include_collision=False)

    # --- budgets and states ----------------------------------------------------------------------
    def test_budgets_within_the_tighter_meridian_rule(self):
        # REL-ART-028 (8,000 / 3,500) is tighter than REL-BLD-015.MC.CORE (12,000 / 4,500)
        self.assertLessEqual(self.m0.triangle_count(), 8000)
        self.assertLessEqual(self.m1.triangle_count(), 3500)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_damaged_state_darkens_a_band_and_opens_a_panel_section(self):
        working_band = [p for p in self.m0.polygons if p.component == "band_lower"]
        damaged_band = [p for p in self.damaged.polygons if p.component == "band_lower"]
        self.assertEqual({self.m0.slots[p.slot] for p in working_band}, {an.STATUS})
        self.assertEqual({self.damaged.slots[p.slot] for p in damaged_band}, {an.FRAME})
        self.assertTrue(any(c.startswith("damage_rib_") for c in self.damaged.components()))
        self.assertFalse(any(c.startswith("damage_") for c in self.m0.components()))

    def test_states_are_declared(self):
        self.assertEqual(tuple(an.STATES), ("working", "damaged"))
        with self.assertRaises(ValueError):
            an.assemble(0, "destroyed")

    # --- determinism -----------------------------------------------------------------------------
    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = an.assemble(0, "working").write_glb(os.path.join(tmp, "a.glb"))
            b = an.assemble(0, "working").write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(an.REVISION, f"{an.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
