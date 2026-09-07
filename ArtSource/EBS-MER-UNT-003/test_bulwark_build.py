#!/usr/bin/env python3
"""Regression checks for the EBS-MER-UNT-003 Bulwark Team concept-v3 source.

Author: Angelis Pseftis.
Usage: python3 -m unittest discover -s ArtSource/EBS-MER-UNT-003 -p 'test_*.py'

Structural checks against concept-fidelity.md, the owner-corrected reference measurements carried
in build_bulwark.MEASURED_PX, and the bounding rules: contract inventory (every concept part
present), the measured proportion ratios, socket names and positions, LOD budgets, material slots,
ground contact for every clip sample, both state assertions (deployed face flat and frontal, rear
open, packed silhouette wider than tall with the frontal face lost, six equal framed cells three
per wing) and byte-level determinism. No renders and no engine are involved.
"""
from __future__ import annotations

import hashlib
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
import ebs_skelkit as skel  # noqa: E402
import build_bulwark as bw  # noqa: E402

EVIDENCE_DIR = ("/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/"
                "asset-production-20260906T221157Z/EBS-MER-UNT-003")


class ContractInventory(unittest.TestCase):
    """Every part the concept and the card name is present, with the right multiplicity."""

    @classmethod
    def setUpClass(cls):
        cls.mesh, cls.skeleton, cls.counts, cls.sockets_on_bones = bw.assemble(0)
        cls.clips = bw.build_clips(bw.build_skeleton())
        cls.inventory = bw.contract_inventory(cls.mesh, cls.skeleton, cls.clips)
        cls.components = set(cls.mesh.components())

    def test_six_barrier_cells_three_per_wing(self):
        self.assertEqual(self.inventory["barrier_cells"]["built"], 6)
        self.assertEqual(self.inventory["barrier_cells"]["fields"], 6)
        left = [n for n in bw.CELL_NAMES if bw.CELL_Y[bw.CELL_NAMES.index(n)] < 0]
        right = [n for n in bw.CELL_NAMES if bw.CELL_Y[bw.CELL_NAMES.index(n)] > 0]
        self.assertEqual((len(left), len(right)), (3, 3))
        self.assertEqual(left, ["cell_01", "cell_02", "cell_03"])
        self.assertEqual(right, ["cell_04", "cell_05", "cell_06"])

    def test_two_articulated_wings_with_hinge_arms(self):
        self.assertEqual(self.inventory["barrier_wings"]["built"], 2)
        self.assertIn("wing_l_hinge_barrel", self.components)
        self.assertIn("wing_r_hinge_barrel", self.components)

    def test_two_operator_stations_with_cyan_visors(self):
        self.assertEqual(self.inventory["operator_stations"]["built"], 2)
        self.assertEqual(self.inventory["operator_stations"]["visors"], 2)
        visor_slot = self.mesh.slots.index(bw.STATUS)
        for side in ("l", "r"):
            visors = [p for p in self.mesh.polygons if p.component == f"op_{side}_visor"]
            self.assertTrue(visors)
            self.assertTrue(all(p.slot == visor_slot for p in visors))

    def test_four_block_legs_with_pads(self):
        self.assertEqual(self.inventory["legs"]["built"], 4)
        for tag in ("fl", "fr", "rl", "rr"):
            self.assertIn(f"leg_{tag}_foot", self.components)
            self.assertIn(f"leg_{tag}_hub", self.components)

    def test_central_emitter_with_cyan_lens_on_the_centreline(self):
        self.assertEqual(self.inventory["central_emitter"]["built"], 1)
        self.assertEqual(self.inventory["central_emitter"]["lens"], 1)
        (y0, y1) = (self.mesh.component_bounds("emitter_lens")[0][1], self.mesh.component_bounds("emitter_lens")[1][1])
        self.assertAlmostEqual((y0 + y1) / 2.0, 0.0, places=6)

    def test_chassis_present_with_an_open_rear(self):
        self.assertEqual(self.inventory["chassis"]["built"], 1)
        self.assertTrue(self.inventory["chassis"]["open_rear"])

    def test_sub_object_separation_for_both_shield_panels(self):
        self.assertEqual(self.inventory["sub_objects"]["built"], [bw.LEFT_PART, bw.RIGHT_PART])
        for side, expected_cells in (("l", ("cell_01_", "cell_02_", "cell_03_")), ("r", ("cell_04_", "cell_05_", "cell_06_"))):
            part = bw.wing_part(0, side)
            self.assertTrue(part.polygons)
            prefixes = {c.split("_")[0] + "_" + c.split("_")[1] + "_" for c in part.components() if c.startswith("cell_")}
            self.assertEqual(prefixes, set(expected_cells))
            self.assertTrue(all(c.startswith("cell_") or c.startswith("wing_") for c in part.components()))
            self.assertEqual({s.name for s in part.sockets},
                             {f"Cell_{k:02d}" for k in ((1, 2, 3) if side == "l" else (4, 5, 6))})


