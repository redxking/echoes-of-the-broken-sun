#!/usr/bin/env python3
"""Regression checks for the EBS-HOL-BLD-004 Phase Anchor concept blockout.

Author: Angelis Pseftis. Run: python3 test_phase_anchor_build.py
"""
from __future__ import annotations

import json
import math
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_skelkit as skel  # noqa: E402
import build_phase_anchor as pa  # noqa: E402


class PhaseAnchorBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = pa.assemble(0, "field_active")
        cls.m1 = pa.assemble(1, "field_active")[0]
        cls.lost = pa.assemble(0, "field_lost")[0]
        cls.destroyed = pa.assemble(0, "destroyed")[0]
        cls.clips = pa.build_clips(cls.s)
        cls.inv = pa.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_contract_inventory(self):
        self.assertEqual(self.inv["plinth"]["built"], len(pa.PLINTH_STEPS))
        self.assertEqual(self.inv["shaft"]["built"], pa.SEGMENTS)
        self.assertTrue(self.inv["apex_cap"]["built"])
        self.assertEqual(self.inv["arris_lines"]["built"], pa.SIDES)
        self.assertEqual(self.inv["register"]["built"], (pa.REGISTER_NODES - 1) * 2)

    def test_the_taper_matches_the_trace(self):
        m = pa.measurements(self.m0)
        self.assertAlmostEqual(m["slenderness"], m["traced_slenderness"], delta=0.005)
        self.assertAlmostEqual(m["plinth_fraction_of_height"], m["traced_plinth_fraction"], delta=0.005)
        # the shaft narrows monotonically and never inverts
        radii = [pa.shaft_radius(z) for z in range(int(pa.PLINTH_H), int(pa.CAP_Z), 20)]
        self.assertEqual(radii, sorted(radii, reverse=True))
        self.assertGreater(pa.SHAFT_TOP_R, 0.0)

    def test_the_aura_field_is_not_geometry(self):
        # the 700 cm radius belongs to gameplay data; the mesh must imply no radius of its own
        self.assertFalse(self.inv["aura_field"]["built_as_geometry"])
        self.assertIn("Field_Ring_Origin", [sk.name for sk in self.m0.sockets])
        (x0, y0, _z0), (x1, y1, _z1) = self.m0.bounds()
        self.assertLess(max(abs(x0), abs(x1), abs(y0), abs(y1)), pa.AURA_CM)

    def test_the_register_lies_on_a_face_not_across_an_edge(self):
        # with a vertex on -Y the register straddled an edge and never showed on the front face
        self.assertEqual(pa.HEX_PHASE, 0.0)
        verts = pa.hexagon(1.0)
        self.assertAlmostEqual(min(abs(y) for _x, y in verts), 0.0, places=6, msg="a vertex sits on the X axis")
        for c in self.m0.components():
            if c.startswith("register_"):
                (_x0, y0, _z0), (_x1, y1, _z1) = self.m0.component_bounds(c)
                self.assertLess(y1, 0.0, f"{c} must sit on the -Y face")
                z_mid = (_z0 + _z1) / 2.0
                self.assertLess(abs(y0), pa.apothem(pa.shaft_radius(z_mid)) + 4 * pa.REGISTER_R, c)

    def test_the_two_states_differ_and_the_lit_read_is_a_slot_change(self):
        self.assertEqual(pa.lit_lines(self.m0), pa.SIDES + (pa.REGISTER_NODES - 1) * 2)
        self.assertEqual(pa.lit_lines(self.lost), 0, "field lost: the spire is inert stone")
        self.assertEqual(self.m0.triangle_count(), self.lost.triangle_count(),
                         "the two states share their geometry; only the slot changes")
        self.assertNotIn(pa.MAGENTA, [self.lost.slots[p.slot] for p in self.lost.polygons])

    def test_material_slots_and_magenta_is_the_only_emissive_and_under_the_ceiling(self):
        self.assertEqual(self.m0.slots, [pa.VITRIFIED, pa.GROUND, pa.MAGENTA])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == pa.MAGENTA}
        self.assertTrue(all(c.startswith("arris_") or c.startswith("register_") for c in lit), lit)
        self.assertLessEqual(pa.slot_area_fraction(self.m0, pa.MAGENTA), 0.12)

    def test_no_other_factions_language(self):
        comps = " ".join(self.m0.components())
        for foreign in ("strata", "amber", "cyan", "conduit"):
            self.assertNotIn(foreign, comps)
        for slot in self.m0.slots:
            self.assertTrue(slot.startswith("MI_EBS_HOL_"), slot)

    def test_destroyed_is_recorded_as_a_proposed_study(self):
        # segmentation is neither mechanically mandatory nor concept-approved: the candidate has no
        # destroyed panel, and the placement ruling does not require cosmetic debris to stay inside
        # the footprint. This test pins what the build DOES, not that it must be this way.
        comps = self.destroyed.components()
        self.assertEqual(len([c for c in comps if c.startswith("fallen_section_")]), 2)
        self.assertIn("stump", comps)
        self.assertFalse([c for c in comps if c.startswith("shaft_") or c.startswith("arris_")])
        self.assertTrue([c for c in comps if c.startswith("plinth_step_")])

    def test_everything_stays_inside_the_two_by_two_footprint(self):
        for mesh, label in ((self.m0, "field_active"), (self.m1, "LOD1"), (self.lost, "field_lost"),
                            (self.destroyed, "destroyed")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), pa.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, -1e-6, label)

    def test_the_plinth_is_a_recorded_footprint_adaptation(self):
        # the traced proportion would put the plinth near 500 cm across, outside the footprint
        self.assertLessEqual(2 * pa.PLINTH_STEPS[0][0], pa.F)
        self.assertGreater(2 * pa.PLINTH_STEPS[0][0], 2 * pa.SHAFT_BASE_R * 2)
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        self.assertIn("footprint_adaptation", data)
        self.assertIn("Array Foundry", data["footprint_adaptation"]["authority"])
        # the matching HEIGHT ratio must not be read as agreement on the WIDTH proportion
        self.assertIn("different measurements",
                      data["footprint_adaptation"]["what_the_matching_ratios_do_NOT_establish"])

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(17):
                mesh = pa.posed(0, pa.sample_pose(clip, i / 16.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -0.01, f"{clip.name} at {i / 16.0:.3f}")

    def test_the_apex_drifts_out_of_phase_with_the_shaft(self):
        clip = {c.name: c for c in self.clips}["field_active_idle"]

        def profile(bone):
            return tuple(round(pa.sample_pose(clip, f / 8.0).get(bone, (0.0,) * 6)[1], 4) for f in range(8))

        self.assertNotEqual(profile("spire"), profile("apex"))

    def test_sockets(self):
        self.assertEqual(sorted(sk.name for sk in self.m0.sockets), sorted(pa.SOCKETS))
        self.assertEqual(self.socks["Apex_Beacon"], "apex")
        self.assertEqual(self.socks["Register_Center"], "spire")
        by_name = {sk.name: sk for sk in self.m0.sockets}
        self.assertLess(by_name["Register_Center"].position[1], 0.0, "the register is on the -Y face")
        self.assertLess(by_name["Field_Ring_Origin"].position[2], 10.0, "the ring is projected at the ground")

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips), ["field_active_idle", "field_lost", "restore"])

    def test_rig_is_root_spire_and_apex(self):
        self.assertEqual([b.name for b in self.s.bones], ["root", "spire", "apex"])
        for b in self.s.bones:
            self.assertTrue(b.purpose, b.name)

    def test_provisional_card_recorded_as_traced_first(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        self.assertIn(pa.CARD, data["provisional_contract"]["card"])
        self.assertIn("PROVISIONAL", data["provisional_contract"]["status"])
        self.assertTrue(data["traced_before_the_card"]["card_written_after"])
        self.assertTrue(data["traced_before_the_card"]["measurements_rejected_as_perspective"])
        self.assertLessEqual(data["budgets"]["worst_state_triangles"], 4000)
        self.assertLessEqual(data["budgets"]["lod1_triangles"], 1600)
        self.assertIn("HEADROOM", data["budgets"]["headroom_note"])

    def test_lod1_keeps_the_taper_the_steps_and_an_arris_read(self):
        inv = pa.contract_inventory(self.m1, self.s, self.clips)
        self.assertEqual(inv["plinth"]["built"], len(pa.PLINTH_STEPS))
        self.assertEqual(inv["shaft"]["built"], pa.SEGMENTS)
        self.assertTrue(inv["apex_cap"]["built"])
        self.assertEqual(inv["arris_lines"]["built"], 2, "the two silhouette edges keep the lit read")
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_states_are_declared(self):
        self.assertEqual(tuple(pa.STATES), ("field_active", "field_lost", "destroyed"))
        with self.assertRaises(ValueError):
            pa.assemble(0, "producing")

    def test_the_destroyed_study_is_labelled_as_proposed(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        destroyed = data["states"]["destroyed"]
        self.assertIn("PROPOSED DESTRUCTION STUDY", destroyed)
        self.assertIn("NOT require cosmetic debris", destroyed)
        self.assertIn("neither mechanically mandatory nor concept-approved", destroyed)

    def test_the_silhouette_figure_carries_no_acceptance_meaning(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        sep = data["monochrome_separation"]
        self.assertIn("COMPARISON EVIDENCE ONLY", sep["status"])
        self.assertIn("PENDING", sep["required_evaluation"])

    def test_no_collision_travels_in_the_skinned_glb(self):
        # every skeletal package in this pipeline exports without collision; a UBX box in the LOD0 file
        # was imported by Interchange as a SECOND SkeletalMesh with its own skeleton
        self.assertTrue(self.m0.collision)
        with open(os.path.join(HERE, "build_phase_anchor.py"), encoding="utf-8") as handle:
            src = handle.read()
        self.assertIn("include_collision=False", src)
        self.assertNotIn("include_collision=(lod == 0)", src)

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = pa.assemble(0, "field_active")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = pa.assemble(0, "field_active")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(pa.REVISION, f"{pa.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
