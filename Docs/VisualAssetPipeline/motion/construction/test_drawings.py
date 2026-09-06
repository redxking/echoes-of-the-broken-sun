"""Author: Angelis Pseftis. Topology checks, not rig collision validation."""
import math,unittest
from build_drawings import make,svg
class GeometryTests(unittest.TestCase):
 def setUp(self): self.assets={a['subject']:{c['id']:c for c in a['components']} for a in make()}
 def test_unique_finite_components(self):
  for a in make():
   self.assertEqual(len(a['components']),len({c['id'] for c in a['components']}))
   for c in a['components']:
    self.assertTrue(all(math.isfinite(x) for v in c['vertices'] for x in v))
    self.assertTrue(all(0<=i<len(c['vertices']) for edge in c['edges'] for i in edge))
 def test_tripod_below_bearing(self):
  a=self.assets['Aegis Post'];legs=[v for k,v in a.items() if k.startswith('leg_')]
  self.assertEqual(len(legs),3);self.assertEqual(sum(k.startswith('foot_') for k in a),3)
  self.assertLess(max(v[2] for c in legs for v in c['vertices']),min(v[2] for v in a['azimuth_bearing']['vertices']))
 def test_foundry_straight_axis(self):
  a=self.assets['Array Foundry']
  for k in ['rail','intake_apron','output_apron','intake_lintel','output_lintel']:
   self.assertAlmostEqual(sum(v[0] for v in a[k]['vertices'])/8,0)
 def test_concordance_twelve_panes(self):
  a=self.assets['Concordance'];self.assertEqual({k for k in a if k.startswith('pair_')},{f'pair_{i}_pane_{j}' for i in range(1,7) for j in [1,2]})
 def test_interval_grounded_and_separated(self):
  a=self.assets['Interval Loom']
  for axis in [1,2]:
   for segment,end in [(1,slice(0,4)),(16,slice(4,8))]:
    self.assertAlmostEqual(sum(v[2] for v in a[f'span_{axis}_segment_{segment}']['vertices'][end])/4,0)
  crown1=max(v[2] for k,c in a.items() if k.startswith('span_1') for v in c['vertices'])
  crown2=min(v[2] for v in a['span_2_segment_8']['vertices'][4:])
  self.assertGreater(crown2-crown1,.1)
 def test_power_link_inventory(self):
  a=self.assets['Power Link'];self.assertEqual({k for k in a if k.startswith('access_panel')},{f'access_panel_{i:02}' for i in range(1,5)})
  self.assertEqual(sum(k.startswith('conduit_') for k in a),4)
 def test_candidate_boundary_on_every_sheet(self):
  for a in make():self.assertIn('NOT A PRODUCTION MODEL',svg(a));self.assertIn('Angelis Pseftis',svg(a))
if __name__=='__main__':unittest.main()