class ConceptProportions(unittest.TestCase):
    """The measured ratios of the owner-corrected reference (build_bulwark.MEASURED_PX), as a
    fraction of W = 500 cm, the deployed wall width fixed by cover_half_width_cm 250."""

    @classmethod
    def setUpClass(cls):
        cls.mesh = bw.assemble(0)[0]
        cls.measure = bw.concept_measurements(cls.mesh)
        cls.face = cls.measure["deployed_face"]

    def ratio(self, key):
        return bw.MEASURED_PX[key] / bw.MEASURED_PX["wall_width"]

    def test_wall_width_matches_the_cover_it_grants(self):
        self.assertAlmostEqual(self.face["span_y_cm"], bw.W, delta=6.0)
        self.assertAlmostEqual(self.face["span_y_cm"] / 2.0, 250.0, delta=3.0)   # cover_half_width_cm

    def test_wall_height_ratio(self):
        got = (self.face["wall_top_z_cm"] - self.face["wall_bottom_z_cm"]) / bw.W
        self.assertAlmostEqual(got, self.ratio("wall_height"), delta=0.01)
        self.assertAlmostEqual(got, 0.560, delta=0.01)

    def test_wall_stands_clear_of_the_ground_on_the_legs(self):
        got = self.face["wall_bottom_z_cm"] / bw.W
        self.assertAlmostEqual(got, self.ratio("wall_bottom_above_ground"), delta=0.01)
        self.assertGreater(self.face["wall_bottom_z_cm"], bw.HUB_Z)

    def test_cell_width_and_height_ratios(self):
        self.assertAlmostEqual(self.face["cell_width_cm"][0] / bw.W, self.ratio("cell_pitch"), delta=0.006)
        self.assertAlmostEqual(self.face["cell_height_cm"][0] / bw.W, self.ratio("cell_field_height") + 0.042, delta=0.02)

    def test_operator_stations_flank_the_centreline_at_the_measured_offset(self):
        self.assertAlmostEqual(bw.OPERATOR_Y / bw.W, self.ratio("operator_centre_y"), delta=0.005)
        self.assertGreater(bw.OPERATOR_Y * 2.0, bw.OPERATOR_W)   # the two cowls do not overlap

    def test_wall_is_taller_than_the_operators(self):
        self.assertTrue(self.measure["wall_taller_than_operators"])
        # the reference's measured cue is the COWL top: 97 px below the wall top
        self.assertAlmostEqual(self.measure["wall_top_above_operator_cowl_cm"],
                               bw.MEASURED_PX["operator_top_below_wall_top"] * bw.PX_PER_CM, delta=6.0)
        # and the WHOLE station (the frame arch over the cowl is the tallest part) must still sit
        # inside the reference's machinery band, 47..99 px below the wall top
        gap = self.measure["wall_top_above_operator_cm"]
        self.assertGreaterEqual(gap, 47.0 * bw.PX_PER_CM - 3.0)
        self.assertLessEqual(gap, 99.0 * bw.PX_PER_CM + 3.0)
        self.assertGreaterEqual(self.measure["operator_top_z_cm"], self.measure["operator_cowl_top_z_cm"])

    def test_chassis_width_ratio(self):
        self.assertAlmostEqual(self.measure["chassis_width_over_W"], 0.44, delta=0.005)

    def test_leg_and_pad_offsets(self):
        self.assertAlmostEqual(bw.LEG_Y / bw.W, self.ratio("leg_centre_y"), delta=0.005)
        self.assertAlmostEqual(bw.FOOT_Y_CM / bw.W, self.ratio("foot_centre_y"), delta=0.005)
        self.assertAlmostEqual(bw.FOOT_L / bw.W, 0.19, delta=0.02)

    def test_emitter_sits_on_the_centreline_at_the_base_of_the_seam(self):
        self.assertTrue(self.measure["emitter_on_centreline"])
        self.assertTrue(self.measure["emitter_at_face_base"])
        self.assertAlmostEqual(self.face["seam_y_cm"], 0.0, delta=0.5)

    def test_deployed_height_over_W(self):
        self.assertAlmostEqual(self.measure["deployed_height_over_W"],
                               (bw.MEASURED_PX["wall_bottom_above_ground"] + bw.MEASURED_PX["wall_height"]) / bw.MEASURED_PX["wall_width"],
                               delta=0.02)


