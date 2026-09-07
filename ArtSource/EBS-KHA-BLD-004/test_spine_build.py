#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-BLD-004 Listening Spine concept blockout.

Author: Angelis Pseftis. Run: python3 test_spine_build.py
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
import build_spine as ls  # noqa: E402


class SpineBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = ls.assemble(0, "listening")
        cls.m1 = ls.assemble(1, "listening")[0]
        cls.contact = ls.assemble(0, "contact")[0]
        cls.damaged = ls.assemble(0, "damaged")[0]
        cls.destroyed = ls.assemble(0, "destroyed")[0]
        cls.clips = ls.build_clips(cls.s)
        cls.inv = ls.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_contract_inventory(self):
        self.assertEqual(self.inv["sensor_nodules"]["built"], ls.NODULE_COUNT)
        self.assertEqual(self.inv["rooted_socket"]["steps"], len(ls.SOCKET_STEPS))
        self.assertEqual(self.inv["rooted_socket"]["roots"], ls.ROOT_COUNT)
        self.assertTrue(self.inv["single_rib"]["tip"])
        self.assertGreaterEqual(self.inv["single_rib"]["courses"], 5)

    def test_it_is_a_spine_and_never_a_weapon(self):
        # canon: "A spine, not a weapon." No dish, antenna, radar, mast, turret ring, trunnion or muzzle.
        for mesh in (self.m0, self.contact, self.damaged, self.destroyed, self.m1):
            inv = ls.contract_inventory(mesh, self.s, self.clips)
            self.assertEqual(inv["not_a_weapon"]["violations"], [])
        # and the rig has no aiming structure: two lean bones, no yaw ring or pitch trunnion
        names = [b.name for b in self.s.bones]
        self.assertEqual(names, ["root", "socket", "spine_lower", "spine_upper"])
        self.assertFalse([n for n in names if "yaw" in n or "pitch" in n or "turret" in n])

    def test_there_is_exactly_one_rib(self):
        tips = [c for c in self.m0.components() if c == "rib_tip"]
        self.assertEqual(len(tips), 1)

    def test_traced_concept_proportions(self):
        m = ls.measurements(self.m0)
        self.assertAlmostEqual(m["height_over_root_disc"], 1.87, delta=0.03)
        self.assertAlmostEqual(m["rib_base_width_over_height"], 0.176, delta=0.01)
        self.assertAlmostEqual(m["tip_offset_over_height"], 0.125, delta=0.01)

    def test_the_rib_curves_and_tapers(self):
        widths = [ls.rib_curve(t / 8.0)[2] for t in range(9)]
        for a, b in zip(widths, widths[1:]):
            self.assertLess(b, a, "the rib narrows all the way to the tip")
        offsets = [ls.rib_curve(t / 8.0)[0] for t in range(9)]
        for a, b in zip(offsets, offsets[1:]):
            self.assertGreater(b, a - 1e-9, "and leans further out as it rises")
        self.assertGreater(offsets[-1], 60.0, "the tip finishes well off the base axis")

    def test_the_nodules_climb_the_concave_leading_edge(self):
        # the candidate puts the nodule line on the inside of the curve; the rib bends toward +X, so
        # every nodule must sit on the -X side of the rib centre at its own height
        for n in range(1, ls.NODULE_COUNT + 1):
            b = self.m0.component_bounds(f"nodule_{n:02d}")
            mid_z = (b[0][2] + b[1][2]) / 2.0
            t = (mid_z - ls.SOCKET_Z) / (ls.TOTAL_Z - ls.SOCKET_Z)
            centre_x = ls.rib_curve(max(0.0, min(1.0, t)))[0]
            nodule_x = (b[0][0] + b[1][0]) / 2.0
            self.assertLess(nodule_x, centre_x, f"nodule {n} sits on the concave side")

    def test_the_nodules_are_individually_addressable_and_ordered(self):
        heights = [(self.m0.component_bounds(f"nodule_{n:02d}")[0][2] +
                    self.m0.component_bounds(f"nodule_{n:02d}")[1][2]) / 2.0
                   for n in range(1, ls.NODULE_COUNT + 1)]
        for a, b in zip(heights, heights[1:]):
            self.assertLess(a, b, "nodules are ordered base to tip so a sequence can run along them")
        self.assertEqual(ls.lit_nodules(self.m0), [])
        self.assertEqual(ls.lit_nodules(self.contact), list(range(1, ls.NODULE_COUNT + 1)))

    def test_material_slots_and_amber_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [ls.STRATA, ls.AMBER])
        lit = {p.component for p in self.contact.polygons if self.contact.slots[p.slot] == ls.AMBER}
        self.assertTrue(all(c.startswith("nodule_") for c in lit), lit)
        self.assertLessEqual(ls.slot_area_fraction(self.contact, ls.AMBER), 0.15)  # REL-ART-029 ceiling

    def test_the_rib_leans_it_does_not_aim(self):
        sweep = {c.name: c for c in self.clips}["detect_sweep"]
        self.assertEqual(sorted(sweep.tracks), ["spine_lower", "spine_upper"])
        for keys in sweep.tracks.values():
            for k in keys:
                self.assertEqual(k.translation_cm, (0.0, 0.0, 0.0), "a lean, not a translation")
                self.assertEqual(k.rotation_deg[0], 0.0)
                self.assertEqual(k.rotation_deg[2], 0.0)
        upper = [k.rotation_deg[1] for k in sweep.tracks["spine_upper"]]
        self.assertGreater(max(upper), 5.0, "the upper rib leads the lean")
        lower = [k.rotation_deg[1] for k in sweep.tracks["spine_lower"]]
        self.assertLess(max(lower), max(upper), "and the lower rib bends less than the upper")

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips),
                         ["detect_sweep", "idle_pulse", "offline", "restore"])

    def test_the_socket_and_roots_ride_the_socket_bone(self):
        for c in self.m0.components():
            if c.startswith(("socket_step_", "root_")):
                bones = {p.bone for p in self.m0.polygons if p.component == c}
                self.assertEqual(bones, {"socket"}, c)

    def test_flat_roots_respect_the_ground_clutter_ceiling(self):
        self.assertLessEqual(ls.ROOT_FLAT_Z + ls.ROOT_FLAT_R, 20.0)  # REL-ART-030

    def test_everything_stays_inside_the_two_by_two_footprint(self):
        for mesh, label in ((self.m0, "listening"), (self.m1, "LOD1"), (self.contact, "contact"),
                            (self.damaged, "damaged"), (self.destroyed, "destroyed")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), ls.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, -1e-6, label)

    def test_no_posed_frame_leaves_the_ground_or_the_footprint(self):
        by_name = {c.name: c for c in self.clips}
        for name, fraction, state in ls.POSE_SAMPLES:
            mesh = ls.posed(0, ls.sample_pose(by_name[name], fraction), state)
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertGreaterEqual(z0, -1e-6, f"{name}@{fraction}")
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), ls.HALF + 1e-6, f"{name}@{fraction}")

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(ls.SOCKETS))
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Nodule_Base"].position[2], by_name["Nodule_Tip"].position[2])
        self.assertEqual(self.socks["Nodule_Tip"], "spine_upper")
        self.assertEqual(self.socks["Socket_Root"], "root")

    def test_destroyed_snaps_at_the_socket(self):
        comps = self.destroyed.components()
        self.assertFalse([c for c in comps if c.startswith(("rib_", "nodule_"))])
        self.assertTrue([c for c in comps if c.startswith("socket_step_")])
        self.assertTrue([c for c in comps if c.startswith("fallen_rib_")])
        self.assertLess(self.destroyed.bounds()[1][2], ls.SOCKET_Z)

    def test_provisional_card_budget(self):
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-BLD-016.KA.SPINE.ASSET", data["provisional_contract"]["card"])
            self.assertIn("PENDING", data["acceptance"]["technical"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        lod0 = max(lod0 or self.m0.triangle_count(), self.contact.triangle_count())
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(lod0, 3000)
        self.assertLessEqual(lod1, 1200)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_keeps_every_nodule_resolvable(self):
        # card .READABILITY: the sequence is the asset's only information channel and shall survive LOD1
        self.assertEqual(sum(1 for c in self.m1.components() if c.startswith("nodule_")), ls.NODULE_COUNT)

    def test_states_are_declared(self):
        self.assertEqual(tuple(ls.STATES), ("listening", "contact", "damaged", "destroyed"))
        with self.assertRaises(ValueError):
            ls.assemble(0, "detecting")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = ls.assemble(0, "listening")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = ls.assemble(0, "listening")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(ls.REVISION, f"{ls.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
