"""Geometry-contract checks for original world kits; no Unreal runtime claims."""
import importlib.util
import math
from pathlib import Path
import sys
import types
import unittest

spec = importlib.util.spec_from_file_location("world_kits", Path(__file__).parents[2] / "Scripts/echoes_world_kits.py")
kits = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kits)


class Mesh:
    def __init__(self): self.solids, self.buffers = [], []
    def append_simple_extrude_polygon(self, options, transform, points, height, *args):
        self.solids.append((options, transform, points, height))
    def append_buffers_to_mesh(self, buffers, material_id=0):
        self.buffers.append((buffers,material_id))


class WorldKitContract(unittest.TestCase):
    def setUp(self):
        self.previous = sys.modules.get("unreal")
        sys.modules["unreal"] = types.SimpleNamespace(
            Vector=lambda *x: x, Vector2D=lambda *x: x, IntVector=lambda *x: x, Rotator=lambda *x: x,
            GeometryScriptSimpleMeshBuffers=lambda **x: x,
            Transform=lambda **x: x, GeometryScriptPrimitiveOptions=lambda **x: x,
            GeometryScriptPrimitiveOriginMode=types.SimpleNamespace(BASE=0))

    def tearDown(self):
        if self.previous is None: del sys.modules["unreal"]
        else: sys.modules["unreal"] = self.previous

    def test_ground_height_and_formation_footprints(self):
        for kind in kits.KINDS:
            for high in (False, True):
                for ground in (False, True):
                    with self.subTest(kind=kind, high=high, ground=ground):
                        mesh = Mesh(); kits.build(mesh, high, kind, ground)
                        self.assertTrue(mesh.solids or mesh.buffers)
                        for buffers, material in mesh.buffers:
                            self.assertIn(material, range(4))
                            for x,y,z in buffers['vertices']:
                                self.assertLessEqual(abs(x),100)
                                self.assertLessEqual(abs(y),100)
                                if ground: self.assertLessEqual(z,20)
                            for normal in buffers['normals']:
                                self.assertAlmostEqual(sum(n*n for n in normal),1)
                        for options, transform, points, height in mesh.solids:
                            self.assertIn(options['material_id'], range(4))
                            self.assertGreater(height, 0)
                            x,y,z = transform['location']
                            if ground: self.assertLessEqual(z + height, 20)
                            yaw = math.radians(transform.get('rotation', (0,0,0))[1])
                            for px,py in points:
                                self.assertLessEqual(abs(x+px*math.cos(yaw)-py*math.sin(yaw)), 100)
                                self.assertLessEqual(abs(y+px*math.sin(yaw)+py*math.cos(yaw)), 100)
                            area = sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(points, points[1:]+points[:1]))
                            self.assertGreater(area, 0, 'outward winding required')

    def test_shelf_has_no_emissive_bars_or_raised_seams(self):
        for high in (False, True):
            mesh = Mesh(); kits.shelf(mesh, high)
            for buffers, material in mesh.buffers:
                self.assertNotEqual(material,3)
                self.assertTrue(all(p[2] <= 39 for p in buffers['vertices']))
                self.assertTrue(all(abs(p[0]) <= 390 and abs(p[1]) <= 390 for p in buffers['vertices']))
            for options, transform, points, height in mesh.solids:
                self.assertNotEqual(options['material_id'], 3)
                self.assertLessEqual(transform['location'][2] + height, 39)
                # Detect crossing edges, which extrusion can otherwise turn into holes.
                def cross(a,b,c): return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
                edges = list(zip(points, points[1:]+points[:1]))
                for i,(a,b) in enumerate(edges):
                    for c,d in edges[i+1:]:
                        self.assertFalse(cross(a,b,c)*cross(a,b,d)<0 and cross(c,d,a)*cross(c,d,b)<0)

    def test_unreal_front_face_normals_and_uv_area(self):
        builders = [(lambda mesh, high, k=kind, g=ground: kits.build(mesh,high,k,g))
                    for kind in kits.KINDS for ground in (False,True)]
        builders += [kits.walk_surface, kits.shelf, kits.sun_crust, kits.radial_paving]
        for builder in builders:
            for high in (False,True):
                mesh=Mesh(); builder(mesh,high)
                for buffers,_ in mesh.buffers:
                    for a,b,c in buffers['triangles']:
                        p,q,r=(buffers['vertices'][i] for i in (a,b,c))
                        u,v=[q[i]-p[i] for i in range(3)],[r[i]-p[i] for i in range(3)]
                        # UE GeometryCore uses edge2 cross edge1, not edge1 cross edge2.
                        normal=(v[1]*u[2]-v[2]*u[1],v[2]*u[0]-v[0]*u[2],v[0]*u[1]-v[1]*u[0])
                        self.assertGreater(sum(normal[i]*buffers['normals'][a][i] for i in range(3)),0)
                        p,q,r=(buffers['uv0'][i] for i in (a,b,c))
                        self.assertGreater(abs((q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])),1e-12)

    def test_recipe_repeatability(self):
        for kind in kits.KINDS:
            a,b = Mesh(),Mesh()
            kits.build(a,True,kind); kits.build(b,True,kind)
            self.assertEqual(a.solids,b.solids)
            self.assertEqual(a.buffers,b.buffers)


if __name__ == '__main__': unittest.main()
