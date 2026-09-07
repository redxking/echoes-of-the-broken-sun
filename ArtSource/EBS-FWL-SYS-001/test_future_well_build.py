#!/usr/bin/env python3
"""Regression checks for the EBS-FWL-SYS-001 Future Well concept-v3 source.

Author: Angelis Pseftis. Run: python3 test_future_well_build.py
Structural checks against concept-fidelity.md (as amended concept-v3) and the bounding rules: contract
inventory, the one-tile impassable dais and collision box, the recessed socket the shard stands in,
walkable relief, the single shard part and its fold, the ring wall part (radius 2.0-2.2 S, one dominant
peak over three stubs, four cardinal gaps >= 140 cm, no collision), the torn apron and its +X tongue, the
flush light-only channels (ring hugging the wall, trace <= 1.5 cm proud, no rungs), review parking,
sockets, budgets, determinism, and two evidence pins (parts scene shard height, emissive-area limit;
set EBS_FWL_EVIDENCE_DIR or rely on the default evidence root, skipped when absent). Not gate acceptance.
"""
from __future__ import annotations

import json
import math
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import build_future_well as fw  # noqa: E402

S = fw.SPIRE_HEIGHT
EVIDENCE_DIR = os.environ.get("EBS_FWL_EVIDENCE_DIR", "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/asset-production-20260906T221157Z/EBS-FWL-SYS-001")
TRACKS = ("dormant", "harvest_telegraph", "harvest_cancel", "harvest_commit", "spent", "preserve_hold", "preserve_loss",
          "reshape_telegraph", "reshape_cancel", "reshape_manifest", "reshape_warning", "reshape_expiry", "restore")


class FutureWellConceptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main0, cls.main1 = fw.build_main(0), fw.build_main(1)
        cls.spire0, cls.spire1 = fw.build_spire(0), fw.build_spire(1)
        cls.wall0, cls.wall1 = fw.build_wall(0), fw.build_wall(1)
        cls.inventory = fw.contract_inventory(cls.main0, cls.spire0, cls.wall0)
        cls.wall_measure = fw.measure_wall(cls.wall0)

    # --- contract and states ---------------------------------------------------------------
    def test_contract_inventory_and_presentations(self):
        self.assertEqual(self.inventory["courtyard_bowl"]["built"], 1)
        self.assertEqual(self.inventory["ring_wall"]["built"], 1)
        self.assertEqual(self.inventory["core_spire"]["built"], 1)
        self.assertEqual(self.inventory["core_spire"]["hinge_sockets"], 1)
        self.assertEqual(self.inventory["state_family"]["built"], 4)
        self.assertEqual(len(fw.STATE_PRESENTATIONS), 13)
        self.assertEqual([p["track"] for p in fw.STATE_PRESENTATIONS], list(TRACKS))
        self.assertEqual(set(fw.REVIEW_STATES), set(fw.POSE_AMOUNT))
        self.assertEqual(fw.REVIEW_STATES, ("dormant", "harvest_telegraph", "harvest_fold_mid", "harvest_commit", "spent", "preserve", "reshape", "all_channels"))

    def test_revision_is_concept_v3(self):
        self.assertEqual(fw.REVISION, "ebs-fwl-sys-001-concept-v3")

    # --- one-tile impassable mass ---------------------------------------------------------
    def test_impassable_mass_and_collision_inside_one_tile(self):
        half = fw.TILE_CM / 2.0
        for comp in self.main0.components():
            if comp.startswith("dais_"):
                (x0, y0, _), (x1, y1, _) = self.main0.component_bounds(comp)
                self.assertGreaterEqual(min(x0, y0), -half, comp)
                self.assertLessEqual(max(x1, y1), half, comp)
        self.assertEqual(len(self.main0.collision), 1)
        box = self.main0.collision[0]
        self.assertLessEqual(box.size[0], fw.TILE_CM)
        self.assertLessEqual(box.size[1], fw.TILE_CM)
        self.assertLessEqual(abs(box.center[0]) + box.size[0] / 2.0, half)
        self.assertLessEqual(abs(box.center[1]) + box.size[1] / 2.0, half)
        # the shard base stands on the dais: every base corner inside the dais top step
        base, _mid, _apex = fw.spire_rings()
        for p in base:
            self.assertLessEqual(math.hypot(p[0], p[1]), fw.DAIS_STEP_R)

    def test_dais_socket_reads_as_recessed_bowl_centre(self):
        # concept-fidelity.md item 5 / candidate spent inset: a round dark socket recessed inside the rim, cracked, permanent
        (x0, y0, z0), (x1, y1, z1) = self.main0.component_bounds("dais_socket")
        self.assertLess(z1, fw.DAIS_H)                      # recessed below the rim top
        self.assertAlmostEqual(z1, fw.DAIS_H - fw.SOCKET_DROP)
        self.assertLessEqual(max(abs(x0), abs(x1), abs(y0), abs(y1)), fw.SOCKET_R + 1e-6)
        self.assertGreater(min(x1 - x0, y1 - y0), 1.9 * fw.SOCKET_R)  # round, not a square plate
        slots = {self.main0.slots[p.slot] for p in self.main0.polygons if p.component == "dais_socket"}
        self.assertEqual(slots, {fw.VITRIFIED})
        rim = {self.main0.slots[p.slot] for p in self.main0.polygons if p.component == "dais_step_02"}
        self.assertEqual(rim, {fw.BASALT})
        (_, _, rz0), (_, _, rz1) = self.main0.component_bounds("dais_step_02")
        self.assertAlmostEqual(rz1, fw.DAIS_H)
        veins = [c for c in self.main0.components() if c.startswith("dais_vein_")]
        self.assertEqual(len(veins), fw.DAIS_VEINS)
        for c in veins:  # the cracks sit ON the socket top (not buried) and stay inside the socket
            (vx0, vy0, vz0), (vx1, vy1, vz1) = self.main0.component_bounds(c)
            self.assertAlmostEqual(vz0, z1)
            self.assertGreater(vz1, z1)
            self.assertLess(vz1, fw.HINGE_Z)
            self.assertLess(max(math.hypot(vx0, vy0), math.hypot(vx1, vy1), math.hypot(vx0, vy1), math.hypot(vx1, vy0)), fw.SOCKET_R + 1e-6)
            self.assertEqual({self.main0.slots[p.slot] for p in self.main0.polygons if p.component == c}, {fw.STATE})
        # the shard's base corners rest on the rim annulus
        base, _mid, _apex = fw.spire_rings()
        for p in base:
            self.assertTrue(fw.SOCKET_R <= math.hypot(p[0], p[1]) <= fw.DAIS_STEP_R, p)
        # once spent the socket is the highest thing left inside the tile centre
        spent = fw.assemble(0, "spent")
        self.assertLess(max(spent.component_bounds(c)[1][2] for c in spent.components() if c.startswith("spire_")), spent.component_bounds("dais_socket")[1][2])

    def test_walkable_dressing_relief(self):
        for mesh in (self.main0, self.main1):
            for comp in mesh.components():
                if not comp.startswith("dais_"):
                    (_, _, z0), (_, _, z1) = mesh.component_bounds(comp)
                    self.assertLessEqual(z1, 20.0, comp)  # REL-ART-016 / REL-ART-030
                    self.assertGreaterEqual(z0, 0.0, comp)

    # --- the shard ---------------------------------------------------------------------
    def test_single_faceted_shard(self):
        comps = self.spire0.components()
        self.assertEqual([c for c in comps if not c.startswith("spire_seam_")], ["spire_body"])
        self.assertEqual(sum(1 for c in comps if c.startswith("spire_seam_")), 4)
        m = fw.measure_spire(self.spire0)
        self.assertAlmostEqual(m["height_cm"], S, places=3)
        self.assertGreaterEqual(m["facets"], 12)  # faceted, not a plain 4-face pyramid
        across = 2 * fw.SPIRE_BASE_HALF / S
        self.assertTrue(0.5 <= across <= 0.7, across)  # candidate: base ~0.63 S
        self.assertEqual(self.spire0.slots, [fw.VITRIFIED, fw.STATE])
        self.assertEqual(self.spire0.collision, [])
        self.assertLess(self.spire1.triangle_count(), self.spire0.triangle_count())

    def test_shard_stands_then_sinks_below_the_paving_inside_the_tile(self):
        up, spent = fw.assemble(0, "dormant"), fw.assemble(0, "spent")
        self.assertGreater(up.bounds()[1][2], fw.DAIS_H + S - 1.0)
        spire_comps = [c for c in spent.components() if c.startswith("spire_")]
        self.assertTrue(spire_comps)
        top = max(spent.component_bounds(c)[1][2] for c in spire_comps)
        self.assertLess(top, fw.PAVING_Z)  # gone below the paving
        for c in spire_comps:
            (x0, y0, _), (x1, y1, _) = spent.component_bounds(c)
            self.assertGreaterEqual(min(x0, y0), -fw.TILE_CM / 2.0, c)
            self.assertLessEqual(max(x1, y1), fw.TILE_CM / 2.0, c)
        # nothing rises above the wall once spent
        wall_top = max(self.wall_measure["tall_heights_cm"])
        non_wall = [c for c in spent.components() if not c.startswith("wall_")]
        self.assertLess(max(spent.component_bounds(c)[1][2] for c in non_wall), wall_top)
        # intermediate poses are monotone: mid < commit < spent in sink depth
        mid, commit = fw.assemble(0, "harvest_fold_mid"), fw.assemble(0, "harvest_commit")
        tops = [max(a.component_bounds(c)[1][2] for c in a.components() if c.startswith("spire_")) for a in (up, mid, commit, spent)]
        self.assertEqual(tops, sorted(tops, reverse=True))

    # --- the ring wall part ---------------------------------------------------------------
    def test_wall_is_a_separate_collisionless_component_group(self):
        self.assertEqual(self.wall0.name, fw.WALL_ASSET)
        self.assertEqual(self.wall0.collision, [])
        self.assertEqual(self.wall1.collision, [])
        self.assertTrue(all(c.startswith("wall_") for c in self.wall0.components()))
        self.assertFalse(any(c.startswith("wall_") for c in self.main0.components()))
        self.assertEqual(self.wall0.slots, [fw.BASALT])

    def test_wall_proportions_to_the_concept(self):
        m = self.wall_measure
        self.assertAlmostEqual(m["outer_radius_cm"], fw.WALL_R_OUT, delta=2.5)
        self.assertAlmostEqual(fw.WALL_R_OUT, 450.0)
        self.assertTrue(1.9 <= m["ratios_to_S"]["outer_radius"] <= 2.3, m["ratios_to_S"])   # candidate pixels 2.0-2.4 S (fidelity file amended concept-v3)
        self.assertTrue(0.25 <= m["ratios_to_S"]["low_wall_crest_mean"] <= 0.40, m["ratios_to_S"])  # concept 0.26-0.35 S
        self.assertEqual(m["tall_segments"], 4)
        heights = sorted(m["tall_heights_cm"])
        self.assertTrue(0.8 * S <= heights[-1] <= 0.95 * S, heights)                   # ONE dominant peak (candidate back peak ~0.85-0.95 S)
        self.assertTrue(all(0.55 * S <= h <= 0.7 * S for h in heights[:-1]), heights)  # three lower stubs
        self.assertGreater(heights[-1] - heights[-2], 0.15 * S)                        # the peak dominates, the four do not read near-equal
        self.assertEqual(max(range(4), key=lambda k: fw.WALL_TALL_H[k]), 3)            # the peak is wall_tall_04 on the far diagonal (306-322 deg)
        self.assertGreater(m["low_crest_max_cm"] - m["low_crest_min_cm"], 20.0)  # ruined rhythm, not a flat parapet
        for mesh in (self.wall0, self.wall1):
            self.assertEqual(sum(1 for c in mesh.components() if c.startswith("wall_tall_")), 4)
            self.assertGreaterEqual(sum(1 for c in mesh.components() if c.startswith("wall_block_")), 20)

    def test_four_cardinal_gaps(self):
        for mesh in (self.wall0, self.wall1):
            gaps = fw.measure_gaps(mesh)
            self.assertEqual(sorted(g["facing_deg"] for g in gaps.values()), [0, 90, 180, 270])
            for name, g in gaps.items():
                self.assertGreaterEqual(g["chord_cm"], fw.WALL_GAP_MIN_CM, name)
        sockets = {s.name: s for s in self.main0.sockets}
        for k in range(4):
            s = sockets[f"Wall_Gap_{k + 1:02d}"]
            self.assertEqual(round(s.yaw_deg), 90 * k)
            self.assertAlmostEqual(math.hypot(s.position[0], s.position[1]), (fw.WALL_R_OUT + fw.WALL_R_IN) / 2.0, places=3)

    # --- state channels --------------------------------------------------------------------
    def test_preserve_ring_hugs_the_wall_and_trace_through_the_plus_x_gap(self):
        (x0, y0, z0), (x1, y1, z1) = self.main0.component_bounds("preserve_ring")
        self.assertLess(max(abs(x0), abs(x1), abs(y0), abs(y1)), fw.WALL_R_IN)      # inside the wall's inner face ...
        self.assertGreaterEqual(fw.PRESERVE_RING_R / fw.WALL_R_OUT, 0.88)             # ... hugging it (candidate 03: 0.90 R_out, ring touches the wall)
        self.assertGreater(max(abs(x0), abs(x1)), fw.PAVING_COURSES[-1])
        self.assertLessEqual(z1, fw.PAVING_Z + fw.FLUSH_PROUD)                         # flat light on the paving, no hoop
        (tx0, ty0, _), (tx1, ty1, _) = self.main0.component_bounds("reshape_trace")
        self.assertGreater(tx1, fw.WALL_R_OUT)  # leaves the courtyard ...
        self.assertLess(tx0, fw.DAIS_R + 1.0)   # ... from the dais ...
        self.assertLess(max(abs(ty0), abs(ty1)), fw.WALL_GAP_WIDTH[0] / 2.0)  # ... through the +X gap
        end = next(s for s in self.main0.sockets if s.name == "Reshape_Trace_End")
        self.assertAlmostEqual(end.position[0], tx1, places=3)
        self.assertEqual(end.position[1], 0.0)
        for comp in ("preserve_ring", "reshape_trace"):
            slots = {self.main0.slots[p.slot] for p in self.main0.polygons if p.component == comp}
            self.assertEqual(slots, {fw.STATE}, comp)

    def test_reshape_trace_is_flush_light_not_relief(self):
        # candidates 01/03 and the spent inset show no trace; 04 shows a thin line ON the paving: the trace must vanish at mask 0
        for mesh in (self.main0, self.main1):
            for poly in mesh.polygons:
                if poly.component != "reshape_trace":
                    continue
                for x, y, z in poly.points:
                    self.assertLessEqual(abs(y), fw.RESHAPE_TRACE_W / 2.0 + 1e-6)   # one thin line, no rung geometry
                    substrate = fw.SPOKE_TOP_Z if x <= fw.SPOKE_R_END + 1e-6 else (fw.PAVING_Z if x <= fw.PAVING_R - 2.0 + 1e-6 else fw.APRON_Z)
                    self.assertLessEqual(z, substrate + fw.FLUSH_PROUD + 1e-6, (x, z))   # <= 1.5 cm proud of what it rides on
            self.assertLessEqual(fw.RESHAPE_TRACE_W, fw.SPOKE_W / 2.0)                  # a line on the band, not the band
        self.assertLessEqual(self.main0.component_bounds("reshape_trace")[1][2], fw.SPOKE_TOP_Z + 1.0)

    def test_apron_is_a_torn_field_with_the_plus_x_tongue(self):
        for lod, mesh in ((0, self.main0), (1, self.main1)):
            m = fw.measure_apron(mesh)
            self.assertEqual(m["sectors"], fw.APRON_SECTORS[lod])
            self.assertGreaterEqual(m["outer_radius_max_cm"] - m["outer_radius_min_cm"], 80.0)      # bites and steps, not a smooth disc
            self.assertTrue(1.2 <= m["ratios_to_wall_R_out"]["outer_radius_mean"] <= 1.4, m)        # candidate: ~1.3 R_out
            self.assertGreater(m["tongue_reach_x_cm"], m["outer_radius_max_cm"] + 40.0)             # the +X tongue reaches past the field ...
            self.assertTrue(1.35 <= m["ratios_to_wall_R_out"]["tongue_reach"] <= 1.6, m)            # ... to ~1.4-1.5 R_out like the painting
            self.assertLess(m["tongue_reach_x_cm"], fw.CAPTURE_RADIUS_CM)                            # inside the capture zone
            self.assertLessEqual(m["relief_cm"], 20.0)                                               # REL-ART-016 / REL-ART-030
            (_, ty0, _), (_, ty1, _) = mesh.component_bounds("apron_tongue")
            self.assertTrue(ty0 < -fw.RESHAPE_TRACE_W and ty1 > fw.RESHAPE_TRACE_W)                  # the trace runs down the tongue
            self.assertGreaterEqual(fw.RESHAPE_TRACE_END_X, mesh.component_bounds("apron_sector_01")[1][0] - 1.0)   # the trace end lies on the tongue, past the field
            for comp in mesh.components():  # rubble keeps the four gap approaches (and the tongue) clear
                if comp.startswith("apron_crumble_"):
                    (cx0, cy0, _), (cx1, cy1, _) = mesh.component_bounds(comp)
                    a = math.degrees(math.atan2((cy0 + cy1) / 2.0, (cx0 + cx1) / 2.0)) % 360.0
                    self.assertGreater(min(abs(((a - c + 180.0) % 360.0) - 180.0) for c in (0.0, 90.0, 180.0, 270.0)), 12.0, comp)

    def test_review_assemblies_light_only_their_channel(self):
        for state, active in fw.ACTIVE_CHANNELS.items():
            scene = fw.assemble(0, state)
            lit = {fw.state_channel(p.component) for p in scene.polygons if scene.slots[p.slot] == fw.STATE}
            if active is None:
                self.assertEqual(lit, {"seams", "spire_seam", "preserve_ring", "reshape_trace"}, state)
                self.assertNotIn(fw.STATE_OFF_REVIEW, scene.slots)
                self.assertNotIn(fw.STATE_HIDDEN_REVIEW, scene.slots)
            else:
                self.assertEqual(lit, set(active), state)
                # unlit seams park as dark fracture; the unlit ring/trace park as the paving itself (nothing implies another state)
                for p in scene.polygons:
                    name = scene.slots[p.slot]
                    if name == fw.STATE_OFF_REVIEW:
                        self.assertNotIn(fw.state_channel(p.component), fw.LIGHT_ONLY_CHANNELS, (state, p.component))
                    elif name == fw.STATE_HIDDEN_REVIEW:
                        self.assertIn(fw.state_channel(p.component), fw.LIGHT_ONLY_CHANNELS, (state, p.component))
        dormant = fw.assemble(0, "dormant")
        self.assertNotIn(fw.STATE_OFF_REVIEW, dormant.slots)      # Dormant lights every seam ...
        self.assertIn(fw.STATE_HIDDEN_REVIEW, dormant.slots)      # ... and hides the ring and the trace

    # --- sockets, budgets, determinism -------------------------------------------------------
    def test_sockets(self):
        names = [s.name for s in self.main0.sockets]
        self.assertEqual(names, ["Target_Anchor_Center", "State_VFX_Origin", "Spire_Hinge", "Reshape_Trace_End", "Wall_Gap_01", "Wall_Gap_02", "Wall_Gap_03", "Wall_Gap_04"])
        self.assertEqual(names, [s.name for s in self.main1.sockets])
        hinge = next(s for s in self.main0.sockets if s.name == "Spire_Hinge")
        self.assertEqual(hinge.position[:2], (0.0, 0.0))
        self.assertAlmostEqual(hinge.position[2], fw.HINGE_Z)
        self.assertGreater(hinge.position[2], fw.DAIS_H)  # rests on the rim, above the recessed socket
        self.assertEqual(self.spire0.sockets, [])
        self.assertEqual(self.wall0.sockets, [])

    def test_budgets_and_slots(self):
        lod0 = self.main0.triangle_count() + self.spire0.triangle_count() + self.wall0.triangle_count()
        lod1 = self.main1.triangle_count() + self.spire1.triangle_count() + self.wall1.triangle_count()
        self.assertLessEqual(lod0, 8000)
        self.assertLessEqual(lod1, 3500)
        self.assertLess(lod1, lod0)
        self.assertEqual(self.main0.slots, [fw.BASALT, fw.VITRIFIED, fw.STATE])
        self.assertEqual(self.main1.slots, [fw.BASALT, fw.VITRIFIED, fw.STATE])
        self.assertEqual(fw.assemble(0, "all_channels").slots, [fw.BASALT, fw.VITRIFIED, fw.STATE])
        dormant = fw.assemble(0, "dormant")
        self.assertEqual(dormant.slots, [fw.BASALT, fw.VITRIFIED, fw.STATE, fw.STATE_HIDDEN_REVIEW])  # ring and trace hidden
        self.assertEqual(fw.assemble(0, "spent").slots, [fw.BASALT, fw.VITRIFIED, fw.STATE, fw.STATE_OFF_REVIEW, fw.STATE_HIDDEN_REVIEW])
        self.assertEqual(dormant.triangle_count(), lod0)

    def test_state_slot_bounded(self):
        # Triangle share is only a proxy; the emissive AREA limit (<= 15%) is measured from the unlit
        # worst-case render (renders/area_check/emissive-area.json).
        by_slot = self.main0.triangle_count_by("slot")
        self.assertLess(by_slot[fw.STATE] / self.main0.triangle_count(), 0.5)

    # --- evidence pins (skipped when the evidence directory is absent) -----------------------
    def test_evidence_parts_scene_instances_the_shard_at_the_hinge(self):
        path = os.path.join(EVIDENCE_DIR, "scenes", "parts.json")
        if not os.path.exists(path):
            self.skipTest(f"no evidence scene at {path}")
        with open(path, "r", encoding="utf-8") as handle:
            scene = json.load(handle)
        shard = [m for m in scene["meshes"] if "_Spire_" in m["obj"]]
        self.assertEqual(len(shard), 1)
        self.assertEqual([float(v) for v in shard[0]["translate"]], [0.0, 0.0, fw.HINGE_Z])

    def test_evidence_emissive_area_within_limit(self):
        path = os.path.join(EVIDENCE_DIR, "renders", "area_check", "emissive-area.json")
        if not os.path.exists(path):
            self.skipTest(f"no emissive measurement at {path}")
        with open(path, "r", encoding="utf-8") as handle:
            area = json.load(handle)
        views = {k: v for k, v in area.items() if isinstance(v, dict)}
        self.assertTrue(views)
        for view, v in views.items():
            self.assertIsNotNone(v.get("state_fraction"), view)
            self.assertLessEqual(v["state_fraction"], 0.15, view)   # concept-fidelity.md: emissive <= 15 % of area at the game framing

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            for builder in (fw.build_main, fw.build_spire, fw.build_wall):
                a = builder(0).write_glb(os.path.join(tmp, "a.glb"))
                b = builder(0).write_glb(os.path.join(tmp, "b.glb"))
                self.assertEqual(a, b, builder.__name__)
            a = fw.assemble(0, "spent").write_obj(os.path.join(tmp, "a.obj"))
            b = fw.assemble(0, "spent").write_obj(os.path.join(tmp, "b.obj"))
            self.assertEqual(a, b)


if __name__ == "__main__":
    unittest.main(verbosity=1)
