#!/usr/bin/env python3
"""Read-only visual discovery and concept validation. Author: Angelis Pseftis.
Writes JSON to stdout only; never edits sources, the register, or Unreal assets.
"""
import argparse, collections, datetime, hashlib, json, os, pathlib, re, subprocess, sys, urllib.parse, zipfile
from concurrent.futures import ThreadPoolExecutor
from html.parser import HTMLParser

VISUAL = {'.png','.jpg','.jpeg','.webp','.tif','.tiff','.svg','.gif','.bmp','.avif','.exr','.hdr','.ico','.tga','.psd','.ai','.eps'}
TEXT = {'.html','.css','.js','.jsx','.ts','.tsx','.md','.json'}
AUTHOR = 'Angelis Pseftis'
BASE = 'https://redxking.github.io/echoes-of-the-broken-sun/'
def digest(b): return hashlib.sha256(b).hexdigest()
def git(root,*args): return subprocess.check_output(['git','-C',str(root),*args])
def head_bytes(root,path):
    p=subprocess.run(['git','-C',str(root),'show','HEAD:'+path],capture_output=True)
    return p.stdout if p.returncode==0 else None
def load(path): return json.loads(pathlib.Path(path).read_text())
def kind(path):
    if '/engine/' in path or 'BuildArtifacts/' in path or 'Saved/' in path: return 'IMPLEMENTATION_REFERENCE'
    if '/Textures/' in path: return 'PRODUCTION_TEXTURE_REFERENCE'
    if '/concepts/' in path or 'hero-soryn' in path: return 'CONCEPT_SOURCE'
    return 'PRESENTATION_OR_UNKNOWN'
def refs(text):
    found = re.findall(r'''(?:src|href|poster|srcSet|srcset)\s*=\s*["']([^"']+)["']''', text)
    found += re.findall(r'''url\(\s*["']?([^\s)'"<>]+)''', text)
    found += re.findall(r'''["'(`]([^\s"'`<>]*\.(?:png|jpe?g|webp|svg|gif|tiff?|avif|exr|hdr)(?:\?[^\s"'`<>]*)?)["')`]''',text,re.I)
    return sorted(set(found))
def dimensions(data):
    try:
        from PIL import Image
        import io
        with Image.open(io.BytesIO(data)) as im: return list(im.size)
    except Exception: return None

