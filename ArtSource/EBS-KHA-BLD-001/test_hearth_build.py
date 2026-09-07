#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-BLD-001 Memory Hearth concept blockout.

Author: Angelis Pseftis. Run: python3 test_hearth_build.py
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
import build_hearth as hh  # noqa: E402


class HearthBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0 = hh.assemble(0, "working")
        cls.m1 = hh.assemble(1, "working")
        cls.damaged = hh.assemble(0, "damaged")
        cls.destroyed = hh.assemble(0, "destroyed")
        cls.inv = hh.contract_inventory(cls.m0)

    def test_contract_inventory(self):
        self.assertEqual(self.inv["banded_shell"]["built"], hh.BANDS)
        self.assertEqual(self.inv["worker_hollows"]["interiors"], hh.HOLLOW_COUNT)
        self.assertEqual(self.inv["worker_hollows"]["jambs"], 2 * hh.HOLLOW_COUNT)
        self.assertEqual(self.inv["worker_hollows"]["lintels"], hh.HOLLOW_COUNT)
        self.assertEqual(self.inv["worker_hollows"]["thresholds"], hh.HOLLOW_COUNT)
        self.assertEqual(self.inv["adaptation_crown"]["built"], len(hh.SPIRES))
        self.assertTrue(all(self.inv["intake_cleft"][k] for k in ("hood", "recess", "lip")))

    def test_material_slots(self):
        self.assertEqual(self.m0.slots, [hh.STRATA, hh.FIBRE, hh.AMBER])

    def test_the_shell_is_faceted_bands_not_a_smooth_dome(self):
        # REL-ART-029 forbids organic smoothing: every band is a straight-sided prism, and each one
        # steps in above the one below it
        radii = [hh.band_profile(i)[0] for i in range(hh.BANDS)]
        for a, b in zip(radii, radii[1:]):
            self.assertLess(b, a, "each band steps in as it rises")
        self.assertGreaterEqual(hh.BANDS, 6)

    def test_breadth_is_greater_than_shell_height(self):
        # the canon phrase, asserted numerically
        m = hh.measurements(self.m0)
        self.assertTrue(m["breadth_greater_than_shell_height"])
        self.assertLess(hh.SHELL_Z, 2 * hh.BASE_R)

    def test_traced_concept_proportions(self):
        m = hh.measurements(self.m0)
        self.assertAlmostEqual(m["shell_height_over_base_width"], 0.44, delta=0.015)
        self.assertAlmostEqual(m["total_height_over_base_width"], 0.69, delta=0.015)
        self.assertAlmostEqual(m["spire_spread_over_base_width"], 0.55, delta=0.02)

    def test_everything_stays_inside_the_five_by_five_footprint(self):
        for mesh, label in ((self.m0, "LOD0"), (self.m1, "LOD1"), (self.damaged, "damaged"),
                            (self.destroyed, "destroyed")):
            (x0, y0, _), (x1, y1, _) = mesh.bounds()
            self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), hh.HALF + 1e-6, label)

    def test_nothing_below_the_ground(self):
        for mesh in (self.m0, self.m1, self.damaged, self.destroyed):
            self.assertGreaterEqual(mesh.bounds()[0][2], -1e-6)

    def test_the_hollows_are_openings_not_lit_plates(self):
        # the glow sits BEHIND the jamb faces, so it reads through a doorway rather than as a panel
        for i in range(1, hh.HOLLOW_COUNT + 1):
            glow = self.m0.component_bounds(f"hollow_{i:02d}_interior")
            jamb = self.m0.component_bounds(f"hollow_{i:02d}_jamb_l")
            # compare radial distance to each component's CENTRE: a bounding-box extent is not a
            # radial measure for a part sitting off the axes
            def centre_radius(b):
                cx = (b[0][0] + b[1][0]) / 2.0
                cy = (b[0][1] + b[1][1]) / 2.0
                return (cx * cx + cy * cy) ** 0.5

            gr = centre_radius(glow)
            jr = centre_radius(jamb)
            self.assertLess(gr, jr, f"hollow {i}: the glow must sit behind the jamb face")
            self.assertIn(f"hollow_{i:02d}_arch_head", self.m0.components())

    def test_the_cleft_differs_from_an_arch_in_silhouette(self):
        hood = self.m0.component_bounds("cleft_hood")
        arch = self.m0.component_bounds("hollow_03_lintel")
        cleft_w = max(hood[1][k] - hood[0][k] for k in (0, 1))
        arch_w = max(arch[1][k] - arch[0][k] for k in (0, 1))
        self.assertGreater(cleft_w, arch_w, "the cleft is wider than a worker arch")
        self.assertLess(hh.CLEFT_Z, hh.HOLLOW_Z, "and lower")
        self.assertGreater(self.m0.component_bounds("cleft_hood")[1][1], 0.0, "it sits on the +Y flank")

    def test_the_hollows_and_the_cleft_are_on_different_sides(self):
        cleft = self.m0.component_bounds("cleft_hood")
        for i in range(1, hh.HOLLOW_COUNT + 1):
            hollow = self.m0.component_bounds(f"hollow_{i:02d}_lintel")
            self.assertLess(hollow[0][0], cleft[0][0], f"hollow {i} sits away from the cleft flank")

    def test_amber_is_the_only_emissive_and_stays_under_the_kharuun_cap(self):
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == hh.AMBER}
        self.assertTrue(all(c.startswith(("hollow_", "growth_seam_")) or c.endswith("_vein") for c in lit), lit)
        self.assertLessEqual(hh.slot_area_fraction(self.m0, hh.AMBER), 0.15)  # REL-ART-029

    def test_states_differ_where_canon_says(self):
        # damaged: one spire's vein dark, and the strata cracked
        working_veins = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == hh.AMBER
                         and p.component.endswith("_vein")}
        damaged_veins = {p.component for p in self.damaged.polygons if self.damaged.slots[p.slot] == hh.AMBER
                         and p.component.endswith("_vein")}
        self.assertEqual(len(working_veins) - len(damaged_veins), 1, "exactly one spire goes dark")
        self.assertTrue([c for c in self.damaged.components() if c.startswith("crack_")])
        self.assertFalse([c for c in self.m0.components() if c.startswith("crack_")])
        # destroyed: collapse inward, no hollows, cleft or spires, and lower than the working shell
        for prefix in ("hollow_", "cleft_", "spire_", "growth_seam_"):
            self.assertFalse([c for c in self.destroyed.components() if c.startswith(prefix)], prefix)
        self.assertTrue([c for c in self.destroyed.components() if c.startswith("rubble_")])
        self.assertLess(self.destroyed.bounds()[1][2], self.m0.bounds()[1][2] * 0.7)

    def test_sockets(self):
        names = sorted(s.name for s in self.m0.sockets)
        self.assertEqual(names, sorted(hh.SOCKETS))
        by_name = {s.name: s for s in self.m0.sockets}
        self.assertLess(by_name["Unit_Emergence"].position[0], 0.0, "workers emerge on the hollow side")
        self.assertGreater(by_name["Matter_Dropoff"].position[1], 0.0, "matter is delivered at the cleft flank")
        self.assertLess(by_name["Rally_Default"].position[0], -hh.BASE_R, "the rally point is clear of the thresholds")
        self.assertGreater(by_name["Adaptation_Crown"].position[2], hh.SHELL_Z)

    def test_collision_is_the_shell_only(self):
        self.assertEqual(len(self.m0.collision), 2)
        for box in self.m0.collision:
            self.assertLessEqual(box.size[0] / 2.0, hh.BASE_R + 1e-6)

    def test_provisional_budget(self):
        # the ceilings come from the PROVISIONAL Kharuun card, which is not yet in the authoritative
        # requirements; the manifest has to keep saying so
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-BLD-016.KA.HEARTH.ASSET", data["provisional_contract"]["card"])
            self.assertIn("PROVISIONAL", data["provisional_contract"]["status"])
            self.assertIn("PENDING", data["acceptance"]["technical"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        lod0 = lod0 or self.m0.triangle_count()
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(lod0, 8000)
        self.assertLessEqual(lod1, 3500)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_preserves_what_the_card_requires(self):
        # REL-BLD-016.KA.HEARTH.ASSET .READABILITY: LOD1 shall preserve the crown, the worker hollows,
        # the intake cleft and the headquarters silhouette. A LOD that drops any of them fails the card
        # regardless of triangle count.
        comps = self.m1.components()
        self.assertEqual(sum(1 for c in comps if c.startswith("spire_") and "_vein" not in c),
                         len(hh.SPIRES), "LOD1 keeps every spire: the crown is a required read")
        self.assertEqual(sum(1 for c in comps if c.endswith("_interior")), hh.HOLLOW_COUNT,
                         "LOD1 keeps every worker hollow")
        for part in ("cleft_hood", "cleft_recess", "cleft_lip"):
            self.assertIn(part, comps, "LOD1 keeps the intake cleft distinct")
        tall = self.m1.bounds()[1][2]
        self.assertAlmostEqual(tall, self.m0.bounds()[1][2], delta=1.0,
                               msg="LOD1 keeps the headquarters silhouette height")

    def test_states_are_declared(self):
        self.assertEqual(tuple(hh.STATES), ("working", "damaged", "destroyed"))
        with self.assertRaises(ValueError):
            hh.assemble(0, "researching")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = hh.assemble(0, "working").write_glb(os.path.join(tmp, "a.glb"))
            b = hh.assemble(0, "working").write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(hh.REVISION, f"{hh.PRODUCTION_ID.lower()}-concept-v1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