class DeployedFaceState(unittest.TestCase):
    """Fidelity check 1/2: one flat frontal face of six equal cells, seam on the centreline."""

    @classmethod
    def setUpClass(cls):
        cls.mesh = bw.assemble(0)[0]
        cls.face = bw.deployed_face_measure(cls.mesh)

    def test_face_is_flat_within_a_narrow_slab(self):
        self.assertLessEqual(self.face["slab_thickness_cm"], 40.0)
        self.assertGreater(self.face["slab_x_min"], bw.CHASSIS_X1)   # entirely ahead of the chassis front

    def test_face_is_frontal_and_spans_about_500_cm(self):
        self.assertGreater(self.face["span_y_cm"], 480.0)
        self.assertLess(self.face["span_y_cm"], 510.0)
        self.assertAlmostEqual(self.face["span_over_W"], 1.0, delta=0.01)

    def test_six_equal_framed_cells_three_per_wing(self):
        self.assertEqual(self.face["cells"], 6)
        self.assertEqual((self.face["left_wing_cells"], self.face["right_wing_cells"]), (3, 3))
        self.assertLess(self.face["cell_width_spread_cm"], 0.5)
        self.assertLess(self.face["cell_height_spread_cm"], 0.5)
        self.assertLess(max(self.face["cell_pitch_cm"]) - min(self.face["cell_pitch_cm"]), 0.5)
        for name in bw.CELL_NAMES:
            self.assertIn(f"{name}_frame", self.mesh.components())
            self.assertIn(f"{name}_field", self.mesh.components())

    def test_seam_between_cell_03_and_cell_04_is_on_the_centreline(self):
        self.assertAlmostEqual(self.face["seam_y_cm"], 0.0, delta=0.5)
        c3 = self.mesh.component_bounds("cell_03_frame")
        c4 = self.mesh.component_bounds("cell_04_frame")
        self.assertLess(c3[1][1], 0.7)
        self.assertGreater(c4[0][1], -0.7)

    def test_the_face_never_wraps_the_unit(self):
        for name in bw.CELL_NAMES:
            (x0, _y0, _z0), (x1, _y1, _z1) = self.mesh.component_bounds(f"{name}_frame")
            self.assertGreater(x0, bw.CHASSIS_X1)
            self.assertLess(x1 - x0, 40.0)


class RearOpenState(unittest.TestCase):
    """Fidelity check 4: the rear stays open above the legs; no shield behind the chassis."""

    @classmethod
    def setUpClass(cls):
        cls.mesh = bw.assemble(0)[0]
        cls.packed = bw.packed_mesh(0)

    def test_no_geometry_behind_the_chassis_rear_plane_above_the_legs(self):
        for mesh, label in ((self.mesh, "deployed"), (self.packed, "packed")):
            report = bw.rear_open_measure(mesh)
            self.assertEqual(report["polygons_behind_rear_plane_above_legs"], 0,
                             f"{label}: {report['offenders']}")

    def test_the_chassis_rear_face_is_omitted(self):
        self.assertTrue(bw.rear_open_measure(self.mesh)["rear_face_open"])

    def test_no_barrier_pane_is_visible_from_the_rear(self):
        """The property fidelity check 4 actually claims: from a camera behind the unit, no cell
        field slab and no cyan cell rim is ever the frontmost surface. Asserting a placement plane
        instead (every pane sits ahead of the chassis anyway) cannot fail and tests nothing."""
        for mesh, label in ((self.mesh, "deployed"), (self.packed, "packed")):
            visible = bw.visible_cell_pane_components(mesh, "-X", grid=200)
            self.assertEqual(visible, [], f"{label}: barrier panes visible from the rear: {visible}")
            census = bw.visibility_census(mesh, "-X", grid=200)
            self.assertEqual(census["field_fraction"], 0.0, label)
            self.assertLess(census["status_fraction"], 0.02, f"{label}: {census['slot_fraction']}")

    def test_every_field_slab_is_backed_by_an_opaque_frame_plate(self):
        """Geometric form of the same rule: each cell's charcoal back plate covers the whole field
        footprint and lies strictly behind it, so the rear read does not depend on the material."""
        for name in bw.CELL_NAMES:
            (fx0, fy0, fz0), (fx1, fy1, fz1) = self.mesh.component_bounds(f"{name}_field")
            (bx0, by0, bz0), (bx1, by1, bz1) = self.mesh.component_bounds(f"{name}_backplate")
            self.assertLess(bx1, fx0, f"{name}: back plate must sit behind the field")
            self.assertLessEqual(by0, fy0 + 0.001, name)
            self.assertGreaterEqual(by1, fy1 - 0.001, name)
            self.assertLessEqual(bz0, fz0 + 0.001, name)
            self.assertGreaterEqual(bz1, fz1 - 0.001, name)
            frame = self.mesh.slots.index(bw.FRAME)
            plates = [p for p in self.mesh.polygons
                      if p.component == f"{name}_backplate" and p.slot == frame]
            self.assertTrue(plates, name)

    def test_the_cyan_rim_is_on_the_front_face_of_every_pane(self):
        """The reference's front-read cue. Built on the back face it inverts: invisible from the
        front, fully lit from behind."""
        for name in bw.CELL_NAMES:
            (ex0, _ey0, _ez0), (_ex1, _ey1, _ez1) = self.mesh.component_bounds(f"{name}_edge")
            (_fx0, _fy0, _fz0), (fx1, _fy1, _fz1) = self.mesh.component_bounds(f"{name}_field")
            self.assertGreater(ex0, fx1, f"{name}: the cyan rim must lie ahead of the field slab")
        visible = bw.visible_cell_pane_components(self.mesh, "+X", grid=200)
        for name in bw.CELL_NAMES:
            self.assertIn(f"{name}_edge", visible)
            self.assertIn(f"{name}_field", visible)


