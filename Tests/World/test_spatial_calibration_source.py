"""Physical invariants of the editor calibration fixture. Author: Angelis Pseftis."""
import copy
import importlib.util
import json
import math
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('spatial', ROOT/'Scripts/generate_spatial_calibration.py')
spatial = importlib.util.module_from_spec(spec)
spec.loader.exec_module(spatial)


class SpatialCalibrationTests(unittest.TestCase):
    def setUp(self):
        self.data = spatial.load_source()
        self.plan = spatial.build_plan(self.data)
        self.actors = {a['name']:a for a in self.plan['actors']}

    def test_ramp_top_endpoints_meet_plateau_without_step(self):
        for ramp in self.plan['ramps']:
            actor = self.actors[ramp['name']]
            pitch = math.radians(actor['pitch'])
            length, _, depth = actor['size']
            for sign, expected in [(-1,ramp['top_start']),(1,ramp['top_end'])]:
                point = [actor['center'][0]+sign*length/2*math.cos(pitch)-depth/2*math.sin(pitch),
                         actor['center'][1],
                         actor['center'][2]+sign*length/2*math.sin(pitch)+depth/2*math.cos(pitch)]
                for actual, target in zip(point,expected):
                    self.assertAlmostEqual(actual,target,places=7)

    def test_clearances_are_between_wall_faces_not_pivots(self):
        for gap in self.plan['clearance']:
            left = self.actors[gap['name']+'_Wall_-1']
            right = self.actors[gap['name']+'_Wall_1']
            width = right['center'][0]-right['size'][0]/2-(left['center'][0]+left['size'][0]/2)
            self.assertEqual(width,gap['clear_width_cm'])
        self.assertEqual(self.plan['clearance'][0]['side_margin_cm'],0)
        self.assertGreater(self.plan['clearance'][1]['side_margin_cm'],0)

    def test_camera_optical_axis_hits_focus_at_requested_distance(self):
        for camera in self.plan['cameras']:
            offset = [a-b for a,b in zip(camera['focus'],camera['location'])]
            self.assertAlmostEqual(math.sqrt(sum(v*v for v in offset)),camera['distance_cm'])
            pitch,yaw = math.radians(camera['pitch']),math.radians(camera['yaw'])
            forward = [math.cos(pitch)*math.cos(yaw),math.cos(pitch)*math.sin(yaw),math.sin(pitch)]
            for v,f in zip(offset,forward):
                self.assertAlmostEqual(v/camera['distance_cm'],f)

    def test_unresolved_inner_radius_has_no_fabricated_disc(self):
        self.assertIn('Radius_zergling',self.actors)
        self.assertNotIn('Inner_zergling',self.actors)
        self.assertIn('Unknown_zergling',self.actors)
        self.assertGreater(self.actors['Inner_roach']['size'][0],self.actors['Radius_roach']['size'][0])

    def test_source_rejects_invalid_dimensions_and_evidence_promotion(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'source.json'
            for field,value in [('cell_cm',0),('cell_cm',float('nan')),('runtime_binding','campaign')]:
                data=copy.deepcopy(self.data); data[field]=value
                path.write_text(json.dumps(data))
                with self.assertRaises(ValueError):spatial.load_source(path)
            data=copy.deepcopy(self.data); data['camera']['fov_axis_verified']=True
            path.write_text(json.dumps(data))
            with self.assertRaises(ValueError):spatial.load_source(path)

    def test_reference_scale_changes_all_measurement_dimensions(self):
        data=copy.deepcopy(self.data); data['cell_cm']=100
        small=spatial.build_plan(data)
        self.assertEqual(small['cliff_step_cm'],200)
        self.assertEqual(small['ramps'][0]['width_cm'],300)
        self.assertEqual(small['clearance'][1]['clear_width_cm'],300)
        self.assertEqual(small['cameras'][0]['distance_cm'],3400)
        self.assertEqual(small['cameras'][0]['horizontal_fov'],self.plan['cameras'][0]['horizontal_fov'])


if __name__=='__main__':
    unittest.main()
