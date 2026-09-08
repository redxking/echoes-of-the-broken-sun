"""Transfer matching polygon atlases by barycentric sampling. Author: Angelis Pseftis.
No geometry mutation. Normal vectors are transformed between per-triangle tangent frames.
"""
import json,sys,pathlib,hashlib
import numpy as np
from PIL import Image

def frame(pos,uv):
 e1,e2=pos[1]-pos[0],pos[2]-pos[0];d1,d2=uv[1]-uv[0],uv[2]-uv[0]
 n=np.cross(e1,e2);n/=np.linalg.norm(n)
 det=d1[0]*d2[1]-d1[1]*d2[0]
 t=(e1*d2[1]-e2*d1[1])/det;t-=n*np.dot(t,n);t/=np.linalg.norm(t)
 rawb=(e2*d1[0]-e1*d2[0])/det
 b=np.cross(n,t)*np.sign(np.dot(np.cross(n,t),rawb))
 return np.stack([t,b,n],axis=1)

def run(src,dst,out):
 out.mkdir(parents=True,exist_ok=True);source_size=2048;size=4096;output_size=2048
 maps={k:np.asarray(Image.open(src/'textures'/f'T_EBS_KHA_UNT_002_{k}.png').convert('RGB')) for k in ['BaseColor','Normal','MRE','StateMask']}
 targets={k:np.zeros((size,size,3),dtype=np.uint8) for k in maps};targets['Normal'][:]=[128,128,255];targets['MRE'][:]=[0,255,0]
 filled=np.zeros((size,size),bool);seen=set();count=0;degenerate_source_normals=0
 for f in sorted(src.glob('*_lod*.json')):
  if not (dst/f.name).exists():continue
  a=json.loads(f.read_text());b=json.loads((dst/f.name).read_text())
  for fa,fb in zip(a['faces'],b['faces']):
   ia,ib=fa['vertices'],fb['vertices']
   assert [a['vertices'][i] for i in ia]==[b['vertices'][i] for i in ib], 'Geometry differs'
   for j in range(1,len(ia)-1):
    ids=[0,j,j+1];u=np.array([a['uv'][ia[k]] for k in ids]);v=np.array([b['uv'][ib[k]] for k in ids]);pos=np.array([a['vertices'][ia[k]] for k in ids],float)
    key=tuple(np.round(v.flatten(),7))
    if key in seen:continue
    seen.add(key);p=v*size-.5
    lo=np.maximum(0,np.floor(p.min(0)).astype(int));hi=np.minimum(size-1,np.ceil(p.max(0)).astype(int))
    yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1];q=np.stack([xx-p[0,0],yy-p[0,1]],-1)
    mat=np.stack([p[1]-p[0],p[2]-p[0]],axis=1)
    if abs(np.linalg.det(mat))<1e-10:continue
    w=q@np.linalg.inv(mat).T;inside=(w[:,:,0]>=-1e-7)&(w[:,:,1]>=-1e-7)&(w.sum(2)<=1+1e-7)
    x,y=xx[inside],yy[inside]
    if not len(x):continue
    ww=w[inside];s=u[0]+ww[:,0,None]*(u[1]-u[0])+ww[:,1,None]*(u[2]-u[0]);sp=np.clip(np.floor(s*source_size).astype(int),0,source_size-1)
    for k,img in maps.items():
     vals=img[sp[:,1],sp[:,0]].copy()
     if k=='Normal':
      if abs(np.linalg.det(np.stack([u[1]-u[0],u[2]-u[0]])))<1e-12:
       vals[:]=[128,128,255];degenerate_source_normals+=1
      else:
       n=vals.astype(float)/127.5-1;n=n@frame(pos,u).T@frame(pos,v);n/=np.maximum(1e-10,np.linalg.norm(n,axis=1))[:,None];assert np.isfinite(n).all();vals=np.clip(np.rint((n+1)*127.5),0,255).astype(np.uint8)
     targets[k][y,x]=vals
    filled[y,x]=True;count+=1
 # Two-pixel dilation stays inside the four-pixel layout gutter.
 for _ in range(4):
  old=filled.copy()
  for dy,dx in [(0,1),(0,-1),(1,0),(-1,0)]:
   mask=np.roll(old,(dy,dx),(0,1))&~filled
   if dy==1:mask[0]=False
   if dy==-1:mask[-1]=False
   if dx==1:mask[:,0]=False
   if dx==-1:mask[:,-1]=False
   for k in targets:targets[k][mask]=np.roll(targets[k],(dy,dx),(0,1))[mask]
   filled|=mask
 targets={k:np.asarray(Image.fromarray(im).resize((output_size,output_size),Image.Resampling.BOX)).copy() for k,im in targets.items()}
 nn=targets['Normal'].astype(float)/127.5-1;nn/=np.maximum(1e-10,np.linalg.norm(nn,axis=2))[:,:,None];targets['Normal']=np.rint((nn+1)*127.5).astype(np.uint8)
 report={'author':'Angelis Pseftis','method':'matching polygon barycentric nearest sampling; normal retangent; 2x raster supersampling; 2px output dilation','degenerate_source_uv_normal_fallback_triangles':degenerate_source_normals,'triangles_rasterized':count,'covered_pixels':int(filled.sum()),'maps':{}}
 for k,im in targets.items():
  path=out/f'T_EBS_KHA_UNT_002_{k}.png';Image.fromarray(im).save(path)
  report['maps'][k]={'min':im.min((0,1)).tolist(),'max':im.max((0,1)).tolist(),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
 Image.fromarray(targets['StateMask']).resize((512,512),Image.Resampling.BOX).save(out/'T_EBS_KHA_UNT_002_MoltBlend.png')
 assert targets['MRE'][:,:,2].max()>100,'Lost emissive'
 assert targets['StateMask'][:,:,2].max()>200,'Lost ownership mask'
 assert targets['StateMask'][:,:,1].max()>200,'Lost core mask'
 assert np.ptp(targets['Normal'][:,:,0])>10,'Lost normal detail'
 (out/'transfer-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':run(*map(pathlib.Path,sys.argv[1:4]))
