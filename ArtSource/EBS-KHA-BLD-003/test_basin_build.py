#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-BLD-003 Growth Basin concept blockout.

Author: Angelis Pseftis. Run: python3 test_basin_build.py
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
import build_basin as gb  # noqa: E402


class BasinBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = gb.assemble(0, "idle")
        cls.m1 = gb.assemble(1, "idle")[0]
        cls.growing = gb.assemble(0, "growing")[0]
        cls.molting = gb.assemble(0, "molting")[0]
        cls.damaged = gb.assemble(0, "damaged")[0]
        cls.destroyed = gb.assemble(0, "destroyed")[0]
        cls.clips = gb.build_clips(cls.s)
        cls.inv = gb.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_contract_inventory(self):
        self.assertEqual(self.inv["shallow_bowl"]["courses"], gb.RIM_COURSES)
        self.assertTrue(self.inv["shallow_bowl"]["floor"])
        self.assertTrue(self.inv["matrix_pool"]["built"])
        self.assertEqual(self.inv["molt_niches"]["walls"], gb.NICHE_COUNT)
        self.assertEqual(self.inv["molt_niches"]["floors"], gb.NICHE_COUNT)
        self.assertEqual(self.inv["molt_niches"]["ring_spurs"], gb.NICHE_COUNT)

    def test_the_niches_are_open_alcoves_and_never_close(self):
        # the candidate's MOLT NICHE panel shows a warform standing in an OPEN alcove; the card's first
        # draft said the niche closes around it, which was my invention and was corrected on the card
        self.assertFalse(self.inv["molt_niches"]["closes"])
        for state in ("idle", "growing", "molting"):
            comps = gb.assemble(0, state)[0].components()
            self.assertFalse([c for c in comps if "hood" in c or "shutter" in c or "lid" in c])

    def test_the_bowl_is_shallow_and_the_pool_dominates_it(self):
        m = gb.measurements(self.m0)
        self.assertAlmostEqual(m["pool_over_basin"], 0.75, delta=0.01)   # traced
        self.assertLess(m["height_over_basin_diameter"], 0.25, "a shallow bowl, per canon")

    def test_material_slots_and_amber_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [gb.STRATA, gb.AMBER])
        lit = {p.component for p in self.molting.polygons if self.molting.slots[p.slot] == gb.AMBER}
        self.assertTrue(all(c.startswith(("matrix_pool", "niche_")) for c in lit), lit)
        self.assertLessEqual(gb.slot_area_fraction(self.molting, gb.AMBER), 0.15)  # REL-ART-029 ceiling

    def test_occupancy_is_readable_per_niche(self):
        self.assertEqual(gb.lit_niches(self.m0), [], "idle: every niche dark")
        self.assertEqual(gb.lit_niches(self.growing), [], "growing changes the pool, not the niches")
        self.assertEqual(gb.lit_niches(self.molting), sorted(gb.MOLTING_NICHES))
        # the lit lip is what carries the tell past the ring spurs at gameplay distance
        lips = {c for c in self.molting.components() if c.endswith("_lip")}
        self.assertEqual(len(lips), len(gb.MOLTING_NICHES))
        self.assertFalse([c for c in self.m0.components() if c.endswith("_lip")])

    def test_growing_changes_the_pools_emissive_not_the_geometry(self):
        self.assertEqual(sorted(self.growing.components()), sorted(self.m0.components()))
        clip = {c.name: c for c in self.clips}["growing"]
        for keys in clip.tracks.values():
            for k in keys:
                self.assertEqual(k.translation_cm, (0.0, 0.0, 0.0))
                self.assertEqual(k.rotation_deg, (0.0, 0.0, 0.0))

    def test_the_molt_complete_clip_is_canons_crack_and_settle(self):
        clip = {c.name: c for c in self.clips}["molt_complete"]
        active = f"niche_{gb.MOLTING_NICHES[0]:02d}"
        drops = [k.translation_cm[2] for k in clip.tracks[active]]
        self.assertLess(min(drops), -8.0, "a sharp drop")
        self.assertGreater(max(drops), 0.0, "then a recovery past rest")
        self.assertAlmostEqual(drops[-1], 0.0, places=6, msg="and a settle back to rest")
        idle_niche = f"niche_{[i for i in range(1, gb.NICHE_COUNT + 1) if i not in gb.MOLTING_NICHES][0]:02d}"
        self.assertEqual({k.translation_cm[2] for k in clip.tracks[idle_niche]}, {0.0},
                         "an unoccupied niche does not settle")

    def test_clip_durations_use_the_canon_molt_ticks(self):
        by_name = {c.name: c for c in self.clips}
        self.assertAlmostEqual(by_name["molt_complete"].duration_s * gb.TICKS_PER_SECOND,
                               gb.MOLT_TICKS / 4.0, places=3)
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)

    def test_the_rig_is_root_plus_one_bone_per_niche(self):
        names = [b.name for b in self.s.bones]
        self.assertEqual(names[0], "root")
        self.assertEqual(len(names), gb.NICHE_COUNT + 1)
        self.assertEqual(names[1:], [f"niche_{i:02d}" for i in range(1, gb.NICHE_COUNT + 1)])

    def test_the_plinth_is_bound_to_root_not_to_the_niche(self):
        # the crack-and-settle must not push the alcove through the ground
        bones = {p.bone for p in self.m0.polygons if p.component.startswith("plinth_")}
        self.assertEqual(bones, {"root"})

    def test_everything_stays_inside_the_four_by_four_footprint(self):
        for mesh, label in ((self.m0, "idle"), (self.m1, "LOD1"), (self.molting, "molting"),
                            (self.damaged, "damaged"), (self.destroyed, "destroyed")):
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), gb.HALF + 1e-6, label)
            self.assertGreaterEqual(z0, -1e-6, label)

    def test_no_posed_frame_leaves_the_ground_or_the_footprint(self):
        by_name = {c.name: c for c in self.clips}
        for name, fraction, state in gb.POSE_SAMPLES:
            mesh = gb.posed(0, gb.sample_pose(by_name[name], fraction), state)
            (x0, y0, z0), (x1, y1, _) = mesh.bounds()
            self.assertGreaterEqual(z0, -1e-6, f"{name}@{fraction}")
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), gb.HALF + 1e-6, f"{name}@{fraction}")

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(gb.SOCKETS))
        for i in range(1, gb.NICHE_COUNT + 1):
            self.assertEqual(self.socks[f"Molt_Niche_{i:02d}"], f"niche_{i:02d}")
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Rally_Default"].position[0], -gb.OUTER_R)

    def test_states_differ_where_canon_says(self):
        self.assertNotIn(gb.AMBER, {self.damaged.slots[p.slot] for p in self.damaged.polygons
                                    if p.component == "matrix_pool"})
        self.assertTrue([c for c in self.damaged.components() if c.startswith("crack_")])
        for prefix in ("niche_", "matrix_pool"):
            self.assertFalse([c for c in self.destroyed.components() if c.startswith(prefix)
                              and c != "matrix_pool_dark"], prefix)
        self.assertTrue([c for c in self.destroyed.components() if c.startswith("rubble_")])

    def test_provisional_card_budget_and_the_recorded_gap(self):
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-BLD-016.KA.BASIN.ASSET", data["provisional_contract"]["card"])
            self.assertIn("PENDING", data["acceptance"]["technical"])
            self.assertIn("niche count", data["unresolved_contract_gap"]["field"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        lod0 = max(lod0 or self.m0.triangle_count(), self.molting.triangle_count())
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(lod0, 8000)
        self.assertLessEqual(lod1, 3500)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_preserves_the_pool_and_the_niches(self):
        comps = self.m1.components()
        self.assertIn("matrix_pool", comps)
        self.assertEqual(len({c for c in comps if c.endswith("_wall")}), gb.NICHE_COUNT)
        self.assertEqual(len({c for c in comps if c.startswith("niche_") and c.endswith("_floor")}),
                         gb.NICHE_COUNT)

    def test_states_are_declared(self):
        self.assertEqual(tuple(gb.STATES), ("idle", "growing", "molting", "damaged", "destroyed"))
        with self.assertRaises(ValueError):
            gb.assemble(0, "researching")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = gb.assemble(0, "idle")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = gb.assemble(0, "idle")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(gb.REVISION, f"{gb.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
