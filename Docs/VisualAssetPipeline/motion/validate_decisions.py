"""Author: Angelis Pseftis. Read-only decision/evidence separation checks."""
import json,pathlib,hashlib,re,sys

def validate(path):
 d=json.loads(path.read_text());rows=d['items'];selected=[x for x in rows if x.get('design_resolution')]
 if len(rows)!=63 or len({x['gap_id'] for x in rows})!=63:raise ValueError('Original 63-item census changed')
 if len(selected)!=53:raise ValueError('53 delegated decisions required')
 if {x['gap_id'] for x in selected}!={x['gap_id'] for x in rows if x['status']=='EXECUTION_PENDING'}:raise ValueError('Decision/execution coverage mismatch')
 seen={}
 root=pathlib.Path(__file__).resolve().parents[3]
 def bind(p,sha):
  if p not in seen:
   f=pathlib.Path(p)
   if not f.is_file():raise ValueError('Missing decision source')
   seen[p]=hashlib.sha256(f.read_bytes()).hexdigest()
  if seen[p]!=sha:raise ValueError('Decision source drift')
 for x in selected:
  r=x['design_resolution']
  if r['status']!='DECIDED_UNDER_DELEGATION' or not x['decision'] or not r['rationale'] or not r['book_paragraphs']:raise ValueError('Incomplete design rationale/source')
  if r['execution_status']!='NOT_EXECUTED' or r['owner_acceptance']!='NOT_ACCEPTED' or x['acceptance']!='NOT_ACCEPTED':raise ValueError('Decision promoted execution or acceptance')
  bind(r['candidate_path'],r['candidate_sha256']);bind(r['book_path'],r['book_sha256'])
  if not r['canon_sources']:raise ValueError('Missing canon binding')
  for source in r['canon_sources']:
   source_path=root/source['path'];bind(str(source_path),source['sha256'])
   lines=source_path.read_text().splitlines()
   if not 1<=source['line']<=len(lines) or lines[source['line']-1]!=source['text']:raise ValueError('Canon line drift')
 policy=d['production_policy']
 if len(policy)!=21 or len({x['package_id'] for x in policy})!=21:raise ValueError('21 production packages required')
 if len({x['reserved_production_asset_id'] for x in policy})!=21:raise ValueError('Duplicate reserved production ID')
 for x in policy:
  if not re.fullmatch(r'EBS-[A-Z]{3}-[A-Z]{2,3}-\d{3}',x['reserved_production_asset_id']):raise ValueError('Invalid production ID')
  if x['production_maturity']!='NOT_STARTED' or x['acceptance']!='NOT_ACCEPTED':raise ValueError('Policy promoted production')
  if not x['component_inventory'] or not x['required_track_inventory']:raise ValueError('Missing production contract')
  if x['subject']=='Resonant' and (x['component_inventory']['weapons']!=0 or any(t.startswith('attack') for t in x['required_track_inventory'])):raise ValueError('Unarmed Resonant has attack production tracks')
  if x['subject']=='Future Well' and ('indestructible' not in x['destruction_policy'] or any(t in x['required_track_inventory'] for t in ['death','destruction'])):raise ValueError('Indestructible Well given combat destruction')
 expected={'FWL-HARVEST','RESONANT-COMBAT','PHASE-OVERLAP','MERIDIAN-ANATOMY','MERIDIAN-BUDGETS','DESTRUCTION-CLEARANCE'}
 amendments=d['master_amendments']
 if len(amendments)!=6 or {a['id'] for a in amendments}!=expected:raise ValueError('Missing or duplicate amendment')
 if any(not a['affected_ids'] or not a['selected_wording'] for a in amendments):raise ValueError('Incomplete amendment')
 if not {c['id'] for c in d['authority_conflicts']}<=expected:raise ValueError('Conflict without amendment')
 if any(a['status']!='PREPARED_NOT_APPLIED' or not a['runtime_unchanged'] for a in d['master_amendments']):raise ValueError('Amendment falsely applied')
 if d['decision_summary']['decided']!=53 or d['decision_summary']['undecided']!=0:raise ValueError('Decision summary mismatch')
 return dict(author='Angelis Pseftis',creator='Angelis Pseftis',valid=True,decided=53,undecided=0,production_packages=21,production_execution_pending=53,owner_accepted=0)
if __name__=='__main__':print(json.dumps(validate(pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else pathlib.Path(__file__).with_name('gap-decisions.json')),indent=2))