def discover(root, history=False):
    root=pathlib.Path(root).resolve(); paths=git(root,'ls-files','-z').decode().split('\0');paths=[p for p in paths if p and not p.startswith('Docs/VisualAssetPipeline/')]
    files=[]; references=[]; inline=[]; archives=[]
    for p in paths:
        path=root/p;ext=path.suffix.lower()
        if not path.is_file(): continue
        if ext in VISUAL:
            data=path.read_bytes();sha=digest(data)
            head_data=head_bytes(root,p)
            expected=None if head_data is None else re.search(rb'oid sha256:([a-f0-9]{64})',head_data).group(1).decode() if head_data.startswith(b'version https://git-lfs.github.com/spec/v1') else digest(head_data)
            pointer=data.startswith(b'version https://git-lfs.github.com/spec/v1')
            observed=re.search(rb'oid sha256:([a-f0-9]{64})',data).group(1).decode() if pointer else sha
            dirty=observed!=expected
            files.append(dict(source_id='EBS-SRC-'+sha[:16],path=p,filename=path.name,sha256=sha,bytes=len(data),dimensions=dimensions(data),classification=kind(p),working_tree_dirty=dirty,payload_available=not pointer,payload_sha256=observed,head_payload_sha256=expected,content_commit=None if dirty else git(root,'log','-1','--format=%H','--',p).decode().strip(),git_blob=None if head_data is None else git(root,'rev-parse','HEAD:'+p).decode().strip(),last_commit=git(root,'log','-1','--format=%H','--',p).decode().strip(),lfs_pointer=data.startswith(b'version https://git-lfs.github.com/spec/v1')))
        elif ext in TEXT:
            raw=path.read_bytes();text=raw.decode(errors='replace');text_dirty=raw!=head_bytes(root,p);text_commit=None if text_dirty else git(root,'rev-parse','HEAD').decode().strip()
            for ref in refs(text): references.append(dict(path=p,reference=ref,source_file_sha256=digest(raw),working_tree_dirty=text_dirty,content_commit=text_commit))
            for n,m in enumerate(re.finditer(r'<svg\b[\s\S]*?</svg>|<canvas\b[^>]*>',text,re.I)):
                snippet=m.group();inline.append(dict(source_id='EBS-GFX-'+digest(snippet.encode())[:16],path=p,line=text[:m.start()].count('\n')+1,ordinal=n+1,sha256=digest(snippet.encode()),element='canvas' if snippet.startswith('<canvas') else 'svg',source_file_sha256=digest(path.read_bytes()),working_tree_dirty=text_dirty,content_commit=text_commit,opening_tag=snippet.split('>')[0]+'>'))
        elif ext in {'.docx','.pptx','.xlsx','.zip','.epub'}:
            try:
                with zipfile.ZipFile(path) as z:
                    members=[dict(member=n,sha256=digest(z.read(n)),bytes=z.getinfo(n).file_size) for n in z.namelist() if pathlib.Path(n).suffix.lower() in VISUAL]
                archives.append(dict(path=p,visual_members=members,source_file_sha256=digest(path.read_bytes()),working_tree_dirty=path.read_bytes()!=head_bytes(root,p),content_commit=None if path.read_bytes()!=head_bytes(root,p) else git(root,'rev-parse','HEAD').decode().strip()))
            except Exception as e: archives.append(dict(path=p,error=str(e)))
    by_path={f['path']:f for f in files}
    for f in files:
        p=f['path']
        if p.startswith('website/public/archive-static/'): p='site/'+p[len('website/public/archive-static/'):]
        elif p.startswith('website/public/') and not p.endswith('favicon.svg'): p='site/'+p[len('website/public/'):]
        f['canonical_repository_path']=p if p in by_path else f['path']
        original=f['canonical_repository_path']
        if original.endswith('.webp'):
            for ext in ['.png','.jpg','.jpeg']:
                candidate=original[:-5]+ext
                if candidate in by_path:original=candidate;break
        f['retained_original_path']=original
        f['representation']='DELIVERY_DERIVATIVE' if f['canonical_repository_path']!=original else 'RETAINED_SOURCE'
        f['is_mirror']=f['path']!=f['canonical_repository_path']
        f['source_url']=BASE+f['canonical_repository_path'][5:] if f['canonical_repository_path'].startswith('site/') else None
        f['relationship_evidence']='Docs/Archive/AssetRegister.md CONCEPT/CAPTURE records; exact mirror hashes verified separately' if f['path'].startswith(('site/','website/public/')) else None
    historical=[]
    if history:
        current_blobs={f['git_blob'] for f in files}
        for line in git(root,'rev-list','--objects','--all').decode().splitlines():
            blob,_,p=line.partition(' ')
            if pathlib.Path(p).suffix.lower() in VISUAL:
                data=git(root,'cat-file','blob',blob)
                storage_sha=digest(data); is_pointer=data.startswith(b'version https://git-lfs.github.com/spec/v1'); resolved=True
                if is_pointer:
                    oid=re.search(rb'oid sha256:([0-9a-f]{64})',data).group(1).decode()
                    common=pathlib.Path(git(root,'rev-parse','--git-common-dir').decode().strip()); common=common if common.is_absolute() else root/common
                    obj=common/'lfs'/'objects'/oid[:2]/oid[2:4]/oid
                    if obj.is_file():data=obj.read_bytes()
                    else:resolved=False
                historical.append(dict(git_blob=blob,path_hint=p,git_storage_sha256=storage_sha,lfs_pointer=is_pointer,payload_available=resolved,sha256=digest(data),bytes=len(data),dimensions=dimensions(data),present_in_current_tree=blob in current_blobs,classification=kind(p)))
    return dict(author=AUTHOR,schema_version=1,root=str(root),source_commit=git(root,'rev-parse','HEAD').decode().strip(),files=files,inline_graphics=inline,references=references,archives=archives,history=historical,scope='Tracked files at source commit; all locally reachable Git refs when history requested. Ignored/untracked files require supplemental walk. No claim about unavailable remote refs.')

