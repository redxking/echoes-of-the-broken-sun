#!/usr/bin/env python3
"""Regression checks for the EBS-MER-UNT-003 Bulwark Team concept-v5 source.

Author: Angelis Pseftis.
Usage: python3 -m unittest discover -s ArtSource/EBS-MER-UNT-003 -p 'test_*.py'

Structural checks against concept-fidelity.md, the owner-corrected reference measurements carried
in build_bulwark.MEASURED_PX, and the bounding rules: contract inventory (every concept part
present), the measured proportion ratios, socket names and positions, LOD budgets, material slots,
ground contact for every clip sample, both state assertions (deployed face flat and frontal, rear
open, packed silhouette wider than tall with the frontal face lost, six equal framed cells three
per wing) and byte-level determinism. No renders and no engine are involved.

concept-v4 added the owner ruling of 2026-09-07 as a testable contract (class BarrierPaneSubObject):
the six structural cell frames stay in the main skinned mesh in both states, the removable pane
assembly is a third sub-object skinned to the same cell bones, the packed shipped mesh contains no
field slab at all, the assembly covers every frame when deployed, the whole-unit budget clears both
the card cap and the tighter REL-ART-028 cap, and every review still is drawn under the visibility
contract. Each of those assertions fails against the concept-v3 source, which carried the panes
inside the main mesh - confirmed by running this module against `git show HEAD:...build_bulwark.py`
(README section 6).

concept-v5 adds class RulingRecordFidelity: the ruling text this package quotes is re-read from
ArtSource/production-ledger.json and compared character for character, and the words concept-v4
attributed to the owner but that the owner never wrote may only appear inside a struck-through
correction. It also adds the cutaway read-through measurement and the pane assembly's authored LOD1.
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
import ebs_sheet as sheet_tool  # noqa: E402
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

    def test_sub_object_separation_for_three_sub_objects(self):
        """Owner ruling 2026-09-07: three sub-objects, not two. The two wing panels carry their wing
        arm and their three STRUCTURAL CELL FRAMES and no pane geometry at all."""
        self.assertEqual(self.inventory["sub_objects"]["built"], [bw.LEFT_PART, bw.RIGHT_PART, bw.BARRIER_PART])
        for side, expected_cells in (("l", ("cell_01_", "cell_02_", "cell_03_")), ("r", ("cell_04_", "cell_05_", "cell_06_"))):
            part = bw.wing_part(0, side)
            self.assertTrue(part.polygons)
            prefixes = {c.split("_")[0] + "_" + c.split("_")[1] + "_" for c in part.components() if c.startswith("cell_")}
            self.assertEqual(prefixes, set(expected_cells))
            self.assertTrue(all(c.startswith("cell_") or c.startswith("wing_") for c in part.components()))
            self.assertEqual({s.name for s in part.sockets},
                             {f"Cell_{k:02d}" for k in ((1, 2, 3) if side == "l" else (4, 5, 6))})
            panes = [c for c in part.components() if c.rsplit("_", 1)[-1] in bw.PANE_COMPONENTS]
            self.assertEqual(panes, [], f"{bw.LEFT_PART if side == 'l' else bw.RIGHT_PART} carries pane geometry: {panes}")
            self.assertNotIn(bw.FIELD, part.slot_names_in_primitive_order())


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
        cls.assembly = bw.deployed_assembly(0)
        cls.face = bw.deployed_face_measure(cls.mesh)

    def test_the_deployed_face_is_still_flat_frontal_and_centred_with_the_assembly_on(self):
        """The ruling keeps the deployed read exactly as it was: the SHIPPED deployed assembly (main
        mesh + pane assembly) is one flat frontal wall, centred, with the 03/04 seam on y = 0."""
        panes = [self.assembly.component_bounds(f"{n}_field") for n in bw.CELL_NAMES]
        xs = [b[0][0] for b in panes] + [b[1][0] for b in panes]
        self.assertLess(max(xs) - min(xs), 10.0, "the six field slabs are not coplanar")
        self.assertGreater(min(xs), bw.CHASSIS_X1, "the wall must lie entirely ahead of the chassis front")
        centres = sorted((b[0][1] + b[1][1]) / 2.0 for b in panes)
        self.assertAlmostEqual((centres[2] + centres[3]) / 2.0, 0.0, delta=0.5)
        self.assertAlmostEqual(centres[0] + centres[5], 0.0, delta=0.5)     # symmetric about the centreline
        self.assertEqual((sum(1 for c in centres if c < 0), sum(1 for c in centres if c > 0)), (3, 3))
        rims = bw.visible_cell_pane_components(self.assembly, "+X", grid=200)
        for name in bw.CELL_NAMES:
            self.assertIn(f"{name}_edge", rims)
            self.assertIn(f"{name}_field", rims)

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
            self.assertIn(f"{name}_frame", self.mesh.components())          # structural frame: main mesh
            self.assertIn(f"{name}_field", self.assembly.components())      # field slab: pane assembly
            self.assertNotIn(f"{name}_field", self.mesh.components())

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
        cls.mesh = bw.deployed_assembly(0)       # the SHIPPED deployed read: main mesh + pane assembly
        cls.packed = bw.packed_mesh(0)           # the SHIPPED packed read: the assembly hidden
        cls.panes = bw.barrier_assembly(0)[0]

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
        footprint and lies strictly behind it, so the rear read does not depend on the material. The
        back plate travels WITH the pane assembly (owner ruling: it is part of the removable pane
        assembly), so hiding the assembly can never leave a lit field with its backing gone."""
        for name in bw.CELL_NAMES:
            self.assertIn(f"{name}_backplate", self.panes.components())
            self.assertNotIn(f"{name}_backplate", bw.assemble(0)[0].components())
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

    def test_the_packed_shipped_mesh_carries_no_field_slab_at_all(self):
        """The owner ruling's core claim, on the SHIPPED packed mesh: not a dark material state, not a
        review variant - there is no pane geometry in the mesh, so there is nothing to fail to hide."""
        self.assertEqual(self.report["packed_pane_components_in_shipped_mesh"], [])
        self.assertEqual(self.report["packed_field_polygons_in_shipped_mesh"], 0)
        for lod in (0, 1):
            packed = bw.packed_mesh(lod)
            self.assertNotIn(bw.FIELD, packed.slot_names_in_primitive_order(), f"LOD{lod}")
            for name in bw.CELL_NAMES:
                for suffix in bw.PANE_COMPONENTS:
                    self.assertIsNone(packed.component_bounds(f"{name}_{suffix}"), f"LOD{lod} {name}_{suffix}")

    def test_the_packed_flanks_are_open_cages_by_geometry(self):
        """What a flank camera sees, measured: 0.0 percent barrier field on the shipped packed mesh,
        with the six structural frames still frontmost - the reference's open cages. The failure
        control (the same pose with the assembly still drawn, i.e. what concept-v3 shipped) is
        asserted to still read 30-55 percent field, so the 0.0 above cannot pass vacuously."""
        control = bw.packed_with_panes(0)
        for side, wing in (("-Y", ("cell_01", "cell_02", "cell_03")), ("+Y", ("cell_04", "cell_05", "cell_06"))):
            census = bw.visibility_census(self.packed, side, grid=200)
            self.assertEqual(census["field_fraction"], 0.0, side)
            self.assertLess(census["status_fraction"], 0.01, side)
            frames = [c for c in census["frontmost_components"] if c.endswith("_frame") and c.startswith("cell_")]
            self.assertEqual(sorted(frames), [f"{c}_frame" for c in wing],
                             f"{side}: that flank's three folded cages must still be there: {frames}")
            failed = bw.visibility_census(control, side, grid=200)
            self.assertGreater(failed["field_fraction"], 0.30, side)
            self.assertLess(failed["field_fraction"], 0.55, side)

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


