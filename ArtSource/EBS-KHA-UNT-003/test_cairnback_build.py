#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-UNT-003 Cairnback concept blockout.

Author: Angelis Pseftis. Run: python3 test_cairnback_build.py
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
import build_cairnback as cb  # noqa: E402


class CairnbackBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = cb.assemble(0, "intact")
        cls.m1 = cb.assemble(1, "intact")[0]
        cls.chipped = cb.assemble(0, "chipped")[0]
        cls.clips = cb.build_clips(cls.s)
        cls.inv = cb.contract_inventory(cls.m0, cls.s, cls.clips)
        cls.mm = cb.measurements(cls.m0)

    # --- silhouette first: the owner's 2026-09-07 directive ---

    def test_silhouette_contrasts_with_the_riftstalker(self):
        self.assertLessEqual(self.mm["leg_share_of_height"], 0.24,
                             "short legs: the shell, not the limbs, carries the height")
        self.assertGreaterEqual(self.mm["width_over_length"], 0.70, "broad, not lean")
        separation = self.mm["contrast_with_riftstalker"]
        self.assertGreater(separation["leg_share"]["separation"], 0.40)
        self.assertGreater(separation["width_over_length"]["separation"], 0.40)

    def test_the_head_is_tucked_behind_the_shell(self):
        # the Riftstalker's prow LEADS its body; this one hides under its own roof
        self.assertTrue(self.mm["head_is_tucked_behind_the_shell"])
        self.assertLess(self.inv["low_protected_head"]["head_front_x"],
                        self.inv["low_protected_head"]["shell_front_x"])

    def test_the_gait_differs_from_the_riftstalkers(self):
        # the owner asked for a different stance AND gait; the phase pattern is lateral, not diagonal
        move = {c.name: c for c in self.clips}["move"]
        # compare the limbs by the PHASE of their swing, not by key times: every limb has a key at
        # t = 0 because the cycle wraps, which made an earlier version of this test compare nothing
        def swing_at(tag, fraction):
            pose = cb.sample_pose(move, fraction)
            return round(pose.get(f"{tag}_upper", (0.0,) * 6)[0], 4)

        samples = {tag: tuple(swing_at(tag, f / 8.0) for f in range(8)) for tag in cb.LEG_TAGS}
        self.assertNotEqual(samples["fl"], samples["rr"], "not a diagonal pair gait")
        self.assertNotEqual(samples["fr"], samples["rl"], "nor the other diagonal pair")
        self.assertEqual(len(set(samples.values())), 4,
                         "four limbs on four distinct phases: a lateral sequence")

    # --- shape and contract ---

    def test_contract_inventory(self):
        self.assertEqual(self.inv["domed_shell"]["courses"], cb.SHELL_COURSES)
        self.assertEqual(self.inv["short_thick_limbs"]["legs"], 4)
        self.assertFalse(self.inv["mineral_cover"]["built_here"])
        self.assertEqual(self.inv["mineral_cover"]["cast_origin_socket"], "Cover_Cast_Origin")

    def test_the_shell_steps_inward_as_it_rises(self):
        widths = [cb.shell_course(i)[4] for i in range(cb.SHELL_COURSES)]
        for a, b in zip(widths, widths[1:]):
            self.assertLess(b, a)

    def test_traced_proportion(self):
        self.assertAlmostEqual(self.mm["length_over_height"], 1.45, delta=0.05)

    def test_material_slots_and_amber_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [cb.STRATA, cb.AMBER])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == cb.AMBER}
        self.assertTrue(all(c.startswith("seam_") for c in lit), lit)
        self.assertLessEqual(cb.slot_area_fraction(self.m0, cb.AMBER), 0.15)

    def test_the_mineral_cover_is_not_part_of_this_asset(self):
        for mesh in (self.m0, self.m1, self.chipped):
            self.assertFalse([c for c in mesh.components() if "barrier" in c or "cover" in c])
        self.assertIn("Cover_Cast_Origin", [s.name for s in self.m0.sockets])

    def test_chipped_state_breaks_strata(self):
        self.assertTrue([c for c in self.chipped.components() if c.startswith("chip_")])
        self.assertFalse([c for c in self.m0.components() if c.startswith("chip_")])

    # --- rig and motion ---

    def test_the_rig_is_the_cards_heavy_plan(self):
        names = [b.name for b in self.s.bones]
        self.assertEqual(names[:6], ["root", "body", "slab_lower", "slab_upper", "neck", "head"])
        self.assertEqual(len(names), 22)
        limb_bones = [n for n in names if n.split("_")[0] in cb.LEG_TAGS]
        self.assertEqual(len(limb_bones), 16, "weighted toward the limbs and the back")

    def test_the_attack_does_not_pitch_the_body(self):
        # pitching this animal drives its flat foot plates into the ground; the neck carries the strike
        attack = {c.name: c for c in self.clips}["attack"]
        for k in attack.tracks["body"]:
            self.assertEqual(k.rotation_deg, (0.0, 0.0, 0.0))
        self.assertTrue(any(k.rotation_deg[0] != 0.0 for k in attack.tracks["neck"]))

    def test_the_heave_moves_the_back_slab(self):
        heave = {c.name: c for c in self.clips}["heave"]
        self.assertIn("slab_lower", heave.tracks)
        self.assertIn("slab_upper", heave.tracks)
        lower = [k.rotation_deg[0] for k in heave.tracks["slab_lower"]]
        self.assertLess(min(lower), 0.0)
        self.assertGreater(max(lower), 0.0)

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(33):
                mesh = cb.posed(0, cb.sample_pose(clip, i / 32.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -0.01, f"{clip.name} at {i / 32.0:.3f}")

    def test_foot_planting_is_measured_not_guessed(self):
        self.assertTrue(cb.FOOT_LIFT, "every clip records the body lift it needed")
        for name, lift in cb.FOOT_LIFT.items():
            self.assertGreaterEqual(lift, 0.0, name)
            self.assertLess(lift, 12.0, f"{name}: a small measured correction, not a hidden float")

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips),
                         ["attack", "damage_chip", "death", "heave", "idle", "move"])

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(cb.SOCKETS))
        self.assertEqual(self.socks["Back_Slab_Center"], "slab_upper")
        self.assertEqual(self.socks["Cover_Cast_Origin"], "head")

    def test_provisional_card_budget_and_headroom_note(self):
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-FAC-025.KA.CAIRNBACK.ASSET", data["provisional_contract"]["card"])
            self.assertIn("HEADROOM, not sufficiency", data["budgets"]["headroom_note"])
            self.assertIn("SEPARATE asset", data["excluded_from_this_asset"]["mineral_cover"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        worst = max(lod0 or self.m0.triangle_count(), self.chipped.triangle_count())
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(worst, 8000)
        self.assertLessEqual(lod1, 3500)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_keeps_the_shell_and_four_legs(self):
        inv = cb.contract_inventory(self.m1, self.s, self.clips)
        self.assertEqual(inv["domed_shell"]["courses"], cb.SHELL_COURSES)
        self.assertEqual(inv["short_thick_limbs"]["legs"], 4)

    def test_states_are_declared(self):
        self.assertEqual(tuple(cb.STATES), ("intact", "chipped"))
        with self.assertRaises(ValueError):
            cb.assemble(0, "destroyed")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = cb.assemble(0, "intact")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = cb.assemble(0, "intact")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(cb.REVISION, f"{cb.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
