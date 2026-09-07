#!/usr/bin/env python3
"""Negative checks for inventory evidence integrity. Author: Angelis Pseftis."""
import copy, json, pathlib, tempfile, unittest, sys
sys.dont_write_bytecode=True
import audit
HERE=pathlib.Path(__file__).parent
class RegisterChecks(unittest.TestCase):
    def setUp(self):
        self.reg=json.loads((HERE/'concept-register.json').read_text())
        self.src=json.loads((HERE/'sources.json').read_text())
    def errors(self): return audit.validate(self.reg,self.src)['errors']
    def test_retained_register(self): self.assertEqual(self.errors(),[])
    def test_duplicate_identity_rejected(self):
        self.reg['concept_objects'].append(copy.deepcopy(self.reg['concept_objects'][0]));self.assertTrue(self.errors())
    def test_required_field_rejected(self):
        del self.reg['concept_objects'][0]['silhouette_requirements'];self.assertTrue(self.errors())
    def test_missing_source_refs_rejected(self):
        del self.reg['concept_objects'][0]['source_refs'];self.assertTrue(self.errors())
    def test_unknown_source_rejected(self):
        self.reg['concept_objects'][0]['source_refs'][0]['source_id']='EBS-SRC-missing';self.assertTrue(self.errors())
    def test_wrong_source_path_rejected(self):
        self.reg['concept_objects'][0]['source_refs'][0]['source_repository_path']='site/wrong.png';self.assertTrue(self.errors())
    def test_wrong_source_hash_rejected(self):
        self.reg['concept_objects'][0]['source_refs'][0]['source_sha256']='0'*64;self.assertTrue(self.errors())
    def test_missing_locator_rejected(self):
        self.reg['concept_objects'][0]['source_refs'][0]['locator']='';self.assertTrue(self.errors())
    def test_bad_crop_rejected(self):
        self.reg['concept_objects'][0]['source_refs'][0]['bbox_normalized']=[.7,0,.3,1];self.assertTrue(self.errors())
    def test_approval_without_evidence_rejected(self):
        self.reg['concept_objects'][0]['canon_status']='CANON_APPROVED';self.assertTrue(self.errors())
    def test_maturity_and_canon_independent(self):
        o=self.reg['concept_objects'][0];o['canon_status']='CANON_APPROVED';o['canon_approval_evidence']={'path':'synthetic-fixture.md','sha256':'0'*64,'locator':'Synthetic test decision','reviewer':'Angelis Pseftis','date':'2026-09-06','concept_id':o['concept_id'],'source_sha256':o['source_refs'][0]['source_sha256'],'decision':'CANON_APPROVED'};o['production_maturity']='NOT_STARTED';self.assertEqual(self.errors(),[])
    def test_unsupported_mapping_claim_rejected(self):
        self.reg['concept_objects'][0]['gameplay_mapping']={'status':'SUPPORTED','evidence':[]};self.assertTrue(self.errors())
    def test_missing_dependency_rejected(self):
        self.reg['concept_objects'][0]['dependencies']=['EBS-CON-MER-BLD-999'];self.assertTrue(self.errors())
    def test_wrong_domain_rejected(self):
        self.reg['concept_objects'][0]['domain']='KHA';self.assertTrue(self.errors())
    def test_source_drift_rejected_without_writing(self):
        with tempfile.TemporaryDirectory() as td:
            p=pathlib.Path(td)/self.src['files'][0]['path'];p.parent.mkdir(parents=True);p.write_bytes(b'changed')
            before=p.read_bytes();result=audit.validate(self.reg,self.src,td)
            self.assertTrue(any('Source drift:' in e for e in result['errors']));self.assertEqual(p.read_bytes(),before)
    def test_invented_mapping_rejected(self):
        self.reg['concept_objects'][0]['gameplay_mapping']={'status':'SUPPORTED','entity_id':'invented','evidence':[{'path':'missing.json','source_file_sha256':'0'*64,'locator':'id=invented'}]}
        with tempfile.TemporaryDirectory() as td:self.assertTrue(audit.validate(self.reg,self.src,td)['errors'])
    def test_advancement_without_evidence_rejected(self):
        self.reg['concept_objects'][0]['production_maturity']='VERIFIED';self.assertTrue(self.errors())
    def test_supersession_without_successor_rejected(self):
        self.reg['concept_objects'][0]['canon_status']='SUPERSEDED';self.assertTrue(self.errors())
    def test_missing_family_member_rejected(self):
        self.reg['asset_families'][0]['member_concept_ids'].append('EBS-CON-FWL-SYS-999');self.assertTrue(self.errors())
    def test_missing_well_state_rejected(self):
        self.reg['asset_families'][0]['states'].remove('Harvest');self.assertTrue(self.errors())
    def test_production_without_provenance_rejected(self):
        self.reg['production_assets']=[{'production_asset_id':'BAD','concept_ids':['MISSING'],'provenance':None}];self.assertTrue(self.errors())
    def test_unsupported_schema_keyword_fails_closed(self):
        self.assertTrue(audit.schema_errors({}, {'allOf':[]}))
    def test_missing_decision_rejected(self):
        self.reg['concept_objects'][0]['open_decisions']=['DEC-999'];self.assertTrue(self.errors())
    def test_fake_owner_decision_subject_rejected(self):
        o=self.reg['concept_objects'][0];o['canon_status']='CANON_APPROVED';o['canon_approval_evidence']={'path':'AGENTS.md','sha256':'0'*64,'locator':'fake','reviewer':'Angelis Pseftis','date':'2026-09-06'};self.assertTrue(self.errors())
    def test_visual_selection_without_evidence_rejected(self):
        self.reg['concept_objects'][0]['visual_use_status']='SELECTED_FOR_PRODUCTION';self.assertTrue(self.errors())
    def test_dirty_source_and_clean_lfs_identity(self):
        import subprocess, zipfile
        with tempfile.TemporaryDirectory() as td:
            root=pathlib.Path(td)
            def git(*a):return subprocess.run(['git','-C',td,*a],check=True,capture_output=True)
            git('init');git('config','user.name','Angelis Pseftis');git('config','user.email','fixture@example.invalid')
            (root/'image.svg').write_text('<svg>original</svg>');(root/'page.html').write_text('<svg>original</svg>')
            (root/'image.png').write_text('version https://git-lfs.github.com/spec/v1\noid sha256:'+'a'*64+'\nsize 100\n')
            with zipfile.ZipFile(root/'archive.docx','w') as z:z.writestr('media/a.svg','<svg/>')
            git('add','.');git('-c','core.hooksPath=/dev/null','-c','commit.gpgsign=false','commit','-m','fixture')
            (root/'image.svg').write_text('<svg>changed</svg>');(root/'page.html').write_text('<svg>changed</svg>')
            with zipfile.ZipFile(root/'archive.docx','w') as z:z.writestr('media/a.svg','<svg>changed</svg>')
            (root/'new.svg').write_text('<svg>new</svg>');git('add','new.svg')
            result=audit.discover(root);files={x['path']:x for x in result['files']}
            self.assertTrue(files['new.svg']['working_tree_dirty']);self.assertIsNone(files['new.svg']['git_blob'])
            self.assertTrue(files['image.svg']['working_tree_dirty']);self.assertIsNone(files['image.svg']['content_commit'])
            self.assertFalse(files['image.png']['working_tree_dirty']);self.assertFalse(files['image.png']['payload_available'])
            self.assertTrue(result['inline_graphics'][0]['working_tree_dirty']);self.assertIsNone(result['inline_graphics'][0]['content_commit'])
            self.assertTrue(result['archives'][0]['working_tree_dirty']);self.assertIsNone(result['archives'][0]['content_commit'])
    def test_reference_extraction(self):
        text='<img src="a.png"><source srcset="a.webp 1x, b.webp 2x"><style>x{background:url(c.jpg)}</style>'
        self.assertIn('a.png',audit.refs(text));self.assertIn('c.jpg',audit.refs(text));self.assertIn('a.webp 1x, b.webp 2x',audit.refs(text))
if __name__=='__main__':unittest.main()
