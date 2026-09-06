#!/usr/bin/env python3
"""Read-only reference-package preparation and drift validation. Author: Angelis Pseftis.
Prints JSON; callers explicitly retain output in the documentation workstream.
"""
import argparse, collections, hashlib, json, pathlib, re, sys, zipfile
import xml.etree.ElementTree as ET
HERE=pathlib.Path(__file__).resolve().parent
AUTHOR='Angelis Pseftis'
def read(p):return json.loads(p.read_text())
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def binding(p,base):return {'path':str(p.relative_to(base)),'sha256':digest(p)}
def prepare(root,folder=HERE):
    reg=read(folder/'concept-register.json');review=read(folder/'book-based-review.json');state=read(folder/'review-selections.json')
    if state['snapshot_sha256']!=digest(folder/'concept-register.json'):raise ValueError('Register changed since review; reconcile first')
    if digest(folder/'review-selections.json')!=review['application']['review_selections_sha256']:raise ValueError('Review notes or choices changed; reconcile book review before preparation')
    book_path=root/review['book']['retained_path_from_project']
    if not book_path.is_file():raise ValueError('Retained book bytes unavailable; restore the evidence source before validation')
    if digest(book_path)!=review['book']['sha256']:raise ValueError('Retained book hash mismatch')
    with zipfile.ZipFile(book_path) as z:
        doc=ET.fromstring(z.read('word/document.xml'))
    ns={'w':'http://schemas.openxmlformats.org/wordprocessingml/2006/main'}
    paragraphs={i:''.join(t.text or '' for t in el.findall('.//w:t',ns)) for i,el in enumerate(doc.findall('.//w:p',ns),1)}
    excerpts={x['paragraph']:x['text'] for x in review['evidence_excerpts']}
    for d in review['decisions']:
        for n in d['book_paragraphs']:
            if n not in excerpts or excerpts[n]!=paragraphs.get(n):raise ValueError('Book paragraph/excerpt mismatch: '+str(n))
    objects={o['concept_id']:o for o in reg['concept_objects']};decisions={d['concept_id']:d for d in review['decisions']}
    if len(decisions)!=len(review['decisions']):raise ValueError('Duplicate review decisions')
    for cid,d in decisions.items():
        if cid not in objects:raise ValueError('Unknown concept '+cid)
        if state['selections'].get(cid,{}).get('choice')!=d['choice']:raise ValueError('Live choice differs from book review: '+cid)
        if state['selections'][cid]['source_sha256']!=d['source_sha256'] or objects[cid]['source_refs'][0]['source_sha256']!=d['source_sha256']:raise ValueError('Source binding changed: '+cid)
    bible_path=root/'Docs/Archive/DevelopmentBible.md';bible=bible_path.read_text().splitlines();authority=[]
    for n in ['Docs/Archive/DevelopmentBible.md','Docs/Requirements.md','Docs/ArtDirection.md','Content/Data/Source/units.json','Content/Data/Source/buildings.json','Content/Data/Source/future_wells.json']:
        authority.append(binding(root/n,root))
    data={}
    for name in ['units','buildings']:
        path=root/f'Content/Data/Source/{name}.json'
        for row in read(path)[name]:data[row['id']]={'source':binding(path,root),'record':row}
    grouped=collections.OrderedDict()
    for cid,d in decisions.items():
        o=objects[cid];key=o.get('asset_family_id') or o['gameplay_mapping']['entity_id']
        if not key:raise ValueError('Reviewed object has no entity/family: '+cid)
        grouped.setdefault(key,[]).append(o)
    priority=['mc_lancer','mc_relay_skiff','EBS-FAM-FWL-001','mc_surveyor','mc_bulwark_team','ka_tender','mc_anchor','mc_power_link','mc_array_foundry','mc_aegis_post','ka_memory_hearth','ka_waystone','ka_riftstalker','ka_cairnback','ka_resonant','ka_growth_basin','ka_listening_spine','hc_concordance','hc_interval_loom','hc_chorus_loom','hc_phase_anchor']
    blockers=[
      {'id':'PKG-GATE-VISUAL','resolution':'Record final visual design acceptance and exact source/derived-reference identity; delegated Keep/Rework/Replace is direction input.'},
      {'id':'PKG-GATE-RIGHTS','resolution':'Reconcile retained original method, inputs, terms and intended-use rights with AssetRegister; no presence-based clearance.'},
      {'id':'PKG-GATE-SCALE','resolution':'Resolve production dimensions and exact pixel crop. Runtime footprint snapshot is not mesh scale; reject printed concept heights.'},
      {'id':'PKG-GATE-VIEWS','resolution':'Supply necessary orthographic, underside, state, socket and material views as labeled derived references, retaining originals.'},
      {'id':'PKG-GATE-INTERFACES','resolution':'Bind approved sockets, state/event interfaces, collision/selection and construction exits to current runtime contracts in a later integration task.'},
      {'id':'PKG-GATE-BUDGET','resolution':'Adopt asset-specific material/texture/LOD budgets against target-platform tests; no numeric budget inferred from artwork.'},
      {'id':'PKG-GATE-PLAN','resolution':'Confirm art-production package in the current plan and source ownership before geometry or artwork production; integration remains separately scoped.'},
      {'id':'PKG-CONFLICT-SKIFF','resolution':review['unresolved_conflicts'][0]},
      {'id':'PKG-CONFLICT-WELL','resolution':review['unresolved_conflicts'][1]},
    ]
    packages=[]
    for key,group in grouped.items():
        iswell=key=='EBS-FAM-FWL-001';name='Future Well' if iswell else group[0]['gameplay_mapping']['display_name'];canon=[]
        for line_no,line in enumerate(bible,1):
            if (not iswell and line.startswith('| **'+name+'**')) or (iswell and line.startswith('| **') and any(line.startswith('| **'+s+'**') for s in ['Dormant','Harvest','Preserve','Reshape'])):
                canon.append({'path':'Docs/Archive/DevelopmentBible.md','line':line_no,'text':line,'sha256':digest(bible_path)})
        if not canon:raise ValueError('No explicit canon production row for '+name)
        refs=[]
        for o in group:
            d=decisions[o['concept_id']]
            source=o['source_refs'][0];p=root/source['source_repository_path']
            if digest(p)!=source['source_sha256']:raise ValueError('Original image drift: '+str(p))
            refs.append({'concept_id':o['concept_id'],'decision':d['choice'],'use':'RETAINED_HISTORY_ONLY' if d['choice']=='REPLACE' else 'DESIGN_IDENTITY' if d['choice']=='KEEP' else 'REWORK_INPUT','source':source,'direction':d['reason'],'book_paragraphs':d['book_paragraphs'],'canon_status':o['canon_status']})
        order=priority.index(key)+1 if key in priority else len(priority)+1
        allreplace=all(x['decision']=='REPLACE' for x in refs)
        next_action='RESOLVE_BLOCKERS_THEN_CREATE_REPLACEMENT_CONCEPT' if allreplace else 'RESOLVE_BLOCKERS_THEN_CREATE_REWORK_REFERENCE' if any(x['decision']=='REWORK' for x in refs) else 'RESOLVE_BLOCKERS_THEN_COMPLETE_REFERENCE_VIEWS'
        runtime=data.get(key)
        contract={'status':'OBSERVED_SOURCE_NOT_ACCEPTANCE','entity_id':None if iswell else key,'evidence':runtime,'footprint_cells':runtime['record'].get('footprint_cells','TBD') if runtime else 'TBD','production_dimensions_cm':'TBD'}
        if iswell:contract['evidence']={'source':binding(root/'Content/Data/Source/future_wells.json',root),'record':read(root/'Content/Data/Source/future_wells.json')}
        required=['Front, side, rear and top views at a shared confirmed scale','Exact source pixel region retaining complete silhouette','Material zones: ceramic/metal or grown strata or charcoal glass as applicable; no invented PBR numbers','Damaged/destruction sequence and construction/creation sequence','Selection and faction/team ownership treatment at tactical camera','Socket/event map with named purposes; actual engine socket names TBD']
        if group[0]['object_type']=='UNT':required+=['Locomotion/turn/stop poses, work or weapon anticipation/action/recovery, interruption and carried equipment states']
        elif not iswell:required+=['Operational/offline states, delivery/exit paths, production/research or rooted/mobile variants where canon requires']
        if iswell:required+=['One common bowl/core with Dormant, Harvest transition and exhausted endpoint, Preserve and Reshape','Reshape links to a particular authorized terrain feature and its expiry; no freeform geometry change','Resolve Harvest color before supplemental effect design']
        blocked=[b['id'] for b in blockers[:7]]
        if key=='mc_relay_skiff':blocked.append('PKG-CONFLICT-SKIFF')
        if iswell:blocked.append('PKG-CONFLICT-WELL')
        packages.append({'package_id':'EBS-PKG-'+key.upper().replace('_','-'),'subject':name,'domain':group[0]['domain'],'object_type':group[0]['object_type'],'priority':order,'priority_basis':'Preparation sequencing judgment: M01 role/state defects first, then faction infrastructure; does not reorder active DeliveryPlan','status':'BRIEF_PREPARED','reference_ready':False,'production_asset_id':None,'production_maturity':'NOT_STARTED','next_action':next_action,'concept_inputs':refs,'canon_production_brief':canon,'gameplay_contract':contract,'reference_items_required':required,'silhouette_brief':'Use cited canonical form/function and individual input directions below; exclude retained-history-only source geometry from target constraints.','material_and_motion_brief':'The cited canon row specifies silhouette/materials, readable function and motion/sound; retain it intact rather than paraphrasing new mechanics.','family_direction':review['family_direction'] if iswell else None,'unreal_handoff':{'destination':'TBD in authorized integration task','asset_name':'TBD after production ID allocation','units':'centimeters after confirmed scale','pivot_orientation':'TBD per registered import convention','nanite':'OFF in current ArtDirection baseline; no change proposed','lod':'Authored LOD0/LOD1 per REL-ART-005; per-asset limits TBD','collision_navigation':'Cosmetic art must not alter authoritative footprint, navigation or selection behavior','material_texture_budget':'TBD'},'blocking_dependencies':blocked,'acceptance':{'art':'Concept fidelity, faction form, role silhouette, material and state coherence; exact revisions reviewed','gameplay':'Representative RTS camera, crowded/occluded views, ownership/selection, function, damage/construction/state; applicable human recognition evidence','technical':'Naming/path/scale/pivot, collision/LOD/material/texture/rig, source rights and target-platform packaged performance','status':'NOT_EVALUATED'}})
    packages.sort(key=lambda p:p['priority'])
    reviewed=set(decisions);pending=[{'concept_id':o['concept_id'],'name':o['display_name'],'domain':o['domain'],'type':o['object_type'],'next_action':'VISUAL_DIRECTION_REVIEW','production_maturity':o['production_maturity']} for o in reg['concept_objects'] if o['concept_id'] not in reviewed]
    return {'author':AUTHOR,'creator':AUTHOR,'schema_version':1,'status':'PREPARATION_ONLY','book_source':{'path_from_project':review['book']['retained_path_from_project'],'sha256':digest(book_path),'cited_paragraphs_verified':True},'input_bindings':[binding(folder/n,folder) for n in ['concept-register.json','book-based-review.json','review-selections.json']],'authority_bindings':authority,'review_revision':state['revision'],'counts':{'reviewed_concepts':len(reviewed),'packages':len(packages),'pending_visual_reviews':len(pending),'text_only_gaps':len(reg['text_only_specifications']),'reference_ready':0,'production_assets_allocated':0},'blocking_decisions':blockers,'packages':packages,'pending_visual_reviews':pending,'missing_concept_backlog':[{'spec_id':s['spec_id'],'name':s['name'],'next_action':'MISSING_CONCEPT_BRIEF','source_specification':s,'status':'NOT_STARTED'} for s in reg['text_only_specifications']],'boundary':'No new art, crops, geometry, production IDs, canon approval, maturity advancement or Unreal changes. Reference briefs are preparation; stage exit checks remain open.'}
