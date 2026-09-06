#!/usr/bin/env python3
"""Local concept review board. Author: Angelis Pseftis.
Reads original artwork; writes only the explicit review-state file. Never changes canon.
"""
import argparse, datetime, hashlib, json, pathlib, secrets, threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit, parse_qs
HERE=pathlib.Path(__file__).resolve().parent
AUTHOR='Angelis Pseftis'
CHOICES={'KEEP','REWORK','REPLACE',None}
def sha(data):return hashlib.sha256(data).hexdigest()
def read(path):return json.loads(path.read_text())
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def initial(snapshot):return {'author':AUTHOR,'creator':AUTHOR,'schema_version':1,'snapshot_sha256':snapshot,'status':'REVIEW_DRAFT','revision':0,'created_utc':now(),'updated_utc':None,'selections':{},'family_notes':'','history':[],'boundary':'Keep/Rework/Replace choices are visual-direction review input, not canon acceptance, production authorization or asset changes.'}

def build_data(root):
    raw=(HERE/'concept-register.json').read_bytes();reg=json.loads(raw);assessment={x['source_id']:x for x in read(HERE/'visual-use-assessment.json')['assessments']};objs=[];images={};groups={}
    for o in reg['concept_objects']:
        if not ((o['object_type']=='UNT' and 'original sheet' in o['display_name']) or o['object_type']=='BLD' or o.get('asset_family_id')=='EBS-FAM-FWL-001'):continue
        s=o['source_refs'][0];path=(root/s['source_repository_path']).resolve()
        if not path.is_relative_to(root.resolve()):raise ValueError('Source path leaves checkout')
        data=path.read_bytes()
        if sha(data)!=s['source_sha256']:raise ValueError('Original source hash mismatch: '+s['source_repository_path'])
        images[s['source_id']]={'path':path,'sha256':s['source_sha256']}
        a=assessment[s['source_id']];group='future-well' if o.get('asset_family_id') else o['gameplay_mapping']['entity_id'];name='Future Well family' if group=='future-well' else o['gameplay_mapping']['display_name'];domain=o['domain'];label={'MER':'Meridian Compact','KHA':'Kharuun Assemblies','HOL':'Hollow Choir','FWL':'Future Well'}[domain]
        groups.setdefault(group,{'id':group,'name':name,'domain':domain,'label':label,'ids':[]})['ids'].append(o['concept_id'])
        suggestion='KEEP' if a['replacement_assessment']=='RETAIN_FOR_DESIGN_REVIEW' else 'REWORK'
        question='Which silhouette should define this role?'
        if o['object_type']=='BLD':question='Keep the architectural identity, or change its massing and proportions? Printed scale is unresolved.'
        if domain=='FWL':question='Does this view belong to one coherent four-state design? Keep states related; identify what must change.'
        objs.append({'id':o['concept_id'],'name':o['display_name'],'group':group,'domain':domain,'source_id':s['source_id'],'source_path':s['source_repository_path'],'source_sha256':s['source_sha256'],'locator':s['locator'],'box':s.get('bbox_normalized'),'description':o['visual_description'],'footprint':o['dimensions']['gameplay_footprint'],'role':o['gameplay_mapping'].get('role','stateful landmark'),'suggestion':suggestion,'assessment':a['observed_weakness_or_strength'],'retain':a['recommended_treatment'],'question':question})
    data={'author':AUTHOR,'snapshot_sha256':sha(raw),'objects':objs,'groups':list(groups.values()),'boundary':'Visual direction review only. No canon status, asset, or production stage changes.'}
    return data,images

