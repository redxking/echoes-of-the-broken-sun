#!/usr/bin/env python3
"""Regression checks for the EBS-MER-BLD-002 Power Link source (concept-v3).

Author: Angelis Pseftis. Run: python3 test_power_link_build.py
These checks bind the contract inventory, budgets, footprint, axes, sockets and
export validity to the generator, and the measurable concept-fidelity items
(concept-fidelity.md: proportion ratios, component presence and order,
containment). They are structural checks, not gate acceptance.
"""
from __future__ import annotations

import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import build_power_link as pl  # noqa: E402


class PowerLinkConceptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main0 = pl.build_main(0)
        cls.main1 = pl.build_main(1)
        cls.panel0 = pl.build_panel(0)
        cls.panel1 = pl.build_panel(1)
        cls.stub0 = pl.build_stub(0)
        cls.stub1 = pl.build_stub(1)

    # -- contract inventory ------------------------------------------------------
    def test_revision_is_the_concept_rebuild(self):
        self.assertEqual(pl.REVISION, "ebs-mer-bld-002-concept-v3")

    def test_contract_component_counts(self):
        inventory = pl.contract_inventory(self.main0)
        self.assertEqual(inventory["numbered_panels"]["built"], 4)
        self.assertEqual(sum(1 for s in self.main0.sockets if s.name.startswith("Panel_")), 4)
        self.assertEqual(inventory["collar_assembly"]["built"], 1)
        self.assertEqual(inventory["collar_assembly"]["segments"], 4)   # one cyan strip per wide face
        self.assertEqual(inventory["base_couplings"]["built"], 2)
        self.assertEqual(inventory["conduits"]["built"], 4)
        self.assertEqual(inventory["frame_rails"]["built"], 4)
        self.assertEqual(inventory["frame_brackets"]["built"], 4)

    def test_socket_names_unchanged_from_the_contract(self):
        names = sorted(s.name for s in self.main0.sockets)
        self.assertEqual(names, sorted(["Panel_01", "Panel_02", "Panel_03", "Panel_04", "Conduit_Left_01", "Conduit_Left_02",
                                        "Conduit_Right_01", "Conduit_Right_02", "Collar_Center", "Cap_Top", "Bay_Center"]))
        self.assertEqual([s.name for s in self.panel0.sockets], ["Label"])
        self.assertEqual([s.name for s in self.stub0.sockets], ["Span_End"])

    def test_panel_order_top_to_bottom_with_01_above_the_collar(self):
        z = {s.name: s.position[2] for s in self.main0.sockets if s.name.startswith("Panel_")}
        self.assertGreater(z["Panel_01"], z["Panel_02"])
        self.assertGreater(z["Panel_02"], z["Panel_03"])
        self.assertGreater(z["Panel_03"], z["Panel_04"])
        # fidelity §3: 01 above the collar, 02-04 below it, plates clear of the collar band
        self.assertGreater(z["Panel_01"] - pl.PANEL_H / 2.0, pl.COLLAR_Z[1])
        self.assertLess(z["Panel_02"] + pl.PANEL_H / 2.0, pl.COLLAR_Z[0])
        self.assertGreater(z["Panel_04"] - pl.PANEL_H / 2.0, pl.STEP_TOP)
        self.assertLess(z["Panel_01"] + pl.PANEL_H / 2.0, pl.SHAFT_TOP)
        for s in self.main0.sockets:
            if s.name.startswith("Panel_"):
                self.assertAlmostEqual(s.position[0], pl.SHAFT_HALF)  # service face is +X
                self.assertAlmostEqual(s.yaw_deg, 0.0)

    def test_panels_do_not_overlap_each_other_and_cover_the_shaft(self):
        zs = sorted(pl.PANEL_Z.values())
        for lower, upper in zip(zs, zs[1:]):
            self.assertGreater(upper - lower, pl.PANEL_H)
        # candidate: four plates fill the shaft with thin seams; bare shaft (outside the collar, team
        # band and seams) stays under 10 % of the shaft height
        shaft = pl.SHAFT_TOP - pl.SHAFT_Z0
        covered = 4 * pl.PANEL_H + (pl.COLLAR_Z[1] - pl.COLLAR_Z[0]) + (pl.TEAM_BAND_Z[1] - pl.TEAM_BAND_Z[0])
        self.assertLess((shaft - covered) / shaft, 0.10)

    def test_maintenance_bay_spans_panels_02_and_03(self):
        self.assertLess(pl.BAY_Z[0], pl.PANEL_Z["03"] - pl.PANEL_H / 2 + 1)
        self.assertGreater(pl.BAY_Z[1], pl.PANEL_Z["02"] + pl.PANEL_H / 2 - 1)
        self.assertGreater(pl.BAY_Z[0], pl.PANEL_Z["04"] + pl.PANEL_H / 2)
        self.assertLess(pl.BAY_Z[1], pl.PANEL_Z["01"] - pl.PANEL_H / 2)
        self.assertEqual(pl.MAINTENANCE_REMOVED, ("02", "03"))

    def test_conduit_sockets_paired_per_coupling_and_outward(self):
        left = [s for s in self.main0.sockets if s.name.startswith("Conduit_Left_")]
        right = [s for s in self.main0.sockets if s.name.startswith("Conduit_Right_")]
        self.assertEqual(len(left), 2)
        self.assertEqual(len(right), 2)
        for s in left:
            self.assertAlmostEqual(s.position[1], -pl.PORT_Y)
            self.assertAlmostEqual(s.yaw_deg, -90.0)
            self.assertGreater(s.position[0], 0.0)   # front-left corner of the plinth
        for s in right:
            self.assertAlmostEqual(s.position[1], pl.PORT_Y)
            self.assertAlmostEqual(s.yaw_deg, 90.0)
            self.assertGreater(s.position[0], 0.0)   # front-right corner of the plinth

    def test_conduit_stub_ends_on_the_footprint_edge(self):
        span_end = next(s for s in self.stub0.sockets if s.name == "Span_End").position[0]
        self.assertAlmostEqual(pl.PORT_Y + span_end, pl.FOOTPRINT[1] / 2.0)
        (_, _, _), (x1, _, _) = self.stub0.bounds()
        self.assertLessEqual(x1, span_end + 1e-6)

    # -- concept proportions (concept-fidelity.md) ---------------------------------------
    def test_height_to_shaft_width_ratio(self):
        # concept-fidelity.md §Proportion 1 (amended 2026-09-07): H = 3.8-4.0 W, measured band 3.5-4.2 W
        (_, _, z0), (_, _, z1) = self.main0.bounds()
        self.assertAlmostEqual(z0, 0.0)
        self.assertAlmostEqual(z1, pl.HEIGHT)
        ratio = z1 / pl.SHAFT_W
        self.assertGreaterEqual(ratio, 3.8)
        self.assertLessEqual(ratio, 4.0)
        self.assertAlmostEqual(2 * pl.PLINTH_HALF / pl.SHAFT_W, 1.8, delta=0.05)   # plinth 1.8 W as measured

    def test_shaft_section_is_an_octagon_with_wide_and_narrow_faces(self):
        outline = pl.outline_octagon(pl.SHAFT_HALF, pl.SHAFT_CHAMFER)
        self.assertEqual(len(outline), 8)
        wide = 2 * (pl.SHAFT_HALF - pl.SHAFT_CHAMFER)
        narrow = pl.SHAFT_CHAMFER * 2 ** 0.5
        self.assertGreater(wide, 2 * narrow)      # chamfers read as the narrow faces
        self.assertGreater(narrow, 0.2 * wide)    # but heavy enough to read as an octagon
        self.assertAlmostEqual(2 * pl.SHAFT_HALF, pl.SHAFT_W)

    def test_collar_is_one_broad_band_at_0_72_H(self):
        centre = (pl.COLLAR_Z[0] + pl.COLLAR_Z[1]) / 2.0 / pl.HEIGHT
        self.assertGreaterEqual(centre, 0.70)
        self.assertLessEqual(centre, 0.74)
        self.assertAlmostEqual((pl.COLLAR_Z[1] - pl.COLLAR_Z[0]) / pl.HEIGHT, 0.07, delta=0.015)
        self.assertAlmostEqual(2 * pl.COLLAR_HALF / pl.SHAFT_W, 1.35, delta=0.1)
        (x0, y0, z0), (x1, y1, z1) = self.main0.component_bounds("collar_assembly")
        self.assertAlmostEqual(z0, pl.COLLAR_Z[0])
        self.assertAlmostEqual(z1, pl.COLLAR_Z[1])
        self.assertAlmostEqual(x1 - x0, 2 * pl.COLLAR_HALF)
        self.assertEqual(len(pl.COLLAR_COURSES), 3)
        self.assertEqual(len(pl.COLLAR_GROOVES), 2)
        collar_socket = next(s for s in self.main0.sockets if s.name == "Collar_Center")
        self.assertAlmostEqual(collar_socket.position[2], (pl.COLLAR_Z[0] + pl.COLLAR_Z[1]) / 2.0)

    def test_four_cyan_strips_sit_on_the_wide_faces_inside_the_collar_band(self):
        face = 2 * (pl.COLLAR_HALF - pl.COLLAR_CHAMFER)
        self.assertGreaterEqual(pl.STRIP[1] / face, 0.5)   # candidate: the strip spans ~0.55-0.65 of its collar face
        self.assertLessEqual(pl.STRIP[1] / face, 0.7)
        middle = pl.COLLAR_COURSES[1]
        for k in range(1, 5):
            b = self.main0.component_bounds(f"collar_segment_{k:02d}")
            self.assertIsNotNone(b)
            (x0, y0, z0), (x1, y1, z1) = b
            self.assertGreater(z0, pl.COLLAR_Z[0])
            self.assertLess(z1, pl.COLLAR_Z[1])
            self.assertGreaterEqual(z0, middle[0])   # on the middle course, clear of the grooves
            self.assertLessEqual(z1, middle[1])
            # each strip is centred on one axis (a wide face), never on a chamfer
            cx, cy = (x0 + x1) / 2.0, (y0 + y1) / 2.0
            self.assertTrue(abs(cx) < 1.0 or abs(cy) < 1.0)
            self.assertGreater(max(abs(cx), abs(cy)), pl.COLLAR_HALF)

    def test_panels_are_near_square_plates_that_fit_the_wide_face(self):
        self.assertAlmostEqual(pl.PANEL_H / pl.HEIGHT, 0.19, delta=0.02)
        self.assertLess(pl.PANEL_W, 2 * (pl.SHAFT_HALF - pl.SHAFT_CHAMFER))
        self.assertGreaterEqual(pl.PANEL_W / pl.PANEL_H, 0.85)   # candidate plates read near-square (w/h ~1.0-1.15)
        self.assertGreaterEqual(pl.PANEL_W / pl.SHAFT_W, 0.58)   # and span ~0.6 W
        (x0, y0, z0), (x1, y1, z1) = self.panel0.bounds()
        self.assertAlmostEqual(y1 - y0, pl.PANEL_W)
        self.assertAlmostEqual(z1 - z0, pl.PANEL_H)
        self.assertAlmostEqual(x0, 0.0)
        self.assertGreaterEqual(x1, pl.PANEL_T)
        comps = self.panel0.components()
        self.assertIn("panel_fasteners", comps)
        self.assertIn("panel_label", comps)
        self.assertEqual(sum(1 for p in self.panel0.polygons if p.atlas_cells == 4), 1)   # one numeral cell strip

    def test_cap_is_a_low_plate_with_a_square_fastener_plate_and_no_crystal(self):
        (x0, y0, z0), (x1, y1, z1) = self.main0.component_bounds("cap")
        rail_w = pl.RAIL[1]
        self.assertLess(rail_w / (pl.SHAFT_CHAMFER * 2 ** 0.5), 0.4)   # the pale chamfer shows either side of the rib
        self.assertAlmostEqual(z1, pl.HEIGHT)
        self.assertLess(z1 - z0, 0.05 * pl.HEIGHT)
        self.assertLess(x1 - x0, 1.2 * pl.SHAFT_W)
        self.assertGreater(x1 - x0, pl.SHAFT_W)
        cap_socket = next(s for s in self.main0.sockets if s.name == "Cap_Top")
        self.assertAlmostEqual(cap_socket.position[2], pl.HEIGHT)
        for comp in self.main0.components():
            self.assertNotIn("crystal", comp)
            self.assertNotIn("antenna", comp)

    def test_two_step_plinth_about_0_10_H_with_couplings_at_the_front_corners(self):
        (x0, y0, z0), (x1, y1, z1) = self.main0.component_bounds("plinth")
        self.assertAlmostEqual(x1 - x0, 2 * pl.PLINTH_HALF)
        self.assertAlmostEqual(z0, 0.0)
        (sx0, sy0, sz0), (sx1, sy1, sz1) = self.main0.component_bounds("plinth_step")
        self.assertLess(sx1 - sx0, x1 - x0)
        self.assertAlmostEqual(sz1, pl.STEP_TOP)
        self.assertAlmostEqual(pl.STEP_TOP / pl.HEIGHT, 0.10, delta=0.02)
        self.assertAlmostEqual(2 * pl.STEP_HALF / pl.SHAFT_W, 1.3, delta=0.05)   # upper step 1.3 W as measured
        for side, sign in (("left", -1.0), ("right", 1.0)):
            (cx0, cy0, cz0), (cx1, cy1, cz1) = self.main0.component_bounds(f"coupling_{side}")
            self.assertGreater(cx0, 0.0)                         # front half of the plinth
            self.assertLessEqual(cx1, pl.FOOTPRINT[0] / 2.0)     # never past the footprint edge
            outer = cy0 if sign < 0 else cy1
            self.assertAlmostEqual(abs(outer), pl.COUPLING_Y_OUT, delta=pl.LAMP[1] + 0.5)   # block face at 140 (+4 cm status lamp)
            self.assertEqual(outer * sign > 0, True)
            self.assertAlmostEqual(cz0, pl.PLINTH_TOP)            # sits on the lower step
            self.assertAlmostEqual(pl.COUPLING[0] / pl.SHAFT_W, 0.9, delta=0.02)
            self.assertAlmostEqual(pl.COUPLING[2] / pl.SHAFT_W, 0.5, delta=0.02)
            # depth: bounded by the stub (outer face at 140) and the panel-04 clearance (README §8.1 D8)
            inner = cy1 if sign < 0 else cy0
            self.assertGreaterEqual(abs(inner), pl.PANEL_W / 2.0 + 3.0)
            self.assertGreater(cz1, pl.STEP_TOP)                 # the block stands proud of the upper step

    def test_coupling_overhang_measured_against_the_plinth_octagon(self):
        # The block hangs past the plinth-top outline at the front (candidate: outboard feet), bounded
        # by the 400 cm footprint; the outer corner overhang is measured against the chamfered outline,
        # not only along X.
        over = pl.coupling_overhang()
        x1 = pl.COUPLING_X + pl.COUPLING[0] / 2.0
        self.assertGreater(over["x_axis_past_plinth_top_edge"], 0.0)
        self.assertLessEqual(x1, pl.FOOTPRINT[0] / 2.0)
        self.assertAlmostEqual(over["outer_corner_past_plinth_top_chamfer"],
                               pl.octagon_overhang(x1, pl.COUPLING_Y_OUT, pl.SKIRT_TOP_HALF, pl.SKIRT_TOP_CHAMFER), places=1)
        self.assertLess(over["outer_corner_past_plinth_ground_chamfer"], 0.15 * pl.SHAFT_W)   # a foot, not a cantilevered wing
        self.assertEqual(pl.octagon_overhang(0.0, 0.0, 10.0, 2.0), -10.0)
        self.assertAlmostEqual(pl.octagon_overhang(10.0, 10.0, 10.0, 2.0), 2.0 / 2 ** 0.5)

    def test_coupling_blocks_are_closed_boxes_with_ports_inside_the_face_and_clear_of_the_lamp(self):
        for side, sign in (("left", -1.0), ("right", 1.0)):
            polys = [p for p in self.main0.polygons if p.component == f"coupling_{side}"]
            underside = [p for p in polys if p.normal[2] < -0.999 and abs(p.points[0][2] - pl.COUPLING_Z[0]) < 1e-6]
            self.assertEqual(len(underside), 1)   # closed underside at the lower-step level
            # port collars (Ø 2 PORT_R at PORT_Z) inside the outer face, clear of the lamp above them
            self.assertGreater(pl.PORT_Z - pl.PORT_R, pl.COUPLING_Z[0])
            self.assertLess(pl.PORT_Z + pl.PORT_R, pl.LAMP_Z - pl.LAMP[2] / 2.0)
            self.assertLess(pl.LAMP_Z + pl.LAMP[2] / 2.0, pl.COUPLING_Z[1])
            for k in (1, 2):
                (px0, py0, pz0), (px1, py1, pz1) = self.main0.component_bounds(f"coupling_{side}_port_{k:02d}")
                self.assertGreater(px0, pl.COUPLING_X - pl.COUPLING[0] / 2.0)
                self.assertLess(px1, pl.COUPLING_X + pl.COUPLING[0] / 2.0)
                self.assertGreater(pz0, pl.COUPLING_Z[0])
                self.assertLess(pz1, pl.LAMP_Z - pl.LAMP[2] / 2.0)
                self.assertAlmostEqual(max(abs(py0), abs(py1)), pl.PORT_Y)   # cap 2 cm proud of the face

    def test_conduit_diameter_0_16_W_as_measured(self):
        # concept-fidelity.md §Proportion 6 (amended 2026-09-07 from the candidate pixels): Ø 0.15-0.18 W,
        # target 0.16 W; the port collar must be wider than the conduit it receives.
        self.assertAlmostEqual(2 * pl.STUB_R / pl.SHAFT_W, 0.16, delta=0.015)
        self.assertGreater(pl.PORT_R, pl.STUB_R)
        self.assertLess(pl.PORT_R, pl.COUPLING[2] / 2.0)   # the port stays inside the coupling face

    def test_conduit_stub_is_segmented_with_pale_clamp_rings_and_a_pulse_window(self):
        rings = [p for p in self.stub0.polygons if p.component == "conduit_clamp_ring"]
        xs = sorted({round(q[0], 3) for p in rings for q in p.points})
        self.assertEqual(xs, sorted({x for pair in pl.STUB_RINGS for x in pair}))
        (px0, _, _), (px1, _, _) = self.stub0.component_bounds("conduit_pulse_strip")
        for x0, x1 in pl.STUB_RINGS:
            self.assertTrue(px1 <= x0 or px0 >= x1)   # the pulse window sits between the rings, not under one
        (nx0, _, _), (nx1, _, _) = self.stub0.component_bounds("conduit_end_nut")
        self.assertAlmostEqual(nx1, pl.STUB_LENGTH + 2.0)
        self.assertEqual(self.stub1.components().count("conduit_clamp_ring"), 0)   # LOD1 drops the rings
        # rings and end nut are pale ceramic insulators so the segmentation and an unplugged stub separate
        # from the charcoal plinth; the jacket stays charcoal
        for mesh in (self.stub0, self.stub1):
            for poly in mesh.polygons:
                if poly.component in ("conduit_clamp_ring", "conduit_end_nut"):
                    self.assertEqual(mesh.slots[poly.slot], pl.CERAMIC)
                elif poly.component == "conduit_jacket":
                    self.assertEqual(mesh.slots[poly.slot], pl.FRAME)
            self.assertLessEqual(len(mesh.slots), 3)

    def test_numeral_cell_is_centred_and_large_enough_to_read(self):
        (x0, y0, z0), (x1, y1, z1) = self.panel0.component_bounds("panel_label")
        self.assertAlmostEqual((y0 + y1) / 2.0, 0.0)
        self.assertAlmostEqual((z0 + z1) / 2.0, pl.LABEL_Z)
        self.assertGreaterEqual((y1 - y0) / pl.PANEL_W, 0.4)    # candidate numerals span ~0.35-0.4 of the plate width
        self.assertGreaterEqual((z1 - z0) / pl.PANEL_H, 0.25)   # and ~0.25-0.3 of its height
        self.assertLess(y1, pl.PANEL_W / 2.0 - 14.0)             # clear of the corner fastener bosses
        self.assertLess(z1, pl.PANEL_H / 2.0 - 14.0)
        label = next(s for s in self.panel0.sockets if s.name == "Label")
        self.assertAlmostEqual(label.position[1], 0.0)
        self.assertAlmostEqual(label.position[2], pl.LABEL_Z)
        # the export keeps the cell on the FRAME slot (the baker paints the numerals under compact_metal)
        for poly in self.panel0.polygons:
            if poly.component == "panel_label":
                self.assertEqual(self.panel0.slots[poly.slot], pl.FRAME)

    def test_review_assembly_remaps_only_the_label_cells_to_the_review_material(self):
        scene = pl.assemble(0, "connected")
        self.assertIn(pl.REVIEW_LABEL, scene.slots)
        review = scene.slots.index(pl.REVIEW_LABEL)
        for poly in scene.polygons:
            self.assertEqual(poly.component.endswith("panel_label"), poly.slot == review)
        self.assertEqual(sum(1 for p in scene.polygons if p.slot == review), 4 * sum(1 for p in self.panel0.polygons if p.component == "panel_label"))
        self.assertNotIn(pl.REVIEW_LABEL, self.main0.slots + self.panel0.slots + self.stub0.slots)

    def test_service_bay_walls_face_into_the_cavity_and_the_bundle_has_pale_fittings(self):
        polys = [p for p in self.main0.polygons if p.component == "service_bay"]
        self.assertEqual(len(polys), 5)
        back = [p for p in polys if abs(p.normal[0] - 1.0) < 1e-6]
        self.assertEqual(len(back), 1)
        self.assertAlmostEqual(back[0].points[0][0], pl.SHAFT_HALF - pl.BAY_DEPTH)
        for p in polys:
            centre = [sum(q[i] for q in p.points) / len(p.points) for i in range(3)]
            cavity = (pl.SHAFT_HALF - pl.BAY_DEPTH / 2.0, 0.0, (pl.BAY_Z[0] + pl.BAY_Z[1]) / 2.0)
            self.assertGreater(kit.v_dot(p.normal, kit.v_sub(cavity, centre)), 0.0)
        fittings = [p for p in self.main0.polygons if p.component == "service_bay_fittings"]
        self.assertTrue(fittings)
        for p in fittings:
            self.assertEqual(self.main0.slots[p.slot], pl.CERAMIC)
            (fx0, _, fz0), (fx1, _, fz1) = self.main0.component_bounds("service_bay_fittings")
        self.assertGreater(fx0, pl.SHAFT_HALF - pl.BAY_DEPTH)
        self.assertLess(fx1, pl.SHAFT_HALF)
        self.assertGreater(fz0, pl.BAY_Z[0])
        self.assertLess(fz1, pl.BAY_Z[1])

    def test_team_band_sits_just_under_the_collar(self):
        (x0, y0, z0), (x1, y1, z1) = self.main0.component_bounds("team_band")
        self.assertLess(z1, pl.COLLAR_Z[0])
        self.assertGreater(z0, pl.PANEL_Z["02"] + pl.PANEL_H / 2.0)
        self.assertGreater(x1 - x0, pl.SHAFT_W)

    # -- budgets and footprint -----------------------------------------------------
    def test_triangle_budgets(self):
        lod0 = self.main0.triangle_count() + 4 * self.panel0.triangle_count() + 4 * self.stub0.triangle_count()
        lod1 = self.main1.triangle_count() + 4 * self.panel1.triangle_count() + 4 * self.stub1.triangle_count()
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
        self.assertAlmostEqual(z1, pl.HEIGHT)
        self.assertAlmostEqual(y1 - y0, 2 * pl.PLINTH_HALF)   # the plinth announces the footprint on Y
        self.assertAlmostEqual(x0, -pl.PLINTH_HALF)           # and on -X; +X carries the coupling feet
        self.assertAlmostEqual(x1, pl.COUPLING_X + pl.COUPLING[0] / 2.0)

    def test_assembled_states_stay_inside_the_footprint_except_ground_debris(self):
        connected = pl.assemble(0, "connected")
        (x0, y0, _), (x1, y1, _) = connected.bounds()
        self.assertGreaterEqual(min(x0, y0), -200.0 - 1e-6)
        self.assertLessEqual(max(x1, y1), 200.0 + 1e-6)
        maintenance = pl.assemble(0, "maintenance")
        comps = maintenance.components()
        self.assertTrue(any(c.startswith("panel_02_") for c in comps))
        self.assertTrue(any(c.startswith("panel_03_") for c in comps))
        self.assertTrue(any(c.startswith("panel_01_") for c in comps))
        self.assertTrue(any(c.startswith("conduit_right_02_hanging_") for c in comps))
        (_, _, z0), (_, _, _) = maintenance.component_bounds("panel_02_panel_plate")
        self.assertGreaterEqual(z0, 0.0)
        (_, _, z0), (_, _, z1) = maintenance.component_bounds("panel_03_panel_plate")
        self.assertGreaterEqual(z0, 0.0)
        self.assertLess(z1, 20.0)   # lying flat on the ground
        # the shed panels are cosmetic debris outside the footprint (README §8.1 D11); everything that
        # belongs to the structure, the hanging conduit included, stays inside it
        for poly in maintenance.polygons:
            if poly.component.startswith(("panel_02_", "panel_03_")):
                continue
            for p in poly.points:
                self.assertLessEqual(max(abs(p[0]), abs(p[1])), 200.0 + 1e-6, poly.component)
                self.assertGreaterEqual(p[2], -1e-6, poly.component)

    def test_hanging_conduit_clears_the_plinth_and_reaches_the_ground(self):
        maintenance = pl.assemble(0, "maintenance")
        prefix = pl.MAINTENANCE_HANGING.lower() + "_hanging_"
        (_, _, hz0), (_, _, hz1) = maintenance.component_bounds(prefix + "conduit_jacket")
        self.assertLess(hz0, pl.PORT_Z - 30.0)   # droops well below its port
        (_, _, nz0), (_, _, _) = maintenance.component_bounds(prefix + "span_conduit_end_nut")
        self.assertLess(nz0, 8.0)                 # the chained span's end nut rests at the ground
        self.assertGreaterEqual(nz0, 0.0)
        # nothing of the hanging cable is inside the lower step (z < 50 inside the plinth-top outline) or
        # under the skirt slope (the concept-v2 single stub at -70 deg ended at z 30 inside the step)

        def surface_z(x, y):
            top = pl.octagon_overhang(x, y, pl.SKIRT_TOP_HALF, pl.SKIRT_TOP_CHAMFER)
            ground = pl.octagon_overhang(x, y, pl.PLINTH_HALF, pl.PLINTH_CHAMFER)
            if top <= 0.0:
                return pl.PLINTH_TOP
            if ground >= 0.0:
                return 0.0
            return pl.SKIRT_TOP_Z * (-ground) / (top - ground)
        for poly in maintenance.polygons:
            if not poly.component.startswith(prefix):
                continue
            for p in poly.points:
                self.assertGreaterEqual(p[2] - surface_z(p[0], p[1]), -1e-6, (poly.component, p))
        # the chain starts where the hanging stub ends (Span_End of the first instance)
        (sx0, sy0, sz0), (sx1, sy1, sz1) = maintenance.component_bounds(prefix + "span_conduit_jacket")
        (jx0, jy0, jz0), (jx1, jy1, jz1) = maintenance.component_bounds(prefix + "conduit_jacket")
        self.assertLess(sz1, jz1)
        self.assertGreater(sz1, jz0)

    def test_pivot_is_ground_contact_centre(self):
        (x0, y0, _), (x1, y1, _) = self.main0.component_bounds("plinth")
        self.assertAlmostEqual((x0 + x1) / 2, 0.0)
        self.assertAlmostEqual((y0 + y1) / 2, 0.0)

    def test_material_slots(self):
        self.assertEqual(self.main0.slots, [pl.CERAMIC, pl.FRAME, pl.STATUS])
        self.assertLessEqual(len(self.main0.slots), 3)
        self.assertEqual(self.stub0.slots, [pl.FRAME, pl.STATUS, pl.CERAMIC])
        by_slot = self.main0.triangle_count_by("slot")
        self.assertIn(pl.STATUS, by_slot)

    def test_status_slot_is_a_small_fraction(self):
        # Emissive geometry must stay well under the 15% visible-area ceiling; triangles are a proxy here,
        # the pixel measurement lives in renders/area_check/emissive-area.json.
        by_slot = self.main0.triangle_count_by("slot")
        self.assertLess(by_slot[pl.STATUS] / self.main0.triangle_count(), 0.15)

    # -- kit geometry sanity --------------------------------------------------------
    def test_normals_point_outward_for_convex_primitives(self):
        m = kit.Mesh("t")
        m.slot("a")
        m.box((0, 0, 0), (2, 2, 2), 0, "box")
        m.prism(kit.octagon(1.0, 0.3), 0.0, 2.0, 0, "prism")
        m.tube((0, 0, 0), (0, 0, 3), 1.0, 8, 0, "tube")
        pl.add_frustum(m, kit.octagon(2.0, 0.5), 0.0, kit.octagon(1.5, 0.4), 1.0, 0, "frustum")
        for poly in m.polygons:
            c = [sum(p[i] for p in poly.points) / len(poly.points) for i in range(3)]
            if poly.component == "tube":
                c = (c[0], c[1], 0.0)
                self.assertGreaterEqual(kit.v_dot(poly.normal, c), -1e-6)
            elif poly.component == "prism":
                self.assertGreater(kit.v_dot(poly.normal, (c[0], c[1], c[2] - 1.0)), 0.0)
            elif poly.component == "frustum":
                self.assertGreater(kit.v_dot(poly.normal, (c[0], c[1], 0.0)), 0.0)
                self.assertGreater(poly.normal[2], 0.0)   # the skirt slopes inward going up
            else:
                self.assertGreater(kit.v_dot(poly.normal, c), 0.0)

    def test_frustum_and_stud_polygons_are_planar(self):
        m = kit.Mesh("t")
        m.slot("a")
        pl.add_frustum(m, kit.octagon(pl.PLINTH_HALF, pl.PLINTH_CHAMFER), 0.0, kit.octagon(pl.SKIRT_TOP_HALF, pl.SKIRT_TOP_CHAMFER), pl.SKIRT_TOP_Z, 0, "f")
        pl.add_stud(m, (0.0, 0.0, 0.0), 4.0, 1.5, 6, 0, "s")
        for poly in m.polygons:
            n = poly.normal
            d = kit.v_dot(n, poly.points[0])
            for p in poly.points[1:]:
                self.assertAlmostEqual(kit.v_dot(n, p), d, places=6)

    def test_no_coincident_opposed_faces_between_parts_and_frame(self):
        # Every component pair that shares a plane must not present two faces on the same plane
        # facing the same way (z-fighting). Checked on axis-aligned faces of the main mesh.
        seen = {}
        for poly in self.main0.polygons:
            n = tuple(round(c, 4) for c in poly.normal)
            if sum(1 for c in n if abs(c) > 0.999) != 1:
                continue
            axis = max(range(3), key=lambda i: abs(n[i]))
            key = (axis, n[axis] > 0, round(poly.points[0][axis], 3))
            box = [[min(p[i] for p in poly.points), max(p[i] for p in poly.points)] for i in range(3)]
            for other_comp, other_box in seen.get(key, []):
                if other_comp == poly.component:
                    continue
                overlap = all(box[i][0] < other_box[i][1] - 0.5 and other_box[i][0] < box[i][1] - 0.5 for i in range(3) if i != axis)
                self.assertFalse(overlap, f"coplanar same-facing faces: {poly.component} vs {other_comp} at {key}")
            seen.setdefault(key, []).append((poly.component, box))

    def test_panel_04_clears_the_coupling_blocks(self):
        # the plate (|y| <= PANEL_W/2, x from the face) must not run into the coupling boxes beside it
        (px0, py0, pz0), (px1, py1, pz1) = self.panel0.bounds()
        panel_y = pl.PANEL_W / 2.0
        for side in ("left", "right"):
            (cx0, cy0, cz0), (cx1, cy1, cz1) = self.main0.component_bounds(f"coupling_{side}")
            inner = min(abs(cy0), abs(cy1))
            z_overlap = pl.PANEL_Z["04"] - pl.PANEL_H / 2.0 < cz1
            self.assertTrue(inner > panel_y or not z_overlap)

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
            self.assertIn("SOCKET_S", names)
            mesh_node = doc["nodes"][0]
            self.assertIn(names.index("SOCKET_S"), mesh_node["children"])
            self.assertIn("UBX_SM_T_01", names)
            socket = next(n for n in doc["nodes"] if n["name"] == "SOCKET_S")
            # Unreal (100, 200, 300) cm -> glTF (X, Z, Y) / 100 = (1.0, 3.0, 2.0) m
            self.assertEqual(socket["translation"], [1.0, 3.0, 2.0])
            self.assertEqual(socket["scale"], [-1.0, 1.0, 1.0])
            # Encoding established by the 5.8.2 probe imports: q_y(-90) * (q_y(180) * q_x(-90))
            expect = kit.quat_mul(kit.q_axis(1, -90.0), kit.quat_mul(kit.q_axis(1, 180.0), kit.q_axis(0, -90.0)))
            for got, want in zip(socket["rotation"], expect):
                self.assertAlmostEqual(got, want, places=4)
            pos = doc["accessors"][doc["meshes"][0]["primitives"][0]["attributes"]["POSITION"]]
            self.assertAlmostEqual(pos["min"][0], 0.95, places=5)
            self.assertAlmostEqual(pos["max"][1], 1.0, places=5)   # Unreal Z 100 cm -> glTF Y 1.0 m
            self.assertAlmostEqual(pos["max"][2], 0.05, places=5)  # Unreal Y 5 cm -> glTF Z 0.05 m
            self.assertEqual(doc["asset"]["copyright"], "Angelis Pseftis")

    def test_main_glb_carries_all_sockets_and_collision_only_in_lod0(self):
        with tempfile.TemporaryDirectory() as tmp:
            for lod, mesh in ((0, self.main0), (1, self.main1)):
                path = os.path.join(tmp, f"m{lod}.glb")
                mesh.write_glb(path, include_collision=(lod == 0))
                doc = kit.read_glb(path)
                names = [n["name"] for n in doc["nodes"]]
                for s in mesh.sockets:
                    self.assertIn(f"SOCKET_{s.name}", names)
                ubx = [n for n in names if n.startswith("UBX_")]
                self.assertEqual(len(ubx), 2 if lod == 0 else 0)
                self.assertEqual(len(doc["meshes"][0]["primitives"]), 3)   # ceramic / frame / status sections

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
            self.assertIn("g service_bay", text)
            self.assertIn("usemtl MI_EBS_MER_StatusCyan", text)
            self.assertNotIn(pl.REVIEW_LABEL, text)   # review-only material never reaches an export

    def test_deterministic_export_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = pl.build_main(0).write_glb(os.path.join(tmp, "a.glb"))
            b = pl.build_main(0).write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)
            c = pl.assemble(0, "maintenance").write_obj(os.path.join(tmp, "c.obj"))
            d = pl.assemble(0, "maintenance").write_obj(os.path.join(tmp, "d.obj"))
            self.assertEqual(c, d)

    def test_generate_is_self_consistent_and_check_mode_writes_nothing(self):
        # A build into a scratch directory reproduces the hashes the --check comparison relies on, and
        # --check itself never touches the package exports, the bake manifest or the evidence directory.
        with tempfile.TemporaryDirectory() as tmp:
            manifest = pl.generate(os.path.join(tmp, "export"), os.path.join(tmp, "review"), os.path.join(tmp, "bake-manifest.json"))
            again = pl.generate(os.path.join(tmp, "export2"), os.path.join(tmp, "review2"), os.path.join(tmp, "bake-manifest2.json"))
            self.assertEqual(pl.compare_manifests(again, manifest), [])
            broken = json.loads(json.dumps(manifest))
            broken["outputs"] = broken["outputs"][:-1]
            broken["review_assemblies"][0]["sha256"] = "0" * 64
            broken["uv_atlas"]["sha256"] = "0" * 64
            drift = pl.compare_manifests(again, broken)
            self.assertTrue(any(d.startswith("outputs:") for d in drift))
            self.assertTrue(any(d.startswith("review/") for d in drift))
            self.assertIn("bake-manifest.json", drift)
            self.assertIn(manifest["outputs"][-1]["path"], drift)
            evidence = os.path.join(tmp, "evidence")
            os.makedirs(evidence)
            export_dir = os.path.join(HERE, "export")
            before = {name: os.stat(os.path.join(export_dir, name)).st_mtime_ns for name in os.listdir(export_dir)} if os.path.isdir(export_dir) else {}
            bake = os.path.join(HERE, "bake-manifest.json")
            bake_before = os.stat(bake).st_mtime_ns if os.path.exists(bake) else None
            result = subprocess.run([sys.executable, os.path.join(HERE, "build_power_link.py"), "--evidence-dir", evidence, "--check"],
                                    capture_output=True, text=True, check=False)
            self.assertIn(result.returncode, (0, 2, 3), result.stderr)
            self.assertEqual(os.listdir(evidence), [])
            after = {name: os.stat(os.path.join(export_dir, name)).st_mtime_ns for name in os.listdir(export_dir)} if os.path.isdir(export_dir) else {}
            self.assertEqual(before, after)
            self.assertEqual(bake_before, os.stat(bake).st_mtime_ns if os.path.exists(bake) else None)

    def test_bake_manifest_component_names_keep_the_baker_rules(self):
        comps = set(self.main0.components()) | set(self.panel0.components()) | set(self.stub0.components())
        for needed in ("panel_plate", "panel_label", "conduit_pulse_strip", "team_band", "coupling_left", "coupling_right"):
            self.assertIn(needed, comps)
        self.assertEqual(sorted(c for c in comps if c.startswith("collar_segment_")), [f"collar_segment_{k:02d}" for k in range(1, 5)])


if __name__ == "__main__":
    unittest.main(verbosity=2)