def validate(manifest,root,folder=HERE):
    errors=[]
    for base,key in [(folder,'input_bindings'),(root,'authority_bindings')]:
        for b in manifest.get(key,[]):
            p=base/b['path']
            if not p.is_file() or digest(p)!=b['sha256']:errors.append('Drift: '+b['path'])
    try:
        expected=prepare(root,folder)
        if manifest!=expected:errors.append('Manifest differs from current reproducible preparation; investigate changes before replacing it')
    except (ValueError,KeyError,OSError) as e:errors.append(str(e))
    return {'author':AUTHOR,'creator':AUTHOR,'valid':not errors,'errors':errors,'boundary':'Structural/source consistency only; no art, human, rights or runtime acceptance'}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('command',choices=['prepare','validate']);p.add_argument('--root',type=pathlib.Path,required=True);p.add_argument('--manifest',type=pathlib.Path,default=HERE/'reference-packages.json');a=p.parse_args()
    try:r=prepare(a.root) if a.command=='prepare' else validate(read(a.manifest),a.root)
    except (ValueError,KeyError,OSError) as e:print(json.dumps({'error':str(e)}));return 1
    print(json.dumps(r,indent=2,ensure_ascii=False));return 1 if r.get('valid') is False else 0
if __name__=='__main__':sys.exit(main())