class Store:
    def __init__(self,path,snapshot,objects):
        self.path=path;self.snapshot=snapshot;self.objects={x['id']:x for x in objects};self.lock=threading.Lock()
        if not path.exists():path.write_text(json.dumps(initial(snapshot),indent=2)+'\n')
        state=read(path)
        if state.get('snapshot_sha256')!=snapshot:raise ValueError('Review source changed; reconcile the existing review record before proceeding.')
    def get(self):
        with self.lock:return read(self.path)
    def update(self,body):
        if not isinstance(body,dict):raise ValueError('Request must be an object')
        with self.lock:
            state=read(self.path)
            if body.get('revision')!=state['revision']:raise RuntimeError('Another tab changed this review. Reload before saving.')
            if body.get('kind')=='family_notes':
                notes=body.get('notes')
                if not isinstance(notes,str) or len(notes)>6000:raise ValueError('Notes must be text, at most 6000 characters.')
                state['family_notes']=notes;event={'kind':'family_notes','notes':notes}
            else:
                cid=body.get('id');choice=body.get('choice');notes=body.get('notes','')
                if cid not in self.objects or choice not in CHOICES or not isinstance(notes,str) or len(notes)>6000:raise ValueError('Invalid concept, choice or notes.')
                o=self.objects[cid]
                record={'concept_id':cid,'source_sha256':o['source_sha256'],'choice':choice,'notes':notes,'updated_utc':now()}
                state['selections'][cid]=record;event={'kind':'concept','record':record.copy()}
            state['revision']+=1;state['updated_utc']=now();event.update(revision=state['revision'],time_utc=state['updated_utc']);state['history'].append(event)
            tmp=self.path.with_suffix('.json.tmp');tmp.write_text(json.dumps(state,indent=2,ensure_ascii=False)+'\n');tmp.replace(self.path)
            return state

def make_server(root,state_path,port):
    data,images=build_data(root);store=Store(state_path,data['snapshot_sha256'],data['objects']);token=secrets.token_urlsafe(32)
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def send(self,status,body,ctype='application/json'):
            if not isinstance(body,bytes):body=json.dumps(body).encode()
            self.send_response(status);self.send_header('Content-Type',ctype);self.send_header('Content-Length',str(len(body)));self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');self.send_header('Content-Security-Policy',"default-src 'self'; img-src 'self' data:; style-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'");self.end_headers();self.wfile.write(body)
        def host_ok(self):return self.headers.get('Host')==f'127.0.0.1:{self.server.server_port}'
        def do_GET(self):
            if not self.host_ok():return self.send(403,{'error':'Local host required'})
            u=urlsplit(self.path)
            if u.path=='/':return self.send(200,(HERE/'review-board.html').read_bytes(),'text/html; charset=utf-8')
            if u.path=='/api/review':return self.send(200,{'data':data,'state':store.get(),'token':token})
            if u.path=='/image':
                key=parse_qs(u.query).get('id',[''])[0];img=images.get(key)
                if not img:return self.send(404,{'error':'Unknown source'})
                raw=img['path'].read_bytes()
                if sha(raw)!=img['sha256']:return self.send(409,{'error':'Source changed since board started'})
                return self.send(200,raw,'image/png' if img['path'].suffix=='.png' else 'image/jpeg')
            return self.send(404,{'error':'Not found'})
        def do_POST(self):
            origin=f'http://127.0.0.1:{self.server.server_port}'
            if not self.host_ok() or self.headers.get('Origin')!=origin or self.headers.get('X-Review-Token')!=token:return self.send(403,{'error':'Local review session required'})
            if self.path!='/api/selection':return self.send(404,{'error':'Not found'})
            try:
                n=int(self.headers.get('Content-Length','0'))
                if not 0<n<=32000:raise ValueError('Invalid request size')
                body=json.loads(self.rfile.read(n));state=store.update(body);self.send(200,{'state':state})
            except RuntimeError as e:self.send(409,{'error':str(e)})
            except (ValueError,TypeError,KeyError) as e:self.send(400,{'error':str(e)})
    return ThreadingHTTPServer(('127.0.0.1',port),Handler)

def main():
    p=argparse.ArgumentParser();p.add_argument('--source-root',type=pathlib.Path,required=True);p.add_argument('--port',type=int,default=8846);p.add_argument('--state',type=pathlib.Path,default=HERE/'review-selections.json');a=p.parse_args()
    server=make_server(a.source_root,a.state,a.port);print(f'http://127.0.0.1:{server.server_port}',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()
if __name__=='__main__':main()