class PackedState(unittest.TestCase):
    """Fidelity check 3: cells folded as open frames along the chassis sides, compact crawler."""

    @classmethod
    def setUpClass(cls):
        cls.report = bw.packed_measure(0)
        cls.packed = bw.packed_mesh(0)

    def test_packed_silhouette_is_wider_than_tall(self):
        self.assertTrue(self.report["wider_than_tall"])
        self.assertGreater(self.report["packed_aspect_w_over_h"], 1.1)
        low, high = self.report["reference_aspect_range"]
        self.assertGreaterEqual(self.report["packed_aspect_w_over_h"], low - 0.1)
        self.assertLessEqual(self.report["packed_aspect_w_over_h"], high + 0.2)

    def test_packed_loses_the_frontal_face(self):
        self.assertTrue(self.report["frontal_face_lost"])
        self.assertGreater(self.report["packed_cell_slab_thickness_cm"], 100.0)
        self.assertLess(self.report["packed_cell_frontal_span_y_cm"], bw.WALL_W)

    def test_packed_is_more_compact_across_the_front_than_the_deployed_wall(self):
        self.assertTrue(self.report["packed_narrower_than_deployed_wall"])
        self.assertLess(self.report["packed_height_z_cm"], self.report["deployed_height_z_cm"])

    def test_folded_cells_flank_the_chassis_and_clear_the_legs(self):
        self.assertTrue(self.report["cells_clear_of_legs"])
        self.assertGreater(self.report["packed_cell_inner_face_y_cm"], bw.CHASSIS_W / 2.0)
        self.assertGreater(self.report["packed_cell_bottom_z_cm"], 0.0)

    def test_the_packed_front_read_loses_the_barrier_field_entirely(self):
        census = bw.visibility_census(self.packed, "+X", grid=200)
        self.assertEqual(census["field_fraction"], 0.0)

    def test_the_packed_flank_is_clean_once_the_field_material_is_off(self):
        """The shipped packed pose still carries the six field slabs - the rig is rotation and
        translation only, so a rigid 62 x 260 cm pane cannot leave the mesh, and the fold turns each
        pane's front face outboard (README section 8 deviation 14). What the runtime must deliver is
        the field material OFF while packed; that state is the packed_field_dark review assembly and
        this test bounds it. The shipped-mesh fraction is asserted only to keep the deviation's
        recorded number honest."""
        dark = bw.state_mesh(self.packed, field="dark", edges="dark")
        for side in ("-Y", "+Y"):
            census = bw.visibility_census(dark, side, grid=200)
            self.assertEqual(census["field_fraction"], 0.0, side)
            self.assertLess(census["status_fraction"], 0.01, side)
        lit = bw.visibility_census(self.packed, "-Y", grid=200)
        self.assertGreater(lit["field_fraction"], 0.30)   # the recorded deviation, not a target
        self.assertLess(lit["field_fraction"], 0.55)

    def test_three_folded_cells_on_each_side(self):
        left = right = 0
        for name in bw.CELL_NAMES:
            b = self.packed.component_bounds(f"{name}_frame")
            centre = (b[0][1] + b[1][1]) / 2.0
            left += centre < 0.0
            right += centre > 0.0
        self.assertEqual((left, right), (3, 3))

    def test_the_packed_state_is_reachable_by_the_rig_alone(self):
        pose = bw.packed_pose()
        self.assertEqual(set(pose), {"wing_root_l", "wing_root_r"})
        for value in pose.values():
            self.assertEqual(len(value), 6)


