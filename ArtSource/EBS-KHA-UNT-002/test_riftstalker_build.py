#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-UNT-002 Riftstalker concept blockout.

Author: Angelis Pseftis. Run: python3 test_riftstalker_build.py
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
import build_riftstalker as rs  # noqa: E402

GROUND_TOLERANCE_CM = 0.05   # see README section 8: the deepest sampled frame sits 0.18 mm under


class RiftstalkerBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = rs.assemble(0, "baseline")
        cls.m1 = rs.assemble(1, "baseline")[0]
        cls.carapace = rs.assemble(0, "carapace_molt")[0]
        cls.striker = rs.assemble(0, "striker_molt")[0]
        cls.clips = rs.build_clips(cls.s)
        cls.inv = rs.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_it_is_a_quadruped_with_no_neck(self):
        # the candidate's SKIRMISHER and MOVING FIRE views both show four legs and no neck; my card's
        # first draft said biped, and the CARD was corrected
        self.assertEqual(self.inv["quadruped"]["legs"], 4)
        self.assertEqual(self.inv["prow"]["neck_bones"], [])
        self.assertTrue(self.inv["prow"]["built"])

    def test_every_leg_has_three_driven_segments(self):
        names = {b.name for b in self.s.bones}
        for tag in rs.LEG_TAGS:
            for part in ("hip", "upper", "lower", "foot"):
                self.assertIn(f"{tag}_{part}", names)

    def test_bone_heads_are_at_the_proximal_end_of_each_segment(self):
        # a first pass put them at the distal ends, so rotating a thigh pivoted it about the knee and
        # tore the leg off the body in the death pose
        heads = {b.name: b.head for b in self.s.bones}
        for tag in rs.LEG_TAGS:
            self.assertEqual(heads[f"{tag}_upper"], heads[f"{tag}_hip"], f"{tag}: thigh pivots at the hip")
            self.assertAlmostEqual(heads[f"{tag}_lower"][2], rs.KNEE_Z, places=6, msg=f"{tag}: shin pivots at the knee")
            self.assertLess(heads[f"{tag}_foot"][2], heads[f"{tag}_lower"][2], f"{tag}: foot pivots at the ankle")

    def test_traced_proportion(self):
        m = rs.measurements(self.m0)
        self.assertAlmostEqual(m["body_length_over_height"], 1.46, delta=0.03)
        self.assertTrue(m["carapace_slopes_forward"], "the low forward posture is in the rest geometry")
        self.assertTrue(m["prow_ahead_of_front_feet"])

    def test_material_slots_and_amber_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [rs.STRATA, rs.AMBER])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == rs.AMBER}
        self.assertTrue(all(c.startswith("seam_") or c in ("prow_seam", "caster_slot") for c in lit), lit)
        self.assertLessEqual(rs.slot_area_fraction(self.m0, rs.AMBER), 0.15)  # REL-ART-029 ceiling

    def test_the_caster_is_a_shoulder_slot_aimed_past_the_prow(self):
        muzzle = {s.name: s for s in self.m0.sockets}["Shard_Caster_Muzzle"]
        self.assertEqual(self.socks["Shard_Caster_Muzzle"], "caster_pitch")
        self.assertGreater(muzzle.position[2], rs.H - 60.0, "the caster sits high on the shoulder")
        prow_tip_z = self.m0.component_bounds("prow")[0][2]
        self.assertGreater(muzzle.position[2], prow_tip_z, "its line clears the prow")

    def test_it_fires_while_moving_and_the_two_do_not_fight(self):
        by_name = {c.name: c for c in self.clips}
        move, fire = by_name["move"], by_name["fire_on_the_move"]
        for tag in rs.LEG_TAGS:
            self.assertEqual([k.rotation_deg for k in move.tracks[f"{tag}_upper"]],
                             [k.rotation_deg for k in fire.tracks[f"{tag}_upper"]],
                             "the gait is identical whether or not it is firing")
        self.assertIn("caster_pitch", fire.tracks)
        self.assertNotIn("caster_pitch", move.tracks)

    def test_the_stride_is_pitch_not_yaw(self):
        # ebs_skelkit takes (pitch, yaw, roll); writing a stride into the yaw slot swung the legs
        # sideways instead of stepping, which is how the first pass was wrong
        move = {c.name: c for c in self.clips}["move"]
        for tag in rs.LEG_TAGS:
            for k in move.tracks[f"{tag}_upper"]:
                self.assertEqual(k.rotation_deg[1], 0.0, "no yaw in a stride")
                self.assertEqual(k.rotation_deg[2], 0.0, "no roll in a stride")
            self.assertTrue(any(k.rotation_deg[0] != 0.0 for k in move.tracks[f"{tag}_upper"]))

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(49):
                mesh = rs.posed(0, rs.sample_pose(clip, i / 48.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -GROUND_TOLERANCE_CM,
                                        f"{clip.name} at {i / 48.0:.3f}")

    def test_the_descent_solver_measures_the_real_rig(self):
        # _safe_drop poses the actual mesh; two earlier analytic attempts got the pitch sign wrong and
        # bisected a non-monotonic function, and both produced folds that drove the feet underground
        shallow = rs._safe_drop(0.0, 0.0)
        deep = rs._safe_drop(20.0, 90.0)
        self.assertGreater(deep, shallow, "a folded leg supports a deeper settle than a straight one")
        self.assertGreater(deep, 30.0)

    def test_molt_states_are_visible_on_the_unit(self):
        base = set(self.m0.components())
        self.assertTrue(set(self.carapace.components()) - base, "a carapace molt adds visible plate")
        self.assertTrue(set(self.striker.components()) - base, "a striker molt adds visible vanes")
        self.assertGreater(self.carapace.triangle_count(), self.m0.triangle_count())

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips),
                         ["death", "fire_on_the_move", "idle", "molt", "move", "sidestep"])

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(rs.SOCKETS))
        self.assertEqual(self.socks["Molt_Striker_Anchor"], "caster_pitch")
        self.assertEqual(self.socks["Molt_Carapace_Anchor"], "body")

    def test_provisional_card_budget(self):
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-FAC-025.KA.RIFTSTALKER.ASSET", data["provisional_contract"]["card"])
            self.assertIn("PENDING", data["acceptance"]["technical"])
            self.assertIn("FOUR legs", data["anatomy_correction"]["why_wrong"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        worst = max(lod0 or self.m0.triangle_count(), self.carapace.triangle_count(),
                    self.striker.triangle_count())
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(worst, 6000)
        self.assertLessEqual(lod1, 2600)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_keeps_four_legs_and_the_caster(self):
        inv = rs.contract_inventory(self.m1, self.s, self.clips)
        self.assertEqual(inv["quadruped"]["legs"], 4)
        self.assertTrue(inv["shard_caster"]["housing"])

    def test_states_are_declared(self):
        self.assertEqual(tuple(rs.STATES), ("baseline", "carapace_molt", "striker_molt"))
        with self.assertRaises(ValueError):
            rs.assemble(0, "dead")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = rs.assemble(0, "baseline")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = rs.assemble(0, "baseline")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(rs.REVISION, f"{rs.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
