"""Author: Angelis Pseftis. Preservation integrity and refusal tests."""
import json,pathlib,tempfile,unittest,zipfile
from export_handoff import export,verify,digest
class HandoffTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.p=pathlib.Path(self.tmp.name);self.repo=self.p/'repo';self.docs=self.repo/'Docs/VisualAssetPipeline';self.docs.mkdir(parents=True);self.e=self.p/'evidence';self.e.mkdir();self.img=self.repo/'art.png';self.img.write_bytes(b'original');(self.docs/'sources.json').write_text(json.dumps({'files':[{'path':'art.png','sha256':digest(b'original')}]}));self.out=self.p/'bundle.zip'
 def test_roundtrip(self):
  r=export(self.repo,self.e,self.out);self.assertTrue(r['valid']);self.assertEqual(r['inventory_paths'],1);self.assertTrue(verify(self.out)['valid']);self.assertEqual(self.img.read_bytes(),b'original')
 def test_source_drift(self):
  self.img.write_bytes(b'changed')
  with self.assertRaises(ValueError):export(self.repo,self.e,self.out)
  self.assertFalse(self.out.exists())
 def test_no_overwrite(self):
  self.out.write_bytes(b'keep')
  with self.assertRaises(ValueError):export(self.repo,self.e,self.out)
  self.assertEqual(self.out.read_bytes(),b'keep')
 def test_output_outside_sources(self):
  with self.assertRaises(ValueError):export(self.repo,self.e,self.e/'recursive.zip')
 def test_changed_archive_refused(self):
  export(self.repo,self.e,self.out)
  with zipfile.ZipFile(self.out) as z:members={k:z.read(k) for k in z.namelist()}
  members['repository/art.png']=b'tampered'
  with zipfile.ZipFile(self.out,'w') as z:
   for k,v in members.items():z.writestr(k,v)
  with self.assertRaises(ValueError):verify(self.out)
 def test_unmanifested_member_refused(self):
  export(self.repo,self.e,self.out)
  with zipfile.ZipFile(self.out,'a') as z:z.writestr('extra',b'extra')
  with self.assertRaises(ValueError):verify(self.out)
if __name__=='__main__':unittest.main()