def fetch(url):
    p=subprocess.run(['curl','-sS','-L','--max-time','25','--max-redirs','4','--proto','=https','--proto-redir','=https','-w','\n%{http_code}\n%{content_type}\n%{url_effective}',url],capture_output=True)
    try: body,status,ctype,effective=p.stdout.rsplit(b'\n',3)
    except ValueError: return dict(url=url,error=p.stderr.decode(errors='replace')),b''
    rec=dict(url=url,status=int(status or 0),content_type=ctype.decode(),effective_url=effective.decode(),sha256=digest(body),bytes=len(body))
    if p.returncode:rec['error']=p.stderr.decode(errors='replace')
    return rec,body

def crawl(base,limit):
    pending={base};seen=set();results=[];external=set(); origin=urllib.parse.urlsplit(base)
    while pending and len(seen)<limit:
        batch=sorted(pending-seen)[:min(8,limit-len(seen))]
        if not batch:break
        pending.difference_update(batch);seen.update(batch)
        with ThreadPoolExecutor(max_workers=4) as pool:
            for rec,body in pool.map(fetch,batch):
                results.append(rec)
                if rec.get('status')!=200:continue
                if any(t in rec.get('content_type','') for t in ['html','css','javascript','svg','json']):
                    txt=body.decode(errors='replace');rec['references']=refs(txt)
                    rec['inline_svg_count']=len(re.findall(r'<svg\b',txt));rec['canvas_count']=len(re.findall(r'<canvas\b',txt))
                    for ref in rec['references']:
                        if ref.startswith(('data:','#','mailto:','tel:')): continue
                        # srcset candidates can contain descriptors; retain original reference above.
                        for candidate in ref.split(','):
                            url=urllib.parse.urljoin(rec['effective_url'],candidate.strip().split(' ')[0]);s=urllib.parse.urlsplit(url)
                            url=urllib.parse.urlunsplit((s.scheme,s.netloc,s.path,s.query,''))
                            if s.netloc==origin.netloc and s.path.startswith(origin.path):
                                if url not in seen:pending.add(url)
                            elif s.scheme in {'http','https'}:external.add(url)
    return dict(author=AUTHOR,base=base,resources=results,external_references=sorted(external),unvisited=sorted(pending-seen),limit=limit,coverage_complete=not(pending-seen),scope='Recursive same-site HTML/CSS/JS/image references; external links listed, not crawled. Dynamic runtime-only URLs require source/render review.')

def supplement(root):
    root=pathlib.Path(root).resolve(); tracked=set(git(root,'ls-files','-z').decode().split('\0')); rows=[]; exclusions=[]
    for base,dirs,names in os.walk(root):
        for n in dirs[:]:
            if n in {'.git','node_modules','Intermediate','DerivedDataCache','Binaries','.next'}:
                exclusions.append(str((pathlib.Path(base)/n).relative_to(root)));dirs.remove(n)
        for n in names:
            p=pathlib.Path(base)/n; rel=str(p.relative_to(root))
            if p.suffix.lower() in VISUAL and rel not in tracked:
                try:
                    data=p.read_bytes();rows.append(dict(path=rel,sha256=digest(data),bytes=len(data),classification=kind(rel),visual_review='Not individually reviewed'))
                except OSError as e:rows.append(dict(path=rel,error=str(e)))
    return dict(author=AUTHOR,root=str(root),observed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),untracked_or_ignored_visual_files=rows,excluded_dependency_or_engine_cache_directories=exclusions,boundary='Directory symlinks are not followed. Captures are implementation evidence, not concept approval.')

def compare(before,after):
    old={f['path']:f for f in before['files']};new={f['path']:f for f in after['files']}
    return dict(author=AUTHOR,added_paths=sorted(new.keys()-old.keys()),removed_paths=sorted(old.keys()-new.keys()),changed_paths=sorted(p for p in old.keys()&new.keys() if old[p]['sha256']!=new[p]['sha256']),boundary='Discovery delta only; never allocates or changes concept IDs. Review new/changed visual content and preserve old references.')

