#!/usr/bin/env python3
"""Regression checks for the EBS-MER-BLD-002 Power Link blockout source.

Author: Angelis Pseftis. Run: python3 test_power_link_build.py
These checks bind the contract inventory, budgets, footprint, axes, sockets and
export validity to the generator; they are structural checks, not gate acceptance.
"""
from __future__ import annotations

import json
import os
import struct
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import build_power_link as pl  # noqa: E402


class PowerLinkBlockoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main0 = pl.build_main(0)
        cls.main1 = pl.build_main(1)
        cls.panel0 = pl.build_panel(0)
        cls.stub0 = pl.build_stub(0)

    # -- contract inventory ------------------------------------------------------
    def test_contract_component_counts(self):
        inventory = pl.contract_inventory(self.main0)
        self.assertEqual(inventory["numbered_panels"]["built"], 4)
        self.assertEqual(sum(1 for s in self.main0.sockets if s.name.startswith("Panel_")), 4)
        self.assertEqual(inventory["collar_assembly"]["built"], 1)
        self.assertEqual(inventory["collar_assembly"]["segments"], 8)
        self.assertEqual(inventory["base_couplings"]["built"], 2)
        self.assertEqual(inventory["conduits"]["built"], 4)
        self.assertEqual(inventory["rear_paired_conduits"]["built"], 2)
        self.assertEqual(inventory["frame_rails"]["built"], 4)

    def test_panel_order_top_to_bottom(self):
        z = {s.name: s.position[2] for s in self.main0.sockets if s.name.startswith("Panel_")}
        self.assertGreater(z["Panel_01"], z["Panel_02"])
        self.assertGreater(z["Panel_02"], z["Panel_03"])
        self.assertGreater(z["Panel_03"], z["Panel_04"])
        for s in self.main0.sockets:
            if s.name.startswith("Panel_"):
                self.assertAlmostEqual(s.position[0], pl.SHAFT_HALF)  # service face is +X
                self.assertAlmostEqual(s.yaw_deg, 0.0)

    def test_maintenance_bay_spans_panels_02_and_03(self):
        self.assertLess(pl.BAY_Z[0], pl.PANEL_Z["03"] - pl.PANEL_H / 2 + 1)
        self.assertGreater(pl.BAY_Z[1], pl.PANEL_Z["02"] + pl.PANEL_H / 2 - 1)
        self.assertGreater(pl.BAY_Z[0], pl.PANEL_Z["04"] + pl.PANEL_H / 2)
        self.assertLess(pl.BAY_Z[1], pl.PANEL_Z["01"] - pl.PANEL_H / 2)

    def test_conduit_sockets_paired_per_coupling_and_outward(self):
        left = [s for s in self.main0.sockets if s.name.startswith("Conduit_Left_")]
        right = [s for s in self.main0.sockets if s.name.startswith("Conduit_Right_")]
        self.assertEqual(len(left), 2)
        self.assertEqual(len(right), 2)
        for s in left:
            self.assertAlmostEqual(s.position[1], -pl.FOOTPRINT[1] / 2)
            self.assertAlmostEqual(s.yaw_deg, -90.0)
        for s in right:
            self.assertAlmostEqual(s.position[1], pl.FOOTPRINT[1] / 2)
            self.assertAlmostEqual(s.yaw_deg, 90.0)

    # -- budgets and footprint -----------------------------------------------------
    def test_triangle_budgets(self):
        lod0 = self.main0.triangle_count() + 4 * self.panel0.triangle_count() + 4 * self.stub0.triangle_count()
        lod1 = self.main1.triangle_count() + 4 * pl.build_panel(1).triangle_count() + 4 * pl.build_stub(1).triangle_count()
        self.assertLessEqual(lod0, 3500)
        self.assertLessEqual(lod1, 1200)
        self.assertLess(lod1, lod0)

    def test_main_mesh_inside_footprint_and_grounded(self):
        (x0, y0, z0), (x1, y1, z1) = self.main0.bounds()
        self.assertGreaterEqual(x0, -200.0)
        self.assertLessEqual(x1, 200.0)
        self.assertGreaterEqual(y0, -200.0)
        self.assertLessEqual(y1, 200.0)
        self.assertAlmostEqual(z0, 0.0)
        self.assertGreater(z1, 1000.0)

    def test_pivot_is_ground_contact_centre(self):
        (x0, y0, _), (x1, y1, _) = self.main0.component_bounds("plinth")
        self.assertAlmostEqual((x0 + x1) / 2, 0.0)
        self.assertAlmostEqual((y0 + y1) / 2, 0.0)

    def test_material_slots(self):
        self.assertEqual(self.main0.slots, [pl.CERAMIC, pl.FRAME, pl.STATUS])
        self.assertLessEqual(len(self.main0.slots), 3)
        by_slot = self.main0.triangle_count_by("slot")
        self.assertIn(pl.STATUS, by_slot)

    def test_status_slot_is_a_small_fraction(self):
        # Emissive geometry must stay well under the 15% visible-area ceiling; triangles are a proxy here.
        by_slot = self.main0.triangle_count_by("slot")
        self.assertLess(by_slot[pl.STATUS] / self.main0.triangle_count(), 0.15)

    # -- kit geometry sanity --------------------------------------------------------
    def test_normals_point_outward_for_convex_primitives(self):
        m = kit.Mesh("t")
        m.slot("a")
        m.box((0, 0, 0), (2, 2, 2), 0, "box")
        m.prism(kit.octagon(1.0, 0.3), 0.0, 2.0, 0, "prism")
        m.tube((0, 0, 0), (0, 0, 3), 1.0, 8, 0, "tube")
        for poly in m.polygons:
            c = [sum(p[i] for p in poly.points) / len(poly.points) for i in range(3)]
            if poly.component == "tube":
                c = (c[0], c[1], 0.0)
                self.assertGreaterEqual(kit.v_dot(poly.normal, c), -1e-6)
            elif poly.component == "prism":
                self.assertGreater(kit.v_dot(poly.normal, (c[0], c[1], c[2] - 1.0)), 0.0)
            else:
                self.assertGreater(kit.v_dot(poly.normal, c), 0.0)

    def test_pitch_rotation_matches_unreal(self):
        self.assertEqual(tuple(round(v, 6) for v in kit.rot_y((1, 0, 0), 90)), (0.0, 0.0, 1.0))
        self.assertEqual(tuple(round(v, 6) for v in kit.rot_z((1, 0, 0), 90)), (0.0, 1.0, 0.0))

    # -- export ------------------------------------------------------------------------
    def test_glb_round_trip_and_axis_rule(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "m.glb")
            m = kit.Mesh("SM_T")
            m.slot("a")
            m.box((100, 0, 50), (10, 10, 100), 0, "box")
            m.sockets.append(kit.Socket("S", (100, 200, 300), 90.0, "p"))
            m.collision.append(kit.CollisionBox("c", (0, 0, 50), (10, 10, 100)))
            m.write_glb(path)
            doc = kit.read_glb(path)
            names = [n["name"] for n in doc["nodes"]]
            self.assertIn("SOCKET_SM_T_S", names)
            self.assertIn("UBX_SM_T_01", names)
            socket = next(n for n in doc["nodes"] if n["name"] == "SOCKET_SM_T_S")
            # Unreal (100, 200, 300) cm -> glTF (X, Z, Y) / 100 = (1.0, 3.0, 2.0) m
            self.assertEqual(socket["translation"], [1.0, 3.0, 2.0])
            pos = doc["accessors"][doc["meshes"][0]["primitives"][0]["attributes"]["POSITION"]]
            self.assertAlmostEqual(pos["min"][0], 0.95, places=5)
            self.assertAlmostEqual(pos["max"][1], 1.0, places=5)   # Unreal Z 100 cm -> glTF Y 1.0 m
            self.assertAlmostEqual(pos["max"][2], 0.05, places=5)  # Unreal Y 5 cm -> glTF Z 0.05 m
            self.assertEqual(doc["asset"]["copyright"], "Angelis Pseftis")

    def test_glb_front_faces_are_counter_clockwise_in_gltf_frame(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "m.glb")
            m = kit.Mesh("SM_T")
            m.slot("a")
            m.box((0, 0, 50), (10, 10, 100), 0, "box")
            m.write_glb(path)
            with open(path, "rb") as handle:
                handle.seek(12)
                json_len, = struct.unpack("<I", handle.read(4))
                handle.seek(4, 1)
                doc = json.loads(handle.read(json_len))
                handle.seek(8, 1)
                binary = handle.read()
            prim = doc["meshes"][0]["primitives"][0]
            pos_acc = doc["accessors"][prim["attributes"]["POSITION"]]
            nrm_acc = doc["accessors"][prim["attributes"]["NORMAL"]]
            idx_acc = doc["accessors"][prim["indices"]]
            def read(acc, fmt, n):
                view = doc["bufferViews"][acc["bufferView"]]
                out = []
                for i in range(acc["count"]):
                    off = view["byteOffset"] + i * struct.calcsize(fmt)
                    out.append(struct.unpack_from(fmt, binary, off))
                return out
            positions = read(pos_acc, "<3f", 3)
            normals = read(nrm_acc, "<3f", 3)
            indices = [i[0] for i in read(idx_acc, "<I", 1)]
            for t in range(0, len(indices), 3):
                a, b, c = (positions[indices[t + k]] for k in range(3))
                n = normals[indices[t]]
                cross = kit.v_cross(kit.v_sub(b, a), kit.v_sub(c, a))
                self.assertGreater(kit.v_dot(cross, n), 0.0)

    def test_obj_exports_reference_frame_and_groups(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "m.obj")
            self.main0.write_obj(path)
            with open(path, encoding="utf-8") as handle:
                text = handle.read()
            self.assertIn("# Author: Angelis Pseftis", text)
            self.assertIn("g collar_assembly", text)
            self.assertIn("usemtl MI_EBS_MER_StatusCyan", text)

    def test_deterministic_export_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = pl.build_main(0).write_glb(os.path.join(tmp, "a.glb"))
            b = pl.build_main(0).write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)


if __name__ == "__main__":
    unittest.main(verbosity=2)
