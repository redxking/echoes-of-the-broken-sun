"""Author: Angelis Pseftis. Read sources; emit review JSON to stdout only."""
import argparse, hashlib, json, pathlib

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def build(packages, evidence):
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
    if len(result)!=len(receipts):raise ValueError('Unmatched receipt/package coverage')
    return dict(author='Angelis Pseftis',creator='Angelis Pseftis',schema_version=1,source_packages_sha256=digest(packages),evidence_root=str(evidence),boundary='Candidate and schematic reference only; no Unreal rig, integration, rights clearance or owner acceptance',packages=result)
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--packages',type=pathlib.Path,required=True);ap.add_argument('--evidence',type=pathlib.Path,required=True);a=ap.parse_args()
    print(json.dumps(build(a.packages,a.evidence),indent=2))