REQUIRED=['concept_id','production_asset_id','display_name','source_refs','domain','object_type','canon_status','production_maturity','gameplay_mapping','visual_description','silhouette_requirements','dimensions','material_language','team_readability','animation_rigging','construction_behavior','state_changes','damage_destruction','vfx_requirements','audio_interfaces','collision_navigation','nanite_lod','texture_material_budget','rts_readability_status','performance_validation_status','future_unreal_destination','provenance_rights','dependencies','open_decisions','notes']
CANON={'CANON_APPROVED','CANDIDATE','EXPLORATORY','SUPERSEDED','UNKNOWN'}
MATURITY={'NOT_STARTED','REFERENCE_READY','BLOCKOUT','GAMEPLAY_PROXY','ART_ALPHA','ART_BETA','PRODUCTION','UE_INTEGRATED','VERIFIED'}
def schema_errors(value,schema,path='$'):
    """Validate the keywords used by our checked-in schema, not arbitrary JSON Schema."""
    allowed={'$schema','title','description','type','required','properties','const','enum','pattern','minLength','minItems','maxItems','items','minimum','maximum'}
    errors=[path+': unsupported schema keyword '+k for k in schema if k not in allowed];typ=schema.get('type');types=typ if isinstance(typ,list) else [typ] if typ else []
    matches={'null':value is None,'object':isinstance(value,dict),'array':isinstance(value,list),'string':isinstance(value,str),'number':isinstance(value,(int,float)) and not isinstance(value,bool),'integer':isinstance(value,int) and not isinstance(value,bool),'boolean':isinstance(value,bool)}
    if types and not any(matches.get(t,False) for t in types):return errors+[path+': wrong type']
    if 'const' in schema and value!=schema['const']:errors.append(path+': wrong constant')
    if 'enum' in schema and value not in schema['enum']:errors.append(path+': invalid enum')
    if isinstance(value,str):
        if len(value)<schema.get('minLength',0):errors.append(path+': too short')
        if 'pattern' in schema and not re.search(schema['pattern'],value):errors.append(path+': pattern mismatch')
    if isinstance(value,(int,float)) and not isinstance(value,bool):
        if value<schema.get('minimum',float('-inf')) or value>schema.get('maximum',float('inf')):errors.append(path+': out of range')
    if isinstance(value,list):
        if len(value)<schema.get('minItems',0) or len(value)>schema.get('maxItems',float('inf')):errors.append(path+': array size')
        for i,x in enumerate(value):errors+=schema_errors(x,schema.get('items',{}),path+'['+str(i)+']')
    if isinstance(value,dict):
        for k in schema.get('required',[]):
            if k not in value:errors.append(path+': missing '+k)
        for k,v in value.items():errors+=schema_errors(v,schema.get('properties',{}).get(k,{}),path+'.'+k)
    return errors

def evidence_errors(ev,label,root=None,owner=False):
    if not isinstance(ev,dict):return [label+': evidence must be structured']
    errors=[];p=ev.get('path');sha=ev.get('source_file_sha256') or ev.get('sha256')
    if not isinstance(p,str) or not p or pathlib.PurePosixPath(p).is_absolute() or '..' in pathlib.PurePosixPath(p).parts:errors.append(label+': invalid evidence path')
    if not isinstance(sha,str) or not re.fullmatch('[a-f0-9]{64}',sha):errors.append(label+': missing evidence hash')
    if not ev.get('locator'):errors.append(label+': missing evidence locator')
    if owner:
        if ev.get('reviewer')!=AUTHOR:errors.append(label+': owner decision required')
        if not re.fullmatch(r'\d{4}-\d{2}-\d{2}',str(ev.get('date',''))):errors.append(label+': decision date required')
    if root and not errors:
        file=pathlib.Path(root)/p
        if not file.is_file():errors.append(label+': evidence path missing')
        elif digest(file.read_bytes())!=sha:errors.append(label+': evidence hash drift')
        else:
            content=file.read_text(errors='replace');loc=str(ev['locator']);located=loc in content
            if loc.startswith('id=') and file.suffix=='.json':
                data=load(file);located=any(isinstance(v,list) and any(isinstance(r,dict) and r.get('id')==loc[3:] for r in v) for v in data.values()) if isinstance(data,dict) else False
            if not located:errors.append(label+': evidence locator not found')
    return errors

