"""Author: Angelis Pseftis. Negative evidence checks for candidate audit."""
import json,pathlib,tempfile,unittest
from build_review import build
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
if __name__=='__main__':unittest.main()
