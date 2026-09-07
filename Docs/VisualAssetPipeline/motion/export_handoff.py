"""Author: Angelis Pseftis. Read-only source packaging; writes a new explicit archive only."""
import argparse,hashlib,json,pathlib,zipfile,datetime,subprocess

def git_info(repo,args):
 r=subprocess.run(['git','-C',str(repo)]+args,capture_output=True,text=True)
 return r.stdout.strip() if r.returncode==0 else 'NOT_A_GIT_REPOSITORY'
def digest(data):return hashlib.sha256(data).hexdigest()
def verify(path):
 with zipfile.ZipFile(path) as z:
  names=z.namelist()
  if len(names)!=len(set(names)):raise ValueError('Duplicate archive member')
  for name in names:
   p=pathlib.PurePosixPath(name)
   if p.is_absolute() or '..' in p.parts:raise ValueError('Unsafe archive member')
  m=json.loads(z.read('handoff-manifest.json'))
  expected={x['member'] for x in m['files']}|{'handoff-manifest.json'}
  if expected!=set(names):raise ValueError('Manifest coverage mismatch')
  for x in m['files']:
   data=z.read(x['member'])
   if digest(data)!=x['sha256'] or len(data)!=x['bytes']:raise ValueError('Archive payload changed')
  return dict(valid=True,files=len(m['files']),bytes=sum(x['bytes'] for x in m['files']))
def export(repo,evidence,output,source_root=None):
 repo=repo.resolve();evidence=evidence.resolve();output=output.resolve();docs=repo/'Docs/VisualAssetPipeline'
 if output.exists():raise ValueError('Refuse to overwrite existing archive')
 if output.is_relative_to(repo) or output.is_relative_to(evidence):raise ValueError('Output must be outside source trees')
 source_root=(source_root or repo).resolve()
 if output.is_relative_to(source_root):raise ValueError('Output must be outside visual source tree')
 files=[]
 inventory=json.loads((docs/'sources.json').read_text())['files']
 for x in inventory:
  f=source_root/x['path']
  files.append((f,'repository/'+x['path'],x.get('payload_sha256') or x['sha256'],'inventory source'))
 for root,prefix in [(docs,'pipeline'),(evidence,'evidence')]:
  for f in sorted(root.rglob('*')):
   if not f.is_file() or '__pycache__' in f.parts or f.name.startswith('.'):continue
   files.append((f,prefix+'/'+str(f.relative_to(root)),None,prefix))
 manifest=[]
 for f,member,expected,kind in files:
  if f.is_symlink() or not f.is_file():raise ValueError('Missing or symlink input: '+str(f))
  if kind=='inventory source' and not f.resolve().is_relative_to(source_root):raise ValueError('Source outside repo')
  data=f.read_bytes();sha=digest(data)
  if expected and sha!=expected:raise ValueError('Inventory source drift: '+str(f))
  manifest.append(dict(original_path=str(f),member=member,sha256=sha,bytes=len(data),kind=kind))
 output.parent.mkdir(parents=True,exist_ok=True)
 # Exclusive mode protects an archive created since the preflight check.
 owned_identity=None
 try:
  with output.open('xb') as handle:
   owned_identity=(output.stat().st_dev,output.stat().st_ino)
   with zipfile.ZipFile(handle,'w',compression=zipfile.ZIP_STORED) as z:
    for (f,member,_,_),m in zip(files,manifest):
     data=f.read_bytes()
     if digest(data)!=m['sha256']:raise ValueError('Input changed during packaging')
     z.writestr(member,data)
    z.writestr('handoff-manifest.json',json.dumps(dict(author='Angelis Pseftis',creator='Angelis Pseftis',created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),source_head=git_info(repo,['rev-parse','HEAD']),visual_source_root=str(source_root),visual_source_head=git_info(source_root,['rev-parse','HEAD']),visual_source_dirty_paths=git_info(source_root,['status','--short']),source_dirty_paths=git_info(repo,['status','--short']),boundary='Local preservation export; no rights clearance, off-device backup or asset acceptance. Historical absolute paths map to archive members here. Original documents remain authoritative.',files=manifest),indent=2)+'\n')
  result=verify(output)
 except BaseException:
  if owned_identity is not None and output.exists() and (output.stat().st_dev,output.stat().st_ino)==owned_identity:
   output.unlink()  # Only this attempt's partial archive, never an existing output or source.
  raise
 result.update(author='Angelis Pseftis',creator='Angelis Pseftis',archive=str(output),sha256=digest(output.read_bytes()),inventory_paths=len(inventory))
 return result
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--repo',type=pathlib.Path);ap.add_argument('--evidence',type=pathlib.Path);ap.add_argument('--output',type=pathlib.Path);ap.add_argument('--verify',type=pathlib.Path);ap.add_argument('--source-root',type=pathlib.Path,help='Optional read-only root holding hash-matched full image payloads');a=ap.parse_args()
 if a.verify:result=verify(a.verify)
 else:
  if not all([a.repo,a.evidence,a.output]):ap.error('repo, evidence and output required')
  result=export(a.repo,a.evidence,a.output,a.source_root)
 print(json.dumps(result,indent=2))