def decision_binding_errors(ev,obj,status,root=None):
    cid=obj['concept_id'];errors=[]
    if not isinstance(ev,dict):return [cid+': missing bound decision']
    expected={'concept_id':cid,'source_sha256':obj['source_refs'][0]['source_sha256'],'decision':status}
    if any(ev.get(k)!=v for k,v in expected.items()):errors.append(cid+': decision subject/source/status mismatch')
    if root and isinstance(ev.get('path'),str):
        file=pathlib.Path(root)/ev['path']
        if file.suffix!='.json' or not file.is_file():errors.append(cid+': structured JSON owner decision record required')
        else:
            data=load(file);records=data.get('decisions',[]) if isinstance(data,dict) else []
            if not any(isinstance(r,dict) and r.get('decision_id')==ev.get('locator') and all(r.get(k)==v for k,v in expected.items()) and r.get('reviewer')==AUTHOR and r.get('date')==ev.get('date') for r in records):errors.append(cid+': exact owner decision record missing')
    return errors

def validate(register,sources,root=None):
    errors=schema_errors(register,load(pathlib.Path(__file__).with_name('concept-register.schema.json')))
    if errors:return dict(author=AUTHOR,errors=errors,scope='Schema checks failed; semantic audit not run')
    objs=register['concept_objects'];ids=[o.get('concept_id') for o in objs];source_ids={f['source_id'] for f in sources['files']}|{f['source_id'] for f in sources['inline_graphics']}
    if len(ids)!=len(set(ids)):errors.append('Duplicate concept IDs')
    for o in objs:
        cid=o.get('concept_id','MISSING')
        for k in REQUIRED:
            if k not in o:errors.append(f'{cid}: missing {k}')
        if not re.fullmatch(r'EBS-CON-[A-Z]{3}-[A-Z]{2,3}-\d{3,}',cid):errors.append(f'{cid}: invalid ID')
        if o.get('canon_status') not in CANON:errors.append(f'{cid}: invalid canon status')
        if o.get('production_maturity') not in MATURITY:errors.append(f'{cid}: invalid maturity')
        if o.get('canon_status')=='CANON_APPROVED':
            ev=o.get('canon_approval_evidence');errors+=evidence_errors(ev,cid+' approval',root,owner=True)
            errors+=decision_binding_errors(ev,o,'CANON_APPROVED',root)
        if o.get('visual_use_status')=='SELECTED_FOR_PRODUCTION':
            ev=o.get('owner_visual_selection_evidence');errors+=evidence_errors(ev,cid+' visual selection',root,owner=True);errors+=decision_binding_errors(ev,o,'SELECTED_FOR_PRODUCTION',root)
        if o.get('canon_status')=='SUPERSEDED':
            if o.get('superseded_by') not in ids or o.get('superseded_by')==cid:errors.append(cid+': valid successor required')
            errors+=evidence_errors(o.get('supersession_evidence'),cid+' supersession',root,owner=True)
            errors+=decision_binding_errors(o.get('supersession_evidence'),o,'SUPERSEDED',root)
        if o.get('production_maturity')!='NOT_STARTED':
            errors+=evidence_errors(o.get('maturity_evidence'),cid+' maturity',root)
            if not o.get('production_asset_id'):errors.append(cid+': production ID required for stage advancement')
            if not isinstance(o.get('maturity_evidence'),dict) or o['maturity_evidence'].get('stage')!=o['production_maturity']:errors.append(cid+': stage evidence mismatch')
        parts=cid.split('-')
        if len(parts)>=5 and (parts[2]!=o.get('domain') or parts[3]!=o.get('object_type')):errors.append(f'{cid}: ID domain/type mismatch')
        pid=o.get('production_asset_id')
        if pid is not None and not re.fullmatch(r'EBS-[A-Z]{3}-[A-Z]{2,3}-\d{3,}',pid):errors.append(f'{cid}: invalid production ID')
        if not o.get('source_refs'):errors.append(f'{cid}: no visual provenance')
        for s in o.get('source_refs',[]):
            if s.get('source_id') not in source_ids:errors.append(f'{cid}: unknown source {s.get("source_id")}')
            if not s.get('locator'):errors.append(f'{cid}: missing object locator')
            box=s.get('bbox_normalized')
            if box is not None and (len(box)!=4 or not(0<=box[0]<box[2]<=1 and 0<=box[1]<box[3]<=1)):errors.append(f'{cid}: invalid bounding box')
        if o.get('gameplay_mapping',{}).get('status')=='SUPPORTED' and not o['gameplay_mapping'].get('evidence'):errors.append(f'{cid}: supported mapping lacks evidence')
        for r in o.get('source_refs',[]):
            matched=[f for f in sources['files']+sources['inline_graphics'] if f['source_id']==r.get('source_id') and f['path']==r.get('source_repository_path')]
            if not matched:errors.append(f'{cid}: source ID/path mismatch')
            elif r.get('source_sha256')!=matched[0]['sha256']:errors.append(f'{cid}: source hash mismatch')
        for ev in o.get('gameplay_mapping',{}).get('evidence',[]):
            errors+=evidence_errors(ev,cid+' mapping',root)
            entity=o.get('gameplay_mapping',{}).get('entity_id')
            if entity and ev.get('path') in {'Content/Data/Source/units.json','Content/Data/Source/buildings.json'} and ev.get('locator')!='id='+entity:errors.append(cid+': mapping locator/entity mismatch')
            if root and entity and ev.get('path') in {'Content/Data/Source/units.json','Content/Data/Source/buildings.json'}:
                file=pathlib.Path(root)/ev['path']
                if file.is_file():
                    data=load(file);records=data.get('units',data.get('buildings',[]))
                    if not any(r.get('id')==entity for r in records):errors.append(cid+': gameplay entity not found')
        for dep in o.get('dependencies',[]):
            if dep.startswith('EBS-CON-') and dep not in ids:errors.append(f'{cid}: missing dependency {dep}')
    decisions=register.get('decisions',[]);decision_ids=[x.get('decision_id') for x in decisions]
    if len(decision_ids)!=len(set(decision_ids)):errors.append('Duplicate decision IDs')
    for x in decisions:
        if not re.fullmatch(r'DEC-\d{3,}',str(x.get('decision_id',''))):errors.append('Invalid decision ID')
    for o in objs:
        if any(x not in decision_ids for x in o.get('open_decisions',[])):errors.append(o['concept_id']+': unknown decision reference')
    families=register.get('asset_families',[]);family_ids=[f.get('family_id') for f in families]
    if len(family_ids)!=len(set(family_ids)):errors.append('Duplicate family IDs')
    for f in families:
        fid=f.get('family_id');members=f.get('member_concept_ids',[])
        if not isinstance(fid,str) or not re.fullmatch(r'EBS-FAM-[A-Z]{3}-\d{3,}',fid):errors.append('Invalid family ID')
        if not members or len(members)!=len(set(members)) or any(m not in ids for m in members):errors.append(str(fid)+': invalid family members')
        for m in members:
            obj=next((o for o in objs if o['concept_id']==m),None)
            if obj and obj.get('asset_family_id')!=fid:errors.append(str(fid)+': member back-reference mismatch')
        if fid=='EBS-FAM-FWL-001':
            states={'Dormant','Harvest','Preserve','Reshape'}
            if set(f.get('states',[]))!=states:errors.append(fid+': missing Future Well states')
            depicted={o.get('state_changes',{}).get('depicted_state') for o in objs if o.get('asset_family_id')==fid and isinstance(o.get('state_changes'),dict)}
            if not states<=depicted:errors.append(fid+': missing state concept objects')
    for o in objs:
        if o.get('asset_family_id') and o['asset_family_id'] not in family_ids:errors.append(o['concept_id']+': unknown family')
    products=register.get('production_assets',[]);pids=[p.get('production_asset_id') for p in products]
    if len(pids)!=len(set(pids)):errors.append('Duplicate production IDs')
    for p in products:
        pid=p.get('production_asset_id');concepts=p.get('concept_ids',[])
        if not isinstance(pid,str) or not re.fullmatch(r'EBS-[A-Z]{3}-[A-Z]{2,3}-\d{3,}',pid):errors.append('Invalid production ID')
        if not concepts or any(c not in ids for c in concepts):errors.append(str(pid)+': invalid production concept links')
        errors+=evidence_errors(p.get('provenance'),str(pid)+' production provenance',root)
    for o in objs:
        if o.get('production_asset_id') and o['production_asset_id'] not in pids:errors.append(o['concept_id']+': production record missing')
    if root:
        req_text=(pathlib.Path(root)/'Docs/Requirements.md').read_text() if (pathlib.Path(root)/'Docs/Requirements.md').is_file() else ''
        for o in objs:
            for ev in o.get('gameplay_mapping',{}).get('evidence',[]):
                if ev.get('path')=='Docs/Requirements.md' and ev.get('locator','').startswith('SPEC-') and ev['locator'] not in req_text:errors.append(o['concept_id']+': missing requirement '+ev['locator'])
        for g in sources['inline_graphics']:
            p=pathlib.Path(root)/g['path']
            if not p.is_file() or digest(p.read_bytes())!=g['source_file_sha256']:errors.append('Inline source drift: '+g['path'])
        for f in sources['files']:
            p=pathlib.Path(root)/f['path']
            if not p.is_file() or digest(p.read_bytes())!=f['sha256']:errors.append('Source drift: '+f['path'])
    covered={r['source_repository_path'] for o in objs for r in o.get('source_refs',[])}
    uncovered=sorted({f['retained_original_path'] for f in sources['files'] if f['classification']=='CONCEPT_SOURCE'}-covered)
    mapped=[o for o in objs if o.get('gameplay_mapping',{}).get('status')=='SUPPORTED']
    return dict(author=AUTHOR,errors=errors,concept_sources_without_objects=uncovered,concept_objects=len(objs),mapped=len(mapped),unmapped=len(objs)-len(mapped),by_domain=dict(collections.Counter(o.get('domain','MISSING') for o in objs)),by_type=dict(collections.Counter(o.get('object_type','MISSING') for o in objs)),by_canon=dict(collections.Counter(o.get('canon_status','MISSING') for o in objs)),by_maturity=dict(collections.Counter(o.get('production_maturity','MISSING') for o in objs)),unmapped_ids=[o['concept_id'] for o in objs if o not in mapped],decision_backlog={o['concept_id']:o['open_decisions'] for o in objs if o.get('open_decisions')},production_without_provenance=[o['concept_id'] for o in objs if o.get('production_asset_id') and not o.get('source_refs')],scope='Register validation and source identity; no visual, gameplay, performance or rights acceptance.')

def main():
    p=argparse.ArgumentParser();sub=p.add_subparsers(dest='command',required=True)
    d=sub.add_parser('discover');d.add_argument('--root',required=True);d.add_argument('--history',action='store_true')
    c=sub.add_parser('crawl');c.add_argument('--base',default=BASE);c.add_argument('--limit',type=int,default=500)
    w=sub.add_parser('supplement');w.add_argument('--root',required=True)
    x=sub.add_parser('compare');x.add_argument('--before',required=True);x.add_argument('--after',required=True)
    v=sub.add_parser('validate');v.add_argument('--register',required=True);v.add_argument('--sources',required=True);v.add_argument('--root')
    a=p.parse_args()
    if a.command=='discover': result=discover(a.root,a.history)
    elif a.command=='crawl':result=crawl(a.base,a.limit)
    elif a.command=='supplement':result=supplement(a.root)
    elif a.command=='compare':result=compare(load(a.before),load(a.after))
    else:result=validate(load(a.register),load(a.sources),a.root)
    print(json.dumps(result,indent=2,ensure_ascii=False));return 1 if result.get('errors') else 0
if __name__=='__main__':sys.exit(main())
