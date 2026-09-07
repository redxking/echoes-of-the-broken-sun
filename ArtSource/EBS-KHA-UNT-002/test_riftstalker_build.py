#!/usr/bin/env python3
"""Regression checks for the EBS-KHA-UNT-002 Riftstalker concept blockout.

Author: Angelis Pseftis. Run: python3 test_riftstalker_build.py
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
import ebs_meshkit as kit  # noqa: E402
import build_riftstalker as rs  # noqa: E402

GROUND_TOLERANCE_CM = 0.05   # see README section 8: the deepest sampled frame sits 0.18 mm under


class RiftstalkerBlockout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m0, cls.s, cls.counts, cls.socks = rs.assemble(0, "baseline")
        cls.m1 = rs.assemble(1, "baseline")[0]
        cls.carapace = rs.assemble(0, "carapace_molt")[0]
        cls.striker = rs.assemble(0, "striker_molt")[0]
        cls.clips = rs.build_clips(cls.s)
        cls.inv = rs.contract_inventory(cls.m0, cls.s, cls.clips)

    def test_it_is_a_quadruped_with_no_neck(self):
        # the candidate's SKIRMISHER and MOVING FIRE views both show four legs and no neck; my card's
        # first draft said biped, and the CARD was corrected
        self.assertEqual(self.inv["quadruped"]["legs"], 4)
        self.assertEqual(self.inv["prow"]["neck_bones"], [])
        self.assertTrue(self.inv["prow"]["built"])

    def test_every_leg_has_three_driven_segments(self):
        names = {b.name for b in self.s.bones}
        for tag in rs.LEG_TAGS:
            for part in ("hip", "upper", "lower", "foot"):
                self.assertIn(f"{tag}_{part}", names)

    def test_bone_heads_are_at_the_proximal_end_of_each_segment(self):
        # a first pass put them at the distal ends, so rotating a thigh pivoted it about the knee and
        # tore the leg off the body in the death pose
        heads = {b.name: b.head for b in self.s.bones}
        for tag in rs.LEG_TAGS:
            self.assertEqual(heads[f"{tag}_upper"], heads[f"{tag}_hip"], f"{tag}: thigh pivots at the hip")
            self.assertAlmostEqual(heads[f"{tag}_lower"][2], rs.KNEE_Z, places=6, msg=f"{tag}: shin pivots at the knee")
            self.assertLess(heads[f"{tag}_foot"][2], heads[f"{tag}_lower"][2], f"{tag}: foot pivots at the ankle")

    def test_traced_proportion(self):
        m = rs.measurements(self.m0)
        self.assertAlmostEqual(m["body_length_over_height"], 1.46, delta=0.03)
        self.assertTrue(m["carapace_slopes_forward"], "the low forward posture is in the rest geometry")
        self.assertTrue(m["prow_ahead_of_front_feet"])

    def test_material_slots_and_amber_is_the_only_emissive(self):
        self.assertEqual(self.m0.slots, [rs.STRATA, rs.AMBER])
        lit = {p.component for p in self.m0.polygons if self.m0.slots[p.slot] == rs.AMBER}
        self.assertTrue(all(c.startswith("seam_") or c in ("prow_seam", "caster_slot") for c in lit), lit)
        self.assertLessEqual(rs.slot_area_fraction(self.m0, rs.AMBER), 0.15)  # REL-ART-029 ceiling

    def test_the_caster_is_a_shoulder_slot_aimed_past_the_prow(self):
        muzzle = {s.name: s for s in self.m0.sockets}["VFX_Muzzle_Shard_01"]
        self.assertEqual(self.socks["VFX_Muzzle_Shard_01"], "caster_pitch")
        self.assertGreater(muzzle.position[2], rs.H - 60.0, "the caster sits high on the shoulder")
        prow_tip_z = self.m0.component_bounds("prow")[0][2]
        self.assertGreater(muzzle.position[2], prow_tip_z, "its line clears the prow")

    def test_it_fires_while_moving_and_the_two_do_not_fight(self):
        by_name = {c.name: c for c in self.clips}
        move, fire = by_name["move"], by_name["fire_on_the_move"]
        for tag in rs.LEG_TAGS:
            self.assertEqual([k.rotation_deg for k in move.tracks[f"{tag}_upper"]],
                             [k.rotation_deg for k in fire.tracks[f"{tag}_upper"]],
                             "the gait is identical whether or not it is firing")
        self.assertIn("caster_pitch", fire.tracks)
        self.assertNotIn("caster_pitch", move.tracks)

    def test_the_stride_is_pitch_not_yaw(self):
        # ebs_skelkit takes (pitch, yaw, roll); writing a stride into the yaw slot swung the legs
        # sideways instead of stepping, which is how the first pass was wrong
        move = {c.name: c for c in self.clips}["move"]
        for tag in rs.LEG_TAGS:
            for k in move.tracks[f"{tag}_upper"]:
                self.assertEqual(k.rotation_deg[1], 0.0, "no yaw in a stride")
                self.assertEqual(k.rotation_deg[2], 0.0, "no roll in a stride")
            self.assertTrue(any(k.rotation_deg[0] != 0.0 for k in move.tracks[f"{tag}_upper"]))

    def test_no_frame_of_any_clip_passes_through_the_ground(self):
        for clip in self.clips:
            for i in range(49):
                mesh = rs.posed(0, rs.sample_pose(clip, i / 48.0))
                self.assertGreaterEqual(mesh.bounds()[0][2], -GROUND_TOLERANCE_CM,
                                        f"{clip.name} at {i / 48.0:.3f}")

    def test_the_descent_solver_measures_the_real_rig(self):
        # _safe_drop poses the actual mesh; two earlier analytic attempts got the pitch sign wrong and
        # bisected a non-monotonic function, and both produced folds that drove the feet underground
        shallow = rs._safe_drop(0.0, 0.0)
        deep = rs._safe_drop(20.0, 90.0)
        self.assertGreater(deep, shallow, "a folded leg supports a deeper settle than a straight one")
        self.assertGreater(deep, 30.0)

    def test_molt_states_are_visible_on_the_unit(self):
        base = set(self.m0.components())
        self.assertTrue(set(self.carapace.components()) - base, "a carapace molt adds visible plate")
        self.assertTrue(set(self.striker.components()) - base, "a striker molt adds visible vanes")
        self.assertGreater(self.carapace.triangle_count(), self.m0.triangle_count())

    def test_clips_are_frame_aligned(self):
        for c in self.clips:
            self.assertTrue(skel.is_frame_aligned(c.duration_s), c.name)
        self.assertEqual(sorted(c.name for c in self.clips),
                         ["death", "fire_on_the_move", "idle", "molt", "move", "sidestep"])

    def test_sockets(self):
        self.assertEqual(sorted(s.name for s in self.m0.sockets), sorted(rs.SOCKETS))
        # the authoritative card REL-ART-005.KA.RIFTSTALKER fixes these three names
        for required in ("VFX_Muzzle_Shard_01", "VFX_Molt_Origin_Base", "Target_Hitbox_Center"):
            self.assertIn(required, rs.SOCKETS, "a card-required socket name is missing")
        self.assertEqual(self.socks["Molt_Striker_Anchor"], "caster_pitch")
        self.assertEqual(self.socks["VFX_Molt_Origin_Base"], "body")
        # verify the alias bindings: every alias must resolve to a socket that exists, must not shadow
        # a real socket name, and must land on the same bone the card's socket does
        emitted = {s.name: s for s in self.m0.sockets}
        for old, new in rs.SOCKET_ALIASES.items():
            self.assertIn(new, rs.SOCKETS, f"alias {old} points at a socket that does not exist")
            self.assertNotIn(old, rs.SOCKETS, f"alias {old} must not also be an emitted socket")
            self.assertIn(new, emitted, f"alias target {new} is not emitted on the mesh")
            self.assertIn(self.socks[new], {b.name for b in self.s.bones},
                          f"alias target {new} is bound to a bone that does not exist")

    def test_the_vertex_id_channels_are_verified_through_export_and_import(self):
        # the card requires three; presence of the attribute alone is not evidence (owner ruling 2026-09-07)
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        v = data["vertex_id_channels"]
        self.assertTrue(v["verified"])
        self.assertNotIn("blocked_by", v, "the COLOR_0 blocker is cleared")
        self.assertEqual(v["verification"]["export"]["result"], "PASS")
        self.assertEqual(v["verification"]["import"]["result"], "PASS")
        self.assertIn("RENDERED IN ENGINE", v["verification"]["material"]["result"])
        material = v["verification"]["material"]
        self.assertIn("no Time or Panner node", material["graph"])
        self.assertIn("RESTARTS from zero", material["interruption_and_restoration"])
        self.assertIn("CONSUMES authoritative progress", material["timing"])
        self.assertIn("RUNNING simulation", material["outstanding"])
        self.assertTrue(v["still_outstanding"])
        self.assertEqual(sorted(v["phase_read"]), sorted(list(rs.STATES) + ["transition"]))
        self.assertIn("IN NO CHANNEL", v["team_ownership"])
        self.assertIn("8-bit", v["verification"]["quantisation"])

    def test_the_import_claim_is_about_shipped_lods_not_engine_reduction(self):
        # "LOD generation altered nothing" was unsupported: nothing generated LODs (owner ruling 2026-09-07)
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        lods = data["vertex_id_channels"]["verification"]["lods_tested"]
        self.assertIn("ENGINE LOD REDUCTION", lods["what_was_NOT_tested"])
        self.assertIn("withdrawn", lods["what_was_NOT_tested"])
        self.assertIn("spatial_placement", data["vertex_id_channels"]["verification"]["import"])

    def test_the_inspection_method_is_not_claimed_to_be_the_only_one(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        method = data["vertex_id_channels"]["verification"]["import"]["method"]
        self.assertIn("not the only", method)

    def test_the_team_identification_mechanism_is_recorded(self):
        # owner ruling: record it BEFORE material acceptance
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        team = data["vertex_id_channels"]["team_identification"]
        self.assertIn("TeamColor", team["mechanism"])
        self.assertIn("NOT BUILT", team["status"])

    def test_the_sweep_convention_completes_at_both_ends(self):
        # higher R sweeps earlier; progress 0 sweeps nothing and progress 1 sweeps the R = 0 legs
        HIGH, LOW = 1.02, -0.02
        quantise = lambda x: round(x * 255) / 255.0
        levels = sorted({quantise(v[0]) for state in rs.STATES
                         for v in rs.assemble(0, state)[0].vertex_colors.values()})
        threshold = lambda p: HIGH - p * (HIGH - LOW)
        swept = lambda p: [l for l in levels if l >= threshold(p) - 1e-9]
        self.assertEqual(swept(0.0), [], "progress 0 must sweep nothing")
        self.assertEqual(len(swept(1.0)), len(levels), "progress 1 must sweep everything, legs included")
        self.assertIn(0.0, swept(1.0), "the R = 0 legs must complete")
        # the strict '>' the first material used left the legs behind for ever
        self.assertFalse([l for l in levels if l > threshold(1.0)] == levels and 0.0 not in levels)
        # intermediate boundaries must stay resolvable after 8-bit quantisation
        crossings = sorted((HIGH - l) / (HIGH - LOW) for l in levels)
        margins = [b - a for a, b in zip(crossings, crossings[1:])]
        self.assertGreater(min(margins) * 80.0, 1.0, "levels must be more than a tick apart in the 80-tick window")
        self.assertGreater(min(margins), 1.0 / 255.0, "and further apart than the quantisation step")

    def test_the_rig_discrepancy_is_recorded_not_resolved(self):
        path = os.path.join(HERE, "build-manifest.json")
        if not os.path.exists(path):
            self.skipTest("no manifest")
        with open(path, encoding="utf-8") as handle:
            data = json.load(handle)
        rig = data["authoritative_card"]["compliance"]["rig"]
        self.assertIn("NON-COMPLIANT", rig)
        self.assertIn("NOT amended", rig)
        self.assertEqual(len(self.s.bones), 22)

    def test_provisional_card_budget(self):
        path = os.path.join(HERE, "build-manifest.json")
        lod0 = lod1 = 0
        if os.path.exists(path):
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
            self.assertIn("REL-ART-005.KA.RIFTSTALKER", data["authoritative_card"]["card"])
            self.assertIn("NON-COMPLIANT", data["authoritative_card"]["compliance"]["rig"])
            self.assertIn("PENDING", data["acceptance"]["technical"])
            self.assertIn("FOUR legs", data["anatomy_correction"]["why_wrong"])
            for row in data.get("outputs", []):
                if str(row.get("path", "")).endswith(".glb") and "triangles" in row:
                    if int(row["lod"]) == 0:
                        lod0 += row["triangles"]
                    else:
                        lod1 += row["triangles"]
        worst = max(lod0 or self.m0.triangle_count(), self.carapace.triangle_count(),
                    self.striker.triangle_count())
        lod1 = lod1 or self.m1.triangle_count()
        self.assertLessEqual(worst, 7500)
        self.assertLessEqual(lod1, 3200)
        self.assertLess(self.m1.triangle_count(), self.m0.triangle_count())

    def test_lod1_keeps_four_legs_and_the_caster(self):
        inv = rs.contract_inventory(self.m1, self.s, self.clips)
        self.assertEqual(inv["quadruped"]["legs"], 4)
        self.assertTrue(inv["shard_caster"]["housing"])

    def test_states_are_declared(self):
        self.assertEqual(tuple(rs.STATES), ("baseline", "carapace_molt", "striker_molt"))
        with self.assertRaises(ValueError):
            rs.assemble(0, "dead")

    def test_deterministic_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = rs.assemble(0, "baseline")[0].write_glb(os.path.join(tmp, "a.glb"))
            b = rs.assemble(0, "baseline")[0].write_glb(os.path.join(tmp, "b.glb"))
            self.assertEqual(a, b)

    def test_revision_string(self):
        self.assertEqual(rs.REVISION, f"{rs.PRODUCTION_ID.lower()}-concept-v1")


    # --- vertex ID channels (card .MESH_PROP; owner ruling 2026-09-07) -------------------------
    def test_every_component_has_a_deliberate_channel_mapping(self):
        for state in rs.STATES:
            m = rs.assemble(0, state)[0]
            self.assertEqual(sorted(m.vertex_colors), sorted(m.components()), state)
        with self.assertRaises(ValueError):
            rs.vertex_colors_for(["a_part_nobody_mapped"])

    def test_team_ownership_is_in_no_channel(self):
        # owner ruling: team ownership must stay distinguishable from adaptation state
        src = open(os.path.join(HERE, "build_riftstalker.py"), encoding="utf-8").read()
        window = src[src.index("MOLT_SWEEP"):src.index("def build_skeleton")]
        self.assertNotIn("team", window.lower().replace("team colour stays", "").replace("for team", ""))
        for state in rs.STATES:
            m = rs.assemble(0, state)[0]
            for component, rgba in m.vertex_colors.items():
                self.assertEqual(rgba[3], 1.0, f"alpha is reserved: {component}")

    def test_the_channels_separate_the_two_adaptations(self):
        base = rs.assemble(0, "baseline")[0].vertex_colors
        cara = rs.assemble(0, "carapace_molt")[0].vertex_colors
        strk = rs.assemble(0, "striker_molt")[0].vertex_colors
        self.assertFalse([c for c, v in base.items() if v[1] > 0 or v[2] > 0], "baseline carries neither adaptation")
        self.assertTrue([c for c, v in cara.items() if v[1] > 0], "carapace sets G")
        self.assertFalse([c for c, v in cara.items() if v[2] > 0], "carapace must not set B")
        self.assertTrue([c for c, v in strk.items() if v[2] > 0], "striker sets B")
        self.assertFalse([c for c, v in strk.items() if v[1] > 0], "striker must not set G")

    def test_the_sweep_channel_is_graded_and_directional(self):
        colors = rs.assemble(0, "baseline")[0].vertex_colors
        shells = [colors[f"shell_{i + 1:02d}"][0] for i in range(rs.CARAPACE_SHELLS)]
        self.assertEqual(shells, sorted(shells, reverse=True), "the sweep runs nose to tail")
        self.assertEqual(colors["seam_01_l"][0], 1.0, "seams sweep first")
        self.assertEqual(colors["fl_foot"][0], 0.0, "legs never take the molt treatment")
        self.assertGreater(len({v[0] for v in colors.values()}), 3, "a graded sweep, not a flag")

    def test_color_0_is_written_only_when_colours_are_supplied(self):
        # every other package must export byte for byte as before
        plain = kit.Mesh("plain")
        plain.box((0.0, 0.0, 5.0), (10.0, 10.0, 10.0), plain.slot("MI_Test"), "box")
        with tempfile.TemporaryDirectory() as tmp:
            before = plain.write_glb(os.path.join(tmp, "a.glb"))
            plain.vertex_colors = {"box": (1.0, 0.0, 0.0, 1.0)}
            after = plain.write_glb(os.path.join(tmp, "b.glb"))
        self.assertNotEqual(before, after, "supplying colours must change the file")

    def test_the_exported_glb_carries_the_authored_channel_values(self):
        import struct
        path = os.path.join(HERE, "export", f"{rs.ASSET}_LOD0.glb")
        if not os.path.exists(path):
            self.skipTest("no export")
        data = open(path, "rb").read()
        length = struct.unpack("<I", data[12:16])[0]
        gltf = json.loads(data[20:20 + length])
        for primitive in gltf["meshes"][0]["primitives"]:
            self.assertIn("COLOR_0", primitive["attributes"])
        authored = {tuple(v) for v in rs.assemble(0, "baseline")[0].vertex_colors.values()}
        binary = data[20 + length + 8:]
        found = set()
        for primitive in gltf["meshes"][0]["primitives"]:
            acc = gltf["accessors"][primitive["attributes"]["COLOR_0"]]
            view = gltf["bufferViews"][acc["bufferView"]]
            offset = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
            for i in range(acc["count"]):
                found.add(tuple(round(c, 4) for c in struct.unpack_from("<4f", binary, offset + i * 16)))
        self.assertEqual(found, authored, "the GLB must carry exactly the authored values")


if __name__ == "__main__":
    unittest.main(verbosity=1)
