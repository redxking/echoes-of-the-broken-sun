#!/usr/bin/env python3
"""Regression checks for the EBS-FWL-SYS-001 Future Well blockout source.

Author: Angelis Pseftis. Run: python3 test_future_well_build.py
Structural checks: contract inventory, one-tile impassable mass, walkable apron relief,
petal fold, sockets, budgets, determinism. Not gate acceptance.
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
import build_future_well as fw  # noqa: E402


class FutureWellBlockoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main0 = fw.build_main(0)
        cls.main1 = fw.build_main(1)
        cls.petal0 = fw.build_petal(0)
        cls.inventory = fw.contract_inventory(cls.main0)

    def test_contract_inventory(self):
        self.assertEqual(self.inventory["bowl"]["built"], 1)
        self.assertEqual(self.inventory["bowl"]["rim_blocks"], fw.RIM_BLOCKS)
        self.assertEqual(self.inventory["core_spire"]["petal_sockets"], 4)
        self.assertEqual(self.inventory["state_family"]["built"], 4)
        self.assertEqual(len(fw.STATE_PRESENTATIONS), 13)
        for track in ("dormant", "harvest_telegraph", "harvest_cancel", "harvest_commit", "spent", "preserve_hold", "preserve_loss",
                      "reshape_telegraph", "reshape_cancel", "reshape_manifest", "reshape_warning", "reshape_expiry", "restore"):
            self.assertIn(track, [p["track"] for p in fw.STATE_PRESENTATIONS])

    def test_impassable_mass_inside_one_tile(self):
        half = fw.TILE_CM / 2.0
        for comp in self.main0.components():
            if comp.startswith(("rim_block_", "bowl_floor", "core_stub", "bowl_vein_")):
                (x0, y0, _), (x1, y1, _) = self.main0.component_bounds(comp)
                self.assertGreaterEqual(x0, -half, comp)
                self.assertLessEqual(x1, half, comp)
                self.assertGreaterEqual(y0, -half, comp)
                self.assertLessEqual(y1, half, comp)
        box = self.main0.collision[0]
        self.assertLessEqual(box.size[0], fw.TILE_CM)
        self.assertLessEqual(box.size[1], fw.TILE_CM)

    def test_apron_is_walkable_relief(self):
        for comp in self.main0.components():
            if comp.startswith(("apron_", "preserve_ring", "reshape_trace")):
                (_, _, z0), (_, _, z1) = self.main0.component_bounds(comp)
                self.assertLessEqual(z1, 20.0, comp)  # REL-ART-030 decorative displacement cap
                self.assertGreaterEqual(z0, 0.0, comp)

    def test_sockets(self):
        names = [s.name for s in self.main0.sockets]
        for required in ("Target_Anchor_Center", "State_VFX_Origin"):
            self.assertIn(required, names)
        petals = [s for s in self.main0.sockets if s.name.startswith("Petal_")]
        self.assertEqual(sorted(round(s.yaw_deg) for s in petals), [0, 90, 180, 270])

    def test_spire_closes_and_folds(self):
        up = fw.assemble(0, "dormant")
        spent = fw.assemble(0, "spent")
        (_, _, _), (_, _, up_top) = up.bounds()
        (_, _, _), (_, _, spent_top) = spent.bounds()
        self.assertGreater(up_top, 150.0)          # spire stands
        self.assertLess(spent_top, 90.0)           # folded into the bowl: nothing rises above the rim
        # folded petals stay inside the one-tile footprint
        for comp in spent.components():
            if comp.startswith("petal_"):
                (x0, y0, _), (x1, y1, _) = spent.component_bounds(comp)
                self.assertGreaterEqual(min(x0, y0), -fw.TILE_CM / 2.0 - 1.0, comp)
                self.assertLessEqual(max(x1, y1), fw.TILE_CM / 2.0 + 1.0, comp)

    def test_budgets_and_slots(self):
        lod0 = self.main0.triangle_count() + 4 * self.petal0.triangle_count()
        lod1 = self.main1.triangle_count() + 4 * fw.build_petal(1).triangle_count()
        self.assertLessEqual(lod0, 8000)
        self.assertLessEqual(lod1, 3500)
        self.assertLess(lod1, lod0)
        self.assertEqual(self.main0.slots, [fw.BASALT, fw.VITRIFIED, fw.STATE])

    def test_state_slot_bounded(self):
        # Triangle share is only a proxy (the Preserve ring is a 32-gon annulus); the emissive AREA
        # limit (<= 15%) is measured from the unlit render (renders/area_check/emissive-area.json).
        by_slot = self.main0.triangle_count_by("slot")
        self.assertLess(by_slot[fw.STATE] / self.main0.triangle_count(), 0.5)

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = fw.build_main(0).write_glb(os.path.join(tmp, "a.glb"))
            b = fw.build_main(0).write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)


if __name__ == "__main__":
    unittest.main(verbosity=1)
