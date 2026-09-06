"""Unit tests for ebs_skelkit (standard library unittest).

Author: Angelis Pseftis.

Run: /opt/homebrew/bin/python3 -m unittest ArtSource/tools/test_ebs_skelkit.py -v
"""
import json
import math
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ebs_meshkit as kit  # noqa: E402
import ebs_skelkit as skel  # noqa: E402
import skeletal_probe as probe  # noqa: E402


def probe_skeleton():
    s = skel.Skeleton("root")
    s.add("root", None, (0, 0, 0))
    s.add("hip", "root", (0, 0, 100))
    s.add("knee", "hip", (0, 0, 50))
    s.add("foot", "knee", (0, 0, 0))
    return s


def probe_mesh():
    m = kit.Mesh("SK_Test")
    m.slot("Probe")
    m.box((20, 10, 50), (20, 10, 10), 0, "box_knee")
    m.box((-25, 0, 10), (20, 20, 20), 0, "box_root")
    m.box((25, 0, 110), (20, 20, 20), 0, "box_hip")
    m.sockets.append(kit.Socket("Toe", (20, 0, 0), 0.0, "toe"))
    m.sockets.append(kit.Socket("Side", (0, 30, 100), 90.0, "side"))
    return m


def bounds_of(mesh, component):
    return mesh.component_bounds(component)


class SkeletonTests(unittest.TestCase):
    def test_add_and_parent_validation(self):
        s = skel.Skeleton("root")
        s.add("root", None, (0, 0, 0))
        with self.assertRaises(ValueError):
            s.add("root", None, (0, 0, 0))
        with self.assertRaises(ValueError):
            s.add("knee", "hip", (0, 0, 50))
        s.add("hip", "root", (0, 0, 100))
        s.add("knee", "hip", (0, 0, 50))
        self.assertEqual(s.chain_to_root("knee"), ["knee", "hip", "root"])
        self.assertEqual(s.children("root"), ["hip"])
        self.assertEqual(s.index("knee"), 2)
        self.assertEqual(s.get("hip").head, (0.0, 0.0, 100.0))
        with self.assertRaises(KeyError):
            s.get("toe")

    def test_bind_polygons_counts(self):
        m = probe_mesh()
        counts = skel.bind_polygons(m, "root", {"box_knee": "knee", "box_hip": "hip"})
        self.assertEqual(counts, {"knee": 12, "hip": 12, "root": 12})
        self.assertTrue(all(hasattr(p, "bone") for p in m.polygons))
        self.assertEqual({p.bone for p in m.polygons if p.component == "box_root"}, {"root"})


