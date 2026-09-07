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
        # concept-fidelity.md item 8: span across the arms 1.15-1.30 H (H = 176), and wider than tall
        H = z1
        self.assertTrue(1.15 * H <= (y1 - y0) <= 1.30 * H, y1 - y0)
        self.assertGreater(y1 - y0, z1)  # wider than tall in the front view
        for side in ("r", "l"):
            (fx0, fy0, fz0), (fx1, fy1, fz1) = self.mesh0.component_bounds(f"{side}_foot_foot_sole")
            self.assertAlmostEqual(fz0, 0.0, places=3)  # planted feet
        m = sv.concept_measurements(self.mesh0)
        # item 1: tool tips 5-15 cm up, 60-90 cm ahead of the hip line
        self.assertTrue(5.0 <= m["drill_tip_height_cm"] <= 15.0, m["drill_tip_height_cm"])
        self.assertTrue(60.0 <= m["drill_tip_ahead_of_hip_line_cm"] <= 90.0, m["drill_tip_ahead_of_hip_line_cm"])
        # item 2 (corrected to the concept pixels): feet ~0.45 H apart
        self.assertTrue(0.40 <= m["feet_apart_over_H"] <= 0.50, m["feet_apart_over_H"])
        # items 3-5: sides as read on the KEEP concept (README section 8 for the derived-panel mirror)
        self.assertEqual(m["cradle_side"], "rear-right (+Y)")
        self.assertEqual(m["mast_side"], "rear-left (-Y)")
        self.assertEqual(m["lens_side"], "front upper-left (-Y)")

    def test_legs_are_vertical_columns(self):
        # concept pixels (surveyor_concept_2x.png legs; reference front and REAR VIEW panels): thigh and shin
        # near-vertical, knee under or slightly behind the hip, ankle under or slightly ahead of the knee
        p = sv.rest_positions()
        self.assertLess(p["knee"][2], sv.HIP[2])
        self.assertLessEqual(p["knee"][0], sv.HIP[0])          # knee under/behind the hip
        self.assertGreaterEqual(p["ankle"][0], p["knee"][0])  # ankle under/ahead of the knee
        m = sv.concept_measurements(self.mesh0)
        self.assertLessEqual(abs(m["thigh_lean_deg_from_vertical"]), 12.0, m["thigh_lean_deg_from_vertical"])
        self.assertLessEqual(abs(m["shin_lean_deg_from_vertical"]), 15.0, m["shin_lean_deg_from_vertical"])
        # knee hub at mid leg height; foot centred on the ankle with the toe plate longer than the heel block
        self.assertTrue(0.4 <= (p["knee"][2] - sv.ANKLE_Z) / (sv.HIP[2] - sv.ANKLE_Z) <= 0.6)
        for side in ("r", "l"):
            (sx0, _, _), (sx1, _, _) = self.mesh0.component_bounds(f"{side}_foot_foot_sole")
            ankle_x = self.skel.get(f"{side}_foot").head[0]
            self.assertAlmostEqual((sx0 + sx1) / 2.0, ankle_x, places=3)
            toe = self.mesh0.component_bounds(f"{side}_foot_foot_toe_plate")
            heel = self.mesh0.component_bounds(f"{side}_foot_heel_block")
            self.assertGreater(toe[1][0] - toe[0][0], heel[1][0] - heel[0][0])
        # the plated front faces of the leg columns face forward (status strips ahead of the strut)
        for side in ("r", "l"):
            strut = self.mesh0.component_bounds(f"{side}_thigh_thigh_strut")
            status = self.mesh0.component_bounds(f"{side}_thigh_thigh_status")
            self.assertGreater(status[0][0], (strut[0][0] + strut[1][0]) / 2.0, side)

    def test_cradle_hangs_on_the_rear_at_shoulder_height(self):
        # concept: rack level with the shoulder hub, canister caps only just above the torso top, tray flush
        # with the flank (not on top of the torso, not overhanging the flank)
        m = sv.concept_measurements(self.mesh0)
        self.assertTrue(3.0 <= m["canister_cap_above_torso_top_cm"] <= 12.0, m["canister_cap_above_torso_top_cm"])
        self.assertTrue(0.45 <= m["cradle_floor_over_torso_height"] <= 0.80, m["cradle_floor_over_torso_height"])
        self.assertLessEqual(m["cradle_tray_y_max_cm"], m["torso_flank_y_cm"] + 4.0)
        torso_x_min = self.mesh0.component_bounds("body_torso_frame")[0][0]
        self.assertLess(m["cradle_tray_x_cm"][1], torso_x_min)  # tray entirely behind the rear face
        for k in (1, 2, 3):
            (_, _, cz0), (_, _, cz1) = self.mesh0.component_bounds(f"body_canister_{k:02d}")
            self.assertAlmostEqual(cz0, sv.CRADLE_FLOOR_Z, places=3)
        anchor = next(s for s in self.mesh0.sockets if s.name == "Cargo_Drop_Anchor")
        self.assertAlmostEqual(anchor.position[2], sv.CRADLE_FLOOR_Z + sv.CANISTER_H / 2.0, places=3)
        self.assertLess(anchor.position[0], torso_x_min)
        self.assertLess(self.mesh0.bounds()[1][2] - 1e-6, 176.0 + 1e-6)  # mast cap still sets H

    def test_limbs_point_downward_in_rest(self):
        for comp in ("r_thigh_thigh_strut", "r_shin_shin_strut", "l_thigh_thigh_strut", "l_shin_shin_strut"):
            (x0, y0, z0), (x1, y1, z1) = self.mesh0.component_bounds(comp)
            self.assertGreater(z1 - z0, 20.0, comp)

    def test_compatibility_parts_against_runtime_constants(self):
        # The M01 static-part route (EchoesM01SurveyorRig.cpp 42/46/+10) is recorded, not assumed:
        # the exported part lengths must match the manifest record, and any mismatch beyond the
        # tolerance must be flagged requires_runtime_change (README section 8).
        rec = sv.compatibility_parts_record()
        for lod in (0, 1):
            thigh = sv.part_upper_leg(lod).component_bounds("thigh_strut")
            shin = sv.part_lower_leg(lod).component_bounds("shin_strut")
            self.assertAlmostEqual(thigh[1][0] - thigh[0][0], rec["authored"]["thigh_cm"], places=3)
            self.assertAlmostEqual(shin[1][0] - shin[0][0], rec["authored"]["shin_cm"], places=3)
            sole = sv.part_foot(lod).component_bounds("foot_sole")
            self.assertAlmostEqual(-sole[0][2], rec["authored"]["ankle_above_sole_cm"], places=3)
            self.assertAlmostEqual(sole[1][0] - sole[0][0], rec["authored"]["foot_len_cm"], places=3)
        tol = rec["runtime"]["tolerance_cm"]
        mismatch = any(abs(v) > tol for v in rec["delta_cm"].values())
        self.assertEqual(rec["requires_runtime_change"], mismatch)
        self.assertEqual(rec["matches_runtime"], not mismatch)
        self.assertEqual(rec["runtime"]["thigh_cm"], 42.0)
        self.assertEqual(rec["runtime"]["shin_cm"], 46.0)
        self.assertEqual(rec["runtime"]["ankle_above_sole_cm"], 10.0)

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
        for clip in self.clips:
            self.assertNotIn("root", clip.tracks, clip.name)  # no clip keys root (README section 5)
        restore = next(c for c in self.clips if c.name == "restore")
        self.assertEqual(restore.duration_s, 0.0)
        self.assertEqual(sorted(restore.tracks), sorted(b.name for b in self.skel.bones if b.name != "root"))
        for keys in restore.tracks.values():
            self.assertEqual([tuple(k.rotation_deg) for k in keys], [(0.0, 0.0, 0.0)])  # identity rest key
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

    @staticmethod
    def _centre(mesh, component):
        (x0, y0, z0), (x1, y1, z1) = mesh.component_bounds(component)
        return ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0)

    def _posed(self, clip_name, fraction):
        import pose_review as pr
        mesh, skeleton, _counts, sockets = sv.assemble(0)
        clip = next(c for c in sv.build_clips(skeleton) if c.name == clip_name)
        pose = pr.sample(clip, fraction)
        pose["_sockets"] = sockets
        return skel.pose_mesh(mesh, skeleton, pose)

    def _drill_elevation_deg(self, posed):
        import math
        muzzle = next(s for s in posed.sockets if s.name == "Harvest_Tether_Muzzle").position
        housing = self._centre(posed, "r_forearm_drill_housing")
        axis = kit.v_norm(kit.v_sub(muzzle, housing))
        return math.degrees(math.asin(axis[2]))

    def test_death_settles_low_with_tools_on_the_ground(self):
        # concept-fidelity.md rig section: the torso drops forward onto its arms (the long arms carry it).
        # The rear mast rises as the frame pitches onto its elbows, so the held pose tops at ~0.73 H.
        posed = self._posed("death", 1.0)
        _, (_, _, top) = posed.bounds()
        self.assertLess(top, 0.75 * 176.0)
        muzzle = next(s for s in posed.sockets if s.name == "Harvest_Tether_Muzzle")
        self.assertLess(muzzle.position[2], 6.0)
        self.assertGreater(muzzle.position[0], 0.0)  # drill splays ahead, not under the frame
        torso = posed.component_bounds("body_torso_frame")
        self.assertTrue(20.0 <= torso[0][2] <= 40.0, torso)  # torso front edge propped ~30 cm up
        for side in ("r", "l"):
            elbow = self._centre(posed, f"{side}_forearm_elbow_hub")
            self.assertTrue(30.0 <= elbow[2] <= 50.0, elbow)                 # elbows under the torso front
            self.assertTrue(torso[0][0] <= elbow[0] <= torso[1][0] + 25.0, elbow)

    def test_gather_drills_into_the_ground_ahead(self):
        # GATHERING panel: crouched, elbow high, bit driven steeply into the ground ahead of the toe
        posed = self._posed("gather", 0.5)
        muzzle = next(s for s in posed.sockets if s.name == "Harvest_Tether_Muzzle").position
        toe_x = posed.component_bounds("r_foot_foot_sole")[1][0]
        self.assertTrue(-0.5 <= muzzle[2] <= 4.0, muzzle)
        self.assertTrue(60.0 <= muzzle[0] - toe_x <= 90.0, muzzle[0] - toe_x)
        self.assertLessEqual(self._drill_elevation_deg(posed), -50.0)
        elbow = self._centre(posed, "r_forearm_elbow_hub")
        self.assertGreater(elbow[2], self._centre(posed, "r_forearm_drill_housing")[2] + 20.0)  # elbow above the housing
        _, (_, _, top) = posed.bounds()
        self.assertLess(top, 176.0 - 12.0)  # body lowered

    def test_deliver_squats_with_tools_folded_down(self):
        # DELIVERY panel: ~31 cm squat with both tools pointing down, tips clear of the ground
        posed = self._posed("deliver", 0.5)
        muzzle = next(s for s in posed.sockets if s.name == "Harvest_Tether_Muzzle").position
        toe_x = posed.component_bounds("r_foot_foot_sole")[1][0]
        self.assertTrue(5.0 <= muzzle[2] <= 20.0, muzzle)
        self.assertTrue(55.0 <= muzzle[0] - toe_x <= 80.0, muzzle[0] - toe_x)
        self.assertLessEqual(self._drill_elevation_deg(posed), -45.0)
        grip = self._centre(posed, "l_forearm_gripper_tip_r")
        self.assertTrue(5.0 <= grip[2] <= 22.0, grip)
        _, (_, _, top) = posed.bounds()
        self.assertTrue(176.0 - 40.0 <= top <= 176.0 - 22.0, top)  # ~31 cm drop


if __name__ == "__main__":
    unittest.main(verbosity=1)
