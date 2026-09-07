#!/usr/bin/env python3
"""Regression checks for the EBS-HOL-BLD-003 Chorus Loom concept blockout.

Author: Angelis Pseftis. Run: python3 test_chorus_loom_build.py
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
import build_chorus_loom as cl  # noqa: E402


class ChorusLoomBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = cl.assemble(0, "producing")
        cls.m1 = cl.assemble(1, "producing")[0]
        cls.research = cl.assemble(0, "researching")[0]
        cls.insolvent = cl.assemble(0, "insolvent")[0]
        cls.destroyed = cl.assemble(0, "destroyed")[0]
        cls.clips = cl.build_clips(cls.s)
        cls.inv = cl.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_contract_inventory(self):
        self.assertTrue(self.inv["platform"]["built"])
        self.assertEqual(self.inv["posts"]["built"], 2)
        self.assertGreater(self.inv["posts"]["edge_strips"], 0)
        self.assertEqual(self.inv["warp"]["built"], cl.WARP_COUNT)
        self.assertTrue(self.inv["hovering_beam"]["built"])

    def test_the_beam_hovers_and_touches_nothing(self):
        # the faction read: adding a strut would destroy it
        m = cl.measurements(self.m0)
        self.assertTrue(m["beam_hovers"])
        self.assertGreater(m["beam_gap_cm"], 0.0)
        beam_low = cl.BEAM_Z - cl.BEAM[2] / 2.0
        post_top = cl.PLATFORM[2] + cl.POST[2]
        self.assertGreater(beam_low, post_top, "nothing may reach the beam")
        self.assertGreater(beam_low, cl.WARP_HIGH, "the beam clears the warp")
        for c in self.m0.components():
            self.assertFalse("strut" in c or "brace" in c or "support" in c, c)

    def test_the_woven_form_is_not_geometry(self):
        self.assertFalse(self.inv["woven_form"]["built_as_geometry"])
        self.assertIn("Weave_Center", [sk.name for sk in self.m0.sockets])

    def test_the_three_lit_states_differ_in_geometry(self):
        self.assertEqual(cl.lit_threads(self.m0), cl.WARP_COUNT)
        self.assertGreater(cl.lit_threads(self.research), cl.lit_threads(self.m0),
                           "researching doubles the warp")
        self.assertEqual(cl.lit_threads(self.insolvent), 0, "insolvent: every thread dark")
        # geometry, not brightness alone: the states differ in triangle count too
        self.assertGreater(self.research.triangle_count(), self.m0.triangle_count())
        under = {p.component for p in self.research.polygons
                 if p.component == "beam_underside" and self.research.slots[p.slot] == cl.MAGENTA}
        self.assertTrue(under, "researching lights the beam underside")
        self.assertFalse([p for p in self.m0.polygons
                          if p.component == "beam_underside" and self.m0.slots[p.slot] == cl.MAGENTA],
                         "producing leaves the beam underside dark")

    def test_material_slots_and_magenta_is_the_only_emissive_and_under_the_ceiling(self):
        self.assertEqual(self.m0.slots, [cl.VITRIFIED, cl.GROUND, cl.MAGENTA])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == cl.MAGENTA}
        self.assertTrue(all(c.startswith("warp_") or "_edge_" in c or c == "beam_underside" for c in lit), lit)
        for mesh, label in ((self.m0, "producing"), (self.research, "researching")):
            self.assertLessEqual(cl.slot_area_fraction(mesh, cl.MAGENTA), 0.12, label)

    def test_no_other_factions_language(self):
        comps = " ".join(self.m0.components())
        for foreign in ("strata", "amber", "cyan", "conduit"):
            self.assertNotIn(foreign, comps)
        for slot in self.m0.slots:
            self.assertTrue(slot.startswith("MI_EBS_HOL_"), slot)

    def test_destroyed_keeps_the_platform_and_drops_the_beam(self):
        comps = self.destroyed.components()
        self.assertIn("platform", comps)
        self.assertNotIn("beam", comps)
        self.assertIn("fallen_beam", comps)
        self.assertFalse([c for c in comps if c.startswith("warp_")])
        self.assertEqual(len([c for c in comps if c.startswith("fallen_post_")]), 2)

    def test_everything_stays_inside_the_four_by_four_footprint(self):
        for mesh, label in ((self.m0, "producing"), (self.m1, "LOD1"), (self.research, "researching"),
                            (self.insolvent, "insolvent"), (self.destroyed, "destroyed")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), cl.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, -1e-6, label)

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(17):
                mesh = cl.posed(0, cl.sample_pose(clip, i / 16.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -0.01, f"{clip.name} at {i / 16.0:.3f}")

    def test_the_beam_hangs_out_of_step_with_the_posts(self):
        clip = {c.name: c for c in self.clips}["producing_idle"]

        def profile(bone):
            return tuple(round(cl.sample_pose(clip, f / 8.0).get(bone, (0.0,) * 6)[i], 4)
                         for f in range(8) for i in (1, 2))

        beam, post_l, post_r = (profile(b) for b in ("beam", "post_l", "post_r"))
        self.assertNotEqual(beam, post_l)
        self.assertNotEqual(post_l, post_r, "the posts must not drift in unison")

    def test_sockets(self):
        self.assertEqual(sorted(sk.name for sk in self.m0.sockets), sorted(cl.SOCKETS))
        by_name = {sk.name: sk for sk in self.m0.sockets}
        self.assertLess(by_name["Unit_Emergence"].position[1], 0.0, "units step off the near edge")
        self.assertEqual(self.socks["Research_Beam"], "beam", "the research read rides the beam bone")

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips), ["producing_idle", "researching", "restore"])

    def test_rig_is_root_two_posts_and_the_beam(self):
        self.assertEqual([b.name for b in self.s.bones], ["root", "post_l", "post_r", "beam"])
        for b in self.s.bones:
            self.assertTrue(b.purpose, b.name)

    def test_provisional_card_recorded_as_traced_first(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        self.assertIn(cl.CARD, data["provisional_contract"]["card"])
        self.assertIn("PROVISIONAL", data["provisional_contract"]["status"])
        self.assertTrue(data["traced_before_the_card"]["card_written_after"])
        self.assertLessEqual(data["budgets"]["worst_state_triangles"], 6000)
        self.assertLessEqual(data["budgets"]["lod1_triangles"], 2400)
        self.assertIn("HEADROOM", data["budgets"]["headroom_note"])

    def test_lod1_keeps_both_posts_the_warp_and_the_beam(self):
        inv = cl.contract_inventory(self.m1, self.s, self.clips)
        self.assertEqual(inv["posts"]["built"], 2)
        self.assertEqual(inv["warp"]["built"], cl.WARP_COUNT)
        self.assertTrue(inv["hovering_beam"]["built"])
        self.assertTrue(inv["platform"]["built"])
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_states_are_declared(self):
        self.assertEqual(tuple(cl.STATES), ("producing", "researching", "insolvent", "destroyed"))
        with self.assertRaises(ValueError):
            cl.assemble(0, "supplied")

    def test_lod1_carries_no_collision(self):
        # a LOD1 file carrying collision inflated an earlier import; the exporter drops it
        self.assertTrue(self.m1.collision)  # the mesh has it; the export excludes it
        with tempfile.TemporaryDirectory() as tmp:
            with open(os.path.join(HERE, "build_chorus_loom.py"), encoding="utf-8") as handle:
                src = handle.read()
        self.assertIn("include_collision=False", src)

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = cl.assemble(0, "producing")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = cl.assemble(0, "producing")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(cl.REVISION, f"{cl.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
