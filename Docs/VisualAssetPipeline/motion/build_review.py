"""Author: Angelis Pseftis. Read sources; emit review JSON to stdout only."""
import argparse, hashlib, json, pathlib

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def build(packages, evidence, decisions=None):
    data=json.loads(packages.read_text())
    receipts={}
    for path in evidence.glob('*/generation-record.json'):
        r=json.loads(path.read_text())
        if r.get('family_id')=='EBS-FAM-FWL-001': r['package_id']='EBS-PKG-EBS-FAM-FWL-001'
        if r.get('package_id') in receipts: raise ValueError('Duplicate receipt package')
        receipts[r.get('package_id')]=(path,r)
    result=[]
    for p in data['packages']:
        path,r=receipts[p['package_id']]
        output=path.parent/r['output']
        if not output.is_file(): raise ValueError(f'Missing image: {output}')
        actual=digest(output)
        expected=r.get('output_sha256')
        if expected and expected!=actual: raise ValueError(f'Image hash mismatch: {output}')
        gaps=r.get('limitations') or r.get('notes') or ['Mechanical consistency and source fidelity require review; receipt has no normalized limitations field.']
        if isinstance(gaps,str):gaps=[gaps]
        if not isinstance(gaps,list):gaps=[json.dumps(gaps)]
        brief=p['canon_production_brief']
        tracks=['idle / operational','damage response','destruction / death','selection and ownership readability']
        tracks+=['locomotion / turn / stop','action anticipation / execution / recovery','cancel / interruption / recovery'] if p['object_type']=='UNT' else ['construction','offline / recovery','role-specific operation / interruption']
        result.append(dict(package_id=p['package_id'],subject=p['subject'],domain=p['domain'],concept_ids=[x['concept_id'] for x in p['concept_inputs']],candidate_path=str(output),candidate_sha256=actual,receipt_path=str(path),receipt_sha256=digest(path),canon_status='CANDIDATE',production_maturity=p['production_maturity'],review_status='OPEN',source_motion_contract=brief,gameplay_entity=p['gameplay_contract'].get('entity_id'),required_track_families=tracks,event_binding='TBD: bind existing authoritative adapter event/state; no animation-owned timers or gameplay mutations',interruption='Cancel cosmetic action on authoritative state exit; stop loops and clear effects; reconstruct pose from current state after reload or visibility return',accessibility='Provide shape/posture/icon redundancy; reduced motion uses discrete state poses, no rapid flashes',audio='Cues described by cited creative brief; loops stop on state exit. Audio files and mixer acceptance not produced.',geometry_gate='Fixed component inventory and source-matched front/side/rear/top views before rigging',gaps=[dict(id=f"{p['package_id']}-GAP-{i:02}",status='OPEN',finding=g,closure='Matched-view correction or authoritative rendered evidence; no automatic closure') for i,g in enumerate(gaps,1)]))
    if decisions is not None:
        apply_decisions(result, decisions)
        if json.loads(decisions.read_text()).get('schema_version',1)>=3:
            from validate_decisions import validate
            validate(decisions)
    if len(result)!=len(receipts):raise ValueError('Unmatched receipt/package coverage')
    return dict(author='Angelis Pseftis',creator='Angelis Pseftis',schema_version=1,source_packages_sha256=digest(packages),evidence_root=str(evidence),boundary='Candidate and schematic reference only; no Unreal rig, integration, rights clearance or owner acceptance',decision_summary=(json.loads(decisions.read_text()).get('decision_summary') if decisions else None),packages=result)
def apply_decisions(packages, path):
    entries=json.loads(path.read_text())['items']
    gaps={g['id']:g for p in packages for g in p['gaps']}
    if len(entries)!=len(gaps) or len({e['gap_id'] for e in entries})!=len(entries) or set(gaps)!={e['gap_id'] for e in entries}:
        raise ValueError('Decision coverage mismatch or duplicate gap ID')
    allowed={'AUTHORITY_DEPENDENCY','CORRECTED_REFERENCE','DESIGN_DECIDED','INTEGRATION_DEPENDENCY','MOTION_REFERENCE_REQUIRED','REFERENCE_REQUIRED','EXECUTION_PENDING'}
    for e in entries:
        g=gaps[e['gap_id']]
        if e['status'] not in allowed or e['original_finding']!=g['finding']:
            raise ValueError('Decision state invalid or source finding drift')
        if e['status']=='CORRECTED_REFERENCE':
            proof=e.get('evidence',{}); f=pathlib.Path(proof.get('path',''))
            if not f.is_file() or digest(f)!=proof.get('sha256'): raise ValueError('Correction evidence missing or changed')
        if e.get('acceptance','NOT_ACCEPTED')!='NOT_ACCEPTED':
            raise ValueError('Reference decisions cannot promote acceptance')
        prep=e.get('preparation')
        if prep is not None:
            if prep.get('status') not in {'BRIEF_READY','PARTIAL_REFERENCE','ARTIFACT_READY','DEFERRED_TO_INTEGRATION','BLOCKED_BY_AUTHORITY'}:
                raise ValueError('Invalid preparation status')
            if not prep.get('scope') or not prep.get('remaining') or prep.get('acceptance')!='NOT_ACCEPTED':
                raise ValueError('Preparation needs scope, remaining evidence and unaccepted boundary')
            if prep['status'] in {'PARTIAL_REFERENCE','ARTIFACT_READY'} and not prep.get('evidence'):
                raise ValueError('Prepared artifact lacks evidence')
            for proof in prep.get('evidence',[]):
                f=pathlib.Path(proof.get('path',''))
                if not f.is_absolute(): f=path.parent/f
                if not proof.get('scope') or not f.is_file() or digest(f)!=proof.get('sha256'):
                    raise ValueError('Preparation evidence missing or changed')
        g.update(status=e['status'],decision=e['decision'],next_evidence=e['next_evidence'],acceptance='NOT_ACCEPTED',evidence=e.get('evidence'),preparation=prep,design_resolution=e.get('design_resolution'),execution_lane=e.get('execution_lane'),original_disposition=e.get('original_disposition'))
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--packages',type=pathlib.Path,required=True);ap.add_argument('--evidence',type=pathlib.Path,required=True);ap.add_argument('--decisions',type=pathlib.Path,default=pathlib.Path(__file__).with_name('gap-decisions.json'));a=ap.parse_args()
    print(json.dumps(build(a.packages,a.evidence,a.decisions),indent=2))
