#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-UNT-001 Tender concept blockout.

Author: Angelis Pseftis. Run: python3 test_tender_build.py
"""
from __future__ import annotations

import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import ebs_skelkit as skel  # noqa: E402
import build_tender as td  # noqa: E402


class TenderBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.sk, cls.counts, cls.socks = td.assemble(0, True)
        cls.m1 = td.assemble(1, True)[0]
        cls.empty = td.assemble(0, False)[0]
        cls.clips = td.build_clips(cls.sk)
        cls.inv = td.contract_inventory(cls.m0, cls.sk, cls.clips)

    # --- the concept's parts --------------------------------------------------------------------
    def test_contract_inventory(self):
        self.assertEqual(self.inv["thickened_forearms"]["built"], 2)
        self.assertEqual(self.inv["wrist_nodules"]["built"], 2)
        self.assertEqual(self.inv["resonance_staff"]["built"], 1)
        self.assertEqual(self.inv["resonance_staff"]["forked_head"], 2)
        self.assertTrue(self.inv["resonance_staff"]["bead"])
        self.assertEqual(self.inv["woven_sling"]["built"], 1)
        self.assertEqual(self.inv["woven_sling"]["matter_nodules"], td.SLING_NODULES)
        self.assertEqual(self.inv["bare_feet"]["built"], 2)
        self.assertEqual(self.inv["kilt_wrap"]["built"], 1)

    def test_no_machined_panel_or_bolt_anywhere(self):
        # REL-ART-007.FAIL: machined metallic panels or industrial bolts on Kharuun assets fail
        self.assertEqual(self.inv["forbidden"]["panel_or_bolt_components"], [])

    def test_no_cyan_slot(self):
        # cyan is the Meridian Compact's colour; the Kharuun palette is strata, fibre and amber
        self.assertEqual(self.m0.slots, [td.STRATA, td.FIBRE, td.AMBER])
        for slot in self.m0.slots:
            self.assertNotIn("Cyan", slot)

    def test_forearms_are_heavier_than_the_upper_arms(self):
        self.assertGreater(td.FORE_W, td.UPPER_W)

    def test_loaded_and_empty_read_differently(self):
        loaded = {c for c in self.m0.components() if c.startswith("sling_matter_")}
        empty = {c for c in self.empty.components() if c.startswith("sling_matter_")}
        self.assertTrue(loaded)
        self.assertEqual(empty, set())
        self.assertIn("sling_basket", self.empty.components())   # the sling itself stays

    # --- proportions and stance ------------------------------------------------------------------
    def test_stocky_humanoid_proportions(self):
        meas = td.measurements(self.m0)
        self.assertAlmostEqual(meas["shoulder_width_over_H"], 0.30, delta=0.06)
        self.assertAlmostEqual(meas["head_height_over_H"], 0.13, delta=0.05)
        self.assertAlmostEqual(meas["staff_length_over_H"], 0.95, delta=0.05)
        self.assertTrue(meas["feet_on_the_ground"])

    def test_the_staff_stands_taller_than_the_figure(self):
        top = max(self.m0.component_bounds(c)[1][2] for c in self.m0.components() if c.startswith("staff_"))
        self.assertGreater(top, td.H * 0.95)

    # --- rig, sockets, clips ---------------------------------------------------------------------
    def test_rig_and_sockets(self):
        names = [b.name for b in self.sk.bones]
        self.assertEqual(names[0], "root")
        for required in ("pelvis", "chest", "head", "staff", "sling", "r_hand", "l_foot"):
            self.assertIn(required, names)
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(td.SOCKETS))
        self.assertEqual(self.socks["Harvest_Tether_Muzzle"], "staff")
        self.assertEqual(self.socks["Cargo_Drop_Anchor"], "sling")

    def test_clip_inventory_matches_the_contract(self):
        built = [c.name for c in self.clips]
        for required in self.inv["tracks"]["contract"]:
            self.assertIn(required, built)
        self.assertNotIn("root", {b for c in self.clips for b in c.tracks})   # no root motion

    def test_every_clip_duration_is_a_whole_frame(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), f"{c.name} {c.duration_s}")

    def test_no_clip_breaks_the_ground_plane(self):
        for c in self.clips:
            steps = max(1, int(round(c.duration_s / 0.05)))
            for i in range(steps + 1):
                mesh = td.posed(0, td.sample_pose(c, i / steps))
                (_, _, z0), _ = mesh.bounds()
                self.assertGreaterEqual(z0, -1.0, f"{c.name} at {i / steps:.2f}: {z0:.2f}")

    def test_gather_is_a_kneeling_press(self):
        clip = next(c for c in self.clips if c.name == "gather")
        mesh = td.posed(0, td.sample_pose(clip, 0.5))
        knee = mesh.component_bounds("r_knee")[0][2]
        self.assertLess(knee, 12.0, "the trailing knee must reach the ground")
        staff_low = min(mesh.component_bounds(c)[0][2] for c in mesh.components() if c.startswith("staff_"))
        self.assertLess(staff_low, 30.0, "the staff must be pressed toward the strata")
        chest = mesh.component_bounds("chest_block")
        self.assertLess(chest[1][2], td.CHEST_Z + 30.0, "the cultivator is down, not standing")

    def test_grow_is_an_upright_circling_walk(self):
        clip = next(c for c in self.clips if c.name == "grow")
        mesh = td.posed(0, td.sample_pose(clip, 0.25))
        self.assertGreater(mesh.bounds()[1][2], td.H * 0.85, "grow stays upright")

    # --- budgets ---------------------------------------------------------------------------------
    def test_budgets_and_emissive(self):
        self.assertLessEqual(self.m0.triangle_count(), 4500)
        self.assertLessEqual(self.m1.triangle_count(), 1800)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())
        self.assertLessEqual(td.slot_area_fraction(self.m0, td.AMBER), 0.05)

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = td.assemble(0, True)[0].write_obj(os.path.join(tmp, "a.obj"))
            b = td.assemble(0, True)[0].write_obj(os.path.join(tmp, "b.obj"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(td.REVISION, f"{td.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
