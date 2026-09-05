"""Pure-Python geometry contract for the M02 migration prop source recipes."""
import importlib.util
import math
from pathlib import Path
import sys
import types
import unittest

sys.path.insert(0, str(Path(__file__).parents[2] / "Scripts"))
spec = importlib.util.spec_from_file_location("migration_props", Path(__file__).parents[2] / "Scripts/echoes_migration_props.py")
props = importlib.util.module_from_spec(spec)
spec.loader.exec_module(props)


class Mesh:
    def __init__(self): self.buffers = []
    def append_buffers_to_mesh(self, buffers, material_id=0): self.buffers.append((buffers, material_id))


class MigrationPropsContract(unittest.TestCase):
    def setUp(self):
        self.previous = sys.modules.get("unreal")
        sys.modules["unreal"] = types.SimpleNamespace(
            Vector=lambda *x: x, Vector2D=lambda *x: x, IntVector=lambda *x: x,
            GeometryScriptSimpleMeshBuffers=lambda **x: x)

    def tearDown(self):
        if self.previous is None: del sys.modules["unreal"]
        else: sys.modules["unreal"] = self.previous

    def test_bounds_materials_normals_and_winding(self):
        limits = {"ObservationSill": 122, "RootingShoulder": 120, "PassagePaving": 4}
        for kind in props.KINDS:
            for high in (False, True):
                with self.subTest(kind=kind, high=high):
                    mesh = Mesh(); props.build(mesh, high, kind)
                    self.assertTrue(mesh.buffers)
                    for buffers, material in mesh.buffers:
                        self.assertIn(material, range(4))
                        self.assertTrue(buffers["triangles"])
                        for x, y, z in buffers["vertices"]:
                            x_limit = 300 if kind == "ObservationSill" else 100
                            y_limit = 100
                            self.assertLessEqual(abs(x), x_limit)
                            self.assertLessEqual(abs(y), y_limit)
                            self.assertGreaterEqual(z, 0)
                            self.assertLessEqual(z, limits[kind])
                        for normal in buffers["normals"]:
                            self.assertAlmostEqual(sum(component*component for component in normal), 1.0)
                        for a, b, c in buffers["triangles"]:
                            self.assertNotEqual(a, b); self.assertNotEqual(b, c); self.assertNotEqual(a, c)
                            p, q, r = (buffers["vertices"][i] for i in (a,b,c))
                            u, v = [q[i]-p[i] for i in range(3)], [r[i]-p[i] for i in range(3)]
                            cross = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
                            self.assertGreater(-sum(cross[i]*buffers["normals"][a][i] for i in range(3)), 0)
                            p, q, r = (buffers["uv0"][i] for i in (a,b,c))
                            self.assertGreater(abs((q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])), 1e-10)


    def test_lods_reduce_faces_and_are_repeatable(self):
        for kind in props.KINDS:
            high, low, repeated = Mesh(), Mesh(), Mesh()
            props.build(high, True, kind); props.build(low, False, kind); props.build(repeated, True, kind)
            high_faces = sum(len(buffers["triangles"]) for buffers, _ in high.buffers)
            low_faces = sum(len(buffers["triangles"]) for buffers, _ in low.buffers)
            self.assertGreater(high_faces, low_faces)
            self.assertEqual(high.buffers, repeated.buffers)

    def test_authored_material_zones(self):
        for kind in props.KINDS:
            for high in (False, True):
                mesh = Mesh(); props.build(mesh, high, kind)
                self.assertEqual({material for _, material in mesh.buffers}, {0, 1, 2, 3})

if __name__ == "__main__": unittest.main()