class BarrierPaneSubObject(unittest.TestCase):
    """Owner ruling 2026-09-07: the field slabs and the rest of the removable pane assembly are a
    THIRD sub-object, separately controlled, while the chassis, the hinged wings and the six
    structural cell frames stay physically present in the main skinned mesh in both states."""

    @classmethod
    def setUpClass(cls):
        cls.main = bw.assemble(0)[0]
        cls.panes, cls.pane_skeleton, cls.pane_bones, cls.pane_sockets = bw.barrier_assembly(0)
        cls.assembly = bw.deployed_assembly(0)

    def test_the_third_sub_object_exists_and_has_exactly_one_name(self):
        self.assertEqual(bw.BARRIER_PART, "SK_EBS_MER_UNT_003_Barrier_Panes")
        self.assertEqual(self.panes.name, bw.BARRIER_PART)
        names = {o["name"] for o in bw.component_contract(0)["objects"]}
        self.assertEqual(names, {bw.ASSET, bw.BARRIER_PART, bw.LEFT_PART, bw.RIGHT_PART})
        self.assertIn(bw.BARRIER_PART, bw.contract_inventory(self.main, bw.build_skeleton(),
                                                             bw.build_clips(bw.build_skeleton()))["sub_objects"]["built"])

    def test_the_pane_assembly_carries_the_panes_and_nothing_else(self):
        expected = {f"{name}_{suffix}" for name in bw.CELL_NAMES for suffix in bw.PANE_COMPONENTS}
        self.assertEqual(set(self.panes.components()), expected)
        self.assertEqual(len(expected), 18)

    def test_the_six_structural_frames_stay_in_the_main_mesh_in_both_states(self):
        """Three per side, centred when deployed, and still there when packed - the cages the
        owner-corrected reference shows folded along the flanks."""
        for lod in (0, 1):
            main = bw.assemble(lod)[0]
            packed = bw.packed_mesh(lod)
            for mesh, label in ((main, f"LOD{lod} deployed"), (packed, f"LOD{lod} packed")):
                frames = [c for c in mesh.components() if c.startswith("cell_") and c.endswith("_frame")]
                self.assertEqual(len(frames), 6, label)
                centres = [(mesh.component_bounds(c)[0][1] + mesh.component_bounds(c)[1][1]) / 2.0 for c in frames]
                self.assertEqual((sum(1 for v in centres if v < 0), sum(1 for v in centres if v > 0)), (3, 3), label)
            face = bw.deployed_face_measure(main)
            self.assertAlmostEqual(face["seam_y_cm"], 0.0, delta=0.5, msg=f"LOD{lod}")

    def test_the_main_mesh_carries_no_pane_geometry_in_either_state_or_lod(self):
        for lod in (0, 1):
            for mesh, label in ((bw.assemble(lod)[0], "deployed"), (bw.packed_mesh(lod), "packed")):
                offenders = [c for c in mesh.components() if c.rsplit("_", 1)[-1] in bw.PANE_COMPONENTS]
                self.assertEqual(offenders, [], f"LOD{lod} {label}: {offenders}")

    def test_the_pane_assembly_covers_every_one_of_the_six_frames_when_deployed(self):
        """Measured, not asserted from placement: sample each frame's inner opening and check that
        it is open through the frames-only mesh and filled by that cell's own pane on the shipped
        deployed assembly."""
        report = bw.pane_coverage_measure(0)
        self.assertTrue(report["every_frame_covered_when_deployed"], report["cells"])
        self.assertTrue(report["every_frame_open_when_the_assembly_is_hidden"], report["cells"])
        self.assertEqual(report["covered_by_the_assembly"], report["opening_samples"])
        self.assertGreaterEqual(report["opening_samples"], 1200)
        for cell in report["cells"]:
            self.assertTrue(cell["fully_covered"], cell)
            self.assertEqual(cell["uncovered_samples_show"], [], cell)

    def test_the_pane_assembly_follows_the_same_cell_bones_as_the_frames(self):
        """Hiding it must not disturb the frames, the deploy or the pack: same skeleton, and every
        pane bound to the same cell bone as the frame it fills."""
        main_skeleton = bw.build_skeleton()
        self.assertEqual([(b.name, b.parent, b.head) for b in self.pane_skeleton.bones],
                         [(b.name, b.parent, b.head) for b in main_skeleton.bones])
        for poly in self.panes.polygons:
            cell = poly.component.rsplit("_", 1)[0]
            self.assertIn(cell, bw.CELL_NAMES, poly.component)
            self.assertEqual(getattr(poly, "bone", None), cell, poly.component)
        self.assertEqual(set(self.pane_bones), set(bw.CELL_NAMES))

    def test_the_pane_assembly_tracks_the_frames_through_deploy_and_pack(self):
        """Every pane stays inside its own frame in every sampled pose of every clip, so the assembly
        never separates from the cage it fills - the property that makes hiding it safe."""
        clips = bw.build_clips(bw.build_skeleton())
        for name, fraction in (("deploy", 0.15), ("deploy", 0.55), ("deploy", 1.0), ("pack", 0.55),
                               ("idle_packed", 0.0), ("idle_deployed", 0.5), ("death", 1.0)):
            clip = next(c for c in clips if c.name == name)
            pose = bw.sample_pose(clip, fraction)
            main = bw.posed(0, pose)
            panes = bw.posed_barrier(0, pose)
            for cell in bw.CELL_NAMES:
                (fx0, fy0, fz0), (fx1, fy1, fz1) = main.component_bounds(f"{cell}_frame")
                (px0, py0, pz0), (px1, py1, pz1) = panes.component_bounds(f"{cell}_field")
                tol = 1.0
                self.assertGreaterEqual(px0, fx0 - tol, f"{name}@{fraction} {cell}")
                self.assertLessEqual(px1, fx1 + tol, f"{name}@{fraction} {cell}")
                self.assertGreaterEqual(py0, fy0 - tol, f"{name}@{fraction} {cell}")
                self.assertLessEqual(py1, fy1 + tol, f"{name}@{fraction} {cell}")
                self.assertGreaterEqual(pz0, fz0 - tol, f"{name}@{fraction} {cell}")
                self.assertLessEqual(pz1, fz1 + tol, f"{name}@{fraction} {cell}")

    def test_hiding_the_assembly_leaves_the_main_mesh_untouched(self):
        """Drawing the two together and drawing the main mesh alone give byte-identical main-mesh
        geometry, deployed and packed: the separation is a visibility switch, not a rebuild."""
        def digest(mesh, keep):
            payload = []
            for poly in mesh.polygons:
                if keep(poly.component):
                    payload.append(f"{poly.component}|{poly.slot}|" +
                                   ";".join(f"{p[0]:.4f},{p[1]:.4f},{p[2]:.4f}" for p in poly.points))
            return hashlib.sha256("\n".join(payload).encode()).hexdigest()
        main_only = lambda c: c.rsplit("_", 1)[-1] not in bw.PANE_COMPONENTS
        self.assertEqual(digest(self.assembly, main_only), digest(self.main, main_only))
        pose = bw.packed_pose()
        self.assertEqual(digest(bw.packed_with_panes(0), main_only), digest(bw.posed(0, pose), main_only))

    def test_the_visibility_contract_follows_the_authoritative_state_not_the_clip(self):
        """The contract the ruling requires, recorded in code so a review still cannot claim a state
        it is not in: the assembly is visible IF AND ONLY IF the authoritative state is DEPLOYED.

        THE ENDPOINT RULE, in the same words as build_bulwark.pane_assembly_visible, README section
        4.2 and README section 6: the DEPLOYED state is ENTERED at `deploy` t = 1.0 and LEFT at
        `pack` t > 0, so `deploy` t = 1.0 and `pack` t = 0.0 are DEPLOYED frames, not transition
        frames, and the assembly is drawn in exactly those two. Every other frame of `deploy` and
        `pack` is a transition frame and it is hidden; `cancel` never reaches DEPLOYED at all."""
        self.assertFalse(bw.pane_assembly_visible("idle_packed", 0.0))
        self.assertFalse(bw.pane_assembly_visible("move_packed", 0.25))
        self.assertFalse(bw.pane_assembly_visible("cancel", 0.0))
        self.assertFalse(bw.pane_assembly_visible("cancel", 1.0))
        for fraction in (0.0, 0.15, 0.55, 0.82, 0.99):
            self.assertFalse(bw.pane_assembly_visible("deploy", fraction), fraction)
        self.assertTrue(bw.pane_assembly_visible("deploy", 1.0))
        self.assertTrue(bw.pane_assembly_visible("pack", 0.0))
        for fraction in (0.01, 0.55, 1.0):
            self.assertFalse(bw.pane_assembly_visible("pack", fraction), fraction)
        for name in ("idle_deployed", "drag_deployed", "damage", "death", "restore"):
            self.assertTrue(bw.pane_assembly_visible(name, 0.5), name)
        clips = {c.name for c in bw.build_clips(bw.build_skeleton())}
        covered = set(bw.DEPLOYED_STATE_CLIPS) | set(bw.PACKED_STATE_CLIPS) | set(bw.TRANSITION_CLIPS)
        self.assertEqual(clips, covered, "every clip must be classified by the visibility contract")

    def test_the_visibility_contract_is_recorded_as_runtime_work_this_blockout_cannot_enforce(self):
        contract = bw.visibility_contract()
        self.assertEqual(contract["component"], bw.BARRIER_PART)
        self.assertFalse(contract["enforceable_here"])
        self.assertIn("PACKED", contract["hidden_in"])
        self.assertTrue(contract["cancellation"] and contract["save_restoration"])
        self.assertIn("authoritative", contract["rule"].lower())
        self.assertTrue(any("notify" in v for v in contract["never_driven_by"]))

    def test_every_review_pose_is_drawn_under_the_visibility_contract(self):
        """No posed still may show a barrier the authoritative state would not be showing."""
        for name, fraction in bw.POSE_SAMPLES:
            clip = next(c for c in bw.build_clips(bw.build_skeleton()) if c.name == name)
            pose = bw.sample_pose(clip, fraction)
            visible = bw.pane_assembly_visible(name, fraction)
            mesh = bw.posed_assembly(0, pose, visible)
            has_field = any(mesh.slots[p.slot] == bw.FIELD for p in mesh.polygons)
            self.assertEqual(has_field, visible, f"{name}@{fraction:.2f}")

    def test_the_deploy_sequence_ends_on_the_finished_wall(self):
        self.assertIn(("deploy", 1.0), bw.POSE_SAMPLES)


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

    def test_lod_ceilings_hold_for_the_WHOLE_UNIT_not_just_the_main_mesh(self):
        """Owner ruling 2026-09-07: the added sub-object does not increase the whole-unit budget, and
        BOTH ceilings bind - the card's 8,200/3,600 and REL-ART-028's tighter 8,000/3,500. The whole
        unit drawn is the main skinned mesh PLUS the barrier pane assembly; the literal sum of all
        four exports is bounded too, so no accounting of the split can slip past a cap."""
        for lod, cap, art028 in ((0, bw.LOD0_CAP, bw.LOD0_CAP_ART028), (1, bw.LOD1_CAP, bw.LOD1_CAP_ART028)):
            split = bw.budget_split(lod)
            self.assertEqual(split["whole_unit_drawn"], split["main_mesh"] + split["barrier_panes"])
            self.assertLessEqual(split["whole_unit_drawn"], art028, f"LOD{lod} whole unit over REL-ART-028")
            self.assertLessEqual(split["whole_unit_drawn"], cap, f"LOD{lod} whole unit over the card cap")
            self.assertLessEqual(split["all_exports_summed"], art028, f"LOD{lod} all exports over REL-ART-028")
            self.assertTrue(split["whole_unit_within_rel_art_028"] and split["all_exports_within_rel_art_028"])
            self.assertGreater(split["barrier_panes"], 0)

    def test_the_split_did_not_grow_the_whole_unit(self):
        """The pane assembly is geometry MOVED, not geometry added: main + panes is exactly what one
        mesh carried before the split, so every cell contributes its frame once and its pane once."""
        for lod in (0, 1):
            split = bw.budget_split(lod)
            per_cell_panes = bw.part_cell(lod, 0, "panes").triangle_count()
            self.assertEqual(split["barrier_panes"], 6 * per_cell_panes)
            frames = sum(bw.part_cell(lod, k, "frame").triangle_count() for k in range(6))
            cells_in_main = sum(t for c, t in bw.assemble(lod)[0].triangle_count_by("component").items()
                                if c.startswith("cell_"))
            self.assertEqual(cells_in_main, frames)

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
        main_required = {"chassis_bay_core", "chassis_conduit_l", "chassis_conduit_r"}
        main_required |= {f"{name}_frame" for name in bw.CELL_NAMES}
        pane_required = {f"{name}_edge" for name in bw.CELL_NAMES} | {f"{name}_backplate" for name in bw.CELL_NAMES}
        for lod, label in ((0, "LOD0"), (1, "LOD1")):
            comps = set(bw.assemble(lod)[0].components())
            self.assertTrue(main_required <= comps, f"{label} main mesh is missing {sorted(main_required - comps)}")
            panes = set(bw.barrier_assembly(lod)[0].components())
            self.assertTrue(pane_required <= panes, f"{label} pane assembly is missing {sorted(pane_required - panes)}")
            self.assertEqual(bw.visible_cell_pane_components(bw.deployed_assembly(lod), "-X", grid=160), [], label)
            self.assertEqual(bw.visible_cell_pane_components(bw.packed_mesh(lod), "-X", grid=160), [], label)

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
        self.assertEqual(bw.barrier_assembly(0)[0].slots, list(bw.SLOTS))
        for poly in self.m0.polygons:
            self.assertLess(poly.slot, len(bw.SLOTS))

    def test_the_barrier_field_slot_lives_only_in_the_pane_assembly(self):
        """The shield-field slot carries the six field slabs and nothing else, and it is not in the
        main mesh at all - so a Bulwark drawn without its pane assembly cannot show a barrier field."""
        panes = bw.barrier_assembly(0)[0]
        field = panes.slots.index(bw.FIELD)
        for poly in panes.polygons:
            if poly.slot == field:
                self.assertTrue(poly.component.endswith("_field"), poly.component)
        self.assertEqual(sum(1 for p in panes.polygons if p.slot == field) > 0, True)
        for lod in (0, 1):
            self.assertNotIn(bw.FIELD, bw.assemble(lod)[0].slot_names_in_primitive_order(), f"LOD{lod}")

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
        self.assertEqual(bw.REVISION, "ebs-mer-unt-003-concept-v5")
        self.assertEqual(bw.REVISION, f"{bw.PRODUCTION_ID.lower()}-concept-v5")

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
        # lod0_triangles is the MAIN MESH (the import inspector's expectation); the ruling's budget is
        # the whole unit, reported separately and bounded by both ceilings.
        self.assertEqual(self.manifest["budgets"]["lod0_triangles"], bw.assemble(0)[0].triangle_count())
        self.assertEqual(self.manifest["budgets"]["lod1_triangles"], bw.assemble(1)[0].triangle_count())
        self.assertEqual(self.manifest["budgets"]["whole_unit_lod0_triangles"], bw.budget_split(0)["whole_unit_drawn"])
        self.assertEqual(self.manifest["budgets"]["whole_unit_lod1_triangles"], bw.budget_split(1)["whole_unit_drawn"])
        self.assertTrue(self.manifest["budgets"]["lod0_within_cap"])
        self.assertTrue(self.manifest["budgets"]["lod1_within_cap"])
        self.assertEqual(self.manifest["component_inventory"]["bones"]["built"], 18)
        self.assertTrue(self.manifest["packed_state"]["wider_than_tall"])
        self.assertEqual(self.manifest["material_slots"], list(bw.SLOTS))
        self.assertEqual(self.manifest["planned_unreal_folder"], "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_003/")
        self.assertEqual(self.manifest["asset_name"], "SK_EBS_MER_UNT_003")
        self.assertEqual(self.manifest["parts"], [bw.ASSET, bw.LEFT_PART, bw.RIGHT_PART, bw.BARRIER_PART])

    def test_the_manifest_states_the_component_contract_explicitly(self):
        """Owner ruling 2026-09-07: the contract lists the sub-objects, what each carries and the
        state each is visible in - in the manifest, not only in prose."""
        if self.manifest is None:
            self.skipTest("build-manifest.json not written yet")
        objects = {o["name"]: o for o in self.manifest["component_contract"]["objects"]}
        self.assertEqual(set(objects), {bw.ASSET, bw.BARRIER_PART, bw.LEFT_PART, bw.RIGHT_PART})
        self.assertEqual(objects[bw.BARRIER_PART]["visible_in_states"], ["DEPLOYED"])
        for name in (bw.ASSET, bw.LEFT_PART, bw.RIGHT_PART):
            self.assertIn("PACKED", objects[name]["visible_in_states"], name)
        for entry in objects.values():
            self.assertTrue(entry["carries"], entry["name"])
            self.assertGreater(entry["triangles"], 0, entry["name"])
        contract = self.manifest["visibility_contract"]
        self.assertEqual(contract["component"], bw.BARRIER_PART)
        self.assertFalse(contract["enforceable_here"])
        split = self.manifest["budgets"]["split"]
        self.assertTrue(split["lod0"]["whole_unit_within_rel_art_028"])
        self.assertTrue(split["lod1"]["whole_unit_within_rel_art_028"])

    def test_the_manifest_records_the_barrier_pane_exports(self):
        if self.manifest is None:
            self.skipTest("build-manifest.json not written yet")
        paths = {entry["path"] for entry in self.manifest["outputs"]}
        for lod in (0, 1):
            for extension in ("glb", "obj"):
                self.assertIn(f"export/{bw.BARRIER_PART}_LOD{lod}.{extension}", paths)
        states = {entry.get("state") for entry in self.manifest["review_assemblies"]}
        self.assertIn("packed_panes_shown", states)
        self.assertIn("deployed_frames_only", states)
        self.assertNotIn("packed_field_dark", states,
                         "no field-off material variant may stand in for a shipped state")

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