class RigAndSockets(unittest.TestCase):
    """18-bone transformation rig and the card's named sockets."""

    @classmethod
    def setUpClass(cls):
        cls.mesh, cls.skeleton, cls.counts, cls.sockets_on_bones = bw.assemble(0)

    def test_eighteen_bones_in_the_documented_hierarchy(self):
        names = [b.name for b in self.skeleton.bones]
        self.assertEqual(len(names), 18)
        self.assertEqual(names[0], "root")
        self.assertEqual(self.skeleton.get("root").parent, None)
        for name in ("chassis", "operator_l", "operator_r", "leg_fl", "leg_fr", "leg_rl", "leg_rr",
                     "emitter", "shield_anchor", "wing_root_l", "wing_root_r", *bw.CELL_NAMES):
            self.assertIn(name, names)
        self.assertEqual(self.skeleton.get("chassis").parent, "root")
        for name in ("operator_l", "operator_r", "leg_fl", "leg_fr", "leg_rl", "leg_rr",
                     "emitter", "shield_anchor", "wing_root_l", "wing_root_r"):
            self.assertEqual(self.skeleton.get(name).parent, "chassis")
        for index, name in enumerate(bw.CELL_NAMES):
            self.assertEqual(self.skeleton.get(name).parent, "wing_root_l" if index < 3 else "wing_root_r")

    def test_rest_orientation_is_identity_and_the_root_is_at_ground_contact(self):
        self.assertEqual(self.skeleton.get("root").head, (0.0, 0.0, 0.0))
        self.assertAlmostEqual(self.mesh.bounds()[0][2], 0.0, delta=0.5)

    def test_socket_names_and_positions(self):
        by_name = {s.name: s for s in self.mesh.sockets}
        self.assertEqual(set(by_name), {"Target_Anchor_Center", "Emitter_Muzzle", "Shield_Face_Center",
                                        "Cell_01", "Cell_02", "Cell_03", "Cell_04", "Cell_05", "Cell_06"})
        self.assertEqual(by_name["Target_Anchor_Center"].position, (0.0, 0.0, 200.0))
        self.assertEqual(by_name["Emitter_Muzzle"].position, (bw.EMIT_X + 14.0, 0.0, bw.EMIT_Z))
        self.assertEqual(by_name["Shield_Face_Center"].position, (bw.WALL_FACE_X + bw.CELL_T / 2.0, 0.0, bw.CELL_MID_Z))
        for index, name in enumerate(bw.CELL_NAMES):
            socket = by_name[f"Cell_{index + 1:02d}"]
            self.assertEqual(socket.position, (bw.WALL_FACE_X, bw.CELL_HINGE_Y[index], bw.CELL_MID_Z))
            self.assertEqual(self.sockets_on_bones[socket.name], name)

    def test_shield_face_center_sits_on_the_deployed_face(self):
        socket = next(s for s in self.mesh.sockets if s.name == "Shield_Face_Center")
        face = bw.deployed_face_measure(self.mesh)
        self.assertAlmostEqual(socket.position[0], face["slab_x_max"], delta=0.5)
        self.assertAlmostEqual(socket.position[2], (face["wall_top_z_cm"] + face["wall_bottom_z_cm"]) / 2.0, delta=0.5)

    def test_every_polygon_is_bound_to_a_real_bone(self):
        names = {b.name for b in self.skeleton.bones}
        for poly in self.mesh.polygons:
            self.assertIn(getattr(poly, "bone", None), names, poly.component)
        for bone in ("chassis", "emitter", "operator_l", "operator_r", "leg_fl", "leg_fr", "leg_rl", "leg_rr",
                     "wing_root_l", "wing_root_r", *bw.CELL_NAMES):
            self.assertGreater(self.counts.get(bone, 0), 0, bone)


