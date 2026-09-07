#!/usr/bin/env python3
"""Regression checks for the EBS-MER-UNT-004 Relay Skiff blockout source.

Author: Angelis Pseftis. Run: python3 test_relay_skiff_build.py
Structural checks binding the package contract, the concept-measured proportions, the 8-bone hover rig,
budgets, states and clips to the generator. Not gate acceptance.

The proportion tests carry the ratios of concept-fidelity.md (items 1-7) with the tolerance the pixel
measurement supports; where the prose and the candidate's pixels disagree the test asserts the PIXEL
value and names the deviation (README section 8.2): the mast stands at the REAR quarter, the hull plan
is 0.333 L wide, and there are TWO lift pods per side rather than the prose's three.

Several tests assert measured concept numbers, not just internal consistency: the tail face width, the
pod stations, the dish footprint in both panels and the pale flank's share of the hull band. One test
reads README.md and asserts its rig table and emissive figures against the generator, so the source
record cannot silently desynchronise from the constants it documents.
"""
from __future__ import annotations

import math
import os
import re
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_skelkit as skel  # noqa: E402
import build_relay_skiff as rs  # noqa: E402
import pose_review as pr  # noqa: E402

L = rs.L


class RelaySkiffBlockoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mesh0, cls.skeleton, cls.counts, cls.sockets_on_bones = rs.assemble(0)
        cls.mesh1 = rs.assemble(1)[0]
        cls.unloaded0 = rs.unloaded_mesh(cls.mesh0)
        cls.clips = rs.build_clips(cls.skeleton)
        cls.inventory = rs.contract_inventory(cls.mesh0, cls.skeleton, cls.clips)
        cls.measure = rs.concept_measurements(cls.mesh0)

    # --- contract inventory ------------------------------------------------------------------
    def test_contract_component_inventory(self):
        """gap-decisions.json component_inventory plus the candidate's own parts."""
        inv = self.inventory
        for part in ("hull", "relay_mast", "dish", "archive_cradle", "secondary_emitter"):
            self.assertEqual(inv[part]["built"], 1, part)
        # PIXELS (README section 8.2): two per side, not the prose's three
        self.assertEqual(inv["lift_pods"]["built"], 4)
        self.assertEqual(inv["lift_pods"]["concept"], 4)
        self.assertEqual(inv["lift_pods"]["prose"], 6)
        self.assertEqual(inv["whip_antennas"]["built"], 2)      # item 4
        self.assertEqual(inv["cable_loop"]["built"], 1)         # item 4: slack cable from the dish to the deck
        self.assertEqual(inv["deck_rails"]["built"], 2)         # card .MESH_PROP orthogonal rail framework
        self.assertEqual(inv["tail_stubs"]["built"], 2)         # candidate side view: pipe stubs behind the squared tail
        comps = self.mesh0.components()
        for needed in ("hull_shell", "hull_prow", "hull_keel", "hull_nose_bay", "mast_column_01", "mast_head",
                       "dish_face_01", "dish_hub", "emitter_barrel", "emitter_muzzle", "deck_archive_case",
                       "hull_deck_band",
                       "deck_archive_strap", "deck_archive_status", "hull_flank_pipe_l", "hull_flank_pipe_r"):
            self.assertIn(needed, comps, needed)

    def test_two_lift_pods_per_side_at_the_measured_concept_stations(self):
        """PIXELS over prose (README section 8.2, OWNER-QUESTION 5). concept-fidelity.md item 3 says six
        pods, three per side. In the candidate's SIDE VIEW the underside drops to the pod line (y 490)
        only over x 1027-1140 and x 1298-1395 of a hull spanning x 951-1484; between them it holds
        y 470-478 with no pod body, end caps or cyan intake. The flat-run centres are 0.738 L and 0.238 L
        from the bow, so the stations are x -0.238 L and +0.262 L and there are four pods in all."""
        self.assertEqual(len(rs.POD_STATIONS_X), 2)
        self.assertEqual(self.measure["pod_count"], 4)
        for side, sign in (("l", -1.0), ("r", 1.0)):
            xs = []
            for i in (1, 2):
                bounds = self.mesh0.component_bounds(f"pod_{side}_{i:02d}_pod_body")
                self.assertIsNotNone(bounds, side)
                centre_y = (bounds[0][1] + bounds[1][1]) / 2.0
                self.assertEqual(sign > 0, centre_y > 0.0)                       # pods on their own side
                self.assertGreater(abs(centre_y), rs.HULL_HALF_W * 0.6)          # slung under the hull EDGES
                self.assertLess(bounds[0][2], rs.HULL_Z0)                        # hanging below the hull line
                xs.append((bounds[0][0] + bounds[1][0]) / 2.0)
            self.assertEqual(xs, sorted(xs))
            # measured stations, as fractions of L from the bow at x = +L/2
            for x, expected in zip(xs, (0.738, 0.238)):
                self.assertAlmostEqual((L / 2.0 - x) / L, expected, places=3)
            # and nothing hangs in the concept's pod-free stretch (x -0.14 L .. +0.14 L)
            for name in self.mesh0.components():
                if not name.startswith(f"pod_{side}_"):
                    continue
                b = self.mesh0.component_bounds(name)
                centre_x = (b[0][0] + b[1][0]) / 2.0
                self.assertGreater(abs(centre_x), 0.14 * L, name)

    def test_tail_face_and_plan_widths_match_the_measured_silhouette(self):
        """The TACTICAL SILHOUETTE holds 146-150 px of a 533 px length across the stern from x 968 to
        x 1008 (0.274-0.281 L) and 176 px (0.333 L) through the middle, bulging to 199 px (0.373 L) at
        the mast station. concept-v1 cited a 124 px tail, which only occurs 9 px from the stern tip."""
        m = self.measure
        self.assertAlmostEqual(m["tail_face_width_over_L"], 0.278, delta=0.006)
        self.assertAlmostEqual(m["hull_width_over_L"], 0.333, places=3)
        self.assertLess(m["nose_face_width_over_L"], m["tail_face_width_over_L"])
        outline = rs.hull_outline()
        tail = [p for p in outline if abs(p[0] + rs.HULL_LEN / 2.0) < 1e-9]
        self.assertEqual(len(tail), 2)
        self.assertAlmostEqual(abs(tail[0][1] - tail[1][1]) / L, m["tail_face_width_over_L"], places=6)

    def test_pale_flank_is_a_plate_on_a_deep_dark_frame(self):
        """SIDE VIEW: the pale ceramic flank runs y 415-451 inside a hull band from the deck rail top
        (y 392) to the underframe bottom (y 478) - 42% of the band. concept-v1 gave the pale plate 61%
        because hull_shell ran the full body depth."""
        m = self.measure
        self.assertAlmostEqual(m["hull_thickness_over_L"], 0.10, places=3)        # item 1: body unchanged
        self.assertAlmostEqual(m["hull_underside_over_L"], 0.09, places=3)        # item 2
        self.assertAlmostEqual(m["hull_deck_top_over_L"], 0.19, places=3)         # item 2
        self.assertLess(m["hull_pale_share_of_band"], 0.50)                       # a plate, not a slab
        self.assertGreater(m["hull_pale_share_of_band"], 0.38)
        pale = self.mesh0.component_bounds("hull_shell")
        band = self.mesh0.component_bounds("hull_deck_band")
        self.assertAlmostEqual(pale[1][2], band[0][2], places=6)                  # they meet, no gap
        self.assertAlmostEqual(band[1][2], rs.HULL_Z1, places=6)
        ceramic = self.mesh0.slot(rs.CERAMIC)
        frame = self.mesh0.slot(rs.FRAME)
        self.assertTrue(all(p.slot == ceramic for p in self.mesh0.polygons if p.component == "hull_shell"))
        self.assertTrue(all(p.slot == frame for p in self.mesh0.polygons if p.component == "hull_deck_band"))

    # --- concept proportions (concept-fidelity.md; ratios of L) --------------------------------
    def test_concept_proportions_hull(self):
        m = self.measure
        self.assertAlmostEqual(m["hull_length_over_L"], 1.0, places=6)            # item 1: L is the hull length
        self.assertAlmostEqual(m["hull_thickness_over_L"], 0.10, places=3)        # item 1: hull thickness 0.10 L
        self.assertAlmostEqual(m["hull_underside_over_L"], 0.09, places=3)        # item 2: underside at 0.09 L
        self.assertAlmostEqual(m["hull_deck_top_over_L"], 0.19, places=3)         # item 2: deck top at 0.19 L
        # DEVIATION (README section 8): the prose says 0.42 L wide; the TACTICAL SILHOUETTE measures 0.333 L
        self.assertAlmostEqual(m["hull_width_over_L"], 0.333, places=3)
        self.assertLess(m["nose_face_width_over_L"], m["tail_face_width_over_L"])  # blunt chamfered nose, squared tail
        self.assertLess(m["tail_face_width_over_L"], m["hull_width_over_L"])
        self.assertGreater(m["length_cm"] / m["width_cm"], 2.5)                    # long and thin

    def test_concept_proportions_pods_mast_dish_cradle_emitter(self):
        m = self.measure
        self.assertAlmostEqual(m["pod_length_over_L"], 0.14, places=3)             # item 3
        self.assertTrue(0.02 <= m["pod_bottom_over_L"] <= 0.04, m["pod_bottom_over_L"])
        self.assertAlmostEqual(m["mast_top_over_L"], 0.55, places=3)               # item 4: column to 0.55 L
        self.assertGreater(m["mast_above_deck_over_hull_thickness"], 3.0)          # item 4: ~3x the hull thickness above the deck
        self.assertAlmostEqual(m["dish_diameter_over_L"], 0.13, places=3)          # item 4
        self.assertAlmostEqual(m["whip_top_over_L"], 0.71, places=3)               # item 4: whips above the mast head
        self.assertAlmostEqual(m["cradle_length_over_L"], 0.42, places=3)          # item 5
        self.assertAlmostEqual(m["cradle_width_over_L"], 0.20, places=3)
        self.assertAlmostEqual(m["cradle_height_over_L"], 0.13, places=3)
        self.assertAlmostEqual(m["emitter_length_over_L"], 0.11, places=3)         # item 6

    def test_dish_footprint_and_height_against_both_candidate_panels(self):
        """The relay disc is the most identifying mark in the TACTICAL SILHOUETTE, so its footprint is
        asserted against the measured panels rather than against itself. Concept: plan 33.1 x 53.6 cm
        (disc 48.8 x 79.1 px on a 533 px hull), side 19.6 x 39.8 cm (29 x 59 px), disc centre 295 px
        above the ground line = 0.554 L. concept-v1 aimed the dish 35 deg starboard and 16 deg up and
        presented 474 cm2 of plan area against the concept's 1,393."""
        proj = self.measure["dish_projection"]
        self.assertGreater(proj["dish_elevation_deg"], 24.0)                       # not the 16 deg of concept-v1
        self.assertLess(proj["dish_elevation_deg"], 34.0)
        self.assertGreater(proj["top_projected_area_cm2"], 750.0)                  # 474 cm2 in concept-v1
        self.assertLess(proj["top_footprint_cm"][0], proj["top_footprint_cm"][1])  # a disc across the hull, as drawn
        # every panel extent within 30% of the measured concept value at the built 0.13 L diameter
        for built, concept in zip(proj["top_footprint_cm"] + proj["side_footprint_cm"],
                                  proj["concept_top_footprint_cm"] + proj["concept_side_footprint_cm"]):
            self.assertLess(abs(built - concept) / concept, 0.30, (built, concept))
        self.assertAlmostEqual(self.measure["dish_centre_over_L"], 0.554, places=3)
        head = self.mesh0.component_bounds("mast_head")
        hub = rs.dish_hub()
        self.assertGreater(hub[2], head[0][2])                                     # the disc centre is level with
        self.assertLess(hub[2], head[1][2])                                        # the mast head block, as drawn

    def test_secondary_emitter_barrel_is_the_full_item_6_length(self):
        """concept-v1 built 0.095 L of barrel while the README quoted the 0.11 L constant."""
        barrel = self.mesh0.component_bounds("emitter_barrel")
        self.assertAlmostEqual((barrel[1][0] - barrel[0][0]) / L, 0.11, places=3)
        self.assertAlmostEqual(barrel[1][0], rs.HULL_LEN / 2.0, delta=0.01)        # tip on the nose face
        self.assertAlmostEqual(self.measure["emitter_length_over_L"], 0.11, places=3)

    def test_readme_rig_table_and_emissive_figures_match_the_generator(self):
        """The README is the single authoritative source record; nothing may drift from the constants.
        concept-v1's table gave the emitter bone at (135, 0, 29.5) against a built 145.8."""
        with open(os.path.join(HERE, "README.md"), encoding="utf-8") as handle:
            text = handle.read().replace("\u2212", "-")
        heads = {b.name: b.head for b in self.skeleton.bones}
        found = set()
        for line in text.splitlines():
            match = re.match(r"^\|\s*((?:`[a-z_]+`\s*/?\s*)+)\|\s*[^|]*\|\s*\(([^)]*)\)\s*\|", line)
            if not match:
                continue
            names = re.findall(r"`([a-z_]+)`", match.group(1))
            if not names or names[0] not in heads:
                continue
            for name in names:
                sign = -1.0 if name.endswith("_l") else 1.0
                values = []
                for part in match.group(2).split(","):
                    part = part.strip().replace("\u2213", "-" if sign < 0 else "+").replace("\u00b1", "+" if sign < 0 else "-")
                    values.append(float(part))
                self.assertEqual(len(values), 3, line)
                for got, want in zip(values, heads[name]):
                    self.assertAlmostEqual(got, want, delta=0.06, msg=f"README rig table {name}: {line}")
                found.add(name)
        self.assertEqual(found, set(heads), "every bone must appear in the README rig table")
        cyan = round(self.measure["emissive_area_fraction"] * 100.0, 2)
        quoted = [float(v) for v in re.findall(r"([0-9]+\.[0-9]+)% of the surface is cyan", text)]
        quoted += [float(v) for v in re.findall(r"cyan share of the surface ([0-9]+\.[0-9]+)%", text)]
        quoted += [float(v) for v in re.findall(r"The ([0-9]+\.[0-9]+)% emissive share", text)]
        self.assertEqual(len(quoted), 3, "the README must quote the cyan share in section 4, 6 and 8.3")
        for value in quoted:
            self.assertAlmostEqual(value, cyan, places=2)

    def test_mast_stands_at_the_rear_quarter_on_the_deck(self):
        """DEVIATION (README section 8): concept-fidelity.md says 'front-left quarter'; in the candidate the
        mast is 0.26 L ahead of the SQUARED TAIL, i.e. the opposite end from the chamfered nose and its emitter."""
        m = self.measure
        self.assertAlmostEqual(m["mast_ahead_of_tail_over_L"], 0.26, places=3)
        self.assertLess(rs.MAST_X, 0.0)
        base = self.mesh0.component_bounds("mast_base")
        self.assertAlmostEqual(base[0][2], rs.HULL_Z1, delta=0.01)                 # the mast stands ON the deck
        emitter = self.mesh0.component_bounds("emitter_barrel")
        self.assertGreater(emitter[1][0], 0.0)                                     # the emitter is at the other end
        self.assertGreater(emitter[1][0] - rs.MAST_X, 0.55 * L)

    def test_archive_cradle_sits_on_the_deck_centre_with_two_straps(self):
        case = self.mesh0.component_bounds("deck_archive_case")
        self.assertAlmostEqual((case[0][0] + case[1][0]) / 2.0, 0.0, places=6)     # item 5: lashed to the deck centre
        self.assertAlmostEqual(case[0][2], rs.HULL_Z1, delta=0.01)                 # standing on the deck
        straps = self.mesh0.component_bounds("deck_archive_strap")
        self.assertGreater(straps[1][2], case[1][2])                               # straps pass over the top
        self.assertEqual(len(rs.CRADLE_STRAP_X), 2)
        self.assertLess(case[1][1] - case[0][1], rs.HULL_HALF_W * 2.0)             # narrower than the deck

    def test_hull_plan_is_a_stretched_hexagon(self):
        outline = rs.hull_outline()
        self.assertEqual(len(outline), 6)
        ys = [p[1] for p in outline]
        self.assertEqual(sum(1 for y in ys if abs(abs(y) - rs.HULL_HALF_W) < 1e-9), 4)   # parallel sides
        # convex, counted once: cross products all the same sign
        signs = set()
        for i in range(len(outline)):
            a, b, c = outline[i], outline[(i + 1) % 6], outline[(i + 2) % 6]
            cross = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0])
            signs.add(cross > 0)
        self.assertEqual(len(signs), 1)

    # --- rig, sockets, pivot -------------------------------------------------------------------
    def test_eight_bone_rig_and_hierarchy(self):
        names = [b.name for b in self.skeleton.bones]
        self.assertEqual(len(names), 8)                                            # contract bone_count_source 8
        self.assertEqual(names[0], "root")
        self.assertEqual(self.skeleton.get("hull").parent, "root")
        for child in ("mast", "pod_bank_l", "pod_bank_r", "emitter", "cradle"):
            self.assertEqual(self.skeleton.get(child).parent, "hull", child)
        self.assertEqual(self.skeleton.get("dish").parent, "mast")
        self.assertLess(self.skeleton.get("pod_bank_l").head[1], 0.0)              # port is -Y
        self.assertGreater(self.skeleton.get("pod_bank_r").head[1], 0.0)

    def test_every_polygon_bound_and_no_geometry_on_root(self):
        self.assertTrue(all(getattr(p, "bone", None) for p in self.mesh0.polygons))
        self.assertNotIn("root", self.counts)
        self.assertEqual(set(self.counts) | {"root"}, {b.name for b in self.skeleton.bones})
        for prefix, bone in (("deck_archive_", "cradle"), ("mast_", "mast"), ("dish_", "dish"), ("emitter_", "emitter")):
            for poly in self.mesh0.polygons:
                if poly.component.startswith(prefix):
                    self.assertEqual(poly.bone, bone, poly.component)

    def test_sockets_named_positioned_and_on_their_bones(self):
        names = [s.name for s in self.mesh0.sockets]
        self.assertEqual(sorted(names), ["Logistics_Relay_Beam", "Scouting_Sensor_Pod"])   # contract socket_intent
        self.assertEqual(self.sockets_on_bones, {"Scouting_Sensor_Pod": "dish", "Logistics_Relay_Beam": "mast"})
        beam = next(s for s in self.mesh0.sockets if s.name == "Logistics_Relay_Beam")
        self.assertAlmostEqual(beam.position[0], rs.MAST_X, places=6)
        self.assertAlmostEqual(beam.position[1], 0.0, places=6)
        self.assertAlmostEqual(beam.position[2], rs.MAST_TOP_Z, places=6)                  # mast head, the relay cone origin
        pod = next(s for s in self.mesh0.sockets if s.name == "Scouting_Sensor_Pod")
        hub = rs.dish_hub()
        self.assertGreater(pod.position[0], hub[0])                                        # in front of the dish face
        self.assertAlmostEqual(math.dist(pod.position, hub), 0.024 * L, places=3)          # on the dish axis
        self.assertGreater(pod.position[2], hub[2])                                        # the rest aim is tilted up
        self.assertGreater(pod.position[2], rs.HULL_Z1)                                    # up on the mast, not on the hull

    def test_pivot_is_ground_point_hover_additive(self):
        """The pivot is the GROUND point under the hull centre, not the hull: the whole asset sits above
        z = 0 with the authored 0.09 L gap, so the runtime's hover offset adds to it instead of replacing it."""
        root = self.skeleton.get("root")
        self.assertEqual(root.head, (0.0, 0.0, 0.0))
        self.assertIsNone(root.parent)
        (x0, y0, z0), (x1, y1, z1) = self.mesh0.bounds()
        self.assertGreater(z0, 0.0)                                                        # nothing touches the ground
        self.assertAlmostEqual(z0, rs.POD_BOTTOM_Z, delta=0.5)                             # the gap is the pods' hang line
        hull = self.mesh0.component_bounds("hull_shell")
        self.assertAlmostEqual(hull[0][2], 0.09 * L, places=3)                             # hull underside at 0.09 L
        prow = self.mesh0.component_bounds("hull_prow")
        self.assertAlmostEqual((hull[0][0] + prow[1][0]) / 2.0, 0.0, delta=0.5)             # hull centred over the pivot in X
        self.assertAlmostEqual((y0 + y1) / 2.0, 0.0, delta=0.01)                           # centred over the pivot in Y
        self.assertAlmostEqual(self.skeleton.get("hull").head[2], rs.HULL_Z0 + rs.HULL_THICK / 2.0, places=6)
        # an additive runtime hover offset only translates the asset: the internal gap is unchanged
        offset = 25.0
        lifted = min(p[2] + offset for poly in self.mesh0.polygons for p in poly.points)
        self.assertAlmostEqual(lifted - z0, offset, places=6)
        for clip in self.clips:
            self.assertNotIn("root", clip.tracks, clip.name)                               # no clip keys root

    # --- budgets, slots, states ----------------------------------------------------------------
    def test_budgets_and_material_slots(self):
        self.assertLessEqual(self.mesh0.triangle_count(), 5000)                             # card .MESH_PROP
        self.assertLessEqual(self.mesh1.triangle_count(), 2100)
        self.assertLess(self.mesh1.triangle_count(), self.mesh0.triangle_count())
        export0 = rs.export_mesh(self.mesh0)
        self.assertEqual(export0.slot_names_in_primitive_order(), [rs.CERAMIC, rs.FRAME])   # 2 slots (contract max)
        self.assertEqual(len(export0.slots), 2)
        self.assertIn(rs.STATUS_REVIEW, self.mesh0.slots)                                   # review-only third slot
        self.assertLessEqual(self.measure["emissive_area_fraction"], 0.08)                  # card .MAT_RULE cyan <= 8%
        self.assertGreater(self.measure["emissive_area_fraction"], 0.0)

    def test_no_collision_primitive_is_authored(self):
        """Hovering must not alter simulation passability (card .MAT_RULE); the asset owns no collision."""
        self.assertEqual(self.mesh0.collision, [])
        self.assertEqual(rs.export_mesh(self.mesh0).collision, [])

    def test_loaded_and_unloaded_states(self):
        loaded_components = set(self.mesh0.components())
        unloaded_components = set(self.unloaded0.components())
        cradle_components = {c for c in loaded_components if c.startswith("deck_archive_")}
        self.assertTrue(cradle_components)
        self.assertEqual(loaded_components - unloaded_components, cradle_components)        # only the cradle disappears
        self.assertLess(self.unloaded0.triangle_count(), self.mesh0.triangle_count())
        self.assertEqual([s.name for s in self.unloaded0.sockets], [s.name for s in self.mesh0.sockets])
        # the separate part carries the same cradle, pivoted at its deck contact centre
        part = rs.build_cradle(0)
        part_bounds = part.component_bounds("deck_archive_case")
        self.assertAlmostEqual(part_bounds[0][2], 0.0, places=6)
        self.assertAlmostEqual((part_bounds[0][0] + part_bounds[1][0]) / 2.0, 0.0, places=6)
        assembly_bounds = self.mesh0.component_bounds("deck_archive_case")
        self.assertAlmostEqual(assembly_bounds[0][2] - part_bounds[0][2], rs.HULL_Z1, places=6)

    def test_track_inventory_matches_the_frozen_contract(self):
        built = {c.name for c in self.clips}
        for track in rs.TRACK_CONTRACT:
            expected = rs.TRACK_ALIAS.get(track, track)
            self.assertIn(expected, built, track)
        self.assertEqual(len(built), len(rs.TRACK_CONTRACT))
        for name, duration in (("idle", 2.0), ("restore", 0.0)):
            self.assertAlmostEqual(next(c for c in self.clips if c.name == name).duration_s, duration, places=6)
        self.assertTrue(next(c for c in self.clips if c.name == "idle").loop)
        self.assertTrue(next(c for c in self.clips if c.name == "move").loop)
        self.assertFalse(next(c for c in self.clips if c.name == "death").loop)
        restore = next(c for c in self.clips if c.name == "restore")
        self.assertEqual(set(restore.tracks), {b.name for b in self.skeleton.bones} - {"root"})
        for keys in restore.tracks.values():
            self.assertEqual(keys[0].rotation_deg, (0.0, 0.0, 0.0))
            self.assertEqual(keys[0].translation_cm, (0.0, 0.0, 0.0))

    # --- clip behaviour ------------------------------------------------------------------------
    def _sweep(self, clip, step=0.02):
        steps = max(1, int(round(clip.duration_s / step)))
        for i in range(steps + 1):
            fraction = i / steps if steps else 0.0
            posed = skel.pose_mesh(self.mesh0, self.skeleton, pr.sample(clip, fraction))
            yield fraction, posed

    def test_every_clip_sample_stays_above_the_ground(self):
        """A hovering unit never touches the ground; the death settle rests on the pod shoes."""
        for clip in self.clips:
            floor = min(posed.bounds()[0][2] for _f, posed in self._sweep(clip))
            self.assertGreaterEqual(floor, 0.0, f"{clip.name} dips into the ground at z={floor:.2f}")
            if clip.name == "death":
                self.assertLess(floor, 2.0, "death should settle onto its pods")
            else:
                self.assertGreater(floor, 1.0, f"{clip.name} clips the ground")

    def test_idle_hover_bob_within_four_centimetres(self):
        idle = next(c for c in self.clips if c.name == "idle")
        heights = [posed.component_bounds("hull_shell")[0][2] for _f, posed in self._sweep(idle)]
        self.assertLessEqual(max(heights) - min(heights), rs.HOVER_BOB_MAX_CM)              # rig line: bob <= 4 cm
        self.assertGreater(max(heights) - min(heights), 1.0)                                # and it is actually moving
        for keys in idle.tracks.values():
            for k in keys:
                self.assertEqual(k.translation_cm[0], 0.0)
                self.assertEqual(k.translation_cm[1], 0.0)                                   # no drift along the ground

    def test_move_leans_nose_down_and_no_clip_moves_along_the_ground(self):
        move = next(c for c in self.clips if c.name == "move")
        pitches = [k.rotation_deg[0] for k in move.tracks["hull"]]
        self.assertLess(sum(pitches) / len(pitches), -1.0)                                   # canon nose-down lean
        self.assertGreater(min(pitches), -8.0)                                               # "slight"
        for clip in self.clips:
            for bone, keys in clip.tracks.items():
                for k in keys:
                    if bone == "emitter":
                        continue   # the only in-frame translation is the emitter recoil along its own barrel
                    self.assertEqual((k.translation_cm[0], k.translation_cm[1]), (0.0, 0.0), f"{clip.name}/{bone}")

    def test_turn_banks_without_yawing_the_frame(self):
        turn = next(c for c in self.clips if c.name == "turn")
        rolls = [k.rotation_deg[2] for k in turn.tracks["hull"]]
        self.assertGreater(max(rolls) - min(rolls), 4.0)                                     # a real bank
        for clip in self.clips:
            for bone in ("hull",):
                for k in clip.tracks.get(bone, []):
                    self.assertEqual(k.rotation_deg[1], 0.0, f"{clip.name}: facing stays runtime-owned (SPEC-MOV-010)")

    def test_relay_turns_the_dish_and_only_the_dish(self):
        relay = next(c for c in self.clips if c.name == "relay_extend")
        end = relay.tracks["dish"][-1].rotation_deg
        self.assertAlmostEqual(end[1], rs.RELAY_DISH_YAW_DEG, places=6)
        self.assertAlmostEqual(end[0], rs.RELAY_DISH_PITCH_DEG, places=6)
        posed_rest = skel.pose_mesh(self.mesh0, self.skeleton, pr.sample(next(c for c in self.clips if c.name == "restore"), 0.0))
        posed_relay = skel.pose_mesh(self.mesh0, self.skeleton, {**pr.sample(relay, 1.0), "_sockets": self.sockets_on_bones})
        rest_pod = next(s for s in posed_rest.sockets if s.name == "Scouting_Sensor_Pod").position
        relay_pod = next(s for s in posed_relay.sockets if s.name == "Scouting_Sensor_Pod").position
        self.assertGreater(math.dist(rest_pod, relay_pod), 5.0)                              # the sensor pod swings with the dish
        rest_yaw = next(s for s in posed_rest.sockets if s.name == "Scouting_Sensor_Pod").yaw_deg
        relay_yaw = next(s for s in posed_relay.sockets if s.name == "Scouting_Sensor_Pod").yaw_deg
        self.assertAlmostEqual(relay_yaw - rest_yaw, rs.RELAY_DISH_YAW_DEG, delta=2.0)       # it aims where the clip says
        beam_rest = next(s for s in posed_rest.sockets if s.name == "Logistics_Relay_Beam").position
        beam_relay = next(s for s in posed_relay.sockets if s.name == "Logistics_Relay_Beam").position
        self.assertLess(math.dist(beam_rest, beam_relay), 6.0)                               # the beam origin stays on the mast head
        hold = next(c for c in self.clips if c.name == "relay_hold")
        self.assertTrue(hold.loop)
        expiry = next(c for c in self.clips if c.name == "relay_expiry")
        self.assertEqual(expiry.tracks["dish"][-1].rotation_deg, (0.0, 0.0, 0.0))            # returns to the rest aim

    def test_attack_triple_is_secondary_and_recoils_along_the_barrel(self):
        execution = next(c for c in self.clips if c.name == "attack_execution")
        recoils = [k.translation_cm for k in execution.tracks["emitter"]]
        self.assertTrue(any(r[0] < 0.0 for r in recoils))                                     # recoil is backward
        self.assertTrue(all(abs(r[0]) <= 6.0 for r in recoils))                               # "clearly secondary": no drama
        for clip_name in ("attack_anticipation", "attack_execution", "attack_recovery"):
            clip = next(c for c in self.clips if c.name == clip_name)
            self.assertNotIn("cradle", clip.tracks, "the archive rack never moves for the weapon")
            for k in clip.tracks.get("emitter", []):
                self.assertLessEqual(abs(k.rotation_deg[1]), 15.0)

    def test_archive_cradle_only_moves_on_death(self):
        """Canon line 512: the archive rack never changes unless an authoritative event binds it."""
        for clip in self.clips:
            if clip.name in ("death", "restore"):
                continue
            self.assertNotIn("cradle", clip.tracks, clip.name)

    def test_death_settles_and_folds_the_mast(self):
        death = next(c for c in self.clips if c.name == "death")
        posed = skel.pose_mesh(self.mesh0, self.skeleton, pr.sample(death, 1.0))
        rest_top = self.mesh0.bounds()[1][2]
        self.assertLess(posed.bounds()[1][2], rest_top - 15.0)                                # the mast is down
        self.assertLess(posed.bounds()[0][2], 2.0)                                            # resting on the pods
        self.assertGreaterEqual(posed.bounds()[0][2], 0.0)
        mast_roll = death.tracks["mast"][-1].rotation_deg[2]
        self.assertAlmostEqual(abs(mast_roll), rs.DEATH_MAST_ROLL_DEG, places=6)

    # --- determinism ---------------------------------------------------------------------------
    def test_exports_are_deterministic(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            first = rs.build_outputs(os.path.join(a, "export"), os.path.join(a, "review"))[0]
            second = rs.build_outputs(os.path.join(b, "export"), os.path.join(b, "review"))[0]
        self.assertEqual([(o["path"], o["sha256"]) for o in first], [(o["path"], o["sha256"]) for o in second])
        self.assertEqual(len({o["path"] for o in first}), len(first))

    def test_revision_string(self):
        self.assertEqual(rs.REVISION, "ebs-mer-unt-004-concept-v3")
        self.assertEqual(rs.ASSET, "SK_EBS_MER_UNT_004")
        self.assertEqual(rs.PLANNED_FOLDER, "/Game/Echoes/Production/MER/UNT/EBS_MER_UNT_004/")


if __name__ == "__main__":
    unittest.main(verbosity=1)