class RulingRecordFidelity(unittest.TestCase):
    """concept-v5: what this package says the owner said must BE what the owner said.

    Review found the record quoting words that exist nowhere in the authoritative ledger - an asset
    name, a naming licence ("you decide, but justify and use one name") and a skinning requirement -
    and using them to justify a "deviation from the ruling's letter" against a clause the owner never
    wrote. It also found a rewritten, imperative-mood restatement printed under the label "the
    owner's words, verbatim", with a reworded copy of the ledger entry's separate `boundary` field
    folded inside the quotation marks. These tests make that class of drift fail the suite."""

    LEDGER = os.path.join(os.path.dirname(HERE), "production-ledger.json")
    INVENTED = ("you decide, but justify and use one name", "SM_EBS_MER_UNT_003_Barrier_Panes")

    @classmethod
    def setUpClass(cls):
        with open(cls.LEDGER, encoding="utf-8") as handle:
            cls.ledger = json.load(handle)
        cls.entry = cls.ledger["owner_rulings"][0]
        cls.ruling = next(r for r in cls.entry["rulings"] if r["subject"].startswith("EBS-MER-UNT-003"))

    def test_the_recorded_ruling_text_matches_the_ledger_character_for_character(self):
        self.assertEqual(bw.OWNER_RULING, self.ruling["ruling"])
        self.assertEqual(bw.OWNER_RULING_BOUNDARY, self.entry["boundary"])
        self.assertEqual(self.entry["date"], "2026-09-07")
        self.assertIn("production-ledger.json", bw.OWNER_RULING_SOURCE)

    def test_the_boundary_is_a_separate_field_and_not_part_of_the_ruling_sentence(self):
        """concept-v4 quoted 'This is my selected implementation direction, not evidence of completed
        integration.' inside the ruling's quotation marks. The ledger has no such sentence anywhere:
        the nearest text is the entry's own `boundary` field, which is not the ruling."""
        self.assertNotIn("selected implementation direction", self.ruling["ruling"])
        self.assertNotIn("selected implementation direction", self.entry["boundary"])
        self.assertNotIn(bw.OWNER_RULING_BOUNDARY, bw.OWNER_RULING)

    def test_the_ledger_ruling_names_no_asset_and_grants_no_naming_licence(self):
        """The premise of the correction: the words concept-v4 attributed to the owner are not in
        the ledger, so `SK_` is a free engineering choice, not a documented deviation."""
        text = json.dumps(self.ledger)
        for phrase in self.INVENTED:
            self.assertNotIn(phrase, text, f"the ledger does contain {phrase!r}; the correction is wrong")
        for token in ("SM_", "SK_", "Barrier_Panes"):
            self.assertNotIn(token, self.ruling["ruling"])

    def test_the_package_quotes_the_ruling_verbatim_where_it_claims_to(self):
        with open(os.path.join(HERE, "README.md"), encoding="utf-8") as handle:
            readme = handle.read()
        flat = " ".join(readme.replace("\n", " ").replace("> ", " ").split())
        self.assertIn(" ".join(bw.OWNER_RULING.split()), flat,
                      "README must carry the ledger's ruling string verbatim")
        self.assertIn(" ".join(bw.OWNER_RULING_BOUNDARY.split()), flat,
                      "README must carry the ledger entry's boundary field verbatim, outside the ruling quote")

    def test_words_the_owner_never_wrote_appear_only_inside_a_struck_correction(self):
        """They may stay in the record - a struck decision is never silently deleted - but every
        occurrence must sit next to the strike that withdraws it."""
        for name in ("README.md", "build_bulwark.py", "make_sheets.py", "make_scenes.py"):
            path = os.path.join(HERE, name)
            if not os.path.exists(path):
                continue
            with open(path, encoding="utf-8") as handle:
                text = handle.read()
            for phrase in self.INVENTED:
                start = 0
                while True:
                    at = text.find(phrase, start)
                    if at < 0:
                        break
                    window = text[max(0, at - 900):at + 900]
                    self.assertTrue("STRUCK" in window or "Struck" in window or "~~" in window,
                                    f"{name}: {phrase!r} at {at} is stated as fact, not as a struck claim")
                    start = at + len(phrase)


