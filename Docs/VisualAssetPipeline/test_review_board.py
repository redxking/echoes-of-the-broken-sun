"""Review persistence checks. Author: Angelis Pseftis."""
import pathlib, tempfile, unittest
from review_board import Store, read

class ReviewStoreTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.path=pathlib.Path(self.tmp.name)/'review.json'
        self.store=Store(self.path,'snapshot',[{'id':'concept','source_sha256':'source'}])
    def test_roundtrip_history_and_clear(self):
        self.store.update({'revision':0,'id':'concept','choice':'REWORK','notes':'Keep silhouette'})
        self.store.update({'revision':1,'id':'concept','choice':None,'notes':'Keep silhouette'})
        state=Store(self.path,'snapshot',[]).get()
        self.assertEqual(state['selections']['concept']['source_sha256'],'source')
        self.assertIsNone(state['selections']['concept']['choice'])
        self.assertEqual(state['history'][0]['record']['choice'],'REWORK')
        self.assertEqual(state['revision'],2)
    def test_stale_write_preserves_record(self):
        self.store.update({'revision':0,'kind':'family_notes','notes':'Related states'})
        before=self.path.read_bytes()
        with self.assertRaises(RuntimeError):self.store.update({'revision':0,'kind':'family_notes','notes':'stale'})
        self.assertEqual(before,self.path.read_bytes())
    def test_invalid_input_preserves_record(self):
        before=self.path.read_bytes()
        for body in [[],{'revision':0,'id':'unknown','choice':'KEEP'}, {'revision':0,'id':'concept','choice':'APPROVE'}, {'revision':0,'kind':'family_notes','notes':'x'*6001}]:
            with self.assertRaises(ValueError):self.store.update(body)
            self.assertEqual(before,self.path.read_bytes())
    def test_snapshot_drift_rejected(self):
        with self.assertRaises(ValueError):Store(self.path,'different',[])
    def test_empty_initial_state(self):
        self.assertEqual(read(self.path)['selections'],{})
        self.assertEqual(read(self.path)['author'],'Angelis Pseftis')

if __name__=='__main__':unittest.main()
