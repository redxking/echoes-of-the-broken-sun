"""Package gate regression checks. Author: Angelis Pseftis."""
import copy,json,pathlib,tempfile,unittest,shutil
from unittest.mock import patch
import prepare_packages as p
ROOT=pathlib.Path(__file__).resolve().parents[4]/'Project'
class PackageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.manifest=p.prepare(ROOT)
    def test_coverage_and_family(self):
        m=self.manifest;self.assertEqual(m['counts']['reviewed_concepts'],34);self.assertEqual(len(m['packages']),21)
        ids=[r['concept_id'] for x in m['packages'] for r in x['concept_inputs']]
        self.assertEqual(len(ids),len(set(ids)));self.assertEqual(len(m['pending_visual_reviews']),128)
        well=[x for x in m['packages'] if x['subject']=='Future Well'];self.assertEqual(len(well),1);self.assertEqual(len(well[0]['concept_inputs']),6)
        self.assertTrue(all(x['production_asset_id'] is None and not x['reference_ready'] for x in m['packages']))
    def test_exact_reproduction(self):self.assertTrue(p.validate(self.manifest,ROOT)['valid'])
    def test_tampered_stage_rejected(self):
        m=copy.deepcopy(self.manifest);m['packages'][0]['reference_ready']=True
        self.assertFalse(p.validate(m,ROOT)['valid'])
    def test_missing_package_rejected(self):
        m=copy.deepcopy(self.manifest);m['packages'].pop()
        self.assertFalse(p.validate(m,ROOT)['valid'])
    def test_changed_notes_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            folder=pathlib.Path(d)
            for name in ['concept-register.json','book-based-review.json','review-selections.json']:(folder/name).write_bytes((p.HERE/name).read_bytes())
            state=p.read(folder/'review-selections.json');state['family_notes']='New owner direction'
            (folder/'review-selections.json').write_text(json.dumps(state))
            with self.assertRaisesRegex(ValueError,'notes or choices changed'):p.prepare(ROOT,folder)
    def test_relocated_checkout(self):
        with tempfile.TemporaryDirectory() as d:
            base=pathlib.Path(d);root=base/'Project';root.mkdir()
            for name in ['Docs','Content','site']:(root/name).symlink_to(ROOT/name,target_is_directory=True)
            rel=self.manifest['book_source']['path_from_project'];book=(root/rel);book.parent.mkdir(parents=True)
            shutil.copyfile(ROOT/rel,book)
            self.assertTrue(p.validate(self.manifest,root)['valid'])
    def test_missing_book_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root=pathlib.Path(d)/'Project';root.mkdir()
            result=p.validate(self.manifest,root)
            self.assertFalse(result['valid'])
            self.assertTrue(any('book bytes unavailable' in e for e in result['errors']))
    def test_tampered_book_rejected(self):
        original=p.digest
        with patch.object(p,'digest',side_effect=lambda path:'tampered' if path.name=='book-source.docx' else original(path)):
            self.assertFalse(p.validate(self.manifest,ROOT)['valid'])
    def test_missing_excerpt_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            folder=pathlib.Path(d)
            for name in ['concept-register.json','book-based-review.json','review-selections.json']:(folder/name).write_bytes((p.HERE/name).read_bytes())
            review=p.read(folder/'book-based-review.json');review['evidence_excerpts']=[]
            (folder/'book-based-review.json').write_text(json.dumps(review))
            with self.assertRaisesRegex(ValueError,'paragraph/excerpt mismatch'):p.prepare(ROOT,folder)
    def test_changed_source_rejected(self):
        original=p.digest
        def changed(path):return 'changed' if path.name=='meridian-units.png' else original(path)
        with patch.object(p,'digest',side_effect=changed):
            self.assertFalse(p.validate(self.manifest,ROOT)['valid'])
if __name__=='__main__':unittest.main()
