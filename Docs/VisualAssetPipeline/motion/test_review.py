"""Author: Angelis Pseftis. Negative evidence checks for candidate audit."""
import json,pathlib,tempfile,unittest
from build_review import build, apply_decisions, digest
class AuditTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.p=pathlib.Path(self.tmp.name);(self.p/'one').mkdir();self.img=self.p/'one/art.png';self.img.write_bytes(b'fixture');self.receipt=self.p/'one/generation-record.json';self.receipt.write_text(json.dumps(dict(package_id='P',output='art.png',limitations=['gap'])));self.pkg=self.p/'packages.json';self.pkg.write_text(json.dumps({'packages':[dict(package_id='P',subject='Test',domain='MER',concept_inputs=[],canon_production_brief=[],gameplay_contract={},object_type='UNT',production_maturity='NOT_STARTED')]}))
 def test_retains_gap_and_no_promotion(self):
  r=build(self.pkg,self.p)['packages'][0];self.assertEqual(r['gaps'][0]['status'],'OPEN');self.assertEqual(r['production_maturity'],'NOT_STARTED')
 def test_missing_image_refused(self):
  self.img.unlink()
  with self.assertRaises(ValueError):build(self.pkg,self.p)
 def test_modified_image_refused(self):
  r=json.loads(self.receipt.read_text());r['output_sha256']='wrong';self.receipt.write_text(json.dumps(r))
  with self.assertRaises(ValueError):build(self.pkg,self.p)
 def test_duplicate_receipt_refused(self):
  (self.p/'two').mkdir();(self.p/'two/generation-record.json').write_bytes(self.receipt.read_bytes())
  with self.assertRaises(ValueError):build(self.pkg,self.p)
 def test_missing_receipt_refused(self):
  self.receipt.unlink()
  with self.assertRaises(KeyError):build(self.pkg,self.p)
 def decision_fixture(self, status='DESIGN_DECIDED'):
  packages=build(self.pkg,self.p)['packages'];g=packages[0]['gaps'][0]
  e=dict(gap_id=g['id'],original_finding=g['finding'],status=status,decision='candidate choice',next_evidence='rig review')
  f=self.p/'decisions.json';f.write_text(json.dumps({'items':[e]}));return packages,e,f
 def test_decision_does_not_accept(self):
  ps,e,f=self.decision_fixture();apply_decisions(ps,f);self.assertEqual(ps[0]['gaps'][0]['acceptance'],'NOT_ACCEPTED')
 def test_decision_drift_refused(self):
  ps,e,f=self.decision_fixture();e['original_finding']='changed';f.write_text(json.dumps({'items':[e]}))
  with self.assertRaises(ValueError):apply_decisions(ps,f)
 def test_duplicate_decision_refused(self):
  ps,e,f=self.decision_fixture();f.write_text(json.dumps({'items':[e,e]}))
  with self.assertRaises(ValueError):apply_decisions(ps,f)
 def test_correction_without_evidence_refused(self):
  ps,e,f=self.decision_fixture('CORRECTED_REFERENCE')
  with self.assertRaises(ValueError):apply_decisions(ps,f)
 def test_bound_correction_allowed(self):
  ps,e,f=self.decision_fixture('CORRECTED_REFERENCE');e['evidence']={'path':str(self.img),'sha256':digest(self.img)};f.write_text(json.dumps({'items':[e]}));apply_decisions(ps,f);self.assertEqual(ps[0]['gaps'][0]['status'],'CORRECTED_REFERENCE')
if __name__=='__main__':unittest.main()