class RotatorTests(unittest.TestCase):
    def test_pitch_yaw_roll_axes(self):
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.rot_rotator((1, 0, 0), pitch_deg=90), (0, 0, 1))))
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.rot_rotator((1, 0, 0), yaw_deg=90), (0, 1, 0))))
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.rot_rotator((0, 0, 1), roll_deg=90), (0, 1, 0))))
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.rot_rotator((0, 1, 0), roll_deg=90), (0, 0, -1))))

    def test_quaternion_matches_rotation_functions(self):
        """FRotator::Quaternion reproduced here must rotate vectors exactly like rot_rotator
        (UE FQuatRotationMatrix rows: X' = (1-2(yy+zz), 2(xy+wz), 2(xz-wy)) ...)."""
        for pitch, yaw, roll in ((90, 0, 0), (0, 90, 0), (0, 0, 90), (30, -45, 60), (-20, 200, -100)):
            x, y, z, w = skel.ue_quat_from_rotator(pitch, yaw, roll)
            rows = [(1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)),
                    (2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)),
                    (2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y))]
            for axis, row in zip(((1, 0, 0), (0, 1, 0), (0, 0, 1)), rows):
                got = skel.rot_rotator(axis, pitch, yaw, roll)
                self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(got, row)), (pitch, yaw, roll, axis, got, row))

    def test_quat_rotate_and_compose_match_rotators(self):
        for pitch, yaw, roll in ((90, 0, 0), (0, 90, 0), (0, 0, 90), (30, -45, 60), (-20, 200, -100), (12.5, 170, 33)):
            q = skel.ue_quat_from_rotator(pitch, yaw, roll)
            for v in ((1, 0, 0), (0, 1, 0), (0, 0, 1), (0.3, -0.5, 0.8)):
                self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.ue_quat_rotate(q, v), skel.rot_rotator(v, pitch, yaw, roll))))
            back = skel.ue_rotator_from_quat(q)
            for v in ((1, 0, 0), (0, 1, 0), (0, 0, 1)):
                self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.rot_rotator(v, *back), skel.rot_rotator(v, pitch, yaw, roll))), (pitch, yaw, roll, back))
        # singular pitch: engine folds the rest into yaw with roll 0
        self.assertEqual([round(c, 6) for c in skel.ue_rotator_from_quat(skel.ue_quat_from_rotator(90, 15, 0))], [90.0, 15.0, 0.0])
        # outer * inner applies inner first
        inner, outer = skel.ue_quat_from_rotator(0, 15, 0), skel.ue_quat_from_rotator(90, 0, 0)
        got = skel.ue_quat_rotate(skel.ue_quat_compose(outer, inner), (1, 0, 0))
        want = skel.rot_rotator(skel.rot_rotator((1, 0, 0), yaw_deg=15), pitch_deg=90)
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(got, want)), (got, want))

    def test_gltf_quaternion_round_trip(self):
        q = skel.ue_quat_from_rotator(30, -45, 60)
        g = skel.ue_quat_to_gltf(q)
        back = (-g[0], -g[2], -g[1], g[3])  # GLTFCore ConvertQuat
        self.assertTrue(all(abs(a - b) < 1e-12 for a, b in zip(q, back)))