class Clips(unittest.TestCase):
    """Track contract, tick counts and ground contact for every sampled pose."""

    @classmethod
    def setUpClass(cls):
        cls.skeleton = bw.build_skeleton()
        cls.clips = bw.build_clips(cls.skeleton)
        cls.by_name = {c.name: c for c in cls.clips}

    def test_track_contract(self):
        self.assertEqual([c.name for c in self.clips],
                         ["idle_packed", "move_packed", "deploy", "idle_deployed", "drag_deployed", "pack",
                          "damage", "death", "cancel", "restore"])
        for name in ("idle_packed", "move_packed", "idle_deployed", "drag_deployed"):
            self.assertTrue(self.by_name[name].loop, name)
        for name in ("deploy", "pack", "damage", "death", "cancel", "restore"):
            self.assertFalse(self.by_name[name].loop, name)

    def test_deploy_is_twenty_ticks_and_pack_is_fifteen(self):
        # SPEC-UNIT-003 fixes 20-tick deploy and 15-tick pack. Every clip duration must also land on a
        # whole 30 fps frame or the Interchange skeletal import silently drops the clip (skeletal kit
        # ANIMATION_FPS, probe evidence 2026-09-07). 20 ticks = 1.0 s = 30 frames exactly; 15 ticks =
        # 0.75 s = 22.5 frames does not exist on the grid, so pack is authored at the next whole frame
        # (23 = 0.7667 s, +2.2%) and the runtime still drives the authoritative 15-tick timing.
        self.assertAlmostEqual(self.by_name["deploy"].duration_s * bw.TICKS_PER_SECOND, 20.0, places=6)
        pack_ticks = self.by_name["pack"].duration_s * bw.TICKS_PER_SECOND
        self.assertAlmostEqual(pack_ticks, 15.0, delta=0.5)
        self.assertEqual(self.by_name["pack"].duration_s, skel.frame_aligned_duration(15.0 / bw.TICKS_PER_SECOND))
        for clip in self.clips:
            self.assertTrue(skel.is_frame_aligned(clip.duration_s), f"{clip.name} {clip.duration_s}")

    def test_deploy_starts_packed_and_ends_on_the_flat_frontal_face(self):
        start = bw.posed(0, bw.sample_pose(self.by_name["deploy"], 0.0))
        end = bw.posed(0, bw.sample_pose(self.by_name["deploy"], 1.0))
        packed_span = max(start.component_bounds(f"{n}_frame")[1][0] for n in bw.CELL_NAMES) - \
            min(start.component_bounds(f"{n}_frame")[0][0] for n in bw.CELL_NAMES)
        self.assertGreater(packed_span, 100.0)
        self.assertLessEqual(bw.deployed_face_measure(end)["slab_thickness_cm"], 40.0)
        self.assertAlmostEqual(bw.deployed_face_measure(end)["span_y_cm"], bw.WALL_W, delta=6.0)

    def test_pack_ends_in_the_same_travel_profile_the_packed_pose_defines(self):
        end = bw.posed(0, bw.sample_pose(self.by_name["pack"], 1.0))
        target = bw.packed_mesh(0)
        self.assertEqual(len(end.polygons), len(target.polygons))
        for a, b in zip(end.polygons, target.polygons):
            for pa, pb in zip(a.points, b.points):
                for ca, cb in zip(pa, pb):
                    self.assertAlmostEqual(ca, cb, delta=0.05)

    def test_every_clip_keeps_the_pads_on_the_ground(self):
        for clip in self.clips:
            for step in range(21):
                pose = bw.sample_pose(clip, step / 20.0)
                z = bw.posed(1, pose).bounds()[0][2]
                self.assertGreaterEqual(z, -1.0, f"{clip.name} at {step / 20.0:.2f} sinks to {z:.2f}")
                self.assertLessEqual(z, 3.0, f"{clip.name} at {step / 20.0:.2f} floats to {z:.2f}")

    def test_no_root_motion(self):
        for clip in self.clips:
            self.assertNotIn("root", clip.tracks, clip.name)

    def test_cancel_returns_to_the_travel_profile(self):
        end = bw.posed(0, bw.sample_pose(self.by_name["cancel"], 1.0))
        report_span = max(end.component_bounds(f"{n}_frame")[1][0] for n in bw.CELL_NAMES) - \
            min(end.component_bounds(f"{n}_frame")[0][0] for n in bw.CELL_NAMES)
        self.assertGreater(report_span, 100.0)

    def test_restore_returns_to_the_flat_frontal_face(self):
        end = bw.posed(0, bw.sample_pose(self.by_name["restore"], 1.0))
        self.assertLessEqual(bw.deployed_face_measure(end)["slab_thickness_cm"], 40.0)


