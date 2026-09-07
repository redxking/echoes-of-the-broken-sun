#!/usr/bin/env python3
"""Regression checks for the EBS-HOL-BLD-002 Interval Loom concept blockout.

Author: Angelis Pseftis. Run: python3 test_interval_loom_build.py
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
import ebs_skelkit as skel  # noqa: E402
import build_interval_loom as il  # noqa: E402


class IntervalLoomBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = il.assemble(0, "supplied")
        cls.m1 = il.assemble(1, "supplied")[0]
        cls.tick = il.assemble(0, "upkeep_tick")[0]
        cls.insolvent = il.assemble(0, "insolvent")[0]
        cls.destroyed = il.assemble(0, "destroyed")[0]
        cls.clips = il.build_clips(cls.s)
        cls.inv = il.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_contract_inventory(self):
        self.assertEqual(self.inv["arches"]["built"], 2)
        self.assertEqual(self.inv["feet"]["built"], 4)
        self.assertTrue(self.inv["dropoff_pad"]["built"])
        self.assertGreater(self.inv["dropoff_pad"]["delivered_matter"], 0)

    def test_the_arches_cross(self):
        # two arches on opposite diagonals must intersect over the centre
        yaws = [y for _tag, y in il.ARCHES]
        self.assertEqual(len(set(yaws)), 2)
        self.assertAlmostEqual(abs(yaws[0] - yaws[1]), 90.0, places=6)
        crossing = {s.name: s for s in self.m0.sockets}["Arch_Crossing_Center"]
        self.assertAlmostEqual(crossing.position[0], 0.0, places=6)
        self.assertAlmostEqual(crossing.position[1], 0.0, places=6)

    def test_the_arch_springs_from_its_foot_plate(self):
        # starting the ribbon at z = 0 put its underside 2.4 cm below the floor
        self.assertAlmostEqual(il.arch_point(45.0, 0.0)[2], il.FOOT_PLATE[2], places=6)
        self.assertEqual(self.m0.bounds()[0][2], 0.0)

    def test_the_arch_is_broad_not_pointed(self):
        m = il.measurements(self.m0)
        self.assertLess(m["height_over_span"], 0.75)
        self.assertGreater(m["height_over_span"], 0.5)

    def test_the_three_power_states_differ(self):
        # the solvency read is this asset's whole job
        self.assertGreater(il.lit_edges(self.m0), 0, "supplied: edges lit")
        self.assertEqual(il.lit_edges(self.insolvent), 0, "insolvent: every edge dark")
        self.assertEqual(self.inv["surge_band"]["built"], 0, "no surge outside the tick")
        tick_inv = il.contract_inventory(self.tick, self.s, self.clips)
        self.assertEqual(tick_inv["surge_band"]["built"], len(il.ARCHES),
                         "the tick carries a surge band the other states do not")
        self.assertGreater(self.tick.triangle_count(), self.m0.triangle_count())

    def test_material_slots_and_magenta_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [il.VITRIFIED, il.GROUND, il.MAGENTA])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == il.MAGENTA}
        self.assertTrue(all("_edge_" in c or c.endswith("_surge") for c in lit), lit)
        self.assertLessEqual(il.slot_area_fraction(self.m0, il.MAGENTA), 0.12)
        self.assertLessEqual(il.slot_area_fraction(self.tick, il.MAGENTA), 0.12)

    def test_no_other_factions_language(self):
        comps = " ".join(self.m0.components())
        for foreign in ("strata", "amber", "cyan", "conduit"):
            self.assertNotIn(foreign, comps)
        for slot in self.m0.slots:
            self.assertTrue(slot.startswith("MI_EBS_HOL_"), slot)

    def test_destroyed_keeps_the_feet_and_the_pad(self):
        comps = self.destroyed.components()
        self.assertFalse([c for c in comps if "_span_" in c])
        self.assertTrue([c for c in comps if "_foot_" in c])
        self.assertIn("dropoff_pad", comps)
        self.assertTrue([c for c in comps if c.startswith("fallen_")])

    def test_everything_stays_inside_the_two_by_two_footprint(self):
        for mesh, label in ((self.m0, "supplied"), (self.m1, "LOD1"), (self.tick, "tick"),
                            (self.insolvent, "insolvent"), (self.destroyed, "destroyed")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), il.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, -1e-6, label)

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(17):
                mesh = il.posed(0, il.sample_pose(clip, i / 16.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -0.01, f"{clip.name} at {i / 16.0:.3f}")

    def test_the_two_arches_drift_out_of_step(self):
        clip = {c.name: c for c in self.clips}["supplied_idle"]

        def profile(bone):
            return tuple(round(il.sample_pose(clip, f / 8.0).get(bone, (0.0,) * 6)[2], 4) for f in range(8))

        a, b = (profile(bone) for bone in il.ARCH_BONES)
        self.assertNotEqual(a, b, "the arches must not drift in unison")

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(il.SOCKETS))
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Matter_Dropoff"].position[0], 0.0)

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips), ["restore", "supplied_idle", "upkeep"])

    def test_provisional_card_and_the_intervalist_disclaimer(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        self.assertIn("REL-BLD-017.HC.INTERVAL.ASSET", data["provisional_contract"]["card"])
        self.assertIn("INTERVALIST", data["provisional_contract"]["status"])
        self.assertTrue(data["traced_before_the_card"]["card_written_after"])
        self.assertLessEqual(data["budgets"]["worst_state_triangles"], 3000)
        self.assertLessEqual(data["budgets"]["lod1_triangles"], 1200)

    def test_lod1_keeps_both_arches_the_feet_and_the_pad(self):
        inv = il.contract_inventory(self.m1, self.s, self.clips)
        self.assertEqual(inv["arches"]["built"], 2)
        self.assertEqual(inv["feet"]["built"], 4)
        self.assertTrue(inv["dropoff_pad"]["built"])
        self.assertGreater(inv["edge_lines"]["built"], 0)

    def test_states_are_declared(self):
        self.assertEqual(tuple(il.STATES), ("supplied", "upkeep_tick", "insolvent", "destroyed"))
        with self.assertRaises(ValueError):
            il.assemble(0, "working")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = il.assemble(0, "supplied")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = il.assemble(0, "supplied")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(il.REVISION, f"{il.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
