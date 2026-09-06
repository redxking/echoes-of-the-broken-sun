#!/usr/bin/env python3
"""Regression checks for the EBS-MER-UNT-001 Surveyor blockout source.

Author: Angelis Pseftis. Run: python3 test_surveyor_build.py
Structural checks binding the contract inventory, rig, budgets, stance and clips to the
generator; not gate acceptance.
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
import build_surveyor as sv  # noqa: E402


class SurveyorBlockoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mesh0, cls.skel, cls.counts, cls.sockets_on_bones = sv.assemble(0)
        cls.mesh1 = sv.assemble(1)[0]
        cls.clips = sv.build_clips(cls.skel)
        cls.inventory = sv.contract_inventory(cls.mesh0, cls.skel, cls.clips)

    def test_contract_component_counts(self):
        inv = self.inventory
        self.assertEqual(inv["legs"]["built"], 2)
        self.assertEqual(inv["tool_arms"]["built"], 2)
        self.assertEqual(inv["tool_arms"]["right"], "drill")
        self.assertEqual(inv["tool_arms"]["left"], "gripper")
        self.assertEqual(inv["rear_canisters"]["built"], 3)
        self.assertEqual(inv["optical_mast"]["built"], 1)
        self.assertEqual(inv["feet"]["built"], 2)

    def test_twelve_bone_rig_and_hierarchy(self):
        names = [b.name for b in self.skel.bones]
        self.assertEqual(len(names), 12)
        self.assertEqual(names[0], "root")
        for side in ("r", "l"):
            self.assertEqual(self.skel.get(f"{side}_thigh").parent, "body")
            self.assertEqual(self.skel.get(f"{side}_shin").parent, f"{side}_thigh")
            self.assertEqual(self.skel.get(f"{side}_foot").parent, f"{side}_shin")
            self.assertEqual(self.skel.get(f"{side}_shoulder").parent, "body")
            self.assertEqual(self.skel.get(f"{side}_forearm").parent, f"{side}_shoulder")
        # anatomical right is +Y in the Unreal frame
        self.assertGreater(self.skel.get("r_thigh").head[1], 0.0)
        self.assertLess(self.skel.get("l_thigh").head[1], 0.0)

    def test_every_polygon_bound_and_no_root_geometry(self):
        self.assertTrue(all(getattr(p, "bone", None) for p in self.mesh0.polygons))
        self.assertNotIn("root", self.counts)
        self.assertEqual(set(self.counts) | {"root"}, {b.name for b in self.skel.bones})

    def test_sockets_named_and_on_contract_bones(self):
        names = sorted(s.name for s in self.mesh0.sockets)
        self.assertEqual(names, ["Cargo_Drop_Anchor", "Center_Hitbox_Socket", "Harvest_Tether_Muzzle"])
        self.assertEqual(self.sockets_on_bones["Harvest_Tether_Muzzle"], "r_forearm")
        self.assertEqual(self.sockets_on_bones["Cargo_Drop_Anchor"], "body")
        muzzle = next(s for s in self.mesh0.sockets if s.name == "Harvest_Tether_Muzzle")
        self.assertGreater(muzzle.position[1], 0.0)   # drill on the anatomical right (+Y)
        self.assertGreater(muzzle.position[0], 60.0)  # reaches forward

    def test_rest_stance_grounded_and_person_sized(self):
        (x0, y0, z0), (x1, y1, z1) = self.mesh0.bounds()
        self.assertAlmostEqual(z0, 0.0, places=3)
        self.assertTrue(150.0 <= z1 <= 200.0, z1)
        self.assertTrue(100.0 <= (y1 - y0) <= 140.0, y1 - y0)
        for side in ("r", "l"):
            (fx0, fy0, fz0), (fx1, fy1, fz1) = self.mesh0.component_bounds(f"{side}_foot_foot_sole")
            self.assertAlmostEqual(fz0, 0.0, places=3)  # planted feet
        p = sv.rest_positions()
        self.assertLess(p["knee"][2], sv.HIP_Z)
        self.assertGreater(p["knee"][0], 0.0)  # knee forward of the hip
        self.assertLess(p["ankle"][0], p["knee"][0])  # shin runs back down to the ankle

    def test_limbs_point_downward_in_rest(self):
        for comp in ("r_thigh_thigh_strut", "r_shin_shin_strut", "l_thigh_thigh_strut", "l_shin_shin_strut"):
            (x0, y0, z0), (x1, y1, z1) = self.mesh0.component_bounds(comp)
            self.assertGreater(z1 - z0, 20.0, comp)

    def test_budgets(self):
        self.assertLessEqual(self.mesh0.triangle_count(), 4500)
        self.assertLessEqual(self.mesh1.triangle_count(), 1800)
        self.assertLess(self.mesh1.triangle_count(), self.mesh0.triangle_count())

    def test_two_export_slots(self):
        ex = sv.export_mesh(self.mesh0)
        self.assertEqual(ex.slots, [sv.FRAME, sv.CERAMIC])
        self.assertEqual(ex.triangle_count(), self.mesh0.triangle_count())
        self.assertTrue(all(getattr(p, "bone", None) for p in ex.polygons))

    def test_clip_inventory_matches_contract(self):
        names = [c.name for c in self.clips]
        for required in ("idle", "move", "turn", "stop", "damage", "death", "cancel", "restore", "gather", "carry", "deliver", "build", "repair_when_authorized"):
            self.assertIn(required, names)
        move = next(c for c in self.clips if c.name == "move")
        self.assertTrue(move.loop)
        for side in ("r", "l"):
            self.assertIn(f"{side}_thigh", move.tracks)
            self.assertIn(f"{side}_foot", move.tracks)
        self.assertNotIn("root", move.tracks)  # no root motion
        restore = next(c for c in self.clips if c.name == "restore")
        self.assertEqual(restore.duration_s, 0.0)
        repair = next(c for c in self.clips if c.name == "repair_when_authorized")
        self.assertIn("l_forearm", repair.tracks)
        self.assertNotIn("r_forearm", repair.tracks)  # welder arm only

    def test_keyframes_sorted_and_within_duration(self):
        for clip in self.clips:
            for bone, keys in clip.tracks.items():
                times = [k.time_s for k in keys]
                self.assertEqual(times, sorted(times), clip.name)
                self.assertLessEqual(times[-1], clip.duration_s + 1e-9, clip.name)
                self.assertIn(bone, [b.name for b in self.skel.bones])

    def test_deterministic_static_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = sv.export_mesh(sv.assemble(0)[0]).write_glb(os.path.join(tmp, "a.glb"), include_collision=False)
            b = sv.export_mesh(sv.assemble(0)[0]).write_glb(os.path.join(tmp, "b.glb"), include_collision=False)
            self.assertEqual(a, b)

    def test_clips_never_break_the_ground_plane(self):
        # Every clip sampled at 20 ms: no vertex below z = -1 cm (feet planted, tools rest on the ground at most).
        import pose_review as pr
        mesh, skeleton, _counts, sockets = sv.assemble(0)
        for clip in sv.build_clips(skeleton):
            steps = max(1, int(round(clip.duration_s / 0.02)))
            for i in range(steps + 1):
                fraction = i / steps
                pose = pr.sample(clip, fraction)
                posed = skel.pose_mesh(mesh, skeleton, pose)
                (_, _, z0), _ = posed.bounds()
                self.assertGreaterEqual(z0, -1.0, f"{clip.name} at {fraction:.2f}: lowest z {z0:.1f}")

    def test_hold_poses_keep_soles_planted(self):
        # Work leans and the deliver dip keep both soles on the ground (|z| <= 1.5 cm) throughout.
        import pose_review as pr
        mesh, skeleton, _counts, _sockets = sv.assemble(0)
        for clip in sv.build_clips(skeleton):
            if clip.name not in ("idle", "stop", "gather", "build", "repair_when_authorized", "deliver", "damage", "cancel"):  # turn is a shuffle: feet lift
                continue
            steps = max(1, int(round(clip.duration_s / 0.05)))
            for i in range(steps + 1):
                posed = skel.pose_mesh(mesh, skeleton, pr.sample(clip, i / steps))
                for comp in ("r_foot_foot_sole", "l_foot_foot_sole"):
                    (_, _, z0), _ = posed.component_bounds(comp)
                    self.assertLessEqual(abs(z0), 1.5, f"{clip.name} at {i / steps:.2f}: {comp} sole at {z0:.1f}")

    def test_death_settles_low_with_tools_on_the_ground(self):
        import pose_review as pr
        mesh, skeleton, _counts, sockets = sv.assemble(0)
        death = next(c for c in sv.build_clips(skeleton) if c.name == "death")
        pose = pr.sample(death, 1.0)
        pose["_sockets"] = sockets
        posed = skel.pose_mesh(mesh, skeleton, pose)
        _, (_, _, top) = posed.bounds()
        self.assertLess(top, 0.65 * 176.0)
        muzzle = next(s for s in posed.sockets if s.name == "Harvest_Tether_Muzzle")
        self.assertLess(muzzle.position[2], 6.0)
        self.assertGreater(muzzle.position[0], 0.0)  # drill splays ahead, not under the frame


if __name__ == "__main__":
    unittest.main(verbosity=1)
