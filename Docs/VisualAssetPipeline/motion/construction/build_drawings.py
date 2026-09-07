"""Author: Angelis Pseftis. Dimensionless construction-reference projections.
These simplified solids constrain topology; they are not approved asset models.
"""
import math,json,pathlib,html,hashlib
P=pathlib.Path(__file__).resolve().parent

def box(name,c,size,yaw=0):
 pts=[]
 for z in [-1,1]:
  for y in [-1,1]:
   for x in [-1,1]:
    a=x*size[0]/2;b=y*size[1]/2
    pts.append([c[0]+a*math.cos(yaw)-b*math.sin(yaw),c[1]+a*math.sin(yaw)+b*math.cos(yaw),c[2]+z*size[2]/2])
 return {'id':name,'vertices':pts,'edges':[[0,1],[0,2],[1,3],[2,3],[4,5],[4,6],[5,7],[6,7],[0,4],[1,5],[2,6],[3,7]]}
def beam(name,a,b,w=.12):
 # Wire prism with horizontal width; sufficient for these nonvertical segments.
 dx=b[0]-a[0];dy=b[1]-a[1];n=math.hypot(dx,dy);nx=-dy/n*w/2 if n else w/2;ny=dx/n*w/2 if n else 0
 pts=[]
 for p in [a,b]:
  for z in [-1,1]:
   for s in [-1,1]:pts.append([p[0]+s*nx,p[1]+s*ny,p[2]+z*w/2])
 return {'id':name,'vertices':pts,'edges':[[0,1],[0,2],[1,3],[2,3],[4,5],[4,6],[5,7],[6,7],[0,4],[1,5],[2,6],[3,7]]}
def make():
 all=[]
 a=[box('stationary_hub',[0,0,.9],[.8,.8,.4]),box('azimuth_bearing',[0,0,1.18],[.7,.7,.12]),box('elevation_cradle',[0,0,1.4],[.5,.55,.3])]
 for i in range(3):
  t=math.radians(90+120*i);v=(math.cos(t),math.sin(t));a+=[beam(f'leg_{i+1}',[v[0]*.4,v[1]*.4,.9],[v[0]*1.5,v[1]*1.5,.15],.22),box(f'foot_{i+1}',[v[0]*1.5,v[1]*1.5,.08],[.4,.4,.16])]
 for i,x in enumerate([-.2,.2]):a.append(box(f'emitter_{i+1}',[x,-.48,1.55],[.18,1.1,.22]))
 all.append(dict(subject='Aegis Post',components=a,invariants=['Three stationary support legs and three feet at 120-degree intervals.','All support attachments below azimuth bearing; two emitters on head.'],limits='Proposed support topology; detailed joint design, scale, rotation sweep and rig remain open.'))
 a=[box('floor',[0,0,.08],[2.4,5,.16]),box('rail',[0,0,.22],[.12,6,.08]),box('intake_apron',[0,-2.9,.1],[1.2,.8,.12]),box('output_apron',[0,2.9,.1],[1.2,.8,.12]),box('research_platform',[0,1.95,1.85],[1.7,.8,.12])]
 for end,y in [('intake',-2.4),('output',2.4)]:
  for side,x in [('left',-1),('right',1)]:a.append(box(end+'_'+side,[x,y,.95],[.22,.25,1.7]))
  a.append(box(end+'_lintel',[0,y,1.72],[2.2,.25,.18]))
 for x in [-1,1]:a.append(box('side_rail_'+str(x),[x,0,1.4],[.12,4.6,.12]))
 all.append(dict(subject='Array Foundry',components=a,invariants=['Intake, longitudinal rail and output have x=0 centerline.','Both end portals face the same axis; no side exit.'],limits='Reference proportions only. Authoritative 4x4 occupancy and real unit clearance not established.'))
 a=[]
 for i in range(6):
  t=math.radians(45+i*54)
  for j in range(2):
   r=2.1+j*.13;off=j*.12;x=r*math.cos(t)-off*math.sin(t);y=r*math.sin(t)+off*math.cos(t)
   a.append(box(f'pair_{i+1}_pane_{j+1}',[x,y,1.5],[.72,.035,3],t+math.pi/2))
 a.append(box('intake_pane',[2.65,0,.025],[.6,.8,.05]))
 all.append(dict(subject='Concordance',components=a,invariants=['Six candidate pairs, twelve persistent pane identities.','One 90-degree intake sector at +x; five 54-degree sectors elsewhere.'],limits='Count is delegated candidate choice, not book canon. Glass thickness, passage width and scale await production review.'))
 a=[]
 for axis in range(2):
  for i in range(16):
   ends=[]
   for j in [i,i+1]:
    t=math.pi*j/16;x=1.6*math.cos(t);z=(1.6+(.18 if axis else 0))*math.sin(t)
    ends.append([x,0,z] if axis==0 else [0,x,z])
   a.append(beam(f'span_{axis+1}_segment_{i+1}',*ends,.07))
 a.append(box('drop_pane',[0,0,.04],[.8,.8,.08]))
 all.append(dict(subject='Interval Loom',components=a,invariants=['Two complete spans in perpendicular planes.','Second crown is 0.18 reference units higher; positive crossing clearance.'],limits='These are geometric curves, not final glass forms; end supports and shader shadow alternatives need detailed design.'))
 a=[box('base',[0,0,.15],[1,1,.3]),box('shaft',[0,0,2.05],[.46,.46,3.5]),box('collar',[0,0,2.85],[.68,.68,.16])]
 for i in range(4):a.append(box(f'access_panel_{4-i:02}',[0,-.25,.75+i*.85],[.4,.035,.78]))
 for side,x in [('left',-.58),('right',.58)]:
  a.append(box('coupling_'+side,[x,0,.25],[.2,.32,.25]))
  for j,y in enumerate([-.08,.08]):a.append(box('conduit_'+side+str(j),[x*1.7,y,.12],[.6,.05,.05]))
 all.append(dict(subject='Power Link',components=a,invariants=['Four numbered access panels; maintenance removes 02 and 03.','Paired physical conduits on both sides; single collar assembly.'],limits='Simplified topology; panel clearances, exact base coupling positions and gameplay scale remain open.'))
 return all

