"""Author: Angelis Pseftis. Decision closure never substitutes for production evidence."""
import json,pathlib,tempfile,unittest
from validate_decisions import validate
P=pathlib.Path(__file__).resolve().parent
class DecisionTests(unittest.TestCase):
 def setUp(self):
  self.t=tempfile.TemporaryDirectory();self.addCleanup(self.t.cleanup);self.f=pathlib.Path(self.t.name)/'fixture.json';self.d=json.loads((P/'gap-decisions.json').read_text())
 def run_validation(self):self.f.write_text(json.dumps(self.d));return validate(self.f)
 def test_complete_decisions_unaccepted(self):
  r=self.run_validation();self.assertEqual(r['decided'],53);self.assertEqual(r['owner_accepted'],0);self.assertEqual(r['production_execution_pending'],53)
 def test_missing_decision_fails(self):
  next(x for x in self.d['items'] if x.get('design_resolution')).pop('design_resolution')
  with self.assertRaises(ValueError):self.run_validation()
 def test_duplicate_asset_id_fails(self):
  self.d['production_policy'][1]['reserved_production_asset_id']=self.d['production_policy'][0]['reserved_production_asset_id']
  with self.assertRaises(ValueError):self.run_validation()
 def test_acceptance_promotion_fails(self):
  self.d['production_policy'][0]['production_maturity']='VERIFIED'
  with self.assertRaises(ValueError):self.run_validation()
 def test_source_drift_fails(self):
  next(x for x in self.d['items'] if x.get('design_resolution'))['design_resolution']['candidate_sha256']='bad'
  with self.assertRaises(ValueError):self.run_validation()
 def test_unarmed_attack_track_fails(self):
  next(x for x in self.d['production_policy'] if x['subject']=='Resonant')['required_track_inventory'].append('attack_execution')
  with self.assertRaises(ValueError):self.run_validation()
 def test_master_application_claim_fails(self):
  self.d['master_amendments'][0]['status']='APPLIED'
  with self.assertRaises(ValueError):self.run_validation()
 def test_canon_text_drift_fails(self):
  next(x for x in self.d['items'] if x.get('design_resolution'))['design_resolution']['canon_sources'][0]['text']='changed'
  with self.assertRaises(ValueError):self.run_validation()
 def test_missing_amendment_fails(self):
  self.d['master_amendments'].pop()
  with self.assertRaises(ValueError):self.run_validation()
 def test_well_combat_death_fails(self):
  next(x for x in self.d['production_policy'] if x['subject']=='Future Well')['required_track_inventory'].append('death')
  with self.assertRaises(ValueError):self.run_validation()
if __name__=='__main__':unittest.main()