class BudgetsAndSlots(unittest.TestCase):
    """Card ceilings, LOD spacing, material slots and the emissive share."""

    @classmethod
    def setUpClass(cls):
        cls.m0 = bw.assemble(0)[0]
        cls.m1 = bw.assemble(1)[0]

    def test_lod_ceilings(self):
        self.assertLessEqual(self.m0.triangle_count(), bw.LOD0_CAP)
        self.assertLessEqual(self.m1.triangle_count(), bw.LOD1_CAP)

    def test_lod1_is_a_reduction_that_keeps_the_silhouette(self):
        ratio = self.m1.triangle_count() / self.m0.triangle_count()
        self.assertLess(ratio, 0.75)
        self.assertGreater(ratio, 0.25)   # REL-ART-005.AUTH: LOD1 must not pop
        face0 = bw.deployed_face_measure(self.m0)
        face1 = bw.deployed_face_measure(self.m1)
        self.assertAlmostEqual(face0["span_y_cm"], face1["span_y_cm"], delta=1.0)
        self.assertAlmostEqual(face0["wall_top_z_cm"], face1["wall_top_z_cm"], delta=1.0)
        self.assertEqual(face1["cells"], 6)

    def test_lod1_keeps_the_operator_station_and_the_overall_envelope(self):
        """The tri ratio and the wall span cannot see the operator superstructure or the rear bay;
        an unbounded LOD1 dropped the station 12 cm and popped at the transition."""
        s0 = bw.operator_station_measure(self.m0)
        s1 = bw.operator_station_measure(self.m1)
        self.assertAlmostEqual(s0["station_top_z_cm"], s1["station_top_z_cm"], delta=2.0)
        self.assertAlmostEqual(s0["cowl_top_z_cm"], s1["cowl_top_z_cm"], delta=2.0)
        (a0, a1), (b0, b1) = self.m0.bounds(), self.m1.bounds()
        for k in range(3):
            self.assertAlmostEqual(a0[k], b0[k], delta=4.0, msg=f"min axis {k}")
            self.assertAlmostEqual(a1[k], b1[k], delta=4.0, msg=f"max axis {k}")

    def test_the_rear_and_front_cues_survive_the_lod_transition(self):
        """Fidelity check 4 is certified at LOD0; it must hold at LOD1 too. The cues are the bay
        core and the two flank conduits (the only cyan the rear shows) and the twelve pane rims."""
        required = {"chassis_bay_core", "chassis_conduit_l", "chassis_conduit_r"}
        required |= {f"{name}_edge" for name in bw.CELL_NAMES}
        required |= {f"{name}_backplate" for name in bw.CELL_NAMES}
        for mesh, label in ((self.m0, "LOD0"), (self.m1, "LOD1")):
            comps = set(mesh.components())
            self.assertTrue(required <= comps, f"{label} is missing {sorted(required - comps)}")
            self.assertEqual(bw.visible_cell_pane_components(mesh, "-X", grid=160), [], label)

    def test_cells_do_not_interpenetrate_their_neighbours(self):
        """Left_Shield_Panel and Right_Shield_Panel are separate sub-objects the card requires for
        physical transformation: their parts must not sweep through each other, and no cell casting
        or hinge may cross into the 1.333 cm reveal of the next cell."""
        boxes = {}
        for name in bw.CELL_NAMES:
            for comp in self.m0.components():
                if comp.startswith(f"{name}_"):
                    boxes.setdefault(name, []).append((comp, self.m0.component_bounds(comp)))
        for i, a in enumerate(bw.CELL_NAMES):
            for b in bw.CELL_NAMES[i + 1:]:
                for ca, ba in boxes[a]:
                    for cb, bb in boxes[b]:
                        overlap = all(ba[0][k] < bb[1][k] - 1e-6 and bb[0][k] < ba[1][k] - 1e-6 for k in range(3))
                        self.assertFalse(overlap, f"{ca} intersects {cb}")

    def test_the_two_shield_panels_stay_on_their_own_side_of_the_centreline(self):
        for lod in (0, 1):
            left = bw.wing_part(lod, "l").bounds()
            right = bw.wing_part(lod, "r").bounds()
            pivot_l = -bw.WING_PIVOT[1]
            pivot_r = bw.WING_PIVOT[1]
            self.assertLessEqual(left[1][1] + pivot_l, 0.0, f"LOD{lod} left panel crosses y = 0")
            self.assertGreaterEqual(right[0][1] + pivot_r, 0.0, f"LOD{lod} right panel crosses y = 0")

    def test_material_slots(self):
        self.assertEqual(self.m0.slots, list(bw.SLOTS))
        self.assertEqual(self.m1.slots, list(bw.SLOTS))
        for poly in self.m0.polygons:
            self.assertLess(poly.slot, len(bw.SLOTS))

    def test_the_barrier_field_slot_carries_only_the_six_cell_fields(self):
        field = self.m0.slots.index(bw.FIELD)
        for poly in self.m0.polygons:
            if poly.slot == field:
                self.assertTrue(poly.component.endswith("_field"), poly.component)

    def test_cyan_stays_a_small_share_of_the_surface(self):
        fraction = bw.slot_area_fraction(self.m0, bw.STATUS)
        self.assertLess(fraction, 0.10)
        self.assertGreater(fraction, 0.0)

    def test_collision_boxes_do_not_extend_past_the_stance(self):
        self.assertTrue(self.m0.collision)
        for box in self.m0.collision:
            self.assertLessEqual(abs(box.center[1]) + box.size[1] / 2.0, bw.FOOT_Y_CM + bw.FOOT_W)