class ReviewVariantsAreWhatTheyClaim(unittest.TestCase):
    """concept-v5: the three non-shipped tiles must show what the record says they show."""

    def test_the_glass_cutaway_reads_through_to_what_is_behind_the_pane(self):
        """~~concept-v4: the field polygons alone were cut, so the six opaque back plates stayed and
        the tile showed a flat charcoal plate behind every rim - an unlit, field-off barrier, and no
        operator at all.~~ Struck 2026-09-07. The cutaway now removes the back plates too, and the
        claim is measured: every frame opening reads through to something BEHIND the pane."""
        cut = bw.glass_cutaway(bw.deployed_assembly(0))
        comps = set(cut.components())
        self.assertFalse([c for c in comps if c.endswith("_backplate")], "back plates must be cut too")
        self.assertNotIn(bw.FIELD, cut.slot_names_in_primitive_order())
        for name in bw.CELL_NAMES:                       # the frames and the lit rims stay
            self.assertIn(f"{name}_frame", comps)
            self.assertIn(f"{name}_edge", comps)
        read = bw.cutaway_read_measure(0)
        self.assertEqual(read["blocked_by_a_back_plate"], 0)
        self.assertTrue(read["every_cell_reads_through"])
        self.assertGreater(read["operator_samples"], 0, "the operators must actually read through")
        self.assertGreater(read["read_through_fraction"], 0.9)
        self.assertEqual(read["concept_v4_result"]["read_through"], 0)

    def test_the_shipped_meshes_are_untouched_by_the_cutaway(self):
        """A review variant may never change what the two shipped states draw."""
        self.assertIn(bw.FIELD, bw.deployed_assembly(0).slot_names_in_primitive_order())
        for name in bw.CELL_NAMES:
            self.assertIn(f"{name}_backplate", bw.deployed_assembly(0).components())
        self.assertEqual(bw.visible_cell_pane_components(bw.deployed_assembly(0), "-X", grid=160), [])

    def test_the_pane_assembly_lod1_is_an_authored_reduction_not_a_duplicate(self):
        """~~concept-v4 shipped SK_EBS_MER_UNT_003_Barrier_Panes_LOD1 byte-identical to its LOD0:
        720/720, no reduction at all.~~ Struck 2026-09-07 (REL-ART-005 authored LOD0/LOD1)."""
        lod0 = bw.barrier_assembly(0)[0]
        lod1 = bw.barrier_assembly(1)[0]
        self.assertLess(lod1.triangle_count(), lod0.triangle_count())
        ratio = lod1.triangle_count() / lod0.triangle_count()
        self.assertLess(ratio, 0.75)
        self.assertGreater(ratio, 0.25)               # REL-ART-005.AUTH: LOD1 must not pop
        self.assertEqual(set(lod1.components()), set(lod0.components()))   # nothing disappears
        for k in range(3):                            # and the silhouette does not move
            self.assertAlmostEqual(lod0.bounds()[0][k], lod1.bounds()[0][k], delta=0.01)
            self.assertAlmostEqual(lod0.bounds()[1][k], lod1.bounds()[1][k], delta=0.01)
        for name in bw.CELL_NAMES:                    # the lit rim stays on the front plane
            self.assertAlmostEqual(lod0.component_bounds(f"{name}_edge")[1][0],
                                   lod1.component_bounds(f"{name}_edge")[1][0], delta=0.01)

    def test_the_lod1_reduction_keeps_both_certified_cues(self):
        """Fidelity check 1 (twelve lit rims frontmost from +X) and check 4 (no pane frontmost from
        -X) are certified at LOD0; the reduction must not cost either of them at LOD1."""
        for lod in (0, 1):
            visible = bw.visible_cell_pane_components(bw.deployed_assembly(lod), "+X", grid=200)
            for name in bw.CELL_NAMES:
                self.assertIn(f"{name}_edge", visible, f"LOD{lod} {name}")
                self.assertIn(f"{name}_field", visible, f"LOD{lod} {name}")
            self.assertEqual(bw.visible_cell_pane_components(bw.deployed_assembly(lod), "-X", grid=200), [])


