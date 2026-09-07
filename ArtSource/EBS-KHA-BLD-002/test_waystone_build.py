#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-BLD-002 Waystone concept blockout.

Author: Angelis Pseftis. Run: python3 test_waystone_build.py
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
import build_waystone as ws  # noqa: E402


class WaystoneBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = ws.assemble(0, "rooted")
        cls.m1 = ws.assemble(1, "rooted")[0]
        cls.mobile = ws.assemble(0, "uprooted_mobile")[0]
        cls.damaged = ws.assemble(0, "damaged")[0]
        cls.destroyed = ws.assemble(0, "destroyed")[0]
        cls.clips = ws.build_clips(cls.s)
        cls.inv = ws.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_contract_inventory(self):
        self.assertEqual(self.inv["monolith_courses"]["built"], ws.COURSES)
        self.assertEqual(self.inv["amber_seams"]["built"], ws.COURSES - 1)
        self.assertEqual(self.inv["root_plinth"]["built"], len(ws.PLINTH_STEPS))
        self.assertGreaterEqual(self.inv["root_plinth"]["flare_tiers"], 2)
        self.assertEqual(self.inv["roots"]["built"], ws.ROOT_COUNT)

    def test_the_carriage_has_four_legs_and_exists_only_while_migrating(self):
        # the candidate's MIGRATING view shows four legs; the card was corrected to match it
        mob = ws.contract_inventory(self.mobile, self.s, self.clips)
        self.assertEqual(mob["carriage"]["legs"], 4)
        self.assertTrue(mob["carriage"]["plate"])
        self.assertFalse(self.inv["carriage"]["plate"], "no carriage while rooted")
        self.assertEqual(self.inv["carriage"]["legs"], 0)

    def test_rooted_and_mobile_are_distinguishable_from_the_roots_alone(self):
        # canon: "Rooted or moving is visible from the roots alone."
        rooted_roots = [c for c in self.m0.components() if c.startswith("root_") and not c.endswith("_tip")]
        mobile_roots = [c for c in self.mobile.components() if c.startswith("root_") and not c.endswith("_tip")]
        self.assertEqual(len(rooted_roots), len(mobile_roots), "the same roots exist in both states")
        flat = max(self.m0.component_bounds(c)[1][2] for c in rooted_roots)
        braid = max(self.mobile.component_bounds(c)[1][2] for c in mobile_roots)
        self.assertLess(flat, 40.0, "rooted: the roots lie flat on the ground")
        self.assertGreater(braid, 120.0, "mobile: the roots have retracted upward into braided bundles")
        spread_rooted = max(abs(v) for c in rooted_roots for b in (self.m0.component_bounds(c),) for p in b for v in p[:2])
        spread_mobile = max(abs(v) for c in mobile_roots for b in (self.mobile.component_bounds(c),) for p in b for v in p[:2])
        self.assertLess(spread_mobile, spread_rooted, "the roots retract inward as well as upward")

    def test_the_migrating_footprint_is_narrower_than_the_rooted_one(self):
        self.assertLess(ws.footprint_span(self.mobile), ws.footprint_span(self.m0))

    def test_traced_concept_proportions(self):
        m = ws.measurements(self.m0, self.mobile)
        self.assertAlmostEqual(m["total_height_over_root_spread"], 1.09, delta=0.03)
        self.assertAlmostEqual(m["shaft_width_over_root_spread"], 0.25, delta=0.02)
        self.assertAlmostEqual(m["mobile_span_over_rooted"], 0.92, delta=0.02)
        self.assertAlmostEqual(m["shaft_share_of_height"], 0.56, delta=0.02)

    def test_material_slots_and_amber_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [ws.STRATA, ws.AMBER])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == ws.AMBER}
        self.assertTrue(all(c.startswith(("seam_", "nodule_")) for c in lit), lit)
        self.assertLessEqual(ws.slot_area_fraction(self.m0, ws.AMBER), 0.15)  # REL-ART-029, a ceiling

    def test_the_shaft_is_continuous_from_the_plinth(self):
        # a first pass left a 50 cm gap between the plinth top and the first course
        plinth_top = max(self.m0.component_bounds(c)[1][2] for c in self.m0.components() if c.startswith("plinth_"))
        flare_bottom = min(self.m0.component_bounds(c)[0][2] for c in self.m0.components() if c.startswith("flare_"))
        flare_top = max(self.m0.component_bounds(c)[1][2] for c in self.m0.components() if c.startswith("flare_"))
        course_bottom = self.m0.component_bounds("course_01")[0][2]
        self.assertLessEqual(flare_bottom, plinth_top + 1e-6)
        self.assertGreaterEqual(flare_top, course_bottom - 1e-6)

    def test_flat_roots_respect_the_ground_clutter_ceiling(self):
        # REL-ART-030 clamps decorative ground assets on open paths to <= 20 cm vertical displacement
        self.assertLessEqual(ws.ROOT_FLAT_Z + ws.ROOT_FLAT_R, 20.0)

    def test_everything_stays_inside_the_two_by_two_footprint(self):
        for mesh, label in ((self.m0, "rooted"), (self.m1, "LOD1"), (self.mobile, "mobile"),
                            (self.damaged, "damaged"), (self.destroyed, "destroyed")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), ws.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, -1e-6, label)

    def test_no_posed_frame_leaves_the_ground_or_the_footprint(self):
        by_name = {c.name: c for c in self.clips}
        for name, fraction, state in ws.POSE_SAMPLES:
            mesh = ws.posed(0, ws.sample_pose(by_name[name], fraction), state)
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertGreaterEqual(z0, -1e-6, f"{name}@{fraction}")
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), ws.HALF + 1e-6, f"{name}@{fraction}")

    def test_the_rig_is_the_cards_own_plan(self):
        names = [b.name for b in self.s.bones]
        self.assertEqual(names, ["root", "ring_sink", "carriage_lift", "foot_01", "foot_02", "foot_03", "foot_04"])
        for foot in ws.FEET:
            self.assertEqual({b.name: b.parent for b in self.s.bones}[foot], "carriage_lift")

    def test_clip_durations_match_the_canon_tick_counts(self):
        by_name = {c.name: c for c in self.clips}
        self.assertAlmostEqual(by_name["uproot"].duration_s * ws.TICKS_PER_SECOND, 40, places=3)
        self.assertAlmostEqual(by_name["root"].duration_s * ws.TICKS_PER_SECOND, 60, places=3)
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)

    def test_the_root_clip_has_canons_four_beats(self):
        keys = {c.name: c for c in self.clips}["root"].tracks["ring_sink"]
        self.assertGreaterEqual(len(keys), 5, "preparation, contact, settling, release")
        heights = [k.translation_cm[2] for k in keys]
        self.assertLess(min(heights), 0.0, "the settle overshoots below the rest height")
        self.assertAlmostEqual(heights[-1], 0.0, places=6, msg="and releases back to rest")

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(ws.SOCKETS))
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Matter_Dropoff"].position[0], 0.0)
        self.assertGreater(by_name["Carriage_Front"].position[0], 0.0)
        self.assertEqual(self.socks["Root_Ring_Center"], "ring_sink")
        self.assertEqual(self.socks["Carriage_Front"], "carriage_lift")

    def test_provisional_card_budget(self):
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-BLD-016.KA.WAYSTONE.ASSET", data["provisional_contract"]["card"])
            self.assertIn("PROVISIONAL", data["provisional_contract"]["status"])
            self.assertIn("PENDING", data["acceptance"]["technical"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        lod0 = max(lod0 or self.m0.triangle_count(), self.mobile.triangle_count())
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(lod0, 5000)
        self.assertLessEqual(lod1, 2200)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_preserves_what_the_card_requires(self):
        # card .READABILITY: a LOD that removes the root ring fails, because rooted-or-moving must stay
        # readable from the roots alone
        comps = self.m1.components()
        self.assertTrue([c for c in comps if c.startswith("root_")], "LOD1 keeps the root ring")
        self.assertTrue([c for c in comps if c.startswith("plinth_")])
        self.assertEqual(sum(1 for c in comps if c.startswith("course_")), ws.COURSES)

    def test_states_are_declared(self):
        self.assertEqual(tuple(ws.STATES), ("rooted", "uprooted_mobile", "damaged", "destroyed"))
        with self.assertRaises(ValueError):
            ws.assemble(0, "researching")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = ws.assemble(0, "rooted")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = ws.assemble(0, "rooted")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(ws.REVISION, f"{ws.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
