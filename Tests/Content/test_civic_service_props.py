"""Geometry and terrain-envelope contract for M03 civic reserve props."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import types
import unittest

ROOT = Path(__file__).parents[2]
sys.path.insert(0, str(ROOT / "Scripts"))

spec = importlib.util.spec_from_file_location(
    "civic_service_props", ROOT / "Scripts/echoes_civic_service_props.py"
)
props = importlib.util.module_from_spec(spec)
spec.loader.exec_module(props)


class Mesh:
    def __init__(self):
        self.buffers = []

    def append_buffers_to_mesh(self, buffers, material_id=0):
        self.buffers.append((buffers, material_id))


class CivicServicePropsContract(unittest.TestCase):
    def setUp(self):
        self.previous = sys.modules.get("unreal")
        sys.modules["unreal"] = types.SimpleNamespace(
            Vector=lambda *value: value,
            Vector2D=lambda *value: value,
            IntVector=lambda *value: value,
            GeometryScriptSimpleMeshBuffers=lambda **value: value,
        )

    def tearDown(self):
        if self.previous is None:
            del sys.modules["unreal"]
        else:
            sys.modules["unreal"] = self.previous

    def test_bounds_normals_uvs_winding_and_material_zones(self):
        limits = {
            "LifeSupportBank": 330,
            "TransitSupport": 400,
            "ArchiveStack": 355,
            "ReservePaving": 4,
        }
        for kind in props.KINDS:
            for high in (False, True):
                with self.subTest(kind=kind, high=high):
                    mesh = Mesh()
                    props.build(mesh, high, kind)
                    self.assertTrue(mesh.buffers)
                    self.assertEqual({material for _, material in mesh.buffers}, {0, 1, 2, 3})
                    for buffers, material in mesh.buffers:
                        self.assertIn(material, range(4))
                        self.assertTrue(buffers["triangles"])
                        for x, y, z in buffers["vertices"]:
                            self.assertLessEqual(abs(x), 300)
                            self.assertLessEqual(abs(y), 100)
                            self.assertGreaterEqual(z, 0)
                            self.assertLessEqual(z, limits[kind])
                        for normal in buffers["normals"]:
                            self.assertAlmostEqual(sum(component * component for component in normal), 1.0)
                        for a, b, c in buffers["triangles"]:
                            self.assertEqual(len({a, b, c}), 3)
                            p, q, r = (buffers["vertices"][index] for index in (a, b, c))
                            u = [q[index] - p[index] for index in range(3)]
                            v = [r[index] - p[index] for index in range(3)]
                            cross = (
                                u[1] * v[2] - u[2] * v[1],
                                u[2] * v[0] - u[0] * v[2],
                                u[0] * v[1] - u[1] * v[0],
                            )
                            self.assertGreater(
                                -sum(cross[index] * buffers["normals"][a][index] for index in range(3)),
                                0,
                            )
                            p, q, r = (buffers["uv0"][index] for index in (a, b, c))
                            self.assertGreater(
                                abs((q[0] - p[0]) * (r[1] - p[1]) - (q[1] - p[1]) * (r[0] - p[0])),
                                1e-10,
                            )

    def test_lods_reduce_faces_and_repeat_exactly(self):
        for kind in props.KINDS:
            high, low, repeated = Mesh(), Mesh(), Mesh()
            props.build(high, True, kind)
            props.build(low, False, kind)
            props.build(repeated, True, kind)
            high_faces = sum(len(buffers["triangles"]) for buffers, _ in high.buffers)
            low_faces = sum(len(buffers["triangles"]) for buffers, _ in low.buffers)
            self.assertGreater(high_faces, low_faces)
            self.assertEqual(high.buffers, repeated.buffers)

    def test_structural_contact_planes_are_continuous(self):
        """Major caps, rails, and supports share a real contact plane."""
        contact_levels = {
            # central life-support cap -> crossbar -> inspection plate
            "LifeSupportBank": (302, 311, 323),
            # pylon -> lower rail -> upper rail
            "TransitSupport": (365, 388, 396),
            # cassette -> restraint rail; pier -> lower and upper crossbar
            "ArchiveStack": (240, 314, 331, 338),
        }
        for kind, levels in contact_levels.items():
            mesh = Mesh()
            props.build(mesh, True, kind)
            z_values = {round(vertex[2], 4) for buffers, _ in mesh.buffers for vertex in buffers["vertices"]}
            for level in levels:
                self.assertIn(level, z_values)

    def test_records_preserve_primary_sites_and_match_every_doctrine_mask(self):
        presentation = ROOT / "Content/World/Source/Presentation/m03_reserve_landmarks_v1.json"
        terrain = ROOT / "Content/World/Source/Campaign/m03_ark-city-reserve-service_v1.json"
        doc = json.loads(presentation.read_text())
        terrain_bytes = terrain.read_bytes()
        self.assertEqual(doc["terrain_source_sha256"], hashlib.sha256(terrain_bytes).hexdigest())

        compiler_spec = importlib.util.spec_from_file_location(
            "campaign_map_pack", ROOT / "Content/World/Tools/compile_campaign_map_pack.py"
        )
        compiler = importlib.util.module_from_spec(compiler_spec)
        compiler_spec.loader.exec_module(compiler)
        source = json.loads(terrain_bytes)
        parsed = compiler.parse_source(
            source,
            hashlib.sha256(terrain_bytes).hexdigest(),
            {"mission_code": "M03", "map_id": "ark-city-reserve-service", "operation_mode": "CampaignCityReserve"},
            "m03 source",
        )
        masks = [
            {index for index, passable in enumerate(variant["movement_mask"]) if not passable}
            for variant in parsed["variants"]
        ]
        primary = {(24, 10), (10, 24), (20, 20), (32, 32)}
        paving = {(record["x"], record["y"]) for record in doc["records"] if record["kind"] == "ReservePaving"}
        self.assertTrue(primary.issubset(paving))
        occupied = set()
        for record in doc["records"]:
            if record["kind"] == "ReservePaving":
                cells = {record["y"] * 64 + record["x"]}
                should_be_blocked = False
            else:
                footprint = record["footprint"]
                cells = {
                    y * 64 + x
                    for x in range(footprint["x0"], footprint["x1"] + 1)
                    for y in range(footprint["y0"], footprint["y1"] + 1)
                }
                self.assertEqual((footprint["x1"] - footprint["x0"] + 1, footprint["y1"] - footprint["y0"] + 1), (3, 1))
                self.assertIn(record["yaw"], (0, 180))
                should_be_blocked = True
            self.assertFalse(occupied.intersection(cells))
            occupied.update(cells)
            for mask in masks:
                self.assertTrue(all((cell in mask) == should_be_blocked for cell in cells))


if __name__ == "__main__":
    unittest.main()
