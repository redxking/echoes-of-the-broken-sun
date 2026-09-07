#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-UNT-004 Resonant concept blockout.

Author: Angelis Pseftis. Run: python3 test_resonant_build.py
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
import build_resonant as rn  # noqa: E402


class ResonantBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = rn.assemble(0, "passive")
        cls.m1 = rn.assemble(1, "passive")[0]
        cls.detecting = rn.assemble(0, "detecting")[0]
        cls.clips = rn.build_clips(cls.s)
        cls.inv = rn.contract_inventory(cls.m0, cls.s, cls.clips)
        cls.mm = rn.measurements(cls.m0)

    def test_it_is_a_quadruped_not_the_biped_my_card_described(self):
        self.assertEqual(self.inv["quadruped_on_stilts"]["legs"], 4)
        names = {b.name for b in self.s.bones}
        for tag in rn.LEG_TAGS:
            self.assertIn(f"{tag}_upper", names)

    def test_it_is_taller_than_it_is_long(self):
        # its separation from BOTH other Kharuun quadrupeds, neither of which is
        self.assertTrue(self.mm["taller_than_it_is_long"])
        self.assertGreaterEqual(self.mm["height_over_extent"], 1.45)
        for name, data in self.mm["separation_from_the_other_quadrupeds"].items():
            self.assertGreater(data["separation"], 0.6, name)

    def test_the_fin_array_runs_the_length_of_the_back(self):
        # a first pass packed thirteen fins into 40 cm and they merged into one lit crest
        xs = [rn.fin_at(n)[0] for n in range(rn.FIN_COUNT)]
        self.assertGreater(max(xs) - min(xs), rn.BODY_LEN * 0.6)
        self.assertEqual(self.inv["fin_array"]["built"], rn.FIN_COUNT)
        self.assertEqual(self.inv["fin_array"]["veins"], rn.FIN_COUNT)

    def test_the_fins_are_individually_addressable_and_ordered(self):
        centres = []
        for n in range(1, rn.FIN_COUNT + 1):
            b = self.m0.component_bounds(f"fin_{n:02d}")
            centres.append((b[0][0] + b[1][0]) / 2.0)
        for a, b in zip(centres, centres[1:]):
            self.assertLess(a, b, "fins are ordered along the spine so a sequence can run through them")
        self.assertEqual(rn.lit_fins(self.m0), [])
        self.assertEqual(rn.lit_fins(self.detecting), list(range(1, rn.FIN_COUNT + 1)))

    def test_the_emissive_is_the_vein_not_the_whole_fin(self):
        # thirteen fully amber blades measured 32% of surface area against REL-ART-029's 15% ceiling
        lit = {p.component for p in self.detecting.polygons if self.detecting.slots[p.slot] == rn.AMBER}
        self.assertTrue(all(c.endswith("_vein") for c in lit), lit)
        self.assertLessEqual(rn.slot_area_fraction(self.detecting, rn.AMBER), 0.15)
        self.assertEqual(rn.slot_area_fraction(self.m0, rn.AMBER), 0.0, "passive: nothing lit")

    def test_the_spine_arches(self):
        self.assertTrue(self.inv["arched_spine"]["rises"])
        self.assertGreater(rn.spine_point(1.0)[1], rn.spine_point(0.0)[1])

    def test_the_frame_is_delicate(self):
        self.assertLessEqual(rn.LIMB_R, 9.0)
        self.assertLessEqual(rn.BODY_W, 60.0)

    def test_material_slots(self):
        self.assertEqual(self.m0.slots, [rn.STRATA, rn.AMBER])

    def test_the_rig_is_weighted_toward_the_spine(self):
        names = [b.name for b in self.s.bones]
        self.assertEqual(len(names), 24)
        spine = [n for n in names if n.startswith("spine_") or n in ("neck", "head")]
        self.assertGreaterEqual(len(spine), rn.SPINE_SEGMENTS + 2)

    def test_the_detect_clip_lowers_and_sweeps_the_spine(self):
        detect = {c.name: c for c in self.clips}["detect"]
        self.assertIn("neck", detect.tracks)
        pitches = [k.rotation_deg[0] for k in detect.tracks["neck"]]
        yaws = [k.rotation_deg[1] for k in detect.tracks["neck"]]
        self.assertLess(min(pitches), -5.0, "the spine lowers")
        self.assertGreater(max(yaws) - min(yaws), 5.0, "and sweeps")

    def test_the_gait_phases_are_real(self):
        # the defect that shipped in the Riftstalker: key time and swing phase from the same value
        move = {c.name: c for c in self.clips}["move"]

        def swing(tag, fraction):
            return round(rn.sample_pose(move, fraction).get(f"{tag}_upper", (0.0,) * 6)[0], 4)

        samples = {tag: tuple(swing(tag, f / 8.0) for f in range(8)) for tag in rn.LEG_TAGS}
        self.assertEqual(samples["fl"], samples["rr"], "a diagonal gait pairs fl with rr")
        self.assertNotEqual(samples["fl"], samples["fr"], "and opposes them to the other pair")

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(33):
                mesh = rn.posed(0, rn.sample_pose(clip, i / 32.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -0.01, f"{clip.name} at {i / 32.0:.3f}")

    def test_foot_planting_is_measured(self):
        self.assertTrue(rn.FOOT_LIFT)
        for name, lift in rn.FOOT_LIFT.items():
            self.assertGreaterEqual(lift, 0.0, name)
            self.assertLess(lift, 20.0, name)

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips),
                         ["attack", "death", "detect", "idle", "move"])

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(rn.SOCKETS))
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Fin_Array_Base"].position[0], by_name["Fin_Array_Tip"].position[0])
        self.assertEqual(self.socks["Emitter_Muzzle"], "head")

    def test_provisional_card_budget(self):
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-FAC-025.KA.RESONANT.ASSET", data["provisional_contract"]["card"])
            self.assertIn("FOUR stilt legs", data["anatomy_correction"]["why_wrong"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        worst = max(lod0 or self.m0.triangle_count(), self.detecting.triangle_count())
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(worst, 4500)
        self.assertLessEqual(lod1, 1800)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_keeps_every_fin_resolvable(self):
        self.assertEqual(sum(1 for c in self.m1.components()
                             if c.startswith("fin_") and not c.endswith("_vein")), rn.FIN_COUNT)

    def test_states_are_declared(self):
        self.assertEqual(tuple(rn.STATES), ("passive", "detecting"))
        with self.assertRaises(ValueError):
            rn.assemble(0, "contact")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = rn.assemble(0, "passive")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = rn.assemble(0, "passive")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(rn.REVISION, f"{rn.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