def svg(asset):
 out=['<svg xmlns="http://www.w3.org/2000/svg" width="1500" height="960" viewBox="0 0 1500 960">','<metadata>Author: Angelis Pseftis. Procedural dimensionless construction reference.</metadata>','<rect width="1500" height="960" fill="#eeeae1"/>',f'<text x="45" y="62" font-family="sans-serif" font-size="32">{html.escape(asset["subject"])} — shared geometry reference</text>','<text x="45" y="98" font-family="sans-serif" font-size="18">DIMENSIONLESS • CANDIDATE TOPOLOGY • NOT A PRODUCTION MODEL</text>']
 for k,(label,axes) in enumerate([('FRONT: X / Z',(0,2)),('SIDE: Y / Z',(1,2)),('TOP: X / Y',(0,1))]):
  pts=[v for c in asset['components'] for v in c['vertices']];lo=[min(v[axis] for v in pts) for axis in axes];hi=[max(v[axis] for v in pts) for axis in axes];scale=min(410/(hi[0]-lo[0]+.1),510/(hi[1]-lo[1]+.1));cx=250+500*k;cy=425
  out.append(f'<text x="{cx-160}" y="155" font-family="sans-serif" font-size="20">{label}</text>')
  def proj(v):return (cx+(v[axes[0]]-(lo[0]+hi[0])/2)*scale,cy-(v[axes[1]]-(lo[1]+hi[1])/2)*scale)
  for c in asset['components']:
   color='#217e8b' if any(w in c['id'] for w in ['emitter','panel','rail','pane']) else '#424953'
   for i,j in c['edges']:
    a=proj(c['vertices'][i]);b=proj(c['vertices'][j]);out.append(f'<path d="M{a[0]:.2f},{a[1]:.2f} L{b[0]:.2f},{b[1]:.2f}" stroke="{color}" stroke-width="1.5" fill="none"/>')
 out.append('<path d="M45,735 H1455" stroke="#9d9b95"/>')
 for i,s in enumerate(asset['invariants']+[asset['limits']]):out.append(f'<text x="45" y="{775+i*35}" font-family="sans-serif" font-size="17">{html.escape(s)}</text>')
 out.append('<text x="45" y="920" font-family="sans-serif" font-size="16">Author: Angelis Pseftis • All views project the same vertices; view framing scales independently.</text></svg>');return '\n'.join(out)
if __name__=='__main__':
 assets=make();(P/'component-geometry.json').write_text(json.dumps({'author':'Angelis Pseftis','creator':'Angelis Pseftis','units':'dimensionless reference units','status':'CONSTRUCTION_REFERENCE_NOT_PRODUCTION','assets':assets},indent=2)+'\n')
 for a in assets:(P/(a['subject'].lower().replace(' ','-')+'.svg')).write_text(svg(a))
 print('5 shared-geometry drawings emitted; no Unreal assets')