class PoseMeshTests(unittest.TestCase):
    def test_knee_pitch_lifts_x_extent_to_z(self):
        m = probe_mesh()
        skel.bind_polygons(m, "root", {"box_knee": "knee", "box_hip": "hip"})
        posed = skel.pose_mesh(m, probe_skeleton(), {"knee": (90, 0, 0)})
        (x0, y0, z0), (x1, y1, z1) = bounds_of(posed, "box_knee")
        # box was x 10..30, y 5..15, z 45..55 around the knee head (0,0,50): +X extent -> +Z extent
        self.assertAlmostEqual(z0, 60.0, places=6)
        self.assertAlmostEqual(z1, 80.0, places=6)
        self.assertAlmostEqual(x0, -5.0, places=6)
        self.assertAlmostEqual(x1, 5.0, places=6)
        self.assertAlmostEqual(y0, 5.0, places=6)
        self.assertAlmostEqual(y1, 15.0, places=6)
        # other bones untouched
        self.assertEqual(bounds_of(posed, "box_hip"), bounds_of(m, "box_hip"))
        self.assertEqual(len(posed.polygons), len(m.polygons))
        # normals rotate with the geometry: the former +X face now points +Z
        top = [p for p in posed.polygons if p.component == "box_knee" and p.normal[2] > 0.99]
        self.assertEqual(len(top), 1)
        # source mesh is not modified
        self.assertEqual(bounds_of(m, "box_knee"), ((10.0, 5.0, 45.0), (30.0, 15.0, 55.0)))

    def test_parent_first_composition(self):
        m = probe_mesh()
        skel.bind_polygons(m, "root", {"box_knee": "knee", "box_hip": "hip"})
        posed = skel.pose_mesh(m, probe_skeleton(), {"hip": (0, 90, 0), "knee": (90, 0, 0)})
        (x0, y0, z0), (x1, y1, z1) = bounds_of(posed, "box_knee")
        center = ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
        # knee pitch in the hip's frame, then hip yaw about the hip head: (20,10,50) -> (-10, 0, 70)
        self.assertTrue(all(abs(a - b) < 1e-6 for a, b in zip(center, (-10.0, 0.0, 70.0))), center)
        # hip box: yaw 90 about (0,0,100): (25,0,110) -> (0,25,110)
        (hx0, hy0, hz0), (hx1, hy1, hz1) = bounds_of(posed, "box_hip")
        self.assertTrue(all(abs(a - b) < 1e-6 for a, b in zip(((hx0 + hx1) / 2, (hy0 + hy1) / 2, (hz0 + hz1) / 2), (0.0, 25.0, 110.0))))

    def test_translation_and_sockets(self):
        m = probe_mesh()
        skel.bind_polygons(m, "root", {"box_knee": "knee", "box_hip": "hip"})
        posed = skel.pose_mesh(m, probe_skeleton(), {"hip": (0, 0, 0, 0, 0, -20), "_sockets": {"Toe": "foot", "Side": "hip"}})
        (_, _, z0), (_, _, z1) = bounds_of(posed, "box_knee")
        self.assertAlmostEqual(z0, 25.0)
        self.assertAlmostEqual(z1, 35.0)
        toe = next(s for s in posed.sockets if s.name == "Toe")
        side = next(s for s in posed.sockets if s.name == "Side")
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(toe.position, (20, 0, -20))))
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(side.position, (0, 30, 80))))
        self.assertAlmostEqual(side.yaw_deg, 90.0, places=9)
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(side.rotation_deg, (0.0, 90.0, 0.0))), side.rotation_deg)
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(toe.rotation_deg, (0.0, 0.0, 0.0))), toe.rotation_deg)
        # sockets not listed in _sockets are copied unchanged (no posed attributes)
        untouched = skel.pose_mesh(m, probe_skeleton(), {"hip": (0, 45, 0)}).sockets
        self.assertEqual([(s.name, s.position, s.yaw_deg) for s in untouched], [(s.name, s.position, s.yaw_deg) for s in m.sockets])

    def test_socket_rotation_under_pitched_and_rolled_bones(self):
        """A socket under a pitched bone must carry the full composed rotation, not just a summed yaw."""
        m = probe_mesh()
        skel.bind_polygons(m, "root", {"box_knee": "knee", "box_hip": "hip"})
        m.sockets.append(kit.Socket("S", (10, 0, 50), 15.0, "s"))
        posed = skel.pose_mesh(m, probe_skeleton(), {"knee": (90, 0, 0), "_sockets": {"S": "knee"}})
        s = next(x for x in posed.sockets if x.name == "S")
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(s.position, (0, 0, 60))), s.position)
        want_x = (0.0, math.sin(math.radians(15)), math.cos(math.radians(15)))  # socket +X: yaw 15 then lifted to +Z
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.ue_quat_rotate(s.quaternion, (1, 0, 0)), want_x)))
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.rot_rotator((1, 0, 0), *s.rotation_deg), want_x)), s.rotation_deg)
        self.assertAlmostEqual(s.yaw_deg, s.rotation_deg[1], places=9)
        # non-singular chain: hip yaw+roll, knee pitch; socket axes must match the chained rotators
        posed = skel.pose_mesh(m, probe_skeleton(), {"hip": (0, 40, 20), "knee": (30, 0, 0), "_sockets": {"S": "knee"}})
        s = next(x for x in posed.sockets if x.name == "S")
        for axis in ((1, 0, 0), (0, 1, 0), (0, 0, 1)):
            want = skel.rot_rotator(skel.rot_rotator(skel.rot_rotator(axis, yaw_deg=15), 30, 0, 0), 0, 40, 20)
            self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.rot_rotator(axis, *s.rotation_deg), want)), (axis, s.rotation_deg))
            self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(skel.ue_quat_rotate(s.quaternion, axis), want)))
        self.assertTrue(all(abs(a - b) < 1e-9 for a, b in zip(s.position, skel.pose_mesh(m, probe_skeleton(), {"hip": (0, 40, 20), "knee": (30, 0, 0), "_sockets": {"S": "knee"}}).sockets[-1].position)))


