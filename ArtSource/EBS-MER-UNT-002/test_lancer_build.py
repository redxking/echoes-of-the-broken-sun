#!/usr/bin/env python3
"""Regression checks for the EBS-MER-UNT-002 Lancer blockout source.

Author: Angelis Pseftis. Run: python3 test_lancer_build.py
Structural checks binding the concept inventory, the measured proportions from concept-fidelity.md,
the 18-bone rig, the card sockets and sub-objects, the budgets, the clip states and the ground rule
to the generator. Not gate acceptance.
"""
from __future__ import annotations

import math
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import ebs_skelkit as skel  # noqa: E402
import build_lancer as bl  # noqa: E402
import pose_review as pr  # noqa: E402

H = bl.H


class LancerBlockoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mesh0, cls.skel, cls.counts, cls.sockets_on_bones = bl.assemble(0)
        cls.mesh1 = bl.assemble(1)[0]
        cls.clips = bl.build_clips(cls.skel)
        cls.inventory = bl.contract_inventory(cls.mesh0, cls.skel, cls.clips)
        cls.m = bl.concept_measurements(cls.mesh0)

    # --- contract inventory: every concept part is present -----------------------------------
    def test_contract_inventory(self):
        inv = self.inventory
        for key, expected in (("legs", 2), ("feet", 2), ("toe_plates", 2), ("knee_hubs", 2),
                              ("shoulder_pods", 2), ("antennas", 2), ("sensor_cowl", 1),
                              ("hip_mount", 1), ("recoil_strut", 1), ("rail_lance", 1),
                              ("exposed_flank_struts", 4)):
            self.assertEqual(inv[key]["built"], expected, key)
        lance = inv["rail_lance"]
        self.assertEqual(lance["rails"], 2)          # concept item 2: a stack of two parallel rails
        self.assertEqual(lance["channel"], 1)        # one continuous cyan channel between them
        self.assertEqual(lance["clamps"], 3)         # three collar clamps along its length
        self.assertEqual(lance["muzzle_head"], 1)
        self.assertEqual(inv["sensor_cowl"]["visor"], 1)
        self.assertEqual(inv["recoil_strut"]["slide_section"], 1)
        self.assertEqual(inv["ceramic_skirt"]["built"], 0)   # concept item 5: flanks stay open
        self.assertTrue(self.m["no_ceramic_skirt"])

    # --- measured proportions from concept-fidelity.md (as corrected on the pixels) -----------
    def test_scale_and_stance_proportions(self):
        m = self.m
        self.assertAlmostEqual(m["H_pod_top_cm"], H, places=3)                 # H measured at the pod tops
        self.assertGreater(m["H_pod_top_cm"], 176.0)                           # canon: taller than the Surveyor
        # item 1, corrected: foot centres 0.60-0.70 H apart, heel-to-toe 0.80-0.95 H, 0.35 H across Y
        self.assertTrue(0.60 <= m["foot_centres_apart_over_H"] <= 0.70, m["foot_centres_apart_over_H"])
        self.assertTrue(0.80 <= m["heel_to_toe_span_over_H"] <= 0.95, m["heel_to_toe_span_over_H"])
        self.assertAlmostEqual(m["feet_apart_across_Y_over_H"], 0.35, places=2)
        self.assertLess(m["heel_to_toe_span_cm"] and -m["body_pitch_deg"], 12.0)  # body pitched ~8 deg forward
        self.assertTrue(6.0 <= -m["body_pitch_deg"] <= 10.0)
        self.assertTrue(m["mirror_brace"])           # lead knee ahead of the hip, trailing knee behind

    def test_lance_proportions(self):
        m = self.m
        # item 2: one lance at hip height, level, on the centreline, 1.05 H long, 0.10 H across
        self.assertTrue(0.62 <= m["lance_axis_over_H"] <= 0.70, m["lance_axis_over_H"])
        self.assertTrue(1.00 <= m["lance_length_over_H"] <= 1.12, m["lance_length_over_H"])
        self.assertTrue(0.09 <= m["lance_rail_section_diameter_over_H"] <= 0.12, m["lance_rail_section_diameter_over_H"])
        self.assertTrue(m["lance_on_centreline"])
        self.assertTrue(0.48 <= m["muzzle_ahead_of_lead_toe_over_H"] <= 0.65, m["muzzle_ahead_of_lead_toe_over_H"])
        self.assertGreater(m["cyan_channel_full_length_fraction"], 0.95)       # continuous, full length
        self.assertEqual(m["collar_clamps"], 3)
        # the lance is level: both rails at the same z across the whole run
        rail = self.mesh0.component_bounds("lance_lance_rail")
        self.assertAlmostEqual((rail[0][2] + rail[1][2]) / 2.0, bl.LANCE_Z, places=3)
        # heaviest single element in the silhouette: the lance is longer than the frame is tall
        self.assertGreater(m["lance_length_cm"], m["H_pod_top_cm"])

    def test_head_and_pod_proportions(self):
        m = self.m
        # item 3: low cowl between the pods, visor at ~0.89 H, cowl top level with or under the pod tops
        self.assertTrue(0.86 <= m["cowl_visor_over_H"] <= 0.93, m["cowl_visor_over_H"])
        self.assertTrue(0.0 <= m["cowl_below_pod_top_cm"] <= 12.0, m["cowl_below_pod_top_cm"])
        # item 4: pods 0.22 H long, 0.13 H tall, tops at H, widest part of the upper body
        self.assertAlmostEqual(m["pod_length_over_H"], 0.22, places=2)
        self.assertAlmostEqual(m["pod_height_over_H"], 0.13, places=2)
        self.assertTrue(m["pods_are_widest_upper"])
        self.assertTrue(0.40 <= m["pod_span_over_H"] <= 0.50, m["pod_span_over_H"])
        self.assertAlmostEqual(max(self.mesh0.component_bounds(f"{side}_pod_{c}")[1][2]
                                   for side in ("r", "l") for c in bl.POD_TOP_COMPONENTS), H, places=3)
        # the cowl sits BETWEEN the pods in Y
        cowl = self.mesh0.component_bounds("cowl_cowl_lower")
        self.assertLess(cowl[1][1], self.mesh0.component_bounds("r_pod_pod_shell")[0][1] + 1e-6)
        self.assertGreater(cowl[0][1], self.mesh0.component_bounds("l_pod_pod_shell")[1][1] - 1e-6)
        # no neck: the cowl reaches down to the deck it sits on
        deck_top = self.mesh0.component_bounds("spine_deck_plate")[1][2]
        self.assertLessEqual(cowl[0][2] - deck_top, 4.0)

    def test_leg_proportions(self):
        m = self.m
        # item 7: thigh 0.30 H, shin 0.25 H, knee disc 0.08 H, block foot 0.22 H with a raised toe plate
        self.assertAlmostEqual(m["thigh_over_H"], 0.30, places=2)
        self.assertAlmostEqual(m["shin_over_H"], 0.25, places=2)
        self.assertAlmostEqual(m["knee_disc_diameter_over_H"], 0.08, places=2)
        self.assertTrue(0.20 <= m["foot_length_over_H"] <= 0.24, m["foot_length_over_H"])
        for side in ("r", "l"):
            plate = self.mesh0.component_bounds(f"{side}_toe_toe_plate")
            block = self.mesh0.component_bounds(f"{side}_toe_toe_block")
            self.assertGreater(plate[0][2], block[0][2])            # the toe plate is raised on the block
            self.assertAlmostEqual(self.mesh0.component_bounds(f"{side}_foot_foot_sole")[0][2], 0.0, places=3)
        # digitigrade: knee above mid-leg, ankle well above the ground
        p = bl.rest_positions()
        for side in ("r", "l"):
            self.assertTrue(0.40 <= (p[f"{side}_knee"][2] - bl.ANKLE_Z) / (bl.HIP[2] - bl.ANKLE_Z) <= 0.65)
        self.assertAlmostEqual(p["r_ankle"][2], bl.ANKLE_Z, places=6)
        self.assertAlmostEqual(p["l_ankle"][2], bl.ANKLE_Z, places=6)
        # both legs are built from parts of identical length (one rig, mirrored stance)
        self.assertAlmostEqual(bl._len(bl.LEAD_THIGH_D), bl._len(bl.TRAIL_THIGH_D), places=6)
        self.assertAlmostEqual(bl._len(bl.LEAD_SHIN_D), bl._len(bl.TRAIL_SHIN_D), places=6)

    def test_upper_assembly_is_carried_back_over_the_hips(self):
        """concept-v2 regression. Both concept side panels, segmented and scaled so H = 200 cm with the
        ground on the sole line and the fore-aft axis anchored on the trailing heel and the leading toe,
        put the 0.80-1.00 H silhouette band's centroid at -30.4 cm (turnaround LEFT SIDE) and -30.2 cm
        (candidate BRACED SIDE), its rear edge at -75.4 / -72.6 and its front edge at +16.6 / +13.4.
        concept-v1 built that centroid at +7.7 cm with the cowl nosing out past the lance yoke."""
        m = self.m
        self.assertTrue(-34.0 <= m["upper_assembly_band_centroid_x_cm"] <= -22.0, m["upper_assembly_band_centroid_x_cm"])
        rear, front = m["upper_assembly_band_x_cm"]
        self.assertLessEqual(rear, -68.0, rear)          # the assembly really overhangs behind the hips
        self.assertLessEqual(front, 24.0, front)         # and does not nose out over the lance mount
        self.assertGreaterEqual(front, 8.0, front)
        # the cowl's front face stays behind the lance yoke's cheeks, as in both concept side panels
        self.assertLess(self.mesh0.component_bounds("cowl_cowl_nose")[1][0],
                        self.mesh0.component_bounds("yaw_yoke_cheek")[0][0])
        # the assembly's own length is unchanged (concept 92-94 cm): only its position moved
        self.assertTrue(88.0 <= front - rear <= 100.0, front - rear)

    def test_lance_section_depth_matches_the_concept(self):
        """concept-v2 regression. Column-by-column on the candidate BRACED SIDE panel at 0.514 cm/px:
        clear shaft 19.0 cm = 0.095 H, thickest collar 22.1 cm = 0.110 H, muzzle end 17.5-19.0 cm.
        concept-v1 built 0.105 H of shaft, 0.131 H of collar and a 0.143 H muzzle strip."""
        m = self.m
        self.assertTrue(0.090 <= m["lance_shaft_depth_over_H"] <= 0.100, m["lance_shaft_depth_over_H"])
        self.assertTrue(0.104 <= m["lance_collar_depth_over_H"] <= 0.116, m["lance_collar_depth_over_H"])
        self.assertLessEqual(m["lance_muzzle_head_depth_over_H"], 0.100, m["lance_muzzle_head_depth_over_H"])
        # nothing on the lance forward of the breech stands proud of the concept's thickest collar
        for comp in self.mesh0.components():
            if not comp.startswith("lance_") or "breech" in comp:
                continue
            lo, hi = self.mesh0.component_bounds(comp)
            if lo[0] >= bl.CLAMP_X[0] - 10.0:
                self.assertLessEqual((hi[2] - lo[2]) / H, 0.116, comp)

    def test_cyan_channel_reads_as_one_stroke(self):
        """concept-v2 regression. The concept keeps its collars clear of the cyan line so it survives as
        ONE stroke in the LEFT SIDE and TOP panels; concept-v1's 26 cm collars cut the 8 cm channel into
        four dashes whose longest run was 34 cm."""
        m = self.m
        self.assertEqual(m["cyan_side_profile_runs"], 1, m)
        self.assertGreaterEqual(m["cyan_side_profile_longest_run_cm"], 120.0, m["cyan_side_profile_longest_run_cm"])
        self.assertGreater(m["cyan_channel_full_length_fraction"], 0.95)
        # every collar and the muzzle head clear the channel band in z (that is what keeps the run whole)
        channel = self.mesh0.component_bounds("lance_lance_channel")
        for comp in ("lance_lance_clamp_01", "lance_lance_clamp_02", "lance_lance_clamp_03", "lance_lance_muzzle_head"):
            for poly in self.mesh0.polygons:
                if poly.component != comp:
                    continue
                zs = [p[2] for p in poly.points]
                self.assertTrue(min(zs) >= channel[1][2] - 1e-6 or max(zs) <= channel[0][2] + 1e-6, comp)
        self.assertNotIn("lance_lance_muzzle_status", self.mesh0.components())   # it sat on top of the collars

    def test_ceramic_forms_outer_faces_not_the_rear(self):
        """concept-v2 regression. A 2.2x read of the concept REAR panel shows hub stacks, struts and brass
        edges only: the pale plate is a cap on the pod tops, not a rear surface. concept-v1 faced the full
        68 cm deck plate, both pod rear faces and the full-box leg ceramic aft."""
        m = self.m
        self.assertLessEqual(m["ceramic_rear_over_front"], 0.62, m["ceramic_rear_over_front"])
        self.assertLess(m["ceramic_rear_facing_fraction"], m["ceramic_front_facing_fraction"])
        # the charcoal deck frame is the rearmost deck surface, not the ceramic plate
        plate = self.mesh0.component_bounds("spine_deck_plate")
        frame = self.mesh0.component_bounds("spine_deck_frame")
        self.assertLess(frame[0][0], plate[0][0] - 3.0)
        self.assertGreaterEqual(frame[1][1] - frame[0][1], plate[1][1] - plate[0][1])
        # each pod closes aft with charcoal under a pale top cap
        for side in ("r", "l"):
            back = self.mesh0.component_bounds(f"{side}_pod_pod_back")
            shell = self.mesh0.component_bounds(f"{side}_pod_pod_shell")
            self.assertLess(back[0][0], shell[0][0])
            self.assertLess(back[1][2], shell[1][2])
        # the leg ceramic is a pair of outer-face plates inboard of the charcoal strut in the limb's z
        for part, plate_c, strut_c in ((bl.part_thigh(0), "thigh_plate", "thigh_strut"),
                                       (bl.part_shin(0), "shin_plate", "shin_strut")):
            p, st = part.component_bounds(plate_c), part.component_bounds(strut_c)
            self.assertLess(p[1][2], st[1][2])                       # thinner than the strut it dresses
            self.assertGreater(p[1][1], st[1][1])                    # and proud of it on both flanks
            self.assertEqual(sum(1 for q in part.polygons if q.component == plate_c) // 6, 2)

    def test_recoil_strut_is_a_slim_separate_member(self):
        # item 6: a thin charcoal strut from the lower rear of the body down and back to the trailing
        # ankle, with a slide section at its middle; it must not read as a leg plate
        m = self.m
        self.assertTrue(m["strut_is_slim"])
        self.assertLess(2.0 * bl.STRUT_R, 0.5 * bl.LEG_W)
        self.assertLess(bl.STRUT_END[0], bl.STRUT_TOP[0])            # runs backward
        self.assertLess(bl.STRUT_END[2], bl.STRUT_TOP[2])            # and downward
        self.assertLess(m["strut_reaches_trailing_ankle_cm"], 20.0)  # lands at the trailing ankle
        self.assertTrue(0.35 <= bl.STRUT_SPLIT <= 0.65)              # the slide joint is at its middle
        self.assertLess(bl.STRUT_TOP[2], bl.HIP[2] + 1e-6)           # anchored at the lower rear
        # clear of the trailing shin in the side view at every height it shares with it
        p = bl.rest_positions()
        knee, ankle = p["l_knee"], p["l_ankle"]
        for f in (0.2, 0.4, 0.6, 0.8):
            z = knee[2] + (ankle[2] - knee[2]) * f
            shin_x = knee[0] + (ankle[0] - knee[0]) * f
            t = (bl.STRUT_TOP[2] - z) / (bl.STRUT_TOP[2] - bl.STRUT_END[2])
            strut_x = bl.STRUT_TOP[0] + (bl.STRUT_END[0] - bl.STRUT_TOP[0]) * t
            self.assertLess(strut_x, shin_x - 2.0, f"strut overlaps the shin at z {z:.1f}")

    # --- rig, sockets and sub-objects ----------------------------------------------------------
    def test_eighteen_bone_rig_and_hierarchy(self):
        names = [b.name for b in self.skel.bones]
        self.assertEqual(len(names), 18)
        self.assertEqual(names[0], "root")
        self.assertEqual(self.skel.get("spine").parent, "body")
        self.assertEqual(self.skel.get("cowl").parent, "spine")
        self.assertEqual(self.skel.get("lance_yaw").parent, "body")
        self.assertEqual(self.skel.get("lance_barrel").parent, "lance_yaw")
        self.assertEqual(self.skel.get("strut_slide").parent, "strut_upper")
        for side in ("r", "l"):
            self.assertEqual(self.skel.get(f"{side}_pod").parent, "spine")
            self.assertEqual(self.skel.get(f"{side}_thigh").parent, "body")
            self.assertEqual(self.skel.get(f"{side}_shin").parent, f"{side}_thigh")
            self.assertEqual(self.skel.get(f"{side}_foot").parent, f"{side}_shin")
            self.assertEqual(self.skel.get(f"{side}_toe").parent, f"{side}_foot")
        self.assertGreater(self.skel.get("r_thigh").head[1], 0.0)     # anatomical right is +Y
        self.assertLess(self.skel.get("l_thigh").head[1], 0.0)
        self.assertGreater(self.skel.get("r_foot").head[0], self.skel.get("l_foot").head[0])  # right leads

    def test_card_sub_objects(self):
        # REL-ART-005.MC.LANCER: sub-object separation required for Turret_Y and Barrel_X
        subs = self.inventory["sub_objects"]["built"]
        self.assertEqual(subs["Turret_Y"], "lance_yaw")
        self.assertEqual(subs["Barrel_X"], "lance_barrel")
        for lod in (0, 1):
            yoke, barrel = bl.part_lance_yoke(lod), bl.part_lance_barrel(lod)
            self.assertGreater(yoke.triangle_count(), 0)
            self.assertGreater(barrel.triangle_count(), 0)
            self.assertIn("Muzzle_Flash_01", [s.name for s in barrel.sockets])
            # each part is authored around its own pivot (the yaw axis / the trunnion)
            self.assertLessEqual(abs(yoke.component_bounds("yaw_ring")[0][0] + yoke.component_bounds("yaw_ring")[1][0]) / 2.0, 1e-6)

    def test_sockets_named_and_placed(self):
        names = sorted(s.name for s in self.mesh0.sockets)
        self.assertEqual(names, ["Left_Tread_Vector", "Muzzle_Flash_01", "Target_Anchor_Center"])
        self.assertEqual(self.sockets_on_bones["Muzzle_Flash_01"], "lance_barrel")
        self.assertEqual(self.sockets_on_bones["Target_Anchor_Center"], "body")
        self.assertEqual(self.sockets_on_bones["Left_Tread_Vector"], "l_foot")
        by_name = {s.name: s for s in self.mesh0.sockets}
        muzzle = by_name["Muzzle_Flash_01"].position
        self.assertAlmostEqual(muzzle[0], bl.LANCE_TIP_X, places=3)
        self.assertAlmostEqual(muzzle[1], 0.0, places=6)
        self.assertAlmostEqual(muzzle[2], bl.LANCE_Z, places=3)
        centre = by_name["Target_Anchor_Center"].position
        self.assertTrue(0.55 * H <= centre[2] <= 0.80 * H, centre)
        self.assertAlmostEqual(centre[1], 0.0, places=6)
        tread = by_name["Left_Tread_Vector"].position
        self.assertAlmostEqual(tread[2], 0.0, places=6)               # ground contact
        self.assertLess(tread[1], 0.0)                                 # LEFT foot (-Y), the trailing leg
        sole = self.mesh0.component_bounds("l_foot_foot_sole")
        self.assertTrue(sole[0][0] - 1e-6 <= tread[0] <= self.mesh0.component_bounds("l_toe_toe_block")[1][0] + 1e-6)
        self.assertIn("naming conflict", by_name["Left_Tread_Vector"].purpose)

    def test_every_polygon_bound_and_no_root_geometry(self):
        self.assertTrue(all(getattr(p, "bone", None) for p in self.mesh0.polygons))
        self.assertNotIn("root", self.counts)
        self.assertEqual(set(self.counts) | {"root"}, {b.name for b in self.skel.bones})

    # --- budgets, slots, determinism -----------------------------------------------------------
    def test_budgets_and_slots(self):
        self.assertLessEqual(self.mesh0.triangle_count(), bl.LOD0_CAP)
        self.assertLessEqual(self.mesh1.triangle_count(), bl.LOD1_CAP)
        self.assertLess(self.mesh1.triangle_count(), self.mesh0.triangle_count())
        self.assertLessEqual(self.m["emissive_area_fraction"], bl.EMISSIVE_CAP)
        ex = bl.export_mesh(self.mesh0)
        self.assertEqual(ex.slots, [bl.FRAME, bl.CERAMIC])
        self.assertEqual(ex.triangle_count(), self.mesh0.triangle_count())
        self.assertTrue(all(getattr(p, "bone", None) for p in ex.polygons))

    def test_deterministic_exports(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = bl.export_mesh(bl.assemble(0)[0]).write_glb(os.path.join(tmp, "a.glb"), include_collision=False)
            b = bl.export_mesh(bl.assemble(0)[0]).write_glb(os.path.join(tmp, "b.glb"), include_collision=False)
            self.assertEqual(a, b)
            sk = bl.build_skeleton()
            c = skel.write_skinned_glb(bl.export_mesh(bl.assemble(0)[0]), sk, os.path.join(tmp, "c.glb"),
                                       animations=bl.build_clips(sk), include_collision=False, sockets_on_bones=bl.SOCKETS_ON_BONES)
            d = skel.write_skinned_glb(bl.export_mesh(bl.assemble(0)[0]), sk, os.path.join(tmp, "d.glb"),
                                       animations=bl.build_clips(sk), include_collision=False, sockets_on_bones=bl.SOCKETS_ON_BONES)
            self.assertEqual(c, d)

    # --- clips ---------------------------------------------------------------------------------
    def test_clip_inventory_matches_contract(self):
        names = [c.name for c in self.clips]
        for required in ("idle", "move", "turn", "stop", "fire", "damage", "death", "cancel", "restore"):
            self.assertIn(required, names)
        for clip in self.clips:
            self.assertNotIn("root", clip.tracks, clip.name)         # no root motion in any clip
            for bone, keys in clip.tracks.items():
                times = [k.time_s for k in keys]
                self.assertEqual(times, sorted(times), clip.name)
                self.assertLessEqual(times[-1], clip.duration_s + 1e-9, clip.name)
                self.assertIn(bone, [b.name for b in self.skel.bones])
        move = next(c for c in self.clips if c.name == "move")
        self.assertTrue(move.loop)
        for side in ("r", "l"):
            self.assertIn(f"{side}_toe", move.tracks)
        restore = next(c for c in self.clips if c.name == "restore")
        self.assertEqual(restore.duration_s, 0.0)
        self.assertEqual(sorted(restore.tracks), sorted(b.name for b in self.skel.bones if b.name != "root"))
        for keys in restore.tracks.values():
            self.assertEqual([tuple(k.rotation_deg) for k in keys], [(0.0, 0.0, 0.0)])

    def test_fire_clip_states(self):
        # canon SPEC-UNIT-002: halt, plant the strut, aim, fire with recoil through the mount, recover
        fire = next(c for c in self.clips if c.name == "fire")
        self.assertIn("lance_barrel", fire.tracks)
        self.assertIn("strut_slide", fire.tracks)
        recoil = min(k.translation_cm[0] for k in fire.tracks["lance_barrel"])
        self.assertLessEqual(recoil, -0.9 * bl.RECOIL_CM)           # the lance recoils along its own -X axis
        self.assertEqual(max(k.translation_cm[0] for k in fire.tracks["lance_barrel"]), 0.0)
        for k in fire.tracks["lance_barrel"]:                        # recoil stays on the lance axis
            self.assertAlmostEqual(k.translation_cm[1], 0.0, places=6)
            self.assertAlmostEqual(k.translation_cm[2], 0.0, places=6)
        slide = [k.translation_cm for k in fire.tracks["strut_slide"]]
        along = [kit.v_dot(t, bl.STRUT_DIR) for t in slide]
        self.assertGreater(max(along) - min(along), 6.0)             # the slide section really moves
        # and it works through the braced section too, not only on the halt-to-plant stow release
        braced = [kit.v_dot(k.translation_cm, bl.STRUT_DIR) for k in fire.tracks["strut_slide"] if k.time_s >= 0.35]
        self.assertGreater(max(braced) - min(braced), 6.0, braced)
        for t, a in zip(slide, along):                               # and only along the strut axis
            self.assertAlmostEqual(kit.v_len(kit.v_sub(t, kit.v_mul(bl.STRUT_DIR, a))), 0.0, places=6)
        aim = pr.sample(fire, 0.85)
        posed = skel.pose_mesh(self.mesh0, self.skel, dict(aim, _sockets=self.sockets_on_bones))
        muzzle = next(s for s in posed.sockets if s.name == "Muzzle_Flash_01").position
        self.assertAlmostEqual(muzzle[1], 0.0, places=2)             # aim brings the lance onto the centreline
        breech = posed.component_bounds("lance_lance_breech")
        self.assertLess(abs((breech[0][2] + breech[1][2]) / 2.0 - muzzle[2]), 6.0)   # lance held level at the aim
        shot = pr.sample(fire, 1.0)
        posed_shot = skel.pose_mesh(self.mesh0, self.skel, dict(shot, _sockets=self.sockets_on_bones))
        muzzle_shot = next(s for s in posed_shot.sockets if s.name == "Muzzle_Flash_01").position
        self.assertLess(muzzle_shot[0], muzzle[0] - 20.0)            # the muzzle is pulled back on the shot

    def test_move_leaves_the_brace_and_never_carries_the_lance_off_axis(self):
        move = next(c for c in self.clips if c.name == "move")
        mid = pr.sample(move, 0.15)
        posed = skel.pose_mesh(self.mesh0, self.skel, dict(mid, _sockets=self.sockets_on_bones))
        r_sole = posed.component_bounds("r_foot_foot_sole")
        l_sole = posed.component_bounds("l_foot_foot_sole")
        self.assertLess(abs(r_sole[0][0] - l_sole[0][0]), abs(self.mesh0.component_bounds("r_foot_foot_sole")[0][0]
                                                              - self.mesh0.component_bounds("l_foot_foot_sole")[0][0]))
        for k in move.tracks["lance_barrel"]:
            self.assertEqual(tuple(k.translation_cm), (0.0, 0.0, 0.0))   # never fires (no recoil) while moving

    def test_death_drops_the_lance_toward_the_ground_without_breaking_it(self):
        death = next(c for c in self.clips if c.name == "death")
        posed = skel.pose_mesh(self.mesh0, self.skel, dict(pr.sample(death, 1.4), _sockets=self.sockets_on_bones))
        muzzle = next(s for s in posed.sockets if s.name == "Muzzle_Flash_01").position
        self.assertTrue(0.0 <= muzzle[2] <= 40.0, muzzle)            # nose-down, at or just above the ground
        self.assertLess(posed.bounds()[1][2], H)                     # the frame has collapsed below its standing height

    def test_clips_never_break_the_ground_plane(self):
        for clip in self.clips:
            steps = max(1, int(round(clip.duration_s / 0.01)))    # 10 ms: 20 ms sampling steps over the minima
            for i in range(steps + 1):
                t = clip.duration_s * i / max(1, steps)
                posed = skel.pose_mesh(self.mesh0, self.skel, pr.sample(clip, t))
                (_, _, z0), _ = posed.bounds()
                self.assertGreaterEqual(z0, -1.0, f"{clip.name} at {t:.2f}s: lowest z {z0:.2f}")

    def _component_centre(self, mesh, comp):
        lo, hi = mesh.component_bounds(comp)
        return tuple((lo[i] + hi[i]) / 2.0 for i in range(3))

    def test_strut_stays_bolted_to_the_trailing_ankle_or_is_drawn_home(self):
        """concept-v2 regression. concept-v1 keyed the strut in place while the trailing leg swung away,
        so in `move` (the most-seen state), `stop`, `turn` and the fire halt the strut's lower end hung in
        empty air — worst case 132.9 cm from the ankle it is meant to brace, 47 cm above the ground with
        nothing under it. The strut is now solved from the trailing leg: it is either bolted to the ankle
        or drawn home into its gland, never left reaching for a leg that is not there."""
        for clip in self.clips:
            steps = max(1, int(round(clip.duration_s / 0.01)))
            for i in range(steps + 1):
                t = clip.duration_s * i / max(1, steps)
                posed = skel.pose_mesh(self.mesh0, self.skel, pr.sample(clip, t))
                foot = self._component_centre(posed, "strut_slide_strut_foot_joint")
                ankle = self._component_centre(posed, "l_foot_ankle_hub")
                anchor = self._component_centre(posed, "strut_upper_strut_anchor")
                planted = math.dist(foot, ankle)
                free = math.dist(foot, anchor)
                self.assertLessEqual(min(planted, free), 58.0,
                                     f"{clip.name} at {t:.2f}s: strut end {planted:.1f} cm from the ankle "
                                     f"and {free:.1f} cm from its own anchor - dangling")
        # every braced state keeps it actually bolted on
        for name in ("idle", "turn", "damage", "cancel", "restore", "death"):
            clip = next(c for c in self.clips if c.name == name)
            steps = max(1, int(round(clip.duration_s / 0.02)))
            for i in range(steps + 1):
                t = clip.duration_s * i / max(1, steps)
                posed = skel.pose_mesh(self.mesh0, self.skel, pr.sample(clip, t))
                d = math.dist(self._component_centre(posed, "strut_slide_strut_foot_joint"),
                              self._component_centre(posed, "l_foot_ankle_hub"))
                self.assertLessEqual(d, 15.0, f"{name} at {t:.2f}s: strut end {d:.1f} cm from the ankle")
        # and the fire clip is bolted on from the plant through the recover (the canon "plants the strut")
        fire = next(c for c in self.clips if c.name == "fire")
        for t in (0.35, 0.55, 0.75, 0.95, 1.00, 1.15, 1.45, 1.80, 2.20):
            posed = skel.pose_mesh(self.mesh0, self.skel, pr.sample(fire, t))
            d = math.dist(self._component_centre(posed, "strut_slide_strut_foot_joint"),
                          self._component_centre(posed, "l_foot_ankle_hub"))
            self.assertLessEqual(d, 15.0, f"fire at {t:.2f}s: strut end {d:.1f} cm from the ankle")
        # the travel stow is a retraction, not a rotation into the leg: the rod keeps its rest angle
        move = next(c for c in self.clips if c.name == "move")
        for key in move.tracks["strut_upper"]:
            self.assertAlmostEqual(kit.v_len(key.rotation_deg), 0.0, places=6)
        for key in move.tracks["strut_slide"]:
            self.assertAlmostEqual(kit.v_dot(key.translation_cm, bl.STRUT_DIR), -bl.STOW_RETRACT, places=6)

    def test_strut_solver_is_exact_at_the_rest_pose(self):
        """The aim solve must be the identity where the strut already points along its rest axis, or the
        rest stance and the `restore` key would disagree with every clip's first frame."""
        pitch, yaw = bl._aim_rotator(bl.STRUT_DIR, bl.STRUT_DIR)
        self.assertAlmostEqual(pitch, 0.0, places=9)
        self.assertAlmostEqual(yaw, 0.0, places=9)
        keys = bl.strut_keys(bl.make_pose(), 1.0)
        self.assertAlmostEqual(kit.v_len(keys["strut_upper"]), 0.0, places=6)
        self.assertAlmostEqual(kit.v_len(keys["strut_slide"][3:]), 0.0, places=6)
        # and it really carries the rest direction onto an arbitrary target direction
        for target in ((-0.6, -0.1, -0.79), (-0.2, -0.3, -0.93), (-0.9, 0.05, -0.43)):
            unit = kit.v_norm(target)
            p, y = bl._aim_rotator(bl.STRUT_DIR, unit)
            got = skel.rot_rotator(bl.STRUT_DIR, p, y, 0.0)
            for a, b in zip(got, unit):
                self.assertAlmostEqual(a, b, places=9)

    def test_walk_speed_is_recorded_against_the_draw_scale(self):
        """The stride is matched to the authoritative 320 cm/s at AUTHORED scale; the unit draws at x1.60,
        so the integration task must set the play rate rather than discover a 60% foot slide."""
        m = self.m
        self.assertAlmostEqual(m["move_authored_speed_cm_s"], 320.0, places=6)   # units.json mc_lancer move_speed_cm_s
        self.assertAlmostEqual(m["move_world_speed_at_draw_scale_cm_s"], 320.0 * bl.PRESENTATION_SCALE, places=6)
        self.assertAlmostEqual(m["integration_play_rate"], 1.0 / bl.PRESENTATION_SCALE, places=6)

    def test_hold_poses_keep_the_planted_soles_on_the_ground(self):
        for clip in self.clips:
            if clip.name not in ("idle", "damage", "cancel", "restore"):   # move/turn/stop/fire/death step or fold
                continue
            steps = max(1, int(round(clip.duration_s / 0.05)))
            for i in range(steps + 1):
                t = clip.duration_s * i / max(1, steps)
                posed = skel.pose_mesh(self.mesh0, self.skel, pr.sample(clip, t))
                for comp in ("r_foot_foot_sole", "l_foot_foot_sole"):
                    (_, _, z0), _ = posed.component_bounds(comp)
                    self.assertLessEqual(abs(z0), 1.5, f"{clip.name} at {t:.2f}s: {comp} sole at {z0:.2f}")
        # the fire clip holds the brace from the plant to the recover: both soles planted there
        fire = next(c for c in self.clips if c.name == "fire")
        for t in (0.35, 0.55, 0.75, 0.95, 1.45, 1.80, 2.20):
            posed = skel.pose_mesh(self.mesh0, self.skel, pr.sample(fire, t))
            for comp in ("r_foot_foot_sole", "l_foot_foot_sole"):
                (_, _, z0), _ = posed.component_bounds(comp)
                self.assertLessEqual(abs(z0), 1.5, f"fire at {t:.2f}s: {comp} sole at {z0:.2f}")

    def test_rest_stance_is_grounded_and_forward_facing(self):
        (x0, y0, z0), (x1, y1, z1) = self.mesh0.bounds()
        self.assertAlmostEqual(z0, 0.0, places=3)
        self.assertGreater(x1, 0.9 * H)                              # the lance reaches far forward
        self.assertLess(abs(y1 + y0), 2.0)                           # symmetric across the centreline
        self.assertGreater(x1 - x0, z1)                              # longer than tall: a line-fire frame
        self.assertLess(y1 - y0, 0.6 * H)                            # narrow across
        for side in ("r", "l"):
            self.assertAlmostEqual(self.mesh0.component_bounds(f"{side}_foot_foot_sole")[0][2], 0.0, places=3)

    def test_pose_review_samples_cover_every_clip(self):
        sampled = {name for name, _t, _label, _tac in pr.SAMPLES}
        self.assertEqual(sampled, {c.name for c in self.clips} - {"restore"})


if __name__ == "__main__":
    unittest.main(verbosity=1)
