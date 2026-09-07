#!/usr/bin/env python3
"""Regression checks for the EBS-MER-BLD-004 Aegis Post concept blockout.

Author: Angelis Pseftis. Run: python3 test_aegis_build.py
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
import build_aegis as ap  # noqa: E402


class AegisBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = ap.assemble(0, "powered")
        cls.m1 = ap.assemble(1, "powered")[0]
        cls.off = ap.assemble(0, "offline")[0]
        cls.clips = ap.build_clips(cls.s)
        cls.inv = ap.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_three_legs_not_four_and_not_a_pedestal(self):
        self.assertEqual(self.inv["three_legged_mount"]["built"], 3)
        self.assertEqual(self.inv["three_legged_mount"]["knees"], 3)
        self.assertEqual(len(ap.LEG_YAWS), 3)
        forward = [y for y in ap.LEG_YAWS if abs(y) < 90.0]
        self.assertEqual(len(forward), 2, "two legs forward, one to the rear (concept fidelity section 1)")

    def test_twin_emitters_side_by_side_and_level(self):
        self.assertEqual(self.inv["twin_emitters"]["built"], 2)
        left = self.m0.component_bounds("barrel_l")
        right = self.m0.component_bounds("barrel_r")
        self.assertLess(left[1][1], 0.0)
        self.assertGreater(right[0][1], 0.0)
        for axis in (0, 2):
            self.assertAlmostEqual(left[0][axis], right[0][axis], places=6, msg="the barrels are parallel and level")

    def test_material_slots_and_the_band_is_the_only_status_geometry(self):
        self.assertEqual(self.m0.slots, [ap.CERAMIC, ap.FRAME, ap.STATUS])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == ap.STATUS}
        self.assertTrue(lit)
        self.assertTrue(all(c.startswith(("supply_band_", "muzzle_core_")) for c in lit), lit)

    def test_offline_darkens_every_status_surface(self):
        self.assertNotIn(ap.STATUS, {self.off.slots[p.slot] for p in self.off.polygons})
        self.assertEqual(sorted(self.off.components()), sorted(self.m0.components()),
                         "offline changes slots and the pitch pose, never the component set")

    def test_the_offline_clip_droops_the_head_nose_down(self):
        clip = {c.name: c for c in self.clips}["offline"]
        keys = clip.tracks["turret_pitch"]
        self.assertLess(keys[-1].rotation_deg[0], -20.0)
        self.assertAlmostEqual(keys[0].rotation_deg[0], 0.0, places=6)

    def test_two_aiming_bones_plus_the_root(self):
        names = [b.name for b in self.s.bones]
        self.assertEqual(names, ["root", "turret_yaw", "turret_pitch"])
        self.assertEqual(self.s.bones[2].parent, "turret_yaw")

    def test_head_and_barrels_ride_the_pitch_bone_and_the_hub_rides_the_yaw(self):
        for comp in ("head_shell", "barrel_l", "barrel_r", "supply_band_f"):
            self.assertEqual(self.counts_bone(comp), "turret_pitch", comp)
        self.assertEqual(self.counts_bone("hub"), "turret_yaw")
        for comp in ("leg_01_pad", "power_coupling"):
            self.assertEqual(self.counts_bone(comp), "root", comp)

    def counts_bone(self, component):
        bones = {self.m0.bone_names[p.bone] if hasattr(self.m0, "bone_names") else p.bone
                 for p in self.m0.polygons if p.component == component}
        self.assertEqual(len(bones), 1, component)
        return bones.pop()

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(ap.SOCKETS))
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertGreater(by_name["Muzzle_Left"].position[0], 0.0, "the muzzles face +X, the rest facing")
        self.assertLess(by_name["Muzzle_Left"].position[1], by_name["Muzzle_Right"].position[1])
        self.assertLess(by_name["Power_Coupling"].position[0], 0.0, "the coupling is on the rear of the hub")
        self.assertEqual(self.socks["Muzzle_Left"], "turret_pitch")
        self.assertEqual(self.socks["Power_Coupling"], "root")

    def test_the_power_coupling_reads_as_the_supply_connection(self):
        self.assertEqual(self.inv["power_coupling"]["cables"], 2)
        block = self.m0.component_bounds("power_coupling")
        self.assertLess(block[1][0], 0.0)
        for cable in ("power_cable_01", "power_cable_02"):
            self.assertLess(self.m0.component_bounds(cable)[0][2], 40.0, "the cables run down to the ground")

    def test_concept_proportions(self):
        m = ap.measurements(self.m0)
        # measured off the candidate's main POWERED view by a silhouette trace (README section 8.1):
        # long splayed legs carrying 0.85 of the height under a compact, wide, flat head
        self.assertAlmostEqual(m["height_over_leg_spread"], 0.87, delta=0.02)
        self.assertAlmostEqual(m["head_width_over_leg_spread"], 0.60, delta=0.02)
        self.assertAlmostEqual(m["hub_top_over_height"], 0.85, delta=0.02)
        self.assertAlmostEqual(ap.HEAD_SIZE[2] / m["height_cm"], 0.14, delta=0.02)

    def test_footprint_and_ground(self):
        for mesh, label in ((self.m0, "LOD0"), (self.m1, "LOD1"), (self.off, "offline")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), ap.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, 0.0, label)

    def test_no_posed_frame_leaves_the_ground_or_the_footprint(self):
        by_name = {c.name: c for c in self.clips}
        for name, fraction, state in ap.POSE_SAMPLES:
            mesh = ap.posed(0, ap.sample_pose(by_name[name], fraction), state)
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertGreaterEqual(z0, -1e-6, f"{name}@{fraction}")
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), ap.HALF + 1e-6, f"{name}@{fraction}")

    def test_every_clip_is_frame_aligned_at_thirty_fps(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)

    def test_whole_asset_budget(self):
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
        self.assertLessEqual(lod0, 4000)
        self.assertLessEqual(lod1, 1600)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_states_are_declared(self):
        self.assertEqual(tuple(ap.STATES), ("powered", "offline"))
        with self.assertRaises(ValueError):
            ap.assemble(0, "destroyed")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = ap.assemble(0, "powered")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = ap.assemble(0, "powered")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(ap.REVISION, f"{ap.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
