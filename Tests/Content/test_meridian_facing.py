"""Generator-facing contracts for Meridian forward-axis presentation geometry."""
import importlib.util
import math
from pathlib import Path
import sys
import types
import unittest


class _Rotator:
    def __init__(self, pitch, yaw, roll):
        self.pitch, self.yaw, self.roll = pitch, yaw, roll


class _CaptureMesh:
    def __init__(self): self.calls = []
    def append_cylinder(self, options, transform, radius, height, *args): self.calls.append(("cylinder", transform, radius, height))
    def append_cone(self, options, transform, base_radius, top_radius, height, *args): self.calls.append(("cone", transform, base_radius, top_radius, height))
    def append_sphere_lat_long(self, options, transform, radius, *args): self.calls.append(("sphere", transform, radius))
    def append_simple_extrude_polygon(self, *args): pass
    def append_box(self, *args): pass
    def append_torus(self, *args): pass


def _load_generator():
    unreal = types.SimpleNamespace(
        DynamicMesh=object, Vector=lambda *value: value, Vector2D=lambda *value: value, Rotator=_Rotator,
        Transform=lambda **kwargs: kwargs,
        GeometryScriptPrimitiveOptions=lambda **kwargs: kwargs,
        GeometryScriptPrimitiveOriginMode=types.SimpleNamespace(CENTER=0, BASE=1),
        GeometryScriptRevolveOptions=lambda: object())
    previous = sys.modules.get("unreal")
    sys.modules["unreal"] = unreal
    script_directory = str(Path(__file__).parents[2] / "Scripts")
    if script_directory not in sys.path: sys.path.insert(0, script_directory)
    try:
        spec = importlib.util.spec_from_file_location("echoes_meridian_facing_generator", Path(script_directory) / "generate_art_assets.py")
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        return module
    finally:
        if previous is None: del sys.modules["unreal"]
        else: sys.modules["unreal"] = previous


assets = _load_generator()


def _local_z_axis(rotator):
    """Apply UE FRotationTranslationMatrix to a local +Z primitive axis."""
    pitch, yaw, roll = map(math.radians, (rotator.pitch, rotator.yaw, rotator.roll))
    sp, cp = math.sin(pitch), math.cos(pitch)
    sy, cy = math.sin(yaw), math.cos(yaw)
    sr, cr = math.sin(roll), math.cos(roll)
    return (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp)


class MeridianFacingContract(unittest.TestCase):
    def test_bulwark_shield_cells_are_forward_facing_with_symmetric_cant(self):
        mesh = _CaptureMesh()
        assets.meridian_bulwark(mesh, True)
        shield_layers = [call for call in mesh.calls if call[0] == "cylinder"
                         and call[1]["location"][0] >= 50.0
                         and abs(call[1]["location"][1]) in (34.0, 64.0)
                         and call[1]["location"][2] in (72.0, 92.0, 114.0)]
        # Six cells, each with frame, inset, glow face, and high-detail honeycomb.
        self.assertEqual(len(shield_layers), 24)
        cant_by_center = {}
        for _, transform, *_ in shield_layers:
            axis = _local_z_axis(transform["rotation"])
            self.assertAlmostEqual(axis[2], 0.0, delta=1e-6)
            self.assertGreater(axis[0], 0.9)
            self.assertGreater(axis[1] * transform["location"][1], 0.0)
            center = (transform["location"][1] < 0.0, transform["location"][2])
            cant_by_center.setdefault(center, []).append(math.degrees(math.atan2(axis[1], axis[0])))
        self.assertEqual(len(cant_by_center), 6)
        for values in cant_by_center.values():
            self.assertTrue(all(math.isclose(value, values[0], abs_tol=1e-6) for value in values))
        for z in (72.0, 114.0, 92.0):
            self.assertAlmostEqual(cant_by_center[(False, z)][0], -cant_by_center[(True, z)][0], delta=1e-6)

    def test_lancer_muzzle_points_along_local_forward_to_its_contact_glow(self):
        mesh = _CaptureMesh()
        assets.meridian_lancer(mesh, True)
        muzzle = next(call for call in mesh.calls if call[0] == "cone" and call[1]["location"] == (124.0, -6.0, 81.0))
        contact = next(call for call in mesh.calls if call[0] == "sphere" and call[1]["location"] == (144.0, -6.0, 81.0))
        axis = _local_z_axis(muzzle[1]["rotation"])
        self.assertAlmostEqual(axis[0], 1.0, delta=1e-6)
        self.assertAlmostEqual(axis[1], 0.0, delta=1e-6)
        self.assertAlmostEqual(axis[2], 0.0, delta=1e-6)
        tip = tuple(muzzle[1]["location"][index] + axis[index] * muzzle[4] * 0.5 for index in range(3))
        self.assertLessEqual(math.dist(tip, contact[1]["location"]), 2.0)


if __name__ == "__main__": unittest.main()