class WriteSkinnedGlbTests(unittest.TestCase):
    def clips(self):
        lift = skel.AnimationClip("PitchLift", 1.0)
        lift.key("knee", 0.0)
        lift.key("knee", 1.0, rotation_deg=(90, 0, 0))
        move = skel.AnimationClip("Translate", 1.0)
        move.key("hip", 0.0)
        move.key("hip", 1.0, translation_cm=(0, 0, -20))
        return [lift, move]

    def write(self, path):
        m = probe_mesh()
        skel.bind_polygons(m, "root", {"box_knee": "knee", "box_hip": "hip"})
        return m, skel.write_skinned_glb(m, probe_skeleton(), path, animations=self.clips(), include_collision=False,
                                        sockets_on_bones={"Toe": "foot", "Side": "hip"})

    def test_glb_structure(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "probe.glb")
            m, digest = self.write(path)
            self.assertEqual(len(digest), 64)
            doc = kit.read_glb(path)
            self.assertEqual(len(doc["skins"]), 1)
            skin = doc["skins"][0]
            self.assertEqual(len(skin["joints"]), 4)
            self.assertEqual(doc["accessors"][skin["inverseBindMatrices"]]["count"], 4)
            self.assertEqual(doc["accessors"][skin["inverseBindMatrices"]]["type"], "MAT4")
            names = [doc["nodes"][j]["name"] for j in skin["joints"]]
            self.assertEqual(names, ["root", "hip", "knee", "foot"])
            self.assertEqual(doc["nodes"][skin["skeleton"]]["name"], "root")
            mesh_node = doc["nodes"][0]
            self.assertEqual(mesh_node["mesh"], 0)
            self.assertEqual(mesh_node["skin"], 0)
            self.assertIn(0, doc["scenes"][0]["nodes"])
            self.assertIn(skin["skeleton"], doc["scenes"][0]["nodes"])
            vertex_count = m.triangle_count() * 3
            for prim in doc["meshes"][0]["primitives"]:
                attrs = prim["attributes"]
                pos = doc["accessors"][attrs["POSITION"]]["count"]
                self.assertEqual(pos, vertex_count)
                self.assertEqual(doc["accessors"][attrs["JOINTS_0"]]["count"], pos)
                self.assertEqual(doc["accessors"][attrs["JOINTS_0"]]["type"], "VEC4")
                self.assertEqual(doc["accessors"][attrs["JOINTS_0"]]["componentType"], 5121)
                self.assertEqual(doc["accessors"][attrs["WEIGHTS_0"]]["count"], pos)
                self.assertEqual(doc["accessors"][attrs["WEIGHTS_0"]]["componentType"], 5126)
            # joint local translations (glTF = (X, Z, Y)/100 of the local Unreal offset)
            by_name = {n["name"]: n for n in doc["nodes"]}
            self.assertEqual(by_name["hip"]["translation"], [0.0, 1.0, 0.0])
            self.assertEqual(by_name["knee"]["translation"], [0.0, -0.5, 0.0])
            self.assertEqual(by_name["foot"]["translation"], [0.0, -0.5, 0.0])
            self.assertNotIn("rotation", by_name["hip"])

    def test_animations(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "probe.glb")
            self.write(path)
            doc = kit.read_glb(path)
            anims = {a["name"]: a for a in doc["animations"]}
            self.assertEqual(sorted(anims), ["PitchLift", "Translate"])
            self.assertEqual(len(anims["PitchLift"]["samplers"]), 1)
            self.assertEqual(len(anims["PitchLift"]["channels"]), 1)
            self.assertEqual(anims["PitchLift"]["channels"][0]["target"]["path"], "rotation")
            self.assertEqual(doc["nodes"][anims["PitchLift"]["channels"][0]["target"]["node"]]["name"], "knee")
            self.assertEqual(len(anims["Translate"]["samplers"]), 2)
            self.assertEqual(sorted(c["target"]["path"] for c in anims["Translate"]["channels"]), ["rotation", "translation"])
            for a in anims.values():
                for s in a["samplers"]:
                    self.assertEqual(s["interpolation"], "LINEAR")
                    inp = doc["accessors"][s["input"]]
                    self.assertEqual(inp["min"], [0.0])
                    self.assertEqual(inp["max"], [1.0])
            rot_acc = doc["accessors"][anims["PitchLift"]["samplers"][0]["output"]]
            self.assertEqual(rot_acc["type"], "VEC4")
            self.assertEqual(rot_acc["count"], 2)
            self.assertIn("bufferView", rot_acc)

    def test_sockets_are_children_of_their_joint(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "probe.glb")
            self.write(path)
            doc = kit.read_glb(path)
            index = {n["name"]: i for i, n in enumerate(doc["nodes"])}
            self.assertIn("SOCKET_Toe", index)
            self.assertIn("SOCKET_Side", index)
            self.assertIn(index["SOCKET_Toe"], doc["nodes"][index["foot"]]["children"])
            self.assertIn(index["SOCKET_Side"], doc["nodes"][index["hip"]]["children"])
            self.assertNotIn("children", doc["nodes"][index["SOCKET_Toe"]])
            self.assertEqual(doc["nodes"][index["SOCKET_Toe"]]["translation"], [0.2, 0.0, 0.0])
            self.assertEqual(doc["nodes"][index["SOCKET_Side"]]["translation"], [0.0, 0.0, 0.3])
            self.assertNotIn("scale", doc["nodes"][index["SOCKET_Side"]])
            expected = [kit._r(c) for c in skel.rotator_gltf_quaternion(0, 90, 0)]
            self.assertEqual(doc["nodes"][index["SOCKET_Side"]]["rotation"], expected)

    def test_deterministic_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = os.path.join(tmp, "a.glb")
            b = os.path.join(tmp, "b.glb")
            _, da = self.write(a)
            _, db = self.write(b)
            self.assertEqual(da, db)
            with open(a, "rb") as fa, open(b, "rb") as fb:
                self.assertEqual(fa.read(), fb.read())

    def test_unknown_bone_rejected(self):
        m = probe_mesh()
        skel.bind_polygons(m, "root", {"box_knee": "shin"})
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(KeyError):
                skel.write_skinned_glb(m, probe_skeleton(), os.path.join(tmp, "x.glb"))

    def test_collision_nodes_optional(self):
        m = probe_mesh()
        m.collision.append(kit.CollisionBox("body", (0, 0, 50), (40, 40, 100)))
        skel.bind_polygons(m, "root", {})
        with tempfile.TemporaryDirectory() as tmp:
            with_boxes = os.path.join(tmp, "c.glb")
            skel.write_skinned_glb(m, probe_skeleton(), with_boxes, include_collision=True)
            doc = kit.read_glb(with_boxes)
            self.assertEqual([n["name"] for n in doc["nodes"] if n["name"].startswith("UBX_")], ["UBX_SK_Test_01"])
            self.assertEqual(len(doc["meshes"]), 2)
            without = os.path.join(tmp, "n.glb")
            skel.write_skinned_glb(m, probe_skeleton(), without, include_collision=False)
            self.assertEqual(len(kit.read_glb(without)["meshes"]), 1)


