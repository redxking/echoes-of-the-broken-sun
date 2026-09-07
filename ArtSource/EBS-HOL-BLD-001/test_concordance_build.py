#!/usr/bin/env python3
"""Regression checks for the EBS-HOL-BLD-001 Concordance concept blockout.

Author: Angelis Pseftis. Run: python3 test_concordance_build.py
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
import build_concordance as cc  # noqa: E402


class ConcordanceBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = cc.assemble(0, "working")
        cls.m1 = cc.assemble(1, "working")[0]
        cls.damaged = cc.assemble(0, "damaged")[0]
        cls.destroyed = cc.assemble(0, "destroyed")[0]
        cls.clips = cc.build_clips(cls.s)
        cls.inv = cc.contract_inventory(cls.m0, cls.s, cls.clips)
        cls.mm = cc.measurements(cls.m0)

    def test_contract_inventory(self):
        self.assertTrue(self.inv["disc"]["built"])
        self.assertEqual(self.inv["slab_pairs"]["built"], cc.PAIR_COUNT)
        self.assertEqual(self.inv["slab_pairs"]["slabs"], cc.PAIR_COUNT * 2)
        self.assertTrue(self.inv["intake_apron"]["built"])

    def test_traced_proportion_and_pair_count(self):
        self.assertAlmostEqual(self.mm["slab_height_over_ring_diameter"], 0.24, delta=0.01)
        self.assertEqual(self.mm["pair_count"], 14)

    def test_each_pair_is_two_slabs_offset_by_three_centimetres(self):
        for i in range(1, cc.PAIR_COUNT + 1):
            a = self.m0.component_bounds(f"pair_{i:02d}_slab_a")
            b = self.m0.component_bounds(f"pair_{i:02d}_slab_b")
            ca = ((a[0][0] + a[1][0]) / 2.0, (a[0][1] + a[1][1]) / 2.0)
            cb = ((b[0][0] + b[1][0]) / 2.0, (b[0][1] + b[1][1]) / 2.0)
            distance = math.hypot(ca[0] - cb[0], ca[1] - cb[1])
            self.assertAlmostEqual(distance, cc.OFFSET_CM, delta=0.2, msg=f"pair {i}")

    def test_the_duplicate_does_not_merge_into_its_parent(self):
        # the card's readability clause: if the offset duplicate merges, the reality-bleed read is lost
        self.assertGreater(cc.OFFSET_CM, 0.0)
        self.assertLess(cc.OFFSET_CM, cc.SLAB_T, "a 3 cm offset on a 9 cm slab stays visible as a duplicate")

    def test_the_fracture_edges_are_two_per_slab_and_not_stacked(self):
        # an earlier slip put both edges at the slab centre, drawing one bar instead of two borders
        for i in range(1, cc.PAIR_COUNT + 1):
            left = self.m0.component_bounds(f"pair_{i:02d}_edge_al")
            right = self.m0.component_bounds(f"pair_{i:02d}_edge_ar")
            cl = ((left[0][0] + left[1][0]) / 2.0, (left[0][1] + left[1][1]) / 2.0)
            cr = ((right[0][0] + right[1][0]) / 2.0, (right[0][1] + right[1][1]) / 2.0)
            self.assertGreater(math.hypot(cl[0] - cr[0], cl[1] - cr[1]), cc.SLAB_W * 0.6, f"pair {i}")

    def test_material_slots_and_magenta_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [cc.VITRIFIED, cc.GROUND, cc.MAGENTA])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == cc.MAGENTA}
        self.assertTrue(all("_edge_" in c for c in lit), lit)
        self.assertLessEqual(cc.slot_area_fraction(self.m0, cc.MAGENTA), 0.12)

    def test_no_other_factions_language(self):
        comps = " ".join(self.m0.components())
        for foreign in ("strata", "amber", "cyan", "conduit", "plate_seam"):
            self.assertNotIn(foreign, comps)
        for slot in self.m0.slots:
            self.assertTrue(slot.startswith("MI_EBS_HOL_"), slot)

    def test_the_centre_thread_is_not_geometry(self):
        self.assertFalse(self.inv["centre_thread"]["built_as_geometry"])
        self.assertIn("Choir_Thread_Center", [s.name for s in self.m0.sockets])

    def test_states_differ_where_the_concept_says(self):
        self.assertTrue([c for c in self.damaged.components() if c.startswith("crack_")])
        self.assertFalse([c for c in self.m0.components() if c.startswith("crack_")])
        self.assertFalse([c for c in self.destroyed.components() if c.startswith("pair_")],
                         "no pair stands in the destroyed state")
        self.assertTrue([c for c in self.destroyed.components() if c.startswith("fallen_slab_")])

    def test_the_damaged_pair_drifts_further_than_the_rest(self):
        clip = {c.name: c for c in self.clips}["damaged"]
        hurt = max(abs(k.rotation_deg[1]) for k in clip.tracks[f"pair_{cc.DAMAGED_PAIR:02d}"])
        others = [max(abs(k.rotation_deg[1]) for k in clip.tracks[b])
                  for b in cc.PAIR_BONES if b != f"pair_{cc.DAMAGED_PAIR:02d}"]
        self.assertGreater(hurt, max(others) * 1.5)

    def test_the_pair_drift_phases_are_real(self):
        # the defect that shipped in the Riftstalker: key time and phase from the same value
        clip = {c.name: c for c in self.clips}["working_idle"]

        def profile(bone):
            return tuple(round(cc.sample_pose(clip, f / 8.0).get(bone, (0.0,) * 6)[1], 4) for f in range(8))

        profiles = {b: profile(b) for b in cc.PAIR_BONES}
        self.assertGreater(len(set(profiles.values())), cc.PAIR_COUNT // 2,
                           "pairs must not all drift in unison")

    def test_everything_stays_inside_the_five_by_five_footprint(self):
        for mesh, label in ((self.m0, "working"), (self.m1, "LOD1"), (self.damaged, "damaged"),
                            (self.destroyed, "destroyed")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), cc.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, -1e-6, label)

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(17):
                mesh = cc.posed(0, cc.sample_pose(clip, i / 16.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -0.01, f"{clip.name} at {i / 16.0:.3f}")

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(cc.SOCKETS))
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Matter_Dropoff"].position[0], 0.0, "the apron is at the -X front")
        self.assertLess(by_name["Rally_Default"].position[0], -cc.DISC_R)

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips), ["damaged", "restore", "working_idle"])

    def test_provisional_card_and_its_provenance(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        self.assertIn("REL-BLD-017.HC.CONCORDANCE.ASSET", data["provisional_contract"]["card"])
        self.assertTrue(data["traced_before_the_card"]["card_written_after"])
        self.assertIn("INFERENCE", data["faction_language_inference"]["status"])
        self.assertIn("HEADROOM", data["budgets"]["headroom_note"])
        worst = data["budgets"]["worst_state_triangles"]
        self.assertLessEqual(worst, 8000)
        self.assertLessEqual(data["budgets"]["lod1_triangles"], 3500)

    def test_lod1_keeps_the_ring_the_pairs_and_the_apron(self):
        inv = cc.contract_inventory(self.m1, self.s, self.clips)
        self.assertEqual(inv["slab_pairs"]["built"], cc.PAIR_COUNT)
        self.assertTrue(inv["intake_apron"]["built"])
        self.assertTrue(inv["disc"]["built"])

    def test_states_are_declared(self):
        self.assertEqual(tuple(cc.STATES), ("working", "damaged", "destroyed"))
        with self.assertRaises(ValueError):
            cc.assemble(0, "researching")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = cc.assemble(0, "working")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = cc.assemble(0, "working")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(cc.REVISION, f"{cc.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