class SheetTileLabelling(unittest.TestCase):
    """concept-v5: 'each is labelled' must be true OF THE ARTIFACT, not only of the prose.

    Review found no caption anywhere in any composed sheet (ebs_sheet.py has no text-drawing code at
    all), which left check3_packed_travel.png ending on two unlabelled tiles of a packed Bulwark with
    a fully lit barrier - the integration failure control - directly below three shipped packed
    tiles. make_sheets.py now burns the caption into the tile image before tiling AND writes a
    per-sheet sidecar naming every tile in order."""

    @classmethod
    def setUpClass(cls):
        sys.path.insert(0, HERE)
        import make_sheets  # noqa: E402
        cls.make_sheets = make_sheets
        cls.plan, cls.out_dir = make_sheets.sheets(os.path.dirname(EVIDENCE_DIR))

    def test_every_non_shipped_tile_in_every_sheet_is_captioned(self):
        for name, _cols, _cell, _crop, tiles in self.plan:
            for tile in tiles:
                caption = self.make_sheets.caption_for(tile)
                if self.make_sheets.is_non_shipped(tile):
                    self.assertTrue(caption, f"{name}: {tile} is non-shipped and carries no caption")
                    self.assertIn("NOT SHIPPED", caption)
                else:
                    self.assertIsNone(caption, f"{name}: {tile} is shipped and must not be captioned")

    def test_the_caption_blitter_can_draw_every_character_it_is_asked_for(self):
        for _variant, text in self.make_sheets.NON_SHIPPED.items():
            for ch in text:
                self.assertIn(ch, self.make_sheets.FONT_5X7, f"no glyph for {ch!r} in {text!r}")

    def test_the_failure_control_tiles_are_the_ones_that_get_the_loudest_caption(self):
        control = self.make_sheets.caption_for("x/packed_panes_shown/right.png")
        self.assertIn("INTEGRATION FAILURE CONTROL", control)
        self.assertIn("NOT SHIPPED", control)

    def test_every_sheet_has_a_sidecar_naming_every_tile_in_order(self):
        if not os.path.isdir(self.out_dir):
            self.skipTest("sheets not composed yet")
        for name, _cols, _cell, _crop, tiles in self.plan:
            sidecar = os.path.join(self.out_dir, name.replace(".png", ".tiles.json"))
            self.assertTrue(os.path.exists(sidecar), sidecar)
            with open(sidecar, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertEqual(data["sheet"], name)
            self.assertEqual(len(data["tiles"]), len(tiles))
            for entry, tile in zip(data["tiles"], tiles):
                self.assertEqual(entry["source"], os.path.basename(os.path.dirname(tile)) + "/" + os.path.basename(tile))
                self.assertEqual(entry["kind"], self.make_sheets.kind_of(tile))
                self.assertEqual(entry["caption"], self.make_sheets.caption_for(tile))
                if entry["kind"] == "non-shipped":
                    self.assertTrue(entry["caption"])

    def test_the_captioned_copies_exist_and_the_originals_are_untouched(self):
        renders = os.path.join(EVIDENCE_DIR, "renders")
        if not os.path.isdir(os.path.join(renders, "labelled")):
            self.skipTest("sheets not composed yet")
        seen = set()
        for _name, _cols, _cell, _crop, tiles in self.plan:
            for tile in tiles:
                if not self.make_sheets.is_non_shipped(tile) or tile in seen:
                    continue
                seen.add(tile)
                copy = os.path.join(renders, "labelled",
                                    f"{self.make_sheets.variant_of(tile)}__{os.path.basename(tile)}")
                self.assertTrue(os.path.exists(copy), copy)
                w0, h0, _b0, rows0 = sheet_tool.read_png(tile)
                w1, h1, _b1, rows1 = sheet_tool.read_png(copy)
                self.assertEqual((w0, h0), (w1, h1))
                band = 7 * max(3, round(w0 / 260.0)) + 4 * max(3, round(w0 / 260.0))
                self.assertNotEqual(rows0[band // 2], rows1[band // 2], "the caption band is not drawn")
                self.assertEqual(rows0[-1], rows1[-1], "the render below the band must be untouched")
        self.assertTrue(seen, "no non-shipped tile is used by any sheet")


if __name__ == "__main__":
    unittest.main(verbosity=2)