class ProbeHarnessTests(unittest.TestCase):
    """skeletal_probe.harness_checks: a run counts only when it was a fresh import through the override stack."""

    def good_report(self):
        return {"errors": [], "destination_cleared": True, "destination": "/Game/Echoes/SkeletalProbe",
                "destination_used": "/Game/Echoes/SkeletalProbe/Run_20260906T000000Z", "destination_unique_per_run": True,
                "destination_preexisting_assets": [], "destination_disk_files_after_clear": [],
                "destination_registry_assets_after_clear": [], "log_path": "/x/UnrealEditor-Cmd-run4.log",
                "imports": [{"name": "SK", "imported_object_paths": ["a", "b", "c", "d", "e", "f"], "expected_imported_object_count": 6}]}

    def good_log(self):
        return "LogInterchangeEngine: Display: " + probe.OVERRIDE_STACK_LINE + "\nLogInterchangeEngine: Display: Interchange import completed\n"

    def failed(self, checks):
        return [c["check"] for c in checks if not c["passed"]]

    def test_fresh_override_import_passes(self):
        self.assertEqual(self.failed(probe.harness_checks(self.good_report(), self.good_log())), [])

    def test_reimport_symptoms_fail(self):
        report = self.good_report()
        report["destination_cleared"] = False
        report["destination_disk_files_after_clear"] = ["SK_EBS_SkeletalProbe.uasset"]
        report["imports"][0]["imported_object_paths"] = ["a"]
        log = "LogEditorAssetSubsystem: Warning: " + probe.DELETE_WARNING + " but its contained assets have been removed.\n"
        names = self.failed(probe.harness_checks(report, log))
        self.assertIn("destination cleared (delete_directory not False)", names)
        self.assertIn("base destination clean on disk and in the registry before import", names)
        self.assertIn("imported object count == skeleton + mesh + clips", names)
        self.assertIn("editor log names the override pipeline stack", names)
        self.assertNotIn("inspector reported no errors", names)

    def test_errors_preexisting_and_missing_log_fail(self):
        report = self.good_report()
        report["errors"] = ["4 AnimSequence assets expected"]
        report["destination_preexisting_assets"] = ["/Game/Echoes/SkeletalProbe/Run_x/SK.SK"]
        names = self.failed(probe.harness_checks(report, None))
        self.assertIn("inspector reported no errors", names)
        self.assertIn("unique per-run destination, empty before import", names)
        self.assertIn("editor log names the override pipeline stack", names)
        legacy = {"errors": [], "imports": [{"name": "SK", "imported_object_paths": ["a"] * 6}]}  # pre-fix report shape
        self.assertIn("unique per-run destination, empty before import", self.failed(probe.harness_checks(legacy, self.good_log())))

    def test_probe_job_names_clips(self):
        with tempfile.TemporaryDirectory() as tmp:
            import contextlib
            import io
            with contextlib.redirect_stdout(io.StringIO()):
                probe.write_probe(tmp)
            with open(os.path.join(tmp, "probe-job.json"), "r", encoding="utf-8") as handle:
                job = json.load(handle)
            self.assertEqual(job["assets"][0]["clips"], ["PitchLift", "YawTurn", "RollTest", "Translate"])
            self.assertEqual(job["destination"], probe.DESTINATION)
            self.assertEqual(sorted(job["expected"][probe.NAME]["clips"]), sorted(job["assets"][0]["clips"]))


class EncodingRecordTests(unittest.TestCase):
    def test_encoding_is_verified(self):
        self.assertTrue(skel.SKELETAL_ENCODING["status"].startswith("VERIFIED"))
        for key in ("joint_node_translation", "joint_rotation_quaternion", "animation_rotation_quaternion", "animation_translation", "socket_under_joint"):
            self.assertNotIn("TBD", skel.SKELETAL_ENCODING[key])

    def test_animation_encoding_rests_on_two_fresh_runs(self):
        status = skel.SKELETAL_ENCODING["status"]
        self.assertIn("run5", status)
        self.assertIn("run6", status)
        self.assertIn("probe-checks-run5.json", skel.SKELETAL_ENCODING["evidence"])
        self.assertIn("probe-checks-run6.json", skel.SKELETAL_ENCODING["evidence"])


if __name__ == "__main__":
    unittest.main()