class Determinism(unittest.TestCase):
    """Re-running the generator must produce byte-identical exports."""

    def test_revision_string(self):
        self.assertEqual(bw.REVISION, "ebs-mer-unt-003-concept-v3")
        self.assertEqual(bw.REVISION, f"{bw.PRODUCTION_ID.lower()}-concept-v3")

    def test_exports_are_byte_identical_across_two_builds(self):
        digests = []
        for _ in range(2):
            with tempfile.TemporaryDirectory(prefix="ebs-bulwark-test-") as tmp:
                outputs, review, _meshes, _clips = bw.build_outputs(os.path.join(tmp, "export"), os.path.join(tmp, "review"))
                digests.append(({o["path"]: o["sha256"] for o in outputs},
                                {os.path.basename(r["path"]): r["sha256"] for r in review}))
        self.assertEqual(digests[0], digests[1])
        self.assertGreaterEqual(len(digests[0][0]), 12)

    def test_posed_meshes_are_stable(self):
        def digest():
            mesh = bw.packed_mesh(0)
            payload = "".join(f"{p[0]:.4f},{p[1]:.4f},{p[2]:.4f}" for poly in mesh.polygons for p in poly.points)
            return hashlib.sha256(payload.encode()).hexdigest()
        self.assertEqual(digest(), digest())


class ManifestAgreement(unittest.TestCase):
    """The committed build-manifest.json describes the source as it stands."""

    @classmethod
    def setUpClass(cls):
        path = os.path.join(HERE, "build-manifest.json")
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                cls.manifest = json.load(handle)
        else:
            cls.manifest = None

    def test_manifest_matches_the_current_source(self):
        if self.manifest is None:
            self.skipTest("build-manifest.json not written yet")
        self.assertEqual(self.manifest["revision"], bw.REVISION)
        self.assertEqual(self.manifest["author"], "Angelis Pseftis")
        self.assertEqual(self.manifest["budgets"]["lod0_triangles"], bw.assemble(0)[0].triangle_count())
        self.assertEqual(self.manifest["budgets"]["lod1_triangles"], bw.assemble(1)[0].triangle_count())
        self.assertEqual(self.manifest["component_inventory"]["bones"]["built"], 18)
        self.assertTrue(self.manifest["packed_state"]["wider_than_tall"])
        self.assertEqual(self.manifest["material_slots"], list(bw.SLOTS))
        self.assertEqual(self.manifest["planned_unreal_folder"], "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_003/")
        self.assertEqual(self.manifest["asset_name"], "SK_EBS_MER_UNT_003")

    def test_every_recorded_export_exists(self):
        if self.manifest is None:
            self.skipTest("build-manifest.json not written yet")
        for entry in self.manifest["outputs"]:
            self.assertTrue(os.path.exists(os.path.join(HERE, entry["path"])), entry["path"])
            with open(os.path.join(HERE, entry["path"]), "rb") as handle:
                self.assertEqual(hashlib.sha256(handle.read()).hexdigest(), entry["sha256"], entry["path"])

    def test_review_assemblies_and_scenes_exist(self):
        if self.manifest is None or not os.path.isdir(EVIDENCE_DIR):
            self.skipTest("evidence directory not present")
        for entry in self.manifest["review_assemblies"]:
            self.assertTrue(os.path.exists(entry["path"]), entry["path"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
